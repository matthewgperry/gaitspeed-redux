// sensor_board/src/time_sync.cpp
//
// Time synchronisation for sensor boards.
// The master display broadcasts TIME_SYNC every 1 s.
// We compute an offset so our TIM2 ticks map to the master µs epoch.

#include <Arduino.h>
#include <STM32FreeRTOS.h>
#include <task.h>
#include "time_sync.h"
#include "sensor_board_config.h"

static volatile int64_t  sOffsetUs       = 0;
static volatile uint64_t sLastSyncUs     = 0;   // local µs at last sync
static volatile bool     sSyncReceived   = false;

// Convert raw TIM2 ticks to µs (without offset correction).
static uint64_t raw_ticks_to_us(uint32_t ticks) {
    // 64-bit multiply then divide — avoids overflow for large tick values.
    return ((uint64_t)ticks * 1000000ULL) / TIM2_CLOCK_HZ;
}

void time_sync_apply(uint64_t master_us) {
    uint32_t local_ticks = TIM2->CNT;
    uint64_t local_us    = raw_ticks_to_us(local_ticks);

    // 64-bit writes are not atomic on M0+
    taskENTER_CRITICAL();
    sOffsetUs     = (int64_t)master_us - (int64_t)local_us;
    sLastSyncUs   = local_us;
    sSyncReceived = true;
    taskEXIT_CRITICAL();
}

uint64_t ticks_to_master_us(uint32_t ticks) {
    taskENTER_CRITICAL();
    int64_t off = sOffsetUs;
    taskEXIT_CRITICAL();
    return (uint64_t)((int64_t)raw_ticks_to_us(ticks) + off);
}

uint64_t get_master_us() {
    return ticks_to_master_us(TIM2->CNT);
}

uint64_t time_since_last_sync_us() {
    taskENTER_CRITICAL();
    bool     synced = sSyncReceived;
    uint64_t last   = sLastSyncUs;
    taskEXIT_CRITICAL();
    if (!synced) return UINT64_MAX;
    uint64_t now_local = raw_ticks_to_us(TIM2->CNT);
    return now_local - last;
}

