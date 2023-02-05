cmake_minimum_required(VERSION 3.0.0)
project(rtltcp_support)


message("Building with RTL-TCP support")
file(GLOB_RECURSE rtltcp_support_CPPS *.cpp)
add_library(rtltcp_support SHARED ${rtltcp_support_CPPS})
target_link_libraries(rtltcp_support PUBLIC satdump_core)
target_include_directories(rtltcp_support PUBLIC src)

if(MSVC)
    target_link_libraries(rtltcp_support PUBLIC ws2_32)
endif()

install(TARGETS rtltcp_support DESTINATION share/satdump/plugins)