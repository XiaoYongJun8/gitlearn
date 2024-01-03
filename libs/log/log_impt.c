/********************************************************************************
 *                        Copyright (C), 2020, Suxin Inc.
 ********************************************************************************
 * File:           log_impt.c
 * Description:    重要日志统一输出功能模块
 * Author:         zhums
 * E-Mail:         zhums@linygroup.com
 * Version:        1.0
 * Date:           2020-12-30
 * IDE:            Source Insight 4.0
 *
 * History:
 * No.  Date            Author    Modification
 * 1    2020-12-30      zhums     Created file
********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "log_impt.h"

app_log_t log_impt;

//example:
//int main(void)
//{
//	log_impt_init();
//
//	log_impt("tlog", "test important log");
//
//	return 0;
//}

int log_impt_init()
{
	int ret = 0;
	app_log_t* plog_tmp = NULL;

	plog_tmp = g_plog;
	ret = log_open_ex(&log_impt, IMPT_LOG_FILE, IMPT_LOG_LEVEL_FILE, IMPT_LOG_MAX_SIZE, LOG_LV_WARN, LK_PROCESS, LOGLV_MODE_LINEAR);
	g_plog = plog_tmp;	//恢复原app_log_t日志对象

	return ret;
}

/********************************************************************************
 * Function:       log_write_impt
 * Description:    写日志（可以指定日志前缀内容，可以用来记录程序名称等）
 * Prototype:      int log_write_impt(app_log_t* plog, int level, char* file, int line, char* prefix, char *fmt, ...)
 * Param:          app_log_t* plog   - 日志对象
 * Param:          int level         - 日志级别
 * Param:          char* file        - 日志所在文件名
 * Param:          int line          - 日志所在行数
 * Param:          char* prefix      - 日志前缀
 * Param:          char *fmt         - 日志格式化字符串
 * Param:          ...               - 可变参数部分
 * Return:         0: success; 1: failed
********************************************************************************/
int log_write_impt(app_log_t* plog, int level, char* file, int line, char* prefix, char *fmt, ...)
{
    char buf[MAX_LOG_STR_LEN] = {0};
	va_list args;
    int pos = 0;
    int default_level = 0;
    int iret = 0;

    /* 读取默认的日志级别 */
	default_level = get_log_level(plog);
    /* 根据日志级别决定是否写日志 */
    if(level > default_level) {
        return 0;
    }

	//格式化日志内容
	pos += get_log_day_time(buf);
	pos += sprintf(buf + pos, "[%s:%d %s] ", file, line, levelStr[level]);
	if(prefix)
	{
		pos += sprintf(buf + pos, "[%s] ", prefix);
	}
	va_start(args, fmt);
	pos += vsnprintf(buf + pos, sizeof(buf) - pos - 1, fmt, args);
	va_end(args);

	if(pos >= MAX_LOG_STR_LEN) {
		buf[MAX_LOG_STR_LEN - 1] = '\0';
		pos = MAX_LOG_STR_LEN - 1;
	}

    iret = log_write_inner(plog, buf, pos);

    return iret;
}


