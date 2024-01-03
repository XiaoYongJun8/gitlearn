/*******************************************************************************
 * @             Copyright (c) 2023 LinyPower, All Rights Reserved.
 * @*****************************************************************************
 * @FilePath     : load_power_collect.c
 * @Descripttion : 
 * @Author       : zhums
 * @E-Mail       : zhums@linygroup.cn
 * @Version      : v1.0.0
 * @Date         : 2023-11-02 11:06:38
*******************************************************************************/
#include <string.h>
#include <pthread.h>

#include "load_power_collect.h"
#include "debug_and_log.h"
#include "common.h"
#include "vems.h"

/*******************************************************************************
 * @brief 初始化内存块：main函数调用
 * @return
*******************************************************************************/
void emeter_data_collect_init(elec_meter_t* pemeter)
{
    pemeter->ctx = NULL;
    pemeter->def_rsp_tmov.tv_sec = 1;
    pemeter->def_rsp_tmov.tv_usec = 0;
    pemeter->rsp_tmov.tv_sec = 1;
    pemeter->rsp_tmov.tv_usec = 0;
    pemeter->state = ITEM_UNUSE;
    pemeter->dev_port = g_vems.cfg.emeter_port;
    strcpy(pemeter->dev_ip, g_vems.cfg.emeter_ipaddr);
    memset(g_vems.elec_meter.rdata, 0, sizeof(g_vems.elec_meter.rdata));
    pemeter->state = ITEM_USED;
}


/*******************************************************************************
 * @brief 电表数据采集线程
 * @param
 * @return
*******************************************************************************/
static void* emeter_comm_thread_do(void* arg)
{
    int ret = 0;
    elec_meter_t* pemeter = (elec_meter_t*)arg;

    emeter_data_collect_init(pemeter);
    while(1) {
        pemeter->ctx = modbus_new_tcp(pemeter->dev_ip, pemeter->dev_port);
        //pemeter->ctx = modbus_new_tcp("192.168.225.7", 8234);
        if(pemeter->ctx != NULL) {
            LogInfo("modbus_new_tcp(%s, %d) success", pemeter->dev_ip, pemeter->dev_port);
            break;
        } else {
            LogDebug("modbus_new_rtu(%s, %d) failed", pemeter->dev_ip, pemeter->dev_port);
        }
        safe_msleep(1000);
    }
    modbus_set_debug(pemeter->ctx, g_vems.cfg.emeter_mb_debug);
    modbus_set_slave(pemeter->ctx, g_vems.cfg.emeter_slave_id);
    modbus_set_response_timeout(pemeter->ctx, &pemeter->def_rsp_tmov);
    modbus_set_error_recovery(pemeter->ctx, MODBUS_ERROR_RECOVERY_LINK | MODBUS_ERROR_RECOVERY_PROTOCOL);

    while(-1 == modbus_connect(pemeter->ctx)) {
        LogDebug("modbus_connect failed: %s", modbus_strerror(errno));
        safe_msleep(1000);
    }
    g_vems.th_stat.bits.th_emeter_comm = 1;
    LogInfo("load power collect thread modbus-tcp channel is ready, start working");

    while(1) {
        ret = modbus_read_input_registers(pemeter->ctx, GWC_INPUT_REG_ADDR_START, GWC_RDATA_NUM, pemeter->rdata);
        if(ret <= 0) {
            LogEMANA("modbus_read_registers[%d, %d] failed ret=%d", GWC_INPUT_REG_ADDR_START, GWC_RDATA_NUM, ret);
            safe_msleep(1000);
            continue;
        }
        g_vems.elec_meter.load_power_real
            = ((g_vems.elec_meter.rdata[0] * GWC_DATA_RATIO * g_vems.elec_meter.rdata[3] * GWC_DATA_RATIO)
            + (g_vems.elec_meter.rdata[1] * GWC_DATA_RATIO * g_vems.elec_meter.rdata[4] * GWC_DATA_RATIO)
            + (g_vems.elec_meter.rdata[2] * GWC_DATA_RATIO * g_vems.elec_meter.rdata[5] * GWC_DATA_RATIO)) / 1000;
        //g_vems.elec_meter.load_power_pre = g_vems.elec_meter.load_power;  //能量管理模块在功率分配完成后更新
        g_vems.elec_meter.load_power = (int)(g_vems.elec_meter.load_power_real * COMM_DATA_MULTIPLE);

        // LogEMANA("Ua=%.2fV Ub=%.2fV Uc=%.2fV Ia=%.2fA Ib=%.2fA Ic=%.2fA, load_power=%.1fkW", g_vems.elec_meter.rdata[0] * GWC_DATA_RATIO
        //          , g_vems.elec_meter.rdata[1] * GWC_DATA_RATIO, g_vems.elec_meter.rdata[2] * GWC_DATA_RATIO, g_vems.elec_meter.rdata[3] * GWC_DATA_RATIO
        //          , g_vems.elec_meter.rdata[4] * GWC_DATA_RATIO, g_vems.elec_meter.rdata[5] * GWC_DATA_RATIO, g_vems.elec_meter.load_power_real);

        safe_msleep(1000);
    }

    return arg;
}

/*******************************************************************************
 * @brief  创建modbus工作线程：main 函数调用
 * @return 获取成功：0，失败：其他
*******************************************************************************/
int create_emeter_comm_thread(elec_meter_t* pemeter)
{
    int ret = -1;
    pthread_t emeter_tid;
    pthread_attr_t emeter_tattr;

    pthread_attr_init(&emeter_tattr);
    pthread_attr_setdetachstate(&emeter_tattr, PTHREAD_CREATE_DETACHED);
    ret = pthread_create(&emeter_tid, &emeter_tattr, emeter_comm_thread_do, pemeter);
    if(ret == 0) {
        LogInfo("create load power collect thread success");
    } else {
        LogError("create load power collect thread failed: %s", strerror(errno));
    }
    return 0;
}
