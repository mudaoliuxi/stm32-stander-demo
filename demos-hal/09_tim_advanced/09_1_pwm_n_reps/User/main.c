/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-04-25
 * @brief       高级定时器输出指定个数PWM实验
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
#include "./BSP/KEY/key.h"
#include "./BSP/TIMER/atim.h"

int main(void)
{
    uint8_t key = 0;
    
    sys_cache_enable();                             /* 打开L1-Cache */
    HAL_Init();                                     /* 初始化HAL库 */
    sys_stm32_clock_init(240, 2, 2, 4);             /* 设置系统时钟, 480Mhz */
    delay_init(480);                                /* 初始化延时功能 */
    usart_init(115200);                             /* 初始化串口 */
    key_init();                                     /* 初始化按键 */
    atim_timx_npwm_chy_init(5000 - 1, 24000 - 1);   /* 10Khz的计数频率,2Hz的PWM频率. */
    
    while (1)
    {
        key = key_scan(0);
        
        if (key == KEY0_PRES)                       /* KEY0按下 */
        {
            atim_timx_npwm_chy_set(5);              /* 输出5个PWM脉冲 */
        }
        
        delay_ms(10);
    }
}
