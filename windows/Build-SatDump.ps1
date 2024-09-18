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
    $additional_args = @()
}
elseif($platform -eq "x86-windows")
{
    $generator = "Win32"
    $additional_args = @()
}
elseif($platform -eq "arm64-windows")
{
    $generator = "ARM64"
    $additional_args = "-DPLUGIN_USRP_SDR_SUPPORT=OFF", "-DPLUGIN_LIMESDR_SDR_SUPPORT=OFF"
}
else
{
    Write-Error "Unsupported platform: $platform"
    exit 1
}

#Build SatDump
cd "$(Split-Path -Parent $MyInvocation.MyCommand.Path)\.."
mkdir build | Out-Null
cd build
cmake .. -DBUILD_MSVC=ON -DCMAKE_TOOLCHAIN_FILE="$($(Get-Item ..\vcpkg\scripts\buildsystems\vcpkg.cmake).FullName)" -DVCPKG_TARGET_TRIPLET="$platform" -A $generator $additional_args
cmake --build . --config Release
cd ..