// SPDX-License-Identifier: MIT
// Copyright (c) 2018 Alexandru Bogdan
/*
 * MedianFilter.c
 *
 *  Created on: May 19, 2018
 *      Author: alexandru.bogdan
 */

#include "MedianFilter.h"

int MEDIANFILTER_Init(MedianFilter_t *medianFilter)
{
    if(medianFilter && medianFilter->medianBuffer &&
        (medianFilter->numNodes % 2) && (medianFilter->numNodes > 1))
    {
        //initialize buffer nodes in one pass without a modulo: prev starts at the
        //last node, so the wrap-around links (last <-> first) are set as well
        MedianFilterNode_t *buf  = medianFilter->medianBuffer;
        const unsigned int n     = medianFilter->numNodes;
        MedianFilterNode_t *prev = &buf[n - 1];
        for(unsigned int i = 0; i < n; i++)
        {
            buf[i].value     = 0;
            prev->nextValue  = &buf[i];
            buf[i].prevValue = prev;
            prev             = &buf[i];
        }
        //initialize heads
        medianFilter->ageHead = medianFilter->medianBuffer;
        medianFilter->valueHead = medianFilter->medianBuffer;
        medianFilter->medianHead = &medianFilter->medianBuffer[medianFilter->numNodes / 2];

        return 0;
    }

    return -1;
}

/*
 * When MEDIANFILTER_INLINE_API is defined, MedianFilter.h provides Insert as a
 * static inline; compiling an out-of-line copy here too would be a redefinition
 * (and lets a caller link this TU while opting into the inline variant).  So
 * build the out-of-line Insert and its helpers only when the inline API is NOT
 * selected.
 */
#ifndef MEDIANFILTER_INLINE_API

#if MEDIANFILTER_SMALL_MAX >= 3

/*
 * Small windows (numNodes <= MEDIANFILTER_SMALL_MAX): the sample is written into
 * the oldest slot and the median is computed from the buffer values with a
 * branch-free selection network; the value chain, valueHead and medianHead are
 * not used.  Kept out of line so that the list path in Insert keeps its
 * register allocation.
 */
#if defined(__GNUC__) || defined(__clang__)
#  define MEDIANFILTER_NOINLINE_ __attribute__((noinline))
#elif defined(_MSC_VER)
#  define MEDIANFILTER_NOINLINE_ __declspec(noinline)
#else
#  define MEDIANFILTER_NOINLINE_
#endif

static MEDIANFILTER_NOINLINE_ int MEDIANFILTER_Insert3_(MedianFilter_t *restrict medianFilter, int sample)
{
    MedianFilterNode_t *buf = medianFilter->medianBuffer;
    MedianFilterNode_t *age = medianFilter->ageHead;

    age->value = sample;
    age++;
    medianFilter->ageHead = (age == buf + 3) ? buf : age;      //wrap to the start of the buffer

    int a = buf[0].value, b = buf[1].value, c = buf[2].value;
    int lo = (a < b) ? a : b;          //min(a, b)
    int hi = (a > b) ? a : b;          //max(a, b)
    hi = (hi < c) ? hi : c;            //min(max(a, b), c)
    return (lo > hi) ? lo : hi;        //median = max(min(a, b), min(max(a, b), c))
}

#if MEDIANFILTER_SMALL_MAX >= 5
static MEDIANFILTER_NOINLINE_ int MEDIANFILTER_Insert5_(MedianFilter_t *restrict medianFilter, int sample)
{
    MedianFilterNode_t *buf = medianFilter->medianBuffer;
    MedianFilterNode_t *age = medianFilter->ageHead;

    age->value = sample;
    age++;
    medianFilter->ageHead = (age == buf + 5) ? buf : age;      //wrap to the start of the buffer

    int v0 = buf[0].value, v1 = buf[1].value, v2 = buf[2].value, v3 = buf[3].value, v4 = buf[4].value, t;
    t = (v0 < v1) ? v0 : v1;  v1 = (v0 > v1) ? v0 : v1;  v0 = t;     //order (v0, v1)
    t = (v3 < v4) ? v3 : v4;  v4 = (v3 > v4) ? v3 : v4;  v3 = t;     //order (v3, v4)
    v3 = (v0 > v3) ? v0 : v3;                                         //drop the minimum of v0, v1, v3, v4
    v1 = (v1 < v4) ? v1 : v4;                                         //drop the maximum of v0, v1, v3, v4
    t = (v1 < v2) ? v1 : v2;  v2 = (v1 > v2) ? v1 : v2;  v1 = t;     //order (v1, v2)
    v2 = (v2 < v3) ? v2 : v3;
    return (v1 > v2) ? v1 : v2;                                       //median of v1, v2, v3
}
#endif

#undef MEDIANFILTER_NOINLINE_

#endif /* MEDIANFILTER_SMALL_MAX >= 3 */

/*
 * Caller must pass a valid, initialized filter (via MEDIANFILTER_Init).
 * No NULL check here — this is a hot DSP path and Init already validates.
 *
 * Equal values are chained oldest first ("youngest is greatest"), which gives a
 * strict total order without comparing node addresses: the evicted node is the
 * lowest of its equal-value run and the new node goes above every node with an
 * equal value.
 */
