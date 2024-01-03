#ifndef __MESSAGE_QUEUE_H
#define __MESSAGE_QUEUE_H
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>


#define MAX_MSG_SIZE 8192 

#define MQ_KEY_CAN		0x866001	//Websocket client msg queue
#define MQ_KEY_IPC		0x866002	//suxin apps communication msg queue


//mtype enum
enum MTYPE
{
	MT_MIN = 0, 
	MT_CAN_SET_VAL_REQ = 1,	//104 set value to dsp by can interface request
	MT_CAN_SET_VAL_RSP = 2,	//104 set value to dsp by can interface response
	MT_CAN_IPC_REQ,			//其他进程发送CAN数据请求
	MT_CAN_IPC_RSP,			//其他进程发送CAN数据响应
	MT_MAX,
};


//suxin apps 进程间通信相关
enum msg_event
{
	MSG_EVENT_MIN = 1000,
	MSG_EVENT_UNKNOWN,				//1001
	MSG_EVENT_THREAD_INFO,			//1002
	MSG_EVENT_MEM_INFO_ALL,			//1003
	MSG_EVENT_MEM_INFO_RUN,			//1004
	MSG_EVENT_MEM_INFO_INIT,		//1005
	MSG_EVENT_MEM_POOL_INFO,		//1006
	MSG_EVENT_LWRCACHE_INFO,		//1007
	MSG_EVENT_LOG_MEM_DBUG,			//向日志中输出记录的内存非配位置和次数,1008
	MSG_EVENT_FLUX_VERIFY,			//流量完整性核�?1009
	MSG_EVENT_VERSION,				//输出版本�?1010
	MSG_EVENT_RECONF,				//重读配置,1011
	MSG_EVENT_LOG_ALL = 3000,
	MSG_EVENT_LOG_DEBUG = 3001,
	MSG_EVENT_LOG_INFO,
	MSG_EVENT_LOG_WARN,
	MSG_EVENT_LOG_ERROR,
	MSG_EVENT_LOG_NONE,				//3005
	MSG_EVENT_MAX
};
enum msg_type
{
	MSG_TYPE_DEFAULT = 1000,
	MSG_TYPE_REQ,
	MSG_TYPE_RSP,
	MSG_TYPE_MAX
};


typedef struct msgbuf {
	long mtype;
	char mtext[MAX_MSG_SIZE];
}msgbuf_t;
typedef struct msgpbuf {
	long mtype;
	char* pmtext;
}msgpbuf_t;


// typedef struct
// {
// 	int		 event;
// 	uint32_t datalen;
// 	uint8_t  msgdata[MAX_MSG_DATA_LEN];
// }event_msg_t;
//suxin apps 进程间通信相关--结束


int GetMessageQueue(key_t key, int flag);
int CreateMessageQueue(key_t qid);
int GetMessageQueueId(key_t qid);
int DestroyMessageQueue(int msqid);
int SendMessage(int msqid, void* msg,size_t len,int who);
int ReceiveMessage(int msqid, void* msg, int len, int who);
int ReceiveMessageWait(int msqid, void* msg, int len, int who, int timeout_ms);

#endif


