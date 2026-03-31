#include <unity.h>
#include "can_protocol.h"

void setUp(void) {}
void tearDown(void) {}

// ─── can_make_id / field extraction ──────────────────────────

void test_round_trip_all_fields() {
    uint32_t id = can_make_id(CAN_PRIO_HIGH, MSG_SENSOR_TRIP,
                               NODE_SENSOR_1, NODE_DISPLAY_A, 7);
    TEST_ASSERT_EQUAL_UINT8(CAN_PRIO_HIGH,    can_prio(id));
    TEST_ASSERT_EQUAL_UINT8(MSG_SENSOR_TRIP,  can_msg_type(id));
    TEST_ASSERT_EQUAL_UINT8(NODE_SENSOR_1,    can_source(id));
    TEST_ASSERT_EQUAL_UINT8(NODE_DISPLAY_A,   can_dest(id));
    TEST_ASSERT_EQUAL_UINT8(7,                can_seq(id));
}

void test_round_trip_broadcast_destination() {
    uint32_t id = can_make_id(CAN_PRIO_MED, MSG_RESULT,
                               NODE_DISPLAY_A, NODE_BROADCAST, 0);
    TEST_ASSERT_EQUAL_UINT8(NODE_BROADCAST, can_dest(id));
    TEST_ASSERT_EQUAL_UINT8(NODE_DISPLAY_A, can_source(id));
    TEST_ASSERT_EQUAL_UINT8(MSG_RESULT,     can_msg_type(id));
}

void test_round_trip_time_sync() {
    uint32_t id = can_make_id(CAN_PRIO_LOW, MSG_TIME_SYNC,
                               NODE_DISPLAY_A, NODE_BROADCAST, 0);
    TEST_ASSERT_EQUAL_UINT8(CAN_PRIO_LOW,   can_prio(id));
    TEST_ASSERT_EQUAL_UINT8(MSG_TIME_SYNC,  can_msg_type(id));
}

void test_round_trip_error_message() {
    uint32_t id = can_make_id(CAN_PRIO_DIAG, MSG_ERROR,
                               NODE_SENSOR_2, NODE_DISPLAY_A, 63);
    TEST_ASSERT_EQUAL_UINT8(CAN_PRIO_DIAG,  can_prio(id));
    TEST_ASSERT_EQUAL_UINT8(MSG_ERROR,      can_msg_type(id));
    TEST_ASSERT_EQUAL_UINT8(NODE_SENSOR_2,  can_source(id));
    TEST_ASSERT_EQUAL_UINT8(63,             can_seq(id));
}

// ─── Field masking (overflow inputs are truncated) ────────────

void test_priority_masked_to_3_bits() {
    // prio=0xFF should mask to 0x07 (3-bit field)
    uint32_t id = can_make_id(0xFF, MSG_CMD, NODE_DISPLAY_A, NODE_BROADCAST, 0);
    TEST_ASSERT_EQUAL_UINT8(0x07, can_prio(id));
}

void test_seq_masked_to_6_bits() {
    // seq=0xFF should mask to 0x3F (6-bit field)
    uint32_t id = can_make_id(CAN_PRIO_LOW, MSG_CMD,
                               NODE_DISPLAY_A, NODE_BROADCAST, 0xFF);
    TEST_ASSERT_EQUAL_UINT8(0x3F, can_seq(id));
}

void test_source_masked_to_6_bits() {
    uint32_t id = can_make_id(CAN_PRIO_LOW, MSG_CMD, 0xFF, NODE_BROADCAST, 0);
    TEST_ASSERT_EQUAL_UINT8(0x3F, can_source(id));
}

// ─── Fields do not bleed into each other ─────────────────────

void test_fields_are_independent() {
    // Build two IDs that differ only in seq; all other fields must be equal
    uint32_t id0 = can_make_id(CAN_PRIO_MED, MSG_DISPLAY_SYNC,
                                NODE_DISPLAY_A, NODE_BROADCAST, 0);
    uint32_t id1 = can_make_id(CAN_PRIO_MED, MSG_DISPLAY_SYNC,
                                NODE_DISPLAY_A, NODE_BROADCAST, 1);
    TEST_ASSERT_EQUAL_UINT8(can_prio(id0),     can_prio(id1));
    TEST_ASSERT_EQUAL_UINT8(can_msg_type(id0), can_msg_type(id1));
    TEST_ASSERT_EQUAL_UINT8(can_source(id0),   can_source(id1));
    TEST_ASSERT_EQUAL_UINT8(can_dest(id0),     can_dest(id1));
    TEST_ASSERT_NOT_EQUAL(id0, id1);
}

void test_id_fits_in_29_bits() {
    uint32_t id = can_make_id(0x07, 0xFF, 0x3F, 0x3F, 0x3F);
    TEST_ASSERT_EQUAL_UINT32(0, id >> 29);   // bits 31:29 must be zero
}

// ─── Default seq argument ─────────────────────────────────────

void test_default_seq_is_zero() {
    uint32_t id = can_make_id(CAN_PRIO_MED, MSG_CMD, NODE_DISPLAY_A, NODE_SENSOR_1);
    TEST_ASSERT_EQUAL_UINT8(0, can_seq(id));
}

// ─────────────────────────────────────────────────────────────
int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_round_trip_all_fields);
    RUN_TEST(test_round_trip_broadcast_destination);
    RUN_TEST(test_round_trip_time_sync);
    RUN_TEST(test_round_trip_error_message);
    RUN_TEST(test_priority_masked_to_3_bits);
    RUN_TEST(test_seq_masked_to_6_bits);
    RUN_TEST(test_source_masked_to_6_bits);
    RUN_TEST(test_fields_are_independent);
    RUN_TEST(test_id_fits_in_29_bits);
    RUN_TEST(test_default_seq_is_zero);
    return UNITY_END();
}
