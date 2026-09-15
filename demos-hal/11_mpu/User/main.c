/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-04-25
 * @brief       内存保护(MPU)实验
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

#include "stdlib.h"
#include "./SYSTEM/delay/delay.h"
#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./BSP/LED/led.h"
#include "./BSP/KEY/key.h"
#include "./BSP/MPU/mpu.h"

#if !(__ARMCC_VERSION >= 6010050)                                           /* 不是AC6编译器，即使用AC5编译器时 */
uint8_t mpudata[128] __attribute__((at(0X20002000)));                       /* 定义一个数组 */
#else
uint8_t mpudata[128] __attribute__((section(".bss.ARM.__at_0X20002000")));  /* 定义一个数组 */
#endif

int main(void)
{
    uint8_t key = 0;
    uint8_t t = 0;
    
    sys_cache_enable();                                 /* 打开L1-Cache */
    HAL_Init();                                         /* 初始化HAL库 */
    sys_stm32_clock_init(240, 2, 2, 4);                 /* 设置系统时钟, 480Mhz */
    delay_init(480);                                    /* 初始化延时功能 */
    usart_init(115200);                                 /* 初始化串口 */
    led_init();                                         /* 初始化LED */
    key_init();                                         /* 初始化按键 */
    printf("\r\n\r\nMPU closed!\r\n");                  /* 提示MPU关闭 */

    while (1)
    {
        key = key_scan(0);
        
        if (key == WKUP_PRES)                           /* 使能MPU保护数组 mpudata */
        {
            /* 只读,禁止共用,禁止catch,允许缓冲 */
            mpu_set_protection(0X20002000, MPU_REGION_SIZE_128B, MPU_REGION_NUMBER0, MPU_REGION_FULL_ACCESS, MPU_REGION_PRIV_RO_URO,
                               MPU_ACCESS_NOT_SHAREABLE, MPU_ACCESS_NOT_CACHEABLE, MPU_ACCESS_BUFFERABLE);  
            printf("MPU open!\r\n");                    /* 提示MPU打开 */
        }
        else if (key == KEY0_PRES)                      /* 向数组中写入数据，如果开启了MPU保护的话会进入内存访问错误！ */
        {
            printf("Start Writing data...\r\n");
            sprintf((char *)mpudata, "MPU test array %d", t);
            printf("Data Write finshed!\r\n");
            printf("Array data is:%s\r\n", mpudata);    /* 从数组中读取数据，不管有没有开启MPU保护都不会进入内存访问错误！ */
        }
        else 
        {
            delay_ms(10);
        }
        
        t++;
        
        if ((t % 50) == 0)
        {
            LED0_TOGGLE();                              /* LED0取反 */
        }
    }
}
