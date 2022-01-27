cmake_minimum_required (VERSION 3.0.0)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

option(NOGUI                "Disable building the GUI"                                  OFF)
option(BUILD_TESTING        "Build the testing program"                                 OFF)
option(USE_VIDEO_ENCODER    "Build the video encoder module, links against libav."      OFF)
option(BUILD_LIVE           "Build the live processing feature (WIP)"                   ON)
option(BUILD_ZIQ            "Build support for the custom ZIQ format"                   OFF)
option(BUILD_PLUGINS        "Build plugins, needs the submodule to be pulled"           OFF)
option(BUILD_TRACKING       "Build the very WIP Tracking UI"                            OFF)
option(BUILD_MSVC           "Build for Windows with MSVC"                               OFF) # Seems like "MSVC" as a macro messed up below for some reason...

if(APPLE)
    include_directories(/usr/local/include)
    include_directories(/usr/local/opt/jpeg-turbo/include)
    include_directories(/usr/local/Cellar/fftw/3.3.9/include)
    link_directories(/usr/local/lib)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
endif()

if(UNIX)
    set(CMAKE_CXX_FLAGS "-Wall -Wextra")
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
else()
    #set(CMAKE_CXX_FLAGS "-Wall")
    #set(CMAKE_CXX_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2")
endif()

if(MSVC OR BUILD_MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP8 /EHsc /wd4305 /wd4267 /wd4244") # Speed up this to object-level
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP8 /EHsc /wd4305 /wd4267 /wd4244") # Speed up this to object-level
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    include_directories(vcpkg/installed/x64-windows/include)
    link_directories(vcpkg/installed/x64-windows/lib)
endif()

project (SatDump VERSION "0.0.39")

add_compile_definitions(SATDUMP_VERSION="${PROJECT_VERSION}")

include(CheckCXXSourceCompiles)

# Check system has <filesystem>
check_cxx_source_compiles("#include <filesystem>\n int main() { return 0; }" STD_HAS_FILESYSTEM)
if(NOT STD_HAS_FILESYSTEM)
include_directories(std_filesystem)
endif()

add_subdirectory(src-core)
add_subdirectory(src-cli)

if(BUILD_LIVE)
    add_subdirectory(libsdr)
endif()

if(NOT NOGUI)
    add_subdirectory(src-interface)
    add_subdirectory(src-ui)
endif()

if(BUILD_LIVE)
    add_subdirectory(src-ingestor)
    add_subdirectory(src-tools/src-recorder)
endif()

if(BUILD_TESTING)
    add_subdirectory(src-testing)
endif()

if(BUILD_PLUGINS)
    add_subdirectory(plugins)
endif()

# If GDAL is available, build the georef2geotiff tool
find_library(GDAL_LIBRARY gdal)
if(GDAL_LIBRARY)
    add_subdirectory(src-tools/src-georef2geotiff)
endif()

install(DIRECTORY ${CMAKE_SOURCE_DIR}/pipelines DESTINATION share/satdump)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/resources DESTINATION share/satdump)
install(FILES ${CMAKE_SOURCE_DIR}/Roboto-Medium.ttf DESTINATION share/satdump)
install(FILES ${CMAKE_SOURCE_DIR}/icon.png DESTINATION share/satdump)
configure_file(${CMAKE_SOURCE_DIR}/satdump.desktop ${CMAKE_CURRENT_BINARY_DIR}/satdump.desktop @ONLY)

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/satdump.desktop DESTINATION /usr/share/applications)
endif ()

# Create uninstall target
configure_file(${CMAKE_SOURCE_DIR}/cmake/cmake_uninstall.cmake ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake @ONLY)
add_custom_target(uninstall ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

# CPack Configuration
set(CPACK_PACKAGE_NAME ${PROJECT_NAME} CACHE STRING "satdump-${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A satellite data processing software" CACHE STRING "")
set(CPACK_PACKAGE_VENDOR "SatDump")
set(CPACK_PACKAGE_CONTACT "Aang23")

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")

set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})

set(CPACK_GENERATOR DEB)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE)
set(CPACK_DEB_COMPONENT_INSTALL ON)

include(CPack)