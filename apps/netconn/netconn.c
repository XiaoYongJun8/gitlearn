/***************************************************************************
 * Copyright (C), 2016-2026, , All rights reserved.
 * File        :  netconn.c
 * Decription  :  联网模块
 * Author      :  zhums
 * Version     :  v1.0
 * Date        :  2022-11-07
 * Histroy	   :
 **************************************************************************/

/***********************************************************
* INCLUDES
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>

#include "watchdog.h"
#include "util.h"
#include "common.h"
#include "netconn.h"
#include "lte_network.h"
#include "eth_network.h"
#include "debug_and_log.h"
#include "config.h"

#define CHECK_NET_TIME       3
#define MAX_DIAL_FAIL_CNT    1440
#define DEBUG_LEVEL_FILE     "/home/root/debug/debug_netconn"
#define LOG_LEVEL_FILE       "/home/root/debug/log_netconn"
#define LOG_FILE             "/home/root/log/netconn.log"
#define LOG_MAX_SIZE         100000

app_log_t log_internet;

static int net_4g_state = NET_4G_DISCONNECTED;
static int last_net_4g_state = NET_4G_DISCONNECTED;
static int rssi_quality = SIGNAL_STRENGTH_NONE_OR_UNKNOWN;
static int last_rssi_quality = SIGNAL_STRENGTH_NONE_OR_UNKNOWN;

/*********************************************************************
* @fn	    resouce_clean
* @brief    进程资源回收
* @param    void
* @return   void
* @history:
**********************************************************************/
static void resource_clean(void)
{
    dbg_close();
    log_close(&log_internet);
    watchdog_close();
}

/*********************************************************************
* @fn	    sig_catch
* @brief    信号处理函数
* @param    sig_num[in]: 信号
* @return   void
* @history:
**********************************************************************/
static void sig_catch(int signum)
{
    int ret = 0;

    printf(">>>>connect internet process catches a signal [%d]\n", signum);

    switch(signum) {
    case SIGINT:
        resource_clean();
        exit(0);
    case SIGUSR1:
        ret = watchdog_close();
        if(ret != 0) {
            printf("soft_wdt: failed close\n");
        } else {
            printf("watchdog is closed!\n");
        }
        break;
    case SIGUSR2:
        break;

    default:
        break;
    }
}

/*********************************************************************
* @fn	    resource_init
* @brief    进程资源初始化
* @param    void
* @return   0: success -1: fail
* @history:
**********************************************************************/
static int resource_init(void)
{
    int ret = 0;

    ret = DBG_OPEN(DEBUG_LEVEL_FILE, LY_DEBUG_OFF);
    if(ret) {
        printf("dbg_open fail!\n");
        return -1;
    }

    ret = resource_init_common();
    if(ret) {
        printf("resource_init_common fail!\n");
        return -1;
    }

    ret = log_open(&log_internet, LOG_FILE, LOG_LEVEL_FILE, LOG_MAX_SIZE, LOG_LV_ERROR, LOGLV_MODE_LINEAR);
    if(ret) {
        printf("LOG_OPEN fail!\n");
        return -1;
    }
	LogMust("========================= connect internet process start =========================");

    print_core_limit();

    signal(SIGINT,  sig_catch);
    signal(SIGUSR1, sig_catch);
    signal(SIGUSR2, sig_catch);

    /* 从数据库读取软件看门狗超时时间 */

    ret = watchdog_open_ex(300, "connect_internet", 0);
    if(ret != 0) {
        printf("open watchdog fail\n");
        return -1;
    }

    return 0;
}

