/**
 ****************************************************************************************************
 * @file        atim.h
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

/* 高级定时器输出比较模式定义 */
#define ATIM_TIMX_COMP_CH1_GPIO_PORT            GPIOA
#define ATIM_TIMX_COMP_CH1_GPIO_PIN             GPIO_PIN_8
#define ATIM_TIMX_COMP_CH1_GPIO_AF              GPIO_AF1_TIM1
#define ATIM_TIMX_COMP_CH1_GPIO_CLK_ENABLE()    do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)

#define ATIM_TIMX_COMP_CH2_GPIO_PORT            GPIOA
#define ATIM_TIMX_COMP_CH2_GPIO_PIN             GPIO_PIN_9
#define ATIM_TIMX_COMP_CH2_GPIO_AF              GPIO_AF1_TIM1
#define ATIM_TIMX_COMP_CH2_GPIO_CLK_ENABLE()    do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)

#define ATIM_TIMX_COMP_CH3_GPIO_PORT            GPIOA
#define ATIM_TIMX_COMP_CH3_GPIO_PIN             GPIO_PIN_10
#define ATIM_TIMX_COMP_CH3_GPIO_AF              GPIO_AF1_TIM1
#define ATIM_TIMX_COMP_CH3_GPIO_CLK_ENABLE()    do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)

#define ATIM_TIMX_COMP_CH4_GPIO_PORT            GPIOA
#define ATIM_TIMX_COMP_CH4_GPIO_PIN             GPIO_PIN_11
#define ATIM_TIMX_COMP_CH4_GPIO_AF              GPIO_AF1_TIM1
#define ATIM_TIMX_COMP_CH4_GPIO_CLK_ENABLE()    do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)

#define ATIM_TIMX_COMP                          TIM1            
#define ATIM_TIMX_COMP_CH1_CCRX                 ATIM_TIMX_COMP->CCR1
#define ATIM_TIMX_COMP_CH2_CCRX                 ATIM_TIMX_COMP->CCR2
#define ATIM_TIMX_COMP_CH3_CCRX                 ATIM_TIMX_COMP->CCR3
#define ATIM_TIMX_COMP_CH4_CCRX                 ATIM_TIMX_COMP->CCR4
#define ATIM_TIMX_COMP_CLK_ENABLE()             do{ __HAL_RCC_TIM1_CLK_ENABLE(); }while(0)

/* 函数声明 */
void atim_timx_comp_pwm_init(uint16_t arr, uint16_t psc);   /* 初始化高级定时器输出比较模式 */

#endif
