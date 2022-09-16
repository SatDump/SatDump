cmake_minimum_required(VERSION 3.0.0)
project(fengyun2_support)

set(SATDUMP_ROOT "/usr")

file(GLOB_RECURSE fengyun2_support_CPPS *.cpp)
add_library(fengyun2_support SHARED ${fengyun2_support_CPPS})
target_link_libraries(fengyun2_support PUBLIC satdump_core)
target_include_directories(fengyun2_support PUBLIC src ${SATDUMP_ROOT}/include/satdump)

install(TARGETS fengyun2_support DESTINATION share/satdump/plugins)