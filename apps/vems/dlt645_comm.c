/*******************************************************************************
 * @             Copyright (c) 2023 LinyPower, All Rights Reserved.
 * @*****************************************************************************
 * @FilePath     : dlt645_comm.c
 * @Descripttion : 
 * @Author       : xiaoyongjun
 * @E-Mail       : xiaoyongjun@linygroup.cn
 * @Version      : v1.0.0
 * @Date         : 2023-12-08 16:34:44
*******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "dlt645_comm.h"

#include "dlt645_2007_private.h"
#include "debug_and_log.h"
#include "load_power_collect.h"

/*从机地址 也就是电表的地址*/
#define AMMETER_ADDR   {00,01,02,03,04,05}
uint8_t ammeter_addr[8] = { 0 };
/*645协议句柄*/
dlt645_t* g_dlt645_ctx = NULL;

/*需要读取哪些功能参数*/
dlt645_cmd_table_t  dlt645_read_run_info_table[] =
{
    {DIC_2030000,{0},0,0,DLT645_2007,1000},/*瞬时总有功功率*/

    # if 0
    {DIC_0,{0},0,0,DLT645_2007,1000},      /*总电能*/
    {DIC_10000,{0},0,0,DLT645_2007,1000},  /*正相有功总电能*/
    {DIC_20000,{0},0,0,DLT645_2007,1000},  /*反相有功总电能*/

    {DIC_2010100,{0},0,0,DLT645_2007,1000},/*A相电压*/
    {DIC_2010200,{0},0,0,DLT645_2007,1000},/*B相电压*/
    {DIC_2010300,{0},0,0,DLT645_2007,1000},/*C相电压*/

    {DIC_2020100,{0},0,0,DLT645_2007,1000},/*A相电流*/
    {DIC_2020200,{0},0,0,DLT645_2007,1000},/*B相电流*/
    {DIC_2020300,{0},0,0,DLT645_2007,1000},/*C相电流*/

    
    {DIC_2030100,{0},0,0,DLT645_2007,1000},/*A相有功功率*/
    {DIC_2030200,{0},0,0,DLT645_2007,1000},/*B相有功功率*/
    {DIC_2030300,{0},0,0,DLT645_2007,1000},/*C相有功功率*/

    {DIC_2060000,{0},0,0,DLT645_2007,1000},/*功率因数*/
    #endif

};

/*******************************************************************************
 * @brief 设置打印信息
 * @param cmd
 * @return 
*******************************************************************************/
char* dlt645_print_info(uint32_t cmd)
{
    char* p = NULL;
    switch(cmd) {

    case DIC_2030000:p = "read total active power"; break;
    #if 0
    case DIC_0:p = "read  combine active total  elec energy"; break;
    case DIC_10000:p = "read forward active total elec energy"; break;
    case DIC_20000:p = "read inverse active total elec energy"; break;
        
    case DIC_2010100:p = "read phase a vol"; break;
    case DIC_2010200:p = "read phase b vol"; break;
    case DIC_2010300:p = "read phase c vol"; break;

    case DIC_2020100:p = "read phase a current"; break;
    case DIC_2020200:p = "read phase b current"; break;
    case DIC_2020300:p = "read phase c current"; break;
    
    
    case DIC_2030100:p = "read phase a active power"; break;
    case DIC_2030200:p = "read phase b active power"; break;
    case DIC_2030300:p = "read phase c active power"; break;

    case DIC_2060000:p = "read total power factor"; break;
    #endif
  
    default: p = "no dlt645 cmd"; break;
    }

    return p;
}
/*******************************************************************************
 * @brief 判断是否乘上变流比
 * @param cmd
 * @return 0：不乘以变流比 1：乘以变流比
*******************************************************************************/
int dlt645_multi_flow_rate(uint32_t cmd)
{
    int ret = 0;
    switch(cmd) {

    case DIC_2030000:ret = 1; break;
    # if 0    
    case DIC_0: 
    case DIC_10000:
    case DIC_20000

    case DIC_2020100:
    case DIC_2020200:
    case DIC_2020300:


    case DIC_2030100:
    case DIC_2030200:
    case DIC_2030300: break;

    case DIC_2010100:
    case DIC_2010200:
    case DIC_2010300:    
    case DIC_2060000: break;
    #endif

    default:break;
    }
    return ret;
}
/*******************************************************************************
 * @brief 
 * @param arg
 * @return 
*******************************************************************************/
static void* dlt645_comm_thread_do(void* arg)
{
    int i = 0, ret = 0;
    dlt645_cmd_table_t* pcmd = NULL;
    elec_meter_t* pemeter = (elec_meter_t*)arg;
    emeter_data_collect_init(pemeter);
    do {
        pemeter->ctx645 = dlt645_new_tcp(pemeter->dev_ip, pemeter->dev_port);
        if(pemeter->ctx645 == NULL)
            sleep(1);
    } while(pemeter->ctx645 == NULL);
    LogInfo("dlt645_new_tcp(%s, %d) success", pemeter->dev_ip, pemeter->dev_port);
    
    while(dlt645_tcp_connect(pemeter->ctx645) != 0) {
        LogInfo("dlt645_tcp_connect to [%s:%d] failed: %s", pemeter->dev_ip, pemeter->dev_port, strerror(errno));
        sleep(1);
    }
    LogInfo("dlt645_tcp_connect to [%s:%d] success", pemeter->dev_ip, pemeter->dev_port);

    while(dlt645_read_addr(pemeter->ctx645, ammeter_addr) != 0) {
        LogInfo("dlt645 thread tcp read ammeter addr filed\n");
        sleep(1);
    }
    memcpy(pemeter->ctx645->addr, ammeter_addr, 6);
    LogInfo("ammeter addr is [%x] [%x] [%x] [%x] [%x] [%x]\n", ammeter_addr[0], ammeter_addr[1], ammeter_addr[2], ammeter_addr[3], ammeter_addr[4], ammeter_addr[5]);

    while(1) {
        /*读取电表数据*/
        for(i = 0; i < sizeof(dlt645_read_run_info_table) / sizeof(dlt645_cmd_table_t); i++) {
            pcmd = &dlt645_read_run_info_table[i];
            ret = dlt645_read_data(pemeter->ctx645, pcmd->code, pcmd->dlt645_real_data, pcmd->protocal_type, pcmd->timeout_ms);
            if(ret != -1) {
                pcmd->real_data_len = ret;
                memcpy(&pcmd->real_fdata, pcmd->dlt645_real_data, 4);
                if(dlt645_multi_flow_rate(pcmd->code) == 1) {
                    pcmd->real_fdata *= VARIABLE_FLOW_RATE; 
                }
                //LogDebug("%s value[%.4f]", dlt645_print_info(pcmd->code), pcmd->real_fdata);
            } else {
                LogWarn("%s failed", dlt645_print_info(pcmd->code));
            }
        }

        pemeter->load_power_real = dlt645_read_run_info_table[RDT_IDX_P].real_fdata;
        pemeter->load_power = (int)(pemeter->load_power_real * COMM_DATA_MULTIPLE);
    }
    return (void*)0;
}
/*******************************************************************************
 * @brief 创建dlt645通信线程
 * @return 
*******************************************************************************/
int create_dlt645_comm_thread(elec_meter_t* pemeter)
{
    int ret = -1;
    pthread_t dlt645_tid;
    pthread_attr_t dlt645_tattr;

    pthread_attr_init(&dlt645_tattr);
    pthread_attr_setdetachstate(&dlt645_tattr, PTHREAD_CREATE_DETACHED);
    ret = pthread_create(&dlt645_tid, &dlt645_tattr, dlt645_comm_thread_do, pemeter);
    if(ret != 0) {
        LogError("create dlt645 comm thread failed: %s", strerror(errno));
    }
    return ret;
}


