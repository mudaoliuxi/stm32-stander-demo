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

TIM_HandleTypeDef g_timx_cplm_pwm_handle;                   /* 定时器x句柄 */
TIM_BreakDeadTimeConfigTypeDef g_sbreak_dead_time_config;   /* 死区时间设置 */

/**
 * @brief       初始化高级定时器互补PWM输出
 * @note        配置高级定时器TIMX 互补输出,一路OCy 一路OCyN,并且可以设置死区时间
 *              高级定时器的时钟来自APB1,当D2PPRE2≥2分频的时候
 *              高级定时器的时钟为APB2时钟的2倍, 而APB2为120M, 所以定时器时钟 = 240Mhz
 *              定时器溢出时间计算方法:Tout = ((arr + 1) * (psc + 1)) / Ft us.
 *              Ft=定时器工作频率,单位:Mhz
 * @param       arr: 自动重装值。
 * @param       psc: 时钟预分频数
 * @retval      无
 */
void atim_timx_cplm_pwm_init(uint16_t arr, uint16_t psc)
{
    TIM_OC_InitTypeDef tim_oc_cplm_pwm = {0};
    GPIO_InitTypeDef gpio_init_struct = {0};
    
    /* 使能时钟 */
    ATIM_TIMX_CPLM_CLK_ENABLE();                                                                /* 使能高级定时器时钟 */
    ATIM_TIMX_CPLM_CHY_GPIO_CLK_ENABLE();                                                       /* 使能PWM输出引脚端口时钟 */
    ATIM_TIMX_CPLM_CHYN_GPIO_CLK_ENABLE();                                                      /* 使能PWM互补输出引脚端口时钟 */
    ATIM_TIMX_CPLM_BKIN_GPIO_CLK_ENABLE();                                                      /* 使能刹车输入引脚端口时钟 */
    
    /* 配置PWM输出引脚 */
    gpio_init_struct.Pin = ATIM_TIMX_CPLM_CHY_GPIO_PIN;                                         /* PWM输出引脚 */
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;                                                    /* 复用功能模式 */
    gpio_init_struct.Pull = GPIO_PULLUP;                                                        /* 上拉 */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH ;                                             /* 高速 */
    gpio_init_struct.Alternate = ATIM_TIMX_CPLM_CHY_GPIO_AF;                                    /* 配置引脚复用功能 */
    HAL_GPIO_Init(ATIM_TIMX_CPLM_CHY_GPIO_PORT, &gpio_init_struct);                             /* 配置PWM输出引脚 */
    
    /* 配置PWM互补输出引脚 */
    gpio_init_struct.Pin = ATIM_TIMX_CPLM_CHYN_GPIO_PIN;                                        /* PWM互补输出引脚 */
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;                                                    /* 复用功能模式 */
    gpio_init_struct.Pull = GPIO_PULLUP;                                                        /* 上拉 */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH ;                                             /* 高速 */
    gpio_init_struct.Alternate = ATIM_TIMX_CPLM_CHYN_GPIO_AF;                                   /* 配置引脚复用功能 */
    HAL_GPIO_Init(ATIM_TIMX_CPLM_CHYN_GPIO_PORT, &gpio_init_struct);                            /* 配置PWM输出引脚 */
    
    /* 配置刹车输入引脚 */
    gpio_init_struct.Pin = ATIM_TIMX_CPLM_BKIN_GPIO_PIN;                                        /* 刹车输入引脚 */
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;                                                    /* 复用功能模式 */
    gpio_init_struct.Pull = GPIO_PULLUP;                                                        /* 上拉 */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH ;                                             /* 高速 */
    gpio_init_struct.Alternate = ATIM_TIMX_CPLM_BKIN_GPIO_AF;                                   /* 配置引脚复用功能 */
    HAL_GPIO_Init(ATIM_TIMX_CPLM_BKIN_GPIO_PORT, &gpio_init_struct);                            /* 配置PWM输出引脚 */
    
    /* 配置高级定时器 */
    g_timx_cplm_pwm_handle.Instance = ATIM_TIMX_CPLM;                                           /* 高级定时器 */
    g_timx_cplm_pwm_handle.Init.Prescaler = psc;                                                /* 定时器预分频系数 */
    g_timx_cplm_pwm_handle.Init.CounterMode = TIM_COUNTERMODE_UP;                               /* 递增计数模式 */
    g_timx_cplm_pwm_handle.Init.Period = arr;                                                   /* 自动重装载值 */
    g_timx_cplm_pwm_handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV4;                         /* CKD[1:0] = 10, tDTS = 4 * tCK_INT = Ft / 4 = 60Mhz*/
    g_timx_cplm_pwm_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;              /* 使能影子寄存器TIMx_ARR */
    HAL_TIM_PWM_Init(&g_timx_cplm_pwm_handle);                                                  /* 初始化PWM定时器 */
    
    /* 配置输出比较通道 */
    tim_oc_cplm_pwm.OCMode = TIM_OCMODE_PWM1;                                                   /* PWM模式1 */
    tim_oc_cplm_pwm.OCPolarity = TIM_OCPOLARITY_LOW;                                            /* OCy输出极性为低 */
    tim_oc_cplm_pwm.OCNPolarity = TIM_OCNPOLARITY_LOW;                                          /* OCyN互补输出极性为低 */
    tim_oc_cplm_pwm.OCIdleState = TIM_OCIDLESTATE_SET;                                          /* 当MOE=0，OCx=1 */
    tim_oc_cplm_pwm.OCNIdleState = TIM_OCNIDLESTATE_SET;                                        /* 当MOE=0，OCxN=1 */
    HAL_TIM_PWM_ConfigChannel(&g_timx_cplm_pwm_handle, &tim_oc_cplm_pwm, ATIM_TIMX_CPLM_CHY);   /* 配置输出比较通道 */
    
    /* 设置刹车和死区 */
    g_sbreak_dead_time_config.OffStateRunMode = TIM_OSSR_DISABLE;                               /* 运行模式的关闭输出状态 */
    g_sbreak_dead_time_config.OffStateIDLEMode = TIM_OSSI_DISABLE;                              /* 空闲模式的关闭输出状态 */
    g_sbreak_dead_time_config.LockLevel = TIM_LOCKLEVEL_OFF;                                    /* 不用寄存器锁功能 */
    g_sbreak_dead_time_config.BreakState = TIM_BREAK_ENABLE;                                    /* 使能刹车输入 */
    g_sbreak_dead_time_config.BreakPolarity = TIM_BREAKPOLARITY_LOW;                            /* 刹车输入有效信号极性 */
    g_sbreak_dead_time_config.BreakFilter = 0;                                                  /* 刹车输入信号滤波设置 */
    g_sbreak_dead_time_config.Break2State = TIM_BREAK2_DISABLE;                                 /* 不使用断路2 */
    g_sbreak_dead_time_config.AutomaticOutput = TIM_AUTOMATICOUTPUT_ENABLE;                     /* 使能AOE位，允许刹车结束后自动恢复输出 */
    HAL_TIMEx_ConfigBreakDeadTime(&g_timx_cplm_pwm_handle, &g_sbreak_dead_time_config);         /* 配置刹车和死区 */
    
    /* 使能高级定时器和输出比较通道输出 */
    HAL_TIM_PWM_Start(&g_timx_cplm_pwm_handle, ATIM_TIMX_CPLM_CHY);                             /* 使能OCy输出 */
    HAL_TIMEx_PWMN_Start(&g_timx_cplm_pwm_handle,ATIM_TIMX_CPLM_CHY);                           /* 使能OCyN输出 */
}

