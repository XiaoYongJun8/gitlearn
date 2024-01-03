/********************************************************************************
 *                    Copyright (C), 2022-2217, SuxinIOT Inc.
 ********************************************************************************
 * File:           http.c
 * Description:    http client api base on libcurl
 * Author:         zhums
 * E-Mail:         zhums@linygroup.com
 * Version:        1.0
 * Date:           2022-01-05
 * IDE:            Source Insight 4.0
 * 
 * History: 
 * No.  Date            Author    Modification
 * 1    2022-01-05      zhums     Created file
********************************************************************************/

#ifndef __HTTP_H__
#define __HTTP_H__

#include "curl/curl.h"
#include "buffer.h"

typedef struct http{
	CURL* curl;
	struct curl_slist* req_headers;
	struct curl_slist* rsp_headers;

	char url[1024];
	buffer_t* rsp;
}http_t;


int http_init();
void http_cleanup(http_t* http);
http_t* http_create();
void http_destory(http_t* http);
void http_add_header(http_t* http, const char* header);
int http_enable_tls(http_t* http);
int http_get(http_t* http, char* url, buffer_t* rsp, int timeout_ms);
int http_post(http_t* http, char* url, const char* data, int data_size, buffer_t* rsp, int timeout_ms);
//char* http_error_string(http_t* http, char* errbuf, int maxlen);

#endif //__HTTP_H__