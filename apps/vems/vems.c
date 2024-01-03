/*******************************************************************************
 * @             Copyright (c) 2023 LinyPower, All Rights Reserved.
 * @*****************************************************************************
 * @FilePath     : vems.c
 * @Descripttion : 
 * @Author       : zhums
 * @E-Mail       : zhums@linygroup.cn
 * @Version      : v1.0.0
 * @Date         : 2023-10-07 15:01:52
*******************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#include "vems.h"
#include "common.h"
#include "config.h"
#include "watchdog.h"
#include "database.h"
#include "debug_and_log.h"
#include "pcs_comm.h"
#include "cont_comm.h"
#include "dlt645_comm.h"
#include "energy_mana.h"
#include "hmi_comm.h"
#include "hmi_db_util.h"
#include "can_util.h"
#include "atool_msq.h"


#define VEMS_DEBUG_LEVEL_FILE     "/home/root/debug/debug_vems"
#define VEMS_LOG_LEVEL_FILE       "/home/root/debug/log_vems"
#define VEMS_LOG_FILE             "/home/root/log/vems.log"
#define VEMS_LOG_MAX_SIZE         2000000

app_log_t log_vems;
vems_t g_vems;
int g_ipc_msgid = 0;

/*********************************************************************
* @fn	    resouce_clean
* @brief    进程资源回收
* @param    void
* @return   void
* @history:
**********************************************************************/
static void resource_clean(void)
{
    dbg_close();
    log_close(&log_vems);
    watchdog_close();
    db_close_database(DATABASE_PATH_DATA);

    can_destroy(-1);
    //DestroyMessageQueue(g_qid_can);
}

/*********************************************************************
* @fn	    sig_catch
* @brief    信号处理函数
* @param    sig_num[in]: 信号
* @return   void
* @history:
**********************************************************************/
static void sig_catch(int signum)
{
    int ret = 0;

    DBG_LOG_MUST(">>>>pcs ems process catches a signal [%d]\n", signum);
    switch(signum) {
    case SIGINT:
    case SIGKILL:
    case SIGTERM:
        DBG_LOG_MUST("============================== vems process exit ==============================");
        resource_clean();
        exit(0);
    case SIGUSR1:
        ret = watchdog_close();
        if(ret != 0) {
            DBG_LOG_MUST("soft_wdt: failed close\n");
        } else {
            DBG_LOG_MUST("watchdog is closed!\n");
        }
        break;
    case SIGUSR2:
        break;

    default:
        break;
    }
}

/*********************************************************************
* @fn	    resource_init
* @brief    进程资源初始化
* @param    void
* @return   0: success -1: fail
* @history:
**********************************************************************/
static int resource_init(void)
{
    int ret = 0;

    ret = DBG_OPEN(VEMS_DEBUG_LEVEL_FILE, LY_DEBUG_OFF);
    if(ret) {
        printf("dbg_open fail!\n");
        return -1;
    }

    ret = resource_init_common();
    if(ret) {
        printf("resource_init_common fail!\n");
        return -1;
    }

    ret = log_open(&log_vems, VEMS_LOG_FILE, VEMS_LOG_LEVEL_FILE, VEMS_LOG_MAX_SIZE, LOG_LV_DEBUG, LOGLV_MODE_LINEAR);
    if(ret) {
        printf("LOG_OPEN fail!\n");
        return -1;
    }
    LogMust("============================== vems process start ==============================");

    print_core_limit();

    signal(SIGINT, sig_catch);
    signal(SIGKILL, sig_catch);
    signal(SIGTERM, sig_catch);
    signal(SIGUSR1, sig_catch);
    signal(SIGUSR2, sig_catch);

    //打开数据库
    ret = db_open_database(DATABASE_PATH_DATA);
    if(ret != 0) {
        printf("open database[%s] failed", DATABASE_PATH_DATA);
        return -1;
    }

    ret = watchdog_open_ex(300, "vems", 0);
    if(ret != 0) {
        printf("open watchdog fail\n");
        return -1;
    }

    if((g_ipc_msgid = atool_msq_get()) != 0) {
        DBG_LOG_ERROR("atool_msq_get() failed: %s", strerror(errno));
        return ret;
    }

    // g_qid_can = GetMessageQueueId(MQ_KEY_CAN);
    // if(g_qid_can == -1) {
    //     DBG_LOG_ERROR("get message queue id[MQ_KEY_CAN=%x] fail: %s\n", MQ_KEY_CAN, strerror(errno));
    //     return -1;
    // }

    return 0;
}

//读取ini file param config 参数配置
int read_ini_config()
{
    int ret = 0;

    //优先读取本程序配置块下的日志配置
    if(config_read_int(CFG_FILE, "vems", "loglevel_mode", &g_vems.cfg.loglevel_mode, g_vems.cfg.general.loglevel_mode) != 0)
        g_vems.cfg.loglevel_mode = g_vems.cfg.general.loglevel_mode;
    if(config_read_int(CFG_FILE, "vems", "loglevel", &g_vems.cfg.loglevel, g_vems.cfg.general.loglevel) != 0)
        g_vems.cfg.loglevel = g_vems.cfg.general.loglevel;
    if(config_read_str(CFG_FILE, "vems", "loglevel_types", g_vems.cfg.loglevel_types, 64, g_vems.cfg.general.loglevel_types) == NULL)
        strncpy(g_vems.cfg.loglevel_types, g_vems.cfg.general.loglevel_types, sizeof(g_vems.cfg.loglevel_types) - 1);

    config_read_int(CFG_FILE, "vems", "pcs_mb_debug", &ret, 0);    g_vems.cfg.pcs_mb_debug = ret;
    config_read_int(CFG_FILE, "vems", "emeter_mb_debug", &ret, 0); g_vems.cfg.emeter_mb_debug = ret;
    config_read_int(CFG_FILE, "vems", "pcs_pset_min", &ret, 20); g_vems.cfg.pcs_pset_min = ret;
    
    return ret;
}

