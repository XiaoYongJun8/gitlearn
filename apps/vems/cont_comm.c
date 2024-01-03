/*******************************************************************************
 * @             Copyright (c) 2023 LinyPower, All Rights Reserved.
 * @*****************************************************************************
 * @FilePath     : cont_comm.c
 * @Descripttion : 
 * @Author       : xiaoyongjun
 * @E-Mail       : xiaoyongjun@linygroup.cn
 * @Version      : v1.0.0
 * @Date         : 2023-10-19 09:05:01
*******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "debug_and_log.h"
#include "can_util.h"
#include "util.h"
#include "common_define.h"
#include "cont_comm.h"
#include "vems.h"

/*******************************************************************************
 * @brief 控制器控制继电器开合
 * @param pcont 控制器对象
 * @param relay_id 继电器ID, 取值1-4
 * @param relay_oc 继电器开合状态，0开，1合
 * @return 0:success; 1: failed
*******************************************************************************/
int cont_comm_relay_ctrl(ems_cont_t* pcont, uint8_t relay_id, uint8_t relay_oc)
{
    int ret = 0;
    uint32_t can_txid = 0;
    uint8_t data[8] = { 0 };
    cont_ctrl_t* pcont_ctrl = (cont_ctrl_t*)data;
    ems_ext_id_t* pems_eid = (ems_ext_id_t*)&can_txid;

    pcont_ctrl->relay_ctrl.relay.enable = 1;
    if(relay_oc == 0) {
        pcont_ctrl->relay_ctrl.relay.close_id = 0;
    } else {
        pcont_ctrl->relay_ctrl.relay.close_id = relay_id;
        pcont_ctrl->relay_ctrl.byte |= 1 << (relay_id + 3);        //设置relay1--relay4的值 relay_id - 1 + 4
    }
    pems_eid->reserve = 1;
    pems_eid->cmd = CAN_CMD_CONT_CTRL1;
    pems_eid->cont_id = pcont->pcs_idx + 1;
    pems_eid->rack_id = pcont->rtdata.rack_id;

    ret = bms_and_cont_write_data(g_vems.can_sock, can_txid, data);
    return ret;
}

/*******************************************************************************
 * @brief 控制接入的BMS
 * @param pcont 控制器对象
 * @param bms_id BMS ID, 取值1-4
 * @return 0:success; 1: failed
*******************************************************************************/
int cont_comm_bms_can_ctrl(ems_cont_t* pcont, uint8_t bms_id)
{
    int ret = 0;
    uint32_t can_txid = 0;
    uint8_t data[8] = { 0 };
    cont_ctrl_t* pcont_ctrl = (cont_ctrl_t*)data;
    ems_ext_id_t* pems_eid = (ems_ext_id_t*)&can_txid;

    pcont_ctrl->can_ctrl.can.enable = 1;
    pcont_ctrl->can_ctrl.can.conn_id = bms_id;
    pcont_ctrl->can_ctrl.byte |= 1 << (bms_id + 3);        //设置bms1--bms4的值 bms_id - 1 + 4
    pems_eid->reserve = 1;
    pems_eid->cmd = CAN_CMD_CONT_CTRL1;
    pems_eid->cont_id = pcont->pcs_idx + 1;
    pems_eid->rack_id = pcont->rtdata.rack_id;

    ret = bms_and_cont_write_data(g_vems.can_sock, can_txid, data);
    return ret;
}

/*******************************************************************************
 * @brief 控制接入的BMS
 * @param pcont 控制器对象
 * @param io_val 需要控制的IO状态值
 * @return 0:success; 1: failed
*******************************************************************************/
int cont_comm_io_ctrl(ems_cont_t* pcont, uint8_t io_val, enum EMS_RUN_STATE ems_state)
{
    int ret = 0;
    uint32_t can_txid = 0;
    uint8_t data[8] = { 0 };
    cont_ctrl_t* pcont_ctrl = (cont_ctrl_t*)data;
    ems_ext_id_t* pems_eid = (ems_ext_id_t*)&can_txid;

    if((cont_dio_checkbit_var(io_val, IO_LED_RUN_LIGHT) && cont_dio_checkbit_var(io_val, IO_LED_RUN_CLOSE))
    || (cont_dio_checkbit_var(io_val, IO_LED_ERR_LIGHT) && cont_dio_checkbit_var(io_val, IO_LED_ERR_CLOSE))) {
        return 1;
    }
    pcont_ctrl->ems_state = ems_state;
    pcont_ctrl->io_ctrl.bits.enable = 1;
    pcont_ctrl->io_ctrl.bits.led_run_light = cont_dio_getbit_var(io_val, IO_LED_RUN_LIGHT);
    pcont_ctrl->io_ctrl.bits.led_run_close = cont_dio_getbit_var(io_val, IO_LED_RUN_CLOSE);
    pcont_ctrl->io_ctrl.bits.led_err_light = cont_dio_getbit_var(io_val, IO_LED_ERR_LIGHT);
    pcont_ctrl->io_ctrl.bits.led_err_close = cont_dio_getbit_var(io_val, IO_LED_ERR_CLOSE);
    pems_eid->reserve = 1;
    pems_eid->cmd = CAN_CMD_CONT_CTRL1;
    pems_eid->cont_id = pcont->pcs_idx + 1;
    pems_eid->rack_id = pcont->rtdata.rack_id;

    ret = bms_and_cont_write_data(g_vems.can_sock, can_txid, data);
    return ret;
}

void bms_handle_msg_update_rack_id(int pcs_idx, uint16_t now_rack_id)
{
    // if(g_vems.pcs[pcs_idx].cont.rtdata.rack_id_changged == 0 || g_vems.pcs[pcs_idx].cont.rtdata.rack_id == 0) {
    //     g_vems.pcs[pcs_idx].cont.rtdata.rack_id_changged = now_rack_id;
    //     g_vems.pcs[pcs_idx].cont.rtdata.rack_id = now_rack_id;
    // } else if(g_vems.pcs[pcs_idx].cont.rtdata.rack_id != now_rack_id) {
    //     g_vems.pcs[pcs_idx].cont.rtdata.rack_id_changged = g_vems.pcs[pcs_idx].cont.rtdata.rack_id;
    //     g_vems.pcs[pcs_idx].cont.rtdata.rack_id = now_rack_id;
    // }
    //g_vems.pcs[pcs_idx].cont.rtdata.rack_id = now_rack_id;
    g_vems.pcs[pcs_idx].cont.rtdata.rack_id = 0xf1;
}

/*******************************************************************************
 * @brief 读取bms 功能码01 电池组信息
 * @param pcandata can总线数据
 * @param canlen   can数据长度
 * @param pcs_idx    当前pcsid
 * @return 读取成功返回0 失败返回其他
*******************************************************************************/
static int bms_handle_msg01(ems_ext_id_t* pemsid, uint8_t* pcandata, int canlen, int pcs_idx)
{
    int ret = 0, bat_idx = 0;
    battery_rack_t* prack = NULL;
    bms_data_t* pbdata = NULL;

    bat_idx = g_vems.pcs[pcs_idx].cont.rtdata.can_stat.stat.conn_id;
    if(bat_idx <= 0 || bat_idx > BAT_RACK_4) {
        LogWarn("pcs[%d] BMS conn_id=%d is invalid, should[1, %d]", pcs_idx, bat_idx, BAT_RACK_4);
        return 1;
    } else {
        bat_idx -= 1;
    }
    prack = &g_vems.pcs[pcs_idx].bat_rack[bat_idx];
    pbdata = &prack->rtdata;
    pbdata->cell_voltage_max = (pcandata[1] | pcandata[0] << 8) * 0.001;
    pbdata->cell_voltage_min = (pcandata[3] | pcandata[2] << 8) * 0.001;
    pbdata->soc = pcandata[4];
    pbdata->soh = pcandata[5];
    pbdata->km_state = pcandata[7];
    prack->act_state = 1;
    g_vems.pcs[pcs_idx].cont.bms_msg01_rx_cnt++;
    bms_handle_msg_update_rack_id(pcs_idx, pemsid->rack_id);
    LogBMS("got pcs%d.bms%d run_info-01: Vcell_max=%.3f Vcell_min=%.3f soc=%d soh=%d km=%d"
            , pcs_idx, bat_idx, pbdata->cell_voltage_max, pbdata->cell_voltage_min, pbdata->soc, pbdata->soh, pbdata->km_state);

    return ret;
}

/*******************************************************************************
 * @brief 读取bms 功能码02 电池组信息
 * @param pcandata can总线数据
 * @param canlen   can数据长度
 * @param pcs_idx    当前pcsid
 * @return 读取成功返回0 失败返回其他
*******************************************************************************/
static int bms_handle_msg02(ems_ext_id_t* pemsid, uint8_t* pcandata, int canlen, int pcs_idx)
{
    int ret = 0, bat_idx = 0;
    int16_t discharge_curr_max = 0;
    battery_rack_t* prack = NULL;
    bms_data_t* pbdata = NULL;

    bat_idx = g_vems.pcs[pcs_idx].cont.rtdata.can_stat.stat.conn_id;
    if(bat_idx <= 0 || bat_idx > BAT_RACK_4) {
        LogWarn("pcs[%d] conn_id=%d is invalid", pcs_idx, bat_idx);
        return 1;
    } else {
        bat_idx -= 1;
    }
    prack = &g_vems.pcs[pcs_idx].bat_rack[bat_idx];
    pbdata = &prack->rtdata;
    pbdata->voltage_total = (pcandata[1] | pcandata[0] << 8) * 0.1;
    pbdata->current_total = (pcandata[3] | pcandata[2] << 8) * 0.1;
    pbdata->charge_max_curr = (pcandata[5] | pcandata[4] << 8) * 0.1;
    discharge_curr_max = pcandata[7] | pcandata[6] << 8;
    pbdata->discharge_max_curr = discharge_curr_max * 0.1;
    prack->act_state = 1;
    g_vems.pcs[pcs_idx].cont.bms_msg02_rx_cnt++;
    bms_handle_msg_update_rack_id(pcs_idx, pemsid->rack_id);
    LogBMS("got pcs%d.bms%d run_info-02: Vtot=%.1f Ctot=%.1f charge_curr_max=%.1f discharge_curr_max=%.1f"
            , pcs_idx, bat_idx, pbdata->voltage_total, pbdata->current_total, pbdata->charge_max_curr, pbdata->discharge_max_curr);

    return ret;
}

/*******************************************************************************
 * @brief 读取bms 功能码06 电池组信息
 * @param pcandata can总线数据
 * @param canlen   can数据长度
 * @param pcs_idx    当前pcsid
 * @return 读取成功返回0 失败返回其他
*******************************************************************************/
static int bms_handle_msg06(ems_ext_id_t* pemsid, uint8_t* pcandata, int canlen, int pcs_idx)
{
    int ret = 0, bat_idx = 0;
    battery_rack_t* prack = NULL;
    bms_data_t* pbdata = NULL;
    
    bat_idx = g_vems.pcs[pcs_idx].cont.rtdata.can_stat.stat.conn_id;
    if(bat_idx <= 0 || bat_idx > BAT_RACK_4) {
        LogWarn("pcs[%d] conn_id=%d is invalid", pcs_idx, bat_idx);
        return 1;
    } else {
        bat_idx -= 1;
    }
    prack = &g_vems.pcs[pcs_idx].bat_rack[bat_idx];
    pbdata = &prack->rtdata;
    pbdata->bat_state = pcandata[0];
    pbdata->sys_state = pcandata[1];
    pbdata->alarm11.byte = pcandata[2];
    pbdata->alarm12.byte = pcandata[3];
    if(pbdata->alarm11.bits.cell_vol_high) {
        pbdata->bat_chg_dischg_done = 2;    //单体电压高，说明电池已经充满
    } else if(pbdata->alarm11.bits.cell_vol_low) {
        pbdata->bat_chg_dischg_done = 3;    //单体电压低，说明电池已经放空
    } else {
        pbdata->bat_chg_dischg_done = 1;    //电池处于未满未空状态
    }
    prack->act_state = 1;
    g_vems.pcs[pcs_idx].cont.bms_msg06_rx_cnt++;
    bms_handle_msg_update_rack_id(pcs_idx, pemsid->rack_id);
    LogBMS("got pcs%d.bms%d run_info-06: bat_state=%d sys_state=0x%02X alarm11=%02X alarm12=%02X"
            , pcs_idx, bat_idx, pbdata->bat_state, pbdata->sys_state, pbdata->alarm11.byte, pbdata->alarm12.byte);

    return ret;
}

