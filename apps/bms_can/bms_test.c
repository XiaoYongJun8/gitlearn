/*******************************************************************************
 * @             Copyright (c) 2023 LinyPower, All Rights Reserved.
 * @*****************************************************************************
 * @FilePath     : bms_test.c
 * @Descripttion : 
 * @Author       : xiaoyongjun
 * @E-Mail       : xiaoyongjun@linygroup.cn
 * @Version      : v1.0.0
 * @Date         : 2023-11-14 08:48:14
*******************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <signal.h>


#include "msq.h"
#include "common.h"
#include "config.h"
#include "watchdog.h"
#include "database.h"
#include "debug_and_log.h"
#include "can_util.h"

#define VEMS_DEBUG_LEVEL_FILE     "/home/root/debug/debug_vems"
#define VEMS_LOG_LEVEL_FILE       "/home/root/debug/log_vems"
#define VEMS_LOG_FILE             "/home/root/log/vems.log"
#define VEMS_LOG_MAX_SIZE         2000000



#define  BATTERY_RUN_PARA01_CANID            CAN_ID_WRITE_DATA_REQ
#define  BATTERY_RUN_PARA02_CANID            CAN_ID_WRITE_DATA_REQ
#define  BATTERY_RUN_PARA06_CANID            CAN_ID_WRITE_DATA_REQ
#define  BATTERY_RUN_PARA07_CANID            CAN_ID_WRITE_DATA_REQ



typedef struct {
    uint8_t sys_ready : 1;              /*系统准备*/
    uint8_t charge_finish : 1;          /*充电完成*/
    uint8_t discharge_finish : 1;       /*放电完成*/
    uint8_t one_level_warn : 1;         /*一级警报*/
    uint8_t two_level_warn : 1;         /*二级警报*/
    uint8_t three_level_warn : 1;       /*三级警报*/
    uint8_t reserve1 : 1;               /*预留*/
    uint8_t reserve2 : 1;               /*预留*/
}system_status_t;



typedef struct {
    uint8_t temp_high : 1;              /*温度高*/
    uint8_t temp_low : 1;               /*温度低*/
    uint8_t temp_diff_big : 1;          /*温差大*/
    uint8_t tot_vol_high : 1;           /*总压高*/
    uint8_t tot_vol_low : 1;            /*总压低*/
    uint8_t cell_vol_high : 1;          /*单体电压高*/
    uint8_t cell_vol_low : 1;           /*单体电压低*/
    uint8_t cell_vol_diff_big : 1;      /*单体压差大*/
}alarm_flag1_t;                         /*一级/二级/三级报警标识1*/

typedef struct {
    uint8_t charge_curr_big : 1;        /*充电电流大*/
    uint8_t discharge_curr_big : 1;     /*放电电流大*/
    uint8_t rev1 : 1;                   /*预留*/
    uint8_t soc_low : 1;                /*soc低*/
    uint8_t jyzk_low : 1;               /*绝缘阻拦低*/
    uint8_t rev2 : 1;                   /*预留*/
    uint8_t rev3 : 1;                   /*预留*/
    uint8_t rev4 : 1;                   /*预留*/
}alarmL12_flag2_t;                      /*一级/二级报警标识2*/



typedef struct {
    uint8_t charge_curr_big : 1;        /*充电电流大*/
    uint8_t discharge_curr_big : 1;     /*放电电流大*/
    uint8_t rev1 : 1;                   /*预留*/
    uint8_t rev2 : 1;                   /*预留*/
    uint8_t rev3 : 1;                   /*预留*/
    uint8_t rev4 : 1;                   /*预留*/
    uint8_t ms_comm_fault : 1;          /*主从板通信故障*/
    uint8_t main_comm_fault : 1;        /*主通信故障*/
}alarmL3_flag2_t;                       /*三级保护标识2*/


