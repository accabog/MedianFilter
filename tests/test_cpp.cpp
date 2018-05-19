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

    TEST_SUITE_END();
}
