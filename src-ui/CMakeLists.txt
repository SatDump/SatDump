cmake_minimum_required (VERSION 3.0.0)
include(CheckFunctionExists)

file(GLOB_RECURSE SatDump_ui_CPPS  *.cpp tinyfiledialogs/tinyfiledialogs.c)

add_executable(satdump-ui ${SatDump_ui_CPPS})
target_include_directories(satdump-ui PUBLIC . src-core)

# Set the resources path
if(MSVC)
    target_compile_definitions(satdump_core PUBLIC RESOURCES_PATH="./")
else()
    target_compile_definitions(satdump_core PUBLIC RESOURCES_PATH="${CMAKE_INSTALL_PREFIX}/share/satdump/")
endif()

if(BUILD_LIVE)
    add_compile_definitions(BUILD_LIVE="1")
endif()

# Link against the core
target_link_libraries(satdump-ui PUBLIC satdump_core)

# OpenGL
find_package(OpenGL REQUIRED)
target_include_directories(satdump-ui PUBLIC ${OPENGL_INCLUDE_DIR})
target_link_libraries(satdump-ui PUBLIC ${OPENGL_LIBRARIES})

if(MSVC)
    target_link_libraries(satdump-ui PRIVATE glew32.dll)
    target_link_libraries(satdump-ui PRIVATE glfw3dll.dll)
    if(BUILD_LIVE)
        target_link_libraries(satdump-ui PUBLIC airspy.dll)
        target_link_libraries(satdump-ui PUBLIC rtlsdr.dll)
        target_link_libraries(satdump-ui PUBLIC hackrf.dll)
        target_link_libraries(satdump-ui PUBLIC LimeSuite.dll)
    endif()
else()
    # Glew
    find_package(GLEW REQUIRED)
    target_link_libraries(satdump-ui PRIVATE GLEW::GLEW)

    find_package(OpenGL REQUIRED)
    target_include_directories(satdump-ui PUBLIC ${OPENGL_INCLUDE_DIR})
    target_link_libraries(satdump-ui PUBLIC ${OPENGL_LIBRARIES})

    # GLFW3
    find_package(glfw3 CONFIG REQUIRED)
    target_link_libraries(satdump-ui PRIVATE glfw)

    if(BUILD_LIVE)
        # Airspy
        find_library(AIRSPY_LIBRARY airspy REQUIRED)
        target_link_libraries(satdump-ui PUBLIC ${AIRSPY_LIBRARY})

        # RTL-SDR
        find_library(RTLSDR_LIBRARY rtlsdr REQUIRED)
        target_link_libraries(satdump-ui PUBLIC ${RTLSDR_LIBRARY})

        # HackRF
        find_library(HACKRF_LIBRARY hackrf REQUIRED)
        target_link_libraries(satdump-ui PUBLIC ${HACKRF_LIBRARY})

        # LimeSDR
        #find_library(LIME_LIBRARY LimeSuite REQUIRED)
        #target_link_libraries(satdump-ui PUBLIC ${LIME_LIBRARY})
    endif()
endif()

set(CMAKE_REQUIRED_LIBRARIES ${RTLSDR_LIBRARIES})
check_function_exists(rtlsdr_set_bias_tee HAS_RTLSDR_SET_BIAS_TEE)
if(BUILD_LIVE)
    add_compile_definitions(HAS_RTLSDR_SET_BIAS_TEE="1")
endif()
unset(CMAKE_REQUIRED_LIBRARIES)

install(TARGETS satdump-ui DESTINATION bin)