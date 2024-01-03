/***************************************************************************
 * Copyright (C), 2016-2026, All rights reserved.
 * File        :  database.c
 * Decription  :  database api
 * Author      :  zhums
 * Version     :  v1.0
 * Date        :  2022/1/15
 * Histroy     :
 **************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "sqlite3.h"

#include "database.h"
#include "debug_and_log.h"


// sqlite不支持并发导致多进程、多线程同时更新数据库时出现db is locked失败返回的解决办法
// 1. 用信号量、互斥量或记录锁来解决多进程同步，用线程同步的方式解决多线程同步
// 2. 采取重试的方法，超时时间要设置好
// 3. 用sqlite自带的sqlite3_busy_timeout或者sqlite3_busy_handler，其实也是延时重试


#define  SQL_LEN     128
#define  TABLE_DATA_LEN     1000
#define  CONFIG_DATA_LEN    500

// 数据库锁定后重试间隔时间100ms,重试超时时间5000ms,重试最大次数50
#define LOCK_RETRY_DELAY    100000
#define LOCK_RETRY_TIMEOUT  5000000
#define LOCK_RETRY_CNT      (LOCK_RETRY_TIMEOUT/LOCK_RETRY_DELAY)

/*********************************************************************
 * GLOBAL VARIABLES
 */
static sqlite3* db;


static void safe_sleep(long sec,long usec)
{
	struct timeval to;

	to.tv_sec = sec;
	to.tv_usec = usec;

	select(0,NULL,NULL,NULL,&to);
}

//将字符串pstr以psplit分隔为多个子串，原串pstr会被修改，分隔后的子串通过parr返回
__attribute__((unused)) static int split_string_to_array(char* pstr, char* psplit, char** parr, int maxcnt)
{
    if(NULL == pstr || NULL == psplit || NULL == parr || 0 == strlen(pstr))
        return 0;

    char* pcur = pstr;
    char* pfind = NULL;
    int cnt = 0;
    int offset = strlen(psplit);
    while((pfind = strstr(pcur, psplit)) && cnt < maxcnt)
    {
        parr[cnt++] = pcur;
        *pfind = '\0';
        pcur = pfind + offset;
    }
    if(cnt < maxcnt)
    {
        parr[cnt++] = pcur;
    }

    return cnt;
}

/*********************************************************************
* @fn	      db_open_database
* @brief      打开数据库
* @param      db_file[in]: 数据库文件
* @return     0: success, !0: fail
* @history:
**********************************************************************/
int db_open_database(char *db_file)
{
    int ret;

	if (NULL == db_file) {
		LOG(LOG_LV_ERROR, "Invalid path of database file!\n");
		return -1;
	}

    ret = sqlite3_open(db_file, &db);
    if( ret ) {
		LOG(LOG_LV_ERROR, "Can't open database: %s\n", sqlite3_errmsg(db));
        return ret;
    }

    return 0;
}

/*********************************************************************
* @fn	      db_close_database
* @brief      关闭数据库
* @param      db_file[in]: 数据库文件
* @return     0: success, !0: fail
* @history:
**********************************************************************/
int db_close_database(char *db_file)
{
    int ret = 0;

	if (NULL == db_file) {
		LOG(LOG_LV_ERROR, "Invalid path of database file!\n");
		return -1;
	}

	ret = sqlite3_close(db);
    if( ret != SQLITE_OK ) {
		LOG(LOG_LV_ERROR, "close database fail");
        return ret;
    }
    return 0;
}

/*******************************************************************************
 * @brief 释放db接口申请的动态内存
 * @param ptr to free memory pointer
 * @return none
*******************************************************************************/
void db_free(void* ptr)
{
	if(!ptr) free(ptr);
}

/*********************************************************************
* @fn	        db_get_key_data
* @brief        从数据表中获取配置数据(指定关键字的指定字段的值)
* @param        table_name[in]:　表名
* @param        key_name[in]: key字段名
* @param        key_string[in]: key值
* @param        value_name[in]: value字段名
* @param        value_string[in/out]: value值
* @return     　0: success, !0: fail
* @history:
**********************************************************************/
int db_get_key_data(char *table_name, char *key_name, char *key_string, char *value_name, char *value_string, int maxlen)
{
    int ret = -1;
    char sql_buf[TABLE_NAME_LEN] = {0};
    sqlite3_stmt *stat = NULL;
	const unsigned char *read_value = NULL;
	int read_len = -1;

	if (NULL == table_name || NULL == key_name || NULL == key_string
		|| NULL == value_name || NULL == value_string) {
		LOG(LOG_LV_ERROR, "Invalid params!\n");
        return -1;
	}
    snprintf(sql_buf, TABLE_NAME_LEN, "select %s,%s from %s where %s='%s'"
    	, key_name, value_name, table_name, key_name, key_string);
	LOG(LOG_LV_TRACE, "sql: %s\n", sql_buf);

	ret = sqlite3_prepare(db,sql_buf,-1,&stat,0);
	if (ret != SQLITE_OK) {
		LOG(LOG_LV_ERROR, "sql error: %s\n", sqlite3_errmsg(db));
		sqlite3_finalize( stat );
		return ret;
	}

    ret = sqlite3_step(stat);
    if(ret != SQLITE_ROW) {
		LOG(LOG_LV_ERROR, "sql error:%s\n", sqlite3_errmsg(db));
        sqlite3_finalize( stat );
		return ret;
    }

    // todo: 这边应该有问题，TAB_CONFIG_COL_ID_VALUE不对
    read_value = sqlite3_column_text(stat, TAB_CONFIG_COL_ID_VALUE);
    read_len = sqlite3_column_bytes(stat, TAB_CONFIG_COL_ID_VALUE);
    read_len = read_len >= maxlen - 1 ? maxlen - 1 : read_len;
    memcpy(value_string, read_value, read_len);
    value_string[read_len] = '\0';

	LOG(LOG_LV_TRACE,"name: %s, value: %s\n",key_string,value_string);

    sqlite3_finalize( stat );
    return 0;
}

