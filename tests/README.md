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
| `circular_test.c` | Circular value-chain integrity, warmup edge cases, extreme values | 51 |
| `test_cpp.cpp` | C++ template correctness for `int`, `float`, `double` types | 12 |

Every test compares the filter's output against a brute-force reference
median computed by sorting the current window. A mismatch in any sample
fails the test.

## CI

Tests run on every push and PR across 12 compiler/standard combinations
(GCC, Clang, MSVC x C99/C11/C++11/C++14/C++17), plus AddressSanitizer
and UndefinedBehaviorSanitizer. See `.github/workflows/ci.yml`.

## Harness

All test files use shared macros from `test_harness.h` for consistent
output formatting. No external test framework required.
