# Output folder
if (Test-Path -path portable) {
    rm -r -fo portable
}
mkdir portable

# Core
cp satdump.exe portable
cp satdump-ui.exe portable
cp *.dll portable

# Plugins
mkdir portable/plugins
cp plugins/*.dll portable/plugins

# Resources
cp -r ../resources portable
cp ../satdump_cfg.json portable

# Add DLLs
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

$input_dlls = Get-ChildItem -Recurse -ErrorAction SilentlyContinue -Filter portable/*.dll
$input_dlls += Get-ChildItem -ErrorAction SilentlyContinue -Recurse -Filter portable/*.exe
$dll_array = @()

# Read dependencies
foreach($input_dll in $input_dlls)
{
    $dll_array += Parse-Dumpbin $input_dll.FullName
}

$dll_array = $dll_array | select -Unique
$available_dlls = Get-ChildItem ../windows/deps/output/bin -Filter *.dll
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
    cp $dll_to_copy.FullName .
}
