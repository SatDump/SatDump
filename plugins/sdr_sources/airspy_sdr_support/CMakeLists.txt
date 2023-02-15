cmake_minimum_required(VERSION 3.0.0)
project(airspy_sdr_support)


find_library(AIRSPY_LIBRARY airspy)

if(AIRSPY_LIBRARY OR MSVC OR ANDROID)
    message("Building with Airspy support")
    file(GLOB_RECURSE airspy_sdr_support_CPPS *.cpp)
    add_library(airspy_sdr_support SHARED ${airspy_sdr_support_CPPS})

    if(MSVC)
        target_link_libraries(airspy_sdr_support PUBLIC satdump_core airspy.dll)
    elseif(ANDROID)
        target_link_libraries(airspy_sdr_support PUBLIC satdump_core airspy usb)
        target_include_directories(airspy_sdr_support PUBLIC ../../../android/deps/libairspy)
    else()
        target_link_libraries(airspy_sdr_support PUBLIC satdump_core ${AIRSPY_LIBRARY})
    endif()

    target_include_directories(airspy_sdr_support PUBLIC src)

    install(TARGETS airspy_sdr_support DESTINATION share/satdump/plugins)
else()
    message("Airspy Library could not be found! Not building.")
endif()