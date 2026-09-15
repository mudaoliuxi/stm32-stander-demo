/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-04-25
 * @brief       串口通信实验
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
    uint8_t len;
    uint16_t times; 
    
    sys_cache_enable();                     /* 打开L1-Cache */
    HAL_Init();                             /* 初始化HAL库 */
    sys_stm32_clock_init(240, 2, 2, 4);     /* 设置系统时钟, 480Mh */
    delay_init(480);                        /* 初始化延时功能 */
    usart_init(115200);                     /* 初始化串口 */
    led_init();                             /* 初始化LED */
    
    while (1)
    {
        if (g_usart_rx_sta & 0x8000)        /* 接收到了数据 */
        {
            len = g_usart_rx_sta & 0x3FFF;  /* 获取有效数据长度 */
            printf("\r\n您发送的消息为：\r\n");
            
            g_usart_rx_buf[len] = '\0';     /* 末尾插入结束符 */
            printf("%s",g_usart_rx_buf);
            
            printf("\r\n\r\n");             /* 插入换行 */
            g_usart_rx_sta = 0;
        }
        else
        {
            times++;
            
            if (times % 5000 == 0)
            {
                printf("\r\n正点原子 M100Z-M7最小系统板STM32H750版 串口实验\r\n");
                printf("正点原子@ALIENTEK\r\n\r\n\r\n");
            }
            
            if (times % 200 == 0)
            {
                printf("请输入数据，以回车键结束\r\n");
            }
            
            if (times % 30 == 0)
            {
                LED0_TOGGLE();
            }
            
            delay_ms(10);
        }
    }
}
