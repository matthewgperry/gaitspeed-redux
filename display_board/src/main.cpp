// display_board/src/main.cpp
//
// Firmware entry point for the STM32F103RCT6 display board.
// Enables TIM2 as a 32-bit free-running µs counter, creates all
// FreeRTOS queues and the result mutex, spawns the eight
// application tasks, then hands control to the scheduler.
// This file owns all IPC handles; tasks extern them.
//
// NODE_ID 0x01 = master display (runs gait calc + time sync);
// NODE_ID 0x02 = peer display (same binary, different define).

#include <Arduino.h>
#include <STM32FreeRTOS.h>
#include <cstdint>
#include <task.h>
#include <queue.h>
#include <semphr.h>

#include "can_protocol.h"
#include "gait_types.h"
#include "time_sync.h"
#include "display_board_config.h"
#include "ui_event.h"
#include "can_drv.h"
#include "button.h"

// IPC handles
QueueHandle_t   xCanTxQueue;
QueueHandle_t   xCanRxQueue;
QueueHandle_t   xUIEventQueue;
QueueHandle_t   xBuzzerQueue;
QueueHandle_t   xGaitCalcQueue;
SemaphoreHandle_t xResultMutex;

// Shared Display State
GaitResult_t gResult = {};
DisplayState_t gDisplayState = DISPLAY_IDLE;
uint16_t gTrialNumber = 0;

volatile uint8_t gNodeFlags = 0;
volatile uint32_t gLastHb[4] = {};

void vCAN_RX_Task   (void *);
void vCAN_TX_Task   (void *);
void vGaitCalc_Task (void *);
void vDisplay_Task  (void *);
void vTimeSync_Task (void *);
void vBuzzer_Task   (void *);
void vWatchdog_Task (void *);

extern "C" void can_rx_callback(CanFrame_t *frame) {
    BaseType_t woken = pdFALSE;
    xQueueSendFromISR(xCanRxQueue, frame, &woken);
    portYIELD_FROM_ISR(woken);
}

void setup() {
    __HAL_RCC_TIM2_CLK_ENABLE();
    TIM2->PSC  = 0;
    TIM2->ARR  = 0xFFFFFFFF;
    TIM2->CR1 |= TIM_CR1_CEN;

    can_init(NODE_ID);
    button_init();   // configures BTN_PIN, attaches EXTI, creates Button_Task

    // IPC
    xCanTxQueue    = xQueueCreate(16, sizeof(CanFrame_t));
    xCanRxQueue    = xQueueCreate(16, sizeof(CanFrame_t));
    xUIEventQueue  = xQueueCreate(16, sizeof(UIEvent_t));
    xBuzzerQueue   = xQueueCreate(8,  sizeof(BuzzerCmd_t));
    xGaitCalcQueue = xQueueCreate(8,  sizeof(CanFrame_t));
    xResultMutex   = xSemaphoreCreateMutex();

    configASSERT(xCanTxQueue);
    configASSERT(xCanRxQueue);
    configASSERT(xUIEventQueue);
    configASSERT(xBuzzerQueue);
    configASSERT(xGaitCalcQueue);
    configASSERT(xResultMutex);

    // Task creation — stack in words (4 bytes each on M3)
    xTaskCreate(vCAN_RX_Task,   "canrx",   256, NULL, 7, NULL);
    xTaskCreate(vGaitCalc_Task, "gait",    256, NULL, 6, NULL);
    xTaskCreate(vCAN_TX_Task,   "cantx",   128, NULL, 5, NULL);
    // Button_Task created inside button_init() at priority 5
    xTaskCreate(vDisplay_Task,  "disp",   1024, NULL, 4, NULL);
    xTaskCreate(vTimeSync_Task, "tsync",   128, NULL, 3, NULL);
    xTaskCreate(vBuzzer_Task,   "buzz",    128, NULL, 2, NULL);
    xTaskCreate(vWatchdog_Task, "wdog",     64, NULL, 1, NULL);

    vTaskStartScheduler();
    for (;;) {}
}

void loop() {}

