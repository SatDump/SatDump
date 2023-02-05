cmake_minimum_required(VERSION 3.0.0)
project(aim_support)


file(GLOB_RECURSE aim_support_CPPS *.cpp)
add_library(aim_support SHARED ${aim_support_CPPS})
target_link_libraries(aim_support PUBLIC satdump_core)
target_include_directories(aim_support PUBLIC src)

install(TARGETS aim_support DESTINATION share/satdump/plugins)