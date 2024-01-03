#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>

#include "util.h"
#include "common.h"
#include "suxin_database.h"
#include "debug_and_log.h"

#define DEBUG_LEVEL_FILE     "/home/root/debug/debug_test"
#define LOG_LEVEL_FILE       "/home/root/debug/log_test"
#define LOG_FILE             "/home/root/log/test.log"
#define LOG_MAX_SIZE         100000

app_log_t log_test;


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
    log_close(&log_test);
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
    //int ret = 0;

    printf(">>>>connect internet process catches a signal [%d]\n", signum);

    switch(signum) {
    case SIGINT:
        resource_clean();
        exit(0);
    case SIGUSR1:
        break;
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

    ret = DBG_OPEN(DEBUG_LEVEL_FILE, LY_DEBUG_OFF);
    if(ret) {
        printf("dbg_open fail!\n");
        return -1;
    }

    ret = resource_init_common();
    if(ret) {
        printf("resource_init_common fail!\n");
        return -1;
    }

    ret = log_open(&log_test, LOG_FILE, LOG_LEVEL_FILE, LOG_MAX_SIZE, LOG_LV_ERROR);
    if(ret) {
        printf("LOG_OPEN fail!\n");
        return -1;
    }

    print_core_limit();

    signal(SIGINT,  sig_catch);
    signal(SIGUSR1, sig_catch);
    signal(SIGUSR2, sig_catch);

    return 0;
}

int main(void)
{
    int ret = 0;
    char str_buff[1024] = {0};
    int num = 0;
    int padding = 16;

    printf("test database lib process\n");
    ret = resource_init();
    if(ret) {
        resource_clean();
        printf("resource init failed，clean resource\n");
        return -1;
    }

    ret = db_open_database(DATABASE_PATH_DATA);
	if (ret)
	{
		DERROR("db_open_database fail");
		return -1;
	}
    printf("init done, open database file success, start test\n");

    ret = db_get_string_config("plat_addr", str_buff, sizeof(str_buff) - 1, "0.0.0.123");
	DINFO("get string config %-*s=%s, ret = %d", padding, "plat_addr", str_buff, ret);
    ret = db_get_number_config("plat_port", &num, 10086);
	DINFO("get number config %-*s=%d, ret = %d", padding, "plat_port", num, ret);

    ret = task_query_count();
    DINFO("query task count ret = %d", ret);

    task_t task;
    task.task_id = 1001;
    strcpy(task.task_name, "task1001");
    task.plan_num = 100;
    task.cur_num = 0;
    task.state = 0;
    ret = task_insert(&task, 0);
    DINFO("insert task[%d] ret = %d", task.task_id, ret);
    ret = task_query_count();
    DINFO("query task count ret = %d", ret);

    task.cur_num = 23;
    task.state = 1;
    ret = task_update(&task);
    DINFO("update task[%d] ret = %d", task.task_id, ret);

    task_t task2 = {1001, {'\0'}, 0, 0, 0};
    ret = task_query(&task2);
    DINFO("query task[%d] ret = %d", task.task_id, ret);
    if(ret == 1)
    {
        int task_pad = 10;
        DINFO("%-*s = %d", task_pad, "task_id", task2.task_id);
        DINFO("%-*s = %s", task_pad, "task_name", task2.task_name);
        DINFO("%-*s = %d", task_pad, "plan_num", task2.plan_num);
        DINFO("%-*s = %d", task_pad, "cur_num", task2.cur_num);
        DINFO("%-*s = %d", task_pad, "state", task2.state);
    }

    ret = task_delete(task.task_id);
    DINFO("delete task[%d] ret = %d", task.task_id, ret);
    ret = task_query_count();
    DINFO("query task count ret = %d", ret);

    return 0;
}