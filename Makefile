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

# Configuration variants (see MedianFilter.h / MedianFilter.hpp): the inline API
# with and without forced inlining, the value chain for every window
# (SMALL_MAX=0), networks for windows of 3 and 5 (SMALL_MAX=5), the opt-in search
# from the previous sample, and everything together through the inline API.
CFG_inline      = -DMEDIANFILTER_INLINE_API
CFG_force       = -DMEDIANFILTER_INLINE_API -DMEDIANFILTER_FORCE_INLINE
CFG_small0      = -DMEDIANFILTER_SMALL_MAX=0
CFG_small5      = -DMEDIANFILTER_SMALL_MAX=5
CFG_search      = -DMEDIANFILTER_SEARCH_FROM_PREVIOUS
CFG_all         = -DMEDIANFILTER_INLINE_API -DMEDIANFILTER_FORCE_INLINE \
                  -DMEDIANFILTER_SMALL_MAX=5 -DMEDIANFILTER_SEARCH_FROM_PREVIOUS
CPPCFG_force    = -DMEDIANFILTER_FORCE_INLINE
CPPCFG_search   = -DMEDIANFILTER_SEARCH_FROM_PREVIOUS
CPPCFG_search21 = -DMEDIANFILTER_SEARCH_FROM_PREVIOUS -DMEDIANFILTER_SEARCH_FROM_PREVIOUS_MIN_N=21

C_VARIANTS   = inline force small0 small5 search all
CPP_VARIANTS = force search search21

TESTS = $(BUILDDIR)/stress_test $(BUILDDIR)/init_test \
        $(BUILDDIR)/circular_test $(BUILDDIR)/circular_test_small0 \
        $(BUILDDIR)/exhaustive_test \
        $(foreach v,$(C_VARIANTS),$(BUILDDIR)/stress_test_$(v) $(BUILDDIR)/exhaustive_test_$(v)) \
        $(BUILDDIR)/test_cpp \
        $(foreach v,$(CPP_VARIANTS),$(BUILDDIR)/test_cpp_$(v))

test: $(TESTS) $(BUILDDIR)/gnu89_header_test.o
	@for t in $(TESTS); do echo "$$t"; $$t || exit 1; done

$(BUILDDIR)/stress_test: tests/stress_test.c tests/test_harness.h MedianFilter.c MedianFilter.h | $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ tests/stress_test.c MedianFilter.c

# The same tests built per configuration.  With CFG_inline the header's
# static-inline copy of Insert is compiled, warned-on, and tested; MedianFilter.c
# is still linked for Init (its out-of-line Insert is #ifndef'd out then).
$(BUILDDIR)/stress_test_%: tests/stress_test.c tests/test_harness.h MedianFilter.c MedianFilter.h | $(BUILDDIR)
	$(CC) $(CFLAGS) $(CFG_$*) -o $@ tests/stress_test.c MedianFilter.c

# Every reachable filter state for windows of 3, 5 and 7 (see the file header).
$(BUILDDIR)/exhaustive_test: tests/exhaustive_test.c tests/test_harness.h MedianFilter.c MedianFilter.h | $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ tests/exhaustive_test.c MedianFilter.c

$(BUILDDIR)/exhaustive_test_%: tests/exhaustive_test.c tests/test_harness.h MedianFilter.c MedianFilter.h | $(BUILDDIR)
	$(CC) $(CFLAGS) $(CFG_$*) -o $@ tests/exhaustive_test.c MedianFilter.c

$(BUILDDIR)/init_test: tests/init_test.c tests/test_harness.h MedianFilter.c MedianFilter.h | $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ tests/init_test.c MedianFilter.c

$(BUILDDIR)/circular_test: tests/circular_test.c tests/test_harness.h MedianFilter.c MedianFilter.h | $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ tests/circular_test.c MedianFilter.c

$(BUILDDIR)/circular_test_%: tests/circular_test.c tests/test_harness.h MedianFilter.c MedianFilter.h | $(BUILDDIR)
	$(CC) $(CFLAGS) $(CFG_$*) -o $@ tests/circular_test.c MedianFilter.c

# MedianFilter.h must stay usable from C90 code with GNU extensions (compile only).
$(BUILDDIR)/gnu89_header_test.o: tests/gnu89_header_test.c MedianFilter.h | $(BUILDDIR)
	$(CC) -std=gnu89 -Wall -Wextra -Werror -I. -c -o $@ tests/gnu89_header_test.c

$(BUILDDIR)/test_cpp: tests/test_cpp.cpp tests/test_harness.h MedianFilter.hpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -o $@ tests/test_cpp.cpp

$(BUILDDIR)/test_cpp_%: tests/test_cpp.cpp tests/test_harness.h MedianFilter.hpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) $(CPPCFG_$*) -o $@ tests/test_cpp.cpp

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
