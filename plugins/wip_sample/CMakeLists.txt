cmake_minimum_required(VERSION 3.12)
project(wip_sample_app)

if(BUILD_GUI)
    file(GLOB_RECURSE wip_sample_app_CPPS *.cpp)
    add_library(wip_sample_app SHARED ${wip_sample_app_CPPS})
    target_link_libraries(wip_sample_app PUBLIC satdump_core satdump_interface)
    target_include_directories(wip_sample_app PUBLIC . ../../src-interface)

    install(TARGETS wip_sample_app DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins)
endif()
