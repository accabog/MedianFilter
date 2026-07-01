# GNU make predefines CC/CXX with `default` origin, so `?=` would be a no-op and
# the C recipes would silently run `cc`.  Override only the built-in default,
# while still honoring an environment or command-line CC/CXX (e.g. make CC=clang).
ifeq ($(origin CC),default)
CC := gcc
endif
ifeq ($(origin CXX),default)
CXX := g++
endif
CFLAGS  ?= -Wall -Wextra -Wpedantic -Werror -O2 -std=c11
CXXFLAGS?= -Wall -Wextra -Wpedantic -Werror -O2 -std=c++17

CFLAGS  += -I.
CXXFLAGS+= -I.

BUILDDIR = build

.PHONY: all test examples clean

all: test

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

test: $(BUILDDIR)/stress_test $(BUILDDIR)/stress_test_inline $(BUILDDIR)/init_test $(BUILDDIR)/circular_test $(BUILDDIR)/test_cpp
	$(BUILDDIR)/stress_test
	$(BUILDDIR)/stress_test_inline
	$(BUILDDIR)/init_test
	$(BUILDDIR)/circular_test
	$(BUILDDIR)/test_cpp

$(BUILDDIR)/stress_test: tests/stress_test.c tests/test_harness.h MedianFilter.c MedianFilter.h | $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ tests/stress_test.c MedianFilter.c

# Same stress test built against the opt-in inline Insert (MEDIANFILTER_INLINE_API),
# so the header's static-inline copy is actually compiled, warned-on, and tested.
# MedianFilter.c is linked for Init; its out-of-line Insert is #ifndef'd out here.
$(BUILDDIR)/stress_test_inline: tests/stress_test.c tests/test_harness.h MedianFilter.c MedianFilter.h | $(BUILDDIR)
	$(CC) $(CFLAGS) -DMEDIANFILTER_INLINE_API -o $@ tests/stress_test.c MedianFilter.c

$(BUILDDIR)/init_test: tests/init_test.c tests/test_harness.h MedianFilter.c MedianFilter.h | $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ tests/init_test.c MedianFilter.c

$(BUILDDIR)/circular_test: tests/circular_test.c tests/test_harness.h MedianFilter.c MedianFilter.h | $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ tests/circular_test.c MedianFilter.c

$(BUILDDIR)/test_cpp: tests/test_cpp.cpp tests/test_harness.h MedianFilter.hpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -o $@ tests/test_cpp.cpp

# ── Examples ──────────────────────────────────────────────────────

examples: $(BUILDDIR)/01_basic_usage $(BUILDDIR)/02_adc_noise_filter \
          $(BUILDDIR)/03_multichannel_sensors $(BUILDDIR)/04_window_size_comparison \
          $(BUILDDIR)/05_cpp_template

$(BUILDDIR)/0%: examples/0%.c MedianFilter.c MedianFilter.h | $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ $< MedianFilter.c -lm

$(BUILDDIR)/0%: examples/0%.cpp MedianFilter.hpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -o $@ $< -lm

clean:
	rm -rf $(BUILDDIR)
