cmake_minimum_required(VERSION 3.0.0)
project(hackrf_sdr_support)


find_library(HACKRF_LIBRARY hackrf)

if(HACKRF_LIBRARY OR MSVC OR ANDROID)
    message("Building with HackRF support")
    file(GLOB_RECURSE hackrf_sdr_support_CPPS *.cpp)
    add_library(hackrf_sdr_support SHARED ${hackrf_sdr_support_CPPS})

    if(MSVC)
        target_link_libraries(hackrf_sdr_support PUBLIC satdump_core hackrf.dll)
    elseif(ANDROID)
        target_link_libraries(hackrf_sdr_support PUBLIC satdump_core hackrf usb)
        target_include_directories(hackrf_sdr_support PUBLIC ../../../android/deps/libhackrf)
    else()
        target_link_libraries(hackrf_sdr_support PUBLIC satdump_core ${HACKRF_LIBRARY})
    endif()

    target_include_directories(hackrf_sdr_support PUBLIC src)

    install(TARGETS hackrf_sdr_support DESTINATION share/satdump/plugins)
else()
    message("HackRF Library could not be found! Not building.")
endif()