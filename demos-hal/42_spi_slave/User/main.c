/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.4
 * @date        2026-09-17
 * @brief       SPI2从机(Slave)通信实验 (环形DMA常驻 + 位对齐自诊断)
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
 *  2. 支持Master的SCK时钟: 100kHz ~ 10MHz
 *     (SPI2内核时钟 = PCLK1 = 120MHz, 从机模式最高可跟约 60MHz 的SCK)
 *
 *  3. Master写: NSS拉低期间发送的数据, 从机在NSS上升沿(一帧结束)后
 *     通过串口1(115200)以十六进制打印出来
 *
 *  4. Master读: 从机MISO恒定循环返回16字节应答数据
 *     V1.4默认应答 = 16个 0xA5 (位对齐探针, 见下方说明)
 *
 * -------------------------------------------------------------------------------------------------
 * 【V1.3 架构说明: 为什么用"环形DMA常驻"】
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
 *    - TX: 16字节环形缓冲(应答pattern)。
 *    - 完全不用HAL的SPI状态机, DMA也不开中断, 全片唯一中断源就是NSS的EXTI。
 *
 *  好处: 从机在任何时刻都是"武装好"的状态, 不存在重新武装窗口, 也不存在FIFO残留。
 * -------------------------------------------------------------------------------------------------
 * 【V1.4 说明: 为什么把应答pattern换成 0xA5, 以及位对齐怎么自诊断】
 *
 *  现象: V1.3 用 0x30~0x3F 做应答pattern时, 实测"看起来数据不对", 但排查很久
 *        都说不清到底错在哪。根因之一在于 0x30~0x3F 是一条"递增斜坡":
 *        - 任何一个位的偏移, 移完仍然是一条递增斜坡, 肉眼根本区分不出来;
 *        - 逻辑分析仪上看起来也还是"干净漂亮的递增", 极容易被误判为正常;
 *        换句话说, 斜坡数据在诊断上是"最坏的选择"。用逻辑分析仪抓到的
 *        0x70 0x72 0x74 ... 就是斜坡被整体挪动1bit之后的结果。
 *
 *  0xA5 探针的好处: 0xA5 = 1010_0101, 它的8种循环移位互不相同:
 *        A5 / 4B / 96 / 2D / 5A / B4 / 69 / D2
 *        于是:
 *          - 位对齐正确 => Master读到的应该是  A5 A5 A5 A5 ...
 *          - 整体偏1位   => Master读到的会变成  4B 4B 4B 4B ... (或 52 52 ...)
 *          - 偏2位 => 96, 偏3位 => 2D ... 一一对应, 一眼定位"偏了几位"
 *        而且pattern只有1字节周期, 环形缓冲的"相位"是多少都不影响正确性。
 *
 *  更省事的是从机侧会自动判读: Master只要重复发同一个字节(推荐发 0xA5),
 *  从机收到整帧字节相同时, 会自动算出它相对 0xA5 的循环移位量并打印结论,
 *  直接告诉你是"位对齐正确"还是"偏了N位"。
 *
 *  如果实测确认是位偏移, 把下面的 SPI_DEMO_MODE 依次改成 1/2/3 重新编译烧录,
 *  总有一个能对上(0=Mode0, 1=Mode1, 2=Mode2, 3=Mode3)。Master端必须同步改成同样模式。
 * -------------------------------------------------------------------------------------------------
 *
 * 注意事项:
 *  - 单帧最大长度 <= 256 字节(SPI_FRAME_MAX_LEN), 超过视为NSS漏沿被丢弃
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
#define SPI_TX_PATTERN_LEN      16              /* 应答pattern长度                              */
#define SPI_FRAME_MAX_LEN       256             /* 单帧长度上限(超过视为NSS漏沿, 丢弃)          */

/* ==================== V1.4 可配置项 ==================== */

/* 应答pattern选择:
     0 = 0xA5 位对齐探针(强烈推荐) —— 任何位偏移都会变成另一个固定值, 一眼可判
     1 = 0x30~0x3F 递增斜坡       —— V1.3的旧行为, 仅用于兼容老测试脚本         */
#define SPI_TX_PATTERN_MODE     0

/* 从机SPI模式: 0=Mode0(CPOL0/CPHA0) 1=Mode1 2=Mode2 3=Mode3
   默认0(与Master的 spi mode 0x0 对应); 若实测发现主从数据整体错位,
   依次换成 1/2/3 重新编译测试, 同时把Master改成同样模式                  */
#define SPI_DEMO_MODE           0

/* 从机片选(NSS)管理方式 —— V1.5 新增
 *  0 = 硬件NSS (V1.5 默认, 推荐)
 *      PB12 走 AF5 交给 SPI2 硬件做真片选: NSS 无效(高)期间 SPI 直接忽略 SCK。
 *      V1.4 及以前用软件 NSS(SSI 恒 0)把从机"永久选中", 于是 Master 在帧间
 *      切换 CS 时 SCK 上的任何扰动都会被当成一个数据位, 整帧因此偏 1 位
 *      且永不恢复 —— 实测: 发 A5 收 D2(A5 循环右移 1 位),
 *      MISO 上则是 70 72 74 76 78 7A 7C 7E(= 38~3F 各左移 1 位)。
 *  1 = 软件 NSS (旧行为, 保留以便对比/回退)                            */
#define SPI_NSS_MODE            0

/* Master会重复发送的"已知字节", 从机用它自动反推位偏移(诊断用)             */
#define SPI_PROBE_BYTE          0xA5

#if (SPI_TX_PATTERN_MODE == 0)
/* 方案0: 16个 0xA5 —— 位对齐探针 */
static const uint8_t g_spi_reply_pattern[SPI_TX_PATTERN_LEN] =
{
    0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5,
    0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5
};
#else
/* 方案1: 16字节递增 0x30~0x3F (V1.3旧行为) */
static const uint8_t g_spi_reply_pattern[SPI_TX_PATTERN_LEN] =
{
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
    0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F
};
#endif

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

/* 收发DMA缓冲区: 32字节对齐, 位于AXI SRAM(0x24000000, 见User/SCRIPT/qspi_code.scf.scf
   的 RW_m_stmsram 段), DMA1可访问; 该区域可Cache, 由 sys_cache_enable() 打开D-Cache */
ALIGNED32 static uint8_t g_spi_rx_buf[SPI_RX_BUF_SIZE];
ALIGNED32 static uint8_t g_spi_tx_buf[32];      /* 实际只用前16字节, 留满1个Cache行便于整行操作 */

static uint8_t  g_spi_frame_buf[SPI_RX_BUF_SIZE];                       /* 帧影子缓冲(中断中拷贝) */
static uint8_t  g_print_buf[SPI_RX_BUF_SIZE];                           /* 打印用副本(与中断隔离) */
static char     g_hexline[SPI_RX_BUF_SIZE * 3 + SPI_RX_BUF_SIZE / 8 + 16]; /* 十六进制行缓冲     */

volatile uint16_t g_spi_rx_prev_ndtr = SPI_RX_BUF_SIZE; /* 上次NSS上升沿时的DMA剩余计数         */
volatile uint16_t g_spi_rx_ndtr      = SPI_RX_BUF_SIZE; /* 最近一次NSS上升沿时的DMA剩余计数     */
volatile uint8_t  g_spi_frame_ready  = 0;               /* 帧就绪标志(1:有新帧待打印)           */
volatile uint16_t g_spi_frame_len    = 0;               /* 本帧长度                             */
volatile uint32_t g_spi_irq_cnt      = 0;               /* NSS上升沿(EXTI)触发次数              */
volatile uint32_t g_spi_glitch_cnt   = 0;               /* 全0xFF毛刺帧丢弃数                   */
volatile uint32_t g_spi_err_cnt      = 0;               /* SPI OVR/UDR/FRE/MODF 错误次数        */
volatile uint32_t g_spi_fifo_stuck   = 0;               /* RX FIFO排空超时次数(正常恒为0)       */
volatile uint32_t g_spi_toolong_cnt  = 0;               /* 超过SPI_FRAME_MAX_LEN被丢弃的帧数    */

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
static int8_t spi_rot_of(uint8_t v, uint8_t ref);

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

    /* NSS(PB12) 要同时干两件事:
     *   (a) 给SPI硬件当片选输入(AF5) —— NSS无效期间SPI直接忽略SCK (SPI_NSS_MODE==0)
     *   (b) 帧边界检测 —— 上升沿走EXTI中断切帧
     * 先按EXTI输入配好, 稍后再把MODER叠成AF5 (见下方 #if SPI_NSS_MODE==0)。    */
    /* 上拉: 总线空闲时保持高电平(未选中)                                                */
    gpio_init_struct.Pin       = SPI2_NSS_PIN;
    gpio_init_struct.Mode      = GPIO_MODE_IT_RISING;
    gpio_init_struct.Pull      = GPIO_PULLUP;
    gpio_init_struct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init_struct.Alternate = 0;
    HAL_GPIO_Init(SPI2_GPIO_PORT, &gpio_init_struct);

#if (SPI_NSS_MODE == 0)
    /* 同一个 PB12 再叠一层 AF5: 交给 SPI2 硬件当 NSS 输入 (MODER=10, AFR=5)。
     * EXTI 的边沿检测取自引脚输入缓冲, 与 MODER 无关, 所以"硬件片选"与
     * "上升沿切帧"可以并存。PULLUP 保留: 总线空闲时 NSS 保持高(未选中)。   */
    MODIFY_REG(GPIOB->MODER, GPIO_MODER_MODE12, GPIO_MODER_MODE12_1);
    MODIFY_REG(GPIOB->AFR[1], (0xFUL << 16U), ((uint32_t)GPIO_AF5_SPI2 << 16U));
#endif

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
 * @brief   SPI2初始化(从机模式, NSS方式由SPI_NSS_MODE决定, SPI模式由SPI_DEMO_MODE决定)
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

#if   (SPI_DEMO_MODE == 0)                      /* Mode0: CPOL=0, CPHA=0(空闲低, 第1边沿采样) */
    g_spi2_handle.Init.CLKPolarity                = SPI_POLARITY_LOW;
    g_spi2_handle.Init.CLKPhase                   = SPI_PHASE_1EDGE;
#elif (SPI_DEMO_MODE == 1)                      /* Mode1: CPOL=0, CPHA=1                    */
    g_spi2_handle.Init.CLKPolarity                = SPI_POLARITY_LOW;
    g_spi2_handle.Init.CLKPhase                   = SPI_PHASE_2EDGE;
#elif (SPI_DEMO_MODE == 2)                      /* Mode2: CPOL=1, CPHA=0                    */
    g_spi2_handle.Init.CLKPolarity                = SPI_POLARITY_HIGH;
    g_spi2_handle.Init.CLKPhase                   = SPI_PHASE_1EDGE;
#else                                           /* Mode3: CPOL=1, CPHA=1                    */
    g_spi2_handle.Init.CLKPolarity                = SPI_POLARITY_HIGH;
    g_spi2_handle.Init.CLKPhase                   = SPI_PHASE_2EDGE;
#endif

#if (SPI_NSS_MODE == 0)
    g_spi2_handle.Init.NSS                        = SPI_NSS_HARD_INPUT;      /* 硬件NSS(真片选)  */
#else
    g_spi2_handle.Init.NSS                        = SPI_NSS_SOFT;            /* 软件NSS(旧行为)  */
#endif
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
#if (SPI_NSS_MODE != 0)
    CLEAR_BIT(g_spi2_handle.Instance->CR1, SPI_CR1_SSI);
#endif

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

    /* 1. TX应答缓冲填充: 16字节pattern。DMA是环形, 长度=16=pattern长度 */
    for (i = 0; i < SPI_TX_PATTERN_LEN; i++)
    {
        g_spi_tx_buf[i] = g_spi_reply_pattern[i];
    }

    /* 1b. RX/影子缓冲清零并回写, 避免上电残留内容被误当成"收到的数据"(上电首次很关键) */
    memset((void *)g_spi_rx_buf,    0, sizeof(g_spi_rx_buf));
    memset((void *)g_spi_frame_buf, 0, sizeof(g_spi_frame_buf));
    memset((void *)g_print_buf,     0, sizeof(g_print_buf));

    SCB_CleanDCache_by_Addr((uint32_t *)g_spi_tx_buf, 32);
    SCB_CleanDCache_by_Addr((uint32_t *)g_spi_rx_buf, (int32_t)sizeof(g_spi_rx_buf));

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
    printf(" 说明: CR的bit10(MINC)与bit8(CIRC)应都为1; NDTR应等于缓冲大小\r\n");
#if (SPI_NSS_MODE == 0)
    printf("       CFG2的CPOL(bit0)/CPHA(bit1)应对应当前SPI模式; 硬件NSS下SSM(bit8)=0, SSI(bit12)应为1\r\n");
#else
    printf("       CFG2的CPOL(bit0)/CPHA(bit1)应对应当前SPI模式; CR1的SSI(bit12)应为0\r\n");
#endif
    printf("       TX的M0AR应指向g_spi_tx_buf, PAR应指向SPI2->TXDR(0x4000380C)\r\n\r\n");
}

/**
 * @brief   求 v 是 ref 的"循环左移多少位"的结果
 * @param   v   : 收到的字节
 * @param   ref : 参考字节(探针)
 * @retval  -1=不是ref的任何循环移位; 0=完全相同; 1~7=循环左移的位数
 */
static int8_t spi_rot_of(uint8_t v, uint8_t ref)
{
    uint8_t i;
    uint8_t r = ref;

    for (i = 0; i < 8U; i++)
    {
        if (r == v)
        {
            return (int8_t)i;
        }
        r = (uint8_t)((r << 1) | (r >> 7));     /* 循环左移1位 */
    }
    return -1;
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

    /* 1. 等SPI的RX FIFO彻底排空, 说明DMA已经把最后1个字节写进RAM了 */
    /*    只等 RXWNE(RX FIFO Word Not Empty) 是不够的: 它表示"有凑满的32bit字",
     *    DSIZE=8时最后1~3个字节可能凑不满一个字, 此时RXWNE已经是0但数据还在FIFO里。
     *    这里把 RXP(FIFO非空) / RXWNE / RXPLVL(打包余量) 一起判, 保证真的空了 */
    guard = 20000UL;
    while (((SPI2->SR & (SPI_SR_RXP | SPI_SR_RXWNE | SPI_SR_RXPLVL)) != 0UL) && (guard != 0UL))
    {
        guard--;
    }
    if (guard == 0UL)
    {
        g_spi_fifo_stuck++;                     /* 极端情况: FIFO一直不空, 计数便于定位 */
    }

    /* FIFO空了之后, DMA还要几个周期才把最后1个字节落到RAM, 稍等一下再取计数 */
    for (guard = 0UL; guard < 200UL; guard++)
    {
        __NOP();
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
    if (len > SPI_FRAME_MAX_LEN)
    {
        g_spi_toolong_cnt++;                    /* 超长帧(多半是NSS漏沿), 丢弃不打印    */
        return;
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
    uint32_t diag_toolong = 0;
    uint8_t  led_state = 0;

    sys_cache_enable();                  /* 打开L1-Cache(D-Cache强制透写) */
    HAL_Init();                          /* 初始化HAL库 */
    sys_stm32_clock_init(240, 2, 2, 4);  /* 设置系统时钟, 480MHz, APB1/2=120MHz */
    delay_init(480);                     /* 初始化延时功能 */
    usart_init(115200);                  /* 初始化串口1, 115200bps(打印用) */
    led_init();                          /* 初始化LED */

    printf("\r\n\r\n===== 正点原子 M100Z-M7 SPI2 Slave Demo (V1.4 环形DMA常驻+位对齐自诊断) =====\r\n");
    printf("系统时钟: 480MHz | SPI2内核时钟: PCLK1 = 120MHz\r\n");
    printf("从机SPI模式: SPI_DEMO_MODE=%d  (0=Mode0 1=Mode1 2=Mode2 3=Mode3)\r\n", (int)SPI_DEMO_MODE);
    printf("从机NSS模式: %s\r\n", (SPI_NSS_MODE == 0) ?
           "硬件NSS(PB12=AF5, 帧间忽略SCK)" : "软件NSS(SSI恒0, 旧行为)");
    printf("接线: NSS=PB12, SCK=PB13, MISO=PB14, MOSI=PB15, 必须共地\r\n");
    printf("支持Master SCK: 100kHz ~ 10MHz (MSB先发, 8bit)\r\n");
    printf("Master写: 收到数据后打印在串口1\r\n");

#if (SPI_TX_PATTERN_MODE == 0)
    printf("Master读: 每帧循环返回16字节(全 0xA5, 位对齐探针)\r\n");
    printf("建议测试1(验从机RX): spidev_test 发 8 个 0xA5\r\n");
    printf("                     从机会自动判读并打印\"位对齐正确\"或\"偏了N位\"\r\n");
    printf("建议测试2(验从机TX): 读回8字节, 应该正好是 A5 A5 A5 A5 A5 A5 A5 A5\r\n\r\n");
#else
    printf("Master读: 每帧返回16字节(30 31 32 ... 3F)\r\n\r\n");
#endif

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
            /* 关中断把整帧取走: 中断里会往 g_spi_frame_buf 里memcpy下一帧,
               不隔离的话主循环可能读到"半新半旧"的混合数据 */
            __disable_irq();
            len = g_spi_frame_len;
            g_spi_frame_ready = 0;
            if (len > SPI_FRAME_MAX_LEN)
            {
                len = 0;
            }
            if (len != 0U)
            {
                memcpy((void *)g_print_buf, (const void *)g_spi_frame_buf, len);
            }
            __enable_irq();

            if (len != 0U)
            {
                j = 0;
                for (i = 0; i < len; i++)
                {
                    g_hexline[j++] = g_hex_tab[(g_print_buf[i] >> 4) & 0x0F];
                    g_hexline[j++] = g_hex_tab[g_print_buf[i] & 0x0F];
                    g_hexline[j++] = ' ';

                    if ((i & 0x0F) == 0x0F)     /* 每16字节换行 */
                    {
                        g_hexline[j++] = '\r';
                        g_hexline[j++] = '\n';
                    }
                }
                g_hexline[j] = '\0';

                printf("[SPI2 Slave] 收到 %u 字节:\r\n%s\r\n", (unsigned)len, g_hexline);

                /* ---- 位对齐自诊断: 整帧字节完全相同时, 反推相对探针的循环移位量 ---- */
                {
                    uint8_t same = 1;
                    int8_t  rot;

                    for (i = 1; i < len; i++)
                    {
                        if (g_print_buf[i] != g_print_buf[0])
                        {
                            same = 0;
                            break;
                        }
                    }

                    if (same != 0)
                    {
                        rot = spi_rot_of(g_print_buf[0], SPI_PROBE_BYTE);

                        if (rot == 0)
                        {
                            printf("[对齐] 收到 %u 个 0x%02X = 探针 0x%02X => 从机采样位对齐正确\r\n\r\n",
                                   (unsigned)len, (unsigned)g_print_buf[0], (unsigned)SPI_PROBE_BYTE);
                        }
                        else if (rot > 0)
                        {
                            printf("[对齐] 收到 %u 个 0x%02X = 0x%02X 循环左移 %d 位 => 位对齐偏了 %d 位\r\n",
                                   (unsigned)len, (unsigned)g_print_buf[0], (unsigned)SPI_PROBE_BYTE,
                                   (int)rot, (int)rot);
                            printf("       把 SPI_DEMO_MODE 换一个值(依次试 1/2/3)重新编译烧录,\r\n");
                            printf("       Master端同步改成同一模式再测一遍\r\n\r\n");
                        }
                        else
                        {
                            printf("[对齐] 收到 0x%02X 不是 0x%02X 的任何循环移位 => 不是单纯位偏移\r\n",
                                   (unsigned)g_print_buf[0], (unsigned)SPI_PROBE_BYTE);
                            printf("       重点查: 共地、SCK频率是否过高、SCK线上有无过冲振铃\r\n\r\n");
                        }
                    }
                    else
                    {
                        printf("[对齐] 本帧字节不全相同, 跳过位偏移判读(Master发同一个字节时才有效)\r\n\r\n");
                    }
                }
            }
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
                (g_spi_err_cnt != diag_err) || (g_spi_fifo_stuck != diag_stuck) ||
                (g_spi_toolong_cnt != diag_toolong))
            {
                diag_irq     = g_spi_irq_cnt;
                diag_glit    = g_spi_glitch_cnt;
                diag_err     = g_spi_err_cnt;
                diag_stuck   = g_spi_fifo_stuck;
                diag_toolong = g_spi_toolong_cnt;

                printf("[诊断] NSS边沿=%lu  毛刺帧=%lu  超长帧=%lu  SPI错误=%lu  FIFO排空超时=%lu\r\n",
                       (unsigned long)diag_irq, (unsigned long)diag_glit,
                       (unsigned long)diag_toolong, (unsigned long)diag_err,
                       (unsigned long)diag_stuck);
            }
        }
    }
}
