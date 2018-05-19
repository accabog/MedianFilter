/*
 * 03_multichannel_sensors.c — Multiple independent filters on different sensors.
 *
 * Build:  gcc -I.. -o multichannel 03_multichannel_sensors.c ../MedianFilter.c
 * Run:    ./multichannel
 *
 * Demonstrates:
 *   - Running multiple independent filter instances simultaneously
 *   - Using different window sizes per channel
 *   - Zero-heap, fully static allocation pattern
 *
 * Simulates three sensor channels (temperature, humidity, pressure), each
 * with its own filter instance and window size.
 */

#include "MedianFilter.h"
#include <stdio.h>
#include <stdlib.h>

/* Each channel gets its own filter + buffer — fully independent, zero heap */

#define TEMP_WINDOW   5
#define HUMID_WINDOW  7
#define PRESS_WINDOW  9

static sMedianFilter_t temp_filter;
static sMedianNode_t   temp_buffer[TEMP_WINDOW];

static sMedianFilter_t humid_filter;
static sMedianNode_t   humid_buffer[HUMID_WINDOW];

static sMedianFilter_t press_filter;
static sMedianNode_t   press_buffer[PRESS_WINDOW];

/* Helper: init a filter with its buffer and window size */
static int init_filter(sMedianFilter_t *f, sMedianNode_t *buf, unsigned int size)
{
    f->numNodes     = size;
    f->medianBuffer = buf;
    return MEDIANFILTER_Init(f);
}

/* Simulated sensor readings with occasional noise */
static int read_temperature(int step)
{
    int base = 2200 + (step % 10) * 5;  /* ~22.00-22.45 C, scaled x100 */
    if ((rand() % 100) < 10) base += (rand() % 2) ? 800 : -800;
    return base;
}

static int read_humidity(int step)
{
    int base = 5500 - (step % 8) * 10;  /* ~55.00-54.30 %RH, scaled x100 */
    if ((rand() % 100) < 12) base += (rand() % 2) ? 1500 : -1500;
    return base;
}

static int read_pressure(int step)
{
    int base = 101325 + (step % 5) * 10;  /* ~1013.25-1013.65 hPa, scaled x100 */
    if ((rand() % 100) < 8) base += (rand() % 2) ? 5000 : -5000;
    return base;
}

int main(void)
{
    srand(123);

    if (init_filter(&temp_filter,  temp_buffer,  TEMP_WINDOW)  != 0 ||
        init_filter(&humid_filter, humid_buffer, HUMID_WINDOW) != 0 ||
        init_filter(&press_filter, press_buffer, PRESS_WINDOW) != 0) {
        fprintf(stderr, "Filter init failed\n");
        return 1;
    }

    printf("Multichannel Sensor Filtering\n");
    printf("Temp: window=%d  |  Humidity: window=%d  |  Pressure: window=%d\n\n",
           TEMP_WINDOW, HUMID_WINDOW, PRESS_WINDOW);
    printf("%-6s | %-14s %-14s | %-14s %-14s | %-14s %-14s\n",
           "Step", "Temp Raw", "Temp Filt",
           "Humid Raw", "Humid Filt",
           "Press Raw", "Press Filt");
    printf("-------|-------------------------------|-------------------------------|"
           "-------------------------------\n");

    for (int i = 0; i < 20; i++) {
        int t_raw = read_temperature(i);
        int h_raw = read_humidity(i);
        int p_raw = read_pressure(i);

        int t_filt = MEDIANFILTER_Insert(&temp_filter,  t_raw);
        int h_filt = MEDIANFILTER_Insert(&humid_filter, h_raw);
        int p_filt = MEDIANFILTER_Insert(&press_filter, p_raw);

        printf("%-6d | %6d.%02d C  %6d.%02d C  | %6d.%02d %%   %6d.%02d %%   | %6d.%02d hPa %6d.%02d hPa\n",
               i,
               t_raw / 100, abs(t_raw % 100), t_filt / 100, abs(t_filt % 100),
               h_raw / 100, abs(h_raw % 100), h_filt / 100, abs(h_filt % 100),
               p_raw / 100, abs(p_raw % 100), p_filt / 100, abs(p_filt % 100));
    }

    return 0;
}
