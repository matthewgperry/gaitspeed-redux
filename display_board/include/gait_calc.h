#pragma once
#include "gait_types.h"

// gait_calc_compute
//
// Takes the two master-timebase timestamps (µs) from both sensor
// trips, the trial number, and the system config limits.
// Returns a populated GaitResult_t
// result.valid is false when the speed falls outside the plausible range or the transit
// time is zero / exceeds max_transit_us.
// =============================================================
static inline GaitResult_t gait_calc_compute(
        uint64_t ts1_us, uint64_t ts2_us, uint16_t trial,
        uint32_t distance_mm,
        uint32_t max_transit_us,
        uint32_t min_speed_mmps,
        uint32_t max_speed_mmps)
{
    GaitResult_t result{};
    result.trial = trial;

    int64_t delta_us = (int64_t)ts2_us - (int64_t)ts1_us;
    if (delta_us < 0) delta_us = -delta_us;

    if (delta_us > 0 && delta_us < (int64_t)max_transit_us) {
        uint32_t speed = (uint32_t)(
            (uint64_t)distance_mm * 1000000ULL / (uint64_t)delta_us);

        if (speed >= min_speed_mmps && speed <= max_speed_mmps) {
            result.speed_mmps = speed;
            result.speed_ms   = speed / 1000.0f;
            result.valid      = true;
        }
    }

    return result;
}
