cmake_minimum_required(VERSION 3.12)
project(apid_sender)

file(GLOB_RECURSE apid_sender_CPPS *.cpp)
add_library(apid_sender SHARED ${apid_sender_CPPS})
target_link_libraries(apid_sender PUBLIC gobdump_core gobdump_interface)
target_include_directories(apid_sender PUBLIC
    ${CMAKE_SOURCE_DIR}/src-core
    ${CMAKE_SOURCE_DIR}/src-interface
)

install(TARGETS apid_sender DESTINATION ${CMAKE_INSTALL_LIBDIR}/gobdump/plugins)
