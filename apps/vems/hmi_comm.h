/*******************************************************************************
 * @             Copyright (c) 2023 LinyPower, All Rights Reserved.
 * @*****************************************************************************
 * @FilePath     : hmi_comm.h
 * @Descripttion : 
 * @Author       : xiaoyongjun
 * @E-Mail       : xiaoyongjun@linygroup.cn
 * @Version      : v1.0.0
 * @Date         : 2023-10-23 09:23:46
*******************************************************************************/
#ifndef _HMI_COMM_H_
#define _HMI_COMM_H_
#include <stdint.h>

/*弹窗地址*/
#define CLEAR_WINDOW_ADDR       0x77
#define SHOW_WINDOW_ADDR        0x78
#define ICON_ADDR               0x20
#define TITLE_ADDR              0x21
#define CONTEXT_ADDR            0x29
#define BUTTON_NUM_ADDR         0x75

/*登录按钮地址*/
#define LOG_ON_ADDR             0x11
#define LOG_ON_USER_ADDR        0x15a
#define LOG_ON_PASS_ADDR        0x16a
#define LOG_ON_STATUS_ADDR      0x1e
#define LOG_ON_RESULT_ADDR      0x1f
#define HMI_LINK_ADDR           0x168

/*清空历史数据按钮*/
#define CLEAR_HISTORY_DATA_ADDR 0x76f
/*RTC寄存器地址*/
#define RTC_SET_TIME_ADDR       0x76e
#define RTC_READ_YEAR_ADDR      0x761
#define RTC_READ_MONTH_ADDR     0x762
#define RTC_READ_DAY_ADDR       0x763
#define RTC_READ_HOUR_ADDR      0x764
#define RTC_READ_MIN_ADDR       0x765
#define RTC_READ_SEC_ADDR       0x766
#define RTC_WRITE_YEAR_ADDR     0x767
#define RTC_WRITE_MONTH_ADDR    0x768
#define RTC_WRITE_DAY_ADDR      0x769
#define RTC_WRITE_HOUR_ADDR     0x76a
#define RTC_WRITE_MIN_ADDR      0x76b
#define RTC_WRITE_SEC_ADDR      0x76c

/*系统概览寄存器地址*/
#define TOTAL_CHARGE_ADDR            0x150
#define TOTAL_DISCHARGE_ADDR         0x151
#define FAN_ADDR                     0x152
#define ACTIVE_POWER_ADDR            0x166
#define REACTIVE_POWER_ADDR          0x167
#define PCS_CTRL_START_STOP          0x157
#define PCS_CTRL_START_STOP_DONE     0x158

#define LISTEN_PAGE_NUM              14
#define LISTEN_SYSTEM_SCREEN_ADDR    0x01
#define PCS_MAN_PAGE_VAL             0x01
#define EXIT_LOG_ADDR                0x49

/*pcs管理modbus寄存器地址*/
#define LISTEN_ADD_PCS_ADDR
#define LISTEN_CHANGE_PCS_ADDR
#define LISTEN_DEL_PCS_ADDR     
#define PCS_ID_ADDR             0x100
#define PCS_NAME_ADDR           0x101
#define PCS_BAUD_ADDR
#define PCS_POWER_ADDR
#define PCS_MAX_VOL_ADDR
#define PCS_MXA_CUR_ADDR 
#define CONT_COMID_ADDR 
#define CAN_ADDR 
#define CAN_BAUD_ADDR
#define PCS_CHECK_ADDR          0x204 /*复选框*/
#define PCS_SWITCH_ADDR         0x111 /*选择框*/
#define PCS_SHOW_ADDR           0x205 /*显示名称数据*/
#define PCS_ADD_ADDR            0x201
#define PCS_CHANGE_ADDR         0x202
#define PCS_DEL_ADDR            0x203

/*充放电管理地址*/
#define CHANGE_TIME_ADDR        0x300  
#define CHARGE_TIME_1           0x310
#define DISCHARGE_TIME_1        0
#define CHARGE_TIME_2           0
#define DISCHARGE_TIME_2        0
#define NIGHT_STORE_ENERGY_TIME 0x300

/*系统配置参数地址*/
#define SYSTEM_PARA_CHANGE_ADDR 0x508
#define SYSTEM_MODE_ADDR        0x500

/*通信配置参数地址*/
#define CONN_PARA_CHANGE_ADDR   0x55E
#define CONN_PARA_IP_ADDR       0x550

