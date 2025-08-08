cmake_minimum_required(VERSION 3.12)
project(himawari_support)


file(GLOB_RECURSE himawari_support_CPPS *.cpp)
add_library(himawari_support SHARED ${himawari_support_CPPS})
target_link_libraries(himawari_support PUBLIC satdump_core xrit_support)
target_include_directories(himawari_support PUBLIC src . ${CMAKE_SOURCE_DIR}/plugins/xrit_support)

install(TARGETS himawari_support DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins)