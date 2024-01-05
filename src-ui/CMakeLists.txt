cmake_minimum_required(VERSION 3.0.0)

if(MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:windows /ENTRY:mainCRTStartup")
    enable_language("RC")
    set (WIN32_RESOURCES ../windows/resource.rc)
endif()

file(GLOB_RECURSE SatDump_ui_CPPS *.cpp imgui/gl3w/gl3w.c)
add_executable(satdump-ui ${SatDump_ui_CPPS} ${WIN32_RESOURCES})
target_include_directories(satdump-ui PUBLIC . ../src-core ../src-interface ../libsdr)

# Link against the core
target_link_libraries(satdump-ui PUBLIC satdump_core satdump_interface)

# OpenGL or OpenGL ES
option(BUILD_GLES "Build the GUI with OpenGL ES" OFF)

if(BUILD_GLES)
    message("Building GUI with OpenGL ES")
    find_package(OpenGLES2 REQUIRED)
    target_compile_definitions(satdump-ui PUBLIC IMGUI_IMPL_OPENGL_ES2)
    target_include_directories(satdump-ui PUBLIC ${OPENGLES2_INCLUDE_DIR})
    target_link_libraries(satdump-ui PUBLIC ${OPENGLES2_LIBRARIES})
else()
    message("Building GUI with OpenGL")
    set(OpenGL_GL_PREFERENCE GLVND)
    find_package(OpenGL REQUIRED)
    target_compile_definitions(satdump-ui PUBLIC IMGUI_IMPL_OPENGL_LOADER_CUSTOM)
    target_include_directories(satdump-ui PUBLIC ${OPENGL_INCLUDE_DIR})
    target_link_libraries(satdump-ui PUBLIC ${OPENGL_LIBRARIES})
endif()

#GLFW3
if(MSVC)
    target_link_libraries(satdump-ui PRIVATE glfw3dll.dll)
else()
    find_package(glfw3 CONFIG REQUIRED)
    target_link_libraries(satdump-ui PRIVATE glfw)
endif()

install(TARGETS satdump-ui DESTINATION bin)
