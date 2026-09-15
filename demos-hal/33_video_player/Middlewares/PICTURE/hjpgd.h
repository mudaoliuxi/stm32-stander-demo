/**
 ****************************************************************************************************
 * @file        hjpgd.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-04-25
 * @brief       驱动代码-jpeg硬件解码部分代码
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

#ifndef __HJPGD_H
#define __HJPGD_H

#include "./BSP/JPEGCODEC/jpegcodec.h"

extern jpeg_codec_typedef hjpgd;  

/* 函数声明 */
void jpeg_dma_in_callback(void);        /* JPEG输入数据流回调函数 */
void jpeg_dma_out_callback(void);       /* JPEG输出数据流(YCBCR)回调函数 */
void jpeg_endofcovert_callback(void);   /* JPEG整个文件解码完成回调函数 */
void jpeg_hdrover_callback(void);       /* JPEG header解析成功回调函数 */
uint8_t hjpgd_decode(char* pname);      /* JPEG硬件解码图片 */

#endif
