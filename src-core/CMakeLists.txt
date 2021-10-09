cmake_minimum_required (VERSION 3.0.0)
include(CheckLibraryExists)
include(CheckIncludeFile)
include(CheckCSourceCompiles)
include(CMakePushCheckState)
include(CheckCCompilerFlag)

file(GLOB_RECURSE SatDump_core_CPPS  *.cpp 
                                     common/aec/*.c
                                     common/predict/*.c
                                     common/bzlib/*.c
                                     common/miniz/*.c
                                     common/libtom/*.c
                                     common/jpeg/*.c
                                     common/dlfcn/dlfcn.c
                                     common/map/maidenhead.c
                                     common/correct/*.c
)

find_package(PkgConfig)

add_library(satdump_core SHARED ${SatDump_core_CPPS})
target_include_directories(satdump_core PUBLIC .)

# Set the resources path
if(MSVC)
    target_compile_definitions(satdump_core PUBLIC RESOURCES_PATH="./")
else()
    target_compile_definitions(satdump_core PUBLIC RESOURCES_PATH="${CMAKE_INSTALL_PREFIX}/share/satdump/")
endif()

# Threads
find_package(Threads REQUIRED)
target_link_libraries(satdump_core PUBLIC Threads::Threads)

# Set Spdlog to build the library
target_compile_definitions(satdump_core PUBLIC SPDLOG_COMPILED_LIB="1")

if(MSVC)
    #target_link_libraries(satdump_core PUBLIC correct.dll)
    target_link_libraries(satdump_core PUBLIC jpeg)
    target_link_libraries(satdump_core PUBLIC volk.dll)
    target_link_libraries(satdump_core PUBLIC libpng16.dll)
    target_link_libraries(satdump_core PUBLIC fftw3f.dll)
    target_link_libraries(satdump_core PUBLIC nng.dll)

    if(USE_VIDEO_ENCODER)
        target_link_libraries(satdump_core PUBLIC libavcodec.dll)
        target_link_libraries(satdump_core PUBLIC libavformat.dll)
        target_link_libraries(satdump_core PUBLIC libavutil.dll)
        target_link_libraries(satdump_core PUBLIC libavfilter.dll)
        
        # Enable it
        target_compile_definitions(satdump_core PUBLIC USE_VIDEO_ENCODER="1")
    endif()

    if(BUILD_ZSTD)
        target_link_libraries(satdump_core PUBLIC zstd.dll)
        target_compile_definitions(satdump_core PUBLIC ENABLE_DECOMPRESSION="1")
    endif()

    #target_link_libraries(satdump_core PUBLIC stdc++fs)
elseif(ANDROID)
    # LibCorrect
    #target_link_libraries(satdump_core PUBLIC correct)

    # LibJpeg
    target_link_libraries(satdump_core PUBLIC jpeg)

    # VOLK
    target_link_libraries(satdump_core PUBLIC volk cpufeatures m log dl)

    # LibPNG
    target_link_libraries(satdump_core PUBLIC png zlib)

    # FFTW3
    target_link_libraries(satdump_core PUBLIC fftw)

    # NNG
    #target_link_libraries(satdump_core PUBLIC nng)
else()
    # LibCorrect
    #find_library(CORRECT_LIBRARY correct REQUIRED)
    #target_link_libraries(satdump_core PUBLIC ${CORRECT_LIBRARY})

    # LibJpeg
    find_library(JPEG_LIBRARY jpeg REQUIRED)
    target_link_libraries(satdump_core PUBLIC ${JPEG_LIBRARY})

    # VOLK
    pkg_check_modules(VOLK REQUIRED volk)
    set(CMAKE_REQUIRED_INCLUDES "/usr/include;/usr/local/includes")
    check_include_file("volk/volk_alloc.h" VOLK_HAS_ALLOC)
    if(NOT VOLK_HAS_ALLOC)
        message(STATUS "VOLK does not have volk_alloc.hh, including volk_includes")
        target_include_directories(satdump_core PUBLIC ../volk_includes) # If we're running VOLK 1.0, include our own volk_alloc
    endif()
    target_include_directories(satdump_core PUBLIC ${volk_INCLUDE_DIRS})
    target_link_libraries(satdump_core PUBLIC volk)

    # LibPNG
    if(WIN32 AND NOT MINGW)
        find_package(PNG CONFIG REQUIRED)
    else()
        find_package(PNG REQUIRED)
    endif()
    target_link_libraries(satdump_core PUBLIC PNG::PNG)

    # FFTW3
    if(APPLE) 
        set(FFTW3f_DIR /usr/local/lib/cmake/fftw3f)
        find_package(FFTW3f CONFIG REQUIRED)
        target_link_libraries(satdump_core PUBLIC FFTW3::fftw3f)
    else()  
        pkg_check_modules(FFTW3 REQUIRED fftw3f)
        target_link_libraries(satdump_core PUBLIC ${FFTW3_LIBRARIES})
        target_link_libraries(satdump_core PUBLIC stdc++fs atomic)
    endif()

    # NNG
    find_library(NNG_LIBRARY nng REQUIRED)
    target_link_libraries(satdump_core PUBLIC ${NNG_LIBRARY})
    
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

    if(BUILD_ZSTD)
         # zstd
         find_library(ZSTD_LIBRARY zstd REQUIRED)
         target_link_libraries(satdump_core PUBLIC ${ZSTD_LIBRARY}) 
        target_compile_definitions(satdump_core PUBLIC ENABLE_DECOMPRESSION="1")
    endif()

    # Dynamic linking
    target_link_libraries(satdump_core PUBLIC dl)
endif()

install(TARGETS satdump_core DESTINATION lib)
install(DIRECTORY . DESTINATION include/satdump FILES_MATCHING PATTERN "*.h")
install(DIRECTORY . DESTINATION include/satdump FILES_MATCHING PATTERN "*.hpp")