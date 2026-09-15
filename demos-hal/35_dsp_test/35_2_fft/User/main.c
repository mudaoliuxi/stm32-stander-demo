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
#include "./BSP/KEY/key.h"
#include "./BSP/MPU/mpu.h"
#include "./BSP/LCD/lcd.h"
#include "./BSP/TIMER/btim.h"

/* FFT长度，默认是1024点FFT
 * 可选范围为：16、64、256、1024
 */
#define FFT_LENGTH  1024

extern void fft_test_init(float *fft_inputbuf, uint16_t fft_length);                                /* 初始化FFT测试 */
extern void fft_test(float *fft_inputbuf);                                                          /* FFT计算测试 */
extern void fft_test_get_output(float *fft_inputbuf, float *fft_outputbuf, uint16_t fft_length);    /* 对FFT计算测试结果取模求得幅值 */

static float fft_inputbuf[FFT_LENGTH * 2];
static float fft_outputbuf[FFT_LENGTH];

uint8_t g_timeout;

int main(void)
{
    uint8_t t = 0;
    uint8_t key;
    uint32_t time;
    char buf[50];
    uint16_t i;
    
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
    lcd_show_string(30, 70, 200, 16, 16, "DSP FFT TEST", RED);
    lcd_show_string(30, 90, 200, 16, 16, "ATOM@ALIENTEK", RED);
    lcd_show_string(30, 130, 200, 16, 16, "KEY0:Run FFT", RED);
    lcd_show_string(30, 160, 200, 16, 16, "FFT runtime:", RED);
    
    while (1)
    {
        t++;
        key = key_scan(0);
        
        switch (key)
        {
            case KEY0_PRES:
            {
                fft_test_init(fft_inputbuf, FFT_LENGTH);                                        /* 初始化FFT测试 */
                BTIM_TIMX_INT->CNT = 0;;                                                        /* 重设BTIM_TIMX_INT定时器的计数器值 */
                g_timeout = 0;
                
                fft_test(fft_inputbuf);                                                         /* FFT计算测试 */
                time = BTIM_TIMX_INT->CNT + (uint32_t)g_timeout * 0x10000;                      /* 计算所用时间 */
                sprintf(buf, "%0.1fms\r\n", (float)time / 10);
                lcd_show_string(30 + 12 * 8, 160, 100, 16, 16, buf, BLUE);
                
                fft_test_get_output(fft_inputbuf, fft_outputbuf, FFT_LENGTH);                   /* 对FFT计算测试结果取模求得幅值 */
                printf("\r\n%d point FFT runtime:%0.1fms\r\n", FFT_LENGTH, (float)time / 10);
                printf("FFT Result:\r\n");
                
                for (i = 0; i < FFT_LENGTH; i++)
                {
                    printf("fft_outputbuf[%d]:%f\r\n", i, fft_outputbuf[i]);
                }
                break;
            }
            default:
            {
                break;
            }
        }
        
        if (t == 20)
        {
            LED0_TOGGLE();
            t = 0;
        }
        
        delay_ms(10);
    }
}
