#pragma once
// lib/shared/include/time_sync.h
//
// Network time synchronisation interface.
// Implemented separately in sensor_board/src/time_sync.cpp and
// display_board/src/time_sync.cpp — the master (display A) is
// the epoch source; all other nodes apply an offset correction.

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Apply a TIME_SYNC message from the master.
// Computes and stores the local offset so that local_ticks_to_us()
// returns values on the master's timebase.
void     time_sync_apply(uint64_t master_us);

// Convert raw hardware timer ticks to microseconds on the master timebase.
// Uses the stored offset from the most recent time_sync_apply() call.
uint64_t ticks_to_master_us(uint32_t ticks);

// Return the current master-timebase timestamp in microseconds.
// On the master display this is simply TIM2 converted to µs.
// On sensor/peer boards this adds the stored correction offset.
uint64_t get_master_us(void);

// How many µs since the last TIME_SYNC was received.
// Returns UINT64_MAX if no sync has ever been received.
uint64_t time_since_last_sync_us(void);

#ifdef __cplusplus
}
#endif

