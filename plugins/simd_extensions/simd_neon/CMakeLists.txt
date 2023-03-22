cmake_minimum_required(VERSION 3.0.0)
project(simd_neon)

if(NOT ANDROID)
    include(FindNEON)
endif()

if(ANDROID)
    set(NEON_FOUND 0)

    if(ANDROID_ABI STREQUAL "armeabi-v7a")
        set(NEON_FOUND 1)
    endif()

    if(ANDROID_ABI STREQUAL "arm64-v8a")
        set(NEON_FOUND 1)
    endif()
endif()

if(NEON_FOUND)
    message("Found NEON features, enabling NEON plugin")

    file(GLOB_RECURSE simd_neon_CPPS *.cpp)
    add_library(simd_neon SHARED ${simd_neon_CPPS})
    target_link_libraries(simd_neon PUBLIC satdump_core)
    target_include_directories(simd_neon PUBLIC src)

    if(UNIX OR ANDROID)
        set(FINAL_FLAGS_NEON "${NEON_C_FLAGS}")
    elseif(MSVC OR BUILD_MSVC)
        set(FINAL_FLAGS_NEON "${NEON_C_FLAGS}")
    endif()

    # Enable on all files but main.cpp
    foreach(_file ${simd_neon_CPPS})
        if(NOT(${_file} MATCHES "main.cpp$"))
            set_source_files_properties(${_file} PROPERTIES COMPILE_FLAGS "${FINAL_FLAGS_NEON}")
        endif()
    endforeach()

    install(TARGETS simd_neon DESTINATION share/satdump/plugins)
else()
    message("NEON Features not found! This is only an error on x86 CPUs that should support NEON")
endif()