/*******************************************************************************
 * @brief 读取bms 功能码01 电池组信息
 * @param pcandata can总线数据
 * @param canlen   can数据长度
 * @param pcs_idx    当前pcsid
 * @return 读取成功返回0 失败返回其他
*******************************************************************************/
static int bms_handle_msg07(ems_ext_id_t* pemsid, uint8_t* pcandata, int canlen, int pcs_idx)
{
    int ret = 0, bat_idx = 0;
    battery_rack_t* prack = NULL;
    bms_data_t* pbdata = NULL;

    bat_idx = g_vems.pcs[pcs_idx].cont.rtdata.can_stat.stat.conn_id;
    if(bat_idx <= 0 || bat_idx > BAT_RACK_4) {
        LogWarn("pcs[%d] conn_id=%d is invalid", pcs_idx, bat_idx);
        return 1;
    } else {
        bat_idx -= 1;
    }
    prack = &g_vems.pcs[pcs_idx].bat_rack[bat_idx];
    pbdata = &prack->rtdata;
    pbdata->alarm21.byte = pcandata[0];
    pbdata->alarm22.byte = pcandata[1];
    pbdata->alarm31.byte = pcandata[2];
    pbdata->alarm32.byte = pcandata[3];
    if(pbdata->alarm21.bits.cell_vol_high || pbdata->alarm31.bits.cell_vol_high) {
        pbdata->bat_chg_dischg_done = 2;    //单体电压高，说明电池已经充满
    } else if(pbdata->alarm21.bits.cell_vol_low || pbdata->alarm31.bits.cell_vol_low) {
        pbdata->bat_chg_dischg_done = 3;    //单体电压低，说明电池已经放空
    } else {
        pbdata->bat_chg_dischg_done = 1;    //电池处于未满未空状态
    }
    prack->act_state = 1;
    g_vems.pcs[pcs_idx].cont.bms_msg07_rx_cnt++;
    bms_handle_msg_update_rack_id(pcs_idx, pemsid->rack_id);
    LogBMS("got pcs%d.bms%d run_info-07: alarm21=%02X alarm22=%02X alarm31=%02X alarm32=%02X"
            , pcs_idx, bat_idx, pbdata->alarm21.byte, pbdata->alarm22.byte, pbdata->alarm31.byte, pbdata->alarm32.byte);

    return ret;
}

/*******************************************************************************
 * @brief 读取投切控制器 功能码e1 信息
 * @param pcandata can总线数据
 * @param canlen   can数据长度
 * @param pcs_idx    当前pcsid
 * @return 读取成功返回0 失败返回其他
*******************************************************************************/
int cont_handle_msg_e1(uint8_t* pcandata, int canlen, int pcs_idx)
{
    int ret = 0;
    pcs_dev_t* ppcs = NULL;
    cont_data_t* pcont_data = NULL;

    ppcs = &g_vems.pcs[pcs_idx];
    pcont_data = &g_vems.pcs[pcs_idx].cont.rtdata;
    pcont_data->cont_state = pcandata[0];
    pcont_data->gun_stat.byte = pcandata[1];
    pcont_data->can_stat.byte = pcandata[2];
    pcont_data->relay_stat.byte = pcandata[3];
    pcont_data->relay_stat_fb.byte = pcandata[4];
    pcont_data->dio_stat.byte = pcandata[5];
    g_vems.pcs[pcs_idx].cont.bms_msgE1_rx_cnt++;
    // LogBMS("got pcs%d.cont run_info: state=%d gun=%02X can=%02X relay_set=%02X relay_fb=%02X dio=%02X", pcs_idx, pcont_data->cont_state
    //         , pcont_data->gun_stat.byte, pcont_data->can_stat.byte, pcont_data->relay_stat.byte, pcont_data->relay_stat_fb.byte, pcont_data->dio_stat.byte);

    ppcs->bat_rack[0].conn_state = pcont_data->gun_stat.stat.gun1;
    ppcs->bat_rack[1].conn_state = pcont_data->gun_stat.stat.gun2;
    ppcs->bat_rack[2].conn_state = pcont_data->gun_stat.stat.gun3;
    ppcs->bat_rack[3].conn_state = pcont_data->gun_stat.stat.gun4;
    
    return ret;
}

