/* Wrapper that calls the library (non-inlined) MEDIANFILTER_Insert,
 * so the benchmark can compare inlined vs non-inlined in the same run. */
#include "MedianFilter.h"

int MEDIANFILTER_Insert_NoInline(sMedianFilter_t *restrict filter, int sample) {
    return MEDIANFILTER_Insert(filter, sample);
}
