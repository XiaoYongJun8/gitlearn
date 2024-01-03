/*******************************************************************************
 * @             Copyright (c) 2023 LinyPower, All Rights Reserved.
 * @*****************************************************************************
 * @FilePath     : hmi_show_dialog.c
 * @Descripttion : 
 * @Author       : zhums
 * @E-Mail       : zhums@linygroup.cn
 * @Version      : v1.0.0
 * @Date         : 2023-11-08 13:54:04
*******************************************************************************/
#include "hmi_show_dialog.h"
#include "string.h"
#include "modbus.h"
#include "hmi_comm.h"
#include "debug_and_log.h"

extern modbus_t* g_ctx;

int hmi_chinese_conver(char* psrc, uint16_t* pdest)
{
    int len = 0, i = 0, pos = 0;

    len = strlen(psrc);

    for(i = 0; i < len; i += 2) {
        pdest[pos++] = psrc[i] << 8 | psrc[i+1];
    }

    return pos;
}

//此函数未返回前，当前线程应阻塞在此函数中
int hmi_show_msgbox(char* msg, char* title, uint32_t btns, enum DBOX_ICON icon)
{
    int click_btn = BUTTON_NONE;
    int ret = 0, len = 0;
    uint16_t wdata[64] = { 0 };
    //根据函数参数，设置对话框界面相应内容

    //0. 清空字符显示地址
    for(len = 0; len < 40; len++) {
        wdata[len] = 0;
    }
    modbus_write_registers(g_ctx, TITLE_ADDR, 40, &wdata[0]);
    modbus_write_registers(g_ctx, BUTTON_NUM_ADDR, 1, wdata);
    //1. 设置标题列icon
    if(icon == ICON_INFO) {
        wdata[0] = 0;
        modbus_write_registers(g_ctx, ICON_ADDR, 1, wdata);
    } else if(icon == ICON_WARN) {
        wdata[0] = 1;
        modbus_write_registers(g_ctx, ICON_ADDR, 1, wdata);
    } else if(icon == ICON_ERROR) {
        wdata[0] = 2;
        modbus_write_registers(g_ctx, ICON_ADDR, 1, wdata);
    } else {

    }
    //2. 设置标题
    len = hmi_chinese_conver(title, &wdata[0]);
    modbus_write_registers(g_ctx, TITLE_ADDR, len, &wdata[0]);

    //3. 设置内容
    len = hmi_chinese_conver(msg, &wdata[0]);
    modbus_write_registers(g_ctx, CONTEXT_ADDR, len, wdata);

    //4. 设置要显示的按钮
    if(btns == BUTTON_OK) {
        wdata[0] = 1;
        modbus_write_registers(g_ctx, BUTTON_NUM_ADDR, 1, wdata);
    } else if(btns == (BUTTON_OK | BUTTON_NO)) {
        wdata[0] = 3;
        modbus_write_registers(g_ctx, BUTTON_NUM_ADDR, 1, wdata);
    } else {
        
    }
    //5. 清除对话框操作完成标志
    wdata[0] = 0;
    wdata[1] = 0;
    modbus_write_registers(g_ctx, CLEAR_WINDOW_ADDR, 2, wdata);

    //6. 显示对话框界面
    wdata[0] = 1;
    modbus_write_registers(g_ctx, SHOW_WINDOW_ADDR, 1, wdata);
    //7. 轮询对话框操作完成标志
    ret = 0;
    wdata[0] = 0;
    while(1) {
        LogEMSHMI("------wait on operation duihua-------\n");
        ret = modbus_read_registers(g_ctx, CLEAR_WINDOW_ADDR, 1, wdata);
        if((ret == 1) && (wdata[0] != 0)) {
            break;
        }
    }
    //8. 读取点击的按钮
    click_btn = wdata[0];

    //9. 清除对话框操作完成标志
    wdata[0] = 0;
    wdata[1] = 0;
    modbus_write_registers(g_ctx, CLEAR_WINDOW_ADDR, 2, wdata);
    
    return click_btn;
}
