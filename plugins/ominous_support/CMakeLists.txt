cmake_minimum_required(VERSION 3.12)
project(ominous_support)

file(GLOB_RECURSE ominous_support_CPPS *.cpp)
add_library(ominous_support SHARED ${ominous_support_CPPS})
target_link_libraries(ominous_support PUBLIC gobdump_core gobdump_interface)
target_include_directories(ominous_support PUBLIC . ../../src-interface)

install(TARGETS ominous_support
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/gobdump/plugins)
