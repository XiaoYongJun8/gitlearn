/*******************************************************************************
 * @             Copyright (c) 2023 LinyPower, All Rights Reserved.
 * @*****************************************************************************
 * @FilePath     : energy_mana.c
 * @Descripttion :
 * @Author       : zhums
 * @E-Mail       : zhums@linygroup.cn
 * @Version      : v1.0.0
 * @Date         : 2023-10-18 09:11:55
*******************************************************************************/
#include <time.h>
#include <pthread.h>

#include "common.h"
#include "common_define.h"
#include "debug_and_log.h"
#include "energy_mana.h"
#include "pcs_comm.h"
#include "cont_comm.h"
#include "vems.h"

/*******************************************************************************
 * @brief
 * @param ppcs
 * @return -2: 正在查找处理中; -1: 没有需要充放电的电池簇; 0-3: 待充电的电池簇ID
*******************************************************************************/
int em_pcs_get_battery_id(pcs_dev_t* ppcs, enum BAT_CHG_STATE want_state)
{
    int ret = 0;

    if(ppcs->pcs_search_bat_step == PCS_SRCH_BAT_STEP_DONE) {
        ppcs->pcs_search_bat_idx = 0;    //首次查找，从0下表开始
        ppcs->pcs_search_bat_step = PCS_SRCH_BAT_STEP_INIT;
        //LogEMANA("pcs[%d] start new times search charge_discharge battery", ppcs->pcs_idx);
    }

    //读取充电枪连接状态，选择未充满的电池簇进行充电
    //依次读取各电池簇SOC，判断是否需要充电
    while(ppcs->pcs_search_bat_idx < PCS_BAT_NUM) {
        if(ppcs->pcs_search_bat_step == PCS_SRCH_BAT_STEP_INIT) {
            if(ppcs->bat_rack[ppcs->pcs_search_bat_idx].conn_state != BAT_CONNECTED) {
                ppcs->pcs_search_bat_idx++;
                continue;
            } else {
                ppcs->pcs_search_bat_step = PCS_SRCH_BAT_STEP1;
            }
        }

        //对于已插枪的电池簇
        //1. 切换BMS CAN并发送心跳报文激活对应的BMS
        if(ppcs->pcs_search_bat_step == PCS_SRCH_BAT_STEP1) {
            //切换BMS CAN
            ret = cont_comm_bms_can_ctrl(&ppcs->cont, ppcs->pcs_search_bat_idx + 1);
            if(ret != 0) {
                LogError("pcs[%d] do bms_can_ctrl to bms[%d] failed: %s, will try next times", ppcs->pcs_idx, ppcs->pcs_search_bat_idx, strerror(errno));
                ret = -2;
                break;
            } else {
                LogInfo("pcs[%d] do bms_can_ctrl to bms[%d] success, wait cont confirm", ppcs->pcs_idx, ppcs->pcs_search_bat_idx);
            }
            //确认BMS CAN切换是否成功
            if(ppcs->cont.rtdata.can_stat.stat.conn_id - 1 == ppcs->pcs_search_bat_idx) {
                LogInfo("pcs[%d] do bms_can_ctrl to bms[%d] is comfirmed, wait BMS active", ppcs->pcs_idx, ppcs->pcs_search_bat_idx);
                ppcs->pcs_search_bat_step = PCS_SRCH_BAT_STEP2;
            } else {
                LogEMANA("pcs[%d] do bms_can_ctrl to bms[%d] not comfirm, will check next times", ppcs->pcs_idx, ppcs->pcs_search_bat_idx);
                ret = -2;
                break;
            }
        }

        //2. 等待BMS激活并上报数据，并根据BMS上报的数据判断当前电池簇是否已充满或放空，若否则选择这个电池簇进行充放电
        if(ppcs->pcs_search_bat_step == PCS_SRCH_BAT_STEP2) {
            if(ppcs->bat_rack[ppcs->pcs_search_bat_idx].act_state == BMS_ACTIVED) {
                if(ppcs->bat_rack[ppcs->pcs_search_bat_idx].rtdata.bat_chg_dischg_done == 0) {
                    LogEMANA("pcs[%d] battery[%d] bms is actived but bat_chg_dischg_done flag is unknown, will check next times"
                            , ppcs->pcs_idx, ppcs->pcs_search_bat_idx);
                    ret = -2;
                    break;
                } else if(ppcs->bat_rack[ppcs->pcs_search_bat_idx].rtdata.bat_chg_dischg_done == 1) {
                    ret = ppcs->pcs_search_bat_idx;
                    ppcs->pcs_search_bat_step = PCS_SRCH_BAT_STEP_DONE;
                    LogInfo("pcs[%d] get battery[%d] to do charge_discharge");
                    LogEMANA("pcs[%d] search charge_discharge battery done", ppcs->pcs_idx);
                    break;
                } else {
                    LogEMANA("pcs[%d] battery[%d] BMS become active, but this battery no need charge or discharge", ppcs->pcs_idx, ppcs->pcs_search_bat_idx);
                    ppcs->pcs_search_bat_idx++;
                    ppcs->pcs_search_bat_step = PCS_SRCH_BAT_STEP_INIT;
                }
            } else {
                LogEMANA("pcs[%d] battery[%d] bms not active, wait it active", ppcs->pcs_idx, ppcs->pcs_search_bat_idx);
                ret = -2;
                break;
            }
        }
    }
    if(ppcs->pcs_search_bat_idx >= PCS_BAT_NUM) {
        ppcs->pcs_search_bat_step = PCS_SRCH_BAT_STEP_DONE;
        ret = -1;
    }

    return ret;
}

int em_pcs_set_power(pcs_dev_t* ppcs)
{
    int ret = 0;
    
    if(ppcs->power_updated == 0) {
        return ret;
    }    
    ret = pcs_comm_set_write_reg(ppcs->pcs_idx, W_P_SET, ppcs->power_calc);
    if(ret == 0) {
        LogInfo("pcs[%d] update active power reg[%d] success, pcs power will change to[%.1f]", ppcs->pcs_idx, W_P_SET, ppcs->power_calc * 0.1);
    } else {
        //失败不更新power_updated标志，待下个周期重试
        LogWarn("pcs[%d] update active power to[%.1f] failed", ppcs->pcs_idx, ppcs->power_calc * 0.1);
    }

    return ret;
}

/*******************************************************************************
 * @brief PCS运行状态设置,todo: change to set wdata mode
 * @param pcs_id
 * @param run_cmd
 * @return 0:success; 1: failed
*******************************************************************************/
int em_pcs_run_ctrl(int pcs_id, uint8_t run_cmd)
{
    int ret = 0;
    
    ret = pcs_comm_set_write_reg(pcs_id, W_START1, run_cmd);
    if(ret == 0) {
        LogInfo("update pcs[%d] start_stop_reset wdata[%d]=%d success, wait pcs_comm thread send to pcs module", pcs_id, W_START1, run_cmd);
    } else {
        //失败不更新power_updated标志，待下个周期重试
        LogWarn("update pcs[%d] start_stop_reset wdata[%d]=%d failed, will try next times", pcs_id, W_START1, run_cmd);
        ret = 1;
    }
    
    return ret;
}

