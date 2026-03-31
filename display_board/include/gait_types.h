#pragma once
// =============================================================
// lib/shared/include/gait_types.h
//
// Types shared across both firmware targets.
// No hardware-specific includes here.
// =============================================================

#include <stdint.h>
#include <stdbool.h>

// ─── Display state machine ────────────────────────────────────
typedef enum : uint8_t {
    DISPLAY_IDLE    = 0x00,   // waiting for user to start a trial
    DISPLAY_READY   = 0x01,   // armed — waiting for first sensor crossing
    DISPLAY_RESULT  = 0x02,   // showing last valid result
    DISPLAY_ERROR   = 0x03,   // fault condition
} DisplayState_t;

// ─── Gait result ─────────────────────────────────────────────
typedef struct {
    uint32_t speed_mmps;          // mm/s; 0 = invalid
    float    speed_ms;            // m/s, computed from speed_mmps
    uint16_t trial;
    bool     valid;
} GaitResult_t;

static inline void gait_result_compute_si(GaitResult_t *r) {
    r->speed_ms = r->speed_mmps / 1000.0f;
}

// ─── VL53 sensor state (reported in SENSOR_STATUS) ───────────
typedef enum : uint8_t {
    VL53_STATE_IDLE    = 0x00,
    VL53_STATE_RANGING = 0x01,
    VL53_STATE_TRIPPED = 0x02,
    VL53_STATE_ERROR   = 0xFF,
} VL53State_t;

// ─── NeoPixel LED modes ───────────────────────────────────────
typedef enum : uint8_t {
    LED_IDLE    = 0x00,   // slow blue pulse
    LED_READY   = 0x01,   // solid green
    LED_TRIPPED = 0x02,   // brief white flash
    LED_ERROR   = 0x03,   // fast red blink
} LedMode_t;

// ─── Buzzer commands ─────────────────────────────────────────
typedef enum : uint8_t {
    BEEP_SHORT  = 0x01,   // 100 ms — sensor trip acknowledgement
    BEEP_LONG   = 0x02,   // 500 ms — error alarm
    BEEP_DOUBLE = 0x03,   // two short beeps — trial complete
} BeepType_t;

typedef struct {
    BeepType_t type;
    uint16_t   freq_hz;
} BuzzerCmd_t;

// ─── Node status flags (used in DISPLAY_SYNC.flags) ──────────
#define FLAG_SENSOR1_OK  (1u << 0)
#define FLAG_SENSOR2_OK  (1u << 1)
#define FLAG_MASTER_OK   (1u << 2)

