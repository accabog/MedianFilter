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

typedef struct sMedianNode
{
    int value;                      //sample value
    struct sMedianNode *nextValue;  //pointer to next smallest value
    struct sMedianNode *prevValue;  //pointer to previous smallest value
}sMedianNode_t;

typedef struct
{
    unsigned int numNodes;          //median node buffer length
    sMedianNode_t *medianBuffer;    //median node buffer
    sMedianNode_t *ageHead;         //pointer to oldest value
    sMedianNode_t *valueHead;       //pointer to smallest value
    sMedianNode_t *medianHead;      //pointer to median value
}sMedianFilter_t;

int MEDIANFILTER_Init(sMedianFilter_t *medianFilter);

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

static inline int MEDIANFILTER_Insert(sMedianFilter_t *restrict medianFilter, int sample)
{
    sMedianNode_t *newNode, *it;

    sMedianNode_t *ageHead    = medianFilter->ageHead;
    sMedianNode_t *valueHead  = medianFilter->valueHead;
    sMedianNode_t *medianHead = medianFilter->medianHead;
    const unsigned int half   = medianFilter->numNodes >> 1;

    if(ageHead == valueHead)
        valueHead = valueHead->nextValue;

    if(ageHead == medianHead || MEDIANFILTER_NODE_GT_(ageHead, medianHead))
        medianHead = medianHead->prevValue;

    newNode = ageHead;
    newNode->value = sample;

    sMedianNode_t *oldNext = newNode->nextValue;
    sMedianNode_t *oldPrev = newNode->prevValue;
    oldNext->prevValue = oldPrev;
    oldPrev->nextValue = oldNext;

    sMedianNode_t *nextAge = newNode + 1;
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

    sMedianNode_t *insertPrev = it->prevValue;
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

#else
int MEDIANFILTER_Insert(sMedianFilter_t *restrict medianFilter, int sample);
#endif

#ifdef __cplusplus
}
#endif
#endif
