/***************************************************************************
 * Copyright (C), 2016-2026, , All rights reserved.
 * File        :  common.h
 * Decription  :  common interface
 * Author      :  sunjs
 * Version     :  v1.0
 * Date        :  2018/12/17
 * Histroy	   :
 **************************************************************************/
#ifndef __COMMON_H__
#define __COMMON_H__

#include "util.h"
#include "at_util.h"

/****************************** MACRO ******************************/
#define NET_4G_STATUS_FILE        "/home/root/status/net_4g_status"    /* 存储当前4g网络状态，未连接或连接 */
#define RSSI_4G_FILE              "/home/root/status/rssi_4g"
#define WS_CONN_STATUS_FILE       "/home/root/status/ws_conn_status"   /* 存储websocket连接状态 */
#define HW_MODE_FILE              "/home/root/test/hw_mode"            /* 存储硬件模式 */
#define NET_MODE_FILE             "/home/root/test/switch/test_switch_4g_wlan"  /* 存储上网模式 */
#define PROJECT_VERSION_FILE      "/etc/suxin-project-version"    /* 存储硬件版本以及软件版本 */
#define EEPROM_DEV "/sys/devices/platform/ocp/44e0b000.i2c/i2c-0/0-0050/eeprom"

/* connect status */
#define NET_4G_CONNECTED		1
#define NET_4G_DISCONNECTED		0
#define WS_CONNECTED            1
#define WS_DISCONNECTED         0

/* 4G信号强度 */
// 14 <= csq <= 31, rssi >= -85dbm 
#define SIGNAL_STRENGTH_GREAT     4  
// 9 <= csq < 14, -95dbm <= rssi < -85dbm
#define SIGNAL_STRENGTH_GOOD      3
// 4 <= csq < 9,  -105dbm <= rssi < -95dbm
#define SIGNAL_STRENGTH_MODERATE  2
// 2 <= csq < 4, -109dbm <= rssi < -105dbm
#define SIGNAL_STRENGTH_POOR      1
// csq < 2, csq = 99
#define SIGNAL_STRENGTH_NONE_OR_UNKNOWN 0 

/* hardware mode */
#define PRODUCTION_MODE		    0
#define FACTORY_MODE     		1

/* net mode */
#define NET_MODE_4G		        0
#define NET_MODE_LAN     		1
#define NET_CHECK_LAN     		2
#define NET_SWITCHED	        1
#define NET_UNSWITCHED     		0
#define NET_MODE_SPLIT			","


#define MAX_TABLE_NAME_LEN 100
#define MAX_MSG_NAME_LEN 100

typedef struct net_mode_info_st
{
	int net_mode;				//上网模式 0: 4G上网模式　1: WLAN上网模式
	int net_switch_state;		//上网模式切换状态 0: 上网模式未切换　1: 上网模式已切换
	char net_gateway[32];		//WLAN上网模式时的网关地址
	char net_nameserver[32];	//WLAN上网模式时的DNS服务器地址
}net_mode_info_t;

int get_net_4g_status(void);
int set_net_4g_status(int status);
int get_4g_rssi(void);
int set_4g_rssi(int rssi);
int get_ws_conn_status(void);
int set_ws_conn_status(int status);
int get_hw_mode(void);
int set_hw_mode(int mode);
int get_device_id(char device_id[10]);

void dump_hex_ex(int level, char* file, int line, char* msg, uint8_t* data, int len);
void dump_hex(char* msg, uint8_t* data, int len);
char* hex2string(uint8_t* data, int len, char* strbuff, int maxlen);

#endif
