cmake_minimum_required(VERSION 3.12)
include(CheckFunctionExists)

file(GLOB_RECURSE LIBSDR_CPPS *.cpp)

add_library(satdump_interface STATIC ${LIBSDR_CPPS})
target_include_directories(satdump_interface PUBLIC . ../src-core)
set_target_properties(satdump_interface PROPERTIES COMPILE_DEFINITIONS BUILDER_STATIC_DEFINE)

if(BUILD_ZIQ)
    target_compile_definitions(satdump_interface PUBLIC BUILD_ZIQ="1")
endif()

if(BUILD_ZIQ2)
    target_compile_definitions(satdump_interface PUBLIC BUILD_ZIQ2="1")
endif()

# Link against the core
target_link_libraries(satdump_interface PUBLIC satdump_core)

if(MSVC)
else()
endif()
