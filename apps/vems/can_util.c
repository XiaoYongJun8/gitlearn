/*******************************************************************************
 * @             Copyright (c) 2022 LinYuan Power, All Rights Reserved.
 * @*****************************************************************************
 * @FilePath     : can_util.c
 * @Descripttion : 
 * @Author       : zhums
 * @E-Mail       : zhums@linygroup.com
 * @Version      : v1.0.0
 * @Date         : 2023-10-19 10:07:53
*******************************************************************************/
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>

#include "can_util.h"
#include "common.h"
#include "debug_and_log.h"

int can_init(int bitrate, struct can_filter** filter, int cnt)
{
    int can_sock = -1;
    struct sockaddr_can addr;
    struct ifreq ifr;
    int status = 0;
    //struct can_filter rfilter[1];
    char cmd[128] = { 0 };
    //int ret;

    snprintf(cmd, sizeof(cmd), "canconfig can0 bitrate %d", bitrate);
    do_system_ex(cmd, &status, 1);
    do_system_ex("canconfig can0 start", &status, 1);

    can_sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    strcpy(ifr.ifr_name, "can0");
    ioctl(can_sock, SIOCGIFINDEX, &ifr);
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    bind(can_sock, (struct sockaddr*)&addr, sizeof(addr));

    if(filter != NULL && cnt > 0) {
        if(can_add_filter(can_sock, filter, cnt) != 0) {
            close(can_sock);
            can_sock = -1;
            LogError("add filter failed");
        }
    }

    return can_sock;
}

void can_destroy(int can_sock)
{
    int status = 0;
    do_system_ex("canconfig can0 stop", &status, 1);
    if(can_sock > 0)
        close(can_sock);
}

int can_add_filter(int can_sock, struct can_filter** filter, int cnt)
{
    if(filter == NULL || cnt <= 0) {
        LogError("add filter param check failed");
        return -1;
    }
    
    return setsockopt(can_sock, SOL_CAN_RAW, CAN_RAW_FILTER, filter, cnt * sizeof(struct can_filter));
}

/*******************************************************************************
 * @brief 读取CAN帧数据
 * @param can_sock   CAN通信套接字
 * @param pcanframe  读取到的CAN帧
 * @param timeout_ms 读取超时时间，单位毫秒
 * @return -1: failed; 0: timeout; >0: 读取到的CAN帧整体字节数
*******************************************************************************/
int can_read(int can_sock, struct can_frame* pcanframe, int timeout_ms)
{
    int nread = 0;
    fd_set fdset;
    struct timeval timeout = { timeout_ms / 1000, timeout_ms % 1000 * 1000 };
    int ret = 0;

    I64 curr_ms = 0;
    I64 start_ms = get_timestamp_ms();
    while(1) {
        FD_ZERO(&fdset);
        FD_SET(can_sock, &fdset);
        ret = select(can_sock + 1, &fdset, NULL, NULL, &timeout);
        if(-1 == ret) {
            //LogWarn("can select error:%s", strerror(errno));
            curr_ms = get_timestamp_ms();
            if(curr_ms > start_ms + timeout_ms)
                break;
            
            timeout.tv_sec = timeout_ms / 1000;
            timeout.tv_usec = timeout_ms % 1000 * 1000;
            continue;
        } else if(0 == ret) {
            /*　超时没有数据 */
            //LogDebug("can select timeout, no can data");
            break;
        } else {
            if(FD_ISSET(can_sock, &fdset)) {
                nread = read(can_sock, pcanframe, sizeof(struct can_frame));
            }
            break;
        }
    }
    if(nread > 0)
        ret = nread;
    
    return ret;
}

int can_write(int can_sock, uint8_t* data, int len)
{
    return write(can_sock, data, len);
}

/*******************************************************************************
 * @brief 
 * @param can_sock
 * @param func
 * @param saddr
 * @param len
 * @return 0: success; 1: failed
*******************************************************************************/
int can_send_read_request(int can_sock, uint16_t func, uint16_t saddr, uint16_t len)
{
    int ret = 0;
    struct can_frame frame;

    frame.can_id = CAN_ID_READ_DATA_REQ;
    frame.data[0] = LO8_UINT16(func);
    frame.data[1] = HI8_UINT16(func);
    frame.data[2] = LO8_UINT16(saddr);
    frame.data[3] = HI8_UINT16(saddr);
    frame.data[4] = LO8_UINT16(len);
    frame.data[5] = HI8_UINT16(len);
    frame.data[6] = CAN_DATA_RESERVE_VALUE;
    frame.data[7] = CAN_DATA_RESERVE_VALUE;
    frame.can_dlc = 8;
    ret = write(can_sock, &frame, sizeof(frame));

    return (ret == sizeof(frame) ? 0 : 1);
}

/*******************************************************************************
 * @brief 接收读请求的响应数据
 * @param can_sock can通信套接字
 * @param func     功能码：遥测：03；遥信：02，遥控：05，遥调：0x10
 * @param rsp_data 响应数据存放存储空间
 * @param maxlen   存储空间最大元素个数(非字节数)
 * @return 失败：-1； 成功：实际读取到的响应数据(uint16)个数
*******************************************************************************/
int can_recv_read_response(int can_sock, uint16_t func, uint16_t* rsp_data, int maxlen)
{
    int ret = 0;
    struct can_frame cframe;
    uint16_t func_rsp = 0;
    uint16_t data_size = 0;
    uint16_t data_num = 0;
    uint16_t data_cnt = 0;
    uint16_t data;
    uint16_t sn = 0;
    uint16_t sn_rsp = 0;
    int i = 0;

    do {
        memset(&cframe, 0, sizeof(cframe));
        ret = can_read(can_sock, &cframe, 1000);
        if(ret <= 0) {
            LogError("read response[%d] %s", sn, ret == 0 ? "timeout" : "failed");
            ret = -1;
            break;
        }
        if(cframe.can_id != CAN_ID_READ_DATA_RSP || cframe.can_dlc != 8) {
            LogError("read response[%d] failed, can_id=%d can_dlc=%d data=%s", sn, cframe.can_id, cframe.can_dlc, hex2string(cframe.data, cframe.can_dlc, NULL, 0));
            ret = -1;
            break;
        }
        LogDebug("can read data[%s]", hex2string(cframe.data, cframe.can_dlc, NULL, 0));

        if(data_cnt == 0) {
            //收到的第一个响应帧
            func_rsp = COMBINE16_LITTLE(&cframe.data[0]);
            data_size = COMBINE16_LITTLE(&cframe.data[2]);
            data_num = data_size / sizeof(uint16_t);
            data = COMBINE16_LITTLE(&cframe.data[4]);
            rsp_data[data_cnt++] = data;
            sn_rsp = COMBINE16_LITTLE(&cframe.data[6]);
            LogDebug("can read No.%d response, func=0x%x data_size=%d, data_num=%d, data_cnt=%d, sn_rsp=%d", sn + 1, func_rsp, data_size, data_num, data_cnt, sn_rsp);
            if(func_rsp != func) {
                LogError("func type error, want[%u] got[%u]", func, func_rsp);
                ret = -1;
                break;
            } else if(sn_rsp != sn) {
                LogError("data sn error, want[%d] got[%d]", sn, sn_rsp);
                ret = -1;
                break;
            }
        } else {
            //收到的其他响应帧
            LogDebug("can read No.%d response, data_cnt=%d, sn_rsp=%d", sn + 1, data_cnt, sn_rsp);
            sn_rsp = COMBINE16_LITTLE(&cframe.data[6]);
            if(sn_rsp != sn) {
                LogError("data sn error, want[%d] got[%d]", sn, sn_rsp);
                ret = -1;
                break;
            }
            for(i = 0; i < 3 && data_cnt < data_num; i++) {
                data = COMBINE16_LITTLE(&cframe.data[2*i]);
                rsp_data[data_cnt++] = data;
            }
        }
        sn++;
    } while(data_cnt < data_num);
    
    return (ret == -1 ? ret : data_cnt);
}

int can_send_write_request(int can_sock, uint16_t func, uint16_t addr, uint16_t val)
{
    int ret = 0;
    struct can_frame frame;
    uint16_t func_rsp = 0;
    uint16_t addr_rsp = 0;
    uint16_t val_rsp = 0;

    frame.can_id = CAN_ID_WRITE_DATA_REQ;
    frame.data[0] = LO8_UINT16(func);
    frame.data[1] = HI8_UINT16(func);
    frame.data[2] = LO8_UINT16(addr);
    frame.data[3] = HI8_UINT16(addr);
    frame.data[4] = LO8_UINT16(val);
    frame.data[5] = HI8_UINT16(val);
    frame.data[6] = CAN_DATA_RESERVE_VALUE;
    frame.data[7] = CAN_DATA_RESERVE_VALUE;
    frame.can_dlc = 8;
    ret = write(can_sock, &frame, sizeof(frame));
    if(ret == -1) {
        LogError("can_send_write_request() write failed:%s", strerror(errno));
        return -1;
    }

    memset(&frame, 0, sizeof(frame));
    ret = can_read(can_sock, &frame, 1000);
    if(ret <= 0) {
        LogError("can_send_write_request() read rsp failed");
        return -1;
    }
    func_rsp = COMBINE16_LITTLE(&frame.data[0]);
    addr_rsp = COMBINE16_LITTLE(&frame.data[2]);
    val_rsp = COMBINE16_LITTLE(&frame.data[4]);
    if(func_rsp != func || addr_rsp != addr || frame.can_id != CAN_ID_WRITE_DATA_RSP) {
        LogError("can_send_write_request() rsp frame data wrong, can_id[%X, %X] func[%d, %d], addr[%d, %d]"
            , CAN_ID_WRITE_DATA_RSP, frame.can_id, func, func_rsp, addr, addr_rsp);
        return -1;
    } else if(val_rsp != val) {
        LogError("can_send_write_request() rsp write failed, value not update");
        return -1;
    }
    
    return 0;
}
