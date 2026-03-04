# MedianFilter

[![CI](https://github.com/accabog/MedianFilter/actions/workflows/ci.yml/badge.svg)](https://github.com/accabog/MedianFilter/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Language: C/C++](https://img.shields.io/badge/Language-C%2FC%2B%2B-brightgreen.svg)]()
[![Platform: Bare-metal / RTOS / Desktop](https://img.shields.io/badge/Platform-Bare--metal%20%2F%20RTOS%20%2F%20Desktop-orange.svg)]()
[![No dynamic allocation](https://img.shields.io/badge/Heap-None-critical.svg)]()
[![PlatformIO](https://badges.registry.platformio.org/packages/accabog/library/MedianFilter.svg)](https://registry.platformio.org/libraries/accabog/MedianFilter)

**Heap-free, O(n) sliding-window median filter for embedded systems. Drop two files into your project and go.**

Removes impulse noise (salt-and-pepper spikes) from analog signals in real time — ideal for ADC readings, sensor fusion, and any DSP pipeline where you need a robust filter with zero malloc.

## Features

- **Zero heap allocation** — stack or static buffers only, safe for bare-metal and RTOS
- **O(n/2) average insert** — bidirectional search from the median pointer
- **O(1) median tracking** — the median pointer adjusts by at most one step per insert, never recomputed
- **Stable duplicate handling** — address-based tiebreaker eliminates ambiguity for equal-valued samples
- **C API** — caller-allocated buffers, two functions, done
- **C++ header-only template** — type-safe, compile-time `static_assert` on window size and type
- **No dependencies** — only `<stdint.h>` (C) or `<type_traits>` / `<cstdint>` (C++)
- **24 bytes per sample** — constant, predictable RAM footprint (64-bit); 12 bytes on 32-bit ARM

## Quick Start

### C

```c
#include "MedianFilter.h"

#define WINDOW_SIZE 7

static sMedianFilter_t filter;
static sMedianNode_t   buffer[WINDOW_SIZE];

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
| [Arduino](examples/platform/arduino_analog_filter.ino) | `analogRead()` filter |
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

Age order is implicit — nodes are recycled in buffer order via pointer arithmetic, so no explicit age pointer is stored. This saves one pointer per node (25% RAM reduction).

On every insert:

1. **Evict** the oldest node (age head, advanced by `ptr + 1` with wraparound) and unlink it from the value chain
2. **Search** from the median pointer — up or down depending on the new sample's value
3. **Insert** the recycled node at the correct sorted position
4. **Adjust** the median pointer by one step if needed

The bidirectional search means average traversal is ~n/4 rather than n/2. The median pointer never needs to be recomputed from scratch — it tracks incrementally.

## Performance

Consistently among the fastest across all window sizes — and the clear winner at larger windows where it matters most.

Instructions per insert (lower is better), measured with [Valgrind/Callgrind](https://valgrind.org/docs/manual/cl-manual.html) (`gcc -O2`, averaged over 5 random seeds). Results are **deterministic and hardware-independent** — reproducible on any machine:

| Window | **Ours (C)** | **Ours (C++)** | Insertion-sort ring | [vpetrigo](https://github.com/vpetrigo/median-filter) | std::nth_element | Naive sort |
|:------:|:------------:|:--------------:|:-------------------:|:------------------------------------------------------:|:----------------:|:----------:|
| 3      | **124**      | **131**        | 112                 | 159                                                    | 179              | 194        |
| 7      | **128**      | **137**        | 149                 | 190                                                    | 310              | 300        |
| 11     | **134**      | **143**        | 173                 | 218                                                    | 390              | 444        |
| 31     | **162**      | **172**        | 288                 | 359                                                    | 751              | 1446       |
| 51     | **189**      | **200**        | 395                 | 499                                                    | 1078             | 2504       |
| 101    | **259**      | **270**        | 669                 | 847                                                    | 1880             | 5452       |

"Ours (C)" uses the opt-in `MEDIANFILTER_INLINE_API` inline Insert. At small windows (3), insertion-sort ring wins thanks to cache locality on tiny arrays. From window 7+, our linked-list approach takes the lead — and at window 101, it uses **2.6× fewer instructions** than insertion-sort and **3.3× fewer** than vpetrigo.

RAM = `window_size * 24 + 40` bytes (64-bit). On 32-bit ARM, nodes are 12 bytes each. See [`benchmarks/`](benchmarks/) for full results, throughput benchmarks, and methodology.

## API Reference

### C

| Function | Signature | Returns |
|:---------|:----------|:--------|
| **Init** | `int MEDIANFILTER_Init(sMedianFilter_t *filter)` | `0` on success, `-1` on invalid config |
| **Insert** | `int MEDIANFILTER_Insert(sMedianFilter_t *restrict filter, int sample)` | Current median value |

The caller must allocate `sMedianFilter_t` and a `sMedianNode_t[N]` buffer, set `numNodes = N` and `medianBuffer = buffer`, then call Init. Window size must be **odd** and **> 1**.

**Inline variant:** For tight-loop callers (e.g. processing an ADC buffer), define `MEDIANFILTER_INLINE_API` before including the header to get a `static inline` Insert that eliminates function-call overhead.

### C++

| Method | Signature | Returns |
|:-------|:----------|:--------|
| **Constructor** | `MedianFilter<type, N>()` | — |
| **Insert** | `type Insert(type value)` | Current median value |

Template parameters: `type` must be arithmetic, `N` must be odd and >= 3. Enforced via `static_assert`.

## Integration

**Just copy the files — no build system required.**

For C: add `MedianFilter.h` and `MedianFilter.c` to your project.
For C++: add `MedianFilter.hpp` (header-only, nothing to compile).

Both APIs are compatible in the same translation unit — the C header includes `extern "C"` guards.

## License

[MIT](LICENSE) — free for commercial and personal use.
