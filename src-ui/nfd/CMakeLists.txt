cmake_minimum_required(VERSION 3.10)
project(nativefiledialog-extended VERSION 1.2.1)

set(nfd_ROOT_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(nfd_ROOT_PROJECT ON)
endif ()

set(nfd_PLATFORM Undefined)
if(WIN32)
  set(nfd_PLATFORM PLATFORM_WIN32)
elseif(APPLE)
  set(nfd_PLATFORM PLATFORM_MACOS)
elseif(UNIX AND NOT APPLE)
  set(nfd_PLATFORM PLATFORM_LINUX)
endif()

message("nfd Platform: ${nfd_PLATFORM}")

set(nfd_COMPILER Undefined)
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
  # This is clang-cl, which has different compiler options
  set(nfd_COMPILER COMPILER_CLANGCL)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set(nfd_COMPILER COMPILER_MSVC)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  set(nfd_COMPILER COMPILER_GNU)
endif()

message("nfd Compiler: ${nfd_COMPILER}")

# Use latest C++ by default (should be the best one), but let user override it
if(NOT DEFINED CMAKE_CXX_STANDARD)
  if(CMAKE_VERSION VERSION_LESS "3.12")
    set (CMAKE_CXX_STANDARD 17)
  elseif(CMAKE_VERSION VERSION_LESS "3.20")
    set (CMAKE_CXX_STANDARD 20)
  else()
    set (CMAKE_CXX_STANDARD 23)
  endif()
endif()

########################################

set(TARGET_NAME nfd)

set(PUBLIC_HEADER_FILES
  include/nfd.h
  include/nfd.hpp
  include/nfd_sdl2.h
  include/nfd_glfw3.h)

set(SOURCE_FILES ${PUBLIC_HEADER_FILES})

if(nfd_PLATFORM STREQUAL PLATFORM_WIN32)
  list(APPEND SOURCE_FILES nfd_win.cpp)
endif()

if(nfd_PLATFORM STREQUAL PLATFORM_LINUX)
  find_package(PkgConfig REQUIRED)
  # for Linux, we support GTK3 and xdg-desktop-portal
  option(NFD_PORTAL "Use xdg-desktop-portal instead of GTK" ON)
  if(NOT NFD_PORTAL)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
    message("Using GTK version: ${GTK3_VERSION}")
    list(APPEND SOURCE_FILES nfd_gtk.cpp)
  else()
    pkg_check_modules(DBUS REQUIRED dbus-1)
    message("Using DBUS version: ${DBUS_VERSION}")
    list(APPEND SOURCE_FILES nfd_portal.cpp)
  endif()
endif()

if(nfd_PLATFORM STREQUAL PLATFORM_MACOS)
  # For setting the filter list, macOS introduced allowedContentTypes in version 11.0 and deprecated allowedFileTypes in 12.0.
  # By default (set to ON), NFDe will use allowedContentTypes when targeting macOS >= 11.0.
  # Set this option to OFF to always use allowedFileTypes regardless of the target macOS version.
  # This is mainly needed for applications that are built on macOS >= 11.0 but should be able to run on lower versions
  # and should not be used otherwise.
  option(NFD_USE_ALLOWEDCONTENTTYPES_IF_AVAILABLE "Use allowedContentTypes for filter lists on macOS >= 11.0" ON)

  find_library(APPKIT_LIBRARY AppKit)
  if(NFD_USE_ALLOWEDCONTENTTYPES_IF_AVAILABLE)
    include(CheckCXXSourceCompiles)
    check_cxx_source_compiles(
      "
      #include <Availability.h>
      #if !defined(__MAC_OS_X_VERSION_MIN_REQUIRED) || !defined(__MAC_11_0) || __MAC_OS_X_VERSION_MIN_REQUIRED < __MAC_11_0
      static_assert(false);
      #endif
      int main() { return 0; }
      "
      NFD_USE_ALLOWEDCONTENTTYPES
    )
    if(NFD_USE_ALLOWEDCONTENTTYPES)
      find_library(UNIFORMTYPEIDENTIFIERS_LIBRARY UniformTypeIdentifiers)
      if(NOT UNIFORMTYPEIDENTIFIERS_LIBRARY)
        message(FATAL_ERROR "UniformTypeIdentifiers framework is not available even though we are targeting macOS >= 11.0")
      endif()
    endif()
  endif()
  list(APPEND SOURCE_FILES nfd_cocoa.m)
endif()

# Define the library
add_library(${TARGET_NAME} ${SOURCE_FILES})

# Define alias library to fail early in dependent projects
add_library(${TARGET_NAME}::${TARGET_NAME} ALIAS ${TARGET_NAME})

# Allow includes from include/
target_include_directories(${TARGET_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if(nfd_PLATFORM STREQUAL PLATFORM_LINUX)
  if(NOT NFD_PORTAL)
    target_include_directories(${TARGET_NAME}
      PRIVATE ${GTK3_INCLUDE_DIRS})
    target_link_libraries(${TARGET_NAME}
      PRIVATE ${GTK3_LINK_LIBRARIES})
  else()
    target_include_directories(${TARGET_NAME}
      PRIVATE ${DBUS_INCLUDE_DIRS})
    target_link_libraries(${TARGET_NAME}
      PRIVATE ${DBUS_LINK_LIBRARIES})
    target_compile_definitions(${TARGET_NAME}
      PUBLIC NFD_PORTAL)
  endif()

  option(NFD_APPEND_EXTENSION "Automatically append file extension to an extensionless selection in SaveDialog()" OFF)
  if(NFD_APPEND_EXTENSION)
    target_compile_definitions(${TARGET_NAME} PRIVATE NFD_APPEND_EXTENSION)
  endif()
  option(NFD_CASE_SENSITIVE_FILTER "Make filters case sensitive" OFF)
  if(NFD_CASE_SENSITIVE_FILTER)
    target_compile_definitions(${TARGET_NAME} PRIVATE NFD_CASE_SENSITIVE_FILTER)
  endif()
endif()

if(nfd_PLATFORM STREQUAL PLATFORM_MACOS)
  if(NFD_USE_ALLOWEDCONTENTTYPES)
    target_link_libraries(${TARGET_NAME} PRIVATE ${APPKIT_LIBRARY} ${UNIFORMTYPEIDENTIFIERS_LIBRARY})
    target_compile_definitions(${TARGET_NAME} PRIVATE NFD_MACOS_ALLOWEDCONTENTTYPES=1)
  else()
    target_link_libraries(${TARGET_NAME} PRIVATE ${APPKIT_LIBRARY})
    target_compile_definitions(${TARGET_NAME} PRIVATE NFD_MACOS_ALLOWEDCONTENTTYPES=0)
  endif()
endif()

if(nfd_COMPILER STREQUAL COMPILER_MSVC)
  string(REPLACE "/EHsc" "/EHs-c-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  string(REPLACE "/GR" "/GR-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  set_property(TARGET ${TARGET_NAME} APPEND_STRING PROPERTY STATIC_LIBRARY_OPTIONS /NODEFAULTLIB)
endif()

if(nfd_COMPILER STREQUAL COMPILER_CLANGCL)
  string(REPLACE "/EHsc" "/EHs-c-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  string(REPLACE "/GR" "/GR-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
endif()

if(nfd_COMPILER STREQUAL COMPILER_GNU)
  target_compile_options(${TARGET_NAME} PRIVATE -nostdlib -fno-exceptions -fno-rtti)
endif()

set_target_properties(${TARGET_NAME} PROPERTIES
                                       PUBLIC_HEADER "${PUBLIC_HEADER_FILES}"
                                       VERSION ${PROJECT_VERSION}
                                       SOVERSION ${PROJECT_VERSION_MAJOR})

option(NFD_OVERRIDE_RECENT_WITH_DEFAULT "Use defaultPath instead of recent folder on Windows" OFF)
if (NFD_OVERRIDE_RECENT_WITH_DEFAULT)
    target_compile_definitions(${TARGET_NAME} PRIVATE NFD_OVERRIDE_RECENT_WITH_DEFAULT)
endif()
