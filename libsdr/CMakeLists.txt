cmake_minimum_required (VERSION 3.0.0)
include(CheckFunctionExists)

file(GLOB_RECURSE LIBSDR_CPPS  sdr/*.cpp)

add_library(sdr STATIC ${LIBSDR_CPPS})
target_include_directories(sdr PUBLIC . ../src-core)
set_target_properties(sdr PROPERTIES COMPILE_DEFINITIONS BUILDER_STATIC_DEFINE)

target_link_libraries(sdr PUBLIC satdump_core)

if(MSVC)
        target_link_libraries(sdr PUBLIC airspy.dll)
        target_link_libraries(sdr PUBLIC rtlsdr.dll)
        target_link_libraries(sdr PUBLIC hackrf.dll)
        #target_link_libraries(sdr PUBLIC LimeSuite.dll)
else()
        # Airspy
        find_library(AIRSPY_LIBRARY airspy REQUIRED)
        target_link_libraries(sdr PUBLIC ${AIRSPY_LIBRARY})

        # RTL-SDR
        find_library(RTLSDR_LIBRARY rtlsdr REQUIRED)
        target_link_libraries(sdr PUBLIC ${RTLSDR_LIBRARY})

        # HackRF
        find_library(HACKRF_LIBRARY hackrf REQUIRED)
        target_link_libraries(sdr PUBLIC ${HACKRF_LIBRARY})

        # LimeSDR
        #find_library(LIME_LIBRARY LimeSuite REQUIRED)
        #target_link_libraries(sdr PUBLIC ${LIME_LIBRARY})
endif()

set(CMAKE_REQUIRED_LIBRARIES ${RTLSDR_LIBRARIES})
check_function_exists(rtlsdr_set_bias_tee HAS_RTLSDR_SET_BIAS_TEE)
add_compile_definitions(HAS_RTLSDR_SET_BIAS_TEE="1")
unset(CMAKE_REQUIRED_LIBRARIES)
