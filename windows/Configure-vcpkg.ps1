param([string]$platform="x64-windows") #or x86-windows, arm64-windows
$ErrorActionPreference = "Stop"
$PSDefaultParameterValues['*:ErrorAction']='Stop'

if(!!(Get-Command 'tf' -ErrorAction SilentlyContinue) -eq $false -and $Env:GITHUB_WORKSPACE -eq $null)
{
    Write-Error "You must run this script within Developer Powershell for Visual Studio"
    exit 1
}
if(Test-Path "$(Split-Path -Parent $MyInvocation.MyCommand.Path)\..\vcpkg" -ErrorAction SilentlyContinue)
{
    Write-Output "$(Split-Path -Parent $MyInvocation.MyCommand.Path)\..\vcpkg already found! Not setting up."
    exit 1
}

if($platform -eq "x64-windows")
{
    $generator = "x64"
    $arch = "AMD64"
    $sdrplay_arch = "x64"
}
elseif($platform -eq "x86-windows")
{
    $generator = "Win32"
    $arch = "X86"
    $sdrplay_arch = "x86"
}
elseif($platform -eq "arm64-windows")
{
    $generator = "ARM64"
    $arch = "ARM64"
    $sdrplay_arch = "arm64"
}
else
{
    Write-Error "Unsupported platform: $platform"
    exit 1
}

if($env:PROCESSOR_ARCHITECTURE -ne $arch)
{
    if($env:PROCESSOR_ARCHITECTURE -eq "AMD64")
    {
        $host_triplet = "x64-windows"
    }
    elseif($env:PROCESSOR_ARCHITECTURE -eq "X86")
    {
        $host_triplet = "x86-windows"
    }
    elseif($env:PROCESSOR_ARCHITECTURE -eq "ARM64")
    {
        $host_triplet = "arm64-windows"
    }
    else
    {
        Write-Error "Unsupported host platform: $($env:PROCESSOR_ARCHITECTURE)"
        exit 1
    }
}

#Setup vcpkg
Write-Output "Configuring vcpkg..."
cd "$(Split-Path -Parent $MyInvocation.MyCommand.Path)\.."
git clone https://github.com/microsoft/vcpkg -b 2025.01.13
cd vcpkg
.\bootstrap-vcpkg.bat

# Core packages. libxml2 is for libiio
.\vcpkg install --triplet $platform pthreads libjpeg-turbo tiff libpng glfw3 libusb fftw3 libxml2 portaudio nng zstd armadillo opencl curl[schannel] hdf5

# Entirely for UHD...
.\vcpkg install --triplet $platform boost-chrono boost-date-time boost-filesystem boost-program-options boost-system boost-serialization boost-thread `
                                    boost-test boost-format boost-asio boost-math boost-graph boost-units boost-lockfree boost-circular-buffer        `
                                    boost-assign boost-dll

#Start Building Dependencies
$null = mkdir build
cd build
$build_args="-DCMAKE_TOOLCHAIN_FILE=$($(Get-Item ..\scripts\buildsystems\vcpkg.cmake).FullName)", "-DVCPKG_TARGET_TRIPLET=$platform", "-DCMAKE_INSTALL_PREFIX=$($(Get-Item ..\installed\$platform).FullName)", "-DCMAKE_BUILD_TYPE=Release", "-A", $generator
$standard_include=$(Get-Item ..\installed\$platform\include).FullName
$pthread_lib=$(Get-Item ..\installed\$platform\lib\pthreadVC3.lib).FullName
$libusb_include=$(Get-Item ..\installed\$platform\include\libusb-1.0).FullName
$libusb_lib=$(Get-Item ..\installed\$platform\lib\libusb-1.0.lib).FullName
if($env:PROCESSOR_ARCHITECTURE -ne $arch)
{
    $build_args += "-DCMAKE_SYSTEM_NAME=Windows", "-DCMAKE_SYSTEM_PROCESSOR=$arch", "-DCMAKE_CROSSCOMPILING=ON", "-DVCPKG_USE_HOST_TOOLS=ON", "-DVCPKG_HOST_TRIPLET=$host_triplet"
}

