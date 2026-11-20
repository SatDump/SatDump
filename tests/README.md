# Unit tests

Automated unit tests for `satdump_core` and the `satdump` CLI (`src-cli`), built on [Catch2](https://github.com/catchorg/Catch2) v3 and run through CTest.

Catch2 3.16.1 or newer is required. A system install is used if it is recent enough, otherwise Catch2 is fetched and built at configure time (this needs network access on the first configure).

## Running

```sh
cmake -B build -DBUILD_UNIT_TESTS=ON
cmake --build build --target satdump_tests -j$(nproc)
ctest --test-dir build --output-on-failure
```

Add `-DCMAKE_BUILD_TYPE=Release` to test the optimized code that is actually shipped; the suite is expected to pass both with and without optimization.

The executable can also be run directly, which allows filtering by name or tag:

```sh
./build/satdump_tests "[repack]"
./build/satdump_tests "~[!shouldfail]"   # Skip the known bugs, which are noisy
./build/satdump_tests --list-tests
```

Catch2 runs the test cases in a random order when the executable is run directly, so tests must not depend on each other. A failure that only shows up in a full run can be replayed with the `--rng-seed` printed at the top of that run.

The tests are not built by default and are not part of any CI workflow.

## Coverage

Coverage needs GCC or Clang, plus [gcovr](https://gcovr.com) for the report (`pip install gcovr` or your distribution's `gcovr` package). Use a separate build directory, as instrumented builds are unoptimized:

```sh
cmake -B build-coverage -DBUILD_UNIT_TESTS=ON -DENABLE_COVERAGE=ON
cmake --build build-coverage --target coverage -j$(nproc)
```

The `coverage` target runs the tests, prints a summary and writes an HTML report to `build-coverage/coverage/index.html` (and a Cobertura `coverage.xml` next to it). It covers `src-core`, excluding the vendored libraries, and `src-cli`.

## Adding tests

Any `.cpp` file under this directory is picked up automatically. Tests mirror the layout of `src-core`: tests for `src-core/utils/string.h` live in `tests/utils/test_string.cpp`. Tests for `src-cli` live in `tests/cli`, with the same layout: `src-cli/legacy/record.cpp` is tested by `tests/cli/legacy/test_record.cpp`. The sources of `src-cli` are compiled into the test executable, except `src-cli/main.cpp` (the entry point of `satdump`), which has no test. Where a source directory holds many small related files (most of `src-core/dsp`), one test file may cover the whole directory, e.g. `tests/dsp/pll/test_pll.cpp`.

## Conventions

Code is formatted with the repository's `.clang-format` (clang-format 19 or newer).

**Test cases**

- Every `TEST_CASE` has tags, and the first tag is the top-level directory (`[common]`, `[dsp]`, `[image]`...).
- Helpers local to one file go in an anonymous namespace, as every test file is linked into the same executable.
- Tests must pass in any order and leave no state behind. Global state (configuration, registries, working directory...) is changed through a guard that restores it, and files are only written to a `test_helpers::TempDir` / `TempFile`.
- Test data is deterministic: use `test_helpers::Lcg` rather than `rand()` or `std::random_device`.
- Blocking waits are bounded by a timeout, so a hang fails the test.
- Compare floating point values with the Catch2 matchers (`WithinAbs`, `WithinRel`) rather than a hand-written `fabs(a - b) < tol`, as they print both values on failure. `Catch::Approx` is still used by older tests.
- Do not assert once per sample over thousands of samples. Compute the worst case and assert on that.
- Use `M_PI`, like `src-core` does.

**Helpers**

- Helpers shared by the whole suite live in `tests/helpers/`, in the `test_helpers` namespace.
- Helpers for one area live next to its tests in `<area>_test_helpers.h`, in the `<area>_test` namespace (`dsp_test`, `ndsp_test`, `image_test`, `pipeline_test`...).
- Helper headers are included relative to `tests/`, like `src-core` headers are relative to `src-core/`: `#include "helpers/temp_files.h"`, `#include "dsp/ndsp_test_helpers.h"`. Do not use `../`.

**Known bugs**

- A test that documents a bug in `src-core` or `src-cli` is tagged `[!shouldfail]` and is preceded by a comment starting with `// Bug: <file>:<line>` that explains it. It starts failing (as "passed unexpectedly") once the bug is fixed; remove the tag then.
- If the bug only shows on some compilers, standard libraries or optimization levels, tag the test `[!mayfail]` instead and say so in the comment.
- Never exercise undefined behaviour or a crash from a test. Those problems are listed in [UNTESTED_ISSUES.md](UNTESTED_ISSUES.md) instead.

**Portability**

- Code that needs POSIX headers (sockets, `pthread`...) is wrapped in `#ifndef _WIN32`.
- Tests that need a data symbol of `satdump_core` that is not declared in a header are wrapped in `#ifndef _MSC_VER`, as such symbols are not exported from the Windows DLL.

This is unrelated to `BUILD_TESTING` (the `src-testing` scratch program) and `tools/test_satdump` (batch testing against large recordings).
