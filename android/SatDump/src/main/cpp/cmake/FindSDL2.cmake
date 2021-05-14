# Locate SDL2 library
# This module defines
# SDL2_LIBRARY, the name of the library to link against
# SDL2_FOUND, if false, do not try to link to SDL2
# SDL2_INCLUDE_DIR, where to find SDL.h
#
# This module responds to the the flag:
# SDL2_BUILDING_LIBRARY
# If this is defined, then no SDL2main will be linked in because
# only applications need main().
# Otherwise, it is assumed you are building an application and this
# module will attempt to locate and set the the proper link flags
# as part of the returned SDL2_LIBRARY variable.
#
# Don't forget to include SDLmain.h and SDLmain.m your project for the
# OS X framework based version. (Other versions link to -lSDL2main which
# this module will try to find on your behalf.) Also for OS X, this
# module will automatically add the -framework Cocoa on your behalf.
#
#
# Additional Note: If you see an empty SDL2_LIBRARY_TEMP in your configuration
# and no SDL2_LIBRARY, it means CMake did not find your SDL2 library
# (SDL2.dll, libsdl2.so, SDL2.framework, etc).
# Set SDL2_LIBRARY_TEMP to point to your SDL2 library, and configure again.
# Similarly, if you see an empty SDL2MAIN_LIBRARY, you should set this value
# as appropriate. These values are used to generate the final SDL2_LIBRARY
# variable, but when these values are unset, SDL2_LIBRARY does not get created.
#
#
# $SDL2DIR is an environment variable that would
# correspond to the ./configure --prefix=$SDL2DIR
# used in building SDL2.
# l.e.galup  9-20-02
#
# Modified by Eric Wing.
# Added code to assist with automated building by using environmental variables
# and providing a more controlled/consistent search behavior.
# Added new modifications to recognize OS X frameworks and
# additional Unix paths (FreeBSD, etc).
# Also corrected the header search path to follow "proper" SDL guidelines.
# Added a search for SDL2main which is needed by some platforms.
# Added a search for threads which is needed by some platforms.
# Added needed compile switches for MinGW.
#
# On OSX, this will prefer the Framework version (if found) over others.
# People will have to manually change the cache values of
# SDL2_LIBRARY to override this selection or set the CMake environment
# CMAKE_INCLUDE_PATH to modify the search paths.
#
# Note that the header path has changed from SDL2/SDL.h to just SDL.h
# This needed to change because "proper" SDL convention
# is #include "SDL.h", not <SDL2/SDL.h>. This is done for portability
# reasons because not all systems place things in SDL2/ (see FreeBSD).

#=============================================================================
# Copyright 2003-2009 Kitware, Inc.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

# Modified by sfalexrog to accomodate Android builds. SDL2_LIBRARY
# will be a target which you'll want to link against. It will be built
# statically, mostly because that would save you from needing to
# also add SDL2MAIN_LIBRARY to your target, and will not require you to
# add SDL_android_main.c to your source sets.
# You can override this behavior by changing this file, but as far as
# I know having multiple shared libraries don't really give you any
# benefit on Android.

if (ANDROID)
	set(_SDL_SOURCE_SEARCH_PATHS
        SDL
		deps/SDL
		external/SDL
		libs/SDL
	)

    # This is a hack to allow find_path to actually search for headers and sources
	set(_SDL_CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ${CMAKE_FIND_ROOT_PATH_MODE_INCLUDE})
	set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE "NEVER")

	find_path(_SDL2_SOURCE_DIR SDL.c
		HINTS
		${_SDL_SOURCE_SEARCH_PATHS}
		PATH_SUFFIXES src
		PATHS ${_SDL_SOURCE_SEARCH_PATHS}
	)

	set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ${_SDL_CMAKE_FIND_ROOT_PATH_MODE_INCLUDE})

	if (${_SDL2_SOURCE_DIR} STREQUAL "SDL2_SOURCE_DIR-NOTFOUND")
		message(FATAL_ERROR "Could not find SDL2 source directory, cannot continue")
	endif()

	get_filename_component(_SDL2_PROJECT_DIR ${_SDL2_SOURCE_DIR} DIRECTORY)
	add_subdirectory(${_SDL2_PROJECT_DIR})

    # You can't directly specify a target in target_link_libraries
    # in ${}, but you can't specify a variable name without ${},
    # so here's a little hack to make ${target_name} behave like
    # it should.
	set(SDL2_LIBRARY SDL2 SDL2main)
	set(SDL2_INCLUDE_DIR ${_SDL2_PROJECT_DIR}/include)

