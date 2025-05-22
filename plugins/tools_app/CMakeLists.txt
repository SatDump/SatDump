cmake_minimum_required(VERSION 3.12)
project(tools_app)

if(BUILD_GUI)
    file(GLOB_RECURSE tools_app_CPPS *.cpp)
    add_library(tools_app SHARED ${tools_app_CPPS})
    target_link_libraries(tools_app PUBLIC satdump_core satdump_interface)
    target_include_directories(tools_app PUBLIC . ../../src-interface)

    install(TARGETS tools_app DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins)
endif()
