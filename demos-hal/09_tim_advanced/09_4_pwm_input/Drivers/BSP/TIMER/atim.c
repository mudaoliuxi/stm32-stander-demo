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

TIM_HandleTypeDef g_timx_pwmin_chy_handle;  /* 定时器x句柄 */

uint8_t g_timxchy_pwmin_sta  = 0;           /* PWM输入状态，0：没有捕获，1：成功捕获 */
uint32_t g_timxchy_pwmin_hval = 0 ;         /* PWM的高电平脉宽 */
uint32_t g_timxchy_pwmin_cval = 0 ;         /* PWM的周期宽度 */

/**
 * @brief       初始化高级定时器PWM输入模式
 * @note
 *              通用定时器的时钟来自APB1,当D2PPRE1≥2分频的时候
 *              通用定时器的时钟为APB1时钟的2倍, 而APB1为120M, 所以定时器时钟 = 240Mhz
 *              定时器溢出时间计算方法: Tout = ((arr + 1) * (psc + 1)) / Ft us.
 *              Ft=定时器工作频率,单位:MHz
 * @param       psc: 预分频器数值
 * @retval      无
 */
void atim_timx_pwmin_chy_init(uint16_t psc)
{
    GPIO_InitTypeDef gpio_init_struct = {0};
    TIM_SlaveConfigTypeDef slave_config = {0};
    TIM_IC_InitTypeDef tim_ic_pwmin_chy = {0};
    
    /* 使能时钟 */
    ATIM_TIMX_PWMIN_CHY_CLK_ENABLE();                                                       /* 使能高级定时器时钟 */
    ATIM_TIMX_PWMIN_CHY_GPIO_CLK_ENABLE();                                                  /* 使能PWM输入引脚端口时钟 */
    
    /* 配置PWM输入引脚 */
    gpio_init_struct.Pin = ATIM_TIMX_PWMIN_CHY_GPIO_PIN;                                    /* PWM输入引脚 */
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;                                                /* 复用功能模式 */
    gpio_init_struct.Pull = GPIO_PULLDOWN;                                                  /* 下拉 */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH ;                                    /* 高速 */
    gpio_init_struct.Alternate = ATIM_TIMX_PWMIN_CHY_GPIO_AF;                               /* 配置引脚复用功能 */
    HAL_GPIO_Init(ATIM_TIMX_PWMIN_CHY_GPIO_PORT, &gpio_init_struct);                        /* 配置PWM输入引脚 */
    
    /* 配置高级定时器 */
    g_timx_pwmin_chy_handle.Instance = ATIM_TIMX_PWMIN;                                     /* 高级定时器8 */
    g_timx_pwmin_chy_handle.Init.Prescaler = psc;                                           /* 定时器预分频系数 */
    g_timx_pwmin_chy_handle.Init.CounterMode = TIM_COUNTERMODE_UP;                          /* 递增计数模式 */
    g_timx_pwmin_chy_handle.Init.Period = 0xFFFF;                                           /* 自动重装载值 */
    HAL_TIM_IC_Init(&g_timx_pwmin_chy_handle);                                              /* 配置高级定时器 */
    
    /* 配置从模式 */
    slave_config.SlaveMode = TIM_SLAVEMODE_RESET;                                           /* 从模式: 复位模式 */
    slave_config.InputTrigger = TIM_TS_TI1FP1;                                              /* 定时器输入触发源: TI1FP1 */
    slave_config.TriggerPolarity = TIM_INPUTCHANNELPOLARITY_RISING;                         /* 上升沿检测 */
    slave_config.TriggerFilter = 0;                                                         /* 不滤波 */
    HAL_TIM_SlaveConfigSynchronization(&g_timx_pwmin_chy_handle, &slave_config);            /* 配置从模式 */
    
    /* IC1捕获：上升沿触发TI1FP1 */
    tim_ic_pwmin_chy.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;                          /* 上升沿检测 */
    tim_ic_pwmin_chy.ICSelection = TIM_ICSELECTION_DIRECTTI;                                /* 选择输入端 IC1映射到TI1上 */
    tim_ic_pwmin_chy.ICPrescaler = TIM_ICPSC_DIV1;                                          /* 不分频 */
    tim_ic_pwmin_chy.ICFilter = 0;                                                          /* 不滤波 */
    HAL_TIM_IC_ConfigChannel(&g_timx_pwmin_chy_handle, &tim_ic_pwmin_chy, TIM_CHANNEL_1);   /* 配置捕获通道1 */
    
    /* IC2捕获：下降沿触发TI1FP2 */
    tim_ic_pwmin_chy.ICPolarity = TIM_INPUTCHANNELPOLARITY_FALLING;                         /* 下降沿检测 */
    tim_ic_pwmin_chy.ICSelection = TIM_ICSELECTION_INDIRECTTI;                              /* 选择输入端 IC2映射到TI1上 */
    tim_ic_pwmin_chy.ICPrescaler = TIM_ICPSC_DIV1;                                          /* 不分频 */
    tim_ic_pwmin_chy.ICFilter = 0;                                                          /* 不滤波 */
    HAL_TIM_IC_ConfigChannel(&g_timx_pwmin_chy_handle, &tim_ic_pwmin_chy, TIM_CHANNEL_2);   /* 配置捕获通道2 */
    
    /* 使能高级定时器及其相关中断 */
    HAL_NVIC_SetPriority(ATIM_TIMX_PWMIN_CC_IRQn, 1, 0);                                    /* 设置中断优先级，抢占优先级1，子优先级0 */
    HAL_NVIC_EnableIRQ(ATIM_TIMX_PWMIN_CC_IRQn);                                            /* 使能输入捕获中断 */
    __HAL_TIM_ENABLE_IT(&g_timx_pwmin_chy_handle, TIM_IT_UPDATE);                           /* 使能更新中断 */
    HAL_TIM_IC_Start_IT(&g_timx_pwmin_chy_handle, TIM_CHANNEL_1);                           /* 使能CC1中断 */
    HAL_TIM_IC_Start_IT(&g_timx_pwmin_chy_handle, TIM_CHANNEL_2);                           /* 使能CC2中断 */
    __HAL_TIM_ENABLE_IT(&g_timx_pwmin_chy_handle, TIM_IT_CC1);                              /* 使能通道1捕获中断 */
    __HAL_TIM_ENABLE_IT(&g_timx_pwmin_chy_handle, TIM_IT_CC2);                              /* 使能通道2捕获中断 */
    __HAL_TIM_ENABLE(&g_timx_pwmin_chy_handle);                                             /* 使能高级定时器 */
}

