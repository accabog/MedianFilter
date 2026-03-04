/*
 * circular_test.c
 *
 * Targeted test to settle the dispute about the circular value chain
 * initialization in MedianFilter.c.
 *
 * The claim: Init creates a circular doubly-linked list for the value chain,
 * but Insert's downward search uses valueHead as a sentinel.  If the downward
 * search ever walks past valueHead (wrapping around the circular list into
 * the "large" end), the result would be wrong.
 *
 * Strategy:
 *   - Focus on the FIRST N inserts (before the buffer is fully recycled).
 *   - Window size 3 (minimum) with extreme values.
 *   - Patterns that force the downward search to overshoot valueHead.
 *   - Negative values during warmup (all nodes start at 0).
 *   - INT_MIN as first insert — forces maximum downward walk.
 *   - Brute-force reference median for every single result.
 */

#include "MedianFilter.h"
#include "test_harness.h"

#include <stdlib.h>
#include <string.h>
#include <limits.h>

/* ---- brute-force reference median ---- */

static int cmp_int(const void *a, const void *b)
{
    int va = *(const int *)a, vb = *(const int *)b;
    return (va > vb) - (va < vb);
}

/*
 * Given the full insertion history and the current window size,
 * compute the true median of the last `window` samples.
 * Before the window is full, the buffer contains zero-initialised
 * slots — the library treats those as real 0-valued samples.
 */
static int reference_median(const int *history, int count, int window)
{
    int buf[255];  /* generous upper bound */
    for (int i = 0; i < window; i++)
        buf[i] = 0;

    int n = (count < window) ? count : window;
    for (int i = 0; i < n; i++)
        buf[window - n + i] = history[count - n + i];

    qsort(buf, window, sizeof(int), cmp_int);
    return buf[window / 2];
}

/* ---- test helpers ---- */

static int run_sequence(const char *label, int window,
                        const int *samples, int num_samples)
{
    sMedianNode_t nodes[255];
    sMedianFilter_t filter;

    filter.numNodes     = window;
    filter.medianBuffer = nodes;

    if (MEDIANFILTER_Init(&filter) != 0) {
        TEST_BEGIN(label);
        TEST_FAIL("Init returned error for window=%d", window);
        return 1;
    }

    int history[10000];
    int fail_count = 0;

    for (int i = 0; i < num_samples; i++) {
        history[i] = samples[i];
        int got      = MEDIANFILTER_Insert(&filter, samples[i]);
        int expected = reference_median(history, i + 1, window);

        if (got != expected) {
            if (!fail_count) {
                printf("         w=%d insert #%d  sample=%d  got=%d  expected=%d\n",
                       window, i + 1, samples[i], got, expected);
            }
            fail_count++;
        }
    }

    TEST_BEGIN(label);
    if (fail_count)
        TEST_FAIL("w=%d, %d sample mismatches out of %d", window, fail_count, num_samples);
    else
        TEST_PASS();

    return fail_count;
}

/* helper: generate a longer random sequence and test it */
static void run_random_sequence(const char *label, int window,
                                int lo, int hi, int count, unsigned seed)
{
    int *samples = (int *)malloc(count * sizeof(int));
    srand(seed);
    unsigned int range = (unsigned int)hi - (unsigned int)lo + 1u;
    for (int i = 0; i < count; i++) {
        if (range == 0) /* full int range */
            samples[i] = (int)((unsigned int)rand() ^ ((unsigned int)rand() << 16));
        else
            samples[i] = lo + (int)((unsigned int)rand() % range);
    }
    run_sequence(label, window, samples, count);
    free(samples);
}

/* ================================================================
 *                        TEST CASES
 * ================================================================ */

