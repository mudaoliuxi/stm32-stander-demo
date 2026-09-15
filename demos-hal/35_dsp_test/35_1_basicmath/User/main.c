/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-04-25
 * @brief       DSP BasicMath实验
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

#include "stdlib.h"
#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/LED/led.h"
#include "./BSP/MPU/mpu.h"
#include "./BSP/LCD/lcd.h"
#include "./BSP/TIMER/btim.h"

#ifndef PI
#define PI 3.14159265358979f
#endif

/* 正弦余弦测试 */
extern uint8_t sin_cos_test(float angle, uint32_t times, uint8_t mode);

uint8_t g_timeout;

int main(void)
{
    uint32_t time;
    char buf[50];
    uint8_t res;
    
    sys_cache_enable();                     /* 打开L1-Cache */
    HAL_Init();                             /* 初始化HAL库 */
    sys_stm32_clock_init(240, 2, 2, 4);     /* 配置系统时钟, 480Mhz */
    delay_init(480);                        /* 初始化延时功能 */
    usart_init(115200);                     /* 初始化串口 */
    mpu_memory_protection();                /* 保护相关存储区域 */
    led_init();                             /* 初始化LED */
    lcd_init();                             /* 初始化LCD */
    btim_timx_int_init(0xFFFF, 24000 - 1);  /* 10Khz计数频率,最大计时6.5秒超出 */
    
    lcd_show_string(30, 50, 200, 16, 16, "STM32", RED);
    lcd_show_string(30, 70, 200, 16, 16, "DSP BasicMath TEST", RED);
    lcd_show_string(30, 90, 200, 16, 16, "ATOM@ALIENTEK", RED);
    lcd_show_string(30, 120, 200, 16, 16, "No DSP runtime:", RED);
    lcd_show_string(30, 150, 200, 16, 16, "Use DSP runtime:", RED);
    
    while (1)
    {
        /* 不使用DSP优化 */
        BTIM_TIMX_INT->CNT = 0; /* 重设TIM3定时器的计数器值 */
        g_timeout = 0;
        res = sin_cos_test(PI / 6, 2000000, 0);
        time = BTIM_TIMX_INT->CNT + (uint32_t)g_timeout * 0x10000;
        sprintf(buf, "%0.1fms\r\n", (float)time / 10);
        
        if (res == 0)
        {
            lcd_show_string(30 + 16 * 8, 120, 100, 16, 16, buf, BLUE);      /* 显示运行时间 */
        }
        else                                                                /* 显示错误 */
        {
            lcd_show_string(30 + 16 * 8, 120, 100, 16, 16, "error！", BLUE);
        }
        
        /* 使用DSP优化 */
        BTIM_TIMX_INT->CNT = 0; /* 重设TIM6定时器的计数器值 */
        g_timeout = 0;
        res = sin_cos_test(PI / 6, 2000000, 1);
        time = BTIM_TIMX_INT->CNT + (uint32_t)g_timeout * 0x10000;
        sprintf(buf, "%0.1fms\r\n", (float)time / 10);
        
        if (res == 0)
        {
            lcd_show_string(30 + 16 * 8, 150, 100, 16, 16, buf, BLUE);      /* 显示运行时间 */
        }
        else                                                                /* 显示错误 */
        {
            lcd_show_string(30 + 16 * 8, 150, 100, 16, 16, "error！", BLUE);
        }
        
        LED0_TOGGLE();
    }
}
