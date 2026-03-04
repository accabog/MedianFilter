/*
 * callgrind_bench_takingbytes.c — Instruction-count benchmark for takingBytes/MovingMedianFilter.
 *
 * Usage: ./callgrind_bench_takingbytes <window_size>
 */
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "competitors/takingbytes_wrapper.h"

#define NUM_SAMPLES 10000

int main(int argc, char *argv[]) {
    if (argc < 2) return 1;
    int window = atoi(argv[1]);
    if (window < 3 || !(window & 1)) return 1;

    unsigned seed = (argc >= 3) ? (unsigned)atoi(argv[2]) : 42;

    static float samples[NUM_SAMPLES];
    srand(seed);
    for (int i = 0; i < NUM_SAMPLES; i++)
        samples[i] = (float)rand() / (float)RAND_MAX * 2000.0f - 1000.0f;

    float buf[window];
    float *sorted_ptrs[window];
    char med_storage[64] __attribute__((aligned(8)));
    TBMedian *med = (TBMedian *)med_storage;

    TBMedian_Init(med, buf, sorted_ptrs, (uint16_t)window);

    volatile float sink;
    for (int i = 0; i < NUM_SAMPLES; i++)
        sink = TBMedian_Filter(med, samples[i]);

    (void)sink;
    return 0;
}
