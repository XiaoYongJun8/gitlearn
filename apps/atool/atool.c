/*******************************************************************************
 * @             Copyright (c) 2023 LinyPower, All Rights Reserved.
 * @*****************************************************************************
 * @FilePath     : atool.c
 * @Descripttion : 
 * @Author       : zhums
 * @E-Mail       : zhums@linygroup.cn
 * @Version      : v1.0.0
 * @Date         : 2023-11-26 22:58:39
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "atool.h"
#include "atool_msq.h"

int g_msgid = 0;

void usage(char* proc)
{
    printf("****** %s ******\n\n", ATOOL_VERSION);
    printf("%s [args...]\n", proc);
    printf("\t -h       : show help\n");
    printf("\t -v       : show ems version info\n");
    printf("\t -p       : show thread info\n");
    printf("\t reconf   : reload config\n");
    printf("\t stat     : show runtime and statistic info\n");
    printf("\t pcs      : show pcs info\n");
    printf("\t pcs_recfg: reload pcs db config\n");
    printf("\t send     : <event [msg]>. send event with msg, msg can be empty\n");
    printf("\t msgclr   : (mc) clear msessage in messages queue\n");
    printf("\t msgprt   : (mp) print process messages queue info\n");
    printf("\t msgget   : (mg) [req/rsp]. get one process message(req/rsp, default req) from messages queue\n");

    printf("\nlog event:\n");
    printf("3000:LOG_ALL; 3001:LOG_DEBUG; 3002:LOG_INFO; 3003:LOG_WARN; 3004:LOG_ERROR; 3005:LOG_NONE;\n");
    printf("3006:LOGLV_SETS <0,IMPT;1,FATAL;2,ERROR;3,WARN;4,INFO;5,DEBUG;6,TRACE;7,EMSHMI;8,BMS;9,PCS;10,EMANA;>\n");
}

int main(int argc, char** argv)
{
    int ret = 0;
    int evmsglen = 0;
    int timeout = 1;
    int tmargind = 0;
    char* procname = argv[0];
    event_msg_t evsnd = { 0, 0, {0} };
    event_msg_t evrcv = { 0, 0, {0} };

    if((g_msgid = atool_msq_get()) != 0) {
        AtoolLog("%s run failed!\n", procname);
        return ret;
    }

    if(argc > 1) {
        if(argc > 2) {
            tmargind = argc - 1;
            timeout = atoi(argv[tmargind]);
            if(timeout <= 0 || timeout >= 600)
                timeout = 1;
        }
        
        //printf("pid=%u,ppid=%u\n",getpid(),getppid());
        if(strcmp(argv[1], "-p") == 0 || strcmp(argv[1], "p") == 0) {
            //get thread msg
            evsnd.event = MSG_EVENT_THREAD_INFO;
            evsnd.datalen = 0;
            evmsglen = OFFSET(event_msg_t, msgdata) + evsnd.datalen;
            if((ret = atool_msq_send_msg(g_msgid, (char*)&evsnd, evmsglen, MSG_TYPE_REQ)) == 0) {
                atool_msq_recv_msg_timeout(g_msgid, (char*)&evrcv, sizeof(event_msg_t), MSG_TYPE_RSP, timeout);
                printf("%s\n", evrcv.msgdata);
            }
        } else if(strcmp(argv[1], "stat") == 0) {
            evsnd.event = MSG_EVENT_STAT_INFO;
            evsnd.datalen = 0;
            evmsglen = OFFSET(event_msg_t, msgdata) + evsnd.datalen;
            if((ret = atool_msq_send_msg(g_msgid, (char*)&evsnd, evmsglen, MSG_TYPE_REQ)) == 0) {
                atool_msq_recv_msg_timeout(g_msgid, (char*)&evrcv, sizeof(event_msg_t), MSG_TYPE_RSP, timeout);
                printf("%s\n", evrcv.msgdata);
            }
        } else if(strcmp(argv[1], "pcs") == 0) {
            evsnd.event = MSG_EVENT_PCS_INFO;
            evsnd.datalen = 0;
            evmsglen = OFFSET(event_msg_t, msgdata) + evsnd.datalen;
            if((ret = atool_msq_send_msg(g_msgid, (char*)&evsnd, evmsglen, MSG_TYPE_REQ)) == 0) {
                atool_msq_recv_msg_timeout(g_msgid, (char*)&evrcv, sizeof(event_msg_t), MSG_TYPE_RSP, timeout);
                printf("%s\n", evrcv.msgdata);
            }
        } else if(strcmp(argv[1], "pcs_recfg") == 0) {
            evsnd.event = MSG_EVENT_PCS_RECONF;
            evsnd.datalen = 0;
            evmsglen = OFFSET(event_msg_t, msgdata) + evsnd.datalen;
            if((ret = atool_msq_send_msg(g_msgid, (char*)&evsnd, evmsglen, MSG_TYPE_REQ)) != 0) {
                printf("send reload pcs db config command failed!\n");
            }
        } else if(strcmp(argv[1], "reconf") == 0) {
            //reload config
            evsnd.event = MSG_EVENT_RECONF;
            evsnd.datalen = 0;
            evmsglen = OFFSET(event_msg_t, msgdata) + evsnd.datalen;
            if((ret = atool_msq_send_msg(g_msgid, (char*)&evsnd, evmsglen, MSG_TYPE_REQ)) != 0) {
                printf("send reload config command failed!\n");
            }
        } else if(strcmp(argv[1], "send") == 0) {
            if(argc <= 2) {
                printf("%s command:send event, event cat not be empty. use -h for help\n", procname);
                return 1;
            }
            evsnd.event = atoi(argv[2]);
            if(argc > 3) {
                evsnd.datalen = sprintf((char*)evsnd.msgdata, "%s", argv[3]);
            }
            ret = atool_msq_send_event2(g_msgid, &evsnd);
            if(ret == 0) {
                if(!atool_msq_event_islog(evsnd.event)) {
                    atool_msq_recv_msg_timeout(g_msgid, (char*)&evrcv, sizeof(event_msg_t), MSG_TYPE_RSP, timeout);
                    printf("%s\n", evrcv.msgdata);
                } else {
                    printf("send[%d %s] success\n", evsnd.event, (char*)evsnd.msgdata);
                }
            } else {
                printf("send[%d %s] failed\n", evsnd.event, (char*)evsnd.msgdata);
            }
        } else if(strcmp(argv[1], "mc") == 0 || strcmp(argv[1], "msgclr") == 0) {
            atool_msq_clear(g_msgid);
            printf("clear success, here is current message queue info:\n");
            atool_msq_print_queue_info();
        } else if(strcmp(argv[1], "mp") == 0 || strcmp(argv[1], "msgprt") == 0) {
            printf("here is current message queue info:\n");
            atool_msq_print_queue_info();
        } else if(strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "help") == 0) {
            usage(procname);
        } else if(strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "version") == 0) {
            //get version info
            evsnd.event = MSG_EVENT_VERSION;
            evsnd.datalen = 0;
            evmsglen = OFFSET(event_msg_t, msgdata) + evsnd.datalen;
            if((ret = atool_msq_send_msg(g_msgid, (char*)&evsnd, evmsglen, MSG_TYPE_REQ)) == 0) {
                atool_msq_recv_msg_timeout(g_msgid, (char*)&evrcv, sizeof(event_msg_t), MSG_TYPE_RSP, timeout);
                printf("%s\n", evrcv.msgdata);
            }
        } else {
            printf("param is error!\n");
            usage(procname);
            return 1;
        }
        return 0;
    } else {
        usage(procname);
    }

    return 0;
}