void connect_4g_loop(network_ops_t* pnet)
{
    static int conn_net_fail_cnt = 0;
    char has_log_success = 0;
    int ret = 0;
    int rssi = SIGNAL_STRENGTH_NULL;

    while(1) {
        // 检测4G信号强度
        ret = get_4g_signal_intensity(&rssi);
        if(ret) {
            LogError("get 4g signal intensity failed\n");
        } else {
            LogDebug("4g signal intensity: %d", rssi);
            if((rssi < 2) || (rssi == 99)) {
                rssi_quality = SIGNAL_STRENGTH_NONE_OR_UNKNOWN;
            } else if((rssi >= 2) && (rssi < 4)) {
                rssi_quality = SIGNAL_STRENGTH_POOR;
            } else if((rssi >= 4) && (rssi < 9)) {
                rssi_quality = SIGNAL_STRENGTH_MODERATE;
            } else if((rssi >= 9) && (rssi < 14)) {
                rssi_quality = SIGNAL_STRENGTH_GOOD;
            } else if((rssi >= 14) && (rssi <= 31)) {
                rssi_quality = SIGNAL_STRENGTH_GREAT;
            }
        }

        if(last_rssi_quality != rssi_quality) {
            LogInfo("4g rssi quality changed, set 4g rssi quality to %d.\n", rssi_quality);
            set_4g_rssi(rssi_quality);
            last_rssi_quality = rssi_quality;
        }

        //检测是否联网成功
        if(pnet->network_status()) {
            net_4g_state = NET_4G_DISCONNECTED;
            ++conn_net_fail_cnt;
            has_log_success = 0;
            if(conn_net_fail_cnt % 10 == 0) {
                LogInfo("network disconnected, try reconnect and max try times=%d, interval=%d", MAX_DIAL_FAIL_CNT, CHECK_NET_TIME);
            }

            if(conn_net_fail_cnt >= MAX_DIAL_FAIL_CNT) {
                conn_net_fail_cnt = 0;
                LogError("try reconnect network reach max times %d, need reinit connect internet process", MAX_DIAL_FAIL_CNT);
                break;
            }

            //联网操作
            if(pnet->network_connect()) {
                LogError("connect [%s] network failed: %s", pnet->name, pnet->errstr);
            }
            
        } else {
            net_4g_state = NET_4G_CONNECTED;
            conn_net_fail_cnt = 0;
            if(has_log_success == 0) {
                has_log_success = 1;
                LogMust("connect [%s] network success", pnet->name);
            }
        }

        if(last_net_4g_state != net_4g_state) {
            LogMust("4g net state changed, set 4g net state to %d.\n", net_4g_state);
            set_net_4g_status(net_4g_state);
            last_net_4g_state = net_4g_state;
        }

        safe_sleep(CHECK_NET_TIME, 0);
    }
}


void connect_eth_loop(network_ops_t* pnet)
{
    static int conn_eth_fail_cnt = 0;
    char has_log_success = 0;

    while(1) {
        //检测是否联网成功
        if(pnet->network_status()) {
            ++conn_eth_fail_cnt;
            has_log_success = 0;
            if(conn_eth_fail_cnt % 10 == 0) {
                LogInfo("network disconnected, try reconnect and max try times=%d, interval=%d", MAX_DIAL_FAIL_CNT, CHECK_NET_TIME);
            }

            if(conn_eth_fail_cnt >= MAX_DIAL_FAIL_CNT) {
                conn_eth_fail_cnt = 0;
                LogError("try reconnect network reach max times %d, need reinit connect internet process", MAX_DIAL_FAIL_CNT);
                break;
            }

            //联网操作
            if(pnet->network_connect()) {
                LogError("connect [%s] network failed: %s", pnet->name, pnet->errstr);
            }
            
        } else {
            conn_eth_fail_cnt = 0;
            if(has_log_success == 0) {
                has_log_success = 1;
                LogMust("connect [%s] network success", pnet->name);
            }
        }

        safe_sleep(CHECK_NET_TIME, 0);
    }
}

/*********************************************************************
* @fn	    main
* @brief    进程主函数
* @param    argc[in]: 参数数量
* @param    argv[in]: 参数列表
* @return   0: success -1: fail
* @history:
**********************************************************************/
int main(int argc, char* argv[])
{
    int ret = -1;
    int net_mode = NET_MODE_4G;
    network_ops_t* pnet = NULL;
    char net_mode_str[10] = {0};

    ret = resource_init();
    if(ret) {
        resource_clean();
        printf("resource init failed, clean resource\n");
        return -1;
    }

    ret = create_wdog_feed_thread(60);
    if(ret) {
        DBG_LOG_ERROR("create watchdog feed thread fail!\n");
        resource_clean();
        return -1;
    }

    config_read_str(CFG_FILE, "connect_internet", "net_mode", net_mode_str, 10, "0");
    net_mode = atoi(net_mode_str);
    LogMust("connect internet net mode: %d", net_mode);
    if((net_mode != 0) && (net_mode != 1)) {
        net_mode = 0;
        LogError("connect internet net mode error, set to 0");
    }

    if(net_mode == NET_MODE_4G)
        pnet = &lte_network;
    else if(net_mode == NET_MODE_LAN)
        pnet = &eth_network;
    else {
        LOG(LOG_LV_ERROR, "unknown net_mode[%d], use default 4G model\n", net_mode);
        pnet = &lte_network;
        net_mode = NET_MODE_4G;
    }
    LogInfo("connect internet process work with [%s] net mode", pnet->name);

    LogMust("init 4g net state to %d.\n", net_4g_state);
    set_net_4g_status(net_4g_state);
    set_4g_rssi(SIGNAL_STRENGTH_NONE_OR_UNKNOWN);

    while(1) {
        //联网前的初始化操作
        pnet->network_init();
        if(net_mode == NET_MODE_4G) {
            connect_4g_loop(pnet);
        } else {
            connect_eth_loop(pnet);
        }
    }
    
    return 0;
}