/*********************************************************************
* @fn	        db_set_key_data
* @brief        从数据表中设置配置数据(指定关键字的指定字段的值)
* @param        table_name[in]:　表名
* @param        key_name[in]: key字段名
* @param        key_string[in]: key值
* @param        value_name[in]: value字段名
* @param        value_string[in/out]: value值
* @return     　0: success, !0: fail
* @history:
**********************************************************************/
int db_set_key_data(char *table_name, char *key_name, char *key_string, char *value_name, char *value_string)
{
    int ret = -1;
    char sql_buf[TABLE_NAME_LEN] = {0};
    char *errmsg = NULL;

	if (NULL == table_name || NULL == key_name || NULL == key_string
		|| NULL == value_name || NULL == value_string) {
		LOG(LOG_LV_ERROR, "Invalid params!\n");
        return -1;
	}
    snprintf(sql_buf, TABLE_NAME_LEN, "update %s set %s='%s' where %s='%s'"
    	, table_name, value_name, value_string, key_name, key_string);
	LOG(LOG_LV_TRACE, "sql: %s\n", sql_buf);

	ret = sqlite3_exec( db, sql_buf, NULL, NULL, &errmsg );
	if(ret != SQLITE_OK) {
		LOG(LOG_LV_WARN,"exec update sql error ret:%d, errmsg=%s\n", ret,  errmsg);
		LOG(LOG_LV_TRACE,"update sql[%s]\n", sql_buf);
		sqlite3_free(errmsg);
		return ret;
	}
	sqlite3_free(errmsg);

    return 0;
}

/*********************************************************************
* @fn	      db_get_config_data_try
* @brief      获取配置数据
* @param      table_name[in]: 　数据库表名
* @param      key_string[in]:　 key值
* @param      value_string[in]: value值
* @return     0: success, !0: fail
* @history:
**********************************************************************/
int db_get_config_data_try(char *table_name, char *key_string, char *value_string)
{
    char sql_buf[TABLE_NAME_LEN] = "select * from ";
    sqlite3_stmt *stat = NULL;
    int ret = -1;
    int pos = strlen(sql_buf);
	const unsigned char *read_value = NULL;
	int read_len = -1;

    pos += sprintf(sql_buf + pos, "%s where cfgname='%s'",table_name, key_string);

	if (NULL == table_name || NULL == key_string || NULL == value_string) {
		LOG(LOG_LV_ERROR, "Invalid params! %p, %p, %p\n", table_name, key_string, value_string);
        return -1;
	}

    ret = sqlite3_prepare(db, sql_buf, -1, &stat, 0);
    if (ret != SQLITE_OK) {
		LOG(LOG_LV_ERROR, "get config %s sqllite error: %s\n", key_string, sqlite3_errmsg(db));
		sqlite3_finalize( stat );
        return ret;
    }

	// add by zhums@2020-02-14
	// cgi 程序中调用打印信息，会破坏接口结构
	LOG(LOG_LV_TRACE, "get config sql: %s\n", sql_buf);
    ret = sqlite3_step(stat);
    if(ret != SQLITE_ROW) {
		LOG(LOG_LV_ERROR, "get config %s sqllite error: %s\n", key_string, sqlite3_errmsg(db));
        sqlite3_finalize( stat );
		return ret;
    }

    read_value = sqlite3_column_text(stat, TAB_CONFIG_COL_ID_VALUE);
    read_len = sqlite3_column_bytes(stat, TAB_CONFIG_COL_ID_VALUE);
    memcpy(value_string, read_value, read_len);
    value_string[read_len] = '\0';
	LOG(LOG_LV_TRACE, "get config name: %s, return value: %s\n", key_string, value_string);

    sqlite3_finalize( stat );

    return 0;
}

/*********************************************************************
* @fn	      db_set_config_data_try
* @brief      在基础配置数据表中修改配置数据
* @param      table_name[in]: 　数据库表名
* @param      key_string[in]:　 key值
* @param      value_string[in]: value值
* @return     0: success, !0: fail
* @history:
**********************************************************************/
int db_set_config_data_try(char *table_name, char *key_string, char *value_string)
{
    sqlite3_stmt * stat;
    int ret=-1, pos=0;
    char sql_update[ TABLE_DATA_LEN ]="UPDATE ";

    if(NULL == table_name || NULL == key_string || NULL == value_string) {
		LOG(LOG_LV_ERROR, "Invalid params! %p, %p, %p\n", table_name, key_string, value_string);
        return -1;
    }

    pos = strlen(sql_update);
    pos += sprintf(sql_update + pos, "%s SET cfgvalue ='%s' WHERE cfgname = '%s'", table_name, value_string, key_string);

	// add by zhums@2020-02-20
	// cgi 程序中调用打印信息，会破坏接口结构
	LOG(LOG_LV_TRACE, "set config sql: %s\n", sql_update);

    ret = sqlite3_prepare( db, sql_update, -1, &stat, 0 );
    if (ret != SQLITE_OK) {
		LOG(LOG_LV_ERROR, "set config %s sqllite error: %s\n", key_string, sqlite3_errmsg(db));
		sqlite3_finalize( stat );
        return ret;
    }

    ret = sqlite3_step( stat );
    if(ret != SQLITE_DONE) {
		LOG(LOG_LV_ERROR, "get config %s sqllite error: %s\n", key_string, sqlite3_errmsg(db));
		sqlite3_finalize( stat );
        return ret;
    }

    sqlite3_finalize( stat );

    return 0;
}

