cmake_minimum_required(VERSION 3.12)
project(geonetcast_support)

file(GLOB_RECURSE geonetcast_support_CPPS *.cpp)
add_library(geonetcast_support SHARED ${geonetcast_support_CPPS})
target_link_libraries(geonetcast_support PUBLIC satdump_core)
target_include_directories(geonetcast_support PUBLIC src .)

if(MSVC OR BUILD_MSVC)
  install(
    TARGETS geonetcast_support
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins
    RUNTIME_DEPENDENCY_SET satdump_deps)
else()
  install(TARGETS geonetcast_support DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins)
endif()
