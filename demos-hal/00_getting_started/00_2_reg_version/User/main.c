/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-04-25
 * @brief       新建工程实验-寄存器版本
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

int main(void)
{
    sys_stm32_clock_init(240, 2, 2, 4); /* 设置时钟,480Mhz */
    delay_init(480);                    /* 初始化延时功能 */
    usart_init(120, 115200);            /* 初始化串口 */
    
    while (1)
    {
        printf("Hello, M100-M7-STM32H750\r\n");
        delay_ms(1000);
    }
}
