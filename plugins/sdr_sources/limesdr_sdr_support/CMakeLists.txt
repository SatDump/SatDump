cmake_minimum_required(VERSION 3.0.0)
project(limesdr_sdr_support)


find_library(LIMESDR_LIBRARY LimeSuite)

if(LIMESDR_LIBRARY OR MSVC OR ANDROID)
    message("Building with LimeSDR support")
    file(GLOB_RECURSE limesdr_sdr_support_CPPS *.cpp)
    add_library(limesdr_sdr_support SHARED ${limesdr_sdr_support_CPPS})

    if(MSVC)
        target_link_libraries(limesdr_sdr_support PUBLIC satdump_core LimeSuite.dll)
    elseif(ANDROID)
        target_link_libraries(limesdr_sdr_support PUBLIC satdump_core limesuite usb)
        target_include_directories(limesdr_sdr_support PUBLIC ../../../android/deps/liblimesuite)
    else()
        target_link_libraries(limesdr_sdr_support PUBLIC satdump_core ${LIMESDR_LIBRARY})
    endif()

    target_include_directories(limesdr_sdr_support PUBLIC src)

    install(TARGETS limesdr_sdr_support DESTINATION share/satdump/plugins)
else()
    message("LimeSDR Library could not be found! Not building.")
endif()