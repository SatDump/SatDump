param([string]$platform="x64-windows") #or x86-windows, arm64-windows
$ErrorActionPreference = "Stop"
$PSDefaultParameterValues['*:ErrorAction']='Stop'

if(!!(Get-Command 'tf' -ErrorAction SilentlyContinue) -eq $false)
{
    Write-Error "You must run this script within Developer Powershell for Visual Studio"
    exit
}

#Build SatDump
cd "$(Split-Path -Parent $MyInvocation.MyCommand.Path)\.."
mkdir build | Out-Null
cd build
cmake .. -DBUILD_MSVC=ON -DCMAKE_TOOLCHAIN_FILE="$($(Get-Item ..\vcpkg\scripts\buildsystems\vcpkg.cmake).FullName)" -DVCPKG_TARGET_TRIPLET="$platform"
cmake --build . --config Release
cd ..