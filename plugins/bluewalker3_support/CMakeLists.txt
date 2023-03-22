cmake_minimum_required(VERSION 3.0.0)
project(bluewalker3_support)


file(GLOB_RECURSE bluewalker3_support_CPPS *.cpp)
add_library(bluewalker3_support SHARED ${bluewalker3_support_CPPS})
target_link_libraries(bluewalker3_support PUBLIC satdump_core)
target_include_directories(bluewalker3_support PUBLIC src)

install(TARGETS bluewalker3_support DESTINATION share/satdump/plugins)