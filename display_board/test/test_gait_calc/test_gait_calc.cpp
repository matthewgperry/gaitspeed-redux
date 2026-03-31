#include <unity.h>
#include "gait_calc.h"

// Configuration constants mirrored from display_board_config.h.
// Defined here so the test env doesn't pull in STM32 pin headers.
static constexpr uint32_t DIST_MM        = 4000;
static constexpr uint32_t MAX_TRANSIT_US = 30000000UL;
static constexpr uint32_t MIN_SPEED      = 100;
static constexpr uint32_t MAX_SPEED      = 5000;

void setUp(void) {}
void tearDown(void) {}

// ─── Normal walking speeds ────────────────────────────────────

void test_normal_walk_1ms() {
    // 4000 mm / 4 000 000 µs = 1000 mm/s = 1.0 m/s
    GaitResult_t r = gait_calc_compute(0, 4000000ULL, 1,
                                        DIST_MM, MAX_TRANSIT_US,
                                        MIN_SPEED, MAX_SPEED);
    TEST_ASSERT_TRUE(r.valid);
    TEST_ASSERT_EQUAL_UINT32(1000, r.speed_mmps);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, r.speed_ms);
}

void test_slow_walk_half_ms() {
    // 4000 mm / 8 000 000 µs = 500 mm/s = 0.5 m/s
    GaitResult_t r = gait_calc_compute(0, 8000000ULL, 2,
                                        DIST_MM, MAX_TRANSIT_US,
                                        MIN_SPEED, MAX_SPEED);
    TEST_ASSERT_TRUE(r.valid);
    TEST_ASSERT_EQUAL_UINT32(500, r.speed_mmps);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, r.speed_ms);
}

void test_fast_walk_boundary_5ms() {
    // 4000 mm / 800 000 µs = 5000 mm/s = 5.0 m/s (upper bound, inclusive)
    GaitResult_t r = gait_calc_compute(0, 800000ULL, 3,
                                        DIST_MM, MAX_TRANSIT_US,
                                        MIN_SPEED, MAX_SPEED);
    TEST_ASSERT_TRUE(r.valid);
    TEST_ASSERT_EQUAL_UINT32(5000, r.speed_mmps);
}

void test_trial_number_preserved() {
    GaitResult_t r = gait_calc_compute(0, 4000000ULL, 42,
                                        DIST_MM, MAX_TRANSIT_US,
                                        MIN_SPEED, MAX_SPEED);
    TEST_ASSERT_EQUAL_UINT16(42, r.trial);
}

// ─── Timestamps can arrive in either order ────────────────────

void test_reversed_timestamps_same_speed() {
    // sensor 2 fires first — delta must be taken as absolute value
    GaitResult_t r = gait_calc_compute(4000000ULL, 0, 1,
                                        DIST_MM, MAX_TRANSIT_US,
                                        MIN_SPEED, MAX_SPEED);
    TEST_ASSERT_TRUE(r.valid);
    TEST_ASSERT_EQUAL_UINT32(1000, r.speed_mmps);
}

// ─── Invalid / out-of-range results ──────────────────────────

void test_too_fast_is_invalid() {
    // 4000 mm / 100 µs = 40 000 000 mm/s — way above 5 m/s limit
    GaitResult_t r = gait_calc_compute(0, 100ULL, 1,
                                        DIST_MM, MAX_TRANSIT_US,
                                        MIN_SPEED, MAX_SPEED);
    TEST_ASSERT_FALSE(r.valid);
    TEST_ASSERT_EQUAL_UINT32(0, r.speed_mmps);
}

void test_too_slow_is_invalid() {
    // 4000 mm / 50 000 000 µs = 80 mm/s — below 0.1 m/s (100 mm/s) limit
    GaitResult_t r = gait_calc_compute(0, 50000000ULL, 1,
                                        DIST_MM, MAX_TRANSIT_US,
                                        MIN_SPEED, MAX_SPEED);
    TEST_ASSERT_FALSE(r.valid);
}

void test_transit_timeout_is_invalid() {
    // exactly at the MAX_VALID_TRANSIT_US boundary — must be rejected
    GaitResult_t r = gait_calc_compute(0, MAX_TRANSIT_US, 1,
                                        DIST_MM, MAX_TRANSIT_US,
                                        MIN_SPEED, MAX_SPEED);
    TEST_ASSERT_FALSE(r.valid);
}

void test_zero_delta_is_invalid() {
    // both sensors report the same timestamp — division by zero guard
    GaitResult_t r = gait_calc_compute(1000000ULL, 1000000ULL, 1,
                                        DIST_MM, MAX_TRANSIT_US,
                                        MIN_SPEED, MAX_SPEED);
    TEST_ASSERT_FALSE(r.valid);
}

// ─────────────────────────────────────────────────────────────
int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_normal_walk_1ms);
    RUN_TEST(test_slow_walk_half_ms);
    RUN_TEST(test_fast_walk_boundary_5ms);
    RUN_TEST(test_trial_number_preserved);
    RUN_TEST(test_reversed_timestamps_same_speed);
    RUN_TEST(test_too_fast_is_invalid);
    RUN_TEST(test_too_slow_is_invalid);
    RUN_TEST(test_transit_timeout_is_invalid);
    RUN_TEST(test_zero_delta_is_invalid);
    return UNITY_END();
}
