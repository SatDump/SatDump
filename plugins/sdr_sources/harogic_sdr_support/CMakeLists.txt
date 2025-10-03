cmake_minimum_required(VERSION 3.12)
project(harogic_sdr_support)

# Add our custom cmake module path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Find the Harogic HTRA library using our find-module
find_package(LibHTRA REQUIRED)

# If the library was found, proceed with building the plugin
if(LibHTRA_FOUND)
    message(STATUS "Found Harogic HTRA library, building plugin.")
    message(STATUS " -> LibHTRA Version: ${LibHTRA_VERSION}")
    message(STATUS " -> LibHTRA Includes: ${LibHTRA_INCLUDE_DIRS}")
    message(STATUS " -> LibHTRA Libraries: ${LibHTRA_LIBRARIES}")
    
    # Gather all source files
    file(GLOB_RECURSE harogic_sdr_support_CPPS *.cpp)

    # Create the shared library for the plugin
    add_library(harogic_sdr_support SHARED ${harogic_sdr_support_CPPS})

    # Link against the SatDump core and the HTRA library
    # Using the imported target LibHTRA::LibHTRA handles include directories automatically
    target_link_libraries(harogic_sdr_support PUBLIC satdump_core LibHTRA::LibHTRA)
    
    # Explicitly add include directories (good practice, though handled by imported target)
    target_include_directories(harogic_sdr_support PUBLIC 
        ${LibHTRA_INCLUDE_DIRS}
        src # For satdump core headers
    )
    
    # Install the final plugin library to the correct location
    install(TARGETS harogic_sdr_support DESTINATION ${CMAKE_INSTALL_LIBDIR}/satdump/plugins)
else()
    message(WARNING "Harogic HTRA library could not be found! Not building harogic_sdr_support plugin.")
endif()
