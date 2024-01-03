/***************************************************************************
 * Copyright (C), 2016-2026, , All rights reserved.
 * File        :  eth_network.c
 * Decription  :  以太网联网模块
 * Author      :  shiwb
 * Version     :  v1.0
 * Date        :  2020/8/4
 * Histroy	   :
 **************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "netconn.h"
#include "eth_network.h"
#include "debug_and_log.h"
#include "util.h"

static char net_gateway[32];		/* ETH上网模式时的网关地址 */
static char net_nameserver[32];	    /* ETH上网模式时的DNS服务器地址 */


/*********************************************************************
* @fn	    eth_network_init
* @brief    以太网网络初始化
* @param    void
* @return   void
* @history:
**********************************************************************/
void eth_network_init(void)
{

}


/*********************************************************************
* @fn	    eth_network_status
* @brief    以太网检查网络状态
* @param    void
* @return   0: 以太网已经联网 1: 以太网未联网
* @history:
**********************************************************************/
int eth_network_status()
{
	int ret = 0;
    char cmd[128] = {0};
    int status = 0;

    snprintf(cmd, sizeof(cmd), "%s %d %s %s", ETH_SWITCH_SCRIPT, ETH_OP_CHECK
		, net_gateway, net_nameserver);
    ret = do_system_ex(cmd, &status, 0);
    ret = (status == 0 ? ret : status);

	return ret;
}


/*********************************************************************
* @fn	    eth_network_connect
* @brief    以太网拨号
* @param    void
* @return   0: 拨号成功 1: 拨号失败
* @history:
**********************************************************************/
int eth_network_connect(void)
{
    int ret = 0;
    char cmd[128] = {0};
    int status = 0;

    snprintf(cmd, sizeof(cmd), "%s %d %s %s", ETH_SWITCH_SCRIPT, ETH_OP_CONN
				, net_gateway, net_nameserver);
    ret = do_system_ex(cmd, &status, 0);
    ret = (status == 0 ? ret : status);

	return ret;
}

/*********************************************************************
* @fn	    eth_network_disconnect
* @brief    eth断网
* @param    void
* @return   0: 断网成功 -1: 断网失败
* @history:
**********************************************************************/
int eth_network_disconnect(void)
{
	int ret = 0;
	char cmd[128] = {0};
	int status = 0;

	snprintf(cmd, sizeof(cmd), "%s %d", ETH_SWITCH_SCRIPT, ETH_OP_DISCONN);
	ret = do_system_ex(cmd, &status, 0);
	ret = (status == 0 ? ret : status);

	return ret;
}

struct network_ops eth_network = {
    .name = "WAN",
    .errstr = "success",
    .network_init = eth_network_init,
    .network_connect = eth_network_connect,
    .network_disconnect = eth_network_disconnect,
    .network_status = eth_network_status
};
