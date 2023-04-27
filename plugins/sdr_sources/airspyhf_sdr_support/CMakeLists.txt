cmake_minimum_required(VERSION 3.0.0)
project(airspyhf_sdr_support)


find_library(AIRSPYHF_LIBRARY airspyhf)

if(AIRSPYHF_LIBRARY OR MSVC OR ANDROID)
    message("Building with AirspyHF support")
    file(GLOB_RECURSE airspyhf_sdr_support_CPPS *.cpp)
    add_library(airspyhf_sdr_support SHARED ${airspyhf_sdr_support_CPPS})

    if(MSVC)
        target_link_libraries(airspyhf_sdr_support PUBLIC satdump_core airspyhf.dll)
    elseif(ANDROID)
        target_link_libraries(airspyhf_sdr_support PUBLIC satdump_core airspyhf usb)
        target_include_directories(airspyhf_sdr_support PUBLIC ../../../android/deps/libairspyhf)
    else()
        target_link_libraries(airspyhf_sdr_support PUBLIC satdump_core ${AIRSPYHF_LIBRARY})
    endif()

    target_include_directories(airspyhf_sdr_support PUBLIC src)

    install(TARGETS airspyhf_sdr_support DESTINATION share/satdump/plugins)
else()
    message("AirspyHF Library could not be found! Not building.")
endif()