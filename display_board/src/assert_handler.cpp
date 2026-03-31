// display_board/src/assert_handler.cpp
//
// FreeRTOS assert hook.
// When configASSERT() fires, this halts execution with a rapid LED blink so the condition is visible
// on the bench.  The IWDG is left unserviced and resets the MCU
// after ~2 s.  Identical in behaviour to
// sensor_board/src/assert_handler.cpp.

#include <Arduino.h>
#include "STM32FreeRTOSConfig.h"
#include <STM32FreeRTOS.h>
#include <task.h>

extern "C" void vAssertCalled(const char *file, int line) {
    (void)file;
    (void)line;

    taskDISABLE_INTERRUPTS();

    pinMode(LED_BUILTIN, OUTPUT);
    while(1) {
        digitalWrite(LED_BUILTIN, HIGH);
        for (volatile uint32_t i = 0; i < 200000; i++) {}
        digitalWrite(LED_BUILTIN, LOW);
        for (volatile uint32_t i = 0; i < 200000; i++) {}
        // Since IWDG has not been pet, it will reset.
    }
}
