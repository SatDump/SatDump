cmake_minimum_required (VERSION 3.0.0)
project(sdrplay_sdr_support)

set(SATDUMP_ROOT "/usr")

find_library(SDRPLAY_LIBRARY sdrplay_api)

if(SDRPLAY_LIBRARY)
    message("Building with SDRPlay support")
    file(GLOB_RECURSE sdrplay_sdr_support_CPPS *.cpp)
    add_library(sdrplay_sdr_support SHARED ${sdrplay_sdr_support_CPPS})
    target_link_libraries(sdrplay_sdr_support PUBLIC satdump_core ${SDRPLAY_LIBRARY})
    target_include_directories(sdrplay_sdr_support PUBLIC src ${SATDUMP_ROOT}/include/satdump)
else()
    message("SDRlay Library could not be found! Not building.")
endif()