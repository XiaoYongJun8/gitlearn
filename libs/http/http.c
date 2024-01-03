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

#include <stdlib.h>
#include "http.h"
#include "dlog.h"

size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata);

int http_init()
{
	curl_global_init(CURL_GLOBAL_ALL);
	return 0;
}

void http_cleanup(http_t* http)
{
	curl_global_cleanup();
	http_destory(http);
}

http_t* http_create()
{
	http_t* http = NULL;

	http = (http_t*)calloc(sizeof(http_t), 1);
	if(http == NULL){
		return NULL;
	}

	http->curl = curl_easy_init();
	http->req_headers = curl_slist_append(http->req_headers, "User-Agent: SuxinIoT ICBOX 2.0");

	return http;
}

void http_destory(http_t* http)
{
	if(http){
		if(http->curl)
			curl_easy_cleanup(http->curl);
		if(http->req_headers)
			curl_slist_free_all(http->req_headers);
		if(http->rsp_headers)
			curl_slist_free_all(http->rsp_headers);
		
		free(http);
	}
}

void http_add_header(http_t* http, const char* header)
{
	struct curl_slist* tmp = NULL;
	tmp = curl_slist_append(http->req_headers, header);
	if(!tmp){
		derror("add http header[%s] failed", header);
		return;
	}

	http->req_headers = tmp;
}

int http_enable_tls(http_t* http)
{
    // https://blog.csdn.net/ypbsyy/article/details/83784670
    curl_easy_setopt(http->curl, CURLOPT_SSL_VERIFYPEER, 0);//这个是重点。

    // https请求 不验证证书和hosts
    // 不能设置这个，否则连接失败
    //curl_easy_setopt(*curl, CURLOPT_SSL_VERIFYHOST, 0);//

    /*
    curl_setopt($curl, CURLOPT_SSL_VERIFYPEER, 1);
    curl_setopt($curl, CURLOPT_SSL_VERIFYHOST, 1);
    curl_setopt($curl,CURLOPT_CAINFO,dirname(__FILE__).'/cacert.pem');//这是根据http://curl.haxx.se/ca/cacert.pem 下载的证书，添加这句话之后就运行正常了
    */

   return 0;
}

//return: 0:success; 1:failed
int http_get(http_t* http, char* url, buffer_t* rsp, int timeout_ms)
{
	int ret = 0;

	curl_easy_setopt(http->curl, CURLOPT_URL, url);
	curl_easy_setopt(http->curl, CURLOPT_WRITEDATA, rsp);
	curl_easy_setopt(http->curl, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(http->curl, CURLOPT_HTTPHEADER, http->req_headers);
	if(timeout_ms > 0) curl_easy_setopt(http->curl, CURLOPT_TIMEOUT_MS, timeout_ms);

	ret = curl_easy_perform(http->curl);
	if (ret != CURLE_OK) {
		derror("curl_easy_perform() failed: %s", curl_easy_strerror(ret));
		ret = 1;
	}

	return ret;
}

int http_post(http_t* http, char* url, const char* data, int data_size, buffer_t* rsp, int timeout_ms)
{
	int ret = 0;
	
	dtrace("in http_post http=%p, curl=%p", http, http ? http->curl : NULL);
	curl_easy_setopt(http->curl, CURLOPT_URL, url);
	curl_easy_setopt(http->curl, CURLOPT_WRITEDATA, rsp);
	curl_easy_setopt(http->curl, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(http->curl, CURLOPT_HTTPHEADER, http->req_headers);
	if(timeout_ms > 0) curl_easy_setopt(http->curl, CURLOPT_TIMEOUT_MS, timeout_ms);
	/* size of the POST data */
	curl_easy_setopt(http->curl, CURLOPT_POSTFIELDSIZE, data_size);
	curl_easy_setopt(http->curl, CURLOPT_POST, 1);
	curl_easy_setopt(http->curl, CURLOPT_POSTFIELDS, data);

	dtrace("call curl_easy_perform, data_size=%d", data_size);
	ret = curl_easy_perform(http->curl);
	if (ret != CURLE_OK) {
		derror("curl_easy_perform() failed: %s", curl_easy_strerror(ret));
		ret = 1;
	}

	return ret;
}

size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	size_t all_size = size*nmemb;
	buffer_t* data = (buffer_t*)userdata;

	dtrace("recv data(%d=%d*%d): [%s]", all_size, size, nmemb, ptr);
	buffer_append_n(data, ptr, all_size);
	data->len += all_size;

	return all_size;
}
