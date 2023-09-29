cmake_minimum_required(VERSION 3.0.0)

if(MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:windows /ENTRY:mainCRTStartup")
    enable_language("RC")
    set (WIN32_RESOURCES ../windows/resource.rc)
endif()

file(GLOB_RECURSE SatDump_ui_CPPS *.cpp)

add_executable(satdump-ui ${SatDump_ui_CPPS} ${WIN32_RESOURCES})
target_include_directories(satdump-ui PUBLIC . ../src-core ../src-interface ../libsdr)

# Link against the core
target_link_libraries(satdump-ui PUBLIC satdump_core satdump_interface)

# OpenGL
set(OpenGL_GL_PREFERENCE LEGACY)
find_package(OpenGL REQUIRED)

# Tell FindOpengl to use modern GL lib system
set(OpenGL_GL_PREFERENCE GLVND)
target_include_directories(satdump-ui PUBLIC ${OPENGL_INCLUDE_DIR})
target_link_libraries(satdump-ui PUBLIC ${OPENGL_LIBRARIES})

if(MSVC)
    target_link_libraries(satdump-ui PRIVATE glew32.dll)
    target_link_libraries(satdump-ui PRIVATE glfw3dll.dll)
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
endif()

install(TARGETS satdump-ui DESTINATION bin)
