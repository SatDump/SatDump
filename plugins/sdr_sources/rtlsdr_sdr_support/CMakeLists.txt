cmake_minimum_required(VERSION 3.12)
project(rtlsdr_sdr_support)


find_library(RTLSDR_LIBRARY rtlsdr)

if(RTLSDR_LIBRARY OR ANDROID)
    message("Building with RTL-SDR support")
    file(GLOB_RECURSE rtlsdr_sdr_support_CPPS *.cpp)
    add_library(rtlsdr_sdr_support SHARED ${rtlsdr_sdr_support_CPPS})
    target_include_directories(rtlsdr_sdr_support PUBLIC src)

    if(MSVC)
        target_link_libraries(rtlsdr_sdr_support PUBLIC satdump_core rtlsdr.dll)
        install(TARGETS rtlsdr_sdr_support DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins RUNTIME_DEPENDENCY_SET satdump_deps)
    elseif(ANDROID)
        target_link_libraries(rtlsdr_sdr_support PUBLIC satdump_core rtlsdr usb)
        target_include_directories(rtlsdr_sdr_support PUBLIC ../../../android/deps/librtlsdr
            ../../../android/deps/libusb ${ANDROID_NDK}/sources/android/native_app_glue)
        # Use exactly the device catalog of the pinned librtlsdr, without probing USB.
        set(rtl_catalog_source ${CMAKE_CURRENT_SOURCE_DIR}/../../../android/deps/librtlsdr/librtlsdr.c)
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${rtl_catalog_source})
        file(READ ${rtl_catalog_source} rtl_source)
        string(REGEX MATCH "static rtlsdr_dongle_t known_devices\\[\\] = [{][^;]+;" rtl_catalog "${rtl_source}")
        if(NOT rtl_catalog)
            message(FATAL_ERROR "Could not read bundled RTL-SDR USB device catalog")
        endif()
        file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/rtl_android_catalog.h
            "struct rtlsdr_dongle_t { uint16_t vid, pid; const char *name; };\n${rtl_catalog}\n")
        target_include_directories(rtlsdr_sdr_support PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
        install(TARGETS rtlsdr_sdr_support DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins)
    else()
        target_link_libraries(rtlsdr_sdr_support PUBLIC satdump_core ${RTLSDR_LIBRARY})
        install(TARGETS rtlsdr_sdr_support DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins)
    endif()
else()
    message("RTL-SDR Library could not be found! Not building.")
endif()