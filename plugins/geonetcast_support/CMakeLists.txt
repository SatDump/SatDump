cmake_minimum_required(VERSION 3.12)
project(geonetcast_support)

file(GLOB_RECURSE geonetcast_support_CPPS *.cpp)
add_library(geonetcast_support SHARED ${geonetcast_support_CPPS})
target_link_libraries(geonetcast_support PUBLIC satdump_core)
target_include_directories(geonetcast_support PUBLIC src .)

find_library(LIBHDF5_LIBRARY hdf5)
find_library(LIBHDF5HL_LIBRARY hdf5_hl)

if(LIBHDF5_LIBRARY AND LIBHDF5HL_LIBRARY)
    target_compile_definitions(geonetcast_support PUBLIC ENABLE_HDF5_PARSING=1)
    target_link_libraries(geonetcast_support PUBLIC hdf5 hdf5_hl)
    message("Found HDF5 libraries. Geonetcast HDF5 processing enabled!")
else()
    message("Could not find HDF5 libraries. Geonetcast HDF5 processing disabled!")
endif()

install(TARGETS geonetcast_support DESTINATION lib/satdump/plugins)