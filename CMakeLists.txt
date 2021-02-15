cmake_minimum_required (VERSION 3.0.0)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    include_directories(/usr/local/include)
    link_directories(/usr/local/lib)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
endif()

if(NOT MINGW AND NOT WIN32)
    set(CMAKE_CXX_FLAGS "-Wall -Wextra")
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()

project (SatDump)
file(GLOB_RECURSE SatDump_CPPS src/*.cpp src/modules/common/viterbi_lib/metrics.c src/modules/common/viterbi_lib/tab.c src/modules/common/viterbi_lib/viterbi.c)

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

target_link_libraries (satdump PUBLIC correct aec jpeg)

# FMT
find_package(fmt REQUIRED)
target_include_directories(satdump PUBLIC ${fmt_INCLUDE_DIRS})
target_link_libraries(satdump PUBLIC fmt::fmt)

if(WIN32 AND NOT MINGW)
    find_package(PNG CONFIG REQUIRED)
else()
    find_package(PNG REQUIRED)
endif()
target_link_libraries(satdump PUBLIC PNG::PNG)

find_library(CCSDS_LIBRARY ccsds REQUIRED)
target_link_libraries(satdump PUBLIC ${CCSDS_LIBRARY})