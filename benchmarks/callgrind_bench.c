/*
 * callgrind_bench.c — Deterministic instruction-count benchmark for the C API.
 *
 * Usage: ./callgrind_bench <window_size>
 * Run under: valgrind --tool=callgrind --callgrind-out-file=/dev/null ./callgrind_bench 7
 * The "Ir" line in stderr is the total instruction count.
 */
#include <stdlib.h>
#include "MedianFilter.h"

#define NUM_SAMPLES 10000

int main(int argc, char *argv[]) {
    if (argc < 2) return 1;
    int window = atoi(argv[1]);
    if (window < 3 || !(window & 1)) return 1;
    unsigned seed = (argc >= 3) ? (unsigned)atoi(argv[2]) : 42;

    /* Generate deterministic data */
    static int samples[NUM_SAMPLES];
    srand(seed);
    for (int i = 0; i < NUM_SAMPLES; i++)
        samples[i] = rand();

    /* Allocate and run */
    sMedianNode_t buf[window];
    sMedianFilter_t filter;
    filter.numNodes = window;
    filter.medianBuffer = buf;
    MEDIANFILTER_Init(&filter);

    volatile int sink;
    for (int i = 0; i < NUM_SAMPLES; i++)
        sink = MEDIANFILTER_Insert(&filter, samples[i]);

    return (int)sink & 0; /* prevent optimization, return 0 */
}
