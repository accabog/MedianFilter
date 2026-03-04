cmake_minimum_required(VERSION 3.14)
project(MedianFilterBenchmarks LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

include(FetchContent)

# ── Google Benchmark ──────────────────────────────────────────
FetchContent_Declare(
  benchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG        v1.9.1
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(benchmark)

# ── vpetrigo/median-filter ────────────────────────────────────
FetchContent_Declare(
  vpetrigo_median
  GIT_REPOSITORY https://github.com/vpetrigo/median-filter.git
  GIT_TAG        main
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
)
FetchContent_MakeAvailable(vpetrigo_median)

# ── takingBytes/MovingMedianFilter ────────────────────────────
FetchContent_Declare(
  takingbytes_median
  GIT_REPOSITORY https://github.com/takingBytes/MovingMedianFilter.git
  GIT_TAG        develop
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
)
FetchContent_MakeAvailable(takingbytes_median)

# takingBytes is just median.c + median.h — build as a static lib
# We use a C wrapper to avoid MedianFilter() name collision with our C++ class.
add_library(takingbytes_median_lib STATIC
  ${takingbytes_median_SOURCE_DIR}/median.c
  ${CMAKE_CURRENT_SOURCE_DIR}/competitors/takingbytes_wrapper.c
)
target_include_directories(takingbytes_median_lib PUBLIC
  ${takingbytes_median_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/competitors
)

# ── Our library (C API) ──────────────────────────────────────
add_library(median_filter_c STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../MedianFilter.c
)
target_include_directories(median_filter_c PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# ── Benchmark executable ─────────────────────────────────────
add_executable(bench_median_filters bench_median_filters.cpp noninline_wrapper.c)
target_include_directories(bench_median_filters PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${vpetrigo_median_SOURCE_DIR}/src
  ${takingbytes_median_SOURCE_DIR}
)
target_link_libraries(bench_median_filters
  benchmark::benchmark
  benchmark::benchmark_main
  median_filter_c
  takingbytes_median_lib
)

# vpetrigo source compiled directly into the benchmark target
target_sources(bench_median_filters PRIVATE
  ${vpetrigo_median_SOURCE_DIR}/src/median_filter.c
)
