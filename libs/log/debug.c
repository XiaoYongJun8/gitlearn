/***************************************************************************
 * Copyright (C), 2016-2026, All rights reserved.
 * File        :  debug.c
 * Decription  :  debug
 * Author      :  zhums
 * Version     :  v1.0
 * Date        :  2019/03/25
 * Histroy	   :
 **************************************************************************/

/***********************************************************
* INCLUDES
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include "debug.h"

/***********************************************************
* MACROS
*/
#define DBG_LEVEL_STR_LEN	10

#define MAX_DBG_PRINT_BYTES 512


struct app_debug {
    int level_fd;
    char *level_file;
    pthread_mutex_t mutex_lock;
};

struct app_debug debug;
    

/*********************************************************************
* @fn	   dbg_open
*
* @brief   创建调试级别文件
*
* @param   app_debug[in]: debug结构体
*		   level: 默认调试级别
*
* @return  -1: 失败  0: 成功
*/
int dbg_open(char *level_file, int level)
{
    int ret = -1;
	int has_log_level = 1;
	int level_in_file = LY_DEBUG_OFF;
    char buf[DBG_LEVEL_STR_LEN];
    unsigned char len = 0;

    debug.level_file = level_file;
    debug.level_fd = open(debug.level_file, O_RDWR|O_CREAT, 0644);
	if(debug.level_fd < 0) {
		printf("open level_file failed\n");
		return -1;
	}

    ret = lseek(debug.level_fd, 0, SEEK_SET);
    if(-1 != ret )
	{
	    ret = read(debug.level_fd, buf,sizeof(buf));
		if(ret == 0)
		{
			has_log_level = 0;
		}
		else
		{
			buf[ret]='\0';
		   	level_in_file = atoi(buf);
            if(LY_DEBUG_OFF > level_in_file || level_in_file > LY_DEBUG_ALL)
                has_log_level = 0; //错误的日志级别，需要写入level到文件中
		}
    }

	if(has_log_level == 0) {
        /* 写默认的日志级别到日志级别文件 */
        len = sprintf(buf, "%d", level);
        ret = write(debug.level_fd, buf, len);
        if(ret != len) {
            printf("write debug level failed \n");
            return -1;
        }
    }

    pthread_mutex_init(&debug.mutex_lock, NULL);

    return 0;
}



/*********************************************************************
* @fn	   dbg_close
*
* @brief   关闭调试
*
* @param   void
*
* @return  -1: 失败  0: 成功
*/
void dbg_close(void)
{
    close(debug.level_fd);
    debug.level_fd = -1;
    pthread_mutex_destroy(&debug.mutex_lock);
}



/*********************************************************************
* @fn	   dbg_get_level
*
* @brief   从调试级别文件中读取调试级别
*
* @param   void
*
* @return  level: 调试级别
*/
int dbg_get_level(int fd)
{
    char level[DBG_LEVEL_STR_LEN];
    int ret = -1;
    int default_level;

	ret = lseek(fd, 0, SEEK_SET);
    if(-1 == ret ){
        printf("lseek is error");
        return -1;
    }

    ret = read(fd, level, sizeof(level));
    if((ret == -1) || (ret == 0)) {
        printf("debug_level_lseek read failed");
        return -1;
    }

	level[ret]='\0';

   	default_level = atoi(level);

    if(default_level > LY_DEBUG_TRACE) {
        default_level = LY_DEBUG_TRACE;
    } else if(default_level < LY_DEBUG_OFF) {
        default_level = LY_DEBUG_OFF;
    }
	
    return default_level;	
}

/*********************************************************************
* @fn      dbg_print 
*
* @brief   调试printf函数
*
* @param   level: 打印等级 
*		   file : 文件名
*		   function: 函数名
*		   line : 行号
*		   fmt: 可变参数
*
* @return  void
*/
void dbg_print(int level, char* file, const char * function, int line, char *fmt, ...)
{
	char buf[MAX_DBG_PRINT_BYTES] = {0};
	va_list args;
	int len=0, pos=0,num;
    time_t rsec;
    struct tm t;
    int default_level = 0;

    pthread_mutex_lock(&debug.mutex_lock);
    default_level = dbg_get_level(debug.level_fd);
    pthread_mutex_unlock(&debug.mutex_lock);

    if(level > default_level) {    
        return;
    }

	tzset();
    rsec = time(NULL);
    localtime_r(&rsec, &t);
    len = sprintf(buf, "%4d-%d-%d %02d:%02d:%02d ",t.tm_year+1900, 
    											     t.tm_mon+1, 
    											     t.tm_mday, 
    											     t.tm_hour, 
    											     t.tm_min, 
    											     t.tm_sec);

	if (len < 0) {
		len = 0;
	}
	pos += len;

	num = sprintf(buf + pos, "[%s:%d] ",file, line);
	pos += num;

	va_start(args, fmt);
	pos += vsnprintf(buf + pos, sizeof(buf) - pos - 1, fmt, args);
	va_end(args);

    //日志尾添加\n
	if(pos + 2 >= MAX_DBG_PRINT_BYTES) {
		pos = MAX_DBG_PRINT_BYTES - 3;
	}
	if(buf[pos - 1] == '\n') {
		buf[pos - 1] = '\0';
        pos--;
	}

	printf("%s\n", buf);
}

