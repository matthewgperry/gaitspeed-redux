// sensor_board/src/main.cpp
//
// Firmware entry point for the STM32C092FBP sensor board.
// All application logic lives in the tasks below; this file
// only initialises hardware and creates the RTOS tasks.

#include <Arduino.h>
#include <STM32FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>

#include "can_protocol.h"
#include "gait_types.h"
#include "time_sync.h"
#include "sensor_board_config.h"
#include "can_drv.h"
#include "vl53_drv.h"
#include "neopixel_drv.h"

// IPC handles (defined here, declared extern in tasks)
QueueHandle_t     xCanTxQueue;
QueueHandle_t     xCanRxQueue;
QueueHandle_t     xTripQueue;
SemaphoreHandle_t xVL53Mutex;

// Shared sensor state (guarded by xVL53Mutex)
volatile LedMode_t gLedMode         = LED_IDLE;
volatile VL53State_t gVL53State     = VL53_STATE_IDLE;

// Forward declarations of task functions
void vVL53_ISR_Task   (void *);
void vVL53_Poll_Task  (void *);
void vCAN_TX_Task     (void *);
void vCAN_RX_Task     (void *);
void vLED_Task        (void *);
void vHeartbeat_Task  (void *);
void vWatchdog_Task   (void *);

// CAN RX ISR -> queue
// Called from the FDCAN IRQ handler in can_drv.cpp.
extern "C" void can_rx_callback(CanFrame_t *frame) {
    BaseType_t woken = pdFALSE;
    xQueueSendFromISR(xCanRxQueue, frame, &woken);
    portYIELD_FROM_ISR(woken);
}

// VL53 GPIO1 interrupt
// Attached in vl53_drv.cpp via attachInterrupt().
// Timestamp is queued directly so each trip event carries its own value.
extern "C" void vl53_gpio_isr() {
    uint32_t ts = TIM2->CNT;   // latch FIRST — minimum latency path
    BaseType_t woken = pdFALSE;
    xQueueSendFromISR(xTripQueue, &ts, &woken);
    portYIELD_FROM_ISR(woken);
}

// Override the empty variant SystemClock_Config() to run at 48 MHz.
// STM32C092 has a 48 MHz HSI; the reset default divides it by 4 (12 MHz).
// Setting HSIDiv = DIV1 gives the full 48 MHz required for 500 kbps CAN
// (prescaler=6, seg1=12, seg2=3 → 48 MHz / (6×16) = 500 kbps).
extern "C" void SystemClock_Config() {
    RCC_OscInitTypeDef osc = {};
    osc.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    osc.HSIState            = RCC_HSI_ON;
    osc.HSIDiv              = RCC_HSI_DIV1;   // 48 MHz undivided
    osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    HAL_RCC_OscConfig(&osc);

    RCC_ClkInitTypeDef clk = {};
    clk.ClockType      = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                         RCC_CLOCKTYPE_PCLK1;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_HSI;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_1);   // 1 wait state required at 48 MHz
}

void setup() {
    // TIM2: 32-bit free-running counter at system clock (no prescaler).
    // Used for precise timestamp latching in the VL53 ISR.
    __HAL_RCC_TIM2_CLK_ENABLE();
    TIM2->PSC  = 0;
    TIM2->ARR  = 0xFFFFFFFF;
    TIM2->CR1 |= TIM_CR1_CEN;

    // IPC primitives
    xCanTxQueue = xQueueCreate(16, sizeof(CanFrame_t));
    xCanRxQueue = xQueueCreate(16, sizeof(CanFrame_t));
    xTripQueue  = xQueueCreate(4,  sizeof(uint32_t));
    xVL53Mutex  = xSemaphoreCreateMutex();

    // FDCAN peripheral — 500 kbps nominal / 2 Mbps data phase
    can_init(NODE_ID);

    // VL53L4CX — I2C address, XSHUT, interrupt pin
    vl53_init();

    // NeoPixel driver
    neo_init(NEO_DIN_PIN, NEO_NUM_LEDS);


    configASSERT(xCanTxQueue);
    configASSERT(xCanRxQueue);
    configASSERT(xTripQueue);
    configASSERT(xVL53Mutex);

    // ── Task creation ─────────────────────────────────────────
    // Stack sizes in words (4 bytes each on M0+).
    xTaskCreate(vVL53_ISR_Task,  "vl53isr",  256, NULL, 7, NULL);
    xTaskCreate(vCAN_RX_Task,    "canrx",     128, NULL, 6, NULL);
    xTaskCreate(vCAN_TX_Task,    "cantx",     128, NULL, 5, NULL);
    xTaskCreate(vVL53_Poll_Task, "vl53poll",  256, NULL, 4, NULL);
    xTaskCreate(vLED_Task,       "led",        128, NULL, 3, NULL);
    xTaskCreate(vHeartbeat_Task, "hb",         128, NULL, 2, NULL);
    xTaskCreate(vWatchdog_Task,  "wdog",        64, NULL, 1, NULL);

    vTaskStartScheduler();
    // Never reached
    for (;;) {}
}

void loop() {}