/**
 * @brief     设置高级定时器输出比较值和死区时间
 * @param     ccr: 输出比较值
 * @param     dtg: 死区时间
 * @arg       dtg[7:5]=0xx时,死区时间 = dtg[7:0] * tDTS
 * @arg       dtg[7:5]=10x时,死区时间 = (64 + dtg[5:0]) * 2  * tDTS
 * @arg       dtg[7:5]=110时,死区时间 = (32 + dtg[4:0]) * 8  * tDTS
 * @arg       dtg[7:5]=111时,死区时间 = (32 + dtg[4:0]) * 16 * tDTS
 * @note      tDTS = 1 / (Ft / CKD[1:0]) = 1 / 60M = 16.67ns
 * @retval    无
 */
void atim_timx_cplm_pwm_set(uint16_t ccr, uint8_t dtg)
{
    g_sbreak_dead_time_config.DeadTime = dtg;                                           /* 死区时间设置 */
    
    /* 设置输出比较值 */
    HAL_TIMEx_ConfigBreakDeadTime(&g_timx_cplm_pwm_handle, &g_sbreak_dead_time_config); /* 重设死区时间 */
    __HAL_TIM_MOE_ENABLE(&g_timx_cplm_pwm_handle);                                      /* MOE=1,使能主输出 */
    ATIM_TIMX_CPLM_CHY_CCRY = ccr;                                                      /* 设置比较寄存器 */
}
