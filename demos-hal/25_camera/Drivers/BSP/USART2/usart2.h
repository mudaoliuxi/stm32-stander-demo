/**
 ****************************************************************************************************
 * @file        usart2.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-04-25
 * @brief       USART2驱动代码
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

#ifndef __USART22_H
#define __USART22_H

#include "stdio.h"
#include "./SYSTEM/sys/sys.h"

/* 引脚和串口定义 */
#define USART2_TX_GPIO_PORT         GPIOA
#define USART2_TX_GPIO_PIN          GPIO_PIN_2
#define USART2_TX_GPIO_AF           GPIO_AF7_USART2
#define USART2_TX_GPIO_CLK_ENABLE() do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)

#define USART2_RX_GPIO_PORT         GPIOA
#define USART2_RX_GPIO_PIN          GPIO_PIN_3
#define USART2_RX_GPIO_AF           GPIO_AF7_USART2
#define USART2_RX_GPIO_CLK_ENABLE() do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)

extern UART_HandleTypeDef g_uart2_handle;  /* USART2句柄 */

/* 函数声明 */
void usart2_init(uint32_t baudrate);       /* 串口2初始化 */

#endif
