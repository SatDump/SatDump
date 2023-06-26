cmake_minimum_required(VERSION 3.0.0)
project(umkaDer_support)

file(GLOB_RECURSE umkaDer_support_CPPS *.cpp)
add_library(umkaDer_support SHARED ${umkaDer_support_CPPS})
target_link_libraries(umkaDer_support PUBLIC satdump_core)
target_include_directories(umkaDer_support PUBLIC src)

install(TARGETS umkaDer_support DESTINATION share/satdump/plugins)