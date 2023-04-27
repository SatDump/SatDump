cmake_minimum_required(VERSION 3.0.0)
project(analog_support)


file(GLOB_RECURSE analog_support_CPPS *.cpp)
add_library(analog_support SHARED ${analog_support_CPPS})
target_link_libraries(analog_support PUBLIC satdump_core)
target_include_directories(analog_support PUBLIC src)

install(TARGETS analog_support DESTINATION share/satdump/plugins)