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

TIM_HandleTypeDef g_timx_cnt_chy_handle;    /* 定时器x句柄 */
uint32_t g_timxchy_cnt_ofcnt = 0 ;          /* 计数溢出次数 */

/**
 * @brief       初始化通用定时器脉冲计数
 * @note        本函数选择通用定时器的时钟选择: 外部时钟源模式1(SMS[2:0] = 111)
 *              这样CNT的计数时钟源就来自 TIMX_CH1/CH2, 可以实现外部脉冲计数(脉冲接入CH1/CH2)
 *              时钟分频数 = psc, 一般设置为0, 表示每一个时钟都会计数一次, 以提高精度.
 *              通过读取CNT和溢出次数, 经过简单计算, 可以得到当前的计数值, 从而实现脉冲计数
 * @param       arr: 自动重装值 
 * @retval      无
 */
void gtim_timx_cnt_chy_init(uint16_t psc)
{
    GPIO_InitTypeDef gpio_init_struct;
    TIM_SlaveConfigTypeDef tim_slave_config = {0};
    
    /* 使能时钟 */
    GTIM_TIMX_CNT_CHY_CLK_ENABLE();                                                 /* 使能通话用定时器时钟 */
    GTIM_TIMX_CNT_CHY_GPIO_CLK_ENABLE();                                            /* 使能输出捕获引脚端口时钟 */
    
    /* 配置通用定时器 */
    g_timx_cnt_chy_handle.Instance = GTIM_TIMX_CNT;                                 /* 通用定时器2 */
    g_timx_cnt_chy_handle.Init.Prescaler = psc;                                     /* 定时器分频 */
    g_timx_cnt_chy_handle.Init.CounterMode = TIM_COUNTERMODE_UP;                    /* 递增计数模式 */
    g_timx_cnt_chy_handle.Init.Period = 0xFFFF;                                     /* 自动重装载值 */
    HAL_TIM_IC_Init(&g_timx_cnt_chy_handle);
    
    /* 配置输入捕获引脚 */
    gpio_init_struct.Pin = GTIM_TIMX_CNT_CHY_GPIO_PIN;                              /* 输入捕获的GPIO口 */
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;                                        /* 复用推挽输出 */
    gpio_init_struct.Pull = GPIO_PULLDOWN;                                          /* 下拉 */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;                             /* 高速 */
    gpio_init_struct.Alternate = GTIM_TIMX_CNT_CHY_GPIO_AF;                         /* 复用为捕获TIMx的通道 */
    HAL_GPIO_Init(GTIM_TIMX_CNT_CHY_GPIO_PORT, &gpio_init_struct);                  /* 配置输入捕获引脚 */
    
    /* 从模式：外部触发模式1 */
    tim_slave_config.SlaveMode = TIM_SLAVEMODE_EXTERNAL1;                           /* 从模式：外部触发模式1 */
    tim_slave_config.InputTrigger = TIM_TS_TI1FP1;                                  /* 输入触发：选择 TI1FP1(TIMX_CH1) 作为输入源 */
    tim_slave_config.TriggerPolarity = TIM_TRIGGERPOLARITY_RISING;                  /* 触发极性：上升沿 */
    tim_slave_config.TriggerPrescaler = TIM_TRIGGERPRESCALER_DIV1;                  /* 触发预分频：无 */
    tim_slave_config.TriggerFilter = 0x0;                                           /* 滤波：本例中不需要任何滤波 */
    HAL_TIM_SlaveConfigSynchronization(&g_timx_cnt_chy_handle, &tim_slave_config);  /* 从模式下配置通用定时器 */
    
    /* 使能通用定时器及其相关中断 */
    HAL_NVIC_SetPriority(GTIM_TIMX_CNT_IRQn, 1, 3);                                 /* 设置中断优先级，抢占优先级1，子优先级3 */
    HAL_NVIC_EnableIRQ(GTIM_TIMX_CNT_IRQn);                                         /* 开启ITMx中断 */
    __HAL_TIM_ENABLE_IT(&g_timx_cnt_chy_handle, TIM_IT_UPDATE);                     /* 使能更新中断 */
    HAL_TIM_IC_Start(&g_timx_cnt_chy_handle, GTIM_TIMX_CNT_CHY);                    /* 开始捕获TIMx的通道y */
}

/**
 * @brief       获取通用定时器当前计数值（含溢出）
 * @param       无
 * @retval      当前计数值
 */
uint32_t gtim_timx_cnt_chy_get_count(void)
{
    uint32_t count = 0;
    
    count = g_timxchy_cnt_ofcnt * (0xFFFF + 1);             /* 计算溢出次数对应的计数值 */
    count += __HAL_TIM_GET_COUNTER(&g_timx_cnt_chy_handle); /* 加上当前CNT的值 */
    
    return count;
}

/**
 * @brief       重启通用定时器计数
 * @param       无
 * @retval      无
 */
void gtim_timx_cnt_chy_restart(void)
{
    __HAL_TIM_DISABLE(&g_timx_cnt_chy_handle);          /* 关闭定时器TIMX */
    g_timxchy_cnt_ofcnt = 0;                            /* 累加器清零 */
    __HAL_TIM_SET_COUNTER(&g_timx_cnt_chy_handle, 0);   /* 计数器清零 */
    __HAL_TIM_ENABLE(&g_timx_cnt_chy_handle);           /* 使能定时器TIMX */
}

/**
 * @brief       通用定时器中断服务函数
 * @param       无
 * @retval      无
 */
void GTIM_TIMX_CNT_IRQHandler(void)
{
    if(__HAL_TIM_GET_FLAG(&g_timx_cnt_chy_handle, TIM_FLAG_UPDATE) == SET)  /* 判断更新中断标志 */
    {
        g_timxchy_cnt_ofcnt++;                                              /* 更新通用定时器溢出次数 */
    }
    
    __HAL_TIM_CLEAR_IT(&g_timx_cnt_chy_handle, TIM_IT_UPDATE);              /* 清除更新中断标志 */
}
