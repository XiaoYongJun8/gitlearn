#ifndef __DLT645_PORT_H
#define __DLT645_PORT_H
#include "dlt645.h"

dlt645_t* dlt645_new_tcp(const char* ip_addr, uint16_t port);
int dlt645_tcp_connect(dlt645_t* ctx);


#endif
