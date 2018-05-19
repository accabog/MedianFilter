/*
 * stm32_hal_adc.c — STM32 HAL: Median-filter ADC readings in an interrupt.
 *
 * REFERENCE ONLY — not compilable standalone.
 * Requires STM32 HAL drivers and a configured CubeMX project.
 *
 * Shows how to integrate MedianFilter into an STM32 ADC interrupt callback.
 * Adapt the ADC channel and pin config to your specific MCU.
 */

#include "MedianFilter.h"
#include "stm32f4xx_hal.h"   /* adjust for your MCU family */

#define WINDOW_SIZE  7

static sMedianFilter_t adc_filter;
static sMedianNode_t   adc_buffer[WINDOW_SIZE];
static volatile int     filtered_adc = 0;

void MedianFilter_Setup(void)
{
    adc_filter.numNodes     = WINDOW_SIZE;
    adc_filter.medianBuffer = adc_buffer;
    MEDIANFILTER_Init(&adc_filter);
}

/*
 * Called by HAL when ADC conversion completes.
 * Place this in your stm32f4xx_it.c or main.c.
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    int raw = (int)HAL_ADC_GetValue(hadc);
    filtered_adc = MEDIANFILTER_Insert(&adc_filter, raw);
}

/*
 * In your main loop or RTOS task, read filtered_adc:
 *
 *   int value = filtered_adc;   // already spike-free
 */
