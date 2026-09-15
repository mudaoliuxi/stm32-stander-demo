/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-04-25
 * @brief       PVD电压监控实验
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
#include "./USMART/usmart.h"
#include "./BSP/MPU/mpu.h"
#include "./BSP/LED/led.h"
#include "./BSP/LCD/lcd.h"
#include "./BSP/PWR/pwr.h"

int main(void)
{
    uint8_t t = 0;
    
    sys_cache_enable();                                                 /* 打开L1-Cache */
    HAL_Init();                                                         /* 初始化HAL库 */
    sys_stm32_clock_init(240, 2, 2, 4);                                 /* 配置系统时钟, 480Mhz */
    delay_init(480);                                                    /* 初始化延时功能 */
    usart_init(115200);                                                 /* 初始化串口 */
    mpu_memory_protection();                                            /* 保护相关存储区域 */
    led_init();                                                         /* 初始化LED */
    lcd_init();                                                         /* 初始化LCD */
    pwr_pvd_init(PWR_PVDLEVEL_5);                                       /* PVD 2.7V检测 */
    
    lcd_show_string(30, 50, 200, 16, 16, "STM32", RED);
    lcd_show_string(30, 70, 200, 16, 16, "PVD TEST", RED);
    lcd_show_string(30, 90, 200, 16, 16, "ATOM@ALIENTEK", RED);
    lcd_show_string(30, 110, 200, 16, 16, "PVD Voltage OK! ", BLUE);    /* 默认LCD显示电压正常 */
    
    while (1)
    {
        if ((t % 20) == 0)
        {
            LED0_TOGGLE();                                              /* 每200ms,翻转一次LED0 */
        }
        
        delay_ms(10);
        t++;
    }
}
