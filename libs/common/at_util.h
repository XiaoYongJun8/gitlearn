/***************************************************************************
 * Copyright (C), 2016-2026, , All rights reserved.
 * File        : at_util.h
 * Decription  :
 *
 * Author      :  zhums
 * Version     :  v1.0
 * Date        :  2022/03/31
 * Histroy	   :
 **************************************************************************/


#ifndef __AT_UTIL_H__
#define __AT_UTIL_H__


int at_cmd_open(const char *pathname, int flags);
void at_cmd_close(int fd);
int at_cmd_send(int fd, char* at_cmd, char* final_rsp, char* at_rsp, int maxlen, long timeout_ms);

#endif //__AT_UTIL_H__
