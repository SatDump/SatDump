cmake_minimum_required(VERSION 3.12)
project(umka_support)

file(GLOB_RECURSE umka_support_CPPS *.cpp)
add_library(umka_support SHARED ${umka_support_CPPS})
target_link_libraries(umka_support PUBLIC satdump_core)
target_include_directories(umka_support PUBLIC src)

install(TARGETS umka_support DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins)