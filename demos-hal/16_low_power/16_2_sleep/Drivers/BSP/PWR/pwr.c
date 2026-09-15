/**
 ****************************************************************************************************
 * @file        pwr.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-04-25
 * @brief       低功耗模式驱动代码
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

#include "./BSP/PWR/pwr.h"
#include "./BSP/LED/led.h"
#include "./BSP/LCD/lcd.h"

/**
 * @brief       低功耗模式下的按键初始化(用于唤醒睡眠模式/停止模式)
 * @param       无
 * @retval      无
 */
void pwr_wkup_key_init(void)
{
    GPIO_InitTypeDef gpio_init_struct;
    
    PWR_WKUP_GPIO_CLK_ENABLE();                             /* WKUP时钟使能 */
    
    gpio_init_struct.Pin = PWR_WKUP_GPIO_PIN;               /* WKUP引脚 */
    gpio_init_struct.Mode = GPIO_MODE_IT_RISING;            /* 中断,上升沿 */
    gpio_init_struct.Pull = GPIO_PULLDOWN;                  /* 下拉 */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_MEDIUM;        /* 中速 */
    HAL_GPIO_Init(PWR_WKUP_GPIO_PORT, &gpio_init_struct);   /* WKUP引脚初始化 */
    
    HAL_NVIC_SetPriority(PWR_WKUP_INT_IRQn, 2, 2);          /* 抢占优先级2，子优先级2 */
    HAL_NVIC_EnableIRQ(PWR_WKUP_INT_IRQn); 
}

/**
 * @brief       唤醒引脚外部中断服务函数
 * @param       无
 * @retval      无
 */
void PWR_WKUP_INT_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(PWR_WKUP_GPIO_PIN);
}

/**
 * @brief       进入睡眠模式
 * @param       无
 * @retval      无
 */
void pwr_enter_sleep(void)
{
    HAL_SuspendTick();                                                  /* 暂停滴答时钟，防止通过滴答时钟中断唤醒 */
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);   /* 进入睡眠模式 */
}

/**
 * @brief       外部中断回调函数
 * @param       GPIO_Pin:中断线引脚
 * @note        此函数会被PWR_WKUP_INT_IRQHandler()调用
 * @retval      无
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == PWR_WKUP_GPIO_PIN)
    {
        /* HAL_GPIO_EXTI_IRQHandler()函数已经为我们清除了中断标志位，所以我们进了回调函数可以不做任何事 */
    }
}
