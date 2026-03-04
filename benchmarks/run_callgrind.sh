#!/bin/bash
#
# run_callgrind.sh — Deterministic instruction-count benchmarks via callgrind.
#
# Produces 0% variance results regardless of CPU speed, load, or OS.
# Reports "instructions per insert" (IPI) — lower is better.

set -e
cd "$(dirname "$0")"

WINDOWS="3 5 7 9 11 21 31 51 101"

echo "=== Compiling ==="

# Our C API (non-inlined)
gcc -O2 -o /tmp/cg_ours_c callgrind_bench.c ../MedianFilter.c -I..
# Our C API (inlined)
gcc -O2 -DMEDIANFILTER_INLINE_API -o /tmp/cg_ours_c_inline callgrind_bench.c ../MedianFilter.c -I..
# Our C++ template
g++ -O2 -std=c++17 -o /tmp/cg_ours_cpp callgrind_bench_cpp.cpp -I..

echo "=== Running callgrind (this takes a few minutes) ==="
echo ""

# Header
printf "%-8s  %12s  %12s  %12s\n" "Window" "C (Ir/ins)" "C inline" "C++ (Ir/ins)"
printf "%-8s  %12s  %12s  %12s\n" "------" "----------" "--------" "------------"

for w in $WINDOWS; do
    # Run each under callgrind, extract total instruction count
    ir_c=$(valgrind --tool=callgrind --callgrind-out-file=/dev/null \
           /tmp/cg_ours_c $w 2>&1 | grep -oP 'refs:\s+\K[\d,]+' | tr -d ',')

    ir_ci=$(valgrind --tool=callgrind --callgrind-out-file=/dev/null \
            /tmp/cg_ours_c_inline $w 2>&1 | grep -oP 'refs:\s+\K[\d,]+' | tr -d ',')

    ir_cpp=$(valgrind --tool=callgrind --callgrind-out-file=/dev/null \
             /tmp/cg_ours_cpp $w 2>&1 | grep -oP 'refs:\s+\K[\d,]+' | tr -d ',')

    # Instructions per insert (total / NUM_SAMPLES), subtracting ~same baseline overhead
    ipi_c=$((ir_c / 10000))
    ipi_ci=$((ir_ci / 10000))
    ipi_cpp=$((ir_cpp / 10000))

    printf "%-8d  %12d  %12d  %12d\n" "$w" "$ipi_c" "$ipi_ci" "$ipi_cpp"
done

echo ""
echo "Ir/insert = instruction refs per MEDIANFILTER_Insert call (lower is better)"
echo "Results are 100% deterministic — rerun to verify."
