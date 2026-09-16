/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-04-25
 * @brief       跑马灯实验
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

#include "./SYSTEM/delay/delay.h"
#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./BSP/LED/led.h"

int main(void)
{
    sys_cache_enable();                  /* 打开L1-Cache */
    HAL_Init();                          /* 初始化HAL库 */
    sys_stm32_clock_init(240, 2, 2, 4);  /* 设置系统时钟, 480Mhz */
    delay_init(480);                     /* 初始化延时功能 */
    led_init();                          /* 初始化LED */
    
    while (1)
    {
        LED0(1);                         /* LED0亮 */
        LED1(1);                         /* LED1灭 */
        delay_ms(5000);                   /* 延时500毫秒 */
        LED0(0);                         /* LED0灭 */
        LED1(0);                         /* LED1亮 */
        delay_ms(5000);                   /* 延时500毫秒 */
    }
}
