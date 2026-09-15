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

#include "./BSP/TIMER/atim.h"
#include "./BSP/LED/led.h"

TIM_HandleTypeDef g_timx_comp_pwm_handle;   /* 定时器x句柄 */

/**
 * @brief       初始化高级定时器输出比较模式
 * @note
 *              配置高级定时器TIMX 4路输出比较模式PWM输出,实现50%占空比,不同相位控制
 *              注意,本例程输出比较模式,每2个计数周期才能完成一个PWM输出,因此输出频率减半
 *              另外,我们还可以开启中断在中断里面修改CCRx,从而实现不同频率/不同相位的控制
 *              但是我们不推荐这么使用,因为这可能导致非常频繁的中断,从而占用大量CPU资源
 *
 *              高级定时器的时钟来自APB1,当D2PPRE2≥2分频的时候
 *              高级定时器的时钟为APB2时钟的2倍, 而APB2为120M, 所以定时器时钟 = 240Mhz
 *              定时器溢出时间计算方法: Tout = ((arr + 1) * (psc + 1)) / Ft us.
 *              Ft=定时器工作频率,单位:Mhz
 *
 * @param       arr: 自动重装值。
 * @param       psc: 时钟预分频数
 * @retval      无
 */
void atim_timx_comp_pwm_init(uint16_t arr, uint16_t psc)
{
    TIM_OC_InitTypeDef timx_oc_comp_pwm = {0};
    GPIO_InitTypeDef gpio_init_struct = {0};
    
    /* 使能时钟 */
    ATIM_TIMX_COMP_CLK_ENABLE();                                                            /* 使能高级定时器时钟 */
    ATIM_TIMX_COMP_CH1_GPIO_CLK_ENABLE();                                                   /* 使能输出比较通道1端口时钟 */
    ATIM_TIMX_COMP_CH2_GPIO_CLK_ENABLE();                                                   /* 使能输出比较通道2端口时钟 */
    ATIM_TIMX_COMP_CH3_GPIO_CLK_ENABLE();                                                   /* 使能输出比较通道3端口时钟 */
    ATIM_TIMX_COMP_CH4_GPIO_CLK_ENABLE();                                                   /* 使能输出比较通道4端口时钟 */
    
    /* 配置输出比较通道1输出引脚 */
    gpio_init_struct.Pin = ATIM_TIMX_COMP_CH1_GPIO_PIN;                                     /* 输出比较通道1输出引脚 */
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;                                                /* 复用功能模式 */
    gpio_init_struct.Pull = GPIO_NOPULL;                                                    /* 无上下拉 */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;                                          /* 高速 */
    gpio_init_struct.Alternate = ATIM_TIMX_COMP_CH1_GPIO_AF;                                /* 配置引脚复用功能 */
    HAL_GPIO_Init(ATIM_TIMX_COMP_CH1_GPIO_PORT, &gpio_init_struct);                         /* 配置输出比较通道1输出引脚 */
    
    /* 配置输出比较通道2输出引脚 */
    gpio_init_struct.Pin = ATIM_TIMX_COMP_CH2_GPIO_PIN;                                     /* 输出比较通道2输出引脚 */
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;                                                /* 复用功能模式 */
    gpio_init_struct.Pull = GPIO_NOPULL;                                                    /* 无上下拉 */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;                                          /* 高速 */
    gpio_init_struct.Alternate = ATIM_TIMX_COMP_CH2_GPIO_AF;                                /* 配置引脚复用功能 */
    HAL_GPIO_Init(ATIM_TIMX_COMP_CH2_GPIO_PORT, &gpio_init_struct);                         /* 配置输出比较通道2输出引脚 */
    
    /* 配置输出比较通道3输出引脚 */
    gpio_init_struct.Pin = ATIM_TIMX_COMP_CH3_GPIO_PIN;                                     /* 输出比较通道3输出引脚 */
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;                                                /* 复用功能模式 */
    gpio_init_struct.Pull = GPIO_NOPULL;                                                    /* 无上下拉 */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;                                          /* 高速 */
    gpio_init_struct.Alternate = ATIM_TIMX_COMP_CH3_GPIO_AF;                                /* 配置引脚复用功能 */
    HAL_GPIO_Init(ATIM_TIMX_COMP_CH3_GPIO_PORT, &gpio_init_struct);                         /* 配置输出比较通道3输出引脚 */
    
    /* 配置输出比较通道4输出引脚 */
    gpio_init_struct.Pin = ATIM_TIMX_COMP_CH4_GPIO_PIN;                                     /* 输出比较通道4输出引脚 */
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;                                                /* 复用功能模式 */
    gpio_init_struct.Pull = GPIO_NOPULL;                                                    /* 无上下拉 */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;                                          /* 高速 */
    gpio_init_struct.Alternate = ATIM_TIMX_COMP_CH4_GPIO_AF;                                /* 配置引脚复用功能 */
    HAL_GPIO_Init(ATIM_TIMX_COMP_CH4_GPIO_PORT, &gpio_init_struct);                         /* 配置输出比较通道4输出引脚 */
    
    /* 配置高级定时器 */
    g_timx_comp_pwm_handle.Instance = ATIM_TIMX_COMP;                                       /* 高级定时器1 */
    g_timx_comp_pwm_handle.Init.Prescaler = psc  ;                                          /* 定时器分频 */
    g_timx_comp_pwm_handle.Init.CounterMode = TIM_COUNTERMODE_UP;                           /* 递增计数模式 */
    g_timx_comp_pwm_handle.Init.Period = arr;                                               /* 自动重装载值 */
    g_timx_comp_pwm_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;          /* 使能影子寄存器TIMx_ARR */
    HAL_TIM_OC_Init(&g_timx_comp_pwm_handle);                                               /* 输出比较模式初始化 */
    
    /* 配置输出比较通道1 */
    timx_oc_comp_pwm.OCMode = TIM_OCMODE_TOGGLE;                                            /* 比较输出模式翻转功能 */
    timx_oc_comp_pwm.Pulse = (arr + 1) >> 1;                                                /* 设置输出比较寄存器的值 */
    timx_oc_comp_pwm.OCPolarity = TIM_OCPOLARITY_HIGH;                                      /* 输出比较极性为高 */
    HAL_TIM_OC_ConfigChannel(&g_timx_comp_pwm_handle, &timx_oc_comp_pwm, TIM_CHANNEL_1);    /* 初始化定时器的输出比较通道1 */
    __HAL_TIM_ENABLE_OCxPRELOAD(&g_timx_comp_pwm_handle, TIM_CHANNEL_1);                    /* 通道1预装载使能 */
    
    /* 配置输出比较通道2 */
    timx_oc_comp_pwm.OCMode = TIM_OCMODE_TOGGLE;                                            /* 比较输出模式翻转功能 */
    timx_oc_comp_pwm.Pulse = (arr + 1) >> 1;                                                /* 设置输出比较寄存器的值 */
    timx_oc_comp_pwm.OCPolarity = TIM_OCPOLARITY_HIGH;                                      /* 输出比较极性为高 */
    HAL_TIM_OC_ConfigChannel(&g_timx_comp_pwm_handle, &timx_oc_comp_pwm, TIM_CHANNEL_2);    /* 初始化定时器的输出比较通道2 */
    __HAL_TIM_ENABLE_OCxPRELOAD(&g_timx_comp_pwm_handle, TIM_CHANNEL_2);                    /* 通道2预装载使能 */
    
    /* 配置输出比较通道3 */
    timx_oc_comp_pwm.OCMode = TIM_OCMODE_TOGGLE;                                            /* 比较输出模式翻转功能 */
    timx_oc_comp_pwm.Pulse = (arr + 1) >> 1;                                                /* 设置输出比较寄存器的值 */
    timx_oc_comp_pwm.OCPolarity = TIM_OCPOLARITY_HIGH;                                      /* 输出比较极性为高 */
    HAL_TIM_OC_ConfigChannel(&g_timx_comp_pwm_handle, &timx_oc_comp_pwm, TIM_CHANNEL_3);    /* 初始化定时器的输出比较通道3 */
    __HAL_TIM_ENABLE_OCxPRELOAD(&g_timx_comp_pwm_handle, TIM_CHANNEL_3);                    /* 通道3预装载使能 */
    
    /* 配置输出比较通道4 */
    timx_oc_comp_pwm.OCMode = TIM_OCMODE_TOGGLE;                                            /* 比较输出模式翻转功能 */
    timx_oc_comp_pwm.Pulse = (arr + 1) >> 1;                                                /* 设置输出比较寄存器的值 */
    timx_oc_comp_pwm.OCPolarity = TIM_OCPOLARITY_HIGH;                                      /* 输出比较极性为高 */
    HAL_TIM_OC_ConfigChannel(&g_timx_comp_pwm_handle, &timx_oc_comp_pwm, TIM_CHANNEL_4);    /* 初始化定时器的输出比较通道4 */
    __HAL_TIM_ENABLE_OCxPRELOAD(&g_timx_comp_pwm_handle, TIM_CHANNEL_4);                    /* 通道4预装载使能 */
    
    /* 使能高级定时器和输出比较通道输出 */
    HAL_TIM_OC_Start(&g_timx_comp_pwm_handle, TIM_CHANNEL_1);                               /* 使能输出比较通道1输出 */
    HAL_TIM_OC_Start(&g_timx_comp_pwm_handle, TIM_CHANNEL_2);                               /* 使能输出比较通道2输出 */
    HAL_TIM_OC_Start(&g_timx_comp_pwm_handle, TIM_CHANNEL_3);                               /* 使能输出比较通道3输出 */
    HAL_TIM_OC_Start(&g_timx_comp_pwm_handle, TIM_CHANNEL_4);                               /* 使能输出比较通道4输出 */
}
