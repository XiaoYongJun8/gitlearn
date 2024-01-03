/*******************************************************************************
 * @             Copyright (c) 2023 SuXiniot, All Rights Reserved.
 * @*****************************************************************************
 * @FilePath     : fdevent.c
 * @Descripttion :
 * @Author       : zhums
 * @E-Mail       : zhums@suxiniot.com
 * @Version      : v1.0.0
 * @Date         : 2023-04-21 15:42:01
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#include "fdevent.h"

/*******************************************************************************
 * @brief 初始化fdevent_t结构
 * @param pfdev
 * @param name
 * @param flags EFD_NONBLOCK/EFD_SEMAPHORE/EFD_CLOEXEC
 * @return 0：success; 1: failed
*******************************************************************************/
int fdevent_init(fdevent_t* pfdev, char* name, int flags)
{
    pfdev->fd = eventfd(0, flags);
    strncpy(pfdev->name, name, sizeof(pfdev->name));

    return 0;
}

/*******************************************************************************
 * @brief
 * @param pfdev
 * @return 0: success; 1: failed
*******************************************************************************/
int fdevent_trigger(fdevent_t* pfdev)
{
    uint64_t cnt = 1;
    int ret = 0;
    ret = write(pfdev->fd, &cnt, sizeof(uint64_t));
    return (ret == sizeof(uint64_t) ? 0 : 1);
}

int fdevent_pop(fdevent_t* pfdev)
{
    uint64_t cnt = 0;
    return read(pfdev->fd, &cnt, sizeof(uint64_t));
}

int fdevent_clear(fdevent_t* pfdev)
{
    uint64_t cnt = 0;
    int ret = 0;

    do {
        ret = read(pfdev->fd, &cnt, sizeof(uint64_t));
    } while(ret != -1);

    return 0;
}

void fdevent_deinit(fdevent_t* pfdev)
{
    if(pfdev->fd > 0) {
        close(pfdev->fd);
    }
}