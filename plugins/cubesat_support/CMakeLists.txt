cmake_minimum_required(VERSION 3.0.0)
project(cubesat_support)


file(GLOB_RECURSE cubesat_support_CPPS *.cpp instruments/mhs/*.cpp)
add_library(cubesat_support SHARED ${cubesat_support_CPPS})
target_link_libraries(cubesat_support PUBLIC satdump_core)
target_include_directories(cubesat_support PUBLIC src .)

install(TARGETS cubesat_support DESTINATION share/satdump/plugins)