cmake_minimum_required(VERSION 3.12)
project(bochum_support)

file(GLOB_RECURSE bochum_support_CPPS *.cpp)
add_library(bochum_support SHARED ${bochum_support_CPPS})
target_link_libraries(bochum_support PUBLIC satdump_core satdump_interface)
target_include_directories(bochum_support PUBLIC src-core src-interface)

install(TARGETS bochum_support DESTINATION lib/satdump/plugins)