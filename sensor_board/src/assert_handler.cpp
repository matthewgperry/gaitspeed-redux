// sensor_board/src/assert_handler.cpp
// display_board/src/assert_handler.cpp   (identical file, both boards)
//
// configASSERT() calls this function on failure.
// Disables interrupts, blinks the onboard LED rapidly, and spins
// so the IWDG resets the MCU after ~2 s.  In a debug build with
// SWD attached, the debugger will halt here before reset.

#include <Arduino.h>
#include <FreeRTOS.h>         // includes FreeRTOSConfig.h; defines portDISABLE_INTERRUPTS
#include <task.h>             // taskDISABLE_INTERRUPTS

extern "C" void vAssertCalled(const char *file, int line) {
    (void)file;
    (void)line;

    taskDISABLE_INTERRUPTS();

    // Rapid LED blink to signal assert trap visually
    pinMode(LED_BUILTIN, OUTPUT);
    while (1) {
        digitalWrite(LED_BUILTIN, HIGH);
        for (volatile uint32_t i = 0; i < 200000; i++) {}
        digitalWrite(LED_BUILTIN, LOW);
        for (volatile uint32_t i = 0; i < 200000; i++) {}
        // IWDG will reset after ~2 s of no pet
    }
}

