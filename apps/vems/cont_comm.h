/*******************************************************************************
 * @             Copyright (c) 2023 LinyPower, All Rights Reserved.
 * @*****************************************************************************
 * @FilePath     : cont_comm.h
 * @Descripttion : 
 * @Author       : zhums
 * @E-Mail       : zhums@linygroup.cn
 * @Version      : v1.0.0
 * @Date         : 2023-10-19 09:05:10
*******************************************************************************/
#ifndef _CONT_COMM_H_
#define _CONT_COMM_H_
#include <stdint.h>

#define CAN_CMD_BMS_INFO1                   0x01
#define CAN_CMD_BMS_INFO2                   0x02
#define CAN_CMD_BMS_INFO3                   0x06
#define CAN_CMD_BMS_INFO4                   0x07
#define CAN_CMD_CONT_INFO1                  0xE1
#define CAN_CMD_CONT_CTRL1                  0xD1

#define CONT_INITING                        1           //控制器初始化中
#define CONT_RUNNING                        2           //控制器运行中

#define cont_dio_checkbit_var(var,bit)      (var&(0x01<<(bit)))
#define cont_dio_getbit_var(var,bit)        ((var) >> (bit) & 0x01)
#define cont_dio_setbit_var(var,bit)        (var|=(0x01<<(bit)))
#define cont_dio_clrbit_var(var,bit)        (var&=(~(0x01<<(bit))))

//继电器ID
enum RELAY_ID
{
    RELAY_1 = 1,
    RELAY_2,
    RELAY_3,
    RELAY_4,
    RELAY_MAX,
};
//继电器开合状态
enum RELAY_OC
{
    RELAY_OPEN = 0,                 //继电器开
    RELAY_CLOSE                     //继电器合
};

//电池簇ID
enum BAT_RACK_ID
{
    BAT_RACK_1 = 1,
    BAT_RACK_2,
    BAT_RACK_3,
    BAT_RACK_4,
    BAT_RACK_MAX,
};
enum BAT_CONN_STATE
{
    BAT_DISCONNECT = 0,
    BAT_CONNECTED,
};
enum BMS_ACT_STATE
{
    BMS_NOT_ACTIVE = 0,             //BMS未激活
    BMS_ACTIVED,                    //BMS已激活
};
enum BAT_CHG_STATE
{
    BAT_NO_CHARGE = 0,              //未充放电
    BAT_CHARGE,                     //充电
    BAT_DISCHARGE                   //放电
};

enum EMS_RUN_STATE
{
    EMS_INIT = 0,           //初始化
    EMS_RUNNING,            //运行
    EMS_STANDBY,            //待机
    EMS_ERROR,              //故障
};

enum IO_CTRL_BITS
{
    IO_LED_RUN_LIGHT = 0,
    IO_LED_RUN_CLOSE,
    IO_LED_ERR_LIGHT,
    IO_LED_ERR_CLOSE,
};

typedef struct
{
    uint32_t src_id : 8;        //SA, 源地址
    uint32_t dst_id : 8;        //PS, 目标地址
    uint32_t proto_cmd : 8;     //PF, 协议消息命令码
    uint32_t dp : 1;            //DP, 标识是否是数据页，dp:data page
    uint32_t reserve : 1;       //R, 保留位
    uint32_t priority : 3;      //P, 优先级
    uint32_t : 3;               //非包含在29位地址中的位
}bms_ext_id_t;

typedef struct
{
    uint32_t rack_id : 16;      //RACK_ID, 电池簇编号
    uint32_t cont_id : 4;       //CONT_ID, 控制器编号
    uint32_t cmd : 8;           //CMD, 命令码
    uint32_t reserve : 1;       //R, 保留位
    uint32_t : 3;               //非包含在29位地址中的位
}ems_ext_id_t;

typedef union
{
    struct
    {
        uint8_t enable : 1;
        uint8_t conn_id : 3;
        uint8_t bms1 : 1;
        uint8_t bms2 : 1;
        uint8_t bms3 : 1;
        uint8_t bms4 : 1;
    }can;
    uint8_t byte;
}can_ctrl_byte_t;

