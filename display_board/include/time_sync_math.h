#pragma once
#include <stdint.h>

// Convert a number of elapsed timer ticks to microseconds.
static inline uint64_t ticks_elapsed_to_us(uint32_t elapsed_ticks, uint32_t clock_hz) {
    return ((uint64_t)elapsed_ticks * 1000000ULL) / clock_hz;
}

// Return the master-timebase µs timestamp for a given hardware tick value,
// given a sync anchor (sync_ticks, master_us_at_sync).
//
// Uses uint32_t subtraction so a single counter rollover (~59.7 s at 72 MHz)
// is handled correctly: the elapsed tick count wraps safely in 32-bit
// arithmetic before being widened to 64-bit microseconds.
static inline uint64_t master_us_from_anchor(uint64_t master_us_at_sync,
                                              uint32_t sync_ticks,
                                              uint32_t current_ticks,
                                              uint32_t clock_hz) {
    uint32_t elapsed = current_ticks - sync_ticks;   // wraps correctly at rollover
    return master_us_at_sync + ticks_elapsed_to_us(elapsed, clock_hz);
}