#TEMPORARY: Use an unmerged PR of LibUSB to allow setting RAW_IO on USB transferrs. This is needed to
#           prevent sample drops on some Windows machines with USB SDRs
#           Update Jan. 30, 2025: There's nothing more permanent than a temporary measure :-)
Write-Output "Building libusb..."
git clone https://github.com/HannesFranke-smartoptics/libusb -b raw_io_v2
cd libusb\msvc
(Get-Content -raw Base.props) -replace "<TreatWarningAsError>true</TreatWarningAsError>", "<TreatWarningAsError>false</TreatWarningAsError>" | Set-Content -Encoding ASCII Base.props
msbuild -m -v:m -p:Platform=$generator,Configuration=Release .\libusb.sln
msbuild -m -v:m -p:Platform=$generator,Configuration=Debug .\libusb.sln
$toolset_used=$(get-childitem ..\build\)[0].Name
cp -Force ..\build\$toolset_used\$generator\Release\dll\libusb-1.0.dll ..\..\..\installed\$platform\bin
cp -Force ..\build\$toolset_used\$generator\Release\dll\libusb-1.0.pdb ..\..\..\installed\$platform\bin
cp -Force ..\build\$toolset_used\$generator\Release\dll\libusb-1.0.lib ..\..\..\installed\$platform\lib
cp -Force ..\build\$toolset_used\$generator\Debug\dll\libusb-1.0.dll ..\..\..\installed\$platform\Debug\bin
cp -Force ..\build\$toolset_used\$generator\Debug\dll\libusb-1.0.pdb ..\..\..\installed\$platform\Debug\bin
cp -Force ..\build\$toolset_used\$generator\Debug\dll\libusb-1.0.lib ..\..\..\installed\$platform\Debug\lib
cp -force ..\libusb\libusb.h ..\..\..\installed\$platform\include
cd ..\..
rm -recurse -force libusb

Write-Output "Building cpu_features..."
git clone https://github.com/google/cpu_features -b v0.10.1
cd cpu_features
$null = mkdir build
cd build
cmake $build_args -DBUILD_TESTING=OFF -DBUILD_EXECUTABLE=OFF ..
cmake --build . --config Release
cmake --install .
cd ..\..
rm -recurse -force cpu_features

Write-Output "Building Volk..."
#git clone https://github.com/gnuradio/volk --depth 1 -b v3.1.2
git clone https://github.com/JVital2013/volk --depth 1 -b win-arm64 #Patches to fix NEON support on Windows
cd volk
$null = mkdir build
cd build
cmake $build_args -DENABLE_TESTING=OFF -DENABLE_MODTOOL=OFF ..
cmake --build . --config Release
cmake --install .
cd ..\..
rm -recurse -force volk

Write-Output "Building Airspy..."
#git clone https://github.com/airspy/airspyone_host --depth 1 #-b v1.0.10
git clone https://github.com/JVital2013/airspyone_host -b rawio #Enables RAW_IO to avoid sample drops
cd airspyone_host\libairspy
$null = mkdir build
cd build
cmake $build_args -DLIBUSB_INCLUDE_DIR="$($libusb_include)" -DLIBUSB_LIBRARIES="$($libusb_lib)" -DTHREADS_PTHREADS_WIN32_LIBRARY="$($pthread_lib)" ..
cmake --build . --config Release
cmake --install .
cd ..\..\..
rm -recurse -force airspyone_host

Write-Output "Building Airspy HF..."
#git clone https://github.com/airspy/airspyhf --depth 1 #-b 1.6.8
git clone https://github.com/JVital2013/airspyhf -b rawio #Enables RAW_IO to avoid sample drops
cd airspyhf\libairspyhf
$null = mkdir build
cd build
cmake $build_args -DLIBUSB_INCLUDE_DIR="$($libusb_include)" -DLIBUSB_LIBRARIES="$($libusb_lib)" -DTHREADS_PTHREADS_WIN32_LIBRARY="$($pthread_lib)" ..
cmake --build . --config Release
cmake --install .
cd ..\..\..
rm -recurse -force airspyhf

Write-Output "Building RTL-SDR..."
#git clone https://github.com/osmocom/rtl-sdr --depth 1 -b v2.0.2
git clone https://github.com/JVital2013/librtlsdr -b rawio #Enables RAW_IO to avoid sample drops
cd librtlsdr
$null = mkdir build
cd build
cmake $build_args -DLIBUSB_INCLUDE_DIRS="$($libusb_include)" -DLIBUSB_LIBRARIES="$($libusb_lib)" -DTHREADS_PTHREADS_INCLUDE_DIR="$($standard_include)" -DTHREADS_PTHREADS_LIBRARY="$($pthread_lib)" ..
cmake --build . --config Release
cmake --install .
cd ..\..
rm -recurse -force librtlsdr

Write-Output "Building HackRF..."
#git clone https://github.com/greatscottgadgets/hackrf --depth 1 -b v2024.02.1
git clone https://github.com/JVital2013/hackrf -b rawio #Enables RAW_IO to avoid sample drops
cd hackrf\host\libhackrf
$null = mkdir build
cd build
cmake $build_args -DLIBUSB_INCLUDE_DIR="$($libusb_include)" -DLIBUSB_LIBRARIES="$($libusb_lib)" -DTHREADS_PTHREADS_WIN32_LIBRARY="$($pthread_lib)" ..
cmake --build . --config Release
cmake --install .
cd ..\..\..\..
rm -recurse -force hackrf

