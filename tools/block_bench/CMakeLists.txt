cmake_minimum_required(VERSION 3.12)

file(GLOB_RECURSE SatDump_testing_CPPS *.cpp)

add_executable(block_bench ${SatDump_testing_CPPS})
target_include_directories(block_bench PUBLIC src-testing src-core)

# Link against the core
target_link_libraries(block_bench PUBLIC satdump_core)