// display_board/src/time_sync.cpp
//
// Time synchronisation for the display board.
// The master (NODE_ID == MASTER_DISPLAY_ID) is the timebase
// source — it never applies an offset to itself.
// A peer display (NODE_ID 0x02) uses the same offset algorithm
// as the sensor boards so its TIM2 ticks map to the master µs
// epoch for consistent cross-board timestamp comparison.

#include <Arduino.h>
#include <STM32FreeRTOS.h>
#include <task.h>
#include "time_sync.h"
#include "time_sync_math.h"
#include "display_board_config.h"
#include "can_protocol.h"   // for MASTER_DISPLAY_ID, NODE_ID

// Sync anchor: the master µs epoch value and corresponding local tick count
// captured at the most recent TIME_SYNC. Using a uint32_t tick anchor means
// uint32_t subtraction handles a single counter rollover (~59.7 s) correctly.
static volatile uint32_t sSyncTicks    = 0;
static volatile uint64_t sMasterUsAtSync = 0;
static volatile bool     sSyncReceived = false;

void time_sync_apply(uint64_t master_us) {
#if NODE_ID == MASTER_DISPLAY_ID
    (void)master_us;   // master never applies an offset from itself
#else
    uint32_t local_ticks = TIM2->CNT;
    // 64-bit writes are not atomic on M3
    taskENTER_CRITICAL();
    sSyncTicks      = local_ticks;
    sMasterUsAtSync = master_us;
    sSyncReceived   = true;
    taskEXIT_CRITICAL();
#endif
}

uint64_t ticks_to_master_us(uint32_t ticks) {
    taskENTER_CRITICAL();
    uint32_t sync_ticks     = sSyncTicks;
    uint64_t master_at_sync = sMasterUsAtSync;
    taskEXIT_CRITICAL();
    return master_us_from_anchor(master_at_sync, sync_ticks, ticks, TIM2_CLOCK_HZ);
}

uint64_t get_master_us() {
    return ticks_to_master_us(TIM2->CNT);
}

uint64_t time_since_last_sync_us() {
#if NODE_ID == MASTER_DISPLAY_ID
    return 0;   // master is always in sync with itself
#else
    taskENTER_CRITICAL();
    bool     synced     = sSyncReceived;
    uint32_t sync_ticks = sSyncTicks;
    taskEXIT_CRITICAL();
    if (!synced) return UINT64_MAX;
    // uint32_t subtraction wraps correctly across a counter rollover.
    uint32_t elapsed = TIM2->CNT - sync_ticks;
    return ticks_elapsed_to_us(elapsed, TIM2_CLOCK_HZ);
#endif
}

