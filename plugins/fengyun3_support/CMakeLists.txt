cmake_minimum_required (VERSION 3.0.0)
project(fengyun3_support)

set(SATDUMP_ROOT "/usr")

file(GLOB_RECURSE fengyun3_support_CPPS *.cpp)
add_library(fengyun3_support SHARED ${fengyun3_support_CPPS})
target_link_libraries(fengyun3_support PUBLIC satdump_core)
target_include_directories(fengyun3_support PUBLIC src ${SATDUMP_ROOT}/include/satdump)
