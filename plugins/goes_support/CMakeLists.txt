cmake_minimum_required(VERSION 3.12)
project(goes_support)


file(GLOB_RECURSE goes_support_CPPS *.cpp openjp2/*.c)
add_library(goes_support SHARED ${goes_support_CPPS})
target_link_libraries(goes_support PUBLIC satdump_core xrit_support)
target_include_directories(goes_support PUBLIC src . ${CMAKE_SOURCE_DIR}/plugins/xrit_support)

install(TARGETS goes_support DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins)