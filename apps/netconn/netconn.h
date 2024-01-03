/***************************************************************************
 * Copyright (C), 2016-2026, , All rights reserved.
 * File        :  connect_internet.h
 * Decription  :  联网
 * Author      :  zhums
 * Version     :  v1.0
 * Histroy	   :  2018/8/28 created by liuch
 **************************************************************************/

#ifndef __CONNECT_INTERNET_H__
#define __CONNECT_INTERNET_H__

/**
 * 定义联网类
 * @name: 联网对象名称，如ethernet、4G、wifi等
 * @network_init: 初始化驱动或网络参数
 * @dial: 拨号函数
 * @disconnect_internet: 断开网络
 * @check_internet: 检测是否已经联网
 */
typedef struct network_ops {
    char* name;
    char* errstr;

    void (*network_init)(void);
    int (*network_connect)(void);
    int (*network_disconnect)(void);
    int (*network_status)(void);
}network_ops_t;

#endif

