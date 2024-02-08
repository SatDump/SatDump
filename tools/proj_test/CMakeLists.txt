cmake_minimum_required(VERSION 3.12)

file(GLOB_RECURSE SatDump_testing_CPPS *.cpp)

add_executable(proj_test ${SatDump_testing_CPPS})
target_include_directories(proj_test PUBLIC src-testing src-core)

# Link against the core
target_link_libraries(proj_test PUBLIC satdump_core)