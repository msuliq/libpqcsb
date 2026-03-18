/*
 * test_canary.c — canary integrity tests.
 */

#include "test_helpers.h"

TEST(canary_ok_after_create)
{
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    pqcsb_buf_t *buf = NULL;
    ASSERT_OK(pqcsb_create(data, sizeof(data), &buf));

    ASSERT_OK(pqcsb_check_canary(buf));

    pqcsb_destroy(&buf);
}

TEST(canary_ok_after_write)
{
    uint8_t data[] = {0x00, 0x00, 0x00, 0x00};
    pqcsb_buf_t *buf = NULL;
    ASSERT_OK(pqcsb_create(data, sizeof(data), &buf));

    pqcsb_write_guard_t guard = pqcsb_begin_write(buf);
    ASSERT_OK(guard.status);
    guard.data[0] = 0xFF;
    guard.data[3] = 0xEE;
    pqcsb_end_write(&guard);  /* rewrites canaries */

    ASSERT_OK(pqcsb_check_canary(buf));

    pqcsb_destroy(&buf);
}

TEST(canary_check_on_wiped_returns_wiped)
{
    uint8_t data[] = {0x01};
    pqcsb_buf_t *buf = NULL;
    ASSERT_OK(pqcsb_create(data, 1, &buf));
    ASSERT_OK(pqcsb_wipe(buf));

    ASSERT_ERR(pqcsb_check_canary(buf), PQCSB_ERR_WIPED);

    pqcsb_destroy(&buf);
}

TEST(canary_check_null)
{
    ASSERT_ERR(pqcsb_check_canary(NULL), PQCSB_ERR_NULL_PARAM);
}

TEST(canary_ok_after_random)
{
    pqcsb_buf_t *buf = NULL;
    ASSERT_OK(pqcsb_create_random(128, &buf));
    ASSERT_OK(pqcsb_check_canary(buf));
    pqcsb_destroy(&buf);
}

TEST_MAIN_BEGIN("test_canary")
    RUN(canary_ok_after_create);
    RUN(canary_ok_after_write);
    RUN(canary_check_on_wiped_returns_wiped);
    RUN(canary_check_null);
    RUN(canary_ok_after_random);
TEST_MAIN_END