int read_charge_discharge_timespan_config(char* tmsp_name, ems_tmsp_item_t* ptmsp)
{
    int size = 0; 
    char sval[64] = { 0 };
    char* tmsp_detail[4] = { 0 };

    db_get_string_config(tmsp_name, sval, sizeof(sval), "0;0;0;0");
    LogDebug("read timespan config %s=%s", tmsp_name, sval);
    size = split_string_to_array(sval, ";", (char**)&tmsp_detail, ARRAR_SIZE(tmsp_detail));
    if(size != 4) {
        LogWarn("split pcs device info[%s] failed: format not correct", sval);
        memset(ptmsp, 0, sizeof(ems_tmsp_item_t));
        return 1;
    }
    ptmsp->start_hour = atoi(tmsp_detail[0]);
    ptmsp->end_hour = atoi(tmsp_detail[1]);
    ptmsp->power = atoi(tmsp_detail[2]);
    ptmsp->enable = atoi(tmsp_detail[3]);

    return 0;
}

/*******************************************************************************
 * @brief 
 * @param ptmsp
 * @return 0: invalid; 1 valid
*******************************************************************************/
int vems_check_tmsp_item_valid(ems_tmsp_item_t* ptmsp)
{
    int ret = 1;

    if((ptmsp->start_hour == ptmsp->end_hour) || (ptmsp->enable != 0 && ptmsp->enable != 1)) {
        ret = 0;
    }

    return ret;
}

int vems_read_tmsp(void)
{
    int rc = 0;
    
    rc |= read_charge_discharge_timespan_config("charge_tmsp1", &g_vems.tmsp.charge_tmsp1);
    rc |= read_charge_discharge_timespan_config("discharge_tmsp1", &g_vems.tmsp.discharge_tmsp1);
    rc |= read_charge_discharge_timespan_config("charge_tmsp2", &g_vems.tmsp.charge_tmsp2);
    rc |= read_charge_discharge_timespan_config("discharge_tmsp2", &g_vems.tmsp.discharge_tmsp2);
    rc |= read_charge_discharge_timespan_config("charge_tmsp3", &g_vems.tmsp.charge_tmsp3);
    rc |= read_charge_discharge_timespan_config("discharge_tmsp3", &g_vems.tmsp.discharge_tmsp3);
    rc |= read_charge_discharge_timespan_config("night_es_tmsp", &g_vems.tmsp.night_es);
    if(rc != 0)
        return rc;
    g_vems.tmsp.xftg.enable = 1;
    g_vems.tmsp.xftg.power = g_vems.tmsp.charge_tmsp1.power;
    g_vems.tmsp.xftg.start_hour = MIN_(MIN_(g_vems.tmsp.charge_tmsp1.start_hour, g_vems.tmsp.discharge_tmsp1.start_hour)
                                     , MIN_(g_vems.tmsp.charge_tmsp2.start_hour, g_vems.tmsp.discharge_tmsp2.start_hour));
    g_vems.tmsp.xftg.end_hour = MAX_(MAX_(g_vems.tmsp.charge_tmsp1.end_hour, g_vems.tmsp.discharge_tmsp1.end_hour)
                                     , MAX_(g_vems.tmsp.charge_tmsp2.end_hour, g_vems.tmsp.discharge_tmsp2.end_hour));
    g_vems.tmsp.charge_tmsp1.type = BAT_CHARGE; g_vems.tmsp.charge_tmsp1.no = TMSP_CHARGE1;
    g_vems.tmsp.charge_tmsp2.type = BAT_CHARGE; g_vems.tmsp.charge_tmsp2.no = TMSP_CHARGE2;
    g_vems.tmsp.charge_tmsp3.type = BAT_CHARGE; g_vems.tmsp.charge_tmsp3.no = TMSP_CHARGE3;
    g_vems.tmsp.discharge_tmsp1.type = BAT_DISCHARGE; g_vems.tmsp.charge_tmsp1.no = TMSP_DISCHARGE1;
    g_vems.tmsp.discharge_tmsp2.type = BAT_DISCHARGE; g_vems.tmsp.charge_tmsp2.no = TMSP_DISCHARGE2;
    g_vems.tmsp.discharge_tmsp3.type = BAT_DISCHARGE; g_vems.tmsp.charge_tmsp3.no = TMSP_DISCHARGE3;
    LogMust("   charge_tmsp1: start[%2d],stop[%2d],power[%3d],enable[%d]", g_vems.tmsp.charge_tmsp1.start_hour
            , g_vems.tmsp.charge_tmsp1.end_hour, g_vems.tmsp.charge_tmsp1.power, g_vems.tmsp.charge_tmsp1.enable);
    LogMust("discharge_tmsp1: start[%2d],stop[%2d],power[%3d],enable[%d]", g_vems.tmsp.discharge_tmsp1.start_hour
            , g_vems.tmsp.discharge_tmsp1.end_hour, g_vems.tmsp.discharge_tmsp1.power, g_vems.tmsp.discharge_tmsp1.enable);
    LogMust("   charge_tmsp2: start[%2d],stop[%2d],power[%3d],enable[%d]", g_vems.tmsp.charge_tmsp2.start_hour
            , g_vems.tmsp.charge_tmsp2.end_hour, g_vems.tmsp.charge_tmsp2.power, g_vems.tmsp.charge_tmsp2.enable);
    LogMust("discharge_tmsp2: start[%2d],stop[%2d],power[%3d],enable[%d]", g_vems.tmsp.discharge_tmsp2.start_hour
            , g_vems.tmsp.discharge_tmsp2.end_hour, g_vems.tmsp.discharge_tmsp2.power, g_vems.tmsp.discharge_tmsp2.enable);
    LogMust("           xftg: start[%2d],stop[%2d],power[%3d],enable[%d]", g_vems.tmsp.xftg.start_hour
            , g_vems.tmsp.xftg.end_hour, g_vems.tmsp.xftg.power, g_vems.tmsp.xftg.enable);
    LogMust("       night_es: start[%2d],stop[%2d],power[%3d],enable[%d]", g_vems.tmsp.night_es.start_hour
            , g_vems.tmsp.night_es.end_hour, g_vems.tmsp.night_es.power, g_vems.tmsp.night_es.enable);

    return rc;
}

int vems_read_config()
{
    int rc = 0;
    char sval[64] = { 0 };
    char* sdetail[10] = { 0 };

    LogMust("read vems process configs:");
    init_common_configs(&g_vems.cfg.general);
    read_ini_config();
    // if(get_device_id(g_vems.cfg.device_id) != 0) {
    //     LogError("get device_id=%s failed", g_vems.cfg.device_id);
    //     rc = 1;
    // }
    strcpy(g_vems.cfg.device_id, "110010000");
    db_get_string_config("user", sval, sizeof(sval), "lmt;182407");  //读取触摸屏用户登录信息
    split_string_to_array(sval, ";", (char**)&sdetail, 2);
    strncpy(g_vems.user.username, sdetail[0], sizeof(g_vems.user.username) - 1);
    g_vems.user.password = atoi(sdetail[1]);
    LogMust("%-23s=%d", "wdog_timeout", g_vems.cfg.general.wdog_timeout);
    LogMust("%-23s=%d", "loglevel_mode", g_vems.cfg.loglevel_mode);
    LogMust("%-23s=%d", "loglevel", g_vems.cfg.loglevel);
    LogMust("%-23s=%s", "loglevel_types", g_vems.cfg.loglevel_types);
    LogMust("%-23s=%s", "device_id", g_vems.cfg.device_id);
    LogMust("%-23s=%s", "hmi_user", g_vems.user.username);
    LogMust("%-23s=%d", "hmi_pwd", g_vems.user.password);

    rc = vems_read_tmsp();
    if(rc != 0)
        return rc;

    db_get_string_config("concfg", sval, sizeof(sval), "255.255.255.255;8888;88;19200;19200;250");  //读取通信参数配置
    split_string_to_array(sval, ";", (char**)&sdetail, 6);
    strncpy(g_vems.concfg.cloud_ip, sdetail[0], sizeof(g_vems.concfg.cloud_ip));
    g_vems.concfg.cloud_port = atoi(sdetail[1]);
    g_vems.concfg.modbus_id = atoi(sdetail[2]);
    g_vems.concfg.modbus_baud = atoi(sdetail[3]);
    g_vems.concfg.hmi_baud = atoi(sdetail[4]);
    g_vems.concfg.can_baud = atoi(sdetail[5]);
    LogMust("%-23s=%s", "conncfg.cloud_ip", g_vems.concfg.cloud_ip);
    LogMust("%-23s=%d", "conncfg.cloud_port", g_vems.concfg.cloud_port);
    LogMust("%-23s=%d", "conncfg.modbus_id", g_vems.concfg.modbus_id);
    LogMust("%-23s=%d", "conncfg.modbus_baud", g_vems.concfg.modbus_baud);
    LogMust("%-23s=%d", "conncfg.hmi_baud", g_vems.concfg.hmi_baud);
    LogMust("%-23s=%d", "conncfg.can_baud", g_vems.concfg.can_baud);

    /*读取系统参数配置*/
    db_get_string_config("syscfg", sval, sizeof(sval), "0;10;99;1;88;99;99;999");  //读取通信参数配置
    split_string_to_array(sval, ";", (char**)&sdetail, 8);
    g_vems.syscfg.mode = atoi(sdetail[0]);
    g_vems.syscfg.discharge_min_soc = atoi(sdetail[1]);
    g_vems.syscfg.charge_max_soc = atoi(sdetail[2]);
    g_vems.syscfg.discharge_min_soc = atoi(sdetail[3]);
    g_vems.syscfg.debug_mode_power = atoi(sdetail[4]);
    g_vems.syscfg.on_grid_power = atoi(sdetail[5]);
    g_vems.syscfg.bms_power = atoi(sdetail[6]);
    g_vems.syscfg.load_power = atoi(sdetail[7]);
    LogMust("%-23s=%d", "syscfg.mode", g_vems.syscfg.mode);
    LogMust("%-23s=%d", "syscfg.discharge_min_soc", g_vems.syscfg.discharge_min_soc);
    LogMust("%-23s=%d", "syscfg.charge_max_soc", g_vems.syscfg.charge_max_soc);
    LogMust("%-23s=%d", "syscfg.fnl_en", g_vems.syscfg.fnl_en);
    LogMust("%-23s=%d", "syscfg.debug_mode_power", g_vems.syscfg.debug_mode_power);
    LogMust("%-23s=%d", "syscfg.on_grid_power", g_vems.syscfg.on_grid_power);
    LogMust("%-23s=%d", "syscfg.bms_power", g_vems.syscfg.bms_power);
    LogMust("%-23s=%d", "syscfg.load_power", g_vems.syscfg.load_power);

    db_get_string_config("emeter_ipaddr", g_vems.cfg.emeter_ipaddr, sizeof(g_vems.cfg.emeter_ipaddr), "192.168.225.7");
    db_get_number_config("emeter_port", &rc, 8234); g_vems.cfg.emeter_port = rc; rc = 0;
    db_get_number_config("emeter_slave_id", &rc, 1);  //电表默认slaveID
    g_vems.cfg.emeter_slave_id = rc; rc = 0;
    db_get_string_config("station_power_capacity", sval, sizeof(sval), "500");  //默认厂站容量为500kW
    g_vems.cfg.station_power_capacity_real = strtof(sval, NULL);
    g_vems.cfg.station_power_capacity = (int)g_vems.cfg.station_power_capacity_real * COMM_DATA_MULTIPLE;
    db_get_string_config("station_power_reserve", sval, sizeof(sval), "10");    //默认厂站保留容量为10kW
    g_vems.cfg.station_power_reserve_real = strtof(sval, NULL);
    g_vems.cfg.station_power_reserve = (int)g_vems.cfg.station_power_reserve_real * COMM_DATA_MULTIPLE;
    db_get_string_config("power_update_step", sval, sizeof(sval), "1");         //默认最小调整步长为1kW
    g_vems.cfg.power_update_step_real = strtof(sval, NULL);
    g_vems.cfg.power_update_step = (int)g_vems.cfg.power_update_step_real * COMM_DATA_MULTIPLE;
    g_vems.cfg.station_power_available = g_vems.cfg.station_power_capacity - g_vems.cfg.station_power_reserve;
    g_vems.cfg.station_power_available_real = g_vems.cfg.station_power_available * COMM_DATA_RATIO;
    LogMust("%-23s=%s", "emeter_ipaddr", g_vems.cfg.emeter_ipaddr);
    LogMust("%-23s=%d", "emeter_port", g_vems.cfg.emeter_port);
    LogMust("%-23s=%d", "emeter_slave_id", g_vems.cfg.emeter_slave_id);
    LogMust("%-23s=%d, %.1f", "station_power_capacity", g_vems.cfg.station_power_capacity, g_vems.cfg.station_power_capacity_real);
    LogMust("%-23s=%d, %.1f", "station_power_reserve", g_vems.cfg.station_power_reserve, g_vems.cfg.station_power_reserve_real);
    LogMust("%-23s=%d, %.1f", "station_power_available", g_vems.cfg.station_power_available, g_vems.cfg.station_power_available_real);
    LogMust("%-23s=%d, %.1f", "power_update_step", g_vems.cfg.power_update_step, g_vems.cfg.power_update_step_real);
    
    /*读取版本号信息*/
    db_get_config_data("ems_version", g_vems.version.ems_version, 24, EMS_DEFAULT_VERSION);
    db_get_config_data("control_version", g_vems.version.control_version, 24, CON_DEFAULT_VERSION);
    db_get_config_data("hmi_version", g_vems.version.hmi_version, 24, HMI_DEFAULT_VERSION);
    LogMust("ems_version:%s", g_vems.version.ems_version);
    LogMust("control_version:%s", g_vems.version.control_version);
    LogMust("hmi_version:%s", g_vems.version.hmi_version);

    return rc;
}
/*******************************************************************************
 * @brief 读取数据库中保存的PCS设备信息并初始化相关参数
 * @return 0:success; 1: failed
*******************************************************************************/
int vems_read_pcs_config()
{
    int rc = 0;
    char sql[] = "select * from dev_pcs";
    int size = 0;
	int maxrow = EMS_PCS_MAX;
	char *pcs_rows[EMS_PCS_MAX]={0};
	char *pdata_row = NULL;
    char* pcs_detail[PCS_ITEM_NUM] = { 0 };
    dev_pcs_params_t* ppcs_param = NULL;
    int i = 0, j = 0;

    rc = db_do_query(sql, (char**)&pcs_rows, &maxrow, 0);
    if(rc != 0) {
        LOG(LOG_LV_ERROR, "select all pcs device info error return:%d", rc);
        return 1;
    }
    if(maxrow == 0) {
        LOG(LOG_LV_ERROR, "query not found pcs device info");
    }

    g_vems.pcs_cnt = maxrow;
    LogInfo("read pcs device config, pcs_cnt=%d", g_vems.pcs_cnt);
    for(i = 0; i < maxrow; i++) {
        pdata_row = pcs_rows[i];
        ppcs_param = &g_vems.pcs[i].dbcfg;
        size = split_string_to_array(pdata_row, DB_FIELD_SPLIT, (char**)&pcs_detail, PCS_ITEM_NUM);
        if(size == 0) {
            db_free(pcs_rows[i]); pcs_rows[i] = NULL;
            LogWarn("split pcs device info[%s] failed\n", pdata_row);
            continue;
        }
        ppcs_param->pcs_id = atoi(pcs_detail[0]);
        strncpy(ppcs_param->pcs_name, pcs_detail[1], sizeof(ppcs_param->pcs_name) - 1);
        ppcs_param->modbus_id = atoi(pcs_detail[2]);
        ppcs_param->baud_rate = atoi(pcs_detail[3]);
        ppcs_param->rated_power = atoi(pcs_detail[4]);
        ppcs_param->max_voltage = atoi(pcs_detail[5]);
        ppcs_param->max_current = atoi(pcs_detail[6]);
        ppcs_param->cont_comm_id = atoi(pcs_detail[7]);
        ppcs_param->cont_can_addr = atoi(pcs_detail[8]);
        ppcs_param->cont_can_baudrate = atoi(pcs_detail[9]);
        ppcs_param->state = atoi(pcs_detail[10]);
        g_vems.pcs[i].power_reted = ppcs_param->rated_power * COMM_DATA_MULTIPLE;
        for(j = 0; j < PCS_BAT_NUM; j++) {
            g_vems.pcs[i].bat_rack[j].chg_state = BAT_NO_CHARGE;
            g_vems.pcs[i].bat_rack[j].act_state = BMS_NOT_ACTIVE;
            g_vems.pcs[i].bat_rack[j].rtdata.bat_chg_dischg_done = 0;
        }
        g_vems.pcs[i].cont.pcs_idx = i;
        g_vems.pcs[i].chg_bat_idx = -1;
        g_vems.pcs[i].pcs_start_step = PCS_START_STEP_DONE;
        g_vems.pcs[i].pcs_stop_step = PCS_STOP_STEP_DONE;
        g_vems.pcs[i].pcs_search_bat_step = PCS_SRCH_BAT_STEP_DONE;
        g_vems.pcs[i].state = PCS_INITING;
        g_vems.pcs[i].tobms_state = PCS_BMS_INIT;
        g_vems.pcs[i].use_state = ITEM_USED;
        db_free(pcs_rows[i]); pcs_rows[i] = NULL;
    }
    if(g_vems.pcs_cnt > 0) {
        g_vems.pcs_power_rated = g_vems.pcs[0].power_reted;
    }

    return 0;
}

/*******************************************************************************
 * @brief 重新读取数据库中保存的PCS设备信息
 * @return 0:success; 1: failed
*******************************************************************************/
int vems_reload_pcs_config()
{
    int rc = 0;
    char sql[] = "select * from dev_pcs";
    int size = 0;
    int maxrow = EMS_PCS_MAX;
    char* pcs_rows[EMS_PCS_MAX] = { 0 };
    char* pdata_row = NULL;
    char* pcs_detail[PCS_ITEM_NUM] = { 0 };
    dev_pcs_params_t* ppcs_param = NULL;
    int i = 0;

    rc = db_do_query(sql, (char**)&pcs_rows, &maxrow, 0);
    if(rc != 0) {
        LOG(LOG_LV_ERROR, "select all pcs device info error return:%d", rc);
        return 1;
    }
    if(maxrow == 0) {
        LOG(LOG_LV_ERROR, "query not found pcs device info");
    }

    if(g_vems.pcs_cnt > 0 && g_vems.pcs_cnt != maxrow) {
        LogWarn("reload pcs db config failed: pcs_cnt[%d, %d] is changed, please restart the program", g_vems.pcs_cnt, maxrow);
        return 1;
    }
    g_vems.pcs_cnt = maxrow;
    LogInfo("reload pcs device config, pcs_cnt=%d", g_vems.pcs_cnt);
    LogInfo("%-4s %-5s %-5s %-6s %-6s %-6s %-6s %-8s %-10s %-10s", "id", "name", "mbid", "mbbaud", "ratedP", "maxV", "maxC"
          , "contID", "contCID", "contCbaud");
    for(i = 0; i < maxrow; i++) {
        pdata_row = pcs_rows[i];
        ppcs_param = &g_vems.pcs[i].dbcfg;
        size = split_string_to_array(pdata_row, DB_FIELD_SPLIT, (char**)&pcs_detail, PCS_ITEM_NUM);
        if(size == 0) {
            db_free(pcs_rows[i]); pcs_rows[i] = NULL;
            LogWarn("split pcs device info[%s] failed\n", pdata_row);
            continue;
        }
        ppcs_param->pcs_id = atoi(pcs_detail[0]);
        strncpy(ppcs_param->pcs_name, pcs_detail[1], sizeof(ppcs_param->pcs_name) - 1);
        ppcs_param->modbus_id = atoi(pcs_detail[2]);
        ppcs_param->baud_rate = atoi(pcs_detail[3]);
        ppcs_param->rated_power = atoi(pcs_detail[4]);
        ppcs_param->max_voltage = atoi(pcs_detail[5]);
        ppcs_param->max_current = atoi(pcs_detail[6]);
        ppcs_param->cont_comm_id = atoi(pcs_detail[7]);
        ppcs_param->cont_can_addr = atoi(pcs_detail[8]);
        ppcs_param->cont_can_baudrate = atoi(pcs_detail[9]);
        ppcs_param->state = atoi(pcs_detail[10]);
        LogInfo("%-4d %-5s %-5d %-6d %-6d %-6d %-6d %-8d %-10d %-10d", ppcs_param->pcs_id, ppcs_param->pcs_name, ppcs_param->modbus_id
               , ppcs_param->baud_rate, ppcs_param->rated_power, ppcs_param->max_voltage, ppcs_param->max_current, ppcs_param->cont_comm_id
               , ppcs_param->cont_can_addr, ppcs_param->cont_can_baudrate);
        g_vems.pcs[i].power_reted = ppcs_param->rated_power * COMM_DATA_MULTIPLE;
        
        db_free(pcs_rows[i]); pcs_rows[i] = NULL;
    }
    if(g_vems.pcs_cnt > 0) {
        g_vems.pcs_power_rated = g_vems.pcs[0].power_reted;
    }
    LogInfo("reload pcs config done");

    return 0;
}

void vems_reload_config(void)
{
    int ret = 0;

    read_ini_config();

    set_log_level_mode(&log_vems, g_vems.cfg.loglevel_mode);
    set_log_level(&log_vems, g_vems.cfg.loglevel);
    set_log_level_sets(&log_vems, g_vems.cfg.loglevel_types);

    modbus_set_debug(g_pcs_comm.ctx, g_vems.cfg.pcs_mb_debug);
    modbus_set_debug(g_vems.elec_meter.ctx, g_vems.cfg.emeter_mb_debug);

    ret = vems_read_tmsp();
    if(ret != 0) {
        LogError("reload charge-discharge timespan failed");
    }
}

int vems_ipc_get_version(char* buff, int maxlen)
{
    int offset = 0;

    offset += snprintf(buff + offset, maxlen - offset, "EMS: %s\n", g_vems.version.ems_version);
    offset += snprintf(buff + offset, maxlen - offset, "SWH: %s\n", g_vems.version.control_version);
    offset += snprintf(buff + offset, maxlen - offset, "HMI: %s", g_vems.version.hmi_version);
    offset = offset > maxlen ? maxlen : offset;
    return offset;
}

int vems_ipc_get_stat_info(char* buff, int maxlen)
{
    int i = 0, bat_idx = 0;
    int offset = 0;
    pcs_dev_t* ppcs = NULL;
    pcs_real_data_t* prtd = NULL;
    state_bit0_7_t* pbits = NULL;
    cont_data_t* pcont_data = NULL;
    bms_data_t* pbdata = NULL;

    //PCS realtime data -- important state data and charge-discharge data
    offset += snprintf(buff + offset, maxlen - offset, "%-9s %-5s %-5s %-5s %-5s %-5s %-5s %-6s %-6s %-6s %-6s %-6s %-6s %-6s %-6s %-6s\n"
                    , "name", "Sen", "Flt", "Run", "Stp", "Rst", "Plck", "Gstat", "Serr1", "Serr2", "Sys", "Qset", "Pset", "Pa", "Pb", "Pc");
    offset += snprintf(buff + offset, maxlen - offset, "%-9s %-5s %-5s %-5s %-5s %-5s %-5s %-6s %-6s %-6s %-6s %-6s %-6s %-6s %-6s %-6s %-6s %-6s %-6s\n"
                    , "", "Fg", "Ug", "Uga", "Ugb", "Ugc", "Udc", "Idc2", "Idc", "Ia", "Ib", "Ic", "T", "Qa", "Qb", "Qc", "Sa", "Sb", "Sc");
    for(i = 0; i < g_vems.pcs_cnt; i++) {
        pbits = &g_vems.pcs[i].rtdata.coil_state.bits;
        prtd = &g_vems.pcs[i].rtdata;

        offset += snprintf(buff + offset, maxlen - offset, "pcs%-6d %-5d %-5d %-5d %-5d %-5d %-5d %-6d %04X   %04X   %04X   %-6.1f %-6.1f %-6.1f %-6.1f %-6.1f\n"
                    , i, pbits->start_enable, pbits->fault, pbits->run, pbits->stop, pbits->reset, pbits->phase_lock, prtd->grid_state, prtd->sys_err1
                    , prtd->sys_err2, prtd->sys_state, prtd->reactive_power, prtd->active_power, prtd->pcs_pa, prtd->pcs_pb, prtd->pcs_pc);
        offset += snprintf(buff + offset, maxlen - offset, "%-9s %-5.1f %-5.1f %-5.1f %-5.1f %-5.1f %-5.1f %-6.1f %-6.1f %-6.1f %-6.1f %-6.1f %-6.1f "
                    "%-6.1f %-6.1f %-6.1f %-6.1f %-6.1f %-6.1f\n", "", prtd->gird_freq, prtd->grid_voltage, prtd->grid_ua, prtd->grid_ub, prtd->grid_uc
                    , prtd->dcbus_voltage, prtd->dc_current_out, prtd->dc_current, prtd->pcs_ia, prtd->pcs_ib, prtd->pcs_ic, prtd->radiator_temp
                    , prtd->pcs_qa, prtd->pcs_qb, prtd->pcs_qc, prtd->pcs_sa, prtd->pcs_sb, prtd->pcs_sc);
    }
    //PCS realtime data -- cont run info
    // offset += snprintf(buff + offset, maxlen - offset, "%-9s %-5s %-5s %-5s %-5s %-5s %-5s %-6s %-6s %-6s %-6s %-6s %-6s %-6s %-6s %-6s\n"
    //                 , "name", "State", "Flt", "Run", "Stp", "Rst", "Plck", "Gstat", "Serr1", "Serr2", "Sys", "Qset", "Pset", "Pa", "Pb", "Pc");
    for(i = 0; i < g_vems.pcs_cnt; i++) {
        pcont_data = &g_vems.pcs[i].cont.rtdata;
        offset += snprintf(buff + offset, maxlen - offset, "pcs%d.cont %-5s %-5d %-5s %02X    %-5s %02X    %-6s %02X     %-6s %02X     %-6s %02X\n"
            , i, "state", pcont_data->cont_state, "gun", pcont_data->gun_stat.byte, "can", pcont_data->can_stat.byte, "Rset", pcont_data->relay_stat.byte
            , "Rfb", pcont_data->relay_stat_fb.byte, "dio", pcont_data->dio_stat.byte);
    }

    //PCS's BMS data
    offset += snprintf(buff + offset, maxlen - offset, "%-10s %-5s %-5s %-5s %-5s %-5s %-5s %-6s %-6s %-6s %-6s %-6s %-6s %-6s %-6s %-6s %-6s %-6s %-6s %-6s %-6s %-6s %-6s\n"
                    , "\nname", "Vcmax", "Vcmin", "Soc", "Soh", "KM", "Vtot", "Ctot", "Ccmax", "Cdcmax", "Bstat", "Sys", "Alm11", "Alm12", "Alm21", "Alm22", "Alm31"
                    , "Alm32", "Cnt01", "Cnt02", "Cnt06", "Cnt07", "CntE1");
    for(i = 0; i < g_vems.pcs_cnt; i++) {
        ppcs = &g_vems.pcs[i];
        bat_idx = ppcs->cont.rtdata.can_stat.stat.conn_id;
        if(bat_idx <= 0 || bat_idx > BAT_RACK_4) {
            LogDebug("format pcs[%d] bms info failed: conn_id=%d is invalid", i, bat_idx);
            continue;
        } else {
            bat_idx -= 1;
        }
        pbdata = &ppcs->bat_rack[bat_idx].rtdata;

        offset += snprintf(buff + offset, maxlen - offset, "pcs%d.bms%d %-5.3f %-5.3f %-5d %-5d %-5d %-5.1f %-6.1f %-6.1f %-6.1f %-6d %02X     %02X     %02X     %02X     "
                    "%02X     %02X     %02X     %-6u %-6u %-6u %-6u %-6u\n"
                    , i, bat_idx, pbdata->cell_voltage_max, pbdata->cell_voltage_min, pbdata->soc, pbdata->soh, pbdata->km_state, pbdata->voltage_total, pbdata->current_total, pbdata->charge_max_curr
                    , pbdata->discharge_max_curr, pbdata->bat_state, pbdata->sys_state, pbdata->alarm11.byte, pbdata->alarm12.byte, pbdata->alarm21.byte, pbdata->alarm22.byte, pbdata->alarm31.byte
                    , pbdata->alarm32.byte, ppcs->cont.bms_msg01_rx_cnt, ppcs->cont.bms_msg02_rx_cnt, ppcs->cont.bms_msg06_rx_cnt, ppcs->cont.bms_msg07_rx_cnt, ppcs->cont.bms_msgE1_rx_cnt);
    }

    return offset;
}

