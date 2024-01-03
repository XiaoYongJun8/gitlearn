/***************************************************************************
 * Copyright (C), 2016-2026, , All rights reserved.
 * File        :  lte_network.c
 * Decription  :  联网模块
 * Author      :  zhums
 * Version     :  v1.0
 * Date        :  2021/01/08
 * Histroy	   :
 **************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "lte_network.h"
#include "netconn.h"
#include "debug_and_log.h"
#include "common.h"
//#include "util.h"


#define QUECTELCM_PATH          "/usr/sbin/quectel-CM"
#define QUECTELCM_LOG_FILE      "/usr/sbin/quectel-CM.log"
#define QL_USB_AT_PORT          "/dev/ttyUSB2"
#define HIGH_LEVEL      1
#define LOW_LEVEL       0

static int fd_at = -1;

static int lte_network_status();

int check_open_usb_at_port()
{
    if(fd_at > 0) {
        return 0;
    }
    
	fd_at = at_cmd_open(QL_USB_AT_PORT, O_RDWR | O_NONBLOCK | O_NOCTTY);
    return (fd_at == -1 ? 1 : 0);
}

/*********************************************************************
* @fn	    get_4g_signal_intensity
* @brief    检测4G信号强度
* @param    rssi[out]: 信号强度
* @return   -1: 失败 0: 成功
* @history:
**********************************************************************/
int get_4g_signal_intensity(int *rssi)
{
	int value = 0;
	char signal_buf[110] = {0};
	int ret = 0;

    if(check_open_usb_at_port() != 0) {
        return -1;
    }

	ret = at_cmd_send(fd_at, "AT+CSQ", "OK", signal_buf, sizeof(signal_buf), 2000);
	if(0 != ret ) {
        LogWarn("at_cmd_send failed return=%d", ret);
		return -1;
	}
    at_cmd_close(fd_at);
    fd_at = -1;

	char* p = strstr(signal_buf, "+CSQ: ");
	value = atoi(p+6);

	if (value != SIGNAL_STRENGTH_NULL) {
		if(value >= SIGNAL_STRENGTH_MIN && value <= SIGNAL_STRENGTH_MAX) {
			*rssi = value;
		} else {
            LogError("invalid at signal value: %d", value);
            return -1;
        }
	} else {
        *rssi = value;
    }
	return 0;
}

int get_sim_card_insert_status()
{
	int ret = 0;
    char* ptmp = NULL;
    char buff[110] = {0};
    char sim_insert = 1;    //SIM卡是否插上，默认已插卡
    //char op_ident = 0;      //SIM卡所属运营商是否识别

    if(check_open_usb_at_port() != 0) {
        return sim_insert;
    }

	ret = at_cmd_send(fd_at, "AT+QSIMSTAT?", "OK", buff, sizeof(buff), 5000);
    at_cmd_close(fd_at); 
    fd_at = -1;
    LogTrace("AT+QSIMSTAT? return[%d], buff[%s]", ret, buff);
    ptmp = strstr(buff, "+QSIMSTAT: 0,");
    if(ptmp == 0) {
        LogDebug("at rsp[%s] not contains QSIMSTAT info, set sim insert status to default", buff);
        return sim_insert;
    }
    sim_insert = atoi(ptmp + 13);
    LogTrace("SIM card is %s", sim_insert == 0 ? "uninsert" : "insert");
    if(sim_insert == 0)
        return sim_insert;
    // safe_sleep(0, 500000);

	// ret = at_cmd_send(fd_at, "AT+COPS?", "OK", buff, sizeof(buff), 2000);
    // LogInfo("AT+COPS? return[%d], buff[%s]", ret, buff);

    return 1;
}

//EC20启动后会注册wwan0网络设备，但此时是down状态。
//通过检查wwan0是否已注册来判断EC20 boot or reset 是否完成
int get_wwan0_device_register_status()
{
    int is_reg = 0; //0:has not register; 1:already register
    FILE *fp = NULL;
	char *p = NULL;

	char buf[100];
    fp = popen("ifconfig wwan0 2>/dev/null", "r"); //wwan0未注册时会报错:ifconfig: wwan0: error fetching interface information: Device not found
    if(!fp) {
        LogError("popen(ifconfig wwan0 2>/dev/null) failed: %s", strerror(errno));
        return is_reg;
    }

    while(memset(buf, 0, sizeof(buf)), fgets(buf, sizeof(buf) - 1, fp) != 0) {
        //LogDebug("read buf[%s]", buf);
    	p = strstr(buf, "wwan0");
        if(p != NULL)  {
            is_reg = 1;
            break;
        }
    }
    pclose(fp);

	return is_reg;
}

