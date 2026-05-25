cmake_minimum_required(VERSION 3.12)
project(webhook_app)


file(GLOB_RECURSE webhook_app_CPPS *.cpp)
add_library(webhook_app SHARED ${webhook_app_CPPS})
target_link_libraries(webhook_app PUBLIC gobdump_core gobdump_interface)
target_include_directories(webhook_app PUBLIC . ../../src-interface)

install(TARGETS webhook_app DESTINATION ${CMAKE_INSTALL_LIBDIR}/gobdump/plugins)
