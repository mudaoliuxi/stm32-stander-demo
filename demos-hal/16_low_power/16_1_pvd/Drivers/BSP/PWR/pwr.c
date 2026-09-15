/**
 ****************************************************************************************************
 * @file        pwr.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-04-25
 * @brief       低功耗模式驱动代码
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

#include "./BSP/PWR/pwr.h"
#include "./BSP/LED/led.h"
#include "./BSP/LCD/lcd.h"

/**
 * @brief       初始化PVD电压监视器
 * @param       pls: 电压等级
 *   @arg       PWR_PVDLEVEL_0,1.95V;  PWR_PVDLEVEL_1,2.1V
 *   @arg       PWR_PVDLEVEL_2,2.25V;  PWR_PVDLEVEL_3,2.4V;
 *   @arg       PWR_PVDLEVEL_4,2.55V;  PWR_PVDLEVEL_5,2.7V;
 *   @arg       PWR_PVDLEVEL_6,2.85V;  PWR_PVDLEVEL_7,使用PVD_IN脚上的电压(与Vrefint比较)
 * @retval      无
 */
void pwr_pvd_init(uint32_t pls)
{
    PWR_PVDTypeDef pwr_pvd = {0};
    
    HAL_PWR_EnablePVD();                            /* 使能PVD时钟 */
    
    pwr_pvd.PVDLevel = pls;                         /* 检测电压级别 */
    pwr_pvd.Mode = PWR_PVD_MODE_IT_RISING_FALLING;  /* 使用中断线的上升沿和下降沿双边缘触发 */
    HAL_PWR_ConfigPVD(&pwr_pvd);                    /* 配置PVD */
    
    HAL_NVIC_SetPriority(PVD_AVD_IRQn, 3 ,3);       /* 设置中断优先级3，子优先级3 */
    HAL_NVIC_EnableIRQ(PVD_AVD_IRQn);               /* 使能PVD中断 */
    HAL_PWR_EnablePVD();                            /* 使能PVD检测 */
}

/**
 * @brief       PVD中断回调函数
 * @param       无
 * @retval      无
 */
void PVD_AVD_IRQHandler(void)
{
    if (__HAL_PWR_GET_FLAG(PWR_FLAG_PVDO) == SET)                           /* 判断事件线PVD0中断标志 */
    {
        lcd_show_string(30, 130, 200, 16, 16, "PVD Low Voltage!", RED);     /* LCD显示电压低 */
        LED1(0);                                                            /* 点亮LED1 */
    }
    else
    {
        lcd_show_string(30, 130, 200, 16, 16, "PVD Voltage OK! ", BLUE);    /* LCD显示电压正常 */
        LED1(1);                                                            /* 熄灭LED1 */
    }
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_PVDO);                                    /* 清除事件线PVD0中断标志 */
}
