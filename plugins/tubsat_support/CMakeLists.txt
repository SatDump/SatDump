cmake_minimum_required(VERSION 3.0.0)
project(tubsat_support)


file(GLOB_RECURSE tubsat_support_CPPS *.cpp)
add_library(tubsat_support SHARED ${tubsat_support_CPPS})
target_link_libraries(tubsat_support PUBLIC satdump_core)
target_include_directories(tubsat_support PUBLIC src)

install(TARGETS tubsat_support DESTINATION share/satdump/plugins)