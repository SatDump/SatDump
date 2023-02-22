cmake_minimum_required(VERSION 3.0.0)

file(GLOB_RECURSE SatDump_testing_CPPS main.cpp libtom/*.c)

add_executable(satdump-testing ${SatDump_testing_CPPS})
target_include_directories(satdump-testing PUBLIC src-testing src-core)

# Link against the core
target_link_libraries(satdump-testing PUBLIC satdump_core)