cmake_minimum_required(VERSION 3.12)
project(seawifs_support)


file(GLOB_RECURSE seawifs_support_CPPS *.cpp instruments/mhs/*.cpp)
add_library(seawifs_support SHARED ${seawifs_support_CPPS})
target_link_libraries(seawifs_support PUBLIC satdump_core)
target_include_directories(seawifs_support PUBLIC src .)

install(TARGETS seawifs_support DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins)