/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-04-25
 * @brief       DAC输出实验
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
#include "./USMART/usmart.h"
#include "./BSP/MPU/mpu.h"
#include "./BSP/LED/led.h"
#include "./BSP/LCD/lcd.h"
#include "./BSP/KEY/key.h"
#include "./BSP/ADC/adc.h"
#include "./BSP/DAC/dac.h"

extern ADC_HandleTypeDef g_adc_handle; /* ADC句柄 */

int main(void)
{
    uint8_t key;
    uint16_t dacdata = 0;
    uint8_t t = 0;
    uint16_t dacoutdata;
    uint16_t dac_voltage;
    uint16_t adcdata;
    uint16_t adc_voltage;
    
    sys_cache_enable();                                                 /* 打开L1-Cache */
    HAL_Init();                                                         /* 初始化HAL库 */
    sys_stm32_clock_init(240, 2, 2, 4);                                 /* 配置系统时钟, 480Mhz */
    delay_init(480);                                                    /* 初始化延时功能 */
    usart_init(115200);                                                 /* 初始化串口 */
    usmart_dev.init(240);                                               /* 初始化USMART */
    mpu_memory_protection();                                            /* 保护相关存储区域 */
    led_init();                                                         /* 初始化LED */
    lcd_init();                                                         /* 初始化LCD */
    key_init();                                                         /* 初始化按键 */
    adc_init();                                                         /* 初始化ADC */
    dac_init(1);                                                        /* 初始化DAC1_OUT1通道 */
    dac_set_voltage(1, 0);                                              /* 设置DAC输出电压 */
    
    lcd_show_string(30, 50, 200, 16, 16, "STM32", RED);
    lcd_show_string(30, 70, 200, 16, 16, "DAC TEST", RED);
    lcd_show_string(30, 90, 200, 16, 16, "ATOM@ALIENTEK", RED);
    lcd_show_string(30, 110, 200, 16, 16, "WK_UP:+  KEY0:-", RED);
    
    lcd_show_string(30, 150, 200, 16, 16, "DAC VAL:", BLUE);
    lcd_show_string(30, 170, 200, 16, 16, "DAC VOL:0.000V", BLUE);
    lcd_show_string(30, 190, 200, 16, 16, "ADC VOL:0.000V", BLUE);
    
    while (1)
    {
        t++;
        key = key_scan(0);                                                              /* 按键扫描 */
        
        if (key == WKUP_PRES)
        {
            if (dacdata < 4000)
            {
                dacdata += 200;
            }
            HAL_DAC_SetValue(&g_dac_handle, DAC_CHANNEL_1, DAC_ALIGN_12B_R, dacdata);   /* 输出增大200 */
        }
        else if (key == KEY0_PRES)
        {
            if (dacdata > 200)
            {
                dacdata -= 200;
            }
            else
            {
                dacdata = 0;
            }
            HAL_DAC_SetValue(&g_dac_handle, DAC_CHANNEL_1, DAC_ALIGN_12B_R, dacdata);   /* 输出减少200 */
        }
        
        if ((t == 10) || (key == KEY0_PRES) || (key == WKUP_PRES))                      /* WKUP/KEY0按下了,或者定时时间到了 */
        {
            dacoutdata = HAL_DAC_GetValue(&g_dac_handle, DAC_CHANNEL_1);                /* 读取前面设置DAC1_OUT1的值 */
            lcd_show_xnum(94, 150, dacoutdata, 4, 16, 0, BLUE);                         /* 显示DAC寄存器值 */
            
            dac_voltage = (dacoutdata * 3300) / 4095;                                   /* 计算实际输出的电压值（扩大1000倍） */
            lcd_show_xnum(94, 170, dac_voltage / 1000, 1, 16, 0, BLUE);                 /* 显示DAC输出电压值的整数部分 */
            lcd_show_xnum(110, 170, dac_voltage % 1000, 3, 16, 0x80, BLUE);             /* 显示DAC输出电压值的小数部分（保留三位小数） */
            
            adcdata = adc_get_result_average(ADC_ADCX_CHY, 10);                         /* 得到ADC通道19的转换结果 */
            adc_voltage = (adcdata * 3300) / 65535;                                     /* 计算实际电压值（扩大1000倍） */
            lcd_show_xnum(94, 190, adc_voltage / 1000, 1, 16, 0, BLUE);                 /* 显示ADC采集电压值的整数部分 */
            lcd_show_xnum(110, 190, adc_voltage % 1000, 3, 16, 0x80, BLUE);             /* 显示ADC采集电压值的小数部分（保留三位小数） */
            
            LED0_TOGGLE();                                                              /* LED0闪烁 */
            t = 0;
        }
        
        delay_ms(10);
    }
}
