cmake_minimum_required(VERSION 3.12)
project(transit_support)


file(GLOB_RECURSE transit_support_CPPS *.cpp)
add_library(transit_support SHARED ${transit_support_CPPS})
target_link_libraries(transit_support PUBLIC satdump_core)
target_include_directories(transit_support PUBLIC src)

install(TARGETS transit_support DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins)