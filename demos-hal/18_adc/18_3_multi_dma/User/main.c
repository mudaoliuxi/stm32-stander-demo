/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-04-25
 * @brief       多通道ADC采集(DMA读取)实验
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

#define ADC_DMA_BUF_SIZE (50 * 6)           /* ADC DMA缓冲区大小 */
uint16_t g_adc_dma_buf[ADC_DMA_BUF_SIZE];   /* ADC DMA缓冲区 */
extern uint8_t g_adc_dma_sta;               /* DMA传输状态标志, 0,未完成; 1, 已完成 */

int main(void)
{
    uint16_t i,j;
    uint16_t adcdata;
    uint32_t sum;
    float voltage;
    
    sys_cache_enable();                                                 /* 打开L1-Cache */
    HAL_Init();                                                         /* 初始化HAL库 */
    sys_stm32_clock_init(240, 2, 2, 4);                                 /* 配置系统时钟, 480Mhz */
    delay_init(480);                                                    /* 初始化延时功能 */
    usart_init(115200);                                                 /* 初始化串口 */
    mpu_memory_protection();                                            /* 保护相关存储区域 */
    led_init();                                                         /* 初始化LED */
    lcd_init();                                                         /* 初始化LCD */
    adc_nch_dma_init((uint32_t)&ADC1->DR, (uint32_t)&g_adc_dma_buf);    /* 初始化ADC DMA采集 */
    adc_dma_enable(ADC_DMA_BUF_SIZE);                                   /* 使能一次DMA传输ADC数据 */
    
    lcd_show_string(30, 50, 200, 16, 16, "STM32", RED);
    lcd_show_string(30, 70, 200, 16, 16, "ADC DMA TEST", RED);
    lcd_show_string(30, 90, 200, 16, 16, "ATOM@ALIENTEK", RED);
    
    lcd_show_string(30, 130, 200, 12, 12, "ADC1_CH14_VAL:", BLUE);
    lcd_show_string(30, 142, 200, 12, 12, "ADC1_CH14_VOL:0.000V", BLUE);
    
    lcd_show_string(30, 160, 200, 12, 12, "ADC1_CH15_VAL:", BLUE);
    lcd_show_string(30, 172, 200, 12, 12, "ADC1_CH15_VOL:0.000V", BLUE);
    
    lcd_show_string(30, 190, 200, 12, 12, "ADC1_CH16_VAL:", BLUE);
    lcd_show_string(30, 202, 200, 12, 12, "ADC1_CH16_VOL:0.000V", BLUE);
    
    lcd_show_string(30, 220, 200, 12, 12, "ADC1_CH17_VAL:", BLUE);
    lcd_show_string(30, 232, 200, 12, 12, "ADC1_CH17_VOL:0.000V", BLUE);
    
    lcd_show_string(30, 250, 200, 12, 12, "ADC1_CH18_VAL:", BLUE);
    lcd_show_string(30, 262, 200, 12, 12, "ADC1_CH18_VOL:0.000V", BLUE);
    
    lcd_show_string(30, 280, 200, 12, 12, "ADC1_CH19_VAL:", BLUE);
    lcd_show_string(30, 292, 200, 12, 12, "ADC1_CH19_VOL:0.000V", BLUE);
    
    while (1)
    {
        if (g_adc_dma_sta == 1)
        {
            SCB_InvalidateDCache();                                             /* 清除D Cache数据 */
            
            for(j = 0; j < 6; j++)                                              /* 遍历6个通道 */
            {
                sum = 0;                                                        /* 清零 */
                for (i = 0; i < ADC_DMA_BUF_SIZE / 6; i++)                      /* 每个通道采集了50次数据,进行50次累加 */
                {
                    sum += g_adc_dma_buf[(6 * i) + j];                          /* 相同通道的转换数据累加 */
                }
                adcdata = sum / (ADC_DMA_BUF_SIZE / 6);                         /* 取平均值 */
                
                lcd_show_xnum(114, 130 + (j * 30), adcdata, 5, 12, 0, BLUE);    /* 显示ADCC采样后的原始值 */
                voltage = (float)adcdata * (3.3 / 65536);                       /* 获取计算后的带小数的实际电压值 */
                adcdata = voltage;                                              /* 赋值整数部分给adcx变量 */
                lcd_show_xnum(114, 142 + (j * 30), adcdata, 1, 12, 0, BLUE);    /* 显示电压值的整数部分 */
                
                voltage -= adcdata;                                             /* 把已经显示的整数部分去掉，留下小数部分 */
                voltage *= 1000;                                                /* 小数部分乘以1000 */
                lcd_show_xnum(126, 142 + (j * 30), voltage, 3, 12, 0X80, BLUE); /* 显示小数部分（前面转换为了整形显示） */
            }
            
            g_adc_dma_sta = 0;                                                  /* 清除DMA采集完成状态标志 */
            adc_dma_enable(ADC_DMA_BUF_SIZE);                                   /* 使能一次DMA传输ADC数据 */
        }
        
        LED0_TOGGLE();
        delay_ms(100);
    }
}
