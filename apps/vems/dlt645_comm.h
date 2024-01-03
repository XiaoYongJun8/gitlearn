/*******************************************************************************
 * @             Copyright (c) 2023 LinyPower, All Rights Reserved.
 * @*****************************************************************************
 * @FilePath     : dlt645_comm.h
 * @Descripttion : 
 * @Author       : xiaoyongjun
 * @E-Mail       : xiaoyongjun@linygroup.cn
 * @Version      : v1.0.0
 * @Date         : 2023-12-08 16:35:10
*******************************************************************************/
#ifndef _DLT645_COMM_H_
#define _DLT645_COMM_H_

#include <stdint.h>
#include "dlt645_port_tcp.h"
#include "vems.h"


#define DLT645_IP_ADDR "192.168.226.7"
#define DLT645_PORT    8887
#define VARIABLE_FLOW_RATE  200

#define RDT_IDX_P        0       //总有功功率数据下表

typedef struct {
    uint32_t code;
    uint8_t  dlt645_real_data[12];
    uint8_t real_data_len;
    float real_fdata;
    dlt645_protocal protocal_type;
    uint16_t timeout_ms;
}dlt645_cmd_table_t;


int create_dlt645_comm_thread(elec_meter_t* pemeter);

#endif

