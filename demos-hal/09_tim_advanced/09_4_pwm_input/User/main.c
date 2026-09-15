/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-04-25
 * @brief       高级定时器PWM输入模式实验
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
#include "./BSP/TIMER/gtim.h"
#include "./BSP/TIMER/atim.h"

extern uint16_t g_timxchy_pwmin_sta;    /* PWM输入状态，0：没有捕获，1：成功捕获 */
extern uint32_t g_timxchy_pwmin_hval;   /* PWM的高电平脉宽 */
extern uint32_t g_timxchy_pwmin_cval;   /* PWM的周期宽度 */

int main(void)
{
    uint8_t t = 0;
    
    sys_cache_enable();                         /* 打开L1-Cache */
    HAL_Init();                                 /* 初始化HAL库 */
    sys_stm32_clock_init(240, 2, 2, 4);         /* 设置系统时钟, 480Mhz */
    delay_init(480);                            /* 初始化延时功能 */
    usart_init(115200);                         /* 初始化串口 */
    led_init();                                 /* 初始化LED */
    gtim_timx_pwm_chy_init(100 - 1, 24000 - 1); /* 初始化通用定时器PWM输出，频率为100Hz */
    atim_timx_pwmin_chy_init(240 - 1);          /* 初始化PWM输入捕获 */
    
    while (1)
    {
        if (g_timxchy_pwmin_sta == 1)           /* 判断成功捕获标志 */
        {
            g_timxchy_pwmin_sta = 0;            /* 清除捕获成功标志 */
            
            printf("高电平时间：%d us\r\n", g_timxchy_pwmin_hval);
            printf("PWM周期：%d us\r\n", g_timxchy_pwmin_cval);
            printf("PWM频率：%d Hz\r\n", 1000000 / g_timxchy_pwmin_cval);
            printf("\r\n");
        }
        t++;
        
        if (t >= 20)
        {
            t = 0;
            LED0_TOGGLE();                      /* LED0闪烁 */
        }
        delay_ms(10);
    }
}
