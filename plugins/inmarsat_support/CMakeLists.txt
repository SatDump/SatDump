cmake_minimum_required(VERSION 3.0.0)
project(inmarsat_support)

option(ENABLE_INMARSAT_AUDIO "Enable experimental audio output" OFF)

file(GLOB_RECURSE inmarsat_support_CPPS *.cpp aero/libacars/*.c aero/mbelib/*.c)
add_library(inmarsat_support SHARED ${inmarsat_support_CPPS})
target_link_libraries(inmarsat_support PUBLIC satdump_core)
target_include_directories(inmarsat_support PUBLIC src .)

if(ENABLE_INMARSAT_AUDIO)
    target_link_libraries(inmarsat_support PUBLIC rtaudio)
    target_compile_definitions(inmarsat_support PUBLIC ENABLE_AUDIO=1)
endif()

if(MSVC)
    target_link_libraries(inmarsat_support PUBLIC ws2_32 zlib.dll)
endif()

install(TARGETS inmarsat_support DESTINATION share/satdump/plugins)