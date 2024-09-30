cmake_minimum_required(VERSION 3.12)
project(wsf_support)


file(GLOB_RECURSE wsf_support_CPPS *.cpp)
add_library(wsf_support SHARED ${wsf_support_CPPS})
target_link_libraries(wsf_support PUBLIC satdump_core)
target_include_directories(wsf_support PUBLIC src)

install(TARGETS wsf_support DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins)