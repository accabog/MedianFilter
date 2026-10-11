# Changelog

## Unreleased

### Changed

- `MEDIANFILTER_Insert` and `MedianFilter<T, N>::Insert` are faster, with the same API,
  struct layouts and medians. Equal values are kept in arrival order instead of being
  ordered by node address; the new sample is checked against the minimum and maximum
  before searching; a sample that repeats the previous sample at the median or at the
  minimum is linked without a search; windows of 3 use a branch-free median-of-three
  network.
- C API: for windows served by the network (3 by default, also 5 with
  `MEDIANFILTER_SMALL_MAX=5`) `valueHead`, `medianHead` and the node links are no longer
  maintained. Read the median from the return value of `MEDIANFILTER_Insert`.
- Filter state is not compatible with 1.0.0: call `MEDIANFILTER_Init` again if a filter's
  state survives a library update (for example in retained RAM), and rebuild every file
  that defines `MEDIANFILTER_INLINE_API`.
- `MEDIANFILTER_Init` and the C++ constructor no longer compute a modulo per node, so no
  division routine is linked on cores without a hardware divider.
- C++: `MedianFilter<T, 3>` stores only its values and an index. For `float` and
  `double` the sign of a zero median can differ from 1.0.0.
- Code size: `MedianFilter.c` is about 80-105 bytes larger on Cortex-M and 60-90 bytes
  larger on AVR in the default configuration. With `MEDIANFILTER_SMALL_MAX=0` it is within
  about 40 bytes of 1.0.0 on Cortex-M and about 35 bytes smaller on AVR.

### Added

- `MEDIANFILTER_SMALL_MAX` (C): `0`, `3` (default) or `5`.
- `MEDIANFILTER_SEARCH_FROM_PREVIOUS` (C and C++) and
  `MEDIANFILTER_SEARCH_FROM_PREVIOUS_MIN_N` (C++), opt-in.
- `MEDIANFILTER_FORCE_INLINE` (C together with `MEDIANFILTER_INLINE_API`, and C++) and
  `MEDIANFILTER_INLINE_SPEC` (C). `MEDIANFILTER_INLINE_API` alone still produces a plain
  `static inline` Insert, as in 1.0.0.
- An exhaustive state test (`tests/exhaustive_test.c`), configuration builds in
  `make test` and CI, and job time limits in CI.

### Fixed

- `MedianFilter.h` compiles in C90 code with GNU extensions (`-std=gnu89`). In C++,
  `MF_RESTRICT` expands to `__restrict` on gcc and clang.
- Example 05 no longer claims that `MedianFilter<int16_t, N>` saves RAM.
