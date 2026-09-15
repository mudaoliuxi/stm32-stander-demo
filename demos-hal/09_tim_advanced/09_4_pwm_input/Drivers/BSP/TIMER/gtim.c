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

#include "./BSP/TIMER/gtim.h"
#include "./BSP/LED/led.h"

TIM_HandleTypeDef g_timx_pwm_chy_handle;    /* 定时器x句柄 */

/**
 * @brief       初始化通用定时器PWM输出
 * @note
 *              通用定时器的时钟来自APB1,当D2PPRE1≥2分频的时候
 *              通用定时器的时钟为APB1时钟的2倍, 而APB1为120M, 所以定时器时钟 = 240Mhz
 *              定时器溢出时间计算方法: Tout = ((arr + 1) * (psc + 1)) / Ft us.
 *              Ft=定时器工作频率,单位:Mhz
 *
 * @param       arr: 自动重装值。
 * @param       psc: 时钟预分频数
 * @retval      无
 */
void gtim_timx_pwm_chy_init(uint16_t arr, uint16_t psc)
{
    GPIO_InitTypeDef gpio_init_struct;
    TIM_OC_InitTypeDef timx_oc_pwm_chy = {0};                                               /* 定时器输出句柄 */
    
    /* 使能时钟 */                              
    GTIM_TIMX_PWM_CHY_CLK_ENABLE();                                                         /* 使能通用定时器时钟 */
    GTIM_TIMX_PWM_CHY_GPIO_CLK_ENABLE();                                                    /* 使能PWM输出引脚端口时钟 */
    
    /* 配置PWM输出引脚 */                             
    gpio_init_struct.Pin = GTIM_TIMX_PWM_CHY_GPIO_PIN;                                      /* PWM输出引脚 */
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;                                                /* 复用推完输出 */
    gpio_init_struct.Pull = GPIO_PULLUP;                                                    /* 上拉 */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;                                          /* 高速 */
    gpio_init_struct.Alternate = GTIM_TIMX_PWM_CHY_GPIO_AF;                                 /* 配置引脚复用功能 */
    HAL_GPIO_Init(GTIM_TIMX_PWM_CHY_GPIO_PORT, &gpio_init_struct);                          /* 初始化PWM输出引脚 */
    
    /* 配置通用定时器 */
    g_timx_pwm_chy_handle.Instance = GTIM_TIMX_PWM;                                         /* 定时器15 */
    g_timx_pwm_chy_handle.Init.Prescaler = psc;                                             /* 定时器分频 */
    g_timx_pwm_chy_handle.Init.CounterMode = TIM_COUNTERMODE_UP;                            /* 递增计数模式 */
    g_timx_pwm_chy_handle.Init.Period = arr;                                                /* 自动重装载值 */
    HAL_TIM_PWM_Init(&g_timx_pwm_chy_handle);                                               /* 初始化PWM */
    
    /* 配置输出比较通道 */
    timx_oc_pwm_chy.OCMode = TIM_OCMODE_PWM1;                                               /* 模式选择PWM1 */
    timx_oc_pwm_chy.Pulse = (arr + 1) >> 1;                                                 /* PWM有效电平脉宽 */
    timx_oc_pwm_chy.OCPolarity = TIM_OCPOLARITY_LOW;                                        /* 输出比较极性为低 */
    
    if (GTIM_TIMX_PWM_CHY == TIM_CHANNEL_1)
    {
        HAL_TIM_PWM_ConfigChannel(&g_timx_pwm_chy_handle, &timx_oc_pwm_chy, TIM_CHANNEL_1); /* 配置输出比较通道1 */
    }
    else if (GTIM_TIMX_PWM_CHY == TIM_CHANNEL_2)
    {
        HAL_TIM_PWM_ConfigChannel(&g_timx_pwm_chy_handle, &timx_oc_pwm_chy, TIM_CHANNEL_2); /* 配置输出比较通道2 */
    }
    else if(GTIM_TIMX_PWM_CHY == TIM_CHANNEL_3)
    {
        HAL_TIM_PWM_ConfigChannel(&g_timx_pwm_chy_handle, &timx_oc_pwm_chy, TIM_CHANNEL_3); /* 配置输出比较通道3 */
    }
    else if(GTIM_TIMX_PWM_CHY == TIM_CHANNEL_4)
    {
        HAL_TIM_PWM_ConfigChannel(&g_timx_pwm_chy_handle, &timx_oc_pwm_chy, TIM_CHANNEL_4); /* 配置输出比较通道4 */
    }
    
    HAL_TIM_PWM_Start(&g_timx_pwm_chy_handle, GTIM_TIMX_PWM_CHY);                           /* 开启PWM通道输出 */
}
