/**
 ****************************************************************************************************
 * @file        videoplayer.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-04-25
 * @brief       视频播放器应用代码
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

#ifndef __VIDEOPLAYER_H
#define __VIDEOPLAYER_H

#include "./MJPEG/avi.h"
#include "./SYSTEM/sys/sys.h"
#include "./FATFS/source/ff.h"

#define AVI_AUDIO_BUF_SIZE (1024 * 5)   /* 定义avi解码时,音频buf大小 */
#define AVI_VIDEO_BUF_SIZE (1024 * 260) /* 定义avi解码时,视频buf大小.一般等于AVI_MAX_FRAME_SIZE的大小 */

/* 函数声明 */
void video_play(void);  /* 播放视频 */

#endif