/*********************************************************************
* @fn	        db_get_config_data
* @brief        读取配置数据的安全接口
* @param        key_string[in]:　 key值
* @param        value_string[out]: value值
* @param        maxlen[in]: value字符串最大长度
* @param        default_val[in]: 默认值
* @return     　-1: fail, 0: success
* @history:
**********************************************************************/
int db_get_config_data(char *key_string, char *value_string, int maxlen, char *default_val)
{
	int cnt = 0;
	int ret = 0;
	if(NULL == key_string || NULL == value_string) {
		LOG(LOG_LV_ERROR, "Invalid params!\n");
		return -1;
	}

	while(cnt < LOCK_RETRY_CNT) {
		ret = db_get_config_data_try(TABLE_NAME_CONFIG, key_string, value_string);
		if(!SHOULD_KEEP_TRY(ret)) {
			break;
		}
		++cnt;
		safe_sleep(0, LOCK_RETRY_DELAY);
	}

	if (ret && default_val) {
		LOG(LOG_LV_ERROR, "get config %s fail! set to default value %s\n", key_string, default_val);
		strncpy(value_string, default_val, maxlen);
		value_string[maxlen-1] = '\0';
		ret = 0;
	} else if(ret != 0) {
		LOG(LOG_LV_ERROR, "get %s fail return %d, set ret = -1 \n", key_string, ret);
		ret = -1;
	}

	value_string[maxlen-1]='\0';

	return ret;
}

/*********************************************************************
* @fn	      db_set_config_data
* @brief      在基础配置数据表中安全的修改配置数据
* @param      table_name[in]: 　数据库表名
* @param      key_string[in]:　 key值
* @param      value_string[in]: value值
* @return     -1: fail, 0: success
* @history:
**********************************************************************/
int db_set_config_data(char *table_name, char *key_string, char *value_string)
{
    int cnt = 0;
	int ret = 0;

	if(NULL == key_string || NULL == value_string) {
		LOG(LOG_LV_ERROR, "Invalid params!\n");
		return -1;
	}

	while(cnt < LOCK_RETRY_CNT) {
		ret = db_set_config_data_try(TABLE_NAME_CONFIG, key_string, value_string);
		if(!SHOULD_KEEP_TRY(ret)) {
			break;
		}
		++cnt;
		safe_sleep(0, LOCK_RETRY_DELAY);
	}

    if(ret) {
        LOG(LOG_LV_ERROR, "db set config data safe fail!\n");
    }

    return ret;
}

/*********************************************************************
* @fn	        db_get_string_config
* @brief        读取字符串配置数据的安全接口
* @param        key_string[in]:　 key值
* @param        value_string[out]: value值
* @param        maxlen[in]: value字符串最大长度
* @param        default_val[in]: 默认值
* @return     　-1: fail, 0: success
* @history:
**********************************************************************/
int db_get_string_config(char *key_string, char *value_string, int maxlen, char *default_val)
{
	return db_get_config_data(key_string, value_string, maxlen, default_val);
}

/*********************************************************************
* @fn	        db_get_number_config
* @brief        读取配置数据为整数的安全接口
* @param        key_string[in]:　key值
* @param        value_num[out]:  value值
* @param        default_val[in]: 默认值
* @return     　-1: fail, 0: success
* @history:
**********************************************************************/
int db_get_number_config(char *key_string, int *value_num, int default_val)
{
	char numstr[16]={0};

	if(NULL == key_string || NULL == value_num) {
		LOG(LOG_LV_ERROR, "Invalid params!\n");
		return -1;
	}

	if(db_get_config_data(key_string, numstr, sizeof(numstr), NULL) == 0) {
		*value_num = atoi(numstr);
	} else {
		*value_num = default_val;
	}

	return 0;
}

/*********************************************************************
* @fn	        format_query_info
* @brief        sqlite 每查到一条记录，就调用一次这个回调
* @param        args[in]:　  　　　参数
* @param        n_column[in]:  　　列数
* @param        column_value[out]: 列内容
* @param        column_name[out]:  列名
* @return     　-1: fail, 0: success
* @history:
**********************************************************************/
int format_query_info( void * args, int n_column, char ** column_value, char ** column_name )
{
	int i= 0;
	int row = 0;
	int offset = 0;
	int max_rowlen = 0;
	cb_param_t *pargs = (cb_param_t*) args;
	if(pargs == NULL || pargs->presult == NULL)
	{
		LOG(LOG_LV_WARN, "invalid params\n");
		return -1;
	}

	LOG(LOG_LV_TRACE, "current rowid=%d, maxrow=%d, n_column=%d\n", pargs->rowid, pargs->maxrow, n_column);
	row = pargs->rowid;
	max_rowlen = pargs->max_rowlen;
	//result buffer is full, abort get next row info
	if(pargs->rowid >= pargs->maxrow)
	{
		LOG(LOG_LV_INFO, "has get maxrow[%d], rowid=%d\n", pargs->maxrow, pargs->rowid);
		return -1;
	}

	if(pargs->max_rowlen == 0)
	{
		max_rowlen = 0;
		for(i = 0; i< n_column; i++)
		{
			if(column_value[i] == NULL)
				continue;
			max_rowlen += strlen(column_value[i]);
		}
		max_rowlen += n_column + 1;
		LOG(LOG_LV_TRACE, "row[%d] need malloc %d bytes\n", pargs->rowid, max_rowlen);
		pargs->presult[row] = (char*)malloc(max_rowlen);
	}

	if(pargs->presult[row] == NULL)
	{
		LOG(LOG_LV_WARN,"result buffer[%d] is null\n", pargs->rowid);
		return -1;
	}

	for(i = 0; i< n_column; i++)
	{
		if(offset > max_rowlen)
			break;
		if(column_value[i] == NULL)
			offset += snprintf(pargs->presult[row] + offset, max_rowlen - offset, "%s", pargs->split_char);
		else
			offset += snprintf(pargs->presult[row] + offset, max_rowlen - offset, "%s%s", column_value[i], pargs->split_char);
		LOG(LOG_LV_TRACE, "%s=%s\n", column_name[i], column_value[i]);
	}
	offset = strlen(pargs->presult[row]);
	if(pargs->presult[row][offset - 1] == pargs->split_char[0])
		pargs->presult[row][offset - 1] = '\0';
	pargs->rowid++;

	return 0;
}

