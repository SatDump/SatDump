cmake_minimum_required(VERSION 3.12)
project(soapy_sdr_support)

find_library(SOAPYSDR_LIBRARY SoapySDR)

if(SOAPYSDR_LIBRARY)
    message("Building with SoapySDR support")
    file(GLOB_RECURSE soapy_sdr_support_CPPS *.cpp)
    add_library(soapy_sdr_support SHARED ${soapy_sdr_support_CPPS})

    # if(MSVC)
    # target_link_libraries(soapy_sdr_support PUBLIC satdump_core rtlsdr.dll)
    # else()
    target_link_libraries(soapy_sdr_support PUBLIC satdump_core ${SOAPYSDR_LIBRARY})

    # endif()
    target_include_directories(soapy_sdr_support PUBLIC src)

    install(TARGETS soapy_sdr_support DESTINATION lib/satdump/plugins)
else()
    message("SoapySDR Library could not be found! Not building.")
endif()