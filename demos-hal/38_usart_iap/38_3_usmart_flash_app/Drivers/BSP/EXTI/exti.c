/**
 ****************************************************************************************************
 * @file        exti.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-04-25
 * @brief       外部中断驱动代码
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

#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/LED/led.h"
#include "./BSP/KEY/key.h"
#include "./BSP/EXTI/exti.h"

/**
 * @brief       外部中断初始化程序
 * @param       无
 * @retval      无
 */
void extix_init(void)
{
    GPIO_InitTypeDef gpio_init_struct;
    
    /* 使能时钟 */
    KEY0_INT_GPIO_CLK_ENABLE();                             /* KEY0时钟使能 */
    WKUP_INT_GPIO_CLK_ENABLE();                             /* WKUP时钟使能 */
    
    /* 配置KEY0引脚和外部中断 */ 
    gpio_init_struct.Pin = KEY0_INT_GPIO_PIN;               /* KEY0引脚 */
    gpio_init_struct.Mode = GPIO_MODE_IT_FALLING;           /* 下降沿触发 */
    gpio_init_struct.Pull = GPIO_PULLDOWN;                  /* 下拉 */
    HAL_GPIO_Init(KEY0_INT_GPIO_PORT, &gpio_init_struct);   /* KEY0引脚初始化 */
    HAL_NVIC_SetPriority(KEY0_INT_IRQn, 2, 0);              /* 抢占优先级为2，子优先级为0 */
    HAL_NVIC_EnableIRQ(KEY0_INT_IRQn);                      /* 使能中断线15 */
    
    /* 配置WKUP引脚和外部中断 */ 
    gpio_init_struct.Pin = WKUP_INT_GPIO_PIN;               /* WKUP引脚 */
    gpio_init_struct.Mode = GPIO_MODE_IT_RISING;            /* 上升沿触发 */
    gpio_init_struct.Pull = GPIO_PULLDOWN;                  /* 下拉 */
    HAL_GPIO_Init(WKUP_INT_GPIO_PORT, &gpio_init_struct);   /* WKUP引脚初始化 */
    HAL_NVIC_SetPriority(WKUP_INT_IRQn, 2, 0);              /* 抢占优先级为2，子优先级为0 */
    HAL_NVIC_EnableIRQ(WKUP_INT_IRQn);                      /* 使能中断线0 */
}

/**
 * @brief       WKUP按键外部中断服务函数
 * @param       无
 * @retval      无
 */
void EXTI0_IRQHandler(void)
{
    delay_ms(20);                                   /* 消抖 */
    HAL_GPIO_EXTI_IRQHandler(WKUP_INT_GPIO_PIN);    /* 调用中断处理公用函数 */
}

/**
 * @brief       KEY0按键外部中断服务函数
 * @param       无
 * @retval      无
 */
void EXTI15_10_IRQHandler(void)
{
    delay_ms(10);                                   /* 消抖 */
    HAL_GPIO_EXTI_IRQHandler(KEY0_INT_GPIO_PIN);    /* 调用中断处理公用函数 */
}

/**
 * @brief       外部中断回调函数
 * @param       GPIO_Pin: 中断引脚号
 * @note        在HAL库中所有的外部中断服务函数都会调用此函数
 * @retval      无
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    /* 消抖，此处为了方便使用了延时函数，实际代码中禁止在中断服务函数中调用任何delay之类的延时函数 */
    delay_ms(20);
    switch (GPIO_Pin)
    {
        case KEY0_INT_GPIO_PIN:
        {
            if (KEY0 == 0)      /* KEY0中断 */
            {
                LED0_TOGGLE();  /* LED0状态翻转 */
            }
            break;
        }
        case WKUP_INT_GPIO_PIN:
        {
            if (WK_UP == 1)     /* KEY1中断 */
            {
                LED1_TOGGLE();  /* LED1状态翻转 */
            }
            break;
        }
    }
}
