cmake_minimum_required(VERSION 3.0.0)
project(others_support)


file(GLOB_RECURSE others_support_CPPS *.cpp)
add_library(others_support SHARED ${others_support_CPPS})
target_link_libraries(others_support PUBLIC satdump_core)
target_include_directories(others_support PUBLIC src)

install(TARGETS others_support DESTINATION share/satdump/plugins)