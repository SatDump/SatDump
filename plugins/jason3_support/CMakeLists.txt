cmake_minimum_required(VERSION 3.0.0)
project(jason3_support)


file(GLOB_RECURSE jason3_support_CPPS *.cpp)
add_library(jason3_support SHARED ${jason3_support_CPPS})
target_link_libraries(jason3_support PUBLIC satdump_core)
target_include_directories(jason3_support PUBLIC src)

install(TARGETS jason3_support DESTINATION share/satdump/plugins)