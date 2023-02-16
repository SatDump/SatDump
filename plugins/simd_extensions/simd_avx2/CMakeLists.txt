cmake_minimum_required(VERSION 3.0.0)
project(simd_avx2)

if(NOT ANDROID)
    include(FindAVX)
endif()

if(ANDROID)
    set(CXX_HAS_AVX_2 0)

    if(ANDROID_ABI STREQUAL "x86")
        set(CXX_HAS_AVX_2 1)
    endif()

    if(ANDROID_ABI STREQUAL "x86_64")
        set(CXX_HAS_AVX_2 1)
    endif()
endif()

if(CXX_HAS_AVX_2)
    message("Found AVX2 features, enabling AVX2 plugin")

    file(GLOB_RECURSE simd_avx2_CPPS *.cpp)
    add_library(simd_avx2 SHARED ${simd_avx2_CPPS})
    target_link_libraries(simd_avx2 PUBLIC satdump_core)
    target_include_directories(simd_avx2 PUBLIC src)

    if(UNIX OR ANDROID)
        set(FINAL_FLAGS_AVX2 "-mavx2")
    elseif(MSVC OR BUILD_MSVC)
        set(FINAL_FLAGS_AVX2 "${AVX2_C_FLAGS}")
    endif()

    # Enable on all files but main.cpp
    foreach(_file ${simd_avx2_CPPS})
        if(NOT(${_file} MATCHES "main.cpp$"))
            set_source_files_properties(${_file} PROPERTIES COMPILE_FLAGS "${FINAL_FLAGS_AVX2}")
        endif()
    endforeach()

    install(TARGETS simd_avx2 DESTINATION share/satdump/plugins)
else()
    message("AVX2 Features not found! This is only an error on x86 CPUs that should support AVX2")
endif()