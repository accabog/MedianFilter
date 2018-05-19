/*
 * 02_adc_noise_filter.c — ADC spike noise removal demo.
 *
 * Build:  gcc -I.. -O2 -o adc_noise_filter 02_adc_noise_filter.c ../MedianFilter.c -lm
 * Run:    ./adc_noise_filter
 *
 * Demonstrates:
 *   - Removing impulse noise (salt-and-pepper spikes) from an analog signal
 *   - Typical ADC filtering use case
 *
 * Simulates a 12-bit ADC reading a slow sine wave. Random spikes are injected
 * to mimic electrical noise. The median filter removes the spikes while
 * preserving the shape of the underlying signal.
 */

#include "MedianFilter.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define WINDOW_SIZE     7
#define NUM_SAMPLES     60
#define ADC_MAX         4095    /* 12-bit ADC */
#define SPIKE_CHANCE    15      /* percent chance of a spike per sample */

static sMedianFilter_t filter;
static sMedianNode_t   buffer[WINDOW_SIZE];

/* Simulate a 12-bit ADC reading a sine wave with injected spikes */
static int simulate_adc(int step)
{
    /* Clean signal: sine wave centered at mid-range */
    double angle = 2.0 * 3.14159265 * step / NUM_SAMPLES;
    int clean = (int)(2048.0 + 1800.0 * sin(angle));

    /* Inject spike noise */
    if ((rand() % 100) < SPIKE_CHANCE) {
        clean = (rand() % 2) ? ADC_MAX : 0;
    }

    return clean;
}

int main(void)
{
    srand(42);  /* fixed seed for reproducible output */

    filter.numNodes     = WINDOW_SIZE;
    filter.medianBuffer = buffer;
    MEDIANFILTER_Init(&filter);

    printf("ADC Noise Filter Demo (window=%d)\n", WINDOW_SIZE);
    printf("%-8s %-8s %-8s\n", "Sample#", "Raw", "Filtered");
    printf("-------- -------- --------\n");

    int spike_count = 0;
    double error_raw = 0, error_filtered = 0;

    for (int i = 0; i < NUM_SAMPLES; i++) {
        /* Ground truth: clean sine */
        double angle = 2.0 * 3.14159265 * i / NUM_SAMPLES;
        int clean = (int)(2048.0 + 1800.0 * sin(angle));

        int raw = simulate_adc(i);
        int filtered = MEDIANFILTER_Insert(&filter, raw);

        if (raw != clean) spike_count++;
        error_raw      += (raw - clean) * (raw - clean);
        error_filtered += (filtered - clean) * (filtered - clean);

        printf("%-8d %-8d %-8d%s\n", i, raw, filtered,
               (raw != clean) ? "  <-- spike" : "");
    }

    printf("\nSpikes injected: %d / %d\n", spike_count, NUM_SAMPLES);
    printf("RMS error (raw):      %.1f\n", sqrt(error_raw / NUM_SAMPLES));
    printf("RMS error (filtered): %.1f\n", sqrt(error_filtered / NUM_SAMPLES));

    return 0;
}
