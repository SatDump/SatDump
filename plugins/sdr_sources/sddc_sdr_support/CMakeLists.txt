cmake_minimum_required(VERSION 3.0.0)
project(sddc_sdr_support)


message("Building with SDDC support")

# Why the heck does Windows require that CyAPI .cpp and not linux!?
if(MSVC)
    file(GLOB sddc_sdr_support_CPPS main.cpp sddc_sdr.cpp lib/libsddc/*.c lib/libsddc/*.cpp lib/Core/*.c lib/Core/*.cpp lib/Core/radio/*.cpp lib/Core/pffft/*.c lib/Core/pffft/*.cpp lib/Core/arch/win32/*.c lib/Core/arch/win32/*.cpp lib/Core/arch/win32/CyAPI/*.cpp)
else()
    file(GLOB sddc_sdr_support_CPPS main.cpp sddc_sdr.cpp lib/libsddc/*.c lib/libsddc/*.cpp lib/Core/*.c lib/Core/*.cpp lib/Core/radio/*.cpp lib/Core/pffft/*.c lib/Core/pffft/*.cpp lib/Core/arch/linux/*.c lib/Core/arch/linux/*.cpp)
endif()

add_library(sddc_sdr_support SHARED ${sddc_sdr_support_CPPS})

target_include_directories(sddc_sdr_support PRIVATE "lib/" "lib/Core/" "lib/Core/pffft/" "lib/libsddc/")

if(UNIX)
    find_package(PkgConfig)
    pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
    target_include_directories(sddc_sdr_support PRIVATE ${LIBUSB_INCLUDE_DIRS})
    target_link_directories(sddc_sdr_support PRIVATE ${LIBUSB_LIBRARY_DIRS})
    target_link_libraries(sddc_sdr_support PRIVATE ${LIBUSB_LIBRARIES})
endif()

target_link_libraries(sddc_sdr_support PUBLIC satdump_core)
target_include_directories(sddc_sdr_support PUBLIC src)

install(TARGETS sddc_sdr_support DESTINATION share/satdump/plugins)