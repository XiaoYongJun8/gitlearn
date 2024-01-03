/*************************************************
 Copyright (c) 2019
 All rights reserved.
 File name:     dlt645_2007.c
 Description:   DLT645 2007版本
 History:
 1. Version:
    Date:       2019-09-19
    Author:     wangjunjie
    Modify:
*************************************************/
#include <stdio.h>
#include "dlt645_private.h"
#include "dlt645_2007_private.h"
#include "string.h"
#include "dlt645_2007.h"

/**
 * Name:    dlt645_2007_recv_check
 * Brief:   DLT645-2007 数据校验
 * Input:
 *  @msg:   校验数据包
 *  @len:   数据包长度
 *  @addr:  从站地址
 *  @code:  操作码
 * Output:  None
 */
int dlt645_2007_recv_check(uint8_t* msg, int len, uint8_t* addr, uint32_t code)
{
    if(dlt645_common_check(msg, len, addr) < 0) {
        DLT645_LOG("dlt645_common_check error!\n");
        return -1;
    }
    if(msg[DL645_CONTROL_POS] == 0x94) {
        DLT645_LOG("msg[DL645_CONTROL_POS][%x] fail!\n", msg[DL645_CONTROL_POS]);
        return 0;
    }
       

    uint8_t* code_buf = (uint8_t*)&code;

    for(uint8_t i = 0; i < 4; i++) {
        code_buf[i] += 0x33;
    }

    if(*((uint32_t*)(msg + DL645_DATA_POS)) != code) {
        DLT645_LOG("code unmatch[%x][%x]\n", code, *(uint32_t*)(msg + DL645_DATA_POS));
        return -1;
    }
   
    return 0;
}

/**
 * Name:    dlt645_2007_parsing_data
 * Brief:   DLT645-2007 数据包解析
 * Input:
 *  @code:          标识符
 *  @read_data:     数据包指针
 *  @len:           数据包长度
 *  @real_val:      数据存储地址
 * Output:  数据包长度
 */
int dlt645_2007_parsing_data(uint32_t code, uint8_t* read_data, uint16_t len, uint8_t* real_val)
{
    switch(code) {
    case DIC_0:
        DLT645_LOG("prase code active total elec[%x ", DIC_0);
        dlt645_data_parse_by_format_to_float(read_data, 4, "XXXXXX.XX", real_val);
        break;
    case DIC_100:
    case DIC_200:
    case DIC_300:
    case DIC_400:
    case DIC_10000:
        DLT645_LOG("prase code forward active total elec[%x ", DIC_10000);
        dlt645_data_parse_by_format_to_float(read_data, 4, "XXXXXX.XX", real_val);
        break;
    case DIC_10100:
    case DIC_10200:
    case DIC_10300:
    case DIC_10400:
    case DIC_20000:
        DLT645_LOG("prase code inverse active total elec[%x ", DIC_20000);
        dlt645_data_parse_by_format_to_float(read_data, 4, "XXXXXX.XX", real_val);
        break;
    case DIC_20100:
    case DIC_20200:
    case DIC_20300:
    case DIC_20400:
    case DIC_30000:
    case DIC_40000:
    case DIC_50000:
    case DIC_60000:
    case DIC_70000:
    case DIC_80000:
    case DIC_90000:
    {
        dlt645_data_parse_by_format_to_float(read_data, 4, "XXXXXX.XX", real_val);
        break;
    }
    case DIC_2010100:
        DLT645_LOG("prase code ua[%x ", DIC_2010100);
        dlt645_data_parse_by_format_to_float(read_data, 2, "XXX.X", real_val);
        break;
    case DIC_2010200:
        DLT645_LOG("prase code ub[%x ", DIC_2010200);
        dlt645_data_parse_by_format_to_float(read_data, 2, "XXX.X", real_val);
        break;
    case DIC_2010300:
        DLT645_LOG("prase code uc[%x ", DIC_2010300);
        dlt645_data_parse_by_format_to_float(read_data, 2, "XXX.X", real_val);
        break;
    case DIC_20C0100:
    case DIC_20C0200:
    case DIC_20C0300:
    {
        dlt645_data_parse_by_format_to_float(read_data, 2, "XXX.X", real_val);
        break;
    }
    case DIC_2020100:
        DLT645_LOG("prase code ia[%x ", DIC_2020100);
        dlt645_data_parse_by_format_to_float(read_data, 3, "XXX.XXX", real_val);
        break;
    case DIC_2020200:
        DLT645_LOG("prase code ib[%x ", DIC_2020200);
        dlt645_data_parse_by_format_to_float(read_data, 3, "XXX.XXX", real_val);
        break;
    case DIC_2020300:
        DLT645_LOG("prase code ic[%x ", DIC_2020300);
        dlt645_data_parse_by_format_to_float(read_data, 3, "XXX.XXX", real_val);
        break;
    case DIC_2030000:
    {
        DLT645_LOG("prase code toal active power[%x ", DIC_2030000);
        dlt645_data_parse_by_format_to_float(read_data, 3, "XX.XXXX", real_val);
        break;
    }
    case DIC_2030100:
    {
        DLT645_LOG("prase code phase A active power[%x ", DIC_2030000);
        dlt645_data_parse_by_format_to_float(read_data, 3, "XX.XXXX", real_val);
        break;
    }
    case DIC_2030200:
    {
        DLT645_LOG("prase code phase B active  power[%x ", DIC_2030000);
        dlt645_data_parse_by_format_to_float(read_data, 3, "XX.XXXX", real_val);
        break;
    }
    case DIC_2030300:
    {
        DLT645_LOG("prase code phase C active  power[%x ", DIC_2030000);
        dlt645_data_parse_by_format_to_float(read_data, 3, "XX.XXXX", real_val);
        break;
    }
    case DIC_2040000:
    case DIC_2040100:
    case DIC_2040200:
    case DIC_2040300:
    case DIC_2050000:
    {
        DLT645_LOG("prase code toal apparent power[%x ", DIC_2050000);
        dlt645_data_parse_by_format_to_float(read_data, 3, "XX.XXXX", real_val);
        break;
    }
    case DIC_2050100:
    case DIC_2050200:
    case DIC_2050300:
    {
        dlt645_data_parse_by_format_to_float(read_data, 3, "XX.XXXX", real_val);
        break;
    }
    case DIC_2060000:
    {
        DLT645_LOG("prase code toal power factor[%x ", DIC_2060000);
        dlt645_data_parse_by_format_to_float(read_data, 2, "X.XXX", real_val);
        break;
    }
    case DIC_2060100:
    case DIC_2060200:
    case DIC_2060300:
    {
        dlt645_data_parse_by_format_to_float(read_data, 2, "X.XXX", real_val);
        break;
    }
    case DIC_2800002:
    {
        dlt645_data_parse_by_format_to_float(read_data, 2, "XX.XX", real_val);
        break;
    }
    case DIC_4000403:
    case DIC_5060101:
    case DIC_7000001:
    case DIC_7000002:
        for(uint16_t i = 0; i < len; i++) {
            real_val[i] = read_data[i] - 0x33;
        }
        break;
    default:
        for(uint16_t i = 0; i < len; i++) {
            real_val[i] = ((read_data[i] - 0x33) & 0x0f) + ((read_data[i] - 0x33) >> 4) * 10;
        }
        break;
    }
     
    return len;
}

/**
 * Name:    dlt645_2007_read_data
 * Brief:   DLT645-2007 数据读取
 * Input:
 *  @ctx:           645句柄
 *  @addr:          从站地址
 *  @code:          数据标识
 *  @read_data:     数据存储地址
 * Output:  None
 */
int dlt645_2007_read_data(dlt645_t* ctx, uint32_t code, uint8_t* read_data, int timeout_ms)
{
    int i = 0;
    uint8_t send_buf[DL645_2007_RD_CMD_LEN];
    uint8_t read_buf[DL645_RESP_LEN];

    memset(read_buf, 0, sizeof(read_buf));
    memset(send_buf, 0, sizeof(send_buf));

    /*前导码*/
    for(i = 0; i < DL645_PREMBLE_LEN; i++) {
        send_buf[i] = DL645_PREMBLE_CODE;
    }
    /*电表通信地址*/    
    memcpy(send_buf + DL645_ADDR_POS, ctx->addr, DL645_ADDR_LEN);

    send_buf[DL645_CONTROL_POS] = C_2007_CODE_RD;
    send_buf[DL645_LEN_POS] = 4;

    uint8_t send_code[4] = { 0 };
    send_code[0] = (code & 0xff) + 0x33;
    send_code[1] = ((code >> 8) & 0xff) + 0x33;
    send_code[2] = ((code >> 16) & 0xff) + 0x33;
    send_code[3] = ((code >> 24) & 0xff) + 0x33;

    memcpy(send_buf + DL645_DATA_POS, send_code, 4);

    if(dlt645_send_msg(ctx, send_buf, DL645_2007_RD_CMD_LEN) < 0) {
        DLT645_LOG("send data error!\n");
        return -1;
    }

    if(dlt645_receive_msg(ctx, read_buf, DL645_RESP_LEN, code, DLT645_2007, timeout_ms) < 0) {
        DLT645_LOG("receive msg error!\n");
        return -1;
    }
    return dlt645_2007_parsing_data(code, read_buf + DL645_DATA_POS + 4, read_buf[DL645_LEN_POS] - 4, read_data);
}
//FE FE FE FE 68 AA AA AA AA AA AA 68 13 00 DF 16 :读取通信地址报文
//
int dlt645_2007_read_addr(dlt645_t* ctx, uint8_t* addr, int timeout_ms)
{
    int i = 0;
    dit645_ctrl_t cmd = {0};
    uint8_t addr_ctrl[6] = { 0xaa,0xaa, 0xaa, 0xaa, 0xaa, 0xaa };
    uint8_t send_buf[DL645_2007_RD_CMD_LEN];
    uint8_t read_buf[DL645_RESP_LEN];
    int nread = 0;

    memset(read_buf, 0, sizeof(read_buf));
    memset(send_buf, 0, sizeof(send_buf));


    for(i = 0; i < DL645_PREMBLE_LEN; i++) {
        send_buf[i] = DL645_PREMBLE_CODE;
    }

    memcpy(send_buf + DL645_ADDR_POS, addr_ctrl, DL645_ADDR_LEN);

    send_buf[DL645_CONTROL_POS] = C_2007_CODE_RDA;
 
    send_buf[DL645_LEN_POS] = 0;
    if(dlt645_send_msg(ctx, send_buf, DL645_2007_RD_CMD_LEN-4) < 0) {
        DLT645_LOG("send data error!\n");
        return -1;
    }

    if(timeout_ms > 0 && ctx->select) {
        ctx->response_timeout.tv_sec = timeout_ms / 1000;
        ctx->response_timeout.tv_usec = (timeout_ms % 1000) * 1000;
        if(ctx->select(ctx) == ctx->s) {
            nread = ctx->read(ctx, read_buf, DL645_RESP_LEN);
        }
    } else {
        nread = ctx->read(ctx, read_buf, DL645_RESP_LEN);
    }
    if(nread < 0) {
        DLT645_LOG("receive msg error!\n");
        return -1;
    }

    // DLT645_LOG("dlt645_recv_msg [%d, ", nread);
    // for(i = 0; i < nread; i++) {
    //     printf("%02X ", read_buf[i]);
    // }
    cmd.byte = read_buf[12];
    if((cmd.bits.cmd == CMD_READ_COMM_ADDR) && (cmd.bits.slave_res_flag == SLAVE_RES_OK)) {
        
    } else {
        DLT645_LOG("dlt645_2007_read_addr slave res error %x", cmd.byte);
        return -1;
    }
    memcpy(addr, &read_buf[5], 6);
   
    // DLT645_LOG("dlt645_2007_read_addr got rsp[%d, ", nread);
    // for(i = 0; i < nread; i++) {
    //     printf("%02X ", read_buf[i]);
    // }
    // DLT645_LOG("]\n");

    return 0;
}

/**
 * Name:    dlt645_write_data
 * Brief:   DLT645-2007 数据写入
 * Input:
 *  @ctx:           645句柄
 *  @addr:          从站地址
 *  @code:          数据标识
 *  @write_data:    写入数据的指针
 *  @write_len:     写入长度
 * Output:  None
 */
int dlt645_write_data(dlt645_t* ctx, uint32_t addr, uint32_t code, uint8_t* write_data, uint8_t write_len, int timeout_ms)
{
    uint8_t send_buf[DL645_WR_LEN];
    uint8_t read_buf[DL645_RESP_LEN];

    memset(read_buf, 0, sizeof(read_buf));
    memset(send_buf, 0, sizeof(send_buf));

    memcpy(send_buf + 1, ctx->addr, DL645_ADDR_LEN);

    send_buf[DL645_CONTROL_POS] = C_2007_CODE_WR;
    send_buf[DL645_LEN_POS] = 12 + write_len;

    uint8_t send_code[4] = { 0 };
    send_code[0] = (code & 0xff) + 0x33;
    send_code[1] = ((code >> 8) & 0xff) + 0x33;
    send_code[2] = ((code >> 16) & 0xff) + 0x33;
    send_code[3] = ((code >> 24) & 0xff) + 0x33;

    for(uint8_t i = 0; i < write_len; i++) {
        write_data[i] += 0x33;
    }

    memcpy(send_buf + DL645_DATA_POS, send_code, 4);
    memcpy(send_buf + DL645_DATA_POS + 12, write_data, write_len);
    if(dlt645_send_msg(ctx, send_buf, 24 + write_len) < 0) {
        DLT645_LOG("send data error!\n");
        return -1;
    }
    
    if(dlt645_receive_msg(ctx, read_buf, DL645_RESP_LEN, code, DLT645_2007, timeout_ms) < 0) {
        DLT645_LOG("receive msg error!\n");
        return -1;
    }
    return 0;
}