typedef union
{
    struct
    {
        uint8_t enable : 1;     //是否响应此项，0-不响应，1-响应;
        uint8_t close_id : 3;   //0:relay1-relay4=0; 1-4:relay{relay_id}=1, 其余=0
        uint8_t relay1 : 1;     //继电器1开合，0开，1合
        uint8_t relay2 : 1;     //继电器2开合，0开，1合
        uint8_t relay3 : 1;     //继电器3开合，0开，1合
        uint8_t relay4 : 1;     //继电器4开合，0开，1合
    }relay;
    uint8_t byte;
}relay_ctrl_byte_t;

typedef union
{
    struct
    {
        uint8_t enable : 1;            //是否响应此项，0-不响应，1-响应;
        uint8_t led_run_light : 1;     //运行灯亮，充放电时点亮
        uint8_t led_run_close : 1;     //运行灯灭
        uint8_t led_err_light : 1;     //故障灯亮控制，重故障导致停机时
        uint8_t led_err_close : 1;     //故障灯灭控制
        uint8_t reserve : 3;           //保留
    }bits;
    uint8_t byte;
}io_ctrl_byte_t;

typedef union
{
    struct
    {
        uint8_t reserve : 4;        //保留
        uint8_t gun1 : 1;           //充电枪1连接状态
        uint8_t gun2 : 1;           //充电枪2连接状态
        uint8_t gun3 : 1;           //充电枪3连接状态
        uint8_t gun4 : 1;           //充电枪4连接状态
    }stat;
    uint8_t byte;
}gun_state_t;

typedef union
{
    struct
    {
        uint8_t conn_id : 4;        //当前连接的BMS CAN编号，均未连接为0
        uint8_t bms1 : 1;           //BMS1 CAN连接状态
        uint8_t bms2 : 1;           //BMS2 CAN连接状态
        uint8_t bms3 : 1;           //BMS3 CAN连接状态
        uint8_t bms4 : 1;           //BMS4 CAN连接状态
    }stat;
    uint8_t byte;
}bms_can_state_t;

typedef union
{
    struct
    {
        uint8_t close_id : 4;       //当前闭合状态继电器编号，均未闭合为0
        uint8_t relay1 : 1;         //继电器1闭合状态
        uint8_t relay2 : 1;         //继电器2闭合状态
        uint8_t relay3 : 1;         //继电器3闭合状态
        uint8_t relay4 : 1;         //继电器4闭合状态
    }stat;
    uint8_t byte;
}relay_state_t;

typedef struct
{
    uint8_t ems_state;              //系统运行状态下发
    relay_ctrl_byte_t relay_ctrl;
    can_ctrl_byte_t can_ctrl;
    io_ctrl_byte_t io_ctrl;
    int16_t fan_open_temp;          //0 means not set, unit: 0.1 
    int16_t fan_close_temp;
}cont_ctrl_t;

typedef union
{
    struct 
    {
        uint8_t emg_stop : 1;
        uint8_t fu1 : 1;
        uint8_t led_run : 1;
        uint8_t led_fault : 1;
        uint8_t fan : 1;
        uint8_t pcs_emg_stop : 1;
        uint8_t reserve : 2;
    }bits;
    uint8_t byte;
}cont_dio_state_t;

typedef struct
{
    //运行参数1
    uint8_t cont_state;             //1：初始化；2：运行中
    gun_state_t gun_stat;           //bit0-4：1-4号枪；0：未连接，1：已连接
    bms_can_state_t can_stat;       //bit0-4：1-4号BMS；0：未连接，1：已连接；bit[5,6]：当前接通状态编号
    relay_state_t relay_stat;       //直流继电器状态
    relay_state_t relay_stat_fb;    //直流继电器反馈状态
    cont_dio_state_t dio_stat;      //控制器采集的IO状态

    uint16_t rack_id;               //收到的控制器转发过来的CANID中的电池包ID
    uint16_t rack_id_changged;      //上个收到的控制器转发过来的CANID中的电池包ID，变化才会更新
}cont_data_t;

typedef struct {
    uint8_t  pcs_idx;
    uint16_t wait_ready_cnt;
    uint32_t bms_msg01_rx_cnt;
    uint32_t bms_msg02_rx_cnt;
    uint32_t bms_msg06_rx_cnt;
    uint32_t bms_msg07_rx_cnt;
    uint32_t bms_msgE1_rx_cnt;
    int32_t can_sock;                   //CAN通信socket文件描述符
    uint8_t rdata[64];
    uint8_t wdata[64];
    cont_data_t rtdata;
}ems_cont_t;

//BMS相关
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

