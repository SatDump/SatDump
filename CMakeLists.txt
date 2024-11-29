cmake_minimum_required(VERSION 3.12)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(CheckCXXSourceCompiles)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

if(MSVC OR BUILD_MSVC)
    set(VCPKG_APPLOCAL_DEPS CACHE BOOL FALSE)
endif()

project(SatDump)

option(BUILD_GUI "Build the GUI" ON)
option(BUILD_TESTING "Build the testing program" OFF)
option(BUILD_TOOLS "Build the tools programs" OFF)
option(BUILD_ZIQ "Build support for the custom ZIQ format" ON)
option(BUILD_ZIQ2 "Build support for the custom ZIQ2 format (Very WIP)" OFF)
option(BUILD_MSVC "Build for Windows with MSVC" OFF) # Seems like "MSVC" as a macro messed up below for some reason...
option(BUILD_OPENCL "Build with OpenCL GPU Accelerations" ON)
option(BUILD_OPENMP "Build with OpenMP Optimizations" ON)
option(BUILD_DOCS "Build with Doxygen Documentation" OFF)
option(ENABLE_INSTALL "Enable INSTALL TARGET" ON)

if(ANDROID)
    # Build settings
    set(BUILD_OPENMP OFF)
    set(OPENCL_FOUND 1)
endif()

# Set default install prefix
include(GNUInstallDirs) #Retruns good defaults on non-GNU systems as well
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
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
    link_directories(vcpkg/installed/osx-satdump/lib)

    # Jemalloc
    find_package(Jemalloc REQUIRED)
    include_directories(${JEMALLOC_INCLUDE_DIR})
    link_libraries(${JEMALLOC_LIBRARY})

endif()

if(UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-psabi")
    endif()

    if(BUILD_OPENMP)
        message("Compiling with OpenMP support!")
        if(APPLE)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-command-line-argument -Xpreprocessor")
        endif()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
    endif()

    find_package(ZSTD)

    if((NOT ZSTD_FOUND) AND BUILD_ZIQ AND (NOT ANDROID))
        message("ZIQ Was enabled but libzstd was not found!")
        set(BUILD_ZIQ OFF)
    endif()
else()
    # set(CMAKE_CXX_FLAGS "-Wall")
    # set(CMAKE_CXX_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2")
endif()

if(MSVC OR BUILD_MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_WINSOCK_DEPRECATED_NO_WARNINGS -D_CRT_NONSTDC_NO_WARNINGS)
    add_compile_definitions(_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)

    if(BUILD_OPENMP)
        message("Compiling with OpenMP support!")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /openmp")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /openmp")
    endif()

    # This should be the default, but sometimes it isn't apparently
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MD")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MDd")

    # Speed up this to object-level, Suppress type conversion warnings, suppress
    # POSIX function warnings, suppress warnings about DLL-exported functions, and
    # Suppress warnings about template methods defined in another C++ file
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP8 /EHsc /wd4305 /wd4267 /wd4244 /wd4273 /wd4661")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP8 /EHsc /wd4305 /wd4267 /wd4244 /wd4273 /wd4661")

    #Suppress pointless (to us) warnings about DLL-exported functions
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ignore:4286,4217")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /ignore:4286,4217")
    set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} /ignore:4286,4217")

    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    include_directories(vcpkg/installed/${VCPKG_TARGET_TRIPLET}/include)
    link_directories(vcpkg/installed/${VCPKG_TARGET_TRIPLET}/lib)
endif()

project(SatDump VERSION "1.2.3")

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
if(GIT_HAS_TAG STREQUAL "undefined" OR GIT_HAS_TAG STREQUAL "nightly")
    message("-- Commit has no tag/nightly tag (${GIT_HAS_TAG}). Assume dev build")
    set(SATDUMP_VERSION "${PROJECT_VERSION}-${GIT_HASH}")
else()
    message("-- Commit has tag (${GIT_HAS_TAG}). Assume release build")
    set(SATDUMP_VERSION "${PROJECT_VERSION}")
endif()
add_compile_definitions(SATDUMP_VERSION="${SATDUMP_VERSION}")

# Used for error messages
string(LENGTH "${CMAKE_SOURCE_DIR}/" SOURCE_PATH_SIZE)
add_compile_definitions(SOURCE_PATH_SIZE=${SOURCE_PATH_SIZE})

if(ANDROID)
    add_compile_definitions(ANDROID_ABI_LIB="${ANDROID_ABI}")
endif()

# Check we have OpenCL
if(NOT ANDROID)
    find_package(OpenCL)
endif()

if(OPENCL_FOUND AND BUILD_OPENCL)
    message("OpenCL Found! SatDump will support accelerated GPU computing.")
else()
    message("OpenCL NOT found or disabled!")
    set(BUILD_OPENCL OFF)
endif()

# For good measure
if(BUILD_ZIQ)
    message("Building with ZIQ Support")
endif()

