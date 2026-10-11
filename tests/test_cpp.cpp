/*
 * test_cpp.cpp
 *
 * Correctness tests for the C++ MedianFilter<type, N> template.
 * Validates against a brute-force reference median for multiple
 * arithmetic types and window sizes.
 */

#include "MedianFilter.hpp"
#include "test_harness.h"

#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <climits>
#include <limits>

/* Brute-force reference median for any arithmetic type */
template<typename T>
static T reference_median(const T *window, int n)
{
    T tmp[256];
    std::copy(window, window + n, tmp);
    std::sort(tmp, tmp + n);
    return tmp[n / 2];
}

/*
 * Build the reference window accounting for zero-initialized warmup.
 * Before the window is full, unfilled slots are 0 (matching the library).
 */
template<typename T>
static T ref_window_median(const T *history, int count, int window)
{
    T buf[256];
    for (int i = 0; i < window; i++)
        buf[i] = T(0);

    int n = (count < window) ? count : window;
    for (int i = 0; i < n; i++)
        buf[window - n + i] = history[count - n + i];

    return reference_median(buf, window);
}

template<typename T, unsigned int N>
static void run_test(const char *name, const T *samples, int num_samples)
{
    MedianFilter<T, N> filter;
    T history[10000];
    int fails = 0;

    for (int i = 0; i < num_samples; i++) {
        history[i] = samples[i];
        T actual   = filter.Insert(samples[i]);
        T expected = ref_window_median<T>(history, i + 1, N);

        if (actual != expected) {
            if (fails == 0) {
                printf("         N=%u insert #%d: got != expected\n", N, i);
            }
            fails++;
        }
    }

    TEST_BEGIN(name);
    if (fails)
        TEST_FAIL("N=%u, %d sample mismatches out of %d", N, fails, num_samples);
    else
        TEST_PASS();
}

/*
 * Every sequence of `length` samples drawn from {-1, 0, 1} (below, equal to and
 * above the warmup zeros), checked after every insert: windows full of ties at
 * three levels.  (The exhaustive state test of the C API covers every weak
 * order; this covers the template's own code paths on dense ties.)
 */
template<typename T, unsigned int N>
static void run_all_sequences(const char *name, int length)
{
    long long total = 1;
    for (int i = 0; i < length; i++)
        total *= 3;

    long long bad = 0;
    for (long long code = 0; code < total; code++) {
        MedianFilter<T, N> filter;
        T window[N];
        for (unsigned int i = 0; i < N; i++)
            window[i] = T(0);

        long long digits = code;
        for (int i = 0; i < length; i++) {
            T sample = T((int)(digits % 3) - 1);
            digits /= 3;
            window[(unsigned int)i % N] = sample;        /* slot i % N holds the oldest sample */
            if (filter.Insert(sample) != reference_median(window, (int)N)) {
                if (bad == 0)
                    printf("         N=%u sequence #%lld, insert #%d: got != expected\n", N, code, i);
                bad++;
                break;
            }
        }
    }

    TEST_BEGIN(name);
    if (bad)
        TEST_FAIL("N=%u, %lld of %lld sequences wrong", N, bad, total);
    else
        TEST_PASS();
}

/*
 * NaN is unsupported and the medians are then unspecified, but Insert must
 * still return: each walk stops at the minimum or maximum node because the
 * guard before it is the walk's own loop test.  Feeds a mix of NaN, infinities,
 * signed zeros and small values and only checks that every Insert returns (the
 * sanitizer build also checks memory safety).
 */
template<typename T, unsigned int N>
static void run_nan_survival(const char *name)
{
    const T specials[8] = { std::numeric_limits<T>::quiet_NaN(), std::numeric_limits<T>::infinity(),
                            -std::numeric_limits<T>::infinity(), T(0), -T(0), T(1), T(-1), T(2) };
    MedianFilter<T, N> filter;
    unsigned int x = 12345u;
    volatile T sink = T(0);

    for (int i = 0; i < 20000; i++) {
        x = x * 1103515245u + 12345u;
        sink = filter.Insert(specials[(x >> 16) % 8u]);
    }
    (void)sink;

    TEST_BEGIN(name);
    TEST_PASS();
}

