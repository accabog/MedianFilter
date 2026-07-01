// SPDX-License-Identifier: MIT
/*
 * init_test.c
 *
 * Negative/positive tests for the MEDIANFILTER_Init input-validation contract:
 * Init must return -1 for NULL filter, NULL buffer, even numNodes, and
 * numNodes <= 1, and 0 for a valid odd window > 1.  (No prior test exercised
 * the -1 path.)
 */

#include "MedianFilter.h"
#include "test_harness.h"

/*
 * The deprecated aliases sMedianNode_t / sMedianFilter_t must resolve to the
 * SAME types as MedianFilterNode_t / MedianFilter_t (not merely layout-
 * compatible).  These cross-assignments compile without a cast only if the
 * types are identical; under -Werror (-Wincompatible-pointer-types) a mismatch
 * would fail the build.  Compiles on gcc, clang, and MSVC.
 */
static void alias_compat_check(void)
{
    MedianFilter_t      f;  sMedianFilter_t    *fp = &f;  (void)fp;
    sMedianFilter_t     g;  MedianFilter_t     *gp = &g;  (void)gp;
    MedianFilterNode_t  n;  sMedianNode_t      *np = &n;  (void)np;
    sMedianNode_t       m;  MedianFilterNode_t *mp = &m;  (void)mp;
}

static int expect_init(const char *label, MedianFilter_t *f, int expected)
{
    int rc = MEDIANFILTER_Init(f);
    TEST_BEGIN(label);
    if (rc == expected) { TEST_PASS(); return 0; }
    TEST_FAIL("got %d, expected %d", rc, expected);
    return 1;
}

int main(void)
{
    MedianFilterNode_t nodes[8];

    TEST_SUITE_BEGIN("MedianFilter Init Validation Test");

    TEST_SECTION("Backward-compat type aliases");
    alias_compat_check();
    TEST_BEGIN("alias_types_identical");
    TEST_PASS();  /* reaching here means the TU compiled: aliases == canonical types */

    TEST_SECTION("Rejections (expect -1)");

    /* NULL filter pointer */
    expect_init("null_filter", NULL, -1);

    /* NULL buffer */
    {
        MedianFilter_t f = { 3, NULL, NULL, NULL, NULL };
        expect_init("null_buffer", &f, -1);
    }

    /* numNodes == 0 (even and <= 1) */
    {
        MedianFilter_t f = { 0, nodes, NULL, NULL, NULL };
        expect_init("numNodes_0", &f, -1);
    }

    /* numNodes == 1 (odd but not > 1) */
    {
        MedianFilter_t f = { 1, nodes, NULL, NULL, NULL };
        expect_init("numNodes_1", &f, -1);
    }

    /* numNodes == 2 (even) */
    {
        MedianFilter_t f = { 2, nodes, NULL, NULL, NULL };
        expect_init("numNodes_2_even", &f, -1);
    }

    /* numNodes == 4 (even) */
    {
        MedianFilter_t f = { 4, nodes, NULL, NULL, NULL };
        expect_init("numNodes_4_even", &f, -1);
    }

    TEST_SECTION("Acceptance (expect 0) + head wiring");

    /* Valid odd windows > 1 */
    {
        MedianFilter_t f = { 3, nodes, NULL, NULL, NULL };
        expect_init("numNodes_3_valid", &f, 0);

        TEST_BEGIN("valid_heads_wired");
        if (f.ageHead == &nodes[0] && f.valueHead == &nodes[0] &&
            f.medianHead == &nodes[3 / 2])
            TEST_PASS();
        else
            TEST_FAIL("heads not initialized as expected");
    }
    {
        MedianFilter_t f = { 7, nodes, NULL, NULL, NULL };
        expect_init("numNodes_7_valid", &f, 0);
    }

    TEST_SUITE_END();
}
