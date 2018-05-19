/* std::nth_element baseline: O(n) average per insert via introselect.
 * Copies window then uses nth_element to find the median. */
#ifndef NTH_ELEMENT_H
#define NTH_ELEMENT_H

#include <algorithm>
#include <cstring>

template<typename T, int N>
class NthElementMedian {
    T ring[N];
    T scratch[N];
    int pos = 0;
public:
    NthElementMedian() { std::memset(ring, 0, sizeof(ring)); }

    T Insert(T value) {
        ring[pos] = value;
        pos = (pos + 1) % N;

        std::memcpy(scratch, ring, sizeof(ring));
        std::nth_element(scratch, scratch + N / 2, scratch + N);
        return scratch[N / 2];
    }
};

#endif