Write-Output "Building libiio..."
git clone https://github.com/analogdevicesinc/libiio --depth 1 -b v0.26
cd libiio
(Get-Content -raw CMakeLists.txt) -replace "check_symbol_exists\(libusb_get_version libusb.h HAS_LIBUSB_GETVERSION\)", "" | Set-Content -Encoding ASCII CMakeLists.txt #Needed for cross-compilation only
$null = mkdir build
cd build
cmake $build_args -DWITH_IIOD=OFF -DWITH_TESTS=OFF -DWITH_ZSTD=ON -DLIBUSB_INCLUDE_DIR="$($libusb_include)" -DLIBUSB_LIBRARIES="$($libusb_lib)" ..
cmake --build . --config Release
cmake --install .
cd ..\..
rm -recurse -force libiio

Write-Output "Building libad9361-iio..."
git clone https://github.com/analogdevicesinc/libad9361-iio --depth 1 -b v0.3
cd libad9361-iio
$null = mkdir build
cd build
cmake $build_args -DLIBIIO_LIBRARIES="$($(Get-Item ..\..\..\installed\$platform\lib\libiio.lib).FullName)" -DENABLE_PACKAGING=OFF ..
cmake --build . --config Release
cmake --install .
cd ..\..
rm -recurse -force libad9361-iio

# Not compatible with ARM at this time
if($platform -eq "x64-windows" -or $platform -eq "x86-windows")
{
    Write-Output "Building LimeSuite..."
    Invoke-WebRequest -Uri "https://www.satdump.org/FX3-SDK.zip" -OutFile FX3-SDK.zip
    Expand-Archive FX3-SDK.zip .
    $fx3_arg = "-DFX3_SDK_PATH=$($(Get-Item .\FX3-SDK).FullName)"
    git clone https://github.com/myriadrf/LimeSuite # v23.11.0 (latest as of this writing) is not compatible with the latest MSVC
    cd LimeSuite
    $null = mkdir build-dir
    cd build-dir
    cmake $build_args -DENABLE_GUI=OFF $fx3_arg ..
    cmake --build . --config Release
    cmake --install .
    cd ..\..
    rm -recurse -force LimeSuite
}

Write-Output "Building bladeRF..."
git clone https://github.com/Nuand/bladeRF --depth 1 -b 2024.05
cd bladeRF\host
Clear-Content cmake/modules/FindLibPThreadsWin32.cmake
Clear-Content cmake/modules/FindLibUSB.cmake
(Get-Content -raw CMakeLists.txt) -replace "(?ms)find_package\(LibPThreadsWin32\).*endif\(LIBUSB_FOUND\)", "" | Set-Content -Encoding ASCII CMakeLists.txt
$null = mkdir build
cd build
cmake $build_args $fx3_arg -DTREAT_WARNINGS_AS_ERRORS=OFF -DLIBPTHREADSWIN32_INCLUDE_DIRS="$($standard_include)" -DLIBUSB_INCLUDE_DIRS="$($libusb_include)" -DLIBUSB_LIBRARIES="$($libusb_lib)" -DLIBPTHREADSWIN32_LIBRARIES="$($pthread_lib)" -DTEST_LIBBLADERF=OFF -DLIBUSB_FOUND=ON -DLIBPTHREADSWIN32_FOUND=ON -DLIBUSB_VERSION="$($(ls ..\..\..\..\installed\vcpkg\info\libusb*).BaseName.split('_')[1])" ..
cmake --build . --config Release
cmake --install .
cd ..\..\..
rm -recurse -force bladeRF

# Not compatible with ARM at this time
if($platform -eq "x64-windows" -or $platform -eq "x86-windows")
{
    rm -recurse -force FX3-SDK, FX3-SDK.zip
}

Write-Output "Building UHD..."
git clone https://github.com/EttusResearch/uhd # v4.8 (latest as of this writing) is not compatible with the latest MSVC
cd uhd\host
$null = mkdir build
cd build
cmake $build_args -DENABLE_MAN_PAGES=OFF -DENABLE_MANUAL=OFF -DENABLE_PYTHON_API=OFF -DENABLE_EXAMPLES=OFF -DENABLE_UTILS=OFF -DENABLE_TESTS=OFF ..
cmake --build . --config Release
cmake --install .
cd ..\..\..
rm -recurse -force uhd

cd ..
rm -recurse -force build

#Install SDRPlay API
Invoke-WebRequest -Uri "https://www.satdump.org/SDRPlay.zip" -OutFile sdrplay.zip
mkdir sdrplay | Out-Null
Expand-Archive sdrplay.zip .
cp sdrplay\API\inc\*.h installed\$platform\include
cp sdrplay\API\$sdrplay_arch\sdrplay_api.dll installed\$platform\bin
cp sdrplay\API\$sdrplay_arch\sdrplay_api.lib installed\$platform\lib
Remove-Item sdrplay -Force -Recurse -ErrorAction SilentlyContinue
Remove-Item sdrplay.zip

#Clean Up (Some packages are silly)
mv installed\$platform\lib\*.dll installed\$platform\bin\
mv installed\$platform\bin\*.lib installed\$platform\lib\
cd ..