/**
 ****************************************************************************************************
 * @file        adc.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-04-25
 * @brief       ADC驱动代码
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

#include "./BSP/ADC/adc.h"
#include "./SYSTEM/delay/delay.h"

DMA_HandleTypeDef g_dma_nch_adc_handle;     /* 定义要搬运ADC多通道数据的DMA句柄 */
ADC_HandleTypeDef g_adc_nch_dma_handle;     /* 定义ADC（多通道DMA读取）句柄 */
uint8_t g_adc_dma_sta = 0;                  /* DMA传输状态标志, 0,未完成; 1, 已完成 */

/**
 * @brief       初始化ADC多通道和DMA
 * @note        由于本函数用到了6个通道, 宏定义会比较多内容, 因此,本函数就不采用宏定义的方式来修改通道了,
 *              直接在本函数里面修改, 这里我们默认使用PA0~PA5这6个通道.
 *              注意: 本函数还是使用 ADC_ADCX(默认=ADC1) 和 ADC_ADCX_DMASx(默认=DMA1_Stream7) 及其相关定义
 *              不要乱修改adc.h里面的这两部分内容, 必须在理解原理的基础上进行修改, 否则可能导致无法正常使用.
 * @param       par: 外设地址
 * @param       mar: 存储器地址 
 * @retval      无
 */
void adc_nch_dma_init(uint32_t par, uint32_t mar)
{
    GPIO_InitTypeDef gpio_init_struct;
    ADC_ChannelConfTypeDef adc_ch_conf = {0};
    
    /* 使能时钟 */
    __HAL_RCC_GPIOA_CLK_ENABLE();                                                           /* 开启GPIOA引脚时钟 */
    ADC_ADCX_CHY_CLK_ENABLE();                                                              /* 使能ADC1/2时钟 */
    
    if ((uint32_t)ADC_ADCX_DMASx > (uint32_t)DMA2)                                          /* 得到当前stream是属于DMA2还是DMA1 */
    {
        __HAL_RCC_DMA2_CLK_ENABLE();                                                        /* DMA2时钟使能 */
    }
    else
    {
        __HAL_RCC_DMA1_CLK_ENABLE();                                                        /* DMA1时钟使能 */
    }
    
    __HAL_RCC_ADC_CONFIG(RCC_ADCCLKSOURCE_CLKP);                                            /* ADC外设时钟选择 */
    
    /* 配置ADC1通道1/2/3/4/5/6/7输入引脚 */ 
    gpio_init_struct.Pin = GPIO_PIN_0 |                                                     /* ADC1通道0/1/2/3/4/5输入引脚 */
                           GPIO_PIN_1 |
                           GPIO_PIN_2 |
                           GPIO_PIN_3 |
                           GPIO_PIN_4 |
                           GPIO_PIN_5;
    gpio_init_struct.Mode = GPIO_MODE_ANALOG;                                               /* 模拟模式 */
    HAL_GPIO_Init(GPIOA, &gpio_init_struct);                                                /* 配置ADC1通道0/1/2/3/4/5输入引脚 */
    
    /* 配置DMA */
    g_dma_nch_adc_handle.Instance = ADC_ADCX_DMASx;                                         /* 使用DMA1 Stream7 */
    g_dma_nch_adc_handle.Init.Request = ADC_ADCX_DMASx_REQ;                                 /* 请求选择DMA_REQUEST_ADC1 */
    g_dma_nch_adc_handle.Init.Direction = DMA_PERIPH_TO_MEMORY;                             /* 传外设到存储器模式 */
    g_dma_nch_adc_handle.Init.PeriphInc = DMA_PINC_DISABLE;                                 /* 外设非增量模式 */
    g_dma_nch_adc_handle.Init.MemInc = DMA_MINC_ENABLE;                                     /* 存储器增量模式 */
    g_dma_nch_adc_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;                /* 外设数据长度:16位 */
    g_dma_nch_adc_handle.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;                   /* 存储器数据长度:16位 */
    g_dma_nch_adc_handle.Init.Mode = DMA_NORMAL;                                            /* 非循环模式（即使用普通模式） */
    g_dma_nch_adc_handle.Init.Priority = DMA_PRIORITY_MEDIUM;                               /* 中等优先级 */
    g_dma_nch_adc_handle.Init.FIFOMode = DMA_FIFOMODE_DISABLE;                              /* 禁止FIFO*/
    HAL_DMA_Init(&g_dma_nch_adc_handle);                                                    /* 初始化DMA外设 */
    
    __HAL_LINKDMA(&g_adc_nch_dma_handle, DMA_Handle, g_dma_nch_adc_handle);                 /* 将DMA与adc联系起来 */
    
    /* 配置ADC */
    g_adc_nch_dma_handle.Instance = ADC_ADCX;                                               /* 选择哪个ADC */
    g_adc_nch_dma_handle.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV2;                        /* 输入时钟2分频,即adc_ker_ck=per_ck/2=32Mhz */
    g_adc_nch_dma_handle.Init.Resolution = ADC_RESOLUTION_16B;                              /* 16位模式  */
    g_adc_nch_dma_handle.Init.ScanConvMode = ADC_SCAN_ENABLE;                               /* 扫描模式 */
    g_adc_nch_dma_handle.Init.EOCSelection = ADC_EOC_SINGLE_CONV;                           /* 关闭EOC中断 */
    g_adc_nch_dma_handle.Init.LowPowerAutoWait = DISABLE;                                   /* 自动低功耗关闭 */
    g_adc_nch_dma_handle.Init.ContinuousConvMode = ENABLE;                                  /* 使能连续转换模式 */
    g_adc_nch_dma_handle.Init.NbrOfConversion = 6;                                          /* 赋值范围是1~16，本实验用到6个通道 */
    g_adc_nch_dma_handle.Init.DiscontinuousConvMode = DISABLE;                              /* 禁止常规转换组不连续采样模式 */
    g_adc_nch_dma_handle.Init.NbrOfDiscConversion = 0;                                      /* 配置不连续采样模式的通道数，禁止常规转换组不连续采样模式后，此参数忽略 */
    g_adc_nch_dma_handle.Init.ExternalTrigConv = ADC_SOFTWARE_START;                        /* 采用软件触发 */
    g_adc_nch_dma_handle.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;         /* 采用软件触发的话，此位忽略 */
    g_adc_nch_dma_handle.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DMA_ONESHOT;    /* DMA单次传输ADC数据 */
    g_adc_nch_dma_handle.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;                           /* 有新的数据后直接覆盖掉旧数据 */
    g_adc_nch_dma_handle.Init.LeftBitShift = ADC_LEFTBITSHIFT_NONE;                         /* 设置ADC转换结果的左移位数 */
    g_adc_nch_dma_handle.Init.OversamplingMode = DISABLE;                                   /* 过采样关闭 */
    HAL_ADC_Init(&g_adc_nch_dma_handle);                                                    /* 初始化 */
    
    HAL_ADCEx_Calibration_Start(&g_adc_nch_dma_handle, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED); /* ADC校准 */
    
    /* 配置ADC通道 */
    adc_ch_conf.Channel = ADC_CHANNEL_14;                                                   /* 配置使用的ADC通道 */
    adc_ch_conf.Rank = ADC_REGULAR_RANK_1;                                                  /* 采样序列里的第1个 */
    adc_ch_conf.SamplingTime = ADC_SAMPLETIME_810CYCLES_5;                                  /* 采样周期为810.5个时钟周期 */
    adc_ch_conf.SingleDiff = ADC_SINGLE_ENDED ;                                             /* 单端输入 */
    adc_ch_conf.OffsetNumber = ADC_OFFSET_NONE;                                             /* 无偏移 */
    adc_ch_conf.Offset = 0;                                                                 /* 无偏移的情况下，此参数忽略 */
    adc_ch_conf.OffsetRightShift = DISABLE;                                                 /* 禁止右移 */
    adc_ch_conf.OffsetSignedSaturation = DISABLE;                                           /* 禁止有符号饱和 */
    HAL_ADC_ConfigChannel(&g_adc_nch_dma_handle, &adc_ch_conf);                             /* 配置ADC通道 */
    
    adc_ch_conf.Channel = ADC_CHANNEL_15;                                                   /* 配置使用的ADC通道 */
    adc_ch_conf.Rank = ADC_REGULAR_RANK_2;                                                  /* 采样序列里的第2个 */
    HAL_ADC_ConfigChannel(&g_adc_nch_dma_handle, &adc_ch_conf);                             /* 配置ADC通道 */
    
    adc_ch_conf.Channel = ADC_CHANNEL_16;                                                   /* 配置使用的ADC通道 */
    adc_ch_conf.Rank = ADC_REGULAR_RANK_3;                                                  /* 采样序列里的第3个 */
    HAL_ADC_ConfigChannel(&g_adc_nch_dma_handle, &adc_ch_conf);                             /* 配置ADC通道 */
    
    adc_ch_conf.Channel = ADC_CHANNEL_17;                                                   /* 配置使用的ADC通道 */
    adc_ch_conf.Rank = ADC_REGULAR_RANK_4;                                                  /* 采样序列里的第4个 */
    HAL_ADC_ConfigChannel(&g_adc_nch_dma_handle, &adc_ch_conf);                             /* 配置ADC通道 */
    
    adc_ch_conf.Channel = ADC_CHANNEL_18;                                                   /* 配置使用的ADC通道 */
    adc_ch_conf.Rank = ADC_REGULAR_RANK_5;                                                  /* 采样序列里的第5个 */
    HAL_ADC_ConfigChannel(&g_adc_nch_dma_handle, &adc_ch_conf);                             /* 配置ADC通道 */
    
    adc_ch_conf.Channel = ADC_CHANNEL_19;                                                   /* 配置使用的ADC通道 */
    adc_ch_conf.Rank = ADC_REGULAR_RANK_6;                                                  /* 采样序列里的第6个 */
    HAL_ADC_ConfigChannel(&g_adc_nch_dma_handle, &adc_ch_conf);                             /* 配置ADC通道 */
    
    /* 配置DMA数据流请求中断优先级 */                           
    HAL_NVIC_SetPriority(ADC_ADCX_DMASx_IRQn, 3, 3);                            
    HAL_NVIC_EnableIRQ(ADC_ADCX_DMASx_IRQn);                                                /* 配置DMA中断 */
    
    HAL_DMA_Start_IT(&g_dma_nch_adc_handle, par, mar, 0);                                   /* 启动DMA，并开启中断 */
    HAL_ADC_Start_DMA(&g_adc_nch_dma_handle, &mar, 0);                                      /* 开启ADC，通过DMA传输结果 */
}

/**
 * @brief       使能一次DMA传输ADC数据
 * @note        该函数用寄存器来操作，防止用HAL库操作对其他参数有修改
 * @param       ndtr: DMA传输的次数
 * @retval      无
 */
void adc_dma_enable(uint16_t ndtr)
{
    ADC_ADCX->CR &= ~(1 << 0);         /* 先关闭ADC */

    ADC_ADCX_DMASx->CR &= ~(1 << 0);   /* 关闭DMA传输 */
    while (ADC_ADCX_DMASx->CR & 0X1);  /* 确保DMA可以被设置 */
    ADC_ADCX_DMASx->NDTR = ndtr;       /* 要传输的数据项数目 */
    ADC_ADCX_DMASx->CR |= 1 << 0;      /* 开启DMA传输 */
    
    ADC_ADCX->CR |= 1 << 0;            /* 重新启动ADC */
    ADC_ADCX->CR |= 1 << 2;            /* 启动规则转换通道 */
}

/**
 * @brief       DMA数据流中断服务函数
 * @param       无 
 * @retval      无
 */
void ADC_ADCX_DMASx_IRQHandler(void)
{
    if (ADC_ADCX_DMASx_IS_TC())
    {
        g_adc_dma_sta = 1;          /* 标记DMA传输完成 */
        ADC_ADCX_DMASx_CLR_TC();    /* 清除DMA1 数据流7 传输完成中断 */
    }
}
