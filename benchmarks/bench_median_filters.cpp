/*
 * bench_median_filters.cpp — Google Benchmark suite comparing MedianFilter
 * against algorithm baselines and third-party competitors.
 *
 * Build:
 *   cd benchmarks
 *   cmake -B build -DCMAKE_BUILD_TYPE=Release
 *   cmake --build build
 *   ./build/bench_median_filters
 */

#include <benchmark/benchmark.h>
#include <cstdlib>
#include <cstdint>
#include <vector>

/* Our library — 'restrict' is C-only, define it away for C++ */
#ifdef __cplusplus
#define restrict
#endif
extern "C" {
#include "MedianFilter.h"
}
#ifdef __cplusplus
#undef restrict
#endif
#include "MedianFilter.hpp"

/* Algorithm baselines */
#include "competitors/naive_sort.h"
#include "competitors/insertion_sort_ring.h"
#include "competitors/nth_element.h"

/* Third-party: vpetrigo/median-filter
 * Their header's C++ overloads have conflicting return types, so we
 * forward-declare only the int32_t functions we need. */
extern "C" {
struct median_filter_node_int32_t {
    struct median_filter_node_int32_t *prev;
    struct median_filter_node_int32_t *next;
    int32_t value;
};
struct median_filter_int32_t {
    struct median_filter_node_int32_t *buffer;
    struct median_filter_node_int32_t *smallest;
    struct median_filter_node_int32_t *median;
    size_t current_pos;
    size_t size;
};
bool median_filter_init_int32_t(struct median_filter_int32_t *filter,
                                struct median_filter_node_int32_t *buffer, size_t buffer_size);
bool median_filter_insert_value_int32_t(struct median_filter_int32_t *filter, int32_t sample);
int32_t median_filter_get_median_int32_t(const struct median_filter_int32_t *filter);
}

/* Third-party: takingBytes/MovingMedianFilter (via wrapper to avoid name collision) */
#include "competitors/takingbytes_wrapper.h"

/* ── Shared test data ─────────────────────────────────────── */

static const int NUM_SAMPLES = 10000;

static std::vector<int> generate_int_samples(unsigned seed) {
    std::vector<int> v(NUM_SAMPLES);
    std::srand(seed);
    for (auto &x : v) x = std::rand();
    return v;
}

static std::vector<float> generate_float_samples(unsigned seed) {
    std::vector<float> v(NUM_SAMPLES);
    std::srand(seed);
    for (auto &x : v)
        x = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * 2000.0f - 1000.0f;
    return v;
}

static const auto INT_DATA   = generate_int_samples(42);
static const auto FLOAT_DATA = generate_float_samples(42);

/* ── Our library: C API ───────────────────────────────────── */

static void BM_Ours_C(benchmark::State &state) {
    const int win = static_cast<int>(state.range(0));
    std::vector<sMedianNode_t> buf(win);
    sMedianFilter_t filter;
    filter.numNodes = win;
    filter.medianBuffer = buf.data();

    for (auto _ : state) {
        MEDIANFILTER_Init(&filter);
        for (int i = 0; i < NUM_SAMPLES; i++) {
            benchmark::DoNotOptimize(MEDIANFILTER_Insert(&filter, INT_DATA[i]));
        }
    }
    state.SetItemsProcessed(state.iterations() * NUM_SAMPLES);
}

/* ── Our library: C++ template ────────────────────────────── */

template<int N>
static void BM_Ours_Cpp(benchmark::State &state) {
    for (auto _ : state) {
        MedianFilter<int, N> filter;
        for (int i = 0; i < NUM_SAMPLES; i++) {
            benchmark::DoNotOptimize(filter.Insert(INT_DATA[i]));
        }
    }
    state.SetItemsProcessed(state.iterations() * NUM_SAMPLES);
}

/* ── Our library: C++ template float (fair compare to takingBytes) ── */

template<int N>
static void BM_Ours_Cpp_Float(benchmark::State &state) {
    for (auto _ : state) {
        MedianFilter<float, N> filter;
        for (int i = 0; i < NUM_SAMPLES; i++) {
            benchmark::DoNotOptimize(filter.Insert(FLOAT_DATA[i]));
        }
    }
    state.SetItemsProcessed(state.iterations() * NUM_SAMPLES);
}

/* ── Naive sort baseline ──────────────────────────────────── */

template<int N>
static void BM_NaiveSort(benchmark::State &state) {
    for (auto _ : state) {
        NaiveSortMedian<int, N> filter;
        for (int i = 0; i < NUM_SAMPLES; i++) {
            benchmark::DoNotOptimize(filter.Insert(INT_DATA[i]));
        }
    }
    state.SetItemsProcessed(state.iterations() * NUM_SAMPLES);
}

/* ── Insertion-sort ring baseline ─────────────────────────── */

template<int N>
static void BM_InsertionSortRing(benchmark::State &state) {
    for (auto _ : state) {
        InsertionSortMedian<int, N> filter;
        for (int i = 0; i < NUM_SAMPLES; i++) {
            benchmark::DoNotOptimize(filter.Insert(INT_DATA[i]));
        }
    }
    state.SetItemsProcessed(state.iterations() * NUM_SAMPLES);
}

/* ── nth_element baseline ─────────────────────────────────── */

template<int N>
static void BM_NthElement(benchmark::State &state) {
    for (auto _ : state) {
        NthElementMedian<int, N> filter;
        for (int i = 0; i < NUM_SAMPLES; i++) {
            benchmark::DoNotOptimize(filter.Insert(INT_DATA[i]));
        }
    }
    state.SetItemsProcessed(state.iterations() * NUM_SAMPLES);
}

/* ── vpetrigo/median-filter ───────────────────────────────── */

static void BM_Vpetrigo(benchmark::State &state) {
    const int win = static_cast<int>(state.range(0));
    std::vector<median_filter_node_int32_t> buf(win);
    median_filter_int32_t filter;

    for (auto _ : state) {
        median_filter_init_int32_t(&filter, buf.data(), win);
        for (int i = 0; i < NUM_SAMPLES; i++) {
            median_filter_insert_value_int32_t(&filter, static_cast<int32_t>(INT_DATA[i]));
            benchmark::DoNotOptimize(median_filter_get_median_int32_t(&filter));
        }
    }
    state.SetItemsProcessed(state.iterations() * NUM_SAMPLES);
}

/* ── takingBytes/MovingMedianFilter (float-only) ──────────── */

static void BM_TakingBytes(benchmark::State &state) {
    const int win = static_cast<int>(state.range(0));
    std::vector<float> buf(win);
    std::vector<float *> sorted_ptrs(win);
    /* TBMedian is an opaque type wrapping takingBytes' median struct */
    alignas(8) char med_storage[64];
    auto *med = reinterpret_cast<TBMedian *>(med_storage);

    for (auto _ : state) {
        TBMedian_Init(med, buf.data(), sorted_ptrs.data(), win);
        for (int i = 0; i < NUM_SAMPLES; i++) {
            benchmark::DoNotOptimize(TBMedian_Filter(med, FLOAT_DATA[i]));
        }
    }
    state.SetItemsProcessed(state.iterations() * NUM_SAMPLES);
}

/* ── Registration ─────────────────────────────────────────── */

#define WINDOW_SIZES ->Arg(3)->Arg(5)->Arg(7)->Arg(9)->Arg(11)->Arg(21)->Arg(31)->Arg(51)

/* Runtime window size (C APIs) */
BENCHMARK(BM_Ours_C) WINDOW_SIZES;
BENCHMARK(BM_Vpetrigo) WINDOW_SIZES;
BENCHMARK(BM_TakingBytes) WINDOW_SIZES;

/* Compile-time template benchmarks */
#define REG(Fn, N) BENCHMARK(Fn<N>)

#define REG_ALL(Fn) \
    REG(Fn, 3); REG(Fn, 5); REG(Fn, 7); REG(Fn, 9); \
    REG(Fn, 11); REG(Fn, 21); REG(Fn, 31); REG(Fn, 51)

REG_ALL(BM_Ours_Cpp);
REG_ALL(BM_Ours_Cpp_Float);
REG_ALL(BM_NaiveSort);
REG_ALL(BM_InsertionSortRing);
REG_ALL(BM_NthElement);
