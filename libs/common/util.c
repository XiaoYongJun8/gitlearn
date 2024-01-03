#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <linux/socket.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "util.h"
#include "debug_and_log.h"

double time_current_ms()
{
#ifdef _WIN32
	//    SYSTEMTIME st;
	//    GetLocalTime(&st);
	//    return (st.wMilliseconds*1.0);
	return (GetTickCount() * 1.0);
#else
	struct timeval tv = { 0 };
	gettimeofday(&tv, NULL);
	return (tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0);
#endif
}

double time_diff_now(double stm_ms, int islog)
{
	double cur_ms = time_current_ms();
	double d = (cur_ms - stm_ms);
	if(d < 0) {
		printf("error diff time[%.3lf=%.3lf - %.3lf]\n", d, cur_ms, stm_ms);
	} else if(islog) {
		printf("diff time[%.3lf=%.3lf - %.3lf]\n", d, cur_ms, stm_ms);
	}

	return d;
}

double time_diff(double stm_ms, double etm_ms)
{
	return (etm_ms - stm_ms);
}

int do_system(char *str_cmd)
{
	return do_system_ex(str_cmd, NULL, 1);
}

int do_system_ex(char *str_cmd, int* cmd_retcode, int is_log)
{
    int status;

	if (NULL == str_cmd) {
        DBG_LOG_ERROR("Invalid params!\n");
		return -1;
	}

	if(cmd_retcode)
		*cmd_retcode = 0;
    status = system(str_cmd);
    if(status == -1) {
        DBG_LOG_ERROR("call system failed: %s" ,strerror(errno));
        return -1;
    } else {
		if(cmd_retcode!=NULL)
			*cmd_retcode = WEXITSTATUS(status);
        if(WIFEXITED(status)) {
            if (0 == WEXITSTATUS(status)) {
				if(is_log) DBG_LOG_INFO("[%s] run successfully", str_cmd);
            } else {
				if(is_log) DBG_LOG_ERROR("[%s] run fail, exit code [%d]", str_cmd,  WEXITSTATUS(status));
				return -1;
            }
        } else {
            if(is_log) DBG_LOG_INFO("exit status = [%d]", WEXITSTATUS(status));
            return -1;
        }
    }

    return 0;
}

void safe_sleep(long sec,long usec)
{
	struct timeval to;

	to.tv_sec = sec;
	to.tv_usec = usec;

	select(0,NULL,NULL,NULL,&to);
}

void safe_msleep(long msec)
{
	struct timeval to;

	to.tv_sec = msec / 1000;
	to.tv_usec = (msec % 1000) * 1000;

	select(0,NULL,NULL,NULL,&to);
}

I64 get_timestamp_ms()
{
	I64 ts_ms = 0;
	struct timeval tv = {0, 0};

	if(gettimeofday(&tv, NULL) != 0){
		LogWarn("gettimeofday error: %s", strerror(errno));
		return ts_ms;
	}

	//ts_ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;
	ts_ms = tv.tv_sec;
	ts_ms = ts_ms * 1000 + tv.tv_usec / 1000;
	return ts_ms;
}

char* current_date_time(char* strtime, int maxLen, char* datefmt)
{
    struct timeval tv = {0};
    struct tm* ptm = NULL;
    //int nrtn = 0;

    if(NULL == strtime || maxLen < 0)
        return strtime;

    memset(strtime, 0, maxLen);
    gettimeofday(&tv, NULL);
    if(NULL == (ptm = localtime(&tv.tv_sec)))
        return NULL;

    if(NULL == datefmt)
        strftime(strtime, maxLen, "%Y-%m-%d %H:%M:%S", ptm);
    else
        strftime(strtime, maxLen, datefmt, ptm);

    return strtime;
}

char* timestamp2string(time_t ts_sec, char* strtime, int maxLen, char* datefmt)
{
    struct tm* ptm = NULL;
 
    if(NULL == strtime || maxLen < 0)
        return strtime;

    memset(strtime, 0, maxLen);
    if(NULL == (ptm = localtime(&ts_sec)))
        return NULL;
	
    if(NULL == datefmt)
        strftime(strtime, maxLen, "%Y-%m-%d %H:%M:%S", ptm);
    else
        strftime(strtime, maxLen, datefmt, ptm);

    return strtime;
}

int find_pid_by_name(const char *pName)
{
    int szPid = -1;
	FILE *fp = NULL;
    char szProQuery[256] = {0};
	char szBuff[10] = {0};

	if (NULL == pName) {
        DBG_LOG_ERROR("Invalid params!\n");
		return -1;
	}

    sprintf(szProQuery,"ps -ef|grep '%s'|grep -v 'grep'|awk '{print $2}'",pName);  // 打开管道,执行shell命令

    fp = popen(szProQuery,"r");
	if (NULL == fp) {
        DBG_LOG_ERROR("popen fail! %s\n", strerror(errno));
		return -1;
	}

    while(fgets(szBuff,10,fp) != NULL) {
        szPid = atoi(szBuff);
        break;
    }

    pclose(fp); // 关闭管道指针,不是fclose()很容易混淆
    return szPid;
}

/*********************************************************************
* @fn	   get_net_info
*
* @brief   获取网络接口信息
*
* @param   szDevName[in]：网络接口名称
*          type[in]: 信息类型。
*          value[out]：值
*          len[in]: value的长度
*
* @return  -1：失败 0：成功
*
*/
int get_net_info(const char *szDevName, const int type, char *value, const int len)
{
    int s = socket(AF_INET, SOCK_DGRAM, 0);
	struct ifreq ifr;
	int ret = -1;
	struct sockaddr_in *p_sockaddr_in = NULL;

	if ((NULL == szDevName) || (NULL == value)) {
    	LOG(LOG_LV_ERROR, "Invalid params! %p, %d, %p", szDevName, type, value);
        goto RET;
    }

    if (s < 0) {
    	LOG(LOG_LV_ERROR, "Create socket failed! %s", strerror(errno));
        goto RET;
    }

	LOG(LOG_LV_TRACE, "szDevName: %s\n", szDevName);

	if (type == MAC) {
	    strcpy(ifr.ifr_name, szDevName);
	    if (ioctl(s, SIOCGIFHWADDR, &ifr) < 0) {
			LOG(LOG_LV_ERROR, "ioctl fail(SIOCGIFHWADDR): %s\n", strerror(errno));
	        goto RET;
	    }

		snprintf(value, len, "%02X-%02X-%02X-%02X-%02X-%02X",
             (unsigned char)ifr.ifr_hwaddr.sa_data[0],
             (unsigned char)ifr.ifr_hwaddr.sa_data[1],
             (unsigned char)ifr.ifr_hwaddr.sa_data[2],
             (unsigned char)ifr.ifr_hwaddr.sa_data[3],
             (unsigned char)ifr.ifr_hwaddr.sa_data[4],
             (unsigned char)ifr.ifr_hwaddr.sa_data[5]);
	    LOG(LOG_LV_TRACE, "MAC: %s\n", value);
	} else if (type == IP) {
	    strcpy(ifr.ifr_name, szDevName);
	    if (ioctl(s, SIOCGIFADDR, &ifr) < 0) {
            LOG(LOG_LV_ERROR, "ioctl fail(SIOCGIFADDR): %s\n", strerror(errno));
	        goto RET;
	    }

		p_sockaddr_in = (struct sockaddr_in *)&ifr.ifr_addr;
	    snprintf(value, len, "%s", inet_ntoa(p_sockaddr_in->sin_addr));
	    LOG(LOG_LV_TRACE, "IP: %s, inet_ntoa: %s\n", value, inet_ntoa(p_sockaddr_in->sin_addr));

	}

	else if (type == BRDIP)
	{
	    strcpy(ifr.ifr_name, szDevName);
	    if (ioctl(s, SIOCGIFBRDADDR, &ifr) < 0) {
            LOG(LOG_LV_ERROR, "ioctl fail(SIOCGIFBRDADDR): %s\n", strerror(errno));
	        goto RET;
	    }

		p_sockaddr_in = (struct sockaddr_in *)&ifr.ifr_broadaddr;
	    snprintf(value, len, "%s", inet_ntoa(p_sockaddr_in->sin_addr));
	    LOG(LOG_LV_TRACE, "BroadIP: %s, inet_ntoa: %s\n", value, inet_ntoa(p_sockaddr_in->sin_addr));
	}

	else if (type == NETMASK)
	{
	    strcpy(ifr.ifr_name, szDevName);
	    if (ioctl(s, SIOCGIFNETMASK, &ifr) < 0) {
            LOG(LOG_LV_ERROR, "ioctl fail(SIOCGIFNETMASK): %s\n", strerror(errno));
	        goto RET;
	    }

		p_sockaddr_in = (struct sockaddr_in *)&ifr.ifr_netmask;
	    snprintf(value, len, "%s", inet_ntoa(p_sockaddr_in->sin_addr));
	    LOG(LOG_LV_TRACE, "Netmask: %s\n", value);
	}

	ret = 0;

RET:
    close(s);
	return ret;
}

/*********************************************************************
* @fn	    get_lte_network_status
*
* @brief    获取LTE网络状态
*
* @param
*
* @return   1: connected, 0: disconnected, -1: fail
*
*/
int get_lte_network_status()
{
	FILE *fp = NULL;
	int ret = 0;
	char status[2] = {};

	memset(status,0,sizeof(status));

	if (access(LTE_NETWORK_STATUS,F_OK)) {
        LOG(LOG_LV_ERROR, "%s doesn't exist!\n", LTE_NETWORK_STATUS);
		return -1;
	} else {
		if (NULL == (fp = fopen(LTE_NETWORK_STATUS,"r"))) {
            LOG(LOG_LV_ERROR, "fopen %s fail!\n", LTE_NETWORK_STATUS);
			return -1;
		} else {
			ret = fread(status,sizeof(status), 1, fp);
			if (ret != 1) {
                LOG(LOG_LV_ERROR, "fread %s fail! %s\n", LTE_NETWORK_STATUS, strerror(errno));
				fclose(fp);
				return -1;
			} else {
				ret = atoi(status);
			}
		}
	}

	fclose(fp);
	fp = NULL;
	return ret;
}

/*********************************************************************
* @fn	   read_device_id
*
* @brief   获取设备ID
*
* @param   device_id[in]: 设备ID
*
* @return  0：成功 -1：失败
*
*/
int read_device_id(char device_id[DEVICE_ID_LEN+1])
{
	FILE *fp = NULL;
	char *path = "/home/root/routernode_deviceid";

	if (access(path,F_OK)) {
        LOG(LOG_LV_ERROR, "%s doesn't exist!\n", path);
		return -2;
	} else {
		if (NULL == (fp = fopen(path,"r"))) {
            LOG(LOG_LV_ERROR, "open %s fail!\n", path);
			return -1;
		} else {
			if (1 != fread(device_id, 9, 1, fp)) {
                LOG(LOG_LV_ERROR, "fread %s fail!\n", path);
				fclose(fp);
				return -1;
			}
		}
	}

	fclose(fp);
	return 0;
}

/*********************************************************************
* @fn	   remove_json_string_end_quot
*
* @brief   去除json字符串两端的双引号
*
* @param   json_string[in]：json字符串
*          new_string[out]：转化后的字符串
*          len[in]：new_string的长度
*
* @return  0：成功 -1：失败
*
*/
int remove_json_string_end_quot(const char *json_string, char *new_string, const int len)
{
	int m = 0;

	if (NULL == json_string || NULL == new_string || len == 0) {
        LOG(LOG_LV_ERROR, "Invalid params! %p, %p, %d\n", json_string, new_string, len);
		return -1;
	}

	memset(new_string, 0, len);

	m = (strlen(json_string) - 2) > (len-1) ? (len-1) : (strlen(json_string) - 2); //去除头尾两个引号的字符串长度

	strncpy(new_string, json_string+1, m);
	new_string[m] = '\0';

	return 0;
}

void wait_for_valid_time(char* proc_name)
{
    /* 时间不准确的数据不上报 */
	int retry_cnt = 1;

    while ((time(NULL) < TIME_VALID) && (retry_cnt < MAX_RETRY_TIMESYNC_CNT)) {
        DBG_LOG_ERROR("Invalid timestamp! %ld\n",time(NULL));
		retry_cnt++;
        safe_sleep(WAIT_FOR_4G_TIME, 0);//wait for 4g
    }

	if (retry_cnt >= MAX_RETRY_TIMESYNC_CNT) {
        DBG_LOG_MUST("####wait for valid time timeout! rebooting system!\n####");
		log_impt(proc_name, "wait for valid time timeout(retry_cnt=%d)! rebooting system!\n", retry_cnt);
		do_system("reboot");
	}
}

//将字符串pstr以psplit分隔为多个子串，原串pstr会被修改，分隔后的子串通过parr返回
int split_string_to_array(char* pstr, char* psplit, char** parr, int maxcnt)
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

void safe_free(void *ptr)
{
	if(ptr) {
        free(ptr);
        ptr = NULL;
    }
}

char *sx_strcpy(char *dest, const char *src)
{
   if(NULL == dest || NULL == src)
   	return NULL;

   char *pa = dest;
   const char *pb = src;
   while((*pa++ = *pb++));

   return dest;
}

/********************************************************
* function:trim_left
* purpose:去掉字符串左端的空格
* return:处理后的字符串的长度
*
*********************************************************/
int  trim_left(char* RawString)
{
    if(RawString == NULL)return 0;

    int len;
    int rawlen;

    rawlen = strlen(RawString);

    for(len = 0; len < rawlen; len++)
    {
        if(RawString[len] == ' ' ||  RawString[len] == '\t' ||
                RawString[len] == '\r'  ||  RawString[len] == '\n')
            continue;
        else
            break;
    }
    //memcpy(RawString, RawString + len, rawlen - len);
    sx_strcpy(RawString, RawString + len);
    return strlen(RawString);
}

/********************************************************
* function:trim_right
* purpose:去掉字符串右端的空格
* return:处理后的字符串的长度
*
*********************************************************/
int  trim_right(char* RawString)
{

    if(RawString == NULL)return 0;

    int len;
    int rawlen;

    rawlen = strlen(RawString);
    for(len = rawlen - 1; len >= 0; len--)
    {
        if(RawString[len] == ' ' ||  RawString[len] == '\t' ||
                RawString[len] == '\r'  ||  RawString[len] == '\n')
            continue;
        else
            break;
    }

    RawString[len + 1] = 0;
    return strlen(RawString);
}

/********************************************************
* function:trim
* purpose:去掉字符串两端的空格
* return:处理后的字符串
*
*********************************************************/
int  trim(char* RawString)
{

    trim_right(RawString);
    return trim_left(RawString);
}


//删除json值字符串中的双引号
//如："dev_type": "CNC"，值为:"CNC"，则将两端的 " 删除
int del_json_str_quota(char *json_value_str)
{
	int len = 0;
	if(json_value_str == NULL)
		return -1;

	len = strlen(json_value_str);
	if(json_value_str[len - 1] == '\"') {
		json_value_str[len - 1] = '\0';
	}

	if(json_value_str[0] == '\"') {
		sx_strcpy(json_value_str, json_value_str + 1);
	}

	return 0;
}



