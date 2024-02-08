cmake_minimum_required(VERSION 3.12)
project(landsat_support)

file(GLOB_RECURSE landsat_support_CPPS *.cpp)
add_library(landsat_support SHARED ${landsat_support_CPPS})
target_link_libraries(landsat_support PUBLIC satdump_core)
target_include_directories(landsat_support PUBLIC src)

install(TARGETS landsat_support DESTINATION lib/satdump/plugins)