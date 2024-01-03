/*******************************************************************************
 * @             Copyright (c) 2023 SuXiniot, All Rights Reserved.
 * @*****************************************************************************
 * @FilePath     : fdevent.h
 * @Descripttion : 
 * @Author       : zhums
 * @E-Mail       : zhums@suxiniot.com
 * @Version      : v1.0.0
 * @Date         : 2023-04-21 15:42:23
*******************************************************************************/
#ifndef __FDEVENT_H__
#define __FDEVENT_H__
#include <sys/eventfd.h>

typedef struct fdevent {
    char name[32];
    int fd;
}fdevent_t;

int fdevent_init(fdevent_t* pfdev, char* name, int flags);
int fdevent_trigger(fdevent_t* pfdev);
int fdevent_pop(fdevent_t* pfdev);
int fdevent_clear(fdevent_t* pfdev);
void fdevent_deinit(fdevent_t* pfdev);

#endif //__FDEVENT_H__
