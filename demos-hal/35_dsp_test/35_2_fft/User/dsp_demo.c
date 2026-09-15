/**
 ****************************************************************************************************
 * @file        dsp_demo.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-04-25
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

static arm_cfft_radix4_instance_f32 scfft;

/**
 * @brief       初始化FFT测试
 * @param       fft_inputbuf: FFT输入数组
 * @retval      fft_length  : FFT长度
 */
void fft_test_init(float *fft_inputbuf, uint16_t fft_length)
{
    uint16_t i;
    
    arm_cfft_radix4_init_f32(&scfft, fft_length, 0, 1);
    
    /* 生成信号序列 */
    for (i=0; i<fft_length; i++)
    {
        fft_inputbuf[2 * i] = 100 +                                             /* 实部 */
                              10 * arm_sin_f32(2 * PI * i / fft_length) +
                              30 * arm_sin_f32(2 * PI * i * 4 / fft_length) +
                              50 * arm_cos_f32(2 * PI * i * 8 / fft_length);
        fft_inputbuf[2 * i + 1] = 0;                                            /* 虚部 */
    }
}

/**
 * @brief       FFT计算测试
 * @param       fft_inputbuf: FFT输入数组
 * @retval      无
 */
void fft_test(float *fft_inputbuf)
{
    arm_cfft_radix4_f32(&scfft, fft_inputbuf);
}

/**
 * @brief       对FFT计算测试结果取模求得幅值
 * @param       fft_inputbuf : FFT输入数组
 * @param       fft_outputbuf: FFT输出数组
 * @retval      fft_length   : FFT长度
 * @retval      无
 */
void fft_test_get_output(float *fft_inputbuf, float *fft_outputbuf, uint16_t fft_length)
{
    arm_cmplx_mag_f32(fft_inputbuf, fft_outputbuf, fft_length);
}
