cmake_minimum_required (VERSION 3.0.0)

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
    endif()
endif()

install(TARGETS satdump-ui DESTINATION bin)