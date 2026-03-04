@echo off
REM Run benchmark pinned to CPU 0 with high priority
set "EXE=%~dp0build_win\Release\bench_median_filters.exe"
start "" /b /wait /affinity 0x1 /high "%EXE%" --benchmark_repetitions=5 --benchmark_report_aggregates_only=true
