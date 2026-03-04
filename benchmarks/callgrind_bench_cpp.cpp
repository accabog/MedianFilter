/*
 * callgrind_bench_cpp.cpp — Deterministic instruction-count benchmark for the C++ API.
 *
 * Usage: ./callgrind_bench_cpp <window_size>
 */
#include <cstdlib>
#include "MedianFilter.hpp"

#define NUM_SAMPLES 10000

static int samples[NUM_SAMPLES];

static unsigned g_seed = 42;

static void generate_samples() {
    std::srand(g_seed);
    for (int i = 0; i < NUM_SAMPLES; i++)
        samples[i] = std::rand();
}

template<int N>
static int run() {
    MedianFilter<int, N> filter;
    volatile int sink;
    for (int i = 0; i < NUM_SAMPLES; i++)
        sink = filter.Insert(samples[i]);
    return sink & 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) return 1;
    int window = std::atoi(argv[1]);
    if (argc >= 3) g_seed = (unsigned)std::atoi(argv[2]);

    generate_samples();

    switch (window) {
        case 3:   return run<3>();
        case 5:   return run<5>();
        case 7:   return run<7>();
        case 9:   return run<9>();
        case 11:  return run<11>();
        case 21:  return run<21>();
        case 31:  return run<31>();
        case 51:  return run<51>();
        case 101: return run<101>();
        default:  return 1;
    }
}
