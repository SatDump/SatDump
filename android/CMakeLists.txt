cmake_minimum_required(VERSION 3.6)

project(ImGuiExample)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Build in release mod so it isn't unusably slow
set(CMAKE_CXX_FLAGS_RELEASE         "-O3")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO  "-O3")
set(CMAKE_CXX_FLAGS_DEBUG           "-O3")
set(CMAKE_C_FLAGS_RELEASE           "-O3")
set(CMAKE_C_FLAGS_RELWITHDEBINFO    "-O3")
set(CMAKE_C_FLAGS_DEBUG             "-O3")

###########################################################################################################################################
# LIBVOLK
add_library(volk STATIC IMPORTED)
set_target_properties(volk PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/deps/volk/${ANDROID_ABI}/libvolk.a)
include_directories(deps/volk/jni/)

# LIBFFTW3
add_library(fftw STATIC IMPORTED)
set_target_properties(fftw PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/deps/fftw3/${ANDROID_ABI}/libfftw3f.a)
include_directories(deps/fftw3/jni)

# LIBPNG
add_library(png STATIC IMPORTED)
set_target_properties(png PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/deps/libpng/${ANDROID_ABI}/libpng.a)
#target_link_libraries(png zlib)
include_directories(deps/libpng/jni/)

# LIBCPPFEATURES
add_library(cpufeatures STATIC IMPORTED)
set_target_properties(cpufeatures PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/deps/volk/${ANDROID_ABI}/libcpu_features.a)
include_directories(deps/volk/jni/)

# ZLIB
add_library(zlib STATIC IMPORTED)
set_target_properties(zlib PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/deps/zlib/${ANDROID_ABI}/libz.a)
include_directories(deps/zlib/jni/)

# LIBJPEG
add_library(jpeg STATIC IMPORTED)
set_target_properties(jpeg PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/deps/jpeg/${ANDROID_ABI}/libjpeg.a)
include_directories(deps/jpeg/jni)

include_directories(android_imgui PRIVATE
  ${ANDROID_NDK}/sources/android/native_app_glue
)
###########################################################################################################################################

# LibUSB
file(GLOB_RECURSE libusb_cpps deps/libusb/*.c)
add_library(usb STATIC ${libusb_cpps})
target_include_directories(usb PRIVATE deps/libusb/)

# LibAirspy
add_library(airspy STATIC deps/libairspy/airspy.c deps/libairspy/iqconverter_float.c deps/libairspy/iqconverter_int16.c)
target_include_directories(airspy PRIVATE deps/libairspy/ deps/libusb)
target_link_libraries(airspy PRIVATE usb)

# LibAirspyHF
add_library(airspyhf STATIC deps/libairspyhf/airspyhf.c deps/libairspyhf/iqbalancer.c)
target_include_directories(airspyhf PRIVATE deps/libairspyhf/ deps/libusb)
target_link_libraries(airspyhf PRIVATE usb)

# LibHackRF
add_library(hackrf STATIC deps/libhackrf/hackrf.c)
target_include_directories(hackrf PRIVATE deps/libhackrf/ deps/libusb)
target_link_libraries(hackrf PRIVATE usb)

###########################################################################################################################################



add_subdirectory(.. ${CMAKE_BINARY_DIR}/all_libs)

add_library(android_imgui SHARED
  ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui_image.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui_backends/imgui_impl_android.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui_backends/imgui_impl_opengl3.cpp
  ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
)

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

target_compile_definitions(android_imgui PRIVATE IMGUI_IMPL_OPENGL_ES3)

target_include_directories(android_imgui PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui_backends
  ${ANDROID_NDK}/sources/android/native_app_glue
  ${CMAKE_CURRENT_SOURCE_DIR}/../src-core/
)

target_link_libraries(android_imgui PRIVATE
  android
  EGL
  GLESv3
  log
  satdump_core
  satdump_interface
)

