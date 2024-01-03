/*******************************************************************************
 * @             Copyright (c) 2023 LinyPower, All Rights Reserved.
 * @*****************************************************************************
 * @FilePath     : hmi_comm.c
 * @Descripttion :
 * @Author       : xiaoyongjun
 * @E-Mail       : xiaoyongjun@linygroup.cn
 * @Version      : v1.0.0
 * @Date         : 2023-10-23 09:23:29
*******************************************************************************/
#include "hmi_comm.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "debug_and_log.h"
#include "can_util.h"
#include "util.h"
#include "common_define.h"
#include "vems.h"
#include "database.h"
#include "modbus.h"
#include "hmi_db_util.h"
#include "hmi_show_dialog.h"
#include "hmi_ziku.h"

modbus_t* g_ctx = NULL;
int current_pcs_id = 0;
/*只有设备名称发生变化才刷新一次*/
uint8_t pcs_name_change_flag = 1;
/*设备值修改过就刷新一次*/
uint8_t pcs_info_change_flag = 1;
/*版本号更新刷新*/
uint8_t version_change_flag = 0;
/*充放电时间刷新*/
static uint8_t time_change = 1;
/*系统参数刷新*/
static uint8_t syscfg_change = 1;
/*通信参数设置*/
static uint8_t conn_change = 1;
/*登录状态*/
uint8_t logon_status = 0;

hmi_screen_t hmi_screen_table[LISTEN_PAGE_NUM] =
{
    {0,  {0}, {0}, 0, {{0}}},    /*启动页*/
    {1,  {0}, {0}, 2, {{0, 0, hmi_comm_system_overview}, { 0, 0, hmi_comm_overview_ctrl_pcs_start_stop }}},    /*系统概览*/
    {2,  {0}, {0}, 3, {{0, 0, hmi_comm_add_pcs}, { 0, 0, hmi_comm_change_pcs }, { 0, 0, hmi_comm_del_pcs }}},  /*pcs管理*/
    {3,  {0}, {0}, 1, {{0, 0, hmi_comm_change_charge}}},  /*充放电管理*/
    {4,  {0}, {0}, 0, {{0}}}, /*操作日志管理*/
    {5,  {0}, {0}, 1, {{0, 0, hmi_comm_change_system_config}}},  /*系统参数设置管理*/
    {6,  {0}, {0}, 1, {{0, 0, hmi_comm_change_conn_config}}},    /*系统参数设置管理*/
    {7,  {0}, {0}, 1, {{0, 0, hmi_comm_pcs_info_show}}},         /*PCS信息显示*/
    {8,  {0}, {0}, 1, {{0, 0, hmi_comm_bms_info_show}}},         /*bms信息显示*/
    {9,  {0}, {0}, 0, {{0}}},
    {10, {0}, {0}, 1, {{0, 0, hmi_comm_version_show}}},
    {11, {0}, {0}, 0, {{0, 0, hmi_comm_bms_warn_show}}},        /*bms告警显示*/
    {12, {0}, {0}, 0, {{0}}},    /*NULL*/
    {13, {0}, {0}, 1, {{0, 0, hmi_comm_pcs_warn_show}}},    /*PCS故障状态指示信息*/
};


/*******************************************************************************
 * @brief 清除历史告警记录
 * @return
*******************************************************************************/
void hmi_comm_clear_data(void)
{
    int ret = 0;
    uint16_t wdata[2] = { 0 };
    wdata[0] = 1;
    ret = modbus_write_registers(g_ctx, CLEAR_HISTORY_DATA_ADDR, 1, wdata);
    if(ret == 1) {
        LogEMSHMI("clear history data sucess");
    }
}
/*******************************************************************************
 * @brief 和屏幕rtc进行通信
 * @return
*******************************************************************************/
void hmi_comm_show_rtc(void)
{
    int ret = 0;
    uint16_t rdata[16] = { 0 };
    static int cnt = 0;
    cnt++;
    /*读取RTC时间*/
    ret = modbus_read_registers(g_ctx, RTC_READ_YEAR_ADDR, 6, rdata);
    if(ret == 6) {
        LogEMSHMI(" year[%d] month[%d] day[%d] hour[%d] min[%d] sec[%d]\n", rdata[0], rdata[1], rdata[2], rdata[3], rdata[4], rdata[5]);
    }
    /*设置RTC时间*/
    if(cnt++ >= 200) {
        rdata[8] = 2008;
        rdata[9] = 11;
        rdata[10] = 11;
        rdata[11] = 11;
        rdata[12] = 11;
        rdata[13] = 11;
        rdata[14] = 1;
        modbus_write_registers(g_ctx, RTC_WRITE_YEAR_ADDR, 6, &rdata[8]);
        ret = modbus_write_registers(g_ctx, RTC_SET_TIME_ADDR, 1, &rdata[14]);
        if(ret == 1) {
            cnt = 0;
            LogEMSHMI("change rtc time sucess");
        }

    }
}
/*******************************************************************************
 * @brief 显示系统概览信息
 * @param pscreen
 * @param ctrl_id
 * @return
*******************************************************************************/
void hmi_comm_system_overview(struct hmi_screen* pscreen, int ctrl_id)
{
    int ret = 0;
    int pcs_id = 0;
    int bms_id = 0;
    pcs_real_data_t* prtd = NULL;
    bms_data_t* pbmsdata = NULL;
    cont_data_t* pcontdata = NULL;
    char buff[32] = { 0 };
    uint32_t btn = 0;
    uint32_t password = 0;

    /*监听是否退出登录*/
    if(logon_status == 1) {
        ret = modbus_read_registers(g_ctx, EXIT_LOG_ADDR, 1, &pscreen->rdata[0]);
        if((ret == 1) && (pscreen->rdata[0] == 1)) {
            /*在其他界面*/

            /*弹出对话框*/
            btn = hmi_show_msgbox(pcontext[10], ptitle[0], BUTTON_OK | BUTTON_NO, ICON_INFO);
            /*确认退出*/
            if(btn == BUTTON_OK) {
                logon_status = 0;
                /*清除登录状态标志*/
                pscreen->wdata[0] = 0;
                modbus_write_registers(g_ctx, LOG_ON_STATUS_ADDR, 1, &pscreen->wdata[0]);
            }
            /*清除退出按钮标志*/
            pscreen->wdata[0] = 0;
            modbus_write_registers(g_ctx, EXIT_LOG_ADDR, 1, &pscreen->wdata[0]);
        }
    }
    /*监听有没有登录*/
    if(logon_status == 0) {
        ret = modbus_read_registers(g_ctx, LOG_ON_ADDR, 1, &pscreen->rdata[0]);
        /*登录按钮按下*/
        if((ret == 1) && (pscreen->rdata[0] == 1)) {
            /*读取用户名称数据*/
            ret = modbus_read_registers(g_ctx, LOG_ON_USER_ADDR, 12, &pscreen->rdata[0]);
            if(ret == 12) {
                //提取输入的用户名
                buff[0] = (pscreen->rdata[0] >> 8) & 0xff;
                buff[1] = (pscreen->rdata[0]) & 0xff;
                buff[2] = (pscreen->rdata[1] >> 8) & 0xff;
                buff[3] = (pscreen->rdata[1]) & 0xff;
                buff[4] = (pscreen->rdata[2] >> 8) & 0xff;
                buff[5] = (pscreen->rdata[2]) & 0xff;
                buff[6] = (pscreen->rdata[3] >> 8) & 0xff;
                buff[7] = (pscreen->rdata[3]) & 0xff;
                buff[8] = (pscreen->rdata[4] >> 8) & 0xff;
                buff[9] = (pscreen->rdata[4]) & 0xff;
                buff[10] = (pscreen->rdata[5] >> 8) & 0xff;
                buff[11] = (pscreen->rdata[5]) & 0xff;
                buff[12] = (pscreen->rdata[6] >> 8) & 0xff;
                buff[13] = (pscreen->rdata[6]) & 0xff;
                buff[14] = (pscreen->rdata[7] >> 8) & 0xff;
                buff[15] = (pscreen->rdata[7]) & 0xff;
                //提取输入的密码
                buff[17] = (pscreen->rdata[8] >> 8) & 0xff;
                buff[18] = (pscreen->rdata[8]) & 0xff;
                buff[19] = (pscreen->rdata[9] >> 8) & 0xff;
                buff[20] = (pscreen->rdata[9]) & 0xff;
                buff[21] = (pscreen->rdata[10] >> 8) & 0xff;
                buff[22] = (pscreen->rdata[10]) & 0xff;
                buff[23] = (pscreen->rdata[11] >> 8) & 0xff;
                buff[24] = (pscreen->rdata[11]) & 0xff;
                password = atoi(&buff[17]);
                LogEMSHMI("read user login info[%s,%s,%d]", buff, &buff[17], password);
                /*比较用户名和密码*/
                if(1 || ((strcmp(g_vems.user.username, buff) == 0) && (password == g_vems.user.password))) {
                    pscreen->wdata[0] = 1;
                    pscreen->wdata[1] = 0;
                    modbus_write_registers(g_ctx, LOG_ON_STATUS_ADDR, 2, &pscreen->wdata[0]);
                    logon_status = 1;
                } else {
                    pscreen->wdata[0] = 0;
                    pscreen->wdata[1] = 1;
                    modbus_write_registers(g_ctx, LOG_ON_STATUS_ADDR, 2, &pscreen->wdata[0]);
                }
            }
            pscreen->wdata[0] = 0;
            modbus_write_registers(g_ctx, LOG_ON_ADDR, 1, &pscreen->wdata[0]);
        }
    }

    /*当前有设备*/
    if(g_vems.pcs_cnt != 0) {
        /*当前在哪台pcs*/
        ret = modbus_read_registers(g_ctx, PCS_SWITCH_ADDR, 1, &pscreen->rdata[0]);
        if(ret == 1) {
            pcs_id = pscreen->rdata[0];
            pcontdata = &g_vems.pcs[pcs_id].cont.rtdata;
            prtd = &g_vems.pcs[pcs_id].rtdata;

            /*总充电量 总放电量  风机状态*/
            pscreen->wdata[0] = g_vems.pcs[pcs_id].tot_charge_kws / 3600 * COMM_DATA_MULTIPLE2;
            pscreen->wdata[1] = g_vems.pcs[pcs_id].tot_discharge_kws / 3600 * COMM_DATA_MULTIPLE2;
            pscreen->wdata[2] = pcontdata->dio_stat.bits.fan;
            modbus_write_registers(g_ctx, TOTAL_CHARGE_ADDR, 3, &pscreen->wdata[0]);

            /*pcs 有功功率 无功功率 */
            pscreen->wdata[3] = prtd->pcs_pall * COMM_DATA_MULTIPLE;
            pscreen->wdata[4] = prtd->pcs_qall * COMM_DATA_MULTIPLE;
            modbus_write_registers(g_ctx, ACTIVE_POWER_ADDR, 2, &pscreen->wdata[3]);

            bms_id = g_vems.pcs[pcs_id].chg_bat_idx;
            if(bms_id >= 0 && bms_id <= 3) {
                pbmsdata = &g_vems.pcs[pcs_id].bat_rack[bms_id].rtdata;
            }
            //LogEMSHMI("show pcs_%d battery_%d phy info\n", pcs_id + 1, bms_id);
            /*BMS 电池状态 SOC  SOH  总电压 总电流 单体最大电压  单体最小电压 总正继电器状态 充电允许最大电流 放电允许最大电流*/
            if(bms_id == 0) {
                /*隐藏其他bms信息*/
                pscreen->wdata[10] = 1;
                pscreen->wdata[11] = 0;
                pscreen->wdata[12] = 0;
                pscreen->wdata[13] = 0;
                modbus_write_registers(g_ctx, BMS1_HIDE_ADDR, 4, &pscreen->wdata[10]);
                /*其他bms信息状态为等待状态*/
                pscreen->wdata[14] = 0;
                modbus_write_registers(g_ctx, BMS2_BAT_STATUS_ADDR, 1, &pscreen->wdata[14]);
                modbus_write_registers(g_ctx, BMS3_BAT_STATUS_ADDR, 1, &pscreen->wdata[14]);
                modbus_write_registers(g_ctx, BMS4_BAT_STATUS_ADDR, 1, &pscreen->wdata[14]);

                /*显示当前bms实时信息*/
                pscreen->wdata[5] = pbmsdata->bat_state;
                pscreen->wdata[6] = pbmsdata->soc;
                pscreen->wdata[7] = pbmsdata->soh;
                pscreen->wdata[8] = pbmsdata->voltage_total * 10;
                pscreen->wdata[9] = pbmsdata->current_total * 10;
                modbus_write_registers(g_ctx, BMS1_BAT_STATUS_ADDR, 5, &pscreen->wdata[5]);
            } else if(bms_id == 1) {
                /*隐藏其他bms信息*/
                pscreen->wdata[10] = 0;
                pscreen->wdata[11] = 1;
                pscreen->wdata[12] = 0;
                pscreen->wdata[13] = 0;
                modbus_write_registers(g_ctx, BMS1_HIDE_ADDR, 4, &pscreen->wdata[10]);
                /*其他bms信息状态为等待状态*/
                pscreen->wdata[14] = 0;
                modbus_write_registers(g_ctx, BMS1_BAT_STATUS_ADDR, 1, &pscreen->wdata[14]);
                modbus_write_registers(g_ctx, BMS3_BAT_STATUS_ADDR, 1, &pscreen->wdata[14]);
                modbus_write_registers(g_ctx, BMS4_BAT_STATUS_ADDR, 1, &pscreen->wdata[14]);
                /*显示当前bms实时信息*/

                pscreen->wdata[5] = pbmsdata->bat_state;
                pscreen->wdata[6] = pbmsdata->soc;
                pscreen->wdata[7] = pbmsdata->soh;
                pscreen->wdata[8] = pbmsdata->voltage_total * 10;
                pscreen->wdata[9] = pbmsdata->current_total * 10;
                modbus_write_registers(g_ctx, BMS2_BAT_STATUS_ADDR, 5, &pscreen->wdata[5]);
            } else if(bms_id == 2) {
                /*隐藏其他bms信息*/
                pscreen->wdata[10] = 0;
                pscreen->wdata[11] = 0;
                pscreen->wdata[12] = 1;
                pscreen->wdata[13] = 0;
                modbus_write_registers(g_ctx, BMS1_HIDE_ADDR, 4, &pscreen->wdata[10]);
                /*其他bms信息状态为等待状态*/
                pscreen->wdata[14] = 0;
                modbus_write_registers(g_ctx, BMS1_BAT_STATUS_ADDR, 1, &pscreen->wdata[14]);
                modbus_write_registers(g_ctx, BMS2_BAT_STATUS_ADDR, 1, &pscreen->wdata[14]);
                modbus_write_registers(g_ctx, BMS4_BAT_STATUS_ADDR, 1, &pscreen->wdata[14]);
                /*显示当前bms实时信息*/

                pscreen->wdata[5] = pbmsdata->bat_state;
                pscreen->wdata[6] = pbmsdata->soc;
                pscreen->wdata[7] = pbmsdata->soh;
                pscreen->wdata[8] = pbmsdata->voltage_total * 10;
                pscreen->wdata[9] = pbmsdata->current_total * 10;
                modbus_write_registers(g_ctx, BMS3_BAT_STATUS_ADDR, 5, &pscreen->wdata[5]);
            } else if(bms_id == 3) {
                /*隐藏其他bms信息*/
                pscreen->wdata[10] = 0;
                pscreen->wdata[11] = 0;
                pscreen->wdata[12] = 0;
                pscreen->wdata[13] = 1;
                modbus_write_registers(g_ctx, BMS1_HIDE_ADDR, 4, &pscreen->wdata[10]);
                /*其他bms信息状态为等待状态*/
                pscreen->wdata[14] = 0;
                modbus_write_registers(g_ctx, BMS1_BAT_STATUS_ADDR, 1, &pscreen->wdata[14]);
                modbus_write_registers(g_ctx, BMS2_BAT_STATUS_ADDR, 1, &pscreen->wdata[14]);
                modbus_write_registers(g_ctx, BMS3_BAT_STATUS_ADDR, 1, &pscreen->wdata[14]);
                /*显示当前bms实时信息*/

                pscreen->wdata[5] = pbmsdata->bat_state;
                pscreen->wdata[6] = pbmsdata->soc;
                pscreen->wdata[7] = pbmsdata->soh;
                pscreen->wdata[8] = pbmsdata->voltage_total * 10;
                pscreen->wdata[9] = pbmsdata->current_total * 10;
                modbus_write_registers(g_ctx, BMS4_BAT_STATUS_ADDR, 5, &pscreen->wdata[5]);
            } else {

            }
        }
    }
}

