cmake_minimum_required(VERSION 3.12)
project(gk2a_support)


file(GLOB_RECURSE gk2a_support_CPPS *.cpp libtom/*.c)
add_library(gk2a_support SHARED ${gk2a_support_CPPS})
target_link_libraries(gk2a_support PUBLIC satdump_core xrit_support)
target_include_directories(gk2a_support PUBLIC src . ${CMAKE_SOURCE_DIR}/plugins/xrit_support)

install(TARGETS gk2a_support DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins)