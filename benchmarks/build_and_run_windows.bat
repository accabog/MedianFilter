@echo off
REM Native Windows benchmark build & run script
REM Builds with MSVC (Release/O2), runs with CPU pinning + high priority

setlocal enabledelayedexpansion

REM ── Setup MSVC environment ──────────────────────────────────
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
if errorlevel 1 (
    echo ERROR: Could not find MSVC Build Tools
    exit /b 1
)

REM ── Configure & build ───────────────────────────────────────
set "SRC=%~dp0"
REM Remove trailing backslash
if "%SRC:~-1%"=="\" set "SRC=%SRC:~0,-1%"
set "BUILD=%SRC%\build_win"

echo [1/3] Configuring CMake (MSVC Release)...
cmake -S "%SRC%" -B "%BUILD%" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 (
    echo ERROR: CMake configure failed
    exit /b 1
)

echo [2/3] Building...
cmake --build "%BUILD%" --config Release
if errorlevel 1 (
    echo ERROR: Build failed
    exit /b 1
)

REM ── Find the executable ─────────────────────────────────────
set "EXE=%BUILD%\Release\bench_median_filters.exe"
if not exist "%EXE%" (
    set "EXE=%BUILD%\bench_median_filters.exe"
)
if not exist "%EXE%" (
    echo ERROR: Cannot find bench_median_filters.exe
    exit /b 1
)

echo [3/3] Running benchmark (pinned to CPU 0, high priority)...
echo.

REM /affinity 0x1 = pin to CPU 0 (first P-core)
REM /high = high priority class (realtime requires admin)
start /b /wait /affinity 0x1 /high "%EXE%" --benchmark_repetitions=5 --benchmark_report_aggregates_only=true

endlocal
