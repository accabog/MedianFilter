/* Insertion-sort ring buffer: maintain a sorted copy via incremental insertion sort.
 * O(n) worst per insert, but excellent cache locality. */
#ifndef INSERTION_SORT_RING_H
#define INSERTION_SORT_RING_H

#include <cstring>

template<typename T, int N>
class InsertionSortMedian {
    T ring[N];
    T sorted[N];
    int pos = 0;
public:
    InsertionSortMedian() {
        std::memset(ring, 0, sizeof(ring));
        std::memset(sorted, 0, sizeof(sorted));
    }

    T Insert(T value) {
        T old_val = ring[pos];
        ring[pos] = value;
        pos = (pos + 1) % N;

        /* Find old_val in sorted array and replace with new value */
        int idx = 0;
        for (int i = 0; i < N; i++) {
            if (sorted[i] == old_val) { idx = i; break; }
        }

        if (value >= old_val) {
            /* Shift left, scan right for insert point */
            int i = idx;
            while (i < N - 1 && sorted[i + 1] <= value) {
                sorted[i] = sorted[i + 1];
                i++;
            }
            sorted[i] = value;
        } else {
            /* Shift right, scan left for insert point */
            int i = idx;
            while (i > 0 && sorted[i - 1] >= value) {
                sorted[i] = sorted[i - 1];
                i--;
            }
            sorted[i] = value;
        }

        return sorted[N / 2];
    }
};

#endif
