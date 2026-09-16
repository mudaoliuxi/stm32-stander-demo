/**
 ****************************************************************************************************
 * @file        main.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2026-09-16
 * @brief       SPI2从机(Slave)实验 头文件
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 */

#ifndef __MAIN_H
#define __MAIN_H

#include "./SYSTEM/sys/sys.h"

/******************************************************************************************/
/* SPI2 引脚定义 (M100Z-M7最小系统板, 均引出到排针)                                        */
/******************************************************************************************/

#define SPI2_GPIO_PORT          GPIOB           /* SPI2引脚所在端口                            */
#define SPI2_NSS_PIN            GPIO_PIN_12     /* NSS : 帧边界检测(EXTI上升沿表示一帧结束)     */
#define SPI2_SCK_PIN            GPIO_PIN_13     /* SCK : 时钟由Master提供                       */
#define SPI2_MISO_PIN           GPIO_PIN_14     /* MISO: 从机输出                               */
#define SPI2_MOSI_PIN           GPIO_PIN_15     /* MOSI: 从机输入                               */

/******************************************************************************************/
/* 外部句柄声明 (供stm32h7xx_it.c中断服务函数使用)                                         */
/******************************************************************************************/

extern SPI_HandleTypeDef g_spi2_handle;         /* SPI2句柄                                     */
extern DMA_HandleTypeDef g_spi2_rx_dma;         /* SPI2 RX DMA句柄 (DMA1_Stream0)               */
extern DMA_HandleTypeDef g_spi2_tx_dma;         /* SPI2 TX DMA句柄 (DMA1_Stream1)               */

#endif
