param([string]$platform="x64-windows") #or x86-windows, arm64-windows
$ErrorActionPreference = "Stop"
$PSDefaultParameterValues['*:ErrorAction']='Stop'

if(!!(Get-Command 'tf' -ErrorAction SilentlyContinue) -eq $false)
{
    Write-Error "You must run this script within Developer Powershell for Visual Studio"
    exit
}

if($platform -eq "x64-windows")
{
    $generator = "x64"
    $arch = "AMD64"
    $additional_args = @()
}
elseif($platform -eq "x86-windows")
{
    $generator = "Win32"
    $arch = "X86"
    $additional_args = @()
}
elseif($platform -eq "arm64-windows")
{
    $generator = "ARM64"
    $arch = "ARM64"
    $additional_args = @("-DPLUGIN_LIMESDR_SDR_SUPPORT=OFF")
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
    $additional_args += "-DCMAKE_SYSTEM_NAME=Windows", "-DCMAKE_SYSTEM_PROCESSOR=$arch", "-DCMAKE_CROSSCOMPILING=ON", "-DVCPKG_USE_HOST_TOOLS=ON", "-DVCPKG_HOST_TRIPLET=$host_triplet"
}

#Build SatDump
cd "$(Split-Path -Parent $MyInvocation.MyCommand.Path)\.."
mkdir build | Out-Null
cd build
cmake .. -DBUILD_MSVC=ON -DCMAKE_TOOLCHAIN_FILE="$($(Get-Item ..\vcpkg\scripts\buildsystems\vcpkg.cmake).FullName)" -DVCPKG_TARGET_TRIPLET="$platform" -A $generator $additional_args
cmake --build . --config Release
cd ..