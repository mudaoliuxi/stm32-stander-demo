/**
 ****************************************************************************************************
 * @file        gtim.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-04-25
 * @brief       通用定时器驱动代码
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

#ifndef __GTIM_H
#define __GTIM_H

#include "./SYSTEM/sys/sys.h"

/* 通用定时器PWM输出定义 */
#define GTIM_TIMX_PWM                       TIM14
#define GTIM_TIMX_PWM_CHY                   TIM_CHANNEL_1
#define GTIM_TIMX_PWM_CHY_CLK_ENABLE()      do{ __HAL_RCC_TIM14_CLK_ENABLE(); }while(0)

#define GTIM_TIMX_PWM_CHY_GPIO_PORT         GPIOA
#define GTIM_TIMX_PWM_CHY_GPIO_PIN          GPIO_PIN_7
#define GTIM_TIMX_PWM_CHY_GPIO_AF           GPIO_AF9_TIM14
#define GTIM_TIMX_PWM_CHY_GPIO_CLK_ENABLE() do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)

/* 函数声明 */
void gtim_timx_pwm_chy_init(uint16_t arr, uint16_t psc);    /* 初始化通用定时器PWM输出 */

#endif
