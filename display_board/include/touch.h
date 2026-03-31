#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <TouchScreen.h>

typedef struct {
    int16_t x;
    int16_t y;
    bool valid;     // is touch pressure in range
} TouchPoint;

TouchPoint touch_read(TouchScreen &ts);
