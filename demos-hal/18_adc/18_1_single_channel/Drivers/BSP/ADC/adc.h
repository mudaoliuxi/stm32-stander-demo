/**
 ****************************************************************************************************
 * @file        adc.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-04-25
 * @brief       ADC驱动代码
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

#ifndef __ADC_H
#define __ADC_H

#include "./SYSTEM/sys/sys.h"

/* 单通道ADC采集定义 */
#define ADC_ADCX_CHY_GPIO_PORT          GPIOA
#define ADC_ADCX_CHY_GPIO_PIN           GPIO_PIN_5
#define ADC_ADCX_CHY_GPIO_CLK_ENABLE()  do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)

#define ADC_ADCX                        ADC1
#define ADC_ADCX_CHY                    ADC_CHANNEL_19
#define ADC_ADCX_CHY_CLK_ENABLE()       do{ __HAL_RCC_ADC12_CLK_ENABLE(); }while(0)

/* 函数声明 */
void adc_init(void);                                           /* 初始化ADC */
uint32_t adc_get_result(uint32_t ch);                          /* 获取ADC转换后的结果 */
uint32_t adc_get_result_average(uint32_t ch, uint8_t times);   /* 获取ADC转换且进行均值滤波后的结果 */

#endif 
