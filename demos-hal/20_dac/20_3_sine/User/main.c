/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-04-25
 * @brief       DAC输出正弦波实验
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

#include "math.h"
#include "./SYSTEM/delay/delay.h"
#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./USMART/usmart.h"
#include "./BSP/MPU/mpu.h"
#include "./BSP/LED/led.h"
#include "./BSP/LCD/lcd.h"
#include "./BSP/KEY/key.h"
#include "./BSP/ADC/adc.h"
#include "./BSP/DAC/dac.h"

uint16_t g_dac_sin_buf[4096];    /* 发送数据缓冲区 */

/**
 * @brief       产生正弦波序列
 * @note        需保证: maxval > samples/2
 * @param       maxval : 最大值(0 < maxval < 2048)
 * @param       samples: 采样点的个数
 * @retval      无
 */
void dac_creat_sin_buf(uint16_t maxval, uint16_t samples)
{
    uint8_t i;
    float inc = (2 * 3.1415926) / samples;      /* ω=2π/T*/
    float outdata = 0;
    
    for (i = 0; i < samples; i++)
    {
        outdata = maxval * (1 + sin(inc * i));  /* y=Asin(ωx+φ)+b */
        
        if (outdata > 4095)
        {
            outdata = 4095;                     /* 上限限定 */ 
        }
        g_dac_sin_buf[i] = outdata;
    }
}

/**
 * @brief       通过USMART设置正弦波输出参数,方便修改输出频率.
 * @param       arr : TIM7的自动重装载值
 * @param       psc : TIM7的分频系数
 * @retval      无
 */
void dac_dma_sin_set(uint16_t arr, uint16_t psc)
{
    dac_dma_wave_enable(100, arr, psc);
}

int main(void)
{
    uint8_t key;
    uint8_t t = 0;
    uint16_t dacdata;
    uint16_t dac_voltage;
    uint16_t adcdata;
    uint16_t adc_voltage;
    
    sys_cache_enable();                         /* 打开L1-Cache */
    HAL_Init();                                 /* 初始化HAL库 */
    sys_stm32_clock_init(240, 2, 2, 4);         /* 配置系统时钟, 480Mhz */
    delay_init(480);                            /* 初始化延时功能 */
    usart_init(115200);                         /* 初始化串口 */
    usmart_dev.init(240);                       /* 初始化USMART */
    mpu_memory_protection();                    /* 保护相关存储区域 */
    led_init();                                 /* 初始化LED */
    lcd_init();                                 /* 初始化LCD */
    key_init();                                 /* 初始化按键 */
    adc_init();                                 /* 初始化ADC */
    dac_dma_wave_init(1);                       /* 初始化DAC通道1 DMA波形输出 */
    dac_creat_sin_buf(2048, 100);               /* 生成正弦数据，振幅约3.3(V)，100个数据 */
    dac_dma_wave_enable(100, 100 - 1, 24 - 1);  /* 定时器触发速率100KHz，100个数据，输出约1KHz的正弦波 */
    
    lcd_show_string(30, 50, 200, 16, 16, "STM32", RED);
    lcd_show_string(30, 70, 200, 16, 16, "DAC DMA Sine WAVE TEST", RED);
    lcd_show_string(30, 90, 200, 16, 16, "ATOM@ALIENTEK", RED);
    lcd_show_string(30, 110, 200, 16, 16, "KEY0:5Khz  WK_UP:50Khz", RED);
    
    lcd_show_string(30, 130, 200, 16, 16, "DAC VAL:", BLUE);
    lcd_show_string(30, 150, 200, 16, 16, "DAC VOL:0.000V", BLUE);
    lcd_show_string(30, 170, 200, 16, 16, "ADC VOL:0.000V", BLUE);
    
    while (1)
    {
        t++;
        key = key_scan(0);                                              /* 按键扫描 */
        
        if (key == KEY0_PRES)                                           /* 高采样率 ,约5Khz波形 */
        {
            dac_creat_sin_buf(2048, 100);                               /* 产生正弦波函序列 */
            dac_dma_wave_enable(100, 20 - 1, 24 - 1);                   /* 500Khz触发频率,100个点,得到最高5KHz的正弦波. */
        }
        else if (key == WKUP_PRES)                                      /* 低采样率 ,约50Khz波形 */
        {
            dac_creat_sin_buf(2048, 10);                                /* 产生正弦波函序列 */
            dac_dma_wave_enable(10, 20 - 1, 24 - 1);                    /* 500Khz触发频率,10个点,可以得到最高50KHz的正弦波. */
        }
        
        dacdata = DAC1->DHR12R1;                                        /* 获取DAC1_OUT1的输出状态 */
        lcd_show_xnum(94, 130, dacdata, 5, 16, 0, BLUE);
        
        dac_voltage = (dacdata * 3300) / 4095;                          /* 计算并显示DAC输出电压的模拟量 */
        lcd_show_xnum(94, 150, dac_voltage / 1000, 1, 16, 0, BLUE);
        lcd_show_xnum(110, 150, dac_voltage % 1000, 3, 16, 0x80, BLUE);
        
        adcdata = adc_get_result_average(ADC_ADCX_CHY, 20);             /* 得到ADC通道19的转换结果 */
        adc_voltage = (adcdata * 3300) / 65535;
        lcd_show_xnum(94, 170, adc_voltage / 1000, 1, 16, 0, BLUE);
        lcd_show_xnum(110, 170, adc_voltage % 1000, 3, 16, 0x80, BLUE);
        
        if (t == 10)                                                    /* 定时时间到了 */
        {
            LED0_TOGGLE();                                              /* LED0闪烁 */
            t = 0;
        }
        
        delay_ms(5);
    }
}
