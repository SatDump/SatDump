#Copy all files into the Release folder
cd "$(Split-Path -Parent $MyInvocation.MyCommand.Path)\..\build"
cp ..\vcpkg\installed\x64-windows\bin\*.dll Release
cp -r ..\resources Release
cp -r ..\pipelines Release
mkdir Release\plugins | Out-Null
cp plugins\Release\*.dll Release\plugins
cp ..\satdump_cfg.json Release
cd Release

Write-Output "Done!"