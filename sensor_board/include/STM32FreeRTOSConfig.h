/*
 * sensor_board/include/FreeRTOSConfig.h
 *
 * FreeRTOS configuration for the STM32C092FBP (Cortex-M0+).
 * Heap model 4 is selected in platformio.ini via -D configUSE_HEAP_4=1.
 * Stack sizes throughout the project are in words (4 bytes each).
 */

#pragma once

/* Scheduler */
#define configUSE_PREEMPTION                    1
#define configUSE_TIME_SLICING                  0   /* no round-robin within same prio */
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0   /* M0+ has no CLZ; use generic */
#define configUSE_TICKLESS_IDLE                 0
#define configCPU_CLOCK_HZ                      ( ( unsigned long ) 64000000 )
#define configTICK_RATE_HZ                      ( ( TickType_t ) 1000 )
#define configMAX_PRIORITIES                    8
#define configMINIMAL_STACK_SIZE                ( ( unsigned short ) 64 )
#define configMAX_TASK_NAME_LEN                 12
#define configUSE_16_BIT_TICKS                  0

/* Memory */
#define configTOTAL_HEAP_SIZE                   ( ( size_t ) ( 20 * 1024 ) )
#define configAPPLICATION_ALLOCATED_HEAP        0
#define configSUPPORT_STATIC_ALLOCATION         0
#define configSUPPORT_DYNAMIC_ALLOCATION        1

/* Features */
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             0
#define configUSE_COUNTING_SEMAPHORES           0
#define configUSE_QUEUE_SETS                    0
#define configUSE_TASK_NOTIFICATIONS            1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES   1
#define configUSE_TIMERS                        0   /* software timers not used */
#define configUSE_CO_ROUTINES                   0
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configUSE_MALLOC_FAILED_HOOK            1   /* trap heap exhaustion */
#define configCHECK_FOR_STACK_OVERFLOW          2   /* pattern + watermark check */
#define configUSE_STATS_FORMATTING_FUNCTIONS    0

/* API inclusions */
#define INCLUDE_vTaskPrioritySet                0
#define INCLUDE_uxTaskPriorityGet               0
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetCurrentTaskHandle       0
#define INCLUDE_xTaskGetSchedulerState          0

/* Cortex-M0+ interrupt priority */
/* M0+ has no BASEPRI register; all interrupts can call FromISR APIs.
   The SysTick and PendSV priorities must be the lowest numerically
   highest values so the kernel can preempt them safely. */
#define configKERNEL_INTERRUPT_PRIORITY         ( 0xFF )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    ( 0 )   /* unused on M0+ */

/* Assert */
#ifdef __cplusplus
extern "C" {
#endif
extern void vAssertCalled(const char *file, int line);
#ifdef __cplusplus
}
#endif
#define configASSERT( x ) \
    if( ( x ) == 0 ) { vAssertCalled( __FILE__, __LINE__ ); }

