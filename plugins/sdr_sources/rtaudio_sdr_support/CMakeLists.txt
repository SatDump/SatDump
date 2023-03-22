cmake_minimum_required(VERSION 3.0.0)
project(rtaudio_sdr_support)


find_library(RTAUDIO_LIBRARY rtaudio)

if(RTAUDIO_LIBRARY) # OR MSVC) # OR ANDROID)
    message("Building with RtAudio support")
    file(GLOB_RECURSE rtaudio_sdr_support_CPPS *.cpp)
    add_library(rtaudio_sdr_support SHARED ${rtaudio_sdr_support_CPPS})

    if(MSVC)
        target_link_libraries(rtaudio_sdr_support PUBLIC satdump_core rtlsdr.dll)
    # elseif(ANDROID)
    # target_link_libraries(rtaudio_sdr_support PUBLIC satdump_core rtlsdr usb)
    # target_include_directories(rtaudio_sdr_support PUBLIC ../../../android/deps/librtlsdr)
    else()
        target_link_libraries(rtaudio_sdr_support PUBLIC satdump_core ${RTAUDIO_LIBRARY})
    endif()

    target_include_directories(rtaudio_sdr_support PUBLIC src)

    install(TARGETS rtaudio_sdr_support DESTINATION share/satdump/plugins)
else()
    message("RtAudio Library could not be found! Not building.")
endif()