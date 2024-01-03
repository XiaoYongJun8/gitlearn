/***************************************************************************
 * Copyright (C), All rights reserved.
 * File        :  log.h
 * Decription  :  log
 * Author      :  zhums
 * Version     :  v1.0
 * Date        :  2018/9/28
 * Histroy	   :
 **************************************************************************/
#ifndef __LOG_H__
#define __LOG_H__

/************************************** INCLUDE *************************************/
#include <stdio.h>
#include <pthread.h>

#define MAX_LOG_STR_LEN     4096
#define LOG_LEVEL_STR_LEN	10
#define MAX_FILE_NAME_LEN   256

#define LOG_FLAG 		__FILE__,__LINE__
#define LOG_FLAG_FUNC 	__FILE__,__func__,__LINE__

/* 日志级别 */
enum LY_LOG_LEVEL {
    LOG_LV_OFF   = 0,
    LOG_LV_MUST = 0,
    LOG_LV_IMPT = 1,
    LOG_LV_ERROR = 2,
    LOG_LV_WARN = 3,
    LOG_LV_INFO = 4,
    LOG_LV_DEBUG = 5,
    LOG_LV_TRACE = 6,
    LOG_LV_EMSHMI,          //EMS HMI通信日志
    LOG_LV_BMS,             //BMS相关调试日志
    LOG_LV_PCS,             //PCS相关调试日志
    LOG_LV_EMANA,           //能量管理相关调试日志
    LOG_LV_USER4,           //用户自定义日志4
    LOG_LV_USER5,           //用户自定义日志5
    LOG_LV_ALL,
};
#define LOG_NONE		-1
enum LOGLV_MODE
{
    LOGLV_MODE_LINEAR = 1,
    LOGLV_MODE_SELECT = 2
};

enum LOCK_MODE
{
	LK_THREAD = 1,
	LK_PROCESS,
};

struct app_log;
typedef int (*pflock)(struct app_log * plog);

/**
 * @fd: log日志文件描述符
 * @file: 日志文件名
 * @log_max_size: 日志最大文件大小
 * @level_file: 日志级别文件
 * @mutex_lock: 多线程互斥锁
 */
typedef struct app_log {
    int                 log_fd;
    int                 level_fd;
    char                log_file[MAX_FILE_NAME_LEN];
    char                level_file[MAX_FILE_NAME_LEN];
    int                 log_max_size;
	pflock              lock;
	pflock              unlock;
    pthread_mutex_t     mutex_lock;
	int                 lock_fd;
    char                lock_file[128];
    
    int					log_level_mode;
    volatile int		write_log_level;
    volatile int		log_level_sets[LOG_LV_ALL + 1];
    char				log_level_str[64];
}app_log_t;


extern app_log_t* g_plog;
extern const char levelStr[][8];
int log_open(app_log_t* plog, char* logfname, char* levelfile, int max_log_size, int level, enum LOGLV_MODE log_level_mode);
int log_open_ex(app_log_t* plog, char* logfname, char* levelfile, int max_log_size, int level, enum LOCK_MODE lk_mode, enum LOGLV_MODE log_level_mode);
void log_close(app_log_t* plog);
int log_write_inner(app_log_t* plog, char *logbuff, int len);
int log_write(app_log_t* plog, int level, char* file, int line, char* fmt, ...);
int log_write_ex(app_log_t* plog, int level, char* file, int line, u_int8_t with_date, u_int8_t newline, char* fmt, ...);
void set_app_log(app_log_t* plog);
void set_log_level_mode(app_log_t* plog, enum LOGLV_MODE lv_mode);
void set_log_level(app_log_t* plog, int loglevel);
void set_log_level_sets(app_log_t* plog, char* lv_sets);
//int get_log_level_ex(app_log_t* plog);
int get_log_level(app_log_t* plog);
int get_log_day_time(char* dst);

#define CUR_LOG_LEVEL               get_log_level(g_plog)
#define LogComm(level, with_date, newline, args...)    log_write_ex(g_plog, level, LOG_FLAG, with_date, newline, ##args)
#define LogCommEx(level, file, line, with_date, newline, args...)    log_write_ex(g_plog, level, file, line, with_date, newline, ##args)

#define LOG(level, fmt, args...)    log_write(g_plog, level, LOG_FLAG, fmt, ##args)
#define LogEMSHMI(fmt, args...)     log_write(g_plog, LOG_LV_EMSHMI, LOG_FLAG, fmt, ##args)
#define LogBMS(fmt, args...)        log_write(g_plog, LOG_LV_BMS, LOG_FLAG, fmt, ##args)
#define LogPCS(fmt, args...)        log_write(g_plog, LOG_LV_PCS, LOG_FLAG, fmt, ##args)
#define LogEMANA(fmt, args...)      log_write(g_plog, LOG_LV_EMANA, LOG_FLAG, fmt, ##args)
#define LogTrace(fmt, args...)      log_write(g_plog, LOG_LV_TRACE, LOG_FLAG, fmt, ##args)
#define LogDebug(fmt, args...)      log_write(g_plog, LOG_LV_DEBUG, LOG_FLAG, fmt, ##args)
#define LogInfo(fmt, args...)       log_write(g_plog, LOG_LV_INFO, LOG_FLAG, fmt, ##args)
#define LogWarn(fmt, args...)       log_write(g_plog, LOG_LV_WARN, LOG_FLAG, fmt, ##args)
#define LogError(fmt, args...)      log_write(g_plog, LOG_LV_ERROR, LOG_FLAG, fmt, ##args)
#define LogMust(fmt, args...)       log_write(g_plog, LOG_LV_MUST, LOG_FLAG, fmt, ##args)

#endif
