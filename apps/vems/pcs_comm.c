/*******************************************************************************
 * @             Copyright (c) 2023 LinyPower, All Rights Reserved.
 * @*****************************************************************************
 * @FilePath     : pcs_comm.c
 * @Descripttion :
 * @Author       : xiaoyongjun
 * @E-Mail       : xiaoyongjun@linygroup.cn
 * @Version      : v1.0.0
 * @Date         : 2023-10-08 15:41:35
*******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "pcs_comm.h"
#include "debug_and_log.h"
#include "modbus.h"
#include "util.h"
#include "vems.h"

pcs_comm_t g_pcs_comm;
/*******************************************************************************
 * @brief 初始化内存块：main函数调用
 * @return
*******************************************************************************/
void pcs_comm_init(void)
{
    int i = 0;
    g_pcs_comm.ctx = NULL;
    g_pcs_comm.def_rsp_tmov.tv_sec = 1;
    g_pcs_comm.def_rsp_tmov.tv_usec = 0;
    g_pcs_comm.rsp_tmov.tv_sec = 1;
    g_pcs_comm.rsp_tmov.tv_usec = 0;
    strcpy(g_pcs_comm.dev_name, "/dev/ttyS1");

    for(i = 0; i < PCS_MAX; i++) {
        memset(g_vems.pcs[i].rdata, 0, sizeof(g_vems.pcs[i].rdata));
        memset(g_vems.pcs[i].wdata_current, 0, sizeof(g_vems.pcs[i].wdata_current));
        memset(g_vems.pcs[i].wdata_last, 0, sizeof(g_vems.pcs[i].wdata_last));
        g_vems.pcs[i].state = PCS_INITED;
    }
}

/*******************************************************************************
 * @brief 设置写寄存器的值
 * @param pcs_id 哪台PCS
 * @param reg_idx 待写入寄存器的地址
 * @param val 待写入寄存器的值
 * @return 设置成功：0，设置失败：1
*******************************************************************************/
int pcs_comm_set_write_reg(uint8_t pcs_id, enum WDATA_IDX reg_idx, uint16_t val)
{
    if(pcs_id >= PCS_MAX || reg_idx < W_IDX_MIN || reg_idx >= W_IDX_MAX)
        return 1;
    
    g_vems.pcs[pcs_id].wdata_current[reg_idx] = val;
    if(reg_idx == W_P_SET || reg_idx == W_Q_SET) {
        g_vems.pcs[pcs_id].wdata_last[reg_idx] = 1; //当设置有功功率寄存器时，把wdata_last当成写标志
    }
    return 0;
}

/*******************************************************************************
 * @brief 读取线圈状态值
 * @param pcs_id pcs编号
 * @return 读取成功：0，读取失败：-1
*******************************************************************************/
int pcs_comm_read_status_coil(uint8_t pcs_id)
{
    /*读取线圈状态量：开始地址：0，线圈个数：8*/
    int ret = 0;
    int saddr = 0;
    saddr = STATUS_START_ADDR;
    //state_bit0_7_t* pbits = &g_vems.pcs[pcs_id].rtdata.coil_state.bits;

    ret = modbus_read_input_bits_compact(g_pcs_comm.ctx, saddr, COIL_STATE_NUM, &g_vems.pcs[pcs_id].rtdata.coil_state.byte);
    if(ret != COIL_STATE_NUM) {
        LogPCS("pcs[%d] read coil failed: want read[%d] ret[%d, %s]", pcs_id, (COIL_STATE_NUM + 7) / 8, ret, modbus_strerror(errno));
        return -1;
    }
    // LogPCS("read %d coil value=%x, start_en=%d fault=%d run=%d stop=%d reset=%d phase_lock=%d reserve=%d ", COIL_STATE_NUM
    //       , g_vems.pcs[pcs_id].rtdata.coil_state.byte, pbits->start_enable, pbits->fault, pbits->run, pbits->stop, pbits->reset, pbits->phase_lock, pbits->reserve);

    return 0;
}

