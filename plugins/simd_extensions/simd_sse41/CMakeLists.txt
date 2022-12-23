cmake_minimum_required(VERSION 3.0.0)
project(simd_sse41)

include(FindSSE41)

if(SSE41_FOUND)
    message("Found SSE41 features, enabling SSE41 plugin")

    set(SATDUMP_ROOT "/usr")

    file(GLOB_RECURSE simd_sse41_CPPS *.cpp)
    add_library(simd_sse41 SHARED ${simd_sse41_CPPS})
    target_link_libraries(simd_sse41 PUBLIC satdump_core)
    target_include_directories(simd_sse41 PUBLIC src ${SATDUMP_ROOT}/include/satdump)

    string(REPLACE " " ";" FINAL_FLAGS_SSE41 "${CMAKE_CXX_FLAGS} ${SSE41_C_FLAGS}")
    target_compile_options(simd_sse41 PUBLIC ${FINAL_FLAGS_SSE41})

    install(TARGETS simd_sse41 DESTINATION share/satdump/plugins)
else()
    message("SSE41 Features not found! This is only an error on x86 CPUs that should support SSE41")
endif()