/*******************************************************************************
 * @             Copyright (c) 2023 LinyPower, All Rights Reserved.
 * @*****************************************************************************
 * @FilePath     : pcs_comm.h
 * @Descripttion : 
 * @Author       : xiaoyongjun
 * @E-Mail       : xiaoyongjun@linygroup.cn
 * @Version      : v1.0.0
 * @Date         : 2023-10-08 15:42:22
*******************************************************************************/
#ifndef _PCS_COMM_H_
#define _PCS_COMM_H_
#include <stdint.h>
#include "database.h"
#include "modbus.h"
#include "common_define.h"
#include "cont_comm.h"

/* 读取状态量 功能码 02*/
#define STATUS_START_ADDR           0
#define COIL_STATE_NUM              8

/*启停复位功能参数03 10 可读可写*/
#define START_RESET_START_ADDR      2304
#define START_RESET_REG_NUM         6
#define REG_ADDR_START              2304
#define REG_ADDR_STOP               2306
#define REG_ADDR_RESET              2308
/*现场功能参数参数03 10 可读可写*/
#define FUN_PARA_START_ADDR_1       66
#define FUN_PARA_REG_NUM_1          2
#define FUN_PARA_START_ADDR_2       126
#define FUN_PARA_REG_NUM_2          2
#define FUN_PARA_START_ADDR_3       188
#define FUN_PARA_START_IDX_3        (START_RESET_REG_NUM + FUN_PARA_REG_NUM_1 + FUN_PARA_REG_NUM_2)
#define FUN_PARA_REG_NUM_3          2
#define REG_ADDR_REACTIVE_POWER     188
#define REG_ADDR_ACTIVE_POWER       189
#define FUN_PARA_START_ADDR_4       197
#define FUN_PARA_REG_NUM_4          1
/* 读取运行信息 功能码 03 只读*/
#define RUN_INFO_START_ADDR         720
#define RUN_INFO_REG_NUM            25
#define RDATA_NUM                   (RUN_INFO_REG_NUM + START_RESET_REG_NUM + FUN_PARA_REG_NUM_1+ FUN_PARA_REG_NUM_2 + FUN_PARA_REG_NUM_3 + FUN_PARA_REG_NUM_4)
#define WDATA_NUM                   (START_RESET_REG_NUM + FUN_PARA_REG_NUM_1+ FUN_PARA_REG_NUM_2 + FUN_PARA_REG_NUM_3 + FUN_PARA_REG_NUM_4)


enum PCS_ID
{
    PCS_1 = 0,
    PCS_2,
    PCS_3,
    PCS_4,
    PCS_MAX,
};

enum PCS_RUN_CMD
{
    CMD_NONE = 0,       //no cmd
    CMD_START = 1,      //启动设备
    CMD_STOP,           //停止设备
    CMD_RESET           //复位设备
};

enum PCS_RUN_MODE
{
    PRM_RUN = 0,        //PCS运行模式
    PRM_STOP,           //PCS停机模式
};

enum PCS_STATE
{
    PCS_INITING = 1,                //PCS初始化中
    PCS_INITED,                     //PCS初始化完成
    PCS_STANDBY,                    //PCS待机
    PCS_CHARGE_BAT,                 //PCS给电池充电中
    PCS_DISCHARGE_BAT,              //PCS从电池放电中
    PCS_STOPPING,                   //PCS停机中
    PCS_STOPPED,                    //PCS已停机
    PCS_ERR_STOPPING,               //PCS因EMS处理错误需停机处理中
    PCS_CHARGE_STANDBY,             //PCS由给电池充电转变为待机中
    PCS_DISCHARGE_STANDBY,          //PCS由从电池放电转变为待机中
    PCS_CHARGE_STOPPING,            //PCS由给电池充电转变为停机中
    PCS_DISCHARGE_STOPPING,         //PCS由从电池放电转变为停机中
    PCS_CHARGE_STOPPED,             //PCS由给电池充电转变为已停机
    PCS_DISCHARGE_STOPPED,          //PCS由从电池放电转变为已停机
    PCS_FAULT,                      //PCS故障中
    PCS_RESET,                      //PCS复位中
    PCS_HMI_STOPPING,               //PCS由HMI控制停机中
    PCS_HMI_STOPPED,                //PCS已由HMI控制停机
};

