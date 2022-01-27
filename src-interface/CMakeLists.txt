cmake_minimum_required (VERSION 3.0.0)
include(CheckFunctionExists)

file(GLOB_RECURSE LIBSDR_CPPS *.cpp)

add_library(satdump_interface STATIC ${LIBSDR_CPPS})
target_include_directories(satdump_interface PUBLIC . ../src-core ../libsdr)
set_target_properties(satdump_interface PROPERTIES COMPILE_DEFINITIONS BUILDER_STATIC_DEFINE)

# Set the resources path
if(MSVC)
    target_compile_definitions(satdump_core PUBLIC RESOURCES_PATH="./")
else()
    target_compile_definitions(satdump_core PUBLIC RESOURCES_PATH="${CMAKE_INSTALL_PREFIX}/share/satdump/")
endif()

if(BUILD_LIVE)
    target_compile_definitions(satdump_interface PUBLIC BUILD_LIVE="1")
    target_link_libraries(satdump_interface PUBLIC sdr)
endif()

if(BUILD_ZIQ)
    target_compile_definitions(satdump_interface PUBLIC BUILD_ZIQ="1")
endif()

if(BUILD_TRACKING)
    target_compile_definitions(satdump_interface PUBLIC BUILD_TRACKING="1")
endif()

# Link against the core
target_link_libraries(satdump_interface PUBLIC satdump_core)

if(MSVC)
        
else()
       
endif()
