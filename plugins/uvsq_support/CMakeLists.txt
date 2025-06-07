cmake_minimum_required(VERSION 3.12)
project(uvsq_support)


file(GLOB_RECURSE uvsq_support_CPPS *.cpp)
add_library(uvsq_support SHARED ${uvsq_support_CPPS})
target_link_libraries(uvsq_support PUBLIC satdump_core)
target_include_directories(uvsq_support PUBLIC src)

install(TARGETS uvsq_support DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins)