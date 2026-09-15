/**
 ****************************************************************************************************
 * @file        dma.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-04-25
 * @brief       DMA驱动代码
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

#ifndef __DMA_H
#define __DMA_H

#include "./SYSTEM/sys/sys.h"

extern DMA_HandleTypeDef  g_dma_handle;                             /* DMA句柄 */

/* 函数声明 */
void dma_init(DMA_Stream_TypeDef *dma_stream_handle, uint32_t ch);  /* 初始化DMA */

#endif