void hmi_comm_overview_ctrl_pcs_start_stop(struct hmi_screen* pscreen, int ctrl_id)
{
    static uint8_t first_init_hmi_start_stop = 1;
    int ret = 0;
    uint8_t pcs_idx = 0;
    uint16_t wdata[2] = { 0 };
    pcs_dev_t* ppcs = NULL;

    /*读取当前在哪台pcs*/
    ret = modbus_read_registers(g_ctx, PCS_SWITCH_ADDR, 1, &pscreen->rdata[0]);
    if(ret == 1) {
        pcs_idx = pscreen->rdata[0];
        ppcs = &g_vems.pcs[pcs_idx];
    } else {
        LogEMSHMI("in hmi_comm_overview_ctrl_pcs_start_stop() read current pcs_idx failed: %d %s", ret, modbus_strerror(errno));
        return;
    }

    /*读取是否操作了投运、停用按钮和投运、停用操作是否已完成标志量*/
    ret = modbus_read_registers(g_ctx, PCS_CTRL_START_STOP, 2, &pscreen->rdata[1]);
    if(ret == 2) {
        if(pscreen->rdata[1] == HMI_CTRL_PCS_START || pscreen->rdata[1] == HMI_CTRL_PCS_STOP) {
            if(ppcs->hmi_start_stop == HMI_CTRL_PCS_NONE) {
                ppcs->hmi_start_stop = pscreen->rdata[1];
                ppcs->hmi_start_stop_done = HMI_CTRL_PCS_NONE_DONE;
                LogInfo("user control pcs %s by hmi, now start deal", ppcs->hmi_start_stop == HMI_CTRL_PCS_START ? "start" : "stop");
            }
        }
    } else {
        LogEMSHMI("in hmi_comm_overview_ctrl_pcs_start_stop() read hmi ctrl pcs start stop failed: %d, %s", ret, modbus_strerror(errno));
        return;
    }

    //设置投运、停用操作是否已完成标志量
    if(ppcs->hmi_start_stop != HMI_CTRL_PCS_NONE && ppcs->hmi_start_stop_done != HMI_CTRL_PCS_NONE_DONE) {
        //后台投运、停运状态有更新，前台界面更新按钮状态
        if(pscreen->rdata[1] != HMI_CTRL_PCS_NONE || first_init_hmi_start_stop) {
            //wdata[0] = HMI_CTRL_PCS_NONE;
            wdata[1] = ppcs->hmi_start_stop_done;
            ret = modbus_write_registers(g_ctx, PCS_CTRL_START_STOP_DONE, 1, &wdata[1]);
            if(ret != 1) {
                LogWarn("read hmi ctrl pcs[%d] start stop failed: %d, %s", ppcs->pcs_idx, ret, modbus_strerror(errno));
            } else {
                LogInfo("update pcs[%d] hmi pcs_start_stop=%d, %d,%s state success", ppcs->pcs_idx, ppcs->hmi_start_stop_done, ppcs->hmi_start_stop
                       , ppcs->hmi_start_stop == HMI_CTRL_PCS_START ? "start" : "stop");
                ppcs->hmi_start_stop = HMI_CTRL_PCS_NONE;
            }
        } else {
            LogInfo("pcs[%d] hmi_start_stop=%d hmi_start_stop_done=%d was set, but no need deal", ppcs->pcs_idx, ppcs->hmi_start_stop, ppcs->hmi_start_stop_done);
            ppcs->hmi_start_stop = HMI_CTRL_PCS_NONE;
        }
    }
}

/*******************************************************************************
 * @brief 当选择不同的pcs设备 显示切换后的设备信息
 * @param id 当前设备
 * @return
*******************************************************************************/
int hmi_comm_show_pcs_info(int id)
{
    uint16_t wbuff[20] = { 0 };
    dev_pcs_params_t* ppcs = NULL;
    int ret = 0;
    /*当前有设备*/
    ret = modbus_read_registers(g_ctx, PCS_SWITCH_ADDR, 1, &wbuff[0]);
    if(ret == 1) {
        id = wbuff[0];
    } else {
        return -1;
    }

    if(g_vems.pcs_cnt != 0) {

        ppcs = &g_vems.pcs[id].dbcfg;
        /*pcs编号*/
        wbuff[0] = ppcs->pcs_id;

        ret = modbus_write_registers(g_ctx, PCS_ID_ADDR, 1, (uint16_t*)wbuff);
        if(ret == 1) {
            LogEMSHMI("show pcs%d_id sucess\n", id + 1);
        } else {
            LogEMSHMI("show pcs%d_id failed\n", id + 1);
        }

        /*pcsmodbusid*/
        wbuff[1] = ppcs->modbus_id;
        /*波特率*/
        if((ppcs->baud_rate) == 9600) {
            wbuff[2] = 0;
        } else if((ppcs->baud_rate) == 19200) {
            wbuff[2] = 1;
        } else {

        }
        /*额定功率*/
        wbuff[3] = ppcs->rated_power;
        /*最大电压*/
        wbuff[4] = ppcs->max_voltage;
        /*最大电流*/
        wbuff[5] = ppcs->max_current;
        /*投切控制器ID*/
        wbuff[6] = ppcs->cont_comm_id;
        /*投切控制器CAN地址*/
        wbuff[7] = ppcs->cont_can_addr;
        /*投切控制器CAN波特率*/
        if(((ppcs->cont_can_baudrate)) == 250) {
            wbuff[8] = 0;
        } else if(((ppcs->cont_can_baudrate)) == 500) {
            wbuff[8] = 1;

        } else if(((ppcs->cont_can_baudrate)) == 1000) {
            wbuff[8] = 2;

        } else {

        }
        ret = modbus_write_registers(g_ctx, PCS_ID_ADDR + 9, 8, (uint16_t*)&wbuff[1]);
        if(ret == 8) {
            LogEMSHMI("show pcs%d_info sucess\n", id + 1);
        } else {
            LogEMSHMI("show pcs%d_info failed\n", id + 1);
        }
    }
    return ret;
}
/*******************************************************************************
 * @brief 当设备名称发生变化 告诉前台变化后的设备名称
 * @return
*******************************************************************************/
int  hmi_comm_show_pcs_name(void)
{
    int i = 0;
    int len1 = 0;
    int len2 = 0;
    int len3 = 0;
    char buff[128] = { 0 };
    uint16_t wbuff[64] = { 0 };

    dev_pcs_params_t* ppcs = NULL;


    for(i = 0; i < EMS_PCS_MAX; i++) {
        /*获取设备配置地址*/
        ppcs = &g_vems.pcs[i].dbcfg;
        /*获取名称长度*/
        len1 = strlen(ppcs->pcs_name);
        /*当前地址有设备名称*/
        if(len1 != 0) {
            strncpy(&buff[len2], ppcs->pcs_name, len1);
            len2 += len1;
            buff[len2++] = 0x3b;/*补冒号*/
        }
    }
    /*结束符*/
    buff[len2++] = 0;
    for(i = 0; i < len2; i++) {

    }
    /*拼接前台显示设备名称数据*/
    if(len2 % 2 == 0) {
        for(i = 0; i < len2; i += 2) {
            wbuff[len3++] = buff[i] << 8 | buff[i + 1];
        }
    } else {
        for(i = 0; i < len2 + 1; i += 2) {
            wbuff[len3++] = buff[i] << 8 | buff[i + 1];
        }
    }
    /*发送设备名称*/
    modbus_write_registers(g_ctx, PCS_SHOW_ADDR, len3, (uint16_t*)wbuff);

    return 0;
}

