if(-not $(Test-Path "C:\Program Files (x86)\NSIS\makensis.exe"))
{
    Write-Error "NSIS not found! Aborting."
    return 1
}

cd "$(Split-Path -Parent $MyInvocation.MyCommand.Path)\..\vcpkg"
rm -force nsis-3.10-strlen_8192.zip -erroraction silentlycontinue
rm -recurse -force nsis-3.10-strlen_8192 -erroraction silentlycontinue
Invoke-WebRequest https://downloads.sourceforge.net/project/nsis/NSIS%203/3.10/nsis-3.10-strlen_8192.zip -OutFile nsis-3.10-strlen_8192.zip -UserAgent "Wget/1.21.1" | Out-Null
if(-not $(Test-Path .\nsis-3.10-strlen_8192.zip))
{
    Write-Error "Error downloading NSIS longpath! Aborting"
    return 1
}

Expand-Archive nsis-3.10-strlen_8192.zip
Copy-Item -Recurse -Force nsis-3.10-strlen_8192\* "C:\Program Files (x86)\NSIS\"
