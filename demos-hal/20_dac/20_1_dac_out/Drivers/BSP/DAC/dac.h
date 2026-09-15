/**
 ****************************************************************************************************
 * @file        dac.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-04-25
 * @brief       DAC驱动代码
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 M100Z-M7最小系统板STM32H750版
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 * 
 ****************************************************************************************************
 */

#ifndef __DAC_H
#define __DAC_H

#include "./SYSTEM/sys/sys.h"

extern DAC_HandleTypeDef g_dac_handle;              /* DAC句柄 */

/* 函数声明 */
void dac_init(uint8_t outx);                        /* 初始化DAC */
void dac_set_voltage(uint8_t outx, uint16_t vol);   /* 设置DAC输出电压 */

#endif
