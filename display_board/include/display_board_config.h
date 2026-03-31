#pragma once

#define TFT_CS    PB12
#define TFT_DC    PB10
#define TFT_RST   PB11
#define TFT_WIDTH  240
#define TFT_HEIGHT 320

// Resistive touchscreen (Adafruit TouchScreen library)
// Four ADC-capable pins — two driven as outputs, two read as ADC.
#define TS_XP     PA6    // X+  — ADC input
#define TS_XM     PA5    // X-  — driven output (may share SPI MOSI)
#define TS_YP     PA4    // Y+  — driven output (may share SPI CS)
#define TS_YM     PA3    // Y-  — ADC input

// Calibration constants.
// To update: measure with the Adafruit calibration sketch and replace these values for your specific panel.
#define TS_MINX   150
#define TS_MAXX   920
#define TS_MINY   120
#define TS_MAXY   940
#define TS_X_PLATE_OHMS  300   // X-plate resistance in ohms

// Pressure thresholds — reject phantom touches
#define TS_MIN_PRESSURE   10
#define TS_MAX_PRESSURE  1000

// Hardware action button
#define BTN_PIN          PB12   // pulled high, active low — EXTI12
#define BTN_DEBOUNCE_MS  50

// Buzzer
#define BUZZER_PIN       PB1    // TIM3_CH4 PWM

// CAN
#define CAN_TX_PIN       PA12
#define CAN_RX_PIN       PA11
#define CAN_NOMINAL_KBPS 500
#define CAN_DATA_MBPS    2

// Timing
#define TIM2_CLOCK_HZ    72000000UL   // STM32F103 at 72 MHz PLL

// Gait calculation limits
#ifndef SENSOR_DISTANCE_MM
#  define SENSOR_DISTANCE_MM  4000
#endif

#ifndef MAX_VALID_TRANSIT_US
#  define MAX_VALID_TRANSIT_US 30000000UL
#endif

// Plausibility check: reject results outside 0.1 – 5.0 m/s
#define MIN_VALID_SPEED_MMPS   100
#define MAX_VALID_SPEED_MMPS  5000

// Heartbeat timeout
// if a node misses this many 500 ms periods, flag it
#define NODE_HEARTBEAT_TIMEOUT_MS  2000

