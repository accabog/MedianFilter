# MedianFilter

[![CI](https://github.com/accabog/MedianFilter/actions/workflows/ci.yml/badge.svg)](https://github.com/accabog/MedianFilter/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Language: C/C++](https://img.shields.io/badge/Language-C%2FC%2B%2B-brightgreen.svg)]()
[![Platform: Bare-metal / RTOS / Desktop](https://img.shields.io/badge/Platform-Bare--metal%20%2F%20RTOS%20%2F%20Desktop-orange.svg)]()
[![No dynamic allocation](https://img.shields.io/badge/Heap-None-critical.svg)]()
[![PlatformIO](https://badges.registry.platformio.org/packages/accabog/library/MedianFilter.svg)](https://registry.platformio.org/libraries/accabog/MedianFilter)

**Heap-free sliding-window median filter for embedded systems: O(1) median tracking, fewer than n/2 list steps per sample. Drop two files into your project and go.**

Removes impulse noise (salt-and-pepper spikes) from analog signals in real time — ideal for ADC readings, sensor fusion, and any DSP pipeline where you need a robust filter with zero malloc.

## Features

- **Zero heap allocation** — stack or static buffers only, safe for bare-metal and RTOS
- **Fewer than n/2 list steps per insert** — the search starts at the median; new minima and maxima need no search at all, so constant and monotonic signals (also with repeated values) cost the same at every window size once the warm-up zeros have left the window
- **O(1) median tracking** — the median pointer adjusts by at most one step per insert, never recomputed
- **Duplicate-friendly** — equal values are kept in arrival order, so ties cost no extra comparisons and a flat signal with occasional spikes stays cheap
- **Branch-free window of 3** — a median-of-three network replaces the list for the most common spike-removal window
- **C API** — caller-allocated buffers, two functions, done
- **C++ header-only template** — type-safe, compile-time `static_assert` on window size and type
- **No dependencies** — only `<stdint.h>` (C) or `<type_traits>` / `<cstdint>` (C++)
- **24 bytes per sample** — constant, predictable RAM footprint (64-bit); 12 bytes on 32-bit ARM

## Quick Start

### C

```c
#include "MedianFilter.h"

#define WINDOW_SIZE 7

static MedianFilter_t     filter;
static MedianFilterNode_t buffer[WINDOW_SIZE];

void setup(void)
{
    filter.numNodes     = WINDOW_SIZE;
    filter.medianBuffer = buffer;
    MEDIANFILTER_Init(&filter);
}

int process(int sample)
{
    return MEDIANFILTER_Insert(&filter, sample);
}
```

### C++

```cpp
#include "MedianFilter.hpp"

MedianFilter<int, 7> filter;

int process(int sample)
{
    return filter.Insert(sample);
}
```

The C++ template accepts any arithmetic type (`int`, `float`, `double`, `int16_t`, ...) and enforces an odd window size >= 3 at compile time.

## Examples

The [`examples/`](examples/) folder contains runnable demos and platform-specific reference code:

| Example | Description |
|---------|-------------|
| [Basic Usage](examples/01_basic_usage.c) | Minimal C API — init, insert, print |
| [ADC Noise Filter](examples/02_adc_noise_filter.c) | Spike removal from a simulated 12-bit ADC signal |
| [Multichannel Sensors](examples/03_multichannel_sensors.c) | Independent filters on temperature, humidity, and pressure |
| [Window Size Comparison](examples/04_window_size_comparison.c) | Noise removal vs. signal lag tradeoff |
| [C++ Template](examples/05_cpp_template.cpp) | `float`, `int16_t`, `double` type flexibility |
| [STM32 HAL](examples/platform/stm32_hal_adc.c) | ADC interrupt callback (reference) |
| [Arduino](examples/platform/arduino_analog_filter/arduino_analog_filter.ino) | `analogRead()` filter |
| [ESP-IDF](examples/platform/esp_idf_adc_filter.c) | FreeRTOS ADC task (reference) |

Build and run all desktop demos:

    make examples

## How It Works

Each sample lives in a node with two link chains:

```
Age order:     buffer[0] → [1] → ... → [N-1] → [0]   (implicit, round-robin)

Value chain:   smallest <══> ... <══> largest          (doubly-linked, sorted)

Median ptr:         ──────────> M                      (always the middle node)
```

Age order is implicit — nodes are recycled in buffer order via pointer arithmetic, so no explicit age pointer is stored. This saves one pointer per node (25% RAM reduction). Equal values sit in the value chain in arrival order, oldest first.

On every insert:

1. **Evict** the oldest node (age head, advanced by `ptr + 1` with wraparound) and unlink it from the value chain. Being the oldest, it is the lowest of any run of equal values, so one comparison tells whether it was above or below the median.
2. **Check the ends** — a new minimum or maximum is linked next to the current one without searching.
3. **Search** from the median pointer, up or down, for the new sample's sorted position. A sample that repeats the previous sample's value at the median or at the minimum goes straight above the previous sample's node, so flat and falling signals never walk their own run of equal values.
4. **Insert** the recycled node and **adjust** the median pointer by at most one step.

Because the ends are checked first, neither search direction needs a bounds test: each step is two dependent loads (the link, then the value) and one comparison. The search passes over the nodes between the median and the new sample's position: about n/4 for noise-like input and fewer than n/2 at most. The median pointer never needs to be recomputed from scratch — it tracks incrementally.

Windows of 3 skip the list: the median of the three buffer values comes from a branch-free min/max network (see [Configuration](#configuration)).

## Performance

The fewest instructions of the compared implementations at every window size (the out-of-line C build ties the insertion-sort ring at window 3).

Instructions per insert (lower is better), measured with [Valgrind/Callgrind](https://valgrind.org/docs/manual/cl-manual.html) on x86-64 (gcc 13.3 `-O2`, glibc 2.39, random data from `rand()`, mean of seeds 42-46), using the drivers in [`benchmarks/`](benchmarks/#instruction-counts): the whole-process instruction count divided by the 10,000 inserts. The counts are exact for a given compiler and flags; other compilers and C++ runtimes give different numbers. Every column includes about 74 instructions per insert of process start-up, data generation and initialization:

| Window | **Ours (C)** | **Ours (C++)** | Insertion-sort ring | [vpetrigo](https://github.com/vpetrigo/median-filter) | std::nth_element | Naive sort |
|:------:|:------------:|:--------------:|:-------------------:|:------------------------------------------------------:|:----------------:|:----------:|
| 3      | **102**      | **100**        | 109                 | 156                                                    | 176              | 191        |
| 7      | **125**      | **131**        | 146                 | 187                                                    | 308              | 297        |
| 11     | **128**      | **134**        | 170                 | 215                                                    | 387              | 442        |
| 31     | **143**      | **149**        | 285                 | 356                                                    | 747              | 1441       |
| 51     | **158**      | **165**        | 391                 | 496                                                    | 1060             | 2501       |
| 101    | **196**      | **202**        | 666                 | 844                                                    | 1886             | 5457       |

"Ours (C)" uses the opt-in `MEDIANFILTER_INLINE_API` inline Insert. At window 101 it uses **3.4× fewer instructions** than the insertion-sort ring and **4.3× fewer** than vpetrigo; without the shared start-up overhead the insert itself needs about 4.8× and 6.3× fewer. On a Cortex-M4 (gcc `-O2`) an insert of random data takes about 30 instructions at window 3 and about 150 at window 101.

RAM = `window_size * 24 + 40` bytes (64-bit). On 32-bit targets, nodes are 12 bytes each and the filter struct is 20 bytes, so RAM = `window_size * 12 + 20` bytes. In C++, `MedianFilter<T, 3>` stores only its three values and an index (16 bytes for `int`). See [`benchmarks/`](benchmarks/) for throughput benchmarks and methodology.

## API Reference

### C

| Function | Signature | Returns |
|:---------|:----------|:--------|
| **Init** | `int MEDIANFILTER_Init(MedianFilter_t *filter)` | `0` on success, `-1` on invalid config |
| **Insert** | `int MEDIANFILTER_Insert(MedianFilter_t *restrict filter, int sample)` | Current median value |

The caller must allocate `MedianFilter_t` and a `MedianFilterNode_t[N]` buffer, set `numNodes = N` and `medianBuffer = buffer`, then call Init. Window size must be **odd** and **> 1**.

Read the median from Insert's return value: the struct fields are internal, and for windows served by a selection network (3 by default, also 5 with `MEDIANFILTER_SMALL_MAX=5`) the value chain and its heads are not maintained at all. If a filter's state outlives a library update, for example in RAM retained across a firmware update, call Init again.

> The type names were previously `sMedianFilter_t` / `sMedianNode_t`; those still work as deprecated aliases, so existing code needs no changes.

**Inline variant:** For tight-loop or interrupt callers (e.g. processing an ADC buffer), define `MEDIANFILTER_INLINE_API` before including the header to get a `static inline` Insert. Compilers may still keep it out of line (clang, and gcc at `-Os`, once a file has two call sites); define `MEDIANFILTER_FORCE_INLINE` as well to force it inline at every call site. That saves the call, about 8 instructions per sample with gcc on x86-64, at the cost of one copy of Insert per call site (about 0.2 KB on Cortex-M at `-Os`, 0.3 KB at `-O2`).

### C++

| Method | Signature | Returns |
|:-------|:----------|:--------|
| **Constructor** | `MedianFilter<type, N>()` | — |
| **Insert** | `type Insert(type value)` | Current median value |

Template parameters: `type` must be arithmetic, `N` must be odd and >= 3. Enforced via `static_assert`.

### Configuration

Optional macros. For the C API, define them for the whole build (`-D` options on the compiler command line, PlatformIO `build_flags`), so that `MedianFilter.c` sees the same values as your own files: a `#define` in your source does not reach `MedianFilter.c`. The Arduino IDE has no per-library flags; add the defines at the top of `MedianFilter.h` instead. For the header-only C++ template a `#define` before the include works, but use the same values in every file.

| Macro | Default | Effect |
|:------|:-------:|:-------|
| `MEDIANFILTER_SMALL_MAX` | `3` | C: windows up to this size use a branch-free network instead of the list: `0` (never), `3` or `5`. The window-3 network adds about 60-80 bytes of code on Cortex-M; `0` removes it. `5` pays off on out-of-order cores (x86-64, AArch64) but executes more instructions on Cortex-M and on RISC-V without Zbb. The C++ template always uses the network for windows of 3. |
| `MEDIANFILTER_SEARCH_FROM_PREVIOUS` | off | Start the search at the previous sample's node. For slowly varying signals (ADC readings, sensors) in larger windows: at window 101 on an ADC-like signal it executes about half the instructions. Slower for uncorrelated noise in windows below about 101 and for periodic or multiplexed input (several channels through one filter). |
| `MEDIANFILTER_INLINE_API` | off | C: `static inline` Insert in the header (see above). |
| `MEDIANFILTER_FORCE_INLINE` | off | Force Insert inline at every call site: C (together with `MEDIANFILTER_INLINE_API`) and C++. |
| `MEDIANFILTER_INLINE_SPEC` | — | C: replaces the inline specifier altogether; it must still declare a `static inline` function. |
| `MEDIANFILTER_SEARCH_FROM_PREVIOUS_MIN_N` | `3` | C++: use the previous-sample search only from this window size up; the test is resolved at compile time. |

Build `MedianFilter.c` with `-O2` or `-Os` (with gcc, `-O3` executes the same instructions as `-O2`). Avoid `-O0` builds for interrupt use: an insert costs 2-3 times as much there.

## Integration

**Just copy the files — no build system required.**

For C: add `MedianFilter.h` and `MedianFilter.c` to your project.
For C++: add `MedianFilter.hpp` (header-only, nothing to compile).

Both APIs are compatible in the same translation unit — the C header includes `extern "C"` guards.

## License

[MIT](LICENSE) — free for commercial and personal use.
