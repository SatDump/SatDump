cmake_minimum_required (VERSION 3.0.0)
project(bladerf_sdr_support)


find_library(BLADERF_LIBRARY bladeRF)

if(BLADERF_LIBRARY OR MSVC)# OR ANDROID)
    message("Building with BladeRF support")
    file(GLOB_RECURSE bladerf_sdr_support_CPPS *.cpp)
    add_library(bladerf_sdr_support SHARED ${bladerf_sdr_support_CPPS})

    if(MSVC)
        target_link_libraries(bladerf_sdr_support PUBLIC satdump_core bladerf.dll)
    elseif(ANDROID)
        target_link_libraries(bladerf_sdr_support PUBLIC satdump_core bladerf usb)
        target_include_directories(bladerf_sdr_support PUBLIC ../../../android/deps/libbladerf)
    else()
        target_link_libraries(bladerf_sdr_support PUBLIC satdump_core ${BLADERF_LIBRARY})
    endif()

    target_include_directories(bladerf_sdr_support PUBLIC src)

    install(TARGETS bladerf_sdr_support DESTINATION share/satdump/plugins)
else()
    message("BladeRF Library could not be found! Not building.")
endif()