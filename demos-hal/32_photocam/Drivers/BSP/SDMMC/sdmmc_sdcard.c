/**
 ****************************************************************************************************
 * @file        sdmmc_sdcard.c
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

#include "string.h"
#include "./SYSTEM/usart/usart.h"
#include "./BSP/SDMMC/sdmmc_sdcard.h"

SD_HandleTypeDef g_sd_handle;                    /* SD卡句柄 */
HAL_SD_CardInfoTypeDef g_sd_card_info_handle;    /* SD卡信息结构体 */

/**
 * @brief       初始化SD卡
 * @param       无
 * @retval      返回值:0 初始化正确；其他值，初始化错误
 */
uint8_t sd_init(void)
{
    uint8_t SD_Error;
    GPIO_InitTypeDef gpio_init_struct;
    
    /* 使能时钟 */
    __HAL_RCC_SDMMC1_CLK_ENABLE();                                              /* 使能SDMMC1时钟 */
    SD1_D0_GPIO_CLK_ENABLE();                                                   /* D0引脚IO时钟使能 */
    SD1_D1_GPIO_CLK_ENABLE();                                                   /* D1引脚IO时钟使能 */
    SD1_D2_GPIO_CLK_ENABLE();                                                   /* D2引脚IO时钟使能 */
    SD1_D3_GPIO_CLK_ENABLE();                                                   /* D3引脚IO时钟使能 */
    SD1_CLK_GPIO_CLK_ENABLE();                                                  /* CLK引脚IO时钟使能 */
    SD1_CMD_GPIO_CLK_ENABLE();                                                  /* CMD引脚IO时钟使能 */
    
    /* 配置D0引脚 */
    gpio_init_struct.Pin = SD1_D0_GPIO_PIN;                                     /* D0引脚 */
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;                                    /* 推挽复用 */
    gpio_init_struct.Pull = GPIO_PULLUP;                                        /* 上拉 */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;                         /* 高速 */
    gpio_init_struct.Alternate = GPIO_AF12_SDIO1;                               /* 复用为SDIO */
    HAL_GPIO_Init(SD1_D0_GPIO_PORT, &gpio_init_struct);                         /* 配置D0引脚 */
    
    /* 配置D1引脚 */
    gpio_init_struct.Pin = SD1_D1_GPIO_PIN;                                     /* D1引脚 */
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;                                    /* 推挽复用 */
    gpio_init_struct.Pull = GPIO_PULLUP;                                        /* 上拉 */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;                         /* 高速 */
    gpio_init_struct.Alternate = GPIO_AF12_SDIO1;                               /* 复用为SDIO */
    HAL_GPIO_Init(SD1_D1_GPIO_PORT, &gpio_init_struct);                         /* 配置D1引脚 */
    
    /* 配置D2引脚 */
    gpio_init_struct.Pin = SD1_D2_GPIO_PIN;                                     /* D3引脚 */
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;                                    /* 推挽复用 */
    gpio_init_struct.Pull = GPIO_PULLUP;                                        /* 上拉 */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;                         /* 高速 */
    gpio_init_struct.Alternate = GPIO_AF12_SDIO1;                               /* 复用为SDIO */
    HAL_GPIO_Init(SD1_D2_GPIO_PORT, &gpio_init_struct);                         /* 配置D3引脚 */
    
    /* 配置D3引脚 */
    gpio_init_struct.Pin = SD1_D3_GPIO_PIN;                                     /* D3引脚 */
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;                                    /* 推挽复用 */
    gpio_init_struct.Pull = GPIO_PULLUP;                                        /* 上拉 */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;                         /* 高速 */
    gpio_init_struct.Alternate = GPIO_AF12_SDIO1;                               /* 复用为SDIO */
    HAL_GPIO_Init(SD1_D3_GPIO_PORT, &gpio_init_struct);                         /* 配置D3引脚 */
    
    /* 配置CLK引脚 */
    gpio_init_struct.Pin = SD1_CLK_GPIO_PIN;                                    /* CLK引脚 */
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;                                    /* 推挽复用 */
    gpio_init_struct.Pull = GPIO_PULLUP;                                        /* 上拉 */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;                         /* 高速 */
    gpio_init_struct.Alternate = GPIO_AF12_SDIO1;                               /* 复用为SDIO */
    HAL_GPIO_Init(SD1_CLK_GPIO_PORT, &gpio_init_struct);                        /* 配置CLK引脚 */
    
    /* 配置CMD引脚 */
    gpio_init_struct.Pin = SD1_CMD_GPIO_PIN;                                    /* CMD引脚 */
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;                                    /* 推挽复用 */
    gpio_init_struct.Pull = GPIO_PULLUP;                                        /* 上拉 */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;                         /* 高速 */
    gpio_init_struct.Alternate = GPIO_AF12_SDIO1;                               /* 复用为SDIO */
    HAL_GPIO_Init(SD1_CMD_GPIO_PORT, &gpio_init_struct);                        /* 初始化CMD引脚 */
    /* 初始化时的时钟不能大于400KHZ */
    g_sd_handle.Instance = SDMMC1;
    g_sd_handle.Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;                       /* 上升沿 */
    g_sd_handle.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;           /* 空闲时不关闭时钟电源 */
    g_sd_handle.Init.BusWide = SDMMC_BUS_WIDE_4B;                               /* 4位数据线 */
    g_sd_handle.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE; /* 关闭硬件流控 */
    g_sd_handle.Init.ClockDiv = SDMMC_NSpeed_CLK_DIV;                           /* SD传输时钟频率最大25MHZ */
    
    SD_Error = HAL_SD_Init(&g_sd_handle);
    if (SD_Error != HAL_OK)
    {
        return 1;
    }
    
    HAL_SD_GetCardInfo(&g_sd_handle, &g_sd_card_info_handle);                   /* 获取SD卡信息 */
    return 0;
}

/**
 * @brief       获取卡信息函数
 * @param       cardinfo:SD卡信息句柄
 * @retval      返回值:读取卡信息状态值
 */
uint8_t get_sd_card_info(HAL_SD_CardInfoTypeDef *cardinfo)
{
    uint8_t sta;
    sta = HAL_SD_GetCardInfo(&g_sd_handle, cardinfo);
    return sta;
}

/**
 * @brief       获取SD卡状态
 * @param       无
 * @retval      返回值:SD_TRANSFER_OK      传输完成，可以继续下一次传输
                       SD_TRANSFER_BUSY SD 卡正忙，不可以进行下一次传输
 */
uint8_t get_sd_card_state(void)
{
  return ((HAL_SD_GetCardState(&g_sd_handle) == HAL_SD_CARD_TRANSFER ) ? SD_TRANSFER_OK : SD_TRANSFER_BUSY);
}

/**
 * @brief       读SD卡
 * @param       buf:读数据缓存区
 * @param       sector:扇区地址
 * @param       cnt:扇区个数
 * @retval      返回值:0,正常;其他,错误;
 */
uint8_t sd_read_disk(uint8_t* buf, uint32_t sector, uint32_t cnt)
{
    uint8_t sta = HAL_OK;
    uint32_t timeout = SD_TIMEOUT;
    long long lsector = sector;
    sys_intx_disable();                                                             /* 关闭总中断(POLLING模式,严禁中断打断SDIO读写操作!!!) */
    sta = HAL_SD_ReadBlocks(&g_sd_handle, (uint8_t*)buf,lsector, cnt, SD_TIMEOUT);  /* 多个sector的读操作 */
    
    while (get_sd_card_state() != SD_TRANSFER_OK)                                   /* 等待SD卡读完 */
    {
        if (timeout-- == 0)
        {
            sta = SD_TRANSFER_BUSY;
            break;
        }
    }
    
    sys_intx_enable();                                                              /* 开启总中断 */
    return sta;
}

/**
 * @brief       写SD卡
 * @param       buf:写数据缓存区
 * @param       sector:扇区地址
 * @param       cnt:扇区个数
 * @retval      返回值:0,正常;其他,错误;
 */
uint8_t sd_write_disk(uint8_t *buf, uint32_t sector, uint32_t cnt)
{
    uint8_t sta = HAL_OK;
    uint32_t timeout = SD_TIMEOUT;
    long long lsector = sector;
    sys_intx_disable();                                                                 /* 关闭总中断(POLLING模式,严禁中断打断SDIO读写操作!!!) */
    sta = HAL_SD_WriteBlocks(&g_sd_handle, (uint8_t*)buf, lsector, cnt, SD_TIMEOUT);    /* 多个sector的写操作 */
    
    while (get_sd_card_state() != SD_TRANSFER_OK)                                       /* 等待SD卡写完 */
    {
        if (timeout-- == 0)
        {
            sta = SD_TRANSFER_BUSY;
            break;
        }
    }
    
    sys_intx_enable();                                                                  /* 开启总中断 */
    return sta;
}
