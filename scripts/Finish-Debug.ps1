# Preps satdump for execution within the debugger
# MSVC 2022 seems to build in a ./out/... folder
# Assume this is true

cd "$(Split-Path -Parent $MyInvocation.MyCommand.Path)\..\out\build\x64-Debug"
cp ..\..\..\vcpkg\installed\x64-windows\bin\*.dll Debug
cp -r ..\..\..\resources Debug
cp -r ..\..\..\pipelines Debug
mkdir Debug\plugins | Out-Null
cp plugins\Debug\*.dll Debug\plugins
cp plugins\Debug\*.pdb Debug\plugins
cp ..\..\..\satdump_cfg.json Debug

#Overwrite debug plugins
cp -force ..\..\..\vcpkg\installed\x64-windows\debug\bin\libpng16d.dll Debug\libpng16.dll
cp -force ..\..\..\vcpkg\installed\x64-windows\debug\bin\glew32d.dll Debug\glew32.dll
cp -force ..\..\..\vcpkg\installed\x64-windows\debug\bin\fftw3f.dll Debug\fftw3f.dll
cp -force ..\..\..\vcpkg\installed\x64-windows\debug\bin\glfw3.dll Debug\glfw3.dll
cp -force ..\..\..\vcpkg\installed\x64-windows\debug\bin\jpeg62.dll Debug\jpeg62.dll
cp -force ..\..\..\vcpkg\installed\x64-windows\debug\bin\libusb-1.0.dll Debug\libusb-1.0.dll

#Remove stray libraries from plugin folder
rm Debug\plugins\fftw3f.dll
rm Debug\plugins\libusb-1.0.dll

#Done
cd Debug
Write-Output "Done!"