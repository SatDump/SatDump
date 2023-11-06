# Preps satdump for execution within the debugger
# MSVC 2022 seems to build in a ./out/... folder
# Assume this is true
cd "$(Split-Path -Parent $MyInvocation.MyCommand.Path)\..\out\build\x64-Debug"

#Remove old dirs
if(Test-Path Debug\resources -ErrorAction SilentlyContinue) {rm -Recurse Debug\resources}
if(Test-Path Debug\pipelines -ErrorAction SilentlyContinue) {rm -Recurse Debug\pipelines}
if(Test-Path Debug\plugins -ErrorAction SilentlyContinue) {rm -Recurse Debug\plugins}

cp -force ..\..\..\vcpkg\installed\x64-windows\bin\*.dll Debug
cp -r ..\..\..\resources Debug
cp -r ..\..\..\pipelines Debug

mkdir Debug\plugins | Out-Null
cp -force plugins\Debug\*.dll Debug\plugins
cp -force plugins\Debug\*.pdb Debug\plugins
cp -force ..\..\..\satdump_cfg.json Debug

#Overwrite debug DLLs
cp -force ..\..\..\vcpkg\installed\x64-windows\debug\bin\libpng16d.dll Debug\libpng16.dll
cp -force ..\..\..\vcpkg\installed\x64-windows\debug\bin\fftw3f.dll Debug\fftw3f.dll
cp -force ..\..\..\vcpkg\installed\x64-windows\debug\bin\glfw3.dll Debug\glfw3.dll
cp -force ..\..\..\vcpkg\installed\x64-windows\debug\bin\jpeg62.dll Debug\jpeg62.dll
cp -force ..\..\..\vcpkg\installed\x64-windows\debug\bin\libusb-1.0.dll Debug\libusb-1.0.dll

#Remove stray DLLs from plugin folder, if they're there
rm Debug\plugins\fftw3f.dll | Out-Null
rm Debug\plugins\libusb-1.0.dll | Out-Null

#Done
cd "$(Split-Path -Parent $MyInvocation.MyCommand.Path)\.."
Write-Output "Done!"