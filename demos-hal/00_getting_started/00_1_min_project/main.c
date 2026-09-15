/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-04-25
 * @brief       最精简STM32工程,除了启动文件(.s文件),未使用任何库文件
 *              该代码实现功能：通过PE5控制LED0闪烁
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

/* 总线基地址定义 */
#define PERIPH_BASE         0x40000000                                      /* 外设基地址 */

#define D2_APB1PERIPH_BASE  PERIPH_BASE                                     /* APB1总线基地址 */
#define D3_AHB1PERIPH_BASE  (PERIPH_BASE + 0x18020000UL)                    /* AHB4总线基地址 */

/* 外设基地址定义 */           
#define RCC_BASE            (D3_AHB1PERIPH_BASE + 0x4400UL)                 /* RCC基地址 */
#define GPIOE_BASE          (D3_AHB1PERIPH_BASE + 0x1000UL)                 /* GPIOE基地址 */

/* 外设相关寄存器映射(定义) */
#define RCC_AHB4ENR         *(volatile unsigned int *)(RCC_BASE + 0xE0)     /* RCC_AHB4ENR寄存器映射 */
#define GPIOE_MODER         *(volatile unsigned int *)(GPIOE_BASE + 0x00)   /* GPIOE_MODER寄存器映射 */
#define GPIOE_OTYPER        *(volatile unsigned int *)(GPIOE_BASE + 0x04)   /* GPIOE_OTYPER寄存器映射 */
#define GPIOE_OSPEEDR       *(volatile unsigned int *)(GPIOE_BASE + 0x08)   /* GPIOE_OSPEEDR寄存器映射 */
#define GPIOE_PUPDR         *(volatile unsigned int *)(GPIOE_BASE + 0x0C)   /* GPIOE_PUPDR寄存器映射 */
#define GPIOE_IDR           *(volatile unsigned int *)(GPIOE_BASE + 0x10)   /* GPIOE_IDR寄存器映射 */
#define GPIOE_ODR           *(volatile unsigned int *)(GPIOE_BASE + 0x14)   /* GPIOE_ODR寄存器映射 */

/* 延时函数 */
static void delay_x(volatile unsigned int t)
{
    while(t--);
}

/* main函数 */
int main(void)
{
    /* 未执行任何PLL时钟配置, 默认使用HSI(64M)工作, 相当于工作在主频64Mhz频率下 */
    
    RCC_AHB4ENR |= 1 << 4;            /* GPIOE 时钟使能 */
    GPIOE_MODER &= ~(0X03UL << 10);   /* MODER5[1:0], 清零 */
    GPIOE_MODER |= 0X01UL << 10;      /* MODER5[1:0]=1, PE5输出模式 */
    GPIOE_OTYPER &= ~(0X01UL << 5);   /* OT5, 清零, 推挽输出 */
    GPIOE_OSPEEDR &= ~(0X03UL << 10); /* OSPEEDR5[1:0], 清零 */
    GPIOE_OSPEEDR |= 0X01UL << 10;    /* OSPEEDR5[1:0]=1, 中速 */
    GPIOE_PUPDR &= ~(0X03UL << 10);   /* PUPDR5[1:0], 清零 */
    GPIOE_PUPDR |= 0X01UL << 10;      /* PUPDR5[1:0]=1, 上拉 */
    
    while(1)
    {
        GPIOE_ODR |= 1 << 5;          /* PE5 = 1, LED0灭 */
        delay_x(5000000);             /* 延时一定时间 */
        GPIOE_ODR &= ~(1UL << 5);     /* PE5 = 0, LED0亮 */
        delay_x(5000000);             /* 延时一定时间 */
    }
}
