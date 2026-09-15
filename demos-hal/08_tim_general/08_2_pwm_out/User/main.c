/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-04-25
 * @brief       通用定时器PWM输出实验
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

#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/TIMER/gtim.h"

extern TIM_HandleTypeDef g_timx_pwm_chy_handle; /* 定时器句柄 */

int main(void)
{
    uint16_t ledrpwmval = 0;
    uint8_t dir = 1;
    
    sys_cache_enable();                         /* 打开L1-Cache */
    HAL_Init();                                 /* 初始化HAL库 */
    sys_stm32_clock_init(240, 2, 2, 4);         /* 设置系统时钟, 480Mhz */
    delay_init(480);                            /* 初始化延时功能 */
    usart_init(115200);                         /* 初始化串口 */
    gtim_timx_pwm_chy_init(500 - 1, 240 - 1);   /* 初始化通用定时器PWM输出 */
    
    while (1)
    {
        delay_ms(10);
        
        /* 根据方向修改ledrpwmval */
        if (dir == 1)
        {
            ledrpwmval++;                       /* dir==1 ledrpwmval递增 */
        }
        else
        {
            ledrpwmval--;                       /* dir==0 ledrpwmval递减 */
        }
        
        if (ledrpwmval > 300)
        {
            dir = 0;                            /* ledrpwmval到达300后，方向为递减 */
        }
        
        if (ledrpwmval == 0)
        {
            dir = 1;                            /* ledrpwmval递减到0后，方向改为递增 */
        }
        
        /* 修改比较值控制占空比 */
        __HAL_TIM_SET_COMPARE(&g_timx_pwm_chy_handle, GTIM_TIMX_PWM_CHY, ledrpwmval);
    }
}
