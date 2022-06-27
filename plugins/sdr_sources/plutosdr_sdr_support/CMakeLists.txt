cmake_minimum_required (VERSION 3.0.0)
project(plutosdr_sdr_support)

set(SATDUMP_ROOT "/usr")

find_library(IIO_LIBRARY iio)
find_library(AD9361_LIBRARY ad9361)

if(IIO_LIBRARY OR MSVC)
    message("Building with PlutoSDR support")
    file(GLOB_RECURSE plutosdr_sdr_support_CPPS *.cpp)
    add_library(plutosdr_sdr_support SHARED ${plutosdr_sdr_support_CPPS})

    if(MSVC)
        target_link_libraries(plutosdr_sdr_support PUBLIC satdump_core iio.dll ad9361.dll)
    elseif(ANDROID)
    #    target_link_libraries(plutosdr_sdr_support PUBLIC satdump_core airspy usb)
    #    target_include_directories(plutosdr_sdr_support PUBLIC ../../../android/deps/libairspy)
    else()
        target_link_libraries(plutosdr_sdr_support PUBLIC satdump_core ${IIO_LIBRARY} ${AD9361_LIBRARY})
    endif()

    target_include_directories(plutosdr_sdr_support PUBLIC src ${SATDUMP_ROOT}/include/satdump)

    install(TARGETS plutosdr_sdr_support DESTINATION share/satdump/plugins)
else()
    message("IIO or AD9361 Library could not be found! Not building.")
endif()