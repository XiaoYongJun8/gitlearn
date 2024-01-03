/*******************************************************************************
 * @             Copyright (c) 2023 LinyPower, All Rights Reserved.
 * @*****************************************************************************
 * @FilePath     : common_define.h
 * @Descripttion : 
 * @Author       : zhums
 * @E-Mail       : zhums@linygroup.cn
 * @Version      : v1.0.0
 * @Date         : 2023-10-18 10:11:37
*******************************************************************************/
#ifndef __COMMON_DEFINE_H__
#define __COMMON_DEFINE_H__
#include <stdint.h>

#define EMS_PCS_MAX                     4       //一个EMS可管理的PCS最大个数
#define PCS_BAT_NUM                     4       //一个PCS可接的电池簇个数
#define COMM_DATA_RATIO                 0.1     //EMS与CONT通信数据精度,1位小数
#define COMM_DATA_MULTIPLE              10      //EMS与CONT通信数据倍数,10倍
#define COMM_DATA_RATIO2                0.01    //EMS与CONT通信数据精度2，2位小数
#define COMM_DATA_MULTIPLE2             100     //EMS与CONT通信数据倍数2，100倍
#define POWER_FLOAT_MIN                 0.1     //功率最小有效值
#define POWER_INT_MIN                   1       //功率最小有效值

#define HMI_CTRL_PCS_NONE               0       //界面未操作PCS投运、停用
#define HMI_CTRL_PCS_START              1       //界面控制PCS投运
#define HMI_CTRL_PCS_STOP               2       //界面控制PCS停运
#define HMI_CTRL_PCS_NONE_DONE          0       //界面操作PCS投运、停用未有结束
#define HMI_CTRL_PCS_START_DONE         1       //界面控制PCS投运结束
#define HMI_CTRL_PCS_STOP_DONE          2       //界面控制PCS停运结束

#define TMSP_DISCHARGE1                 10      //10-放电时段1
#define TMSP_CHARGE1                    11      //11-充电时段1
#define TMSP_DISCHARGE2                 20      //20-放电时段2
#define TMSP_CHARGE2                    21      //21-充电时段2
#define TMSP_DISCHARGE3                 30      //30-放电时段3
#define TMSP_CHARGE3                    31      //31-充电时段3
#define IS_IN_TIMESPAN(hour, tmsp)      (tmsp.enable == 1 && tmsp.start_hour <= hour && hour < tmsp.end_hour)      //hour是否在tmsp(类型需为ems_tmsp_item_t)指定时段内
//充放电时段，tm=time, sp=span
typedef struct
{
    int8_t  enable;
    int8_t  start_hour;     //包含起始小时
    int8_t  end_hour;       //不包含结束小时
    int8_t  len;
    int16_t power;
    uint8_t type;           //时段类型：1-充电；2-放电
    uint8_t no;             //时段编号：10-放电时段1,11-充电时段1; 20-放电时段2,21-充电时段2
}ems_tmsp_item_t;
typedef struct 
{
    ems_tmsp_item_t charge_tmsp1;       //充电时段1
    ems_tmsp_item_t discharge_tmsp1;    //放电时段1
    ems_tmsp_item_t charge_tmsp2;       //充电时段2
    ems_tmsp_item_t discharge_tmsp2;    //放电时段2
    ems_tmsp_item_t charge_tmsp3;
    ems_tmsp_item_t discharge_tmsp3;    
    ems_tmsp_item_t night_es;           //夜间储能
    ems_tmsp_item_t xftg;               //削峰填谷(包含所有的充放电时段)
}chg_dischg_tmsp_t;

typedef enum
{
    ITEM_UNUSE = 0,      //未使用
    ITEM_USED,           //正常使用中
    ITEM_DELETING,       //删除中
}item_use_state_e;

#endif //__COMMON_DEFINE_H__
