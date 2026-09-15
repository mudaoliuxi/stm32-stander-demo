/**
 ****************************************************************************************************
 * @file        sdmmc_sdcard.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-04-25
 * @brief       SD卡驱动代码
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

#ifndef __SDMMC_SDCARD_H
#define __SDMMC_SDCARD_H

#include "./SYSTEM/sys/sys.h"

/* 引脚定义 */
#define SD1_D0_GPIO_PORT            GPIOC
#define SD1_D0_GPIO_PIN             GPIO_PIN_8
#define SD1_D0_GPIO_CLK_ENABLE()    do{ __HAL_RCC_GPIOC_CLK_ENABLE(); }while(0)

#define SD1_D1_GPIO_PORT            GPIOC
#define SD1_D1_GPIO_PIN             GPIO_PIN_9
#define SD1_D1_GPIO_CLK_ENABLE()    do{ __HAL_RCC_GPIOC_CLK_ENABLE(); }while(0)

#define SD1_D2_GPIO_PORT            GPIOC
#define SD1_D2_GPIO_PIN             GPIO_PIN_10
#define SD1_D2_GPIO_CLK_ENABLE()    do{ __HAL_RCC_GPIOC_CLK_ENABLE(); }while(0)

#define SD1_D3_GPIO_PORT            GPIOC
#define SD1_D3_GPIO_PIN             GPIO_PIN_11
#define SD1_D3_GPIO_CLK_ENABLE()    do{ __HAL_RCC_GPIOC_CLK_ENABLE(); }while(0)

#define SD1_CLK_GPIO_PORT           GPIOC
#define SD1_CLK_GPIO_PIN            GPIO_PIN_12
#define SD1_CLK_GPIO_CLK_ENABLE()   do{ __HAL_RCC_GPIOC_CLK_ENABLE(); }while(0)

#define SD1_CMD_GPIO_PORT           GPIOD
#define SD1_CMD_GPIO_PIN            GPIO_PIN_2
#define SD1_CMD_GPIO_CLK_ENABLE()   do{ __HAL_RCC_GPIOD_CLK_ENABLE(); }while(0)

#define SD_OK                       0x00
#define SD_TIMEOUT                  ((uint32_t)100000000)           /* 超时时间 */
#define SD_TRANSFER_OK              ((uint8_t)0x00)                 /* 传输完成 */
#define SD_TRANSFER_BUSY            ((uint8_t)0x01)                 /* 卡正忙 */

extern SD_HandleTypeDef g_sd_handle;                                /* SD卡句柄 */
extern HAL_SD_CardInfoTypeDef g_sd_card_info_handle;                /* SD卡信息结构体 */

/* 函数声明 */
uint8_t sd_init(void);                                              /* 初始化SD卡 */
uint8_t get_sd_card_info(HAL_SD_CardInfoTypeDef *cardinfo);         /* 获取卡信息函数 */
uint8_t get_sd_card_state(void);                                    /* 获取SD卡状态 */
uint8_t sd_read_disk(uint8_t* buf, uint32_t sector, uint32_t cnt);  /* 读SD卡 */
uint8_t sd_write_disk(uint8_t *buf, uint32_t sector, uint32_t cnt); /* 写SD卡 */

#endif
