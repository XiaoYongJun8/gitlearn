/*******************************************************************************
 * @             Copyright (c) 2023 LinyPower, All Rights Reserved.
 * @*****************************************************************************
 * @FilePath     : atool_msq.c
 * @Descripttion : 
 * @Author       : zhums
 * @E-Mail       : zhums@linygroup.cn
 * @Version      : v1.0.0
 * @Date         : 2023-12-11 22:19:48
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "atool_msq.h"


#define STM_OFFSET(type, member)     ((size_t)&((type *)0)->member)		//struct member offset

static void SleepA(int sec, int usec)
{
	struct timeval tvTime;
	tvTime.tv_sec = sec;
	tvTime.tv_usec = usec;
	select(0, NULL, NULL, NULL, &tvTime);
}

/*******************************************************************************
 * @brief 
 * @return success: msqid; failed: -1
*******************************************************************************/
int atool_msq_get()
{
	int msqid = -1;

	if((msqid = msgget((key_t)ATOOL_MSQ_MAGIC, 0666 | IPC_CREAT)) < 0)
		return msqid;
	atool_msq_clear(msqid);
	return msqid;
}

void atool_msq_clear(int msqid)
{
	char buff[ATOOL_MSGLEN_MAX] = { 0 };
	while(msgrcv(msqid, buff, sizeof(buff), 0, MSG_NOERROR | IPC_NOWAIT) > 0)
		;
}

void atool_msq_print_queue_info(void)
{
	char cmd[128] = { 0 };
	sprintf(cmd, "ipcs -q|head -3|tail -2;ipcs -q|grep \"%x\";echo; ipcs -q -i 0|tail -5|head -4", ATOOL_MSQ_MAGIC);
	system(cmd);
}

/*******************************************************************************
 * @brief 
 * @param msqid
 * @param buffer
 * @param buflen
 * @param msgtype
 * @return 0: success; 1: failed
*******************************************************************************/
int atool_msq_send_msg(int msqid, char* buffer, int buflen, int msgtype)
{
	msg_t sendmsg;
	if(buffer == NULL)
		return 1;

	memset(&sendmsg, 0, sizeof(msg_t));
	sendmsg.msg_type = msgtype;
	memcpy(sendmsg.data, buffer, buflen);
	return msgsnd(msqid, (void*)&sendmsg, buflen, 0);
}

/*******************************************************************************
 * @brief 
 * @param msqid
 * @param buffer
 * @param buflen
 * @param msgtype
 * @param block
 * @return success: receive bytes; failed: -1
*******************************************************************************/
int atool_msq_recv_msg(int msqid, char* buffer, int buflen, int msgtype, int block)
{
	int nrcv = 0;
	int msgflag = MSG_NOERROR;
	msg_t recvmsg = { 0 };
	if(!block)
		msgflag |= IPC_NOWAIT;
	nrcv = msgrcv(msqid, (void*)&recvmsg, sizeof(recvmsg.data), msgtype, msgflag);
	if(nrcv < 0)
		return nrcv;

	if(nrcv < buflen)
		buflen = nrcv;
	memcpy(buffer, recvmsg.data, buflen);

	return nrcv;
}

/*******************************************************************************
 * @brief 
 * @param buffer
 * @param buflen
 * @param msgtype
 * @param timeout 超时时间，单位：秒
 * @return success: receive bytes; timeout: 0; failed: -1
*******************************************************************************/
int atool_msq_recv_msg_timeout(int msqid, char* buffer, int buflen, int msgtype, int timeout)
{
	int nrcv = 0;
	int msgflag = MSG_NOERROR | IPC_NOWAIT;
	msg_t recvmsg = { 0 };
	int interval = 100; //unit ms
	int times = timeout * 1000 / interval;

	while(times--) {
		nrcv = msgrcv(msqid, (void*)&recvmsg, sizeof(recvmsg.data), msgtype, msgflag);
		if(nrcv < 0) {
			SleepA(0, interval * 1000);
			continue;
		} else
			break;
	}
	if(times > 0) {
		if(nrcv < buflen)
			buflen = nrcv;
		memcpy(buffer, recvmsg.data, buflen);
	} else {
		nrcv = 0;
	}

	return nrcv;
}

int atool_msq_event_islog(int event)
{
	return ((MSG_EVENT_LOG_ALL <= event && event <= MSG_EVENT_LOG_NONE) || event == MSG_EVENT_LOGLV_SETS);
}

int atool_msq_send_event(int msqid, int event, char* msgbuf, int msglen)
{
	int evmsglen = 0;
	event_msg_t evsnd = { 0 };
	if(msglen < 0)
		msglen = 0;
	else if(msglen > sizeof(evsnd.msgdata)) {
		msglen = sizeof(evsnd.msgdata);
	}

	if(msgbuf && msglen > 0)
		memcpy(evsnd.msgdata, msgbuf, msglen);

	evsnd.event = event;
	evsnd.datalen = msglen;
	evmsglen = STM_OFFSET(event_msg_t, msgdata) + evsnd.datalen;

	return atool_msq_send_msg(msqid, (char*)&evsnd, evmsglen, MSG_TYPE_REQ);
}

int atool_msq_send_event2(int msqid, event_msg_t* pevmsg)
{
	int evmsglen = 0;
	if(pevmsg == NULL)
		return 1;

	evmsglen = STM_OFFSET(event_msg_t, msgdata) + pevmsg->datalen;
	return atool_msq_send_msg(msqid, (char*)pevmsg, evmsglen, MSG_TYPE_REQ);
}
