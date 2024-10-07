cmake_minimum_required(VERSION 3.12)
project(meteosat_support)

file(GLOB_RECURSE meteosat_support_CPPS *.cpp)
add_library(meteosat_support SHARED ${meteosat_support_CPPS})
target_link_libraries(meteosat_support PUBLIC satdump_core)
target_include_directories(meteosat_support PUBLIC src)

install(TARGETS meteosat_support DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins)
