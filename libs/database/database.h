/***************************************************************************
 * Copyright (C), 2016-2026, All rights reserved.
 * File        :  database.c
 * Decription  :  database api
 * Author      :  zhums
 * Version     :  v1.0
 * Date        :  2022/1/15
 * Histroy     :
 **************************************************************************/
#ifndef __DATABASE_H__
#define __DATABASE_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>

/***********************************************************
* MACROS
*/

// 数据库文件
#define DATABASE_PATH_DATA    "/home/root/db/data.db"

#define SHOULD_KEEP_TRY(rc)		(rc==SQLITE_BUSY || rc == SQLITE_LOCKED || rc == SQLITE_IOERR)		

//todo: TABLE_NAME_LEN改一下，名字有问题
#define  TABLE_NAME_LEN     500
#define  TABLE_DATA_LEN     1000

#define MAX_QUERY_ROW_NUM				100

// 数据表名称
#define TABLE_NAME_CONFIG               "BasicConfig"               //基础配置数据
#define TABLE_NAME_SUXIN_SEQUENCE 		"SUXIN_SEQUENCE" 			//自用序列表
#define TABLE_NAME_CHANNEL 				"task" 					    //排产任务表

// 基础配置各列
#define TAB_CONFIG_COL_ID_NAME           0 //名称
#define TAB_CONFIG_COL_ID_VALUE          1 //值


// 其他项
#define DB_FIELD_SPLIT		"|"
#define COL_VALUE_STMT		"$$stmt$$"		//更新字段值是sql语句标识
#define COL_VALUE_OFFSET	8				//length(COL_VALUE_STMT)

//业务相关
#define PCS_ITEM_NUM	12		//PCS设备表字段数

typedef enum em_db_operate_type
{
	OP_QUERY=0,
	OP_INSERT,
	OP_UPDATE,
	OP_DELETE,
	OP_MAX
}DB_OP_TYPE;

/***********************************************************
* STRUCT
*/
struct db_table_config_data {
    char *table_name;
    char *key;
    char *value;
};

typedef struct cb_param
{
	int rowid;
	int maxrow;
	int max_rowlen;
	char **presult;
	char split_char[4];
}cb_param_t;

typedef struct st_sql_param
{
	char *table;
	char **column_name;
	char **column_value;
	int n_column;
	char condition[256];
}sql_param_t;


// 打开、关闭数据库
int db_open_database(char* db_file);
int db_close_database(char* db_file);
void db_free(void* ptr);	//释放db接口申请的动态内存

// 查询与修改指定name的值
int db_get_key_data(char *table_name, char *key_name, char *key_string, char *value_name, char *value_string, int maxlen);
int db_set_key_data(char *table_name, char *key_name, char *key_string, char *value_name, char *value_string);

// 查询与修改配置项
int db_get_config_data_try(char* table_name, char *key_string, char *value_string);
int db_set_config_data_try(char* table_name, char *key_string, char *value_string);
int db_get_config_data(char* key_string, char *value_string, int maxlen, char *default_val);
int db_set_config_data(char* table_name, char *key_string, char *value_string);
int db_get_string_config(char* key_string, char *value_string, int maxlen, char *default_val);
int db_get_number_config(char* key_string, int *value_num,int default_val);

//通用查询和更新接口
int assemble_sql(DB_OP_TYPE op_type,sql_param_t *sql_param, char* sql, int max_sqllen);
int db_do_query(const char* sql, char** presult, int *maxrow, int max_rowlen);
int db_do_query2(sql_param_t *sql_param, char** presult, int *maxrow, int max_rowlen);
int db_do_update(const char* sql, int* nchanges);
int db_do_update2(DB_OP_TYPE op_type, sql_param_t *sql_param, int* nchanges);

// 业务相关接口


#if 0
//-- 排产计划数据查询与修改接口
int task_insert(task_t* ptask, int try_update);
int task_update(task_t* ptask, int only_cur_num, int only_state);
int task_query(task_t* ptask);
int task_delete(int task_id, int delete_all);
int task_query_count();
int task_query_all(task_t* ptask);

//-- 工艺数据查询与修改接口
int craft_insert(craft_t* pcraft, int try_update);
int craft_update(craft_t* pcraft, int only_taskID);
int craft_query(craft_t* pcraft);
int craft_delete(int craft_id, int delete_all);
int craft_query_count();
int craft_query_all(craft_t* pcraftlist, int lisize, int taskID);

//　其他操作接口
int login_info_query(login_info_t* login_info);
int login_info_update(login_info_t* login_info);
#endif 

#endif