/*******************************************************************************
 * @brief 当从数据库删除设备的时候、设备的数据信息位置需要发生改变 在删除操作中调用
 * @param ppcs pcs设备存储信息指针
 * @param total 删除设备前设备的总个数
 * @param curpos 删除设备的位置
 * @return
*******************************************************************************/
int hmi_pcs_dev_move(dev_pcs_params_t* ppcs, int total, int curpos)
{
    int ret = 0;
    int i = 0;

    /*删除一台设备*/
    ret = dev_pcs_delete(ppcs->pcs_id, 0);
    /*删除成功设备移位*/
    if(ret == 0) {
        for(i = curpos; i < total - 1; i++) {
            /*设备内容移位*/
            memcpy(&g_vems.pcs[i].dbcfg, &g_vems.pcs[i + 1].dbcfg, sizeof(dev_pcs_params_t));
        }
    }
    return ret;
}
/*******************************************************************************
 * @brief 添加pcs管理信息
 * @return
*******************************************************************************/
void hmi_comm_add_pcs(struct hmi_screen* pscreen, int ctrl_id)
{
    int ret;
    int byte_indx = 0;
    int i = 0;
    dev_pcs_params_t  pcs_dev;
    dev_pcs_params_t* ppcs = &pcs_dev;
    /*如果有设备获取当前显示哪一台PCS*/
    if(g_vems.pcs_cnt != 0) {
        ret = modbus_read_registers(g_ctx, PCS_SWITCH_ADDR, 1, &pscreen->rdata[22]);
        if(ret == 1) {
            current_pcs_id = pscreen->rdata[22];
        } else {

        }
    }
    /*如果当前没有PCS设备告诉前台去新增设备*/
    if(g_vems.pcs_cnt == 0) {
        pscreen->rdata[21] = 1;
        ret = modbus_write_registers(g_ctx, PCS_CHECK_ADDR, 1, (uint16_t*)&pscreen->rdata[21]);
        if(ret == 1) {
            LogEMSHMI("tell qiantai add pcs_dev num_%d sucess\n", g_vems.pcs_cnt + 1);
        } else {
            LogEMSHMI("tell qiantai add pcs_dev num_%d fail\n", g_vems.pcs_cnt + 1);
        }
    }
    /*监听到增加pcs设备控件值发生变化去读取寄存器变化的值*/
    ret = modbus_read_registers(g_ctx, PCS_ADD_ADDR, 1, &pscreen->rdata[20]);
    if((ret == 1) && (pscreen->rdata[20]) == 1) {
        LogEMSHMI("start add pcs_dev num_%d\n", g_vems.pcs_cnt);
    } else {
        return;
    }
    /*读取编号(2byte)+名称(16byte)+modbusid(2)+波特率(2)+额定功率（2）+最大电压（2）+最大电流（2）+投切控制器ID（2）+can地址（2）+波特率（2）*/
    ret = modbus_read_registers(g_ctx, PCS_ID_ADDR, 17, &pscreen->rdata[0]);
    if(ret == 17) {
        if(g_vems.pcs_cnt < EMS_PCS_MAX) {
            /*pcs编号*/
            ppcs->pcs_id = pscreen->rdata[0];
            LogEMSHMI("pcs_%d_id[%d]\n", g_vems.pcs_cnt + 1, pscreen->rdata[0]);
            byte_indx += 1;
            /*pcs名称*/
            ppcs->pcs_name[0] = (pscreen->rdata[byte_indx] >> 8) & 0xff;
            ppcs->pcs_name[1] = (pscreen->rdata[byte_indx]) & 0xff;

            ppcs->pcs_name[2] = (pscreen->rdata[byte_indx + 1] >> 8) & 0xff;
            ppcs->pcs_name[3] = (pscreen->rdata[byte_indx + 1]) & 0xff;

            ppcs->pcs_name[4] = (pscreen->rdata[byte_indx + 2] >> 8) & 0xff;
            ppcs->pcs_name[5] = (pscreen->rdata[byte_indx + 2]) & 0xff;

            ppcs->pcs_name[6] = (pscreen->rdata[byte_indx + 3] >> 8) & 0xff;
            ppcs->pcs_name[7] = (pscreen->rdata[byte_indx + 3]) & 0xff;

            ppcs->pcs_name[8] = (pscreen->rdata[byte_indx + 4] >> 8) & 0xff;
            ppcs->pcs_name[9] = (pscreen->rdata[byte_indx + 4]) & 0xff;

            ppcs->pcs_name[10] = (pscreen->rdata[byte_indx + 5] >> 8) & 0xff;
            ppcs->pcs_name[11] = (pscreen->rdata[byte_indx + 5]) & 0xff;

            ppcs->pcs_name[12] = (pscreen->rdata[byte_indx + 6] >> 8) & 0xff;
            ppcs->pcs_name[13] = (pscreen->rdata[byte_indx + 6]) & 0xff;

            ppcs->pcs_name[14] = (pscreen->rdata[byte_indx + 7] >> 8) & 0xff;
            ppcs->pcs_name[15] = (pscreen->rdata[byte_indx + 7]) & 0xff;

            for(i = 0; i < 16; i++) {
                LogEMSHMI("pcs_%dname[%x]\n", g_vems.pcs_cnt + 1, ppcs->pcs_name[i]);
            }
            byte_indx += 8;
            /*pcsmodbusid*/
            ppcs->modbus_id = pscreen->rdata[byte_indx];
            LogEMSHMI("pcs_%dmodbus_id[%x]\n", g_vems.pcs_cnt + 1, pscreen->rdata[byte_indx]);
            byte_indx += 1;
            /*波特率*/
            if((pscreen->rdata[byte_indx]) == 0) {
                ppcs->baud_rate = 9600;
                LogEMSHMI("pcs_%dbaud_rate[%d]\n", g_vems.pcs_cnt + 1, ppcs->baud_rate);

            } else if((pscreen->rdata[byte_indx]) == 1) {
                ppcs->baud_rate = 19200;
                LogEMSHMI("pcs_%dbaud_rate[%d]\n", g_vems.pcs_cnt + 1, ppcs->baud_rate);
            } else {

            }
            LogEMSHMI("pcs_%dbaud_rate_indx1[%x]\n", g_vems.pcs_cnt + 1, pscreen->rdata[byte_indx]);
            byte_indx += 1;
            /*额定功率*/
            ppcs->rated_power = (pscreen->rdata[byte_indx]);
            LogEMSHMI("pcs_%drated_power[%d]\n", g_vems.pcs_cnt + 1, ppcs->rated_power);
            byte_indx += 1;
            /*最大电压*/
            ppcs->max_voltage = (pscreen->rdata[byte_indx]);
            LogEMSHMI("pcs_%dmax_voltage[%d]\n", g_vems.pcs_cnt + 1, ppcs->max_voltage);
            byte_indx += 1;
            /*最大电流*/
            ppcs->max_current = (pscreen->rdata[byte_indx]);
            LogEMSHMI("pcs_%dmax_current[%d]\n", g_vems.pcs_cnt + 1, ppcs->max_current);
            byte_indx += 1;
            /*投切控制器ID*/
            ppcs->cont_comm_id = pscreen->rdata[byte_indx];
            LogEMSHMI("pcs_%dcont_comm_id[%x]\n", g_vems.pcs_cnt + 1, pscreen->rdata[byte_indx]);
            byte_indx += 1;
            /*投切控制器CAN地址*/
            ppcs->cont_can_addr = pscreen->rdata[byte_indx];
            LogEMSHMI("pcs_%dcont_can_addr[%x]\n", g_vems.pcs_cnt + 1, pscreen->rdata[byte_indx]);
            byte_indx += 1;
            /*投切控制器CAN波特率*/
            if(((pscreen->rdata[byte_indx])) == 0) {
                ppcs->cont_can_baudrate = 250;
                LogEMSHMI("pcs_%dcont_can_baudrate[%d]\n", g_vems.pcs_cnt + 1, ppcs->cont_can_baudrate);
            } else if(((pscreen->rdata[byte_indx])) == 1) {
                ppcs->cont_can_baudrate = 500;
                LogEMSHMI("pcs_%dcont_can_baudrate[%d]\n", g_vems.pcs_cnt + 1, ppcs->cont_can_baudrate);
            } else if(((pscreen->rdata[byte_indx])) == 2) {
                ppcs->cont_can_baudrate = 1000;
                LogEMSHMI("pcs_%dcont_can_baudrate[%d]\n", g_vems.pcs_cnt + 1, ppcs->cont_can_baudrate);
            } else {

            }
            ppcs->state = 1;
            LogEMSHMI("pcs_%dcont_can_baudrate_indx1[%x]\n", g_vems.pcs_cnt + 1, pscreen->rdata[byte_indx]);
            byte_indx += 1;
            /*添加数据库存储接口 存储成功设备数量加1*/
            if(dev_pcs_insert(ppcs, 0) == 0) {
                hmi_show_msgbox(pcontext[8], ptitle[0], BUTTON_OK, ICON_INFO);
                memcpy(&g_vems.pcs[g_vems.pcs_cnt].dbcfg, ppcs, sizeof(dev_pcs_params_t));
                pcs_name_change_flag = 1;
                pcs_info_change_flag = 1;
                g_vems.pcs_cnt++;
                LogEMSHMI("memmory pcs_dev%d sucess\n", g_vems.pcs_cnt);
                /*pcs选择控件偏移指向改变后的pcs设备*/
                pscreen->wdata[26] = g_vems.pcs_cnt - 1;
                ret = modbus_write_registers(g_ctx, PCS_SWITCH_ADDR, 1, &pscreen->wdata[26]);
                if(ret == 1) {
                    current_pcs_id = g_vems.pcs_cnt - 1;
                    LogEMSHMI("current pcs_dev num_%d sucess\n", current_pcs_id + 1);
                } else {

                }
            } else {
                pcs_info_change_flag = 1;
                hmi_show_msgbox(pcontext[9], ptitle[0], BUTTON_OK, ICON_INFO);
                LogEMSHMI("memmory pcs_dev%d fail\n", g_vems.pcs_cnt + 1);
            }

        }
    }
    pscreen->rdata[20] = 0;
    pscreen->rdata[21] = 0;
    pscreen->wdata[26] = 0;
    /*复位输入框的数据*/
    ret = modbus_write_registers(g_ctx, PCS_NAME_ADDR, 1, (uint16_t*)&pscreen->wdata[26]);
    if(ret == 1) {
        LogEMSHMI("reset pcs_dev_name num_%d sucess\n", g_vems.pcs_cnt);
    } else {

    }
    /*复位增加设备按钮的数据*/
    ret = modbus_write_registers(g_ctx, PCS_ADD_ADDR, 1, (uint16_t*)&pscreen->rdata[20]);
    if(ret == 1) {
        LogEMSHMI("add pcs_dev num_%d sucess\n", g_vems.pcs_cnt);
    } else {

    }
    /*复位新增按钮的数据*/
    ret = modbus_write_registers(g_ctx, PCS_CHECK_ADDR, 1, (uint16_t*)&pscreen->rdata[21]);
    if(ret == 1) {
        LogEMSHMI("tell qiantai add pcs_dev num_%d sucess\n", g_vems.pcs_cnt);
    } else {

    }

}
/*******************************************************************************
 * @brief
 * @param pscreen
 * @param ctrl_id
 * @return
*******************************************************************************/
void hmi_comm_change_pcs(struct hmi_screen* pscreen, int ctrl_id)
{
    int ret = 0;
    int byte_indx = 0;
    int i = 0;
    uint32_t btn = 0;
    dev_pcs_params_t  pcs_dev;
    dev_pcs_params_t* ppcs = &pcs_dev;

    /*监听到修改pcs设备控件值发生变化去读取寄存器变化的值*/
    ret = modbus_read_registers(g_ctx, PCS_CHANGE_ADDR, 1, &pscreen->rdata[23]);
    if((ret == 1) && (pscreen->rdata[23] == 1)) {
        btn = hmi_show_msgbox(pcontext[0], ptitle[0], BUTTON_OK | BUTTON_NO, ICON_INFO);
        if(btn == BUTTON_NO) {
            /*复位改变按钮数据*/
            pscreen->rdata[23] = 0;
            ret = modbus_write_registers(g_ctx, PCS_CHANGE_ADDR, 1, (uint16_t*)&pscreen->rdata[23]);
            return;
        }
        LogEMSHMI("start change pcs_dev num_%d\n", current_pcs_id + 1);
    } else {
        return;
    }
    /*检测需要改变哪一个设备的参数*/
    ret = modbus_read_registers(g_ctx, PCS_SWITCH_ADDR, 1, &pscreen->rdata[22]);
    if(ret == 1) {
        current_pcs_id = pscreen->rdata[22];
        LogEMSHMI("change current pcs_dev num_%d sucess\n", current_pcs_id + 1);
    } else {
        return;
    }

    ret = modbus_read_registers(g_ctx, PCS_ID_ADDR, 17, &pscreen->rdata[0]);
    if(ret == 17) {
        if(g_vems.pcs_cnt <= EMS_PCS_MAX) {
            /*保留pcs编号*/
            ppcs->pcs_id = g_vems.pcs[current_pcs_id].dbcfg.pcs_id;
            byte_indx += 1;
            /*pcs名称*/
            memcpy(ppcs->pcs_name, &g_vems.pcs[current_pcs_id].dbcfg.pcs_name, sizeof(ppcs->pcs_name));

            for(i = 0; i < 16; i++) {
                LogEMSHMI("pcs_%dname[%x]\n", current_pcs_id + 1, ppcs->pcs_name[i]);
            }
            byte_indx += 8;
            /*pcsmodbusid*/
            ppcs->modbus_id = pscreen->rdata[byte_indx];
            LogEMSHMI("pcs_%dmodbus_id[%x]\n", current_pcs_id + 1, pscreen->rdata[byte_indx]);
            byte_indx += 1;
            /*波特率*/
            if((pscreen->rdata[byte_indx]) == 0) {
                ppcs->baud_rate = 9600;
                LogEMSHMI("pcs_%dbaud_rate[%d]\n", current_pcs_id + 1, ppcs->baud_rate);

            } else if((pscreen->rdata[byte_indx]) == 1) {
                ppcs->baud_rate = 19200;
                LogEMSHMI("pcs_%dbaud_rate[%d]\n", current_pcs_id + 1, ppcs->baud_rate);
            } else {

            }
            LogEMSHMI("pcs_%dbaud_rate_indx1[%x]\n", current_pcs_id + 1, pscreen->rdata[byte_indx]);
            byte_indx += 1;
            /*额定功率*/
            ppcs->rated_power = (pscreen->rdata[byte_indx]);
            LogEMSHMI("pcs_%drated_power[%d]\n", current_pcs_id + 1, ppcs->rated_power);
            byte_indx += 1;
            /*最大电压*/
            ppcs->max_voltage = (pscreen->rdata[byte_indx]);
            LogEMSHMI("pcs_%dmax_voltage[%d]\n", current_pcs_id + 1, ppcs->max_voltage);
            byte_indx += 1;
            /*最大电流*/
            ppcs->max_current = (pscreen->rdata[byte_indx]);
            LogEMSHMI("pcs_%dmax_current[%d]\n", current_pcs_id + 1, ppcs->max_current);

            byte_indx += 1;
            /*投切控制器ID*/
            ppcs->cont_comm_id = pscreen->rdata[byte_indx];
            LogEMSHMI("pcs_%dcont_comm_id[%x]\n", current_pcs_id + 1, pscreen->rdata[byte_indx]);
            byte_indx += 1;
            /*投切控制器CAN地址*/
            ppcs->cont_can_addr = pscreen->rdata[byte_indx];
            LogEMSHMI("pcs_%dcont_can_addr[%x]\n", current_pcs_id + 1, pscreen->rdata[byte_indx]);
            byte_indx += 1;
            /*投切控制器CAN波特率*/
            if(((pscreen->rdata[byte_indx])) == 0) {
                ppcs->cont_can_baudrate = 250;
                LogEMSHMI("pcs_%dcont_can_baudrate[%d]\n", current_pcs_id + 1, ppcs->cont_can_baudrate);
            } else if(((pscreen->rdata[byte_indx])) == 1) {
                ppcs->cont_can_baudrate = 500;
                LogEMSHMI("pcs_%dcont_can_baudrate[%d]\n", current_pcs_id + 1, ppcs->cont_can_baudrate);
            } else if(((pscreen->rdata[byte_indx])) == 2) {
                ppcs->cont_can_baudrate = 1000;
                LogEMSHMI("pcs_%dcont_can_baudrate[%d]\n", current_pcs_id + 1, ppcs->cont_can_baudrate);
            } else {

            }
            LogEMSHMI("pcs_%dcont_can_baudrate_indx1[%x]\n", current_pcs_id + 1, pscreen->rdata[byte_indx]);
            byte_indx += 1;
            /*添加数据库存储接口*/
            if(dev_pcs_update(ppcs, 0, 0) == 0) {
                memcpy(&g_vems.pcs[current_pcs_id].dbcfg, ppcs, sizeof(dev_pcs_params_t));
                pcs_info_change_flag = 1;
                LogEMSHMI("updata pcs_dev num_%d sucess\n", current_pcs_id + 1);
                hmi_show_msgbox(pcontext[1], ptitle[0], BUTTON_OK, ICON_INFO);
            } else {
                hmi_show_msgbox(pcontext[2], ptitle[0], BUTTON_OK, ICON_ERROR);
                LogEMSHMI("updata pcs_dev num_%d failed\n", current_pcs_id + 1);
                pcs_info_change_flag = 1;
            }
        }
        /*复位改变按钮数据*/
        pscreen->rdata[23] = 0;
        ret = modbus_write_registers(g_ctx, PCS_CHANGE_ADDR, 1, (uint16_t*)&pscreen->rdata[23]);
        if(ret == 1) {
            LogEMSHMI("change pcs_dev num_%d sucess\n", current_pcs_id + 1);
        } else {

        }
    }
}
/*******************************************************************************
 * @brief
 * @param pscreen
 * @param ctrl_id
 * @return
*******************************************************************************/
void hmi_comm_del_pcs(struct hmi_screen* pscreen, int ctrl_id)
{
    int ret = 0, i = 0;
    dev_pcs_params_t* ppcs = NULL;
    uint32_t btn;
    static int last_dev = 10;
    static int current_dev = 0;
    /*如果当前有设备*/

    if(g_vems.pcs_cnt != 0) {
        /*检测到删除按钮数据*/
        ret = modbus_read_registers(g_ctx, PCS_DEL_ADDR, 1, &pscreen->rdata[0]);
        if((ret == 1) && (pscreen->rdata[0] == 1)) {
            btn = hmi_show_msgbox(pcontext[3], ptitle[0], BUTTON_OK | BUTTON_NO, ICON_INFO);
            if(btn == BUTTON_NO) {
                /*复位改变按钮数据*/
                pscreen->wdata[0] = 0;
                ret = modbus_write_registers(g_ctx, PCS_DEL_ADDR, 1, (uint16_t*)&pscreen->wdata[0]);
                return;
            }
            /*删除当前设备*/
            ret = modbus_read_registers(g_ctx, PCS_SWITCH_ADDR, 1, &pscreen->rdata[22]);
            if(ret == 1) {
                current_pcs_id = pscreen->rdata[22];
                LogEMSHMI("delete current pcs_dev num_%d sucess\n", current_pcs_id + 1);
            } else {
                return;
            }
            /*清空设备*/
            ppcs = &g_vems.pcs[current_pcs_id].dbcfg;
            /*加上存储接口*/
            if(hmi_pcs_dev_move(&g_vems.pcs[current_pcs_id].dbcfg, g_vems.pcs_cnt, current_pcs_id) == 0) {
                LogEMSHMI("del pcs%d_dev num_%d sucess\n", current_pcs_id + 1, ppcs->pcs_id);
                pcs_name_change_flag = 1;
                pcs_info_change_flag = 1;
                hmi_comm_show_pcs_info(current_pcs_id);
                g_vems.pcs_cnt--;
                for(i = g_vems.pcs_cnt; i < EMS_PCS_MAX; i++) {
                    memset(&g_vems.pcs[i].dbcfg, 0, sizeof(dev_pcs_params_t));
                }
                hmi_show_msgbox(pcontext[4], ptitle[0], BUTTON_OK, ICON_INFO);
                /*pcs选择控件偏移指向改变后的pcs设备*/
                pscreen->wdata[26] = g_vems.pcs_cnt - 1;
                modbus_write_registers(g_ctx, PCS_SWITCH_ADDR, 1, &pscreen->wdata[26]);
            } else {
                hmi_show_msgbox(pcontext[5], ptitle[0], BUTTON_OK, ICON_INFO);
                LogEMSHMI("del pcs%d_dev num_%d failed\n", current_pcs_id + 1, ppcs->pcs_id);
            }
            pscreen->wdata[0] = 0;
            ret = modbus_write_registers(g_ctx, PCS_DEL_ADDR, 1, &pscreen->wdata[0]);
            if(ret == 1) {
                LogEMSHMI("tell qiantai del pcs_dev num_%d sucess\n", current_pcs_id + 1);
            } else {

            }
        }
    }
    /*显示设备名称*/
    hmi_comm_show_pcs_name();
    /*检测有没有切换过设备设备*/
    ret = modbus_read_registers(g_ctx, PCS_SWITCH_ADDR, 1, &pscreen->rdata[0]);
    if(ret == 1) {
        current_dev = pscreen->rdata[0];
        if(last_dev != current_dev) {
            pcs_info_change_flag = 1;
            last_dev = current_dev;
        }
    } else {
        return;
    }
    /*切换过设备*/
    if(pcs_info_change_flag == 1) {
        /*显示切换后的设备成功*/
        if(hmi_comm_show_pcs_info(current_pcs_id) != 0) {
            pcs_info_change_flag = 0;
        }
    }
}
/*******************************************************************************
 * @brief 显示充放电时间
 * @return
*******************************************************************************/
void hmi_comm_show_time(void)
{
    uint16_t wdata[25] = { 0 };
    int ret = 0;
    wdata[0] = g_vems.tmsp.charge_tmsp1.start_hour;
    wdata[1] = g_vems.tmsp.charge_tmsp1.end_hour;
    wdata[2] = g_vems.tmsp.charge_tmsp1.power;
    wdata[3] = g_vems.tmsp.charge_tmsp1.enable;

    wdata[4] = g_vems.tmsp.discharge_tmsp1.start_hour;
    wdata[5] = g_vems.tmsp.discharge_tmsp1.end_hour;
    wdata[6] = g_vems.tmsp.discharge_tmsp1.power;
    wdata[7] = g_vems.tmsp.discharge_tmsp1.enable;

    wdata[8] = g_vems.tmsp.charge_tmsp2.start_hour;
    wdata[9] = g_vems.tmsp.charge_tmsp2.end_hour;
    wdata[10] = g_vems.tmsp.charge_tmsp2.power;
    wdata[11] = g_vems.tmsp.charge_tmsp2.enable;

    wdata[12] = g_vems.tmsp.discharge_tmsp2.start_hour;
    wdata[13] = g_vems.tmsp.discharge_tmsp2.end_hour;
    wdata[14] = g_vems.tmsp.discharge_tmsp2.power;
    wdata[15] = g_vems.tmsp.discharge_tmsp2.enable;

    wdata[16] = g_vems.tmsp.night_es.start_hour;
    wdata[17] = g_vems.tmsp.night_es.end_hour;
    wdata[18] = g_vems.tmsp.night_es.power;
    wdata[19] = g_vems.tmsp.night_es.enable;

    ret = modbus_write_registers(g_ctx, CHARGE_TIME_1, 20, wdata);
    if(ret == 20) {
        LogEMSHMI("change  charge time sucess\n");
    } else {
        LogEMSHMI("change  charge time failed\n");
    }
}
/*******************************************************************************
 * @brief 充放电时段管理
 * @param pscreen
 * @param ctrl_id
 * @return
*******************************************************************************/
void hmi_comm_change_charge(struct hmi_screen* pscreen, int ctrl_id)
{
    int ret = 0;
    int i = 0;
    ems_tmsp_item_t tmsp;
    uint32_t btn;
    /*读取修改寄存器需不需要修改充放电时间*/
    ret = modbus_read_registers(g_ctx, CHANGE_TIME_ADDR, 1, &pscreen->rdata[0]);
    if((ret == 1) && (pscreen->rdata[0] == 1)) {
        /*检测弹窗信息*/
        btn = hmi_show_msgbox(pcontext[0], ptitle[0], BUTTON_OK | BUTTON_NO, ICON_INFO);
        if(btn == BUTTON_NO) {
            /*复位修改按钮*/
            pscreen->wdata[0] = 0;
            modbus_write_registers(g_ctx, CHANGE_TIME_ADDR, 1, &pscreen->wdata[0]);
            return;
        }

        /*复位修改按钮*/
        pscreen->wdata[0] = 0;
        modbus_write_registers(g_ctx, CHANGE_TIME_ADDR, 1, &pscreen->wdata[0]);
        ret = modbus_read_registers(g_ctx, CHARGE_TIME_1, 20, &pscreen->rdata[1]);
        if(ret == 20) {
            for(i = 0; i < 20; i++) {
                LogEMSHMI("read time mang data [%d]", pscreen->rdata[i + 1]);
            }
            /*充电时段1*/
            tmsp.start_hour = pscreen->rdata[1];
            tmsp.end_hour = pscreen->rdata[2];
            tmsp.power = pscreen->rdata[3];
            tmsp.enable = pscreen->rdata[4];
            if(charge_time_update("charge_tmsp1", &tmsp) == 0) {
                memcpy(&g_vems.tmsp.charge_tmsp1, &tmsp, sizeof(ems_tmsp_item_t));
                time_change = 1;
            }
            /*放电时段1*/
            tmsp.start_hour = pscreen->rdata[5];
            tmsp.end_hour = pscreen->rdata[6];
            tmsp.power = pscreen->rdata[7];
            tmsp.enable = pscreen->rdata[8];
            if(charge_time_update("discharge_tmsp1", &tmsp) == 0) {
                memcpy(&g_vems.tmsp.discharge_tmsp1, &tmsp, sizeof(ems_tmsp_item_t));
                time_change = 1;
            }

            /*充电时段2*/
            tmsp.start_hour = pscreen->rdata[9];
            tmsp.end_hour = pscreen->rdata[10];
            tmsp.power = pscreen->rdata[11];
            tmsp.enable = pscreen->rdata[12];
            if(charge_time_update("charge_tmsp2", &tmsp) == 0) {
                memcpy(&g_vems.tmsp.charge_tmsp2, &tmsp, sizeof(ems_tmsp_item_t));
                time_change = 1;
            }
            /*放电时段2*/
            tmsp.start_hour = pscreen->rdata[13];
            tmsp.end_hour = pscreen->rdata[14];
            tmsp.power = pscreen->rdata[15];
            tmsp.enable = pscreen->rdata[16];
            if(charge_time_update("discharge_tmsp2", &tmsp) == 0) {
                memcpy(&g_vems.tmsp.discharge_tmsp2, &tmsp, sizeof(ems_tmsp_item_t));
                time_change = 1;
            }
            /*夜间储能时段*/
            tmsp.start_hour = pscreen->rdata[17];
            tmsp.end_hour = pscreen->rdata[18];
            tmsp.power = pscreen->rdata[19];
            tmsp.enable = pscreen->rdata[20];
            if(charge_time_update("night_es_tmsp", &tmsp) == 0) {
                memcpy(&g_vems.tmsp.night_es, &tmsp, sizeof(ems_tmsp_item_t));
                time_change = 1;
                hmi_show_msgbox(pcontext[1], ptitle[0], BUTTON_OK, ICON_INFO);
            } else {
                hmi_show_msgbox(pcontext[2], ptitle[0], BUTTON_OK, ICON_INFO);
            }
        }
    }

    /*当时间配置发生变化的时候重新刷新一次参数*/
    if(time_change == 1) {
        hmi_comm_show_time();
        time_change = 0;
    }

}
/*******************************************************************************
 * @brief 显示系统配置参数
 * @return
*******************************************************************************/
void hmi_comm_show_system_cfg(void)
{
    uint16_t wdata[25] = { 0 };
    int ret = 0;

    wdata[0] = g_vems.syscfg.mode;
    wdata[1] = g_vems.syscfg.discharge_min_soc;
    wdata[2] = g_vems.syscfg.charge_max_soc;
    wdata[3] = g_vems.syscfg.fnl_en;
    wdata[4] = g_vems.syscfg.debug_mode_power;
    wdata[5] = g_vems.syscfg.on_grid_power;
    wdata[6] = g_vems.syscfg.bms_power;
    wdata[7] = g_vems.syscfg.load_power;

    ret = modbus_write_registers(g_ctx, SYSTEM_MODE_ADDR, 8, wdata);
    if(ret == 8) {
        LogEMSHMI("change  syetem config sucess\n");
    } else {
        LogEMSHMI("change  charge config failed\n");
    }
}
/*******************************************************************************
 * @brief 修改系统配置数据
 * @param pscreen
 * @param ctrl_id
 * @return
*******************************************************************************/
void hmi_comm_change_system_config(struct hmi_screen* pscreen, int ctrl_id)
{
    int ret = 0;
    int i = 0;
    uint32_t btn;
    ems_system_config_t syscfg;
    /*读取修改寄存器需不需要修改系统配置*/
    ret = modbus_read_registers(g_ctx, SYSTEM_PARA_CHANGE_ADDR, 1, &pscreen->rdata[0]);
    if((ret == 1) && (pscreen->rdata[0] == 1)) {
        /*检测弹窗信息*/
        btn = hmi_show_msgbox(pcontext[0], ptitle[0], BUTTON_OK | BUTTON_NO, ICON_INFO);
        if(btn == BUTTON_NO) {
            /*复位修改按钮*/
            pscreen->wdata[0] = 0;
            modbus_write_registers(g_ctx, SYSTEM_PARA_CHANGE_ADDR, 1, &pscreen->wdata[0]);
            return;
        }
        /*复位修改按钮*/
        pscreen->wdata[0] = 0;
        modbus_write_registers(g_ctx, SYSTEM_PARA_CHANGE_ADDR, 1, &pscreen->wdata[0]);
        ret = modbus_read_registers(g_ctx, SYSTEM_MODE_ADDR, 8, &pscreen->rdata[1]);
        if(ret == 8) {
            for(i = 0; i < 8; i++) {
                LogEMSHMI("read system config data [%d]", pscreen->rdata[i + 1]);
            }
            syscfg.mode = pscreen->rdata[1];
            syscfg.discharge_min_soc = pscreen->rdata[2];
            syscfg.charge_max_soc = pscreen->rdata[3];
            syscfg.fnl_en = pscreen->rdata[4];
            syscfg.debug_mode_power = pscreen->rdata[5];
            syscfg.on_grid_power = pscreen->rdata[6];
            syscfg.bms_power = pscreen->rdata[7];
            syscfg.load_power = pscreen->rdata[8];
            if(system_para_update("syscfg", &syscfg) == 0) {
                syscfg_change = 1;
                memcpy(&g_vems.syscfg, &syscfg, sizeof(ems_system_config_t));
                hmi_show_msgbox(pcontext[1], ptitle[0], BUTTON_OK, ICON_INFO);
            } else {
                hmi_show_msgbox(pcontext[2], ptitle[0], BUTTON_OK, ICON_INFO);
            }
        }
    }
    /*系统参数配置发生变化后重新刷新次页面*/
    if(syscfg_change == 1) {
        hmi_comm_show_system_cfg();
        syscfg_change = 0;
    }

}
/*******************************************************************************
 * @brief 显示通信参数
 * @return
*******************************************************************************/
void hmi_comm_show_conn_cfg(void)
{
    uint16_t wdata[25] = { 0 };
    int ret = 0;
    int len = 0;
    int len1 = 0;
    int i = 0;

    len = strlen(g_vems.concfg.cloud_ip);

    LogEMSHMI("cloud_ip:%s\n", g_vems.concfg.cloud_ip);
    /*拼接前台显示设备名称数据*/
    if(len % 2 == 0) {
        for(i = 0; i < len; i += 2) {
            wdata[len1++] = g_vems.concfg.cloud_ip[i] << 8 | g_vems.concfg.cloud_ip[i + 1];
            LogEMSHMI("wdata[%d] :[%d]", len1 - 1, wdata[len1 - 1]);
        }
    } else {
        for(i = 0; i < len + 1; i += 2) {
            wdata[len1++] = g_vems.concfg.cloud_ip[i] << 8 | g_vems.concfg.cloud_ip[i + 1];
            LogEMSHMI("wdata[%d] :[%d]", len1 - 1, wdata[len1 - 1]);
        }
    }

    wdata[9] = g_vems.concfg.cloud_port;
    wdata[10] = g_vems.concfg.modbus_id;

    if(g_vems.concfg.modbus_baud == 9600) {
        wdata[11] = 0;
    } else if(g_vems.concfg.modbus_baud == 19200) {
        wdata[11] = 1;
    } else {

    }

    if(g_vems.concfg.hmi_baud == 9600) {
        wdata[12] = 0;
    } else if(g_vems.concfg.hmi_baud == 19200) {
        wdata[12] = 1;
    } else {

    }

    if(g_vems.concfg.can_baud == 250) {
        wdata[13] = 0;
    } else if(g_vems.concfg.can_baud == 500) {
        wdata[13] = 1;
    } else if(g_vems.concfg.can_baud == 1000) {
        wdata[13] = 2;
    } else {

    }

    ret = modbus_write_registers(g_ctx, CONN_PARA_IP_ADDR, 14, wdata);
    LogEMSHMI("show conn cfgpara %s", ret == 14 ? "success" : "failed");
}
/*******************************************************************************
 * @brief 修改系统配置-通信参数配置
 * @param pscreen
 * @param ctrl_id
 * @return
*******************************************************************************/
void hmi_comm_change_conn_config(struct hmi_screen* pscreen, int ctrl_id)
{
    int ret = 0;
    int i = 0;
    uint32_t btn;
    ems_conn_para_config_t concfg = { 0 };

    /*读取修改寄存器需不需要修改通信参数配置*/
    ret = modbus_read_registers(g_ctx, CONN_PARA_CHANGE_ADDR, 1, &pscreen->rdata[0]);
    if((ret == 1) && (pscreen->rdata[0] == 1)) {
        /*检测弹窗信息*/
        btn = hmi_show_msgbox(pcontext[0], ptitle[0], BUTTON_OK | BUTTON_NO, ICON_INFO);
        if(btn == BUTTON_NO) {
            /*复位修改按钮*/
            pscreen->wdata[0] = 0;
            modbus_write_registers(g_ctx, CONN_PARA_CHANGE_ADDR, 1, &pscreen->wdata[0]);
            return;
        }
        /*复位修改按钮*/
        pscreen->wdata[0] = 0;
        modbus_write_registers(g_ctx, CONN_PARA_CHANGE_ADDR, 1, &pscreen->wdata[0]);
        ret = modbus_read_registers(g_ctx, CONN_PARA_IP_ADDR, 14, &pscreen->rdata[1]);
        if(ret == 14) {
            for(i = 0; i < 14; i++) {
                LogEMSHMI("read conn config data [%d]", pscreen->rdata[i + 1]);
            }
            /*云平台IP*/
            concfg.cloud_ip[0] = (pscreen->rdata[1] >> 8) & 0xff;
            concfg.cloud_ip[1] = (pscreen->rdata[1]) & 0xff;
            concfg.cloud_ip[2] = (pscreen->rdata[2] >> 8) & 0xff;
            concfg.cloud_ip[3] = (pscreen->rdata[2]) & 0xff;
            concfg.cloud_ip[4] = (pscreen->rdata[3] >> 8) & 0xff;
            concfg.cloud_ip[5] = (pscreen->rdata[3]) & 0xff;
            concfg.cloud_ip[6] = (pscreen->rdata[4] >> 8) & 0xff;
            concfg.cloud_ip[7] = (pscreen->rdata[4]) & 0xff;
            concfg.cloud_ip[8] = (pscreen->rdata[5] >> 8) & 0xff;
            concfg.cloud_ip[9] = (pscreen->rdata[5]) & 0xff;
            concfg.cloud_ip[10] = (pscreen->rdata[6] >> 8) & 0xff;
            concfg.cloud_ip[11] = (pscreen->rdata[6]) & 0xff;
            concfg.cloud_ip[12] = (pscreen->rdata[7] >> 8) & 0xff;
            concfg.cloud_ip[13] = (pscreen->rdata[7]) & 0xff;
            concfg.cloud_ip[14] = (pscreen->rdata[8] >> 8) & 0xff;
            concfg.cloud_ip[15] = (pscreen->rdata[8]) & 0xff;
            concfg.cloud_ip[16] = 0;

            LogEMSHMI("cloud_ip[%s]", concfg.cloud_ip);

            concfg.cloud_port = pscreen->rdata[10];
            concfg.modbus_id = pscreen->rdata[11];

            if(pscreen->rdata[12] == 0) {
                concfg.modbus_baud = 9600;
            } else if(pscreen->rdata[12] == 1) {
                concfg.modbus_baud = 19200;
            } else {

            }

            if(pscreen->rdata[13] == 0) {
                concfg.hmi_baud = 9600;
            } else if(pscreen->rdata[13] == 1) {
                concfg.hmi_baud = 19200;
            } else {

            }

            if(pscreen->rdata[14] == 0) {
                concfg.can_baud = 250;
            } else if(pscreen->rdata[14] == 1) {
                concfg.can_baud = 500;
            } else if(pscreen->rdata[14] == 2) {
                concfg.can_baud = 1000;
            } else {

            }
            if(conn_para_update("concfg", &concfg) == 0) {
                memcpy(&g_vems.concfg, &concfg, sizeof(ems_conn_para_config_t));
                conn_change = 1;
                hmi_show_msgbox(pcontext[1], ptitle[0], BUTTON_OK, ICON_INFO);
            } else {
                hmi_show_msgbox(pcontext[2], ptitle[0], BUTTON_OK, ICON_INFO);
            }
        }
    }
    if(conn_change == 1) {
        hmi_comm_show_conn_cfg();
        conn_change = 0;
    }
}
/*******************************************************************************
 * @brief PCS 信息显示
 * @param pscreen
 * @param ctrl_id
 * @return
*******************************************************************************/
void hmi_comm_pcs_info_show(struct hmi_screen* pscreen, int ctrl_id)
{
    int ret = 0;
    uint8_t indx = 0;
    pcs_real_data_t* prealdata = NULL;
    /*如果有设备获取当前显示哪一台PCS*/
    if(g_vems.pcs_cnt != 0) {
        ret = modbus_read_registers(g_ctx, PCS_SWITCH_ADDR, 1, &pscreen->rdata[0]);
        if(ret == 1) {
            indx = pscreen->rdata[0];
            LogEMSHMI("current pcs_dev num_%d sucess\n", indx + 1);
            /*上传PCS信息*/
            prealdata = &g_vems.pcs[indx].rtdata;
            /*电网电压  A相电网电压 A相装置电流 A相有功功率 A相无功功率*/
            pscreen->wdata[0] = prealdata->grid_voltage * COMM_DATA_MULTIPLE;
            pscreen->wdata[1] = prealdata->grid_ua * COMM_DATA_MULTIPLE;
            pscreen->wdata[2] = prealdata->pcs_ia * COMM_DATA_MULTIPLE;
            pscreen->wdata[3] = prealdata->pcs_pa * COMM_DATA_MULTIPLE;
            pscreen->wdata[4] = prealdata->pcs_qa * COMM_DATA_MULTIPLE;
            /*A相视在功率 直流母线电压 系统状态 散热器温度 故障启动次数*/
            pscreen->wdata[5] = prealdata->pcs_sa * COMM_DATA_MULTIPLE;
            pscreen->wdata[6] = prealdata->dcbus_voltage * COMM_DATA_MULTIPLE;
            pscreen->wdata[7] = prealdata->sys_state;
            pscreen->wdata[8] = prealdata->radiator_temp * COMM_DATA_MULTIPLE;
            pscreen->wdata[9] = prealdata->fault_start_cnt;

            /*电网频率 B相电网电压 B相装置电流 B相有功功率 B相无功功率*/
            pscreen->wdata[10] = prealdata->gird_freq * COMM_DATA_MULTIPLE;
            pscreen->wdata[11] = prealdata->grid_ub * COMM_DATA_MULTIPLE;
            pscreen->wdata[12] = prealdata->pcs_ib * COMM_DATA_MULTIPLE;
            pscreen->wdata[13] = prealdata->pcs_pb * COMM_DATA_MULTIPLE;
            pscreen->wdata[14] = prealdata->pcs_qb * COMM_DATA_MULTIPLE;
            /*B相视在功率 直流母线电流 系统故障字1 无功给定值*/
            pscreen->wdata[15] = prealdata->pcs_sb * COMM_DATA_MULTIPLE;
            pscreen->wdata[16] = prealdata->dc_current * COMM_DATA_MULTIPLE;
            pscreen->wdata[17] = prealdata->sys_err1;
            pscreen->wdata[18] = prealdata->reactive_power * COMM_DATA_MULTIPLE;


            /*电网状态 C相电网电压 C相装置电流 C相有功功率 C相无功功率 */
            pscreen->wdata[19] = prealdata->grid_state;
            pscreen->wdata[20] = prealdata->grid_uc * COMM_DATA_MULTIPLE;
            pscreen->wdata[21] = prealdata->pcs_ic * COMM_DATA_MULTIPLE;
            pscreen->wdata[22] = prealdata->pcs_pc * COMM_DATA_MULTIPLE;
            pscreen->wdata[23] = prealdata->pcs_qc * COMM_DATA_MULTIPLE;
            /*C相视在功率 外部直流电流 系统故障字2 故障复位次数 有功给定值*/
            pscreen->wdata[24] = prealdata->pcs_sc * COMM_DATA_MULTIPLE;
            pscreen->wdata[25] = prealdata->dc_current_out * COMM_DATA_MULTIPLE;
            pscreen->wdata[26] = prealdata->sys_err2;
            pscreen->wdata[27] = prealdata->fault_reset_cnt;
            pscreen->wdata[28] = prealdata->active_power * COMM_DATA_MULTIPLE;

            modbus_write_registers(g_ctx, PCS_INFO_START_ADDR, 29, &pscreen->wdata[0]);
        } else {

        }
    }
}
/*******************************************************************************
 * @brief bms 信息显示
 * @param pscreen
 * @param ctrl_id
 * @return
*******************************************************************************/
void hmi_comm_bms_info_show(struct hmi_screen* pscreen, int ctrl_id)
{

    int ret = 0;
    uint8_t indx = 0;
    bms_data_t* pbmsdata = NULL;
    uint8_t bmsid = 0;
    /*如果有设备获取当前显示哪一台PCS*/
    if(g_vems.pcs_cnt != 0) {
        ret = modbus_read_registers(g_ctx, PCS_SWITCH_ADDR, 1, &pscreen->rdata[0]);
        if(ret == 1) {
            indx = pscreen->rdata[0];
            bmsid = g_vems.pcs[indx].chg_bat_idx;
            if(bmsid >= 0 && bmsid <= 3) {
                pbmsdata = &g_vems.pcs[indx].bat_rack[bmsid].rtdata;
            }
            LogEMSHMI(" show pcs_%d battery_%d phy info\n", indx + 1, bmsid);
            /*BMS 电池状态 SOC  SOH  总电压 总电流 单体最大电压  单体最小电压 总正继电器状态 充电允许最大电流 放电允许最大电流*/
            if(bmsid == 0) {

                /*隐藏其他bms信息*/
                pscreen->wdata[10] = 1;
                pscreen->wdata[11] = 0;
                pscreen->wdata[12] = 0;
                pscreen->wdata[13] = 0;
                modbus_write_registers(g_ctx, BMS1_HIDE_ADDR, 4, &pscreen->wdata[10]);

                /*其他bms信息状态为等待状态*/
                pscreen->wdata[14] = 0;
                modbus_write_registers(g_ctx, BMS2_BAT_STATUS_ADDR, 1, &pscreen->wdata[14]);
                modbus_write_registers(g_ctx, BMS3_BAT_STATUS_ADDR, 1, &pscreen->wdata[14]);
                modbus_write_registers(g_ctx, BMS4_BAT_STATUS_ADDR, 1, &pscreen->wdata[14]);

                /*显示当前bms实时信息*/
                pscreen->wdata[0] = pbmsdata->bat_state;
                pscreen->wdata[1] = pbmsdata->soc;
                pscreen->wdata[2] = pbmsdata->soh;
                pscreen->wdata[3] = pbmsdata->voltage_total *COMM_DATA_MULTIPLE;
                pscreen->wdata[4] = pbmsdata->current_total *COMM_DATA_MULTIPLE;
                pscreen->wdata[5] = pbmsdata->cell_voltage_max /0.001;
                pscreen->wdata[6] = pbmsdata->cell_voltage_min /0.001;
                pscreen->wdata[7] = pbmsdata->km_state;
                pscreen->wdata[8] = pbmsdata->charge_max_curr/0.1;
                pscreen->wdata[9] = pbmsdata->discharge_max_curr/0.1;
                modbus_write_registers(g_ctx, BMS1_BAT_STATUS_ADDR, 10, &pscreen->wdata[0]);

            } else if(bmsid == 1) {

                /*隐藏其他bms信息*/
                pscreen->wdata[10] = 0;
                pscreen->wdata[11] = 1;
                pscreen->wdata[12] = 0;
                pscreen->wdata[13] = 0;
                modbus_write_registers(g_ctx, BMS1_HIDE_ADDR, 4, &pscreen->wdata[10]);

                /*其他bms信息状态为等待状态*/
                pscreen->wdata[14] = 0;
                modbus_write_registers(g_ctx, BMS1_BAT_STATUS_ADDR, 1, &pscreen->wdata[14]);
                modbus_write_registers(g_ctx, BMS3_BAT_STATUS_ADDR, 1, &pscreen->wdata[14]);
                modbus_write_registers(g_ctx, BMS4_BAT_STATUS_ADDR, 1, &pscreen->wdata[14]);

                /*显示当前bms实时信息*/
                pscreen->wdata[0] = pbmsdata->bat_state;
                pscreen->wdata[1] = pbmsdata->soc;
                pscreen->wdata[2] = pbmsdata->soh;
                pscreen->wdata[3] = pbmsdata->voltage_total * COMM_DATA_MULTIPLE;
                pscreen->wdata[4] = pbmsdata->current_total * COMM_DATA_MULTIPLE;
                pscreen->wdata[5] = pbmsdata->cell_voltage_max / 0.001;
                pscreen->wdata[6] = pbmsdata->cell_voltage_min / 0.001;
                pscreen->wdata[7] = pbmsdata->km_state;
                pscreen->wdata[8] = pbmsdata->charge_max_curr / 0.1;
                pscreen->wdata[9] = pbmsdata->discharge_max_curr / 0.1;

                modbus_write_registers(g_ctx, BMS2_BAT_STATUS_ADDR, 10, &pscreen->wdata[0]);

            } else if(bmsid == 2) {

                /*隐藏其他bms信息*/
                pscreen->wdata[10] = 0;
                pscreen->wdata[11] = 0;
                pscreen->wdata[12] = 1;
                pscreen->wdata[13] = 0;
                modbus_write_registers(g_ctx, BMS1_HIDE_ADDR, 4, &pscreen->wdata[10]);

                /*其他bms信息状态为等待状态*/
                pscreen->wdata[14] = 0;
                modbus_write_registers(g_ctx, BMS1_BAT_STATUS_ADDR, 1, &pscreen->wdata[14]);
                modbus_write_registers(g_ctx, BMS2_BAT_STATUS_ADDR, 1, &pscreen->wdata[14]);
                modbus_write_registers(g_ctx, BMS4_BAT_STATUS_ADDR, 1, &pscreen->wdata[14]);

                /*显示当前bms实时信息*/
                pscreen->wdata[0] = pbmsdata->bat_state;
                pscreen->wdata[1] = pbmsdata->soc;
                pscreen->wdata[2] = pbmsdata->soh;
                pscreen->wdata[3] = pbmsdata->voltage_total * COMM_DATA_MULTIPLE;
                pscreen->wdata[4] = pbmsdata->current_total * COMM_DATA_MULTIPLE;
                pscreen->wdata[5] = pbmsdata->cell_voltage_max / 0.001;
                pscreen->wdata[6] = pbmsdata->cell_voltage_min / 0.001;
                pscreen->wdata[7] = pbmsdata->km_state;
                pscreen->wdata[8] = pbmsdata->charge_max_curr / 0.1;
                pscreen->wdata[9] = pbmsdata->discharge_max_curr / 0.1;
                modbus_write_registers(g_ctx, BMS3_BAT_STATUS_ADDR, 10, &pscreen->wdata[0]);

            } else if(bmsid == 3) {

                /*隐藏其他bms信息*/
                pscreen->wdata[10] = 0;
                pscreen->wdata[11] = 0;
                pscreen->wdata[12] = 0;
                pscreen->wdata[13] = 1;
                modbus_write_registers(g_ctx, BMS1_HIDE_ADDR, 4, &pscreen->wdata[10]);

                /*其他bms信息状态为等待状态*/
                pscreen->wdata[14] = 0;
                modbus_write_registers(g_ctx, BMS1_BAT_STATUS_ADDR, 1, &pscreen->wdata[14]);
                modbus_write_registers(g_ctx, BMS2_BAT_STATUS_ADDR, 1, &pscreen->wdata[14]);
                modbus_write_registers(g_ctx, BMS3_BAT_STATUS_ADDR, 1, &pscreen->wdata[14]);

                /*显示当前bms实时信息*/
                pscreen->wdata[0] = pbmsdata->bat_state;
                pscreen->wdata[1] = pbmsdata->soc;
                pscreen->wdata[2] = pbmsdata->soh;
                pscreen->wdata[3] = pbmsdata->voltage_total * COMM_DATA_MULTIPLE;
                pscreen->wdata[4] = pbmsdata->current_total * COMM_DATA_MULTIPLE;
                pscreen->wdata[5] = pbmsdata->cell_voltage_max / 0.001;
                pscreen->wdata[6] = pbmsdata->cell_voltage_min / 0.001;
                pscreen->wdata[7] = pbmsdata->km_state;
                pscreen->wdata[8] = pbmsdata->charge_max_curr / 0.1;
                pscreen->wdata[9] = pbmsdata->discharge_max_curr / 0.1;
                modbus_write_registers(g_ctx, BMS4_BAT_STATUS_ADDR, 10, &pscreen->wdata[0]);

            } else {

            }
        }
    }
}
/*******************************************************************************
 * @brief 版本号显示
 * @param pscreen
 * @param ctrl_id
 * @return
*******************************************************************************/
void hmi_comm_version_show(struct hmi_screen* pscreen, int ctrl_id)
{
    uint16_t  wbuff[96] = { 0 };
    char* buff = NULL;
    int len = 0;
    int len1 = 0;
    int i = 0;

    /*如果没有更新版本号 就使用默认版本号*/
    if(strlen(g_vems.version.ems_version) == 0) {
        strncpy(g_vems.version.ems_version, EMS_DEFAULT_VERSION, strlen(EMS_DEFAULT_VERSION));
    }

    if(strlen(g_vems.version.control_version) == 0) {
        strncpy(g_vems.version.control_version, CON_DEFAULT_VERSION, strlen(CON_DEFAULT_VERSION));
    }

    if(strlen(g_vems.version.hmi_version) == 0) {
        strncpy(g_vems.version.hmi_version, HMI_DEFAULT_VERSION, strlen(HMI_DEFAULT_VERSION));
    }

    /*拼接ems版本号名称数据*/
    len1 = 0;
    len = strlen(g_vems.version.ems_version);
    buff = g_vems.version.ems_version;

    if(len % 2 == 0) {
        for(i = 0; i < len; i += 2) {
            wbuff[len1++] = buff[i] << 8 | buff[i + 1];
        }
    } else {
        for(i = 0; i < len + 1; i += 2) {
            wbuff[len1++] = buff[i] << 8 | buff[i + 1];
        }
    }
    /*发送ems版本号名称*/
    modbus_write_registers(g_ctx, EMS_VERSION_ADDR, len1, (uint16_t*)wbuff);

    /*拼接控制器版本号名称数据*/
    len1 = 0;
    len = strlen(g_vems.version.control_version);
    buff = g_vems.version.control_version;

    if(len % 2 == 0) {
        for(i = 0; i < len; i += 2) {
            wbuff[len1++] = buff[i] << 8 | buff[i + 1];
        }
    } else {
        for(i = 0; i < len + 1; i += 2) {
            wbuff[len1++] = buff[i] << 8 | buff[i + 1];
        }
    }
    /*发送控制器版本号名称*/
    modbus_write_registers(g_ctx, CON_VERSION_ADDR, len1, (uint16_t*)wbuff);

    /*拼接触摸屏版本号名称数据*/
    len1 = 0;
    len = strlen(g_vems.version.hmi_version);
    buff = g_vems.version.hmi_version;

    if(len % 2 == 0) {
        for(i = 0; i < len; i += 2) {
            wbuff[len1++] = buff[i] << 8 | buff[i + 1];
        }
    } else {
        for(i = 0; i < len + 1; i += 2) {
            wbuff[len1++] = buff[i] << 8 | buff[i + 1];
        }
    }
    /*发送触摸屏版本号名称*/
    modbus_write_registers(g_ctx, HMI_VERSION_ADDR, len1, (uint16_t*)wbuff);

    /*版本号发生改变*/
    if(version_change_flag == 1) {
        ems_version_update("ems_version", g_vems.version.ems_version);
        control_version_update("control_version", g_vems.version.control_version);
        hmi_version_update("hmi_version", g_vems.version.hmi_version);
        version_change_flag = 0;
    }
}
/*******************************************************************************
 * @brief bms告警信息显示
 * @param pscreen
 * @param ctrl_id
 * @return
*******************************************************************************/
void hmi_comm_bms_warn_show(struct hmi_screen* pscreen, int ctrl_id)
{
    int ret = 0;
    uint8_t pcsid = 0;
    uint8_t byte_indx = 0;
    bms_data_t* pbmsdata = NULL;
    uint8_t bmsid = 0;
    system_status_t sys_status;
    alarm_flag1_t alarmL12_flag1;
    alarmL12_flag2_t  alarmL12_flag2;
    alarm_flag1_t alarmL3_flag1;
    alarmL3_flag2_t alarmL3_flag2;

    /*如果有设备获取当前显示哪一台PCS*/
    if(g_vems.pcs_cnt != 0) {
        ret = modbus_read_registers(g_ctx, PCS_SWITCH_ADDR, 1, &pscreen->rdata[0]);
        if(ret == 1) {
            pcsid = pscreen->rdata[0];              //当前在哪台pcs
            bmsid = g_vems.pcs[pcsid].chg_bat_idx;  //获取当前bms编号
            if((bmsid >= 0) && (bmsid <= 3)) {
                /*发送当前bms id*/
                pscreen->wdata[0] = bmsid;
                modbus_write_registers(g_ctx, BMS_ID_ADDR, 1, &pscreen->wdata[0]);

                pbmsdata = &g_vems.pcs[pcsid].bat_rack[bmsid].rtdata;
                /*发送系统状态信息*/
                byte_indx = 1;
                memcpy(&sys_status, &pbmsdata->sys_state, sizeof(uint8_t));
                pscreen->wdata[byte_indx] = sys_status.sys_ready;
                pscreen->wdata[byte_indx + 1] = sys_status.charge_finish;
                pscreen->wdata[byte_indx + 2] = sys_status.discharge_finish;
                pscreen->wdata[byte_indx + 3] = sys_status.one_level_warn;
                pscreen->wdata[byte_indx + 4] = sys_status.two_level_warn;
                pscreen->wdata[byte_indx + 5] = sys_status.three_level_warn;
                pscreen->wdata[byte_indx + 6] = sys_status.reserve1;
                pscreen->wdata[byte_indx + 7] = sys_status.reserve2;
                modbus_write_registers(g_ctx, SYSTEM_STATUS_ADDR, 8, &pscreen->wdata[1]);

                /*发送一级警报信息*/
                memcpy(&alarmL12_flag1, &pbmsdata->alarm11, sizeof(uint8_t));
                memcpy(&alarmL12_flag2, &pbmsdata->alarm12, sizeof(uint8_t));
                byte_indx += 8;
                pscreen->wdata[byte_indx] = alarmL12_flag1.bits.temp_high * (pcsid * 4 + bmsid);
                pscreen->wdata[byte_indx + 1] = alarmL12_flag1.bits.temp_low * (pcsid * 4 + bmsid);
                pscreen->wdata[byte_indx + 2] = alarmL12_flag1.bits.temp_diff_big * (pcsid * 4 + bmsid);
                pscreen->wdata[byte_indx + 3] = alarmL12_flag1.bits.tot_vol_high * (pcsid * 4 + bmsid);
                pscreen->wdata[byte_indx + 4] = alarmL12_flag1.bits.tot_vol_low * (pcsid * 4 + bmsid);
                pscreen->wdata[byte_indx + 5] = alarmL12_flag1.bits.cell_vol_high * (pcsid * 4 + bmsid);
                pscreen->wdata[byte_indx + 6] = alarmL12_flag1.bits.cell_vol_low * (pcsid * 4 + bmsid);
                pscreen->wdata[byte_indx + 7] = alarmL12_flag1.bits.cell_vol_diff_big * (pcsid * 4 + bmsid);
                byte_indx += 8;
                pscreen->wdata[byte_indx] = alarmL12_flag2.bits.charge_curr_big * (pcsid * 4 + bmsid);
                pscreen->wdata[byte_indx + 1] = alarmL12_flag2.bits.discharge_curr_big * (pcsid * 4 + bmsid);
                pscreen->wdata[byte_indx + 2] = alarmL12_flag2.bits.rev1 * (pcsid * 4 + bmsid);
                pscreen->wdata[byte_indx + 3] = alarmL12_flag2.bits.soc_low * (pcsid * 4 + bmsid);
                pscreen->wdata[byte_indx + 4] = alarmL12_flag2.bits.jyzk_low * (pcsid * 4 + bmsid);
                pscreen->wdata[byte_indx + 5] = alarmL12_flag2.bits.rev2 * (pcsid * 4 + bmsid);
                pscreen->wdata[byte_indx + 6] = alarmL12_flag2.bits.rev3 * (pcsid * 4 + bmsid);
                pscreen->wdata[byte_indx + 7] = alarmL12_flag2.bits.rev4 * (pcsid * 4 + bmsid);
                modbus_write_registers(g_ctx, ONE_LEVEL_WARN_ADDR, 16, &pscreen->wdata[9]);

                /*发送二级警报信息*/
                memcpy(&alarmL12_flag1, &pbmsdata->alarm21, sizeof(uint8_t));
                memcpy(&alarmL12_flag2, &pbmsdata->alarm22, sizeof(uint8_t));
                byte_indx += 8;
                pscreen->wdata[byte_indx] = alarmL12_flag1.bits.temp_high * (pcsid * 4 + bmsid);
                pscreen->wdata[byte_indx + 1] = alarmL12_flag1.bits.temp_low * (pcsid * 4 + bmsid);
                pscreen->wdata[byte_indx + 2] = alarmL12_flag1.bits.temp_diff_big * (pcsid * 4 + bmsid);
                pscreen->wdata[byte_indx + 3] = alarmL12_flag1.bits.tot_vol_high * (pcsid * 4 + bmsid);
                pscreen->wdata[byte_indx + 4] = alarmL12_flag1.bits.tot_vol_low * (pcsid * 4 + bmsid);
                pscreen->wdata[byte_indx + 5] = alarmL12_flag1.bits.cell_vol_high * (pcsid * 4 + bmsid);
                pscreen->wdata[byte_indx + 6] = alarmL12_flag1.bits.cell_vol_low * (pcsid * 4 + bmsid);
                pscreen->wdata[byte_indx + 7] = alarmL12_flag1.bits.cell_vol_diff_big * (pcsid * 4 + bmsid);
                byte_indx += 8;
                pscreen->wdata[byte_indx] = alarmL12_flag2.bits.charge_curr_big * (pcsid * 4 + bmsid);
                pscreen->wdata[byte_indx + 1] = alarmL12_flag2.bits.discharge_curr_big * (pcsid * 4 + bmsid);
                pscreen->wdata[byte_indx + 2] = alarmL12_flag2.bits.rev1 * (pcsid * 4 + bmsid);
                pscreen->wdata[byte_indx + 3] = alarmL12_flag2.bits.soc_low * (pcsid * 4 + bmsid);
                pscreen->wdata[byte_indx + 4] = alarmL12_flag2.bits.jyzk_low * (pcsid * 4 + bmsid);
                pscreen->wdata[byte_indx + 5] = alarmL12_flag2.bits.rev2 * (pcsid * 4 + bmsid);
                pscreen->wdata[byte_indx + 6] = alarmL12_flag2.bits.rev3 * (pcsid * 4 + bmsid);
                pscreen->wdata[byte_indx + 7] = alarmL12_flag2.bits.rev4 * (pcsid * 4 + bmsid);
                modbus_write_registers(g_ctx, TWO_LEVEL_WARN_ADDR, 16, &pscreen->wdata[25]);

                /*发送三级警报信息*/
                memcpy(&alarmL3_flag1, &pbmsdata->alarm31, sizeof(uint8_t));
                memcpy(&alarmL3_flag2, &pbmsdata->alarm32, sizeof(uint8_t));
                byte_indx += 8;
                pscreen->wdata[byte_indx] = alarmL3_flag1.bits.temp_high * (pcsid * 4 + bmsid);
                pscreen->wdata[byte_indx + 1] = alarmL3_flag1.bits.temp_low * (pcsid * 4 + bmsid);
                pscreen->wdata[byte_indx + 2] = alarmL3_flag1.bits.temp_diff_big * (pcsid * 4 + bmsid);
                pscreen->wdata[byte_indx + 3] = alarmL3_flag1.bits.tot_vol_high * (pcsid * 4 + bmsid);
                pscreen->wdata[byte_indx + 4] = alarmL3_flag1.bits.tot_vol_low * (pcsid * 4 + bmsid);
                pscreen->wdata[byte_indx + 5] = alarmL3_flag1.bits.cell_vol_high * (pcsid * 4 + bmsid);
                pscreen->wdata[byte_indx + 6] = alarmL3_flag1.bits.cell_vol_low * (pcsid * 4 + bmsid);
                pscreen->wdata[byte_indx + 7] = alarmL3_flag1.bits.cell_vol_diff_big * (pcsid * 4 + bmsid);
                byte_indx += 8;
                pscreen->wdata[byte_indx] = alarmL3_flag2.bits.charge_curr_big * (pcsid * 4 + bmsid);
                pscreen->wdata[byte_indx + 1] = alarmL3_flag2.bits.discharge_curr_big * (pcsid * 4 + bmsid);
                pscreen->wdata[byte_indx + 2] = alarmL3_flag2.bits.rev1 * (pcsid * 4 + bmsid);
                pscreen->wdata[byte_indx + 3] = alarmL3_flag2.bits.rev2 * (pcsid * 4 + bmsid);
                pscreen->wdata[byte_indx + 4] = alarmL3_flag2.bits.rev3 * (pcsid * 4 + bmsid);
                pscreen->wdata[byte_indx + 5] = alarmL3_flag2.bits.rev4 * (pcsid * 4 + bmsid);
                pscreen->wdata[byte_indx + 6] = alarmL3_flag2.bits.ms_comm_fault * (pcsid * 4 + bmsid);
                pscreen->wdata[byte_indx + 7] = alarmL3_flag2.bits.main_comm_fault * (pcsid * 4 + bmsid);
                modbus_write_registers(g_ctx, THREE_LEVEL_WARN_ADDR, 16, &pscreen->wdata[41]);
            }
        }
    }
}
/*******************************************************************************
 * @brief 显示PCS故障状态信息
 * @param pscreen
 * @param ctrl_id
 * @return
*******************************************************************************/
void hmi_comm_pcs_warn_show(struct hmi_screen* pscreen, int ctrl_id)
{
    int ret = 0;
    int pcsid = 0;
    pcs_real_data_t* prealdata = NULL;
    system_error1_bit0_15_t err1;
    system_error2_bit0_15_t err2;
    /*如果有设备获取当前显示哪一台PCS*/
    if(g_vems.pcs_cnt != 0) {
        ret = modbus_read_registers(g_ctx, PCS_SWITCH_ADDR, 1, &pscreen->rdata[0]);
        if(ret == 1) {
            /*当前在哪台pcs*/
            pcsid = pscreen->rdata[0];
            /*上传PCS信息*/
            prealdata = &g_vems.pcs[pcsid].rtdata;
            /*保留 并网状态 启动允许 锁相成功 故障 运行 停机*/
            pscreen->wdata[0] = 0;
            pscreen->wdata[1] = prealdata->grid_state;
            pscreen->wdata[2] = prealdata->coil_state.bits.start_enable;
            pscreen->wdata[3] = prealdata->coil_state.bits.phase_lock;
            pscreen->wdata[4] = prealdata->coil_state.bits.fault;
            pscreen->wdata[5] = prealdata->coil_state.bits.run;
            pscreen->wdata[6] = prealdata->coil_state.bits.stop;
            /*C相软件过流 B相软件过流 A相软件过流 过载 交流软件过压 急停 WN故障2 交流软件欠压*/
            memcpy(&err1, &prealdata->sys_err1, sizeof(uint16_t));
            pscreen->wdata[7] = err1.software_overcur_c;
            pscreen->wdata[8] = err1.software_overcur_b;
            pscreen->wdata[9] = err1.software_overcur_a;
            pscreen->wdata[10] = err1.over_load;
            pscreen->wdata[11] = err1.ac_software_overvol;
            pscreen->wdata[12] = err1.crash_stop;
            pscreen->wdata[13] = err1.wn_error_2;
            pscreen->wdata[14] = err1.ac_software_undervol;
            /*VN故障2 模块过温 UN故障2 下分裂过压 上分裂过压 WP故障1 直流软件过压 VP故障1*/
            pscreen->wdata[15] = err1.vn_error_2;
            pscreen->wdata[16] = err1.module_overtemp;
            pscreen->wdata[17] = err1.un_error_2;
            pscreen->wdata[18] = err1.down_spilt_overvol;
            pscreen->wdata[19] = err1.upper_spilt_overvol;
            pscreen->wdata[20] = err1.wp_error_1;
            pscreen->wdata[21] = err1.dc_software_overvol;
            pscreen->wdata[22] = err1.vp_error_1;
            /*直流软件欠压 UP故障1 INODV3 INODV1 24V电源异常  C相硬件过流 B相硬件过流 A相硬件过流 */
            memcpy(&err2, &prealdata->sys_err2, sizeof(uint16_t));
            pscreen->wdata[23] = err2.dc_software_undervol;
            pscreen->wdata[24] = err2.up_error_1;
            pscreen->wdata[25] = err2.inodv3;
            pscreen->wdata[26] = err2.inodv1;
            pscreen->wdata[27] = err2.power_24v_abnormal;
            pscreen->wdata[28] = err2.hardware_overcur_c;
            pscreen->wdata[29] = err2.hardware_overcur_b;
            pscreen->wdata[30] = err2.hardware_overcur_a;

            /*高穿故障 高压预充失败 低穿故障 电网频率异常 电网电压不平衡*/
            pscreen->wdata[31] = err2.high_punture_fail;
            pscreen->wdata[32] = err2.high_press_precharge_fail;
            pscreen->wdata[33] = err2.low_punture_fail;
            pscreen->wdata[34] = err2.grid_freq_abnormal;
            pscreen->wdata[35] = err2.grid_vol_imbalance;

            modbus_write_registers(g_ctx, PCS_ERROR_START_ADDR, 36, &pscreen->wdata[0]);
        }
    }
}
/*******************************************************************************
 * @brief
 * @param arg
 * @return
*******************************************************************************/
static void* hmi_comm_thread_do(void* arg)
{
    int i = 0;
    int ret = 1;
    uint16_t screen_id = 0;
    hmi_screen_t* phmi_page = NULL;
    //hmi_screen_t hmi_screen;

    do {
        g_ctx = modbus_new_rtu("/dev/ttyS0", 115200, 'N', 8, 1);
        if(g_ctx == NULL) {
            LogEMSHMI("modbus_new_rtu hmi_comm failed\n");
            safe_msleep(500);
        }
    } while(g_ctx == NULL);
    modbus_set_debug(g_ctx, FALSE);
    modbus_set_error_recovery(g_ctx, MODBUS_ERROR_RECOVERY_LINK | MODBUS_ERROR_RECOVERY_PROTOCOL);
    modbus_set_slave(g_ctx, 11);
    while(-1 == modbus_connect(g_ctx)) {
        LogEMSHMI("modbus_connect hmi_comm failed: %s\n", modbus_strerror(errno));
        sleep(3);
    }
    g_vems.th_stat.bits.th_hmi_comm = 1;
    LogInfo("hmi_comm thread modbus channel is ready, start working");

    while(1) {
        ret = modbus_read_registers(g_ctx, LISTEN_SYSTEM_SCREEN_ADDR, 1, &screen_id);
        if((ret == 1) && (screen_id >= 0) && (screen_id < LISTEN_PAGE_NUM)) {
            /*重新切换到当前页面刷新一次页面信息*/
            if(screen_id != 2) {
                pcs_info_change_flag = 1;
            }
            if(screen_id != 3) {
                time_change = 1;
            }
            if(screen_id != 5) {
                syscfg_change = 1;
            }
            if(screen_id != 6) {
                conn_change = 1;
            }

            phmi_page = &hmi_screen_table[screen_id];
            /*修改时间*/
            //hmi_comm_show_rtc();
            //检测当前界面上的监听控件是否有事件产生
            /*实时上报bms故障信息*/
            //hmi_comm_bms_warn_show(&hmi_screen, 0);
            for(i = 0; i < phmi_page->mon_ctrl_cnt; i++) {
                phmi_page->mon_ctrl[i].handle(phmi_page, i);
            }
        } else {
            phmi_page = NULL;
            /*表示从机离线 从连再次刷新PCS信息*/
            pcs_info_change_flag = 1;
            logon_status = 0;
            time_change = 1;
            syscfg_change = 1;
            conn_change = 1;

            LogEMSHMI("read hmi_comm page fail\n");
        }
        safe_msleep(50);
    }
    return (void*)0;
}
/*******************************************************************************
 * @brief
 * @return
*******************************************************************************/
int create_hmi_comm_thread(void)
{
    int ret = -1;
    pthread_t hmi_tid;
    pthread_attr_t hmi_tattr;

    pthread_attr_init(&hmi_tattr);
    pthread_attr_setdetachstate(&hmi_tattr, PTHREAD_CREATE_DETACHED);
    ret = pthread_create(&hmi_tid, &hmi_tattr, hmi_comm_thread_do, NULL);
    if(ret == 0) {
        LogInfo("create hmi comm thread success");
    } else {
        LogError("create hmi comm thread failed: %s", strerror(errno));
    }

    return 0;
}
