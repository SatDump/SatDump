cmake_minimum_required (VERSION 3.0.0)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tools)

option(BUILD_GEOREF2GEOTIFF "Build georef2geotiff" OFF)

if(BUILD_GEOREF2GEOTIFF)
file(GLOB_RECURSE SatDump_georef2geotiff_CPPS  *.cpp)

add_executable(georef2geotiff ${SatDump_georef2geotiff_CPPS})
target_include_directories(georef2geotiff PUBLIC src-cli src-core)

# Link against the core
target_link_libraries(georef2geotiff PUBLIC satdump_core gdal)

install(TARGETS georef2geotiff DESTINATION bin)
endif()