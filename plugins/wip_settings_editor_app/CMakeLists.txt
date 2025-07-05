cmake_minimum_required(VERSION 3.12)
project(wipsettings_app)

if(BUILD_GUI)

    file(GLOB_RECURSE wipsettings_app_CPPS *.cpp)
    add_library(wipsettings_app SHARED ${wipsettings_app_CPPS})

    target_link_libraries(wipsettings_app
        PUBLIC
            satdump_core
            satdump_interface
    )

    target_include_directories(wipsettings_app
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../src-interface>
        PRIVATE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../src-core/core>
    )

    install(TARGETS wipsettings_app
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins
    )
endif()
