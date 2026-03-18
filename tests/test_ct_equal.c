/*
 * test_ct_equal.c — constant-time equality tests.
 */

#include "test_helpers.h"

TEST(equal_same_data)
{
    uint8_t data[] = {0xDE, 0xAD, 0xBE, 0xEF};
    pqcsb_buf_t *buf = NULL;
    ASSERT_OK(pqcsb_create(data, sizeof(data), &buf));

    ASSERT_EQ(pqcsb_ct_equal(buf, data, sizeof(data)), 1);

    pqcsb_destroy(&buf);
}

TEST(not_equal_different_data)
{
    uint8_t data[] = {0xDE, 0xAD, 0xBE, 0xEF};
    uint8_t other[] = {0xDE, 0xAD, 0xBE, 0x00};
    pqcsb_buf_t *buf = NULL;
    ASSERT_OK(pqcsb_create(data, sizeof(data), &buf));

    ASSERT_EQ(pqcsb_ct_equal(buf, other, sizeof(other)), 0);

    pqcsb_destroy(&buf);
}

TEST(not_equal_different_length)
{
    uint8_t data[] = {0x01, 0x02, 0x03};
    uint8_t short_data[] = {0x01, 0x02};
    pqcsb_buf_t *buf = NULL;
    ASSERT_OK(pqcsb_create(data, sizeof(data), &buf));

    ASSERT_EQ(pqcsb_ct_equal(buf, short_data, sizeof(short_data)), 0);

    pqcsb_destroy(&buf);
}

TEST(equal_bufs_same)
{
    uint8_t data[] = {0xAA, 0xBB, 0xCC, 0xDD};
    pqcsb_buf_t *a = NULL, *b = NULL;
    ASSERT_OK(pqcsb_create(data, sizeof(data), &a));
    ASSERT_OK(pqcsb_create(data, sizeof(data), &b));

    ASSERT_EQ(pqcsb_ct_equal_bufs(a, b), 1);

    pqcsb_destroy(&a);
    pqcsb_destroy(&b);
}

TEST(equal_bufs_different)
{
    uint8_t d1[] = {0x01, 0x02};
    uint8_t d2[] = {0x01, 0x03};
    pqcsb_buf_t *a = NULL, *b = NULL;
    ASSERT_OK(pqcsb_create(d1, sizeof(d1), &a));
    ASSERT_OK(pqcsb_create(d2, sizeof(d2), &b));

    ASSERT_EQ(pqcsb_ct_equal_bufs(a, b), 0);

    pqcsb_destroy(&a);
    pqcsb_destroy(&b);
}

TEST(equal_null_returns_zero)
{
    uint8_t data[] = {0x01};
    pqcsb_buf_t *buf = NULL;
    ASSERT_OK(pqcsb_create(data, 1, &buf));

    ASSERT_EQ(pqcsb_ct_equal(NULL, data, 1), 0);
    ASSERT_EQ(pqcsb_ct_equal(buf, NULL, 1), 0);
    ASSERT_EQ(pqcsb_ct_equal_bufs(NULL, buf), 0);
    ASSERT_EQ(pqcsb_ct_equal_bufs(buf, NULL), 0);
    ASSERT_EQ(pqcsb_ct_equal_bufs(NULL, NULL), 0);

    pqcsb_destroy(&buf);
}

TEST(equal_wiped_returns_zero)
{
    uint8_t data[] = {0x01, 0x02};
    pqcsb_buf_t *a = NULL, *b = NULL;
    ASSERT_OK(pqcsb_create(data, sizeof(data), &a));
    ASSERT_OK(pqcsb_create(data, sizeof(data), &b));
    ASSERT_OK(pqcsb_wipe(a));

    ASSERT_EQ(pqcsb_ct_equal(a, data, sizeof(data)), 0);
    ASSERT_EQ(pqcsb_ct_equal_bufs(a, b), 0);

    pqcsb_destroy(&a);
    pqcsb_destroy(&b);
}

TEST(equal_single_byte)
{
    uint8_t data[] = {0x42};
    pqcsb_buf_t *buf = NULL;
    ASSERT_OK(pqcsb_create(data, 1, &buf));

    ASSERT_EQ(pqcsb_ct_equal(buf, data, 1), 1);

    uint8_t diff[] = {0x43};
    ASSERT_EQ(pqcsb_ct_equal(buf, diff, 1), 0);

    pqcsb_destroy(&buf);
}

TEST_MAIN_BEGIN("test_ct_equal")
    RUN(equal_same_data);
    RUN(not_equal_different_data);
    RUN(not_equal_different_length);
    RUN(equal_bufs_same);
    RUN(equal_bufs_different);
    RUN(equal_null_returns_zero);
    RUN(equal_wiped_returns_zero);
    RUN(equal_single_byte);
TEST_MAIN_END
