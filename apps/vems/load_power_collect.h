/*******************************************************************************
 * @             Copyright (c) 2023 LinyPower, All Rights Reserved.
 * @*****************************************************************************
 * @FilePath     : load_power_collect.h
 * @Descripttion : 
 * @Author       : zhums
 * @E-Mail       : zhums@linygroup.cn
 * @Version      : v1.0.0
 * @Date         : 2023-11-02 11:07:01
*******************************************************************************/
#ifndef __LOAD_POWER_COLLECT_H__
#define __LOAD_POWER_COLLECT_H__
#include <stdint.h>
#include "database.h"
#include "modbus.h"
#include "common_define.h"
#include "dlt645.h"
#define GWC_INPUT_REG_ADDR_START      0       //采集网关输入寄存器开始地址(协议地址)
#define GWC_RDATA_NUM                 6       //电表输入寄存器个数
#define GWC_DATA_RATIO                0.01

typedef struct
{
    uint8_t state;                  //线程状态
    //char dev_name[64];              //modbus-rtu 485设备名
    char dev_ip[16];
    u_int16_t dev_port;
    modbus_t* ctx;
    dlt645_t* ctx645;
    struct timeval def_rsp_tmov;    //默认等待响应超时时间，初始化后不应再修改
    struct timeval rsp_tmov;        //等待响应超时时间，用来设置新的超时时间
    uint16_t rdata[GWC_RDATA_NUM];

    //电表实时数据
    float load_power_real;
    int   load_power_pre;
    int   load_power;                   //负载实时功率(带分辨率的整型值)
}elec_meter_t;

int create_emeter_comm_thread(elec_meter_t* pemeter);
void emeter_data_collect_init(elec_meter_t* pemeter);
#endif //__LOAD_POWER_COLLECT_H__
