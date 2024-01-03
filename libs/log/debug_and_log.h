/***************************************************************************
 * Copyright (C), All rights reserved.
 * File        :  debug_and_log.h
 * Decription  :  debug与log结合到一起
 * Author      :  shiwb
 * Version     :  v1.0
 * Date        :  2018/9/28
 * Histroy	   :
 **************************************************************************/
#ifndef __DEBUG_AND_LOG_H__
#define __DEBUG_AND_LOG_H__

/************************************** INCLUDE *************************************/
#include <errno.h>
#include "debug.h"
#include "log.h"
#include "log_impt.h"
#include "log_test.h"


#define DBG_AND_LOG(debug_level, log_level, ...) \
do{ \
	dbg_print(debug_level, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__); \
	log_write(g_plog, log_level, LOG_FLAG, __VA_ARGS__); \
}while(0)

#define DBG_LOG_DEBUG(fmt, args...) \
do{ \
	dbg_print(LY_DEBUG_DEBUG, __FILE__, __FUNCTION__, __LINE__, fmt, ##args); \
	log_write(g_plog, LOG_LV_DEBUG, LOG_FLAG, fmt, ##args); \
}while(0)

#define DBG_LOG_INFO(fmt, args...) \
do{ \
	dbg_print(LY_DEBUG_INFO, __FILE__, __FUNCTION__, __LINE__, fmt, ##args); \
	log_write(g_plog, LOG_LV_INFO, LOG_FLAG, fmt, ##args); \
}while(0)

#define DBG_LOG_WARN(fmt, args...) \
do{ \
	dbg_print(LY_DEBUG_WARN, __FILE__, __FUNCTION__, __LINE__, fmt, ##args); \
	log_write(g_plog, LOG_LV_WARN, LOG_FLAG, fmt, ##args); \
}while(0)

#define DBG_LOG_ERROR(fmt, args...) \
do{ \
	dbg_print(LY_DEBUG_ERROR, __FILE__, __FUNCTION__, __LINE__, fmt, ##args); \
	log_write(g_plog, LOG_LV_ERROR, LOG_FLAG, fmt, ##args); \
}while(0)

#define DBG_LOG_MUST(fmt, args...) \
do{ \
	dbg_print(LY_DEBUG_MUST, __FILE__, __FUNCTION__, __LINE__, fmt, ##args); \
	log_write(g_plog, LOG_LV_MUST, LOG_FLAG, fmt, ##args); \
}while(0)
#endif
