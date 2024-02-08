cmake_minimum_required(VERSION 3.12)
project(udp_source_support)

message("Building with UDP Source support")
file(GLOB_RECURSE udp_source_support_CPPS *.cpp)
add_library(udp_source_support SHARED ${udp_source_support_CPPS})
target_link_libraries(udp_source_support PUBLIC satdump_core)
target_include_directories(udp_source_support PUBLIC src)

if(MSVC)
    target_link_libraries(udp_source_support PUBLIC ws2_32)
endif()

install(TARGETS udp_source_support DESTINATION lib/satdump/plugins)
