# Contributing to MedianFilter

Thank you for your interest in contributing to MedianFilter. This guide covers
everything you need to get a pull request accepted.

## Getting Started

1. Fork the repository and clone your fork.
2. Create a feature branch from `master`.
3. Make your changes.
4. Run the tests and verify they pass.
5. Push to your fork and open a pull request.

## Building & Testing

```bash
make test    # build and run all tests
make clean   # remove build artifacts
```

The CI pipeline tests across:

- **Compilers:** GCC, Clang, MSVC
- **C standards:** C99, C11
- **C++ standards:** C++11, C++14, C++17
- **Sanitizers:** AddressSanitizer + UndefinedBehaviorSanitizer
- **Static analysis:** cppcheck + SonarCloud

All CI checks must pass before a PR is merged.

## Coding Style

- C functions use the `MEDIANFILTER_` prefix with PascalCase (e.g., `MEDIANFILTER_Init`).
- C structs use an `s` prefix and `_t` suffix (e.g., `sMedianNode_t`).
- C++ member variables use the `m_` prefix (e.g., `m_medianHead`).
- No dynamic memory allocation — the library targets bare-metal and RTOS environments.
- All C headers must include `extern "C"` guards.

## Submitting Changes

- Open your PR against `master`.
- Describe **what** changed and **why**.
- Keep each PR to one logical change.
- Reference related issues (e.g., "Fixes #12").
- All CI checks must be green.

## What Makes a Good Contribution

- Bug fixes with a test or reproducer.
- Platform ports and integration examples.
- Performance improvements backed by benchmark data.
- Documentation improvements and typo fixes.
- New test cases.