/*********************************************************************
* @fn	        assemble_sql
* @brief        拼装sql语句
* @param        sql[in]: 要执行的更新(insert/update/delete)sql语句，在函数执行结束前不要修改其值
* @param        presult[in]: 存放查得的数据，此函数不负责分配内存，切记从外部传入可用内存
* @return     　-1: fail, 0: success
* @history:
**********************************************************************/
int assemble_sql(DB_OP_TYPE op_type,sql_param_t *sql_param, char* sql, int max_sqllen)
{
	static char* opts[] = {"select", "insert into", "update", "delete from"};
	char* table = NULL;
	char* condition = NULL;
	char sql_items[512] = {0};
	int sql_item_offset = 0;
	int offset = 0;
	int ret = 0;
	int i = 0;

	if(sql_param == NULL || sql_param->table == NULL || sql == NULL)
	{
		LOG(LOG_LV_WARN, "invaild params\n");
		return -1;
	}

	table = sql_param->table;
	condition = sql_param->condition;
	switch(op_type)
	{
		case OP_QUERY:
			if(sql_param->column_name == NULL)
			{
				ret = -1;
				LOG(LOG_LV_WARN, "invaild params, column_name is null\n");
				break;
			}

			//format sql items
			for(i=0;i<sql_param->n_column; i++)
			{
				if(sql_param->column_name[i] == NULL
					|| sql_item_offset >= sizeof(sql_items))
					break;
				sql_item_offset += snprintf(sql_items + sql_item_offset, sizeof(sql_items) - sql_item_offset
									, "%s,", sql_param->column_name[i]);
			}
			sql_items[sql_item_offset - 1] = '\0';

			offset += snprintf(sql + offset , max_sqllen - offset
				, "%s %s from %s", opts[op_type], sql_items, table);
			if(condition && condition[0] != '\0')
			{
				offset += snprintf(sql + offset , max_sqllen - offset
				, " where %s", condition);
			}
			break;
		case OP_INSERT:
			if(sql_param->column_value == NULL)
			{
				ret = -1;
				LOG(LOG_LV_WARN, "invaild params, column_value is null\n");
				break;
			}

			//format sql items
			for(i=0;i<sql_param->n_column; i++)
			{
				if(sql_param->column_value[i] == NULL
					|| sql_item_offset >= sizeof(sql_items))
					break;
				sql_item_offset += snprintf(sql_items + sql_item_offset, sizeof(sql_items) - sql_item_offset
									, "'%s',", sql_param->column_value[i]);
			}
			sql_items[sql_item_offset - 1] = '\0';

			offset += snprintf(sql + offset , max_sqllen - offset
				, "%s %s values(%s)", opts[op_type], table, sql_items);
			break;
		case OP_UPDATE:
			if(sql_param->column_name == NULL || sql_param->column_value == NULL)
			{
				ret = -1;
				LOG(LOG_LV_WARN, "invaild params, column_name or column_value is null\n");
				break;
			}

			//format sql items
			for(i=0;i<sql_param->n_column; i++)
			{
				if(sql_param->column_name[i] == NULL || sql_param->column_value[i] == NULL
					|| sql_item_offset >= sizeof(sql_items))
					break;

				if(sql_param->column_value[i][0] == '$'
				  &&strncmp(sql_param->column_value[i], COL_VALUE_STMT, COL_VALUE_OFFSET) == 0)
				{//此字段值是由sql语句产生的，字段值不用引号包围
					sql_item_offset += snprintf(sql_items + sql_item_offset, sizeof(sql_items) - sql_item_offset
									, "%s = %s,", sql_param->column_name[i], sql_param->column_value[i] + COL_VALUE_OFFSET);
				}
				else
				{
					sql_item_offset += snprintf(sql_items + sql_item_offset, sizeof(sql_items) - sql_item_offset
									, "%s = '%s',", sql_param->column_name[i], sql_param->column_value[i]);
				}
			}
			sql_items[sql_item_offset - 1] = '\0';

			offset += snprintf(sql + offset , max_sqllen - offset
				, "%s %s set %s", opts[op_type], table, sql_items);
			if(condition && condition[0] != '\0')
			{
				offset += snprintf(sql + offset , max_sqllen - offset
				, " where %s", condition);
			}
			break;
		case OP_DELETE:
			offset += snprintf(sql + offset , max_sqllen - offset
				, "%s %s", opts[op_type], table);
			if(condition && condition[0] != '\0')
			{
				offset += snprintf(sql + offset , max_sqllen - offset
				, " where %s", condition);
			}
			break;
		default:
			ret = -1;
			LOG(LOG_LV_WARN, "unknown operator type[%d]\n", op_type);
			break;
	}
	LOG(LOG_LV_TRACE, "assemble sql ret=%d, sql=%s, sqllen=%d, max_sqllen=%d\n", ret, sql, offset, max_sqllen);

	return ret;
}

