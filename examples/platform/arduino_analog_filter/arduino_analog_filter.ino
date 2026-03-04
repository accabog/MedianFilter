/*
 * arduino_analog_filter.ino — Arduino: Smooth analogRead() with median filter.
 *
 * Copy MedianFilter.h and MedianFilter.c into your Arduino sketch folder,
 * then use this sketch directly.
 *
 * Wiring: connect an analog sensor (potentiometer, LDR, thermistor)
 * to pin A0.
 */

#include "MedianFilter.h"

#define SENSOR_PIN   A0
#define WINDOW_SIZE  5

static sMedianFilter_t filter;
static sMedianNode_t   buffer[WINDOW_SIZE];

void setup()
{
    Serial.begin(115200);

    filter.numNodes     = WINDOW_SIZE;
    filter.medianBuffer = buffer;
    MEDIANFILTER_Init(&filter);

    Serial.println("MedianFilter Arduino Demo");
    Serial.println("Raw\tFiltered");
}

void loop()
{
    int raw      = analogRead(SENSOR_PIN);
    int filtered = MEDIANFILTER_Insert(&filter, raw);

    Serial.print(raw);
    Serial.print('\t');
    Serial.println(filtered);

    delay(50);  /* 20 Hz sampling rate */
}
