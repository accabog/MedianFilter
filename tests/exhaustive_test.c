/*
 * exhaustive_test.c
 *
 * Exhaustive state test for the C API: checks Insert for windows of 3, 5 and
 * 7 against EVERY sequence of int samples, not a sample of them.
 *
 * Why the enumeration below covers every input sequence:
 *  - On the value-chain path the chain is sorted by value, and equal values
 *    are ordered by age (oldest first).  The whole filter state is therefore
 *    a function of the N stored values and the position of the oldest node
 *    (ageHead): the chain order, valueHead (smallest) and medianHead (rank
 *    N/2) follow from them.  Call such a state canonical.  Init's all-zero
 *    state is canonical.
 *  - Insert only compares sample values and never computes with them, so its
 *    behaviour depends only on the weak order (the ranking with ties) of the
 *    N stored values and the new sample.
 *  - For every weak order of the N + 1 values and every ageHead position the
 *    test builds the canonical state, calls Insert once and requires exactly
 *    the canonical state of the new window (every link, both heads, ageHead,
 *    the values) and its median as the return value.
 *  By induction from Init, Insert returns the right median for every input
 *  sequence at these window sizes and the state never leaves canonical form.
 *
 * Windows served by a selection network (numNodes <= MEDIANFILTER_SMALL_MAX)
 * leave the links and heads exactly as Init set them; for those the test
 * requires that state to stay unchanged and the median to be correct.
 *
 * The filter under test lives in an exactly sized heap buffer, so a sanitizer
 * or valgrind flags any access outside the window.
 *
 * Build once per configuration (the Makefile and CI do this): default, the
 * inline API with and without MEDIANFILTER_FORCE_INLINE, MEDIANFILTER_SMALL_MAX=0
 * and 5, MEDIANFILTER_SEARCH_FROM_PREVIOUS, and all of them together with the
 * inline API.  -DEXHAUSTIVE_MAX_N=9 adds windows of 9 (about two billion
 * transitions: minutes rather than seconds).
 */

#include "MedianFilter.h"
#include "test_harness.h"

#include <limits.h>
#include <stdlib.h>

#ifndef EXHAUSTIVE_MAX_N
#define EXHAUSTIVE_MAX_N 7
#endif
#if (EXHAUSTIVE_MAX_N < 3) || (EXHAUSTIVE_MAX_N > 9)
#error "EXHAUSTIVE_MAX_N must be 3..9"
#endif
#define MAX_NODES 16                        /* scratch size: the Init check goes up to windows of 15 */

/* largest window served by a selection network in this build */
#if MEDIANFILTER_SMALL_MAX >= 5
#define NETWORK_MAX 5
#elif MEDIANFILTER_SMALL_MAX >= 3
#define NETWORK_MAX 3
#else
#define NETWORK_MAX 0
#endif

/* number of weak orders of len elements (Fubini numbers), to check the enumeration is complete */
static const long long fubini[] = { 1, 1, 3, 13, 75, 541, 4683, 47293, 545835, 7087261, 102247563 };

/* rank -> sample value.  Any strictly increasing map gives the same control
 * flow; the "extreme" map spreads the ranks over INT_MIN..INT_MAX. */
static int map_value(int rank, int len, int extreme)
{
    if (!extreme)
        return rank - len / 2;
    return (int)((long long)INT_MIN + (long long)rank * ((long long)INT_MAX - (long long)INT_MIN) / (len - 1));
}

/* slots in canonical chain order: by rank, equal ranks oldest first (slot h is the oldest) */
static void canonical_order(const int *rank, int n, int h, int *order)
{
    int start[MAX_NODES + 1];

    for (int r = 0; r <= n + 1; r++)
        start[r] = 0;
    for (int i = 0; i < n; i++)
        start[rank[i] + 1]++;
    for (int r = 0; r <= n; r++)
        start[r + 1] += start[r];
    for (int a = 0; a < n; a++) {
        int slot = (h + a) % n;
        order[start[rank[slot]]++] = slot;
    }
}

/* the state a correct filter has for these values (by slot) and oldest slot h */
static void build_state(MedianFilter_t *f, MedianFilterNode_t *buf, const int *rank,
                        const int *value, int n, int h)
{
    int order[MAX_NODES];

    if (n <= NETWORK_MAX) {
        for (int i = 0; i < n; i++)
            order[i] = i;                   /* the links and heads Init sets */
    } else {
        canonical_order(rank, n, h, order);
    }
    for (int k = 0; k < n; k++) {
        buf[order[k]].value     = value[order[k]];
        buf[order[k]].nextValue = &buf[order[(k + 1) % n]];
        buf[order[k]].prevValue = &buf[order[(k + n - 1) % n]];
    }
    f->numNodes     = (unsigned int)n;
    f->medianBuffer = buf;
    f->ageHead      = &buf[h];
    f->valueHead    = &buf[order[0]];
    f->medianHead   = &buf[order[n / 2]];
}

/* same state, compared by node index (the two filters use different buffers) */
static int same_state(const MedianFilter_t *a, const MedianFilterNode_t *abuf,
                      const MedianFilter_t *b, const MedianFilterNode_t *bbuf, int n)
{
    if (a->numNodes != b->numNodes || a->medianBuffer != abuf)
        return 0;
    if (a->ageHead - abuf != b->ageHead - bbuf ||
        a->valueHead - abuf != b->valueHead - bbuf ||
        a->medianHead - abuf != b->medianHead - bbuf)
        return 0;
    for (int i = 0; i < n; i++) {
        if (abuf[i].value != bbuf[i].value ||
            abuf[i].nextValue - abuf != bbuf[i].nextValue - bbuf ||
            abuf[i].prevValue - abuf != bbuf[i].prevValue - bbuf)
            return 0;
    }
    return 1;
}

