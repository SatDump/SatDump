cmake_minimum_required(VERSION 3.0.0)
project(rtaudio_audio_sink)

find_library(RTAUDIO_LIBRARY rtaudio)

if(RTAUDIO_LIBRARY) # OR MSVC)
    message("Building with RtAudio sink")
    file(GLOB_RECURSE rtaudio_audio_sink *.cpp)
    add_library(rtaudio_audio_sink SHARED ${rtaudio_audio_sink})

    if(MSVC)
        target_link_libraries(rtaudio_audio_sink PUBLIC satdump_core rtaudio.dll)
    else()
        target_link_libraries(rtaudio_audio_sink PUBLIC satdump_core ${RTAUDIO_LIBRARY})
    endif()

    install(TARGETS rtaudio_audio_sink DESTINATION share/satdump/plugins)
else()
    message("RtAudio Library could not be found! Not building.")
endif()