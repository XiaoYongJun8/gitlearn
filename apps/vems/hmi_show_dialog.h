/*******************************************************************************
 * @             Copyright (c) 2023 LinyPower, All Rights Reserved.
 * @*****************************************************************************
 * @FilePath     : hmi_show_dialog.h
 * @Descripttion : 
 * @Author       : xiaoyongjun
 * @E-Mail       : xiaoyongjun@linygroup.cn
 * @Version      : v1.0.0
 * @Date         : 2023-11-08 15:13:40
*******************************************************************************/
#ifndef _HMI_SHOW_DIALOG_H_
#define _HMI_SHOW_DIALOG_H_

#include "stdint.h"
enum DBOX_ICON
{
    ICON_INFO = 0,
    ICON_WARN,
    ICON_ERROR
};

enum DBOX_BUTTONS
{
    BUTTON_NONE = 0,
    BUTTON_OK = 0x00000001,
    BUTTON_NO = 0x00000002,
    
};

int hmi_chinese_conver(char* psrc, uint16_t* pdest);
int hmi_show_msgbox(char* msg, char* title, uint32_t btns, enum DBOX_ICON icon);


#endif

