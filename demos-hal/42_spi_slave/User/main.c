/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.3
 * @date        2026-09-16
 * @brief       SPI2从机(Slave)通信实验 (环形DMA常驻版)
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 M100Z-M7最小系统板STM32H750版
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 *
 ****************************************************************************************************
 * 实验说明:
 *
 *  1. SPI2从机, 引脚: NSS=PB12, SCK=PB13, MISO=PB14, MOSI=PB15 (AF5)
 *     (M100Z-M7板上SPI2是唯一引出到排针的通用SPI, QSPI已接NOR FLASH不能占用)
 *
 *  2. 支持Master的SCK时钟: 5 / 10 / 15 / 20 MHz
 *     (SPI2内核时钟 = PCLK1 = 120MHz, 从机模式最高可跟约 60MHz 的SCK)
 *     Master端需配置为: SPI Mode0 (CPOL=0, CPHA=0), MSB先发, 8bit
 *
 *  3. Master写: NSS拉低期间发送的数据, 从机在NSS上升沿(一帧结束)后
 *     通过串口1(115200)以十六进制打印出来
 *
 *  4. Master读: 从机MISO恒定返回16字节应答数据(0x30~0x3F循环),
 *     Master每次读16字节都能拿到完整的 30 31 32 ... 3F
 *
 * -------------------------------------------------------------------------------------------------
 * 【V1.3 架构说明: 为什么改成"环形DMA常驻"】
 *
 *  V1.0~V1.2 用的是"每帧NSS上升沿 -> 停SPI/DMA -> 处理 -> 重新武装"的模型,
 *  实测在STM32H7上会踩三个大坑:
 *
 *   坑1  H7的SPI从机必须"先武装好, 再来时钟"。每帧重新武装的那段窗口里,
 *        如果Master已经开始打时钟, 首字节就会丢/错位。
 *   坑2  H7的SPI TX FIFO会被DMA预填充, 而 SPE=0 并不保证把FIFO清空;
 *        反复"停/启"会让Master读到上一次残留在FIFO里的旧字节。
 *   坑3  HAL的 HAL_SPI_TransmitReceive_DMA() 会顺带打开 OVR/UDR/FRE/MODF
 *        错误中断并维护内部State状态机。一旦DMA报错(例如FEIF), 它会直接把
 *        DMA流停掉、把State置为ERROR/READY, 之后 帧数据/应答数据 全部错乱,
 *        重新武装还会直接返回 HAL_BUSY(表现为串口打印"武装失败")。
 *
 *  V1.3 的做法: SPI使能一次就不再关, 收发DMA都配成 CIRCULAR(环形)模式常驻运行,
 *  只用 NSS(PB12) 的上升沿中断来"切割"帧:
 *
 *    - RX: 512字节环形缓冲, 永不停止。NSS上升沿时读DMA剩余计数NDTR,
 *          本帧长度 = (上次NDTR - 本次NDTR) & (512-1), 再从环形缓冲对应位置取出数据。
 *    - TX: 16字节环形缓冲(应答pattern)。缓冲长度正好等于pattern长度,
 *          因此FIFO里预装的永远是 30 31 ... 3F 的连续片段, Master读到的必然正确。
 *    - 完全不用HAL的SPI状态机, DMA也不开中断, 全片唯一中断源就是NSS的EXTI。
 *
 *  好处: 从机在任何时刻都是"武装好"的状态, 不存在重新武装窗口, 也不存在FIFO残留。
 * -------------------------------------------------------------------------------------------------
 *
 * 注意事项:
 *  - 单帧最大长度 <= 512 字节(SPI_RX_BUF_SIZE), 超过会回绕丢数据
 *  - 若业务上确实需要传输"全0xFF"的数据帧, 请删除毛刺过滤那一段
 *  - Master两次帧传输之间建议NSS保持高电平 >= 50us
 *
 ****************************************************************************************************
 */

#include <stdio.h>
#include <string.h>
#include "./SYSTEM/delay/delay.h"
#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./BSP/LED/led.h"
#include "main.h"

/******************************************************************************************/
/* 参数定义                                                                                */
/******************************************************************************************/

#define SPI_RX_BUF_SIZE         512             /* RX环形缓冲大小, 必须是2的幂(用于取模)        */
#define SPI_TX_PATTERN_LEN      16              /* 应答pattern长度(0x30~0x3F)                   */

