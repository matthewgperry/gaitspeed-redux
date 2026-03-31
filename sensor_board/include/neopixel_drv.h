#pragma once
// sensor_board/include/neopixel_drv.h
#include <stdint.h>

void neo_init(uint8_t pin, uint8_t num_leds);
void neo_set_all(uint8_t r, uint8_t g, uint8_t b);
void neo_set(uint8_t idx, uint8_t r, uint8_t g, uint8_t b);
void neo_push();   // Blocking bit-bang transfer (~2.4 ms for 8 LEDs)