//判断原理： ec20 boot前/dev/下无ttyUSB0-3,boot成功后会出现ttyUSB0-3
int get_ec20_boot_status()
{
	int is_boot = 0; //0:has not boot; 1:already boot
    FILE *fp = NULL;
	char *p = NULL;

	char buf[100];
    fp = popen("ls /dev/ttyUSB[0-3] 2>/dev/null", "r");
    if(!fp) {
        LogError("popen(ls /dev/ttyUSB[0-3] 2>/dev/null) failed: %s", strerror(errno));
        return is_boot;
    }

    while(memset(buf, 0, sizeof(buf)), fgets(buf, sizeof(buf) - 1, fp) != 0) {
        LogTrace("read cmd[ls /dev/ttyUSB[0-3]] output[%s]", buf);
    	p = strstr(buf, "ttyUSB2");
        if(p != NULL)  {
            is_boot = 1;
            LogTrace("ec20 at port[ttyUSB2] is appear, ec20 now has power on");
            break;
        }
    }
    pclose(fp);

	return is_boot;
}

void wait_ec20_boot_or_reset_done(int timeout, int pre_wait_sec)
{
    int i = 0;

    if(pre_wait_sec > 0) {
        safe_sleep(pre_wait_sec, 0);
        timeout -= pre_wait_sec;
    }
    
    while (i++ < timeout)
    {
        if(get_wwan0_device_register_status() == 1)
            break;

        safe_sleep(1, 0);
    }
}

void wait_connect_done(int timeout, int pre_wait_sec)
{
    int i = 0;

    if(pre_wait_sec > 0) {
        safe_sleep(pre_wait_sec, 0);
        timeout -= pre_wait_sec;
    }
    
    while (i < timeout)
    {
        if(lte_network_status() == 0)
            break;

        i += 3;
        safe_sleep(3, 0);
    }
}

int ec20_boot(int want_power_on)
{
    char dev_path[] = "/sys/class/leds/ec20-powerkey/brightness";
    char cmd[128] = {0};
    int sleep_ms = 900;
    int status = 0;
    int has_boot = 0;

    has_boot = get_ec20_boot_status();
    if(want_power_on == has_boot)  {
        LogInfo("ec20 already power %s, no need do anything", has_boot ? "on" : "off");
        return 0;
    }
    
    LogInfo("set ec20 power %s", has_boot ? "off" : "on");
    LogInfo("pull up ec20 powerkey and keep %dms", sleep_ms);
    snprintf(cmd, sizeof(cmd), "echo %d > %s", HIGH_LEVEL, dev_path);
    if(do_system_ex(cmd, &status, 0) != 0)
    {
        LogError("pull up ec20 powerkey failed, status=%d", status);
        return -1;
    }
    safe_sleep(0, sleep_ms * 1000);
    
    LogInfo("pull down ec20 powerkey and wait ec20 boot done");
    snprintf(cmd, sizeof(cmd), "echo %d > %s", LOW_LEVEL, dev_path);
    if(do_system_ex(cmd, &status, 0) != 0)
    {
        LogError("pull down ec20 powerkey failed, status=%d", status);
        return -1;
    }

    int pre_wait_sec = has_boot ? 25 : 8;
    wait_ec20_boot_or_reset_done(25, pre_wait_sec); // wait ec20 boot done
    LogInfo("ec20 boot done");
    
    return 0;
}

