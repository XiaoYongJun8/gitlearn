/********************************************************************************
 *                        Copyright (C), 2020, Suxin Inc.
 ********************************************************************************
 * File:           log_test.h
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

#ifndef __LOG_TEST_H__
#define __LOG_TEST_H__

/************************************** INCLUDE *************************************/
#include <stdio.h>
#include "log.h"

#define WITH_INFO_NOT		0
#define WITH_INFO			1

enum SWITCH_TYPE
{
	TESTLOG,
};

extern app_log_t log_test;

int log_test_init(char* logfile, char* levelfile, int maxsize, int deflevel);
#define log_test(level, withInfo, fmt, args...) log_write_test(&log_test, level, LOG_FLAG, withInfo, fmt, ##args)
void get_local_time(void);
int get_test_switch(enum SWITCH_TYPE test_switch_type);

#endif //__LOG_TEST_H__