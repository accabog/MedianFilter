/*
 * 05_cpp_template.cpp — C++ header-only template demo.
 *
 * Build:  g++ -std=c++17 -I.. -o cpp_template 05_cpp_template.cpp -lm
 * Run:    ./cpp_template
 *
 * Demonstrates:
 *   - MedianFilter<float, N>  — floating-point sensor data
 *   - MedianFilter<int16_t, N> — memory-efficient 16-bit ADC values
 *   - MedianFilter<double, N> — high-precision scientific data
 *   - Compile-time enforcement of odd window size and arithmetic type
 */

#include "MedianFilter.hpp"
#include <cstdio>
#include <cstdint>
#include <cmath>
#include <cstdlib>

/* ── Float filter: thermistor temperature readings ─────────────── */

static void demo_float(void)
{
    MedianFilter<float, 5> filter;

    float readings[] = {23.1f, 23.3f, 99.9f, 23.2f, 23.4f,
                        23.1f, -40.0f, 23.3f, 23.5f, 23.2f};
    int n = sizeof(readings) / sizeof(readings[0]);

    printf("=== Float filter (window=5): Thermistor ===\n");
    printf("%-10s %-10s %-10s\n", "Reading", "Raw", "Filtered");
    for (int i = 0; i < n; i++) {
        float median = filter.Insert(readings[i]);
        printf("%-10d %-10.1f %-10.1f%s\n", i, (double)readings[i], (double)median,
               (readings[i] > 50.0f || readings[i] < -10.0f) ? "  <-- spike" : "");
    }
    printf("\n");
}

/* ── int16_t filter: memory-efficient 12-bit ADC ───────────────── */

static void demo_int16(void)
{
    MedianFilter<int16_t, 7> filter;

    /* Simulated 12-bit ADC values (0-4095) with spikes */
    int16_t readings[] = {2048, 2052, 2045, 4095, 2050, 2047, 0,
                          2051, 2049, 2053, 2046, 4095, 2048, 2050};
    int n = sizeof(readings) / sizeof(readings[0]);

    printf("=== int16_t filter (window=7): 12-bit ADC ===\n");
    printf("%-10s %-10s %-10s\n", "Reading", "Raw", "Filtered");
    for (int i = 0; i < n; i++) {
        int16_t median = filter.Insert(readings[i]);
        printf("%-10d %-10d %-10d%s\n", i, readings[i], median,
               (readings[i] == 4095 || readings[i] == 0) ? "  <-- spike" : "");
    }
    printf("\n");
}

/* ── Double filter: scientific measurement ─────────────────────── */

static void demo_double(void)
{
    MedianFilter<double, 3> filter;

    /* High-precision pressure sensor readings (Pa) */
    double readings[] = {101325.42, 101325.38, 999999.99,
                         101325.45, 101325.41, 0.01,
                         101325.39, 101325.44, 101325.40};
    int n = sizeof(readings) / sizeof(readings[0]);

    printf("=== Double filter (window=3): Pressure sensor ===\n");
    printf("%-10s %-14s %-14s\n", "Reading", "Raw", "Filtered");
    for (int i = 0; i < n; i++) {
        double median = filter.Insert(readings[i]);
        printf("%-10d %-14.2f %-14.2f%s\n", i, readings[i], median,
               (readings[i] > 200000.0 || readings[i] < 1.0) ? "  <-- spike" : "");
    }
    printf("\n");
}

int main(void)
{
    printf("MedianFilter C++ Template Demo\n");
    printf("==============================\n\n");

    demo_float();
    demo_int16();
    demo_double();

    printf("All types use the same MedianFilter<T, N> template.\n");
    printf("Window size and type are validated at compile time.\n");

    return 0;
}
