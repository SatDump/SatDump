cmake_minimum_required(VERSION 3.12)

file(GLOB_RECURSE SatDump_testing_CPPS *.cpp)

add_executable(stereo_merger ${SatDump_testing_CPPS})
target_include_directories(stereo_merger PUBLIC src-testing src-core)

# Link against the core
target_link_libraries(stereo_merger PUBLIC satdump_core)