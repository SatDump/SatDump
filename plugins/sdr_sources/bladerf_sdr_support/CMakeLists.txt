cmake_minimum_required(VERSION 3.0.0)
project(bladerf_sdr_support)
include(CheckCXXSourceCompiles)

find_library(BLADERF_LIBRARY bladeRF)

if(BLADERF_LIBRARY OR MSVC) # OR ANDROID)
    message("Building with BladeRF support")
    file(GLOB_RECURSE bladerf_sdr_support_CPPS *.cpp)
    add_library(bladerf_sdr_support SHARED ${bladerf_sdr_support_CPPS})

    set(BLADERF_WIDEBAND_TEST_SOURCE
        "
        #include <libbladeRF.h>
        int main() {
            bladerf *bladerf_dev_obj;
            bladerf_enable_feature(bladerf_dev_obj, BLADERF_FEATURE_OVERSAMPLE, true);
        }
        ")

    if(MSVC)
        target_link_libraries(bladerf_sdr_support PUBLIC satdump_core bladerf.dll)
        # set(CMAKE_REQUIRED_LIBRARIES bladerf.dll)
        # check_cxx_source_compiles("${BLADERF_WIDEBAND_TEST_SOURCE}" BLADERF_HAVE_WIDEBAND)
        set(BLADERF_HAVE_WIDEBAND 1)
    elseif(ANDROID)
        target_link_libraries(bladerf_sdr_support PUBLIC satdump_core bladerf usb)
        target_include_directories(bladerf_sdr_support PUBLIC ../../../android/deps/libbladerf)
    else()
        target_link_libraries(bladerf_sdr_support PUBLIC satdump_core ${BLADERF_LIBRARY})
        set(CMAKE_REQUIRED_LIBRARIES ${BLADERF_LIBRARY})
        check_cxx_source_compiles("${BLADERF_WIDEBAND_TEST_SOURCE}" BLADERF_HAVE_WIDEBAND)
    endif()

    if(BLADERF_HAVE_WIDEBAND)
        message("libBladeRF has wideband feature, enabling support!")
        target_compile_definitions(bladerf_sdr_support PUBLIC BLADERF_HAS_WIDEBAND=1)
    endif()

    target_include_directories(bladerf_sdr_support PUBLIC src)

    install(TARGETS bladerf_sdr_support DESTINATION share/satdump/plugins)
else()
    message("BladeRF Library could not be found! Not building.")
endif()