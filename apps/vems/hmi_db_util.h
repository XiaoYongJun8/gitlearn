/*******************************************************************************
 * @             Copyright (c) 2023 LinyPower, All Rights Reserved.
 * @*****************************************************************************
 * @FilePath     : hmi_db_util.h
 * @Descripttion : 
 * @Author       : xiaoyongjun
 * @E-Mail       : xiaoyongjun@linygroup.cn
 * @Version      : v1.0.0
 * @Date         : 2023-10-24 13:08:49
*******************************************************************************/
#ifndef _HMI_DB_UTIL_H_
#define _HMI_DB_UTIL_H_

#include "common_define.h"
#include "vems.h"
#include "database.h"

int dev_pcs_insert(dev_pcs_params_t* ppcs, int try_update);
int dev_pcs_update(dev_pcs_params_t* ppcs, int only_pcs_name, int only_state);
int dev_pcs_delete(int pcs_id, int delete_all);

int charge_time_update(char* tmsp_name, ems_tmsp_item_t* ptmsp);

int system_para_update(char* pitemname, ems_system_config_t* psyscfg);

int conn_para_update(char* pitemname, ems_conn_para_config_t* pconncfg);

int ems_version_update(char* pitemname, char* pemsver);

int control_version_update(char* pitemname, char* pconver);

int hmi_version_update(char* pitemname, char* phmiver);

int hmi_user_update(char* pitemname, hmi_logon_t* puser);

int hmi_pcs_power_update(char* pitemname, float fnum, int inum);

#endif 
