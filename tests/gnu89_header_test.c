/*
 * gnu89_header_test.c
 *
 * Compile-only check: MedianFilter.h must stay usable from C90 code with GNU
 * extensions (gcc -std=gnu89, also the default mode of some embedded compilers).
 * C90 does not define __STDC_VERSION__, so MF_RESTRICT must expand to nothing.
 */

#include "MedianFilter.h"

int gnu89_header_test(MedianFilter_t *filter, int sample);

int gnu89_header_test(MedianFilter_t *filter, int sample)
{
    return MEDIANFILTER_Insert(filter, sample);
}
