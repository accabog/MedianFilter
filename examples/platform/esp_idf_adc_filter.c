/*
 * esp_idf_adc_filter.c — ESP-IDF: Filter ADC readings in a FreeRTOS task.
 *
 * REFERENCE ONLY — not compilable standalone.
 * Requires ESP-IDF build system (idf.py).
 *
 * Shows how to integrate MedianFilter into an ESP32 ADC reading task.
 * Adapt ADC channel and attenuation for your hardware.
 */

#include "MedianFilter.h"
#include "esp_adc/adc_oneshot.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define WINDOW_SIZE  7
#define ADC_CHANNEL  ADC_CHANNEL_0   /* GPIO36 on most ESP32 boards */

static const char *TAG = "median_filter";

static sMedianFilter_t adc_filter;
static sMedianNode_t   adc_buffer[WINDOW_SIZE];

static void adc_filter_task(void *arg)
{
    adc_oneshot_unit_handle_t adc_handle = (adc_oneshot_unit_handle_t)arg;
    int raw, filtered;

    adc_filter.numNodes     = WINDOW_SIZE;
    adc_filter.medianBuffer = adc_buffer;
    MEDIANFILTER_Init(&adc_filter);

    while (1) {
        adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw);
        filtered = MEDIANFILTER_Insert(&adc_filter, raw);

        ESP_LOGI(TAG, "Raw: %d  Filtered: %d", raw, filtered);

        vTaskDelay(pdMS_TO_TICKS(50));  /* 20 Hz */
    }
}

/*
 * Call from app_main() after configuring the ADC:
 *
 *   adc_oneshot_unit_handle_t adc_handle;
 *   // ... configure ADC ...
 *   xTaskCreate(adc_filter_task, "adc_filter", 2048, adc_handle, 5, NULL);
 */