/* Master读操作时, 从机返回的16字节应答数据 */
static const uint8_t g_spi_reply_pattern[SPI_TX_PATTERN_LEN] =
{
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
    0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F
};

/* 32字节对齐宏(兼容AC5/AC6) */
#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
#define ALIGNED32       __attribute__((aligned(32)))
#else
#define ALIGNED32       __align(32)
#endif

/******************************************************************************************/
/* 全局变量                                                                                */
/******************************************************************************************/

SPI_HandleTypeDef g_spi2_handle;                /* SPI2句柄                                     */
DMA_HandleTypeDef g_spi2_rx_dma;                /* SPI2 RX DMA句柄 (DMA1_Stream0)               */
DMA_HandleTypeDef g_spi2_tx_dma;                /* SPI2 TX DMA句柄 (DMA1_Stream1)               */

/* 收发DMA缓冲区: 32字节对齐, 位于AXI SRAM(0x24000000), DMA1可访问且可缓存 */
/* DCache为透写模式: CPU写直达RAM; RX方向DMA写内存后CPU读取前必须Invalidate */
ALIGNED32 static uint8_t g_spi_rx_buf[SPI_RX_BUF_SIZE];
ALIGNED32 static uint8_t g_spi_tx_buf[32];      /* 实际只用前16字节, 留满1个Cache行便于整行操作 */

static uint8_t  g_spi_frame_buf[SPI_RX_BUF_SIZE];                       /* 帧影子缓冲(中断中拷贝) */
static char     g_hexline[SPI_RX_BUF_SIZE * 3 + SPI_RX_BUF_SIZE / 8 + 16]; /* 十六进制行缓冲     */

volatile uint16_t g_spi_rx_prev_ndtr = SPI_RX_BUF_SIZE; /* 上次NSS上升沿时的DMA剩余计数         */
volatile uint16_t g_spi_rx_ndtr      = SPI_RX_BUF_SIZE; /* 最近一次NSS上升沿时的DMA剩余计数     */
volatile uint8_t  g_spi_frame_ready  = 0;               /* 帧就绪标志(1:有新帧待打印)           */
volatile uint16_t g_spi_frame_len    = 0;               /* 本帧长度                             */
volatile uint32_t g_spi_irq_cnt      = 0;               /* NSS上升沿(EXTI)触发次数              */
volatile uint32_t g_spi_glitch_cnt   = 0;               /* 全0xFF毛刺帧丢弃数                   */
volatile uint32_t g_spi_err_cnt      = 0;               /* SPI OVR/UDR/FRE/MODF 错误次数        */
volatile uint32_t g_spi_fifo_stuck   = 0;               /* RX FIFO排空超时次数(正常恒为0)       */

static const char g_hex_tab[16] = "0123456789ABCDEF";

/******************************************************************************************/
/* 函数声明                                                                                */
/******************************************************************************************/

static void spi2_gpio_init(void);
static void spi2_dma_init(void);
static void spi2_init(void);
static void spi2_slave_start(void);
static uint8_t spi_probe_pin(uint16_t pin);
static void spi_line_report(void);
static void spi_dump_regs(void);

/**
 * @brief   SPI2引脚初始化
 * @param   无
 * @retval  无
 */
static void spi2_gpio_init(void)
{
    GPIO_InitTypeDef gpio_init_struct = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();               /* 使能GPIOB时钟                                */

    /* SCK/MISO/MOSI: 复用推挽(AF5=SPI2) */
    gpio_init_struct.Pin       = SPI2_SCK_PIN | SPI2_MISO_PIN | SPI2_MOSI_PIN;
    gpio_init_struct.Mode      = GPIO_MODE_AF_PP;
    gpio_init_struct.Pull      = GPIO_NOPULL;
    gpio_init_struct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init_struct.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(SPI2_GPIO_PORT, &gpio_init_struct);

    /* NSS(PB12): SPI用软件NSS(SSI=0, 从机常选中), 该脚只作普通输入+EXTI上升沿, 用于切帧 */
    /* 上拉: 总线空闲时保持高电平(未选中)                                                */
    gpio_init_struct.Pin       = SPI2_NSS_PIN;
    gpio_init_struct.Mode      = GPIO_MODE_IT_RISING;
    gpio_init_struct.Pull      = GPIO_PULLUP;
    gpio_init_struct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init_struct.Alternate = 0;
    HAL_GPIO_Init(SPI2_GPIO_PORT, &gpio_init_struct);

    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);             /* 最高抢占优先级               */
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

