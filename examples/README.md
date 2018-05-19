# Examples

Runnable demos and platform-specific reference code for the MedianFilter library.

## Desktop Demos (compilable)

Build all demos from the project root:

    make examples

Or build individually from this directory:

| # | File | Build Command | What It Shows |
|---|------|---------------|---------------|
| 1 | `01_basic_usage.c` | `gcc -I.. -o basic_usage 01_basic_usage.c ../MedianFilter.c` | Minimal C API usage |
| 2 | `02_adc_noise_filter.c` | `gcc -I.. -O2 -o adc_noise 02_adc_noise_filter.c ../MedianFilter.c -lm` | Spike removal from a simulated ADC signal |
| 3 | `03_multichannel_sensors.c` | `gcc -I.. -o multichannel 03_multichannel_sensors.c ../MedianFilter.c` | Multiple independent filters on different sensors |
| 4 | `04_window_size_comparison.c` | `gcc -I.. -O2 -o window_cmp 04_window_size_comparison.c ../MedianFilter.c -lm` | Noise removal vs. signal lag tradeoff |
| 5 | `05_cpp_template.cpp` | `g++ -std=c++17 -I.. -o cpp_demo 05_cpp_template.cpp -lm` | C++ template with float, int16_t, and double |

## Platform Reference (not compilable standalone)

These files show integration patterns for popular embedded platforms. Copy the relevant snippet into your project and adapt it.

| File | Platform | Pattern |
|------|----------|---------|
| `platform/stm32_hal_adc.c` | STM32 (HAL) | ADC interrupt callback |
| `platform/arduino_analog_filter.ino` | Arduino | `analogRead()` in `loop()` |
| `platform/esp_idf_adc_filter.c` | ESP32 (ESP-IDF) | FreeRTOS task with `adc_oneshot` |
