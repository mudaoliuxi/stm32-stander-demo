/**
 ****************************************************************************************************
 * @file        dac.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-04-25
 * @brief       DAC驱动代码
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

#include "./BSP/DAC/dac.h"
#include "./SYSTEM/delay/delay.h"

DMA_HandleTypeDef g_dma_dac_handle;     /* 定义要搬运DAC数据的DMA句柄 */
DAC_HandleTypeDef g_dac_dma_handle;     /* 定义DAC（DMA输出）句柄 */
extern uint16_t g_dac_sin_buf[4096];    /* 发送数据缓冲区 */

/**
 * @brief       初始化DAC输出波形
 * @note        DAC的输入时钟来自APB1, 时钟频率=120Mhz=8.3ns
 *              DAC在输出buffer关闭的时候, 输出建立时间: tSETTLING = 2us (H750数据手册有写)
 *              因此DAC输出的最高速度约为:500Khz, 以10个点为一个周期, 最大能输出50Khz左右的波形
 *
 * @retval      无
 */
void dac_dma_wave_init(uint8_t outx)
{
    DAC_ChannelConfTypeDef dac_ch_conf={0};
    GPIO_InitTypeDef gpio_init_struct;
    
    /* 使能时钟 */
    __HAL_RCC_GPIOA_CLK_ENABLE();                                           /* DAC通道引脚端口时钟使能 */
    __HAL_RCC_DAC12_CLK_ENABLE();                                           /* DAC外设时钟使能 */
    __HAL_RCC_DMA2_CLK_ENABLE();                                            /* DMA时钟使能 */
    
    /* 配置DAC输出引脚 */
    gpio_init_struct.Pin = (outx == 1) ? GPIO_PIN_4 : GPIO_PIN_5;           /* DAC输出引脚 */
    gpio_init_struct.Mode = GPIO_MODE_ANALOG;                               /* 模拟模式 */
    HAL_GPIO_Init(GPIOA, &gpio_init_struct);                                /* 配置DAC输出引脚 */
    
    /* 配置DMA */
    g_dma_dac_handle.Instance = DMA2_Stream6;                               /* 使用的DAM2 Stream6 */
    g_dma_dac_handle.Init.Request = DMA_REQUEST_DAC1_CH1;                   /* DAC触发DMA传输 */
    g_dma_dac_handle.Init.Direction = DMA_MEMORY_TO_PERIPH;                 /* 存储器到外设模式 */
    g_dma_dac_handle.Init.PeriphInc = DMA_PINC_DISABLE;                     /* 外设地址禁止自增 */
    g_dma_dac_handle.Init.MemInc = DMA_MINC_ENABLE;                         /* 存储器地址自增 */
    g_dma_dac_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;    /* 外设数据长度:16位 */
    g_dma_dac_handle.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;       /* 存储器数据长度:16位 */
    g_dma_dac_handle.Init.Mode = DMA_CIRCULAR;                              /* 循环模式 */
    g_dma_dac_handle.Init.Priority = DMA_PRIORITY_MEDIUM;                   /* 中等优先级 */
    g_dma_dac_handle.Init.FIFOMode = DMA_FIFOMODE_DISABLE;                  /* 不使用FIFO */
    HAL_DMA_Init(&g_dma_dac_handle);                                        /* 初始化DMA */
    __HAL_LINKDMA(&g_dac_dma_handle, DMA_Handle1, g_dma_dac_handle);        /* DMA句柄与DAC句柄关联 */
    
    /* 配置DAC */
    g_dac_dma_handle.Instance = DAC1;                                       /* 选择哪个DAC */
    HAL_DAC_Init(&g_dac_dma_handle);                                        /* DAC初始化 */
    dac_ch_conf.DAC_SampleAndHold = DAC_SAMPLEANDHOLD_DISABLE;              /* 关闭采样保持模式，这个模式主要用于低功耗 */
    dac_ch_conf.DAC_Trigger = DAC_TRIGGER_T7_TRGO;                          /* 采用定时器7触发 */
    dac_ch_conf.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;                 /* 使能输出缓冲 */
    dac_ch_conf.DAC_ConnectOnChipPeripheral = DAC_CHIPCONNECT_DISABLE;      /* 不将DAC连接到片上外设 */
    dac_ch_conf.DAC_UserTrimming = DAC_TRIMMING_FACTORY;                    /* 使用出厂校准 */
    HAL_DAC_ConfigChannel(&g_dac_dma_handle, &dac_ch_conf, DAC_CHANNEL_1);  /* DAC通道输出配置 */
}

/**
 * @brief       使能DAC输出波形
 * @note        TIM7的输入时钟频率(f)来自APB1, f = 120M * 2 = 240Mhz.
 *              DAC触发频率 ftrgo = f / ((psc + 1) * (arr + 1))
 *              波形频率 = ftrgo / ndtr;
 *
 * @param       ndtr        : DMA通道单次传输数据量
 * @param       arr         : TIM7的自动重装载值
 * @param       psc         : TIM7的分频系数
 * @retval      无
 */
void dac_dma_wave_enable(uint16_t ndtr, uint16_t arr, uint16_t psc)
{
    TIM_HandleTypeDef tim7_handle = {0};
    TIM_MasterConfigTypeDef master_config = {0};
    
    /* 使能时钟 */
    __HAL_RCC_TIM7_CLK_ENABLE();                                                                            /* TIM7时钟使能 */
    
    /* 配置TIM7 */
    tim7_handle.Instance = TIM7;                                                                            /* 选择定时器7 */
    tim7_handle.Init.Prescaler = psc;                                                                       /* 分频系数 */
    tim7_handle.Init.CounterMode = TIM_COUNTERMODE_UP;                                                      /* 递增计数 */
    tim7_handle.Init.Period = arr;                                                                          /* 重装载值 */
    tim7_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;                                     /* 自动重装 */
    HAL_TIM_Base_Init(&tim7_handle);                                                                        /* 初始化定时器7 */
    master_config.MasterOutputTrigger = TIM_TRGO_UPDATE;                                                    /* 定时器更新事件用于触发 */
    master_config.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;                                            /* 禁止定时器主从模式 */
    HAL_TIMEx_MasterConfigSynchronization(&tim7_handle, &master_config);                                    /* 配置TIM7 TRGO */
    HAL_TIM_Base_Start(&tim7_handle);                                                                       /* 使能定时器7 */
    
    /* 配置DAC和DMA */
    HAL_DAC_Stop_DMA(&g_dac_dma_handle, DAC_CHANNEL_1);                                                     /* 先停止之前的传输 */
    HAL_DAC_Start_DMA(&g_dac_dma_handle, DAC_CHANNEL_1, (uint32_t *)g_dac_sin_buf, ndtr, DAC_ALIGN_12B_R);  /* 开启传输 */
}
