# - Try to find LibHTRA
# Once done this will define
#
#  LibHTRA_FOUND - System has libhtra
#  LibHTRA_INCLUDE_DIRS - The libhtra include directories
#  LibHTRA_LIBRARIES - The libraries needed to use libhtra
#  LibHTRA_CFLAGS_OTHER - Compiler switches required for using libhtra
#  LibHTRA_VERSION - The libhtra version
#

find_package(PkgConfig)
pkg_check_modules(PC_LibHTRA libhtra)
set(LibHTRA_CFLAGS_OTHER ${PC_LibHTRA_CFLAGS_OTHER})

find_path(
    LibHTRA_INCLUDE_DIRS
    NAMES HTRA_API.h htra_api.h
    HINTS $ENV{LibHTRA_DIR}/include
        $ENV{LibHTRA_DIR}/inc
        ${PC_LibHTRA_INCLUDEDIR}
        $ENV{HTRA_ROOT}/include
        $ENV{HTRA_ROOT}/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        /opt/htraapi/inc
        /opt/htraapi/include
    PATHS /usr/local/include
          /usr/include
          /opt/htra/include
          /opt/htra/inc
    PATH_SUFFIXES htra
                  libhtra
                  inc
                  include
)

find_library(
    LibHTRA_LIBRARIES
    NAMES HTRA_API htra_api libHTRA_API htra libhtra_api htraapi
    HINTS $ENV{LibHTRA_DIR}/lib
        $ENV{LibHTRA_DIR}/lib64
        ${PC_LibHTRA_LIBDIR}
        $ENV{HTRA_ROOT}/lib
        $ENV{HTRA_ROOT}/lib64
        ${CMAKE_CURRENT_SOURCE_DIR}/lib
        /opt/htraapi/lib
        /opt/htraapi/lib64
	/opt/htraapi/lib/x86_64
	/opt/htraapi/lib/aarch64
	/opt/htraapi/lib/armv7
	/opt/htraapi/lib/aarch64_gcc7.5
	/opt/htraapi/lib/x86_64_gcc5.4
    PATHS /usr/local/lib
          /usr/lib
          /opt/htra/lib
          /opt/htra/lib64
    PATH_SUFFIXES lib
                  lib64
)

# Try to find version information
if(LibHTRA_INCLUDE_DIRS)
    set(LibHTRA_VERSION_FILE "${LibHTRA_INCLUDE_DIRS}/HTRA_API.h")
    if(EXISTS ${LibHTRA_VERSION_FILE})
        file(STRINGS ${LibHTRA_VERSION_FILE} LibHTRA_VERSION_MAJOR_LINE REGEX "^#define[ \t]+Major_Version[ \t]+[0-9]+")
        file(STRINGS ${LibHTRA_VERSION_FILE} LibHTRA_VERSION_MINOR_LINE REGEX "^#define[ \t]+Minor_Version[ \t]+[0-9]+")
        file(STRINGS ${LibHTRA_VERSION_FILE} LibHTRA_VERSION_PATCH_LINE REGEX "^#define[ \t]+Increamental_Version[ \t]+[0-9]+")
        
        string(REGEX REPLACE "^#define[ \t]+Major_Version[ \t]+([0-9]+).*" "\\1" LibHTRA_VERSION_MAJOR "${LibHTRA_VERSION_MAJOR_LINE}")
        string(REGEX REPLACE "^#define[ \t]+Minor_Version[ \t]+([0-9]+).*" "\\1" LibHTRA_VERSION_MINOR "${LibHTRA_VERSION_MINOR_LINE}")
        string(REGEX REPLACE "^#define[ \t]+Increamental_Version[ \t]+([0-9]+).*" "\\1" LibHTRA_VERSION_PATCH "${LibHTRA_VERSION_PATCH_LINE}")
        
        set(LibHTRA_VERSION "${LibHTRA_VERSION_MAJOR}.${LibHTRA_VERSION_MINOR}.${LibHTRA_VERSION_PATCH}")
    endif()
endif()

# Set version from pkg-config if available
if(NOT LibHTRA_VERSION AND PC_LibHTRA_VERSION)
    set(LibHTRA_VERSION ${PC_LibHTRA_VERSION})
endif()

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LibHTRA_FOUND to TRUE
# if all listed variables are TRUE
# Note that `FOUND_VAR LibHTRA_FOUND` is needed for cmake 3.2 and older.
find_package_handle_standard_args(LibHTRA
                                  FOUND_VAR LibHTRA_FOUND
                                  REQUIRED_VARS LibHTRA_LIBRARIES LibHTRA_INCLUDE_DIRS
                                  VERSION_VAR LibHTRA_VERSION)

mark_as_advanced(LibHTRA_LIBRARIES LibHTRA_INCLUDE_DIRS LibHTRA_CFLAGS_OTHER LibHTRA_VERSION)

# Create imported target if found
if(LibHTRA_FOUND AND NOT TARGET LibHTRA::LibHTRA)
    add_library(LibHTRA::LibHTRA UNKNOWN IMPORTED)
    set_target_properties(LibHTRA::LibHTRA PROPERTIES
        IMPORTED_LOCATION "${LibHTRA_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${LibHTRA_INCLUDE_DIRS}"
        INTERFACE_COMPILE_OPTIONS "${LibHTRA_CFLAGS_OTHER}"
    )
endif()
