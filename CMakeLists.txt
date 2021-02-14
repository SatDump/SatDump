cmake_minimum_required (VERSION 3.0.0)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT MINGW AND NOT WIN32)
    set(CMAKE_CXX_FLAGS "-Wall -Wextra")
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()

project (SatDump)
file(GLOB_RECURSE SatDump_CPPS src/*.cpp src/viterbi_lib/metrics.c src/viterbi_lib/tab.c src/viterbi_lib/viterbi.c)

add_executable(satdump ${SatDump_CPPS})

target_include_directories(satdump PUBLIC src)

find_package(Threads REQUIRED)
target_link_libraries(satdump PUBLIC Threads::Threads)

# Spdlog
find_package(Spdlog REQUIRED)
target_include_directories(satdump PUBLIC ${SPDLOG_INCLUDE_DIR})
target_link_libraries(satdump PUBLIC Spdlog::Spdlog)

# LibDSP
find_library(DSP_LIBRARY dsp REQUIRED)
target_link_libraries(satdump PUBLIC ${DSP_LIBRARY})

target_link_libraries (satdump PUBLIC "correct")

if(WIN32 AND NOT MINGW)
    find_package(PNG CONFIG REQUIRED)
else()
    find_package(PNG REQUIRED)
endif()
target_link_libraries(satdump PUBLIC PNG::PNG)

find_library(CCSDS_LIBRARY ccsds REQUIRED)
target_link_libraries(satdump PUBLIC ${CCSDS_LIBRARY})