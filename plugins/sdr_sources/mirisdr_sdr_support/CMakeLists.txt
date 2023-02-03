cmake_minimum_required(VERSION 3.0.0)
project(mirisdr_sdr_support)


find_library(USB_LIBRARY usb-1.0)

# if(RTLSDR_LIBRARY OR MSVC OR ANDROID)
if(USB_LIBRARY OR MSVC OR ANDROID)
    message("Building with MiriSDR support")
    file(GLOB_RECURSE mirisdr_sdr_support_CPPS *.cpp mirisdr/libmirisdr.c)
    add_library(mirisdr_sdr_support SHARED ${mirisdr_sdr_support_CPPS})

    if(MSVC)
        target_link_libraries(mirisdr_sdr_support PUBLIC satdump_core libusb-1.0.dll)
    elseif(ANDROID)
        target_link_libraries(mirisdr_sdr_support PUBLIC satdump_core usb)
        target_include_directories(mirisdr_sdr_support PUBLIC ../../../android/deps/libusb)
    else()
        target_link_libraries(mirisdr_sdr_support PUBLIC satdump_core ${USB_LIBRARY})
    endif()

    target_include_directories(mirisdr_sdr_support PUBLIC src)

    install(TARGETS mirisdr_sdr_support DESTINATION share/satdump/plugins)
else()
    message("MiriSDR Library could not be found! Not building.")
endif()