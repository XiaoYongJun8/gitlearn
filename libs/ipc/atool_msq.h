/*******************************************************************************
 * @             Copyright (c) 2023 LinyPower, All Rights Reserved.
 * @*****************************************************************************
 * @FilePath     : atool_msq.h
 * @Descripttion : 
 * @Author       : zhums
 * @E-Mail       : zhums@linygroup.cn
 * @Version      : v1.0.0
 * @Date         : 2023-11-26 22:21:18
*******************************************************************************/
#ifndef __ATOOL_MSQ_H__
#define __ATOOL_MSQ_H__
#include <stdint.h>

#define ATOOL_MSQ_MAGIC	 	0x6868668
#define ATOOL_MSGLEN_MAX	1024

enum msg_event
{
	MSG_EVENT_MIN = 1000,
	MSG_EVENT_UNKNOWN = 1000,   	//1001
	MSG_EVENT_THREAD_INFO,			//输出线程信息
	MSG_EVENT_STAT_INFO,			//输出采集的实时数据和统计信息
	MSG_EVENT_PCS_INFO,			    //输出各PCS信息：运行状态
	MSG_EVENT_VERSION,				//输出版本
	MSG_EVENT_RECONF,				//重读配置
	MSG_EVENT_PCS_RECONF,			//重读PCS数据库配置
	MSG_EVENT_LOG_ALL = 3000,
	MSG_EVENT_LOG_DEBUG,
	MSG_EVENT_LOG_INFO,
	MSG_EVENT_LOG_WARN,
	MSG_EVENT_LOG_ERROR,
	MSG_EVENT_LOG_NONE,				//3005
	MSG_EVENT_LOGLV_SETS,			//选择模式时：日志级别集
	MSG_EVENT_MAX
};

enum msg_type
{
	MSG_TYPE_DEFAULT = 1000,
	MSG_TYPE_REQ,
	MSG_TYPE_RSP,
	MSG_TYPE_MAX
};

typedef struct
{
	long int msg_type;
	unsigned char data[ATOOL_MSGLEN_MAX];
}msg_t;

typedef struct
{
	int		 event;
	uint32_t datalen;
	uint8_t	 msgdata[ATOOL_MSGLEN_MAX];
}event_msg_t;

int atool_msq_get();
void atool_msq_clear(int msqid);
void atool_msq_print_queue_info(void);
int atool_msq_send_msg(int msqid, char* buffer, int buflen, int msgtype);
int atool_msq_recv_msg(int msqid, char* buffer, int buflen, int msgtype, int block);
int atool_msq_recv_msg_timeout(int msqid, char* buffer, int buflen, int msgtype, int timeout);
int atool_msq_event_islog(int event);
int atool_msq_send_event(int msqid, int event, char* msgbuf, int msglen);
int atool_msq_send_event2(int msqid, event_msg_t * pevmsg);

#endif //__ATOOL_MSQ_H__
