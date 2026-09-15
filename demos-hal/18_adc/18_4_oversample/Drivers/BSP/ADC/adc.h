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

/* ADC引脚定义 */
#define ADC_ADCX_CHY_GPIO_PORT          GPIOA
#define ADC_ADCX_CHY_GPIO_PIN           GPIO_PIN_5
#define ADC_ADCX_CHY_GPIO_CLK_ENABLE()  do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)

#define ADC_ADCX                        ADC1 
#define ADC_ADCX_CHY                    ADC_CHANNEL_19
#define ADC_ADCX_CHY_CLK_ENABLE()       do{ __HAL_RCC_ADC12_CLK_ENABLE(); }while(0)

#define ADC_ADCX_DMASx                  DMA1_Stream7 
#define ADC_ADCX_DMASx_REQ              DMA_REQUEST_ADC1
#define ADC_ADCX_DMASx_IRQn             DMA1_Stream7_IRQn 
#define ADC_ADCX_DMASx_IRQHandler       DMA1_Stream7_IRQHandler 

#define ADC_ADCX_DMASx_IS_TC()          ( __HAL_DMA_GET_FLAG(&g_dma_nch_adc_handle, DMA_FLAG_TCIF3_7) )   /* 获取DMA1 Stream7传输完成标志位，
                                                                                                           * 这是一个假函数形式，不能当函数使用
                                                                                                           */

#define ADC_ADCX_DMASx_CLR_TC()         do{ __HAL_DMA_CLEAR_FLAG(&g_dma_nch_adc_handle, DMA_FLAG_TCIF3_7); }while(0)

/* 函数初始化 */
void adc_oversample_init(uint32_t osr, uint32_t ovss);          /* 初始化ADC过采样函数 */
uint32_t adc_get_result(uint32_t ch);                           /* 获取ADC转换后的结果  */
uint32_t adc_get_result_average(uint32_t ch, uint8_t times);    /* 获取ADC转换且进行均值滤波后的结果 */

#endif
