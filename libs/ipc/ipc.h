/*******************************************************************************
 * @             Copyright (c) 2023 LinyPower, All Rights Reserved.
 * @*****************************************************************************
 * @FilePath     : ipc.h
 * @Descripttion : 
 * @Author       : zhums
 * @E-Mail       : zhums@linygroup.cn
 * @Version      : v1.0.0
 * @Date         : 2023-07-04 13:27:11
*******************************************************************************/
#ifndef __IPC_H__
#define __IPC_H__

#include "msq.h"
#include "name_pipe.h"
#include "shm_util.h"
#include "fdevent.h"


typedef struct can_ipc_msg {
    uint8_t data[8];
}can_ipc_msg_t;

typedef struct limit_data
{
    uint16_t cglc;
    uint16_t dcglc;
    uint16_t cglv;
    uint16_t dcglv;
}limit_data_t;

typedef struct bms_stat_data
{
    uint16_t stat;
    uint16_t res[3];
}bms_stat_data_t;

typedef struct common_rsp
{
    uint16_t can_id;
    uint16_t rc;
    uint16_t res[2];
}common_rsp_t;


#endif //__IPC_H__
