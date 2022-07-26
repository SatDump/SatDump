cmake_minimum_required (VERSION 3.0.0)
project(spacex_support)

set(SATDUMP_ROOT "/usr")

option(USE_VIDEO_ENCODER    "Build the video encoder module, links against libav."      OFF)

file(GLOB_RECURSE spacex_support_CPPS *.cpp)
add_library(spacex_support SHARED ${spacex_support_CPPS})
target_link_libraries(spacex_support PUBLIC satdump_core)
target_include_directories(spacex_support PUBLIC src ${SATDUMP_ROOT}/include/satdump)

install(TARGETS spacex_support DESTINATION share/satdump/plugins)