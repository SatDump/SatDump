cmake_minimum_required(VERSION 3.12)
project(rfnm_sdr_support)


#find_package(PkgConfig)
#pkg_check_modules(RFNM rfnm)
find_library(USB_LIBRARY usb-1.0)

if(USB_LIBRARY OR MSVC) #if(RFNM_LIBRARY) # OR MSVC
    message("Building with RFNM support")
    file(GLOB_RECURSE rfnm_sdr_support_CPPS *.cpp)
    add_library(rfnm_sdr_support SHARED ${rfnm_sdr_support_CPPS})
    target_include_directories(rfnm_sdr_support PUBLIC src ${RFNM_INCLUDE_DIRS})

    if(MSVC)
        target_link_libraries(rfnm_sdr_support PUBLIC satdump_core libusb-1.0.dll)
        install(TARGETS rfnm_sdr_support DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins RUNTIME_DEPENDENCY_SET satdump_deps)
   # elseif(ANDROID)
   #     target_link_libraries(rfnm_sdr_support PUBLIC satdump_core airspy usb)
   #     target_include_directories(rfnm_sdr_support PUBLIC ../../../android/deps/libairspy)
    else()
        target_link_libraries(rfnm_sdr_support PUBLIC satdump_core ${USB_LIBRARY}) #${RFNM_LIBRARY})
        install(TARGETS rfnm_sdr_support DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins)
    endif()
else()
    message("RFNM Library could not be found! Not building.")
endif()