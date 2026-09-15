/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-04-25
 * @brief       单通道ADC采集(DMA读取)实验
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

#define ADC_DMA_BUF_SIZE 100                /* ADC DMA缓冲区大小 */
uint16_t g_adc_dma_buf[ADC_DMA_BUF_SIZE];   /* ADC DMA缓冲区 */
extern uint8_t g_adc_dma_sta;               /* DMA传输状态标志, 0,未完成; 1, 已完成 */

int main(void)
{
    uint16_t i;
    uint16_t adcx;
    uint32_t sum;
    float temp;
    
    sys_cache_enable();                                                     /* 打开L1-Cache */
    HAL_Init();                                                             /* 初始化HAL库 */
    sys_stm32_clock_init(240, 2, 2, 4);                                     /* 配置系统时钟, 480Mhz */
    delay_init(480);                                                        /* 初始化延时功能 */
    usart_init(115200);                                                     /* 初始化串口 */
    mpu_memory_protection();                                                /* 保护相关存储区域 */
    led_init();                                                             /* 初始化LED */
    lcd_init();                                                             /* 初始化LCD */
    adc_dma_init((uint32_t)&ADC1->DR, (uint32_t)&g_adc_dma_buf);            /* 初始化ADC DMA采集 */ 
    adc_dma_enable(ADC_DMA_BUF_SIZE);                                       /* 启动ADC DMA采集 */
    lcd_show_string(30, 50, 200, 16, 16, "STM32", RED);
    lcd_show_string(30, 70, 200, 16, 16, "ADC DMA TEST", RED);
    lcd_show_string(30, 90, 200, 16, 16, "ATOM@ALIENTEK", RED);
    
    lcd_show_string(30, 110, 200, 16, 16, "ADC1_CH19_VAL:", BLUE);
    lcd_show_string(30, 130, 200, 16, 16, "ADC1_CH19_VOL:0.000V", BLUE);    /* 先在固定位置显示小数点 */
    
    while (1)
    {
        if (g_adc_dma_sta == 1)                                 /* 等待DMA传输结束 */
        {
            SCB_InvalidateDCache();                             /* 清D Cache */
            
            sum = 0;                                            /* 计算DMA采集到的ADC数据的平均值 */
            for (i = 0; i < ADC_DMA_BUF_SIZE; i++)              /* 对ADC的多次采样值进行均值滤波 */
            {
                sum += g_adc_dma_buf[i];
            }
            adcx= sum / ADC_DMA_BUF_SIZE;                       /* 取平均值 */
            
            lcd_show_xnum(142, 110, adcx, 5, 16, 0, BLUE);      /* 显示ADCC采样后的原始值 */
            
            temp = (float)adcx * (3.3 / 65536);                 /* 获取计算后的带小数的实际电压值 */
            adcx = temp;                                        /* 赋值整数部分给adcx变量 */
            lcd_show_xnum(142, 130, adcx, 1, 16, 0, BLUE);      /* 显示电压值的整数部分 */
            
            temp -= adcx;                                       /* 把已经显示的整数部分去掉，留下小数部分 */
            temp *= 1000;                                       /* 小数部分乘以1000 */
            lcd_show_xnum(158, 130, temp, 3, 16, 0X80, BLUE);   /* 显示小数部分（前面转换为了整形显示 */
            
            g_adc_dma_sta = 0;                                  /* 清除DMA采集完成状态标志 */
            adc_dma_enable(ADC_DMA_BUF_SIZE);                   /* 使能下一次DMA传输ADC数据 */
        }
        
        LED0_TOGGLE();
        delay_ms(100);
    }
}
