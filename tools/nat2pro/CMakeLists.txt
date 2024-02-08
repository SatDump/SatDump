cmake_minimum_required(VERSION 3.12)

file(GLOB_RECURSE SatDump_testing_CPPS *.cpp)

add_executable(nat2pro ${SatDump_testing_CPPS})
target_include_directories(nat2pro PUBLIC src-testing src-core)

# Link against the core
target_link_libraries(nat2pro PUBLIC satdump_core)