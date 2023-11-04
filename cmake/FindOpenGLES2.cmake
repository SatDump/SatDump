#-------------------------------------------------------------------
# Modified from https://github.com/freeminer/freeminer/blob/master/cmake/Modules/FindOpenGLES2.cmake
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# - Try to find OpenGLES and EGL
# Once done this will define
#
#  OPENGLES2_FOUND        - system has OpenGLES
#  OPENGLES2_INCLUDE_DIR  - the GL include directory
#  OPENGLES2_LIBRARIES    - Link these to use OpenGLES

if(WIN32)
	find_path(OPENGLES2_INCLUDE_DIR GLES2/gl2.h)
	find_library(OPENGLES2_LIBRARY libGLESv2)
else()
	find_path(OPENGLES2_INCLUDE_DIR GLES2/gl2.h
		PATHS /usr/openwin/share/include
			/opt/graphics/OpenGL/include
			/opt/vc/include
			/usr/X11R6/include
			/usr/include
	)

	find_library(OPENGLES2_LIBRARY
		NAMES GLESv2
		PATHS /opt/graphics/OpenGL/lib
			/usr/openwin/lib
			/usr/X11R6/lib
			/usr/shlib /usr/X11R6/lib
			/opt/vc/lib
			/usr/lib/aarch64-linux-gnu
			/usr/lib/arm-linux-gnueabihf
			/usr/lib
	)

	find_library(OPENGLES1_gl_LIBRARY
		NAMES GLESv1_CM
		PATHS /opt/graphics/OpenGL/lib
			/usr/openwin/lib
			/usr/X11R6/lib
			/usr/shlib /usr/X11R6/lib
			/opt/vc/lib
			/usr/lib/aarch64-linux-gnu
			/usr/lib/arm-linux-gnueabihf
			/usr/lib
	)

	include(FindPackageHandleStandardArgs)
	find_package_handle_standard_args(OpenGLES2 DEFAULT_MSG OPENGLES2_LIBRARY OPENGLES2_INCLUDE_DIR)
endif()

set(OPENGLES2_LIBRARIES ${OPENGLES2_LIBRARIES} ${OPENGLES2_LIBRARY} ${OPENGLES1_gl_LIBRARY})

mark_as_advanced(
	OPENGLES2_INCLUDE_DIR
	OPENGLES2_LIBRARY
	OPENGLES1_gl_LIBRARY
)