/*******************************************************************************
 * @brief 通过can读取bms和投切控制器的数据
 * @param can_sock can sock
 * @param canid    can报文id
 * @param pbuff    接收Can数据缓存区
 * @param timeout_ms 读取数据超时时间
 * @return 读取成功：返回0  读取失败：返回< 0
*******************************************************************************/
int bms_and_cont_read_data(int can_sock, uint32_t canid, uint8_t* pbuff,int timeout_ms)
{
    int ret = 0;
    struct can_frame frame;
    ret = can_read(can_sock, &frame, timeout_ms);
    if(ret <= 0) {
        LogDebug("read can_id[%x] failed: %s", canid, ret == 0 ? "timeout" : strerror(errno));
        return -1;
    }

    if(frame.can_id & CAN_EFF_FLAG) {
        frame.can_id &= ~(CAN_EFF_FLAG);
    }
    if(frame.can_id == canid) {
        pbuff[0] = frame.data[0];
        pbuff[1] = frame.data[1];
        pbuff[2] = frame.data[2];
        pbuff[3] = frame.data[3];
        pbuff[4] = frame.data[4];
        pbuff[5] = frame.data[5];
        pbuff[6] = frame.data[6];
        pbuff[7] = frame.data[7];
        LogBMS("read can frame[0x%08X, %02X %02X %02X %02X %02X %02X %02X %02X", canid, frame.data[0], frame.data[1], frame.data[2], frame.data[3], frame.data[4], frame.data[5], frame.data[6], frame.data[7]);
    } else {
        LogBMS("read can_id[%x] unmatch want can_id[%x]", frame.can_id, canid);
        return -2;
    }
    return 0;
}
/*******************************************************************************
 * @brief 通过can发送bms和投切控制器的数据
 * @param can_sock can_sock
 * @param canid    can报文id
 * @param pbuff    发送Can数据缓存区
 * @return 发送成功：返回0  发送失败：返回 -1
*******************************************************************************/
int bms_and_cont_write_data(int can_sock, uint32_t canid, uint8_t* pbuff)
{
    int ret = 0;
    struct can_frame frame;

    if((canid >> 11) & 0x3ffff) {
        frame.can_id = canid | CAN_EFF_FLAG;
    } else {
        frame.can_id = canid;
    }
    frame.can_dlc = 8;
    memcpy(frame.data, pbuff, frame.can_dlc);
    ret = can_write(can_sock, (uint8_t*)&frame, sizeof(frame));
    if(ret == -1) {
        //LogDebug("bms_and_cont_write_data() failed:%s", strerror(errno));
        return -1;
    } else {
        LogTrace("bms_and_cont_write_data() success send can frame[0x%08X, %02X %02X %02X %02X %02X %02X %02X %02X]"
                , canid, frame.data[0], frame.data[1], frame.data[2], frame.data[3], frame.data[4], frame.data[5], frame.data[6], frame.data[7]);
    }
    return 0;
}
/*******************************************************************************
 * @brief 设置心跳值
 * @param heart 心跳值
 * @return 
*******************************************************************************/
void bms_set_heart(int pcsid, int batid,uint8_t heart)
{
    uint8_t* pwdata = &g_vems.pcs[pcsid].bat_rack[batid].wdata[0];
    pwdata[0] = heart;
}
/*******************************************************************************
 * @brief 设置状态值
 * @param status    PCS/HPS/PBD状态
 * @return 
*******************************************************************************/
void bms_set_pcs_status(int pcsid, int batid,uint8_t status)
{
    uint8_t* pwdata = &g_vems.pcs[pcsid].bat_rack[batid].wdata[0];
    pwdata[1] = status;
}
/*******************************************************************************
 * @brief  设置电池功率
 * @param power 电池功率
 * @return 
*******************************************************************************/
void bms_set_bat_power(int pcsid, int batid,int16_t power)
{ 
    uint8_t* pwdata = &g_vems.pcs[pcsid].bat_rack[batid].wdata[0];
    pwdata[2] = power & 0xff;
    pwdata[3] = (power>>8) & 0xff;
}

