cmake_minimum_required (VERSION 3.0.0)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_compile_definitions(SATDUMP_VERSION="0.0.36")

option(NOGUI                "Disable building the GUI"                                  OFF)
option(BUILD_TESTING        "Build the testing program"                                 OFF)
option(USE_VIDEO_ENCODER    "Build the video encoder module, links against libav."      OFF)
option(BUILD_LIVE           "Build the live processing feature (WIP)"                   OFF)
option(BUILD_ZSTD           "Build support for decompressing i8 basebands with zstd"    OFF)
option(BUILD_MSVC           "Build for Windows with MSVC"                               OFF) # Seems like "MSVC" as a macro messed up below for some reason...

if(NOGUI)
    set(BUILD_LIVE OFF)
endif()

if(APPLE)
    include_directories(/usr/local/include)
    include_directories(/usr/local/opt/jpeg-turbo/include)
    include_directories(/usr/local/Cellar/fftw/3.3.9/include)
    link_directories(/usr/local/lib)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
endif()

if(UNIX)
    set(CMAKE_CXX_FLAGS "-Wall -Wextra")
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
else()
    #set(CMAKE_CXX_FLAGS "-Wall")
    #set(CMAKE_CXX_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2")
endif()

if(MSVC OR BUILD_MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP8") # Speed up this to object-level
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP8") # Speed up this to object-level
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    include_directories(vcpkg/installed/x64-windows/include)
    link_directories(vcpkg/installed/x64-windows/lib)
endif()

project (SatDump)

add_subdirectory(src-core)
add_subdirectory(src-cli)

if(BUILD_LIVE)
    add_subdirectory(libsdr)
endif()

if(NOT NOGUI)
    add_subdirectory(src-interface)
    add_subdirectory(src-ui)
endif()

if(BUILD_LIVE)
    add_subdirectory(src-ingestor)
endif()

if(BUILD_TESTING)
    add_subdirectory(src-testing)
endif()

install(DIRECTORY ${CMAKE_SOURCE_DIR}/pipelines DESTINATION share/satdump)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/resources DESTINATION share/satdump)
install(FILES ${CMAKE_SOURCE_DIR}/Roboto-Medium.ttf DESTINATION share/satdump)

# Create uninstall target
configure_file(${CMAKE_SOURCE_DIR}/cmake/cmake_uninstall.cmake ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake @ONLY)
add_custom_target(uninstall ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
