cmake_minimum_required(VERSION 3.12)
project(ssdv_support)

file(GLOB_RECURSE ssdv_support_CPPS *.cpp)
add_library(ssdv_support SHARED ${ssdv_support_CPPS})
target_link_libraries(ssdv_support PUBLIC gobdump_core gobdump_interface)
target_include_directories(ssdv_support PUBLIC src-core src-interface)

install(TARGETS ssdv_support DESTINATION ${CMAKE_INSTALL_LIBDIR}/gobdump/plugins)
