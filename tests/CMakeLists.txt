cmake_minimum_required(VERSION 3.16)

# Use a system Catch2 if it is recent enough, otherwise fetch it
set(SATDUMP_CATCH2_VERSION 3.16.1)
find_package(Catch2 ${SATDUMP_CATCH2_VERSION} QUIET)

if(NOT Catch2_FOUND)
    message("Catch2 >= ${SATDUMP_CATCH2_VERSION} not found, fetching it")
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v${SATDUMP_CATCH2_VERSION}
        GIT_SHALLOW ON
    )
    FetchContent_MakeAvailable(Catch2)
    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
endif()

file(GLOB_RECURSE SatDump_tests_CPPS CONFIGURE_DEPENDS *.cpp)

add_executable(satdump_tests ${SatDump_tests_CPPS})
target_link_libraries(satdump_tests PRIVATE satdump_core Catch2::Catch2)

# Lets tests load files shipped in the repository (e.g. resources/)
target_compile_definitions(satdump_tests PRIVATE SATDUMP_TEST_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

# Register every TEST_CASE with CTest
include(Catch)
catch_discover_tests(satdump_tests)

# Code coverage
if(ENABLE_COVERAGE)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(FATAL_ERROR "ENABLE_COVERAGE is only supported with GCC and Clang")
    endif()

    message("Building with coverage instrumentation")

    # -O0 is added last so it overrides the build type's optimization level,
    # otherwise inlining and elided code skew the line counts.
    foreach(target satdump_core satdump_tests)
        target_compile_options(${target} PRIVATE --coverage -O0 -g)
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
                -j 8
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
