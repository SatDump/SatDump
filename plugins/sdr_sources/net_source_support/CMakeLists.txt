cmake_minimum_required(VERSION 3.12)
project(net_source_support)

message("Building with Network Source support")
file(GLOB_RECURSE net_source_support_CPPS *.cpp)
add_library(net_source_support SHARED ${net_source_support_CPPS})
target_link_libraries(net_source_support PUBLIC satdump_core)
target_include_directories(net_source_support PUBLIC src)

if(MSVC)
    target_link_libraries(net_source_support PUBLIC ws2_32)
endif()

install(TARGETS net_source_support DESTINATION lib/satdump/plugins)
