/***************************************************************************
 * Copyright (C), 2016-2026, , All rights reserved.
 * File        :  lte_network.h
 * Decription  :  4G联网
 * Author      :  shiwb
 * Version     :  v1.0
 * Histroy	   :  2020/8/4        
 **************************************************************************/

#ifndef __LTE_NETWORK_H__
#define __LTE_NETWORK_H__

#define SIGNAL_STRENGTH_MIN     0
#define SIGNAL_STRENGTH_MAX     31
#define SIGNAL_STRENGTH_NULL    99



extern struct network_ops lte_network;
extern int get_4g_signal_intensity(int *rssi);


#endif