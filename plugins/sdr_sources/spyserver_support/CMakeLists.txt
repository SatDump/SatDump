cmake_minimum_required(VERSION 3.0.0)
project(spyserver_support)


message("Building with SpyServer support")
file(GLOB_RECURSE spyserver_support_CPPS *.cpp)
add_library(spyserver_support SHARED ${spyserver_support_CPPS})
target_link_libraries(spyserver_support PUBLIC satdump_core)
target_include_directories(spyserver_support PUBLIC src)

if(MSVC)
    target_link_libraries(spyserver_support PUBLIC ws2_32)
endif()

install(TARGETS spyserver_support DESTINATION share/satdump/plugins)