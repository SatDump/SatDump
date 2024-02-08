cmake_minimum_required(VERSION 3.12)
project(eos_support)


file(GLOB_RECURSE eos_support_CPPS *.cpp)
add_library(eos_support SHARED ${eos_support_CPPS})
target_link_libraries(eos_support PUBLIC satdump_core)
target_include_directories(eos_support PUBLIC src)

install(TARGETS eos_support DESTINATION lib/satdump/plugins)