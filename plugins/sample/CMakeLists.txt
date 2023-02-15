cmake_minimum_required(VERSION 3.0.0)
project(sample)


file(GLOB_RECURSE sample_CPPS *.cpp)
add_library(sample SHARED ${sample_CPPS})
target_link_libraries(sample PUBLIC satdump_core)
target_include_directories(sample PUBLIC src)

install(TARGETS sample DESTINATION share/satdump/plugins)