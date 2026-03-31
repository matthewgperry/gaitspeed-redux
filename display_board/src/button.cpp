// display_board/src/button.cpp
//
// Hardware action button driver.
// Attaches an EXTI interrupt to BTN_PIN (active-low), debounces
// via a raw-event queue + timer, and forwards UI_EVT_BTN_PRESS /
// UI_EVT_BTN_RELEASE to xUIEventQueue for vDisplay_Task.

#include <Arduino.h>
#include <STM32FreeRTOS.h>
#include <cstdint>
#include <task.h>
#include <queue.h>

#include "display_board_config.h"
#include "ui_event.h"

static QueueHandle_t xBtnRawQueue;

static void btn_isr() {
    BaseType_t woken = pdFALSE;
    uint8_t dummy = 1;
    xQueueSendFromISR(xBtnRawQueue, &dummy, &woken);
    portYIELD_FROM_ISR(woken);
}

static void vButton_Task(void *) {
    uint8_t dummy;
    TickType_t last_confirmed = 0;

    for (;;) {
        xQueueReceive(xBtnRawQueue, &dummy, portMAX_DELAY);

        while (xQueueReceive(xBtnRawQueue, &dummy, pdMS_TO_TICKS(BTN_DEBOUNCE_MS)) == pdTRUE) {
        }

        TickType_t now = xTaskGetTickCount();
        if ((now - last_confirmed) < pdMS_TO_TICKS(BTN_DEBOUNCE_MS)) {
            continue;
        }
        last_confirmed = now;

        bool pressed = (digitalRead(BTN_PIN) == LOW);
        UIEvent_t evt{};
        evt.type = pressed ? UI_EVT_BTN_PRESS : UI_EVT_BTN_RELEASE;
        xQueueSend(xUIEventQueue, &evt, 0);
    }
}

void button_init() {
    xBtnRawQueue = xQueueCreate(8, sizeof(uint8_t));
    configASSERT(xBtnRawQueue);

    pinMode(BTN_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BTN_PIN), btn_isr, CHANGE);

    xTaskCreate(vButton_Task, "btn", 64, NULL, 5, NULL);
}
