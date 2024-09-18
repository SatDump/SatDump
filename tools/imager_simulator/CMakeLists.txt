cmake_minimum_required(VERSION 3.12)

file(GLOB_RECURSE SatDump_testing_CPPS *.cpp)

add_executable(imager_simulator ${SatDump_testing_CPPS})
target_include_directories(imager_simulator PUBLIC src-testing src-core)

# Link against the core
target_link_libraries(imager_simulator PUBLIC satdump_core)