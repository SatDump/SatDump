cmake_minimum_required(VERSION 3.12)
project(aaronia_sdr_support)


if(MSVC)
    set(AARONIA_RTSA_SUITE_PATH "C:/Program Files/Aaronia AG/Aaronia RTSA-Suite PRO/" CACHE STRING "Path of RTSA Aaronia SW")
else()
    set(AARONIA_RTSA_SUITE_PATH "~/Aaronia/RTSA/Aaronia-RTSA-Suite-PRO" CACHE STRING "Path of RTSA Aaronia SW")
endif()

if(UNIX OR MSVC)
    message("Building with Aaronia support")
    file(GLOB_RECURSE aaronia_sdr_support_CPPS *.cpp)
    add_library(aaronia_sdr_support SHARED ${aaronia_sdr_support_CPPS})

    if(MSVC)
        target_link_directories(aaronia_sdr_support PUBLIC "${AARONIA_RTSA_SUITE_PATH}/sdk")
        target_include_directories(aaronia_sdr_support PUBLIC "${AARONIA_RTSA_SUITE_PATH}/sdk")
        target_link_libraries(aaronia_sdr_support PUBLIC satdump_core AaroniaRTSAAPI.dll)
    else()
        target_link_libraries(aaronia_sdr_support PUBLIC satdump_core ${AARONIA_RTSA_SUITE_PATH}/libAaroniaRTSAAPI.so)
        target_include_directories(aaronia_sdr_support PUBLIC ${AARONIA_RTSA_SUITE_PATH}/sdk)
    endif()

    target_include_directories(aaronia_sdr_support PUBLIC src)

    install(TARGETS aaronia_sdr_support DESTINATION lib/satdump/plugins)
else()
    message("Aaronia Library could not be found! Not building.")
endif()
