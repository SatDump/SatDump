cmake_minimum_required(VERSION 3.12)
project(scripting_support)


file(GLOB_RECURSE scripting_support_CPPS *.cpp)
add_library(scripting_support SHARED ${scripting_support_CPPS})
target_link_libraries(scripting_support PUBLIC satdump_core)
target_include_directories(scripting_support PUBLIC src)

install(TARGETS scripting_support DESTINATION lib/satdump/plugins)