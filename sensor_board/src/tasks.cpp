// sensor_board/src/tasks.cpp
//
// All FreeRTOS task bodies for the sensor board.
// IPC handles are defined in main.cpp and declared extern here.

#include <Arduino.h>
#include <STM32FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>
#include <vl53l4cx_class.h>

#include "can_protocol.h"
#include "gait_types.h"
#include "time_sync.h"
#include "sensor_board_config.h"
#include "can_drv.h"
#include "vl53_drv.h"
#include "neopixel_drv.h"

// IPC handles (owned by main.cpp)
extern QueueHandle_t     xCanTxQueue;
extern QueueHandle_t     xCanRxQueue;
extern QueueHandle_t     xTripQueue;
extern SemaphoreHandle_t xVL53Mutex;

extern volatile LedMode_t  gLedMode;
extern volatile VL53State_t gVL53State;

// Helper: build and enqueue a CAN frame
static void enqueue_tx(uint8_t msg_type, uint8_t prio,
                        const void *payload, uint8_t len) {
    CanFrame_t frame{};
    frame.id  = can_make_id(prio, msg_type, NODE_ID, NODE_BROADCAST);
    frame.dlc = len;
    memcpy(frame.data, payload, len);
    // Non-blocking: if TX queue is full we drop rather than block a high-pri task
    xQueueSend(xCanTxQueue, &frame, 0);
}

// vVL53_ISR_Task  pri 7
// Woken by the GPIO EXTI ISR via xTripQueue.
// Reads the I2C result, validates, and sends SENSOR_TRIP.
void vVL53_ISR_Task(void *) {
    uint32_t ts_ticks;
    for (;;) {
        // Timestamp is carried in the queue item — no shared global needed.
        xQueueReceive(xTripQueue, &ts_ticks, portMAX_DELAY);

        // Static avoids a ~300-byte stack frame; safe because vVL53_ISR_Task
        // is the only caller and it never runs concurrently with itself.
        static VL53L4CX_MultiRangingData_t data;
        data = {};

        xSemaphoreTake(xVL53Mutex, portMAX_DELAY);
        vl53_get_data(&data);          // reads result registers over I2C
        vl53_clear_interrupt();        // re-arm for next measurement
        gVL53State = VL53_STATE_TRIPPED;
        xSemaphoreGive(xVL53Mutex);

        // Pick the closest valid object in the multi-ranging result.
        int16_t best_dist = INT16_MAX;
        for (int i = 0; i < data.NumberOfObjectsFound; i++) {
            auto &obj = data.RangeData[i];
            if (obj.RangeStatus == 0 && obj.RangeMilliMeter < best_dist)
                best_dist = obj.RangeMilliMeter;
        }

        if (best_dist > TRIP_THRESHOLD_MM) {
            // Object too far away — not a valid crossing; re-arm and continue.
            xSemaphoreTake(xVL53Mutex, portMAX_DELAY);
            gVL53State = VL53_STATE_RANGING;
            xSemaphoreGive(xVL53Mutex);
            continue;
        }

        // Build and send SENSOR_TRIP.
        // Sensor identity is implicit in the CAN frame's source field (NODE_ID).
        CanMsg_SensorTrip_t trip{};
        trip.timestamp_us = ticks_to_master_us(ts_ticks);

        // Use finite timeout — SENSOR_TRIP must not be silently dropped.
        CanFrame_t tx_frame{};
        tx_frame.id  = can_make_id(CAN_PRIO_HIGH, MSG_SENSOR_TRIP, NODE_ID, NODE_BROADCAST);
        tx_frame.dlc = sizeof(trip);
        memcpy(tx_frame.data, &trip, sizeof(trip));
        xQueueSend(xCanTxQueue, &tx_frame, pdMS_TO_TICKS(5));

        // Update LED
        gLedMode = LED_TRIPPED;
    }
}

// vCAN_TX_Task  pri 6
// Drains xCanTxQueue to the FDCAN peripheral.
void vCAN_TX_Task(void *) {
    CanFrame_t frame;
    for (;;) {
        xQueueReceive(xCanTxQueue, &frame, portMAX_DELAY);
        can_transmit(&frame);
    }
}

