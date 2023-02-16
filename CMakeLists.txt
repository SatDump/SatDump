cmake_minimum_required(VERSION 3.0.0)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(CheckCXXSourceCompiles)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

project(SatDump)

option(BUILD_GUI "Build the GUI" ON)
option(BUILD_TESTING "Build the testing program" OFF)
option(BUILD_TOOLS "Build the tools programs" OFF)
option(BUILD_ZIQ "Build support for the custom ZIQ format" ON)
option(BUILD_ZIQ2 "Build support for the custom ZIQ2 format (Very WIP)" OFF)
option(BUILD_MSVC "Build for Windows with MSVC" OFF) # Seems like "MSVC" as a macro messed up below for some reason...
option(BUILD_OPENCL "Build with OpenCL GPU Accelerations" ON)
option(BUILD_OPENMP "Build with OpenMP Optimizations" ON)

if(ANDROID)
    # Build settings
    set(BUILD_OPENCL OFF)
    set(BUILD_OPENMP OFF)
endif()

# Set default install prefix to /usr
if(UNIX)
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        if(APPLE)
            set(CMAKE_INSTALL_PREFIX /usr/local CACHE PATH "Set CMAKE Default to /usr/local" FORCE)
        else()
            set(CMAKE_INSTALL_PREFIX /usr CACHE PATH "Set CMAKE Default to /usr" FORCE)
        endif()
    endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
endif()

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

    if(BUILD_OPENMP)
        message("Compiling with OpenMP support!")

        if(APPLE)
            # set(CMAKE_C_COMPILER "/usr/local/opt/llvm/bin/clang")
            # set(CMAKE_CXX_COMPILER "/usr/local/opt/llvm/bin/clang++")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Xpreprocessor -fopenmp -I/usr/local/opt/llvm/include -I/usr/local/opt/libomp/include -L/usr/local/opt/libomp/lib")
        else()
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
        endif()
    endif()

    find_package(ZSTD)

    if((NOT ZSTD_FOUND) AND BUILD_ZIQ AND(NOT ANDROID))
        message("ZIQ Was enabled but libzstd was not found!")
        set(BUILD_ZIQ OFF)
    endif()
else()
    # set(CMAKE_CXX_FLAGS "-Wall")
    # set(CMAKE_CXX_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2")
endif()

if(MSVC OR BUILD_MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP8 /EHsc /wd4305 /wd4267 /wd4244") # Speed up this to object-level
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP8 /EHsc /wd4305 /wd4267 /wd4244") # Speed up this to object-level
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    include_directories(vcpkg/installed/x64-windows/include)
    link_directories(vcpkg/installed/x64-windows/lib)
endif()

project(SatDump VERSION "1.1.0")

# Check if the current commit has a tag
execute_process(
    COMMAND git name-rev --name-only --tags HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    OUTPUT_VARIABLE GIT_HAS_TAG
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Get the latest abbreviated commit hash of current branch
execute_process(
    COMMAND git log -1 --format=%h
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    OUTPUT_VARIABLE GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Set version
if(GIT_HAS_TAG STREQUAL "undefined")
    message("-- Commit has no tag (${GIT_HAS_TAG}). Assume dev build")
    add_compile_definitions(SATDUMP_VERSION="${PROJECT_VERSION}-${GIT_HASH}")
else()
    message("-- Commit has no tag (${GIT_HAS_TAG}). Assume release build")
    add_compile_definitions(SATDUMP_VERSION="${PROJECT_VERSION}")
endif()

if(ANDROID)
    add_compile_definitions(ANDROID_ABI_LIB="${ANDROID_ABI}")
endif()

# Check we have OpenCL
if(MSVC)
    set(OPENCL_FOUND 1)
    set(OpenCL_LIBRARY OpenCL.dll)
    set(OpenCL_LIBRARIES OpenCL.dll)
else()
    find_package(OpenCL)
endif()

if(OPENCL_FOUND AND BUILD_OPENCL)
    message("OpenCL Found! SatDump will support accelerated GPU computing.")

# add_compile_definitions(USE_OPENCL="1") # This does NOT work with MSVC. Kept anyway for now.
else()
    message("OpenCL NOT found or disabled!")
    set(BUILD_OPENCL OFF)
endif()

# For good measure
if(BUILD_ZIQ)
    message("Building with ZIQ Support")
endif()

# If in debug, set the macro
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(BUILD_IS_DEBUG=1)
    message("")
    message("██████╗  █████╗ ███╗   ██╗ ██████╗ ███████╗██████╗ ")
    message("██╔══██╗██╔══██╗████╗  ██║██╔════╝ ██╔════╝██╔══██╗")
    message("██║  ██║███████║██╔██╗ ██║██║  ███╗█████╗  ██████╔╝")
    message("██║  ██║██╔══██║██║╚██╗██║██║   ██║██╔══╝  ██╔══██╗")
    message("██████╔╝██║  ██║██║ ╚████║╚██████╔╝███████╗██║  ██║")
    message("╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═══╝ ╚═════╝ ╚══════╝╚═╝  ╚═╝")
    message("Building SatDump in Debug mode!")
    message("")
endif()

# Check system has <filesystem>
check_cxx_source_compiles("#include <filesystem>\n int main() { return 0; }" STD_HAS_FILESYSTEM)

if(NOT STD_HAS_FILESYSTEM)
    include_directories(std_filesystem)
endif()

add_subdirectory(src-core)
add_subdirectory(src-cli)

if(BUILD_GUI)
    message("Building the GUI")
    add_subdirectory(src-interface)

    if(NOT ANDROID)
        add_subdirectory(src-ui)
    endif()
endif()

if(BUILD_TESTING)
    add_subdirectory(src-testing)
endif()

if(BUILD_TOOLS)
    add_subdirectory(tools)
endif()

add_subdirectory(plugins)

# Install configuration
if(NOT ANDROID)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/pipelines DESTINATION share/satdump)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/resources DESTINATION share/satdump)
    install(FILES ${CMAKE_SOURCE_DIR}/satdump_cfg.json DESTINATION share/satdump)
    install(FILES ${CMAKE_SOURCE_DIR}/icon.png DESTINATION share/satdump)
    configure_file(${CMAKE_SOURCE_DIR}/satdump.desktop ${CMAKE_CURRENT_BINARY_DIR}/satdump.desktop @ONLY)

    if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/satdump.desktop DESTINATION share/applications)
    endif()

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
endif()