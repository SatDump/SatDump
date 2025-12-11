cmake_minimum_required(VERSION 3.12)
project(bitview_app)


file(GLOB_RECURSE bitview_app_CPPS *.cpp)
add_library(bitview_app SHARED ${bitview_app_CPPS})
target_link_libraries(bitview_app PUBLIC satdump_core)
target_include_directories(bitview_app PUBLIC .)

install(TARGETS bitview_app DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins)