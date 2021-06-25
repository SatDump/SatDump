cmake_minimum_required (VERSION 3.0.0)

file(GLOB_RECURSE LIBAIRSPY_CS *.c)

add_library(airspy STATIC ${LIBAIRSPY_CS})
target_include_directories(airspy PUBLIC .)
set_target_properties(airspy PROPERTIES COMPILE_DEFINITIONS BUILDER_STATIC_DEFINE)

target_link_libraries(airspy PUBLIC usb)