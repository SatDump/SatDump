cmake_minimum_required(VERSION 3.12)
project(wiptracking_app)

if(BUILD_GUI)
    file(GLOB_RECURSE wiptracking_app_CPPS *.cpp)
    add_library(wiptracking_app SHARED ${wiptracking_app_CPPS})
    target_link_libraries(wiptracking_app PUBLIC satdump_core satdump_interface)
    target_include_directories(wiptracking_app PUBLIC . ../../src-interface)

    install(TARGETS wiptracking_app DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins)
endif()
