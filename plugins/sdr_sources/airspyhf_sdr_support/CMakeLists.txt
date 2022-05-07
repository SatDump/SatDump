cmake_minimum_required (VERSION 3.0.0)
project(airspyhf_sdr_support)

set(SATDUMP_ROOT "/usr")

find_library(AIRSPYHF_LIBRARY airspyhf)

if(AIRSPYHF_LIBRARY OR MSVC)
    message("Building with AirspyHF support")
    file(GLOB_RECURSE airspyhf_sdr_support_CPPS *.cpp)
    add_library(airspyhf_sdr_support SHARED ${airspyhf_sdr_support_CPPS})

    if(MSVC)
        target_link_libraries(airspyhf_sdr_support PUBLIC satdump_core airspyhf.dll)
    else()
        target_link_libraries(airspyhf_sdr_support PUBLIC satdump_core ${AIRSPYHF_LIBRARY})
    endif()

    target_include_directories(airspyhf_sdr_support PUBLIC src ${SATDUMP_ROOT}/include/satdump)
else()
    message("AirspyHF Library could not be found! Not building.")
endif()