enum PCS_TOBMS_STATE
{
    PCS_BMS_INIT = 0,               //PCS下发给BMS的状态-初始
    PCS_BMS_READY,                  //PCS下发给BMS的状态-就绪
    PCS_BMS_CHARGE,                 //PCS下发给BMS的状态-充电
    PCS_BMS_DISCHARGE,              //PCS下发给BMS的状态-放电
    PCS_BMS_FAULT,                  //PCS下发给BMS的状态-故障
};

enum
{
    R_IDX_MIN = 0,
    R_IDX_RUN_INFO_BASE = 0,
    GRID_VOL = 0,
    GRID_FREQ,
    RADIATOR_TEMP,
    DCBUS_VOL,
    GRID_UA,
    GRID_UB,
    GRID_UC,
    PCS_IA,
    PCS_IB,
    PCS_IC,
    OUTPUT_PA,
    OUTPUT_PB,
    OUTPUT_PC,
    OUTPUT_QA,
    OUTPUT_QB,
    OUTPUT_QC,
    OUTPUT_SA,
    OUTPUT_SB,
    OUTPUT_SC,
    SYSTEM_ERR1,
    SYSTEM_ERR2,
    DC_CURRENT,
    SYS_STATE,
    DC_CURRENT_OUT,
    GRID_STATE,

    //启动复位功能参数
    R_IDX_RSS_BASE = RUN_INFO_REG_NUM,       //读取参数（启停复位功能参数基地址）,RSS=RUN STOP RESET
    R_START1 = R_IDX_RSS_BASE,
    R_START2,
    R_STOP1,
    R_STOP2,
    R_RESET1,
    R_RESET2,

    //功能参数
    R_IDX_FUNC_PARAM_BASE = RUN_INFO_REG_NUM + START_RESET_REG_NUM,
    FAULT_RESET_CNT = R_IDX_FUNC_PARAM_BASE + 0,
    FAULT_START_CNT,
    SYS_READY,
    SYS_RUN,
    Q_SET,
    P_SET,
    RUN_MODE,
    R_IDX_MAX
};

enum WDATA_IDX
{
    W_IDX_MIN = 0,
    W_START1 = 0,
    W_START2,
    W_STOP1,
    W_STOP2,
    W_RESET1,
    W_RESET2,
    W_FAULT_RESET_CNT = 6,
    W_FAULT_START_CNT,
    W_SYS_READY,
    W_SYS_RUN,
    W_Q_SET,
    W_P_SET,
    W_RUN_MODE,
    W_IDX_MAX
};

enum PCS_CTRL_STEP
{
    PCS_START_STEP_INIT = 1,
    PCS_START_STEP1 = 1,
    PCS_START_STEP2,
    PCS_START_STEP21,
    PCS_START_STEP22,
    PCS_START_STEP3,
    PCS_START_STEP31,
    PCS_START_STEP32,
    PCS_START_STEP4,
    PCS_START_STEP41,
    PCS_START_STEP42,
    PCS_START_STEP5,
    PCS_START_STEP_DONE,


    PCS_STOP_STEP_INIT = 21,
    PCS_STOP_STEP1 = 21,
    PCS_STOP_STEP11,
    PCS_STOP_STEP12,
    PCS_STOP_STEP2,
    PCS_STOP_STEP21,
    PCS_STOP_STEP22,
    PCS_STOP_STEP3,
    PCS_STOP_STEP31,
    PCS_STOP_STEP32,
    PCS_STOP_STEP_DONE,

    PCS_SRCH_BAT_STEP_INIT = 41,
    PCS_SRCH_BAT_STEP1 = 41,
    PCS_SRCH_BAT_STEP11,
    PCS_SRCH_BAT_STEP12,
    PCS_SRCH_BAT_STEP2,
    PCS_SRCH_BAT_STEP21,
    PCS_SRCH_BAT_STEP22,
    PCS_SRCH_BAT_STEP3,
    PCS_SRCH_BAT_STEP31,
    PCS_SRCH_BAT_STEP32,
    PCS_SRCH_BAT_STEP_DONE,
};

