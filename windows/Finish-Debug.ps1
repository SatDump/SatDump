# Preps satdump for execution within the debugger
# MSVC 2022 seems to build in a ./out/... folder
# Assume this is true
param([string]$platform="x64-windows") #or x86-windows, arm64-windows

function Parse-DumpBin($binary_path)
{
    $return_val = @()
    $dumpbin_result = dumpbin /dependents $binary_path
    $reading = $false
    for($i = 0; $i -lt $dumpbin_result.Count; $i++)
    {
        $this_line = $dumpbin_result[$i].trim()
        if($this_line -eq "Image has the following dependencies:")
        {
            $i++
            $reading = $true
            continue
        }
        if($reading -and [string]::IsNullOrWhiteSpace($this_line))
        {
            break
        }
        if($reading)
        {
            $return_val += $this_line
        }
    }

    return $return_val
}

cd "$(Split-Path -Parent $MyInvocation.MyCommand.Path)\..\out\build\$($platform.Split("-")[0])-Debug"

#Remove old dirs
if(Test-Path Debug\resources -ErrorAction SilentlyContinue) {rm -Recurse Debug\resources}
if(Test-Path Debug\pipelines -ErrorAction SilentlyContinue) {rm -Recurse Debug\pipelines}
if(Test-Path Debug\plugins -ErrorAction SilentlyContinue) {rm -Recurse Debug\plugins}

cp -r ..\..\..\resources Debug
cp -r ..\..\..\pipelines Debug

mkdir Debug\plugins | Out-Null
cp -force plugins\Debug\*.dll Debug\plugins
cp -force plugins\Debug\*.pdb Debug\plugins
cp -force ..\..\..\satdump_cfg.json Debug

$input_dlls = Get-ChildItem Debug\ -Recurse -Filter *.dll
$input_dlls += Get-ChildItem Debug\ -Recurse -Filter *.exe
$dll_array = @()

foreach($input_dll in $input_dlls)
{
    $dll_array += Parse-Dumpbin $input_dll.FullName
}

$dll_array = $dll_array | select -Unique
$available_dlls = Get-ChildItem ..\..\..\vcpkg\installed\$platform\bin -Filter *.dll
$dlls_to_copy = @()
foreach($available_dll in $available_dlls)
{
    if($dll_array.Contains($available_dll.Name))
    {
        $dlls_to_copy += $available_dll
    }
}

# Recursively get remaining dependencies
$last_count = 0
while($last_count -ne $dlls_to_copy.Count)
{
    $last_count = $dlls_to_copy.Count
    foreach($dll_to_copy in $dlls_to_copy)
    {
        $potential_dlls = Parse-DumpBin $dll_to_copy.FullName
        foreach($available_dll in $available_dlls)
        {
            if($potential_dlls.Contains($available_dll.Name) -and -not $dlls_to_copy.Name.Contains($available_dll.Name))
            {
                $dlls_to_copy += $available_dll
            }
        }
    }
}

# Copy determined dependencies
foreach($dll_to_copy in $dlls_to_copy)
{
    cp -Force $dll_to_copy.FullName Debug
}

#Overwrite debug DLLs
cp -force ..\..\..\vcpkg\installed\$platform\debug\bin\libpng16d.dll Debug\libpng16.dll
cp -force ..\..\..\vcpkg\installed\$platform\debug\bin\fftw3f.dll Debug\fftw3f.dll
cp -force ..\..\..\vcpkg\installed\$platform\debug\bin\glfw3.dll Debug\glfw3.dll
cp -force ..\..\..\vcpkg\installed\$platform\debug\bin\jpeg62.dll Debug\jpeg62.dll
cp -force ..\..\..\vcpkg\installed\$platform\debug\bin\tiffd.dll Debug\tiff.dll
cp -force ..\..\..\vcpkg\installed\$platform\debug\bin\liblzma.dll Debug\liblzma.dll
cp -force ..\..\..\vcpkg\installed\$platform\debug\bin\libcurl-d.dll Debug\libcurl.dll
cp -force ..\..\..\vcpkg\installed\$platform\debug\bin\libusb-1.0.dll Debug\libusb-1.0.dll
cp -force ..\..\..\vcpkg\installed\$platform\debug\bin\pthreadVC3d.dll Debug\pthreadVC3.dll

#Remove stray DLLs from plugin folder, if they're there
rm Debug\plugins\fftw3f.dll | Out-Null
rm Debug\plugins\nng.dll | Out-Null
rm Debug\plugins\libusb-1.0.dll | Out-Null
rm Debug\plugins\portaudio.dll | Out-Null
rm Debug\plugins\zstd.dll | Out-Null

#Done
cd "$(Split-Path -Parent $MyInvocation.MyCommand.Path)\.."
Write-Output "Done!"