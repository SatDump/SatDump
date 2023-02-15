cmake_minimum_required(VERSION 3.0.0)
project(dmsp_support)


file(GLOB_RECURSE dmsp_support_CPPS *.cpp instruments/mhs/*.cpp)
add_library(dmsp_support SHARED ${dmsp_support_CPPS})
target_link_libraries(dmsp_support PUBLIC satdump_core)
target_include_directories(dmsp_support PUBLIC src .)

install(TARGETS dmsp_support DESTINATION share/satdump/plugins)