cmake_minimum_required(VERSION 3.12)
project(inmarsat_support)

file(GLOB_RECURSE inmarsat_support_CPPS *.cpp aero/libacars/*.c aero/mbelib/*.c)
add_library(inmarsat_support SHARED ${inmarsat_support_CPPS})
target_link_libraries(inmarsat_support PUBLIC satdump_core)
target_include_directories(inmarsat_support PUBLIC src .)

if(MSVC)
    target_link_libraries(inmarsat_support PUBLIC ws2_32 zlib.dll)
endif()

install(TARGETS inmarsat_support DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins)