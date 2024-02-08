cmake_minimum_required(VERSION 3.12)

file(GLOB_RECURSE SatDump_testing_CPPS *.cpp)

add_executable(test_satdump ${SatDump_testing_CPPS})
target_include_directories(test_satdump PUBLIC src-testing src-core)

# Link against the core
target_link_libraries(test_satdump PUBLIC satdump_core)