int ec20_reset()
{
    char dev_path[] = "/sys/class/leds/ec20-resetn/brightness";
    char cmd[128] = {0};
    int sleep_ms = 600;
    int status = 0;
    
    LogInfo("pull up ec20 resetkey and keep %dms", sleep_ms);
    snprintf(cmd, sizeof(cmd), "echo %d > %s", HIGH_LEVEL, dev_path);
    if(do_system_ex(cmd, &status, 0) != 0)
    {
        LogError("pull up ec20 reset failed, status=%d", status);
        return -1;
    }
    safe_sleep(0, sleep_ms * 1000);
    
    LogInfo("pull down ec20 resetkey and wait ec20 reset done");
    snprintf(cmd, sizeof(cmd), "echo %d > %s", LOW_LEVEL, dev_path);
    if(do_system_ex(cmd, &status, 0) != 0)
    {
        LogError("pull down ec20 reset failed, status=%d", status);
        return -1;
    }
    wait_ec20_boot_or_reset_done(30, 5); // wait ec20 reset done
    LogInfo("ec20 reset done");
    
    return 0;
}

/*********************************************************************
* @fn	    lte_network_init
* @brief    lte网络初始化
* @param    void
* @return   void
* @history:
**********************************************************************/
static void lte_network_init(void)
{
    LogInfo("start lte network init");
    if(get_ec20_boot_status() == 0) {
        LogInfo("in lte_network_init, ec20 in power off, need power on ec20");
        ec20_boot(1);
    } else {
        LogInfo("in lte_network_init, ec20 already power on, no need do anything");
    }
    LogInfo("lte network init done");
}


/*********************************************************************
* @fn	    lte_network_status
* @brief    lte检查网络状态 todo: 应该用回调函数判断是否断网，ifconfig不能及时判断是否断网
* @param    void
* @return   0: 联网 -1: 断网
* @history:
**********************************************************************/
static int lte_network_status()
{
    /* todo: 解决ifconfig不能及时判断是否断网的问题 */
	FILE *fp = NULL;
	char *p = NULL;
    int has_wwan0 = 0;

	char buf[100];
    fp = popen("ifconfig", "r");
    if(!fp) {
        LogError("popen(ifconfig) failed: %s", strerror(errno));
        return -1;
    }

    while(memset(buf, 0, sizeof(buf)), fgets(buf, sizeof(buf) - 1, fp) != 0) {
        //LogDebug("read buf[%s], has_wwan0=%d", buf, has_wwan0);
    	if(has_wwan0 == 0) {
            p = strstr(buf, "wwan0");
            if(p!=NULL)  {
                has_wwan0 = 1;
            }
        } else {
            p = strstr(buf, "inet addr:");
            if(p!=NULL)  {
                pclose(fp);
                return 0;
            }
        }
    }
    pclose(fp);
	
    return -1;
}

/*********************************************************************
* @fn	    lte_network_connect
* @brief    lte拨号
* @param    void
* @return   0: 拨号成功 -1: 拨号失败
* @history:
**********************************************************************/
static int lte_network_connect(void)
{
    static char is_reconnect = 0;
    char cmd[256] = {0};
    int status = 0;

    if(get_ec20_boot_status() == 0) {
        DBG_LOG_INFO("in lte_network_connect, check not found usb attach, need power on ec20");
        ec20_boot(1);
    } else if(get_sim_card_insert_status() != 1) {
        lte_network.errstr = "SIM card is uninsert";
        safe_sleep(3, 0);
        return -1;
    } else if(is_reconnect) {
        ec20_reset();
    } else {
        is_reconnect = 1;
    }

    snprintf(cmd, sizeof(cmd), "killall quectel-CM 2>/dev/null; echo "" > %s; %s -f %s 1>/dev/null 2>&1 &", QUECTELCM_LOG_FILE, QUECTELCM_PATH, QUECTELCM_LOG_FILE);
    LogInfo("run quectel-CM to connect 4g network");
    if(do_system_ex(cmd, &status, 0) != 0)
    {
        //LogError("run[%s] failed, status=%d", cmd, status);
        lte_network.errstr = "quectel-CM run failed";
        return -1;
    }
    wait_connect_done(30, 15);  //wait quectel-CM success

    return 0;
}

/*********************************************************************
* @fn	    lte_network_disconnect
* @brief    lte断网
* @param    void
* @return   0: 断网成功 -1: 断网失败
* @history:
**********************************************************************/
static int lte_network_disconnect(void)
{
    at_cmd_close(fd_at);

    return 0;
}


struct network_ops lte_network = {
    .name = "4G",
    .errstr = "success",
    .network_init = lte_network_init,
    .network_connect = lte_network_connect,
    .network_disconnect = lte_network_disconnect,
    .network_status = lte_network_status
};

