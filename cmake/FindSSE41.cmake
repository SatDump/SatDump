#.rst:
# FindSSE41
# ---------
#
# Finds SSE41 support
#
# This module can be used to detect SSE41 support in a C compiler.  If
# the compiler supports SSE41, the flags required to compile with
# SSE41 support are returned in variables for the different languages.
# The variables may be empty if the compiler does not need a special
# flag to support SSE41.
#
# The following variables are set:
#
# ::
#
#    SSE41_C_FLAGS - flags to add to the C compiler for SSE41 support
#    SSE41_FOUND - true if SSE41 is detected
#
#=============================================================================

set(_SSE41_REQUIRED_VARS)
set(CMAKE_REQUIRED_QUIET_SAVE ${CMAKE_REQUIRED_QUIET})
set(CMAKE_REQUIRED_QUIET ${SSE41_FIND_QUIETLY})

# sample SSE41 source code to test
set(SSE41_C_TEST_SOURCE
"
#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <smmintrin.h>
#endif
int foo() {
    __m128i vOne = _mm_set1_epi8(1);
    __m128i result =  _mm_max_epi8(vOne,vOne);
    return _mm_extract_epi8(result, 0);
}
int main(void) { return foo(); }
")

# if these are set then do not try to find them again,
# by avoiding any try_compiles for the flags
if((DEFINED SSE41_C_FLAGS) OR (DEFINED HAVE_SSE41))
else()
  if(WIN32)
    set(SSE41_C_FLAG_CANDIDATES
      #Empty, if compiler automatically accepts SSE41
      " "
      "/arch:SSE2")
  else()
    set(SSE41_C_FLAG_CANDIDATES
      #Empty, if compiler automatically accepts SSE41
      " "
      #GNU, Intel
      "-march=corei7"
      #clang
      "-msse4"
      #GNU 4.4.7 ?
      "-msse4.1"
    )
  endif()

  include(CheckCSourceCompiles)

  foreach(FLAG IN LISTS SSE41_C_FLAG_CANDIDATES)
    set(SAFE_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
    set(CMAKE_REQUIRED_FLAGS "${FLAG}")
    unset(HAVE_SSE41 CACHE)
    if(NOT CMAKE_REQUIRED_QUIET)
      message(STATUS "Try SSE41 C flag = [${FLAG}]")
    endif()
    check_c_source_compiles("${SSE41_C_TEST_SOURCE}" HAVE_SSE41)
    set(CMAKE_REQUIRED_FLAGS "${SAFE_CMAKE_REQUIRED_FLAGS}")
    if(HAVE_SSE41)
      set(SSE41_C_FLAGS_INTERNAL "${FLAG}")
      break()
    endif()
  endforeach()

  unset(SSE41_C_FLAG_CANDIDATES)
  
  set(SSE41_C_FLAGS "${SSE41_C_FLAGS_INTERNAL}"
    CACHE STRING "C compiler flags for SSE41 intrinsics")
endif()

list(APPEND _SSE41_REQUIRED_VARS SSE41_C_FLAGS)

set(CMAKE_REQUIRED_QUIET ${CMAKE_REQUIRED_QUIET_SAVE})

if(_SSE41_REQUIRED_VARS)
  include(FindPackageHandleStandardArgs)

  find_package_handle_standard_args(SSE41
                                    REQUIRED_VARS ${_SSE41_REQUIRED_VARS})

  mark_as_advanced(${_SSE41_REQUIRED_VARS})

  unset(_SSE41_REQUIRED_VARS)
else()
  message(SEND_ERROR "FindSSE41 requires C or CXX language to be enabled")
endif()

# begin tests for SSE4.1 specfic features

set(SSE41_C_TEST_SOURCE_INSERT64
"
#include <stdint.h>
#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <smmintrin.h>
#endif
__m128i foo() {
    __m128i vOne = _mm_set1_epi8(1);
    return _mm_insert_epi64(vOne,INT64_MIN,0);
}
int main(void) { foo(); return 0; }
")

if(SSE41_C_FLAGS)
  include(CheckCSourceCompiles)
  set(SAFE_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
  set(CMAKE_REQUIRED_FLAGS "${SSE41_C_FLAGS}")
  check_c_source_compiles("${SSE41_C_TEST_SOURCE_INSERT64}" HAVE_SSE41_MM_INSERT_EPI64)
  set(CMAKE_REQUIRED_FLAGS "${SAFE_CMAKE_REQUIRED_FLAGS}")
endif()

set(SSE41_C_TEST_SOURCE_EXTRACT64
"
#include <stdint.h>
#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <smmintrin.h>
#endif
int64_t foo() {
    __m128i vOne = _mm_set1_epi8(1);
    return (int64_t)_mm_extract_epi64(vOne,0);
}
int main(void) { return (int)foo(); }
")

if(SSE41_C_FLAGS)
  include(CheckCSourceCompiles)
  set(SAFE_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
  set(CMAKE_REQUIRED_FLAGS "${SSE41_C_FLAGS}")
  check_c_source_compiles("${SSE41_C_TEST_SOURCE_EXTRACT64}" HAVE_SSE41_MM_EXTRACT_EPI64)
  set(CMAKE_REQUIRED_FLAGS "${SAFE_CMAKE_REQUIRED_FLAGS}")
endif()

