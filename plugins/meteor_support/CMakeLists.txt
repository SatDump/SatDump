cmake_minimum_required(VERSION 3.12)
project(meteor_support)


file(GLOB_RECURSE meteor_support_CPPS *.cpp)
add_library(meteor_support SHARED ${meteor_support_CPPS})
target_link_libraries(meteor_support PUBLIC satdump_core)
target_include_directories(meteor_support PUBLIC src)

install(TARGETS meteor_support DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins)