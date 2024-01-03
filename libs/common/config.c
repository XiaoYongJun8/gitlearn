#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>

#include "util.h"
#include "config.h"
#include "dlog.h"
#include "debug_and_log.h"

#define MAX_INI_FILE_BUFFER_SIZE	1024*2
#define MAX_INI_FILE_NAME_LENTH		256
#define MAX_INT_VALUE       		256

#define ERR_NO_KEY      99999
#define ERR_NOT_INT     99998
#define DEFAULT_INT     99997

#define LINE_CM		'#'		//comment start char, which all line is comment, CM means comment
#define TAIL_CM		';'		//comment start char, which comment at config line tail, CM means comment
#define SEC_LEFT	'['		//config section left char
#define SEC_RIGHT	']'		//config section right char
#define KV_SPLIT	'='		//config item key/value split char

typedef struct ini_config_st {
    FILE* fp;   		//指向配置文件的指针
    char fmt_checked;	//ini config file format has checked or not
    long cur_pos;      //文件指针的当前位置
    char fname[MAX_INI_FILE_NAME_LENTH]; 		//配置文件名
    char buff[MAX_INI_FILE_BUFFER_SIZE + 1]; 	//缓冲区
} ini_config_t;
static ini_config_t inicfg = {NULL, 0, 0, {0}, {0}};

/********************************************************************************
 * Function:       config_get_sign_encap
 * Description:    获得指定顺序的两个字符之间的不带两端空格的字符串
 * Prototype:      char* config_get_sign_encap(ini_config_t* pini, char* psrc, const char lsign, const char rsign)
 * Param:          ini_config_t* pini   - 
 * Param:          char* psrc           - 
 * Param:          const char lsign     - 
 * Param:          const char rsign     - 
 * Return:         NULL: failed; not NULL: success, dst str pointer
********************************************************************************/
char* config_get_sign_encap(ini_config_t* pini, char* psrc, const char lsign, const char rsign)
{
    char* str1 = NULL;
    char* str2 = NULL;

    str1 = strchr(psrc, lsign);
    if(!str1)return NULL;

    str2 = strrchr(psrc, rsign);
    if(!str2) return NULL;

    if(str1 > str2 - 1) return NULL;

    *str2 = '\0';
    str1++;
    trim(str1);
    return str1;
}

/********************************************************************************
 * Function:       config_format_check
 * Description:    检查配置文件书写格式
 * Prototype:      int config_format_check(ini_config_t* pini, const char* pfname)
 * Param:          ini_config_t* pini   - 
 * Param:          const char* pfname   - ini config file name
 * Return:         0--格式符合要求; -1--格式不符合要求
********************************************************************************/
int config_format_check(ini_config_t* pini)
{
    char* stmp;
    int irows;			//计算行数
    int isSecExit = 0;	//段名存在布尔值
    
    inicfg.fp = fopen(inicfg.fname, "r");
    if(inicfg.fp == NULL) {
        dlog("open file %s failed: %s", inicfg.fname, strerror(errno));
        return -1;
    }

	//检查每行
    for(irows = 1; !feof(inicfg.fp); irows++) {
        memset(inicfg.buff, 0, sizeof(inicfg.buff));
		//读一行
        if(fgets(inicfg.buff, MAX_INI_FILE_BUFFER_SIZE, inicfg.fp) == NULL) {
            fclose(inicfg.fp);
            return 0;
        }
        trim_left(inicfg.buff);
        if(*inicfg.buff == LINE_CM)	//如果是行注释，跳过
            continue;
		
        //去掉行尾注释
        if((stmp = strchr(inicfg.buff, TAIL_CM)) != NULL) 
			*stmp = 0;
        stmp = (inicfg.buff);
        if(trim(stmp) == 0)	//如果是空行，跳过继续下一行
			continue;

		//字串(不含[]）为空
        if(strlen(stmp) <= 2) {
            dlog("row %d:length is less than 2!", irows);
            fclose(inicfg.fp);
            return -1;
        }
		//如果不是段名，看是否是键名
        if(*inicfg.buff != SEC_LEFT) {
			//如果上面没有段名
            if(isSecExit == 0) {
                dlog("Row %d: maybe key does not along to one section!", irows);
                fclose(inicfg.fp);
                return -1;
            }
			//察看是否是键名, 找到“=”的位置
            stmp = strchr(inicfg.buff, KV_SPLIT);
            if(stmp == NULL) {
                dlog("Row %d: key lost split char'%c' ", irows, KV_SPLIT);
                fclose(inicfg.fp);
                return -1;
            }
            *stmp = 0;	//去掉=号

			//查看键名
            stmp = (inicfg.buff);
            if(trim(stmp) == 0) {
                dlog("Row: %d: no key exists!", irows);
                fclose(inicfg.fp);
                return -1;
            }
			//是键名， 继续下一行
            continue;
        }

		//返回[]之间的字符串,查看是否是段名
        stmp = config_get_sign_encap(pini, inicfg.buff, SEC_LEFT, SEC_RIGHT);
        if(strlen(stmp) == 0) {
            dlog("Row %d:Section name does not fit requirement!", irows);
            fclose(inicfg.fp);
            return -1;
        }

        isSecExit = 1;	//存在段名
    }
    fclose(inicfg.fp);
    return 0;
}

