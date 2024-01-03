/***************************************************************************
 * Copyright (C), All rights reserved.
 * File        :  log.c
 * Decription  :  日志实现
 * Author      :  zhums
 * Version     :  v1.0
 * Date        :  2018/9/28
 * Histroy	   :
 **************************************************************************/

/****************************** INCLUDE ********************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <sys/syscall.h>
#include "log.h"


app_log_t* g_plog;
const char levelStr[][8] = { "MUST", "IMPT", "ERROR", "WARN", "INFO", "DEBUG", "TRACE", "EMSHMI", "BMS", "PCS", "EMANA"
						   , "USER4", "USER5", "", "" };

/*****************************************************************************
 * 函 数 名  : splitStringToArray
 * 负 责 人  : zhums
 * 创建日期  : 2018年6月17日
 * 函数功能  : 已指定字符(串)分割目的串, 结果放在数组中(存放的是指针,值在目
			   的串中,且目的串会被修改)
 * 输入参数  : char *pstr    待分割字符串
			   char *psplit  分隔串
			   char **parr   分割结果数组
			   int maxcnt    返回记录最大个数
 * 输出参数  : 无
 * 返 回 值  : 实际分割的次数
 * 调用关系  :
 * 其    它  :
*****************************************************************************/
static int splitStringToArray(char* pstr, char* psplit, char** parr, int maxcnt)
{
	if(NULL == pstr || NULL == psplit || NULL == parr || 0 == strlen(pstr))
		return 0;

	char* pcur = pstr;
	char* pfind = NULL;
	int cnt = 0;
	int offset = strlen(psplit);
	while((pfind = strstr(pcur, psplit)) && cnt < maxcnt) {
		parr[cnt++] = pcur;
		*pfind = '\0';
		pcur = pfind + offset;
	}
	if(cnt < maxcnt) {
		parr[cnt++] = pcur;
	}

	return cnt;
}

/*********************************************************************
* @fn       get_log_day_time
* @brief  	get the current time from system
* @param  	dst: time
* @return	len
*/
int get_log_day_time(char *dst)
{
    int len = 0;
    time_t rsec;
    struct tm t;

    tzset();
    rsec = time(NULL);
    localtime_r(&rsec, &t);
    len = sprintf(dst, "%4d-%d-%d %02d:%02d:%02d ", t.tm_year+1900,
     			t.tm_mon+1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);

    if(len < 0) {
        len = 0;
    }
    return len;
}

static int check_make_dir(char* dirname)
{
	char cmd[64] = { 0 };
	DIR* dirp = NULL;

	if(NULL == dirname)
        return 1;

	snprintf(cmd, sizeof(cmd), "mkdir -p %s", dirname);
	system(cmd);

    if((dirp = opendir(dirname)) == NULL)
    {
        return 1;
	}
	closedir(dirp);

    return 0;
}

static char* get_file_name(char* pathname, char* fname, int maxlen)
{
	int len = 0;
	char *ptmp = NULL;

	len = strlen(pathname);
	ptmp = pathname + (len - 1);
	while(ptmp != pathname && *ptmp != '/' && *ptmp != '\\')
		ptmp--;

	if(*ptmp == '/' || *ptmp == '\\')
		ptmp++;

	snprintf(fname, maxlen, "%s", ptmp);

	return fname;
}

static char* dot_to_underline(char* pathname)
{
	char *ptmp = NULL;

	ptmp = pathname;
	while(*ptmp != '\0')
	{
		if(*ptmp == '.')
			*ptmp = '_';
		ptmp++;
	}

	return pathname;
}


int get_log_level(app_log_t* plog)
{
	int ret;
	int log_level = LOG_LV_WARN;
	char level_buf[8] = {0};

	ret = lseek(plog->level_fd, 0, SEEK_SET);
	if( -1 == ret ) {
		perror("log_level_lseek set failed");
        return log_level;
	}
	ret = read(plog->level_fd, level_buf, sizeof(level_buf));
	if((ret == -1) || (ret == 0)) {
		perror("log_level_lseek read failed");
		return log_level;
	}
	level_buf[ret]='\0';
	log_level = atoi(level_buf);

	if(log_level > LOG_LV_TRACE) {
		log_level = LOG_LV_TRACE;
	} else if(log_level < LOG_LV_OFF) {
		log_level = LOG_LV_OFF;
	}

	return log_level;
}


/*********************************************************************
* @fn       lock_file
* @brief  	对文件加独占写锁
* @param  	fd[in]: 文件描述符
* @return	-1：失败 0: 成功
*/
int lock_file(int fd)
{
    int ret;
    struct flock lock;

    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;

    ret = fcntl(fd, F_SETLKW, &lock);

    return ret;
}


/*********************************************************************
* @fn       unlock_file
* @brief  	解锁
* @param  	fd[in]: 文件描述符
* @return	-1：失败 0: 成功
*/
int unlock_file(int fd)
{
    int ret;
    struct flock lock;

    lock.l_type = F_UNLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;

    ret = fcntl(fd, F_SETLK, &lock);

    return ret;
}

int lock_thread(app_log_t* plog)
{
	return pthread_mutex_lock(&plog->mutex_lock);
}
int unlock_thread(app_log_t* plog)
{
	return pthread_mutex_unlock(&plog->mutex_lock);
}

int lock_process(app_log_t* plog)
{
	return lock_file(plog->lock_fd);
}
int unlock_process(app_log_t* plog)
{
	return unlock_file(plog->lock_fd);
}


/*********************************************************************
* @fn       log_open
* @brief  	打开日志文件
* @param  	log[in]：log结构体
* @return	-1：失败 0: 成功
*/
inline int log_open(app_log_t* plog, char* logfname, char* levelfile, int max_log_size, int level, enum LOGLV_MODE log_level_mode)
{
	return log_open_ex(plog, logfname, levelfile, max_log_size, level, LK_THREAD, log_level_mode);
}

int log_open_ex(app_log_t* plog, char* logfname, char* levelfile, int max_log_size, int level, enum LOCK_MODE lk_mode, enum LOGLV_MODE log_level_mode)
{
	int ret = -1;
	int has_log_level = 1;
	char buf[LOG_LEVEL_STR_LEN];
	int len = 0;

	if((logfname == NULL) || (levelfile == NULL)) {
		printf("invalid params\n");
		return -1;
	}
	memset(plog, 0, sizeof(app_log_t));
	g_plog = plog;
	plog->log_level_mode = log_level_mode;
	plog->write_log_level = level;
	plog->log_max_size = max_log_size;
	strncpy(plog->log_file, logfname, sizeof(plog->log_file));
	strncpy(plog->level_file, levelfile, sizeof(plog->level_file));
	len = snprintf(plog->lock_file, sizeof(plog->lock_file), "/etc/flock/");
	if(check_make_dir(plog->lock_file)!=0)
	{
		//printf("mkdir(%s) failed\n", plog->lock_file);
		return -1;
	}
	get_file_name(logfname, plog->lock_file + len, sizeof(plog->lock_file) - len);
	dot_to_underline(plog->lock_file);
	//printf("log[%s] lock file is[%s]\n", plog->log_file, plog->lock_file);

	if(lk_mode == LK_PROCESS)
	{
		plog->lock = lock_process;
		plog->unlock = unlock_process;
	}
	else
	{
		plog->lock = lock_thread;
		plog->unlock = unlock_thread;
	}

	plog->log_fd = open(plog->log_file, O_RDWR|O_CREAT, 0644);
	plog->level_fd = open(plog->level_file, O_RDWR|O_CREAT, 0644);
	plog->lock_fd = open(plog->lock_file, O_RDWR|O_CREAT, 0644);
	if((plog->log_fd < 0) || (plog->level_fd < 0) || (plog->lock_fd < 0)) {
		printf("open log or log level file failed\n");
		return -1;
	}

	/* 写默认的日志级别到日志级别文件 */
	/* add by zhums, 2020-10-31 */
	//判断日志级别文件中是否已设置了日志级别，若已设置则不再写入默认日志级别
	//否则手动修改日志级别后，还是会被修改为默认日志级别
	has_log_level = 1;
	ret = lseek(plog->level_fd, 0, SEEK_SET);
	if(-1 != ret ) {
		ret = read(plog->level_fd, buf,sizeof(buf));
		if(ret == 0) {
			has_log_level = 0;
		}
	}
	if(has_log_level == 0) {
		//日志级别文件内容为空，写入默认日志级别
		len = sprintf(buf, "%d", level);
		ret = write(plog->level_fd, buf, len);
		if(ret != len){
			printf("write failed \n");
			return -1;
		}
	}

	/* 将读写位置移到文件尾 */
	ret = lseek(plog->log_fd, 0, SEEK_END);
	if( -1 == ret ) {
		printf("log lseek failed");
		return -1;
	}

	pthread_mutex_init(&plog->mutex_lock, NULL);

	return 0;

}

