cmake_minimum_required (VERSION 3.0.0)

file(GLOB_RECURSE SatDump_core_CPPS  *.cpp 
                                     modules/common/viterbi_lib/metrics.c 
                                     modules/common/viterbi_lib/tab.c 
                                     modules/common/viterbi_lib/viterbi.c 
                                     modules/common/aec/*.c
)

add_library(satdump_core SHARED ${SatDump_core_CPPS})
target_include_directories(satdump_core PUBLIC .)

# Threads
find_package(Threads REQUIRED)
target_link_libraries(satdump_core PUBLIC Threads::Threads)

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

# LibPNG
if(WIN32 AND NOT MINGW)
    find_package(PNG CONFIG REQUIRED)
else()
    find_package(PNG REQUIRED)
endif()
target_link_libraries(satdump_core PUBLIC PNG::PNG)