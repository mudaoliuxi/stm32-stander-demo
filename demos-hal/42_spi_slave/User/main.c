/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V2.0
 * @date        2026-09-17
 * @brief       SPI2从机(Slave)通信实验 (每帧独立 + 收完即清空)
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
 *     与Master必须共地。
 *
 *  2. 支持Master的SCK时钟: 100kHz ~ 10MHz
 *
 *  3. Master写: NSS拉低期间发送的数据, 在NSS上升沿(一帧结束)后由串口1(115200)打印
 *
 *  4. Master读: 从机MISO每帧回发16字节 0xA5
 *
 * -------------------------------------------------------------------------------------------------
 * 【V2.0 核心: 一帧一清空, 不留任何残留】
 *
 *  把"NSS脉冲"当成唯一的生命周期单位, 一个脉冲 = 一帧 = 下面这一串动作:
 *
 *      NSS 上升沿 (一帧结束)
 *          |
 *          +-- 1. 等 SPI 的 RX FIFO 排空(保证最后一个字节已被DMA搬进RAM)
 *          +-- 2. 停掉 RX/TX 两路 DMA 流
 *          +-- 3. len = 缓冲大小 - NDTR   (Normal模式NDTR一路递减, 不用再算差值)
 *          +-- 4. 把本帧数据搬到打印缓冲
 *          +-- 5. 把 RX 缓冲整块清零          <==== 这就是"每次接收都清空"
 *          +-- 6. 重装 RX/TX 两路 DMA(NDTR复原, 地址复原)并重新使能
 *          |
 *          v
 *      下一帧从 缓冲[0] 开始, 干净的
 *
 *  为什么用 NORMAL 模式而不是 CIRCULAR:
 *      CIRCULAR 是一条"环形跑道", DMA写指针绕圈跑, 上一帧的数据还留在环上。
 *      一旦帧边界判断差一点(丢一个NSS边沿、或者最后一个字节还没搬完就读走),
 *      就会把上一帧的尾巴当成本帧的开头打出来 —— 现象就是"数据像是累计的、
 *      混着上一次的"。NORMAL 模式下每帧都从 缓冲[0] 重新开始写, 物理上不可能混。
 *
 *  时序上的安全性:
 *      "停DMA -> 取数 -> 清零 -> 重装" 全部发生在 NSS 为高(片选无效)期间。
 *      SPI Mode0 下空闲SCK为低, Master在片选无效时不会打时钟, 所以这段时间的
 *      任何操作都不会丢数据。整套动作在480MHz下约 2~3us, 而两帧间隔通常是毫秒级。
 * -------------------------------------------------------------------------------------------------
 * 【片选(NSS)的两条路】—— 由 SPI_NSS_MODE 选择
 *
 *  SPI_NSS_MODE = 0 (默认, 推荐): 硬件 NSS
 *      PB12 走 AF5 交给 SPI2 硬件, 由硬件判断"本帧是否有效"。
 *      片选无效(高)期间 SPI 硬件直接忽略 SCK —— 这是根治"整帧数据偏若干bit"的关键:
 *      偏bit的根源就是从机在片选无效期间也跟着SCK移位, 移位寄存器相位一漂移就回不来。
 *      PB12 同时保留 EXTI(上升沿切帧): EXTI 取自引脚输入缓冲, 与 MODER 无关,
 *      所以"硬件片选"和"上升沿切帧"可以并存。
 *
 *  SPI_NSS_MODE = 1: 软件 NSS (SSI 恒 0, 从机"永久选中")
 *      只在片选线很干净、帧间隙没有毛刺的场合可靠。保留用于对比/回退。
 *      注意: HAL 只在 (从机 && NSSPolarity==HIGH) 或 (主机 && LOW) 时置 SSI=1;
 *      本工程是从机+LOW, HAL 不动 SSI, 保持复位值 0 => 内部SS为低(低有效) => 选中。
 * -------------------------------------------------------------------------------------------------
 *
 * 注意事项:
 *  - 单帧最大长度 <= 512 字节(SPI_RX_BUF_SIZE), 超出部分会被丢弃并计入 g_spi_drop_cnt
 *  - Master两次传输之间建议NSS保持高电平 >= 50us
 *  - 应答数据刻意用 0xA5 而不是 0x30 0x31 0x32... : 递增斜坡被移位后还是递增斜坡,
 *    肉眼和逻辑分析仪都分辨不出来, 是诊断上最坏的测试数据。0xA5 的8种循环移位
 *    互不相同(A5/4B/96/2D/5A/B4/69/D2), 偏几位一眼就能看出来。
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

/* ---- 可配置项 ---- */

/* 从机SPI模式: 0=Mode0(CPOL0/CPHA0) 1=Mode1 2=Mode2 3=Mode3
   必须与Master(例如 /dev/spidev2.0 的 spi mode)一致, RK3576默认 Mode0 */
#define SPI_DEMO_MODE           0

/* 片选管理: 0=硬件NSS(PB12走AF5)  1=软件NSS(SSI恒0)
   ★ 默认1: 软件NSS是V1.4实测跑通的路径("之前接收是正确的"), 别默认开硬件NSS --
     那是V1.5的试验项, 上板未验证, 默认开着只会让问题更难定位。 */
/* V2.5: 0->1 退回软件NSS。
 * 理由: 实测一次16字节传输会被拆成 2+14 / 3+14 帧 => NSS线上有毛刺上升沿。
 * 硬件NSS模式下毛刺会复位SPI内部位计数器(固件无法拦截, 表现为双方向对称偏1位);
 * 软件NSS模式下SPI位计数器不受NSS引脚影响, 配合EXTI去抖可完全免疫毛刺。 */
#define SPI_NSS_MODE            1

/* Master读走的应答内容: 16个 0xA5
   [!] 若心跳里 UDR 每帧+1, 说明 Master 读的字节数 >= 从机 TX 准备的长度,
       TX FIFO 被读空 -> 欠载。数据本身仍正确(补发内容由 CFG1.UDRCFG 决定),
       想让 UDR 归零: 把下面长度改成 SPI_RX_BUF_SIZE, 并把 g_spi_tx_buf 同步改大。
       先用分类计数确认到底是不是 UDR, 别猜。 */
#define SPI_TX_REPLY_BYTE       0xA5
#define SPI_TX_PATTERN_LEN      16

/* RX缓冲大小 = 单帧长度上限。Normal模式下不需要是2的幂 */
#define SPI_RX_BUF_SIZE         512

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

/* 收发DMA缓冲区: 32字节对齐, 位于AXI SRAM(0x24000000, 见 User/SCRIPT/qspi_code.scf.scf
   的 RW_m_stmsram 段), DMA1可访问; 该区域可Cache, 由 sys_cache_enable() 打开D-Cache。
   注意: 绝对不能放进 DTCM(0x20000000) —— H7的DMA1/DMA2访问不了DTCM */
ALIGNED32 static uint8_t g_spi_rx_buf[SPI_RX_BUF_SIZE];
ALIGNED32 static uint8_t g_spi_tx_buf[32];      /* 只用前16字节, 留满1个Cache行便于整行操作 */

static uint8_t  g_spi_frame_buf[SPI_RX_BUF_SIZE];                        /* 帧缓冲(中断里拷贝) */
static uint8_t  g_print_buf[SPI_RX_BUF_SIZE];                            /* 打印副本(与中断隔离) */
static char     g_hexline[SPI_RX_BUF_SIZE * 3 + SPI_RX_BUF_SIZE / 8 + 16]; /* 十六进制行缓冲   */

