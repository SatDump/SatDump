cmake_minimum_required(VERSION 3.0.0)
project(jpss_support)


file(GLOB_RECURSE jpss_support_CPPS *.cpp)
add_library(jpss_support SHARED ${jpss_support_CPPS})
target_link_libraries(jpss_support PUBLIC satdump_core)
target_include_directories(jpss_support PUBLIC src)

install(TARGETS jpss_support DESTINATION share/satdump/plugins)