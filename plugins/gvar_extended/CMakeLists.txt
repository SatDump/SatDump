cmake_minimum_required (VERSION 3.0.0)
project(gvar_extended)

set(SATDUMP_ROOT "/usr")

file(GLOB_RECURSE gvar_extended_CPPS *.cpp)
add_library(gvar_extended SHARED ${gvar_extended_CPPS})
target_link_libraries(gvar_extended PUBLIC satdump_core)
target_include_directories(gvar_extended PUBLIC src ${SATDUMP_ROOT}/include/satdump)
