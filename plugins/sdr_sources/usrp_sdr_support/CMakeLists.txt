cmake_minimum_required(VERSION 3.12)
project(usrp_sdr_support)


find_library(UHD_LIBRARY uhd)

if(UHD_LIBRARY OR MSVC) # OR ANDROID)
    message("Building with USRP support")
    file(GLOB_RECURSE usrp_sdr_support_CPPS *.cpp)
    add_library(usrp_sdr_support SHARED ${usrp_sdr_support_CPPS})

    if(MSVC)
        target_link_libraries(usrp_sdr_support PUBLIC satdump_core uhd.dll)
    # elseif(ANDROID)
    # target_link_libraries(usrp_sdr_support PUBLIC satdump_core airspy usb)
    # target_include_directories(usrp_sdr_support PUBLIC ../../../android/deps/libairspy)
    else()
        target_link_libraries(usrp_sdr_support PUBLIC satdump_core ${UHD_LIBRARY})
    endif()

    target_include_directories(usrp_sdr_support PUBLIC src)

    install(TARGETS usrp_sdr_support DESTINATION lib/satdump/plugins)
else()
    message("UHD Library could not be found! Not building.")
endif()