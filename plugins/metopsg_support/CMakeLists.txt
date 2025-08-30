cmake_minimum_required(VERSION 3.12)
project(metopsg_support)


file(GLOB_RECURSE metopsg_support_CPPS *.cpp instruments/mhs/*.cpp)
add_library(metopsg_support SHARED ${metopsg_support_CPPS})
target_link_libraries(metopsg_support PUBLIC satdump_core)
target_include_directories(metopsg_support PUBLIC src .)

install(TARGETS metopsg_support DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins)