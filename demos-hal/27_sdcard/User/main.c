/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-04-25
 * @brief       SD卡实验
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

#include "./SYSTEM/delay/delay.h"
#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./USMART/usmart.h"
#include "./MALLOC/malloc.h"
#include "./BSP/MPU/mpu.h"
#include "./BSP/LED/led.h"
#include "./BSP/LCD/lcd.h"
#include "./BSP/KEY/key.h"
#include "./BSP/SDMMC/sdmmc_sdcard.h"

/**
 * @brief       通过串口打印SD卡相关信息
 * @param       无
 * @retval      无
 */
void show_sdcard_info(void)
{
    uint64_t card_capacity;                         /* SD卡容量 */
    HAL_SD_CardCIDTypeDef sd_card_cid;
    
    HAL_SD_GetCardCID(&g_sd_handle, &sd_card_cid);  /* 获取CID */
    get_sd_card_info(&g_sd_card_info_handle);       /* 获取SD卡信息 */
    
    switch (g_sd_card_info_handle.CardType)
    {
        case CARD_SDSC:
        {
            if (g_sd_card_info_handle.CardVersion == CARD_V1_X)
            {
                printf("Card Type:SDSC V1\r\n");
            }
            else if (g_sd_card_info_handle.CardVersion == CARD_V2_X)
            {
                printf("Card Type:SDSC V2\r\n");
            }
            break;
        }
    
        case CARD_SDHC_SDXC:
        {
            printf("Card Type:SDHC\r\n");
            break;
        }
    }
    
    card_capacity = (uint64_t)(g_sd_card_info_handle.LogBlockNbr) * (uint64_t)(g_sd_card_info_handle.LogBlockSize); /* 计算SD卡容量 */
    printf("Card ManufacturerID:%d\r\n", sd_card_cid.ManufacturerID);                                               /* 制造商ID */
    printf("Card RCA:%d\r\n", g_sd_card_info_handle.RelCardAdd);                                                    /* 卡相对地址 */
    printf("LogBlockNbr:%d \r\n", (uint32_t)(g_sd_card_info_handle.LogBlockNbr));                                   /* 显示逻辑块数量 */
    printf("LogBlockSize:%d \r\n", (uint32_t)(g_sd_card_info_handle.LogBlockSize));                                 /* 显示逻辑块大小 */
    printf("Card Capacity:%d MB\r\n", (uint32_t)(card_capacity >> 20));                                             /* 显示容量 */
    printf("Card BlockSize:%d\r\n\r\n", g_sd_card_info_handle.BlockSize);                                           /* 显示块大小 */
    }

/**
 * @brief       测试SD卡的读取
 * @note        从secaddr地址开始,读取seccnt个扇区的数据
 * @param       secaddr : 扇区地址
 * @param       seccnt  : 扇区数
 * @retval      无
 */
void sd_test_read(uint32_t secaddr, uint32_t seccnt)
{
    uint32_t i;
    uint8_t *buf;
    uint8_t sta = 0;
    buf = mymalloc(SRAMIN, seccnt * 512);       /* 申请内存,从SDRAM申请内存 */
    sta = sd_read_disk(buf, secaddr, seccnt);   /* 读取secaddr扇区开始的内容 */
    
    if (sta == 0)
    {
        printf("SECTOR %d DATA:\r\n", secaddr);
        
        for (i = 0; i < seccnt * 512; i++)
        {
            printf("%x ", buf[i]);              /* 打印secaddr开始的扇区数据 */
        }
        
        printf("\r\nDATA ENDED\r\n");
    }
    else printf("err:%d\r\n", sta);
    
    myfree(SRAMIN, buf);                        /* 释放内存 */
}

/**
 * @brief       测试SD卡的写入
 * @note        从secaddr地址开始,写入seccnt个扇区的数据
 *              慎用!! 最好写全是0XFF的扇区,否则可能损坏SD卡.
 * @param       secaddr : 扇区地址
 * @param       seccnt  : 扇区数
 * @retval      无
 */