else()

    SET(SDL2_SEARCH_PATHS
    	~/Library/Frameworks
    	/Library/Frameworks
    	/usr/local
    	/usr
    	/sw # Fink
    	/opt/local # DarwinPorts
    	/opt/csw # Blastwave
    	/opt
    )

    FIND_PATH(SDL2_INCLUDE_DIR SDL.h
    	HINTS
    	$ENV{SDL2DIR}
    	PATH_SUFFIXES include/SDL2 include
    	PATHS ${SDL2_SEARCH_PATHS}
    )

    FIND_LIBRARY(SDL2_LIBRARY_TEMP
    	NAMES SDL2
    	HINTS
    	$ENV{SDL2DIR}
    	PATH_SUFFIXES lib64 lib
    	PATHS ${SDL2_SEARCH_PATHS}
    )

    IF(NOT SDL2_BUILDING_LIBRARY)
    	IF(NOT ${SDL2_INCLUDE_DIR} MATCHES ".framework")
    		# Non-OS X framework versions expect you to also dynamically link to
    		# SDL2main. This is mainly for Windows and OS X. Other (Unix) platforms
    		# seem to provide SDL2main for compatibility even though they don't
    		# necessarily need it.
    		FIND_LIBRARY(SDL2MAIN_LIBRARY
    			NAMES SDL2main
    			HINTS
    			$ENV{SDL2DIR}
    			PATH_SUFFIXES lib64 lib
    			PATHS ${SDL2_SEARCH_PATHS}
    		)
    	ENDIF(NOT ${SDL2_INCLUDE_DIR} MATCHES ".framework")
    ENDIF(NOT SDL2_BUILDING_LIBRARY)

    # SDL2 may require threads on your system.
    # The Apple build may not need an explicit flag because one of the
    # frameworks may already provide it.
    # But for non-OSX systems, I will use the CMake Threads package.
    IF(NOT APPLE)
    	FIND_PACKAGE(Threads)
    ENDIF(NOT APPLE)

    # MinGW needs an additional library, mwindows
    # It's total link flags should look like -lmingw32 -lSDL2main -lSDL2 -lmwindows
    # (Actually on second look, I think it only needs one of the m* libraries.)
    IF(MINGW)
    	SET(MINGW32_LIBRARY mingw32 CACHE STRING "mwindows for MinGW")
    ENDIF(MINGW)

    IF(SDL2_LIBRARY_TEMP)
    	# For SDL2main
    	IF(NOT SDL2_BUILDING_LIBRARY)
    		IF(SDL2MAIN_LIBRARY)
    			SET(SDL2_LIBRARY_TEMP ${SDL2MAIN_LIBRARY} ${SDL2_LIBRARY_TEMP})
    		ENDIF(SDL2MAIN_LIBRARY)
    	ENDIF(NOT SDL2_BUILDING_LIBRARY)

    	# For OS X, SDL2 uses Cocoa as a backend so it must link to Cocoa.
    	# CMake doesn't display the -framework Cocoa string in the UI even
    	# though it actually is there if I modify a pre-used variable.
    	# I think it has something to do with the CACHE STRING.
    	# So I use a temporary variable until the end so I can set the
    	# "real" variable in one-shot.
    	IF(APPLE)
    		SET(SDL2_LIBRARY_TEMP ${SDL2_LIBRARY_TEMP} "-framework Cocoa")
    	ENDIF(APPLE)

    	# For threads, as mentioned Apple doesn't need this.
    	# In fact, there seems to be a problem if I used the Threads package
    	# and try using this line, so I'm just skipping it entirely for OS X.
    	IF(NOT APPLE)
    		SET(SDL2_LIBRARY_TEMP ${SDL2_LIBRARY_TEMP} ${CMAKE_THREAD_LIBS_INIT})
    	ENDIF(NOT APPLE)

    	# For MinGW library
    	IF(MINGW)
    		SET(SDL2_LIBRARY_TEMP ${MINGW32_LIBRARY} ${SDL2_LIBRARY_TEMP})
    	ENDIF(MINGW)

    	# Set the final string here so the GUI reflects the final state.
    	SET(SDL2_LIBRARY ${SDL2_LIBRARY_TEMP} CACHE STRING "Where the SDL2 Library can be found")
    	# Set the temp variable to INTERNAL so it is not seen in the CMake GUI
    	SET(SDL2_LIBRARY_TEMP "${SDL2_LIBRARY_TEMP}" CACHE INTERNAL "")
    ENDIF(SDL2_LIBRARY_TEMP)

    INCLUDE(FindPackageHandleStandardArgs)

    FIND_PACKAGE_HANDLE_STANDARD_ARGS(SDL2 REQUIRED_VARS SDL2_LIBRARY SDL2_INCLUDE_DIR)
endif()
