/* Baseline: std::sort the entire window on every insert. O(n log n) per insert. */
#ifndef NAIVE_SORT_H
#define NAIVE_SORT_H

#include <algorithm>
#include <cstring>

template<typename T, int N>
class NaiveSortMedian {
    T ring[N];
    T sorted[N];
    int pos = 0;
public:
    NaiveSortMedian() { std::memset(ring, 0, sizeof(ring)); }

    T Insert(T value) {
        ring[pos] = value;
        pos = (pos + 1) % N;

        std::memcpy(sorted, ring, sizeof(ring));
        std::sort(sorted, sorted + N);
        return sorted[N / 2];
    }
};

#endif
