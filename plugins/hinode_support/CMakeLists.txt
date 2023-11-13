cmake_minimum_required(VERSION 3.0.0)
project(hinode_support)


file(GLOB_RECURSE hinode_support_CPPS *.cpp)
add_library(hinode_support SHARED ${hinode_support_CPPS})
target_link_libraries(hinode_support PUBLIC satdump_core)
target_include_directories(hinode_support PUBLIC src)

install(TARGETS hinode_support DESTINATION lib/satdump/plugins)