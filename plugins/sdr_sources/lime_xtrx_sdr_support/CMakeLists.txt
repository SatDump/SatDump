cmake_minimum_required(VERSION 3.15)
project(lime_xtrx_sdr_support)


find_library(LIME_SUITE_NG_LIBRARY limesuiteng)

if(LIME_SUITE_NG_LIBRARY OR MSVC OR ANDROID)
    message("Building with Lime XTRX SDR support")
    file(GLOB_RECURSE lime_xtrx_sdr_support_CPPS *.cpp)
    add_library(lime_xtrx_sdr_support SHARED ${lime_xtrx_sdr_support_CPPS})
    target_include_directories(lime_xtrx_sdr_support PUBLIC src)

    if(MSVC)
        target_link_libraries(lime_xtrx_sdr_support PUBLIC satdump_core limesuiteng.dll)
        install(TARGETS lime_xtrx_sdr_support DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins RUNTIME_DEPENDENCY_SET satdump_deps)
    elseif(ANDROID)
    else()
        target_link_libraries(lime_xtrx_sdr_support PUBLIC satdump_core ${LIME_SUITE_NG_LIBRARY})
        install(TARGETS lime_xtrx_sdr_support DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins)
    endif()
else()
    message("Lime Suite NG Library could not be found! Not building.")
endif()