/***************************************************************************
 * Copyright (C), 2016-2026, , All rights reserved.
 * File        :  eth_network.h
 * Decription  :  以太网联网
 * Author      :  shiwb
 * Version     :  v1.0
 * Histroy	   :  2020/8/4
 **************************************************************************/
#ifndef __ETH_NETWORK_H__
#define __ETH_NETWORK_H__


#define ETH_OP_CONN			0
#define ETH_OP_DISCONN		1
#define ETH_OP_CHECK		2

#define ETH_SWITCH_SCRIPT 		"/home/root/shell/eth_switch.sh"
#define DEF_GATEWAY				"192.168.225.254"
#define DEF_DNS					"192.168.1.1"

extern struct network_ops eth_network;




#endif