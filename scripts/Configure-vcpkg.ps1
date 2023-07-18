if(!!(Get-Command 'tf' -ErrorAction SilentlyContinue) -eq $false)
{
    Write-Error "You must run this script within Developer Powershell for Visual Studio"
    exit
}

if(Test-Path "$(Split-Path -Parent $MyInvocation.MyCommand.Path)\..\vcpkg" -ErrorAction SilentlyContinue)
{
    Write-Output "$(Split-Path -Parent $MyInvocation.MyCommand.Path)\..\vcpkg already found! Not setting up."
    exit
}

#Download custom vcpkg
Write-Output "Configuing vcpkg..."
cd "$(Split-Path -Parent $MyInvocation.MyCommand.Path)\.."
mkdir vcpkg | Out-Null
git clone -b debug --depth 1 https://github.com/JVital2013/vcpkg-satdump.git vcpkg
cd vcpkg

#Install SDRPlay API into vcpkg
Invoke-WebRequest -Uri "https://www.satdump.org/SDRPlay.zip" -OutFile sdrplay.zip
mkdir sdrplay | Out-Null
Expand-Archive sdrplay.zip .
cp sdrplay\API\inc\*.h installed\x64-windows\include
cp sdrplay\API\x64\sdrplay_api.dll installed\x64-windows\bin
cp sdrplay\API\x64\sdrplay_api.lib installed\x64-windows\lib
Remove-Item sdrplay -Force -Recurse -ErrorAction SilentlyContinue
Remove-Item sdrplay.zip
cd ..