volatile uint8_t  g_spi_frame_ready = 0;        /* 1 = 有新帧待打印                             */
volatile uint16_t g_spi_frame_len   = 0;        /* 新帧长度                                     */
volatile uint32_t g_spi_irq_cnt     = 0;        /* NSS上升沿(EXTI)次数                          */
volatile uint32_t g_spi_drop_cnt    = 0;        /* 空帧丢弃次数(多半是NSS线上的毛刺)            */
volatile uint32_t g_spi_err_cnt     = 0;        /* SPI OVR/UDR/FRE 错误总数                     */
volatile uint32_t g_spi_ovr_cnt     = 0;        /* RX FIFO 溢出 Overrun  (接收侧真故障, 该恒0)  */
volatile uint32_t g_spi_udr_cnt     = 0;        /* TX FIFO 欠载 Underrun (从机TX给少了会这样)   */
volatile uint32_t g_spi_fre_cnt     = 0;        /* 帧错误 TIFRE          (非TI模式应恒为0)      */
volatile uint32_t g_spi_nss_glitch_cnt = 0;     /* NSS上升沿毛刺次数(EXTI触发时引脚已是低电平)  */

static const char g_hex_tab[16] = "0123456789ABCDEF";

/******************************************************************************************/
/* 函数声明                                                                                */
/******************************************************************************************/

static void spi2_gpio_init(void);
static void spi2_dma_init(void);
static void spi2_init(void);
static void spi2_slave_start(void);
static void dma_stream_reload(DMA_Stream_TypeDef *s, uint32_t par, uint32_t m0ar, uint16_t ndtr);
static void spi_frame_close(void);
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

    /* SCK/MISO/MOSI: 复用推挽(AF5=SPI2)
     * V2.6: Speed VERY_HIGH -> LOW。Speed只决定输出驱动斜率(对输入脚无意义),
     * 从机唯一的输出脚是MISO。100kHz完全不需要高速边沿; 缓边沿可显著减少
     * MISO对SCK的串扰(实测: 仅当MISO翻转时双方向偏1位, MISO平线时全对)。 */
    gpio_init_struct.Pin       = SPI2_SCK_PIN | SPI2_MISO_PIN | SPI2_MOSI_PIN;
    gpio_init_struct.Mode      = GPIO_MODE_AF_PP;
    gpio_init_struct.Pull      = GPIO_NOPULL;
    gpio_init_struct.Speed     = GPIO_SPEED_FREQ_LOW;
    gpio_init_struct.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(SPI2_GPIO_PORT, &gpio_init_struct);

    /* NSS(PB12) 要同时干两件事:
     *   (a) 给SPI硬件当片选输入(AF5) —— 片选无效期间SPI硬件直接忽略SCK (SPI_NSS_MODE==0)
     *   (b) 帧边界检测 —— 上升沿走EXTI中断切帧
     * 先按"EXTI输入"配好(这一步顺带把EXTI和NVIC都配好), 之后再把MODER叠成AF5。 */
    gpio_init_struct.Pin       = SPI2_NSS_PIN;
    gpio_init_struct.Mode      = GPIO_MODE_IT_RISING;
    gpio_init_struct.Pull      = GPIO_PULLUP;   /* 总线空闲时保持高电平(未选中)                 */
    gpio_init_struct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init_struct.Alternate = 0;
    HAL_GPIO_Init(SPI2_GPIO_PORT, &gpio_init_struct);

#if (SPI_NSS_MODE == 0)
    /* 同一个PB12再叠一层AF5: 交给SPI2硬件当NSS输入(MODER=10, AFR=5)。
     * EXTI的边沿检测取自引脚输入缓冲, 与MODER无关, 所以两者可以并存。
     * PULLUP保留: 总线空闲时NSS保持高(未选中)。                                */
    MODIFY_REG(GPIOB->MODER, GPIO_MODER_MODE12, GPIO_MODER_MODE12_1);
    MODIFY_REG(GPIOB->AFR[1], (0xFUL << 16U), ((uint32_t)GPIO_AF5_SPI2 << 16U));
#endif

    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);             /* 最高抢占优先级               */
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

/**
 * @brief   SPI2的DMA初始化(RX=DMA1_Stream0, TX=DMA1_Stream1, 都是NORMAL模式)
 * @note    不使用DMA中断: 帧边界完全由NSS(PB12)EXTI判定;
 *          DMA的NDTR/地址在 dma_stream_reload() 里每帧重装
 * @param   无
 * @retval  无
 */
static void spi2_dma_init(void)
{
    __HAL_RCC_DMA1_CLK_ENABLE();                /* 使能DMA1时钟                                 */

    /* ---- SPI2 RX DMA : DMA1_Stream0, 外设->内存, NORMAL模式 ---- */
    g_spi2_rx_dma.Instance                   = DMA1_Stream0;
    g_spi2_rx_dma.Init.Request               = DMA_REQUEST_SPI2_RX;
    g_spi2_rx_dma.Init.Direction             = DMA_PERIPH_TO_MEMORY;
    g_spi2_rx_dma.Init.PeriphInc             = DMA_PINC_DISABLE;
    g_spi2_rx_dma.Init.MemInc                = DMA_MINC_ENABLE;
    g_spi2_rx_dma.Init.PeriphDataAlignment   = DMA_PDATAALIGN_BYTE;
    g_spi2_rx_dma.Init.MemDataAlignment      = DMA_MDATAALIGN_BYTE;
    g_spi2_rx_dma.Init.Mode                  = DMA_NORMAL;      /* 关键: 不是CIRCULAR!      */
    g_spi2_rx_dma.Init.Priority              = DMA_PRIORITY_VERY_HIGH;
    g_spi2_rx_dma.Init.FIFOMode              = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&g_spi2_rx_dma);

    g_spi2_rx_dma.XferCpltCallback     = NULL;  /* 不用回调, 也就不会开DMA中断                  */
    g_spi2_rx_dma.XferHalfCpltCallback = NULL;
    g_spi2_rx_dma.XferErrorCallback    = NULL;
    g_spi2_rx_dma.XferAbortCallback    = NULL;

    __HAL_LINKDMA(&g_spi2_handle, hdmarx, g_spi2_rx_dma);

    /* ---- SPI2 TX DMA : DMA1_Stream1, 内存->外设, NORMAL模式 ---- */
    g_spi2_tx_dma.Instance                   = DMA1_Stream1;
    g_spi2_tx_dma.Init.Request               = DMA_REQUEST_SPI2_TX;
    g_spi2_tx_dma.Init.Direction             = DMA_MEMORY_TO_PERIPH;
    g_spi2_tx_dma.Init.PeriphInc             = DMA_PINC_DISABLE;
    g_spi2_tx_dma.Init.MemInc                = DMA_MINC_ENABLE;
    g_spi2_tx_dma.Init.PeriphDataAlignment   = DMA_PDATAALIGN_BYTE;
    g_spi2_tx_dma.Init.MemDataAlignment      = DMA_MDATAALIGN_BYTE;
    g_spi2_tx_dma.Init.Mode                  = DMA_NORMAL;
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
 * @brief   SPI2初始化(从机模式)
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
    g_spi2_handle.Init.NSSPolarity                = SPI_NSS_POLARITY_LOW;    /* 片选低有效       */
    g_spi2_handle.Init.FifoThreshold              = SPI_FIFO_THRESHOLD_01DATA; /* 每1字节搬运    */
    g_spi2_handle.Init.MasterKeepIOState          = SPI_MASTER_KEEP_IO_STATE_DISABLE;
    HAL_SPI_Init(&g_spi2_handle);

#if (SPI_NSS_MODE != 0)
    /* 软件NSS: SSI的值就是"内部片选电平"。NSSPolarity=LOW(低有效), 所以
       SSI=0 => 内部片选为低 => 选中。HAL在这种组合下不会动SSI(复位值0), 
       这里显式写一次, 表达"从机常选中"的意图, 同时兼容HAL版本差异。          */
    CLEAR_BIT(g_spi2_handle.Instance->CR1, SPI_CR1_SSI);
#endif

    /* 关闭SPI自身的错误中断(本工程不用HAL的SPI状态机, 出错靠轮询SR诊断) */
    WRITE_REG(g_spi2_handle.Instance->IER, 0);
}

/**
 * @brief   DMA流重装: 停流 -> 改地址/计数 -> 清标志 -> 重新使能
 * @note    NORMAL模式下每帧都要调一次。关键点有两个:
 *            1. NDTR/M0AR/PAR 只能在 EN=0 时写, 所以必须先停流, 并等 EN 真的被
 *               硬件清0(H7要等几个时钟周期), 否则写入无效 —— 这正是"帧长累加"
 *               类Bug的根源(HAL的 HAL_SPI_DMAStop 在H7上是空壳, 别指望它)
 *            2. 全程不碰 SPI 的 SPE: SPI 使能一次就永不关闭
 * @param   s    : 目标DMA流
 * @param   par  : 外设地址
 * @param   m0ar : 内存地址
 * @param   ndtr : 传输长度
 * @retval  无
 */
static void dma_stream_reload(DMA_Stream_TypeDef *s, uint32_t par, uint32_t m0ar, uint16_t ndtr)
{
    uint32_t guard;

    CLEAR_BIT(s->CR, DMA_SxCR_EN);                      /* 1. 停流                      */

    /* 2. 等硬件真的清0 —— ★ 必须带超时。万一流被外设请求异常挂住、EN位清不掉,
     *    原来的空转死等会让程序在这里永久卡死(串口从此静默)。
     *    超时后强行继续: 最坏这一帧数据不干净, 但板子还活着, 还能给你报错。   */
    guard = 200000UL;
    while (((s->CR & DMA_SxCR_EN) != 0UL) && (guard != 0UL))
    {
        guard--;
    }

    s->PAR  = par;                                      /* 3. EN=0时才允许改地址/计数   */
    s->M0AR = m0ar;
    s->NDTR = ndtr;

    if (s == DMA1_Stream0)                              /* 4. 清干净本流的全部标志      */
    {
        DMA1->LIFCR = 0x0000003DUL;
    }
    else
    {
        DMA1->LIFCR = 0x00000F40UL;
    }

    SET_BIT(s->CR, DMA_SxCR_EN);                        /* 5. 重新使能                  */
}

/**
 * @brief   启动SPI2从机: 武装收发DMA + 使能SPI(此后一直不停)
 * @param   无
 * @retval  无
 */
static void spi2_slave_start(void)
{
    uint16_t i;

    /* 1. TX应答缓冲填充: 16字节 0xA5 */
    for (i = 0; i < SPI_TX_PATTERN_LEN; i++)
    {
        g_spi_tx_buf[i] = SPI_TX_REPLY_BYTE;
    }
    for (i = SPI_TX_PATTERN_LEN; i < 32U; i++)
    {
        g_spi_tx_buf[i] = 0x00;
    }
    SCB_CleanDCache_by_Addr((uint32_t *)g_spi_tx_buf, 32);

    /* 2. RX/帧/打印缓冲全部清零 —— 保证上电第一帧就是干净的 */
    memset((void *)g_spi_rx_buf,    0, sizeof(g_spi_rx_buf));
    memset((void *)g_spi_frame_buf, 0, sizeof(g_spi_frame_buf));
    memset((void *)g_print_buf,     0, sizeof(g_print_buf));
    SCB_CleanDCache_by_Addr((uint32_t *)g_spi_rx_buf, (int32_t)sizeof(g_spi_rx_buf));

    /* 3. 打开SPI的DMA请求, 再使能SPI。之后SPI一直常驻, 永不再关 */
    SET_BIT(SPI2->CFG1, SPI_CFG1_RXDMAEN | SPI_CFG1_TXDMAEN);
    __HAL_SPI_ENABLE(&g_spi2_handle);

    /* 4. 最后武装两路DMA, 静静等第一帧到来
    *    ★ 注意参数顺序: PAR=外设寄存器, M0AR=内存缓冲。
    *      V2.2曾把TX这路的两个参数写反(PAR=buf,M0AR=TXDR), 结果DMA从TXDR读数
    *      (恒0)写进FIFO -> MISO输出全0、UDR不置位, 极难察觉。 */
    dma_stream_reload(DMA1_Stream0, (uint32_t)&SPI2->RXDR, (uint32_t)g_spi_rx_buf, SPI_RX_BUF_SIZE);
    dma_stream_reload(DMA1_Stream1, (uint32_t)&SPI2->TXDR, (uint32_t)g_spi_tx_buf, SPI_TX_PATTERN_LEN);
}

/**
 * @brief   一帧结束(在NSS上升沿的EXTI中断里调用)
 * @note    顺序很重要: 先停DMA再取数, 取完立刻清零+重装, 全程在片选无效期间完成
 * @param   无
 * @retval  无
 */
static void spi_frame_close(void)
{
    uint16_t len;
    uint32_t guard;

    g_spi_irq_cnt++;

    /* 1. 等SPI的RX FIFO彻底排空, 说明DMA已经把最后1个字节写进RAM了。
     *    注意不能只看 RXWNE(RX FIFO Word Not Empty): DSIZE=8时最后1~3个字节
     *    凑不满一个32bit word, 此时RXWNE已经是0但数据还在FIFO里。
     *    所以 RXP(FIFO非空) / RXWNE / RXPLVL(打包余量) 一起判。                  */
    guard = 20000UL;
    while (((SPI2->SR & (SPI_SR_RXP | SPI_SR_RXWNE | SPI_SR_RXPLVL)) != 0UL) && (guard != 0UL))
    {
        guard--;
    }

    /* 2. 停掉两路DMA流 (只清EN位, 绝不碰SPI的SPE) */
    CLEAR_BIT(DMA1_Stream0->CR, DMA_SxCR_EN);
    CLEAR_BIT(DMA1_Stream1->CR, DMA_SxCR_EN);
    guard = 200000UL;                                   /* ★ 必须带超时: 这里在最高 */
    while (((DMA1_Stream0->CR & DMA_SxCR_EN) != 0UL) && (guard != 0UL)) { guard--; }
    guard = 200000UL;                                   /*   优先级中断里, 裸死等   */
    while (((DMA1_Stream1->CR & DMA_SxCR_EN) != 0UL) && (guard != 0UL)) { guard--; }

    /* 3. 算本帧长度: NORMAL模式下NDTR从 SIZE 一路递减, 收到几个字节就减几 */
    len = (uint16_t)(SPI_RX_BUF_SIZE - (uint16_t)DMA1_Stream0->NDTR);

    /* 4. SPI错误统计 —— 分类计数, 一眼看出是哪种。
     *    OVR: RX FIFO 满了但没被DMA及时搬走 (接收侧真故障, 该恒为0)
     *    UDR: 从机被选中要发数据时 TX FIFO 是空的。从机 TX 只准备了固定长度,
     *         而 Master 读得比它多, 就会 UDR —— 属"正常但要知道"的现象,
     *         补发什么由 CFG1.UDRCFG 决定; 想让它归零就得把 TX 长度给满。
     *    FRE: TI 模式专用, 本工程恒为 0。                                    */
    {
        uint32_t sr = SPI2->SR;

        if ((sr & SPI_SR_OVR)   != 0UL) { g_spi_ovr_cnt++; }
        if ((sr & SPI_SR_UDR)   != 0UL) { g_spi_udr_cnt++; }
        if ((sr & SPI_SR_TIFRE) != 0UL) { g_spi_fre_cnt++; }

        if ((sr & (SPI_SR_OVR | SPI_SR_UDR | SPI_SR_TIFRE)) != 0UL)
        {
            g_spi_err_cnt++;
            __HAL_SPI_CLEAR_OVRFLAG(&g_spi2_handle);
            __HAL_SPI_CLEAR_UDRFLAG(&g_spi2_handle);
            __HAL_SPI_CLEAR_FREFLAG(&g_spi2_handle);
        }
    }

    /* 5. 取本帧数据(DMA写的是AXI SRAM, CPU读前必须先失效Cache) */
    if (len != 0U)
    {
        SCB_InvalidateDCache_by_Addr((uint32_t *)g_spi_rx_buf, (int32_t)sizeof(g_spi_rx_buf));
        memcpy((void *)g_spi_frame_buf, (const void *)g_spi_rx_buf, len);
    }
    else
    {
        g_spi_drop_cnt++;                   /* 空帧: 多半是NSS线上的毛刺, 直接忽略   */
    }

    /* 6. ★ 把RX缓冲整块清零 —— "每次接收都清空"就在这里 */
    memset((void *)g_spi_rx_buf, 0, sizeof(g_spi_rx_buf));
    SCB_CleanDCache_by_Addr((uint32_t *)g_spi_rx_buf, (int32_t)sizeof(g_spi_rx_buf));

    /* 7. 重装两路DMA: 下一帧从 缓冲[0] 重新开始收 */
    dma_stream_reload(DMA1_Stream0, (uint32_t)&SPI2->RXDR, (uint32_t)g_spi_rx_buf, SPI_RX_BUF_SIZE);
    dma_stream_reload(DMA1_Stream1, (uint32_t)&SPI2->TXDR, (uint32_t)g_spi_tx_buf, SPI_TX_PATTERN_LEN);

    /* 8. 通知主循环打印(慢速的串口输出不放在中断里做) */
    if (len != 0U)
    {
        g_spi_frame_len   = len;
        g_spi_frame_ready = 1;
    }
}

/**
 * @brief   EXTI回调: NSS(PB12)上升沿 => 一帧结束
 * @param   GPIO_Pin : 触发的引脚
 * @retval  无
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == SPI2_NSS_PIN)
    {
        /* V2.5 去抖: 上升沿触发时读引脚实际电平, 若已回落为低则是毛刺, 忽略。
         * 真帧结束: NSS 持续为高, 读到 SET, 正常切帧。 */
        if (HAL_GPIO_ReadPin(SPI2_GPIO_PORT, SPI2_NSS_PIN) == GPIO_PIN_RESET)
        {
            g_spi_nss_glitch_cnt++;
            return;
        }
        spi_frame_close();
    }
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
    printf(" DMA_S0: CR=%08lX NDTR=%lu M0AR=%08lX (RX, NORMAL %u字节)\r\n",
           (unsigned long)DMA1_Stream0->CR, (unsigned long)DMA1_Stream0->NDTR,
           (unsigned long)DMA1_Stream0->M0AR, (unsigned)SPI_RX_BUF_SIZE);
    printf(" DMA_S1: CR=%08lX NDTR=%lu M0AR=%08lX (TX, NORMAL %u字节)\r\n",
           (unsigned long)DMA1_Stream1->CR, (unsigned long)DMA1_Stream1->NDTR,
           (unsigned long)DMA1_Stream1->M0AR, (unsigned)SPI_TX_PATTERN_LEN);
    printf(" 说明: CR的bit0(EN)应为1, bit8(CIRC)应为0; NDTR应为缓冲大小;\r\n");
    printf("       M0AR应分别指向 g_spi_rx_buf / g_spi_tx_buf\r\n");
#if (SPI_NSS_MODE == 0)
    printf("       硬件NSS: CFG2的SSM(bit8)=0, 片选由PB12引脚决定\r\n\r\n");
#else
    printf("       软件NSS: CR1的SSI(bit12)=0 => 内部片选为低 => 从机常选中\r\n\r\n");
#endif
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
    uint32_t diag_ok   = 0;
    uint8_t  led_state = 0;

    sys_cache_enable();                  /* 打开L1-Cache(D-Cache强制透写) */
    HAL_Init();                          /* 初始化HAL库 */
    sys_stm32_clock_init(240, 2, 2, 4);  /* 设置系统时钟, 480MHz, APB1/2=120MHz */
    delay_init(480);                     /* 初始化延时功能 */
    usart_init(115200);                  /* 初始化串口1, 115200bps(打印用) */
    led_init();                          /* 初始化LED */

    printf("\r\n\r\n===== 正点原子 M100Z-M7 SPI2 Slave Demo (V2.6 MISO边沿减速) =====\r\n");
    printf("系统时钟: 480MHz | SPI2内核时钟: PCLK1 = 120MHz\r\n");
    printf("接线: NSS=PB12, SCK=PB13, MISO=PB14, MOSI=PB15, 必须共地\r\n");
    printf("从机SPI模式: SPI_DEMO_MODE=%d (0=Mode0 1=Mode1 2=Mode2 3=Mode3), 须与Master一致\r\n",
           (int)SPI_DEMO_MODE);
    printf("片选方式: %s\r\n", (SPI_NSS_MODE == 0) ?
           "硬件NSS (PB12走AF5, 片选无效时SPI忽略SCK)" : "软件NSS (SSI恒0, 从机常选中)");
    printf("支持Master SCK: 100kHz ~ 10MHz (MSB先发, 8bit)\r\n");
    printf("Master写: 每帧(NSS脉冲)收到的数据单独打印, 帧与帧之间不累计\r\n");
    printf("Master读: 每帧回发16字节 0x%02X\r\n\r\n", (unsigned)SPI_TX_REPLY_BYTE);

    spi2_gpio_init();                    /* SPI2引脚 + NSS帧边界EXTI */
    spi2_dma_init();                     /* SPI2收发DMA (NORMAL模式) */
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
            if (len > SPI_RX_BUF_SIZE)
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
                diag_ok++;
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

                printf("[SPI2 Slave] 本帧 %u 字节:\r\n%s\r\n\r\n", (unsigned)len, g_hexline);
            }
        }

        /* LED0心跳翻转, 500ms一次, 指示程序在运行 */
        if ((HAL_GetTick() - tick_led) >= 500)
        {
            tick_led = HAL_GetTick();
            led_state = !led_state;
            LED0(led_state);
        }

        /* ★ 心跳: 每2秒无条件打一行 —— 排查"串口没反应"的第一判据。
         *   有这行 => 程序在跑、串口通, 问题只在SPI数据通路上;
         *   没这行 => 板子根本没跑起来(烧录/复位/串口线), 跟SPI逻辑无关。
         *   别删。                                                            */
        if ((HAL_GetTick() - tick_diag) >= 2000)
        {
            tick_diag = HAL_GetTick();

            printf("[心跳] %lu s | NSS=%lu 帧=%lu 空=%lu 毛刺=%lu | 错 OVR=%lu UDR=%lu FRE=%lu | SPI_EN=%d\r\n",
                   (unsigned long)(HAL_GetTick() / 1000UL), (unsigned long)g_spi_irq_cnt,
                   (unsigned long)diag_ok, (unsigned long)g_spi_drop_cnt,
                   (unsigned long)g_spi_nss_glitch_cnt,
                   (unsigned long)g_spi_ovr_cnt, (unsigned long)g_spi_udr_cnt,
                   (unsigned long)g_spi_fre_cnt,
                   ((SPI2->CR1 & SPI_CR1_SPE) != 0UL) ? 1 : 0);
        }
    }
}