/*********************************************************************
* @fn	        db_do_query
* @brief        执行sql语句，返回查的的信息(失败时再重试指定次数)
* @param        sql[in]: 要执行的sql语句，在函数执行结束前不要修改其值
* @param        presult[in]: 存放查得的数据，此函数不负责分配内存，切记从外部传入可用内存
* @param   　　　maxrow[in/out]: 最多取几条数据，并返回实际读取的行数
* @param        max_rowlen[in]: presult[i]指向的内存最多写入多少字节；若设置为0，则由本函数动态申请内存
* @return     　-1: fail, 0: success
* @history:
**********************************************************************/
int db_do_query(const char* sql, char** presult, int *maxrow, int max_rowlen)
{
	int ret = 0;
	int try_cnt = 0;
	char* errmsg = NULL;
	cb_param_t qparam;

	if(NULL == sql || NULL == presult || NULL == maxrow)
	{
		LOG(LOG_LV_WARN,"invalid params\n");
		return -1;
	}

	memset(&qparam, 0, sizeof(cb_param_t));
	qparam.rowid = 0;
	qparam.maxrow = *maxrow;
	qparam.max_rowlen = max_rowlen;
	qparam.presult = presult;
	strcpy(qparam.split_char, DB_FIELD_SPLIT);
	LOG(LOG_LV_TRACE,"query sql[%s]\n", sql);
	do
	{
		try_cnt++;
		ret = sqlite3_exec( db, sql, format_query_info, (void*)&qparam, &errmsg );
		if(ret != SQLITE_OK && ret != SQLITE_ABORT)
		{
			LOG(LOG_LV_WARN,"exec query sql error ret:%d, errmsg=%s\n", ret,  errmsg);
			sqlite3_free(errmsg);
			safe_sleep(0, LOCK_RETRY_DELAY);
		}
		else
		{
			break;
		}
	}while(try_cnt < LOCK_RETRY_CNT);
	if(ret != SQLITE_OK && ret != SQLITE_ABORT)
		return -1;

	*maxrow = qparam.rowid;
	sqlite3_free(errmsg);
	LOG(LOG_LV_TRACE,"return maxrow=%d\n", *maxrow);

	return 0;
}

/*********************************************************************
* @fn	        db_do_query2
* @brief        现根据sql参数构造完整的sql语句，然后执行sql语句，返回查的的信息(失败时再重试指定次数)
* @param        sql_param[in]: sql参数，用来构造查询sql
* @param        presult[in]: 存放查得的数据，此函数不负责分配内存，切记从外部传入可用内存
* @param        maxrow[in/out]: 最多取几条数据，并返回实际读取的行数
* @param        max_rowlen[in]: presult[i]指向的内存最多写入多少字节；若设置为0，则由本函数动态申请内存
* @return     　-1: fail, 0: success
* @history:
**********************************************************************/
int db_do_query2(sql_param_t *sql_param, char** presult, int *maxrow, int max_rowlen)
{
	int ret = 0;
	char sql[512] = {0};

	if(NULL == sql_param || NULL == presult)
	{
		LOG(LOG_LV_WARN,"invalid params\n");
		return -1;
	}

	ret = assemble_sql(OP_QUERY, sql_param, sql, sizeof(sql));
	if(ret!=0)
	{
		LOG(LOG_LV_ERROR, "assemble sql failed return:%d\n", ret);
		return -1;
	}
	ret = db_do_query(sql, presult, maxrow, max_rowlen);

	return ret;
}

/*********************************************************************
* @fn	        db_do_update
* @brief        执行sql语句，返回查的的信息(失败时再重试指定次数)
* @param        sql[in]: 要执行的更新(insert/update/delete)sql语句，在函数执行结束前不要修改其值
* @param        nchanges[out]: 执行的更新(insert/update/delete)sql语句受影响的行数
* @return     　SQLITE_OK: success, other: failed(-1: param error; >0: sqlite error)
* @history:
**********************************************************************/
int db_do_update(const char* sql, int* nchanges)
{
	int ret = 0;
	int try_cnt = 0;
	char* errmsg = NULL;

	if(NULL == sql)
	{
		LOG(LOG_LV_WARN,"invalid params\n");
		return -1;
	}

	LOG(LOG_LV_TRACE,"update sql[%s]\n", sql);
	do
	{
		try_cnt++;
		ret = sqlite3_exec( db, sql, NULL, NULL, &errmsg );
		if(ret != SQLITE_OK)
		{
			LOG(LOG_LV_WARN,"exec update sql error ret:%d, errmsg=%s\n", ret,  errmsg);
			sqlite3_free(errmsg);
			if(ret != SQLITE_BUSY && ret != SQLITE_LOCKED)
				break;
			safe_sleep(0, LOCK_RETRY_DELAY);
		}
		else
		{
			if(nchanges != NULL)
			{
				*nchanges = sqlite3_changes(db);
			}
			break;
		}
	}while(try_cnt < LOCK_RETRY_CNT);
	
	return ret;
}

/*********************************************************************
* @fn	        db_do_update2
* @brief        先根据传入的sql参数构造sql语句，然后执行sql语句。若失败，再重试指定次数
* @param        op_type[in]: 要执行的更新(insert/update/delete)
* @param        sql_param[in]: sql参数，用来构造完整的sql语句
* @param        nchanges[out]: 执行的更新(insert/update/delete)sql语句受影响的行数
* @return     　SQLITE_OK: success, other: failed(-1: param error; >0: sqlite error)
* @history:
**********************************************************************/
int db_do_update2(DB_OP_TYPE op_type, sql_param_t *sql_param, int* nchange)
{
	int ret = 0;
	char sql[512] = {0};

	if(NULL == sql_param)
	{
		LOG(LOG_LV_WARN,"invalid params\n");
		return -1;
	}

	ret = assemble_sql(op_type, sql_param, sql, sizeof(sql));
	if(ret!=0)
	{
		LOG(LOG_LV_ERROR, "assemble sql failed return:%d\n", ret);
		return -1;
	}
	ret = db_do_update(sql, nchange);

	return ret;
}

// 业务相关接口

#if 0
int login_info_query(login_info_t* login_info)
{
	int ret = 0;
	int size = 0;
	int maxrow = 1;
	char sql[128] = {0};
	char *login_rows[1]={0};
	char *pdata_row = NULL;
	char *login_detail[LOGIN_ITEM_NUM] = {0};

	snprintf(sql, sizeof(sql), "select * from login");
	ret = db_do_query(sql, (char **)&login_rows, &maxrow, 0);
	if(ret != 0) {
		LogError("select login info error return:%d", ret);
		return -1;
	}

	if(maxrow == 0) {
		LogError("query not found login");
		return 0;
	}

	pdata_row = login_rows[0];
	size = split_string_to_array(pdata_row, DB_FIELD_SPLIT, (char**)&login_detail, LOGIN_ITEM_NUM);
	if(size == 0) {
		db_free(login_rows[0]);
		LogError("split login info[%s] failed\n", pdata_row);
		return -1;
	}

    login_info->state = atoi(login_detail[0]);
    strncpy(login_info->weld_machineid, login_detail[1], WELD_MACHINEID_LEN);
    strncpy(login_info->welder_name, login_detail[2], WELDER_NAME_LEN);
    strncpy(login_info->welder_card, login_detail[3], WELDER_CARD_LEN);

    db_free(login_rows[0]);
	
	return 1;
}

