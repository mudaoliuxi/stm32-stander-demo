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

TIM_HandleTypeDef a_timx_npwm_chy_handle;     /* 高级定时器句柄 */

/* g_npwm_remain表示当前还剩下多少个脉冲要发送 
 * 每次最多发送256个脉冲
 */
static uint32_t g_npwm_remain = 0;

/**
 * @brief       初始化高级定时器输出指定个数PWM
 * @note        高级定时器的时钟来自APB1,当D2PPRE1≥2分频的时候
 *              高级定时器的时钟为APB1时钟的2倍, 而APB1为120M, 所以定时器时钟 = 240MHz
 *              定时器溢出时间计算方法: Tout = ((arr + 1) * (psc + 1)) / Ft us.
 *              Ft=定时器工作频率,单位:Mhz
 * @param       arr: 自动重装值
 * @param       psc: 时钟预分频数
 * @retval      无
 */
void atim_timx_npwm_chy_init(uint16_t arr,uint16_t psc)
{
    GPIO_InitTypeDef gpio_init_struct;
    TIM_OC_InitTypeDef timx_oc_npwm_chy = {0};                                                  /* 定时器输出 */
    
    /* 使能时钟 */
    ATIM_TIMX_NPWM_CHY_GPIO_CLK_ENABLE();                                                       /* 开启通道y的GPIO时钟 */
    ATIM_TIMX_NPWM_CHY_CLK_ENABLE();
    
    /* 配置高级定时器 */
    a_timx_npwm_chy_handle.Instance = ATIM_TIMX_NPWM;                                           /* 高级定时8 */
    a_timx_npwm_chy_handle.Init.Prescaler = psc;                                                /* 定时器分频 */
    a_timx_npwm_chy_handle.Init.CounterMode = TIM_COUNTERMODE_UP;                               /* 递增计数模式 */
    a_timx_npwm_chy_handle.Init.Period = arr;                                                   /* 自动重装载值 */
    a_timx_npwm_chy_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;              /* 使能TIMx_ARR 寄存器进行缓冲 */
    a_timx_npwm_chy_handle.Init.RepetitionCounter = 0;                                          /* 重复计数器初始值 */
    HAL_TIM_PWM_Init(&a_timx_npwm_chy_handle);                                                  /* 初始化PWM */
    
    /* 配置PWM输出引脚 */
    gpio_init_struct.Pin = ATIM_TIMX_NPWM_CHY_GPIO_PIN;                                         /* PWM输出引脚 */
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;                                                    /* 复用推挽输出 */
    gpio_init_struct.Pull = GPIO_PULLDOWN;                                                      /* 下拉 */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;                                         /* 高速 */
    gpio_init_struct.Alternate = ATIM_TIMX_NPWM_CHY_GPIO_AF;                                    /* 配置引脚复用功能 */
    HAL_GPIO_Init(ATIM_TIMX_NPWM_CHY_GPIO_PORT, &gpio_init_struct);                             /* 配置PWM输出引脚 */
    
    /* 配置输出比较通道 */
    timx_oc_npwm_chy.OCMode = TIM_OCMODE_PWM1;                                                  /* 模式选择PWM1 */
    timx_oc_npwm_chy.Pulse = (arr + 1) >> 1;                                                    /* PWM有效电平脉宽 */
    timx_oc_npwm_chy.OCPolarity = TIM_OCPOLARITY_LOW;                                           /* 输出比较极性为低 */
    if (ATIM_TIMX_NPWM_CHY == TIM_CHANNEL_1)
    {
        HAL_TIM_PWM_ConfigChannel(&a_timx_npwm_chy_handle, &timx_oc_npwm_chy, TIM_CHANNEL_1);   /* 配置输出比较通道1 */
    }
    else if(ATIM_TIMX_NPWM_CHY == TIM_CHANNEL_2)
    {
        HAL_TIM_PWM_ConfigChannel(&a_timx_npwm_chy_handle, &timx_oc_npwm_chy, TIM_CHANNEL_2);   /* 配置输出比较通道2 */
    }
    else if(ATIM_TIMX_NPWM_CHY == TIM_CHANNEL_3)
    {
        HAL_TIM_PWM_ConfigChannel(&a_timx_npwm_chy_handle, &timx_oc_npwm_chy, TIM_CHANNEL_3);   /* 配置输出比较通道3 */
    }
    else if(ATIM_TIMX_NPWM_CHY == TIM_CHANNEL_4)
    {
        HAL_TIM_PWM_ConfigChannel(&a_timx_npwm_chy_handle, &timx_oc_npwm_chy, TIM_CHANNEL_4);   /* 配置输出比较通道4 */
    }
    
    /* 使能高级定时器及其相关中断 */
    HAL_NVIC_SetPriority(ATIM_TIMX_NPWM_IRQn, 1, 3);                                            /* 设置中断优先级，抢占优先级1，子优先级3 */
    HAL_NVIC_EnableIRQ(ATIM_TIMX_NPWM_IRQn);                                                    /* 开启ITMx中断 */
    __HAL_TIM_ENABLE_IT(&a_timx_npwm_chy_handle, TIM_IT_UPDATE);                                /* 使能更新中断 */
    HAL_TIM_PWM_Start(&a_timx_npwm_chy_handle, ATIM_TIMX_NPWM_CHY);                             /* 使能输出比较通道输出 */
}