/*******************************************************************************
 * @brief 读取运行信息
 * @param pcs_id pcs编号
 * @return 读取成功：0，读取失败：-1
*******************************************************************************/
int pcs_comm_read_run_info_reg(uint8_t pcs_id)
{
    int ret = 0;
    int saddr = 0;
    uint16_t* prdata = g_vems.pcs[pcs_id].rdata;
    pcs_real_data_t* prtd = &g_vems.pcs[pcs_id].rtdata;

    /*读取运行信息：开始地址：720，寄存器个数：25*/
    saddr = RUN_INFO_START_ADDR;
    ret = modbus_read_registers(g_pcs_comm.ctx, saddr, RUN_INFO_REG_NUM, &prdata[R_IDX_RUN_INFO_BASE]);
    if(ret != RUN_INFO_REG_NUM) {
        LogPCS("pcs[%d] read run_info failed: want read[%d] ret[%d, %s]",pcs_id, RUN_INFO_REG_NUM, ret, modbus_strerror(errno));
        return -1;
    }
    prtd->grid_voltage = prdata[GRID_VOL] * COMM_DATA_RATIO;
    prtd->gird_freq = prdata[GRID_FREQ] * COMM_DATA_RATIO2;
    prtd->radiator_temp = (int16_t)prdata[RADIATOR_TEMP] * COMM_DATA_RATIO;
    prtd->dcbus_voltage = (int16_t)prdata[DCBUS_VOL];
    prtd->grid_ua = prdata[GRID_UA] * COMM_DATA_RATIO;
    prtd->grid_ub = prdata[GRID_UB] * COMM_DATA_RATIO;
    prtd->grid_uc = prdata[GRID_UC] * COMM_DATA_RATIO;
    prtd->pcs_ia = (int16_t)prdata[PCS_IA] * COMM_DATA_RATIO;
    prtd->pcs_ib = (int16_t)prdata[PCS_IB] * COMM_DATA_RATIO;
    prtd->pcs_ic = (int16_t)prdata[PCS_IC] * COMM_DATA_RATIO;
    prtd->pcs_pa = (int16_t)prdata[OUTPUT_PA] * COMM_DATA_RATIO;
    prtd->pcs_pb = (int16_t)prdata[OUTPUT_PB] * COMM_DATA_RATIO;
    prtd->pcs_pc = (int16_t)prdata[OUTPUT_PC] * COMM_DATA_RATIO;
    prtd->pcs_pall = prtd->pcs_pa + prtd->pcs_pb + prtd->pcs_pc;
    prtd->pcs_qa = (int16_t)prdata[OUTPUT_QA] * COMM_DATA_RATIO;
    prtd->pcs_qb = (int16_t)prdata[OUTPUT_QB] * COMM_DATA_RATIO;
    prtd->pcs_qc = (int16_t)prdata[OUTPUT_QC] * COMM_DATA_RATIO;
    prtd->pcs_qall = prtd->pcs_qa + prtd->pcs_qb + prtd->pcs_qc;
    prtd->pcs_sa = prdata[OUTPUT_SA] * COMM_DATA_RATIO;
    prtd->pcs_sb = prdata[OUTPUT_SB] * COMM_DATA_RATIO;
    prtd->pcs_sc = prdata[OUTPUT_SC] * COMM_DATA_RATIO;
    prtd->pcs_sall = prtd->pcs_sa + prtd->pcs_sb + prtd->pcs_sc;
    prtd->sys_err1 = prdata[SYSTEM_ERR1];
    prtd->sys_err2 = prdata[SYSTEM_ERR2];
    prtd->dc_current = (int16_t)prdata[DC_CURRENT];
    prtd->sys_state = prdata[SYS_STATE];
    prtd->dc_current_out = (int16_t)prdata[DC_CURRENT_OUT];
    prtd->grid_state = prdata[GRID_STATE];
    if(prtd->pcs_pa > 0) {
        g_vems.pcs[pcs_id].today_charge_kws += prtd->pcs_pall;
        g_vems.pcs[pcs_id].tot_charge_kws += prtd->pcs_pall;
    } else {
        g_vems.pcs[pcs_id].today_discharge_kws += -prtd->pcs_pall;
        g_vems.pcs[pcs_id].tot_discharge_kws += -prtd->pcs_pall;
    }

    // LogPCS("read run_info: Ug=%.1f Fg=%.2f T=%.1f Uga=%.1f Ugb=%.1f Ugc=%.1f Ia=%.1f Ib=%.1f Ic=%.1f"
    //       , prtd->grid_voltage, prtd->gird_freq, prtd->radiator_temp, prtd->grid_ua, prtd->grid_ub, prtd->grid_uc, prtd->pcs_ia, prtd->pcs_ib, prtd->pcs_ic);
    // LogPCS("read run_info: Pa=%.1f Pb=%.1f Pc=%.1f Qa=%.1f Qb=%.1f Qc=%.1f Sa=%.1f Sb=%.1f Sc=%.1f"
    //       , prtd->pcs_pa, prtd->pcs_pb, prtd->pcs_pc, prtd->pcs_qa, prtd->pcs_qb, prtd->pcs_qc, prtd->pcs_sa, prtd->pcs_sb, prtd->pcs_sc);
    // LogPCS("read run_info: Udc=%.1f Idc=%.1f Idc2=%.1f syserr1=0x%04X syserr2=0x%04X sys=0x%04X Gstat=%d"
    //       , prtd->dcbus_voltage, prtd->dc_current, prtd->dc_current_out, prtd->sys_err1, prtd->sys_err2, prtd->sys_state, prtd->grid_state);

    return 0;
}

