cmake_minimum_required (VERSION 3.0.0)
project(hackrf_sdr_support)

set(SATDUMP_ROOT "/usr")

find_library(HACKRF_LIBRARY hackrf)

if(HACKRF_LIBRARY OR MSVC)
    message("Building with HackRF support")
    file(GLOB_RECURSE hackrf_sdr_support_CPPS *.cpp)
    add_library(hackrf_sdr_support SHARED ${hackrf_sdr_support_CPPS})

    if(MSVC)
        target_link_libraries(hackrf_sdr_support PUBLIC hackrf.dll)
    else()
        target_link_libraries(hackrf_sdr_support PUBLIC satdump_core ${HACKRF_LIBRARY})
    endif()

    target_include_directories(hackrf_sdr_support PUBLIC src ${SATDUMP_ROOT}/include/satdump)
else()
    message("HackRF Library could not be found! Not building.")
endif()