void sd_test_write(uint32_t secaddr, uint32_t seccnt)
{
    uint32_t i;
    uint8_t *buf;
    uint8_t sta = 0;
    buf = mymalloc(SRAMIN, seccnt * 512);       /* 从SDRAM申请内存 */
    
    for (i = 0; i < seccnt * 512; i++)          /* 初始化写入的数据,是3的倍数. */
    {
        buf[i] = i * 3;
    }
    
    sta = sd_write_disk(buf, secaddr, seccnt);  /* 从secaddr扇区开始写入seccnt个扇区内容 */
    
    if (sta == 0)
    {
        printf("Write over!\r\n");
    }
    else 
    {
        printf("err:%d\r\n", sta);
    }
    
    myfree(SRAMIN, buf);                        /* 释放内存 */
}

int main(void)
{
    uint8_t key;
    uint32_t sd_size;
    uint8_t t = 0;
    uint8_t *buf;
    uint64_t card_capacity;             /* SD卡容量 */
    
    sys_cache_enable();                 /* 打开L1-Cache */
    HAL_Init();                         /* 初始化HAL库 */
    sys_stm32_clock_init(240, 2, 2, 4); /* 配置系统时钟, 480Mhz */
    delay_init(480);                    /* 初始化延时功能 */
    usart_init(115200);                 /* 初始化串口 */
    usmart_dev.init(240);               /* 初始化USMART */
    mpu_memory_protection();            /* 保护相关存储区域 */
    led_init();                         /* 初始化LED */
    lcd_init();                         /* 初始化LCD */
    key_init();                         /* 初始化按键 */
    my_mem_init(SRAMIN);                /* 初始化内部内存池(AXI) */
    my_mem_init(SRAM12);                /* 初始化SRAM12内存池(SRAM1+SRAM2) */
    my_mem_init(SRAM4);                 /* 初始化SRAM4内存池(SRAM4) */
    my_mem_init(SRAMDTCM);              /* 初始化DTCM内存池(DTCM) */
    my_mem_init(SRAMITCM);              /* 初始化ITCM内存池(ITCM) */
    
    lcd_show_string(30, 50, 200, 16, 16, "STM32", RED);
    lcd_show_string(30, 70, 200, 16, 16, "SD  TEST", RED);
    lcd_show_string(30, 90, 200, 16, 16, "ATOM@ALIENTEK", RED);
    lcd_show_string(30, 110, 200, 16, 16, "KEY0:Read Sector 0", RED);
    
    while (sd_init() != SD_OK)                                                                                      /* 初始化SD卡 */
    {
        lcd_show_string(30, 150, 200, 16, 16, "SD Card Error!", RED);
        delay_ms(500);
        lcd_show_string(30, 150, 200, 16, 16, "Please Check! ", RED);
        delay_ms(500);
        LED0_TOGGLE();
    }
    
    /* 打印SD卡相关信息 */
    show_sdcard_info();
    
    /* 检测SD卡成功 */
    lcd_show_string(30, 150, 200, 16, 16, "SD Card OK    ", BLUE);
    lcd_show_string(30, 170, 200, 16, 16, "SD Card Size:     MB", BLUE);
    card_capacity = (uint64_t)(g_sd_card_info_handle.LogBlockNbr) * (uint64_t)(g_sd_card_info_handle.LogBlockSize); /* 计算SD卡容量 */
    lcd_show_num(30 + 13 * 8, 170, card_capacity >> 20, 5, 16, BLUE);                                               /* 显示SD卡容量 */
    
    while (1)
    {
        t++;
        key = key_scan(0);
        
        if (key == KEY0_PRES)                                                                                       /* KEY0按下了 */
        {
            buf = mymalloc(0, 512);                                                                                 /* 申请内存 */
            key = sd_read_disk(buf, 0, 1);
            
            if (key == 0)                                                                                           /* 读取0扇区的内容 */
            {
                lcd_show_string(30, 190, 200, 16, 16, "USART1 Sending Data...", BLUE);
                printf("SECTOR 0 DATA:\r\n");
                
                for (sd_size = 0; sd_size < 512; sd_size++)
                {
                    printf("%x ", buf[sd_size]);                                                                    /* 打印0扇区数据 */
                }
                
                printf("\r\nDATA ENDED\r\n");
                lcd_show_string(30, 190, 200, 16, 16, "USART1 Send Data Over!", BLUE);
            }
            else printf("err:%d\r\n", key);
            
            myfree(0, buf);                                                                                         /* 释放内存 */
        }
        
        if (t == 20)
        {
            LED0_TOGGLE();  
            t = 0;
        }
        delay_ms(10);
    }
}