/*******************************************************************************
 * @brief 读取启停复位功能参数
 * @param pcs_id pcs编号
 * @return 读取成功：0，读取失败：-1
*******************************************************************************/
int pcs_comm_read_run_mode_str_reg(uint8_t pcs_id)
{
    int ret = 0;
    int saddr = 0;
    uint16_t* prdata = g_vems.pcs[pcs_id].rdata;
    pcs_real_data_t* prtd = &g_vems.pcs[pcs_id].rtdata;

    /*读取启停复位信息：开始地址：2304，寄存器个数：12*/
    saddr = START_RESET_START_ADDR;
    ret = modbus_read_registers(g_pcs_comm.ctx, saddr, START_RESET_REG_NUM, &prdata[R_IDX_RSS_BASE]);
    if(ret != START_RESET_REG_NUM) {
        LogPCS("pcs[%d] read run_mode_str failed: want read[%d] ret[%d, %s]", pcs_id, START_RESET_REG_NUM, ret, modbus_strerror(errno));
        return -1;
    }

    prtd->start_str[0] = prdata[R_START1] & 0x00FF;
    prtd->start_str[1] = prdata[R_START2] >> 8;
    prtd->start_str[2] = prdata[R_START2] & 0x00FF;
    prtd->start_str[3] = 0;

    prtd->stop_str[0] = prdata[R_STOP1] & 0x00FF;
    prtd->stop_str[1] = prdata[R_STOP2] >> 8;
    prtd->stop_str[2] = prdata[R_STOP2] & 0x00FF;
    prtd->stop_str[3] = 0;

    prtd->reset_str[0] = prdata[R_RESET1] & 0x00FF;
    prtd->reset_str[1] = prdata[R_RESET2] >> 8;
    prtd->reset_str[2] = prdata[R_RESET2] & 0x00FF;
    prtd->reset_str[3] = 0;
    LogPCS("read run_mode_str: start[%s] stop[%s] reset[%s]", prtd->start_str, prtd->stop_str, prtd->reset_str);

    return 0;
}

