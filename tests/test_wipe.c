/*
 * test_wipe.c — secure wipe and post-wipe behaviour tests.
 */

#include "test_helpers.h"

TEST(wipe_marks_wiped)
{
    uint8_t data[] = {0xFF, 0xEE, 0xDD};
    pqcsb_buf_t *buf = NULL;
    ASSERT_OK(pqcsb_create(data, sizeof(data), &buf));

    ASSERT_EQ(pqcsb_is_wiped(buf), 0);
    ASSERT_OK(pqcsb_wipe(buf));
    ASSERT_EQ(pqcsb_is_wiped(buf), 1);

    pqcsb_destroy(&buf);
}

TEST(wipe_idempotent)
{
    uint8_t data[] = {0x01};
    pqcsb_buf_t *buf = NULL;
    ASSERT_OK(pqcsb_create(data, 1, &buf));

    ASSERT_OK(pqcsb_wipe(buf));
    ASSERT_OK(pqcsb_wipe(buf));  /* second wipe is no-op */
    ASSERT_EQ(pqcsb_is_wiped(buf), 1);

    pqcsb_destroy(&buf);
}

TEST(wipe_null_returns_error)
{
    ASSERT_ERR(pqcsb_wipe(NULL), PQCSB_ERR_NULL_PARAM);
}

TEST(operations_after_wipe_return_error)
{
    uint8_t data[] = {0xAA, 0xBB};
    pqcsb_buf_t *buf = NULL;
    ASSERT_OK(pqcsb_create(data, sizeof(data), &buf));
    ASSERT_OK(pqcsb_wipe(buf));

    /* begin_read */
    pqcsb_read_guard_t rguard = pqcsb_begin_read(buf);
    ASSERT(rguard.data == NULL);
    ASSERT_ERR(rguard.status, PQCSB_ERR_WIPED);

    /* begin_write */
    pqcsb_write_guard_t wguard = pqcsb_begin_write(buf);
    ASSERT(wguard.data == NULL);
    ASSERT_ERR(wguard.status, PQCSB_ERR_WIPED);

    /* check_canary */
    ASSERT_ERR(pqcsb_check_canary(buf), PQCSB_ERR_WIPED);

    /* slice */
    pqcsb_buf_t *sliced = NULL;
    ASSERT_ERR(pqcsb_slice(buf, 0, 1, &sliced), PQCSB_ERR_WIPED);
    ASSERT(sliced == NULL);

    /* ct_equal */
    ASSERT_EQ(pqcsb_ct_equal(buf, data, sizeof(data)), 0);

    /* len still works */
    ASSERT_EQ(pqcsb_len(buf), sizeof(data));

    pqcsb_destroy(&buf);
}

TEST(is_wiped_null)
{
    ASSERT_EQ(pqcsb_is_wiped(NULL), 1);
}

TEST(secure_zero_utility)
{
    uint8_t buf[32];
    memset(buf, 0xFF, sizeof(buf));
    pqcsb_secure_zero(buf, sizeof(buf));
    for (size_t i = 0; i < sizeof(buf); i++)
        ASSERT_EQ(buf[i], 0);
}

TEST(secure_zero_null_is_safe)
{
    pqcsb_secure_zero(NULL, 0);
    pqcsb_secure_zero(NULL, 100);
    /* No crash = pass */
}

TEST_MAIN_BEGIN("test_wipe")
    RUN(wipe_marks_wiped);
    RUN(wipe_idempotent);
    RUN(wipe_null_returns_error);
    RUN(operations_after_wipe_return_error);
    RUN(is_wiped_null);
    RUN(secure_zero_utility);
    RUN(secure_zero_null_is_safe);
TEST_MAIN_END