// 更新登录信息
int login_info_update(login_info_t* login_info)
{
    int ret = 0;
	int nchanges = 0;
	char sql[128] = {0};

	snprintf(sql, sizeof(sql), "update login set state=%d, weld_machineid='%s', welder_name='%s', welder_card='%s'", 
            login_info->state, login_info->weld_machineid, login_info->welder_name, login_info->welder_card);

	ret = db_do_update(sql, &nchanges);
	if(ret != SQLITE_OK) {
		LogError("update login with sql[%s] error return:%d", sql, ret);
		ret = -1;
	} else {
		LogInfo("update login success, nchanges=%d", nchanges);
	}

	return ret;
}

//-- 排产计划数据查询与修改接口
//return: 0: success; -1: failed
int task_insert(task_t* ptask, int try_update)
{
	int ret = 0;
	int nchanges = 0;
	char sql[256] = {0};

	snprintf(sql, sizeof(sql), "insert into task values(%d, '%s', '%s', %d, %d, %d)", ptask->task_id, ptask->task_name
			, ptask->wo_number, ptask->plan_num, ptask->cur_num, ptask->state);
	ret = db_do_update(sql, &nchanges);
	if(ret != SQLITE_OK)
	{
		if(try_update && ret == SQLITE_CONSTRAINT){
            LogTrace("insert task[%d] failed, try do update task", ptask->task_id);
            ret = task_update(ptask, 0, 0);
		} else {
			LOG(LOG_LV_ERROR, "insert task[%d] with sql[%s] error return:%d\n", ptask->task_id, sql, ret);
			ret = -1;
		}
	}
	else
	{
		LOG(LOG_LV_INFO, "insert task[%d] success, nchanges=%d", ptask->task_id, nchanges);
	}

	return ret;
}

int task_update(task_t* ptask, int only_cur_num, int only_state)
{
	int ret = 0;
	int nchanges = 0;
	char sql[128] = {0};

	if(only_cur_num && only_state)
		snprintf(sql, sizeof(sql), "update task set curNum=%d, state=%d where taskID=%d", ptask->cur_num, ptask->state, ptask->task_id);
	else if(only_cur_num)
		snprintf(sql, sizeof(sql), "update task set curNum=%d where taskID=%d", ptask->cur_num, ptask->task_id);
	else if(only_state)
		snprintf(sql, sizeof(sql), "update task set state=%d where taskID=%d", ptask->state, ptask->task_id);
	else
		snprintf(sql, sizeof(sql), "update task set taskName='%s', planNum=%d, curNum=%d, state=%d where taskID=%d", ptask->task_name
			, ptask->plan_num, ptask->cur_num, ptask->state, ptask->task_id);

	ret = db_do_update(sql, &nchanges);
	if(ret != SQLITE_OK)
	{
		LOG(LOG_LV_ERROR, "update task[%d] with sql[%s] error return:%d\n", ptask->task_id, sql, ret);
		ret = -1;
	}
	else
	{
		LOG(LOG_LV_INFO, "update task[%d] success, nchanges=%d", ptask->task_id, nchanges);
	}

	return ret;
}

//return: 1:success; 0: 无指定排产计划; -1: failed
int task_query(task_t* ptask)
{
	int ret = 0;
	int size = 0;
	int maxrow = 1;
	char sql[128] = {0};
	char *task_rows[1]={0};
	char *pdata_row = NULL;
	char *task_detail[TASK_ITEM_NUM] = {0};

	snprintf(sql, sizeof(sql), "select * from task where taskID=%d", ptask->task_id);
	ret = db_do_query(sql, (char **)&task_rows, &maxrow, 0);
	if(ret != 0)
	{
		LOG(LOG_LV_ERROR, "select task[%d] info error return:%d", ptask->task_id, ret);
		return -1;
	}
	if(maxrow == 0)
	{
		db_free(task_rows[0]);
		LOG(LOG_LV_ERROR, "query not fount task[%s]", ptask->task_id);
		return 0;
	}

	pdata_row = task_rows[0];
	size = split_string_to_array(pdata_row, DB_FIELD_SPLIT, (char**)&task_detail, TASK_ITEM_NUM);
	if(size == 0)
	{
		db_free(task_rows[0]);
		LOG(0, "split task info[%s] failed\n", pdata_row);
		return -1;
	}

	strncpy(ptask->task_name, task_detail[1], TASK_NAME_LEN - 1);
    strncpy(ptask->wo_number, task_detail[2], TASK_WO_NUMBER_LEN - 1);
	ptask->plan_num = atoi(task_detail[3]);
	ptask->cur_num = atoi(task_detail[4]);
	ptask->state = atoi(task_detail[5]);
	db_free(task_rows[0]);
	
	return 1;
}

int task_delete(int task_id, int delete_all)
{
	int ret = 0;
	int nchanges = 0;
	char sql[128] = {0};

	if(!delete_all)
		snprintf(sql, sizeof(sql), "delete from task where taskID=%d", task_id);
	else
		snprintf(sql, sizeof(sql), "delete from task");
	ret = db_do_update(sql, &nchanges);
	if(ret != SQLITE_OK)
	{
		LOG(LOG_LV_ERROR, "delete task[%d] with sql[%s] error return:%d\n", task_id, sql, ret);
		ret = -1;
	}
	else
	{
		LOG(LOG_LV_INFO, "delete task[%d] success, nchanges=%d", task_id, nchanges);
	}

	return ret;
}

