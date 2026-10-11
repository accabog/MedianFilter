// SPDX-License-Identifier: MIT
// Copyright (c) 2018 Alexandru Bogdan
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
 * Opt-in: define MEDIANFILTER_FORCE_INLINE before including this header to force
 * Insert() inline at every call site.  Without it compilers keep Insert out of
 * line at -O2/-Os (it exceeds their inlining budgets).  Every call site then gets
 * its own copy of Insert.
 */
#if defined(MEDIANFILTER_FORCE_INLINE)
#  if defined(__GNUC__) || defined(__clang__)
#    define MEDIANFILTER_INSERT_INLINE_ inline __attribute__((always_inline))
#  elif defined(_MSC_VER)
#    define MEDIANFILTER_INSERT_INLINE_ __forceinline
#  else
#    define MEDIANFILTER_INSERT_INLINE_ inline
#  endif
#else
#  define MEDIANFILTER_INSERT_INLINE_
#endif

/*
 * Opt-in: define MEDIANFILTER_SEARCH_FROM_PREVIOUS to start the insertion walk at
 * the previous sample's node when it lies on the same side of the median as the
 * new sample (fewer hops for slowly varying signals in larger windows, see
 * MedianFilter.h).  MEDIANFILTER_SEARCH_FROM_PREVIOUS_MIN_N limits it to windows
 * of at least that many elements; the test is resolved at compile time.  Define
 * both identically in every translation unit.
 */
#if defined(MEDIANFILTER_SEARCH_FROM_PREVIOUS) && !defined(MEDIANFILTER_SEARCH_FROM_PREVIOUS_MIN_N)
#  define MEDIANFILTER_SEARCH_FROM_PREVIOUS_MIN_N 3
#endif

namespace medianfilter_detail
{
	//windows of 3 use a branch-free selection network instead of the value chain:
	//N is a template parameter, so the choice costs nothing at run time
	template <typename type, unsigned int numElements>
	struct UseNetwork : std::integral_constant<bool, std::is_arithmetic<type>::value && (numElements == 3)>
	{
	};

	//median of three values: max(min(a, b), min(max(a, b), c)), no branches
	template <typename type>
	inline type MedianOf3(const type *v)
	{
		type lo = (v[0] < v[1]) ? v[0] : v[1];
		type hi = (v[0] > v[1]) ? v[0] : v[1];
		hi = (hi < v[2]) ? hi : v[2];
		return (lo > hi) ? lo : hi;
	}

	//window of 3: the values in age order, no value chain
	template <typename type, unsigned int numElements>
	class NetworkCore
	{
	public:
		NetworkCore() : m_ageIndex(0)
		{
			for(unsigned int i = 0; i < numElements; i++)
			{
				m_values[i] = 0;
			}
		}

		NetworkCore(const NetworkCore&)            = delete;
		NetworkCore& operator=(const NetworkCore&) = delete;
		NetworkCore(NetworkCore&&)                 = delete;
		NetworkCore& operator=(NetworkCore&&)      = delete;

		MEDIANFILTER_INSERT_INLINE_ type Insert(type value)
		{
			m_values[m_ageIndex] = value;
			unsigned int next = m_ageIndex + 1;
			m_ageIndex = next & (0u - static_cast<unsigned int>(next != numElements));   //wrap to 0 without a branch
			return MedianOf3(m_values);
		}

	private:
		type m_values[numElements];
		unsigned int m_ageIndex;          //index of the oldest value
	};

	//larger windows: sorted doubly linked value chain over the node buffer
	template <typename type, unsigned int numElements>
	class ListCore
	{
	public:
		ListCore();

		ListCore(const ListCore&)            = delete;
		ListCore& operator=(const ListCore&) = delete;
		ListCore(ListCore&&)                 = delete;
		ListCore& operator=(ListCore&&)      = delete;

		MEDIANFILTER_INSERT_INLINE_ type Insert(type value);

	private:
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
	};

	template<typename type, unsigned int numElements>
	ListCore<type, numElements>::ListCore()
	{
		//one pass without a modulo: prev starts at the last node, so the
		//wrap-around links (last <-> first) are set as well
		sMedianNode_t *prev = &m_medianBuffer[numElements - 1];
		for(unsigned int i = 0; i < numElements; i++)
		{
			m_medianBuffer[i].value     = 0;
			prev->nextValue             = &m_medianBuffer[i];
			m_medianBuffer[i].prevValue = prev;
			prev                        = &m_medianBuffer[i];
		}

		m_ageHead = m_medianBuffer;
		m_valueHead = m_medianBuffer;
		m_medianHead = &m_medianBuffer[numElements / 2];
	}

