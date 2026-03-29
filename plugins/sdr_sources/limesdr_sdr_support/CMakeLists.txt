cmake_minimum_required(VERSION 3.12)
project(limesdr_sdr_support)

# Legacy LimeSuite version

find_library(LIMESDR_LIBRARY LimeSuite)

if(LIMESDR_LIBRARY OR MSVC OR ANDROID)
    message("Building with LimeSDR support")
    file(GLOB_RECURSE limesdr_sdr_support_CPPS *.cpp)
    add_library(limesdr_sdr_support SHARED ${limesdr_sdr_support_CPPS})
    target_include_directories(limesdr_sdr_support PUBLIC src)

    target_compile_definitions(limesdr_sdr_support PUBLIC LIMESUITENG=0)

    if(MSVC)
        target_link_libraries(limesdr_sdr_support PUBLIC satdump_core LimeSuite.dll)
        install(TARGETS limesdr_sdr_support DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins RUNTIME_DEPENDENCY_SET satdump_deps)
    elseif(ANDROID)
        target_link_libraries(limesdr_sdr_support PUBLIC satdump_core limesuite usb)
        target_include_directories(limesdr_sdr_support PUBLIC ../../../android/deps/liblimesuite)
        install(TARGETS limesdr_sdr_support DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins)
    else()
        target_link_libraries(limesdr_sdr_support PUBLIC satdump_core ${LIMESDR_LIBRARY})
        install(TARGETS limesdr_sdr_support DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins)
    endif()
else()
    message("LimeSDR Library could not be found! Not building.")
endif()

# LimeSuite-NG version

find_library(LIMESDRNG_LIBRARY limesuiteng-legacyapi)# LimeSuite)

if(LIMESDRNG_LIBRARY) # OR MSVC OR ANDROID)
    message("Building with LimeSDR-NG support")
    file(GLOB_RECURSE limesdrng_sdr_support_CPPS *.cpp)
    add_library(limesdrng_sdr_support SHARED ${limesdrng_sdr_support_CPPS})
    target_include_directories(limesdrng_sdr_support PUBLIC src)

    target_compile_definitions(limesdrng_sdr_support PUBLIC LIMESUITENG=1)

    if(MSVC)
        target_link_libraries(limesdrng_sdr_support PUBLIC satdump_core limesuiteng-legacyapi.dll)
        install(TARGETS limesdrng_sdr_support DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins RUNTIME_DEPENDENCY_SET satdump_deps)
    elseif(ANDROID)
        target_link_libraries(limesdrng_sdr_support PUBLIC satdump_core limesuiteng-legacyapi usb)
        target_include_directories(limesdrng_sdr_support PUBLIC ../../../android/deps/liblimesuiteng-legacyapi)
        install(TARGETS limesdrng_sdr_support DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins)
    else()
        target_link_libraries(limesdrng_sdr_support PUBLIC satdump_core ${LIMESDRNG_LIBRARY})
        install(TARGETS limesdrng_sdr_support DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins)
    endif()
else()
    message("LimeSDR-NG Library could not be found! Not building.")
endif()