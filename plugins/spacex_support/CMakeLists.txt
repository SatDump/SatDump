cmake_minimum_required(VERSION 3.0.0)
project(spacex_support)

set(SATDUMP_ROOT "/usr")

option(USE_VIDEO_ENCODER "Build the video encoder module, links against libav." OFF)

file(GLOB_RECURSE spacex_support_CPPS *.cpp)
add_library(spacex_support SHARED ${spacex_support_CPPS})
target_link_libraries(spacex_support PUBLIC satdump_core)
target_include_directories(spacex_support PUBLIC src ${SATDUMP_ROOT}/include/satdump)

if(MSVC)
    if(USE_VIDEO_ENCODER)
        target_link_libraries(satdump_core PUBLIC libavcodec.dll)
        target_link_libraries(satdump_core PUBLIC libavformat.dll)
        target_link_libraries(satdump_core PUBLIC libavutil.dll)
        target_link_libraries(satdump_core PUBLIC libavfilter.dll)

        # Enable it
        target_compile_definitions(satdump_core PUBLIC USE_VIDEO_ENCODER="1")
    endif()
elseif(ANDROID)
# NO encoder on Android
else()
    if(USE_VIDEO_ENCODER)
        # libavcodec
        find_library(AVCODEC_LIBRARY avcodec REQUIRED)
        target_link_libraries(satdump_core PUBLIC ${AVCODEC_LIBRARY})

        # libavformat
        find_library(AVFORMAT_LIBRARY avformat REQUIRED)
        target_link_libraries(satdump_core PUBLIC ${AVFORMAT_LIBRARY})

        # libavutil
        find_library(AVUTIL_LIBRARY avutil REQUIRED)
        target_link_libraries(satdump_core PUBLIC ${AVUTIL_LIBRARY})

        # libavfilter
        find_library(AVFILTER_LIBRARY avfilter REQUIRED)
        target_link_libraries(satdump_core PUBLIC ${AVFILTER_LIBRARY})

        # Enable it
        target_compile_definitions(satdump_core PUBLIC USE_VIDEO_ENCODER="1")
    endif()
endif()

install(TARGETS spacex_support DESTINATION share/satdump/plugins)