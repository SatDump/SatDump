cmake_minimum_required(VERSION 3.0.0)
project(dvb_support)

file(GLOB_RECURSE dvb_support_CPPS *.cpp)
add_library(dvb_support SHARED ${dvb_support_CPPS})
target_link_libraries(dvb_support PUBLIC satdump_core)
target_include_directories(dvb_support PUBLIC src .)

# SSE41
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
    message("Found SSE features, enabling building SSE code for DVB")

    if(UNIX OR ANDROID)
        string(REPLACE " " ";" FINAL_FLAGS_SSE41 "${CMAKE_CXX_FLAGS} -msse4.1")
    elseif(MSVC OR BUILD_MSVC)
        string(REPLACE " " ";" FINAL_FLAGS_SSE41 "${CMAKE_CXX_FLAGS} ${SSE41_C_FLAGS}")
    endif()

    target_compile_options(dvb_support PUBLIC ${FINAL_FLAGS_SSE41})
else()
    message("SSE Features not found! This is only an error on x86 CPUs that should support SSE4.1")
endif()

install(TARGETS dvb_support DESTINATION share/satdump/plugins)