if(BUILD_ZIQ2)
    message("Building with ZIQ2 Support")
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
if((NOT ANDROID) AND ENABLE_INSTALL)
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

    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libfftw3-bin, libvolk-bin | libvolk2-bin, libpng16-16, libnng1, librtlsdr0 | librtlsdr2, libhackrf0, libairspy0, libairspyhf1, libglfw3 | libglfw3-wayland, libzstd1, libjemalloc2 | libjemalloc1, libportaudiocpp0, libhdf5-hl-100t64 | libhdf5-hl-100 | libhdf5-103 | libhdf5-100")
    set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "libomp5-14, ocl-icd-libopencl1, intel-opencl-icd, mesa-opencl-icd")

    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
    set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

    if(MSVC OR BUILD_MSVC)
        set(CPACK_PACKAGING_INSTALL_PREFIX "/")
    else()
        set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
    endif()

    #set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
    #set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
    #set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
    set(CPACK_PACKAGE_VERSION ${SATDUMP_VERSION})

    if(MSVC OR BUILD_MSVC)
        # Customize NSIS template
        file(READ "${CMAKE_ROOT}/Modules/Internal/CPack/NSIS.template.in" NSIS_template)
        set(CREATE_ICONS_REPLACE "  SetOutPath \"$INSTDIR\\bin\"\n@CPACK_NSIS_CREATE_ICONS@\n@CPACK_NSIS_CREATE_ICONS_EXTRA@\n  SetOutPath \"$INSTDIR\"")
        string(CONCAT INSTALLDIR_REPLACE "  InstallDir \"@CPACK_NSIS_INSTALL_ROOT@\\@CPACK_PACKAGE_INSTALL_DIRECTORY@\""
            "\n  InstallDirRegKey HKLM Software\\@CPACK_PACKAGE_VENDOR@\\@CPACK_PACKAGE_INSTALL_REGISTRY_KEY@ \"\"")
        string(REPLACE "@CPACK_NSIS_CREATE_ICONS@\n@CPACK_NSIS_CREATE_ICONS_EXTRA@" "${CREATE_ICONS_REPLACE}" NSIS_template "${NSIS_template}")
        string(REPLACE "  InstallDir \"@CPACK_NSIS_INSTALL_ROOT@\\@CPACK_PACKAGE_INSTALL_DIRECTORY@\"" "${INSTALLDIR_REPLACE}" NSIS_template "${NSIS_template}")
        file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/NSIS.template.in" "${NSIS_template}")
        set(CPACK_MODULE_PATH "${CMAKE_CURRENT_BINARY_DIR}")

        # Generate Installer
        set(CPACK_GENERATOR NSIS)
        set(CPACK_PACKAGE_INSTALL_DIRECTORY SatDump)
        set(CPACK_NSIS_MANIFEST_DPI_AWARE ON)
        set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
        set(CPACK_NSIS_MODIFY_PATH ON)
        set(CPACK_NSIS_BRANDING_TEXT "SatDump - General Purpose Satellite Data Processor")
        set(CPACK_NSIS_MUI_HEADERIMAGE "${PROJECT_SOURCE_DIR}\\\\windows\\\\installer-header.bmp")
        set(CPACK_NSIS_MUI_ICON "${PROJECT_SOURCE_DIR}\\\\windows\\\\icon.ico")
        if(BUILD_GUI)
            set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\satdump-ui.exe")
            set(CPACK_PACKAGE_EXECUTABLES ${CPACK_PACKAGE_EXECUTABLES} "satdump-ui" "SatDump")
            list(APPEND CPACK_NSIS_CREATE_ICONS_EXTRA "CreateShortCut \\\"$DESKTOP\\\\SatDump.lnk\\\" \\\"$INSTDIR\\\\bin\\\\satdump-ui.exe\\\"")
            list(APPEND CPACK_NSIS_DELETE_ICONS_EXTRA "Delete \\\"$DESKTOP\\\\SatDump.lnk\\\"")
        endif()
        if(PLUGIN_REMOTE_SDR_SUPPORT)
            set(CPACK_PACKAGE_EXECUTABLES ${CPACK_PACKAGE_EXECUTABLES} "satdump_sdr_server" "SatDump SDR Server")
        endif()

        install(RUNTIME_DEPENDENCY_SET satdump_deps
            PRE_EXCLUDE_REGEXES
                "api-ms-.*"
                "ext-ms-.*"
                "^hvsifiletrust\\.dll$"
                "^pdmutilities\\.dll$"
                "^vc.*"
                "^msvcp.*"
                "^concrt.*"
                "satdump_core.dll"
                "satdump_interface.dll"
            POST_EXCLUDE_REGEXES
            ".*system32/.*\\.dll"
            DIRECTORIES
            "${PROJECT_SOURCE_DIR}/vcpkg/installed/${VCPKG_TARGET_TRIPLET}/bin"
        )
    else()
        set(CPACK_GENERATOR DEB)
        #set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
        set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
        set(CPACK_DEB_COMPONENT_INSTALL ON)
    endif()

    set(CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE)

    include(CPack)
endif()

# Doxygen configuration
if((CMAKE_BUILD_TYPE MATCHES "^[Rr]elease") AND BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
        message("-- Doxygen build enabled!")

        add_custom_target(docs_doxygen ALL
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM)
    else(DOXYGEN_FOUND)
        message("Doxygen need to be installed to generate the doxygen documentation")
    endif(DOXYGEN_FOUND)
endif()
