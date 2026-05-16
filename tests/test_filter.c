#include "unity.h"
#include "filter.h"

static Filter f;

void setUp(void)
{
    filter_init(&f);
}

void tearDown(void) {}

/* REQ-FIL-06: init zeros all fields */
void test_filter_init_zeros_all_fields(void)
{
    uint8_t i;
    for (i = 0U; i < (uint8_t)FILTER_WINDOW; i++) {
        TEST_ASSERT_EQUAL_UINT32(0U, f.buf[i]);
    }
    TEST_ASSERT_EQUAL_UINT8(0U, f.idx);
    TEST_ASSERT_EQUAL_UINT8(0U, f.count);
    TEST_ASSERT_EQUAL_UINT32(0U, f.sum);
}

/* REQ-FIL-05: first update divides by 1, not FILTER_WINDOW */
void test_filter_single_value_returns_that_value(void)
{
    uint32_t result = filter_update(&f, 1000U);
    TEST_ASSERT_EQUAL_UINT32(1000U, result);
}

/* REQ-FIL-05: partial fill divides by samples so far */
void test_filter_partial_fill_three_values(void)
{
    filter_update(&f, 100U);
    filter_update(&f, 200U);
    uint32_t result = filter_update(&f, 300U);
    /* sum=600, count=3, average=200 */
    TEST_ASSERT_EQUAL_UINT32(200U, result);
}

/* REQ-FIL-02, REQ-FIL-03: full window averages all FILTER_WINDOW samples */
void test_filter_full_window_averages_all_eight(void)
{
    /* Feed [100, 200, ..., FILTER_WINDOW*100].
       Sum = 100 * FILTER_WINDOW * (FILTER_WINDOW+1) / 2.
       Rounded avg = (sum + FILTER_WINDOW/2) / FILTER_WINDOW
                   = 100 * (FILTER_WINDOW+1) / 2  when sum is divisible. */
    uint32_t i;
    uint32_t result = 0U;
    for (i = 1U; i <= (uint32_t)FILTER_WINDOW; i++) {
        result = filter_update(&f, i * 100U);
    }
    uint32_t sum      = 100U * (uint32_t)FILTER_WINDOW * (uint32_t)(FILTER_WINDOW + 1U) / 2U;
    uint32_t count    = (uint32_t)FILTER_WINDOW;
    uint32_t expected = (sum + count / 2U) / count;
    TEST_ASSERT_EQUAL_UINT32(expected, result);
}

/* REQ-FIL-04: (FILTER_WINDOW+1)th sample drops the oldest */
void test_filter_rollover_drops_oldest(void)
{
    uint32_t i;
    /* Fill with [100, 200, ..., FILTER_WINDOW*100] */
    for (i = 1U; i <= (uint32_t)FILTER_WINDOW; i++) {
        filter_update(&f, i * 100U);
    }
    /* Next sample replaces 100 with (FILTER_WINDOW+1)*100.
       New window: [200, 300, ..., (FILTER_WINDOW+1)*100].
       Sum = 100 * (2 + 3 + ... + (FILTER_WINDOW+1))
           = 100 * ((FILTER_WINDOW+1)*(FILTER_WINDOW+2)/2 - 1). */
    uint32_t result = filter_update(&f, (uint32_t)(FILTER_WINDOW + 1U) * 100U);
    uint32_t sum     = 100U * ((uint32_t)(FILTER_WINDOW + 1U) * (uint32_t)(FILTER_WINDOW + 2U) / 2U - 1U);
    uint32_t count   = (uint32_t)FILTER_WINDOW;
    uint32_t expected = (sum + count / 2U) / count;
    TEST_ASSERT_EQUAL_UINT32(expected, result);
}

/* REQ-FIL-03: eight identical values returns that value exactly */
void test_filter_all_identical_values(void)
{
    uint32_t i;
    uint32_t result = 0U;
    for (i = 0U; i < 8U; i++) {
        result = filter_update(&f, 75000U);
    }
    TEST_ASSERT_EQUAL_UINT32(75000U, result);
}

/* REQ-FIL-05: window count saturates at FILTER_WINDOW, not beyond */
void test_filter_count_saturates_at_window_size(void)
{
    uint32_t i;
    for (i = 0U; i < 16U; i++) {
        filter_update(&f, 1000U);
    }
    TEST_ASSERT_EQUAL_UINT8((uint8_t)FILTER_WINDOW, f.count);
}

/* REQ-FIL-06: re-init resets a used filter */
void test_filter_reinit_resets_used_filter(void)
{
    filter_update(&f, 99999U);
    filter_update(&f, 88888U);
    filter_init(&f);
    uint32_t result = filter_update(&f, 500U);
    TEST_ASSERT_EQUAL_UINT32(500U, result);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_filter_init_zeros_all_fields);
    RUN_TEST(test_filter_single_value_returns_that_value);
    RUN_TEST(test_filter_partial_fill_three_values);
    RUN_TEST(test_filter_full_window_averages_all_eight);
    RUN_TEST(test_filter_rollover_drops_oldest);
    RUN_TEST(test_filter_all_identical_values);
    RUN_TEST(test_filter_count_saturates_at_window_size);
    RUN_TEST(test_filter_reinit_resets_used_filter);
    return UNITY_END();
}
