cmake_minimum_required(VERSION 3.6)

project(ImGuiExample)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

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

add_subdirectory(.. ${CMAKE_BINARY_DIR}/all_libs)

add_library(android_imgui SHARED
  ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui_image.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/backends/imgui_impl_android.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
  ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
)

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

target_compile_definitions(android_imgui PRIVATE IMGUI_IMPL_OPENGL_ES3)

target_include_directories(android_imgui PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/backends
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

