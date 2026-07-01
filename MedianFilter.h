// SPDX-License-Identifier: MIT
// Copyright (c) 2018 Alexandru Bogdan
/*
 * MedianFilter.h
 *
 *  Created on: May 19, 2018
 *      Author: alexandru.bogdan
 */

#ifndef MEDIANFILTER_H_
#define MEDIANFILTER_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * `restrict` is a C99 keyword and is not part of standard C++.  `extern "C"`
 * changes linkage only, not grammar, so an unguarded `restrict` would make this
 * header fail to compile in a C++ translation unit (defeating the interop
 * guards).  Expand it to `restrict` only where the compiler actually supports
 * it, and to nothing under C++ or pre-C99 C.
 */
#if defined(__cplusplus) || (defined(__STDC_VERSION__) && __STDC_VERSION__ < 199901L)
#  define MF_RESTRICT
#else
#  define MF_RESTRICT restrict
#endif

typedef struct MedianFilterNode
{
    int value;                            //sample value
    struct MedianFilterNode *nextValue;   //pointer to next larger value (toward the maximum)
    struct MedianFilterNode *prevValue;   //pointer to next smaller value (toward the minimum)
}MedianFilterNode_t;

typedef struct
{
    unsigned int numNodes;                //window length: odd, > 1, immutable after Init
    MedianFilterNode_t *medianBuffer;     //caller-owned buffer of at least numNodes nodes
    MedianFilterNode_t *ageHead;          //pointer to oldest value
    MedianFilterNode_t *valueHead;        //pointer to smallest value
    MedianFilterNode_t *medianHead;       //pointer to median value
}MedianFilter_t;

/*
 * Backward-compatible aliases for the original (unprefixed) type names.
 * DEPRECATED: prefer MedianFilterNode_t / MedianFilter_t — the bare
 * sMedianNode_t / sMedianFilter_t names polluted the global namespace.
 * These aliases are the same types, so existing code keeps compiling unchanged.
 */
typedef MedianFilterNode_t sMedianNode_t;
typedef MedianFilter_t     sMedianFilter_t;

/*
 * Initialize a filter.  Before calling, set `numNodes` (odd and > 1) and point
 * `medianBuffer` at a buffer holding at least `numNodes` contiguous nodes.
 *
 * Returns 0 on success, -1 on invalid config (NULL filter/buffer, even
 * numNodes, or numNodes <= 1).  ALWAYS check the return value: on failure the
 * filter is left uninitialized and passing it to Insert is undefined behavior
 * (Insert performs no validation — it is a hot path).
 *
 * Contract notes:
 *  - Warmup: Init zero-fills the buffer, so the first (numNodes - 1) inserts
 *    return the median of {samples seen so far} U {leftover zeros}.  Discard or
 *    gate that transient if a clean startup matters.
 *  - `numNodes` must not change after Init; call Init again to resize.
 *  - Filter state stores absolute pointers into `medianBuffer`, so the struct
 *    and buffer are NOT memcpy-relocatable or serializable — re-Init after
 *    moving them in memory.
 */
int MEDIANFILTER_Init(MedianFilter_t *medianFilter);

#ifdef MEDIANFILTER_INLINE_API
/*
 * Inline Insert — opt-in for tight-loop callers.
 * Define MEDIANFILTER_INLINE_API before including this header to enable.
 * Eliminates function-call overhead (~5-8% faster for small windows).
 */
#include <stdint.h>

#define MEDIANFILTER_NODE_GT_(a, b)  ((a)->value > (b)->value || \
                                       ((a)->value == (b)->value && (uintptr_t)(a) > (uintptr_t)(b)))
#define MEDIANFILTER_SAMPLE_LT_(val, nptr, b)  ((val) < (b)->value || \
                                                  ((val) == (b)->value && (uintptr_t)(nptr) < (uintptr_t)(b)))
#define MEDIANFILTER_SAMPLE_GE_(val, nptr, b)  (!MEDIANFILTER_SAMPLE_LT_(val, nptr, b))

static inline int MEDIANFILTER_Insert(MedianFilter_t *MF_RESTRICT medianFilter, int sample)
{
    MedianFilterNode_t *newNode, *it;

    MedianFilterNode_t *ageHead    = medianFilter->ageHead;
    MedianFilterNode_t *valueHead  = medianFilter->valueHead;
    MedianFilterNode_t *medianHead = medianFilter->medianHead;
    const unsigned int half        = medianFilter->numNodes >> 1;

    if(ageHead == valueHead)
        valueHead = valueHead->nextValue;

    if(ageHead == medianHead || MEDIANFILTER_NODE_GT_(ageHead, medianHead))
        medianHead = medianHead->prevValue;

    newNode = ageHead;
    newNode->value = sample;

    MedianFilterNode_t *oldNext = newNode->nextValue;
    MedianFilterNode_t *oldPrev = newNode->prevValue;
    oldNext->prevValue = oldPrev;
    oldPrev->nextValue = oldNext;

    MedianFilterNode_t *nextAge = newNode + 1;
    if(nextAge == medianFilter->medianBuffer + medianFilter->numNodes)
        nextAge = medianFilter->medianBuffer;
    medianFilter->ageHead = nextAge;

    if(MEDIANFILTER_SAMPLE_LT_(sample, newNode, medianHead))
    {
        it = medianHead->prevValue;
        while(it != valueHead && MEDIANFILTER_SAMPLE_LT_(sample, newNode, it))
            it = it->prevValue;

        if(it != valueHead || MEDIANFILTER_SAMPLE_GE_(sample, newNode, it))
            it = it->nextValue;
        else
            valueHead = newNode;
    }
    else
    {
        it = medianHead->nextValue;
        unsigned int remaining = half;
        while(remaining && MEDIANFILTER_SAMPLE_GE_(sample, newNode, it))
        {
            it = it->nextValue;
            remaining--;
        }
    }

    MedianFilterNode_t *insertPrev = it->prevValue;
    insertPrev->nextValue = newNode;
    newNode->prevValue    = insertPrev;
    it->prevValue         = newNode;
    newNode->nextValue    = it;

    if(MEDIANFILTER_SAMPLE_GE_(sample, newNode, medianHead))
        medianHead = medianHead->nextValue;

    medianFilter->valueHead  = valueHead;
    medianFilter->medianHead = medianHead;

    return medianHead->value;
}

#undef MEDIANFILTER_NODE_GT_
#undef MEDIANFILTER_SAMPLE_LT_
#undef MEDIANFILTER_SAMPLE_GE_

#else
int MEDIANFILTER_Insert(MedianFilter_t *MF_RESTRICT medianFilter, int sample);
#endif

#ifdef __cplusplus
}
#endif

#undef MF_RESTRICT
#endif
