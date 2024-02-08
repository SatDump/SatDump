cmake_minimum_required(VERSION 3.12)

file(GLOB_RECURSE SatDump_testing_CPPS *.cpp)

add_executable(msugs_correct ${SatDump_testing_CPPS})
target_include_directories(msugs_correct PUBLIC src-testing src-core)

# Link against the core
target_link_libraries(msugs_correct PUBLIC satdump_core)