/********************************************************************************
 * Function:       config_get_section_pos
 * Description:    定位到指定的段名
 * Prototype:      int config_get_section_pos(ini_config_t* pini, const char* psec)
 * Param:          ini_config_t* pini   - 
 * Param:          const char* psec     - 
 * Return:         0-成功，-1-失败  
********************************************************************************/
int config_get_section_pos(ini_config_t* pini, const char* psec)
{
    char* stmp;

	//必须保证先打开文件
    rewind(inicfg.fp);
    for(; !feof(inicfg.fp);) {
        inicfg.cur_pos = ftell(inicfg.fp);
        memset(inicfg.buff, 0, sizeof(inicfg.buff));
        if(fgets(inicfg.buff, MAX_INI_FILE_BUFFER_SIZE, inicfg.fp) == NULL)
			break;

		//去掉注释
        if((stmp = strchr(inicfg.buff, LINE_CM)) != NULL) 
			*stmp = 0;

		//字串(不含[]）为空
        if(trim(inicfg.buff) == 0) continue;

		//是注释行或不是section
        if(*inicfg.buff == LINE_CM || *inicfg.buff != SEC_LEFT) continue;

		//返回[]之间的字符串
        stmp = config_get_sign_encap(pini, inicfg.buff, SEC_LEFT, SEC_RIGHT);
        if(strlen(stmp) == 0) {
            dlog("config file format error!");
            continue;
        }
		//dlog("got section name[%s]", stmp);
        if(strcmp(stmp, psec))continue;
        return 0;
    }
    inicfg.cur_pos = 0L;
    rewind(inicfg.fp);
    return -1;
}

/********************************************************
 *  描述：定位文件中的key位置，文件指针停在key的行首
 *  输入：section,key
 *  输出：
 *  返回：0-成功，-1-失败  private
 ********************************************************/
int config_get_key_pos(ini_config_t* pini, const char* psec, const char* pkey)
{
    int iRet = 0;
    char* stmp;

    rewind(inicfg.fp);
    iRet = config_get_section_pos(pini, psec);
    if(iRet < 0) return -1;

    for(; !feof(inicfg.fp);) {
        inicfg.cur_pos = ftell(inicfg.fp);
        memset(inicfg.buff, 0, sizeof(inicfg.buff));
        if(fgets(inicfg.buff, MAX_INI_FILE_BUFFER_SIZE, inicfg.fp) == NULL)break;
		//dlog("got line[%s]", inicfg.buff);
		
		//是行末注释
        if((stmp = strchr(inicfg.buff, TAIL_CM)) != NULL) 
			*stmp = 0;

        stmp = inicfg.buff;
        if(trim(stmp) == 0) continue;

		//是注释行
        if(*inicfg.buff == LINE_CM) continue;

		//遇到新的段，没有找到key
        if(*stmp == SEC_LEFT) return -1;

		//找到“=”的位置
        stmp = strchr(inicfg.buff, KV_SPLIT);
        if(stmp == 0) {
            dlog("Format is incorrect, lost key value split char '%c' somewhere", KV_SPLIT);
            return -1;
        }
        *stmp = 0;
		//去除两端空格
        stmp = inicfg.buff;
        if(trim(stmp) == 0) return -1;
		//dlog("got key[%s]", stmp);
		//发现cpKey
        if(strcmp(stmp, pkey) != 0) continue;
        fseek(inicfg.fp, inicfg.cur_pos, SEEK_SET);
        return 0;
    }
    inicfg.cur_pos = 0;
    rewind(inicfg.fp);
    return -1;
}

