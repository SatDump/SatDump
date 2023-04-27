cmake_minimum_required(VERSION 3.0.0)
project(mats_support)


file(GLOB_RECURSE mats_support_CPPS *.cpp jpeg12/*.c)
add_library(mats_support SHARED ${mats_support_CPPS})
target_link_libraries(mats_support PUBLIC satdump_core)
target_include_directories(mats_support PUBLIC src)

install(TARGETS mats_support DESTINATION share/satdump/plugins)