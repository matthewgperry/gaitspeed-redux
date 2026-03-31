// display_board/src/render.cpp
//
// ILI9341 display renderer (240×320 portrait).
// Draws the status bar (node-health indicators, trial counter),
// large m/s speed readout, ft/min secondary value, 10MWT clinical
// interpretation band, the action button, and hint text.
// Partial-region redraws are used throughout to avoid full-screen
// flicker at 30 fps over SPI.

#include <Adafruit_ILI9341.h>
#include <Adafruit_GFX.h>

#include "render.h"
#include "gait_types.h"
#include "display_board_config.h"
#include "can_protocol.h"

// Color palette
#define COL_BG          ILI9341_BLACK
#define COL_TEXT        ILI9341_WHITE
#define COL_MUTED       0x7BEF   // mid-grey
#define COL_GREEN       0x07E0
#define COL_RED         ILI9341_RED
#define COL_AMBER       0xFD20
#define COL_BTN_READY   0x07E0   // green  — tap to start
#define COL_BTN_ARMED   0xFD20   // amber  — tap to cancel
#define COL_BTN_TEXT    ILI9341_BLACK

// Action button geometry
#define BTN_X    20
#define BTN_Y   240
#define BTN_W   200
#define BTN_H    50
#define BTN_R     8

// Internal helpers
static void draw_status_bar(Adafruit_ILI9341 *tft,
                             uint16_t trial, uint8_t flags) {
    tft->fillRect(0, 0, TFT_WIDTH, 38, 0x1082);   // very dark blue strip

    // Node indicator dots — green if OK, red if lost
    uint16_t c1 = (flags & FLAG_SENSOR1_OK) ? COL_GREEN : COL_RED;
    uint16_t c2 = (flags & FLAG_SENSOR2_OK) ? COL_GREEN : COL_RED;
    tft->fillCircle(12, 19, 6, c1);
    tft->fillCircle(30, 19, 6, c2);

    tft->setTextColor(COL_MUTED);
    tft->setTextSize(1);
    tft->setCursor(42, 14);
    tft->print("S1");
    tft->setCursor(58, 14);
    tft->print("S2");

    // Trial counter right-aligned
    tft->setTextColor(COL_MUTED);
    tft->setCursor(160, 14);
    tft->print("Trial #");
    tft->print(trial);
}

static void draw_speed(Adafruit_ILI9341 *tft, const GaitResult_t *r) {
    tft->fillRect(0, 40, TFT_WIDTH, 120, COL_BG);

    if (!r->valid) {
        tft->setTextColor(COL_MUTED);
        tft->setTextSize(2);
        tft->setCursor(70, 90);
        tft->print("--.-");
    } else {
        // Large speed value in m/s
        char buf[16];
        snprintf(buf, sizeof(buf), "%.2f", r->speed_ms);

        tft->setTextColor(COL_TEXT);
        tft->setTextSize(5);
        // Rough centre: each char ≈ 30px wide at size 5
        int16_t xpos = (TFT_WIDTH - (int16_t)(strlen(buf) * 30)) / 2;
        tft->setCursor(xpos, 60);
        tft->print(buf);
    }

    // Unit label
    tft->setTextColor(COL_MUTED);
    tft->setTextSize(2);
    tft->setCursor(95, 140);
    tft->print("m / s");
}

static void draw_secondary(Adafruit_ILI9341 *tft, const GaitResult_t *r) {
    tft->fillRect(0, 160, TFT_WIDTH, 76, COL_BG);

    if (!r->valid) return;

    // Convert to ft/min for US clinical reference
    float ft_min = r->speed_ms * 196.85f;
    char buf[24];
    snprintf(buf, sizeof(buf), "%.0f ft/min", ft_min);

    tft->setTextColor(COL_MUTED);
    tft->setTextSize(2);
    int16_t xpos = (TFT_WIDTH - (int16_t)(strlen(buf) * 12)) / 2;
    tft->setCursor(xpos, 175);
    tft->print(buf);

    // Clinical interpretation (rough 10MWT bands)
    const char *interp;
    if      (r->speed_ms < 0.4f) interp = "Non-functional";
    else if (r->speed_ms < 0.8f) interp = "Household";
    else if (r->speed_ms < 1.2f) interp = "Limited community";
    else                          interp = "Community";

    tft->setTextColor(COL_MUTED);
    tft->setTextSize(1);
    xpos = (TFT_WIDTH - (int16_t)(strlen(interp) * 6)) / 2;
    tft->setCursor(xpos, 208);
    tft->print(interp);
}

static void draw_action_button(Adafruit_ILI9341 *tft, DisplayState_t state) {
    uint16_t col;
    const char *label;

    switch (state) {
        case DISPLAY_IDLE:
        case DISPLAY_RESULT:
            col   = COL_BTN_READY;
            label = "START";
            break;
        case DISPLAY_READY:
            col   = COL_BTN_ARMED;
            label = "CANCEL";
            break;
        case DISPLAY_ERROR:
            col   = COL_RED;
            label = "DISMISS";
            break;
        default:
            col   = COL_MUTED;
            label = "---";
            break;
    }

    tft->fillRoundRect(BTN_X, BTN_Y, BTN_W, BTN_H, BTN_R, col);

    tft->setTextColor(COL_BTN_TEXT);
    tft->setTextSize(3);
    int16_t xpos = BTN_X + (BTN_W - (int16_t)(strlen(label) * 18)) / 2;
    int16_t ypos = BTN_Y + (BTN_H - 24) / 2;
    tft->setCursor(xpos, ypos);
    tft->print(label);
}

static void draw_hint(Adafruit_ILI9341 *tft, DisplayState_t state,
                      const GaitResult_t *r) {
    tft->fillRect(0, 300, TFT_WIDTH, 20, COL_BG);
    tft->setTextSize(1);

    switch (state) {
        case DISPLAY_IDLE:
            tft->setTextColor(COL_MUTED);
            tft->setCursor(30, 305);
            tft->print("Press START or button to arm");
            break;
        case DISPLAY_READY:
            tft->setTextColor(COL_GREEN);
            tft->setCursor(55, 305);
            tft->print("Waiting for subject...");
            break;
        case DISPLAY_RESULT:
            tft->setTextColor(COL_MUTED);
            tft->setCursor(50, 305);
            tft->print("Press START for new trial");
            break;
        case DISPLAY_ERROR:
            tft->setTextColor(COL_RED);
            tft->setCursor(60, 305);
            tft->print("Check sensor connection");
            break;
        default:
            break;
    }
}

void render_screen(Adafruit_ILI9341 *tft, DisplayState_t state,
                   const GaitResult_t *r, uint8_t flags) {
    // Partial redraws only — avoid full fillScreen() each frame
    // to prevent flicker on slow SPI.  Each sub-region clears itself.
    draw_status_bar(tft, r->trial, flags);
    draw_speed(tft, r);
    draw_secondary(tft, r);
    draw_action_button(tft, state);
    draw_hint(tft, state, r);
}

bool render_hit_action_button(int16_t x, int16_t y) {
    return (x >= BTN_X && x <= BTN_X + BTN_W &&
            y >= BTN_Y && y <= BTN_Y + BTN_H);
}

