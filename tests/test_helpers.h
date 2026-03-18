/*
 * test_helpers.h — minimal test harness for libpqcsb tests.
 *
 * No external test framework dependency. Each test file is a standalone
 * executable that returns 0 on success or 1 on failure.
 */

#ifndef PQCSB_TEST_HELPERS_H
#define PQCSB_TEST_HELPERS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pqcsb.h"

static int test_failures = 0;
static int test_count = 0;

#define TEST(name) \
    static void test_##name(void); \
    static void run_test_##name(void) { \
        test_count++; \
        printf("  %-50s ", #name); \
        fflush(stdout); \
        test_##name(); \
        printf("OK\n"); \
    } \
    static void test_##name(void)

#define RUN(name) run_test_##name()

#define ASSERT(cond) do { \
    if (!(cond)) { \
        printf("FAIL\n    %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        test_failures++; \
        return; \
    } \
} while (0)

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        printf("FAIL\n    %s:%d: %s != %s\n", __FILE__, __LINE__, #a, #b); \
        test_failures++; \
        return; \
    } \
} while (0)

#define ASSERT_OK(expr) do { \
    pqcsb_status_t _rc = (expr); \
    if (_rc != PQCSB_OK) { \
        printf("FAIL\n    %s:%d: %s returned %d (%s)\n", \
               __FILE__, __LINE__, #expr, _rc, pqcsb_error_message(_rc)); \
        test_failures++; \
        return; \
    } \
} while (0)

#define ASSERT_ERR(expr, expected) do { \
    pqcsb_status_t _rc = (expr); \
    if (_rc != (expected)) { \
        printf("FAIL\n    %s:%d: expected %d but %s returned %d\n", \
               __FILE__, __LINE__, (expected), #expr, _rc); \
        test_failures++; \
        return; \
    } \
} while (0)

#define TEST_MAIN_BEGIN(suite) \
    int main(void) { \
        printf("%s:\n", suite);

#define TEST_MAIN_END \
        printf("\n%d tests, %d failures\n", test_count, test_failures); \
        return test_failures > 0 ? 1 : 0; \
    }

#endif /* PQCSB_TEST_HELPERS_H */
