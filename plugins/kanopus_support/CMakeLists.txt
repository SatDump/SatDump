cmake_minimum_required(VERSION 3.12)
project(kanopus_support)


file(GLOB_RECURSE kanopus_support_CPPS *.cpp)
add_library(kanopus_support SHARED ${kanopus_support_CPPS})
target_link_libraries(kanopus_support PUBLIC satdump_core)
target_include_directories(kanopus_support PUBLIC src)

install(TARGETS kanopus_support DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins)