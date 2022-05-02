cmake_minimum_required (VERSION 3.0.0)
project(airspy_sdr_support)

set(SATDUMP_ROOT "/usr")

file(GLOB_RECURSE airspy_sdr_support_CPPS *.cpp)
add_library(airspy_sdr_support SHARED ${airspy_sdr_support_CPPS})
target_link_libraries(airspy_sdr_support PUBLIC satdump_core airspy)
target_include_directories(airspy_sdr_support PUBLIC src ${SATDUMP_ROOT}/include/satdump)
