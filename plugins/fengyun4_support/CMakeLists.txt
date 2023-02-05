cmake_minimum_required(VERSION 3.0.0)
project(fengyun4_support)


file(GLOB_RECURSE fengyun4_support_CPPS *.cpp openjp2/*.c)
add_library(fengyun4_support SHARED ${fengyun4_support_CPPS})
target_link_libraries(fengyun4_support PUBLIC satdump_core)
target_include_directories(fengyun4_support PUBLIC src .)

install(TARGETS fengyun4_support DESTINATION share/satdump/plugins)