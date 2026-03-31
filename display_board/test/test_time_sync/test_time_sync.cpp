#include <unity.h>
#include "time_sync_math.h"

// Clock rate mirrored from display_board_config.h.
// Defined here so the test env doesn't pull in STM32 pin headers.
static constexpr uint32_t CLOCK_HZ = 72000000UL;

// At 72 MHz: 72 000 ticks = 1 000 µs exactly.
static constexpr uint32_t TICKS_PER_MS = 72000UL;

void setUp(void)    {}
void tearDown(void) {}

// ─── ticks_elapsed_to_us ─────────────────────────────────────

void test_zero_elapsed_is_zero_us() {
    TEST_ASSERT_EQUAL_UINT64(0, ticks_elapsed_to_us(0, CLOCK_HZ));
}

void test_one_millisecond_elapsed() {
    TEST_ASSERT_EQUAL_UINT64(1000, ticks_elapsed_to_us(TICKS_PER_MS, CLOCK_HZ));
}

void test_ten_milliseconds_elapsed() {
    TEST_ASSERT_EQUAL_UINT64(10000, ticks_elapsed_to_us(10 * TICKS_PER_MS, CLOCK_HZ));
}

void test_master_us_at_sync_point() {
    // When current == sync_ticks, elapsed == 0 and result == master_us_at_sync.
    TEST_ASSERT_EQUAL_UINT64(1000000,
        master_us_from_anchor(1000000, 0xABCD0000U, 0xABCD0000U, CLOCK_HZ));
}

void test_master_us_ten_ms_after_sync() {
    // Advance 10 ms worth of ticks with no rollover.
    uint32_t sync_ticks    = 100 * TICKS_PER_MS;
    uint32_t current_ticks = sync_ticks + 10 * TICKS_PER_MS;
    uint64_t master_at_sync = 5000000ULL;  // 5 s in µs

    TEST_ASSERT_EQUAL_UINT64(5010000ULL,
        master_us_from_anchor(master_at_sync, sync_ticks, current_ticks, CLOCK_HZ));
}

void test_master_us_after_counter_rollover() {
    constexpr uint32_t sync_ticks    = 0xFFFF0000U;
    constexpr uint32_t current_ticks = 0x0009FC80U;   // 654 464
    constexpr uint64_t master_at_sync = 1000000ULL;   // 1 s

    uint32_t elapsed = current_ticks - sync_ticks;
    TEST_ASSERT_EQUAL_UINT32(720000, elapsed);

    TEST_ASSERT_EQUAL_UINT64(1010000ULL,
        master_us_from_anchor(master_at_sync, sync_ticks, current_ticks, CLOCK_HZ));
}

void test_elapsed_since_sync_after_rollover() {
    constexpr uint32_t sync_ticks    = 0xFFFF0000U;
    constexpr uint32_t current_ticks = 0x0009FC80U;
    uint32_t elapsed_ticks = current_ticks - sync_ticks;  // uint32 wrap = 720 000
    TEST_ASSERT_EQUAL_UINT64(10000ULL,
        ticks_elapsed_to_us(elapsed_ticks, CLOCK_HZ));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_zero_elapsed_is_zero_us);
    RUN_TEST(test_one_millisecond_elapsed);
    RUN_TEST(test_ten_milliseconds_elapsed);
    RUN_TEST(test_master_us_at_sync_point);
    RUN_TEST(test_master_us_ten_ms_after_sync);
    RUN_TEST(test_master_us_after_counter_rollover);
    RUN_TEST(test_elapsed_since_sync_after_rollover);
    return UNITY_END();
}
