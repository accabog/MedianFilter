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
 * guards).  Use `restrict` in C99 and later, the `__restrict` extension in C++
 * on gcc and clang, and nothing elsewhere (MSVC would make `__restrict` part of
 * Insert's function type in C++).  C90 does not define __STDC_VERSION__, so test
 * for C99 positively.
 */
#if defined(__cplusplus)
#  if defined(__GNUC__) || defined(__clang__)
#    define MF_RESTRICT __restrict
#  else
#    define MF_RESTRICT
#  endif
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
#  define MF_RESTRICT restrict
#else
#  define MF_RESTRICT
#endif

/*
 * MEDIANFILTER_SMALL_MAX: windows of up to this many nodes compute the median
 * from the buffer values with a branch-free selection network instead of the
 * sorted value chain.  0 = never, 3 (default) = windows of 3, 5 = windows of 3
 * and 5.  The network for 5 removes the chain's mispredicted branches: it pays
 * off on out-of-order cores (x86-64, AArch64) but executes more instructions on
 * Cortex-M and on RISC-V without Zbb.  The window check costs larger windows a
 * few instructions per sample (2-5 on Cortex-M) and the window-3 network adds
 * about 60-80 bytes of code on Cortex-M; 0 removes both.
 * Define it for the whole build: MedianFilter.c and every translation unit that
 * inserts into the same filter through MEDIANFILTER_INLINE_API must see the same
 * value, because only the chain path maintains the value chain.  (The C++
 * template always uses the network for windows of 3 and ignores this macro.)
 */
#ifndef MEDIANFILTER_SMALL_MAX
#  define MEDIANFILTER_SMALL_MAX 3
#endif
#if (MEDIANFILTER_SMALL_MAX != 0) && (MEDIANFILTER_SMALL_MAX != 3) && (MEDIANFILTER_SMALL_MAX != 5)
#  error "MEDIANFILTER_SMALL_MAX must be 0, 3 or 5"
#endif

/*
 * MEDIANFILTER_SEARCH_FROM_PREVIOUS (opt-in, not defined by default): start the
 * insertion walk at the previous sample's node when it lies on the same side of
 * the median as the new sample.  Fewer list hops for slowly varying signals
 * (ADC readings, sensors) in windows of about 31 and more; more work for
 * uncorrelated noise in windows below about 101 and for periodic or multiplexed
 * input.  Changes the code size by at most about 20 bytes on Cortex-M.  Medians
 * and filter state are identical either way.
 */

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
    MedianFilterNode_t *ageHead;          //internal: oldest node (replaced by the next Insert)
    MedianFilterNode_t *valueHead;        //internal: smallest node of the value chain
    MedianFilterNode_t *medianHead;       //internal: median node of the value chain (see Init notes)
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
 *  - Read the median from Insert's return value.  ageHead, valueHead,
 *    medianHead and the node links are internal: equal values are chained
 *    oldest first, and windows served by a selection network (numNodes <=
 *    MEDIANFILTER_SMALL_MAX) leave valueHead, medianHead and the links as Init
 *    set them.  The state is specific to this version and configuration: call
 *    Init again when a filter outlives a library update (e.g. retained RAM
 *    across a firmware update).
 */
int MEDIANFILTER_Init(MedianFilter_t *medianFilter);

#ifdef MEDIANFILTER_INLINE_API
/*
 * Inline Insert — opt-in for tight-loop and interrupt callers.
 * Define MEDIANFILTER_INLINE_API before including this header to enable.
 *
 * `static inline` is only a hint: clang, and gcc at -Os, keep a function of this
 * size out of line once a translation unit has two call sites.  Define
 * MEDIANFILTER_FORCE_INLINE as well to force it inline at every call site
 * (always_inline / __forceinline): about 8 fewer instructions per sample than a
 * call on x86-64 (gcc -O2), and one copy of Insert per call site (about 0.2 KB
 * on Cortex-M at -Os, 0.3 KB at -O2, 0.3-0.45 KB on x86-64).
 * MEDIANFILTER_INLINE_SPEC replaces the specifier altogether; it must declare a
 * static inline function.
 */
#include <stdint.h>     /* not used by Insert any more; kept for code that relies on it */

#if defined(MEDIANFILTER_INLINE_SPEC)
#  define MEDIANFILTER_INLINE_ MEDIANFILTER_INLINE_SPEC
#elif defined(MEDIANFILTER_FORCE_INLINE) && (defined(__GNUC__) || defined(__clang__))  /* before _MSC_VER: clang targeting MSVC defines both */
#  define MEDIANFILTER_INLINE_ static inline __attribute__((always_inline))
#elif defined(MEDIANFILTER_FORCE_INLINE) && defined(_MSC_VER)
#  define MEDIANFILTER_INLINE_ static __forceinline
#else
#  define MEDIANFILTER_INLINE_ static inline
#endif

/* small windows: selection networks over the buffer values (see MEDIANFILTER_SMALL_MAX) */
#if MEDIANFILTER_SMALL_MAX >= 3
MEDIANFILTER_INLINE_ int MEDIANFILTER_Insert3_(MedianFilter_t *MF_RESTRICT medianFilter, int sample)
{
    MedianFilterNode_t *buf = medianFilter->medianBuffer;
    MedianFilterNode_t *age = medianFilter->ageHead;

    age->value = sample;
    age++;
    medianFilter->ageHead = (age == buf + 3) ? buf : age;

    int a = buf[0].value, b = buf[1].value, c = buf[2].value;
    int lo = (a < b) ? a : b;
    int hi = (a > b) ? a : b;
    hi = (hi < c) ? hi : c;
    return (lo > hi) ? lo : hi;
}
#endif

#if MEDIANFILTER_SMALL_MAX >= 5
MEDIANFILTER_INLINE_ int MEDIANFILTER_Insert5_(MedianFilter_t *MF_RESTRICT medianFilter, int sample)
{
    MedianFilterNode_t *buf = medianFilter->medianBuffer;
    MedianFilterNode_t *age = medianFilter->ageHead;

    age->value = sample;
    age++;
    medianFilter->ageHead = (age == buf + 5) ? buf : age;

    int v0 = buf[0].value, v1 = buf[1].value, v2 = buf[2].value, v3 = buf[3].value, v4 = buf[4].value, t;
    t = (v0 < v1) ? v0 : v1;  v1 = (v0 > v1) ? v0 : v1;  v0 = t;
    t = (v3 < v4) ? v3 : v4;  v4 = (v3 > v4) ? v3 : v4;  v3 = t;
    v3 = (v0 > v3) ? v0 : v3;
    v1 = (v1 < v4) ? v1 : v4;
    t = (v1 < v2) ? v1 : v2;  v2 = (v1 > v2) ? v1 : v2;  v1 = t;
    v2 = (v2 < v3) ? v2 : v3;
    return (v1 > v2) ? v1 : v2;
}
#endif

/* same algorithm as the out-of-line Insert in MedianFilter.c, which carries the comments */
MEDIANFILTER_INLINE_ int MEDIANFILTER_Insert(MedianFilter_t *MF_RESTRICT medianFilter, int sample)
{
#if MEDIANFILTER_SMALL_MAX >= 5
    if(medianFilter->numNodes <= 5u)
    {
        if(medianFilter->numNodes == 3u)
        {
            return MEDIANFILTER_Insert3_(medianFilter, sample);
        }
        if(medianFilter->numNodes == 5u)
        {
            return MEDIANFILTER_Insert5_(medianFilter, sample);
        }
    }
#elif MEDIANFILTER_SMALL_MAX >= 3
    if(medianFilter->numNodes == 3u)
    {
        return MEDIANFILTER_Insert3_(medianFilter, sample);
    }
#endif

    MedianFilterNode_t *newNode   = medianFilter->ageHead;
    MedianFilterNode_t *valueHead = medianFilter->valueHead;
    MedianFilterNode_t *median    = medianFilter->medianHead;
    MedianFilterNode_t *lo, *hi;
    int result;

    if(newNode == valueHead)
    {
        valueHead = valueHead->nextValue;
    }

    if((newNode == median) || (newNode->value > median->value))
    {
        median = median->prevValue;
    }

    MedianFilterNode_t *oldNext = newNode->nextValue;
    MedianFilterNode_t *oldPrev = newNode->prevValue;
    oldNext->prevValue = oldPrev;
    oldPrev->nextValue = oldNext;
    newNode->value = sample;

    MedianFilterNode_t *nextAge = newNode + 1;
    if(nextAge == medianFilter->medianBuffer + medianFilter->numNodes)
    {
        nextAge = medianFilter->medianBuffer;
    }
    medianFilter->ageHead = nextAge;

#ifdef MEDIANFILTER_SEARCH_FROM_PREVIOUS
    MedianFilterNode_t *const prev = (newNode == medianFilter->medianBuffer) ?
                                     (medianFilter->medianBuffer + (medianFilter->numNodes - 1u)) : (newNode - 1);
#endif

    if(sample < median->value)
    {
        if(sample < valueHead->value)
        {
            hi = valueHead;
            lo = valueHead->prevValue;
            valueHead = newNode;
        }
        else
        {
#ifdef MEDIANFILTER_SEARCH_FROM_PREVIOUS
            if(sample >= prev->value)
            {
                hi = prev;
                do
                {
                    hi = hi->nextValue;
                } while(sample >= hi->value);
                lo = hi->prevValue;
            }
            else
            {
                lo = (prev->value < median->value) ? prev : median;
                do
                {
                    lo = lo->prevValue;
                } while(sample < lo->value);
                hi = lo->nextValue;
            }
#else
            lo = median;
            if(sample == valueHead->value)
            {
                const MedianFilterNode_t *const prevNode = (newNode == medianFilter->medianBuffer) ?
                                                           (medianFilter->medianBuffer + (medianFilter->numNodes - 1u)) : (newNode - 1);
                if(prevNode->value == sample)
                {
                    lo = prevNode->nextValue;
                }
            }
            do
            {
                lo = lo->prevValue;
            } while(sample < lo->value);
            hi = lo->nextValue;
#endif
        }
        result = median->value;
    }
    else
    {
        MedianFilterNode_t *q       = median->nextValue;
        MedianFilterNode_t *maxNode = valueHead->prevValue;
        if(sample >= maxNode->value)
        {
            lo = maxNode;
            hi = valueHead;
            median = q;
        }
        else
        {
#ifdef MEDIANFILTER_SEARCH_FROM_PREVIOUS
            if((sample > median->value) && (sample < prev->value))
            {
                lo = prev;
                do
                {
                    lo = lo->prevValue;
                } while(sample < lo->value);
                hi = lo->nextValue;
            }
            else
            {
                hi = (((sample > median->value) && (prev->value >= median->value)) || (prev->value == sample)) ? prev : median;
                do
                {
                    hi = hi->nextValue;
                } while(sample >= hi->value);
                lo = hi->prevValue;
            }
#else
            hi = median;
            if(sample == median->value)
            {
                MedianFilterNode_t *const prevNode = (newNode == medianFilter->medianBuffer) ?
                                                     (medianFilter->medianBuffer + (medianFilter->numNodes - 1u)) : (newNode - 1);
                if(prevNode->value == sample)
                {
                    hi = prevNode;
                }
            }
            do
            {
                hi = hi->nextValue;
            } while(sample >= hi->value);
            lo = hi->prevValue;
#endif
            median = (hi == q) ? newNode : q;
        }
        result = median->value;
    }

    lo->nextValue      = newNode;
    newNode->prevValue = lo;
    newNode->nextValue = hi;
    hi->prevValue      = newNode;

    medianFilter->valueHead  = valueHead;
    medianFilter->medianHead = median;

    return result;
}

#undef MEDIANFILTER_INLINE_

#else
int MEDIANFILTER_Insert(MedianFilter_t *MF_RESTRICT medianFilter, int sample);
#endif

#ifdef __cplusplus
}
#endif

#undef MF_RESTRICT
#endif
