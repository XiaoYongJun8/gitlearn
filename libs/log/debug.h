/***************************************************************************
 * Copyright (C), 2016-2026, All rights reserved.
 * File        :  debug.h
 * Decription  :  debug
 * Author      :  zhums
 * Version     :  v1.0
 * Date        :  2019/03/25
 * Histroy	   :
 **************************************************************************/
 
#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdio.h>
#include <stdlib.h>

/* 日志级别 */
enum LY_DEBUG_LEVEL {
    LY_DEBUG_OFF = 0,
    LY_DEBUG_MUST = 0,
    LY_DEBUG_IMPT,
    LY_DEBUG_ERROR,
    LY_DEBUG_WARN,
    LY_DEBUG_INFO,
    LY_DEBUG_DEBUG,
    LY_DEBUG_TRACE,
    LY_DEBUG_ALL
};


void dbg_close(void);
void dbg_print(int level, char* file, const char * function, int line, char *fmt, ...);
int dbg_open(char *level_file, int level);

/* DBG_PRINT宏 */
#define   DBG_OPEN(level_file, level)      dbg_open(level_file, level) 
#define   DBG_CLOSE()                      dbg_close()
#define   DBG_PRINT(level, args...)        dbg_print(level, __FILE__, __FUNCTION__, __LINE__, ##args) 
#define   DDEBUG(fmt, args...)             dbg_print(LY_DEBUG_DEBUG, __FILE__, NULL, __LINE__, fmt, ##args) 
#define   DINFO(fmt, args...)              dbg_print(LY_DEBUG_INFO, __FILE__, NULL, __LINE__, fmt, ##args) 
#define   DWARN(fmt, args...)              dbg_print(LY_DEBUG_WARN, __FILE__, NULL, __LINE__, fmt, ##args) 
#define   DERROR(fmt, args...)             dbg_print(LY_DEBUG_ERROR, __FILE__, NULL, __LINE__, fmt, ##args) 


#endif
