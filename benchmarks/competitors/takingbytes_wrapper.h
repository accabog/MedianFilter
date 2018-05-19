/* Wrapper to avoid name collision between takingBytes' MedianFilter()
 * C function and our MedianFilter<> C++ template class. */
#ifndef TAKINGBYTES_WRAPPER_H
#define TAKINGBYTES_WRAPPER_H

#include <stdint.h>

typedef struct TBMedian TBMedian;

#ifdef __cplusplus
extern "C" {
#endif

void TBMedian_Init(TBMedian *m, float *buffer, float **ptSorted, uint16_t size);
float TBMedian_Filter(TBMedian *m, float input);

#ifdef __cplusplus
}
#endif

#endif
