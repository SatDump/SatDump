cmake_minimum_required(VERSION 3.0.0)
project(noaa_metop_support)


file(GLOB_RECURSE noaa_metop_support_CPPS *.cpp instruments/mhs/*.cpp)
add_library(noaa_metop_support SHARED ${noaa_metop_support_CPPS})
target_link_libraries(noaa_metop_support PUBLIC satdump_core)
target_include_directories(noaa_metop_support PUBLIC src .)

install(TARGETS noaa_metop_support DESTINATION share/satdump/plugins)