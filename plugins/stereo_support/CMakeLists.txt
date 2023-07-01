cmake_minimum_required(VERSION 3.0.0)
project(stereo_support)


file(GLOB_RECURSE stereo_support_CPPS *.cpp)
add_library(stereo_support SHARED ${stereo_support_CPPS})
target_link_libraries(stereo_support PUBLIC satdump_core)
target_include_directories(stereo_support PUBLIC src)

install(TARGETS stereo_support DESTINATION share/satdump/plugins)