/**
 * @brief   SPI2的DMA初始化(RX=DMA1_Stream0, TX=DMA1_Stream1, 都是环形模式)
 * @note    不使用DMA中断: 帧边界完全由NSS(PB12)EXTI判定
 * @param   无
 * @retval  无
 */
static void spi2_dma_init(void)
{
    __HAL_RCC_DMA1_CLK_ENABLE();                /* 使能DMA1时钟                                 */

    /* ---- SPI2 RX DMA : DMA1_Stream0, 外设->内存, 环形模式 ---- */
    g_spi2_rx_dma.Instance                   = DMA1_Stream0;
    g_spi2_rx_dma.Init.Request               = DMA_REQUEST_SPI2_RX;
    g_spi2_rx_dma.Init.Direction             = DMA_PERIPH_TO_MEMORY;
    g_spi2_rx_dma.Init.PeriphInc             = DMA_PINC_DISABLE;
    g_spi2_rx_dma.Init.MemInc                = DMA_MINC_ENABLE;
    g_spi2_rx_dma.Init.PeriphDataAlignment   = DMA_PDATAALIGN_BYTE;
    g_spi2_rx_dma.Init.MemDataAlignment      = DMA_MDATAALIGN_BYTE;
    g_spi2_rx_dma.Init.Mode                  = DMA_CIRCULAR;
    g_spi2_rx_dma.Init.Priority              = DMA_PRIORITY_VERY_HIGH;
    g_spi2_rx_dma.Init.FIFOMode              = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&g_spi2_rx_dma);

    g_spi2_rx_dma.XferCpltCallback     = NULL;  /* 不用回调, 也就不会开DMA中断                  */
    g_spi2_rx_dma.XferHalfCpltCallback = NULL;
    g_spi2_rx_dma.XferErrorCallback    = NULL;
    g_spi2_rx_dma.XferAbortCallback    = NULL;

    __HAL_LINKDMA(&g_spi2_handle, hdmarx, g_spi2_rx_dma);

    /* ---- SPI2 TX DMA : DMA1_Stream1, 内存->外设, 环形模式 ---- */
    g_spi2_tx_dma.Instance                   = DMA1_Stream1;
    g_spi2_tx_dma.Init.Request               = DMA_REQUEST_SPI2_TX;
    g_spi2_tx_dma.Init.Direction             = DMA_MEMORY_TO_PERIPH;
    g_spi2_tx_dma.Init.PeriphInc             = DMA_PINC_DISABLE;
    g_spi2_tx_dma.Init.MemInc                = DMA_MINC_ENABLE;
    g_spi2_tx_dma.Init.PeriphDataAlignment   = DMA_PDATAALIGN_BYTE;
    g_spi2_tx_dma.Init.MemDataAlignment      = DMA_MDATAALIGN_BYTE;
    g_spi2_tx_dma.Init.Mode                  = DMA_CIRCULAR;
    g_spi2_tx_dma.Init.Priority              = DMA_PRIORITY_HIGH;
    g_spi2_tx_dma.Init.FIFOMode              = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&g_spi2_tx_dma);

    g_spi2_tx_dma.XferCpltCallback     = NULL;
    g_spi2_tx_dma.XferHalfCpltCallback = NULL;
    g_spi2_tx_dma.XferErrorCallback    = NULL;
    g_spi2_tx_dma.XferAbortCallback    = NULL;

    __HAL_LINKDMA(&g_spi2_handle, hdmatx, g_spi2_tx_dma);

    /* 注意: 这里不开启 DMA1_Stream0/1 的NVIC中断 */
}

/**
 * @brief   SPI2初始化(从机模式, 软件NSS)
 * @param   无
 * @retval  无
 */
