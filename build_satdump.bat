@echo off
set VCVARS="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if not exist %VCVARS% (
    echo Error: vcvars64.bat not found at %VCVARS%
    exit /b 1
)

call %VCVARS%

if not exist build mkdir build
cd build

echo Configuring SatDump (StabilityDump) with shared vcpkg...
cmake .. -G "Visual Studio 17 2022" -A x64 ^
  -DBUILD_MSVC=ON ^
  -DCMAKE_TOOLCHAIN_FILE="C:/Users/kslawek/Desktop/SAOISE/SatDump/vcpkg/scripts/buildsystems/vcpkg.cmake" ^
  -DVCPKG_TARGET_TRIPLET=x64-windows ^
  -DPLUGIN_USRP_SDR_SUPPORT=OFF ^
  -DPLUGIN_BLADERF_SDR_SUPPORT=OFF

if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed.
    exit /b %ERRORLEVEL%
)

echo Building SatDump...
cmake --build . --config Release

if %ERRORLEVEL% neq 0 (
    echo Building SatDump failed.
    exit /b %ERRORLEVEL%
)

echo SatDump built successfully.
