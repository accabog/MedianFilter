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

static int expect_init(const char *label, sMedianFilter_t *f, int expected)
{
    int rc = MEDIANFILTER_Init(f);
    TEST_BEGIN(label);
    if (rc == expected) { TEST_PASS(); return 0; }
    TEST_FAIL("got %d, expected %d", rc, expected);
    return 1;
}

int main(void)
{
    sMedianNode_t nodes[8];

    TEST_SUITE_BEGIN("MedianFilter Init Validation Test");

    TEST_SECTION("Rejections (expect -1)");

    /* NULL filter pointer */
    expect_init("null_filter", NULL, -1);

    /* NULL buffer */
    {
        sMedianFilter_t f = { 3, NULL, NULL, NULL, NULL };
        expect_init("null_buffer", &f, -1);
    }

    /* numNodes == 0 (even and <= 1) */
    {
        sMedianFilter_t f = { 0, nodes, NULL, NULL, NULL };
        expect_init("numNodes_0", &f, -1);
    }

    /* numNodes == 1 (odd but not > 1) */
    {
        sMedianFilter_t f = { 1, nodes, NULL, NULL, NULL };
        expect_init("numNodes_1", &f, -1);
    }

    /* numNodes == 2 (even) */
    {
        sMedianFilter_t f = { 2, nodes, NULL, NULL, NULL };
        expect_init("numNodes_2_even", &f, -1);
    }

    /* numNodes == 4 (even) */
    {
        sMedianFilter_t f = { 4, nodes, NULL, NULL, NULL };
        expect_init("numNodes_4_even", &f, -1);
    }

    TEST_SECTION("Acceptance (expect 0) + head wiring");

    /* Valid odd windows > 1 */
    {
        sMedianFilter_t f = { 3, nodes, NULL, NULL, NULL };
        expect_init("numNodes_3_valid", &f, 0);

        TEST_BEGIN("valid_heads_wired");
        if (f.ageHead == &nodes[0] && f.valueHead == &nodes[0] &&
            f.medianHead == &nodes[3 / 2])
            TEST_PASS();
        else
            TEST_FAIL("heads not initialized as expected");
    }
    {
        sMedianFilter_t f = { 7, nodes, NULL, NULL, NULL };
        expect_init("numNodes_7_valid", &f, 0);
    }

    TEST_SUITE_END();
}
