cmake_minimum_required(VERSION 3.12)
project(rfnm_sdr_support)


find_library(RFNM_LIBRARY librfnm)

if(RFNM_LIBRARY OR MSVC OR ANDROID)
    message("Building with RFNM support")
    file(GLOB_RECURSE rfnm_sdr_support_CPPS *.cpp)
    add_library(rfnm_sdr_support SHARED ${rfnm_sdr_support_CPPS})

    if(MSVC)
        target_link_libraries(rfnm_sdr_support PUBLIC satdump_core airspy.dll)
   # elseif(ANDROID)
   #     target_link_libraries(rfnm_sdr_support PUBLIC satdump_core airspy usb)
   #     target_include_directories(rfnm_sdr_support PUBLIC ../../../android/deps/libairspy)
    else()
        target_link_libraries(rfnm_sdr_support PUBLIC satdump_core ${RFNM_LIBRARY})
    endif()

    target_include_directories(rfnm_sdr_support PUBLIC src)

    install(TARGETS rfnm_sdr_support DESTINATION lib/satdump/plugins)
else()
    message("RFNM Library could not be found! Not building.")
endif()