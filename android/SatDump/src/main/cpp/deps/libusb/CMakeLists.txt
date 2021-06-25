cmake_minimum_required (VERSION 3.0.0)

file(GLOB_RECURSE LIBUSB_CS *.c)

add_library(usb STATIC ${LIBUSB_CS})
target_include_directories(usb PUBLIC .)
set_target_properties(usb PROPERTIES COMPILE_DEFINITIONS BUILDER_STATIC_DEFINE)