int get_querystring_value(char* querystring, char* key, char* key_value, int maxlen)
{
	char* ptrs = NULL;
	char* ptre = NULL;
	char* presult[2] = {NULL, NULL};
	static char buff[128]={0};
	if(querystring == NULL || key == NULL || key_value == NULL) {
		LOG(LOG_LV_ERROR, "invaild params\n");
		return -1;
	}

	ptrs = strstr(querystring, key);
	if(ptrs == NULL) {
		LOG(LOG_LV_ERROR, "can't find key[%s] in querystring[%s]\n", key, querystring);
		return -1;
	}
	ptre = strstr(querystring, "&");
	if(ptre != NULL) {
		*ptre = '\0';
	}

	memset(buff, 0, sizeof(buff));
	strncpy(buff, ptrs, sizeof(buff));
	if(ptre != NULL) {
		*ptre = '&';
	}

	if(split_string_to_array(buff, "=", (char**)&presult, 2) != 2) {
		LOG(LOG_LV_WARN, "can't find key[%s] value in querystring[%s]\n", key, querystring);
		return -1;
	}

	strncpy(key_value, presult[1], maxlen);

	return 0;
}

int is_number(char* str)
{
	int len = 0;
	int i = 0;
	int is_num = 1;
	if(str == NULL)
		return 0;

	len = strlen(str);
	for(i=0;i<len;i++)
	{
		if(str[i] < '0' || str[i] > '9')
		{
			is_num = 0;
			break;
		}
	}

	return is_num;
}
/********************************************************************************
 * Function:       check_ipv4
 * Description:    检查是否是IPv4类型IP地址
 * Prototype:      int check_ipv4(char* ipstr)
 * Param:          char* ipstr   - 带检查字符串
 * Return:         1: 是IPv4类型IP地址; 0: 不是IPv4类型IP地址;
 * Other:
********************************************************************************/
int check_ipv4(char* ipstr)
{
	int is_ip = 0;
	int i = 0, len = 0;
	int dot_pos = 0;

	if(ipstr == NULL)
		return is_ip;

	len = strlen(ipstr);
	if(len > 15) //15 is biggest ip address len
		return is_ip;

	is_ip = 1;
	for(i=0;i<len;i++)
	{
		if(ipstr[i] == '.')
		{
			if(dot_pos <=0 || dot_pos > 3)
			{
				//点分间无数字或数字个数超过3个，则是无效IP地址
				is_ip = 0;
				break;
			}
			else
			{
				dot_pos = 0;
				continue;
			}
		}

		if(ipstr[i] < '0' || ipstr[i] > '9')
		{
			is_ip = 0;
			break;
		}
		dot_pos++;
	}
	if(dot_pos <=0 || dot_pos > 3)
	{
		//IP地址的末尾无'.'号，所以这里要再判断一次
		//点分间无数字或数字个数超过3个，则是无效IP地址
		is_ip = 0;
	}

	return is_ip;
}

void set_core_limit(int size_kb)
{
	struct rlimit rlim = {0, 0};

	rlim.rlim_cur = size_kb * 1024;
	rlim.rlim_max = rlim.rlim_cur;
	setrlimit(RLIMIT_CORE, &rlim);
}

void print_core_limit()
{
	struct rlimit rlim = {0, 0};

	getrlimit(RLIMIT_CORE, &rlim);
	LOG(LOG_LV_INFO, "process core file size: %lu %lu\n", rlim.rlim_cur, rlim.rlim_max);
}

int resource_init_common()
{
	int ret = 0;
	ret = log_impt_init();
	if (ret) {
		printf("log_impt_init fail!\n");
		return -1;
	}

	set_core_limit(5*1024); //5MB

	return ret;
}

/********************************************************************************
 * Function:       strcrpl
 * Description:    字符串中字符替换
 * Prototype:      char* strcrpl(char* src, char old_ch, char new_ch)
 * Param:          char* src     -待替换字符串
 * Param:          char old_ch   -替换前的字符
 * Param:          char new_ch   -替换后的字符
 * Return:         替换后的字符串
********************************************************************************/
char* strcrpl(char* src, char old_ch, char new_ch)
{
	char* ptmp = src;

	if(src == NULL) return NULL;

	while(*ptmp != '\0')
	{
		if(*ptmp == old_ch)
			*ptmp = new_ch;
		ptmp++;
	}

	return src;
}