static void spi2_init(void)
{
    __HAL_RCC_SPI2_CLK_ENABLE();                /* 使能SPI2时钟                                 */

    g_spi2_handle.Instance                        = SPI2;
    g_spi2_handle.Init.Mode                       = SPI_MODE_SLAVE;          /* 从机模式         */
    g_spi2_handle.Init.Direction                  = SPI_DIRECTION_2LINES;    /* 全双工           */
    g_spi2_handle.Init.DataSize                   = SPI_DATASIZE_8BIT;       /* 8bit帧格式       */
    g_spi2_handle.Init.CLKPolarity                = SPI_POLARITY_LOW;        /* CPOL=0           */
    g_spi2_handle.Init.CLKPhase                   = SPI_PHASE_1EDGE;         /* CPHA=0 => Mode0  */
    g_spi2_handle.Init.NSS                        = SPI_NSS_SOFT;            /* 软件NSS          */
    g_spi2_handle.Init.FirstBit                   = SPI_FIRSTBIT_MSB;        /* MSB先发          */
    g_spi2_handle.Init.TIMode                     = SPI_TIMODE_DISABLE;      /* TI模式关闭       */
    g_spi2_handle.Init.CRCCalculation             = SPI_CRCCALCULATION_DISABLE;
    g_spi2_handle.Init.CRCPolynomial              = 7;
    g_spi2_handle.Init.CRCLength                  = SPI_CRC_LENGTH_8BIT;
    g_spi2_handle.Init.NSSPMode                   = SPI_NSS_PULSE_DISABLE;   /* NSS脉冲模式关闭  */
    g_spi2_handle.Init.NSSPolarity                = SPI_NSS_POLARITY_LOW;
    g_spi2_handle.Init.FifoThreshold              = SPI_FIFO_THRESHOLD_01DATA; /* 每1字节搬运    */
    g_spi2_handle.Init.MasterKeepIOState          = SPI_MASTER_KEEP_IO_STATE_DISABLE;
    HAL_SPI_Init(&g_spi2_handle);

    /* HAL只在 (从机 && NSSPolarity==HIGH) 时才置SSI; 这里显式清0 => 从机常选中 */
    /* (MISO常驱动输出, 只适用于单从机总线)                                      */
    CLEAR_BIT(g_spi2_handle.Instance->CR1, SPI_CR1_SSI);

    /* 关闭SPI自身的错误中断(本工程不用HAL的SPI状态机, 出错靠轮询SR诊断) */
    WRITE_REG(g_spi2_handle.Instance->IER, 0);
}

/**
 * @brief   启动SPI2从机: 武装收发环形DMA + 使能SPI(此后一直不停)
 * @param   无
 * @retval  无
 */
static void spi2_slave_start(void)
{
    uint16_t i;

    /* 1. TX应答缓冲填充: 16字节pattern。DMA是环形, 长度=16=pattern长度,  */
    /*    所以FIFO里预装的永远是 30 31 ... 3F 的连续片段                    */
    for (i = 0; i < SPI_TX_PATTERN_LEN; i++)
    {
        g_spi_tx_buf[i] = g_spi_reply_pattern[i];
    }
    SCB_CleanDCache_by_Addr((uint32_t *)g_spi_tx_buf, 32);

    /* 2. 直接启动两路DMA(HAL_DMA_Start_IT只配DMA, 不碰SPI, 不会改SPI状态机) */
    HAL_DMA_Start_IT(&g_spi2_rx_dma, (uint32_t)&SPI2->RXDR, (uint32_t)g_spi_rx_buf, SPI_RX_BUF_SIZE);
    HAL_DMA_Start_IT(&g_spi2_tx_dma, (uint32_t)g_spi_tx_buf, (uint32_t)&SPI2->TXDR, SPI_TX_PATTERN_LEN);

    /* HAL_DMA_Start_IT() 会无条件打开 TC/TE/DME 中断, 本工程不用DMA中断, 显式关掉 */
    __HAL_DMA_DISABLE_IT(&g_spi2_rx_dma, (DMA_IT_TC | DMA_IT_TE | DMA_IT_DME | DMA_IT_HT));
    __HAL_DMA_DISABLE_IT(&g_spi2_tx_dma, (DMA_IT_TC | DMA_IT_TE | DMA_IT_DME | DMA_IT_HT));

    /* 3. 打开SPI的DMA请求, 再使能SPI。之后SPI/DMA一直常驻, 不再关闭 */
    SET_BIT(SPI2->CFG1, SPI_CFG1_RXDMAEN | SPI_CFG1_TXDMAEN);
    __HAL_SPI_ENABLE(&g_spi2_handle);

    /* 4. 记录初始剩余计数 */
    g_spi_rx_prev_ndtr = (uint16_t)__HAL_DMA_GET_COUNTER(&g_spi2_rx_dma);
    g_spi_rx_ndtr      = g_spi_rx_prev_ndtr;
}

