/**
 ****************************************************************************************************
 * @file        gtim.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-04-25
 * @brief       通用定时器驱动代码
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

#include "./BSP/TIMER/gtim.h"
#include "./BSP/LED/led.h"

/* 输入捕获状态(g_timxchy_cap_sta)
 * [7]  :0,没有成功的捕获;1,成功捕获到一次.
 * [6]  :0,还没捕获到高电平;1,已经捕获到高电平了.
 * [5:0]:捕获高电平后溢出的次数,最多溢出63次,所以最长捕获值 = 63*65536 + 65535 = 4194303
 *       注意:为了通用,我们默认ARR和CCRy都是16位寄存器,对于32位的定时器(如:TIM5),也只按16位使用
 *       按1us的计数频率,最长溢出时间为:4194303 us, 约4.19秒
 */
uint8_t g_timxchy_cap_sta = 0;              /* 输入捕获状态 */
uint16_t g_timxchy_cap_val = 0;             /* 输入捕获值 */

TIM_HandleTypeDef g_timx_cap_chy_handle;    /* 定时器x句柄 */

/**
 * @brief       初始化通用定时器输入捕获
 * @note
 *              通用定时器的时钟来自APB1,当D2PPRE1≥2分频的时候
 *              通用定时器的时钟为APB1时钟的2倍, 而APB1为120M, 所以定时器时钟 = 240Mhz
 *              定时器溢出时间计算方法: Tout = ((arr + 1) * (psc + 1)) / Ft us.
 *              Ft=定时器工作频率,单位:Mhz
 *
 * @param       arr: 自动重装值
 * @param       psc: 时钟预分频数
 * @retval      无
 */
void gtim_timx_cap_chy_init(uint16_t arr, uint16_t psc)
{
    GPIO_InitTypeDef gpio_init_struct;
    TIM_IC_InitTypeDef timx_ic_cap_chy = {0};
    
    /* 使能时钟 */
    GTIM_TIMX_CAP_CHY_CLK_ENABLE();                                                         /* 使能TIMx时钟 */
    GTIM_TIMX_CAP_CHY_GPIO_CLK_ENABLE();                                                    /* 开启捕获IO的时钟 */
    
    /* 配置输入捕获引脚 */
    gpio_init_struct.Pin = GTIM_TIMX_CAP_CHY_GPIO_PIN;                                      /* 输入捕获引脚 */
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;                                                /* 复用推挽输出 */
    gpio_init_struct.Pull = GPIO_PULLDOWN;                                                  /* 下拉 */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;                                     /* 高速 */
    gpio_init_struct.Alternate = GTIM_TIMX_CAP_CHY_GPIO_AF;                                 /* 复用为捕获TIM5的通道1 */
    HAL_GPIO_Init(GTIM_TIMX_CAP_CHY_GPIO_PORT, &gpio_init_struct);                          /* 配置输入捕获引脚 */
    HAL_NVIC_SetPriority(GTIM_TIMX_CAP_IRQn, 1, 3);                                         /* 抢占1，子优先级3 */
    HAL_NVIC_EnableIRQ(GTIM_TIMX_CAP_IRQn);                                                 /* 开启ITMx中断 */
    
    /* 配置通用定时器 */
    g_timx_cap_chy_handle.Instance = GTIM_TIMX_CAP;                                         /* 定时器5 */
    g_timx_cap_chy_handle.Init.Prescaler = psc;                                             /* 定时器分频 */
    g_timx_cap_chy_handle.Init.CounterMode = TIM_COUNTERMODE_UP;                            /* 递增计数模式 */
    g_timx_cap_chy_handle.Init.Period = arr;                                                /* 自动重装载值 */
    HAL_TIM_IC_Init(&g_timx_cap_chy_handle);                                                /* 初始化捕获定时器 */
    
    /* 配置输入捕获通道 */
    timx_ic_cap_chy.ICPolarity = TIM_ICPOLARITY_RISING;                                     /* 上升沿捕获 */
    timx_ic_cap_chy.ICSelection = TIM_ICSELECTION_DIRECTTI;                                 /* 映射到TI1上 */
    timx_ic_cap_chy.ICPrescaler = TIM_ICPSC_DIV1;                                           /* 配置输入分频，不分频 */
    timx_ic_cap_chy.ICFilter = 0;                                                           /* 配置输入滤波器，不滤波 */
    HAL_TIM_IC_ConfigChannel(&g_timx_cap_chy_handle, &timx_ic_cap_chy, GTIM_TIMX_CAP_CHY);  /* 配置TIM5通道1 */
    
    if (GTIM_TIMX_CAP_CHY == TIM_CHANNEL_1)
    {
        __HAL_TIM_ENABLE_IT(&g_timx_cap_chy_handle, TIM_IT_CC1);                            /* 使能输入捕获通道1中断 */
    }
    else if (GTIM_TIMX_CAP_CHY == TIM_CHANNEL_2)
    {
        __HAL_TIM_ENABLE_IT(&g_timx_cap_chy_handle, TIM_IT_CC2);                            /* 使能输入捕获通道2中断 */
    }
    else if(GTIM_TIMX_CAP_CHY == TIM_CHANNEL_3)
    {
        __HAL_TIM_ENABLE_IT(&g_timx_cap_chy_handle, TIM_IT_CC3);                            /* 使能输入捕获通道3中断 */
    }
    else if(GTIM_TIMX_CAP_CHY == TIM_CHANNEL_4)
    {
        __HAL_TIM_ENABLE_IT(&g_timx_cap_chy_handle, TIM_IT_CC4);                            /* 使能输入捕获通道4中断 */
    }
    
    HAL_TIM_IC_Start_IT(&g_timx_cap_chy_handle, GTIM_TIMX_CAP_CHY);                         /* 开始捕获TIM5的通道1 */
}

/**
 * @brief       定时器中断服务函数
 * @param       无
 * @retval      无
 */
void GTIM_TIMX_CAP_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&g_timx_cap_chy_handle); /* 定时器HAL库共用处理函数 */
}

/**
 * @brief       定时器输入捕获中断处理回调函数
 * @param       htim:定时器句柄指针
 * @note        该函数在HAL_TIM_IRQHandler中会被调用
 * @retval      无
 */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == GTIM_TIMX_CAP)
    {
        if ((g_timxchy_cap_sta & 0X80) == 0)                                                                /* 还没成功捕获 */
        {
            if (g_timxchy_cap_sta & 0X40)                                                                   /* 捕获到一个下降沿 */
            {
                g_timxchy_cap_sta |= 0X80;                                                                  /* 标记成功捕获到一次高电平脉宽 */
                g_timxchy_cap_val = HAL_TIM_ReadCapturedValue(&g_timx_cap_chy_handle, GTIM_TIMX_CAP_CHY);   /* 获取当前的捕获值 */
                TIM_RESET_CAPTUREPOLARITY(&g_timx_cap_chy_handle, GTIM_TIMX_CAP_CHY);                       /* 一定要先清除原来的设置 */
                TIM_SET_CAPTUREPOLARITY(&g_timx_cap_chy_handle, GTIM_TIMX_CAP_CHY, TIM_ICPOLARITY_RISING);  /* 配置TIM5通道1上升沿捕获 */
            }
            else                                                                                            /* 还未开始,第一次捕获上升沿 */
            {
                g_timxchy_cap_sta = 0;                                                                      /* 清空 */
                g_timxchy_cap_val = 0;
                g_timxchy_cap_sta |= 0X40;                                                                  /* 标记捕获到了上升沿 */
                __HAL_TIM_DISABLE(&g_timx_cap_chy_handle);                                                  /* 关闭定时器5 */
                __HAL_TIM_SET_COUNTER(&g_timx_cap_chy_handle,0);                                            /* 定时器5计数器清零 */
                TIM_RESET_CAPTUREPOLARITY(&g_timx_cap_chy_handle, GTIM_TIMX_CAP_CHY);                       /* 一定要先清除原来的设置！！ */
                TIM_SET_CAPTUREPOLARITY(&g_timx_cap_chy_handle, GTIM_TIMX_CAP_CHY, TIM_ICPOLARITY_FALLING); /* 定时器5通道1设置为下降沿捕获 */
                __HAL_TIM_ENABLE(&g_timx_cap_chy_handle);                                                   /* 使能定时器5 */
            }
        }
    }
}

/**
 * @brief       定时器更新中断回调函数
 * @param        htim:定时器句柄指针
 * @note        此函数会被定时器中断函数共同调用的
 * @retval      无
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == GTIM_TIMX_CAP)
    {
        if ((g_timxchy_cap_sta & 0X80) == 0)                                                                    /* 还没成功捕获 */
        {
            if (g_timxchy_cap_sta & 0X40)                                                                       /* 已经捕获到高电平了 */
            {
                if ((g_timxchy_cap_sta & 0X3F) == 0X3F)                                                         /* 高电平太长了 */
                {
                    TIM_RESET_CAPTUREPOLARITY(&g_timx_cap_chy_handle, GTIM_TIMX_CAP_CHY);                       /* 一定要先清除原来的设置 */
                    TIM_SET_CAPTUREPOLARITY(&g_timx_cap_chy_handle, GTIM_TIMX_CAP_CHY, TIM_ICPOLARITY_RISING);  /* 配置TIM5通道1上升沿捕获 */
                    g_timxchy_cap_sta |= 0X80;                                                                  /* 标记成功捕获了一次 */
                    g_timxchy_cap_val = 0XFFFF;
                }
                else                                                                                            /* 累计定时器溢出次数 */
                {
                    g_timxchy_cap_sta++;
                }
            }
        }
    }
}
