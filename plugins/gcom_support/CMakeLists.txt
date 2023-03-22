cmake_minimum_required(VERSION 3.0.0)
project(gcom_support)


file(GLOB_RECURSE gcom_support_CPPS *.cpp)
add_library(gcom_support SHARED ${gcom_support_CPPS})
target_link_libraries(gcom_support PUBLIC satdump_core)
target_include_directories(gcom_support PUBLIC src)

install(TARGETS gcom_support DESTINATION share/satdump/plugins)