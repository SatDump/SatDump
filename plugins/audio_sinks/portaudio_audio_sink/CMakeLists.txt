cmake_minimum_required(VERSION 3.0.0)
project(portaudio_audio_sink)

find_library(PORTAUDIO_LIBRARY portaudio)

if(PORTAUDIO_LIBRARY OR MSVC)
    message("Building with RtAudio sink")
    file(GLOB_RECURSE portaudio_audio_sink *.cpp)
    add_library(portaudio_audio_sink SHARED ${portaudio_audio_sink})

    if(MSVC)
        target_link_libraries(portaudio_audio_sink PUBLIC satdump_core portaudio.dll)
    else()
        target_link_libraries(portaudio_audio_sink PUBLIC satdump_core ${PORTAUDIO_LIBRARY})
    endif()

    install(TARGETS portaudio_audio_sink DESTINATION share/satdump/plugins)
else()
    message("RtAudio Library could not be found! Not building.")
endif()