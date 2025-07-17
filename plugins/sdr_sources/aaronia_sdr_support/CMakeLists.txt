cmake_minimum_required(VERSION 3.12)
project(aaronia_sdr_support)


if(MSVC)
    set(AARONIA_RTSA_SUITE_PATH "C:/Program Files/Aaronia AG/Aaronia RTSA-Suite PRO/" CACHE STRING "Path of RTSA Aaronia SW")
else()
    set(AARONIA_RTSA_SUITE_PATH "/opt/aaronia-rtsa-suite/Aaronia-RTSA-Suite-PRO" CACHE STRING "Path of RTSA Aaronia SW")
endif()

if(UNIX OR MSVC)
    message("Building with Aaronia support")
    file(GLOB_RECURSE aaronia_sdr_support_CPPS *.cpp)
    add_library(aaronia_sdr_support SHARED ${aaronia_sdr_support_CPPS})
    target_link_libraries(aaronia_sdr_support PUBLIC satdump_core)

    #target_include_directories(aaronia_sdr_support PUBLIC "${AARONIA_RTSA_SUITE_PATH}/sdk")

    if(MSVC)
        target_compile_definitions(aaronia_sdr_support PUBLIC RTSA_LIB_PATH="${AARONIA_RTSA_SUITE_PATH}/sdk/AaroniaRTSAAPI.dll")
        install(TARGETS aaronia_sdr_support DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins RUNTIME_DEPENDENCY_SET satdump_deps)
    else()
        target_compile_definitions(aaronia_sdr_support PUBLIC RTSA_LIB_PATH="${AARONIA_RTSA_SUITE_PATH}/libAaroniaRTSAAPI.so")
        install(TARGETS aaronia_sdr_support DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins)
    endif()

    target_include_directories(aaronia_sdr_support PUBLIC src)
else()
    message("Aaronia Library could not be found! Not building.")
endif()