//PCS相关
typedef struct {
    uint16_t pcs_id;
    char pcs_name[16];
    uint8_t modbus_id;
    uint16_t baud_rate;
    int16_t rated_power;
    int16_t max_voltage;
    int16_t max_current;
    uint8_t cont_comm_id;
    uint8_t cont_can_addr;
    uint16_t cont_can_baudrate;
    uint8_t state;
}dev_pcs_params_t;

typedef struct {
    uint16_t vp_error_1 : 1;
    uint16_t dc_software_overvol : 1;
    uint16_t wp_error_1 : 1;
    uint16_t upper_spilt_overvol : 1;
    uint16_t down_spilt_overvol : 1;
    uint16_t un_error_2 : 1;
    uint16_t module_overtemp : 1;
    uint16_t vn_error_2 : 1;
    uint16_t ac_software_undervol : 1;
    uint16_t wn_error_2 : 1;
    uint16_t crash_stop : 1;
    uint16_t ac_software_overvol : 1;
    uint16_t over_load : 1;
    uint16_t software_overcur_a : 1;
    uint16_t software_overcur_b : 1;
    uint16_t software_overcur_c : 1;
}system_error1_bit0_15_t;

typedef struct {
    uint16_t grid_vol_imbalance : 1;
    uint16_t grid_freq_abnormal : 1;
    uint16_t reserve : 3;
    uint16_t low_punture_fail : 1;
    uint16_t high_press_precharge_fail : 1;
    uint16_t high_punture_fail : 1;
    uint16_t hardware_overcur_a : 1;
    uint16_t hardware_overcur_b : 1;
    uint16_t hardware_overcur_c : 1;
    uint16_t power_24v_abnormal : 1;
    uint16_t inodv1 : 1;
    uint16_t inodv3 : 1;
    uint16_t up_error_1 : 1;
    uint16_t dc_software_undervol : 1;

}system_error2_bit0_15_t;

typedef struct {

    uint16_t reserve1 : 1;
    uint16_t run : 1;
    uint16_t reserve2 : 2;
    uint16_t lock_phase_sucess : 1;
    uint16_t allow_start : 1;
    uint16_t on_grid_status : 1;
    uint16_t reserve3 : 1;
    uint16_t reserve4 : 8;
}system_system_bit0_15_t;

typedef struct {
    uint8_t start_enable : 1;
    uint8_t fault : 1;
    uint8_t run : 1;
    uint8_t stop : 1;
    uint8_t reset : 1;
    uint8_t phase_lock : 1;
    uint8_t reserve : 2;
}state_bit0_7_t;

typedef union {
    state_bit0_7_t bits;
    uint8_t byte;
}pcs_mb02_data_t;

typedef struct {
    uint16_t grid_voltage;
    uint16_t gird_freq;
    uint16_t radiator_temp;		//散热器温度
    uint16_t dcbus_voltage;		//直流母线电压
    uint16_t grid_ua;
    uint16_t grid_ub;
    uint16_t grid_uc;
    uint16_t pcs_ia;			//A相装置电流
    uint16_t pcs_ib;
    uint16_t pcs_ic;
    uint16_t pcs_pa;			//输出A相有功
    uint16_t pcs_pb;
    uint16_t pcs_pc;
    uint16_t pcs_qa;			//输出A相无功
    uint16_t pcs_qb;
    uint16_t pcs_qc;
    uint16_t pcs_sa;			//输出A相视在功率
    uint16_t pcs_sb;
    uint16_t pcs_sc;
    uint16_t sys_err1;
    uint16_t sys_err2;
    uint16_t dc_current;
    uint16_t sys_state;
    uint16_t dc_current_out;
    uint16_t grid_state;		//1-并网，0-离网
}pcs_mb03_data_t;

