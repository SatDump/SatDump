cmake_minimum_required (VERSION 3.0.0)

file(GLOB_RECURSE LIBAIRSPYHF_CS *.c)

add_library(airspyhf STATIC ${LIBAIRSPYHF_CS})
target_include_directories(airspyhf PUBLIC .)
set_target_properties(airspyhf PROPERTIES COMPILE_DEFINITIONS BUILDER_STATIC_DEFINE)

target_link_libraries(airspyhf PUBLIC usb)