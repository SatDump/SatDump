#
# Find the JEMALLOC client includes and library
# Source: https://gist.github.com/Lewiscowles1986/225a10154637d18d6187147015c8d0f7
# 

# This module defines
# JEMALLOC_INCLUDE_DIR, where to find jemalloc.h
# JEMALLOC_LIBRARIES, the libraries to link against
# JEMALLOC_FOUND, if false, you cannot build anything that requires JEMALLOC

# also defined, but not for general use are
# JEMALLOC_LIBRARY, where to find the JEMALLOC library.

set( JEMALLOC_FOUND 0 )

if ( UNIX )
  FIND_PATH( JEMALLOC_INCLUDE_DIR
    NAMES
      jemalloc/jemalloc.h
    PATHS
      /usr/include
      /usr/include/jemalloc
      /usr/local/include
      /usr/local/include/jemalloc
      $ENV{JEMALLOC_ROOT}
      $ENV{JEMALLOC_ROOT}/include
      ${CMAKE_SOURCE_DIR}/externals/jemalloc
  DOC
    "Specify include-directories that might contain jemalloc.h here."
  )
  FIND_LIBRARY( JEMALLOC_LIBRARY 
    NAMES
      jemalloc libjemalloc JEMALLOC
    PATHS
      /usr/lib
      /usr/lib/jemalloc
      /usr/local/lib
      /usr/local/lib/jemalloc
      /usr/local/jemalloc/lib
      $ENV{JEMALLOC_ROOT}/lib
      $ENV{JEMALLOC_ROOT}
    DOC "Specify library-locations that might contain the jemalloc library here."
  )

  if ( JEMALLOC_LIBRARY )
    if ( JEMALLOC_INCLUDE_DIR )
      set( JEMALLOC_FOUND 1 )
      message( STATUS "Found JEMALLOC library: ${JEMALLOC_LIBRARY}")
      message( STATUS "Found JEMALLOC headers: ${JEMALLOC_INCLUDE_DIR}")
    else ( JEMALLOC_INCLUDE_DIR )
      message(FATAL_ERROR "Could not find jemalloc headers! Please install jemalloc libraries and headers")
    endif ( JEMALLOC_INCLUDE_DIR )
  endif ( JEMALLOC_LIBRARY )

  mark_as_advanced( JEMALLOC_FOUND JEMALLOC_LIBRARY JEMALLOC_EXTRA_LIBRARIES JEMALLOC_INCLUDE_DIR )
endif (UNIX)