typedef struct {
    float cell_voltage_max;             /*单体最大电压*/
    float cell_voltage_min;             /*单体最小电压*/
    uint8_t  soc;                       /*soc*/
    uint8_t  soh;                       /*soh*/
    uint8_t  km_state;                  /*总正继电器状态*/
    float voltage_total;                /*总电压*/
    float current_total;                /*总电流*/
    float charge_max_curr;              /*充电允许最大电流*/
    float discharge_max_curr;           /*放电允许最大电流*/
    uint8_t  bat_state;                 /*电池状态*/
    uint8_t  sys_state;                 /*系统状态标识*/
    uint8_t  alarm11;                   /*一级报警标识1*/
    uint8_t  alarm12;                   /*一级报警标识2*/
    uint8_t  alarm21;                   /*二级报警标识1*/
    uint8_t  alarm22;                   /*二级报警标识2*/
    uint8_t  alarm31;                   /*三级报警标识1*/
    uint8_t  alarm32;                   /*三级报警标识2*/

    uint8_t heart;                      /*心跳*/
    uint8_t pcs_status;                 /*PCS/HPS/PBD状态*/
    int16_t bat_power;                  /*电池功率*/
}bms_data_t;

app_log_t log_vems;
//config_vems_t g_cfg_vems;
//vems_t g_vems;
int g_qid_can;
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
    log_close(&log_vems);
    watchdog_close();
    db_close_database(DATABASE_PATH_DATA);

    //DestroyMessageQueue(g_qid_can);
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

    DBG_LOG_MUST(">>>>pcs ems process catches a signal [%d]\n", signum);

    switch(signum) {
    case SIGINT:
        resource_clean();
        exit(0);
    case SIGUSR1:
        ret = watchdog_close();
        if(ret != 0) {
            DBG_LOG_MUST("soft_wdt: failed close\n");
        } else {
            DBG_LOG_MUST("watchdog is closed!\n");
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

    ret = DBG_OPEN(VEMS_DEBUG_LEVEL_FILE, LY_DEBUG_OFF);
    if(ret) {
        printf("dbg_open fail!\n");
        return -1;
    }

    ret = resource_init_common();
    if(ret) {
        printf("resource_init_common fail!\n");
        return -1;
    }

    ret = log_open(&log_vems, VEMS_LOG_FILE, VEMS_LOG_LEVEL_FILE, VEMS_LOG_MAX_SIZE, LOG_LV_ERROR, LOGLV_MODE_LINEAR);
    if(ret) {
        printf("LOG_OPEN fail!\n");
        return -1;
    }

    print_core_limit();

    signal(SIGINT, sig_catch);
    signal(SIGUSR1, sig_catch);
    signal(SIGUSR2, sig_catch);

    //打开数据库
    ret = db_open_database(DATABASE_PATH_DATA);
    if(ret != 0) {
        printf("open database[%s] failed", DATABASE_PATH_DATA);
        return -1;
    }

    ret = watchdog_open_ex(300, "pcs_ems", 0);
    if(ret != 0) {
        printf("open watchdog fail\n");
        return -1;
    }

    g_qid_can = GetMessageQueueId(MQ_KEY_CAN);
    if(g_qid_can == -1) {
        DBG_LOG_ERROR("get message queue id[MQ_KEY_CAN=%x] fail: %s\n", MQ_KEY_CAN, strerror(errno));
        return -1;
    }

    return 0;
}

/*******************************************************************************
 * @brief 通过can发送bms和投切控制器的数据
 * @param can_sock can_sock
 * @param canid    can报文id
 * @param pbuff    发送Can数据缓存区
 * @return 发送成功：返回0  发送失败：返回 -1
*******************************************************************************/
int bms_write(int can_sock, uint32_t canid, uint8_t* pbuff)
{
    int ret = 0;
    struct can_frame frame;

    frame.can_id = canid;
    frame.data[0] = pbuff[0];
    frame.data[1] = pbuff[1];
    frame.data[2] = pbuff[2];
    frame.data[3] = pbuff[3];
    frame.data[4] = pbuff[4];
    frame.data[5] = pbuff[5];
    frame.data[6] = pbuff[6];
    frame.data[7] = pbuff[7];
    frame.can_dlc = 8;
    ret = can_write(can_sock, (uint8_t*)&frame, sizeof(frame));
    if(ret == -1) {
        LogDebug("bsm  failed:%s", strerror(errno));
        return -1;
    } else {
        LogDebug("write can_id [%x] data 0-7 sucess: data[0]:%x data[1]:%x data[2]:%x data[3]:%x data[4]:%x data[5]:%x data[6]: %x data[7]:%x\n", canid, frame.data[0], frame.data[1], frame.data[2], frame.data[3], frame.data[4], frame.data[5], frame.data[6], frame.data[7]);
    }
    return 0;
}

void can_send_run_para01(int can_sock)
{
    uint8_t wbuff[8] = { 0 };
    uint16_t cell_voltage_max = 0x1234;
    uint16_t cell_voltage_min = 0x5678;
    uint8_t soc = 0x88;
    uint8_t soh = 0x88;
    uint8_t km_state = 1;

    wbuff[0] = cell_voltage_max & 0xff;
    wbuff[1] = (cell_voltage_max >> 8) & 0xff;

    wbuff[2] = cell_voltage_min & 0xff;
    wbuff[3] = (cell_voltage_min >> 8) & 0xff;

    wbuff[4] = soc;
    wbuff[5] = soh;

    wbuff[6] = 0;
    wbuff[7] = km_state;

    bms_write(can_sock, BATTERY_RUN_PARA01_CANID, wbuff);
}



void can_send_run_para02(int can_sock)
{

    uint8_t wbuff[8] = { 0 };
    uint16_t voltage_total = 0x1234;
    uint16_t current_total = 0x5678;
    uint16_t charge_max_curr = 0x9abc;
    uint16_t discharge_max_curr = 0xefff;

    wbuff[0] = voltage_total & 0xff;
    wbuff[1] = (voltage_total >> 8) & 0xff;

    wbuff[2] = current_total & 0xff;
    wbuff[3] = (current_total >> 8) & 0xff;

    wbuff[4] = charge_max_curr & 0xff;
    wbuff[5] = (charge_max_curr >> 8) & 0xff;

    wbuff[6] = discharge_max_curr & 0xff;
    wbuff[7] = (discharge_max_curr >> 8) & 0xff;

    bms_write(can_sock, BATTERY_RUN_PARA02_CANID, wbuff);

}

void can_send_run_para06(int can_sock)
{

    bms_data_t rtdata;
    uint8_t wbuff[8] = { 0 };

    system_status_t sys_status;
    alarm_flag1_t  alarm_flag1;
    alarmL12_flag2_t alarm_flag2;

    rtdata.bat_state = 1;
    wbuff[0] = rtdata.bat_state;

    sys_status.sys_ready = 1;
    sys_status.charge_finish = 1;
    sys_status.discharge_finish = 1;
    sys_status.one_level_warn = 1;
    sys_status.two_level_warn = 1;
    sys_status.three_level_warn = 1;
    sys_status.reserve1 = 0;
    sys_status.reserve2 = 0;

    memcpy(&wbuff[1], &sys_status, 1);

    alarm_flag1.temp_high = 1;
    alarm_flag1.temp_low = 1;
    alarm_flag1.temp_diff_big = 1;
    alarm_flag1.tot_vol_high = 1;
    alarm_flag1.tot_vol_low = 1;
    alarm_flag1.cell_vol_high = 1;
    alarm_flag1.cell_vol_low = 1;
    alarm_flag1.cell_vol_diff_big = 1;

    memcpy(&wbuff[2], &alarm_flag1, 1);


    alarm_flag2.charge_curr_big = 1;
    alarm_flag2.discharge_curr_big = 1;
    alarm_flag2.rev1 = 1;
    alarm_flag2.soc_low = 1;
    alarm_flag2.jyzk_low = 1;
    alarm_flag2.rev2 = 0;
    alarm_flag2.rev3 = 0;
    alarm_flag2.rev4 = 0;

    memcpy(&wbuff[3], &alarm_flag2, 1);

    wbuff[4] = 0;
    wbuff[5] = 0;

    wbuff[6] = 0;
    wbuff[7] = 0;

    bms_write(can_sock, BATTERY_RUN_PARA06_CANID, wbuff);

}


void can_send_run_para07(int can_sock)
{

    uint8_t wbuff[8] = { 0 };
    
    alarm_flag1_t alarm12_flag1;
    alarmL12_flag2_t alarm12_flag2;
    alarm_flag1_t alarm3_flag1;
    alarmL3_flag2_t alarm3_flag2;

    alarm12_flag1.temp_high = 1;
    alarm12_flag1.temp_low = 1;
    alarm12_flag1.temp_diff_big = 1;
    alarm12_flag1.tot_vol_high = 1;
    alarm12_flag1.tot_vol_low = 1;
    alarm12_flag1.cell_vol_high = 1;
    alarm12_flag1.cell_vol_low = 1;
    alarm12_flag1.cell_vol_diff_big = 1;
    memcpy(&wbuff[0], &alarm12_flag1, 1);



    alarm12_flag2.charge_curr_big = 1;
    alarm12_flag2.discharge_curr_big = 1;
    alarm12_flag2.rev1 = 1;
    alarm12_flag2.soc_low = 1;
    alarm12_flag2.jyzk_low = 1;
    alarm12_flag2.rev2 = 0;
    alarm12_flag2.rev3 = 0;
    alarm12_flag2.rev4 = 0;
    memcpy(&wbuff[1], &alarm12_flag2, 1);




    alarm3_flag1.temp_high = 1;
    alarm3_flag1.temp_low = 1;
    alarm3_flag1.temp_diff_big = 1;
    alarm3_flag1.tot_vol_high = 1;
    alarm3_flag1.tot_vol_low = 1;
    alarm3_flag1.cell_vol_high = 1;
    alarm3_flag1.cell_vol_low = 1;
    alarm3_flag1.cell_vol_diff_big = 1;
    memcpy(&wbuff[2], &alarm3_flag1, 1);





    alarm3_flag2.charge_curr_big = 1;
    alarm3_flag2.discharge_curr_big = 1;
    alarm3_flag2.rev1 = 1;
    alarm3_flag2.rev2 = 0;
    alarm3_flag2.rev3 = 0;
    alarm3_flag2.rev4 = 0;
    alarm3_flag2.main_comm_fault = 1;
    alarm3_flag2.ms_comm_fault = 1;
    memcpy(&wbuff[3], &alarm3_flag2, 1);

    wbuff[4] = 0;
    wbuff[5] = 0;

    wbuff[6] = 0;
    wbuff[7] = 0;

    bms_write(can_sock, BATTERY_RUN_PARA07_CANID, wbuff);

}




int main(int argc, char* argv[])
{

    int ret = -1;

    int can_sock = -1;
   
    struct can_filter filters[4] = { {CAN_ID_READ_DATA_REQ, CAN_SFF_MASK}, {CAN_ID_READ_DATA_RSP, CAN_SFF_MASK}
                                   , {CAN_ID_WRITE_DATA_REQ, CAN_SFF_MASK}, {CAN_ID_WRITE_DATA_RSP, CAN_SFF_MASK} };
 

    ret = resource_init();

    if(ret) {
        printf("resource init failed, clean resource\n");
        ret = -1;
        goto proc_exit;
    }
    LogMust("vems resource init success");
    LogMust("read vems configs");


    do {
        can_sock = can_init(250000, (struct can_filter**)&filters, ARRAR_SIZE(filters));
        LogDebug("can_init failed: %s", strerror(errno));
        safe_msleep(500);
    } while(can_sock <= 0);
    /*物理值 = 总线值*系数+偏移系数 */
    LogInfo("bsm can channel is ready, start working\n");
    

    

    while(1) {
        can_send_run_para01(can_sock);
        can_send_run_para02(can_sock);
        can_send_run_para06(can_sock);
        can_send_run_para07(can_sock);
        sleep(1);
    }

proc_exit:
    LogMust("vems process exit");
    resource_clean();

    return ret;

}