int vems_ipc_get_pcs_info(char* buff, int maxlen)
{
    int i = 0;
    int offset = 0;

    //hb=heartbeat, tbs=to_bms_state, bpower=battery_power
    offset += snprintf(buff + offset, maxlen - offset, "%-3s %-5s %-5s %-3s %-3s %-3s %-3s %-6s\n", "id", "name", "state", "ss", "ssd", "hb", "tbs", "bpower");
    for(i = 0; i < g_vems.pcs_cnt; i++) {
        offset += snprintf(buff + offset, maxlen - offset, "%-3d %-5s %-5d %-3d %-3d %-3d %-3d %-6.1f\n", g_vems.pcs[i].dbcfg.pcs_id
        , g_vems.pcs[i].dbcfg.pcs_name, g_vems.pcs[i].state, g_vems.pcs[i].hmi_start_stop, g_vems.pcs[i].hmi_start_stop_done
        , g_vems.pcs_heart, g_vems.pcs[i].tobms_state, g_vems.pcs[i].rtdata.active_power);
    }
    offset += snprintf(buff + offset, maxlen - offset, "\nstate=2:INITED; 4:CHARGE_BAT; 5:DISCHARGE_BAT; 7:STOPPED; 14:FAULT;"
              " 16:HMI_STOPPING; 17:HMI_STOPPED");

    return offset;
}

