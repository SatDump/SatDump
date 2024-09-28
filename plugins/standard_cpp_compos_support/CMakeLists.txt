cmake_minimum_required(VERSION 3.12)
project(cpp_compos_support)


file(GLOB_RECURSE cpp_compos_support_CPPS *.cpp)
add_library(cpp_compos_support SHARED ${cpp_compos_support_CPPS})
target_link_libraries(cpp_compos_support PUBLIC satdump_core)
target_include_directories(cpp_compos_support PUBLIC .)

install(TARGETS cpp_compos_support DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins)