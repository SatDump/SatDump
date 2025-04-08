cmake_minimum_required(VERSION 3.12)
include(CheckLibraryExists)
include(CheckIncludeFileCXX)
include(CheckCSourceCompiles)
include(CMakePushCheckState)
include(CheckCCompilerFlag)

# Set the resources path
if(MSVC OR BUILD_MSVC)
    set(RESOURCES_PATH "./")
    set(LIBRARIES_PATH "./")
else()
    set(RESOURCES_PATH "${CMAKE_INSTALL_PREFIX}/share/satdump/")
    set(LIBRARIES_PATH "${CMAKE_INSTALL_FULL_LIBDIR}/satdump/")
endif()

configure_file(${PROJECT_SOURCE_DIR}/src-core/satdump_varst.cxx ${CMAKE_CURRENT_BINARY_DIR}/satdump_varst.cpp @ONLY)

file(GLOB_RECURSE SatDump_core_CPPS *.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/satdump_varst.cpp
    libs/aec/*.c
    libs/predict/*.c
    libs/bzlib/*.c
    libs/miniz/*.c
    libs/libtom/*.c
    libs/jpeg/*.c
    libs/jpeg12/*.c
    libs/dlfcn/dlfcn.c
    common/map/maidenhead.c
    libs/correct/*.c
    libs/openjp2/*.c
    libs/deepspace-turbo/*.c
    libs/bzlib/*.c
    libs/tiny-regex-c/*.c
    libs/lua/onelua.c
)

################################################################
####### AngelScript START
################################################################
# Exclude AngelScript library files
# file(GLOB_RECURSE SatDump_core_angelscript_CPPS angelscript/lib/*.cpp)
#foreach(ii IN ITEMS ${SatDump_core_angelscript_CPPS})
#    list(REMOVE_ITEM SatDump_core_CPPS ${ii})
#endforeach()

set(ARCHFLAGS "${CMAKE_SYSTEM_PROCESSOR}")
if(APPLE AND NOT IOS)
    if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
        set(ARCHFLAGS "aarch64")
    endif()
endif()

if(MSVC AND CMAKE_CL_64)
    enable_language(ASM_MASM)
    if(CMAKE_ASM_MASM_COMPILER_WORKS)
        set(ANGELSCRIPT_SOURCE ${ANGELSCRIPT_SOURCE} angelscript/lib/as_callfunc_x64_msvc_asm.asm)
    else()
        message(FATAL ERROR "MSVC x86_64 target requires a working assembler")
    endif()
endif()

if(${ARCHFLAGS} MATCHES "^arm")
    enable_language(ASM)
    if(CMAKE_ASM_COMPILER_WORKS)
        set(ANGELSCRIPT_SOURCE ${ANGELSCRIPT_SOURCE} angelscript/lib/as_callfunc_arm.cpp angelscript/lib/as_callfunc_arm_gcc.S)
        set_property(SOURCE as_callfunc_arm_gcc.S APPEND PROPERTY COMPILE_FLAGS " -Wa,-mimplicit-it=always")
    else()
        message(FATAL ERROR "ARM target requires a working assembler")
    endif()
endif()

if(${ARCHFLAGS} MATCHES "^aarch64")
    enable_language(ASM)
    if(CMAKE_ASM_COMPILER_WORKS)
        if(NOT APPLE)
            set(ANGELSCRIPT_SOURCE ${ANGELSCRIPT_SOURCE} angelscript/lib/as_callfunc_arm64.cpp angelscript/lib/as_callfunc_arm64_gcc.S)
        else()
            set(ANGELSCRIPT_SOURCE ${ANGELSCRIPT_SOURCE} angelscript/lib/as_callfunc_arm64.cpp angelscript/lib/as_callfunc_arm64_xcode.S)
        endif()
    else()
        message(FATAL ERROR "ARM target requires a working assembler")
    endif()
endif()

if(${ARCHFLAGS} MATCHES "riscv64")
    enable_language(ASM)
    if(CMAKE_ASM_COMPILER_WORKS)
        set(ANGELSCRIPT_SOURCE ${ANGELSCRIPT_SOURCE} angelscript/lib/as_callfunc_riscv64.cpp angelscript/lib/as_callfunc_riscv64_gcc.S)
    else()
        message(FATAL ERROR "RISC-V 64 target requires a working assembler")
    endif()
endif()

if(${ARCHFLAGS} MATCHES "^e2k")
    enable_language(ASM)
    if(CMAKE_ASM_COMPILER_WORKS)
        set(ANGELSCRIPT_SOURCE ${ANGELSCRIPT_SOURCE} angelscript/lib/as_callfunc_e2k.cpp angelscript/lib/as_callfunc_e2k.S)
    else()
        message(FATAL ERROR "E2K target requires a working assembler")
    endif()
endif()

set(SatDump_core_CPPS ${SatDump_core_CPPS} ${ANGELSCRIPT_SOURCE})
unset(ANGELSCRIPT_SOURCE)
################################################################
####### AngelScript STOP
################################################################

find_package(PkgConfig)

add_library(satdump_core SHARED ${SatDump_core_CPPS})
target_include_directories(satdump_core PUBLIC .)

# We have to... For MSVC
if(BUILD_OPENCL)
    target_compile_definitions(satdump_core PUBLIC USE_OPENCL=1)
endif()

# Threads
find_package(Threads REQUIRED)
target_link_libraries(satdump_core PUBLIC Threads::Threads)

# Armadillo
find_package(Armadillo)

if(ARMADILLO_FOUND)
    target_compile_definitions(satdump_core PUBLIC HAVE_ARMADILLO="1")
    target_link_libraries(satdump_core PUBLIC ${ARMADILLO_LIBRARIES})
    target_include_directories(satdump_core PUBLIC ${ARMADILLO_INCLUDE_DIRS})
    message("-- Building with Armadillo!")
endif()

# OpenCL if we found it
if(BUILD_OPENCL AND NOT ANDROID)
    target_link_libraries(satdump_core PUBLIC ${OpenCL_LIBRARIES})
endif()

# OpenMP Linking for macOS
if(APPLE AND BUILD_OPENMP)
    target_link_libraries(satdump_core PUBLIC omp)
endif()

if(BUILD_ZIQ2)
    target_compile_definitions(satdump_core PUBLIC BUILD_ZIQ2="1")
endif()

if(MSVC)
    # Jemalloc: TODO
    target_link_libraries(satdump_core PUBLIC shlwapi)
    target_link_libraries(satdump_core PUBLIC volk.dll)
    target_link_libraries(satdump_core PUBLIC libpng16.dll)
    target_link_libraries(satdump_core PUBLIC fftw3f.dll)
    target_link_libraries(satdump_core PUBLIC nng.dll)
    target_link_libraries(satdump_core PUBLIC tiff.dll)
    target_link_libraries(satdump_core PUBLIC libcurl.dll)

    if(BUILD_ZIQ)
        target_link_libraries(satdump_core PUBLIC zstd.dll)
        target_compile_definitions(satdump_core PUBLIC BUILD_ZIQ="1")
    endif()

    target_include_directories(satdump_core PUBLIC libs/lua)
    target_link_libraries(satdump_core PUBLIC ws2_32)
elseif(ANDROID)
    # Jemalloc
    # TODO

    # VOLK
    target_link_libraries(satdump_core PUBLIC volk cpufeatures m log dl)
    target_include_directories(satdump_core PUBLIC ../volk_includes)

    # LibPNG
    target_link_libraries(satdump_core PUBLIC png zlib)

    # FFTW3
    target_link_libraries(satdump_core PUBLIC fftw)

    # NNG
    target_link_libraries(satdump_core PUBLIC nng)

    # cURL
    target_link_libraries(satdump_core PUBLIC curl wolfssl)

    # Libzstd
    target_link_libraries(satdump_core PUBLIC zstd)
    target_compile_definitions(satdump_core PUBLIC BUILD_ZIQ="1")

    # Tiff
    target_link_libraries(satdump_core PUBLIC tiff)

    target_include_directories(satdump_core PUBLIC libs/lua)
else()
    if(NOT APPLE)
        target_link_libraries(satdump_core PUBLIC stdc++fs)

        # Jemalloc
        find_package(Jemalloc REQUIRED)
        target_include_directories(satdump_core PUBLIC ${JEMALLOC_INCLUDE_DIR})
        target_link_libraries(satdump_core PUBLIC ${JEMALLOC_LIBRARY})
    endif()

    # VOLK
    pkg_check_modules(VOLK REQUIRED volk)
    check_cxx_source_compiles("#include <volk/volk_alloc.hh>\n int main() { return 0; }" VOLK_HAS_ALLOC)

    if(NOT VOLK_HAS_ALLOC)
        message(STATUS "VOLK does not have volk_alloc.hh, including volk_includes")
        target_include_directories(satdump_core PUBLIC ../volk_includes) # If we're running VOLK 1.0, include our own volk_alloc
    endif()

    target_include_directories(satdump_core PUBLIC ${volk_INCLUDE_DIRS})
    target_link_libraries(satdump_core PUBLIC volk)
    set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} volk) # We need to test stuff in volk later
    set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES} ${volk_INCLUDE_DIRS}) # We need to test stuff in volk later

    # LibPNG
    if(WIN32 AND NOT MINGW)
        find_package(PNG CONFIG REQUIRED)
    else()
        find_package(PNG REQUIRED)
    endif()

    target_link_libraries(satdump_core PUBLIC PNG::PNG)

    # FFTW3
    pkg_check_modules(FFTW3 REQUIRED fftw3f)
    target_link_libraries(satdump_core PUBLIC ${FFTW3_LIBRARIES})

    # NNG
    find_library(NNG_LIBRARY nng REQUIRED)
    target_link_libraries(satdump_core PUBLIC ${NNG_LIBRARY})

    if(BUILD_ZIQ)
        # zstd
        find_library(ZSTD_LIBRARY zstd REQUIRED)
        target_link_libraries(satdump_core PUBLIC ${ZSTD_LIBRARY})
        target_compile_definitions(satdump_core PUBLIC BUILD_ZIQ="1")
    endif()

    # Lua
    target_include_directories(satdump_core PUBLIC libs/lua)

    # Curl
    find_package(CURL REQUIRED)

    if(CURL_FOUND)
        target_link_libraries(satdump_core PUBLIC ${CURL_LIBRARIES})
        target_include_directories(satdump_core PUBLIC ${CURL_INCLUDE_DIRS})
        message("-- Building with curl!")
    endif()

    # Dynamic linking
    target_link_libraries(satdump_core PUBLIC dl)

    # Tiff
    find_library(TIFF_LIBRARY tiff REQUIRED)
    target_link_libraries(satdump_core PUBLIC ${TIFF_LIBRARY})

    include(CheckCXXSourceCompiles)

    if(NOT APPLE)
        # Check system has volk_32fc_x2_add_32fc
        check_cxx_source_compiles("#include <volk/volk.h>\n int main() { volk_32fc_x2_add_32fc(nullptr,nullptr,nullptr,0); return 0; }" VOLK_HAS_volk_32fc_x2_add_32fc)

        if(NOT VOLK_HAS_volk_32fc_x2_add_32fc)
            target_compile_definitions(satdump_core PUBLIC VOLK_NO_volk_32fc_x2_add_32fc="1")
        endif()
    endif()
endif()

if(APPLE)
    target_link_libraries(satdump_core PUBLIC "-framework CoreGraphics")
endif()

# Install
if(MSVC OR BUILD_MSVC)
    install(TARGETS satdump_core DESTINATION ${CMAKE_INSTALL_BINDIR} RUNTIME_DEPENDENCY_SET satdump_deps)
else()
    install(TARGETS satdump_core DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

install(DIRECTORY . DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/satdump FILES_MATCHING PATTERN "*.h")
install(DIRECTORY . DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/satdump FILES_MATCHING PATTERN "*.hpp")
