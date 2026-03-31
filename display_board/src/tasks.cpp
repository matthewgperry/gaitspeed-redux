// display_board/src/tasks.cpp
//
// All FreeRTOS task bodies for the display board.
// Covers: CAN RX routing, gait speed calculation (master only),
// CAN TX drain, touchscreen + UI state machine, time-sync
// broadcast (master only), buzzer sequencing, and
// watchdog/heartbeat monitoring.
// IPC handles are owned by main.cpp and extern'd here.

#include <Arduino.h>
#include <STM32FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>

#include <Adafruit_ILI9341.h>
#include <TouchScreen.h>

#include "can_protocol.h"
#include "gait_types.h"
#include "time_sync.h"
#include "display_board_config.h"
#include "ui_event.h"
#include "can_drv.h"
#include "touch.h"
#include "render.h"
#include "gait_calc.h"
#include "buzzer.h"

// IPC handles (owned by main.cpp)
extern QueueHandle_t     xCanTxQueue;
extern QueueHandle_t     xCanRxQueue;
extern QueueHandle_t     xUIEventQueue;
extern QueueHandle_t     xBuzzerQueue;
extern QueueHandle_t     xGaitCalcQueue;
extern SemaphoreHandle_t xResultMutex;

extern GaitResult_t    gResult;
extern DisplayState_t  gDisplayState;
extern uint16_t        gTrialNumber;
extern volatile uint8_t  gNodeFlags;
extern volatile uint32_t gLastHb[4];

// Helper
static void enqueue_tx(uint8_t msg_type, uint8_t prio, uint8_t dst,
                        const void *payload, uint8_t len) {
    CanFrame_t frame{};
    frame.id  = can_make_id(prio, msg_type, NODE_ID, dst);
    frame.dlc = len;
    memcpy(frame.data, payload, len);
    xQueueSend(xCanTxQueue, &frame, 0);
}

static void broadcast_display_sync(DisplayState_t state) {
    CanMsg_DisplaySync_t sync{};
    xSemaphoreTake(xResultMutex, portMAX_DELAY);
    sync.gait_speed_mmps = gResult.speed_mmps;
    sync.trial_number    = gTrialNumber;
    sync.display_state   = (uint8_t)state;
    sync.flags           = gNodeFlags;
    xSemaphoreGive(xResultMutex);

    enqueue_tx(MSG_DISPLAY_SYNC, CAN_PRIO_MED, NODE_BROADCAST, &sync, sizeof(sync));
}

static void broadcast_cmd(CmdId_t cmd) {
    CanMsg_Cmd_t msg{};
    msg.cmd_id = (uint8_t)cmd;
    enqueue_tx(MSG_CMD, CAN_PRIO_MED, NODE_BROADCAST, &msg, sizeof(msg));
}

// handle_action_button
// action button is either touchscreen or hardware button.
static void handle_action_button() {
    xSemaphoreTake(xResultMutex, portMAX_DELAY);
    DisplayState_t current = gDisplayState;
    DisplayState_t next    = current;
    CmdId_t        cmd     = CMD_ARM_SENSORS;
    bool           valid   = true;

    switch (current) {
        case DISPLAY_IDLE:
        case DISPLAY_RESULT:
            next = DISPLAY_READY;
            gTrialNumber++;
            cmd  = CMD_ARM_SENSORS;
            break;
        case DISPLAY_READY:
            next = DISPLAY_IDLE;
            cmd  = CMD_DISARM_SENSORS;
            break;
        case DISPLAY_ERROR:
            next = DISPLAY_IDLE;
            cmd  = CMD_DISARM_SENSORS;
            break;
        default:
            valid = false;
            break;
    }

    if (valid) gDisplayState = next;
    xSemaphoreGive(xResultMutex);

    if (!valid) return;
    broadcast_cmd(cmd);
    broadcast_display_sync(next);
}

// vCAN_RX_Task  pri 7
// Routes all incoming CAN frames to the appropriate handler.
void vCAN_RX_Task(void *) {
    CanFrame_t frame;
    for (;;) {
        xQueueReceive(xCanRxQueue, &frame, portMAX_DELAY);
        uint8_t type = can_msg_type(frame.id);
        uint8_t src  = can_source(frame.id);

        switch (type) {
            case MSG_SENSOR_TRIP:
                if (NODE_ID == MASTER_DISPLAY_ID) {
                    // Only master computes gait; peer waits for DISPLAY_SYNC.
                    if (xQueueSend(xGaitCalcQueue, &frame, pdMS_TO_TICKS(2)) != pdTRUE) {
                        xSemaphoreTake(xResultMutex, portMAX_DELAY);
                        gDisplayState = DISPLAY_ERROR;
                        xSemaphoreGive(xResultMutex);
                    }
                }
                // Both displays sound the trip beep
                {
                    BuzzerCmd_t beep{BEEP_SHORT, 880};
                    xQueueSend(xBuzzerQueue, &beep, 0);
                }
                break;

            case MSG_DISPLAY_SYNC: {
                auto *sync = (CanMsg_DisplaySync_t *) frame.data;
                xSemaphoreTake(xResultMutex, portMAX_DELAY);
                gResult.speed_mmps = sync->gait_speed_mmps;
                gResult.trial      = sync->trial_number;
                gResult.valid      = (sync->gait_speed_mmps > 0);
                gDisplayState      = (DisplayState_t)sync->display_state;
                gNodeFlags         = sync->flags;
                xSemaphoreGive(xResultMutex);
                break;
            }

            case MSG_SENSOR_STATUS: {
                uint8_t idx = src & 0x0F;   // 0x10→0, 0x11→1
                xSemaphoreTake(xResultMutex, portMAX_DELAY);
                if (idx < 4) gLastHb[idx] = (uint32_t)(millis());
                if (src == NODE_SENSOR_1)
                    gNodeFlags |= FLAG_SENSOR1_OK;
                else if (src == NODE_SENSOR_2)
                    gNodeFlags |= FLAG_SENSOR2_OK;
                xSemaphoreGive(xResultMutex);
                break;
            }

            case MSG_CMD: {
                // Non-master receives CMD_ACTION_BUTTON from peer
                auto *cmd = (CanMsg_Cmd_t *)frame.data;
                if (NODE_ID == MASTER_DISPLAY_ID &&
                    cmd->cmd_id == CMD_ACTION_BUTTON) {
                    handle_action_button();
                }
                break;
            }

            case MSG_ERROR: {
                BuzzerCmd_t alarm{BEEP_LONG, 440};
                xQueueSend(xBuzzerQueue, &alarm, 0);
                xSemaphoreTake(xResultMutex, portMAX_DELAY);
                gDisplayState = DISPLAY_ERROR;
                xSemaphoreGive(xResultMutex);
                break;
            }

            case MSG_TIME_SYNC:
                // Accept sync from master only
                // ignore own broadcast echo
                if (src != NODE_ID)
                    time_sync_apply(((CanMsg_TimeSync_t *)frame.data)->master_timestamp_us);
                break;

            default:
                break;
        }
    }
}

// vGaitCalc_Task  pri 6
// Collects SENSOR_TRIP frames from both sensors, computes speed,
// updates shared state, and broadcasts result + DISPLAY_SYNC.
// Only active on the master display (NODE_ID == MASTER_DISPLAY_ID).
void vGaitCalc_Task(void *) {
    if (NODE_ID != MASTER_DISPLAY_ID) {
        // Peer display: task exists but immediately suspends itself
        vTaskSuspend(NULL);
    }

    // Ring buffer for the two most recent trip frames (one per sensor)
    CanFrame_t trips[2];
    bool       got[2] = {false, false};

    CanFrame_t frame;
    for (;;) {
        xQueueReceive(xGaitCalcQueue, &frame, portMAX_DELAY);
        auto *trip = (CanMsg_SensorTrip_t *)frame.data;

        // Check we're in READY state
        // also capture gTrialNumber atomically.
        xSemaphoreTake(xResultMutex, portMAX_DELAY);
        DisplayState_t state       = gDisplayState;
        uint16_t       trial_snap  = gTrialNumber;
        xSemaphoreGive(xResultMutex);
        if (state != DISPLAY_READY) continue;

        uint8_t slot = (can_source(frame.id) == NODE_SENSOR_1) ? 0 : 1;
        trips[slot]  = frame;
        got[slot]    = true;

        if (!got[0] || !got[1]) continue;   // still waiting for the second sensor

        // Both trips received, compute
        got[0] = got[1] = false;

        auto *t1 = (CanMsg_SensorTrip_t *)trips[0].data;
        auto *t2 = (CanMsg_SensorTrip_t *)trips[1].data;

        GaitResult_t result = gait_calc_compute(
            t1->timestamp_us, t2->timestamp_us, trial_snap,
            SENSOR_DISTANCE_MM, MAX_VALID_TRANSIT_US,
            MIN_VALID_SPEED_MMPS, MAX_VALID_SPEED_MMPS);

        // Update shared state; capture next_state locally to avoid re-reading
        // gDisplayState after the mutex is released (DB-12).
        DisplayState_t next_state = result.valid ? DISPLAY_RESULT : DISPLAY_ERROR;
        xSemaphoreTake(xResultMutex, portMAX_DELAY);
        gResult       = result;
        gDisplayState = next_state;
        xSemaphoreGive(xResultMutex);

        // Broadcast RESULT frame (for logging / external observers)
        CanMsg_Result_t res_msg{};
        res_msg.gait_speed_mmps = result.speed_mmps;
        res_msg.trial_number    = result.trial;
        res_msg.flags           = result.valid ? 1u : 0u;
        enqueue_tx(MSG_RESULT, CAN_PRIO_MED, NODE_BROADCAST, &res_msg, sizeof(res_msg));

        // Broadcast DISPLAY_SYNC so peer display mirrors state immediately
        broadcast_display_sync(next_state);

        // Double-beep on valid result
        if (result.valid) {
            BuzzerCmd_t beep{BEEP_DOUBLE, 1000};
            xQueueSend(xBuzzerQueue, &beep, 0);
        }
    }
}

// vCAN_TX_Task  pri 5
void vCAN_TX_Task(void *) {
    CanFrame_t frame;
    for (;;) {
        xQueueReceive(xCanTxQueue, &frame, portMAX_DELAY);
        can_transmit(&frame);
    }
}

// vDisplay_Task  pri 4
// Renders the ILI9341 at 30 fps.  Polls touchscreen in the same
// task to serialise SPI access — no mutex required.
// Forwards button/touch actions to handle_action_button() on
// master, or sends CMD_ACTION_BUTTON over CAN on peer.
void vDisplay_Task(void *) {
    static Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);
    static TouchScreen       ts(TS_XP, TS_YP, TS_XM, TS_YM, TS_X_PLATE_OHMS);

    tft.begin();
    tft.setRotation(2);   // portrait, connector at bottom

    TickType_t last = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&last, pdMS_TO_TICKS(33));

        // ── Process UI events ────────────────────────────────
        UIEvent_t evt;
        while (xQueueReceive(xUIEventQueue, &evt, 0) == pdTRUE) {
            bool is_action = false;

            if (evt.type == UI_EVT_BTN_PRESS) {
                is_action = true;
            } else if (evt.type == UI_EVT_TOUCH) {
                is_action = render_hit_action_button(evt.x, evt.y);
            }

            if (is_action) {
                if (NODE_ID == MASTER_DISPLAY_ID) {
                    handle_action_button();
                } else {
                    // Forward to master via CAN
                    CanMsg_Cmd_t cmd{};
                    cmd.cmd_id = CMD_ACTION_BUTTON;
                    enqueue_tx(MSG_CMD, CAN_PRIO_MED,
                               MASTER_DISPLAY_ID, &cmd, sizeof(cmd));
                }
            }
        }

        // ── Poll touchscreen ─────────────────────────────────
        TouchPoint tp = touch_read(ts);
        if (tp.valid) {
            UIEvent_t tev{ UI_EVT_TOUCH, tp.x, tp.y };
            xQueueSend(xUIEventQueue, &tev, 0);
        }

        // ── Render ───────────────────────────────────────────
        xSemaphoreTake(xResultMutex, portMAX_DELAY);
        GaitResult_t   snap  = gResult;
        DisplayState_t state = gDisplayState;
        uint8_t        flags = gNodeFlags;
        xSemaphoreGive(xResultMutex);

        render_screen(&tft, state, &snap, flags);
    }
}

// vTimeSync_Task  pri 3
// Master display only: broadcasts TIME_SYNC every 1 s so all
// sensor boards can correct their TIM2 timestamps.
void vTimeSync_Task(void *) {
    if (NODE_ID != MASTER_DISPLAY_ID) {
        vTaskSuspend(NULL);
    }

    TickType_t last = xTaskGetTickCount();
    for (;;) {
        vTaskDelayUntil(&last, pdMS_TO_TICKS(1000));

        CanMsg_TimeSync_t ts{};
        ts.master_timestamp_us = get_master_us();
        enqueue_tx(MSG_TIME_SYNC, CAN_PRIO_LOW, NODE_BROADCAST, &ts, sizeof(ts));
    }
}

// vBuzzer_Task  pri 2
void vBuzzer_Task(void *) {
    BuzzerCmd_t cmd;
    for (;;) {
        xQueueReceive(xBuzzerQueue, &cmd, portMAX_DELAY);
        buzzer_play(cmd.freq_hz, cmd.type);
    }
}

// vWatchdog_Task  pri 1
// Pets IWDG and checks node heartbeats.
void vWatchdog_Task(void *) {
    TickType_t last = xTaskGetTickCount();
    for (;;) {
        vTaskDelayUntil(&last, pdMS_TO_TICKS(500));

        uint32_t now = (uint32_t)millis();

        // Check sensor heartbeats — clear OK flags if silent too long.
        // xResultMutex protects gNodeFlags and gLastHb per threading rules.
        xSemaphoreTake(xResultMutex, portMAX_DELAY);
        if ((now - gLastHb[0]) > NODE_HEARTBEAT_TIMEOUT_MS)
            gNodeFlags &= ~FLAG_SENSOR1_OK;
        if ((now - gLastHb[1]) > NODE_HEARTBEAT_TIMEOUT_MS)
            gNodeFlags &= ~FLAG_SENSOR2_OK;
        xSemaphoreGive(xResultMutex);

        IWDG->KR = 0xAAAA;
    }
}

