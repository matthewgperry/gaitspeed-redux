#pragma once
// include/can_protocol.h
//
// Shared between display_board and sensor_board.
// All multi-byte fields are little-endian.
// CAN 2.0B — 29-bit extended IDs, max 8-byte payloads.
// The display board (STM32F103) uses bxCAN (CAN 2.0A/B only).
// The sensor board (STM32C092) uses FDCAN in FDCAN_FRAME_CLASSIC mode.

#include <stdint.h>

// Node IDs
#define NODE_DISPLAY_A   0x01
#define NODE_DISPLAY_B   0x02
#define NODE_SENSOR_1    0x10
#define NODE_SENSOR_2    0x11
#define NODE_BROADCAST   0x3F   // 6-bit all-ones → broadcast

// Priority field (bits 28–26, 3 bits)
#define CAN_PRIO_HIGH    0x7    // sensor trips — time-critical
#define CAN_PRIO_MED     0x4    // results, display sync
#define CAN_PRIO_LOW     0x2    // heartbeats, time sync
#define CAN_PRIO_DIAG    0x1    // errors, debug

// Message type field (bits 25–18, 8 bits)
#define MSG_SENSOR_TRIP     0x01
#define MSG_SENSOR_STATUS   0x02
#define MSG_RESULT          0x10
#define MSG_DISPLAY_SYNC    0x20
#define MSG_CMD             0x30
#define MSG_TIME_SYNC       0x40
#define MSG_ERROR           0xFF

// 29-bit ID layout
// [28:26] priority  (3 bits)
// [25:18] msg_type  (8 bits)
// [17:12] src_node  (6 bits)
// [11:6]  dst_node  (6 bits)
// [5:0]   seq       (6 bits)

static inline uint32_t can_make_id(uint8_t prio, uint8_t type,
                                    uint8_t src,  uint8_t dst,
                                    uint8_t seq = 0) {
    return ((uint32_t)(prio & 0x07) << 26)
         | ((uint32_t)(type & 0xFF) << 18)
         | ((uint32_t)(src  & 0x3F) << 12)
         | ((uint32_t)(dst  & 0x3F) <<  6)
         | ((uint32_t)(seq  & 0x3F));
}

static inline uint8_t can_msg_type(uint32_t id) { return (id >> 18) & 0xFF; }
static inline uint8_t can_source  (uint32_t id) { return (id >> 12) & 0x3F; }
static inline uint8_t can_dest    (uint32_t id) { return (id >>  6) & 0x3F; }
static inline uint8_t can_seq     (uint32_t id) { return  id        & 0x3F; }
static inline uint8_t can_prio    (uint32_t id) { return (id >> 26) & 0x07; }

// Raw CAN frame (passed through FreeRTOS queues)
#define CAN_MAX_DLC 8   // CAN 2.0B max payload

typedef struct {
    uint32_t id;
    uint8_t  dlc;
    uint8_t  data[CAN_MAX_DLC];
} CanFrame_t;

// Payload structs — all __attribute__((packed))

// MSG_SENSOR_TRIP (0x01) — 8 bytes
// Sent by sensor board immediately on a beam crossing.
// timestamp_us is the TIM2 tick value converted to microseconds and
// corrected to the master timebase via the most recent TIME_SYNC offset.
// Sensor identity is carried in the CAN frame's source field (can_source(id)).
typedef struct __attribute__((packed)) {
    uint64_t timestamp_us;    // µs since master epoch, network-synced
} CanMsg_SensorTrip_t;

// MSG_SENSOR_STATUS (0x02) — 8 bytes
// Periodic heartbeat from sensor board to master.
typedef struct __attribute__((packed)) {
    uint8_t  sensor_id;
    uint8_t  vl53_state;      // VL53State_t enum value
    uint8_t  led_mode;        // LedMode_t enum value
    uint8_t  can_err_count;   // CAN bus error counter proxy (0–255)
    uint32_t uptime_s;
} CanMsg_SensorStatus_t;

// MSG_RESULT (0x10) — 8 bytes
// Computed gait measurement, broadcast by master after both trips received.
typedef struct __attribute__((packed)) {
    uint32_t gait_speed_mmps;     // mm per second; 0 = invalid trial
    uint16_t trial_number;
    uint8_t  flags;               // bit0 = valid result
    uint8_t  reserved;
} CanMsg_Result_t;

// MSG_DISPLAY_SYNC (0x20) — 8 bytes
// Broadcast by master after any state change so both screens stay identical.
typedef struct __attribute__((packed)) {
    uint32_t gait_speed_mmps;
    uint16_t trial_number;
    uint8_t  display_state;   // DisplayState_t enum value
    uint8_t  flags;           // bit0=sensor1_ok, bit1=sensor2_ok, bit2=master_ok
} CanMsg_DisplaySync_t;

// MSG_CMD (0x30) — 4 bytes
// Control commands from display master to sensor boards or peer display.
typedef struct __attribute__((packed)) {
    uint8_t  cmd_id;          // CmdId_t enum value
    uint8_t  param1;
    uint16_t param2;
} CanMsg_Cmd_t;

// MSG_TIME_SYNC (0x40) — 8 bytes
// Broadcast by master display every 1 s. Sensor boards apply offset correction.
typedef struct __attribute__((packed)) {
    uint64_t master_timestamp_us;
} CanMsg_TimeSync_t;

// MSG_ERROR (0xFF) — 8 bytes
typedef struct __attribute__((packed)) {
    uint8_t  source_node;
    uint8_t  error_code;      // ErrorCode_t enum value
    uint16_t error_detail;
    uint32_t context;
} CanMsg_Error_t;

// CMD IDs (used in CanMsg_Cmd_t.cmd_id)
typedef enum : uint8_t {
    CMD_ARM_SENSORS    = 0x01,   // tell sensor boards to start ranging
    CMD_DISARM_SENSORS = 0x02,   // stop ranging, LEDs to idle
    CMD_RESET_TRIAL    = 0x03,   // clear last result, return to READY
    CMD_ACTION_BUTTON  = 0x10,   // non-master display forwarding a button press
    CMD_SET_DISTANCE   = 0x20,   // update SENSOR_DISTANCE_MM at runtime (param2 = mm)
    CMD_REBOOT         = 0xF0,   // soft-reset target node (src must be master)
} CmdId_t;

// Error codes
typedef enum : uint8_t {
    ERR_NONE          = 0x00,
    ERR_VL53_INIT     = 0x01,
    ERR_VL53_TIMEOUT  = 0x02,
    ERR_CAN_BUS_OFF   = 0x10,
    ERR_CAN_TX_FULL   = 0x11,
    ERR_NODE_LOST     = 0x20,   // heartbeat timeout for a peer node
    ERR_TRIP_TIMEOUT  = 0x30,   // sensor 2 never fired after sensor 1
    ERR_INVALID_TRIAL = 0x31,   // computed speed outside plausible range
} ErrorCode_t;
