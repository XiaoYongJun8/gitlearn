/***************************************************************************
 * Copyright (C), 2016-2026, , All rights reserved.
 * File        :  watchdog.c
 * Decription  :  watchdog
 * Author      :  shiwb
 * Version     :  v1.0
 * Date        :  2022/02/11
 * Histroy	   :
 **************************************************************************/

/***********************************************************
* INCLUDES
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/watchdog.h>
#include <pthread.h>
#include <unistd.h>

#include "util.h"
#include "watchdog.h"
#include "debug_and_log.h"

/***********************************************************
* MACROS
*/
/* device file name */
#define WATCHDOG           "/dev/watchdog"
#define TIMEOUT            1

/*********************************************************************
* GLOBAL VARIABLES
*/
/* file descriptor */
static int fd_watchdog = -1;
static int watchdog_opened = 0; //whether watchdog is opened
static int feed_intv = -1;


/*********************************************************************
* @fn	      watchdog_open_ex
* @brief      打开看门狗，有内核日志
* @param      timeout[in]:　看门狗超时时间
* @param      proc_name[in]: 进程名称
* @param      max_kernel_log_size: 内核日志最大大小
* @return     0: 成功 -1: 失败
* @history:
**********************************************************************/
int watchdog_open_ex( int timeout, char* proc_name, const int max_kernel_log_size)
{
#if 0
	int ret =-1;
	static un_watchdog_args_t dog_args={0};
	static int kernel_log_size = 0;

	fd_watchdog = open(WATCHDOG, O_WRONLY);
	if(-1 == fd_watchdog){
		int err = errno;
        DBG_LOG_ERROR("failed to open %s, errno: %d, %s\n", WATCHDOG, err, strerror(err));
		return fd_watchdog;
	}

	if(timeout < TIMEOUT) {
        DBG_LOG_ERROR("watchdog: set timeout is not vailed\n");
		return -1;
	}
	ret = ioctl(fd_watchdog, WDIOC_SETTIMEOUT, &timeout);
	if(-1 == ret ){
        DBG_LOG_ERROR("watchdog set time failed\n");
		return -1;
	}

	if(proc_name) {
		memset(&dog_args, 0, sizeof(un_watchdog_args_t));
		strncpy(dog_args.proc_name, proc_name, PROC_NAME_LEN);
		ret = ioctl(fd_watchdog, WDIOC_SETPROCNAME, &dog_args);
		if(-1 == ret ){
            DBG_LOG_ERROR("watchdog set proc_name[%s] failed\n", proc_name);
			return -1;
		}
	}
	if(max_kernel_log_size>0)
	{
		kernel_log_size = max_kernel_log_size;
		ret = ioctl(fd_watchdog, WDIOC_SETLOGMAXSIZE, &kernel_log_size);
		if(-1 == ret ){
            DBG_LOG_ERROR("watchdog set max kernel log size to[%d] failed\n", kernel_log_size);
			return -1;
		}
	}
	watchdog_opened = 1;
	return 0;
#else
	return 0;
	//return watchdog_open(timeout);
#endif
}

/*********************************************************************
* @fn	      watchdog_open
* @brief      打开看门狗
* @param      timeout[in]:　看门狗超时时间
* @return     0: 成功 -1: 失败
* @history:
**********************************************************************/
int watchdog_open(int timeout)
{
    int ret =-1;

	fd_watchdog = open(WATCHDOG, O_WRONLY);
	if(fd_watchdog == -1){
        DBG_LOG_ERROR("failed to open %s, errno: %d, %s\n", WATCHDOG, errno, strerror(errno));
		return -1;
	}

	if(timeout < TIMEOUT) {
        DBG_LOG_ERROR("watchdog: set timeout is not vailed\n");
		return -1;
	}

	ret = ioctl(fd_watchdog, WDIOC_SETTIMEOUT, &timeout);
	if(ret == -1){
        DBG_LOG_ERROR("watchdog set time failed\n");
		return -1;
	}

	watchdog_opened = 1;
	return 0;
}


/*********************************************************************
* @fn	      watchdog_close
* @brief      关闭看门狗
* @param      void
* @return     0: 成功 -1: 失败
* @history:
**********************************************************************/
int watchdog_close(void)
{
	int ret = 0;

	if (watchdog_opened == 0) {
		return 0;
	}

	ret = write(fd_watchdog, "V", 1);
	if(1 != ret) {
        DBG_LOG_ERROR("failed to close %s\n", WATCHDOG);
		return ret;
	}
	ret = close(fd_watchdog);
	watchdog_opened = 0;
	return ret;
}


/*********************************************************************
* @fn	      watchdog_feed
* @brief      喂狗
* @param      void
* @return     0: 成功 -1: 失败
* @history:
**********************************************************************/
int watchdog_feed(void)
{
	unsigned char food = 0;
	ssize_t eaten=0;

	if (!watchdog_opened) {
		return 0;
	}

	eaten = write(fd_watchdog, &food, 1);
	if(1 != eaten ) {
        DBG_LOG_ERROR("failed feeded watchdog\n");
		return -1;
    } else {
        return 0;
    }
}

/*********************************************************************
* @fn	   watchdog_feeding_time
*
* @brief   feed watchdog time
*
* @param   timeout : time
*
* @return  -1 :error  0:success
*/
int watchdog_feeding_time(int timeout)
{
	int ret =-1;

	if(timeout < TIMEOUT) {
        DBG_LOG_ERROR("watchdog: set timeout is not vailed\n");
		return -1;
	}
	ret = ioctl(fd_watchdog, WDIOC_SETTIMEOUT, &timeout);
	if(-1 != ret ){
		//log
		return 0;
	}
    DBG_LOG_ERROR("watchdog set time failed\n");
	return ret;
}

/*********************************************************************
* @fn	      watchdog_thread_fn
* @brief      喂狗线程，周期性喂狗
* @param      arg[in]: 喂狗周期
* @return     0: 成功 -1: 失败
* @history:
**********************************************************************/
void *watchdog_thread_fn(void *arg)
{
	int intv = -1;

	if (NULL == arg) {
        DBG_LOG_ERROR("Invalid params!\n");
		return (void *)-1;
	}

	intv = *((int *)arg);

    DBG_LOG_MUST("watchdog feed intv: %d!\n", intv);

	while (1) {
		watchdog_feed();
		safe_sleep(intv, 0);
	}

	return (void *)0;
}


/*********************************************************************
* @fn	      create_wdog_feed_thread
* @brief      创建喂狗线程
* @param      intv[in]: 喂狗周期
* @return     0: 成功 -1: 失败
* @history:
**********************************************************************/
int create_wdog_feed_thread(int intv)
{
	pthread_t wdog_tid;
	pthread_attr_t wdog_attr;
	int ret;

	feed_intv = intv;

    DBG_AND_LOG(LY_DEBUG_TRACE, LOG_LV_TRACE, "create watchodog thread!\n");
	pthread_attr_init(&wdog_attr);
	pthread_attr_setdetachstate(&wdog_attr, PTHREAD_CREATE_DETACHED);
	ret = pthread_create(&wdog_tid,&wdog_attr,watchdog_thread_fn,&feed_intv);
	if (ret) {
        DBG_LOG_ERROR("create watchdog thread error: %s\n", strerror(errno));
		return -1;
	}

	return 0;
}