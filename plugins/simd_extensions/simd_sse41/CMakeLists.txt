cmake_minimum_required(VERSION 3.0.0)
project(simd_sse41)

if(NOT ANDROID)
    include(FindSSE41)
endif()

if(ANDROID)
    set(SSE41_FOUND 0)

    if(ANDROID_ABI STREQUAL "x86")
        set(SSE41_FOUND 1)
    endif()

    if(ANDROID_ABI STREQUAL "x86_64")
        set(SSE41_FOUND 1)
    endif()
endif()

if(SSE41_FOUND)
    message("Found SSE41 features, enabling SSE41 plugin")

    file(GLOB_RECURSE simd_sse41_CPPS *.cpp)
    add_library(simd_sse41 SHARED ${simd_sse41_CPPS})
    target_link_libraries(simd_sse41 PUBLIC satdump_core)
    target_include_directories(simd_sse41 PUBLIC src)

    if(UNIX OR ANDROID)
        set(FINAL_FLAGS_SSE41 "-msse4.1")
    elseif(MSVC OR BUILD_MSVC)
        set(FINAL_FLAGS_SSE41 "${SSE41_C_FLAGS}")
    endif()

    # Enable on all files but main.cpp
    foreach(_file ${simd_sse41_CPPS})
        if(NOT(${_file} MATCHES "main.cpp$"))
            set_source_files_properties(${_file} PROPERTIES COMPILE_FLAGS "${FINAL_FLAGS_SSE41}")
        endif()
    endforeach()

    install(TARGETS simd_sse41 DESTINATION share/satdump/plugins)
else()
    message("SSE41 Features not found! This is only an error on x86 CPUs that should support SSE41")
endif()