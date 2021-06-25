cmake_minimum_required (VERSION 3.0.0)

file(GLOB_RECURSE LIBRTLSDR_CS *.c)

add_library(rtlsdr STATIC ${LIBRTLSDR_CS})
target_include_directories(rtlsdr PUBLIC .)
set_target_properties(rtlsdr PROPERTIES COMPILE_DEFINITIONS BUILDER_STATIC_DEFINE)

target_link_libraries(rtlsdr PUBLIC usb)