int task_query_count()
{
	int ret = 0;
	int cnt = 0;
	int maxrow = 1;
	char sql[128] = {0};
	char query_buff[32] = {0};
	char *data_rows[1]={query_buff};
	char *pcnt = NULL;

	snprintf(sql, sizeof(sql), "select count(taskID) as 'taskCount' from task");
	ret = db_do_query(sql, (char **)&data_rows, &maxrow, sizeof(query_buff));
	if(ret != 0)
	{
		LOG(LOG_LV_ERROR, "query task count error return:%d", ret);
		return -1;
	}

	pcnt = data_rows[0];
	cnt = atoi(pcnt);

	return cnt;
}


int task_query_all(task_t* ptask)
{
	int ret = 0;
	int size = 0;
	int maxrow = MAX_TASK_NUM;
	char sql[128] = {0};
	char *task_rows[MAX_TASK_NUM]={0};
	char *pdata_row = NULL;
	char *task_detail[TASK_ITEM_NUM] = {0};
    int i = 0;

	snprintf(sql, sizeof(sql), "select * from task");
	ret = db_do_query(sql, (char **)&task_rows, &maxrow, 0);
	if(ret != 0)
	{
		LOG(LOG_LV_ERROR, "select all task info error return:%d", ret);
		return -1;
	}
	if(maxrow == 0)
	{
		LOG(LOG_LV_ERROR, "query not found task");
		return 0;
	}

    for(i = 0; i < maxrow; i++) 
    {
        pdata_row = task_rows[i];
	    size = split_string_to_array(pdata_row, DB_FIELD_SPLIT, (char**)&task_detail, TASK_ITEM_NUM);
	    if(size == 0)
	    {
			db_free(task_rows[i]); task_rows[i] = NULL;
		    LogWarn("split task info[%s] failed\n", pdata_row);
		    continue;
	    }
        
        ptask[i].task_id = atoi(task_detail[0]);
	    strncpy(ptask[i].task_name, task_detail[1], TASK_NAME_LEN - 1);
        strncpy(ptask[i].wo_number, task_detail[2], TASK_WO_NUMBER_LEN - 1);
	    ptask[i].plan_num = atoi(task_detail[3]);
	    ptask[i].cur_num = atoi(task_detail[4]);
	    ptask[i].state = atoi(task_detail[5]);
		
		db_free(task_rows[i]); task_rows[i] = NULL;
    }
	
	return maxrow;
}


//-- 工艺数据查询与修改接口
//return: 0: success; -1: failed
int craft_insert(craft_t* pcraft, int try_update)
{
	int ret = 0;
	int nchanges = 0;
	char sql[512] = {0};

	snprintf(sql, sizeof(sql), "insert into craft values('%s', %d, %d, '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s')", pcraft->craftID, pcraft->taskID
			, pcraft->weldLevel, pcraft->weldMethod, pcraft->current, pcraft->voltage, pcraft->weldSpeed, pcraft->gasSpeed, pcraft->heatInput, pcraft->materialBrand
			, pcraft->wireDiameter, pcraft->currentAndPolarity, pcraft->wfs);
	ret = db_do_update(sql, &nchanges);
	if(ret != SQLITE_OK)
	{
		if(try_update && ret == SQLITE_CONSTRAINT){
            LogTrace("insert craft[%s] failed, try do update craft", pcraft->craftID);
            ret = craft_update(pcraft, 0);
		} else {
			LOG(LOG_LV_ERROR, "insert craft[%s] with sql[%s] error return:%d\n", pcraft->craftID, sql, ret);
			ret = -1;
		}
	}
	else
	{
		LogDebug("insert craft[%s] success, nchanges=%d", pcraft->craftID, nchanges);
	}

	return ret;
}

int craft_update(craft_t* pcraft, int only_taskID)
{
	int ret = 0;
	int nchanges = 0;
	char sql[512] = {0};

	if(only_taskID)
		snprintf(sql, sizeof(sql), "update craft set taskID=%d where craftID=%s", pcraft->taskID, pcraft->craftID);
	else
		snprintf(sql, sizeof(sql), "update craft set taskID=%d, weldLevel=%d, weldMethod='%s', current='%s', voltage='%s', weldSpeed='%s', gasSpeed='%s', heatInput='%s'"
            ", materialBrand='%s', wireDiameter='%s', currentAndPolarity='%s', wfs='%s' where craftID='%s'", pcraft->taskID, pcraft->weldLevel, pcraft->weldMethod
			, pcraft->current, pcraft->voltage, pcraft->weldSpeed, pcraft->gasSpeed, pcraft->heatInput, pcraft->materialBrand, pcraft->wireDiameter
			, pcraft->currentAndPolarity, pcraft->wfs, pcraft->craftID);

	ret = db_do_update(sql, &nchanges);
	if(ret != SQLITE_OK)
	{
		LogWarn("update craft[%s] with sql[%s] error return:%d\n", pcraft->craftID, sql, ret);
		ret = -1;
	}
	else
	{
		LogDebug("update craft[%s] success, nchanges=%d", pcraft->craftID, nchanges);
	}

	return ret;
}

