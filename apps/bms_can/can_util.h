/*******************************************************************************
 * @             Copyright (c) 2023 LinyPower, All Rights Reserved.
 * @*****************************************************************************
 * @FilePath     : can_util.h
 * @Descripttion : 
 * @Author       : xiaoyongjun
 * @E-Mail       : xiaoyongjun@linygroup.cn
 * @Version      : v1.0.0
 * @Date         : 2023-11-13 17:00:38
*******************************************************************************/
/*******************************************************************************
 * @             Copyright (c) 2023 LinyPower, All Rights Reserved.
 * @*****************************************************************************
 * @FilePath     : can_util.h
 * @Descripttion : 
 * @Author       : xiaoyongjun
 * @E-Mail       : xiaoyongjun@linygroup.cn
 * @Version      : v1.0.0
 * @Date         : 2023-10-19 10:08:26
*******************************************************************************/
/*******************************************************************************
 * @             Copyright (c) 2022 LinYuan Power, All Rights Reserved.
 * @*****************************************************************************
 * @FilePath     : can_util.h
 * @Descripttion : 
 * @Author       : zhums
 * @E-Mail       : zhums@linygroup.com
 * @Version      : v1.0.0
 * @Date         : 2022-12-21 09:30:02
*******************************************************************************/

#ifndef __CAN_UTIL_H__
#define __CAN_UTIL_H__
#include <stdint.h>
#include <linux/can.h>

#define CAN_ID_READ_DATA_REQ        0xE0        //读取DSP数据请求帧
#define CAN_ID_READ_DATA_RSP        0x20        //读取DSP数据响应帧
#define CAN_ID_104_CFG_IP_REQ       0x21        //104服务IP地址&端口号配置请求帧
#define CAN_ID_104_CFG_ADDR_REQ     0x22        //104点表地址范围配置请求帧
#define CAN_ID_WRITE_DATA_REQ       0x120       //遥控遥调数据配置请求帧
#define CAN_ID_WRITE_DATA_RSP       0x60        //遥控遥调数据配置响应帧

#define CAN_DATA_RESERVE_VALUE      0xFF

#define CAN_FUNC_YX                 0x02        //遥信
#define CAN_FUNC_YC                 0x03        //要测
#define CAN_FUNC_YK                 0x05        //遥控
#define CAN_FUNC_YT                 0x10        //遥调

typedef struct can_request {
    
}can_request_t;

typedef struct can_response {

}can_response_t;

typedef struct can_session {
    int sock;
    can_request_t req;
    can_response_t rsp;
}can_session_t;

int can_init(int bitrate, struct can_filter** filter, int cnt);
void can_destroy(int can_sock);
int can_add_filter(int can_sock, struct can_filter** filter, int cnt);
int can_read(int can_sock, struct can_frame* pcanframe, int timeout_ms);
int can_write(int can_sock, uint8_t* data, int len);
int can_send_read_request(int can_sock, uint16_t func, uint16_t saddr, uint16_t len);
int can_recv_read_response(int can_sock, uint16_t func, uint16_t* rsp_data, int maxlen);
int can_send_write_request(int can_sock, uint16_t func, uint16_t addr, uint16_t val);

#endif //__CAN_UTIL_H__
