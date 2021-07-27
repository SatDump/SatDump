cmake_minimum_required(VERSION 3.3)

project(satdump_android)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX14_EXTENSION_COMPILE_OPTION "-std=c++17")

# Build in release mod so it isn't unusably slow
# We have to disable fortify... Or some stuff doesn't build
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -D_FORTIFY_SOURCE=0")
set(CMAKE_CXX_FLAGS_DEBUG   "-O3 -D_FORTIFY_SOURCE=0")
set(CMAKE_C_FLAGS_RELEASE   "-O3 -D_FORTIFY_SOURCE=0")
set(CMAKE_C_FLAGS_DEBUG     "-O3 -D_FORTIFY_SOURCE=0")

#set(CFLAGS "${CFLAGS} -D__ANDROID_API__=29")
#set(CMAKE_CXX_FLAGS "-fPIE ${CMAKE_CXX_FLAGS}")
#set(LINK_LLOG_ANDROID "-fPIE -pie -Wl,-Bdynamic -L{ANDROID_SYSROOT}/usr/lib -llog")
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

find_package(SDL2 REQUIRED)

# Use GLES2/GLES3 implementations for Android, GL3 for everything else
if (ANDROID)
    set(GL_PROFILES "GL_PROFILE_GLES2" "GL_PROFILE_GLES3")
else()
    set(GL_PROFILES "GL_PROFILE_GL3")
endif()

file(GLOB_RECURSE SATDUMP_ANDROID_FILES
    src/*.cpp
)

set(SATDUMP_ANDROID_SOURCES ${IMGUI_FILES} ${SATDUMP_ANDROID_FILES} ${GLLOAD_FILES})

# Build as a library for Android, as an executable otherwise
add_library(satdump_android SHARED ${SATDUMP_ANDROID_SOURCES})

target_link_libraries(satdump_android PUBLIC ${SDL2_LIBRARY} GLESv2)
target_include_directories(satdump_android PRIVATE src/)
target_include_directories(satdump_android PRIVATE ${SDL2_INCLUDE_DIR})
target_compile_definitions(satdump_android PRIVATE ${GL_PROFILES})

# LIBJPEG
add_library(zlib STATIC IMPORTED)
set_target_properties(zlib PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/deps/zlib/${ANDROID_ABI}/libz.a)
include_directories(deps/zlib/jni/)

# LIBPNG
add_library(png STATIC IMPORTED)
set_target_properties(png PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/deps/libpng/${ANDROID_ABI}/libpng.a)
#target_link_libraries(png zlib)
include_directories(deps/libpng/jni/)

# LIBVOLK
add_library(volk STATIC IMPORTED)
set_target_properties(volk PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/deps/volk/${ANDROID_ABI}/libvolk.a)
include_directories(deps/volk/jni/)

# LIBCPPFEATURES
add_library(cpufeatures STATIC IMPORTED)
set_target_properties(cpufeatures PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/deps/volk/${ANDROID_ABI}/libcpu_features.a)
include_directories(deps/volk/jni/)

# LIBCORRECT
add_library(correct STATIC IMPORTED)
set_target_properties(correct PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/deps/correct/${ANDROID_ABI}/libcorrect.a)
include_directories(deps/correct/jni/)

# LIBJPEG
add_library(jpeg STATIC IMPORTED)
set_target_properties(jpeg PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/deps/jpeg/${ANDROID_ABI}/libjpeg.a)
include_directories(deps/jpeg/jni)

# LIBFFTW3
add_library(fftw STATIC IMPORTED)
set_target_properties(fftw PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/deps/fftw3/${ANDROID_ABI}/libfftw3f.a)
include_directories(deps/fftw3/jni)

# LIBNNG
#add_library(nng STATIC IMPORTED)
#set_target_properties(nng PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/deps/nng/${ANDROID_ABI}/libnng.a)
#include_directories(deps/nng/jni)

# Enable live processing for remove devices. WIP
set(BUILD_LIVE ON)
if(BUILD_LIVE)
    add_subdirectory(deps/libusb)
    include_directories(deps/libusb)

    add_subdirectory(deps/librtlsdr)
    include_directories(deps/librtlsdr)

    add_subdirectory(deps/libairspy)
    include_directories(deps/libairspy)

    add_subdirectory(../../../../../libsdr ${CMAKE_BINARY_DIR}/libsdr)
endif()


#add_subdirectory(src-core)
add_subdirectory(../../../../../src-core ${CMAKE_BINARY_DIR}/src-core)
target_include_directories(satdump_android PRIVATE ../../../../../src-core)

add_subdirectory(../../../../../src-interface ${CMAKE_BINARY_DIR}/src-interface)
target_include_directories(satdump_android PRIVATE ../../../../../src-core)

target_link_libraries(satdump_android PUBLIC satdump_core satdump_interface)

add_library(app-glue STATIC ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
target_include_directories(satdump_android PRIVATE ${ANDROID_NDK}/sources/android/native_app_glue/)

# You need to link static libraries against your shared native library.
target_link_libraries(satdump_android PUBLIC app-glue ${log-lib})

# LIBRTL-SDR
#add_library(rtlsdr STATIC IMPORTED)
#set_target_properties(rtlsdr PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/deps/rtlsdr/${ANDROID_ABI}/librtlsdr.a)
#include_directories(deps/rtlsdr/jni/)

# LIBRTL-SDR
#add_library(libusb STATIC IMPORTED)
#set_target_properties(libusb PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/deps/libusb/${ANDROID_ABI}/libusb1.0.a)
#include_directories(deps/libusb/jni/)

#target_link_libraries(satdump_android PUBLIC rtlsdr libusb)
