cmake_minimum_required(VERSION 3.12)
project(earthcare_support)

file(GLOB_RECURSE earthcare_support_CPPS *.cpp)
add_library(earthcare_support SHARED ${earthcare_support_CPPS})
target_link_libraries(earthcare_support PUBLIC satdump_core)
target_include_directories(earthcare_support PUBLIC src)

install(TARGETS earthcare_support DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins)