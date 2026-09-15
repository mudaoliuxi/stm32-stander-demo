/**
 ****************************************************************************************************
 * @file        dsp_demo.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2022-10-15
 * @brief       DSP库演示代码
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

#include <math.h>
#include "./CMSIS/DSP/Include/arm_math.h"

#define DELTA   0.0001f /* 误差值 */

/**
 * @brief       正弦余弦测试
 * @param       angle: 起始角度
 * @param       times: 运算次数
 * @param       mode : 是否使用DSP库
 * @arg         0: 不使用
 * @arg         1: 使用
 * @retval      无
 */
uint8_t sin_cos_test(float angle, uint32_t times, uint8_t mode)
{
    float sinx;
    float cosx;
    float result;
    uint32_t i;
    
    if (mode == 0)                              /* 不使用DSP库 */
    {
        for (i=0; i<times; i++)
        {
            cosx = cosf(angle);                 /* 不使用DSP库的sin、cos函数 */
            sinx = sinf(angle);
            result = sinx * sinx + cosx * cosx; /* sin^2 + con^2 = 1 */
            result = fabsf(result - 1.0f);      /* 对比与1的差值 */
            if (result > DELTA)                 /* 结果有误 */
            {
                return 0xFF;
            }
            angle += 0.001f;                    /* 角度自增 */
        }
    }
    else                                        /* 使用DSP库 */
    {
        for (i=0; i<times; i++)
        {
            cosx = arm_cos_f32(angle);          /* 使用DSP库的sin、cos函数 */
            sinx = arm_sin_f32(angle);
            result = sinx * sinx + cosx * cosx; /* sin^2 + con^2 = 1 */
            result = fabsf(result - 1.0f);      /* 对比与1的差值 */
            if (result > DELTA)                 /* 结果有误 */
            {
                return 0xFF;
            }
            angle += 0.001f;                    /* 角度自增 */
        }
    }
    
    return 0;
}