typedef union
{
    struct {
        uint8_t temp_high : 1;              /*温度高*/
        uint8_t temp_low : 1;               /*温度低*/
        uint8_t temp_diff_big : 1;          /*温差大*/
        uint8_t tot_vol_high : 1;           /*总压高*/
        uint8_t tot_vol_low : 1;            /*总压低*/
        uint8_t cell_vol_high : 1;          /*单体电压高*/
        uint8_t cell_vol_low : 1;           /*单体电压低*/
        uint8_t cell_vol_diff_big : 1;      /*单体压差大*/
    }bits;
    uint8_t byte;
}alarm_flag1_t;                         /*一级/二级/三级报警标识1*/

typedef union
{
    struct {
        uint8_t charge_curr_big : 1;        /*充电电流大*/
        uint8_t discharge_curr_big : 1;     /*放电电流大*/
        uint8_t rev1 : 1;                   /*预留*/
        uint8_t soc_low : 1;                /*soc低*/
        uint8_t jyzk_low : 1;               /*绝缘阻拦低*/
        uint8_t rev2 : 1;                   /*预留*/
        uint8_t rev3 : 1;                   /*预留*/
        uint8_t rev4 : 1;                   /*预留*/
    }bits;
    uint8_t byte;
}alarmL12_flag2_t;                      /*一级/二级报警标识2*/

typedef union
{
    struct {
        uint8_t charge_curr_big : 1;        /*充电电流大*/
        uint8_t discharge_curr_big : 1;     /*放电电流大*/
        uint8_t rev1 : 1;                   /*预留*/
        uint8_t rev2 : 1;                   /*预留*/
        uint8_t rev3 : 1;                   /*预留*/
        uint8_t rev4 : 1;                   /*预留*/
        uint8_t ms_comm_fault : 1;          /*主从板通信故障*/
        uint8_t main_comm_fault : 1;        /*主通信故障*/
    }bits;
    uint8_t byte;
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
    alarm_flag1_t    alarm11;           /*一级报警标识1*/
    alarmL12_flag2_t alarm12;           /*一级报警标识2*/
    alarm_flag1_t    alarm21;           /*二级报警标识1*/
    alarmL12_flag2_t alarm22;           /*二级报警标识2*/
    alarm_flag1_t    alarm31;           /*三级报警标识1*/
    alarmL3_flag2_t  alarm32;           /*三级报警标识2*/

    //根据读取到的BMS数据，判断出的状态信息
    uint8_t bat_chg_dischg_done;        //电池充放电是否完成，0:未知/确认中; 1:未完成; 2:充电完成; 3:放电完成
    uint8_t alarm_flag_stop_immed;      //有需要立即停机的告警状态
    uint8_t alarm_flag_stop_smooth;     //有需要快速降功率并停机的告警状态
    uint8_t alarm_flag_power_half;      //有需要降一半功率的告警状态
    uint8_t alarm_flag_power_low;       //有需要快速降功率并维持在低功率充放电的告警状态

    uint8_t heart;                      /*心跳*/
    uint8_t pcs_status;                 /*PCS/HPS/PBD状态*/
    int16_t bat_power;                  /*电池功率*/
}bms_data_t;

typedef struct
{
    uint8_t conn_state : 2;             //连接状态，是否已插枪
    uint8_t act_state : 2;              //激活状态，是否收到BMS数据
    uint8_t chg_state : 4;              //充电状态，0-未充放电; 1-充电; 2-放电
    //uint8_t chg_dischg_done;            //charge or discharge is done

    uint8_t rdata[64];
    uint8_t wdata[64];
    bms_data_t rtdata;
}battery_rack_t;



int create_cont_comm_thread(void);
int cont_comm_relay_ctrl(ems_cont_t* pcont, uint8_t relay_id, uint8_t relay_oc);
int cont_comm_bms_can_ctrl(ems_cont_t* pcont, uint8_t bms_id);
int cont_comm_io_ctrl(ems_cont_t* pcont, uint8_t io_val, enum EMS_RUN_STATE ems_state);
void bms_set_heart(int pcsid, int batid, uint8_t heart);
void bms_set_pcs_status(int pcsid, int batid, uint8_t status);
void bms_set_bat_power(int pcsid, int batid, int16_t power);
int bms_and_cont_read_data(int can_sock, uint32_t canid, uint8_t* pbuff, int timeout_ms);
int bms_and_cont_write_data(int can_sock, uint32_t canid, uint8_t* pbuff);

#endif //_CONT_COMM_H_