/*pcs信息监控地址*/
#define PCS_INFO_START_ADDR             0x700

/*bms信息监控地址*/
#define BMS1_BAT_STATUS_ADDR    0x800
#define BMS2_BAT_STATUS_ADDR    (BMS1_BAT_STATUS_ADDR+10)
#define BMS3_BAT_STATUS_ADDR    (BMS2_BAT_STATUS_ADDR+10)
#define BMS4_BAT_STATUS_ADDR    (BMS3_BAT_STATUS_ADDR+10)
#define BMS1_HIDE_ADDR          0x850
#define BMS2_HIDE_ADDR          0x851
#define BMS3_HIDE_ADDR          0x852
#define BMS4_HIDE_ADDR          0x853

/*bms告警信息地址*/
#define SYSTEM_STATUS_ADDR      0x901
#define ONE_LEVEL_WARN_ADDR     0x909
#define TWO_LEVEL_WARN_ADDR     (0x909+16)
#define THREE_LEVEL_WARN_ADDR   (0x909+32)
#define BMS_ID_ADDR             0x900

/*pcs故障信息*/
#define PCS_ERROR_START_ADDR  0x71D


/*版本号寄存器地址*/
#define EMS_VERSION_ADDR        0x50
#define CON_VERSION_ADDR        0x5C
#define HMI_VERSION_ADDR        0x68
#define EMS_DEFAULT_VERSION     "V1.0.1_release231110"
#define CON_DEFAULT_VERSION     "V1.0.1_release231110"
#define HMI_DEFAULT_VERSION     "V1.0.1_release231110"

#define CLOUD_IP_DEFAULT        "192.168.11.42"
#define CLOUD_PORT_DEFAULT      8888

#define DEFAULT_USER_NAME       "lmt"
#define DEFAULT_USER_PASS       123

struct hmi_screen;
struct hmi_dialog;

typedef struct monitor_control
{
    uint16_t ctrl_addr;
    uint16_t ctrl_val;
    void (*handle)(struct hmi_screen* pscreen, int ctrl_id);
}monitor_control_t;

typedef struct monitor_button
{
    uint16_t btn_addr;
    uint16_t btn_val;
    void (*handle)(struct hmi_dialog* pdialog, int btn_id);
}monitor_button_t;

typedef struct hmi_screen
{
    uint8_t screen_id;
    uint16_t wdata[128];
    uint16_t rdata[128];
    
    uint8_t mon_ctrl_cnt;
    monitor_control_t mon_ctrl[8];
}hmi_screen_t;

typedef struct hmi_dialog
{
    uint16_t parent_id;     //父界面ID, parent screen id
    uint8_t mon_btn_cnt;
    monitor_button_t mon_btn[3];    
}hmi_dialog_t;

int create_hmi_comm_thread(void);
void hmi_comm_system_overview(struct hmi_screen* pscreen, int ctrl_id);
void hmi_comm_overview_ctrl_pcs_start_stop(struct hmi_screen* pscreen, int ctrl_id);
void hmi_comm_add_pcs(struct hmi_screen* pscreen, int ctrl_id);
void hmi_comm_change_pcs(struct hmi_screen* pscreen, int ctrl_id);
void hmi_comm_del_pcs(struct hmi_screen* pscreen, int ctrl_id);
void hmi_dialog_add_pcs(struct hmi_dialog* pdialog, int btn_id);
void hmi_dialog_modify_pcs(struct hmi_dialog* pdialog, int btn_id);
void hmi_dialog_del_pcs(struct hmi_dialog* pdialog, int btn_id);
void hmi_comm_change_charge(struct hmi_screen* pscreen, int ctrl_id);
void hmi_comm_show_time(void);
void hmi_comm_change_system_config(struct hmi_screen* pscreen, int ctrl_id);
void hmi_comm_change_conn_config(struct hmi_screen* pscreen, int ctrl_id);
void hmi_comm_pcs_info_show(struct hmi_screen* pscreen, int ctrl_id);
void hmi_comm_bms_info_show(struct hmi_screen* pscreen, int ctrl_id);
void hmi_comm_bms_warn_show(struct hmi_screen* pscreen, int ctrl_id);
void hmi_comm_version_show(struct hmi_screen* pscreen, int ctrl_id);
void hmi_comm_pcs_warn_show(struct hmi_screen* pscreen, int ctrl_id);
#endif
