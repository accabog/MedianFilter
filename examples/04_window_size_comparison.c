/*
 * 04_window_size_comparison.c — Compare filter behavior across window sizes.
 *
 * Build:  gcc -I.. -O2 -o window_compare 04_window_size_comparison.c ../MedianFilter.c -lm
 * Run:    ./window_compare
 *
 * Demonstrates:
 *   - How window size affects noise removal vs. signal lag
 *   - Helps users choose the right window size for their application
 *
 * The same noisy step signal is fed through three filters with different
 * window sizes. Larger windows remove more noise but introduce more lag.
 */

#include "MedianFilter.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* Three filter instances with different window sizes */
#define WIN_SMALL   3
#define WIN_MEDIUM  7
#define WIN_LARGE  15

static sMedianFilter_t filter_s, filter_m, filter_l;
static sMedianNode_t   buf_s[WIN_SMALL], buf_m[WIN_MEDIUM], buf_l[WIN_LARGE];

#define NUM_SAMPLES 40
#define NOISE_AMP   50

int main(void)
{
    srand(7);

    filter_s.numNodes = WIN_SMALL;  filter_s.medianBuffer = buf_s;
    filter_m.numNodes = WIN_MEDIUM; filter_m.medianBuffer = buf_m;
    filter_l.numNodes = WIN_LARGE;  filter_l.medianBuffer = buf_l;
    MEDIANFILTER_Init(&filter_s);
    MEDIANFILTER_Init(&filter_m);
    MEDIANFILTER_Init(&filter_l);

    printf("Window Size Comparison\n");
    printf("Signal: step from 100 to 500 at sample 15, with +/-%d noise\n\n", NOISE_AMP);
    printf("%-7s %-8s %-8s %-8s %-8s %-8s\n",
           "Sample", "Clean", "Noisy", "Win=3", "Win=7", "Win=15");
    printf("------- -------- -------- -------- -------- --------\n");

    for (int i = 0; i < NUM_SAMPLES; i++) {
        /* Step signal: 100 for first 15 samples, then 500 */
        int clean = (i < 15) ? 100 : 500;

        /* Add uniform noise + occasional spike */
        int noisy = clean + (rand() % (2 * NOISE_AMP + 1)) - NOISE_AMP;
        if ((rand() % 100) < 10) {
            noisy = (rand() % 2) ? 900 : 0;  /* spike */
        }

        int out_s = MEDIANFILTER_Insert(&filter_s, noisy);
        int out_m = MEDIANFILTER_Insert(&filter_m, noisy);
        int out_l = MEDIANFILTER_Insert(&filter_l, noisy);

        printf("%-7d %-8d %-8d %-8d %-8d %-8d\n",
               i, clean, noisy, out_s, out_m, out_l);
    }

    printf("\nObserve: larger windows remove more noise but are slower to\n");
    printf("track the step change at sample 15.\n");

    return 0;
}
