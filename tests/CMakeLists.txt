cmake_minimum_required(VERSION 3.16)

# Use a system Catch2 if it is recent enough, otherwise fetch a current one
set(SATDUMP_CATCH2_MIN_VERSION 3.4.0)
set(SATDUMP_CATCH2_FETCH_VERSION 3.16.1)
find_package(Catch2 ${SATDUMP_CATCH2_MIN_VERSION} QUIET)

if(NOT Catch2_FOUND)
    message("Catch2 >= ${SATDUMP_CATCH2_MIN_VERSION} not found, fetching ${SATDUMP_CATCH2_FETCH_VERSION}")
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v${SATDUMP_CATCH2_FETCH_VERSION}
        GIT_SHALLOW ON
    )
    FetchContent_MakeAvailable(Catch2)
    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
endif()

# Tests mirror the source tree: the tests for <root>/<path> are in
# tests/<root>/<path>. Everything else under tests/ is shared by all of them.
set(SATDUMP_TESTED_ROOTS src-core src-cli)

file(GLOB_RECURSE SatDump_tests_CPPS CONFIGURE_DEPENDS *.cpp)

# The CLI is an executable, so its sources are compiled into the tests to be
# reachable from them. Its entry point (src-cli/main.cpp) is left out.
file(GLOB_RECURSE SatDump_tests_cli_CPPS CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src-cli/*.cpp)
list(REMOVE_ITEM SatDump_tests_cli_CPPS ${CMAKE_SOURCE_DIR}/src-cli/main.cpp)

add_executable(satdump_tests ${SatDump_tests_CPPS} ${SatDump_tests_cli_CPPS})
target_link_libraries(satdump_tests PRIVATE satdump_core Catch2::Catch2)

# Shared test helpers are included relative to tests/ (e.g. "helpers/temp_files.h")
target_include_directories(satdump_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Test helpers of a source root are included relative to tests/<root>/, like
# its headers are relative to <root>/ (e.g. "dsp/ndsp_test_helpers.h")
foreach(root ${SATDUMP_TESTED_ROOTS})
    target_include_directories(satdump_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/${root})
endforeach()

# src-cli headers are included relative to src-cli/ (e.g. "legacy/webserver.h")
target_include_directories(satdump_tests PRIVATE ${CMAKE_SOURCE_DIR}/src-cli)

# Lets tests load files shipped in the repository (e.g. resources/)
target_compile_definitions(satdump_tests PRIVATE SATDUMP_TEST_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

# Register every TEST_CASE with CTest. The timeout is well above the longest
# bounded wait in the test helpers, so a hung test fails instead of stalling
# the run for CTest's 25 minutes default.
#
# Catch2 exits with 4 when every test it ran was skipped. Older versions of
# catch_discover_tests do not tell CTest about it, which then reports a
# SKIP() as a failure.
include(Catch)
catch_discover_tests(satdump_tests PROPERTIES TIMEOUT 300 SKIP_RETURN_CODE 4)

# Code coverage
if(ENABLE_COVERAGE)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(FATAL_ERROR "ENABLE_COVERAGE is only supported with GCC and Clang")
    endif()

    message("Building with coverage instrumentation")

    # -O0 is added last so it overrides the build type's optimization level,
    # otherwise inlining and elided code skew the line counts.
    #
    # The counters are updated atomically where the compiler supports it : the
    # DSP blocks run in their own threads, and racing plain increments leave
    # inconsistent counts that gcov reports as negative hits, which gcovr
    # rejects.
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag(-fprofile-update=atomic COVERAGE_HAS_ATOMIC_PROFILE_UPDATE)
    set(COVERAGE_COMPILE_OPTIONS --coverage -O0 -g)
    if(COVERAGE_HAS_ATOMIC_PROFILE_UPDATE)
        list(APPEND COVERAGE_COMPILE_OPTIONS -fprofile-update=atomic)
    endif()

    foreach(target satdump_core satdump_tests)
        target_compile_options(${target} PRIVATE ${COVERAGE_COMPILE_OPTIONS})
        target_link_options(${target} PRIVATE --coverage)
    endforeach()

    # "coverage" target : run the tests, then generate a report with gcovr
    find_program(GCOVR_EXECUTABLE gcovr)

    if(GCOVR_EXECUTABLE)
        set(COVERAGE_GCOV_ARGS)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            find_program(LLVM_COV_EXECUTABLE llvm-cov REQUIRED)
            set(COVERAGE_GCOV_ARGS --gcov-executable "${LLVM_COV_EXECUTABLE} gcov")
        endif()

        set(COVERAGE_DIR ${CMAKE_BINARY_DIR}/coverage)

        # Unoptimized instrumented tests are slow, so run them in parallel
        include(ProcessorCount)
        ProcessorCount(COVERAGE_JOBS)
        if(COVERAGE_JOBS EQUAL 0)
            set(COVERAGE_JOBS 1)
        endif()

        add_custom_target(coverage
            COMMAND ${CMAKE_COMMAND} -E make_directory ${COVERAGE_DIR}
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -j ${COVERAGE_JOBS}
            COMMAND ${GCOVR_EXECUTABLE}
                --root ${CMAKE_SOURCE_DIR}
                --filter ${CMAKE_SOURCE_DIR}/src-core/
                --filter ${CMAKE_SOURCE_DIR}/src-cli/
                # Vendored third-party code
                --exclude ${CMAKE_SOURCE_DIR}/src-core/libs/
                --exclude ${CMAKE_SOURCE_DIR}/src-core/imgui/
                --exclude ${CMAKE_SOURCE_DIR}/src-core/nlohmann/
                --exclude ${CMAKE_SOURCE_DIR}/src-core/angelscript/
                ${COVERAGE_GCOV_ARGS}
                --delete # Reset the counters, so every run starts from zero
                --print-summary
                --xml ${COVERAGE_DIR}/coverage.xml
                --html-details ${COVERAGE_DIR}/index.html
                -j ${COVERAGE_JOBS}
                ${CMAKE_BINARY_DIR}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            DEPENDS satdump_tests
            USES_TERMINAL
            COMMENT "Running tests and generating coverage report in ${COVERAGE_DIR}"
        )
    else()
        message(WARNING "gcovr not found, the 'coverage' report target will not be available (pip install gcovr)")
    endif()
endif()
