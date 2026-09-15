/**
 ****************************************************************************************************
 * @file        btim.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-04-25
 * @brief       基本定时器驱动代码
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

#include "./BSP/LED/led.h"
#include "./BSP/TIMER/btim.h"
#include "./SYSTEM/usart/usart.h"

extern uint8_t g_timeout;
TIM_HandleTypeDef g_timx_handle;        /* 定时器x句柄 */

/**
 * @brief       初始化基本定时器中断
 * @note        基本定时器的时钟来自APB1,当D2PPRE1≥2分频的时候
 *              基本定时器的时钟为APB1时钟的2倍, 而APB1为120M, 所以定时器时钟 = 240Mhz
 *              定时器溢出时间计算方法: Tout = ((arr + 1) * (psc + 1)) / Ft us.
 *              Ft=定时器工作频率,单位:Mhz
 * @param       arr: 自动重装值。
 * @param       psc: 时钟预分频数
 * @retval      无
 */
void btim_timx_int_init(uint16_t arr, uint16_t psc)
{
    BTIM_TIMX_INT_CLK_ENABLE();                             /* 使能基本定时器时钟 */
    
    /* 配置基本定时器 */
    g_timx_handle.Instance = BTIM_TIMX_INT;                 /* 基本定时器6 */
    g_timx_handle.Init.Prescaler = psc;                     /* 分频 */
    g_timx_handle.Init.CounterMode = TIM_COUNTERMODE_UP;    /* 递增计数模式 */
    g_timx_handle.Init.Period = arr;                        /* 自动装载值 */
    HAL_TIM_Base_Init(&g_timx_handle);                      /* 配置基本定时器 */
    
    /* 使能基本定时器及其相关中断 */
    HAL_NVIC_SetPriority(BTIM_TIMX_INT_IRQn, 1, 0);         /* 设置中断优先级，抢占优先级1，子优先级0 */
    HAL_NVIC_EnableIRQ(BTIM_TIMX_INT_IRQn);                 /* 开启ITM6中断 */
    HAL_TIM_Base_Start_IT(&g_timx_handle);                  /* 使能定时器x和定时器x更新中断 */
}


/**
 * @brief       基本定时器中断服务函数
 * @param       无
 * @retval      无
 */
void BTIM_TIMX_INT_IRQHandler(void)
{
    if (__HAL_TIM_GET_FLAG(&g_timx_handle, TIM_FLAG_UPDATE) == SET) /* 判断更新中断标志 */
    {
        g_timeout++;
        __HAL_TIM_CLEAR_IT(&g_timx_handle ,TIM_IT_UPDATE);          /* 清除更新中断标志 */
    }
}