/**
 * @brief   探测某引脚当前是否被外部驱动(用内部上/下拉对比)
 * @param   pin : 引脚号
 * @retval  0=悬空/未接   1=外部驱动为低   2=外部驱动为高
 */
static uint8_t spi_probe_pin(uint16_t pin)
{
    GPIO_InitTypeDef gpio_init_struct = {0};
    volatile uint32_t n;
    uint8_t lo, hi;

    gpio_init_struct.Pin       = pin;
    gpio_init_struct.Mode      = GPIO_MODE_INPUT;
    gpio_init_struct.Speed     = GPIO_SPEED_FREQ_LOW;
    gpio_init_struct.Alternate = 0;

    gpio_init_struct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(SPI2_GPIO_PORT, &gpio_init_struct);
    for (n = 0; n < 20000; n++) { __NOP(); }
    lo = (HAL_GPIO_ReadPin(SPI2_GPIO_PORT, pin) == GPIO_PIN_SET) ? 1 : 0;

    gpio_init_struct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(SPI2_GPIO_PORT, &gpio_init_struct);
    for (n = 0; n < 20000; n++) { __NOP(); }
    hi = (HAL_GPIO_ReadPin(SPI2_GPIO_PORT, pin) == GPIO_PIN_SET) ? 1 : 0;

    if (lo == hi)                               /* 内部上下拉拉不动 => 外部有强驱动       */
    {
        return (lo != 0) ? 2 : 1;
    }
    return 0;                                   /* 电平跟着内部上下拉变 => 悬空/没接       */
}

/**
 * @brief   信号线自检并打印(用于快速排查接线问题)
 * @param   无
 * @retval  无
 */
static void spi_line_report(void)
{
    uint8_t sck, mosi, miso, nss;
    const char *s;

    sck  = spi_probe_pin(SPI2_SCK_PIN);
    mosi = spi_probe_pin(SPI2_MOSI_PIN);
    miso = spi_probe_pin(SPI2_MISO_PIN);
    nss  = spi_probe_pin(SPI2_NSS_PIN);

    printf("---- 信号线自检(Master保持上电, 用内部上下拉对比, 只作参考) ----\r\n");

    s = (sck == 0) ? "悬空/未接 !!" : ((sck == 1) ? "外部驱动为低" : "外部驱动为高");
    printf("  SCK  (PB13): %s\r\n", s);
    s = (mosi == 0) ? "悬空/未接 !!" : ((mosi == 1) ? "外部驱动为低" : "外部驱动为高");
    printf("  MOSI (PB15): %s\r\n", s);
    s = (miso == 0) ? "悬空(正常, 由STM32驱动)" : ((miso == 1) ? "被拉为低" : "被拉为高");
    printf("  MISO (PB14): %s\r\n", s);
    s = (nss == 0) ? "悬空/未接 !!" : ((nss == 1) ? "外部驱动为低(CS正有效)" : "外部驱动为高(空闲)");
    printf("  NSS  (PB12): %s\r\n", s);
    printf("\r\n");
}

/**
 * @brief   打印关键寄存器快照(用于确认DMA/SPI配置是否真的生效)
 * @param   无
 * @retval  无
 */
static void spi_dump_regs(void)
{
    printf("---- 寄存器快照 ----\r\n");
    printf(" SPI2  : CR1=%08lX CFG1=%08lX CFG2=%08lX SR=%08lX\r\n",
           (unsigned long)SPI2->CR1, (unsigned long)SPI2->CFG1,
           (unsigned long)SPI2->CFG2, (unsigned long)SPI2->SR);
    printf(" DMA_S0: CR=%08lX NDTR=%lu PAR=%08lX M0AR=%08lX (RX, 环形%u字节)\r\n",
           (unsigned long)DMA1_Stream0->CR, (unsigned long)DMA1_Stream0->NDTR,
           (unsigned long)DMA1_Stream0->PAR, (unsigned long)DMA1_Stream0->M0AR,
           (unsigned)SPI_RX_BUF_SIZE);
    printf(" DMA_S1: CR=%08lX NDTR=%lu PAR=%08lX M0AR=%08lX (TX, 环形%u字节)\r\n",
           (unsigned long)DMA1_Stream1->CR, (unsigned long)DMA1_Stream1->NDTR,
           (unsigned long)DMA1_Stream1->PAR, (unsigned long)DMA1_Stream1->M0AR,
           (unsigned)SPI_TX_PATTERN_LEN);
    printf(" 说明: CR的bit10(MINC)与bit8(CIRC)应都为1; NDTR应等于缓冲大小\r\n\r\n");
}

/**
 * @brief   EXTI回调: NSS(PB12)上升沿 => 一帧结束
 * @note    这里只做"读计数 + 搬数据", 不碰SPI/DMA的启停, 所以从机永远是武装好的
 * @param   GPIO_Pin : 触发的引脚
 * @retval  无
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    uint16_t cur;
    uint16_t off;
    uint16_t len;
    uint16_t n1;
    uint16_t n2;
    uint16_t i;
    uint32_t guard;
    uint32_t sr;

    if (GPIO_Pin != SPI2_NSS_PIN)
    {
        return;
    }

    g_spi_irq_cnt++;

    /* 1. 等SPI的RX FIFO排空, 说明DMA已经把最后1个字节写进RAM了 */
    /*    (NSS上升沿到最后一个SCK下降沿之间只有很短时间, 先等DMA搬完) */
    guard = 20000UL;
    while (((SPI2->SR & SPI_SR_RXWNE) != 0UL) && (guard != 0UL))
    {
        guard--;
    }
    if (guard == 0UL)
    {
        g_spi_fifo_stuck++;                     /* 极端情况: FIFO一直不空, 计数便于定位 */
    }

    /* 2. 读DMA剩余计数, 算出本帧在环形缓冲里的起始偏移和长度 */
    cur = (uint16_t)__HAL_DMA_GET_COUNTER(&g_spi2_rx_dma);
    off = (uint16_t)((SPI_RX_BUF_SIZE - g_spi_rx_prev_ndtr) & (SPI_RX_BUF_SIZE - 1U));
    len = (uint16_t)((g_spi_rx_prev_ndtr - cur)              & (SPI_RX_BUF_SIZE - 1U));
    g_spi_rx_prev_ndtr = cur;
    g_spi_rx_ndtr      = cur;

    /* 3. SPI错误标志统计(溢出/欠载/帧错误/模式错误), 并清标志 */
    sr = SPI2->SR;
    if ((sr & (SPI_SR_OVR | SPI_SR_UDR | SPI_SR_TIFRE | SPI_SR_MODF)) != 0UL)
    {
        g_spi_err_cnt++;
        __HAL_SPI_CLEAR_OVRFLAG(&g_spi2_handle);
        __HAL_SPI_CLEAR_UDRFLAG(&g_spi2_handle);
        __HAL_SPI_CLEAR_FREFLAG(&g_spi2_handle);
        __HAL_SPI_CLEAR_MODFFLAG(&g_spi2_handle);
    }

    if ((len == 0U) || (len >= SPI_RX_BUF_SIZE))
    {
        return;                                 /* 空帧(毛刺造成的重复上升沿), 直接忽略 */
    }

    /* 4. DMA写的是AXI SRAM, CPU读前必须失效Cache */
    SCB_InvalidateDCache_by_Addr((uint32_t *)g_spi_rx_buf, SPI_RX_BUF_SIZE);

    /* 5. 从环形缓冲取本帧数据(处理回绕) */
    n1 = len;
    if (((uint32_t)off + (uint32_t)len) > (uint32_t)SPI_RX_BUF_SIZE)
    {
        n1 = (uint16_t)(SPI_RX_BUF_SIZE - off);
    }
    n2 = (uint16_t)(len - n1);

    memcpy((void *)g_spi_frame_buf, (const void *)&g_spi_rx_buf[off], n1);
    if (n2 != 0U)
    {
        memcpy((void *)&g_spi_frame_buf[n1], (const void *)&g_spi_rx_buf[0], n2);
    }

    /* 6. 过滤毛刺帧: Master板(如RK3576)上电/复位期间, SCK电平抖动会被从机   */
    /*    当成时钟采样, MOSI悬空为高 => 收到全0xFF。此类帧静默丢弃            */
    /*    (若业务上确实需要传输全0xFF数据帧, 请删除本段)                      */
    for (i = 0; i < len; i++)
    {
        if (g_spi_frame_buf[i] != 0xFF)
        {
            break;
        }
    }
    if (i >= len)
    {
        g_spi_glitch_cnt++;
        return;
    }

    g_spi_frame_len   = len;
    g_spi_frame_ready = 1;                      /* 通知主循环打印(不在中断里做慢速打印) */
}

