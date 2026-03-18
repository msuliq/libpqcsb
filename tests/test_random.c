/*
 * test_random.c — CSPRNG tests.
 */

#include "test_helpers.h"

TEST(create_random_produces_nonzero)
{
    pqcsb_buf_t *buf = NULL;
    ASSERT_OK(pqcsb_create_random(64, &buf));
    ASSERT(buf != NULL);
    ASSERT_EQ(pqcsb_len(buf), (size_t)64);

    pqcsb_read_guard_t guard = pqcsb_begin_read(buf);
    ASSERT_OK(guard.status);

    /* Extremely unlikely that 64 random bytes are all zero. */
    int all_zero = 1;
    for (size_t i = 0; i < 64; i++) {
        if (guard.data[i] != 0) { all_zero = 0; break; }
    }
    ASSERT(!all_zero);

    pqcsb_end_read(&guard);
    pqcsb_destroy(&buf);
}

TEST(create_random_two_differ)
{
    pqcsb_buf_t *a = NULL, *b = NULL;
    ASSERT_OK(pqcsb_create_random(32, &a));
    ASSERT_OK(pqcsb_create_random(32, &b));

    /* Two independent random buffers should differ (overwhelmingly likely). */
    ASSERT_EQ(pqcsb_ct_equal_bufs(a, b), 0);

    pqcsb_destroy(&a);
    pqcsb_destroy(&b);
}

TEST(create_random_zero_length)
{
    pqcsb_buf_t *buf = NULL;
    ASSERT_ERR(pqcsb_create_random(0, &buf), PQCSB_ERR_RANGE);
    ASSERT(buf == NULL);
}

TEST(fill_random_utility)
{
    uint8_t buf[32];
    memset(buf, 0, sizeof(buf));
    ASSERT_OK(pqcsb_fill_random(buf, sizeof(buf)));

    int all_zero = 1;
    for (size_t i = 0; i < sizeof(buf); i++) {
        if (buf[i] != 0) { all_zero = 0; break; }
    }
    ASSERT(!all_zero);
}

TEST(fill_random_null_returns_error)
{
    ASSERT_ERR(pqcsb_fill_random(NULL, 10), PQCSB_ERR_NULL_PARAM);
}

TEST(fill_random_zero_length_ok)
{
    uint8_t dummy;
    ASSERT_OK(pqcsb_fill_random(&dummy, 0));
}

TEST_MAIN_BEGIN("test_random")
    RUN(create_random_produces_nonzero);
    RUN(create_random_two_differ);
    RUN(create_random_zero_length);
    RUN(fill_random_utility);
    RUN(fill_random_null_returns_error);
    RUN(fill_random_zero_length_ok);
TEST_MAIN_END