/* Generate random int samples in [lo, hi] */
static void fill_random_int(int *buf, int count, int lo, int hi, unsigned seed)
{
    std::srand(seed);
    unsigned range = (unsigned)hi - (unsigned)lo + 1u;
    for (int i = 0; i < count; i++)
        buf[i] = lo + (int)((unsigned)std::rand() % range);
}

static int int_samples[10000];
static float float_samples[10000];
static double double_samples[10000];

int main()
{
    TEST_SUITE_BEGIN("MedianFilter C++ Template Test");

    /* --- int tests, window 3 --- */
    TEST_SECTION("int, window=3");
    {
        int s[] = {5, 3, 8, 1, 9, 2, 7, 4, 6, 0};
        run_test<int, 3>("basic", s, 10);
    }
    {
        int s[] = {42, 42, 42, 42, 42};
        run_test<int, 3>("all_same", s, 5);
    }
    {
        int s[] = {INT_MIN, INT_MAX, 0, INT_MIN, INT_MAX};
        run_test<int, 3>("extremes", s, 5);
    }

    /* --- int tests, window 5 --- */
    TEST_SECTION("int, window=5");
    fill_random_int(int_samples, 10000, -1000000, 1000000, 12345);
    run_test<int, 5>("random_1M", int_samples, 10000);

    fill_random_int(int_samples, 10000, 0, 4, 54321);
    run_test<int, 5>("random_narrow", int_samples, 10000);

    /* --- int tests, window 7 --- */
    TEST_SECTION("int, window=7");
    for (int i = 0; i < 10000; i++) int_samples[i] = i;
    run_test<int, 7>("ascending", int_samples, 10000);

    for (int i = 0; i < 10000; i++) int_samples[i] = 10000 - i;
    run_test<int, 7>("descending", int_samples, 10000);

    for (int i = 0; i < 10000; i++) int_samples[i] = (i % 2) ? 100 : -100;
    run_test<int, 7>("alternating", int_samples, 10000);

    /* --- int tests, larger windows --- */
    TEST_SECTION("int, window=21");
    fill_random_int(int_samples, 10000, -1000000, 1000000, 99999);
    run_test<int, 21>("random_1M", int_samples, 10000);

    /* --- float tests, window 5 --- */
    TEST_SECTION("float, window=5");
    std::srand(11111);
    for (int i = 0; i < 5000; i++)
        float_samples[i] = (float)(std::rand() % 20000 - 10000) / 100.0f;
    run_test<float, 5>("random_float", float_samples, 5000);

    for (int i = 0; i < 5000; i++)
        float_samples[i] = 3.14f;
    run_test<float, 5>("all_same_float", float_samples, 5000);

    /* --- double tests, window 7 --- */
    TEST_SECTION("double, window=7");
    std::srand(22222);
    for (int i = 0; i < 5000; i++)
        double_samples[i] = (double)(std::rand() % 20000 - 10000) / 100.0;
    run_test<double, 7>("random_double", double_samples, 5000);

    /* --- every short sequence over {-1, 0, 1} --- */
    TEST_SECTION("all sequences of 10-12 samples from {-1, 0, 1}");
    run_all_sequences<int, 3>("all_sequences_int_3", 10);
    run_all_sequences<int, 5>("all_sequences_int_5", 11);
    run_all_sequences<int, 7>("all_sequences_int_7", 12);
    run_all_sequences<float, 3>("all_sequences_float_3", 10);
    run_all_sequences<float, 5>("all_sequences_float_5", 11);
    run_all_sequences<float, 7>("all_sequences_float_7", 12);

    /* --- NaN and infinities must not hang Insert --- */
    TEST_SECTION("NaN, infinities and signed zeros: every Insert returns");
    run_nan_survival<float, 3>("nan_survival_float_3");
    run_nan_survival<float, 7>("nan_survival_float_7");
    run_nan_survival<float, 101>("nan_survival_float_101");
    run_nan_survival<double, 5>("nan_survival_double_5");
    run_nan_survival<double, 31>("nan_survival_double_31");

    TEST_SUITE_END();
}
