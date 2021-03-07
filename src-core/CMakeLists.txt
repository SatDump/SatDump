cmake_minimum_required (VERSION 3.0.0)
include(CheckLibraryExists)
include(CheckIncludeFiles)
include(CheckCSourceCompiles)
include(CMakePushCheckState)
include(CheckCCompilerFlag)

file(GLOB_RECURSE SatDump_core_CPPS  *.cpp 
                                     modules/common/aec/*.c
)

find_package(PkgConfig)

add_library(satdump_core SHARED ${SatDump_core_CPPS})
target_include_directories(satdump_core PUBLIC .)

if(NOT CMAKE_CROSSCOMPILING)
  # Check if host machine can compile with SSE 4.1 intrinsic
  cmake_push_check_state(RESET)
  set(CMAKE_REQUIRED_DEFINITIONS -march=native)
  check_c_source_compiles("
    #include <x86intrin.h>
    int main() {
      __m128i a;
      __m128i b;
      __m128i c = _mm_min_epu16(a, b);
      return 0;
    }" HAVE_SSE)
  cmake_pop_check_state()
endif()

if(HAVE_SSE)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse4.1")
endif()

# Threads
find_package(Threads REQUIRED)
target_link_libraries(satdump_core PUBLIC Threads::Threads)

# LibDSP
find_library(DSP_LIBRARY dsp REQUIRED)
target_link_libraries(satdump_core PUBLIC ${DSP_LIBRARY})

# LibCorrect
find_library(CORRECT_LIBRARY correct REQUIRED)
target_link_libraries(satdump_core PUBLIC ${CORRECT_LIBRARY})

# LibJpeg
find_library(JPEG_LIBRARY jpeg REQUIRED)
target_link_libraries(satdump_core PUBLIC ${JPEG_LIBRARY})

# FMT
find_package(fmt REQUIRED)
target_include_directories(satdump_core PUBLIC ${fmt_INCLUDE_DIRS})
target_link_libraries(satdump_core PUBLIC fmt::fmt)

# FMT
pkg_check_modules(VOLK REQUIRED volk)
target_include_directories(satdump_core PUBLIC ${volk_INCLUDE_DIRS})
target_link_libraries(satdump_core PUBLIC volk)

# LibPNG
if(WIN32 AND NOT MINGW)
    find_package(PNG CONFIG REQUIRED)
else()
    find_package(PNG REQUIRED)
endif()
target_link_libraries(satdump_core PUBLIC PNG::PNG)