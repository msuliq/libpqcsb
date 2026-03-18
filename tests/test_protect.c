/*
 * test_protect.c — mprotect access control tests.
 */

#include "test_helpers.h"

TEST(begin_read_end_read)
{
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    pqcsb_buf_t *buf = NULL;
    ASSERT_OK(pqcsb_create(data, sizeof(data), &buf));

    /* First read */
    pqcsb_read_guard_t guard = pqcsb_begin_read(buf);
    ASSERT_OK(guard.status);
    ASSERT(guard.data != NULL);
    ASSERT_EQ(guard.data[0], 0x01);
    ASSERT_EQ(guard.data[3], 0x04);
    pqcsb_end_read(&guard);

    /* Second read (verify re-protection didn't break anything) */
    guard = pqcsb_begin_read(buf);
    ASSERT_OK(guard.status);
    ASSERT(guard.data != NULL);
    ASSERT_EQ(guard.data[0], 0x01);
    pqcsb_end_read(&guard);

    pqcsb_destroy(&buf);
}

TEST(nested_reads)
{
    uint8_t data[] = {0xAA, 0xBB};
    pqcsb_buf_t *buf = NULL;
    ASSERT_OK(pqcsb_create(data, sizeof(data), &buf));

    pqcsb_read_guard_t g1 = pqcsb_begin_read(buf);
    ASSERT_OK(g1.status);
    ASSERT(g1.data != NULL);

    /* Nested read (should succeed, bump refcount) */
    pqcsb_read_guard_t g2 = pqcsb_begin_read(buf);
    ASSERT_OK(g2.status);
    ASSERT(g2.data != NULL);
    ASSERT(g1.data == g2.data);

    /* Inner end_read should NOT re-protect (outer still active) */
    pqcsb_end_read(&g2);

    /* Outer read should still work */
    ASSERT_EQ(g1.data[0], 0xAA);
    ASSERT_EQ(g1.data[1], 0xBB);

    /* Outer end_read re-protects */
    pqcsb_end_read(&g1);

    pqcsb_destroy(&buf);
}

TEST(begin_write_end_write)
{
    uint8_t data[] = {0x00, 0x00, 0x00, 0x00};
    pqcsb_buf_t *buf = NULL;
    ASSERT_OK(pqcsb_create(data, sizeof(data), &buf));

    pqcsb_write_guard_t wguard = pqcsb_begin_write(buf);
    ASSERT_OK(wguard.status);
    ASSERT(wguard.data != NULL);

    /* Mutate */
    wguard.data[0] = 0xFF;
    wguard.data[1] = 0xEE;
    wguard.data[2] = 0xDD;
    wguard.data[3] = 0xCC;

    pqcsb_end_write(&wguard);

    /* Verify mutation persisted */
    pqcsb_read_guard_t rguard = pqcsb_begin_read(buf);
    ASSERT_OK(rguard.status);
    ASSERT_EQ(rguard.data[0], 0xFF);
    ASSERT_EQ(rguard.data[1], 0xEE);
    ASSERT_EQ(rguard.data[2], 0xDD);
    ASSERT_EQ(rguard.data[3], 0xCC);
    pqcsb_end_read(&rguard);

    pqcsb_destroy(&buf);
}

TEST(read_wiped_returns_error)
{
    uint8_t data[] = {0x01};
    pqcsb_buf_t *buf = NULL;
    ASSERT_OK(pqcsb_create(data, 1, &buf));
    ASSERT_OK(pqcsb_wipe(buf));

    pqcsb_read_guard_t guard = pqcsb_begin_read(buf);
    ASSERT(guard.data == NULL);
    ASSERT_ERR(guard.status, PQCSB_ERR_WIPED);

    pqcsb_destroy(&buf);
}

TEST(write_wiped_returns_error)
{
    uint8_t data[] = {0x01};
    pqcsb_buf_t *buf = NULL;
    ASSERT_OK(pqcsb_create(data, 1, &buf));
    ASSERT_OK(pqcsb_wipe(buf));

    pqcsb_write_guard_t guard = pqcsb_begin_write(buf);
    ASSERT(guard.data == NULL);
    ASSERT_ERR(guard.status, PQCSB_ERR_WIPED);

    pqcsb_destroy(&buf);
}

TEST(read_null_returns_error)
{
    pqcsb_read_guard_t guard = pqcsb_begin_read(NULL);
    ASSERT(guard.data == NULL);
    ASSERT_ERR(guard.status, PQCSB_ERR_NULL_PARAM);
}

TEST_MAIN_BEGIN("test_protect")
    RUN(begin_read_end_read);
    RUN(nested_reads);
    RUN(begin_write_end_write);
    RUN(read_wiped_returns_error);
    RUN(write_wiped_returns_error);
    RUN(read_null_returns_error);
TEST_MAIN_END
