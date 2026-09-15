/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-04-25
 * @brief       内部温度传感器实验
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
#include "./BSP/MPU/mpu.h"
#include "./BSP/LED/led.h"
#include "./BSP/LCD/lcd.h"
#include "./BSP/ADC/adc.h"

int main(void)
{
    int16_t temp;
    
    sys_cache_enable();                                                 /* 打开L1-Cache */
    HAL_Init();                                                         /* 初始化HAL库 */
    sys_stm32_clock_init(240, 2, 2, 4);                                 /* 配置系统时钟, 480Mhz */
    delay_init(480);                                                    /* 初始化延时功能 */
    usart_init(115200);                                                 /* 初始化串口 */
    mpu_memory_protection();                                            /* 保护相关存储区域 */
    led_init();                                                         /* 初始化LED */
    lcd_init();                                                         /* 初始化LCD */
    adc_temperature_init();                                             /* 初始化ADC内部温度传感器 */
    
    lcd_show_string(30, 50, 200, 16, 16, "STM32", RED);
    lcd_show_string(30, 70, 200, 16, 16, "Temperature TEST", RED);
    lcd_show_string(30, 90, 200, 16, 16, "ATOM@ALIENTEK", RED);
    
    lcd_show_string(30, 120, 200, 16, 16, "TEMPERATE: 00.00C", BLUE);
    
    while (1)
    {
        temp = adc3_get_temperature();                                  /* 得到温度值 */
        
        if (temp < 0)
        {
            temp = -temp;
            lcd_show_string(30 + 10 * 8, 120, 16, 16, 16, "-", BLUE);   /* 显示负号 */
        }
        else
        {
            lcd_show_string(30 + 10 * 8, 120, 16, 16, 16, " ", BLUE);   /* 无符号 */
        }
        lcd_show_xnum(30 + 11 * 8, 120, temp / 100, 2, 16, 0, BLUE);    /* 显示整数部分 */
        lcd_show_xnum(30 + 14 * 8, 120, temp % 100, 2, 16, 0X80, BLUE); /* 显示小数部分 */
        
        LED0_TOGGLE();                                                  /* LED0闪烁,提示程序运行 */
        delay_ms(250);
    }
}
