/*
 * callgrind_bench_vpetrigo.c — Instruction-count benchmark for vpetrigo/median-filter.
 *
 * Usage: ./callgrind_bench_vpetrigo <window_size>
 */
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "median_filter.h"

#define NUM_SAMPLES 10000

int main(int argc, char *argv[]) {
    if (argc < 2) return 1;
    int window = atoi(argv[1]);
    if (window < 3 || !(window & 1)) return 1;

    unsigned seed = (argc >= 3) ? (unsigned)atoi(argv[2]) : 42;

    static int samples[NUM_SAMPLES];
    srand(seed);
    for (int i = 0; i < NUM_SAMPLES; i++)
        samples[i] = rand();

    struct median_filter_node_int32_t buf[window];
    struct median_filter_int32_t filter;
    median_filter_init_int32_t(&filter, buf, (size_t)window);

    volatile int32_t sink;
    for (int i = 0; i < NUM_SAMPLES; i++) {
        median_filter_insert_value_int32_t(&filter, (int32_t)samples[i]);
        sink = median_filter_get_median_int32_t(&filter);
    }

    return (int)(sink & 0);
}