/*******************************************************************************
 * @brief 向bms发送数据
 * @return
*******************************************************************************/
int vems_send_pcs_state_msg(void)
{
    int ret = 0, i = 0;
    //int bat_idx = 0;
    int16_t bat_power = 0;
    uint32_t can_txid = 0;
    ems_ext_id_t* pems_eid = (ems_ext_id_t*)&can_txid;
    uint8_t sbuff[8] = { 0 };

    for(i = 0; i < g_vems.pcs_cnt; i++) {
        //bat_idx = g_vems.pcs[i].cont.rtdata.can_stat.stat.conn_id - 1;
        //if((bat_idx < 0) || g_vems.pcs[i].cont.rtdata.rack_id <= 0) {
        //    LogBMS("in vems_send_pcs_state_msg() pcs%d's bat_idx=%d, cont.rack_id=%d, no need notify pcs run info", i, bat_idx, g_vems.pcs[i].cont.rtdata.rack_id);
            //continue;
        //}
        g_vems.pcs_heart++;
        if(g_vems.pcs_heart == 0) {
            g_vems.pcs_heart++;
        }
        sbuff[0] = g_vems.pcs_heart;
        sbuff[1] = g_vems.pcs[i].tobms_state;
        bat_power = (int)g_vems.pcs[i].rtdata.active_power;
        sbuff[2] = HI8_UINT16(bat_power);
        sbuff[3] = LO8_UINT16(bat_power);
        pems_eid->reserve = 1;
        pems_eid->cmd = CAN_CMD_BMS_INFO1;
        pems_eid->cont_id = i + 1;
        pems_eid->rack_id = 0xf1;//g_vems.pcs[i].cont.rtdata.rack_id;
        //LogBMS("send pcs[%d] run info to rack[%d] bms, data[0x%08X, %02X %02X %02X %02X]", i, bat_idx, can_txid, sbuff[0], sbuff[1], sbuff[2], sbuff[3]);
        ret = bms_and_cont_write_data(g_vems.can_sock, can_txid, sbuff);
    }
    
    return ret;
}

void* vems_common_thread_do(void* args)
{
    g_vems.th_stat.bits.th_common = 1;
    LogInfo("common thread start running");
    while(1) {
        //下发PCS状态给BMS(can frame), interval=500ms
        vems_send_pcs_state_msg();
        safe_msleep(500);
    }

    return args;
}

int create_common_thread(void)
{
    int ret = -1;
    pthread_t comm_tid;
    pthread_attr_t comm_tattr;

    pthread_attr_init(&comm_tattr);
    pthread_attr_setdetachstate(&comm_tattr, PTHREAD_CREATE_DETACHED);
    ret = pthread_create(&comm_tid, &comm_tattr, vems_common_thread_do, NULL);
    if(ret == 0) {
        LogInfo("create common thread success");
    } else {
        LogError("create common thread failed: %s", strerror(errno));
    }
    return 0;
}

