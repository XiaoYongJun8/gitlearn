/*******************************************************************************
 * @             Copyright (c) 2023 LinyPower, All Rights Reserved.
 * @*****************************************************************************
 * @FilePath     : hmi_db_util.c
 * @Descripttion : 
 * @Author       : xiaoyongjun
 * @E-Mail       : xiaoyongjun@linygroup.cn
 * @Version      : v1.0.0
 * @Date         : 2023-10-24 13:08:11
*******************************************************************************/
#include "hmi_db_util.h"
#include "debug_and_log.h"
#include "util.h"
#include "sqlite3.h"

/*******************************************************************************
 * @brief 往数据库插入新的数据
 * @param ppcs  pcs数据
 * @param try_update 数据库更新标志
 * @return 插入数据成功返回：0 插入数据失败返回 -1
*******************************************************************************/
int dev_pcs_insert(dev_pcs_params_t* ppcs, int try_update)
{
    int ret = 0;
    int nchanges = 0;
    char sql[256] = { 0 };

    snprintf(sql, sizeof(sql), "insert into dev_pcs values(%d, '%s', %d, %d, %d, %d, %d, %d, %d, %d,%d)",\
    ppcs->pcs_id, ppcs->pcs_name,ppcs->modbus_id,ppcs->baud_rate,ppcs->rated_power,ppcs->max_voltage,ppcs->max_current,\
    ppcs->cont_comm_id,ppcs->cont_can_addr,ppcs->cont_can_baudrate,ppcs->state);

    ret = db_do_update(sql, &nchanges);

    if(ret != SQLITE_OK) {
        if(try_update && ret == SQLITE_CONSTRAINT) {
            LogEMSHMI("insert dev_pcs[%d] failed, try do update dev_pcs", ppcs->pcs_id);  
        } else {
            LogEMSHMI("insert dev_pcs[%d] with sql[%s] error return:%d\n", ppcs->pcs_id, sql, ret);
            ret = -1;
        }
    } else { 
       LogEMSHMI("insert dev_pcs[%d] success, nchanges=%d\n", ppcs->pcs_id, nchanges);
    }

    return ret;
}
/*******************************************************************************
 * @brief 往数据库更新历史数据
 * @param ppcs
 * @param only_pcs_name
 * @param only_state
 * @return 更新数据成功返回：0 更新数据失败返回 -1
*******************************************************************************/
int dev_pcs_update(dev_pcs_params_t* ppcs, int only_pcs_name, int only_state)
{
    int ret = 0;
    int nchanges = 0;
    char sql[256] = { 0 };

    if(only_pcs_name && only_state)
        snprintf(sql, sizeof(sql), "update dev_pcs set pcs_name='%s', state=%d, where pcs_id=%d", ppcs->pcs_name, ppcs->state, ppcs->pcs_id);
    else if(only_pcs_name)
        snprintf(sql, sizeof(sql), "update dev_pcs set pcs_name='%s', where pcs_id=%d", ppcs->pcs_name,  ppcs->pcs_id);
    else if(only_state)
        snprintf(sql, sizeof(sql), "update dev_pcs set state=%d, where pcs_id=%d", ppcs->state, ppcs->pcs_id);
    else
    snprintf(sql, sizeof(sql), "update dev_pcs set pcs_name='%s',modbus_id=%d,baudrate=%d,rated_power=%d,max_voltage=%d,max_current=%d,\
    cont_comm_id=%d,cont_can_addr=%d,cont_can_baudrate=%d,state=%d where pcs_id=%d",\
    ppcs->pcs_name,ppcs->modbus_id,ppcs->baud_rate,ppcs->rated_power,ppcs->max_voltage,ppcs->max_current,\
    ppcs->cont_comm_id, ppcs->cont_can_addr, ppcs->cont_can_baudrate, ppcs->state, ppcs->pcs_id);

    ret = db_do_update(sql, &nchanges);
    if(ret != SQLITE_OK) {
        LogEMSHMI("update dev_pcs[%d] with sql[%s] error return:%d\n", ppcs->pcs_id, sql, ret);
        ret = -1;
    } else {
        LogEMSHMI("update dev_pcs[%d] success, nchanges=%d",ppcs->pcs_id, nchanges);
    }
    return ret;
}
/*******************************************************************************
 * @brief 删除数据库里面的数据
 * @param pcs_id 指定删除数据的id
 * @param delete_all 1：全部删除 0：按照pcs_id删除
 * @return 删除数据成功返回：0 删除数据失败返回 -1
*******************************************************************************/
int dev_pcs_delete(int pcs_id, int delete_all)
{
    int ret = 0;
    int nchanges = 0;
    char sql[256] = {0};

    if(!delete_all)
        snprintf(sql, sizeof(sql), "delete from dev_pcs where pcs_id=%d", pcs_id);
    else
        snprintf(sql, sizeof(sql), "delete from dev_pcs");
    LogEMSHMI("delete dev_pcs[%d] sql=%s", pcs_id, sql);
    ret = db_do_update(sql, &nchanges);

    if(ret != SQLITE_OK) {
        LogEMSHMI("delete dev_pcs[%d] with sql[%s] error return:%d\n", pcs_id, sql, ret);
        ret = -1;
    } else {
        LogEMSHMI("delete dev_pcs[%d] success, nchanges=%d", pcs_id, nchanges);
    }
    return ret;
}
/*******************************************************************************
 * @brief 向数据库写入配置项充放电时间段数据
 * @param tmsp_name 配置项名称
 * @param ptmsp 配置项数据
 * @return 更新数据成功返回：0 更新数据失败返回 -其他
*******************************************************************************/
int charge_time_update(char* tmsp_name, ems_tmsp_item_t* ptmsp)
{
    int ret = 0;
    char str[64];
    sprintf(str, "%d;%d;%d;%d", ptmsp->start_hour, ptmsp->end_hour, ptmsp->power, ptmsp->enable);
    
    ret = db_set_config_data("BasicConfig", tmsp_name, str);
    LogEMSHMI("write [%s] to [%s] %s", tmsp_name, str, ret == 0 ? "success" : "failed");

    return ret;
}
/*******************************************************************************
 * @brief 向数据库写入配置项系统参数配置数据
 * @param pitemname 系统配置参数名称
 * @param psyscfg 系统配置参数数据
 * @return 更新数据成功返回：0 更新数据失败返回 -其他
*******************************************************************************/
int system_para_update(char* pitemname, ems_system_config_t* psyscfg)
{
    int ret = 0;
    char str[128];
    sprintf(str, "%d;%d;%d;%d;%d;%d;%d;%d", psyscfg->mode, psyscfg->discharge_min_soc, psyscfg->charge_max_soc, \
    psyscfg->fnl_en, psyscfg->debug_mode_power, psyscfg->on_grid_power, psyscfg->bms_power, psyscfg->load_power);

    ret = db_set_config_data("BasicConfig", pitemname, str);
    LogEMSHMI("write [%s] to [%s] %s", pitemname, str, ret == 0 ? "success" : "failed");

    return ret;
}
/*******************************************************************************
 * @brief 向数据库写入通信参数配置数据
 * @param pitemname
 * @param pconncfg
 * @return 更新数据成功返回：0 更新数据失败返回其他 
*******************************************************************************/
int conn_para_update(char* pitemname, ems_conn_para_config_t* pconncfg)
{
    int ret = 0;
    char str[128];
    sprintf(str, "%s;%d;%d;%d;%d;%d", pconncfg->cloud_ip, pconncfg->cloud_port, pconncfg->modbus_id,\
    pconncfg->modbus_baud,pconncfg->hmi_baud, pconncfg->can_baud);

    ret = db_set_config_data("BasicConfig", pitemname, str);
    LogEMSHMI("write [%s] to [%s] %s", pitemname, str, ret == 0 ? "success" : "failed");

    return ret;
}
/*******************************************************************************
 * @brief 向数据库写入ems版本号数据
 * @param pitemname
 * @param pemsver
 * @return 更新数据成功返回：0 更新数据失败返回其他
*******************************************************************************/
int ems_version_update(char* pitemname, char* pemsver)
{
    int ret = 0;
    ret = db_set_config_data("BasicConfig", pitemname, pemsver);
    LogEMSHMI("write [%s] to [%s] %s", pitemname, pemsver, ret == 0 ? "success" : "failed");
    return ret;
}
/*******************************************************************************
 * @brief 向数据库写入控制器版本号数据
 * @param pitemname
 * @param pconver
 * @return 更新数据成功返回：0 更新数据失败返回其他
*******************************************************************************/
int control_version_update(char* pitemname, char* pconver)
{
    int ret = 0;
    ret = db_set_config_data("BasicConfig", pitemname, pconver);
    LogEMSHMI("write [%s] to [%s] %s", pitemname, pconver, ret == 0 ? "success" : "failed");
    return ret;
}
/*******************************************************************************
 * @brief 向数据库写入触摸屏版本号数据
 * @param pitemname
 * @param phmiver
 * @return 更新数据成功返回：0 更新数据失败返回其他
*******************************************************************************/
int hmi_version_update(char* pitemname, char* phmiver)
{
    int ret = 0;
    ret = db_set_config_data("BasicConfig", pitemname, phmiver);
    LogEMSHMI("write [%s] to [%s] %s", pitemname, phmiver, ret == 0 ? "success" : "failed");
    return ret;
}

/*******************************************************************************
 * @brief 相数据库写入hmi用户登录信息
 * @param pitemname 
 * @param puser
 * @return 更新数据成功返回：0 更新数据失败返回其他
*******************************************************************************/
int hmi_user_update(char* pitemname, hmi_logon_t* puser)
{
    int ret = 0;
    char str[128];
    sprintf(str, "%s;%d", puser->username, puser->password);
    ret = db_set_config_data("BasicConfig", pitemname, str);
    LogEMSHMI("write [%s] to [%s] %s", pitemname, str, ret == 0 ? "success" : "failed");
    return ret;
}

/*******************************************************************************
 * @brief 向数据库写入场站功率等数据
 * @param pitemname 
 * @param fnum
 * @param inum
 * @return 更新数据成功返回：0 更新数据失败返回其他
*******************************************************************************/
int hmi_pcs_power_update(char* pitemname, float fnum,int inum)
{
    int ret = 0;
    char str[128];
    sprintf(str, "%f;%d", fnum, inum);
    ret = db_set_config_data("BasicConfig", pitemname, str);
    LogEMSHMI("write [%s] to [%s] %s", pitemname, str, ret == 0 ? "success" : "failed");
    return ret;
}



            