	/*
	 * Equal values are chained oldest first ("youngest is greatest"): the evicted
	 * node is the lowest of its equal-value run and the new node goes above every
	 * node with an equal value.  Values are compared with <, > and >= only, never
	 * with ==, which keeps float instantiations clean under -Wfloat-equal.
	 */
#if defined(_MSC_VER) && defined(MEDIANFILTER_SEARCH_FROM_PREVIOUS)
#pragma warning(push)
#pragma warning(disable: 4127)      //conditional expression is constant: the compile-time window threshold
#endif
	template<typename type, unsigned int numElements>
	MEDIANFILTER_INSERT_INLINE_ type ListCore<type, numElements>::Insert(type value)
	{
		//cache member fields in locals to avoid repeated pointer indirection
		sMedianNode_t *newNode   = m_ageHead;      //oldest node: evicted, then recycled
		sMedianNode_t *valueHead = m_valueHead;
		sMedianNode_t *median    = m_medianHead;
		sMedianNode_t *lo, *hi;                   //the new node is linked in between lo and hi
		type result;

		if(newNode == valueHead)
		{	//oldest node is also the smallest — advance value head
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
		sMedianNode_t *oldNext = newNode->nextValue;
		sMedianNode_t *oldPrev = newNode->prevValue;
		oldNext->prevValue = oldPrev;
		oldPrev->nextValue = oldNext;
		newNode->value = value;

		//advance age head (age chain is always buffer[0]→[1]→...→[N-1]→[0])
		sMedianNode_t *nextAge = newNode + 1;
		if(nextAge == m_medianBuffer + numElements)
		{
			nextAge = m_medianBuffer;
		}
		m_ageHead = nextAge;

#ifdef MEDIANFILTER_SEARCH_FROM_PREVIOUS
		//opt-in search start: the node written by the previous Insert (never the evicted node)
		constexpr bool fromPrevious = (numElements >= MEDIANFILTER_SEARCH_FROM_PREVIOUS_MIN_N);
		sMedianNode_t *const prev = (newNode == m_medianBuffer) ? (m_medianBuffer + (numElements - 1)) : (newNode - 1);
#endif

		//find the insertion point: the new node goes below node b iff value < b->value.
		//The new-minimum and new-maximum guards are the walks' loop tests at valueHead and
		//at the maximum, so neither walk needs a bounds check (keep them identical).
		if(value < median->value)
		{	//below the lower median: the median node is unchanged
			if(value < valueHead->value)
			{	//new minimum
				hi = valueHead;
				lo = valueHead->prevValue;          //the maximum (the chain is circular)
				valueHead = newNode;
			}
			else
			{
#ifdef MEDIANFILTER_SEARCH_FROM_PREVIOUS
				if(fromPrevious)
				{
					if(value >= prev->value)
					{	//the previous sample's node is at or below the new value, so below the median:
						//walk up from it; stops at the median at the latest
						hi = prev;
						do
						{
							hi = hi->nextValue;
						} while(value >= hi->value);
						lo = hi->prevValue;
					}
					else
					{	//walk down from the previous sample's node if it is below the median, else
						//from the median; stops at valueHead at the latest
						lo = (prev->value < median->value) ? prev : median;
						do
						{
							lo = lo->prevValue;
						} while(value < lo->value);
						hi = lo->nextValue;
					}
				}
				else
#endif
				{
					//walk down from the median; stops at valueHead at the latest
					lo = median;
					if(!(value > valueHead->value))
					{	//repeats the minimum: the new node goes on top of the run of minima.  The
						//previous sample's node is the youngest in the window, so if it holds the
						//minimum it is the top of that run: start one node above it instead of
						//walking down to it (falling signal with repeated values)
						const sMedianNode_t *const prevNode = (newNode == m_medianBuffer) ? (m_medianBuffer + (numElements - 1)) : (newNode - 1);
						if(!(prevNode->value < value) && !(value < prevNode->value))
						{
							lo = prevNode->nextValue;
						}
					}
					do
					{
						lo = lo->prevValue;
					} while(value < lo->value);
					hi = lo->nextValue;
				}
			}
			result = median->value;
		}
		else
		{	//at or above the lower median: the median moves up by one
			sMedianNode_t *q       = median->nextValue;      //upper median of the survivors
			sMedianNode_t *maxNode = valueHead->prevValue;
			if(value >= maxNode->value)
			{	//new maximum
				lo = maxNode;
				hi = valueHead;
				median = q;
			}
			else
			{
#ifdef MEDIANFILTER_SEARCH_FROM_PREVIOUS
				if(fromPrevious)
				{
					if((value > median->value) && (value < prev->value))
					{	//the previous sample's node is above the new value, so above the median:
						//walk down from it; stops at the median at the latest
						lo = prev;
						do
						{
							lo = lo->prevValue;
						} while(value < lo->value);
						hi = lo->nextValue;
					}
					else
					{	//walk up from the previous sample's node if the value is above the median value
						//and that node is at or above the median (it then lies between the median and
						//the insertion point), or if the value equals the median value and the previous
						//sample's value (the node is then the top of that equal-value run); otherwise
						//from the median.  Starting above the insertion point would skip it.  Stops at
						//the maximum at the latest
						hi = (((value > median->value) && (prev->value >= median->value)) ||
						      (!(value > median->value) && !(prev->value < value) && !(value < prev->value))) ? prev : median;
						do
						{
							hi = hi->nextValue;
						} while(value >= hi->value);
						lo = hi->prevValue;
					}
				}
				else
#endif
				{
					//walk up from the median; stops at the maximum at the latest
					hi = median;
					if(!(value > median->value))
					{	//equal to the median: the new node goes on top of the median's equal-value run.
						//The previous sample's node is the youngest in the window, so if it has the same
						//value it is the top of that run: start there instead of crossing the run (flat
						//level with upward spikes)
						sMedianNode_t *const prevNode = (newNode == m_medianBuffer) ? (m_medianBuffer + (numElements - 1)) : (newNode - 1);
						if(!(prevNode->value < value) && !(value < prevNode->value))
						{
							hi = prevNode;
						}
					}
					do
					{
						hi = hi->nextValue;
					} while(value >= hi->value);
					lo = hi->prevValue;
				}
				median = (hi == q) ? newNode : q;   //no hop: the new node is the median
			}
			result = median->value;
		}

		//link new node into value chain
		lo->nextValue      = newNode;
		newNode->prevValue = lo;
		newNode->nextValue = hi;
		hi->prevValue      = newNode;

		m_valueHead  = valueHead;
		m_medianHead = median;

		return result;
	}
#if defined(_MSC_VER) && defined(MEDIANFILTER_SEARCH_FROM_PREVIOUS)
#pragma warning(pop)
#endif

	//the implementation used by MedianFilter<type, numElements>
	template <typename type, unsigned int numElements>
	struct CoreOf
	{
		typedef typename std::conditional<UseNetwork<type, numElements>::value,
		                                  NetworkCore<type, numElements>, ListCore<type, numElements> >::type CoreType;
	};
} //namespace medianfilter_detail

/*
 * Fixed-size sliding-window median filter (header-only).
 *
 * NOTE for floating-point instantiations: NaN inputs are unsupported.  NaN
 * compares false under every relational operator, which breaks the strict
 * total order the sorted value chain relies on and corrupts the running median
 * (persisting after the NaN is evicted), although Insert() still returns and
 * stays inside the filter.  Reject or replace NaNs before calling Insert().  -0.0 and +0.0 compare equal, so when both are in the window the
 * sign of a zero median is unspecified.
 */
template <typename type, unsigned int numElements>
class MedianFilter
{
	static_assert(std::is_arithmetic<type>::value, "type must be an arithmetic type (int, float, etc.)");
	static_assert(numElements % 2, "Number of elements must be ODD");
	static_assert(numElements > 2, "Minimum 3 elements required");

	typename medianfilter_detail::CoreOf<type, numElements>::CoreType m_core;

public:
	MedianFilter() {}
	~MedianFilter() = default;

	/*
	 * Non-copyable and non-movable.  Every node stores absolute pointers into
	 * this object's own buffer, so a compiler-generated copy or move would
	 * leave the destination's links dangling into the source buffer
	 * (use-after-free once the source is destroyed or reused).  Keep a filter
	 * in place: do not return it by value, assign it, or store it in a
	 * relocating container such as std::vector.  Misuse is a compile error.
	 */
	MedianFilter(const MedianFilter&)            = delete;
	MedianFilter& operator=(const MedianFilter&) = delete;
	MedianFilter(MedianFilter&&)                 = delete;
	MedianFilter& operator=(MedianFilter&&)      = delete;

	MEDIANFILTER_INSERT_INLINE_ type Insert(type value) { return m_core.Insert(value); }
};

#undef MEDIANFILTER_INSERT_INLINE_

#endif /* MEDIANFILTER_HPP_ */
