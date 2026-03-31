#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <Adafruit_ILI9341.h>
#include "gait_types.h"

/**
 * Render the full screen for the current state.
 * Called every 33 ms from vDisplay_Task.
 */
void render_screen(Adafruit_ILI9341 *tft, DisplayState_t state,
                   const GaitResult_t *r, uint8_t node_flags);

// Returns true if pixel (x, y) falls inside the action button rect.
bool render_hit_action_button(int16_t x, int16_t y);
