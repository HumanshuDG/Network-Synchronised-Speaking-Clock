/*
 * FreeRTOSConfig.h
 * FreeRTOS configuration for Speaking Clock project
 * Target: MPS2-AN385 (Cortex-M3) on QEMU
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include <stdint.h>

/* ---------------------------------------------------------------------------
 * Core Scheduler Configuration
 * --------------------------------------------------------------------------- */
#define configUSE_PREEMPTION                    1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION  1
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configUSE_DAEMON_TASK_STARTUP_HOOK      0

/* MPS2-AN385 system clock is 25 MHz */
#define configCPU_CLOCK_HZ                      ((uint32_t)25000000)
#define configTICK_RATE_HZ                      ((TickType_t)1000)

/* ---------------------------------------------------------------------------
 * Task Configuration
 * --------------------------------------------------------------------------- */
#define configMAX_PRIORITIES                    5
#define configMINIMAL_STACK_SIZE                ((unsigned short)128)
#define configMAX_TASK_NAME_LEN                 16
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES   3

/* ---------------------------------------------------------------------------
 * Memory Configuration
 * --------------------------------------------------------------------------- */
#define configSUPPORT_STATIC_ALLOCATION         0
#define configSUPPORT_DYNAMIC_ALLOCATION        1
#define configTOTAL_HEAP_SIZE                   ((size_t)(64 * 1024))
#define configAPPLICATION_ALLOCATED_HEAP        0

/* ---------------------------------------------------------------------------
 * Synchronization Primitives
 * --------------------------------------------------------------------------- */
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             0
#define configUSE_COUNTING_SEMAPHORES           1
#define configQUEUE_REGISTRY_SIZE               8
#define configUSE_QUEUE_SETS                    0

/* ---------------------------------------------------------------------------
 * Timer Configuration
 * --------------------------------------------------------------------------- */
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               3
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            (configMINIMAL_STACK_SIZE * 2)

/* ---------------------------------------------------------------------------
 * Interrupt Configuration (Cortex-M3)
 * QEMU doesn't model priority bits — use full 8-bit range
 * --------------------------------------------------------------------------- */
#define configKERNEL_INTERRUPT_PRIORITY         255
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    4
#define configMAX_API_CALL_INTERRUPT_PRIORITY   configMAX_SYSCALL_INTERRUPT_PRIORITY

/* ---------------------------------------------------------------------------
 * Optional API Functions
 * --------------------------------------------------------------------------- */
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#define INCLUDE_xTaskGetIdleTaskHandle          0
#define INCLUDE_eTaskGetState                   1
#define INCLUDE_xTimerPendFunctionCall          1
#define INCLUDE_xTaskAbortDelay                 0
#define INCLUDE_xTaskGetHandle                  1

/* ---------------------------------------------------------------------------
 * Debug / Assert
 * --------------------------------------------------------------------------- */
extern void vAssertCalled(const char *pcFile, uint32_t ulLine);
#define configASSERT(x) if((x) == 0) vAssertCalled(__FILE__, __LINE__)

/* ---------------------------------------------------------------------------
 * Cortex-M3 Interrupt Mapping
 * Map FreeRTOS port handlers to standard CMSIS names
 * --------------------------------------------------------------------------- */
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

#endif /* FREERTOS_CONFIG_H */
