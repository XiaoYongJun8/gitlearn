/********************************************************************************
 *                        Copyright (C), 2020, Suxin Inc.
 ********************************************************************************
 * File:           log_impt.h
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

#ifndef __LOG_IMPT_H__
#define __LOG_IMPT_H__

/************************************** INCLUDE *************************************/
#include <stdio.h>
#include "log.h"

#define IMPT_LOG_LEVEL_FILE				"/home/root/debug/log_impt_log"
#define IMPT_DEBUG_LEVEL_FILE			"/home/root/debug/debug_impt_log"
#define IMPT_LOG_FILE					"/home/root/log/important.log"
#define IMPT_LOG_MAX_SIZE         		1024*1024

extern app_log_t log_impt;

int log_impt_init();
int log_write_impt(app_log_t* plog, int level, char* file, int line, char* prefix, char *fmt, ...);
#define log_impt(proc_name, fmt, args...) log_write_impt(&log_impt, LOG_LV_MUST, LOG_FLAG, proc_name, fmt, ##args)

#endif //__LOG_IMPT_H__
