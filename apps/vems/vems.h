/*******************************************************************************
 * @             Copyright (c) 2023 LinYuan Power, All Rights Reserved.
 * @*****************************************************************************
 * @FilePath     : vems.h
 * @Descripttion : 
 * @Author       : zhums
 * @E-Mail       : zhums@linygroup.com
 * @Version      : v1.0.0
 * @Date         : 2023-10-07 15:22:37
*******************************************************************************/
#ifndef __VEMS_H__
#define __VEMS_H__
#include <stdint.h>
#include "config.h"
#include "common_define.h"
#include "pcs_comm.h"
#include "cont_comm.h"
#include "load_power_collect.h"

#define DBG_PREFIX    "[TDBG] "

typedef struct {
    config_comm_t   general;
    int             loglevel_mode;
    int             loglevel;
    char            loglevel_types[64];
    uint8_t         pcs_mb_debug;
    uint8_t         emeter_mb_debug;
    int16_t         pcs_pset_min;                   //PCS最小有功设置值，分辨率0.1kW

    char            device_id[16];
    char            emeter_ipaddr[16];
    uint16_t        emeter_port;
    uint8_t         emeter_slave_id;
    float           station_power_capacity_real;     //厂站功率容量(最大功率)-真实浮点值
    int             station_power_capacity;          //厂站功率容量(最大功率)-加分辨率后整型值(计算、传输使用)
    float           station_power_reserve_real;      //厂站功率容量保留值(预留一定功率不用，以备不时之需)
    int             station_power_reserve;           //厂站功率容量保留值(预留一定功率不用，以备不时之需)
    float           station_power_available_real;    //厂站功率容量(最大功率)-真实浮点值
    int             station_power_available;         //厂站功率容量(最大功率)-加分辨率后整型值(计算、传输使用)
    float           power_update_step_real;          //调整PCS功率最小步长(变化小于此步长时不调整功率)
    int             power_update_step;               //调整PCS功率最小步长(变化小于此步长时不调整功率)
}config_vems_t;

typedef enum {
    xftg_mode = 0,
    yjcn_mode,
    debug_mode,
}ems_mode_change_t;

typedef struct {
    ems_mode_change_t mode;
    uint16_t discharge_min_soc;
    uint16_t charge_max_soc;
    uint8_t  fnl_en;                //防逆流使能标志
    uint16_t debug_mode_power;
    uint16_t on_grid_power;
    uint16_t bms_power;
    uint16_t load_power;
}ems_system_config_t;

typedef struct {
    char cloud_ip[20];
    uint16_t cloud_port;
    uint8_t modbus_id;
    uint32_t modbus_baud;
    uint32_t hmi_baud;
    uint32_t can_baud;
}ems_conn_para_config_t;

typedef struct {
    char ems_version[24];
    char control_version[24];
    char hmi_version[24];
}version_t;

typedef struct {
    char username[16];
    uint32_t password;
}hmi_logon_t;

#define THREAD_ALL_READY    0x7F
typedef union
{
    struct
    {
        uint8_t th_pcs_comm : 1;
        uint8_t th_cont_comm : 1;
        uint8_t th_emeter_comm : 1;
        uint8_t th_energy_mana : 1;
        uint8_t th_hmi_comm : 1;
        uint8_t th_common : 1;
        uint8_t th_ipc_comm : 1;
        uint8_t : 1;
    }bits;
    uint8_t byte;
}threads_stat_t;

typedef struct {
    threads_stat_t    th_stat;                      //各线程状态：是否初始化完成
    int               pcs_cnt;
    pcs_dev_t         pcs[EMS_PCS_MAX];
    int16_t           pcs_power_total_pre;
    int16_t           pcs_power_total;
    int16_t           pcs_power_calc_pre;
    int16_t           pcs_power_calc;
    int16_t           pcs_power_rated;
    uint8_t           pcs_heart;
    int               can_sock;                     //与投切控制器通信CAN socket
    uint32_t          can_nodata_sec;               //can未收到数据持续秒数
    elec_meter_t      elec_meter;
    chg_dischg_tmsp_t tmsp;
    config_vems_t     cfg;
    //HMI 使用
    ems_system_config_t syscfg;
    ems_conn_para_config_t concfg;
    version_t version;
    hmi_logon_t user;
}vems_t;

extern vems_t g_vems;

int vems_read_tmsp(void);
int vems_check_tmsp_item_valid(ems_tmsp_item_t* ptmsp);
#endif // __VEMS_H__