typedef struct {
    //状态信息
    pcs_mb02_data_t coil_state;

    //运行信息
    float grid_voltage;
    float gird_freq;
    float radiator_temp;		//散热器温度
    float dcbus_voltage;		//直流母线电压
    float grid_ua;
    float grid_ub;
    float grid_uc;
    float pcs_ia;				//A相装置电流
    float pcs_ib;
    float pcs_ic;
    float pcs_pa;			//输出A相有功
    float pcs_pb;
    float pcs_pc;
    float pcs_pall;         //输出总有功
    float pcs_qa;			//输出A相无功
    float pcs_qb;
    float pcs_qc;
    float pcs_qall;         //输出总无功
    float pcs_sa;			//输出A相视在功率
    float pcs_sb;
    float pcs_sc;
    float pcs_sall;         //输出总视在
    uint16_t sys_err1;
    uint16_t sys_err2;
    float dc_current;
    uint16_t sys_state;
    float dc_current_out;
    uint16_t grid_state;			//1-并网，0-离网

    //读取参数（ 启停复位功能参数）
    char start_str[4];
    char stop_str[4];
    char reset_str[4];

    //现场功能参数
    uint16_t fault_reset_cnt;
    uint16_t fault_start_cnt;
    uint16_t sys_ready_state;
    uint16_t sys_run_state;
    float  reactive_power;		//无功功率设置值
    float  active_power;		//有功功率设置值
    int16_t  run_mode;
}pcs_real_data_t;

typedef struct {
    uint8_t pcs_idx;                        //PCS下标
    uint8_t state;                          //PCS设备状态
    uint8_t use_state;                      //结构体使用状态
    uint8_t tobms_state;                    //下发给BMS的状态，0：初始 1：就绪 2：充电 3：放电 4：故障
    int8_t  chg_bat_idx;                    //PCS为充电或放电时连接的电池簇ID, -1:未连接
    uint8_t pcs_start_step;                 //PCS由停止状态转为充放电状态时所处的步骤数
    uint8_t pcs_stop_step;                  //PCS由充放电状态转为停止状态时所处的步骤数
    uint8_t pcs_search_bat_step;            //PCS查找下个待充放电电池簇时所处的步骤数
    uint8_t pcs_search_bat_idx;             //PCS查找下个待充放电电池簇时使用的数组下标
    uint8_t bms_work_done;                  //BMS notify charge or discharge done
    uint8_t hmi_start_stop;                 //界面控制PCS投运或停运
    uint8_t hmi_start_stop_done;            //界面控制PCS投运或停运操作是否完成
    dev_pcs_params_t dbcfg;
    //char parity;
    //int databit;
    //int stopbit;
    int16_t  power_reted;                   //额定功率
    int16_t  power_calc_pre;                //上一周期动态实时计算得到的功率值
    int16_t  power_calc;                    //动态实时计算得到的功率值
    uint16_t power_calc_u16;                //动态实时计算得到的功率值,unsigned, use to compare with rdata[P_SET]
    uint8_t  power_updated;                 //功率是否更新，0-不需更新；1-需更新

    float    tot_charge_kws;                //总充电电量，单位 kW * S, 千瓦秒
    float    tot_discharge_kws;             //总放电电量，单位 kW * S, 千瓦秒
    float    today_charge_kws;              //当日总充电量
    float    today_discharge_kws;           //当日总放电量

    uint16_t rdata[RDATA_NUM];              //从PCS读取到的数据(03功能)
    uint16_t wdata_current[WDATA_NUM];      //当前待写到PCS的数据(10功能)
    uint16_t wdata_last[WDATA_NUM];         //最后一次写到PCS的数据(10功能)
    pcs_real_data_t rtdata;                 //实时数据(真实物理值)

    ems_cont_t cont;
    battery_rack_t bat_rack[PCS_BAT_NUM];   //电池簇信息
}pcs_dev_t;

typedef struct {
    uint8_t state;                  //线程状态
    char dev_name[64];              //modbus-rtu 485设备名
    modbus_t* ctx;
    struct timeval def_rsp_tmov;    //默认等待响应超时时间，初始化后不应再修改
    struct timeval rsp_tmov;        //等待响应超时时间，用来设置新的超时时间
}pcs_comm_t;

extern pcs_comm_t g_pcs_comm;

void pcs_comm_init(void);
int create_pcs_comm_thread(void);
int pcs_comm_set_write_reg(uint8_t pcs_id, enum WDATA_IDX reg_idx, uint16_t val);

#endif //_PCS_COMM_H_
