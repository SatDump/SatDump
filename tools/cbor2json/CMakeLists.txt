cmake_minimum_required(VERSION 3.12)

file(GLOB_RECURSE SatDump_testing_CPPS *.cpp)

add_executable(cbor2json ${SatDump_testing_CPPS})
target_include_directories(cbor2json PUBLIC src-testing src-core)

# Link against the core
target_link_libraries(cbor2json PUBLIC satdump_core)