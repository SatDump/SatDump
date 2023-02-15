cmake_minimum_required(VERSION 3.0.0)
project(orbcomm_support)


file(GLOB_RECURSE orbcomm_support_CPPS *.cpp)
add_library(orbcomm_support SHARED ${orbcomm_support_CPPS})
target_link_libraries(orbcomm_support PUBLIC satdump_core)
target_include_directories(orbcomm_support PUBLIC src)

install(TARGETS orbcomm_support DESTINATION share/satdump/plugins)