int main(void)
{
    TEST_SUITE_BEGIN("Circular Value Chain Test");

    /* ------------------------------------------------------------------
     * GROUP 1: Window=3, first 3 inserts (before ANY recycling)
     * ------------------------------------------------------------------ */
    TEST_SECTION("Group 1: Window=3, extreme first inserts");

    {
        int s[] = { INT_MIN };
        run_sequence("1a: INT_MIN first", 3, s, 1);
    }
    {
        int s[] = { INT_MIN, INT_MIN, INT_MIN };
        run_sequence("1b: 3x INT_MIN", 3, s, 3);
    }
    {
        int s[] = { INT_MIN, INT_MAX, INT_MIN };
        run_sequence("1c: MIN,MAX,MIN", 3, s, 3);
    }
    {
        int s[] = { -1 };
        run_sequence("1d: -1 first", 3, s, 1);
    }
    {
        int s[] = { -1, -1, -1 };
        run_sequence("1e: 3x -1", 3, s, 3);
    }
    {
        int s[] = { INT_MIN, INT_MAX, 0, -1, 1, INT_MIN };
        run_sequence("1f: extreme 6-insert", 3, s, 6);
    }
    {
        int s[] = { -1, -2, -3, -4, -5 };
        run_sequence("1g: descending negatives", 3, s, 5);
    }
    {
        int s[] = { -1000000, -999999, -1000001 };
        run_sequence("1h: large negatives", 3, s, 3);
    }

    /* ------------------------------------------------------------------
     * GROUP 2: Window=3, overshoot patterns
     * ------------------------------------------------------------------ */
    TEST_SECTION("Group 2: Window=3, overshoot patterns");

    {
        int s[] = { 5, 10, -100 };
        run_sequence("2a: 5,10,-100", 3, s, 3);
    }
    {
        int s[] = { 100, 200, -1 };
        run_sequence("2b: 100,200,-1", 3, s, 3);
    }
    {
        int s[] = { 1, 2, -1, -2, -3, 0, 0, 0 };
        run_sequence("2c: mixed small", 3, s, 8);
    }
    {
        int s[] = { INT_MAX, INT_MAX, INT_MIN, INT_MIN, 0, INT_MAX };
        run_sequence("2d: INT extremes alternating", 3, s, 6);
    }
    {
        int s[] = { 42, 42, 42, -42, -42, -42, 0 };
        run_sequence("2e: plateau then drop", 3, s, 7);
    }

    /* ------------------------------------------------------------------
     * GROUP 3: Window=5, warmup and overshoot
     * ------------------------------------------------------------------ */
    TEST_SECTION("Group 3: Window=5, warmup and overshoot");

    {
        int s[] = { INT_MIN };
        run_sequence("3a: w5 INT_MIN first", 5, s, 1);
    }
    {
        int s[] = { INT_MIN, INT_MIN, INT_MIN, INT_MIN, INT_MIN };
        run_sequence("3a2: w5 5x INT_MIN", 5, s, 5);
    }
    {
        int s[] = { -1, -2, -3, -4, -5, -6, -7, -8, -9, -10 };
        run_sequence("3b: w5 descending negatives", 5, s, 10);
    }
    {
        int s[] = { 10, 20, 30, 40, -100, -200, -300, 0 };
        run_sequence("3c: w5 ascending then crash", 5, s, 8);
    }
    {
        int s[] = { INT_MAX, INT_MAX, INT_MAX, INT_MAX, INT_MIN,
                    INT_MIN, INT_MIN, INT_MIN, 0 };
        run_sequence("3d: w5 MAX then MIN", 5, s, 9);
    }
    {
        int s[] = { 0, 0, 0, 0, INT_MIN, INT_MIN, 0, INT_MAX, INT_MIN };
        run_sequence("3e: w5 zeros then extremes", 5, s, 9);
    }

    /* ------------------------------------------------------------------
     * GROUP 4: Window=7, longer chains
     * ------------------------------------------------------------------ */
    TEST_SECTION("Group 4: Window=7, deeper chains");

    {
        int s[] = { INT_MIN, INT_MIN, INT_MIN, INT_MIN, INT_MIN,
                    INT_MIN, INT_MIN };
        run_sequence("4a: w7 7x INT_MIN", 7, s, 7);
    }
    {
        int s[] = { 1, 2, 3, 4, 5, 6, 7, -7, -6, -5, -4, -3, -2, -1 };
        run_sequence("4b: w7 ascending then negdesc", 7, s, 14);
    }
    {
        int s[] = { 100, -100, 100, -100, 100, -100, 100, -100,
                    100, -100, 100, -100, 100, -100 };
        run_sequence("4c: w7 oscillating +-100", 7, s, 14);
    }

    /* ------------------------------------------------------------------
     * GROUP 5: Warmup gauntlet, w=3..15
     * ------------------------------------------------------------------ */
    TEST_SECTION("Group 5: Warmup gauntlet, w=3..15");

    for (int w = 3; w <= 15; w += 2) {
        int seq[30];
        int n = 0;
        for (int i = 0; i < w; i++) seq[n++] = INT_MIN;
        for (int i = 0; i < w; i++) seq[n++] = INT_MAX;

        char label[64];
        snprintf(label, sizeof(label), "5: w=%d MIN*%d then MAX*%d", w, w, w);
        run_sequence(label, w, seq, n);
    }

    /* ------------------------------------------------------------------
     * GROUP 6: Random stress tests
     * ------------------------------------------------------------------ */
    TEST_SECTION("Group 6: Random stress tests");

    run_random_sequence("6a: w3 rand [-1M,1M] 10k",   3,  -1000000, 1000000, 10000, 12345);
    run_random_sequence("6b: w5 rand [-1M,1M] 10k",   5,  -1000000, 1000000, 10000, 54321);
    run_random_sequence("6c: w7 rand [-1M,1M] 10k",   7,  -1000000, 1000000, 10000, 99999);
    run_random_sequence("6d: w3 rand [IMIN,IMAX] 10k", 3,  INT_MIN,  INT_MAX, 10000, 77777);
    run_random_sequence("6e: w5 rand [IMIN,IMAX] 10k", 5,  INT_MIN,  INT_MAX, 10000, 88888);
    run_random_sequence("6f: w9 rand [-10,10] 10k",    9,  -10,      10,      10000, 11111);
    run_random_sequence("6g: w11 rand [-1,1] 10k",    11,  -1,       1,       10000, 22222);
    run_random_sequence("6h: w13 rand [0,0] 10k",     13,   0,       0,       10000, 33333);
    run_random_sequence("6i: w3 rand [-1,0] 10k",      3,  -1,       0,       10000, 44444);
    run_random_sequence("6j: w15 rand [-1M,1M] 10k",  15,  -1000000, 1000000, 10000, 55555);

    /* ------------------------------------------------------------------
     * GROUP 7: Chain integrity verification
     * ------------------------------------------------------------------ */
    TEST_SECTION("Group 7: Chain integrity verification");

    {
        sMedianNode_t nodes[3];
        sMedianFilter_t filter;
        filter.numNodes = 3;
        filter.medianBuffer = nodes;
        MEDIANFILTER_Init(&filter);

        /* Verify initial chain structure */
        int init_ok = 1;

        if (nodes[0].nextValue != &nodes[1]) init_ok = 0;
        if (nodes[1].nextValue != &nodes[2]) init_ok = 0;
        if (nodes[2].nextValue != &nodes[0]) init_ok = 0;
        if (nodes[1].prevValue != &nodes[0]) init_ok = 0;
        if (nodes[2].prevValue != &nodes[1]) init_ok = 0;
        if (nodes[0].prevValue != &nodes[2]) init_ok = 0;

        if (nodes[0].value != 0 || nodes[1].value != 0 || nodes[2].value != 0)
            init_ok = 0;

        if (filter.valueHead != &nodes[0]) init_ok = 0;
        if (filter.medianHead != &nodes[1]) init_ok = 0;
        if (filter.ageHead != &nodes[0]) init_ok = 0;

        TEST_BEGIN("7a: init chain structure");
        if (init_ok) TEST_PASS();
        else         TEST_FAIL("chain pointers incorrect after Init");

        /* Insert INT_MIN — median of {0, 0, INT_MIN} = 0 */
        int result = MEDIANFILTER_Insert(&filter, INT_MIN);
        TEST_BEGIN("7b: INT_MIN insert, median=0");
        if (result == 0) TEST_PASS();
        else             TEST_FAIL("got=%d expected=0", result);

        /* Verify value chain sorted and circular after insert */
        {
            int chain_ok = 1;
            sMedianNode_t *p = filter.valueHead;
            int prev_val = p->value;

            for (unsigned int i = 1; i < filter.numNodes; i++) {
                p = p->nextValue;
                if (p->value < prev_val) chain_ok = 0;
                prev_val = p->value;
            }
            if (p->nextValue != filter.valueHead) chain_ok = 0;

            p = filter.valueHead->prevValue;
            prev_val = p->value;
            for (unsigned int i = 1; i < filter.numNodes; i++) {
                p = p->prevValue;
                if (p->value > prev_val) chain_ok = 0;
                prev_val = p->value;
            }
            if (p != filter.valueHead) chain_ok = 0;

            TEST_BEGIN("7c: chain integrity after INT_MIN insert");
            if (chain_ok) TEST_PASS();
            else          TEST_FAIL("value chain not sorted or not circular");
        }

        /* Insert second INT_MIN.  median of {0, INT_MIN, INT_MIN} = INT_MIN */
        result = MEDIANFILTER_Insert(&filter, INT_MIN);
        TEST_BEGIN("7d: 2nd INT_MIN insert, median=INT_MIN");
        if (result == INT_MIN) TEST_PASS();
        else                   TEST_FAIL("got=%d expected=%d", result, INT_MIN);

        /* Insert INT_MAX.  median of {INT_MIN, INT_MIN, INT_MAX} = INT_MIN */
        result = MEDIANFILTER_Insert(&filter, INT_MAX);
        TEST_BEGIN("7e: INT_MAX after 2xMIN, median=INT_MIN");
        if (result == INT_MIN) TEST_PASS();
        else                   TEST_FAIL("got=%d expected=%d", result, INT_MIN);

        /* Insert 0.  median of {INT_MIN, INT_MAX, 0} = 0 */
        result = MEDIANFILTER_Insert(&filter, 0);
        TEST_BEGIN("7f: 0 after MIN,MIN,MAX, median=0");
        if (result == 0) TEST_PASS();
        else             TEST_FAIL("got=%d expected=%d", result, 0);
    }

    /* ------------------------------------------------------------------
     * GROUP 8: Identical values then outlier
     * ------------------------------------------------------------------ */
    TEST_SECTION("Group 8: Identical values then outlier");

    for (int w = 3; w <= 11; w += 2) {
        int seq[30];
        int n = 0;
        for (int i = 0; i < w; i++) seq[n++] = 42;
        seq[n++] = -999999;
        seq[n++] = 999999;

        char label[64];
        snprintf(label, sizeof(label), "8: w=%d identical then outliers", w);
        run_sequence(label, w, seq, n);
    }

    /* ------------------------------------------------------------------
     * GROUP 9: Reference median self-check
     * ------------------------------------------------------------------ */
    TEST_SECTION("Group 9: Reference median self-check");
    {
        int ok = 1;
        int h1[] = {5};
        if (reference_median(h1, 1, 3) != 0) ok = 0;

        int h2[] = {5, 10};
        if (reference_median(h2, 2, 3) != 5) ok = 0;

        int h3[] = {5, 10, 1};
        if (reference_median(h3, 3, 3) != 5) ok = 0;

        int h4[] = {5, 10, 1, 8};
        if (reference_median(h4, 4, 3) != 8) ok = 0;

        int h5[] = {INT_MIN};
        if (reference_median(h5, 1, 5) != 0) ok = 0;

        TEST_BEGIN("9: reference median self-check");
        if (ok) TEST_PASS();
        else    TEST_FAIL("reference_median returned wrong value");
    }

    TEST_SUITE_END();
}
