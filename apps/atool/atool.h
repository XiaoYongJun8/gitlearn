/*******************************************************************************
 * @             Copyright (c) 2023 LinyPower, All Rights Reserved.
 * @*****************************************************************************
 * @FilePath     : atool.h
 * @Descripttion : 
 * @Author       : zhums
 * @E-Mail       : zhums@linygroup.cn
 * @Version      : v1.0.0
 * @Date         : 2023-11-26 22:59:00
*******************************************************************************/
#ifndef __ATOOL_H__
#define __ATOOL_H__
#include <stdint.h>

#define MAX_MSG_DATA_LEN	8000

#define INT_FILE_NAME 		"/usr/local/etc/fgtool.conf"
#define ATOOL_VERSION 		"AToolV1.0.0"

#define UINT8  unsigned char
#define UINT16 unsigned short
#define UINT32 unsigned int

#define OFFSET(type,field) ((uintptr_t)(&((type *)0)->field))
#define ST_HEAD(type,var,field) (((var)->field) - OFFSET(type,field))

#define AtoolLog(fmt, args...)      printf("[ATOOL]" fmt, ##args)
#define AtoolLogLn(fmt, args...)    printf("[ATOOL]" fmt "\n", ##args)




typedef struct gconfig
{
    int 	logLevel;						//
    char 	logPath[256];					//
    char 	logname[256];					//
}gconfig_t;

#endif //__ATOOL_H__