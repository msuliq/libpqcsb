/*
 * test_alloc.c — allocation, data integrity, and destroy tests.
 */

#include "test_helpers.h"

TEST(create_copies_data)
{
    uint8_t data[] = {0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02, 0x03, 0x04};
    pqcsb_buf_t *buf = NULL;

    ASSERT_OK(pqcsb_create(data, sizeof(data), &buf));
    ASSERT(buf != NULL);
    ASSERT_EQ(pqcsb_len(buf), sizeof(data));

    pqcsb_read_guard_t guard = pqcsb_begin_read(buf);
    ASSERT_OK(guard.status);
    ASSERT(guard.data != NULL);
    ASSERT(memcmp(guard.data, data, sizeof(data)) == 0);
    pqcsb_end_read(&guard);

    pqcsb_destroy(&buf);
    ASSERT(buf == NULL);
}

TEST(create_null_data_returns_error)
{
    pqcsb_buf_t *buf = NULL;
    ASSERT_ERR(pqcsb_create(NULL, 10, &buf), PQCSB_ERR_NULL_PARAM);
    ASSERT(buf == NULL);
}

TEST(create_null_out_returns_error)
{
    uint8_t data[] = {0x01};
    ASSERT_ERR(pqcsb_create(data, 1, NULL), PQCSB_ERR_NULL_PARAM);
}

TEST(create_zero_length_returns_error)
{
    uint8_t data[] = {0x01};
    pqcsb_buf_t *buf = NULL;
    ASSERT_ERR(pqcsb_create(data, 0, &buf), PQCSB_ERR_RANGE);
    ASSERT(buf == NULL);
}

TEST(destroy_null_is_safe)
{
    pqcsb_destroy(NULL);
    pqcsb_buf_t *buf = NULL;
    pqcsb_destroy(&buf);
    /* No crash = pass */
}

TEST(create_large_buffer)
{
    size_t len = 8192;
    uint8_t *data = (uint8_t *)malloc(len);
    ASSERT(data != NULL);
    for (size_t i = 0; i < len; i++)
        data[i] = (uint8_t)(i & 0xFF);

    pqcsb_buf_t *buf = NULL;
    ASSERT_OK(pqcsb_create(data, len, &buf));
    ASSERT_EQ(pqcsb_len(buf), len);

    pqcsb_read_guard_t guard = pqcsb_begin_read(buf);
    ASSERT_OK(guard.status);
    ASSERT(memcmp(guard.data, data, len) == 0);
    pqcsb_end_read(&guard);

    pqcsb_destroy(&buf);
    free(data);
}

static pqcsb_status_t fill_aa(uint8_t *data, size_t len, void *ctx)
{
    (void)ctx;
    memset(data, 0xAA, len);
    return PQCSB_OK;
}

TEST(create_inplace)
{
    pqcsb_buf_t *buf = NULL;
    ASSERT_OK(pqcsb_create_inplace(64, fill_aa, NULL, &buf));
    ASSERT_EQ(pqcsb_len(buf), (size_t)64);

    pqcsb_read_guard_t guard = pqcsb_begin_read(buf);
    ASSERT_OK(guard.status);
    for (size_t i = 0; i < 64; i++)
        ASSERT_EQ(guard.data[i], 0xAA);
    pqcsb_end_read(&guard);

    pqcsb_destroy(&buf);
}

TEST(version_string)
{
    const char *v = pqcsb_version();
    ASSERT(v != NULL);
    ASSERT(strlen(v) > 0);
    ASSERT(strcmp(v, PQCSB_VERSION_STRING) == 0);
}

TEST(error_messages)
{
    ASSERT(strlen(pqcsb_error_message(PQCSB_OK)) > 0);
    ASSERT(strlen(pqcsb_error_message(PQCSB_ERR_ALLOC)) > 0);
    ASSERT(strlen(pqcsb_error_message(PQCSB_ERR_WIPED)) > 0);
    ASSERT(strlen(pqcsb_error_message(PQCSB_ERR_CANARY)) > 0);
}

TEST_MAIN_BEGIN("test_alloc")
    RUN(create_copies_data);
    RUN(create_null_data_returns_error);
    RUN(create_null_out_returns_error);
    RUN(create_zero_length_returns_error);
    RUN(destroy_null_is_safe);
    RUN(create_large_buffer);
    RUN(create_inplace);
    RUN(version_string);
    RUN(error_messages);
TEST_MAIN_END
