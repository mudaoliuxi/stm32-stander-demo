/**
 ****************************************************************************************************
 * @file        exti.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-04-25
 * @brief       外部中断驱动代码
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

#ifndef __EXTI_H
#define __EXTI_H

#include "./SYSTEM/sys/sys.h"

/* 引脚、外部中断编号和中断服务函数定义 */ 
#define KEY0_INT_GPIO_PORT          GPIOA
#define KEY0_INT_GPIO_PIN           GPIO_PIN_15
#define KEY0_INT_GPIO_CLK_ENABLE()  do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)
#define KEY0_INT_EXTI_LINE          EXTI_LINE_15
#define KEY0_INT_IRQn               EXTI15_10_IRQn
#define KEY0_INT_IRQHandler         EXTI15_10_IRQHandler

#define WKUP_INT_GPIO_PORT          GPIOA
#define WKUP_INT_GPIO_PIN           GPIO_PIN_0
#define WKUP_INT_GPIO_CLK_ENABLE()  do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)
#define WKUP_INT_EXTI_LINE          EXTI_LINE_0
#define WKUP_INT_IRQn               EXTI0_IRQn
#define WKUP_INT_IRQHandler         EXTI0_IRQHandler

/* 函数声明 */
void extix_init(void);  /* 初始化外部中断 */

#endif
