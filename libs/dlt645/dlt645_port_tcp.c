/*******************************************************************************
 * @             Copyright (c) 2023 LinyPower, All Rights Reserved.
 * @*****************************************************************************
 * @FilePath     : dlt645_port_tcp.c
 * @Descripttion : 
 * @Author       : zhums
 * @E-Mail       : zhums@linygroup.cn
 * @Version      : v1.0.0
 * @Date         : 2023-12-08 16:48:38
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <signal.h>
#include <sys/types.h>

#if defined(_WIN32)
# define OS_WIN32
/* ws2_32.dll has getaddrinfo and freeaddrinfo on Windows XP and later.
 * minwg32 headers check WINVER before allowing the use of these */
# ifndef WINVER
# define WINVER 0x0501
# endif
# include <ws2tcpip.h>
# define SHUT_RDWR 2
# define close closesocket
#else
# include <sys/socket.h>
# include <sys/ioctl.h>

#if defined(__OpenBSD__) || (defined(__FreeBSD__) && __FreeBSD__ < 5)
# define OS_BSD
# include <netinet/in_systm.h>
#endif

# include <netinet/in.h>
# include <netinet/ip.h>
# include <netinet/tcp.h>
# include <arpa/inet.h>
# include <poll.h>
# include <netdb.h>
#endif

#include "dlt645.h"
#include "dlt645_port_tcp.h"

typedef struct dlt645_tcp
{
    char ip[16];
    int port;
}dlt645_tcp_t;

static int dlt645_tcp_set_ipv4_options(int s)
{
    int rc;
    int option;

    /* Set the TCP no delay flag */
    /* SOL_TCP = IPPROTO_TCP */
    option = 1;
    rc = setsockopt(s, IPPROTO_TCP, TCP_NODELAY,
                    (const void*)&option, sizeof(int));
    if(rc == -1) {
        return -1;
    }

#ifndef OS_WIN32
    /**
     * Cygwin defines IPTOS_LOWDELAY but can't handle that flag so it's
     * necessary to workaround that problem.
     **/
     /* Set the IP low delay option */
    option = IPTOS_LOWDELAY;
    rc = setsockopt(s, IPPROTO_IP, IP_TOS,
                    (const void*)&option, sizeof(int));
    if(rc == -1) {
        return -1;
    }
#endif

    return 0;
}

int dlt645_tcp_connect(dlt645_t* ctx)
{
    int rc;
    struct sockaddr_in addr;
    dlt645_tcp_t* ctx_tcp = (dlt645_tcp_t*)ctx->port_data;

#ifdef OS_WIN32
    if(_modbus_tcp_init_win32() == -1) {
        return -1;
    }
#endif

    ctx->s = socket(PF_INET, SOCK_STREAM, 0);
    if(ctx->s == -1) {
        return -1;
    }

    rc = dlt645_tcp_set_ipv4_options(ctx->s);
    if(rc == -1) {
        close(ctx->s);
        return -1;
    }

    if(ctx->debug) {
        printf("Connecting to %s\n", ctx_tcp->ip);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(ctx_tcp->port);
    addr.sin_addr.s_addr = inet_addr(ctx_tcp->ip);
    rc = connect(ctx->s, (struct sockaddr*)&addr,
                 sizeof(struct sockaddr_in));
    if(rc == -1) {
        close(ctx->s);
        return -1;
    }

    return 0;
}

int dlt645_port_tcp_select(dlt645_t* ctx)
{
    int ret = 0;

    struct timeval rsp_timeout = ctx->response_timeout;
    fd_set rset;
    FD_ZERO(&rset);
    FD_SET(ctx->s, &rset);

    ret = select(ctx->s + 1, &rset, NULL, NULL, &rsp_timeout);
    if(ret > 0) {
        return ctx->s;
    }
    
    return ret;
}

//实际接收长度
int dlt645_port_tcp_read(dlt645_t* ctx, uint8_t* msg, uint16_t len)
{
    return read(ctx->s, msg, len);
}

int dlt645_port_tcp_write(dlt645_t *ctx, uint8_t *buf, uint16_t len)
{
    return write(ctx->s, buf, len);
}

dlt645_t* dlt645_new_tcp(const char* ip_addr, uint16_t port)
{
    dlt645_tcp_t* ctx_tcp = NULL;
    dlt645_t* pdlt = (dlt645_t*)malloc(sizeof(dlt645_t));
    if(pdlt == NULL) {
        return pdlt;
    }
    memset(pdlt, 0, sizeof(dlt645_t));
    pdlt->debug = 0;
    pdlt->response_timeout.tv_sec = 1;
    pdlt->response_timeout.tv_usec = 0;
    pdlt->read = dlt645_port_tcp_read;
    pdlt->write = dlt645_port_tcp_write;
    pdlt->select = dlt645_port_tcp_select;
    pdlt->port_data = malloc(sizeof(dlt645_tcp_t));
    if(pdlt->port_data == NULL) {
        free(pdlt);
        pdlt = NULL;
        return pdlt;
    }
    ctx_tcp = (dlt645_tcp_t*)pdlt->port_data;
    strncpy(ctx_tcp->ip, ip_addr, sizeof(ctx_tcp->ip));
    ctx_tcp->port = port;

    return pdlt;
}