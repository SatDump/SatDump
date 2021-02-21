cmake_minimum_required (VERSION 3.0.0)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    include_directories(/usr/local/include)
    include_directories(/usr/local/opt/jpeg-turbo/include)
    link_directories(/usr/local/lib)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
endif()

if(NOT MINGW AND NOT WIN32)
    set(CMAKE_CXX_FLAGS "-Wall -Wextra")
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()

project (SatDump)

# Core
file(GLOB_RECURSE SatDump_core_CPPS  src-core/*.cpp 
                                     src-core/modules/common/viterbi_lib/metrics.c 
                                     src-core/modules/common/viterbi_lib/tab.c 
                                     src-core/modules/common/viterbi_lib/viterbi.c 
                                     src-core/modules/common/aec/*.c
                                     src-core/pipe.c
)

add_library(satdump_core SHARED ${SatDump_core_CPPS})

target_include_directories(satdump_core PUBLIC src-core)

find_package(Threads REQUIRED)
target_link_libraries(satdump_core PUBLIC Threads::Threads)

# Spdlog
find_package(Spdlog REQUIRED)
target_include_directories(satdump_core PUBLIC ${SPDLOG_INCLUDE_DIR})
target_link_libraries(satdump_core PUBLIC Spdlog::Spdlog)

# LibDSP
find_library(DSP_LIBRARY dsp REQUIRED)
target_link_libraries(satdump_core PUBLIC ${DSP_LIBRARY})

# Other crap
target_link_libraries (satdump_core PUBLIC correct jpeg)

# FMT
find_package(fmt REQUIRED)
target_include_directories(satdump_core PUBLIC ${fmt_INCLUDE_DIRS})
target_link_libraries(satdump_core PUBLIC fmt::fmt)

if(WIN32 AND NOT MINGW)
    find_package(PNG CONFIG REQUIRED)
else()
    find_package(PNG REQUIRED)
endif()
target_link_libraries(satdump_core PUBLIC PNG::PNG)




# CLI
file(GLOB_RECURSE SatDump_cli_CPPS  src-cli/*.cpp)

add_executable(satdump ${SatDump_cli_CPPS})

target_include_directories(satdump_core PUBLIC src-cli src-core)

target_link_libraries (satdump PUBLIC satdump_core)


# GUI
file(GLOB_RECURSE SatDump_ui_CPPS  src-ui/*.cpp)

add_executable(satdump-ui ${SatDump_ui_CPPS})

target_include_directories(satdump_core PUBLIC src-ui src-core)

target_link_libraries(satdump-ui PUBLIC satdump_core)

# Glew
find_package(GLEW REQUIRED)
target_link_libraries(satdump-ui PRIVATE GLEW::GLEW)

find_package(OpenGL REQUIRED)
target_include_directories(satdump-ui PUBLIC ${OPENGL_INCLUDE_DIR})
target_link_libraries(satdump-ui PUBLIC ${OPENGL_LIBRARIES})

# GLFW3
find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(satdump-ui PRIVATE glfw)

target_link_libraries(satdump-ui PRIVATE airspy fftw3)