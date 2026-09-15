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

ADC_HandleTypeDef g_adc_handle;   /* ADC句柄 */

/**
 * @brief       初始化ADC过采样函数
 *   @note      本函数可以控制ADC过采样范围从1x ~ 1024x, 得到最高26位分辨率的AD转换结果
 * @param       osr : 过采样倍率, 0 ~ 1023, 表示:1x ~ 1024x过采样倍率
 * @param       ovss: 过采样右移位数, 0~11, 表示右移0位~11位.
 *   @note      过采样后, ADC的转换时间相应的会慢 osr倍.
 * @retval      无
 */
void adc_oversample_init(uint32_t osr, uint32_t ovss)
{
    GPIO_InitTypeDef gpio_init_struct;
    
    /* 使能时钟 */
    ADC_ADCX_CHY_GPIO_CLK_ENABLE();                                                             /* 开启ADC通道IO引脚时钟 */
    ADC_ADCX_CHY_CLK_ENABLE();                                                                  /* 使能ADC1/2时钟 */
    __HAL_RCC_ADC_CONFIG(RCC_ADCCLKSOURCE_CLKP);                                                /* ADC外设时钟选择 */
    
    /* 配置ADC1通道输入引脚 */
    gpio_init_struct.Pin = ADC_ADCX_CHY_GPIO_PIN;                                               /* ADC通道IO引脚 */
    gpio_init_struct.Mode = GPIO_MODE_ANALOG;                                                   /* 模拟 */
    HAL_GPIO_Init(ADC_ADCX_CHY_GPIO_PORT, &gpio_init_struct);                                   /* 配置ADC1通道输入引脚 */
    
    /* 配置ADC */
    g_adc_handle.Instance = ADC_ADCX;                                                           /* 选择哪个ADC */
    g_adc_handle.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV2;                                    /* 输入时钟2分频,即adc_ker_ck=per_ck/2=32Mhz */
    g_adc_handle.Init.Resolution = ADC_RESOLUTION_16B;                                          /* 16位模式  */
    g_adc_handle.Init.ScanConvMode = ADC_SCAN_DISABLE;                                          /* 非扫描模式，仅用到一个通道 */
    g_adc_handle.Init.EOCSelection = ADC_EOC_SINGLE_CONV;                                       /* 关闭EOC中断 */
    g_adc_handle.Init.LowPowerAutoWait = DISABLE;                                               /* 自动低功耗关闭 */
    g_adc_handle.Init.ContinuousConvMode = DISABLE;                                             /* 关闭连续转换 */
    g_adc_handle.Init.NbrOfConversion = 1;                                                      /* 赋值范围是1~16，本实验用到1个通道 */
    g_adc_handle.Init.DiscontinuousConvMode = DISABLE;                                          /* 禁止常规转换组不连续采样模式 */
    g_adc_handle.Init.NbrOfDiscConversion = 0;                                                  /* 配置不连续采样模式的通道数，禁止常规转换组不连续采样模式后，此参数忽略 */
    g_adc_handle.Init.ExternalTrigConv = ADC_SOFTWARE_START;                                    /* 软件触发 */
    g_adc_handle.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;                     /* 采用软件触发的话，此位忽略 */
    g_adc_handle.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DR;                         /* 常规通道的数据仅仅保存在DR寄存器里面 */
    g_adc_handle.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;                                       /* 有新的数据后直接覆盖掉旧数据 */
    g_adc_handle.Init.LeftBitShift = ADC_LEFTBITSHIFT_NONE;                                     /* 设置ADC转换结果的左移位数 */
    g_adc_handle.Init.OversamplingMode = ENABLE;                                                /* 开启过采样 */
    g_adc_handle.Init.Oversampling.Ratio = osr;                                                 /* 设置osr+1倍过采样 */
    g_adc_handle.Init.Oversampling.RightBitShift = ovss;                                        /* 数据右移ovss  bit */
    g_adc_handle.Init.Oversampling.TriggeredMode = ADC_TRIGGEREDMODE_SINGLE_TRIGGER;            /* 会在触发后连续完成通道的所有过采样转换 */
    g_adc_handle.Init.Oversampling.OversamplingStopReset = ADC_REGOVERSAMPLING_CONTINUED_MODE;  /* ROVSE=1, 使能常规过采样 */
    HAL_ADC_Init(&g_adc_handle);                                                                /* 初始化外设 */
    HAL_ADCEx_Calibration_Start(&g_adc_handle, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED);             /* ADC校准 */
}

/**
 * @brief       获取ADC转换后的结果 
 * @param       ch: 通道值 0~19，取值范围为：ADC_CHANNEL_0~ADC_CHANNEL_19
 * @retval      返回值:转换结果
 */
uint32_t adc_get_result(uint32_t ch)
{
    ADC_ChannelConfTypeDef adc_ch_conf = {0};
    
    adc_ch_conf.Channel = ch;                              /* 通道 */
    adc_ch_conf.Rank = ADC_REGULAR_RANK_1;                 /* 序列 */
    adc_ch_conf.SamplingTime = ADC_SAMPLETIME_810CYCLES_5; /* 采样时间，设置最大采样周期: 810.5个ADC周期 */
    adc_ch_conf.SingleDiff = ADC_SINGLE_ENDED;             /* 单边采集 */
    adc_ch_conf.OffsetNumber = ADC_OFFSET_NONE;            /* 不使用偏移量的通道 */
    adc_ch_conf.Offset = 0;                                /* 偏移量为0 */
    HAL_ADC_ConfigChannel(&g_adc_handle, &adc_ch_conf);    /* 通道配置 */
    
    HAL_ADC_Start(&g_adc_handle);                          /* 开启ADC */
    HAL_ADC_PollForConversion(&g_adc_handle, 10);          /* 轮询转换 */
    
    return HAL_ADC_GetValue(&g_adc_handle);                /* 返回最近一次ADC1常规组的转换结果 */
}

/**
 * @brief       获取ADC转换且进行均值滤波后的结果
 * @param       ch   : 通道号, 0~19
 * @param       times: 获取次数
 * @retval      通道ch的times次转换结果平均值
 */
uint32_t adc_get_result_average(uint32_t ch, uint8_t times)
{
    uint32_t temp_val = 0;
    uint8_t t;
    
    for (t = 0; t < times; t++) /* 获取times次数据 */
    {
        temp_val += adc_get_result(ch);
        delay_ms(5);
    }
    
    return temp_val / times;    /* 返回平均值 */
}