//return: 1:success; 0: 无指定排产计划; -1: failed
int craft_query(craft_t* pcraft)
{
	int ret = 0;
	int size = 0;
	int maxrow = 1;
	char sql[128] = {0};
	char *craft_rows[1]={0};
	char *pdata_row = NULL;
	char *craft_detail[TASK_ITEM_NUM] = {0};

	snprintf(sql, sizeof(sql), "select * from craft where craftID=%s", pcraft->craftID);
	ret = db_do_query(sql, (char **)&craft_rows, &maxrow, 0);
	if(ret != 0)
	{
		LOG(LOG_LV_ERROR, "select craft[%s] info error return:%d", pcraft->craftID, ret);
		return -1;
	}
	if(maxrow == 0)
	{
		LOG(LOG_LV_ERROR, "query not fount craft[%s]", pcraft->craftID);
		return 0;
	}

	pdata_row = craft_rows[0];
	size = split_string_to_array(pdata_row, DB_FIELD_SPLIT, (char**)&craft_detail, TASK_ITEM_NUM);
	if(size == 0)
	{
		db_free(craft_rows[0]);
		LogWarn("split craft info[%s] failed\n", pdata_row);
		return -1;
	}

	pcraft->taskID = atoi(craft_detail[1]);
	pcraft->weldLevel = atoi(craft_detail[2]);
	strncpy(pcraft->weldMethod, craft_detail[3], sizeof(pcraft->weldMethod) - 1);
	strncpy(pcraft->current, craft_detail[4], sizeof(pcraft->current) - 1);
	strncpy(pcraft->voltage, craft_detail[5], sizeof(pcraft->voltage) - 1);
	strncpy(pcraft->weldSpeed, craft_detail[6], sizeof(pcraft->weldSpeed) - 1);
	strncpy(pcraft->gasSpeed, craft_detail[7], sizeof(pcraft->gasSpeed) - 1);
	strncpy(pcraft->heatInput, craft_detail[8], sizeof(pcraft->heatInput) - 1);
	strncpy(pcraft->materialBrand, craft_detail[9], sizeof(pcraft->materialBrand) - 1);
    strncpy(pcraft->wireDiameter, craft_detail[10], sizeof(pcraft->wireDiameter) - 1);
	strncpy(pcraft->currentAndPolarity, craft_detail[11], sizeof(pcraft->currentAndPolarity) - 1);
	strncpy(pcraft->wfs, craft_detail[12], sizeof(pcraft->wfs) - 1);
	db_free(craft_rows[0]);

	return 1;
}

int craft_delete(int craft_id, int delete_all)
{
	int ret = 0;
	int nchanges = 0;
	char sql[128] = {0};

	if(!delete_all)
		snprintf(sql, sizeof(sql), "delete from craft where craftID=%d", craft_id);
	else
		snprintf(sql, sizeof(sql), "delete from craft");
	ret = db_do_update(sql, &nchanges);
	if(ret != SQLITE_OK)
	{
		LOG(LOG_LV_ERROR, "delete craft[%d] with sql[%s] error return:%d\n", craft_id, sql, ret);
		ret = -1;
	}
	else
	{
		LOG(LOG_LV_INFO, "delete craft[%d] success, nchanges=%d", craft_id, nchanges);
	}

	return ret;
}

int craft_query_count()
{
	int ret = 0;
	int cnt = 0;
	int maxrow = 1;
	char sql[128] = {0};
	char query_buff[32] = {0};
	char *data_rows[1]={query_buff};
	char *pcnt = NULL;

	snprintf(sql, sizeof(sql), "select count(craftID) as 'craftCount' from craft");
	ret = db_do_query(sql, (char **)&data_rows, &maxrow, sizeof(query_buff));
	if(ret != 0)
	{
		LOG(LOG_LV_ERROR, "query craft count error return:%d", ret);
		return -1;
	}

	pcnt = data_rows[0];
	cnt = atoi(pcnt);

	return cnt;
}

int craft_query_all(craft_t* pcraftlist, int lisize, int taskID)
{
	int ret = 0;
	int size = 0;
	int maxrow = lisize < MAX_CRAFT_NUM ? lisize : MAX_CRAFT_NUM;
	char sql[128] = {0};
	char *craft_rows[MAX_CRAFT_NUM]={0};
	char *pdata_row = NULL;
	char *craft_detail[CRAFT_ITEM_NUM] = {0};
	craft_t* pcraft = NULL;
    int i = 0;

	if(taskID != -1)
		snprintf(sql, sizeof(sql), "select * from craft where taskID=%d", taskID);
	else
		snprintf(sql, sizeof(sql), "select * from craft");
	ret = db_do_query(sql, (char **)&craft_rows, &maxrow, 0);
	if(ret != 0)
	{
		LOG(LOG_LV_ERROR, "select all craft info error return:%d", ret);
		return -1;
	}
	if(maxrow == 0)
	{
		LOG(LOG_LV_ERROR, "query not found craft");
		return 0;
	}

    for(i = 0; i < maxrow; i++) 
    {
        pdata_row = craft_rows[i];
	    size = split_string_to_array(pdata_row, DB_FIELD_SPLIT, (char**)&craft_detail, CRAFT_ITEM_NUM);
	    if(size == 0)
	    {
			db_free(craft_rows[i]); craft_rows[i] = NULL;
		    LogWarn("split craft info[%s] failed\n", pdata_row);
		    continue;
	    }
        
		pcraft = &pcraftlist[i];
		strncpy(pcraft->craftID, craft_detail[0], sizeof(pcraft->craftID) - 1);
		pcraft->taskID = atoi(craft_detail[1]);
		pcraft->weldLevel = atoi(craft_detail[2]);
		strncpy(pcraft->weldMethod, craft_detail[3], sizeof(pcraft->weldMethod) - 1);
		strncpy(pcraft->current, craft_detail[4], sizeof(pcraft->current) - 1);
		strncpy(pcraft->voltage, craft_detail[5], sizeof(pcraft->voltage) - 1);
		strncpy(pcraft->weldSpeed, craft_detail[6], sizeof(pcraft->weldSpeed) - 1);
		strncpy(pcraft->gasSpeed, craft_detail[7], sizeof(pcraft->gasSpeed) - 1);
		strncpy(pcraft->heatInput, craft_detail[8], sizeof(pcraft->heatInput) - 1);
		strncpy(pcraft->materialBrand, craft_detail[9], sizeof(pcraft->materialBrand) - 1);
        strncpy(pcraft->wireDiameter, craft_detail[10], sizeof(pcraft->wireDiameter) - 1);
		strncpy(pcraft->currentAndPolarity, craft_detail[11], sizeof(pcraft->currentAndPolarity) - 1);
		strncpy(pcraft->wfs, craft_detail[12], sizeof(pcraft->wfs) - 1);

		db_free(craft_rows[i]); craft_rows[i] = NULL;
    }
	
	return maxrow;
}
#endif
