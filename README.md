# SatDump

A generic satellite data processing software.  
*Thanks Mnux for the icon!*

**Still WIP**

# Usage

First of all, as with any program using volk, running volk_profile once is highly recommended for better performances.

SatDump comes in 2 variants, doing essentially the same thing :
- A GUI version, meant to be user-friendly
![The UI](https://github.com/altillimity/satdump/raw/master/gui_example2.png)
- A CLI version for command-line / headless processing
![The CLI](https://github.com/altillimity/satdump/raw/master/cli_example.png)

Otherwise, here's roughly how SatDump works :
- You record / get some data, often baseband from some supported satellite
- You start SatDump (UI for an user interface), select the pipeline and input format & folder
- You start processing, and SatDump will do everything required to get down to useful data

Supported baseband formats are :
- i16 - Signed 16-bits integer
- i8 - Signed 8-bits integer
- f32 - Raw complex 32-bits float
- w8 - 8-bits wav, for compatibility with files from SDR#, HDSDR and others   
- ZIQ - Signed 16 or 8-bits integer or 32-bit float (autodetected) compressed with zst (optional)
Of course, selecting the wrong format will result in nothing, or very little working...

As for other formats often used throughought the program :
- .soft  : 8-bits soft demodulated symbols
- .cadu  : CCSDS (or similar) transport frames, deframed, derandomized and corrected
- .frm   : Other frame formats
- .raw16 : NOAA HRPT Frame format, compatible with HRPT Reader

SatDump also support batch, or command-line processing with a common syntax :
The UI version can also take the same commands to bypass the UI.
```
satdump pipeline input_format path/to/input/file.something products output_folder [-baseband_format...]

# Example for Falcon 9 data
satdump falcon9_tlm baseband falcon9-felix.wav products falcon9_data -samplerate 6000000 -baseband_format i16

# Example for MetOp AHRPT data
satdump metop_ahrpt baseband metopb.wav products metopb_ahrpt -samplerate 6000000 -baeband_format i16
```

Live processing is now supported (but WIP) on all platforms. You will have to enable it manually when building from source with -DBUILD_LIVE=ON.   
Supported devices currently include :   
- HackRF Devices
- Airspy One Devices (Mini, R2 with some temporary caveats)
- RTL-SDR Devices
- RTL-TCP
- SpyServer
- SDRPlay

### Satdump Baseband Recorder

Satdump now also has baseband recorder built in.
Ofcourse supports all SDRs listed above and can record in all supported baseband formats aswell as compressed ZIQ format.

- ziq8 - ZST compressed 8-bits integer.
- ziq16 - ZST compressed 16-bits integer.
- ziq32 - ZST compressed 32-bits float.

This functionality is most helpful for high bandwidth or long duration recording aswell as recording to a slow storage device that otherwise would not manage to write so much raw baseband data.
With a very simple FFT spectrum analyzer and waterfall it has very low requirements on CPU. 
Its meant mainly for high bandwidth (40MSPS and more) but ofcourse works no problem even on lowest samplerates.

CLI version of the recorder `satdump_recorder` needs a .json config file for SDR settings.
Example SDR settings .json config.
```
{
    "sdr_type": "airspy",
    "sdr_settings": {
        "gain": "21",
        "bias": "0"
    }
}

```

# Building / Installing

### Windows

On Windows the recommend method of running SatDump is getting the latest pre-built release off the [Release](https://github.com/altillimity/SatDump/releases) page, which includes everything you will need to run it.  
Those builds are made with Visual Studio 2019 for x64, so the appropriate Visual C++ Runtime will be required. You can get it [here](https://support.microsoft.com/en-us/topic/the-latest-supported-visual-c-downloads-2647da03-1eea-4433-9aff-95f26a218cc0).   

From there, just run either satdump-ui.exe or satdump.exe (CLI) and everything will work.

If you really want to build it yourself on Windows, you will need some version of the [MSVC compiler](https://visualstudio.microsoft.com/downloads/) (with C++ 17), [CMake](https://cmake.org/download/) and [Git](https://gitforwindows.org/).  
Some knowledge of using VCPKG and building with CMake is assumed.

As dependencies that are not included in [VCPKG](https://github.com/Microsoft/vcpkg), you will need to build [VOLK](https://github.com/gnuradio/volk). Otherwise, you will also need libpng, libjpeg, libfftw3 and fmt for the core, and additionally glew and glfw3 for the UI version.

VCPK is expected to be in the root of SatDump's git directory in the build system. 

```
git clone https://github.com/altillimity/satdump.git
cd satdump
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.bat
.\vcpkg.exe install libpng:x64-windows libjpeg-turbo:x64-windows fftw3:x64-windows glew:x64-windows glfw3:x64-windows # Gotta check those are correct... Should be
# At this point, I copied over the files from libraries compiled from source in vcpkg/installed/x64-windows
cd ..
mkdir build
cd build
cmake -G "Visual Studio 16 2019" -DBUILD_MSVC=ON -DCMAKE_TOOLCHAIN_FILE=../vcpk/scripts/buildsystems/vcpkg.cmake .. # Or whatever version you wanna use
cmake --build . --config Release
```

Note : Mingw builds are NOT supported, VOLK will not work.

### Linux (or MacOS)

On Linux (or MacOS), building from source is recommended and no build are currently provided.
[libcorrect](https://github.com/quiet/libcorrect) is now included in SatDump, hence it is not required to build it from source anymore.

Here are some generic Debian build instructions.

```
# Linux: Install dependencies
sudo apt install git build-essential cmake g++ pkgconf libfftw3-dev libvolk1-dev libjpeg-dev libpng-dev # Core dependencies
sudo apt install libnng-dev                                                                             # If this packages is not found, follow build instructions below for NNG
sudo apt install librtlsdr-dev libhackrf-dev libairspy-dev libairspyhf-dev                              # All libraries required for live processing (optional)
sudo apt install libglew-dev libglfw3-dev   # Only if you want to build the GUI Version (optional)
sudo apt install libzstd-dev                  # Only if you want to build with ZIQ Recording compression (optional)

# If libnng-dev is not available, you will have to build it from source
git clone https://github.com/nanomsg/nng.git
cd nng
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON ..                             # MacOS
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=/usr .. # Linux
make -j4
sudo make install
cd ../..
rm -rf nng

# macOS: Install dependencies
brew install cmake volk jpeg libpng glew glfw nng

# macOS ONLY: build and install libfftw3
# if you install fftw via brew, cmake won't be able to find it
wget http://www.fftw.org/fftw-3.3.9.tar.gz
tar xf fftw-3.3.9.tar.gz
rm fftw-3.3.9.tar.gz
cd fftw-3.3.9
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=false -DENABLE_FLOAT=true ..
make
sudo make install
cd ../..
rm -rf fftw-3.3.9

# Finally, SatDump
git clone https://github.com/altillimity/satdump.git
cd satdump
mkdir build && cd build
# If you want to build Live-processing (required for the ingestor), add -DBUILD_LIVE=ON to the command
# If you do not want to build the GUI Version, add -DNO_GUI=ON to the command
# If you want to disable some SDRs, you can add -DENABLE_SDR_AIRSPY=OFF or similar
# If you want to build with ZIQ compression, you can add -DBUILD_ZIQ=1
cmake -DCMAKE_BUILD_TYPE=Release ..                             # MacOS
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr .. # Linux
make -j4
ln -s ../pipelines . # Symlink pipelines so it can run
ln -s ../resources . # Symlink pipelines so it can run
ln -s ../Ro* . # Symlink fonts for the GUI version so it can run

# Run (if you want!)
./satdump-ui
```

### Android (Very, very WIP)

SatDump can now be used on Android devices as a simple APK, which will be built alongside normal releases.   
Keep in mind this port is very much a work-in-progress, and most things have not been tested or may not perform as expected yet. Though, any input is welcome!

If you wish to build it for yourself, make sure to have the Android SDK and NDK installed, then just :

```
git clone https://github.com/altillimity/satdump.git --recursive # For SDL2

cd satdump/android

./gradlew assembleDebug # Your .apk will be in android/SatDump/build/outputs/apk/debug/SatDump-debug.apk
```

Credits for the ImGui port over to Android / SDL2 to https://github.com/sfalexrog/Imgui_Android.
