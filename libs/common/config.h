/********************************************************************************
 *                    Copyright (C), 2021-2217, SuxinIOT Inc.
 ********************************************************************************
 * File:           config.h
 * Description:    ini format config file read/write util
 * Author:         zhums
 * E-Mail:         zhums@linygroup.com
 * Version:        1.0
 * Date:           2021-12-25
 * IDE:            Source Insight 4.0
 *
 * History:
 * No.  Date            Author    Modification
 * 1    2021-12-25      zhums     Created file
********************************************************************************/

#ifndef __CONFIG_H__
#define __CONFIG_H__

#define CFG_FILE     	"/home/root/apps/config.ini" //配置文件
#define CONFIG_OK       0
#define CONFIG_FAIL     -1

typedef struct
{
	int 	wdog_timeout;
	int		loglevel_mode;
	int		loglevel;
	char	loglevel_types[64];
}config_comm_t;

void init_common_configs(config_comm_t *cfg_comm);

/********************************************************************************
 * Function:       config_read_str
 * Description:    read int config with specify default value
 * Param:          const char* pfile   - config file name
 * Param:          const char* psec    - config item belongs section name
 * Param:          const char* pkey    - config item key name
 * Param:          char* pval          - config item value, out param
 * Param:          int maxlen          - pval memory max len
 * Param:          const char* pdef    - default value while read config failed, can also specify to NULL
 * Return:         NULL: failed;  !NULL: config value
********************************************************************************/
char* config_read_str(const char* pfile, const char* psec, const char* pkey, char* pval, int maxlen, const char* pdef);

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
int config_write_str(const char* pfile, const char* psec, const char* pkey, const char* pval);

/********************************************************************************
 * Function:       config_read_int
 * Description:    read int config with specify default value
 * Prototype:      int config_read_int(const char* pfile, const char* psec, const char* pkey, int* pval, const int def)
 * Param:          const char* pfile   - config file name
 * Param:          const char* psec    - config item belongs section name
 * Param:          const char* pkey    - config item key name
 * Param:          const char* pval    - config item value
 * Param:          const int def       - default value while read config failed
 * Return:         0:success;  -1:failed
********************************************************************************/
int config_read_int(const char* pfile, const char* psec, const char* pkey, int* pval, const int def);

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
int config_read_int2(const char* pfile, const char* psec, const char* pkey, int* pval);

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
int config_write_int(const char* pfile, const char* psec, const char* pkey, const int pval);

#endif //__CONFIG_H__