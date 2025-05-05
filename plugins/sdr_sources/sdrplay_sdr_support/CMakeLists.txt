cmake_minimum_required(VERSION 3.12)
project(sdrplay_sdr_support)


if(APPLE)
    include_directories("/usr/local/include")
    set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} /usr/local/lib)
endif()

find_library(SDRPLAY_LIBRARY sdrplay_api)

if(SDRPLAY_LIBRARY OR MSVC)
    message("Building with SDRPlay support")
    file(GLOB_RECURSE sdrplay_sdr_support_CPPS *.cpp)
    add_library(sdrplay_sdr_support SHARED ${sdrplay_sdr_support_CPPS})
    target_include_directories(sdrplay_sdr_support PUBLIC src)

    if(MSVC)
        target_link_libraries(sdrplay_sdr_support PUBLIC satdump_core sdrplay_api.dll)
        install(TARGETS sdrplay_sdr_support DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins RUNTIME_DEPENDENCY_SET satdump_deps)
    else()
        target_link_libraries(sdrplay_sdr_support PUBLIC satdump_core ${SDRPLAY_LIBRARY})
        install(TARGETS sdrplay_sdr_support DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins)
    endif()
else()
    message("SDRlay Library could not be found! Not building.")
endif()