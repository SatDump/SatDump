if(!!(Get-Command 'tf' -ErrorAction SilentlyContinue) -eq $false)
{
    Write-Error "You must run this script within Developer Powershell for Visual Studio"
    exit
}

#Build SatDump
cd "$(Split-Path -Parent $MyInvocation.MyCommand.Path)\.."
rm -recurse -force -ErrorAction SilentlyContinue build | Out-Null
mkdir build | Out-Null
cd build
cmake .. -G "Visual Studio 17 2022" -DBUILD_MSVC=ON -DBUILD_OPENCL=OFF -DPLUGIN_MIRISDR_SDR_SUPPORT=OFF -DPLUGIN_AIRSPY_SDR_SUPPORT=OFF -DPLUGIN_AIRSPYHF_SDR_SUPPORT=OFF -DPLUGIN_HACKRF_SDR_SUPPORT=OFF -DPLUGIN_LIMESDR_SDR_SUPPORT=OFF -DPLUGIN_SDRPLAY_SDR_SUPPORT=OFF -DPLUGIN_PLUTOSDR_SDR_SUPPORT=OFF -DPLUGIN_BLADERF_SDR_SUPPORT=OFF -DPLUGIN_USRP_SDR_SUPPORT=OFF -DPLUGIN_RTLSDR_SDR_SUPPORT=OFF -DPLUGIN_RTLTCP_SUPPORT=OFF -A Win32 -T v141_xp -DCMAKE_FIND_ROOT_PATH="../vcpkg/installed/x64-windows"
cmake --build . --config Release
cd ..