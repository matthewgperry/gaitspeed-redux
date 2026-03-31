#include <cstdint>
#include <stdint.h>
#include <STM32FreeRTOS.h>
#include <queue.h>

typedef enum : uint8_t {
    UI_EVT_BTN_PRESS,   // hardawre button pressed
    UI_EVT_BTN_RELEASE, // hardware button released
    UI_EVT_TOUCH,       // touchscreen tap
    UI_EVT_CAN_STATE,   // display state update arrived via CAN DISPLAY_SYNC
} UIEventType_t;

typedef struct {
    UIEventType_t type;
    int16_t x;      // valid for UI_EVT_TOUCH only
    int16_t y;      // valid for UI_EVT_TOUCH only
} UIEvent_t;

extern QueueHandle_t xUIEventQueue;
