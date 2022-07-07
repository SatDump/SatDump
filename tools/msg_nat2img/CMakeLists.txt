cmake_minimum_required (VERSION 3.0.0)

file(GLOB_RECURSE SatDump_testing_CPPS  *.cpp)

add_executable(msg_nat2img ${SatDump_testing_CPPS})
target_include_directories(msg_nat2img PUBLIC src-testing src-core)

# Link against the core
target_link_libraries (msg_nat2img PUBLIC satdump_core)