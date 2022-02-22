cmake_minimum_required (VERSION 3.0.0)
project(gk2a_support)

set(SATDUMP_ROOT "/usr")

file(GLOB_RECURSE gk2a_support_CPPS *.cpp libtom/*.c)
add_library(gk2a_support SHARED ${gk2a_support_CPPS})
target_link_libraries(gk2a_support PUBLIC satdump_core)
target_include_directories(gk2a_support PUBLIC src ${SATDUMP_ROOT}/include/satdump .)
