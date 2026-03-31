// display_board/src/touch.cpp
//
// Resistive touchscreen input driver.
// Reads the four-wire panel via the Adafruit TouchScreen library,
// filters by pressure threshold, maps raw ADC values to pixel
// coordinates, and clamps to screen bounds.
// Restores SPI-shared pins (TS_YP, TS_XM) after every read.
// Called by vDisplay_Task at ~30 fps.

#include <Arduino.h>
#include <TouchScreen.h>
#include "touch.h"
#include "display_board_config.h"

TouchPoint touch_read(TouchScreen &ts) {
    TSPoint raw = ts.getPoint();

    // Restore SPI-shared pins to OUTPUT regardless of share status.
    // Cost: two pinMode() calls (~1 µs each).  Always safe.
    pinMode(TS_YP, OUTPUT);
    pinMode(TS_XM, OUTPUT);

    TouchPoint pt{};

    if (raw.z < TS_MIN_PRESSURE || raw.z > TS_MAX_PRESSURE) {
        pt.valid = false;
        return pt;
    }

    // Map raw ADC → pixel coordinates.
    // The axis mapping may need swapping or inverting depending on
    // panel orientation relative to the display rotation set in
    // vDisplay_Task (tft.setRotation(2) = portrait, connector bottom).
    pt.x = map(raw.x, TS_MINX, TS_MAXX, 0, TFT_WIDTH  - 1);
    pt.y = map(raw.y, TS_MINY, TS_MAXY, 0, TFT_HEIGHT - 1);

    // Clamp to screen bounds
    pt.x = constrain(pt.x, 0, TFT_WIDTH  - 1);
    pt.y = constrain(pt.y, 0, TFT_HEIGHT - 1);
    pt.valid = true;
    return pt;
}

