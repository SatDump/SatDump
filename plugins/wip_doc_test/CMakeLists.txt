cmake_minimum_required(VERSION 3.12)
project(wipdocktest_app)

if(BUILD_GUI)
    file(GLOB_RECURSE wipdocktest_app_CPPS *.cpp)
    add_library(wipdocktest_app SHARED ${wipdocktest_app_CPPS})
    target_link_libraries(wipdocktest_app PUBLIC satdump_core /home/alan/Downloads/litehtml/build/litehtml-tests-build/litehtml/liblitehtml.so)
    target_include_directories(wipdocktest_app PUBLIC . ../../src-interface /home/alan/Downloads/litehtml/include)

    install(TARGETS wipdocktest_app DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins)
endif()
