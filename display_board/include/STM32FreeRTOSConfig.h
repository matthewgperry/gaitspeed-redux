/*
 * display_board/include/FreeRTOSConfig.h
 *
 * FreeRTOS configuration for the STM32F103RCT (Cortex-M3).
 * More RAM available (48 kB) so heap and stacks are larger than
 * the sensor board config.
 */

#pragma once

/* ── Scheduler ──────────────────────────────────────────────── */
#define configUSE_PREEMPTION                    1
#define configUSE_TIME_SLICING                  0
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1   /* M3 has CLZ — use fast path */
#define configUSE_TICKLESS_IDLE                 0
#define configCPU_CLOCK_HZ                      ( ( unsigned long ) 72000000 )
#define configTICK_RATE_HZ                      ( ( TickType_t ) 1000 )
#define configMAX_PRIORITIES                    8
#define configMINIMAL_STACK_SIZE                ( ( unsigned short ) 128 )
#define configMAX_TASK_NAME_LEN                 12
#define configUSE_16_BIT_TICKS                  0

/* ── Memory ─────────────────────────────────────────────────── */
#define configTOTAL_HEAP_SIZE                   ( ( size_t ) ( 32 * 1024 ) )
#define configAPPLICATION_ALLOCATED_HEAP        0
#define configSUPPORT_STATIC_ALLOCATION         0
#define configSUPPORT_DYNAMIC_ALLOCATION        1

/* ── Features ───────────────────────────────────────────────── */
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             0
#define configUSE_COUNTING_SEMAPHORES           0
#define configUSE_QUEUE_SETS                    0
#define configUSE_TASK_NOTIFICATIONS            1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES   1
#define configUSE_TIMERS                        0
#define configUSE_CO_ROUTINES                   0
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configUSE_MALLOC_FAILED_HOOK            1
#define configCHECK_FOR_STACK_OVERFLOW          2
#define configUSE_STATS_FORMATTING_FUNCTIONS    0

/* ── API inclusions ─────────────────────────────────────────── */
#define INCLUDE_vTaskPrioritySet                0
#define INCLUDE_uxTaskPriorityGet               0
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetCurrentTaskHandle       0
#define INCLUDE_xTaskGetSchedulerState          0

/* ── Cortex-M3 interrupt priorities ────────────────────────────
   Lower numeric value = higher hardware priority.
   SYSCALL priority must be >= (numerically) KERNEL priority so that
   FromISR calls are only made from interrupts at or below SYSCALL level.
   FDCAN RX ISR and GPIO EXTI must be configured at or below
   configMAX_SYSCALL_INTERRUPT_PRIORITY in HAL init.              */
#define configKERNEL_INTERRUPT_PRIORITY         ( 15 << 4 )   /* priority 15 */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    (  5 << 4 )   /* priority  5 */

