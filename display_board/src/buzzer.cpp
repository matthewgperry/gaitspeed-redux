// display_board/src/buzzer.cpp
//
// Buzzer output driver.
// Generates short (100 ms), long (500 ms), and double-beep tones
// on BUZZER_PIN (TIM3_CH4 PWM) via the Arduino tone() API.
// Called by vBuzzer_Task, which drains commands from xBuzzerQueue.

#include <Arduino.h>
#include <STM32FreeRTOS.h>
#include <task.h>

#include "buzzer.h"
#include "FreeRTOS/Source/include/projdefs.h"
#include "display_board_config.h"
#include "gait_types.h"

static void play_tone(uint16_t freq_hz, uint32_t duration_ms) {
    if (freq_hz == 0) return;
    tone(BUZZER_PIN, freq_hz, duration_ms);
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    noTone(BUZZER_PIN);
}

void buzzer_play(uint16_t freq_hz, BeepType_t type) {
    switch (type) {
        case BEEP_SHORT:
            play_tone(freq_hz, 100);
            break;
        case BEEP_LONG:
            play_tone(freq_hz, 500);
            break;
        case BEEP_DOUBLE:
            play_tone(freq_hz, 100);
            vTaskDelay(pdMS_TO_TICKS(80));
            play_tone(freq_hz, 100);
            break;
        default:
            break;
    }
}
