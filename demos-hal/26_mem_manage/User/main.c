/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-04-25
 * @brief       内存管理实验
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
#include "./BSP/MPU/mpu.h"
#include "./BSP/LED/led.h"
#include "./BSP/LCD/lcd.h"
#include "./BSP/KEY/key.h"
#include "./MALLOC/malloc.h"

const char *SRAM_NAME_BUF[SRAMBANK] = {"SRAMIN", "SRAM12", "SRAM4   ", "SRAMDTCM", "SRAMITCM"};

int main(void)
{
    uint8_t key;
    uint8_t t = 0;
    uint8_t paddr[32];
    uint16_t memused = 0;
    uint8_t *p_sramin = NULL;
    uint8_t *p_sram12 = NULL;
    uint8_t *p_sram4 = NULL;
    uint8_t *p_sramdtcm = NULL;
    uint8_t *p_sramitcm = NULL;
    uint32_t tp_sramin = 0;
    uint32_t tp_sram12 = 0;
    uint32_t tp_sram4 = 0;
    uint32_t tp_sramdtcm = 0;
    uint32_t tp_sramitcm = 0;
    
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
    
    lcd_show_string(30, 40, 200, 16, 16, "STM32", RED);
    lcd_show_string(30, 60, 200, 16, 16, "MALLOC TEST", RED);
    lcd_show_string(30, 80, 200, 16, 16, "ATOM@ALIENTEK", RED);
    lcd_show_string(30, 100, 200, 16, 16, "KEY0:Malloc & WR & Show", RED);
    lcd_show_string(30, 120, 200, 16, 16, "WK_UP:Free", RED);
    
    lcd_show_string(30, 140, 200, 16, 16, "SRAMIN ", BLUE);
    lcd_show_string(30, 156, 200, 16, 16, "SRAMIN   USED:", BLUE);
    lcd_show_string(30, 172, 200, 16, 16, "SRAM12   USED:", BLUE);
    lcd_show_string(30, 188, 200, 16, 16, "SRAM4    USED:", BLUE);
    lcd_show_string(30, 204, 200, 16, 16, "SRAMDTCM USED:", BLUE);
    lcd_show_string(30, 220, 200, 16, 16, "SRAMITCM USED:", BLUE);

    while (1)
    {
        t++;
        key = key_scan(0);
        
        switch (key)
        {
            case KEY0_PRES:
            {
                p_sramin = mymalloc(SRAMIN, 2048);                                                                                  /* 从所有内存池中申请内存 */
                p_sram12 = mymalloc(SRAM12, 2048);
                p_sram4 = mymalloc(SRAM4, 2048);
                p_sramdtcm = mymalloc(SRAMDTCM, 2048);
                p_sramitcm = mymalloc(SRAMITCM, 2048);
                
                if ((p_sramin != NULL) && (p_sram12 != NULL) && (p_sram4 != NULL) && (p_sramdtcm != NULL) && (p_sramitcm != NULL))  /* 内存申请成功 */
                {
                    sprintf((char *)p_sramin, "SRAMIN: Malloc Test%03d", t + SRAMIN);                                               /* 往申请到的内存中写入数据 */
                    lcd_show_string(30, 318, 300, 16, 16, (char *)p_sramin, BLUE);                                                  /* 将申请到的内存中的数据在LCD上显示 */
                    sprintf((char *)p_sram12, "SRAM12: Malloc Test%03d", t + SRAM12);
                    lcd_show_string(30, 334, 300, 16, 16, (char *)p_sram12, BLUE);
                    sprintf((char *)p_sram4, "SRAM4: Malloc Test%03d", t + SRAM4);
                    lcd_show_string(30, 350, 300, 16, 16, (char *)p_sram4, BLUE);
                    sprintf((char *)p_sramdtcm, "SRAMDTCM: Malloc Test%03d", t + SRAMDTCM);
                    lcd_show_string(30, 366, 300, 16, 16, (char *)p_sramdtcm, BLUE);
                    sprintf((char *)p_sramitcm, "SRAMITCM: Malloc Test%03d", t + SRAMITCM);
                    lcd_show_string(30, 382, 300, 16, 16, (char *)p_sramitcm, BLUE);
                }
                else                                                                                                                /* 内存申请失败 */
                {
                    myfree(SRAMIN, p_sramin);                                                                                       /* 释放申请成功的内存 */
                    myfree(SRAM12, p_sram12);
                    myfree(SRAM4, p_sram4);
                    myfree(SRAMDTCM, p_sramdtcm);
                    myfree(SRAMITCM, p_sramitcm);
                    p_sramin = NULL;
                    p_sram12 = NULL;
                    p_sram4 = NULL;
                    p_sramdtcm = NULL;
                    p_sramitcm = NULL;
                }
                break;
            }
            case WKUP_PRES:
            {
                myfree(SRAMIN, p_sramin);                                                                                           /* 释放申请成功的内存 */
                myfree(SRAM12, p_sram12);
                myfree(SRAM4, p_sram4);
                myfree(SRAMDTCM, p_sramdtcm);
                myfree(SRAMITCM, p_sramitcm);
                p_sramin = NULL;
                p_sram12 = NULL;
                p_sram4 = NULL;
                p_sramdtcm = NULL;
                p_sramitcm = NULL;
                break;
            }
        }
        
        if ((tp_sramin != (uint32_t)p_sramin) ||
            (tp_sram12 != (uint32_t)p_sram12) ||
            (tp_sram4 != (uint32_t)p_sram4) ||
            (tp_sramdtcm != (uint32_t)p_sramdtcm)||
            (tp_sramitcm != (uint32_t)p_sramitcm))
        {
            tp_sramin = (uint32_t)p_sramin;
            tp_sram12 = (uint32_t)p_sram12;
            tp_sram4 = (uint32_t)p_sram4;
            tp_sramdtcm = (uint32_t)p_sramdtcm;
            tp_sramitcm = (uint32_t)p_sramitcm;
            
            sprintf((char *)paddr, "SRAMIN: Addr: 0x%08X", (uint32_t)p_sramin);                                                     /* 显示申请到的内存的首地址 */
            lcd_show_string(30, 236, 300, 16, 16, (char *)paddr, BLUE);
            sprintf((char *)paddr, "SRAM12: Addr: 0x%08X", (uint32_t)p_sram12);
            lcd_show_string(30, 252, 300, 16, 16, (char *)paddr, BLUE);
            sprintf((char *)paddr, "SRAM4: Addr: 0x%08X", (uint32_t)p_sram4);
            lcd_show_string(30, 268, 300, 16, 16, (char *)paddr, BLUE);
            sprintf((char *)paddr, "SRAMDTCM: Addr: 0x%08X", (uint32_t)p_sramdtcm);
            lcd_show_string(30, 284, 300, 16, 16, (char *)paddr, BLUE);
            sprintf((char *)paddr, "SRAMITCM: Addr: 0x%08X", (uint32_t)p_sramitcm);
            lcd_show_string(30, 300, 300, 16, 16, (char *)paddr, BLUE);
        }
        else if ((p_sramin == NULL) ||(p_sram12 == NULL) ||(p_sram4 == NULL) || (p_sramdtcm == NULL) || (p_sramitcm == NULL))
        {
            lcd_fill(30, 236, 300, 400, WHITE);
        }
        
        if ((t % 20) == 0)
        {
            memused = my_mem_perused(SRAMIN);
            sprintf((char *)paddr, "%d.%01d%%", memused / 10, memused % 10);
            lcd_show_string(30 + 112, 156, 200, 16, 16, (char *)paddr, BLUE);                                                       /* 显示内部内存使用率 */
            
            memused = my_mem_perused(SRAM12);
            sprintf((char *)paddr, "%d.%01d%%", memused / 10, memused % 10);
            lcd_show_string(30 + 112, 172, 200, 16, 16, (char *)paddr, BLUE);                                                       /* 显示TCM内存使用率 */
            
            memused = my_mem_perused(SRAM4);
            sprintf((char *)paddr, "%d.%01d%%", memused / 10, memused % 10);
            lcd_show_string(30 + 112, 188, 200, 16, 16, (char *)paddr, BLUE);                                                       /* 显示内部内存使用率 */
            
            memused = my_mem_perused(SRAMDTCM);
            sprintf((char *)paddr, "%d.%01d%%", memused / 10, memused % 10);
            lcd_show_string(30 + 112, 204, 200, 16, 16, (char *)paddr, BLUE);                                                       /* 显示外部内存使用率 */
            
            memused = my_mem_perused(SRAMITCM);
            sprintf((char *)paddr, "%d.%01d%%", memused / 10, memused % 10);
            lcd_show_string(30 + 112, 220, 200, 16, 16, (char *)paddr, BLUE);                                                       /* 显示TCM内存使用率 */
            
            LED0_TOGGLE();
        }
        
        delay_ms(10);
    }
}
