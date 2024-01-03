#ifndef __DLT645_2007_H
#define __DLT645_2007_H

#include "dlt645.h"


#define CMD_RESERVE 0x0     /*预留*/
#define CMD_BROADCAST_HOUR  0x08 /*广播校时*/
#define CMD_READ_DATA       0x11/*读数据*/
#define CMD_READ_CONTI_DATA 0x12/*读后续数据*/
#define CMD_READ_COMM_ADDR  0x13 /*读通信地址*/
#define CMD_WRITE_DATA      0x14 /*写数据*/
#define CMD_WRITE_COMM_ADDR 0x15 /*写通信地址*/
#define CMD_FREEZE          0x16 /*冻结命令*/
#define CMD_CHANGE_COMM_SPEED  0x17/*更改通信速度*/
#define CMD_CHANGE_PASSWD     0x18/*修改密码*/
#define CMD_MAX_DEMAND_CLEAR  0x19/*最大需量清0*/
#define CMD_METER_CLEAR       0x1A/*电表清0*/
#define CMD_EVENT_CLEAR       0x1B/*事件清0*/

#define CONTI_FLAG_0 0  /*无后续数据*/
#define CONTI_FLAG_1 1  /*有后续数据*/

#define SLAVE_RES_OK 0     /*从站正确响应*/
#define SLAVE_RES_ERROR 1  /*从站异常响应*/

#define MASTER_SEND_CMD 0  /*主站发送命令*/
#define SLAVE_SEND_RES  1  /*从站发送响应*/




typedef union {

    struct {
        
    uint8_t cmd : 5;
    uint8_t contiflag : 1;
    uint8_t slave_res_flag : 1;
    uint8_t transe_direction : 1;

    }bits; 
    uint8_t byte;
}dit645_ctrl_t;
//基于 DLT645 2007 协议读取数据
int dlt645_2007_read_data(dlt645_t* ctx, uint32_t code, uint8_t* read_data, int timeout_ms);
//基于 DLT645 2007 协议校验数据
int dlt645_2007_recv_check(uint8_t *msg, int len, uint8_t *addr, uint32_t code);
int dlt645_2007_read_addr(dlt645_t* ctx, uint8_t* addr, int timeout_ms);

#endif /* __DLT645_2007_H */
