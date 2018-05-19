/*
 * 01_basic_usage.c — Minimal MedianFilter usage in C.
 *
 * Build:  gcc -I.. -o basic_usage 01_basic_usage.c ../MedianFilter.c
 * Run:    ./basic_usage
 *
 * Demonstrates:
 *   - Allocating the filter struct and node buffer (static, no heap)
 *   - Initializing with MEDIANFILTER_Init()
 *   - Inserting samples and reading the running median
 */

#include "MedianFilter.h"
#include <stdio.h>

#define WINDOW_SIZE  5

static sMedianFilter_t filter;
static sMedianNode_t   buffer[WINDOW_SIZE];

int main(void)
{
    /* 1. Configure: set window size and point to the node buffer */
    filter.numNodes     = WINDOW_SIZE;
    filter.medianBuffer = buffer;

    /* 2. Initialize the filter (returns 0 on success) */
    if (MEDIANFILTER_Init(&filter) != 0) {
        fprintf(stderr, "Error: filter init failed\n");
        return 1;
    }

    /* 3. Feed samples and observe the running median */
    int samples[] = {3, 1, 4, 1, 5, 9, 2, 6, 5, 3};
    int n = sizeof(samples) / sizeof(samples[0]);

    printf("Sample -> Median\n");
    printf("----------------\n");
    for (int i = 0; i < n; i++) {
        int median = MEDIANFILTER_Insert(&filter, samples[i]);
        printf("  %4d -> %d\n", samples[i], median);
    }

    return 0;
}
