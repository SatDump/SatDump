cmake_minimum_required (VERSION 3.0.0)

file(GLOB_RECURSE SatDump_ui_CPPS  *.cpp tinyfiledialogs/tinyfiledialogs.c)

add_executable(satdump-ui ${SatDump_ui_CPPS})
target_include_directories(satdump-ui PUBLIC src-ui src-core)

# Link against the core
target_link_libraries(satdump-ui PUBLIC satdump_core)

# OpenGL
find_package(OpenGL REQUIRED)
target_include_directories(satdump-ui PUBLIC ${OPENGL_INCLUDE_DIR})
target_link_libraries(satdump-ui PUBLIC ${OPENGL_LIBRARIES})

if(MSVC)
    target_link_libraries(satdump-ui PRIVATE glew32.dll)
    target_link_libraries(satdump-ui PRIVATE glfw3dll.dll)
    #target_link_libraries(satdump-ui PUBLIC airspy.dll)
    target_link_libraries(satdump-ui PUBLIC fftw3.dll)
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

    # Airspy
    #find_library(AIRSPY_LIBRARY airspy REQUIRED)
    #target_link_libraries(satdump-ui PUBLIC ${AIRSPY_LIBRARY})

    # FFTW3
    find_library(FFT_LIBRARY fftw3 REQUIRED)
    target_link_libraries(satdump-ui PUBLIC ${FFT_LIBRARY})
endif()