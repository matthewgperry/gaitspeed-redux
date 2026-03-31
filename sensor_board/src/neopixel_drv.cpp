// sensor_board/src/neopixel_drv.cpp
//
// NeoPixel LED ring driver (thin Adafruit_NeoPixel wrapper).
// Provides neo_init(), neo_set_all(), neo_set(), and neo_push()
// so vLED_Task can update LED state without depending on the
// Adafruit library API directly.

#include <Adafruit_NeoPixel.h>
#include "neopixel_drv.h"

static Adafruit_NeoPixel *strip = nullptr;

void neo_init(uint8_t pin, uint8_t num_leds) {
    strip = new Adafruit_NeoPixel(num_leds, pin, NEO_GRB + NEO_KHZ800);
    strip->begin();
    strip->clear();
    strip->show();
}

void neo_set_all(uint8_t r, uint8_t g, uint8_t b) {
    if (!strip) return;
    strip->fill(strip->Color(r, g, b));
}

void neo_set(uint8_t idx, uint8_t r, uint8_t g, uint8_t b) {
    if (!strip) return;
    strip->setPixelColor(idx, strip->Color(r, g, b));
}

void neo_push() {
    if (!strip) return;
    strip->show();
}
