#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "msq.h"

int GetMessageQueue(key_t key, int flag) {
	int ret = 0;
	if((ret = msgget(key, flag)) == -1) {
		return -1;
	}

	return ret;
}

int CreateMessageQueue(key_t key) {
	return GetMessageQueue(key, IPC_CREAT | IPC_EXCL | 0666);
}

int GetMessageQueueId(key_t key) {
	return GetMessageQueue(key, IPC_CREAT | 0666);
}

int DestroyMessageQueue(int msqid) {
	if(msgctl(msqid, IPC_RMID, NULL) == -1) {
		perror("msgctl");
		return -1;
	}

	printf("Destroy message queue(id:%d) success!\n", msqid);
	return 0;
}

int SendMessage(int msqid, void* msg, size_t len, int who)
{
	msgbuf_t buf;

	if(len > MAX_MSG_SIZE) {
		printf(">>>>message length is too long!\n");
		return -1;
	}

	memset(&buf, 0, sizeof(buf));

	buf.mtype = who;

	memcpy(buf.mtext, msg, len);

	if(msgsnd(msqid, &buf, len, IPC_NOWAIT) == -1) {
		perror("msgsnd");
		return -1;
	}

	return 0;
}

int SendMessageNew(int msqid, void* msg, size_t len, int who) {
	msgpbuf_t buf;

	if(len > MAX_MSG_SIZE) {
		errno = E2BIG;
		return -1;
	}

	buf.mtype = who;
	buf.pmtext = msg;
	if(msgsnd(msqid, &buf, len, IPC_NOWAIT) == -1) {
		return -1;
	}

	return 0;
}

int ReceiveMessage(int msqid, void* msg, int len, int who)
{
	msgbuf_t buf;
	int ret = 0;

	if(len > MAX_MSG_SIZE) {
		errno = E2BIG;
		return -1;
	}

	memset(&buf, 0, sizeof(buf));

	ret = msgrcv(msqid, &buf, len, who, IPC_NOWAIT | MSG_NOERROR);
	if(ret == -1) {
		return -1;
	}

	memcpy(msg, (void*)buf.mtext, len);

	return ret;
}

/*******************************************************************************
 * @brief
 * @param msqid
 * @param msg
 * @param len
 * @param who
 * @return -1: failed; on success return receive byte number
*******************************************************************************/
int ReceiveMessageNew(int msqid, void* msg, int len, int who)
{
	msgpbuf_t buf;
	int ret = 0;

	if(len > MAX_MSG_SIZE) {
		errno = E2BIG;
		return -1;
	}

	buf.pmtext = msg;
	ret = msgrcv(msqid, &buf, len, who, IPC_NOWAIT | MSG_NOERROR);
	if(ret == -1) {
		return -1;
	}

	return ret;
}

/*******************************************************************************
 * @brief 阻塞式接收消息（带超时时间）
 * @param msqid
 * @param msg
 * @param len
 * @param who
 * @param timeout_ms
 * @return -1: failed; 0: timeout; >0: receive msg size
*******************************************************************************/
int ReceiveMessageWait(int msqid, void* msg, int len, int who, int timeout_ms)
{
	msgpbuf_t buf;
	int ret = 0;
	int check_interval = 200;
	int check_times = timeout_ms / check_interval;
	int i = 0;

	if(len > MAX_MSG_SIZE) {
		errno = E2BIG;
		return -1;
	}

	if(timeout_ms > 0) {
		for(i = 0; i < check_times; i++) {
			buf.pmtext = msg;
			ret = msgrcv(msqid, &buf, len, who, IPC_NOWAIT | MSG_NOERROR);
			if(ret > 0)
				break;
			usleep(check_interval * 1000);
		}
	} else {
		ret = msgrcv(msqid, &buf, len, who, 0);
	}

	if(ret == -1) {
		return -1;
	}

	return ret;
}
