param(
    [string]$BuildPath="$(Split-Path -Parent $MyInvocation.MyCommand.Path)\..\build",
    [string]$SourcePath="$(Split-Path -Parent $MyInvocation.MyCommand.Path)\..",
    [string]$platform="x64-windows" #or x86-windows, arm64-windows
)

#Copy all files into the Release folder
cd $BuildPath
mkdir Release\plugins | Out-Null
cp plugins\Release\*.dll Release\plugins
cp -r ..\resources Release
cp -r ..\pipelines Release
cp ..\satdump_cfg.json Release
cd Release

$input_dlls = Get-ChildItem -Recurse -Filter *.dll
$input_dlls += Get-ChildItem -Recurse -Filter *.exe
$dll_array = @()

foreach($input_dll in $input_dlls)
{
    $dumpbin_result = dumpbin /dependents $input_dll.FullName
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
            $dll_array += $this_line
        }
    }
}


$dll_array = $dll_array | select -Unique
$available_dlls = Get-ChildItem $SourcePath\vcpkg\installed\$platform\bin -Filter *.dll
foreach($available_dll in $available_dlls)
{
    if($dll_array.Contains($available_dll.Name))
    {
        cp $available_dll.FullName .
    }
}

cd ..\..
Write-Output "Done!"