/**
 * @brief       设置高级定时器PWM个数
 * @param       npwm: PWM的个数,范围1~2^32
 * @retval      无
 */
void atim_timx_npwm_chy_set(uint32_t npwm)
{
    if (npwm == 0)
    {
        return ;
    }
    
    g_npwm_remain = npwm;                                                   /* 保存脉冲个数 */
    HAL_TIM_GenerateEvent(&a_timx_npwm_chy_handle, TIM_EVENTSOURCE_UPDATE); /* 产生一次更新事件,在中断里面处理脉冲输出 */
    __HAL_TIM_ENABLE(&a_timx_npwm_chy_handle);                              /* 使能定时器TIMX */
}

/**
 * @brief       高级定时器中断服务函数
 * @param       无
 * @retval      无
 */
void ATIM_TIMX_NPWM_IRQHandler(void)
{
    uint16_t npwm = 0;
    
    if(__HAL_TIM_GET_FLAG(&a_timx_npwm_chy_handle, TIM_FLAG_UPDATE) == SET)         /* 判断更新中断标志 */
    {
        if (g_npwm_remain >= 256)                                                   /* 还有大于或等于256个脉冲需要发送 */
        {
            g_npwm_remain -= 256;                                                   /* 每次最多发送256个脉冲 */
            npwm = 256;
        }
        else if (g_npwm_remain % 256)                                               /* 剩余发送脉冲数量不超过256 */
        {
            npwm = g_npwm_remain % 256;                                             /* 发送剩余脉冲 */
            g_npwm_remain = 0;                                                      /* 没有脉冲了 */
        }
        if (npwm != 0)                                                              /* 有脉冲要发送 */
        { 
            ATIM_TIMX_NPWM->RCR = npwm - 1;                                         /* 设置重复计数寄存器值为npwm-1, 即npwm个脉冲 */
            HAL_TIM_GenerateEvent(&a_timx_npwm_chy_handle, TIM_EVENTSOURCE_UPDATE); /* 产生一次更新事件,以更新RCR寄存器 */
            __HAL_TIM_ENABLE(&a_timx_npwm_chy_handle);                              /* 使能高级定时器 */
        }
        else
        { 
            ATIM_TIMX_NPWM->CR1 &= ~(1 << 0);                                       /* 关闭高级定时器 */
        }
        
        __HAL_TIM_CLEAR_IT(&a_timx_npwm_chy_handle, TIM_IT_UPDATE);                 /* 清除定时器溢出中断标志位 */
    }
}
