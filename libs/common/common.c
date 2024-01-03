/***************************************************************************
 * Copyright (C), 2016-2026, , All rights reserved.
 * File        : common.c
 * Decription  : common interface
 *
 * Author      : sunjs
 * Version     : v1.0
 * Date        : 2018/12/17
 * History	   :   
 **************************************************************************/
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "define.h"
#include "debug_and_log.h"
#include "util.h"
#include "common.h"

/****************************** MACRO ******************************/
#define MAX_AT_LEN	512

/*********************************************************************
* @fn	      get_net_4g_status
* @brief      获取4G联网状态
* @param      void
* @return     -1: 失败, 0: 未连接， 1: 连接
* @history:
**********************************************************************/
int get_net_4g_status(void)
{
	FILE *fp = NULL;
	int ret = 0;
	char status[2] = {};

	memset(status,0,sizeof(status));

	if (access(NET_4G_STATUS_FILE,F_OK)) {
        DBG_LOG_ERROR("%s doesn't exist!\n", NET_4G_STATUS_FILE);
		return -1;
	} else {
		if (NULL == (fp = fopen(NET_4G_STATUS_FILE,"r"))) {
            DBG_LOG_ERROR("fopen %s fail!\n", NET_4G_STATUS_FILE);
			return -1;
		} else {
			ret = fread(status,sizeof(status),1,fp);
			if (ret != 1) {
                DBG_LOG_ERROR("fread %s fail! %s\n", NET_4G_STATUS_FILE, strerror(errno));
				fclose(fp);
				return -1;
			} else {
				ret = atoi(status);
			}
		}
	}

	fclose(fp);
	fp = NULL;
	return ret;
}


/*********************************************************************
* @fn	      set_conn_net_status
* @brief      设置4G联网状态
* @param      status[in]: 0是未联网，1是已联网
* @return     0: 成功 -1: 失败
* @history:
**********************************************************************/
int set_net_4g_status(int status)
{
	FILE *fp = NULL;
	int ret = 0;
	char *status_str = NULL;

	if ((status != NET_4G_CONNECTED) && (status != NET_4G_DISCONNECTED)) {
        DBG_LOG_ERROR("Invalid params! %c\n", status);
		return -1;
	}

	status_str = (status == NET_4G_CONNECTED) ? "1" : "0";

	fp = fopen(NET_4G_STATUS_FILE,"w+");
	if (fp == NULL) {
        DBG_LOG_ERROR("fopen %s fail! %s\n", NET_4G_STATUS_FILE, strerror(errno));
		return -1;
	}

	if (fwrite(status_str,2,1,fp) != 1) {
        DBG_LOG_ERROR("fwrite %s fail! %s\n", NET_4G_STATUS_FILE, strerror(errno));
		fclose(fp);
		return -1;
	}

	fclose(fp);
	fp = NULL;
	return ret;
}


/*********************************************************************
* @fn	      get_4g_rssi
* @brief      获取4G信号强度
* @param      void
* @return     -1: 失败, 其他：信号强度值[0-4]
* @history:
**********************************************************************/
int get_4g_rssi(void)
{
	FILE *fp = NULL;
	int ret = 0;
	char rssi[2] = {0};

	memset(rssi, 0, sizeof(rssi));

	if (access(RSSI_4G_FILE, F_OK)) {
        DBG_LOG_ERROR("%s doesn't exist!\n", RSSI_4G_FILE);
		return -1;
	} else {
		if (NULL == (fp = fopen(RSSI_4G_FILE,"r"))) {
            DBG_LOG_ERROR("fopen %s fail!\n", RSSI_4G_FILE);
			return -1;
		} else {
			ret = fread(rssi, sizeof(rssi), 1, fp);
			if (ret != 1) {
                DBG_LOG_ERROR("fread %s fail! %s\n", RSSI_4G_FILE, strerror(errno));
				fclose(fp);
				return -1;
			} else {
				ret = atoi(rssi);
			}
		}
	}

	fclose(fp);
	fp = NULL;
	return ret;
}


/*********************************************************************
* @fn	      set_4g_rssi
* @brief      设置4G信号强度
* @param      rssi[in]: [0-4]
* @return     0: 成功 -1: 失败
* @history:
**********************************************************************/
int set_4g_rssi(int rssi)
{
	FILE *fp = NULL;
	int ret = 0;
	char rssi_str[2] = {0};

	if ((rssi < SIGNAL_STRENGTH_NONE_OR_UNKNOWN) || (rssi > SIGNAL_STRENGTH_GREAT)) {
        DBG_LOG_ERROR("Invalid params! %c\n", rssi);
		return -1;
	}

    snprintf(rssi_str, sizeof(rssi_str), "%d", rssi);

	fp = fopen(RSSI_4G_FILE, "w+");
	if (fp == NULL) {
        DBG_LOG_ERROR("fopen %s fail! %s\n", RSSI_4G_FILE, strerror(errno));
		return -1;
	}

	if (fwrite(rssi_str, 2, 1, fp) != 1) {
        DBG_LOG_ERROR("fwrite %s fail! %s\n", RSSI_4G_FILE, strerror(errno));
		fclose(fp);
		return -1;
	}

	fclose(fp);
	fp = NULL;
	return ret;
}


/*********************************************************************
* @fn	      get_ws_conn_status
* @brief      获取websocket连接状态
* @param      void
* @return     -1: 失败, 0: 未连接， 1: 连接
* @history:
**********************************************************************/
int get_ws_conn_status(void)
{
	FILE *fp = NULL;
	int ret = 0;
	char status[2] = {};

	memset(status,0,sizeof(status));

	if (access(WS_CONN_STATUS_FILE,F_OK)) {
        DBG_LOG_ERROR("%s doesn't exist!\n", WS_CONN_STATUS_FILE);
		return -1;
	} else {
		if (NULL == (fp = fopen(WS_CONN_STATUS_FILE,"r"))) {
            DBG_LOG_ERROR("fopen %s fail!\n", WS_CONN_STATUS_FILE);
			return -1;
		} else {
			ret = fread(status,sizeof(status),1,fp);
			if (ret != 1) {
                DBG_LOG_ERROR("fread %s fail! %s\n", WS_CONN_STATUS_FILE, strerror(errno));
				fclose(fp);
				return -1;
			} else {
				ret = atoi(status);
			}
		}
	}

	fclose(fp);
	fp = NULL;
	return ret;
}


/*********************************************************************
* @fn	      set_ws_conn_status
* @brief      设置websocket连接状态
* @param      status[in]: 0是未联网，1是已联网
* @return     0: 成功 -1: 失败
* @history:
**********************************************************************/
int set_ws_conn_status(int status)
{
	FILE *fp = NULL;
	int ret = 0;
	char *status_str = NULL;

	if ((status != WS_CONNECTED) && (status != WS_DISCONNECTED)) {
        DBG_LOG_ERROR("Invalid params! %c\n", status);
		return -1;
	}

	status_str = (status == WS_CONNECTED) ? "1" : "0";

	fp = fopen(WS_CONN_STATUS_FILE,"w+");
	if (fp == NULL) {
        DBG_LOG_ERROR("fopen %s fail! %s\n", WS_CONN_STATUS_FILE, strerror(errno));
		return -1;
	}

	if (fwrite(status_str,2,1,fp) != 1) {
        DBG_LOG_ERROR("fwrite %s fail! %s\n", WS_CONN_STATUS_FILE, strerror(errno));
		fclose(fp);
		return -1;
	}

	fclose(fp);
	fp = NULL;
	return ret;
}