int MEDIANFILTER_Insert(MedianFilter_t *restrict medianFilter, int sample)
{
#if MEDIANFILTER_SMALL_MAX >= 5
    if(medianFilter->numNodes <= 5u)
    {   //small window: selection network (see MEDIANFILTER_SMALL_MAX)
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
    {   //small window: selection network (see MEDIANFILTER_SMALL_MAX)
        return MEDIANFILTER_Insert3_(medianFilter, sample);
    }
#endif

    //cache struct fields in locals to avoid repeated pointer indirection
    MedianFilterNode_t *newNode   = medianFilter->ageHead;      //oldest node: evicted, then recycled
    MedianFilterNode_t *valueHead = medianFilter->valueHead;
    MedianFilterNode_t *median    = medianFilter->medianHead;
    MedianFilterNode_t *lo, *hi;                                //the new node is linked in between lo and hi
    int result;

    if(newNode == valueHead)
    {   //oldest node is also the smallest — advance value head
        valueHead = valueHead->nextValue;
    }

    //pre-correct median for the node about to be evicted: as the oldest node it is
    //the lowest of its equal-value run, so it sits at or above the median iff it IS
    //the median or its value is greater.  median becomes the lower median of the
    //N-1 survivors.
    if((newNode == median) || (newNode->value > median->value))
    {
        median = median->prevValue;
    }

    //unlink the oldest node from the value chain and recycle it with the new sample
    MedianFilterNode_t *oldNext = newNode->nextValue;
    MedianFilterNode_t *oldPrev = newNode->prevValue;
    oldNext->prevValue = oldPrev;
    oldPrev->nextValue = oldNext;
    newNode->value = sample;

    //advance age head (age chain is always buffer[0]→[1]→...→[N-1]→[0])
    MedianFilterNode_t *nextAge = newNode + 1;
    if(nextAge == medianFilter->medianBuffer + medianFilter->numNodes)
    {
        nextAge = medianFilter->medianBuffer;
    }
    medianFilter->ageHead = nextAge;

#ifdef MEDIANFILTER_SEARCH_FROM_PREVIOUS
    //opt-in search start: the node written by the previous Insert (never the evicted node)
    MedianFilterNode_t *const prev = (newNode == medianFilter->medianBuffer) ?
                                     (medianFilter->medianBuffer + (medianFilter->numNodes - 1u)) : (newNode - 1);
#endif

    /*
     * Find the insertion point.  The new node is the youngest, so it goes below
     * node b iff sample < b->value.  Neither walk needs a bounds check: the
     * new-minimum guard (sample < valueHead->value) is the down-walk test at
     * valueHead and the new-maximum guard (sample >= maximum) is the up-walk test
     * at the maximum, so once a guard has failed its walk stops at that node at
     * the latest.  Keep each guard and its loop test the identical comparison.
     */
    if(sample < median->value)
    {   //below the lower median: the median node is unchanged
        if(sample < valueHead->value)
        {   //new minimum
            hi = valueHead;
            lo = valueHead->prevValue;          //the maximum (the chain is circular)
            valueHead = newNode;
        }
        else
        {
#ifdef MEDIANFILTER_SEARCH_FROM_PREVIOUS
            if(sample >= prev->value)
            {   //the previous sample's node is at or below the new sample, so below the median:
                //walk up from it; stops at the median at the latest
                hi = prev;
                do
                {
                    hi = hi->nextValue;
                } while(sample >= hi->value);
                lo = hi->prevValue;
            }
            else
            {   //walk down from the previous sample's node if it is below the median, else from
                //the median; stops at valueHead at the latest
                lo = (prev->value < median->value) ? prev : median;
                do
                {
                    lo = lo->prevValue;
                } while(sample < lo->value);
                hi = lo->nextValue;
            }
#else
            //walk down from the median; stops at valueHead at the latest
            lo = median;
            if(sample == valueHead->value)
            {   //repeats the minimum: the new node goes on top of the run of minima.  The
                //previous sample's node is the youngest in the window, so if it holds the
                //minimum it is the top of that run: start one node above it instead of
                //walking down to it (falling signal with repeated values)
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
    {   //at or above the lower median: the median moves up by one
        MedianFilterNode_t *q       = median->nextValue;      //upper median of the survivors
        MedianFilterNode_t *maxNode = valueHead->prevValue;
        if(sample >= maxNode->value)
        {   //new maximum
            lo = maxNode;
            hi = valueHead;
            median = q;
        }
        else
        {
#ifdef MEDIANFILTER_SEARCH_FROM_PREVIOUS
            if((sample > median->value) && (sample < prev->value))
            {   //the previous sample's node is above the new sample, so above the median:
                //walk down from it; stops at the median at the latest
                lo = prev;
                do
                {
                    lo = lo->prevValue;
                } while(sample < lo->value);
                hi = lo->nextValue;
            }
            else
            {   //walk up from the previous sample's node if the sample is above the median value
                //and that node is at or above the median (it then lies between the median and the
                //insertion point), or if the node holds the sample's value (it is then the top of
                //that equal-value run); otherwise from the median.  Starting above the insertion
                //point would skip it.  Stops at the maximum at the latest
                hi = (((sample > median->value) && (prev->value >= median->value)) || (prev->value == sample)) ? prev : median;
                do
                {
                    hi = hi->nextValue;
                } while(sample >= hi->value);
                lo = hi->prevValue;
            }
#else
            //walk up from the median; stops at the maximum at the latest
            hi = median;
            if(sample == median->value)
            {   //the new node goes on top of the median's equal-value run.  The previous sample's
                //node is the youngest in the window, so if it has the same value it is the top of
                //that run: start there instead of crossing the run (flat level with upward spikes)
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
            median = (hi == q) ? newNode : q;   //no hop: the new node is the median
        }
        result = median->value;
    }

    //link new node into value chain
    lo->nextValue      = newNode;
    newNode->prevValue = lo;
    newNode->nextValue = hi;
    hi->prevValue      = newNode;

    medianFilter->valueHead  = valueHead;
    medianFilter->medianHead = median;

    return result;
}

#endif /* !MEDIANFILTER_INLINE_API */
