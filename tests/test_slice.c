/*
 * test_slice.c — sub-range extraction tests.
 */

#include "test_helpers.h"

TEST(slice_copies_range)
{
    uint8_t data[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};
    pqcsb_buf_t *buf = NULL;
    ASSERT_OK(pqcsb_create(data, sizeof(data), &buf));

    pqcsb_buf_t *sliced = NULL;
    ASSERT_OK(pqcsb_slice(buf, 2, 4, &sliced));
    ASSERT(sliced != NULL);
    ASSERT_EQ(pqcsb_len(sliced), (size_t)4);

    pqcsb_read_guard_t guard = pqcsb_begin_read(sliced);
    ASSERT_OK(guard.status);
    ASSERT_EQ(guard.data[0], 0x22);
    ASSERT_EQ(guard.data[1], 0x33);
    ASSERT_EQ(guard.data[2], 0x44);
    ASSERT_EQ(guard.data[3], 0x55);
    pqcsb_end_read(&guard);

    pqcsb_destroy(&sliced);
    pqcsb_destroy(&buf);
}

TEST(slice_full_range)
{
    uint8_t data[] = {0xAA, 0xBB, 0xCC};
    pqcsb_buf_t *buf = NULL;
    ASSERT_OK(pqcsb_create(data, sizeof(data), &buf));

    pqcsb_buf_t *sliced = NULL;
    ASSERT_OK(pqcsb_slice(buf, 0, 3, &sliced));
    ASSERT_EQ(pqcsb_len(sliced), (size_t)3);

    /* Sliced is independent — wipe original, sliced still works */
    ASSERT_OK(pqcsb_wipe(buf));

    pqcsb_read_guard_t guard = pqcsb_begin_read(sliced);
    ASSERT_OK(guard.status);
    ASSERT_EQ(guard.data[0], 0xAA);
    pqcsb_end_read(&guard);

    pqcsb_destroy(&sliced);
    pqcsb_destroy(&buf);
}

TEST(slice_offset_out_of_bounds)
{
    uint8_t data[] = {0x01, 0x02};
    pqcsb_buf_t *buf = NULL;
    ASSERT_OK(pqcsb_create(data, sizeof(data), &buf));

    pqcsb_buf_t *sliced = NULL;
    ASSERT_ERR(pqcsb_slice(buf, 5, 1, &sliced), PQCSB_ERR_RANGE);
    ASSERT(sliced == NULL);

    pqcsb_destroy(&buf);
}

TEST(slice_length_exceeds_bounds)
{
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    pqcsb_buf_t *buf = NULL;
    ASSERT_OK(pqcsb_create(data, sizeof(data), &buf));

    pqcsb_buf_t *sliced = NULL;
    ASSERT_ERR(pqcsb_slice(buf, 2, 3, &sliced), PQCSB_ERR_RANGE);
    ASSERT(sliced == NULL);

    pqcsb_destroy(&buf);
}

TEST(slice_zero_length)
{
    uint8_t data[] = {0x01};
    pqcsb_buf_t *buf = NULL;
    ASSERT_OK(pqcsb_create(data, 1, &buf));

    pqcsb_buf_t *sliced = NULL;
    ASSERT_ERR(pqcsb_slice(buf, 0, 0, &sliced), PQCSB_ERR_RANGE);
    ASSERT(sliced == NULL);

    pqcsb_destroy(&buf);
}

TEST(slice_wiped_returns_error)
{
    uint8_t data[] = {0x01};
    pqcsb_buf_t *buf = NULL;
    ASSERT_OK(pqcsb_create(data, 1, &buf));
    ASSERT_OK(pqcsb_wipe(buf));

    pqcsb_buf_t *sliced = NULL;
    ASSERT_ERR(pqcsb_slice(buf, 0, 1, &sliced), PQCSB_ERR_WIPED);

    pqcsb_destroy(&buf);
}

TEST(slice_null)
{
    pqcsb_buf_t *sliced = NULL;
    ASSERT_ERR(pqcsb_slice(NULL, 0, 1, &sliced), PQCSB_ERR_NULL_PARAM);
}

TEST_MAIN_BEGIN("test_slice")
    RUN(slice_copies_range);
    RUN(slice_full_range);
    RUN(slice_offset_out_of_bounds);
    RUN(slice_length_exceeds_bounds);
    RUN(slice_zero_length);
    RUN(slice_wiped_returns_error);
    RUN(slice_null);
TEST_MAIN_END
