/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-04-25
 * @brief       高级定时器输出比较模式实验
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
#include "./BSP/TIMER/atim.h"

int main(void)
{
    uint8_t t = 0;
    
    sys_cache_enable();                         /* 打开L1-Cache */
    HAL_Init();                                 /* 初始化HAL库 */
    sys_stm32_clock_init(240, 2, 2, 4);         /* 设置系统时钟, 480Mhz */
    delay_init(480);                            /* 初始化延时功能 */
    usart_init(115200);                         /* 初始化串口 */
    led_init();                                 /* 初始化LED */
    atim_timx_comp_pwm_init(1000 - 1, 240 - 1); /* 1MHz的计数频率,1KHz的周期 */
    
    /* atim_timx_comp_pwm_init已经设置过输出比较寄存器，这是寄存器操作的方法 */
    ATIM_TIMX_COMP_CH1_CCRX = 250 - 1;          /* 通道1相位25% */
    ATIM_TIMX_COMP_CH2_CCRX = 500 - 1;          /* 通道2相位50% */
    ATIM_TIMX_COMP_CH3_CCRX = 750 - 1;          /* 通道3相位75% */
    ATIM_TIMX_COMP_CH4_CCRX = 1000 - 1;         /* 通道4相位100% */
    
    while (1)
    {
        delay_ms(10);
        t++;
        
        if (t >= 20)
        {
            LED0_TOGGLE();                      /* LED0闪烁 */
            t = 0;
        }
    }
}