int bms_and_cont_handle_can_msg(struct can_frame* pcmsg)
{
    int ret = 0;
    int pcs_idx = 0;
    ems_ext_id_t* pemsid = NULL;

    pemsid = (ems_ext_id_t*)&pcmsg->can_id;
    pcmsg->can_id &= ~(CAN_EFF_FLAG);       //清除扩展帧标志位
    pcs_idx = pemsid->cont_id;
    if((pcs_idx > PCS_BAT_NUM) || (pcs_idx <= 0)) {
        LogError("this can frame[0x%08X] cont_id[%d] is invalid", pcmsg->can_id, pemsid->cont_id);
        return 1;
    }

    pcs_idx -= 1;
    if(pemsid->cmd == CAN_CMD_BMS_INFO1) {
        ret = bms_handle_msg01(pemsid, pcmsg->data, pcmsg->can_dlc, pcs_idx);
    } else if(pemsid->cmd == CAN_CMD_BMS_INFO2) {
        ret = bms_handle_msg02(pemsid, pcmsg->data, pcmsg->can_dlc, pcs_idx);
    } else if(pemsid->cmd == CAN_CMD_BMS_INFO3) {
        ret = bms_handle_msg06(pemsid, pcmsg->data, pcmsg->can_dlc, pcs_idx);
    } else if(pemsid->cmd == CAN_CMD_BMS_INFO4) {
        ret = bms_handle_msg07(pemsid, pcmsg->data, pcmsg->can_dlc, pcs_idx);
    } else if(pemsid->cmd == CAN_CMD_CONT_INFO1) {
        ret = cont_handle_msg_e1(pcmsg->data, pcmsg->can_dlc, pcs_idx);
    } else {
        LogBMS("receive can frame[0x%08X, %02X %02X %02X %02X %02X %02X %02X %02X], cmd[%02X] unknown", pcmsg->can_id, pcmsg->data[0], pcmsg->data[1]
            , pcmsg->data[2], pcmsg->data[3], pcmsg->data[4], pcmsg->data[5], pcmsg->data[6], pcmsg->data[7], pemsid->cmd);
    }

    return ret;
}

/*******************************************************************************
 * @brief bms和投切控制器数据处理线程
 * @param arg NULL
 * @return 
*******************************************************************************/
static void* bms_and_cont_collect_thread_do(void* arg)
{
    int ret = 0;
    int can_sock = -1;
    int recv_msg_cnt = 0;
    fd_set afds;
    fd_set rfds;
    struct timeval timeout = { 1, 0 };
    struct can_frame rxcmsg;
    struct can_filter filters[] = { {0x10000000, 0x10000000} };  //{ {0x102100F1, CAN_EFF_MASK} }

    while(1) {
        can_sock = can_init(250000, (struct can_filter**)&filters, ARRAR_SIZE(filters));
        if(can_sock > 0) {
            break;
        } else {
            LogDebug("can_init(500000) failed: %s", strerror(errno));
        }
        safe_msleep(1000);
    }
    g_vems.can_sock = can_sock;
    FD_ZERO(&afds);
    FD_SET(can_sock, &afds);

    g_vems.th_stat.bits.th_cont_comm = 1;
    LogInfo("cont communication thread can channel is ready, start working\n");
    while(1) {
        rfds = afds;
        timeout.tv_sec = 1; timeout.tv_usec = 0;
        ret = select(can_sock + 1, &rfds, NULL, NULL, &timeout);
        if(ret > 0) {
            if(FD_ISSET(can_sock, &rfds)) {
                recv_msg_cnt = 0;
                do {
                    ret = recv(can_sock, &rxcmsg, sizeof(struct can_frame), MSG_DONTWAIT);
                    if(ret == sizeof(struct can_frame)) {
                        recv_msg_cnt++;
                        if(bms_and_cont_handle_can_msg(&rxcmsg) == 0) {
                            g_vems.can_nodata_sec = 0;
                        }
                    }
                } while(ret != sizeof(struct can_frame));
                //LogPCS("this times recv_msg_cnt=%d", recv_msg_cnt);
            }
        } else if(ret == 0) {
            g_vems.can_nodata_sec++;
            //LogInfo("read can socket timeout, enter next loop");
        } else {
            LogError("read can socket failed: %s", strerror(errno));
        }
    }
    g_vems.can_sock = -1;
    close(can_sock);
    return (void*)0;
}

/*******************************************************************************
 * @brief  bms和投切控制器主线程创建
 * @return 创建成功：返回0  创建失败：返回1
*******************************************************************************/
int create_cont_comm_thread(void)
{
    int ret = -1;
    pthread_t bc_tid;
    pthread_attr_t bc_tattr;

    pthread_attr_init(&bc_tattr);
    pthread_attr_setdetachstate(&bc_tattr, PTHREAD_CREATE_DETACHED);
    ret = pthread_create(&bc_tid, &bc_tattr, bms_and_cont_collect_thread_do, NULL);
    if(ret == 0) {
        LogInfo("create bms and cont data collect thread success");
    } else {
        LogError("create bms and cont data collect thread failed: %s", strerror(errno));
    }
    return 0;
}


