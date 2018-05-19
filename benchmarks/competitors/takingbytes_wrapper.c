#include "takingbytes_wrapper.h"
#include <stdbool.h>
#include "median.h"

/* Static assert that our opaque type matches the real struct size */
_Static_assert(sizeof(median) <= 64, "TBMedian size assumption broken");

void TBMedian_Init(TBMedian *m, float *buffer, float **ptSorted, uint16_t size)
{
    MedianInit((median *)m, buffer, ptSorted, size);
}

float TBMedian_Filter(TBMedian *m, float input)
{
    return MedianFilter((median *)m, input);
}
