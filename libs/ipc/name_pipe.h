/*******************************************************************************
 * @             Copyright (c) 2023 LinyPower, All Rights Reserved.
 * @*****************************************************************************
 * @FilePath     : name_pipe.h
 * @Descripttion : 
 * @Author       : zhums
 * @E-Mail       : zhums@linygroup.cn
 * @Version      : v1.0.0
 * @Date         : 2023-07-04 09:30:33
*******************************************************************************/
#ifndef __NAME_PIPE_H__
#define __NAME_PIPE_H__

int name_pipe_make(char* name, mode_t mode, int oflag);
int name_pipe_read(int fd, void* data, int maxlen);
int name_pipe_write(int fd, void* data, int len);
void name_pipe_close(char* name, int fd);

#endif //__NAME_PIPE_H__
