#include "MedianFilter.h"
#include "test_harness.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>

/* Brute-force reference: sort a copy of the window array, return middle element */
static int reference_median(const int *window, int n)
{
    int tmp[256];
    memcpy(tmp, window, n * sizeof(int));
    /* Simple insertion sort */
    for (int i = 1; i < n; i++) {
        int key = tmp[i];
        int j = i - 1;
        while (j >= 0 && tmp[j] > key) {
            tmp[j + 1] = tmp[j];
            j--;
        }
        tmp[j + 1] = key;
    }
    return tmp[n / 2];
}

static int window_buf[256]; /* circular buffer for the sliding window */

/* Returns number of sample mismatches (0 = pass) */
static int run_test(int win_size, const int *samples, int num_samples)
{
    int failed = 0;

    sMedianFilter_t filter;
    sMedianNode_t nodes[256];
    filter.numNodes = win_size;
    filter.medianBuffer = nodes;
    MEDIANFILTER_Init(&filter);

    for (int i = 0; i < num_samples; i++) {
        /* Update sliding window */
        if (i < win_size)
            window_buf[i] = samples[i];
        else
            window_buf[i % win_size] = samples[i];

        int actual = MEDIANFILTER_Insert(&filter, samples[i]);

        int ref_window[256];
        if (i < win_size) {
            for (int j = 0; j < win_size; j++)
                ref_window[j] = 0;
            for (int j = 0; j <= i; j++)
                ref_window[j] = samples[j];
        } else {
            for (int j = 0; j < win_size; j++)
                ref_window[j] = samples[i - win_size + 1 + j];
        }
        int expected = reference_median(ref_window, win_size);

        if (actual != expected) {
            if (!failed) {
                printf("         win=%d sample_idx=%d: inserted=%d, got=%d, expected=%d\n",
                       win_size, i, samples[i], actual, expected);
                printf("         Window: [");
                for (int j = 0; j < win_size; j++)
                    printf("%d%s", ref_window[j], j < win_size - 1 ? ", " : "");
                printf("]\n");
            }
            failed++;
        }
    }
    return failed;
}

#define MAX_SAMPLES 10000

static int samples[MAX_SAMPLES];

#define RUN(test_name, ws) \
    do { \
        int fails = run_test(ws, samples, MAX_SAMPLES); \
        TEST_BEGIN(test_name); \
        if (fails) TEST_FAIL("win=%d, %d sample mismatches", ws, fails); \
        else       TEST_PASS(); \
    } while (0)

int main(void)
{
    int win_sizes[] = {3, 5, 7, 9, 11, 15, 21, 31, 51, 99};
    int num_wins = sizeof(win_sizes) / sizeof(win_sizes[0]);

    TEST_SUITE_BEGIN("Median Filter Stress Test");

    for (int w = 0; w < num_wins; w++) {
        int ws = win_sizes[w];
        char section[64];
        snprintf(section, sizeof(section), "Window size %d", ws);
        TEST_SECTION(section);

        for (int i = 0; i < MAX_SAMPLES; i++) samples[i] = 42;
        RUN("all_same", ws);

        for (int i = 0; i < MAX_SAMPLES; i++) samples[i] = i;
        RUN("ascending", ws);

        for (int i = 0; i < MAX_SAMPLES; i++) samples[i] = MAX_SAMPLES - i;
        RUN("descending", ws);

        for (int i = 0; i < MAX_SAMPLES; i++) samples[i] = (i % 2) ? INT_MAX : 0;
        RUN("alt_min_max", ws);

        for (int i = 0; i < MAX_SAMPLES; i++) samples[i] = i % (ws + 1);
        RUN("sawtooth", ws);

        for (int i = 0; i < MAX_SAMPLES; i++) samples[i] = (i == ws * 2) ? 9999 : 0;
        RUN("zero_spike", ws);

        srand(12345);
        for (int i = 0; i < MAX_SAMPLES; i++) samples[i] = rand() % 5;
        RUN("rand_dups", ws);

        srand(54321);
        for (int i = 0; i < MAX_SAMPLES; i++) {
            int r = rand() % 4;
            if (r == 0) samples[i] = INT_MIN;
            else if (r == 1) samples[i] = INT_MAX;
            else if (r == 2) samples[i] = 0;
            else samples[i] = rand() - RAND_MAX / 2;
        }
        RUN("int_extremes", ws);

        srand(99999);
        for (int i = 0; i < MAX_SAMPLES; i++) samples[i] = rand();
        RUN("rand_uniform", ws);

        for (int i = 0; i < MAX_SAMPLES; i++) samples[i] = (i % 2) ? 100 : 200;
        RUN("two_val_alt", ws);

        for (int i = 0; i < MAX_SAMPLES; i++) samples[i] = (i < MAX_SAMPLES / 2) ? 0 : 1000;
        RUN("step_func", ws);

        for (int i = 0; i < MAX_SAMPLES; i++) samples[i] = -MAX_SAMPLES + i;
        RUN("negative_asc", ws);

        srand(77777);
        for (int i = 0; i < MAX_SAMPLES; i++) samples[i] = rand() % 3;
        RUN("rand_narrow", ws);

        for (int i = 0; i < MAX_SAMPLES; i++) samples[i] = (i % 5 == 0) ? 100 : 1;
        RUN("repeated_pat", ws);
    }

    TEST_SUITE_END();
}