/********************************************************************************
 * Function:       config_open_file
 * Description:    judge file name has change or not and then open file
 * Prototype:      int config_open_file(const char* pfile)
 * Param:          const char* pfile   - 
 * Return:         0: success; -1: failed
********************************************************************************/
int config_open_file(const char* pfile)
{
	//如果文件名不为空的话，重置文件名
    if(pfile != NULL && strcmp(pfile, inicfg.fname) != 0)
        strncpy(inicfg.fname, pfile, sizeof(inicfg.fname) - 1);
    else if(inicfg.fname == NULL) { 
        dlog("There is no file to operate!");	//如果都为空的话，报错退出
        return -1;
    }
	
	if(!inicfg.fmt_checked)
	{
		if(config_format_check(&inicfg) == -1){
			dlog("config file format check failed");
			return -1;
		}
		inicfg.fmt_checked = 1;
	}
	
    inicfg.fp = fopen(inicfg.fname, "r");
    if(inicfg.fp == NULL) {
        dlog("Open file %s failed:%s", inicfg.fname, strerror(errno));
        return -1;
    }

	return 0;
}

/********************************************************************************
 * Function:       config_read_str
 * Description:    read string config with specify default value
 * Prototype:      char* config_read_str(const char* pfile, const char* psec, const char* pkey, char* pval, int maxlen, const char* pdef)
 * Param:          const char* pfile   - config file name
 * Param:          const char* psec    - config item belongs section name
 * Param:          const char* pkey    - config item key name
 * Param:          char* pval          - config item value, out param
 * Param:          int maxlen          - pval memory max len
 * Param:          const char* pdef    - default value while read config failed, can also specify to NULL
 * Return:         NULL: failed;  !NULL: config value
********************************************************************************/
char* config_read_str(const char* pfile, const char* psec, const char* pkey, char* pval, int maxlen, const char* pdef)
{
    char* stmp;
    int ret;

	if(config_open_file(pfile) == -1) {
        stmp = NULL;
		goto END;
	}

    //定位cpSection,pkey 
    ret = config_get_key_pos(&inicfg, psec, pkey);
    if(ret < 0) {
        fclose(inicfg.fp);
        dlog("[%s|%s] can't find the section or the key you provide!", psec, pkey);
        stmp = NULL;
		goto END;
    }
	
    //从定位处读一行
    memset(inicfg.buff, 0, sizeof(inicfg.buff));
    if(fgets(inicfg.buff, MAX_INI_FILE_BUFFER_SIZE, inicfg.fp) == NULL) {
        fclose(inicfg.fp);
        dlog("file is empty!");
        stmp = NULL;
		goto END;
    }
    fclose(inicfg.fp);

	//去除行末注释
    if((stmp = strchr(inicfg.buff, TAIL_CM)) != NULL)
        * stmp = '\0';
    if((stmp = strchr(inicfg.buff, KV_SPLIT)) == NULL) {
        dlog("config file format is not right, lost key value split char '%c' somewhere!", KV_SPLIT);
        stmp = NULL;
		goto END;
    }

    stmp++;
    if(trim(stmp) == 0 )
    {
        dlog("[%s|%s] This key hasnot value in the file!", psec, pkey);
		stmp = NULL;
		goto END;
    }
	
END:
	if(pdef && stmp == NULL) {
		dlog("default value is specify, set to default value[%s]", pdef);
		stmp = (char*)pdef;	//返回默认值
	}
	if(stmp == NULL) 
		return NULL;
	strncpy(pval, stmp, maxlen-1);

    return pval;
}

