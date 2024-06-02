if(!!(Get-Command 'tf' -ErrorAction SilentlyContinue) -eq $false)
{
    Write-Error "You must run this script within Developer Powershell for Visual Studio"
    exit
}

cd "$(Split-Path -Parent $MyInvocation.MyCommand.Path)\.."
rm -recurse -force -ErrorAction SilentlyContinue vcpkg | Out-Null
mkdir vcpkg | Out-Null
cd vcpkg
mkdir installed | Out-Null
cd installed
git clone https://github.com/JVital2013/SatDump-WinXP-Deps x64-windows #These are 32-bit deps; the name is just 64-bit here to facilitate building
cd ../..