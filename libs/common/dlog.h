/********************************************************************************
 *                    Copyright (C), 2021-2217, SuxinIOT Inc.
 ********************************************************************************
 * File:           dlog.h
 * Description:    common debug log util
 * Author:         zhums
 * E-Mail:         zhums@linygroup.com
 * Version:        1.0
 * Date:           2021-12-25
 * IDE:            Source Insight 4.0
 *
 * History:
 * No.  Date            Author    Modification
 * 1    2021-12-25      zhums     Created file
********************************************************************************/

#ifndef __DLOG_H__
#define __DLOG_H__
#include <stdarg.h>
void logmsg(char* file, int line, int logLevel, char* fmt, ...)
{
    va_list ap;
    char buf[1024] = {0};
    static char strLevel[][8] = {"", "ERROR", "WARN", "INFO", "DEBUG"};

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    printf("[%s:%-4d %5s] %s\n", file, line, strLevel[logLevel], buf);
}

// #define dlog(fmt, args...)		logmsg(__FILE__, __LINE__, 4, fmt, ##args)
// #define ddebug(fmt, args...)	logmsg(__FILE__, __LINE__, 4, fmt, ##args)
// #define dinfo(fmt, args...)		logmsg(__FILE__, __LINE__, 3, fmt, ##args)
// #define dwarn(fmt, args...)		logmsg(__FILE__, __LINE__, 2, fmt, ##args)
// #define derror(fmt, args...)	logmsg(__FILE__, __LINE__, 1, fmt, ##args)

#include "log.h"
#if 0
#define dlog(fmt, args...)		log_write(g_plog, LOG_LV_DEBUG, LOG_FLAG, fmt, ##args)
#define ddebug(fmt, args...)	log_write(g_plog, LOG_LV_DEBUG, LOG_FLAG, fmt, ##args)
#define dinfo(fmt, args...)		log_write(g_plog, LOG_LV_INFO,  LOG_FLAG, fmt, ##args)
#define dwarn(fmt, args...)		log_write(g_plog, LOG_LV_WARN,  LOG_FLAG, fmt, ##args)
#define derror(fmt, args...)	log_write(g_plog, LOG_LV_ERROR, LOG_FLAG, fmt, ##args)
#else
#define dlog(fmt, args...)		
#define ddebug(fmt, args...)	
#define dinfo(fmt, args...)		
#define dwarn(fmt, args...)		
#define derror(fmt, args...)	
#endif 

#endif //__DLOG_H__
