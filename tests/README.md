# Tests

## Running

```bash
make test       # build and run all tests
make clean      # remove build artifacts
```

## Test Files

| File | Focus | Tests |
|------|-------|-------|
| `stress_test.c` | Correctness across 14 input patterns x 10 window sizes (3-99) | 140 |
| `circular_test.c` | Circular value-chain integrity, warmup edge cases, extreme values | 48 |
| `init_test.c` | Init argument validation, initial head wiring and the type aliases | 10 |
| `exhaustive_test.c` | Every reachable filter state for windows 3, 5 and 7 (proof for every input sequence, see the file header) | 7 |
| `test_cpp.cpp` | C++ template correctness for `int`, `float`, `double`, every short sequence over {-1, 0, 1}, and NaN input that must not hang Insert | 23 |
| `gnu89_header_test.c` | Compile-only: `MedianFilter.h` in C90 code with GNU extensions | — |

The correctness tests compare the filter's output against a brute-force
reference median computed by sorting the current window. A mismatch in any sample
fails the test. `exhaustive_test.c` also compares the complete filter state
(every link and head) after each insert.

`make test` builds the stress and exhaustive tests once per configuration:
default, inline API with and without `MEDIANFILTER_FORCE_INLINE`,
`MEDIANFILTER_SMALL_MAX=0` and `=5`, `MEDIANFILTER_SEARCH_FROM_PREVIOUS`, and
all of them together through the inline API. `circular_test` is also built with
`MEDIANFILTER_SMALL_MAX=0`, and the C++ test with `MEDIANFILTER_FORCE_INLINE`
and `MEDIANFILTER_SEARCH_FROM_PREVIOUS` (minimum window 3 and 21).

## CI

Tests run on every push and PR across 12 compiler/standard combinations
(GCC, Clang, MSVC x C99/C11/C++11/C++14/C++17) and the configuration
macros, plus AddressSanitizer and UndefinedBehaviorSanitizer. See
`.github/workflows/ci.yml`.

## Harness

All test files use shared macros from `test_harness.h` for consistent
output formatting. No external test framework required.
