/*
 * test_harness.h — Minimal test output macros for MedianFilter tests.
 *
 * Provides consistent PASS/FAIL formatting and per-test-case counting
 * across all test executables. No external dependencies.
 *
 * Usage:
 *   TEST_SUITE_BEGIN("Suite Name");
 *   TEST_SECTION("group name");
 *   TEST_BEGIN("test name");
 *   if (ok) TEST_PASS();
 *   else    TEST_FAIL("got=%d expected=%d", actual, expected);
 *   TEST_SUITE_END();
 */

#ifndef TEST_HARNESS_H
#define TEST_HARNESS_H

#include <stdio.h>

static int th_passed_ = 0;
static int th_failed_ = 0;
static const char *th_current_test_ = "";

#define TEST_SUITE_BEGIN(name) \
    do { \
        th_passed_ = 0; \
        th_failed_ = 0; \
        printf("=== %s ===\n", (name)); \
    } while (0)

#define TEST_SUITE_END() \
    do { \
        printf("\n=== %d passed, %d failed ===\n", th_passed_, th_failed_); \
        return th_failed_ > 0 ? 1 : 0; \
    } while (0)

#define TEST_SECTION(name) \
    printf("\n--- %s ---\n", (name))

#define TEST_BEGIN(name) \
    th_current_test_ = (name)

#define TEST_PASS() \
    do { \
        th_passed_++; \
        printf("  PASS [%s]\n", th_current_test_); \
    } while (0)

#define TEST_FAIL(...) \
    do { \
        th_failed_++; \
        printf("  FAIL [%s] ", th_current_test_); \
        printf(__VA_ARGS__); \
        printf("\n"); \
    } while (0)

#endif /* TEST_HARNESS_H */
