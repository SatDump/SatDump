cmake_minimum_required(VERSION 3.12)
project(insat_support)


file(GLOB_RECURSE insat_support_CPPS *.cpp)
add_library(insat_support SHARED ${insat_support_CPPS})
target_link_libraries(insat_support PUBLIC satdump_core)
target_include_directories(insat_support PUBLIC src)

install(TARGETS insat_support DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins)