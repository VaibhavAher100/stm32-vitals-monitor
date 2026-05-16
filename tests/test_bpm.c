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

/* Feed N identical values */
static void feed_n(uint32_t val, uint8_t n)
{
    uint8_t i;
    for (i = 0U; i < n; i++) {
        bpm_update(&b, val, 0U);
    }
}

/* Fill BPM_HISTORY samples ending on a LOW value so:
   (a) history is full, threshold is meaningful
   (b) below_lower=1, ready for the next rising edge
   (c) no crossing fires on the fill-completing sample */
static void prime_history(void)
{
    uint8_t highs = (uint8_t)(BPM_HISTORY / 2U);
    uint8_t lows  = (uint8_t)(BPM_HISTORY - highs);
    feed_n(90000U, highs);
    feed_n(5000U,  lows);   /* end on LOW */
}

/* REQ-BPM-05: invalid before any crossings */
void test_bpm_invalid_before_two_crossings(void)
{
    TEST_ASSERT_EQUAL_UINT32(BPM_INVALID, bpm_get(&b));
}

/* REQ-BPM-02: history must be full (BPM_HISTORY) before threshold is
   meaningful; no crossing fires during fill when last sample is LOW */
void test_bpm_no_crossing_during_history_fill(void)
{
    uint8_t i;
    /* Alternating low/high for exactly BPM_HISTORY samples.
       BPM_HISTORY=25 (odd) → last sample is LOW (i=24 even → 5000) */
    for (i = 0U; i < (uint8_t)BPM_HISTORY; i++) {
        bpm_update(&b, (i % 2U == 0U) ? 5000U : 90000U, 0U);
    }
    TEST_ASSERT_EQUAL_UINT32(BPM_INVALID, bpm_get(&b));
}

/* REQ-BPM-05: still invalid after exactly one crossing */
void test_bpm_invalid_after_one_crossing(void)
{
    prime_history();  /* hist_count=BPM_HISTORY, ends LOW, below_lower=1 */

    bpm_update(&b, 90000U, 0U);  /* rising edge: CROSSING 1 */

    TEST_ASSERT_EQUAL_UINT32(BPM_INVALID, bpm_get(&b));
}

/* REQ-BPM-03, REQ-BPM-05: valid BPM after two crossings */
void test_bpm_valid_after_two_crossings(void)
{
    prime_history();  /* ends LOW, below_lower=1 */

    bpm_update(&b, 90000U, 0U);  /* crossing 1 at t=0, below_lower→0 */

    bpm_update(&b, 5000U, 0U);   /* signal goes low → below_lower=1 */

    stub_set_tick(600U);
    bpm_update(&b, 90000U, 600U);  /* crossing 2 at t=600, elapsed=600ms */

    /* 60000 / 600 = 100 BPM */
    TEST_ASSERT_EQUAL_UINT32(100U, bpm_get(&b));
}

/* REQ-BPM-04: crossing within refractory window is rejected */
void test_bpm_refractory_rejects_fast_crossing(void)
{
    prime_history();  /* ends LOW, below_lower=1 */

    bpm_update(&b, 90000U, 0U);  /* crossing 1 at t=0 */

    bpm_update(&b, 5000U, 0U);   /* below_lower=1 */

    stub_set_tick(500U);
    bpm_update(&b, 90000U, 500U);  /* 500ms < BPM_REFRACTORY_MS (600ms) → REJECTED */

    TEST_ASSERT_EQUAL_UINT32(BPM_INVALID, bpm_get(&b));
}

/* REQ-BPM-05: known interval → known BPM.
   700ms > BPM_REFRACTORY_MS (600ms), 60000/700 = 85 BPM <= BPM_MAX (100). */
void test_bpm_known_interval_700ms_gives_85bpm(void)
{
    prime_history();  /* ends LOW, below_lower=1 */

    bpm_update(&b, 90000U, 0U);  /* crossing 1 at t=0 */

    bpm_update(&b, 5000U, 0U);   /* below_lower=1 */

    stub_set_tick(700U);
    bpm_update(&b, 90000U, 700U);  /* crossing 2 at t=700, elapsed=700ms */

    /* 60000 / 700 = 85 BPM */
    TEST_ASSERT_EQUAL_UINT32(85U, bpm_get(&b));
}

/* REQ-BPM-02: flat signal never crosses — PPG_MIN_AMPLITUDE gate */
void test_bpm_flat_signal_never_crosses(void)
{
    uint8_t i;
    /* Feed more than BPM_HISTORY samples at a constant value.
       range = 0 < PPG_MIN_AMPLITUDE → compute_threshold returns 0 → no crossings. */
    for (i = 0U; i < (uint8_t)(BPM_HISTORY + 10U); i++) {
        bpm_update(&b, 50000U, 0U);
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
    RUN_TEST(test_bpm_known_interval_700ms_gives_85bpm);
    RUN_TEST(test_bpm_flat_signal_never_crosses);
    return UNITY_END();
}