/**
 * @brief   主函数
 * @param   无
 * @retval  int
 */
int main(void)
{
    uint16_t i;
    uint16_t j;
    uint16_t len;
    uint32_t tick_led  = 0;
    uint32_t tick_diag = 0;
    uint32_t diag_irq  = 0;
    uint32_t diag_glit = 0;
    uint32_t diag_err  = 0;
    uint32_t diag_stuck = 0;
    uint8_t  led_state = 0;

    sys_cache_enable();                  /* 打开L1-Cache(D-Cache强制透写) */
    HAL_Init();                          /* 初始化HAL库 */
    sys_stm32_clock_init(240, 2, 2, 4);  /* 设置系统时钟, 480MHz, APB1/2=120MHz */
    delay_init(480);                     /* 初始化延时功能 */
    usart_init(115200);                  /* 初始化串口1, 115200bps(打印用) */
    led_init();                          /* 初始化LED */

    printf("\r\n\r\n===== 正点原子 M100Z-M7 SPI2 Slave Demo (V1.3 环形DMA常驻版) =====\r\n");
    printf("系统时钟: 480MHz | SPI2内核时钟: PCLK1 = 120MHz\r\n");
    printf("支持Master SCK: 5 / 10 / 15 / 20 MHz (Mode0, MSB先发, 8bit)\r\n");
    printf("接线: NSS=PB12, SCK=PB13, MISO=PB14, MOSI=PB15, 必须共地\r\n");
    printf("Master写: 收到数据后打印在串口1\r\n");
    printf("Master读: 每帧返回16字节(30 31 32 ... 3F)\r\n\r\n");

    spi_line_report();                   /* 上电先报告各线电平, 便于排查接线 */

    spi2_gpio_init();                    /* SPI2引脚 + NSS帧边界EXTI */
    spi2_dma_init();                     /* SPI2收发环形DMA */
    spi2_init();                         /* SPI2从机模式初始化 */
    spi2_slave_start();                  /* 武装DMA并使能SPI, 之后一直常驻 */

    spi_dump_regs();                     /* 打印寄存器快照, 确认配置生效 */

    printf("已就绪, 等待Master...\r\n\r\n");

    while (1)
    {
        /* 有新帧? 打印本帧收到的数据 */
        if (g_spi_frame_ready)
        {
            len = g_spi_frame_len;          /* 先取走长度和标志, 防止与新帧竞争 */
            g_spi_frame_ready = 0;

            j = 0;
            for (i = 0; i < len; i++)
            {
                g_hexline[j++] = g_hex_tab[(g_spi_frame_buf[i] >> 4) & 0x0F];
                g_hexline[j++] = g_hex_tab[g_spi_frame_buf[i] & 0x0F];
                g_hexline[j++] = ' ';

                if ((i & 0x0F) == 0x0F)     /* 每16字节换行 */
                {
                    g_hexline[j++] = '\r';
                    g_hexline[j++] = '\n';
                }
            }
            g_hexline[j] = '\0';

            printf("[SPI2 Slave] 收到 %u 字节:\r\n%s\r\n", (unsigned)len, g_hexline);
        }

        /* LED0心跳翻转, 500ms一次, 指示程序在运行 */
        if ((HAL_GetTick() - tick_led) >= 500)
        {
            tick_led = HAL_GetTick();
            led_state = !led_state;
            LED0(led_state);
        }

        /* 每3秒输出一次诊断计数(只在有变化时打印, 避免刷屏) */
        if ((HAL_GetTick() - tick_diag) >= 3000)
        {
            tick_diag = HAL_GetTick();

            if ((g_spi_irq_cnt != diag_irq) || (g_spi_glitch_cnt != diag_glit) ||
                (g_spi_err_cnt != diag_err) || (g_spi_fifo_stuck != diag_stuck))
            {
                diag_irq   = g_spi_irq_cnt;
                diag_glit  = g_spi_glitch_cnt;
                diag_err   = g_spi_err_cnt;
                diag_stuck = g_spi_fifo_stuck;

                printf("[诊断] NSS边沿=%lu  毛刺帧丢弃=%lu  SPI错误=%lu  FIFO排空超时=%lu\r\n",
                       (unsigned long)diag_irq, (unsigned long)diag_glit,
                       (unsigned long)diag_err, (unsigned long)diag_stuck);
            }
        }
    }
}