/*********************************************************************
* @fn	   get_net_mode
*
* @brief   获取上网模式
*
* @param   net_mode_info[in]: 保存上网模式信息
*
* @return  0:success -1:failed
*
*/
int get_net_mode(net_mode_info_t* net_mode_info)
{
	FILE *fp = NULL;
	int ret = 0;
	char mode[64];
	
	LogDebug("start get net mode\n");
	memset(mode, 0, sizeof(mode));
	if(net_mode_info==NULL)
	{
		dbg_print(LY_DEBUG_ERROR, __FILE__, __func__, __LINE__, "Invalid params! net_mode_info is null \n");
		LogError("Invalid params! net_mode_info is null \n");
		return -1;
	}

	if (access(NET_MODE_FILE, F_OK)) {
		dbg_print(LY_DEBUG_ERROR,__FILE__,__func__,__LINE__,"%s doesn't exist!\n", NET_MODE_FILE);
		LogError("%s doesn't exist!\n", NET_MODE_FILE);
		return -1;
	} else {
		if (NULL == (fp = fopen(NET_MODE_FILE,"r"))) {
			dbg_print(LY_DEBUG_ERROR,__FILE__,__func__,__LINE__,"fopen %s fail!\n", NET_MODE_FILE);
			LogError("fopen %s fail!\n", NET_MODE_FILE);
			return -1;
		} else {
			ret = fread(mode, 1, sizeof(mode), fp);
			if (ret == -1) {
				dbg_print(LY_DEBUG_ERROR,__FILE__,__func__,__LINE__,"fread %s failed ret=%d! %s\n", NET_MODE_FILE, ret, strerror(errno));
				LogError("fread %s failed ret=%d! %s\n", NET_MODE_FILE, ret, strerror(errno));
				ret = -1;
				net_mode_info->net_mode = NET_MODE_4G;
				net_mode_info->net_switch_state = NET_SWITCHED;
			} else {
				char* infos[4] = {0};
				LogDebug("read mode info:%s\n", mode);
				ret = split_string_to_array(mode, NET_MODE_SPLIT, (char**)&infos, 4);
				if(infos[0] == NULL)
				{
					net_mode_info->net_mode = NET_MODE_4G;
					net_mode_info->net_switch_state = NET_SWITCHED;
					LogError("get net mode failed: file %s is empty\n", NET_MODE_FILE);
					ret = -1;
				}
				else
				{
					memset(net_mode_info, 0, sizeof(net_mode_info_t));
					net_mode_info->net_mode = atoi(infos[0]);
					net_mode_info->net_switch_state = infos[1] == NULL ? NET_UNSWITCHED : atoi(infos[1]);
					if(infos[2] != NULL)
					{
						strncpy(net_mode_info->net_gateway, infos[2], sizeof(net_mode_info->net_gateway));
					}
					if(infos[3] != NULL)
					{
						strncpy(net_mode_info->net_nameserver, infos[3], sizeof(net_mode_info->net_nameserver));
					}
					ret = 0;
				}
			}
		}
	}

	fclose(fp);
	fp = NULL;
	return ret;
}


/*********************************************************************
* @fn	    set_net_mode
*
* @brief	设置上网模式
*
* @param  	net_mode_info[in]: 上网模式信息
*
* @return	0:success -1:failed
*
*/
int set_net_mode(net_mode_info_t* net_mode_info)
{
	FILE *fp = NULL;
	int ret = 0;
	int len = 0;
	int mode=0;
	char mode_str[64] = {0};
	
	if(net_mode_info==NULL)
	{
		dbg_print(LY_DEBUG_ERROR, __FILE__, __func__, __LINE__, "Invalid params! net_mode_info is null \n");
		LogError("Invalid params! net_mode_info is null \n");
		return -1;
	}
	if ((mode != NET_MODE_4G) && (mode != NET_MODE_LAN)) {
		dbg_print(LY_DEBUG_ERROR, __FILE__, __func__, __LINE__, "Invalid params! %d\n", mode);
		LogError("Invalid params! %d\n", mode);
		return -1;
	}

	len = snprintf(mode_str, sizeof(mode_str), "%d,%d,%s,%s"
				, net_mode_info->net_mode, net_mode_info->net_switch_state
				, net_mode_info->net_gateway, net_mode_info->net_nameserver);
	LogInfo("set net mode to %s\n", mode_str);

	fp = fopen(NET_MODE_FILE, "w+");	
	if (fp == NULL) {
		dbg_print(LY_DEBUG_ERROR, __FILE__, __func__, __LINE__, "fopen %s fail! %s\n", NET_MODE_FILE, strerror(errno));
		LogError("fopen %s fail! %s\n", NET_MODE_FILE, strerror(errno));
		return -1;
	}

	if ((ret = fwrite(mode_str, 1, len, fp)) != len) {
		dbg_print(LY_DEBUG_ERROR, __FILE__, __func__, __LINE__, "fwrite %s failed ret %d not equal %d! %s\n", NET_MODE_FILE, ret, len, strerror(errno));
		LogError("fwrite %s failed ret %d not equal %d! %s\n", NET_MODE_FILE, ret, len, strerror(errno));
		fclose(fp);
		return -1;
	}

	fclose(fp);
	fp = NULL;
	return 0;
}


