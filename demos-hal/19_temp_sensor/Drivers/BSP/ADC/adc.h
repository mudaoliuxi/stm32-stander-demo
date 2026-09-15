/**
 ****************************************************************************************************
 * @file        adc.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-04-25
 * @brief       ADC(开启内部温度传感器)驱动代码
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

#ifndef __ADC3_H
#define __ADC3_H

#include "./SYSTEM/sys/sys.h"

/* ADC内部温度传感器定义 */ 
#define ADC_ADCX                    ADC3
#define ADC_ADCX_CHY                ADC_CHANNEL_TEMPSENSOR
#define ADC_ADCX_CHY_CLK_ENABLE()   do{ __HAL_RCC_ADC3_CLK_ENABLE(); }while(0)

/* 函数声明 */
void adc_temperature_init(void);                                                            /* 初始化ADC内部温度传感器 */
uint32_t adc3_get_result(ADC_HandleTypeDef adc_handle, uint32_t ch);                        /* 获取ADC转换后的结果 */
uint32_t adc3_get_result_average(ADC_HandleTypeDef adc_handle, uint32_t ch, uint8_t times); /* 获取ADC转换且进行均值滤波后的结果 */
short adc3_get_temperature(void);                                                           /* 获取内部温度传感器温度值 */

#endif
