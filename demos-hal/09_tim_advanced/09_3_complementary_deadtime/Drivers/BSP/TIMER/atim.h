/**
 ****************************************************************************************************
 * @file        atim.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-04-25
 * @brief       高级定时器驱动代码
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

#ifndef __ATIM_H
#define __ATIM_H

#include "./SYSTEM/sys/sys.h"

/* 高级定时器互补输出带死区控制定义 */
#define ATIM_TIMX_CPLM_CHY_GPIO_PORT            GPIOC
#define ATIM_TIMX_CPLM_CHY_GPIO_PIN             GPIO_PIN_6
#define ATIM_TIMX_CPLM_CHY_GPIO_AF              GPIO_AF3_TIM8
#define ATIM_TIMX_CPLM_CHY_GPIO_CLK_ENABLE()    do{ __HAL_RCC_GPIOC_CLK_ENABLE(); }while(0)

#define ATIM_TIMX_CPLM_CHYN_GPIO_PORT           GPIOA
#define ATIM_TIMX_CPLM_CHYN_GPIO_PIN            GPIO_PIN_7
#define ATIM_TIMX_CPLM_CHYN_GPIO_AF             GPIO_AF3_TIM8
#define ATIM_TIMX_CPLM_CHYN_GPIO_CLK_ENABLE()   do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)

#define ATIM_TIMX_CPLM_BKIN_GPIO_PORT           GPIOA
#define ATIM_TIMX_CPLM_BKIN_GPIO_PIN            GPIO_PIN_6
#define ATIM_TIMX_CPLM_BKIN_GPIO_AF             GPIO_AF3_TIM8
#define ATIM_TIMX_CPLM_BKIN_GPIO_CLK_ENABLE()   do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)

#define ATIM_TIMX_CPLM                          TIM8
#define ATIM_TIMX_CPLM_CHY                      TIM_CHANNEL_1
#define ATIM_TIMX_CPLM_CHY_CCRY                 ATIM_TIMX_CPLM->CCR1
#define ATIM_TIMX_CPLM_CLK_ENABLE()             do{ __HAL_RCC_TIM8_CLK_ENABLE(); }while(0)

/* 函数声明 */
void atim_timx_cplm_pwm_init(uint16_t arr, uint16_t psc);   /* 初始化高级定时器互补PWM输出 */
void atim_timx_cplm_pwm_set(uint16_t ccr, uint8_t dtg);     /* 设置高级定时器输出比较值和死区时间 */

#endif
