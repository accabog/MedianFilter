/*
 * MedianFilter.c
 *
 *  Created on: May 19, 2018
 *      Author: alexandru.bogdan
 */

#include "MedianFilter.h"
#include <stdint.h>

/*
 * Composite comparisons: value first, address tiebreaker.
 *
 * NODE_GT compares two node pointers.
 * SAMPLE_LT / SAMPLE_GE compare a (sample, node_addr) pair against a node.
 * The address tiebreaker uses uintptr_t casts which are well-defined for
 * pointers into the same array.
 */
#define NODE_GT(a, b)  ((a)->value > (b)->value || \
                         ((a)->value == (b)->value && (uintptr_t)(a) > (uintptr_t)(b)))

#define SAMPLE_LT(val, nptr, b)  ((val) < (b)->value || \
                                   ((val) == (b)->value && (uintptr_t)(nptr) < (uintptr_t)(b)))
#define SAMPLE_GE(val, nptr, b)  (!SAMPLE_LT(val, nptr, b))

int MEDIANFILTER_Init(sMedianFilter_t *medianFilter)
{
    if(medianFilter && medianFilter->medianBuffer &&
        (medianFilter->numNodes % 2) && (medianFilter->numNodes > 1))
    {
        //initialize buffer nodes
        for(unsigned int i = 0; i < medianFilter->numNodes; i++)
        {
            medianFilter->medianBuffer[i].value = 0;
            medianFilter->medianBuffer[i].nextValue = &medianFilter->medianBuffer[(i + 1) % medianFilter->numNodes];
            medianFilter->medianBuffer[(i + 1) % medianFilter->numNodes].prevValue = &medianFilter->medianBuffer[i];
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
 * Caller must pass a valid, initialized filter (via MEDIANFILTER_Init).
 * No NULL check here — this is a hot DSP path and Init already validates.
 */
int MEDIANFILTER_Insert(sMedianFilter_t *restrict medianFilter, int sample)
{
    sMedianNode_t *newNode, *it;

    //cache struct fields in locals to avoid repeated pointer indirection
    sMedianNode_t *ageHead    = medianFilter->ageHead;
    sMedianNode_t *valueHead  = medianFilter->valueHead;
    sMedianNode_t *medianHead = medianFilter->medianHead;
    const unsigned int half   = medianFilter->numNodes >> 1;

    if(ageHead == valueHead)
    {   //oldest node is also the smallest — advance value head
        valueHead = valueHead->nextValue;
    }

    //pre-correct median for the node about to be evicted
    if(ageHead == medianHead || NODE_GT(ageHead, medianHead))
    {
        medianHead = medianHead->prevValue;
    }

    //recycle oldest node with new sample
    newNode = ageHead;
    newNode->value = sample;

    //unlink from value chain
    sMedianNode_t *oldNext = newNode->nextValue;
    sMedianNode_t *oldPrev = newNode->prevValue;
    oldNext->prevValue = oldPrev;
    oldPrev->nextValue = oldNext;

    //advance age head (age chain is always buffer[0]→[1]→...→[N-1]→[0])
    sMedianNode_t *nextAge = newNode + 1;
    if(nextAge == medianFilter->medianBuffer + medianFilter->numNodes)
        nextAge = medianFilter->medianBuffer;
    medianFilter->ageHead = nextAge;

    //find insertion point — bidirectional search from median
    if(SAMPLE_LT(sample, newNode, medianHead))
    {   //search downward — valueHead acts as sentinel
        it = medianHead->prevValue;
        while(it != valueHead && SAMPLE_LT(sample, newNode, it))
            it = it->prevValue;

        if(it != valueHead || SAMPLE_GE(sample, newNode, it))
            it = it->nextValue;
        else
            valueHead = newNode;
    }
    else
    {   //search upward — countdown bounds check (compare-to-zero is cheap)
        it = medianHead->nextValue;
        unsigned int remaining = half;
        while(remaining && SAMPLE_GE(sample, newNode, it))
        {
            it = it->nextValue;
            remaining--;
        }
    }

    //link new node into value chain
    sMedianNode_t *insertPrev = it->prevValue;
    insertPrev->nextValue = newNode;
    newNode->prevValue    = insertPrev;
    it->prevValue         = newNode;
    newNode->nextValue    = it;

    //post-correct median
    if(SAMPLE_GE(sample, newNode, medianHead))
        medianHead = medianHead->nextValue;

    medianFilter->valueHead  = valueHead;
    medianFilter->medianHead = medianHead;

    return medianHead->value;
}