typedef struct {
    MedianFilterNode_t *exact;  /* exactly n nodes: the buffer of the filter under test */
    int n;
    int extreme;
    int rank[MAX_NODES];          /* rank[0..n-1]: stored values by slot, rank[n]: the new sample */
    int used[MAX_NODES];
    long long orders;
    long long transitions;
    long long failures;
} enum_state;

static void check_transition(enum_state *e, int h)
{
    const int n = e->n, len = n + 1;
    MedianFilterNode_t *buf = e->exact, want_buf[MAX_NODES];
    MedianFilter_t filter, want;
    int value[MAX_NODES], new_rank[MAX_NODES], new_value[MAX_NODES], order[MAX_NODES];

    for (int i = 0; i < n; i++) {
        value[i]     = map_value(e->rank[i], len, e->extreme);
        new_rank[i]  = e->rank[i];
        new_value[i] = value[i];
    }
    const int sample = map_value(e->rank[n], len, e->extreme);
    new_rank[h]  = e->rank[n];
    new_value[h] = sample;

    build_state(&filter, buf, e->rank, value, n, h);
    const int got = MEDIANFILTER_Insert(&filter, sample);

    build_state(&want, want_buf, new_rank, new_value, n, (h + 1) % n);
    canonical_order(new_rank, n, (h + 1) % n, order);
    const int expected = new_value[order[n / 2]];

    e->transitions++;
    if (got != expected || !same_state(&filter, buf, &want, want_buf, n)) {
        if (e->failures < 5) {
            printf("         N=%d oldest slot=%d sample rank=%d ranks=[", n, h, e->rank[n]);
            for (int i = 0; i < n; i++)
                printf("%s%d", i ? "," : "", e->rank[i]);
            printf("]  got=%d expected=%d%s\n", got, expected, got == expected ? " (state differs)" : "");
        }
        e->failures++;
    }
}

/* every weak order of n + 1 elements: rank vectors whose ranks are exactly 0..k-1 */
static void enumerate(enum_state *e, int pos, int distinct, int max_rank)
{
    const int len = e->n + 1;

    if (pos == len) {
        if (distinct == max_rank + 1) {
            e->orders++;
            for (int h = 0; h < e->n; h++)
                check_transition(e, h);
        }
        return;
    }
    for (int r = 0; r < len; r++) {
        const int new_max      = (r > max_rank) ? r : max_rank;
        const int new_distinct = distinct + (e->used[r] == 0);
        if ((new_max + 1) - new_distinct > len - pos - 1)
            continue;                       /* the remaining slots cannot fill the rank gaps */
        e->rank[pos] = r;
        e->used[r]++;
        enumerate(e, pos + 1, new_distinct, new_max);
        e->used[r]--;
    }
}

/* induction base: Init yields the canonical all-zero state (oldest slot 0) */
static int init_is_canonical(int n)
{
    MedianFilterNode_t want_buf[MAX_NODES];
    MedianFilter_t filter, want;
    int zero[MAX_NODES];
    int ok;

    MedianFilterNode_t *buf = (MedianFilterNode_t *)malloc((size_t)n * sizeof(MedianFilterNode_t));
    if (buf == NULL)
        return 0;
    for (int i = 0; i < n; i++)
        zero[i] = 0;
    filter.numNodes     = (unsigned int)n;
    filter.medianBuffer = buf;
    ok = (MEDIANFILTER_Init(&filter) == 0);
    if (ok) {
        build_state(&want, want_buf, zero, zero, n, 0);
        ok = same_state(&filter, buf, &want, want_buf, n);
    }
    free(buf);
    return ok;
}

int main(void)
{
    TEST_SUITE_BEGIN("Exhaustive State Test");
    printf("config: MEDIANFILTER_SMALL_MAX=%d, search from previous: %s, inline API: %s\n",
           MEDIANFILTER_SMALL_MAX,
#ifdef MEDIANFILTER_SEARCH_FROM_PREVIOUS
           "on",
#else
           "off",
#endif
#ifdef MEDIANFILTER_INLINE_API
           "on");
#else
           "off");
#endif

    TEST_SECTION("Init produces the canonical state");
    {
        int ok = 1;
        for (int n = 3; n <= 15; n += 2)
            if (!init_is_canonical(n))
                ok = 0;
        TEST_BEGIN("Init, windows 3..15");
        if (ok) TEST_PASS();
        else    TEST_FAIL("Init state is not the canonical all-zero state");
    }

    TEST_SECTION("Every state and every sample");
    for (int n = 3; n <= EXHAUSTIVE_MAX_N; n += 2) {
        for (int extreme = 0; extreme <= 1; extreme++) {
            enum_state e = { 0 };
            e.n = n;
            e.extreme = extreme;
            e.exact = (MedianFilterNode_t *)malloc((size_t)n * sizeof(MedianFilterNode_t));
            if (e.exact == NULL) {
                TEST_BEGIN("allocation");
                TEST_FAIL("out of memory");
                continue;
            }
            enumerate(&e, 0, 0, -1);
            free(e.exact);

            char label[160];
            snprintf(label, sizeof(label), "N=%d %s path, %s values: %lld weak orders x %d = %lld transitions",
                     n, (n <= NETWORK_MAX) ? "network" : "chain", extreme ? "INT_MIN..INT_MAX" : "small",
                     e.orders, n, e.transitions);
            TEST_BEGIN(label);
            if (e.orders != fubini[n + 1])
                TEST_FAIL("enumerated %lld weak orders, expected %lld", e.orders, fubini[n + 1]);
            else if (e.failures)
                TEST_FAIL("%lld of %lld transitions wrong", e.failures, e.transitions);
            else
                TEST_PASS();
        }
    }

    TEST_SUITE_END();
}
