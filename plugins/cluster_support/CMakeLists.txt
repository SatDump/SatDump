cmake_minimum_required(VERSION 3.12)
project(cluster_support)

file(GLOB_RECURSE cluster_support_CPPS *.cpp)
add_library(cluster_support SHARED ${cluster_support_CPPS})
target_link_libraries(cluster_support PUBLIC satdump_core)
target_include_directories(cluster_support PUBLIC src)

install(TARGETS cluster_support DESTINATION lib/satdump/plugins)