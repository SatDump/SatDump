cmake_minimum_required(VERSION 3.12)
project(fengyun4_support)


file(GLOB_RECURSE fengyun4_support_CPPS *.cpp openjp2/*.c)
add_library(fengyun4_support SHARED ${fengyun4_support_CPPS})
target_link_libraries(fengyun4_support PUBLIC satdump_core xrit_support)
target_include_directories(fengyun4_support PUBLIC src . ${CMAKE_SOURCE_DIR}/plugins/xrit_support)

install(TARGETS fengyun4_support DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins)