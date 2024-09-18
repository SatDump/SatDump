cmake_minimum_required(VERSION 3.12)
project(fengyun2_support)


file(GLOB_RECURSE fengyun2_support_CPPS *.cpp)
add_library(fengyun2_support SHARED ${fengyun2_support_CPPS})
target_link_libraries(fengyun2_support PUBLIC satdump_core)
target_include_directories(fengyun2_support PUBLIC src)

install(TARGETS fengyun2_support DESTINATION lib/satdump/plugins)