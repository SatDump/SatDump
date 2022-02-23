cmake_minimum_required (VERSION 3.0.0)

file(GLOB_RECURSE SatDump_ui_CPPS  *.cpp)

add_executable(satdump-ui ${SatDump_ui_CPPS})
target_include_directories(satdump-ui PUBLIC . ../src-core ../src-interface ../libsdr)

# Set the resources path
if(MSVC)
    target_compile_definitions(satdump_core PUBLIC RESOURCES_PATH="./")
else()
    target_compile_definitions(satdump_core PUBLIC RESOURCES_PATH="${CMAKE_INSTALL_PREFIX}/share/satdump/")
endif()

# Link against the core
target_link_libraries(satdump-ui PUBLIC satdump_core satdump_interface)

# OpenGL
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
