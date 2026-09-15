/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-04-25
 * @brief       IIC实验
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
#include "./BSP/KEY/key.h"
#include "./BSP/24CXX/24cxx.h"

/* 要写入到24c02的字符串数组 */
const uint8_t g_text_buf[] = {"STM32 IIC TEST"};

#define TEXT_SIZE sizeof(g_text_buf)    /* TEXT字符串长度 */

int main(void)
{
    uint8_t key;
    uint16_t t = 0;
    uint8_t datatemp[TEXT_SIZE];
    
    sys_cache_enable();                 /* 打开L1-Cache */
    HAL_Init();                         /* 初始化HAL库 */
    sys_stm32_clock_init(240, 2, 2, 4); /* 配置系统时钟, 480Mhz */
    delay_init(480);                    /* 初始化延时功能 */
    usart_init(115200);                 /* 初始化串口 */
    usmart_dev.init(240);               /* 初始化USMART */
    mpu_memory_protection();            /* 保护相关存储区域 */
    led_init();                         /* 初始化LED */
    lcd_init();                         /* 初始化LCD */
    key_init();                         /* 初始化按键 */
    at24cxx_init();                     /* 初始化24CXX */
    
    lcd_show_string(30, 50, 200, 16, 16, "STM32", RED);
    lcd_show_string(30, 70, 200, 16, 16, "IIC TEST", RED);
    lcd_show_string(30, 90, 200, 16, 16, "ATOM@ALIENTEK", RED);
    lcd_show_string(30, 110, 200, 16, 16, "WK_UP:Write  KEY0:Read", RED);           /* 显示提示信息 */
    
    while (at24cxx_check())                                                         /* 检测不到24c02 */
    {
        lcd_show_string(30, 130, 200, 16, 16, "24C02 Check Failed!", RED);
        delay_ms(500);
        lcd_show_string(30, 130, 200, 16, 16, "Please Check!      ", RED);
        delay_ms(500);
        LED0_TOGGLE();                                                              /* 红灯闪烁 */
    }

    lcd_show_string(30, 130, 200, 16, 16, "24C02 Ready!", RED);
    
    while (1)
    {
        t++;
        key = key_scan(0);
        
        if (key == WKUP_PRES)                                                       /* WK_UP按下,写入24C02 */
        {
            lcd_fill(0, 150, 239, 319, WHITE);                                      /* 清除半屏 */
            lcd_show_string(30, 150, 200, 16, 16, "Start Write 24C02....", BLUE);
            at24cxx_write(0, (uint8_t *)g_text_buf, TEXT_SIZE);
            lcd_show_string(30, 150, 200, 16, 16, "24C02 Write Finished!", BLUE);   /* 提示传送完成 */
        }
        
        if (key == KEY0_PRES)                                                       /* KEY0按下,读取字符串并显示 */
        {
            lcd_show_string(30, 150, 200, 16, 16, "Start Read 24C02.... ", BLUE);
            at24cxx_read(0, datatemp, TEXT_SIZE);
            lcd_show_string(30, 150, 200, 16, 16, "The Data Readed Is:  ", BLUE);   /* 提示传送完成 */
            lcd_show_string(30, 170, 200, 16, 16, (char *)datatemp, BLUE);          /* 显示读到的字符串 */
        }
        
        if (t == 20)
        {
            LED0_TOGGLE();                                                          /* 红灯闪烁 */
            t = 0;
        }
        
        delay_ms(10);
    }
}
