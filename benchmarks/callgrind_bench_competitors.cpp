/*
 * callgrind_bench_competitors.cpp — Instruction-count benchmark for C++ baselines.
 *
 * Usage: ./callgrind_bench_competitors <algo> <window_size>
 *   algo: naive | insertion | nth
 */
#include <cstdlib>
#include <cstring>
#include "competitors/naive_sort.h"
#include "competitors/insertion_sort_ring.h"
#include "competitors/nth_element.h"

#define NUM_SAMPLES 10000

static int samples[NUM_SAMPLES];

static unsigned g_seed = 42;

static void generate_samples() {
    std::srand(g_seed);
    for (int i = 0; i < NUM_SAMPLES; i++)
        samples[i] = std::rand();
}

template<template<typename,int> class Filter, int N>
static int run() {
    Filter<int, N> filter;
    volatile int sink;
    for (int i = 0; i < NUM_SAMPLES; i++)
        sink = filter.Insert(samples[i]);
    return sink & 0;
}

template<template<typename,int> class Filter>
static int dispatch(int window) {
    switch (window) {
        case 3:   return run<Filter, 3>();
        case 5:   return run<Filter, 5>();
        case 7:   return run<Filter, 7>();
        case 9:   return run<Filter, 9>();
        case 11:  return run<Filter, 11>();
        case 21:  return run<Filter, 21>();
        case 31:  return run<Filter, 31>();
        case 51:  return run<Filter, 51>();
        case 101: return run<Filter, 101>();
        default:  return 1;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) return 1;
    const char *algo = argv[1];
    int window = std::atoi(argv[2]);
    if (argc >= 4) g_seed = (unsigned)std::atoi(argv[3]);

    generate_samples();

    if (std::strcmp(algo, "naive") == 0)
        return dispatch<NaiveSortMedian>(window);
    if (std::strcmp(algo, "insertion") == 0)
        return dispatch<InsertionSortMedian>(window);
    if (std::strcmp(algo, "nth") == 0)
        return dispatch<NthElementMedian>(window);

    return 1;
}
