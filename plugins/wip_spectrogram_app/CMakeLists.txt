cmake_minimum_required(VERSION 3.12)
project(wipspectrogram_app)

if(BUILD_GUI)
    file(GLOB_RECURSE wipspectrogram_app_CPPS *.cpp)
    add_library(wipspectrogram_app SHARED ${wipspectrogram_app_CPPS})
    target_link_libraries(wipspectrogram_app PUBLIC satdump_core satdump_interface)
    target_include_directories(wipspectrogram_app PUBLIC . ../../src-interface)

    install(TARGETS wipspectrogram_app DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins)
endif()