/*******************************************************************************
 * @brief 检测PCS充放电状态控制运行和故障指示灯
 * @param ppcs
 * @return 
*******************************************************************************/
void em_pcs_check_ctrl_io(pcs_dev_t* ppcs)
{
    uint8_t io_val = 0;

    if((ppcs->state == PCS_CHARGE_BAT || ppcs->state == PCS_DISCHARGE_BAT)) {
        if(ppcs->cont.rtdata.dio_stat.bits.led_run == 0) {
            //PCS 充放电中，点亮运行灯
            cont_dio_setbit_var(io_val, IO_LED_RUN_LIGHT);
            cont_comm_io_ctrl(&ppcs->cont, io_val, EMS_RUNNING);
        }
    } else {
        if(ppcs->cont.rtdata.dio_stat.bits.led_run == 1) {
            //PCS 非充放电，熄灭运行灯
            cont_dio_setbit_var(io_val, IO_LED_RUN_CLOSE);
            cont_comm_io_ctrl(&ppcs->cont, io_val, EMS_STANDBY);
        }
    }

    //故障灯控制
    if(ppcs->state == PCS_FAULT) {
        if(ppcs->cont.rtdata.dio_stat.bits.led_fault == 0) {
            //PCS 故障中，点亮故障灯
            cont_dio_setbit_var(io_val, IO_LED_ERR_LIGHT);
            cont_comm_io_ctrl(&ppcs->cont, io_val, EMS_ERROR);
        }
    } else {
        if(ppcs->cont.rtdata.dio_stat.bits.led_fault == 1) {
            //PCS 无故障，熄灭故障灯
            cont_dio_setbit_var(io_val, IO_LED_ERR_CLOSE);
            cont_comm_io_ctrl(&ppcs->cont, io_val, EMS_STANDBY);
        }
    }
}

void em_pcs_sync_start_stop_to_hmi(pcs_dev_t* ppcs, uint8_t start_stop_done_type)
{
    if(start_stop_done_type == HMI_CTRL_PCS_START) {
        ppcs->hmi_start_stop = HMI_CTRL_PCS_START;
        ppcs->hmi_start_stop_done = HMI_CTRL_PCS_START_DONE;
        LogInfo("pcs[%d] now in run mode, sync to hmi", ppcs->pcs_idx);
    } else if(start_stop_done_type == HMI_CTRL_PCS_STOP) {
        ppcs->hmi_start_stop = HMI_CTRL_PCS_STOP;
        ppcs->hmi_start_stop_done = HMI_CTRL_PCS_STOP_DONE;
        LogInfo("pcs[%d] now in stop mode, sync to hmi", ppcs->pcs_idx);
    }
}

int em_pcs_reset_cont_relay_and_bms_can(pcs_dev_t* ppcs)
{
    int ret = 0;
    int8_t reset_bat_idx = 0;

    //切换BMS CAN
    ret = cont_comm_bms_can_ctrl(&ppcs->cont, reset_bat_idx + 1);
    if(ret != 0) {
        LogError("pcs[%d] do bms_can_ctrl to bms[%d] failed: %s, will try next times", ppcs->pcs_idx, reset_bat_idx, strerror(errno));
        return 1;
    } else {
        LogInfo("pcs[%d] do bms_can_ctrl to bms[%d] success, wait cont confirm", ppcs->pcs_idx, reset_bat_idx);
    }
    //确认BMS CAN切换是否成功
    if(ppcs->cont.rtdata.can_stat.stat.conn_id - 1 == reset_bat_idx) {
        LogInfo("pcs[%d] do bms_can_ctrl to bms[%d] is comfirmed, wait BMS active", ppcs->pcs_idx, reset_bat_idx);
    } else {
        LogEMANA("pcs[%d] do bms_can_ctrl to bms[%d] not comfirm, will check next times", ppcs->pcs_idx, reset_bat_idx);
        return 1;
    }

    //22.1 断路器控制，断开所有投切断路器
    ret = cont_comm_relay_ctrl(&ppcs->cont, 0, RELAY_OPEN);
    if(ret != 0) {
        LogError("pcs[%d] set all relay to open failed: %s, will try next times", ppcs->pcs_idx, strerror(errno));
        return 1;
    } else {
        LogInfo("pcs[%d] set all relay to open success", ppcs->pcs_idx);
    }
    //22.2 根据继电器反馈状态判断继电器是否已闭合
    if(ppcs->cont.rtdata.relay_stat_fb.stat.close_id == reset_bat_idx) {
        LogInfo("pcs[%d] ctrl all relay open has confirmed", ppcs->pcs_idx);
    } else {
        LogEMANA("pcs[%d] ctrl all relay open not confirm, wait confirm", ppcs->pcs_idx);
        return 1;
    }

    return 0;
}

/*******************************************************************************
 * @brief 根据PCS采集值更新PCS运行状态
 * @param pcs_id 哪台PCS
 * @return 0: success; 1: failed
*******************************************************************************/
int em_pcs_check_update_pcs_run_state(pcs_dev_t* ppcs)
{
    pcs_real_data_t* prtd = &ppcs->rtdata;

    //prtd->coil_state.bits.stop = 1; //do test
    //coil.reset no need care
    if(prtd->coil_state.bits.fault && ppcs->state != PCS_FAULT) {
        LogInfo("pcs[%d] fault bit is set, change pcs state[%d-->%d]", ppcs->pcs_idx, ppcs->state, PCS_FAULT);
        ppcs->state = PCS_FAULT;
    } else if(prtd->coil_state.bits.stop && ppcs->state != PCS_STOPPED && ppcs->state != PCS_HMI_STOPPED
        && ppcs->state != PCS_HMI_STOPPING && ppcs->pcs_stop_step == PCS_STOP_STEP_DONE) {
        LogInfo("pcs[%d] stop bit is set, change pcs state[%d-->%d]", ppcs->pcs_idx, ppcs->state, PCS_STOPPED);
        ppcs->state = PCS_STOPPED;
    } /*else if(prtd->coil_state.bits.start_enable && ppcs->state != PCS_STANDBY) {
        LogInfo("pcs[%d] start_enable bit is set, change pcs state[%d-->%d]", ppcs->pcs_idx, ppcs->state, PCS_STANDBY);
        ppcs->state = PCS_STANDBY;
    }*/ else if(prtd->coil_state.bits.run && (ppcs->state == PCS_STOPPED || ppcs->state == PCS_INITED)
             && ppcs->pcs_start_step == PCS_START_STEP_DONE && ppcs->pcs_stop_step == PCS_STOP_STEP_DONE) {
        //ppcs->state != PCS_CHARGE_BAT && ppcs->state != PCS_DISCHARGE_BAT && ppcs->state != PCS_HMI_STOPPING && ppcs->state != PCS_HMI_STOPPED
        if(prtd->active_power > POWER_FLOAT_MIN) {
            ppcs->state = PCS_CHARGE_BAT;
            em_pcs_check_ctrl_io(ppcs);
            em_pcs_sync_start_stop_to_hmi(ppcs, HMI_CTRL_PCS_START_DONE);
            LogInfo("pcs[%d] already in charge run mode, keep it run ok", ppcs->pcs_idx);
        } else if(prtd->active_power < -POWER_FLOAT_MIN) {
            ppcs->state = PCS_DISCHARGE_BAT;
            em_pcs_check_ctrl_io(ppcs);
            em_pcs_sync_start_stop_to_hmi(ppcs, HMI_CTRL_PCS_START_DONE);
            LogInfo("pcs[%d] already in discharge run mode, keep it run ok", ppcs->pcs_idx);
        } else {
            LogInfo("pcs[%d] active_power=%.1f is invalid, now need stop this pcs", ppcs->pcs_idx, prtd->active_power);
            ppcs->state = PCS_ERR_STOPPING;
        }
    }

    return 0;
}

/*******************************************************************************
 * @brief 处理PCS由停机转为运行状态
 * @param ppcs pcs设备对象
 * @param bat_chg_state 期望的充放电状态
 * @return 0: 步骤已完成; 1: 当前步骤未完成
*******************************************************************************/
int em_strategy_handle_pcs_stop_to_run(pcs_dev_t* ppcs, enum BAT_CHG_STATE bat_chg_state)
{
    int ret = 0;
    int chg_bat_idx = 0;
    
    //获取待充电电池簇ID
    if(ppcs->pcs_start_step == PCS_START_STEP1 || ppcs->pcs_start_step == PCS_START_STEP_INIT) {
        if(-POWER_INT_MIN <= ppcs->power_calc && ppcs->power_calc <= POWER_INT_MIN) {
            //当前PCS待设置功率为0时不启动PCS
            LogDebug("pcs[%d] power_calc=%d is invalid, will not start", ppcs->pcs_idx, ppcs->power_calc);
            return 1;
        }

        chg_bat_idx = em_pcs_get_battery_id(ppcs, bat_chg_state);
        if(chg_bat_idx == -1) {
            //LogEMANA("pcs[%d] no need charge battery", ppcs->pcs_idx);
            if(ppcs->cont.rtdata.relay_stat_fb.stat.close_id != 0) {
                em_pcs_reset_cont_relay_and_bms_can(ppcs);
                ppcs->chg_bat_idx = chg_bat_idx;
            }
            return 1;
        } else if(chg_bat_idx >= 0) {
            ppcs->chg_bat_idx = chg_bat_idx;
            ppcs->pcs_start_step = PCS_START_STEP22;    //STEP21在查找待充放电电池簇时已经做过了，这里不再重复做
        }
    } else {
        chg_bat_idx = ppcs->chg_bat_idx;
    }

    //对需要充电的电池簇准备进行充电
    if(ppcs->pcs_start_step == PCS_START_STEP21) {
        //切换BMS CAN
        ret = cont_comm_bms_can_ctrl(&ppcs->cont, chg_bat_idx + 1);
        if(ret != 0) {
            LogError("pcs[%d] switch to bms can[%d] failed: %s, will try next times", ppcs->pcs_idx, chg_bat_idx, strerror(errno));
            return 1;
        } else {
            LogInfo("pcs[%d] switch to bms can[%d] success", ppcs->pcs_idx, chg_bat_idx);
        }

        //确认BMS CAN切换是否成功
        if(ppcs->cont.rtdata.can_stat.stat.conn_id - 1 == chg_bat_idx) {
            //ppcs->pcs_start_step = PCS_START_STEP22;
            LogInfo("pcs[%d] switch to bms can[%d] conn_id is comfirmed", ppcs->pcs_idx, chg_bat_idx);
        } else {
            LogEMANA("pcs[%d] switch to bms can[%d] conn_id not comfirmed, will check next times", ppcs->pcs_idx, chg_bat_idx);
            return 1;
        }

        //确认切换后是否收到BMS数据
        safe_msleep(500);
        if(g_vems.can_nodata_sec == 0) {
            ppcs->pcs_start_step = PCS_START_STEP22;
            LogInfo("pcs[%d] switch to bms can[%d] bms msg has received, all has comfirmed", ppcs->pcs_idx, chg_bat_idx, bat_chg_state);
        } else {
            LogEMANA("pcs[%d] switch to bms can[%d] bms msg not received, will check next times", ppcs->pcs_idx, chg_bat_idx);
            return 1;
        }
    }
    if(ppcs->pcs_start_step == PCS_START_STEP22) {
        //22.1 断路器控制，断开其他KM并闭合KMx
        ret = cont_comm_relay_ctrl(&ppcs->cont, chg_bat_idx + 1, RELAY_CLOSE);
        if(ret != 0) {
            LogError("pcs[%d] set relay[%d] to close failed: %s, will try next times", ppcs->pcs_idx, chg_bat_idx, strerror(errno));
            return 1;
        } else {
            LogInfo("pcs[%d] set relay[%d] to close success", ppcs->pcs_idx, chg_bat_idx);
        }
        //22.2 根据继电器反馈状态判断继电器是否已闭合
        if(ppcs->cont.rtdata.relay_stat_fb.stat.close_id - 1 == chg_bat_idx) {
            ppcs->bat_rack[chg_bat_idx].chg_state = bat_chg_state;
            ppcs->pcs_start_step = PCS_START_STEP31;
            LogInfo("pcs[%d] has control relay[%d] close, set rack chg_state=%d", ppcs->pcs_idx, chg_bat_idx, bat_chg_state);
        } else {
            LogEMANA("pcs[%d] want %s rack[%d] relay not close, wait close ok", ppcs->pcs_idx, bat_chg_state == BAT_CHARGE ? "charge" : "discharge", chg_bat_idx);
            return 1;
        }
    }

    //2. 设置PCS充电功率
    if(ppcs->pcs_start_step == PCS_START_STEP31) {
        ret = em_pcs_set_power(ppcs);
        if(ret != 0) {
            LogError("pcs[%d] set start power failed", ppcs->pcs_idx);
            return 1;
        }
        ppcs->pcs_start_step = PCS_START_STEP32;
    }
    if(ppcs->pcs_start_step == PCS_START_STEP32) {
        //判断功率是否设置成功
        LogEMANA("pcs[%d] check set power is take effect, want[%d, %04X] read[%d, %04X]", ppcs->pcs_idx, ppcs->power_calc_u16, ppcs->power_calc_u16
                , ppcs->rdata[P_SET], ppcs->rdata[P_SET]);
        if(ppcs->rdata[P_SET] == ppcs->power_calc_u16) {
            ppcs->power_updated = 0;
            ppcs->pcs_start_step = PCS_START_STEP41;         //未接电池测试时改为STEP42, 正常使用时要改回STEP41
            LogInfo("pcs[%d] set active_power[%d, %04X] take effect, set success", ppcs->pcs_idx, ppcs->power_calc, ppcs->power_calc_u16);
        } else {
            LogEMANA("pcs[%d] set power[%d] not confirm, wait confirm ok", ppcs->pcs_idx, ppcs->power_calc);
            return 1;
        }
    }

    if(ppcs->pcs_start_step == PCS_START_STEP41) {
        ret = em_pcs_run_ctrl(ppcs->pcs_idx, CMD_START);
        if(ret != 0) {
            LogWarn("control pcs[%d] to run state failed", ppcs->pcs_idx);
            return 1;
        }
        ppcs->pcs_start_step = PCS_START_STEP42;
    }
    if(ppcs->pcs_start_step == PCS_START_STEP42) {
        //do check pcs noww in run or not
        LogEMANA("pcs[%d] check run_ctrl cmd is take effet, coil_run=%d", ppcs->pcs_idx, ppcs->rtdata.coil_state.bits.run);
        if(ppcs->rtdata.coil_state.bits.run) {
            ppcs->state = bat_chg_state == BAT_CHARGE ? PCS_CHARGE_BAT : PCS_DISCHARGE_BAT;
            ppcs->chg_bat_idx = chg_bat_idx;
            ppcs->pcs_start_step = PCS_START_STEP_DONE;
            em_pcs_check_ctrl_io(ppcs);
            LogMust("pcs[%d] start %s on battery[%d] at power[%.1f] success", ppcs->pcs_idx, bat_chg_state == BAT_CHARGE ? "charge" : "discharge"
                  , chg_bat_idx, ppcs->rtdata.active_power);

            em_pcs_sync_start_stop_to_hmi(ppcs, HMI_CTRL_PCS_START_DONE);
        } else {
            ret = 1;
        }
    }

    return ret;
}

/*******************************************************************************
 * @brief 控制PCS按平滑功率停机
 * @param ppcs
 * @param bat_chg_state
 * @return 1: 操作未完成; 0: 操作已完成
*******************************************************************************/
int em_strategy_handle_pcs_run_to_stop_smoothly(pcs_dev_t* ppcs, enum BAT_CHG_STATE bat_chg_state)
{
    int ret = 0;
    int16_t power_calc_now = 0;

    //平滑降功率至小于10kW
    if(ppcs->pcs_stop_step == PCS_STOP_STEP11 || ppcs->pcs_stop_step == PCS_STOP_STEP_INIT) {
        ppcs->power_updated = 0;
        ppcs->pcs_stop_step = PCS_STOP_STEP12;
    }
    if(ppcs->pcs_stop_step == PCS_STOP_STEP12) {
        while(abs(ppcs->rtdata.active_power) > 10) {
            if(ppcs->power_updated == 0) {
                //第一次或上一次功率调整已完成
                if(ppcs->power_calc > 500) {
                    //50kW以上功率每次降30%
                    power_calc_now = ppcs->power_calc * 0.7;
                } else {
                    //50kW以下功率每次降50%
                    power_calc_now = ppcs->power_calc * 0.5;
                }
                ppcs->power_updated = 1;
                ppcs->power_calc_pre = ppcs->power_calc;
                ppcs->power_calc_u16 = power_calc_now;
                ppcs->power_calc = power_calc_now;
            }
            
            em_pcs_set_power(ppcs);
            if(ppcs->rdata[P_SET] == ppcs->power_calc_u16) {
                ppcs->power_updated = 0;
                LogInfo("pcs[%d] smooth_stop set active_power=%d done", ppcs->pcs_idx, ppcs->power_calc);
                ret = 0;
            } else {
                LogEMANA("pcs[%d] smooth_stop set active_power=%d not effect, will check next times", ppcs->pcs_idx);
                ret = 1;
                break;
            }
        }
        if(ret == 0) {
            //平滑降功率结束，开始停机操作
            ppcs->pcs_stop_step = PCS_STOP_STEP21;
            LogInfo("pcs[%d] smooth_stop falling power control finish, last set active_power=%dkW", ppcs->pcs_idx, ppcs->power_calc);
        }
    }

    if(ppcs->pcs_stop_step == PCS_STOP_STEP21) {
        ret = em_pcs_run_ctrl(ppcs->pcs_idx, CMD_STOP);
        if(ret != 0) {
            return 1;
        }
        ppcs->pcs_stop_step = PCS_STOP_STEP22;
    }
    if(ppcs->pcs_stop_step == PCS_STOP_STEP22) {
        //do check pcs noww in run or not
        LogEMANA("pcs[%d] check stop_ctrl cmd is take effet, coil_stop=%d", ppcs->pcs_idx, ppcs->rtdata.coil_state.bits.stop);
        if(ppcs->rtdata.coil_state.bits.stop) {
            ppcs->pcs_stop_step = PCS_STOP_STEP2;   //PCS_STOP_STEP21
            LogInfo("pcs[%d] stop bit is set, stop run_ctrl ok");
        }
    }

    //清有功给定值
    if(ppcs->pcs_stop_step == PCS_STOP_STEP2) { //PCS_STOP_STEP21
        ppcs->power_updated = 1;
        ppcs->power_calc = 0;
        ret = em_pcs_set_power(ppcs);
        if(ret != 0) {
            return 1;
        }
        //ppcs->pcs_stop_step = PCS_STOP_STEP22;
    }
    if(ppcs->pcs_stop_step == PCS_STOP_STEP2) { //PCS_STOP_STEP22
        if(ppcs->rdata[P_SET] == 0) {
            ppcs->power_updated = 0;
            ppcs->pcs_stop_step = PCS_STOP_STEP3;
            LogInfo("pcs[%d] active_power set value clear ok", ppcs->pcs_idx);
        } else {
            LogEMANA("pcs[%d] active_power set value clear not effect, will check next times", ppcs->pcs_idx);
            return 1;
        }
    }

    //复位直流继电器和BMS CAN
    if(ppcs->pcs_stop_step == PCS_STOP_STEP3) {
        if(em_pcs_reset_cont_relay_and_bms_can(ppcs) == 0) {
            ppcs->bms_work_done = 0;
            if(ppcs->state != PCS_HMI_STOPPING) {
                ppcs->state = PCS_STOPPED;
            }
            ppcs->pcs_stop_step = PCS_STOP_STEP_DONE;
            em_pcs_check_ctrl_io(ppcs);
            em_pcs_sync_start_stop_to_hmi(ppcs, HMI_CTRL_PCS_STOP_DONE);
            LogMust("pcs[%d] stop %s on battery[%d] success", ppcs->pcs_idx, bat_chg_state == BAT_CHARGE ? "charge" : "discharge", ppcs->chg_bat_idx);
            ppcs->chg_bat_idx = 0;
        }
    }

    return ret;
}

/*******************************************************************************
 * @brief 控制PCS立即停机
 * @param ppcs
 * @param bat_chg_state
 * @return 1: 操作未完成; 0: 操作已完成
*******************************************************************************/
int em_strategy_handle_pcs_run_to_stop_immediately(pcs_dev_t* ppcs, enum BAT_CHG_STATE bat_chg_state)
{
    int ret = 0;

    if(ppcs->pcs_stop_step == PCS_STOP_STEP11 || ppcs->pcs_stop_step == PCS_STOP_STEP_INIT) {
        ret = em_pcs_run_ctrl(ppcs->pcs_idx, CMD_STOP);
        if(ret != 0) {
            return 1;
        }
        ppcs->pcs_stop_step = PCS_STOP_STEP12;
    }
    if(ppcs->pcs_stop_step == PCS_STOP_STEP12) {
        //do check pcs noww in run or not
        LogEMANA("pcs[%d] check stop_ctrl cmd is take effet, coil_stop=%d", ppcs->pcs_idx, ppcs->rtdata.coil_state.bits.stop);
        if(ppcs->rtdata.coil_state.bits.stop) {
            ppcs->pcs_stop_step = PCS_STOP_STEP2;   //PCS_STOP_STEP21
            LogInfo("pcs[%d] stop bit is set, stop run_ctrl ok");
        }
    }

    //清有功给定值
    if(ppcs->pcs_stop_step == PCS_STOP_STEP2) { //PCS_STOP_STEP21
        ppcs->power_updated = 1;
        ppcs->power_calc = 0;
        ret = em_pcs_set_power(ppcs);
        if(ret != 0) {
            return 1;
        }
        //ppcs->pcs_stop_step = PCS_STOP_STEP22;
    }
    if(ppcs->pcs_stop_step == PCS_STOP_STEP2) { //PCS_STOP_STEP22
        if(ppcs->rdata[P_SET] == 0) {
            ppcs->power_updated = 0;
            ppcs->pcs_stop_step = PCS_STOP_STEP3;
            LogInfo("pcs[%d] active_power set value clear ok", ppcs->pcs_idx);
        } else {
            LogEMANA("pcs[%d] active_power set value clear not effect, will check next times", ppcs->pcs_idx);
            return 1;
        }
    }

    //复位直流继电器和BMS CAN
    if(ppcs->pcs_stop_step == PCS_STOP_STEP3) {
        if(em_pcs_reset_cont_relay_and_bms_can(ppcs) == 0) {
            ppcs->bms_work_done = 0;
            if(ppcs->state != PCS_HMI_STOPPING) {
                ppcs->state = PCS_STOPPED;
            }
            ppcs->pcs_stop_step = PCS_STOP_STEP_DONE;
            em_pcs_check_ctrl_io(ppcs);
            em_pcs_sync_start_stop_to_hmi(ppcs, HMI_CTRL_PCS_STOP_DONE);
            LogMust("pcs[%d] stop %s on battery[%d] success", ppcs->pcs_idx, bat_chg_state == BAT_CHARGE ? "charge" : "discharge", ppcs->chg_bat_idx);
            ppcs->chg_bat_idx = 0;
        }
    }

    return ret;
}

/*******************************************************************************
 * @brief 
 * @param ppcs
 * @param ptmsp
 * @return 0: timespan is match pcs state; 1: not match
*******************************************************************************/
int em_strategy_check_tmsp_with_pcs_state(pcs_dev_t* ppcs, ems_tmsp_item_t* ptmsp)
{
    int ret = 0;

    if((ptmsp->type == BAT_CHARGE && ppcs->state == PCS_DISCHARGE_BAT)
    || (ptmsp->type == BAT_DISCHARGE && ppcs->state == PCS_CHARGE_BAT)) {
        //pcs state not match charge or discharge timespan, need stop pcs
        LogInfo("pcs[%d] state=%d not match %s timespan, need stop. stop_step=%d"
               , ppcs->pcs_idx, ppcs->state, ptmsp->type == BAT_CHARGE ? "charge" : "discharge", ppcs->pcs_stop_step);
        if(ppcs->pcs_stop_step == PCS_STOP_STEP_DONE) {
            ppcs->pcs_stop_step = PCS_STOP_STEP_INIT;
        }
        em_strategy_handle_pcs_run_to_stop_immediately(ppcs, ptmsp->type);
        ret = 1;
    }

    return ret;
}

/*******************************************************************************
 * @brief 检测是否在触摸屏上操作了投运。停运按钮，并做相应处理
 * @param ppcs
 * @param ptmsp
 * @return 0: 未检测到在触摸屏上做了投运、停运操作; 1: 正在处理投运、停运操作
*******************************************************************************/
int em_strategy_check_handle_hmi_ctrl_start_stop(pcs_dev_t* ppcs, ems_tmsp_item_t* ptmsp)
{
    int ret = 0;

    if(ppcs->state == PCS_HMI_STOPPED) {
        if(ppcs->hmi_start_stop == HMI_CTRL_PCS_START && ppcs->hmi_start_stop_done == HMI_CTRL_PCS_NONE_DONE) {
            //在屏幕上操作让当前PCS投运，将state置为PCS_STOPPED，开始执行后续状态机直至转换为对电池充放电状态
            ppcs->state = PCS_STOPPED;
            ret = 0;    //需要继续执行后续的停机到运行转换处理
        } else {
            //已停运且未重新投运，继续保持停运状态
            ret = 1;
        }
    } else if(ppcs->state == PCS_STOPPED) {
        if(ppcs->hmi_start_stop == HMI_CTRL_PCS_STOP && ppcs->hmi_start_stop_done == HMI_CTRL_PCS_NONE_DONE) {
            //在屏幕上操作让当前PCS停运
            if(ppcs->pcs_start_step == PCS_START_STEP_INIT) {
                //当前状态为停机且未在停机转运行中，则直接转变为HMI_STOPPED状态
                ppcs->state = PCS_HMI_STOPPED;
                //后台响应前台界面控制PCS停运完成，通知前台界面
                ppcs->hmi_start_stop_done = HMI_CTRL_PCS_STOP_DONE;
                LogInfo("pcs[%d] hmi control to stopped", ppcs->pcs_idx);
            } else {
                //当前状态为停机但处于停机转运行中，需将状态先改为HMI_STOPPING，
                //再进行运行转停机操作，停机完成后即完成HMI控制停机操作
                ppcs->state = PCS_HMI_STOPPING;
                if(ppcs->pcs_stop_step == PCS_STOP_STEP_DONE) {
                    ppcs->pcs_stop_step = PCS_STOP_STEP_INIT;
                }
                em_strategy_handle_pcs_run_to_stop_immediately(ppcs, ptmsp->type);
                LogInfo("pcs[%d] in stop_to_run state, but hmi ctrl to stop so need switch to stop", ppcs->pcs_idx);
            }
            ret = 1;
        }
    } else if(ppcs->state == PCS_CHARGE_BAT || ppcs->state == PCS_DISCHARGE_BAT) {
        if(ppcs->bms_work_done == 0) {
            //判断屏幕是否操作停机
            if(ppcs->hmi_start_stop == HMI_CTRL_PCS_STOP && ppcs->hmi_start_stop_done == HMI_CTRL_PCS_NONE_DONE) {
                ppcs->state = PCS_HMI_STOPPING;
                if(ppcs->pcs_stop_step == PCS_STOP_STEP_DONE) {
                    ppcs->pcs_stop_step = PCS_STOP_STEP_INIT;
                }
                em_strategy_handle_pcs_run_to_stop_immediately(ppcs, ptmsp->type);
                LogInfo("pcs[%d] in charge or discharge state, but hmi ctrl to stop so need switch to stop", ppcs->pcs_idx);
                ret = 1;
            }
        }
    } else if(ppcs->state == PCS_HMI_STOPPING) {
        //响应HMI控制PCS停运中且PCS停机过程尚未完成
        if(ppcs->pcs_stop_step != PCS_STOP_STEP_DONE) {
            em_strategy_handle_pcs_run_to_stop_immediately(ppcs, ptmsp->type);
            if(ppcs->pcs_stop_step == PCS_STOP_STEP_DONE) {
                ppcs->state = PCS_HMI_STOPPED;
                //后台响应前台界面控制PCS停运完成，通知前台界面
                ppcs->hmi_start_stop_done = HMI_CTRL_PCS_STOP_DONE;
                LogInfo("pcs[%d] in hmi_stopping state, now stop done", ppcs->pcs_idx);
            } else {
                LogEMANA("pcs[%d] in hmi_stopping state, wait run_to_stop done", ppcs->pcs_idx);
            }
        } else {
            LogInfo("pcs[%d] in hmi_stopping state, now stop done", ppcs->pcs_idx);
            ppcs->state = PCS_HMI_STOPPED;
            //后台响应前台界面控制PCS停运完成，通知前台界面
            ppcs->hmi_start_stop_done = HMI_CTRL_PCS_STOP_DONE;
        }
        ret = 1;
    }

    return ret;
}

/*******************************************************************************
 * @brief PCS功率分配-平均法
 * @param pems
 * @param bat_chg_state
 * @return 0: success; 1: failed
*******************************************************************************/
int em_strategy_pcs_power_distribute_avg(vems_t* pems, enum BAT_CHG_STATE bat_chg_state)
{
    int ret = 0;
    int i = 0;

    if(g_vems.elec_meter.load_power_pre == g_vems.elec_meter.load_power) {
        //LogEMANA("load power[%.1f] has no change so no need redistribution pcs power", g_vems.elec_meter.load_power * COMM_DATA_RATIO);
        if(g_vems.pcs[0].power_calc != 0) {
            //临时这么写，应该时任意一个PCS的计算功率值为0都需要重新计算各PCS的功率值
            return 1;
        }
    }

    if(bat_chg_state == BAT_CHARGE) {
        //平谷时段充电时，各PCS充电功率分配计算
        pems->pcs_power_total_pre = pems->pcs_power_total;
        pems->pcs_power_total = pems->cfg.station_power_available - g_vems.elec_meter.load_power;
        if(pems->pcs_power_calc >= 0)
            pems->pcs_power_calc_pre = pems->pcs_power_calc;
        pems->pcs_power_calc = pems->pcs_power_total / pems->pcs_cnt;
        if(pems->pcs_power_calc < 0) {
            LogWarn("in charge timespan, this pcs_power_calc[%d] is invalid, station_power_available=%d", pems->pcs_power_calc, pems->cfg.station_power_available);
            return 1;
        }
        if(pems->pcs_power_calc > pems->pcs_power_rated) {
            if(pems->pcs[0].power_calc == pems->pcs_power_rated) {
                LogInfo("all pcs has already full power running, no need set more bigger power");
                return 0;
            }
            //首次达到最大功率需要更新PCS功率为最大功率
            pems->pcs_power_calc = pems->pcs_power_rated;
        }
        LogEMANA("in charge timespan, pcs_power_total[%d, %d, %d-%d] pcs_power_calc[%d, %d]", pems->pcs_power_total, pems->pcs_power_total_pre
                , pems->cfg.station_power_available, pems->elec_meter.load_power, pems->pcs_power_calc, pems->pcs_power_calc_pre);
        
        if(pems->pcs_power_calc != pems->pcs_power_rated
           && abs(pems->pcs_power_calc - pems->pcs_power_calc_pre) < g_vems.cfg.power_update_step) {
            LogInfo("new calc pcs power[%d] diff with previous[%d] less than update_step[%d], no need update pcs power");
            return ret;
        }

        for(i = 0; i < pems->pcs_cnt; i++) {
            if(pems->pcs_power_calc == pems->pcs[i].power_calc) {
                LogInfo("pcs in charging, load power has changed but power_calc[%d] no change so no need update pcs power"
                        , pems->pcs_power_calc);
                break;
            }
            pems->pcs[i].power_calc = pems->pcs_power_calc;
            pems->pcs[i].power_calc_u16 = pems->pcs[i].power_calc;
            pems->pcs[i].power_updated = 1;
            LogInfo("pcs[%d] need update power to[%d]", i, pems->pcs[i].power_calc);
        }
    } else if(bat_chg_state == BAT_DISCHARGE) {
        //高峰时段放电时，各PCS放电功率分配计算
        pems->pcs_power_total_pre = pems->pcs_power_total;
        pems->pcs_power_total = g_vems.elec_meter.load_power;
        if(pems->pcs_power_calc >= 0)
            pems->pcs_power_calc_pre = pems->pcs_power_calc;
        pems->pcs_power_calc = pems->pcs_power_total / pems->pcs_cnt;
        if(pems->pcs_power_calc < 0) {
            LogWarn("in discharge timespan, this pcs_power_calc[%d] is invalid, station_power_available=%d", pems->pcs_power_calc, pems->cfg.station_power_available);
            return 1;
        }
        if(pems->pcs_power_calc > pems->pcs_power_rated) {
            if(pems->pcs[0].power_calc == pems->pcs_power_rated) {
                //已经给PCS设置过最大功率，则不需要再次设置为最大功率
                LogInfo("all pcs has already full power running, no need set more bigger power");
                return 0;
            }
            //首次达到最大功率需要更新PCS功率为最大功率
            pems->pcs_power_calc = pems->pcs_power_rated;
        }
        LogEMANA("in discharge timespan, pcs_power_total[%d, %d, %d-%d] pcs_power_calc[%d, %d]", pems->pcs_power_total, pems->pcs_power_total_pre
                , pems->cfg.station_power_available, pems->elec_meter.load_power, pems->pcs_power_calc, pems->pcs_power_calc_pre);
        
        if(pems->pcs_power_calc != pems->pcs_power_rated
           && abs(pems->pcs_power_calc - pems->pcs_power_calc_pre) < g_vems.cfg.power_update_step) {
            LogInfo("new calc pcs power[%d] diff with previous[%d] less than update_step[%d], no need update pcs power");
            return ret;
        }

        for(i = 0; i < pems->pcs_cnt; i++) {
            if(pems->pcs_power_calc == (-pems->pcs[i].power_calc)) {
                LogInfo("pcs in charging, load power has changed but power_calc[%d] no change so no need update pcs power"
                        , pems->pcs_power_calc);
                break;
            }
            pems->pcs[i].power_calc = (-pems->pcs_power_calc);
            pems->pcs[i].power_calc_u16 = pems->pcs[i].power_calc;
            pems->pcs[i].power_updated = 1;
        }
        pems->pcs_power_calc_pre = pems->pcs_power_calc;
    } else {
        LogInfo("bat_chg_state is BAT_NO_CHARGE, no need distribute pcs power");
    }

    return ret;
}

int em_strategy_pcs_power_distribute_check_min(pcs_dev_t* ppcs, int16_t power_calc_now, enum BAT_CHG_STATE bat_chg_state)
{
    if(bat_chg_state == BAT_CHARGE && power_calc_now < g_vems.cfg.pcs_pset_min) {
        power_calc_now = g_vems.cfg.pcs_pset_min;    //减半后不低于最低有功设置值
    } else if(bat_chg_state == BAT_DISCHARGE && power_calc_now > -g_vems.cfg.pcs_pset_min) {
        power_calc_now = -g_vems.cfg.pcs_pset_min;   //减半后不低于最低有功设置值
    }
    
    if(power_calc_now != ppcs->power_calc) {
        ppcs->power_calc_pre = ppcs->power_calc;
        ppcs->power_calc = power_calc_now;
        ppcs->power_calc_u16 = power_calc_now;
        ppcs->power_updated = 1;
    }

    return 0;
}

/*******************************************************************************
 * @brief 根据现在条件调整分配PCS功率
 * @param pems
 * @param bat_chg_state
 * @return 
*******************************************************************************/
int em_strategy_pcs_power_distribute_limit(vems_t* pems, enum BAT_CHG_STATE bat_chg_state)
{
    int ret = 0;
    int i = 0;
    int16_t power_calc_now = 0;
    pcs_dev_t* ppcs = NULL;
    bms_data_t* pbd = NULL;     //BMS实时数据结构体

    for(i = 0; i < pems->pcs_cnt; i++) {
        ppcs = &pems->pcs[i];
        if(ppcs->chg_bat_idx == -1 || ppcs->power_updated) {
            continue;
        }

        pbd = &ppcs->bat_rack[ppcs->chg_bat_idx].rtdata;
        //降功率判断(这里只处理降功率，停机处理在另一个函数中做)
        if(pbd->alarm12.bits.charge_curr_big || pbd->alarm12.bits.discharge_curr_big) {
            //一级过流，功率减半
            power_calc_now = ppcs->power_calc / 2;
            em_strategy_pcs_power_distribute_check_min(ppcs, power_calc_now, bat_chg_state);
        } else if(pbd->alarm11.bits.temp_high || pbd->alarm11.bits.temp_high || pbd->alarm11.bits.temp_diff_big
               || pbd->alarm11.bits.cell_vol_diff_big || pbd->alarm12.bits.soc_low) {
            //一级温度高、温度低、温差大、单体压差大、SOC低，PCS快速降功率并维持在低功率充电
            if(ppcs->power_calc > 500) {
                //50kW以上功率每次降30%
                power_calc_now = ppcs->power_calc * 0.7;
            } else {
                //50kW以下功率每次降50%
                power_calc_now = ppcs->power_calc * 0.5;
            }
            em_strategy_pcs_power_distribute_check_min(ppcs, power_calc_now, bat_chg_state);
        }
    }

    return ret;
}

/*******************************************************************************
 * @brief 能量管理策略-PCS功率分配策略(平均法)
 * @param pems
 * @param bat_chg_state
 * @return 0: success; 1: failed
*******************************************************************************/
int em_strategy_pcs_power_distribute(vems_t* pems, enum BAT_CHG_STATE bat_chg_state)
{
    int ret = 0;

    if(g_vems.pcs[0].power_updated) {
        //判断条件临时这么写，应改为：判断有PCS调整功率还未完成前，其他PCS均不再调整功率
        //LogEMANA("pcs[0] set power[%d] not complete, so no need redistribution pcs power", g_vems.pcs[0].power_calc);
        return ret;
    }

    //负载功率有变，按[平均法]重新计算各PCS功率，其他分配方法待扩展
    ret = em_strategy_pcs_power_distribute_avg(pems, bat_chg_state);
    if(ret == 0) {
        g_vems.elec_meter.load_power_pre = g_vems.elec_meter.load_power;
    }
    //根据BMS状态对PCS降功率
    em_strategy_pcs_power_distribute_limit(&g_vems, bat_chg_state);

    return ret;
}

/*******************************************************************************
 * @brief 夜间储能模式单台PCS控制处理
 * @param pcs_id
 * @return 0: success; 1: failed
*******************************************************************************/
int em_night_es_handle_one_pcs(int pcs_id, ems_tmsp_item_t* ptmsp)
{
    int ret = 0;
    pcs_dev_t* ppcs = &g_vems.pcs[pcs_id];
    ems_cont_t* pcont = &ppcs->cont;
    battery_rack_t* pbat = ppcs->bat_rack;
    //uint8_t cdata[8] = { 0 };

    if(pcont->rtdata.cont_state == CONT_INITING) {
        LogInfo("pcs[%d] controler is initing, wait ready %d times", ppcs->cont.wait_ready_cnt);
        return 1;
    }

    //
    ret = em_strategy_check_tmsp_with_pcs_state(ppcs, ptmsp);
    if(ret == 1) {
        return ret;
    }

    if(ppcs->state == PCS_CHARGE_BAT) {
        //PCS正在充电中，检测电池信息，判断是否充满
        if(pbat->rtdata.soc < 100) {
            //继续充电，根据负载情况实时调整PCS充电功率
            if(em_pcs_set_power(ppcs) == 0) {
                ppcs->power_updated = 0;
            }
        } else {
            //当前电池已充满，控制PCS停机，然后切断对应的直流继电器
            //发送停机指令给PCS
            ret = em_pcs_run_ctrl(pcs_id, CMD_STOP);
            if(ret != 0) {
                LogError("switch pcs run to stop failed, will try next time");
                return ret;
            } else {
                LogInfo("switch pcs run to stop success");
                ppcs->state = ppcs->state == PCS_CHARGE_BAT ? PCS_CHARGE_STOPPING : PCS_DISCHARGE_STOPPING;
                //等待确认PCS已停机后再切断直流继电器
                //(动作将在ppcs->state == PCS_CHARGE_STOPPING && ppcs->rtdata.run_mode == PRM_STOP时执行)
            }
        }
    } else if(ppcs->state == PCS_STOPPED) {
        if(ppcs->pcs_start_step == PCS_START_STEP_DONE) {
            ppcs->pcs_start_step = PCS_START_STEP_INIT;
        }
        em_strategy_handle_pcs_stop_to_run(ppcs, BAT_CHARGE);
    } else if(ppcs->state == PCS_CHARGE_STOPPING) {
        LogInfo("pcs[%d] now in charge_stopping state", pcs_id);
        if(ppcs->rtdata.run_mode == PRM_STOP) {
            //PCS已从充电转换为停机状态
            LogMust("pcs[%d] now in charge_stopped state, now need cut off dc relay[%d]", pcs_id, ppcs->chg_bat_idx);

            //要将PCS/HPS/PBD 状态参数 0x1801F150中的PCS状态设置为初始吗？？？

            //向控制器发送断开直流继电器命令
            ret = cont_comm_relay_ctrl(pcont, ppcs->chg_bat_idx, RELAY_OPEN);
            if(ret != 0) {
                LogError("switch pcs[%d]'s controler relay[%d] to open failed, will try next time", pcs_id, ppcs->chg_bat_idx);
                return ret;
            } else {
                LogInfo("switch pcs[%d]'s controler relay[%d] to open success", pcs_id, ppcs->chg_bat_idx);
                ppcs->state = PCS_DISCHARGE_STOPPED;
                //当前电池簇充电完成
            }
        }
    } else if(ppcs->state == PCS_DISCHARGE_STOPPING) {
        LogInfo("pcs[%d] now in discharge to stop state", pcs_id);
    } else if(ppcs->state == PCS_INITING) {
        LogInfo("pcs[%d] now in initing state", pcs_id);
    } else if(ppcs->state == PCS_INITED) {
        LogInfo("pcs[%d] now in inited state, wait read real pcs state");
    } else if(ppcs->state == PCS_STOPPING) {
        LogInfo("pcs[%d] now in stopping state", pcs_id);
    } else if(ppcs->state == PCS_DISCHARGE_BAT) {
        LogWarn("night_energy_storage mode should't appear dischare state, please check and contact provider!");
    } else {
        LogError("pcs[%d] unknown state[%d]", pcs_id, ppcs->state);
    }

    return 0;
}

/*******************************************************************************
 * @brief 削峰填谷模式单台PCS控制处理
 * @param pcs_id
 * @param ptmsp 当前所处时段
 * @return 0: success; 1: failed
*******************************************************************************/
int em_xftg_handle_one_pcs(int pcs_id, ems_tmsp_item_t* ptmsp)
{
    int ret = 0;
    bms_data_t* pbd = NULL;
    pcs_dev_t* ppcs = &g_vems.pcs[pcs_id];

    if(ptmsp->type == BAT_DISCHARGE || ptmsp->type == BAT_CHARGE) {
        //根据采集到的PCS实时数据，更新PCS运行状态
        em_pcs_check_update_pcs_run_state(ppcs);
        //检查PCS运行状态与充放电是否是否匹配（程序异常退出重启处理）
        ret = em_strategy_check_tmsp_with_pcs_state(ppcs, ptmsp);
        if(ret == 1) {
            return ret;
        }
        //检查是否通过屏幕界面控制PCS停运或投运
        ret = em_strategy_check_handle_hmi_ctrl_start_stop(ppcs, ptmsp);
        if(ret == 1) {
            return ret;
        }

        //充放电时段
        if(ppcs->state == PCS_STOPPED) {
            //起始状态: 选取一个电池对象，然后对其放电或充电
            if(ppcs->pcs_start_step == PCS_START_STEP_DONE) {
                ppcs->pcs_start_step = PCS_START_STEP_INIT;
                ppcs->bms_work_done = 0;
                LogInfo("pcs[%d] in stopped state, start search charge_discharge battery", ppcs->pcs_idx);
            }
            ret = em_strategy_handle_pcs_stop_to_run(ppcs, ptmsp->type);
        } else if(ppcs->state == PCS_CHARGE_BAT || ppcs->state == PCS_DISCHARGE_BAT) {
            //检测充放电是否完成或异常、告警导致充放电结束(需区分立即停机和快降功率后停机)
            pbd = &ppcs->bat_rack[ppcs->chg_bat_idx].rtdata;
            if(pbd->alarm22.bits.charge_curr_big || pbd->alarm22.bits.discharge_curr_big || pbd->alarm32.bits.charge_curr_big
            || pbd->alarm32.bits.discharge_curr_big || pbd->alarm21.bits.cell_vol_high || pbd->alarm21.bits.cell_vol_low
            || pbd->alarm21.bits.tot_vol_high || pbd->alarm21.bits.tot_vol_low || pbd->alarm31.bits.cell_vol_high
            || pbd->alarm31.bits.cell_vol_low || pbd->alarm31.bits.tot_vol_high || pbd->alarm31.bits.tot_vol_low
            || pbd->km_state == 0) {
                //需立即停机的情况
                //ppcs->bat_rack[ppcs->chg_bat_idx].chg_dischg_done = 1;
                ppcs->bms_work_done = 1;
            } else if(pbd->alarm11.bits.cell_vol_high || pbd->alarm11.bits.cell_vol_low
                   || pbd->alarm11.bits.tot_vol_high || pbd->alarm11.bits.tot_vol_low || pbd->alarm32.bits.ms_comm_fault) {
                //PCS快速降功率并停机
                ppcs->bms_work_done = 2;
            } else {
                //正常状态，继续充放电
                if(ppcs->bms_work_done != 0) {
                    ppcs->bms_work_done = 0;
                }
            }

            if(ppcs->bms_work_done == 1) {
                //battery charge or discharge done, need stop pcs
                if(ppcs->pcs_stop_step == PCS_STOP_STEP_DONE) {
                    ppcs->pcs_stop_step = PCS_STOP_STEP_INIT;
                }
                LogEMANA("pcs[%d] %s battery[%d] done, need stop pcs. stop_step=%d", ppcs->pcs_idx
                        , ppcs->state == PCS_CHARGE_BAT ? "charge" : "discharge", ppcs->chg_bat_idx, ppcs->pcs_stop_step);
                em_strategy_handle_pcs_run_to_stop_immediately(ppcs, ptmsp->type);
            } else if(ppcs->bms_work_done == 2) {
                if(ppcs->pcs_stop_step == PCS_STOP_STEP_DONE) {
                    ppcs->pcs_stop_step = PCS_STOP_STEP_INIT;
                }
                LogEMANA("pcs[%d] %s battery[%d] done, need stop pcs. stop_step=%d", ppcs->pcs_idx
                        , ppcs->state == PCS_CHARGE_BAT ? "charge" : "discharge", ppcs->chg_bat_idx, ppcs->pcs_stop_step);
                em_strategy_handle_pcs_run_to_stop_smoothly(ppcs, ptmsp->type);
            } else {
                //充电/放电状态：动态调整PCS功率
                if(em_pcs_set_power(ppcs) == 0) {
                    ppcs->power_updated = 0;
                }
                // LogEMANA("pcs[%d] %s battery[%d] at power=%.1f", ppcs->pcs_idx, ptmsp->type == BAT_CHARGE ? "charging" : "discharging"
                //         , ppcs->chg_bat_idx, ppcs->rtdata.active_power);
            }
        } else if(ppcs->state == PCS_ERR_STOPPING) {
            if(ppcs->pcs_stop_step == PCS_STOP_STEP_DONE) {
                ppcs->pcs_stop_step = PCS_STOP_STEP_INIT;
            }
            em_strategy_handle_pcs_run_to_stop_immediately(ppcs, ptmsp->type);
            LogEMANA("pcs[%d] error_stopping, now state=%d step=%d", ppcs->pcs_idx, ppcs->state, ppcs->pcs_stop_step);
        } else if(ppcs->state == PCS_CHARGE_STOPPING || ppcs->state == PCS_DISCHARGE_STOPPING) {
            LogInfo("pcs[%d] in %s_stopping state", ppcs->pcs_idx, ppcs->state == PCS_CHARGE_STOPPING ? "charge" : "discharge");
            ret = 0;
        } else if(ppcs->state == PCS_CHARGE_STOPPED) {
            ppcs->state = PCS_STOPPED;
            ppcs->pcs_start_step = PCS_START_STEP_INIT;
            LogMust("pcs[%d] charge battery[%d] done", ppcs->pcs_idx, ppcs->chg_bat_idx);
        } else if(ppcs->state == PCS_DISCHARGE_STOPPED) {
            ppcs->state = PCS_STOPPED;
            ppcs->pcs_start_step = PCS_START_STEP_INIT;
            LogMust("pcs[%d] discharge battery[%d] done", ppcs->pcs_idx, ppcs->chg_bat_idx);
        } else {
            LogWarn("pcs[%d] in xftg-%s timespan, pcs_state[%d] not correct", ppcs->pcs_idx, ptmsp->type == BAT_CHARGE ? "charge" : "discharge", ppcs->state);
            ret = 1;
        }

        if(ret == 0) {
            em_pcs_check_ctrl_io(ppcs);
        }
    } else if(ptmsp->type == BAT_NO_CHARGE) {
        //非充放电时段
        ret = 0;
    } else {
        LogError("tmsp[%d]-[%d,%d] unknown type[%d]", ptmsp->no, ptmsp->start_hour, ptmsp->end_hour, ptmsp->type);
        ret = 1;
    }

    return ret;
}

void* energy_mana_thread_do(void* args)
{
    int i = 0;
    ems_tmsp_item_t* ptmsp = NULL;
    time_t curr_sec = 0;
    struct tm tm_now;
    // LogEMANA("in %s xftg[%d, %d] night_es[%d, %d]", __func__, g_vems.tmsp.xftg.start_hour
    //         , g_vems.tmsp.xftg.end_hour, g_vems.tmsp.night_es.start_hour, g_vems.tmsp.night_es.end_hour);

    g_vems.th_stat.bits.th_energy_mana = 1;
    while(g_vems.th_stat.byte != THREAD_ALL_READY) {
        safe_msleep(1000);
    }
    
    while(1) {
        curr_sec = time(NULL);
        localtime_r(&curr_sec, &tm_now);
        //LogEMANA("curr_sec=%ld hour=%d minute=%d", curr_sec, tm_now.tm_hour, tm_now.tm_min);
        
        if(IS_IN_TIMESPAN(tm_now.tm_hour, g_vems.tmsp.xftg)) {
            if(IS_IN_TIMESPAN(tm_now.tm_hour, g_vems.tmsp.discharge_tmsp1)) {
                ptmsp = &g_vems.tmsp.discharge_tmsp1;
            } else if(IS_IN_TIMESPAN(tm_now.tm_hour, g_vems.tmsp.charge_tmsp1)) {
                ptmsp = &g_vems.tmsp.charge_tmsp1;
            } else if(IS_IN_TIMESPAN(tm_now.tm_hour, g_vems.tmsp.discharge_tmsp2)) {
                ptmsp = &g_vems.tmsp.discharge_tmsp2;
            } else if(IS_IN_TIMESPAN(tm_now.tm_hour, g_vems.tmsp.charge_tmsp2)) {
                ptmsp = &g_vems.tmsp.charge_tmsp2;
            } else {
                goto next_loop;
            }

            em_strategy_pcs_power_distribute(&g_vems, ptmsp->type);
            //调用削峰填谷模式函数
            for(i = 0; i < g_vems.pcs_cnt; i++) {
                em_xftg_handle_one_pcs(i, ptmsp);
            }
        } else if(IS_IN_TIMESPAN(tm_now.tm_hour, g_vems.tmsp.night_es)) {
            em_strategy_pcs_power_distribute(&g_vems, BAT_CHARGE);
            //调用夜间储能模式函数
            for(i = 0; i < g_vems.pcs_cnt; i++) {
                em_night_es_handle_one_pcs(i, ptmsp);
            }
        } else {
            LogWarn("hour[%d] not in any timespan, xftg[%d, %d] night_es[%d, %d]", tm_now.tm_hour, g_vems.tmsp.xftg.start_hour
                    , g_vems.tmsp.xftg.end_hour, g_vems.tmsp.night_es.start_hour, g_vems.tmsp.night_es.end_hour);

            if(!vems_check_tmsp_item_valid(&g_vems.tmsp.xftg) || !vems_check_tmsp_item_valid(&g_vems.tmsp.night_es)) {
                LogInfo("xftg timespan or negith_es timespan is invalid, now reload it's config");
                vems_read_tmsp();
            }
        }
        
next_loop:
        safe_msleep(500);
    }


    return args;
}

/*******************************************************************************
 * @brief  创建能量管理线程
 * @return 0: success; 1: failed
*******************************************************************************/
int create_energy_mana_thread(void)
{
    int ret = 0;
    pthread_t emana_tid;
    pthread_attr_t emana_attr;

    pthread_attr_init(&emana_attr);
    pthread_attr_setdetachstate(&emana_attr, PTHREAD_CREATE_DETACHED);
    ret = pthread_create(&emana_tid, &emana_attr, energy_mana_thread_do, NULL);
    if(ret == 0) {
        LogInfo("create energy management thread success");
    } else {
        LogError("create energy management thread failed: %s", strerror(errno));
    }

    return ret;
}
