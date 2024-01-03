// gcc -Wall -g -I/usr/include/curl -lcurl test.c http.c buffer.c -o test

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "http.h"
#include "debug_and_log.h"
#include "dlog.h"

#define dtlog(fmt, args...)		logmsg(__FILE__, __LINE__, 4, fmt, ##args)
#define dtdebug(fmt, args...)	logmsg(__FILE__, __LINE__, 4, fmt, ##args)
#define dtinfo(fmt, args...)		logmsg(__FILE__, __LINE__, 3, fmt, ##args)
#define dtwarn(fmt, args...)		logmsg(__FILE__, __LINE__, 2, fmt, ##args)
#define dterror(fmt, args...)	logmsg(__FILE__, __LINE__, 1, fmt, ##args)

#define CS_DEBUG_LEVEL_FILE     "/home/root/debug/debug_testhttp"
#define CS_LOG_LEVEL_FILE       "/home/root/debug/log_testhttp"
#define CS_LOG_FILE             "/home/root/log/testhttp.log"
#define CS_LOG_MAX_SIZE         100000

app_log_t th_log;

/*********************************************************************
* @fn	    resouce_clean
* @brief    进程资源回收
* @param    void
* @return   void
* @history:
**********************************************************************/
static void resource_clean(void)
{
    dbg_close();
    log_close(&th_log);
}

/*********************************************************************
* @fn	    sig_catch
* @brief    信号处理函数
* @param    sig_num[in]: 信号
* @return   void
* @history:
**********************************************************************/
static void sig_catch(int signum)
{
    int ret = 0;

    printf(">>>>connect internet process catches a signal [%d]\n", signum);

    switch(signum) {
    case SIGINT:
        resource_clean();
        exit(0);
    case SIGUSR2:
        break;

    default:
        break;
    }
}

/*********************************************************************
* @fn	    resource_init
* @brief    进程资源初始化
* @param    void
* @return   0: success -1: fail
* @history:
**********************************************************************/
static int resource_init(void)
{
    int ret = 0;

    ret = DBG_OPEN(CS_DEBUG_LEVEL_FILE, LY_DEBUG_OFF);
    if(ret) {
        printf("dbg_open fail!\n");
        return -1;
    }

    ret = log_open(&th_log, CS_LOG_FILE, CS_LOG_LEVEL_FILE, CS_LOG_MAX_SIZE, LOG_LV_ERROR);
    if(ret) {
        printf("LOG_OPEN fail!\n");
        return -1;
    }

    signal(SIGINT,  sig_catch);
    signal(SIGUSR1, sig_catch);
    signal(SIGUSR2, sig_catch);

    return 0;
}

void test_buffer()
{
	dtlog("test buffer start");
	buffer_t* buff = buffer_new();
    dtlog("test buffer_new()");
	buffer_print(buff);
	buffer_free(buff);
	
	buff = buffer_new_with_string("test buffer_new_with_string()");
	buffer_print(buff);
	buffer_clear(buff);
	dtlog("after buffer_clear()");
	buffer_print(buff);
	int resize = 12;
	dtlog("after buffer_clear() resize to %d", resize);
	buffer_resize(buff, resize);
	buffer_print(buff);
	buffer_free(buff);
	
    dtlog("test buffer_new_with_size()");
	buff = buffer_new_with_size(8);
	buffer_print(buff);
	buffer_append(buff, "test buffer module method: buffer_resize()");
	buffer_print(buff);
	buffer_appendf(buff, "; test buffer module method: %s, num=%d", "buffer_appendf()", 100);
	buffer_print(buff);
	buffer_free(buff);
	
	buff = buffer_new_with_string("  	 test buffer_trim()     ");
	buffer_print(buff);
	buffer_trim(buff);
	buffer_print(buff);
	buffer_free(buff);

    dtlog("buffer module test done");
}

void test_http()
{
	dtlog("test http start");
    http_init();
    http_t* http = http_create();
    if(http == NULL)
    {
        dterror("http_create failed return NULL");
        return;
    }
	dtlog("http init success");

    buffer_t* rsp = buffer_new_with_size(1024);
    buffer_t* data = buffer_new_with_size(1024);
    //http_get(http, "http://httpbin.org/get", rsp); 
    //http_get(http, "http://www.baidu.com", rsp); 
    //printf("http get rsp[%s]\n", rsp->data);

	buffer_clear(rsp);
    buffer_append(data, "{\"hello\": \"world\"}"); 
    http_add_header(http, "Connection: close");
	http_add_header(http, "Content-Type: application/json;charset=UTF-8");
    http_post(http, "http://httpbin.org/post", data->data, data->len, rsp);
    printf("http post rsp[%s]\n", rsp->data);
	//todo repost data use same http object

	buffer_clear(rsp);
	http_post(http, "http://47.92.86.26/api/weld/iothub/data", data->data, data->len, rsp);
    printf("http post to suxin rsp[%s]\n", rsp->data);
	
    //todo test https post
	buffer_clear(rsp);
	http_enable_tls(http);
    http_post(http, "https://httpbin.org/post", data->data, data->len, rsp);
    printf("https post rsp[%s]\n", rsp->data);
	
	buffer_clear(rsp);
	char auth_header[1024]="Authorization: Bearer eyJhbGciOiJIUzUxMiJ9.eyJzdWIiOiIxIiwiaW5mbyI6eyJ1c2VySWQiOjEsInVzZXJuYW1lIjoiYWRtaW4iLCJyb2xlSWQiOjEsImZpcm1JZCI6MSwib3BzIjpmYWxzZSwiZGV2Ijp0cnVlLCJwbGF0Zm9ybU1hbmFnZXIiOmZhbHNlLCJmaXJtTWFuYWdlciI6ZmFsc2V9LCJpYXQiOjE1NjQzODI4NTMsImV4cCI6MTY1ODk5MDg1M30.xFB2v_Tr2KVW4qOmhMHjGqx8sVfqnw7WmK7qWy1A38GobPRMPOddSnFQOM7J-eV__-A_-mdMu056lIqaX_wdaw";
	http_add_header(http, auth_header);
	http_post(http, "https://demo.linygroup.com/api/weld/iothub/data", data->data, data->len, rsp);
    printf("https post to suxin rsp[%s]\n", rsp->data);

	
    free(rsp);
    free(data);
    http_cleanup(http);
}

int main(void)
{
	resource_init();

    test_buffer();

    test_http();

    return 0;
}