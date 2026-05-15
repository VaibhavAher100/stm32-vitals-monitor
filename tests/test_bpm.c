#include "unity.h"
#include "bpm.h"
#include "task.h"  /* stub_set_tick */

static BpmDetector b;

void setUp(void)
{
    bpm_init(&b);
    stub_set_tick(0U);
}

void tearDown(void) {}

/* Feed N identical values - used to prime the history window */
static void feed_n(uint32_t val, uint8_t n)
{
    uint8_t i;
    for (i = 0U; i < n; i++) {
        bpm_update(&b, val);
    }
}

/* REQ-BPM-05: invalid before any crossings */
void test_bpm_invalid_before_two_crossings(void)
{
    TEST_ASSERT_EQUAL_UINT32(BPM_INVALID, bpm_get(&b));
}

/* REQ-BPM-02: history must be full (8) before threshold is meaningful;
   no crossing fires during fill regardless of values */
void test_bpm_no_crossing_during_history_fill(void)
{
    /* Alternating low/high - crossings would fire if window were ready */
    uint8_t i;
    for (i = 0U; i < (uint8_t)BPM_HISTORY; i++) {
        bpm_update(&b, (i % 2U == 0U) ? 5000U : 90000U);
    }
    TEST_ASSERT_EQUAL_UINT32(BPM_INVALID, bpm_get(&b));
}

/* REQ-BPM-05: still invalid after exactly one crossing */
void test_bpm_invalid_after_one_crossing(void)
{
    /* Fill history with mix so threshold ~47500 */
    feed_n(5000U,  4U);
    feed_n(90000U, 4U);
    /* prev_val is now 90000, threshold ~47500 */

    stub_set_tick(0U);
    bpm_update(&b, 5000U);   /* falling: prev=90000, no crossing */
    stub_set_tick(500U);
    bpm_update(&b, 90000U);  /* rising edge: CROSSING 1 */

    TEST_ASSERT_EQUAL_UINT32(BPM_INVALID, bpm_get(&b));
}

/* REQ-BPM-03, REQ-BPM-05: valid BPM after two crossings */
void test_bpm_valid_after_two_crossings(void)
{
    feed_n(5000U,  4U);
    feed_n(90000U, 4U);

    stub_set_tick(0U);
    bpm_update(&b, 5000U);

    stub_set_tick(500U);
    bpm_update(&b, 90000U);  /* crossing 1 at t=500 */

    bpm_update(&b, 5000U);

    stub_set_tick(1100U);
    bpm_update(&b, 90000U);  /* crossing 2 at t=1100, elapsed=600ms */

    /* 60000 / 600 = 100 BPM */
    TEST_ASSERT_EQUAL_UINT32(100U, bpm_get(&b));
}

/* REQ-BPM-04: crossing within refractory window is rejected */
void test_bpm_refractory_rejects_fast_crossing(void)
{
    feed_n(5000U,  4U);
    feed_n(90000U, 4U);

    stub_set_tick(0U);
    bpm_update(&b, 5000U);

    stub_set_tick(500U);
    bpm_update(&b, 90000U);  /* crossing 1 at t=500 */

    bpm_update(&b, 5000U);

    /* Second crossing only 200ms later - within BPM_REFRACTORY_MS (333ms) */
    stub_set_tick(700U);
    bpm_update(&b, 90000U);  /* should be REJECTED */

    TEST_ASSERT_EQUAL_UINT32(BPM_INVALID, bpm_get(&b));
}

/* REQ-BPM-05: known interval -> known BPM */
void test_bpm_known_interval_500ms_gives_120bpm(void)
{
    feed_n(5000U,  4U);
    feed_n(90000U, 4U);

    stub_set_tick(0U);
    bpm_update(&b, 5000U);

    stub_set_tick(1000U);
    bpm_update(&b, 90000U);  /* crossing 1 at t=1000 */

    bpm_update(&b, 5000U);

    stub_set_tick(1500U);
    bpm_update(&b, 90000U);  /* crossing 2 at t=1500, elapsed=500ms */

    /* 60000 / 500 = 120 BPM */
    TEST_ASSERT_EQUAL_UINT32(120U, bpm_get(&b));
}

/* REQ-BPM-02: threshold adapts - flat signal never crosses */
void test_bpm_flat_signal_never_crosses(void)
{
    uint8_t i;
    for (i = 0U; i < 20U; i++) {
        bpm_update(&b, 50000U);
    }
    TEST_ASSERT_EQUAL_UINT32(BPM_INVALID, bpm_get(&b));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_bpm_invalid_before_two_crossings);
    RUN_TEST(test_bpm_no_crossing_during_history_fill);
    RUN_TEST(test_bpm_invalid_after_one_crossing);
    RUN_TEST(test_bpm_valid_after_two_crossings);
    RUN_TEST(test_bpm_refractory_rejects_fast_crossing);
    RUN_TEST(test_bpm_known_interval_500ms_gives_120bpm);
    RUN_TEST(test_bpm_flat_signal_never_crosses);
    return UNITY_END();
}