/**
 * @brief       高级定时器中断服务函数
 * @param       无
 * @retval      无
 */
void ATIM_TIMX_PWMIN_CC_IRQHandler(void)
{
    if (__HAL_TIM_GET_FLAG(&g_timx_pwmin_chy_handle, TIM_FLAG_CC1) == SET)                              /* 判断CC1中断标志 */
    {
        /* 捕获到上升沿 */
        g_timxchy_pwmin_cval = HAL_TIM_ReadCapturedValue(&g_timx_pwmin_chy_handle, TIM_CHANNEL_1) + 1;  /* 周期捕获值 */
        g_timxchy_pwmin_sta = 1;                                                                        /* 标记捕获成功 */
        
        __HAL_TIM_CLEAR_FLAG(&g_timx_pwmin_chy_handle, TIM_FLAG_CC1);                                   /* 清除CC1中断标记 */
    }
    
    if (__HAL_TIM_GET_FLAG(&g_timx_pwmin_chy_handle, TIM_FLAG_CC2) == SET)                              /* 判断CC2中断标志 */
    {
        /* 捕获到下降沿 */
        g_timxchy_pwmin_hval = HAL_TIM_ReadCapturedValue(&g_timx_pwmin_chy_handle, TIM_CHANNEL_2) + 1;  /* 高定平脉宽捕获值 */
        __HAL_TIM_CLEAR_FLAG(&g_timx_pwmin_chy_handle, TIM_FLAG_CC2);                                   /* 清除CC2中断标记 */
    }
}
