# Unit tests

Automated unit tests for `satdump_core`, built on [Catch2](https://github.com/catchorg/Catch2) v3 and run through CTest.

Catch2 3.16.1 or newer is required. A system install is used if it is recent enough, otherwise Catch2 is fetched and built at configure time (this needs network access on the first configure).

## Running

```sh
cmake -B build -DBUILD_UNIT_TESTS=ON
cmake --build build --target satdump_tests -j$(nproc)
ctest --test-dir build --output-on-failure
```

The executable can also be run directly, which allows filtering by name or tag:

```sh
./build/satdump_tests "[repack]"
./build/satdump_tests --list-tests
```

## Coverage

Coverage needs GCC or Clang, plus [gcovr](https://gcovr.com) for the report (`pip install gcovr` or your distribution's `gcovr` package). Use a separate build directory, as instrumented builds are unoptimized:

```sh
cmake -B build-coverage -DBUILD_UNIT_TESTS=ON -DENABLE_COVERAGE=ON
cmake --build build-coverage --target coverage -j$(nproc)
```

The `coverage` target runs the tests, prints a summary and writes an HTML report to `build-coverage/coverage/index.html` (and a Cobertura `coverage.xml` next to it). It covers `src-core`, excluding the vendored libraries.

## Adding tests

Any `.cpp` file under this directory is picked up automatically. Tests mirror the layout of `src-core`, so tests for `src-core/utils/string.h` live in `tests/utils/test_string.cpp`.

This is unrelated to `BUILD_TESTING` (the `src-testing` scratch program) and `tools/test_satdump` (batch testing against large recordings).