// vVL53_Poll_Task  pri 5
// Ensures the sensor stays in ranging mode and handles the
// poll-mode fallback path (ISR is primary; this is belt-and-braces).
void vVL53_Poll_Task(void *) {
    TickType_t last = xTaskGetTickCount();
    for (;;) {
        vTaskDelayUntil(&last, pdMS_TO_TICKS(10));

        xSemaphoreTake(xVL53Mutex, portMAX_DELAY);
        VL53State_t state = gVL53State;

        if (state == VL53_STATE_IDLE) {
            vl53_start_ranging();
            gVL53State = VL53_STATE_RANGING;
            xSemaphoreGive(xVL53Mutex);
        } else if (state == VL53_STATE_TRIPPED) {
            // ISR task has already read the result; return to ranging.
            gVL53State = VL53_STATE_RANGING;
            xSemaphoreGive(xVL53Mutex);
        } else if (state == VL53_STATE_ERROR) {
            // Release mutex before vl53_reset() which calls vTaskDelay().
            gLedMode   = LED_ERROR;
            gVL53State = VL53_STATE_IDLE;
            xSemaphoreGive(xVL53Mutex);
            vl53_reset();
        } else {
            xSemaphoreGive(xVL53Mutex);
        }
    }
}

// vCAN_RX_Task  pri 4
// Handles incoming TIME_SYNC and CMD messages.
void vCAN_RX_Task(void *) {
    CanFrame_t frame;
    for (;;) {
        xQueueReceive(xCanRxQueue, &frame, portMAX_DELAY);

        uint8_t type = can_msg_type(frame.id);
        switch (type) {
            case MSG_TIME_SYNC: {
                auto *ts = (CanMsg_TimeSync_t *)frame.data;
                time_sync_apply(ts->master_timestamp_us);
                break;
            }
            case MSG_CMD: {
                auto *cmd = (CanMsg_Cmd_t *)frame.data;
                switch (cmd->cmd_id) {
                    case CMD_ARM_SENSORS:
                        xSemaphoreTake(xVL53Mutex, portMAX_DELAY);
                        vl53_start_ranging();
                        gVL53State = VL53_STATE_RANGING;
                        gLedMode   = LED_READY;
                        xSemaphoreGive(xVL53Mutex);
                        break;
                    case CMD_DISARM_SENSORS:
                        xSemaphoreTake(xVL53Mutex, portMAX_DELAY);
                        vl53_stop_ranging();
                        gVL53State = VL53_STATE_IDLE;
                        gLedMode   = LED_IDLE;
                        xSemaphoreGive(xVL53Mutex);
                        break;
                    case CMD_REBOOT:
                        NVIC_SystemReset();
                        break;
                    default:
                        break;
                }
                break;
            }
            default:
                break;
        }
    }
}

// vLED_Task  pri 3
// Updates the NeoPixel ring every 20 ms based on gLedMode.
void vLED_Task(void *) {
    TickType_t last = xTaskGetTickCount();
    uint32_t   tick = 0;

    for (;;) {
        vTaskDelayUntil(&last, pdMS_TO_TICKS(20));
        tick++;

        LedMode_t mode = gLedMode;   // atomic 8-bit read

        switch (mode) {
            case LED_IDLE:
                // Slow blue breathing: sin approximation via tick counter
                neo_set_all(0, 0, (uint8_t)(64 + 32 * ((tick & 0x1F) < 16
                                                          ? (tick & 0x1F)
                                                          : 32 - (tick & 0x1F))));
                break;
            case LED_READY:
                neo_set_all(0, 80, 0);   // solid green
                break;
            case LED_TRIPPED:
                neo_set_all(80, 80, 80); // white flash
                // Auto-decay back to READY after ~200 ms (10 LED frames)
                if ((tick & 0x0F) == 0x0F) gLedMode = LED_READY;
                break;
            case LED_ERROR:
                // Fast red blink: toggle every 10 frames (200 ms)
                neo_set_all((tick & 0x0F) < 8 ? 80 : 0, 0, 0);
                break;
        }
        neo_push();
    }
}

// vHeartbeat_Task  pri 2
// Sends SENSOR_STATUS to master every 500 ms.
void vHeartbeat_Task(void *) {
    TickType_t last = xTaskGetTickCount();
    for (;;) {
        vTaskDelayUntil(&last, pdMS_TO_TICKS(500));

        CanMsg_SensorStatus_t st{};
        st.sensor_id  = NODE_ID;
        st.vl53_state = (uint8_t)gVL53State;
        st.led_mode   = (uint8_t)gLedMode;
        st.uptime_s   = (uint32_t)(millis() / 1000UL);

        enqueue_tx(MSG_SENSOR_STATUS, CAN_PRIO_LOW, &st, sizeof(st));
    }
}

// vWatchdog_Task  pri 1
// Pets the IWDG every 500 ms.  Timeout is configured for 2 s
// so one missed pet is survivable; two consecutive misses reset.
void vWatchdog_Task(void *) {
    // IWDG configured for ~2 s timeout in can_drv.cpp / HAL init.
    TickType_t last = xTaskGetTickCount();
    for (;;) {
        vTaskDelayUntil(&last, pdMS_TO_TICKS(500));
        IWDG->KR = 0xAAAA;
    }
}

