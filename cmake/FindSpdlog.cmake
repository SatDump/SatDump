# This module try to find spdlog lib
# Once done this will define
#
#  Spdlog_FOUND - system has spdlog lib
#  SPDLOG_INCLUDE_DIR - the spdlog include directory
#
# Available targets:
#  Spdlog::Spdlog

find_path(SPDLOG_INCLUDE_DIR spdlog/spdlog.h
  PATHS
    ENV SPDLOG_ROOT
    ENV SPDLOG_INCLUDE_DIR
    ${SPDLOG_ROOT}
    /usr
    /usr/local
    $ENV{HOME}/.local
  PATH_SUFFIXES
    include
  )
mark_as_advanced(SPDLOG_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Spdlog
  REQUIRED_VARS SPDLOG_INCLUDE_DIR
  )

if(Spdlog_FOUND AND NOT TARGET Spdlog::Spdlog)
  add_library(Spdlog::Spdlog INTERFACE IMPORTED)
  set_target_properties(Spdlog::Spdlog PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${SPDLOG_INCLUDE_DIR}"
    )
endif()