/*******************************************************************************
 * @brief 读取故障复位、故障启动现场功能参数
 * @param pcs_id pcs编号
 * @return 读取成功：0，读取失败：-1
*******************************************************************************/
int pcs_comm_read_func_param(uint8_t pcs_id)
{
    int ret = 0;
    int saddr = 0;
    uint8_t byte_idx = 0;
    uint16_t* prdata = g_vems.pcs[pcs_id].rdata;
    pcs_real_data_t* prtd = &g_vems.pcs[pcs_id].rtdata;

    //read reg[66,67]
    saddr = FUN_PARA_START_ADDR_1;
    byte_idx = R_IDX_FUNC_PARAM_BASE;
    ret = modbus_read_registers(g_pcs_comm.ctx, saddr, FUN_PARA_REG_NUM_1, &prdata[byte_idx]);
    if(ret != FUN_PARA_REG_NUM_1) {
        LogPCS("pcs[%d] read func_param[66,67] failed: want read[%d] ret[%d, %s]", pcs_id, FUN_PARA_REG_NUM_1, ret, modbus_strerror(errno));
        return -1;
    }
    prtd->fault_reset_cnt = prdata[byte_idx];
    prtd->fault_start_cnt = prdata[byte_idx + 1];

    //read reg[126,127]
    saddr = FUN_PARA_START_ADDR_2;
    byte_idx = R_IDX_FUNC_PARAM_BASE + FUN_PARA_REG_NUM_1;
    ret = modbus_read_registers(g_pcs_comm.ctx, saddr, FUN_PARA_REG_NUM_2, &prdata[byte_idx]);
    if(ret != FUN_PARA_REG_NUM_2) {
        LogPCS("pcs[%d] read func_param[126,127] failed: want read[%d] ret[%d, %s]", pcs_id, FUN_PARA_REG_NUM_2, ret, modbus_strerror(errno));
        return -1;
    }
    prtd->sys_ready_state = prdata[byte_idx];
    prtd->sys_run_state = prdata[byte_idx + 1];

    //read reg[188,189]
    saddr = FUN_PARA_START_ADDR_3;
    byte_idx = R_IDX_FUNC_PARAM_BASE + FUN_PARA_REG_NUM_1 + FUN_PARA_REG_NUM_2;
    ret = modbus_read_registers(g_pcs_comm.ctx, saddr, FUN_PARA_REG_NUM_3, &prdata[byte_idx]);
    if(ret != FUN_PARA_REG_NUM_3) {
        LogPCS("pcs[%d] read func_param[188,189] failed: want read[%d] ret[%d, %s]", pcs_id, FUN_PARA_REG_NUM_3, ret, modbus_strerror(errno));
        return -1;
    }
    prtd->reactive_power = (int16_t)prdata[byte_idx] * COMM_DATA_RATIO;
    prtd->active_power = (int16_t)prdata[byte_idx + 1] * COMM_DATA_RATIO;

    //read reg[197]
    saddr = FUN_PARA_START_ADDR_4;
    byte_idx = R_IDX_FUNC_PARAM_BASE + FUN_PARA_REG_NUM_1 + FUN_PARA_REG_NUM_2 + FUN_PARA_REG_NUM_3;
    ret = modbus_read_registers(g_pcs_comm.ctx, saddr, FUN_PARA_REG_NUM_4, &prdata[byte_idx]);
    if(ret != FUN_PARA_REG_NUM_4) {
        LogPCS("pcs[%d] read func_param[197] failed: want read[%d] ret[%d, %s]", pcs_id, FUN_PARA_REG_NUM_4, ret, modbus_strerror(errno));
        return -1;
    }
    prtd->run_mode = prdata[byte_idx];

    // LogPCS("read func_param: fault_reset_cnt=%d fault_start_cnt=%d sys_ready=%d sys_run=%d Qset=%.1f,0x%04X Pset=%.1f,0x%04X run_mode=%d"
    //       , prtd->fault_reset_cnt, prtd->fault_start_cnt, prtd->sys_ready_state, prtd->sys_run_state
    //       , prtd->reactive_power, prdata[Q_SET], prtd->active_power, prdata[P_SET], prtd->run_mode);

    return 0;
}

/*******************************************************************************
 * @brief 设置启停复位功能参数
 * @param pcs_id pcs编号
 * @return 设置成功：0，设置失败：-1
*******************************************************************************/
int pcs_comm_write_run_mode_str_reg(uint8_t pcs_id)
{
    int ret = 0;
    static char scmd[][5] = { "", "\0RUN", "\0STP", "\0RST" };
    static uint16_t cmd[][2] = { {0, 0}, {0x0052, 0x554E}, {0x0053, 0x5450}, {0x0052, 0x5354} };   //equals {"", "\0RUN", "\0STP", "\0RST"} ASCII Code
    static uint16_t addr[] = { 0, REG_ADDR_START, REG_ADDR_STOP, REG_ADDR_RESET };
    uint8_t run_cmd = CMD_NONE;
    
    //run_cmd number set in 2304 reg, 
    if(g_vems.pcs[pcs_id].wdata_current[W_START1] == CMD_NONE) {
        return ret;
    } else {
        // has cmd need write to pcs
        run_cmd = g_vems.pcs[pcs_id].wdata_current[W_START1];
        if(run_cmd < CMD_START || run_cmd > CMD_RESET) {
            LogError("pcs_comm_write_run_mode_str_reg() unknown run_cmd[%d]", run_cmd);
            return 1;
        }

        g_pcs_comm.rsp_tmov.tv_sec = 0;
        g_pcs_comm.rsp_tmov.tv_usec = 500000;
        modbus_set_response_timeout(g_pcs_comm.ctx, &g_pcs_comm.rsp_tmov);
        ret = modbus_write_registers(g_pcs_comm.ctx, addr[run_cmd], 2, cmd[run_cmd]);
        if(ret != 2) {
            LogError("write pcs[%d] run cmd[%s] at reg[%d] failed: %d, %s", pcs_id, &scmd[run_cmd][1], addr[run_cmd], ret, modbus_strerror(errno));
            ret = 1;
        } else {
            LogInfo("write pcs[%d] run cmd[%s] at reg[%d] success", pcs_id, &scmd[run_cmd][1], addr[run_cmd]);
            g_vems.pcs[pcs_id].wdata_current[W_START1] = CMD_NONE;
            ret = 0;
        }
        modbus_get_response_timeout(g_pcs_comm.ctx, &g_pcs_comm.def_rsp_tmov);
    }

    return ret;
}
/*******************************************************************************
 * @brief 设置故障复位、故障启动次数现场功能参数
 * @param pcs_id pcs编号
 * @return 设置成功：0，设置失败：-1
*******************************************************************************/
int pcs_comm_write_site_func_param_reg(uint8_t pcs_id)
{
    /*设置现场功能参数1：开始地址：66，寄存器个数：2*/
    int i = 0;
    int ret = 0;
    int saddr = 0;
    uint8_t byte_idx = 0;
    
    saddr = FUN_PARA_START_ADDR_1;
    byte_idx = START_RESET_REG_NUM;
    for(i = byte_idx; i < byte_idx + FUN_PARA_REG_NUM_1; i++) {
        if(g_vems.pcs[pcs_id].wdata_current[i] != g_vems.pcs[pcs_id].wdata_last[i]) {
            ret = modbus_write_registers(g_pcs_comm.ctx, saddr, 1, &g_vems.pcs[pcs_id].wdata_current[i]);
            if(ret == 1) {
                g_vems.pcs[pcs_id].wdata_last[i] = g_vems.pcs[pcs_id].wdata_current[i];
                LogPCS("write pcs[%d] addr[%d]=%d success", pcs_id, saddr, g_vems.pcs[pcs_id].wdata_current[i]);
            } else {
                LogPCS("write pcs[%d] addr[%d]=%d failed, will try next times", pcs_id, saddr, g_vems.pcs[pcs_id].wdata_current[i]);
            }
        }
        saddr++;
    }
    
    return ret;
}
/*******************************************************************************
 * @brief 设置系统就绪、系统运行现场功能参数
 * @param pcs_id pcs编号
 * @return 设置成功：0，设置失败：-1
*******************************************************************************/
int pcs_data_collect_write_fun_para2_reg(uint8_t pcs_id)
{
    /*设置现场功能参数2：开始地址：126，寄存器个数：2*/
    int i = 0;
    int ret = 0;
    int saddr = 0;
    uint8_t byte_idx = 0;

    saddr = FUN_PARA_START_ADDR_2;
    byte_idx = START_RESET_REG_NUM + FUN_PARA_REG_NUM_1;
    for(i = byte_idx; i < byte_idx + FUN_PARA_REG_NUM_2; i++) {
        if(g_vems.pcs[pcs_id].wdata_current[i] != g_vems.pcs[pcs_id].wdata_last[i]) {
            g_vems.pcs[pcs_id].wdata_last[i] = g_vems.pcs[pcs_id].wdata_current[i];
            ret = modbus_write_registers(g_pcs_comm.ctx, saddr, 1, &g_vems.pcs[pcs_id].wdata_current[i]);
            LogPCS("write pcs[%d] addr[%d] %s, valve=%d\n", pcs_id, saddr, ret == 1 ? "success" : "failed", g_vems.pcs[pcs_id].wdata_current[i]);
        }
        saddr++;
    }
    return ret;
}
/*******************************************************************************
 * @brief 设置无功、有功功率现场功能参数
 * @param pcs_id pcs编号
 * @return 设置成功：0，设置失败：-1
*******************************************************************************/
int pcs_data_collect_write_fun_para3_reg(uint8_t pcs_id)
{
    /*设置现场功能参数3：开始地址：188，寄存器个数：2*/
    int i = 0;
    int ret = 0;
    int saddr = 0;
    uint8_t byte_idx = 0;

    saddr = FUN_PARA_START_ADDR_3;
    byte_idx = START_RESET_REG_NUM + FUN_PARA_REG_NUM_1 + FUN_PARA_REG_NUM_2;
    for(i = byte_idx; i < byte_idx + FUN_PARA_REG_NUM_3; i++) {
        if(g_vems.pcs[pcs_id].wdata_last[i] == 1) {
            //此处把wdata_last当成写标志位，若被置位，则将值写入PCS寄存器
            ret = modbus_write_registers(g_pcs_comm.ctx, saddr, 1, &g_vems.pcs[pcs_id].wdata_current[i]);
            if(ret == 1) {
                g_vems.pcs[pcs_id].wdata_last[i] = 0;   //重置写标志位
                LogPCS("write pcs[%d] addr[%d]=%d success", pcs_id, saddr, g_vems.pcs[pcs_id].wdata_current[i]);
            } else {
                LogPCS("write pcs[%d] addr[%d]=%d failed, will try next times", pcs_id, saddr, g_vems.pcs[pcs_id].wdata_current[i]);
            }
        }
        saddr++;
    }
    return ret;
}
/*******************************************************************************
 * @brief 设置运行模式现场功能参数
 * @param pcs_id pcs编号
 * @return 设置成功：0，设置失败：-1
*******************************************************************************/
int pcs_data_collect_write_fun_para4_reg(uint8_t pcs_id)
{
    /*设置现场功能参数4：开始地址：197，寄存器个数：1*/
    int i = 0;
    int ret = 0;
    int saddr = 0;
    uint8_t byte_idx = 0;

    saddr = FUN_PARA_START_ADDR_4;
    byte_idx = START_RESET_REG_NUM + FUN_PARA_REG_NUM_1 + FUN_PARA_REG_NUM_2 + FUN_PARA_REG_NUM_3;
    
    for(i = byte_idx; i < byte_idx + FUN_PARA_REG_NUM_4; i++) {
        if(g_vems.pcs[pcs_id].wdata_current[i] != g_vems.pcs[pcs_id].wdata_last[i]) {
            g_vems.pcs[pcs_id].wdata_last[i] = g_vems.pcs[pcs_id].wdata_current[i];
            ret = modbus_write_registers(g_pcs_comm.ctx, saddr, 1, &g_vems.pcs[pcs_id].wdata_current[i]);
            LogPCS("write pcs[%d] addr[%d] %s, valve=%d\n", pcs_id, saddr, ret == 1 ? "success" : "failed", g_vems.pcs[pcs_id].wdata_current[i]);
        }
        saddr++;
    }
    return ret;
}
/*******************************************************************************
 * @brief 处理pcs数据采集线程
 * @param
 * @return
*******************************************************************************/
static void* pcs_data_collect_thread_do(void* arg)
{
    int pcs_id = 0;
    // pcs_real_data_t* prtd = NULL;
    // state_bit0_7_t* pbits = NULL;

    pcs_comm_init();
    do {
        g_pcs_comm.ctx = modbus_new_rtu(g_pcs_comm.dev_name, g_vems.pcs[0].dbcfg.baud_rate, 'N', 8, 1);
        if(g_pcs_comm.ctx != NULL) {
            LogInfo("modbus_new_rtu(%s, %d, 'N', 8, 1) success", g_pcs_comm.dev_name, g_vems.pcs[0].dbcfg.baud_rate);
            break;
        } else {
            LogDebug("modbus_new_rtu(%s, %d, 'N', 8, 1) failed", g_pcs_comm.dev_name, g_vems.pcs[0].dbcfg.baud_rate);
        }
        safe_msleep(1000);
    } while(g_pcs_comm.ctx == NULL);
    modbus_set_debug(g_pcs_comm.ctx, g_vems.cfg.pcs_mb_debug);
    modbus_set_response_timeout(g_pcs_comm.ctx, &g_pcs_comm.def_rsp_tmov);
    modbus_set_error_recovery(g_pcs_comm.ctx, MODBUS_ERROR_RECOVERY_LINK | MODBUS_ERROR_RECOVERY_PROTOCOL);
    while(-1 == modbus_connect(g_pcs_comm.ctx)) {
        LogDebug("modbus_connect failed: %s", modbus_strerror(errno));
        sleep(3);
    }
    g_vems.th_stat.bits.th_pcs_comm = 1;
    LogInfo("pcs_data_collect thread modbus-rtu channel is ready, start working");

    while(1) {
        for(pcs_id = 0; pcs_id < g_vems.pcs_cnt; pcs_id++) {
            if(g_vems.pcs[pcs_id].use_state == ITEM_UNUSE)
                continue;
            modbus_set_slave(g_pcs_comm.ctx, g_vems.pcs[pcs_id].dbcfg.modbus_id);
            pcs_comm_read_status_coil(pcs_id);
            pcs_comm_read_run_info_reg(pcs_id);
            //pcs_comm_read_run_mode_str_reg(pcs_id);   //this part write only, no need read
            pcs_comm_read_func_param(pcs_id);
            // prtd = &g_vems.pcs[pcs_id].rtdata;
            // pbits = &g_vems.pcs[pcs_id].rtdata.coil_state.bits;
            // LogDebug("read pcs[%d] state: Sen=%d Flt=%d run=%d Stp=%d Plck=%d Serr1=0x%04X Serr2=0x%04X sys=0x%04X Gstat=%d Qset=%.1f Pset=%.1f"
            //         , pcs_id, pbits->start_enable, pbits->fault, pbits->run, pbits->stop, pbits->phase_lock, prtd->sys_err1, prtd->sys_err2
            //         , prtd->sys_state, prtd->grid_state, prtd->reactive_power, prtd->active_power);
          
            pcs_comm_write_run_mode_str_reg(pcs_id);
            pcs_comm_write_site_func_param_reg(pcs_id);
            pcs_data_collect_write_fun_para2_reg(pcs_id);
            pcs_data_collect_write_fun_para3_reg(pcs_id);
            pcs_data_collect_write_fun_para4_reg(pcs_id);
        }
        safe_msleep(1000);
    }
    
    return arg;
}
/*******************************************************************************
 * @brief  创建modbus工作线程：main 函数调用
 * @return 获取成功：0，失败：其他
*******************************************************************************/

int create_pcs_comm_thread(void)
{
    int ret = -1;
    pthread_t pcs_tid;
    pthread_attr_t pcs_tattr;

    pthread_attr_init(&pcs_tattr);
    pthread_attr_setdetachstate(&pcs_tattr, PTHREAD_CREATE_DETACHED);
    ret = pthread_create(&pcs_tid, &pcs_tattr, pcs_data_collect_thread_do, NULL);
    if(ret == 0) {
        LogInfo("create pcs data collect thread success");
    } else {
        LogError("create pcs data collect thread failed: %s", strerror(errno));
    }
    return 0;
}

