/*******************************************************************************
 * @             Copyright (c) 2023 LinyPower, All Rights Reserved.
 * @*****************************************************************************
 * @FilePath     : name_pipe.c
 * @Descripttion :
 * @Author       : zhums
 * @E-Mail       : zhums@linygroup.cn
 * @Version      : v1.0.0
 * @Date         : 2023-07-04 09:30:21
*******************************************************************************/
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "name_pipe.h"

int name_pipe_make(char* name, mode_t mode, int oflag)
{
    int fd = 0;

    if(mkfifo(name, mode) < 0) {
        return -1;
    }

    if((fd = open(name, oflag)) < 0) {
        return -1;
    }

    return fd;
}

int name_pipe_read(int fd, void* data, int maxlen)
{
    int nbyte = 0;

    nbyte = read(fd, data, maxlen);
    return nbyte;
}

int name_pipe_write(int fd, void* data, int len)
{
    int nret = 0;

    nret = write(fd, data, len);
    return nret;
}

void name_pipe_close(char* name, int fd)
{
    close(fd);
    unlink(name);
}

