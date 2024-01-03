/***************************************************************************
 * Copyright (C), 2016-2026, , All rights reserved.
 * File        :  watchdog.h
 * Decription  : 
 * Author      :  shiwb
 * Version     :  v1.0
 * Date        :  2022/02/11
 * Histroy	   :
 **************************************************************************/

#ifndef __SOFT_DOG_H__
#define __SOFT_DOG_H__


int watchdog_open(int timeout);
int watchdog_open_ex(int timeout, char* proc_name, const int max_kernel_log_size);
int watchdog_close(void);
// int watchdog_feed(void);
// int watchdog_feeding_time(int timeout);
// int watchdog_is_opened(void);
int create_wdog_feed_thread(int intv);

#endif