/*********************************************************************
* @fn	      get_hw_mode
* @brief      获取硬件模式
* @param      void
* @return     0: 生产模式 1: 出厂模式
* @history:
**********************************************************************/
int get_hw_mode(void)
{
	FILE *fp = NULL;
	int ret = 0;
	char mode[2];

	memset(mode, 0, sizeof(mode));

	if (access(HW_MODE_FILE, F_OK)) {
        DBG_LOG_ERROR("%s doesn't exist!\n", HW_MODE_FILE);
		return -1;
	} else {
		if (NULL == (fp = fopen(HW_MODE_FILE,"r"))) {
            DBG_LOG_ERROR("fopen %s fail!\n", HW_MODE_FILE);
			return -1;
		} else {
			ret = fread(mode, sizeof(mode), 1, fp);
			if (ret != 1) {
                DBG_LOG_ERROR("fread %s fail! %s\n", HW_MODE_FILE, strerror(errno));
				fclose(fp);
				return -1;
			} else {
				ret = atoi(mode);
			}
		}
	}

	fclose(fp);
	fp = NULL;
	return ret;
}

/*********************************************************************
* @fn	      set_hw_mode
* @brief      设置硬件模式
* @param      mode[in]: 模式
* @return     0: 成功 -1: 失败
* @history:
**********************************************************************/
int set_hw_mode(int mode)
{
	FILE *fp = NULL;
	char *mode_str = NULL;

	if ((mode != PRODUCTION_MODE) && (mode != FACTORY_MODE)) {
        DBG_LOG_ERROR("Invalid params! %d\n", mode);
		return -1;
	}

	mode_str = (mode == FACTORY_MODE) ? "1" : "0";

	fp = fopen(HW_MODE_FILE, "w+");
	if (fp == NULL) {
        DBG_LOG_ERROR("fopen %s fail! %s\n", HW_MODE_FILE, strerror(errno));
		return -1;
	}

	if (fwrite(mode_str, 2, 1, fp) != 1) {
        DBG_LOG_ERROR("fwrite %s fail! %s\n", HW_MODE_FILE, strerror(errno));
		fclose(fp);
		return -1;
	}

	fclose(fp);
	fp = NULL;
	return 0;
}

int get_device_id(char device_id[10])
{
    int fd = 0;
    int nret = 0;

	if (NULL == device_id) {
        return -1;
	}
    
    if ((fd = open(EEPROM_DEV, O_RDWR)) < 0) {
        printf("open eeprom failed");
        return -1;
    }

    nret = read(fd, device_id, 9);
    if(nret != 9) {
        printf("read eeprom failed:%s, nret=%d\n", strerror(errno), nret);
        close(fd);
        return -1;
    }
    close(fd);
	return 0;
}


void dump_hex_ex(int level, char* file, int line, char* msg, uint8_t* data, int len)
{
	int i = 0, offset = 0;
	uint8_t with_date = 1;
	char hextmp[1024] = { 0 };

	if(level > CUR_LOG_LEVEL)
		return;

	if(msg != NULL)
		offset += snprintf(hextmp + offset, sizeof(hextmp) - offset, "%s", msg);
	for(i = 0; i < len; i++) {
		offset += snprintf(hextmp + offset, sizeof(hextmp) - offset, "%02X ", data[i]);
		if(i != 0 && i % 256 == 0) {
			LogCommEx(level, file, line, with_date, 0, hextmp);
			with_date = 0;
			offset = 0;
			hextmp[0] = '\0';
		}
	}
	LogCommEx(level, file, line, with_date, 0, "%s\n", hextmp);
}

void dump_hex(char* msg, uint8_t* data, int len)
{
	dump_hex_ex(LOG_LV_DEBUG, LOG_FLAG, msg, data, len);
}

char* hex2string(uint8_t* data, int len, char* strbuff, int maxlen)
{
	static char sbuff[1024] = { 0 };
	int offset = 0;
	int i = 0;

	if(strbuff == NULL) {
		strbuff = sbuff;
		maxlen = sizeof(sbuff);
	}

	for(i = 0; i < len; i++) {
		offset += snprintf(strbuff + offset, maxlen - offset, "%02X ", data[i]);
		// if(i != 0 && i % 32 == 0) {
		// 	strbuff[offset++] = '\n';
		// }
	}

	return strbuff;
}