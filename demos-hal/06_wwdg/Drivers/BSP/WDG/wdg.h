/**
 ****************************************************************************************************
 * @file        wdg.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-04-25
 * @brief       看门狗驱动代码
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

#ifndef __WDG_H
#define __WDG_H

#include "./SYSTEM/sys/sys.h"

/* 函数声明 */
void iwdg_init(uint8_t prer, uint16_t rlr);             /* 初始化独立看门狗 */
void iwdg_feed(void);                                   /* 喂独立看门狗 */
void wwdg_init(uint8_t tr, uint8_t wr, uint32_t fprer); /* 初始化窗口看门狗 */

#endif
