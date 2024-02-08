if(!!(Get-Command 'tf' -ErrorAction SilentlyContinue) -eq $false)
{
    Write-Error "You must run this script within Developer Powershell for Visual Studio"
    exit
}

#Build SatDump
cd "$(Split-Path -Parent $MyInvocation.MyCommand.Path)\.."
mkdir build | Out-Null
cd build
cmake .. -G "Visual Studio 17 2022" -DBUILD_MSVC=ON -DCMAKE_TOOLCHAIN_FILE=..\vcpkg\scripts\buildsystems\vcpkg.cmake -A x64
cmake --build . --config Release
cd ..