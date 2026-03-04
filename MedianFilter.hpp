/*
 * MedianFilter.hpp
 *
 *  Created on: May 20, 2018
 *      Author: alexandru.bogdan
 */

#ifndef MEDIANFILTER_HPP_
#define MEDIANFILTER_HPP_

#include <type_traits>
#include <cstdint>

/*
 * Composite comparisons: value first, address tiebreaker.
 * See MedianFilter.c for detailed rationale.
 */
#define MEDIANFILTER_NODE_GT(a, b)  ((a)->value > (b)->value || \
                                      ((a)->value == (b)->value && (uintptr_t)(a) > (uintptr_t)(b)))

#define MEDIANFILTER_SAMPLE_LT(val, nptr, b)  ((val) < (b)->value || \
                                                 ((val) == (b)->value && (uintptr_t)(nptr) < (uintptr_t)(b)))
#define MEDIANFILTER_SAMPLE_GE(val, nptr, b)  (!MEDIANFILTER_SAMPLE_LT(val, nptr, b))

template <typename type, unsigned int numElements>
class MedianFilter
{
	static_assert(std::is_arithmetic<type>::value, "type must be an arithmetic type (int, float, etc.)");
	static_assert(numElements % 2, "Number of elements must be ODD");
	static_assert(numElements > 2, "Minimum 3 elements required");

	typedef struct sMedianNode
	{
		type value;
		struct sMedianNode *nextValue;
		struct sMedianNode *prevValue;
	}sMedianNode_t;

	sMedianNode_t m_medianBuffer[numElements];
	sMedianNode_t *m_ageHead;
	sMedianNode_t *m_valueHead;
	sMedianNode_t *m_medianHead;

public:
	MedianFilter();
	~MedianFilter() = default;

	type Insert(type value);
};

template<typename type, unsigned int numElements>
MedianFilter<type, numElements>::MedianFilter()
{
	for(unsigned int i = 0; i < numElements; i++)
	{
		m_medianBuffer[i].value = 0;
		m_medianBuffer[i].nextValue = &m_medianBuffer[(i + 1) % numElements];
		m_medianBuffer[(i + 1) % numElements].prevValue = &m_medianBuffer[i];
	}

	m_ageHead = m_medianBuffer;
	m_valueHead = m_medianBuffer;
	m_medianHead = &m_medianBuffer[numElements / 2];
}

template<typename type, unsigned int numElements>
type MedianFilter<type, numElements>::Insert(type value)
{
	sMedianNode_t *newNode, *it;

	//cache member fields in locals to avoid repeated pointer indirection
	sMedianNode_t *ageHead    = m_ageHead;
	sMedianNode_t *valueHead  = m_valueHead;
	sMedianNode_t *medianHead = m_medianHead;
	constexpr unsigned int half = numElements / 2;

	if(ageHead == valueHead)
	{	//oldest node is also the smallest — advance value head
		valueHead = valueHead->nextValue;
	}

	//pre-correct median for the node about to be evicted
	if(ageHead == medianHead || MEDIANFILTER_NODE_GT(ageHead, medianHead))
	{
		medianHead = medianHead->prevValue;
	}

	//recycle oldest node with new sample
	newNode = ageHead;
	newNode->value = value;

	//unlink from value chain
	sMedianNode_t *oldNext = newNode->nextValue;
	sMedianNode_t *oldPrev = newNode->prevValue;
	oldNext->prevValue = oldPrev;
	oldPrev->nextValue = oldNext;

	//advance age head (age chain is always buffer[0]→[1]→...→[N-1]→[0])
	sMedianNode_t *nextAge = newNode + 1;
	if(nextAge == m_medianBuffer + numElements)
		nextAge = m_medianBuffer;
	m_ageHead = nextAge;

	//find insertion point — bidirectional search from median
	if(MEDIANFILTER_SAMPLE_LT(value, newNode, medianHead))
	{	//search downward — valueHead acts as sentinel
		it = medianHead->prevValue;
		while(it != valueHead && MEDIANFILTER_SAMPLE_LT(value, newNode, it))
			it = it->prevValue;

		if(it != valueHead || MEDIANFILTER_SAMPLE_GE(value, newNode, it))
			it = it->nextValue;
		else
			valueHead = newNode;
	}
	else
	{	//search upward — countdown bounds check
		it = medianHead->nextValue;
		unsigned int remaining = half;
		while(remaining && MEDIANFILTER_SAMPLE_GE(value, newNode, it))
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
	if(MEDIANFILTER_SAMPLE_GE(value, newNode, medianHead))
		medianHead = medianHead->nextValue;

	m_valueHead  = valueHead;
	m_medianHead = medianHead;

	return medianHead->value;
}

#undef MEDIANFILTER_NODE_GT
#undef MEDIANFILTER_SAMPLE_LT
#undef MEDIANFILTER_SAMPLE_GE

#endif /* MEDIANFILTER_HPP_ */
