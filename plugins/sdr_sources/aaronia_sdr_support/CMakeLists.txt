cmake_minimum_required(VERSION 3.0.0)
project(aaronia_sdr_support)

set(SATDUMP_ROOT "/usr")

if(UNIX OR MSVC)
    message("Building with Aaronia support")
    file(GLOB_RECURSE aaronia_sdr_support_CPPS *.cpp)
    add_library(aaronia_sdr_support SHARED ${aaronia_sdr_support_CPPS})

    if(MSVC)
        target_link_directories(aaronia_sdr_support PUBLIC "C:/Program Files/Aaronia AG/Aaronia RTSA-Suite PRO/sdk")
        target_include_directories(aaronia_sdr_support PUBLIC "C:/Program Files/Aaronia AG/Aaronia RTSA-Suite PRO/sdk")
        target_link_libraries(aaronia_sdr_support PUBLIC AaroniaRTSAAPI.dll)
    else()
        target_link_libraries(aaronia_sdr_support PUBLIC satdump_core ~/Aaronia/RTSA/Aaronia-RTSA-Suite-PRO/libAaroniaRTSAAPI.so)
        target_include_directories(aaronia_sdr_support PUBLIC ~/Aaronia/RTSA/Aaronia-RTSA-Suite-PRO)
    endif()

    target_include_directories(aaronia_sdr_support PUBLIC src ${SATDUMP_ROOT}/include/satdump)

    install(TARGETS aaronia_sdr_support DESTINATION share/satdump/plugins)
else()
    message("Aaronia Library could not be found! Not building.")
endif()