cmake_minimum_required(VERSION 3.0.0)
project(aaudio_audio_sink)

if(ANDROID)
    message("Building with AAudio sink")
    file(GLOB_RECURSE aaudio_audio_sink *.cpp)
    add_library(aaudio_audio_sink SHARED ${aaudio_audio_sink})

    target_link_libraries(aaudio_audio_sink PUBLIC satdump_core aaudio)

    install(TARGETS aaudio_audio_sink DESTINATION share/satdump/plugins)
else()
    message("AAudio Library could not be found! Not building.")
endif()