/*********************************************************************
* @fn       log_close
* @brief  	关闭日志文件
* @param  	log[in]：log结构体
* @return	-1：失败 0: 成功
*/
void log_close(app_log_t* plog)
{
    close(plog->log_fd);
    plog->log_fd = -1;
    pthread_mutex_destroy(&plog->mutex_lock);
}

/********************************************************************************
 * Function:       log_write_inner
 * Description:    写日志
 * Prototype:      int log_write_inner(app_log_t* plog, int level, char *logbuff, int len)
 * Param:          app_log_t* plog   - 日志对象
 * Param:          char *logbuff     - 已格式化的日志内容
 * Param:          int len           - logbuff的长度
 * Return:         0: success; 1: failed
********************************************************************************/
int log_write_inner(app_log_t* plog, char *logbuff, int len)
{
    char cmd[512] = {0};
    int file_size = 0;
    int iret = 0;

    if(plog->lock(plog) != 0)
	{
		//printf("log lock failed\n");
		return -1;
	}
	lseek(plog->log_fd, 0, SEEK_END); 				//文件指针定位到文件尾，多进程间必须

	//检查日志文件大小，当日志文件达到最大大小时，覆盖最早1/4的内容并从文件尾继续写日志
	file_size = lseek(plog->log_fd, 0, SEEK_CUR);
    if(file_size + len > plog->log_max_size)
	{
        lseek(plog->log_fd, 0, SEEK_SET);
        snprintf(cmd, sizeof(cmd), "tail -c %d %.200s > %.200s_tmp", plog->log_max_size*1/2, plog->log_file, plog->log_file);
        system(cmd);
        close(plog->log_fd);//文件关闭，文件锁自动释放，所以lock_fd不能是log_fd;
        plog->log_fd = -1;
        snprintf(cmd, sizeof(cmd), "rm %s", plog->log_file);
        system(cmd);
        snprintf(cmd, sizeof(cmd), "mv %.200s_tmp %.200s", plog->log_file, plog->log_file);
        system(cmd);
        plog->log_fd = open(plog->log_file, O_RDWR|O_CREAT, 0644);
        lseek(plog->log_fd, 0, SEEK_END);
    }

	//将日志内容写入文件
    write(plog->log_fd, logbuff, len);
	fsync(plog->log_fd);
    iret = 0;
    plog->unlock(plog);

    return iret;
}

/********************************************************************************
 * Function:       log_write
 * Description:    写日志
 * Prototype:      int log_write(app_log_t* plog, int level, char* file, int line, char *fmt, ...)
 * Param:          app_log_t* plog   - 日志对象
 * Param:          int level         - 日志级别
 * Param:          char* file        - 日志所在文件名
 * Param:          int line          - 日志所在行数
 * Param:          char *fmt         - 日志格式化字符串
 * Param:          ...               - 可变参数部分
 * Return:         0: success; 1: failed
********************************************************************************/
int log_write(app_log_t* plog, int level, char* file, int line, char *fmt, ...)
{
    char buf[MAX_LOG_STR_LEN] = {0};
	va_list args;
    int pos = 0;
    int default_level = 0;
    int iret = 0;

    /* 读取默认的日志级别 */
	default_level = get_log_level(plog);
    /* 根据日志级别决定是否写日志 */
	//level = level < 0 ? LOG_LV_INFO : level >= LOG_LV_ALL ? LOG_LV_DEBUG : level;
	if((plog->log_level_mode == LOGLV_MODE_LINEAR && level > default_level)
		|| (plog->log_level_mode == LOGLV_MODE_SELECT && !plog->log_level_sets[level])) {
        return 0;
	}

	//格式化日志内容
	pos += get_log_day_time(buf);
	pos += sprintf(buf + pos, "[%6.6s:%-3d %s T%lu] ", file, line, levelStr[level], syscall(__NR_gettid));
	va_start(args, fmt);
	pos += vsnprintf(buf + pos, sizeof(buf) - pos - 1, fmt, args);
	va_end(args);
    
	//日志尾添加\n
	if(pos + 2 >= MAX_LOG_STR_LEN) {
		pos = MAX_LOG_STR_LEN - 3;
	}
	if(buf[pos - 1] != '\n') {
		buf[pos++] = '\n';
		buf[pos] = '\0';
	}

    iret = log_write_inner(plog, buf, pos);

    return iret;
}