/********************************************************************************
 * Function:       config_write_str
 * Description:    write string config to config file
 * Prototype:      int config_write_str(const char* pfile, const char* psec, const char* pkey, const char* pval)
 * Param:          const char* pfile   - config file name
 * Param:          const char* psec    - config item belongs section name
 * Param:          const char* pkey    - config item key name
 * Param:          const char* pval    - config item value
 * Return:         0:success;  -1:failed
********************************************************************************/
int config_write_str(const char* pfile, const char* psec, const char* pkey, const char* pval)
{
    char bakFname[256];//临时文件缓冲区
    char tmpkey[1024];
    char tmpsec[1024];
    FILE* fpbak;
    long secpos = -1;
    long keypos = -1;
	int ret;
	
	if(config_open_file(pfile) == -1) {
	    return -1;
	}

    snprintf(tmpsec, sizeof(tmpsec), "[%s]\n", psec);			//生成段名
    snprintf(tmpkey, sizeof(tmpkey), "\x09%s=%s\n", pkey, pval);	//生成键值
    snprintf(bakFname, sizeof(bakFname), "%.200s.bak", inicfg.fname);

    ret = config_get_section_pos(&inicfg, psec);	//定位cpSection,pkey
	//存在段名
    if(!ret) {
        secpos = ftell(inicfg.fp);
        ret = config_get_key_pos(&inicfg, psec, pkey);

		//存在键
        if(!ret)
            keypos = ftell(inicfg.fp);
    }
    if((fpbak = fopen(bakFname, "w")) == NULL) {
        dlog("backup file[%s] open failed: %s", bakFname, strerror(errno));
        return -1;
    }
    rewind(inicfg.fp);

	//read from configure file and write to back file
    while(!feof(inicfg.fp)) {
		//exist section,not exist Key
        if(secpos >= 0 && keypos < 0 && secpos == ftell(inicfg.fp)) {
            fputs(tmpkey, fpbak);
            memset(inicfg.buff, 0, sizeof(inicfg.buff));
            fgets(inicfg.buff, MAX_INI_FILE_BUFFER_SIZE, inicfg.fp);
            fputs(inicfg.buff, fpbak);
            break;
        }

		//both section and key exist
        else if(keypos >= 0 && keypos == ftell(inicfg.fp)) {
            memset(inicfg.buff, 0, sizeof(inicfg.buff));
            fgets(inicfg.buff, MAX_INI_FILE_BUFFER_SIZE, inicfg.fp);
            fputs(tmpkey, fpbak);
            break;
        }
        memset(inicfg.buff, 0, sizeof(inicfg.buff));
        fgets(inicfg.buff, MAX_INI_FILE_BUFFER_SIZE, inicfg.fp);
        fputs(inicfg.buff, fpbak);
    }
    while(!feof(inicfg.fp)) {
        memset(inicfg.buff, 0, sizeof(inicfg.buff));
        fgets(inicfg.buff, MAX_INI_FILE_BUFFER_SIZE, inicfg.fp);
        fputs(inicfg.buff, fpbak);
    }

	//if not exist section, write a new section at the bottom
    if(secpos < 0) {
        fputs("\n\0", fpbak);
        fputs(tmpsec, fpbak);
        fputs(tmpkey, fpbak);
    }

    fclose(inicfg.fp);
    fclose(fpbak);
    rename(bakFname, inicfg.fname);
    return 0;
}

/********************************************************************************
 * Function:       config_read_int
 * Description:    read int config with specify default value
 * Prototype:      int config_read_int(const char* pfile, const char* psec, const char* pkey, int* pval, const int def)
 * Param:          const char* pfile   - config file name
 * Param:          const char* psec    - config item belongs section name
 * Param:          const char* pkey    - config item key name
 * Param:          int* pval           - config item value
 * Param:          const int def       - default value while read config failed
 * Return:         0:success;  -1:failed
********************************************************************************/
int config_read_int(const char* pfile, const char* psec, const char* pkey, int* pval, const int def)
{
    char stmp[16] = {0};
	char sdef[16] = {0};
	int ival = 0;

    snprintf(sdef, MAX_INT_VALUE, "%d", def); //将整数的默认返回值设为字符型
    if(config_read_str(pfile, psec, pkey, stmp, sizeof(stmp), sdef) == NULL) {
        dlog("read int config failed, set to default value[%d]", def);
		ival = *pval = def;
        return 0;
    }

	//转换为整数
    ival = atoi(stmp);
	//如果字符串为一个字符0，则返回0
    if(strcmp("0", stmp) == 0) 
		ival = 0;
    else if(ival == 0) {
		//如果转换失败则报错退出
        dlog("The key value you read is not an integer, set to default value[%d]", def);
        ival = def;
    }
	*pval = ival;
	
    return 0;
}

