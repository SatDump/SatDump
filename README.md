# SatDump

A generic satellite data processing software.

**Still WIP**

# Usage

First of all, as with any program using volk, running volk_profile once is highly recommended for better performances.

SatDump comes in 2 variants, doing essentially the same thing :
- A GUI version, meant to be user-friendly
![The UI](https://github.com/altillimity/satdump/raw/master/gui_example.png)
- A CLI version for command-line / headless processing
![The UI](https://github.com/altillimity/satdump/raw/master/cli_example.png)

Otherwise, here's roughly how SatDump works :
- You record / get some data, often baseband from some supported satellite
- You start SatDump (UI for an user interface), select the pipeline and input format & folder
- You start processing, and SatDump will do everything required to get down to useful data

Supported baseband formats are :
- i16 - Signed 16-bits integer
- i8 - Signed 8-bits integer
- f32 - Raw complex 32-bits float
- w8 - 8-bits wav, for compatibility with files from SDR#, HDSDR and others   
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
satdump metop_ahrpt baseband metopb.wav products metopb_ahrpt -samplerate 6000000 -baseband_format i16
```

Live processing is now supported (but WIP) on all platform, currently only with AIRSPY support. You will have to enable it manually when building with -DBUILD_LIVE=ON.

### Notes on Falcon-9 Camera processing

The resulting .mxf file should be readable by software such as VLC, but as it may contain errors, VLC turned out not to be the best at handling this.
I personally got my bests results with GStreamer, using the following command :   

`gst-launch-1.0 filesrc location="camera.mxf" ! decodebin ! videoconvert ! avimux name=mux ! filesink location=camera.avi`

And then converting to mp4 with   

`ffmpeg -i camera.avi camera.mp4`

# Building / Installing

### Windows

On Windows the recommend method of running SatDump is getting the latest pre-built release off the [Release](https://github.com/altillimity/SatDump/releases) page, which includes everything you will need to run it.  
Those builds are made with Visual Studio 2019 for x64, so the appropriate Visual C++ Runtime will be required. You can get it [here](https://support.microsoft.com/en-us/topic/the-latest-supported-visual-c-downloads-2647da03-1eea-4433-9aff-95f26a218cc0).   

From there, just run either satdump-ui.exe or satdump.exe (CLI) and everything will work.

If you really want to build it yourself on Windows, you will need some version of the [MSVC compiler](https://visualstudio.microsoft.com/downloads/) (with C++ 17), [CMake](https://cmake.org/download/) and [Git](https://gitforwindows.org/).  
Some knowledge of using VCPKG and building with CMake is assumed.

As dependencies that are not included in [VCPKG](https://github.com/Microsoft/vcpkg), you will need to build [VOLK](https://github.com/gnuradio/volk) and [libcorrect](https://github.com/quiet/libcorrect). Otherwise, you will also need libpng, libjpeg, libfftw3 and fmt for the core, and additionally glew and glfw3 for the UI version.

VCPK is expected to be in the root of SatDump's git directory in the build system. 

```
git clone https://github.com/altillimity/satdump.git
cd satdump
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.bat
./vcpkg.exe install png:x64-windows jpeg-turbo:x64-windows fftw3:x64-windows glew:x64-windows fmt:x64-windows glfw:x64-windows # Gotta check those are correct... Should be
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
In the same way as Windows, [libcorrect](https://github.com/quiet/libcorrect) is most likely not in your package manager and will have to be built from source.

Here are some generic Debian build instructions.

```
# Linux: Install dependencies
sudo apt install git build-essential cmake g++ libfftw3-dev libvolk1-dev libjpeg-dev libpng-dev libfmt-dev libglew-dev libglfw3-dev

# macOS: Install dependencies
brew install cmake volk jpeg libpng fmt glew glfw

# Build and install libcorrect
git clone https://github.com/quiet/libcorrect.git
cd libcorrect
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j4
sudo make install
cd ../..
rm -rf libcorrect

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
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j4
ln -s ../pipelines . # Symlink pipelines so it can run
ln -s ../Ro* . # Symlink fonts for the GUI version so it can run

# Run (if you want!)
./satdump-ui
```
