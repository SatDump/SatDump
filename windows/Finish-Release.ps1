#Copy all files into the Release folder
cd "$(Split-Path -Parent $MyInvocation.MyCommand.Path)\..\build"
cp ..\vcpkg\installed\x64-windows\bin\*.dll Release
cp -r ..\resources Release
cp -r ..\pipelines Release
mkdir Release\plugins | Out-Null
mkdir Release\resources\c_plugin | Out-Null
cp plugins\Release\*.dll Release\plugins
cp plugins\Release\*.plugin Release\resources\c_plugin
cp ..\satdump_cfg.json Release
cd Release

Write-Output "Done!"
