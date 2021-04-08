cmake_minimum_required (VERSION 3.0.0)
include(CheckLibraryExists)
include(CheckIncludeFiles)
include(CheckCSourceCompiles)
include(CMakePushCheckState)
include(CheckCCompilerFlag)

file(GLOB_RECURSE SatDump_core_CPPS  *.cpp 
                                     modules/common/aec/*.c
)

find_package(PkgConfig)

add_library(satdump_core SHARED ${SatDump_core_CPPS})
target_include_directories(satdump_core PUBLIC .)

# Threads
find_package(Threads REQUIRED)
target_link_libraries(satdump_core PUBLIC Threads::Threads)

if(MSVC)
    target_link_libraries(satdump_core PUBLIC dsp.dll)
    target_link_libraries(satdump_core PUBLIC correct.dll)
    target_link_libraries(satdump_core PUBLIC jpeg)
    target_link_libraries(satdump_core PUBLIC fmt.dll)
    target_link_libraries(satdump_core PUBLIC volk.dll)
    target_link_libraries(satdump_core PUBLIC libpng16.dll)
    target_link_libraries(satdump_core PUBLIC fftw3f.dll)
    target_link_libraries(satdump_core PUBLIC libavcodec.dll)
    target_link_libraries(satdump_core PUBLIC libavformat.dll)
    target_link_libraries(satdump_core PUBLIC libavutil.dll)
    target_link_libraries(satdump_core PUBLIC libavfilter.dll)
    #target_link_libraries(satdump_core PUBLIC stdc++fs)
else()
    # LibDSP
    find_library(DSP_LIBRARY dsp REQUIRED)
    target_link_libraries(satdump_core PUBLIC ${DSP_LIBRARY})

    # LibCorrect
    find_library(CORRECT_LIBRARY correct REQUIRED)
    target_link_libraries(satdump_core PUBLIC ${CORRECT_LIBRARY})

    # LibJpeg
    find_library(JPEG_LIBRARY jpeg REQUIRED)
    target_link_libraries(satdump_core PUBLIC ${JPEG_LIBRARY})

    # FMT
    find_package(fmt REQUIRED)
    target_include_directories(satdump_core PUBLIC ${fmt_INCLUDE_DIRS})
    target_link_libraries(satdump_core PUBLIC fmt::fmt)

    # VOLK
    pkg_check_modules(VOLK REQUIRED volk)
    target_include_directories(satdump_core PUBLIC ${volk_INCLUDE_DIRS})
    target_link_libraries(satdump_core PUBLIC volk)

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

    # LibPNG
    if(WIN32 AND NOT MINGW)
        find_package(PNG CONFIG REQUIRED)
    else()
        find_package(PNG REQUIRED)
    endif()
    target_link_libraries(satdump_core PUBLIC PNG::PNG)

    # FFTW3
    if(APPLE) 
        set(FFTW3f_DIR /usr/local/lib/cmake/fftw3)
        find_package(FFTW3f CONFIG REQUIRED)
        target_link_libraries(satdump_core PUBLIC FFTW3::fftw3f)
    else()  
        pkg_check_modules(FFTW3 REQUIRED fftw3f)
        target_link_libraries(satdump_core PUBLIC ${FFTW3_LIBRARIES})
        target_link_libraries(satdump_core PUBLIC stdc++fs)
    endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_DEBUG "-ggdb")
    
endif()
