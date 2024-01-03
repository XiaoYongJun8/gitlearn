/********************************************************************************
 *                        Copyright (C), 2020, Suxin Inc.
 ********************************************************************************
 * File:           log_test.c
 * Description:    输出单独测试日志内容到日志文件中
 * Author:         zhums
 * E-Mail:         zhums@linygroup.com
 * Version:        1.0
 * Date:           2020-12-31
 * IDE:            Source Insight 4.0
 *
 * History:
 * No.  Date            Author    Modification
 * 1    2020-12-31      zhums     Created file
********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "log_test.h"

app_log_t log_test;

//example:
//int main(void)
//{
//	log_test_init("app_test.log", "/home/root/debug/log_app_test", 1024*1024, LOG_LV_INFO);
//
//	log_test(LOG_LV_INFO, "test print log");
//
//	return 0;
//}

int log_test_init(char* logfile, char* levelfile, int maxsize, int deflevel)
{
	int ret = 0;
	app_log_t* plog_tmp = NULL;

	plog_tmp = g_plog;
	ret = log_open_ex(&log_test, logfile, levelfile, maxsize, deflevel, LK_THREAD, LOGLV_MODE_LINEAR);
	g_plog = plog_tmp;	//恢复原app_log_t日志对象

	return ret;
}

/********************************************************************************
 * Function:       log_write_test
 * Description:    写测试日志（可以指定日志中是否带有时间戳，日志位置信息等）
 * Prototype:      int log_write_test(app_log_t* plog, int level, char* file, int line, char* prefix, char *fmt, ...)
 * Param:          app_log_t* plog   - 日志对象
 * Param:          int level         - 日志级别
 * Param:          char* file        - 日志所在文件名
 * Param:          int line          - 日志所在行数
 * Param:          int withInfo      - 是否携带时间戳及日志位置等信息
 * Param:          char *fmt         - 日志格式化字符串
 * Param:          ...               - 可变参数部分
 * Return:         0: success; 1: failed
********************************************************************************/
int log_write_test(app_log_t* plog, int level, char* file, int line, int withInfo, char *fmt, ...)
{
    char buf[MAX_LOG_STR_LEN] = {0};
	va_list args;
    int pos = 0;
    int default_level = 0;
    int iret = 0;

	if(1 != get_test_switch(TESTLOG))
	{
		return 0;
	}

    /* 读取默认的日志级别 */
	default_level = get_log_level(plog);
    /* 根据日志级别决定是否写日志 */
    if(level > default_level) {
        return 0;
    }

	//格式化日志内容
	if(withInfo)
	{
		pos += get_log_day_time(buf);
		pos += sprintf(buf + pos, "[%s:%d %s] ", file, line, levelStr[level]);
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

void get_local_time(void)
{
	char nowtime[24] = {0};
	struct tm localtimestamp;
	time_t time_tmp = time(NULL);

	localtime_r(&time_tmp, &localtimestamp);
	strftime(nowtime, sizeof(nowtime), "[%Y%m%d%H%M%S]", &localtimestamp);
	log_test(LOG_LV_INFO, WITH_INFO_NOT, nowtime);
}

int get_test_switch(enum SWITCH_TYPE test_switch_type)
{
	static int test_switch_fd = -1;
	int ret = -1;
	int test_switch = 0;
	char ar_switch[LOG_LEVEL_STR_LEN] = {0};

	if (test_switch_fd < 0)
	{
		test_switch_fd = open("/home/root/test/switch/test_switch_test_log", O_RDWR|O_CREAT, 0644);
		if(test_switch_fd < 0)
		{
			perror("open failed!");
			return 0;
		}
	}

	ret = lseek(test_switch_fd, 0, SEEK_SET);
    if(-1 == ret )
	{
        perror("lseek is error");
        return 0;
    }

    ret = read(test_switch_fd,ar_switch,sizeof(ar_switch));
	if(-1 == ret)
	{
        perror("read is error");
        return 0;
	}
	else if(0 == ret)
	{
		ret = lseek(test_switch_fd, 0, SEEK_SET);
	    if(-1 == ret )
		{
	        perror("lseek is error");
	        return 0;
	    }
		ret =write(test_switch_fd,"0\n",sizeof("0\n"));
		if(-1 == ret )
		{
	        perror("write is error");
	        return 0;
	    }
		test_switch =0;
	}
	else
	{
   		test_switch = atoi(ar_switch);
	}

	return test_switch;
}

