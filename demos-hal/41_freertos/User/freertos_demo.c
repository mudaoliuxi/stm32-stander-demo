/**
 ****************************************************************************************************
 * @file        freertos.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2022-01-04
 * @brief       FreeRTOS 移植实验
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 Mini Pro H750开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 ****************************************************************************************************
 */

#include "freertos_demo.h"
#include "./SYSTEM/usart/usart.h"
#include "./BSP/LED/led.h"
#include "FreeRTOS.h"
#include "task.h"

/* START_TASK任务配置
 * 包括: 任务句柄 任务优先级 堆栈大小 创建任务
 */
#define START_TASK_PRIO 1               /* 任务优先级 */
#define START_STK_SIZE  128             /* 任务堆栈大小 */
TaskHandle_t StartTask_Handler;         /* 任务句柄 */
void start_task(void *pvParameters);    /* 任务函数 */

/* LED0任务配置
 * 包括: 任务句柄 任务优先级 堆栈大小 创建任务
 */
#define LED0_PRIO      2                /* 任务优先级 */
#define LED0_STK_SIZE  128              /* 任务堆栈大小 */
TaskHandle_t LED0Task_Handler;          /* 任务句柄 */
void led0_task(void *pvParameters);     /* 任务函数 */

/* LED1任务配置
 * 包括: 任务句柄 任务优先级 堆栈大小 创建任务
 */
#define LED1_PRIO      2                /* 任务优先级 */
#define LED1_STK_SIZE  128              /* 任务堆栈大小 */
TaskHandle_t LED1Task_Handler;          /* 任务句柄 */
void led1_task(void *pvParameters);     /* 任务函数 */

/**
 * @brief       FreeRTOS例程入口函数
 * @param       无
 * @retval      无
 */
void freertos_demo(void)
{
    xTaskCreate((TaskFunction_t )start_task,            /* 任务函数 */
                (const char*    )"start_task",          /* 任务名称 */
                (uint16_t       )START_STK_SIZE,        /* 任务堆栈大小 */
                (void*          )NULL,                  /* 传入给任务函数的参数 */
                (UBaseType_t    )START_TASK_PRIO,       /* 任务优先级 */
                (TaskHandle_t*  )&StartTask_Handler);   /* 任务句柄 */
    vTaskStartScheduler();
}

/**
 * @brief       start_task
 * @param       pvParameters: 传入参数(未用到)
 * @retval      无
 */
void start_task(void *pvParameters)
{
    taskENTER_CRITICAL();           /* 进入临界区 */
    
    /* 创建LED0任务 */
    xTaskCreate((TaskFunction_t )led0_task,
                (const char*    )"led0_task",
                (uint16_t       )LED0_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )LED0_PRIO,
                (TaskHandle_t*  )&LED0Task_Handler);
                
    /* 创建LED1任务 */
    xTaskCreate((TaskFunction_t )led1_task,
                (const char*    )"led1_task",
                (uint16_t       )LED1_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )LED1_PRIO,
                (TaskHandle_t*  )&LED1Task_Handler);
                
    vTaskDelete(StartTask_Handler); /* 删除开始任务 */
    taskEXIT_CRITICAL();            /* 退出临界区 */
}

/**
 * @brief       LED0任务
 * @param       pvParameters: 传入参数(未用到)
 * @retval      无
 */
void led0_task(void *pvParameters)
{
    while(1)
    {
        LED0(0);
        vTaskDelay(80);
        LED0(1);
        vTaskDelay(920);
    }
}

/**
 * @brief       LED1任务
 * @param       pvParameters: 传入参数(未用到)
 * @retval      无
 */
void led1_task(void *pvParameters)
{
    while(1)
    {
        LED1(0);
        vTaskDelay(300);
        LED1(1);
        vTaskDelay(300);
    }
}
