cmake_minimum_required (VERSION 3.0.0)
project(limesdr_sdr_support)

set(SATDUMP_ROOT "/usr")

find_library(LIMESDR_LIBRARY LimeSuite)

if(LIMESDR_LIBRARY)
    message("Building with LimeSDR support")
    file(GLOB_RECURSE limesdr_sdr_support_CPPS *.cpp)
    add_library(limesdr_sdr_support SHARED ${limesdr_sdr_support_CPPS})
    target_link_libraries(limesdr_sdr_support PUBLIC satdump_core ${LIMESDR_LIBRARY})
    target_include_directories(limesdr_sdr_support PUBLIC src ${SATDUMP_ROOT}/include/satdump)
else()
    message("LimeSDR Library could not be found! Not building.")
endif()