void* vems_ipc_thread_do(void* arg)
{
    //int ret = 0;
    int evmsglen = 0;
    event_msg_t evsnd = { 0 };
    event_msg_t evrcv = { 0 };
    //int size = 0;

    g_vems.th_stat.bits.th_ipc_comm = 1;
    LogInfo("ipc thread start running");
    while(1) {
        memset(&evrcv, 0, sizeof(evrcv));
        if(atool_msq_recv_msg(g_ipc_msgid, (char*)&evrcv, sizeof(event_msg_t), MSG_TYPE_REQ, 1) > 0) {
            switch(evrcv.event) {
            case MSG_EVENT_LOG_DEBUG:
                set_log_level(g_plog, LOG_LV_DEBUG);
                LogMust("set log level to DEBUG success");
                break;
            case MSG_EVENT_LOG_INFO:
                set_log_level(g_plog, LOG_LV_INFO);
                LogMust("set log level to INFO success");
                break;
            case MSG_EVENT_LOG_WARN:
                set_log_level(g_plog, LOG_LV_WARN);
                LogMust("set log level to WARN success");
                break;
            case MSG_EVENT_LOG_ERROR:
                set_log_level(g_plog, LOG_LV_ERROR);
                LogMust("set log level to ERROR success");
                break;
            case MSG_EVENT_LOG_NONE:
                set_log_level(g_plog, LOG_NONE);
                LogMust("set log level to NONE success");
                break;
            case MSG_EVENT_LOG_ALL:
                set_log_level(g_plog, LOG_LV_ALL);
                LogMust("set log level to ALL[%d] success", get_log_level(g_plog));
                break;
            case MSG_EVENT_LOGLV_SETS:
                set_log_level_sets(g_plog, (char*)evrcv.msgdata);
                LogMust("set log level sets[%s] success", (char*)evrcv.msgdata);
                break;
            case MSG_EVENT_RECONF:
                LogMust("========== reload global config ==========");
                vems_reload_config();
                break;
            case MSG_EVENT_PCS_RECONF:
                LogMust("========== reload pcs db config ==========");
                vems_reload_pcs_config();
                break;
            case MSG_EVENT_PCS_INFO:
                memset(&evsnd, 0, sizeof(evsnd));
                evsnd.event = MSG_EVENT_PCS_INFO;
                evsnd.datalen = vems_ipc_get_pcs_info((char*)evsnd.msgdata, sizeof(evsnd.msgdata));
                evmsglen = OFFSET_OF(event_msg_t, msgdata) + evsnd.datalen;
                atool_msq_send_msg(g_ipc_msgid, (char*)&evsnd, evmsglen, MSG_TYPE_RSP);
                break;
            case MSG_EVENT_STAT_INFO:
                memset(&evsnd, 0, sizeof(evsnd));
                evsnd.event = MSG_EVENT_STAT_INFO;
                evsnd.datalen = vems_ipc_get_stat_info((char*)evsnd.msgdata, sizeof(evsnd.msgdata));
                evmsglen = OFFSET_OF(event_msg_t, msgdata) + evsnd.datalen;
                atool_msq_send_msg(g_ipc_msgid, (char*)&evsnd, evmsglen, MSG_TYPE_RSP);
                break;
            case MSG_EVENT_VERSION:
                memset(&evsnd, 0, sizeof(evsnd));
                evsnd.event = MSG_EVENT_VERSION;
                evsnd.datalen = vems_ipc_get_version((char*)evsnd.msgdata, sizeof(evsnd.msgdata));
                evmsglen = OFFSET_OF(event_msg_t, msgdata) + evsnd.datalen;
                atool_msq_send_msg(g_ipc_msgid, (char*)&evsnd, evmsglen, MSG_TYPE_RSP);
                LogInfo("current version is[%s]", evsnd.msgdata);
                break;
            default:
                evsnd.event = MSG_EVENT_UNKNOWN;
                evsnd.datalen = sprintf((char*)evsnd.msgdata, "unknown event[%d]", evrcv.event);
                evmsglen = OFFSET_OF(event_msg_t, msgdata) + evsnd.datalen;
                atool_msq_send_msg(g_ipc_msgid, (char*)&evsnd, evmsglen, MSG_TYPE_RSP);
                LogWarn("unknown event[%d]", evrcv.event);
            }
        } else {
            LogError("atool_msq_recv_msg() failed: %s", strerror(errno));
        }
    }

    return arg;
}

int create_ipc_thread(void)
{
    int ret = -1;
    pthread_t ipc_tid;
    pthread_attr_t ipc_tattr;

    pthread_attr_init(&ipc_tattr);
    pthread_attr_setdetachstate(&ipc_tattr, PTHREAD_CREATE_DETACHED);
    ret = pthread_create(&ipc_tid, &ipc_tattr, vems_ipc_thread_do, NULL);
    if(ret == 0) {
        LogInfo("create ipc thread success");
    } else {
        LogError("create ipc thread failed: %s", strerror(errno));
    }
    return 0;
}

/*********************************************************************
* @fn	      main
* @brief      采集传感器进程主函数
* @param      argc[in]: 参数个数
* @param      argv[in]: 参数列表
* @return     0: success -1: fail
* @history:
**********************************************************************/
int main(int argc, char* argv[])
{
    int ret = -1;

    ret = resource_init();
    if(ret) {
        printf("resource init failed, clean resource\n");
        ret = -1;
        goto proc_exit;
    }
    LogMust("vems resource init success");

    memset(&g_vems, 0, sizeof(g_vems));
    ret = vems_read_config();
    if(ret) {
        DBG_LOG_ERROR("read vems config failed");
        ret = -1;
        goto proc_exit;
    }

    set_log_level_mode(&log_vems, g_vems.cfg.loglevel_mode);
    set_log_level(&log_vems, g_vems.cfg.loglevel);
    set_log_level_sets(&log_vems, g_vems.cfg.loglevel_types);

    //读取PCS设备表，获取PCS信息及数量
    ret = vems_read_pcs_config();
    if(ret != 0) {
        DBG_LOG_ERROR("read pcs device info failed");
        ret = -1;
        goto proc_exit;
    }

    ret = create_wdog_feed_thread(g_vems.cfg.general.wdog_timeout);
    if(ret) {
        DBG_LOG_ERROR("create watchdog feed thread fail!\n");
        ret = -1;
        goto proc_exit;
    } else {
        LogInfo("create watchdog feed thread ok!\n");
    }

    LogDebug("vems_t size=%d", sizeof(vems_t));
    LogMust("start create all bussiness thread");
    g_vems.can_sock = -1;
    create_pcs_comm_thread();
    create_cont_comm_thread();   //包含BMS数据读取及PCS状态下发给BMS
    //create_emeter_comm_thread(&g_vems.elec_meter);
    create_dlt645_comm_thread(&g_vems.elec_meter);
    create_energy_mana_thread();
    create_hmi_comm_thread();
    create_common_thread();
    create_ipc_thread();
    LogMust("create all bussiness thread done");

    //todo 自检

    while(1) {
        //主线程中处理多EMS间通信
        safe_msleep(500);
    }

proc_exit:
    DBG_LOG_MUST("============================== vems process exit ==============================");
    resource_clean();

    return ret;
}
