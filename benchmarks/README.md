# Benchmarks

Comparative benchmarks using [Google Benchmark](https://github.com/google/benchmark).

## Build & Run

```bash
cd benchmarks
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/bench_median_filters
```

Requires CMake 3.14+ and internet access (first build fetches dependencies).

## Algorithms Compared

| Name | Type | Source | Complexity |
|------|------|--------|------------|
| **MedianFilter (C)** | int | This repo | O(n/2) avg |
| **MedianFilter (C++)** | int | This repo | O(n/2) avg |
| **MedianFilter (C++ float)** | float | This repo | O(n/2) avg |
| Naive sort | int | Baseline | O(n log n) |
| Insertion-sort ring | int | Baseline | O(n) worst |
| std::nth_element | int | Baseline | O(n) avg |
| [vpetrigo/median-filter](https://github.com/vpetrigo/median-filter) | int32_t | Third-party | O(n) |
| [takingBytes/MovingMedianFilter](https://github.com/takingBytes/MovingMedianFilter) | float | Third-party | O(n log n) |

Note: takingBytes only supports `float`, so we compare it against our C++ float template.

## Sample Output

Machine: 16-core x86-64, GCC, `-O2`, Release build.

```
-----------------------------------------------------------------------------------
Benchmark                         Time             CPU   Iterations UserCounters...
-----------------------------------------------------------------------------------
BM_Ours_C/3                  106562 ns       106558 ns         6294 items_per_second=93.8456M/s
BM_Ours_C/5                  158734 ns       158727 ns         4586 items_per_second=63.0014M/s
BM_Ours_C/7                  179433 ns       179425 ns         3819 items_per_second=55.7337M/s
BM_Ours_C/9                  207402 ns       207386 ns         3509 items_per_second=48.2192M/s
BM_Ours_C/11                 217604 ns       217592 ns         3133 items_per_second=45.9575M/s
BM_Ours_C/21                 293555 ns       293538 ns         2385 items_per_second=34.0671M/s
BM_Ours_C/31                 316788 ns       316766 ns         2014 items_per_second=31.5691M/s
BM_Ours_C/51                 416097 ns       416064 ns         1794 items_per_second=24.0347M/s
BM_Vpetrigo/3                124479 ns       124473 ns         5143 items_per_second=80.3389M/s
BM_Vpetrigo/5                189513 ns       189505 ns         3502 items_per_second=52.7691M/s
BM_Vpetrigo/7                230313 ns       230302 ns         2855 items_per_second=43.4212M/s
BM_Vpetrigo/9                254908 ns       254892 ns         2752 items_per_second=39.2323M/s
BM_Vpetrigo/11               269257 ns       269235 ns         2376 items_per_second=37.1423M/s
BM_Vpetrigo/21               384021 ns       384003 ns         1900 items_per_second=26.0414M/s
BM_Vpetrigo/31               515771 ns       515748 ns         1330 items_per_second=19.3893M/s
BM_Vpetrigo/51               698041 ns       698008 ns          982 items_per_second=14.3265M/s
BM_TakingBytes/3             171369 ns       171361 ns         4746 items_per_second=58.3562M/s
BM_TakingBytes/5             285411 ns       285397 ns         2298 items_per_second=35.0389M/s
BM_TakingBytes/7             400694 ns       400675 ns         1778 items_per_second=24.9579M/s
BM_TakingBytes/9             499092 ns       499069 ns         1435 items_per_second=20.0373M/s
BM_TakingBytes/11            651230 ns       651202 ns         1028 items_per_second=15.3562M/s
BM_TakingBytes/21           1395011 ns      1394935 ns          578 items_per_second=7.16879M/s
BM_TakingBytes/31           2504616 ns      2504500 ns          345 items_per_second=3.99281M/s
BM_TakingBytes/51           3521649 ns      3521490 ns          185 items_per_second=2.83971M/s
BM_Ours_Cpp<3>               116728 ns       116723 ns         5875 items_per_second=85.6727M/s
BM_Ours_Cpp<5>               148212 ns       148206 ns         4435 items_per_second=67.4738M/s
BM_Ours_Cpp<7>               176555 ns       176547 ns         3785 items_per_second=56.6422M/s
BM_Ours_Cpp<9>               207443 ns       207426 ns         3397 items_per_second=48.21M/s
BM_Ours_Cpp<11>              221288 ns       221271 ns         3194 items_per_second=45.1935M/s
BM_Ours_Cpp<21>              260243 ns       260226 ns         2716 items_per_second=38.4282M/s
BM_Ours_Cpp<31>              312612 ns       312598 ns         2147 items_per_second=31.99M/s
BM_Ours_Cpp<51>              391254 ns       391230 ns         1760 items_per_second=25.5604M/s
BM_Ours_Cpp_Float<3>         104789 ns       104783 ns         5981 items_per_second=95.435M/s
BM_Ours_Cpp_Float<5>         173848 ns       173834 ns         4209 items_per_second=57.5262M/s
BM_Ours_Cpp_Float<7>         192480 ns       192472 ns         3760 items_per_second=51.9557M/s
BM_Ours_Cpp_Float<9>         232802 ns       232788 ns         3185 items_per_second=42.9575M/s
BM_Ours_Cpp_Float<11>        249005 ns       248994 ns         2931 items_per_second=40.1616M/s
BM_Ours_Cpp_Float<21>        308818 ns       308801 ns         2260 items_per_second=32.3833M/s
BM_Ours_Cpp_Float<31>        342113 ns       342098 ns         2077 items_per_second=29.2314M/s
BM_Ours_Cpp_Float<51>        454593 ns       454573 ns         1545 items_per_second=21.9987M/s
BM_NaiveSort<3>              138633 ns       138627 ns         5335 items_per_second=72.1359M/s
BM_NaiveSort<5>              247217 ns       247203 ns         2597 items_per_second=40.4525M/s
BM_NaiveSort<7>              584271 ns       584245 ns         1058 items_per_second=17.1161M/s
BM_NaiveSort<9>              829421 ns       829373 ns         1001 items_per_second=12.0573M/s
BM_NaiveSort<11>             968098 ns       968055 ns          657 items_per_second=10.33M/s
BM_NaiveSort<21>            2290793 ns      2290578 ns          328 items_per_second=4.36571M/s
BM_NaiveSort<31>            4235774 ns      4235589 ns          171 items_per_second=2.36095M/s
BM_NaiveSort<51>            7769396 ns      7769038 ns           96 items_per_second=1.28716M/s
BM_InsertionSortRing<3>       57759 ns        57756 ns        12030 items_per_second=173.142M/s
BM_InsertionSortRing<5>      141825 ns       141819 ns         5045 items_per_second=70.5126M/s
BM_InsertionSortRing<7>      180431 ns       180421 ns         3878 items_per_second=55.4258M/s
BM_InsertionSortRing<9>      176114 ns       176103 ns         3349 items_per_second=56.785M/s
BM_InsertionSortRing<11>     204752 ns       204734 ns         3718 items_per_second=48.8438M/s
BM_InsertionSortRing<21>     308072 ns       308053 ns         2273 items_per_second=32.4619M/s
BM_InsertionSortRing<31>     386541 ns       386517 ns         1788 items_per_second=25.8721M/s
BM_InsertionSortRing<51>     450055 ns       450033 ns         1340 items_per_second=22.2206M/s
BM_NthElement<3>             107893 ns       107888 ns         6484 items_per_second=92.6891M/s
BM_NthElement<5>             278849 ns       278836 ns         2331 items_per_second=35.8634M/s
BM_NthElement<7>             644143 ns       644112 ns          984 items_per_second=15.5253M/s
BM_NthElement<9>            1010253 ns      1010149 ns          681 items_per_second=9.89953M/s
BM_NthElement<11>           1207323 ns      1207263 ns          575 items_per_second=8.2832M/s
BM_NthElement<21>           1950631 ns      1950508 ns          382 items_per_second=5.12687M/s
BM_NthElement<31>           2653449 ns      2653213 ns          276 items_per_second=3.76901M/s
BM_NthElement<51>           3629969 ns      3629539 ns          198 items_per_second=2.75517M/s
```

## Useful Flags

```bash
# Filter specific benchmarks
./build/bench_median_filters --benchmark_filter="BM_Ours"

# JSON output
./build/bench_median_filters --benchmark_format=json --benchmark_out=results.json

# CSV output
./build/bench_median_filters --benchmark_format=csv --benchmark_out=results.csv
```
