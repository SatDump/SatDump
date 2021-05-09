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

# Set the resources path
if(MSVC)
    target_compile_definitions(satdump_core PUBLIC RESOURCES_PATH="./")
else()
    target_compile_definitions(satdump_core PUBLIC RESOURCES_PATH="${CMAKE_INSTALL_PREFIX}/share/satdump/")
endif()

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
    #target_link_libraries(satdump_core PUBLIC stdc++fs)
elseif(ANDROID)
    # LibCorrect
    target_link_libraries(satdump_core PUBLIC correct)

    # LibJpeg
    target_link_libraries(satdump_core PUBLIC jpeg)

    # VOLK
    target_link_libraries(satdump_core PUBLIC volk cpufeatures m log)

    # LibPNG
    target_link_libraries(satdump_core PUBLIC png zlib)

    # FFTW3
    # Currently not mandatory
else()
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

    # FMT
    pkg_check_modules(VOLK REQUIRED volk)
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
        set(FFTW3f_DIR /usr/local/lib/cmake/fftw3)
        find_package(FFTW3f CONFIG REQUIRED)
        target_link_libraries(satdump_core PUBLIC FFTW3::fftw3f)
    else()  
        pkg_check_modules(FFTW3 REQUIRED fftw3f)
        target_link_libraries(satdump_core PUBLIC ${FFTW3_LIBRARIES})
        target_link_libraries(satdump_core PUBLIC stdc++fs)
    endif()
endif()

install(TARGETS satdump_core DESTINATION lib)