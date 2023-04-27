cmake_minimum_required(VERSION 3.0.0)
project(rtlsdr_sdr_support)


find_library(RTLSDR_LIBRARY rtlsdr)

if(RTLSDR_LIBRARY OR MSVC OR ANDROID)
    message("Building with RTL-SDR support")
    file(GLOB_RECURSE rtlsdr_sdr_support_CPPS *.cpp)
    add_library(rtlsdr_sdr_support SHARED ${rtlsdr_sdr_support_CPPS})

    if(MSVC)
        target_link_libraries(rtlsdr_sdr_support PUBLIC satdump_core rtlsdr.dll)
    elseif(ANDROID)
        target_link_libraries(rtlsdr_sdr_support PUBLIC satdump_core rtlsdr usb)
        target_include_directories(rtlsdr_sdr_support PUBLIC ../../../android/deps/librtlsdr)
    else()
        target_link_libraries(rtlsdr_sdr_support PUBLIC satdump_core ${RTLSDR_LIBRARY})
    endif()

    target_include_directories(rtlsdr_sdr_support PUBLIC src)

    install(TARGETS rtlsdr_sdr_support DESTINATION share/satdump/plugins)
else()
    message("RTL-SDR Library could not be found! Not building.")
endif()