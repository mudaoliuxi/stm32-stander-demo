/**
 ****************************************************************************************************
 * @file        stmflash.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-04-25
 * @brief       STM32内部FLASH读写驱动代码
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

#ifndef __STMFLASH_H__
#define __STMFLASH_H__

#include "./SYSTEM/sys/sys.h"

/* FLASH起始地址 */
#define STM32_FLASH_BASE        0x08000000      /* STM32 FLASH的起始地址 */
#define STM32_FLASH_SIZE        0x20000         /* STM32 FLASH总大小 */
#define BOOT_FLASH_SIZE         0x8000          /* 前16K FLASH用于保存BootLoader */
#define FLASH_WAITETIME         50000           /* FLASH等待超时时间 */

/* FLASH 扇区的起始地址,H750xx只有BANK1的扇区0有效,共128KB */
#define BANK1_FLASH_SECTOR_0    ((uint32_t)0x08000000)                  /* Bank1扇区0起始地址, 128 Kbytes  */

/* 函数声明 */
uint32_t stmflash_read_word(uint32_t faddr);                            /* 读取一个字(4字节) */
void stmflash_write(uint32_t waddr, uint32_t *pbuf, uint32_t length);   /* 指定地址开始写入指定长度的数据 */
void stmflash_read(uint32_t raddr, uint32_t *pbuf, uint32_t length);    /* 从指定地址开始读出指定长度的数据 */
void test_write(uint32_t waddr, uint32_t wdata);
    
#endif