/********************************************************************************
 * Function:       config_read_int2
 * Description:    read int comfig without default value
 * Prototype:      int config_read_int2(const char* pfile, const char* psec, const char* pkey, int* pval)
 * Param:          const char* pfile   - config file name
 * Param:          const char* psec    - config item belongs section name
 * Param:          const char* pkey    - config item key name
 * Param:          int* pval           - config item value
 * Return:         0:success; -1:failed
********************************************************************************/
int config_read_int2(const char* pfile, const char* psec, const char* pkey, int* pval)
{
    char stmp[16];
	int ival = 0;

    if(config_read_str(pfile, psec, pkey, stmp, sizeof(stmp), NULL) == NULL) {
        dlog("read int config error");
        return -1;
    }

	//转换为整数
    ival = atoi(stmp);
	//如果字符串为一个字符0，则返回0
    if(strcmp("0", stmp) == 0) 
		ival = 0;
    else if(ival == 0) {
		//如果转换失败则报错退出
        dlog("The key value you read is not an integer");
        return -1;
    }
	*pval = ival;
	
    return 0;
}

/********************************************************************************
 * Function:       config_write_int
 * Description:    write int config to config file
 * Prototype:      int config_write_int(const char* pfile, const char* psec, const char* pkey, const int pval)
 * Param:          const char* pfile   - config file name
 * Param:          const char* psec    - config item belongs section name
 * Param:          const char* pkey    - config item key name
 * Param:          const int pval      - config item value
 * Return:         0:success; -1:failed
********************************************************************************/
int config_write_int(const char* pfile, const char* psec, const char* pkey, const int pval)
{
    char stmp[MAX_INT_VALUE + 1];

    snprintf(stmp, MAX_INT_VALUE, "%d", pval);
    return config_write_str(pfile, psec, pkey, stmp);
}

//初始化配置数据
void init_common_configs(config_comm_t *cfg_comm)
{
	char value[256] = {0};
	//int ret = CONFIG_OK;

	if (NULL == cfg_comm)
	{
		dlog("Invalid params!");
		return;
	}

	memset(value,0,sizeof(value));
	memset(cfg_comm,0,sizeof(config_comm_t));

	//config_read_str(CONFIG_FILE, "general", "plat_addr", value, 256, "mqtt.linygroup.com");
    config_read_int(CFG_FILE, "general", "wdog_timeout", &cfg_comm->wdog_timeout, 60);
    config_read_int(CFG_FILE, "general", "loglevel_mode", &cfg_comm->loglevel_mode, LOGLV_MODE_LINEAR);
    config_read_int(CFG_FILE, "general", "loglevel", &cfg_comm->loglevel, LOG_LV_INFO);
    config_read_str(CFG_FILE, "general", "loglevel_types", cfg_comm->loglevel_types, 64, "0,1,2,3,4");
    
	// DBG_LOG_MUST("plat_addr: %s",
	// 	configs->plat_addr
	// 	);
}

#if 0
//gcc config.c util.c ../log/log.c ../log/debug.c ../log/log_impt.c -I../log -o config
int main(void)
{
	int ival = 0;
	char strval[1024] = {0};
	char secname[32] = {0};
	char cfgfile[] = "/home/zhums/gitwork/icbox-v2.0-am335x/suxin/libs/common/config.ini";

	snprintf(secname, sizeof(secname), "RECV");
	if(!config_read_str(cfgfile, secname, "PCAP_FILE_PATH", strval, sizeof(strval), "test.pcap"))
		derror("read config PCAP_FILE_PATH error");
	else
		dinfo("read str config PCAP_FILE_NAME=%s", strval);
	if(!config_read_str(cfgfile, secname, "PCAP_FILE_PATH2", strval, sizeof(strval), "test2.pcap"))
		derror("read config PCAP_FILE_PATH2 error");
	else
		dinfo("read int config PCAP_FILE_PATH2=%s", strval);
	
	snprintf(secname, sizeof(secname), "general");
	if(config_read_int(cfgfile, secname, "city3", &ival, 113) != 0)
		derror("read config city error");
	else
		dinfo("read int config city=%d", ival);
	if(config_read_int2(cfgfile, secname, "city4", &ival) != 0)
		derror("read config city4 error");
	else
		dinfo("read int config city4=%d", ival);
	if(config_write_int(cfgfile, secname, "city", 110) != 0)
		derror("write config city error");
	if(config_write_int(cfgfile, secname, "city2", 110120) != 0)
		derror("write config city2 error");
	if(config_write_int(cfgfile, "test", "age", 20) != 0)
		derror("write config age error");

	return 0;
}
#endif
