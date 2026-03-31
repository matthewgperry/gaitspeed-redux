#pragma once
// sensor_board/include/sensor_board_config.h
//
// All hardware pin assignments and tuning constants for the
// =============================================================

// VL53L4CX
#define VL53_INT_PIN     PA0    // GPIO1 output — EXTI0, active low
#define VL53_XSHUT_PIN   PA1    // active low shutdown
#define VL53_I2C_ADDR    0x52   // default 7-bit address

// CAN FD
// PB1 = FDCAN1_TX (AF3), PB8 = FDCAN1_RX (AF8) — valid FDCAN pins on C092FBP.
// Note: PB9 is NOT a valid FDCAN pin on the C092FBP package.
#define CAN_TX_PIN       PB1
#define CAN_RX_PIN       PB8
#define CAN_NOMINAL_KBPS 500
#define CAN_DATA_MBPS    2

// NeoPixel (WS2812B)
#define NEO_DIN_PIN      PA7
#define NEO_NUM_LEDS     8

// Timing
// TIM2 runs at the core clock with PSC=0.
// Update TIM2_CLOCK_HZ if your clock tree differs.
#define TIM2_CLOCK_HZ    48000000UL   // HSI48 — set by SystemClock_Config() in main.cpp

// Detection thresholds
// Build-system overrides (from platformio.ini -D flags) take precedence;
// these are the compile-time defaults used if not overridden.
#ifndef TRIP_THRESHOLD_MM
#  define TRIP_THRESHOLD_MM   1500
#endif

#ifndef SENSOR_DISTANCE_MM
#  define SENSOR_DISTANCE_MM  4000
#endif

#ifndef MAX_VALID_TRANSIT_US
#  define MAX_VALID_TRANSIT_US 30000000UL
#endif

