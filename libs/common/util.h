/***************************************************************************
 * Copyright (C), 2016-2026, , All rights reserved.
 * File        : sx_mqtt.c
 * Decription  :
 *
 * Author      :  zhums
 * Version     :  v1.0
 * Date        :  2018/09/07
 * Histroy	   :
 **************************************************************************/


#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "define.h"


#define LIB_SET_UINT32_BIT(Data, Offset)    ((Data) |= (uint32_t)((uint32_t)1u << (Offset)))
#define LIB_SET_UINT16_BIT(Data, Offset)    ((Data) |= (uint16_t)((uint16_t)1u << (Offset)))
#define LIB_SET_UINT8_BIT(Data, Offset)     ((Data) |= (uint8_t)((uint8_t) 1u << (Offset)))

#define LIB_RESET_UINT32_BIT(Data, Offset)  ((Data) &= (uint32_t)(~(uint32_t)((uint32_t)1u << (Offset))))
#define LIB_RESET_UINT16_BIT(Data, Offset)  ((Data) &= (uint16_t)(~(uint16_t)((uint16_t)1u << (Offset))))
#define LIB_RESET_UINT8_BIT(Data, Offset)   ((Data) &= (uint8_t )(~(uint8_t )((uint8_t )1u << (Offset))))

#define LIB_GET_UINT32_BIT(Data, Offset)    (((uint8_t)(((uint32_t) (Data)) >> (Offset))) & (uint8_t)0x01u)
#define LIB_GET_UINT16_BIT(Data, Offset)    (((uint8_t)(((uint16_t) (Data)) >> (Offset))) & (uint8_t)0x01u)
#define LIB_GET_UINT8_BIT(Data, Offset)     (((uint8_t)(((uint8_t ) (Data)) >> (Offset))) & (uint8_t)0x01u)

#define LIB_GET_BYTE_0(Data)                ((uint8_t) (Data))
#define LIB_GET_BYTE_1(Data)                ((uint8_t)((Data)>>(uint8_t)8u))
#define LIB_GET_BYTE_2(Data)                ((uint8_t)((Data)>>(uint8_t)16u))
#define LIB_GET_BYTE_3(Data)                ((uint8_t)((Data)>>(uint8_t)24u))

#define LIB_GET_BYTE_LIT_UINT16(Data)		((uint16_t)Data[0u] + ((uint16_t)Data[1u] << 8u))
#define LIB_GET_BYTE_BIG_UINT16(Data)		((uint16_t)Data[1u] + ((uint16_t)Data[0u] << 8u))


 /* Byte swap of 64-bit value */
#define BIT57_BIT64_UINT64(a)  (((a) >> 56) & 0xFF)
#define BIT49_BIT56_UINT64(a)  (((a) >> 48) & 0xFF)
#define BIT41_BIT48_UINT64(a)  (((a) >> 40) & 0xFF)
#define BIT33_BIT40_UINT64(a)  (((a) >> 32) & 0xFF)
#define BIT25_BIT32_UINT64(a)  (((a) >> 24) & 0xFF)
#define BIT17_BIT24_UINT64(a)  (((a) >> 16) & 0xFF)
#define BIT9_BIT16_UINT64(a)   (((a) >> 8) & 0xFF)
#define BIT1_BIT8_UINT64(a)    ((a) & 0xFF)
#define SWAP64(v)               ((BIT1_BIT8_UINT64(v) << 56) | (BIT9_BIT16_UINT64(v) << 48) | (BIT17_BIT24_UINT64(v) << 40) | (BIT25_BIT32_UINT64(v) << 32) \
                                | (BIT33_BIT40_UINT64(v) << 24) | (BIT41_BIT48_UINT64(v) << 16) | (BIT49_BIT56_UINT64(v) << 8) | BIT57_BIT64_UINT64(v)) 

/* Byte swap of 32-bit value */
#define HI8_UINT32(a)           (((a) >> 24) & 0xFF)
#define MIDH8_UINT32(a)         (((a) >> 16) & 0xFF)
#define MIDL8_UINT32(a)         (((a) >> 8) & 0xFF)
#define LO8_UINT32(a)           ((a) & 0xFF)
#define SWAP32(v)               ((LO8_UINT32(v) << 24) | (MIDL8_UINT32(v) << 16) | (MIDH8_UINT32(v) << 8) | HI8_UINT32(v))

/* Byte swap of 16-bit value */
#define HI8_UINT16(a)           (((a) >> 8) & 0xFF)
#define LO8_UINT16(a)           ((a) & 0xFF)
#define SWAP16(v)               ((LO8_UINT16(v) << 8) | HI8_UINT16(v))

/* Byte combine of 16-bit value, has big endian and little endian type */
#define COMBINE16_BIG(puc)      ((*(puc)<<8) | (*((puc)+1)))
#define COMBINE16_LITTLE(puc)   ((*((puc)+1)<<8) | (*(puc)))

/* Byte combine of 32-bit value */
#define COMBINE32_BIG(puc)      ((*(puc)<<24) | (*((puc)+1)<<16) | (*((puc)+2)<<8) | (*((puc)+3)))
#define COMBINE32_LITTLE(puc)   ((*((puc)+3)<<24) | (*((puc)+2)<<16) | (*((puc)+1)<<8) | (*(puc)))

#define ARRAR_SIZE(arr)             (sizeof(arr) / sizeof((arr)[0]))
#define OFFSET_OF(type, member)     ((size_t)&((type *)0)->member)
#define _STR(_S)                    #_S
#define STR(S)                      _STR(S)
#define MAX_(a, b)                   ((a) >= (b) ? (a) : (b))
#define MIN_(a, b)                   ((a) <= (b) ? (a) : (b))

/* 网络接口信息类型 */
#define MAC 1 //MAC地址
#define IP 2 //IP地址
#define BRDIP 3 //广播地址
#define NETMASK 4//网络掩码

#define LTE_NETWORK_STATUS "/home/root/network_status" //store the status of network. 0: disconnected 1: connected.
#define DEVICE_ID_LEN 9

#define CHANNEL_STATE_ENABLE		1
#define CHANNEL_STATE_DISENABLE		0

#define TIME_VALID           0x5D000000 //对应1970年
#define WAIT_FOR_4G_TIME     5    //等待4G时间同步成功的时间，单位为秒
#define MAX_RETRY_TIMESYNC_CNT 120 //尝试等待时间同步的次数，超过该次数重启系统

typedef struct
{
    double btm_ms;
    double etm_ms;
    double tot_sec;
    double tot_cnt;
    double avg_ms;
    double min_ms;
    double max_ms;
    //float    diff_ms[40960];
}time_use_t;

double time_current_ms();
double time_diff_now(double stm_ms, int islog);
double time_diff(double stm_ms, double etm_ms);

int do_system(char* str_cmd);
int do_system_ex(char *str_cmd, int* cmd_retcode, int is_log);
void safe_sleep(long sec,long usec);
void safe_msleep(long msec);
I64 get_timestamp_ms();
char* current_date_time(char* strtime, int maxLen, char* datefmt);
char* timestamp2string(time_t ts_sec, char* strtime, int maxLen, char* datefmt);

int FindPidByName(const char *pName);

int get_net_info(const char *szDevName, const int type, char *value, const int len);
int get_lte_network_status();
int read_device_id(char device_id[DEVICE_ID_LEN+1]);
void wait_for_valid_time(char* proc_name);
void set_core_limit(int size_kb);
void print_core_limit();
int resource_init_common();

int remove_json_string_end_quot(const char *json_string, char *new_string, const int len);
int del_json_str_quota(char *json_value_str);

char* strcrpl(char* src, char old_ch, char new_ch);
int split_string_to_array(char* pstr, char* psplit, char** parr, int maxcnt);
void safe_free(void *ptr);
int  trim_left(char* RawString);
int  trim_right(char* RawString);
int  trim(char* RawString);
int get_querystring_value(char* querystring, char* key, char* key_value, int maxlen);
int is_number(char* str);
int check_ipv4(char* ipstr);


#endif