int log_write_ex(app_log_t* plog, int level, char* file, int line, u_int8_t with_date, u_int8_t newline, char* fmt, ...)
{
	char buf[MAX_LOG_STR_LEN] = { 0 };
	va_list args;
	int pos = 0;
	int default_level = 0;
	int iret = 0;

	/* 读取默认的日志级别 */
	default_level = get_log_level(plog);
	/* 根据日志级别决定是否写日志 */
	//level = level < 0 ? LOG_LV_INFO : level >= LOG_LV_ALL ? LOG_LV_DEBUG : level;
	if((plog->log_level_mode == LOGLV_MODE_LINEAR && level > default_level)
		|| (plog->log_level_mode == LOGLV_MODE_SELECT && !plog->log_level_sets[level])) {
		return 0;
	}

	//格式化日志内容
	if(with_date) {
		pos += get_log_day_time(buf);
		pos += sprintf(buf + pos, "[%s:%d %s T%lu] ", file, line, levelStr[level], syscall(__NR_gettid));
	}
	va_start(args, fmt);
	pos += vsnprintf(buf + pos, sizeof(buf) - pos - 1, fmt, args);
	va_end(args);

	//日志尾添加\n
	if(newline) {
		if(pos + 2 >= MAX_LOG_STR_LEN) {
			pos = MAX_LOG_STR_LEN - 3;
		}
		if(buf[pos - 1] != '\n') {
			buf[pos++] = '\n';
			buf[pos] = '\0';
		}
	}

	iret = log_write_inner(plog, buf, pos);

	return iret;
}

void set_app_log(app_log_t* plog)
{
	app_log_t* plog_tmp = NULL;

	if(g_plog == NULL)
		g_plog = plog;
	else if(g_plog != plog)
	{
		plog_tmp = g_plog;
		pthread_mutex_lock(&plog_tmp->mutex_lock);
		g_plog = plog;
		pthread_mutex_unlock(&plog_tmp->mutex_lock);
	}
}

void set_log_level_mode(app_log_t* plog, enum LOGLV_MODE lv_mode)
{
	if(plog == NULL || (lv_mode != LOGLV_MODE_LINEAR && lv_mode != LOGLV_MODE_SELECT))
		return;

	plog->log_level_mode = lv_mode;
}

void set_log_level(app_log_t* plog, int loglevel)
{
	if(plog->log_level_mode == LOGLV_MODE_LINEAR) {
		if(loglevel == LOG_NONE)
			plog->write_log_level = loglevel;
		else {
			loglevel = loglevel < 0 ? LOG_LV_INFO : loglevel > LOG_LV_ALL ? LOG_LV_DEBUG : loglevel;
			plog->write_log_level = loglevel;
		}
	}
}

/*******************************************************************************
 * @brief 
 * @param plog
 * @param lv_sets each enable log level split with ',' like "0,1,2,3,4,5,8"
 * @return 
*******************************************************************************/
void set_log_level_sets(app_log_t* plog, char* lv_sets)
{
	int i = 0, cnt = 0;
	int logLevel = 0;
	char* pResult[16] = { NULL };

	if(plog->log_level_mode == LOGLV_MODE_SELECT) {
		strncpy(plog->log_level_str, lv_sets, sizeof(plog->log_level_str));
		for(i = 0; i <= LOG_LV_ALL; i++) {
			plog->log_level_sets[i] = 0;
		}
		cnt = splitStringToArray(plog->log_level_str, ",", (char**)&pResult, 16);
		for(i = 0; i < cnt; i++) {
			logLevel = atoi(pResult[i]);
			logLevel = logLevel < 0 ? LOG_LV_INFO : logLevel > LOG_LV_ALL ? LOG_LV_DEBUG : logLevel;
			plog->log_level_sets[logLevel] = 1;
		}
		//split时内容已被破坏，重新赋值留存原字符串
		strncpy(plog->log_level_str, lv_sets, sizeof(plog->log_level_str));
	}
}

