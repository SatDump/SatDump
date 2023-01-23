# SatDump

![Icon](https://github.com/altillimity/satdump/raw/master/icon.png)

A generic satellite data processing software.
*Thanks Mnux for the icon!*

There now also is a [Matrix](https://matrix.to/#/#satdump:altillimity.com) room if you want to chat!

# Introduction

*Note : This is a very basic "how-to" skipping details and assuming some knowledge of what you are doing. For more details and advanced usecases, please see the detailed documention.* 

## GUI Version

### Offline processing (recorded data)

![Img](gui_example.png)

Quick-Start :
- Chose the appropriate pipeline for what you want to process
- Select the input file (baseband, frames, soft symbols...)
- Set the appropriate input level (what your file is)
- Check settings shown below are right (such as samplerate)
- Press "Start"!

![Img](gui_example2.png)  
*SatDump demodulating a DVB-S2 Baseband*

### Live processing or recording (directly from your SDR)

Quick-Start :
- Go in the "Recorder" Tab
- Select and start your SDR Devices
- Chose a pipeline
- Start it, and done!
- For recording, use the recording tab instead. Both processing and recording can be used at once.

![Img](gui_recording.png)  

## CLI Version

![Img](cli_example.png)  

### Offline processing

```
Usage : satdump [pipeline_id] [input_level] [input_file] [output_file_or_directory] [additional options as required]
Extra options (examples. Any parameter used in modules can be used here) :
  --samplerate [baseband_samplerate] --baseband_format [f32/s16/s8/u8] --dc_block --iq_swap
Sample command :
satdump metop_ahrpt baseband /home/user/metop_baseband.s16 metop_output_directory --samplerate 6e6 --baseband_format s16
```

### Live processing

```
Usage : satdump live [pipeline_id] [output_file_or_directory] [additional options as required]
Extra options (examples. Any parameter used in modules or sources can be used here) :
  --samplerate [baseband_samplerate] --baseband_format [f32/i16/i8/w8] --dc_block --iq_swap
  --source [airspy/rtlsdr/etc] --gain 20 --bias
As well as --timeout in seconds
Sample command :
satdump live metop_ahrpt metop_output_directory --source airspy --samplerate 6e6 --frequency 1701.3e6 --general_gain 18 --bias --timeout 780
```

You can find a list of all SDR Options [Here](docs/SDR-Options.md).

### Recording

```
Usage : satdump record [output_baseband (without extension!)] [additional options as required]
Extra options (examples. Any parameter used in sources can be used here) :
  --samplerate [baseband_samplerate] --baseband_format [f32/s16/s8/u8/w16] --dc_block --iq_swap
  --source [airspy/rtlsdr/etc] --gain 20 --bias
As well as --timeout in seconds
Sample command :
satdump record baseband_name --source airspy --samplerate 6e6 --frequency 1701.3e6 --general_gain 18 --bias --timeout 780
```

# Building / Installing

### Windows

On Windows the recommend method of running SatDump is getting the latest pre-built release off the [Release](https://github.com/altillimity/SatDump/releases) page, which includes everything you will need to run it.  
Those builds are made with Visual Studio 2019 for x64, so the appropriate Visual C++ Runtime will be required (though, likely to be already installed). You can get it [here](https://support.microsoft.com/en-us/topic/the-latest-supported-visual-c-downloads-2647da03-1eea-4433-9aff-95f26a218cc0).   

From there, just run either satdump-ui.exe or satdump.exe (CLI) and everything will work.

If you really want to build it yourself on Windows, see the dedicated documentation [TODO].  
*Note : Mingw builds are NOT supported, VOLK will not work.*

### Linux (or MacOS)

On Linux (or MacOS), building from source is recommended and no builds are currently provided.

Here are some generic (Debian-oriented) build instructions.

```
# Linux: Install dependencies
sudo apt install git build-essential cmake g++ pkgconf libfftw3-dev libvolk2-dev libpng-dev libluajit-5.1-dev # Core dependencies. If libvolk2-dev is not available, use libvolk1-dev
sudo apt install libnng-dev                                                                                   # If this packages is not found, follow build instructions below for NNG
sudo apt install librtlsdr-dev libhackrf-dev libairspy-dev libairspyhf-dev                                    # All libraries required for live processing (optional)
sudo apt install libglew-dev libglfw3-dev                                                                     # Only if you want to build the GUI Version (optional)
sudo apt install libzstd-dev                                                                                  # Only if you want to build with ZIQ Recording compression 
sudo apt install libomp-dev                                                                                   # Shouldn't be required in general, but in case you have errors with OMP
(optional)

# Optional, but recommended as it drastically 
# increases speed of some operations.
# Install OpenCL. Not required on MacOS
sudo apt install ocl-icd-opencl-dev

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
brew install cmake volk libpng glew glfw nng pkg-config llvm libomp luajit

# macOS ONLY: build and install libfftw3
# if you install fftw via brew, cmake won't be able to find it
wget http://www.fftw.org/fftw-3.3.9.tar.gz
tar xf fftw-3.3.9.tar.gz
rm fftw-3.3.9.tar.gz
cd fftw-3.3.9
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=false -DENABLE_FLOAT=true -DENABLE_THREADS=true -DENABLE_SSE=true -DENABLE_SSE2=true -DENABLE_AVX=true -DENABLE_AVX2=true ..
make
sudo make install
cd ../..
rm -rf fftw-3.3.9

# Finally, SatDump
git clone https://github.com/altillimity/satdump.git
cd satdump
mkdir build && cd build
# If you do not want to build the GUI Version, add -DNO_GUI=ON to the command
# If you want to disable some SDRs, you can add -DPLUGIN_HACKRF_SDR_SUPPORT=OFF or similar
cmake -DCMAKE_BUILD_TYPE=Release ..                             # MacOS
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr .. # Linux
make -j4

# To run without installing
ln -s ../pipelines .        # Symlink pipelines so it can run
ln -s ../resources .        # Symlink resources so it can run
ln -s ../satdump_cfg.json . # Symlink settings so it can run

# To install system-wide
sudo make install

# Run (if you want!)
# On Raspberry PIs, you will need to export MESA_GL_VERSION_OVERRIDE=4.5
./satdump-ui
```

### Android

On Android, the preferred source is F-Droid [INSERT LINK WHEN POSSIBLE].   

If this is not an option for you, APKs are also available on the [Release](https://github.com/altillimity/SatDump/releases) page.  

Do keep in mind that while pretty much all features are perfectly function on Android, there may be some limitations (either due to the hardware) in some places. For example, not all SDR Devices can be used.  
Supported SDR devices are :
- RTL-SDR
- Airspy
- AirspyHF
- LimeSDR Mini
- HackRF
