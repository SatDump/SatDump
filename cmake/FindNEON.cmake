#.rst:
# FindNEON
# --------
#
# Finds NEON support
#
# This module can be used to detect NEON support in a C compiler.  If
# the compiler supports NEON, the flags required to compile with
# NEON support are returned in variables for the different languages.
# The variables may be empty if the compiler does not need a special
# flag to support NEON.
#
# The following variables are set:
#
# ::
#
#    NEON_C_FLAGS - flags to add to the C compiler for NEON support
#    NEON_FOUND - true if NEON is detected
#
#=============================================================================

set(_NEON_REQUIRED_VARS)
set(CMAKE_REQUIRED_QUIET_SAVE ${CMAKE_REQUIRED_QUIET})
set(CMAKE_REQUIRED_QUIET ${NEON_FIND_QUIETLY})

# sample NEON source code to test
set(NEON_C_TEST_SOURCE
"
#include <arm_neon.h>
uint32x4_t double_elements(uint32x4_t input)
{
    return(vaddq_u32(input, input));
}
int main(void)
{
    uint32x4_t one;
    uint32x4_t two = double_elements(one);
    return 0;
}
")

# if these are set then do not try to find them again,
# by avoiding any try_compiles for the flags
if((DEFINED NEON_C_FLAGS) OR (DEFINED HAVE_NEON))
else()
  if(WIN32)
    set(NEON_C_FLAG_CANDIDATES
      #Empty, if compiler automatically accepts NEON
      " ")
  else()
    set(NEON_C_FLAG_CANDIDATES
      #Empty, if compiler automatically accepts NEON
      " "
      "-mfpu=neon"
      "-mfpu=neon -mfloat-abi=softfp"
    )
  endif()

  include(CheckCSourceCompiles)

  foreach(FLAG IN LISTS NEON_C_FLAG_CANDIDATES)
    set(SAFE_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
    set(CMAKE_REQUIRED_FLAGS "${FLAG}")
    unset(HAVE_NEON CACHE)
    if(NOT CMAKE_REQUIRED_QUIET)
      message(STATUS "Try NEON C flag = [${FLAG}]")
    endif()
    check_c_source_compiles("${NEON_C_TEST_SOURCE}" HAVE_NEON)
    set(CMAKE_REQUIRED_FLAGS "${SAFE_CMAKE_REQUIRED_FLAGS}")
    if(HAVE_NEON)
      set(NEON_C_FLAGS_INTERNAL "${FLAG}")
      break()
    endif()
  endforeach()

  unset(NEON_C_FLAG_CANDIDATES)
  
  set(NEON_C_FLAGS "${NEON_C_FLAGS_INTERNAL}"
    CACHE STRING "C compiler flags for NEON intrinsics")
endif()

list(APPEND _NEON_REQUIRED_VARS NEON_C_FLAGS)

set(CMAKE_REQUIRED_QUIET ${CMAKE_REQUIRED_QUIET_SAVE})

if(_NEON_REQUIRED_VARS)
  include(FindPackageHandleStandardArgs)

  find_package_handle_standard_args(NEON
                                    REQUIRED_VARS ${_NEON_REQUIRED_VARS})

  mark_as_advanced(${_NEON_REQUIRED_VARS})

  unset(_NEON_REQUIRED_VARS)
else()
  message(SEND_ERROR "FindNEON requires C or CXX language to be enabled")
endif()

