if(!!(Get-Command 'tf' -ErrorAction SilentlyContinue) -eq $false)
{
    Write-Error "You must run this script within Developer Powershell for Visual Studio"
    exit
}

mkdir deps
cd deps

mkdir output
$output_folder=$(Resolve-Path output)
$python_interpreter=$($(Get-Command python).Path)
$cmake_params="-G Ninja", "-DCMAKE_FIND_ROOT_PATH='$output_folder'", "-DCMAKE_INSTALL_PREFIX='$output_folder'", "-DCMAKE_BUILD_TYPE=Release", "-DCMAKE_SYSTEM_NAME=Windows", "-DPYTHON_EXECUTABLE:FILEPATH=$python_interpreter"

# ZLib
git clone https://github.com/madler/zlib --depth 1 -b v1.3.1
cd zlib
mkdir build
cd build
cmake $cmake_params .. -DBUILD_SHARED_LIBS=ON
ninja install
cd ../..

# WolfSSL
git clone https://github.com/wolfSSL/wolfssl --depth 1 -b v5.7.2-stable
cd wolfssl
mkdir build
cd build
cmake $cmake_params .. -DBUILD_SHARED_LIBS=yes -DWOLFSSL_CURL=yes -DWOLFSSL_CRYPT_TESTS=no -DWOLFSSL_EXAMPLES=no
ninja install
cd ../..

# Curl
git clone https://github.com/curl/curl --depth 1 -b curl-8_9_1
cd curl
mkdir build
cd build
cmake $cmake_params .. -DHTTP_ONLY=ON -DBUILD_STATIC_LIBS=OFF -DCURL_USE_WOLFSSL=ON -DUSE_LIBIDN2=OFF -DCURL_USE_LIBPSL=OFF
ninja install
cd ../..

# Volk
git clone https://github.com/gnuradio/volk --depth 1 -b v3.1.2
cd volk
mkdir build
cd build
git submodule update --init
cmake $cmake_params .. -DENABLE_TESTING=OFF -DENABLE_MODTOOL=OFF -DENABLE_STATIC_LIBS=OFF
ninja install
cd ../..

# NNG
git clone https://github.com/nanomsg/nng --depth 1 -b v1.8.0
cd nng
mkdir build
cd build
cmake $cmake_params .. -DNNG_TOOLS=OFF -DNNG_TESTS=OFF -DNNG_ENABLE_NNGCAT=OFF -DBUILD_SHARED_LIBS=ON
ninja install
cd ../..

# Zstd
git clone https://github.com/facebook/zstd --depth 1 -b v1.5.6
cd zstd
mkdir build2
cd build2
cmake $cmake_params ../build/cmake -DZSTD_BUILD_PROGRAMS=OFF -DZSTD_BUILD_STATIC=ON -DZSTD_BUILD_SHARED=ON -DCMAKE_RC_FLAGS="-I $output_folder/../zstd/lib"
ninja install
cd ../..

# SQLite
git clone https://github.com/sjinks/sqlite3-cmake --depth 1 -b master sqlite
cd sqlite
mkdir build
cd build
cmake $cmake_params .. -DBUILD_SHARED_LIBS=ON  -DCMAKE_WINDOWS_EXPORT_ALL_SYMBOLS=ON
ninja install
cd ../..

# LibPNG
git clone https://github.com/glennrp/libpng --depth 1 -b v1.6.43
cd libpng
mkdir build
cd build
cmake $cmake_params .. -DPNG_EXECUTABLES=OFF -DPNG_TESTS=OFF -DPNG_SHARED=ON
ninja install
cd ../..

# TIFF
git clone https://github.com/libsdl-org/libtiff --depth 1 -b v4.6.0
cd libtiff
mkdir build
cd build
cmake $cmake_params .. -DBUILD_SHARED_LIBS=ON -Dtiff-tools=OFF -Dtiff-tests=OFF -Dtiff-docs=OFF
ninja install
cd ../..

# HDF5
git clone https://github.com/HDFGroup/hdf5 --depth 1 -b  2.0.0
cd hdf5
mkdir build
cd build
cmake $cmake_params .. -DBUILD_SHARED_LIBS=ON -DHDF5_BUILD_CPP_LIB=1 -DHDF5_ENABLE_ZLIB_SUPPORT=ON -DBUILD_TESTING=OFF
ninja install
cd ../..

# FFTW
Invoke-WebRequest -Uri http://www.fftw.org/fftw-3.3.10.tar.gz -OutFile fftw.tar.gz
tar -zxf fftw.tar.gz
cd fftw-3.3.10
mkdir build
cd build
cmake $cmake_params .. -DBUILD_SHARED_LIBS=ON -DENABLE_FLOAT=ON -DCMAKE_POLICY_VERSION_MINIMUM="3.5"
ninja install
cd ../..

# pthread
git clone https://github.com/GerHobbelt/pthread-win32 --depth 1 -b v4.1.0.9
cd pthread-win32
mkdir build
cd build
cmake $cmake_params .. -DBUILD_SHARED_LIBS=ON
ninja install
cd ../..

# GLFW
git clone https://github.com/glfw/glfw --depth 1 -b 3.5.1
cd glfw
mkdir build
cd build
cmake $cmake_params .. -DBUILD_SHARED_LIBS=ON
ninja install
cd ../..

# Libusb
git clone https://github.com/libusb/libusb-cmake libusb --depth 1 -b v1.0.30
cd libusb
mkdir build
cd build
cmake $cmake_params .. -DBUILD_SHARED_LIBS=ON
ninja install
cd ../..

# Libairspy
git clone https://github.com/airspy/airspyone_host libairspy --depth 1 -b v1.0.10
cd libairspy
mkdir build
cd build
cmake $cmake_params ..  -DCMAKE_WINDOWS_EXPORT_ALL_SYMBOLS=ON -DBUILD_SHARED_LIBS=ON -DCMAKE_POLICY_VERSION_MINIMUM="3.5" -DLIBUSB_INCLUDE_DIR="$output_folder/include/libusb-1.0" -DTHREADS_PTHREADS_WIN32_LIBRARY="$output_folder/lib/pthreadVC3.lib"
ninja install
cd ../..

# HydraSDR
git clone https://github.com/hydrasdr/hydrasdr-host libhydrasdr --depth 1 -b v1.1.2
cd libhydrasdr
mkdir build
cd build
cmake $cmake_params .. -DBUILD_SHARED_LIBS=ON -DCMAKE_POLICY_VERSION_MINIMUM="3.5" -DLIBUSB_INCLUDE_DIR="$output_folder/include/libusb-1.0" -DLIBUSB_LIBRARIES="$output_folder/lib/usb-1.0.lib" -DTHREADS_PTHREADS_WIN32_LIBRARY="$output_folder/lib/pthreadVC3.lib"
ninja install
cd ../..

# RTL-SDR
git clone https://github.com/rtlsdrblog/rtl-sdr-blog --depth 1 -b master
cd rtl-sdr-blog
mkdir build
cd build
cmake $cmake_params .. -DCMAKE_DISABLE_FIND_PACKAGE_PkgConfig=ON -DBUILD_SHARED_LIBS=ON -DCMAKE_POLICY_VERSION_MINIMUM="3.5" -DLIBUSB_INCLUDE_DIRS="$output_folder/include/libusb-1.0" -DLIBUSB_LIBRARIES="$output_folder/lib/usb-1.0.lib" -DTHREADS_PTHREADS_LIBRARY="$output_folder/lib/pthreadVC3.lib" -DTHREADS_PTHREADS_INCLUDE_DIR="$output_folder/include/"
ninja install
cd ../..

# Portaudio
git clone https://github.com/PortAudio/portaudio --depth 1 -b v19.7.0
cd portaudio
mkdir build
cd build
cmake $cmake_params .. -DBUILD_SHARED_LIBS=ON -DCMAKE_POLICY_VERSION_MINIMUM="3.5"
ninja install
cd ../..

# OpenCL
git clone https://github.com/KhronosGroup/OpenCL-SDK --depth 1 -b v2026.05.29 --recursive
cd OpenCL-SDK
mkdir build
cd build
cmake $cmake_params .. -DBUILD_SHARED_LIBS=ON
ninja install
cd ../..

# LimeSuite
git clone https://github.com/myriadrf/Limesuite --depth 1 -b v23.11.0
cd Limesuite
mkdir build
cd build
cmake $cmake_params .. -DBUILD_SHARED_LIBS=ON -DCMAKE_POLICY_VERSION_MINIMUM="3.5"
ninja install
cd ../..

# libxml2
git clone https://github.com/gnome/libxml2 --depth 1 -b v2.15.4
cd libxml2
mkdir build
cd build
cmake $cmake_params .. -DBUILD_SHARED_LIBS=ON -DLIBXML2_WITH_ICONV=OFF
ninja install
cd ../..

# LIBIIO
git clone https://github.com/analogdevicesinc/libiio --depth 1 -b v0.26
cd libiio
mkdir build
cd build
cmake $cmake_params .. -DBUILD_SHARED_LIBS=ON
ninja install
cd ../..

# libad9361
git clone https://github.com/analogdevicesinc/libad9361-iio --depth 1 -b v0.4.0
cd libad9361-iio
mkdir build
cd build
cmake $cmake_params .. -DBUILD_SHARED_LIBS=ON
ninja install
cd ../..

# LibairspyHF
git clone https://github.com/airspy/airspyhf --depth 1 -b master
cd airspyhf
mkdir build
cd build
cmake $cmake_params .. -DCMAKE_WINDOWS_EXPORT_ALL_SYMBOLS=ON -DBUILD_SHARED_LIBS=ON -DCMAKE_POLICY_VERSION_MINIMUM="3.5" -DLIBUSB_INCLUDE_DIR="$output_folder/include/libusb-1.0" -DTHREADS_PTHREADS_WIN32_LIBRARY="$output_folder/lib/pthreadVC3.lib"
ninja install
cd ../..

# Libbladerf
git clone https://github.com/nuand/bladeRF --depth 1 -b libbladeRF_v2.6.0
cd bladeRF
mkdir build
cd build
cmake $cmake_params .. -DBUILD_SHARED_LIBS=ON -DTREAT_WARNINGS_AS_ERRORS=NO
ninja install
cd ../..
cp $output_folder/lib/bladeRF.dll $output_folder/bin

# Fobos
git clone https://github.com/rigexpert/libfobos --depth 1 -b v2.4.0
cd libfobos
mkdir libusb/include
cp -r $output_folder/include/libusb-1.0 libusb/include
mkdir libusb/MS64
mkdir libusb/MS64/dll
mkdir libusb/MS32
mkdir libusb/MS32/dll
cp $output_folder/lib/usb-1.0.lib libusb/MS64/dll/libusb-1.0.lib
cp $output_folder/lib/usb-1.0.lib libusb/MS32/dll/libusb-1.0.lib
cp $output_folder/bin/libusb-1.0.dll libusb/MS64/dll/libusb-1.0.dll
cp $output_folder/bin/libusb-1.0.dll libusb/MS32/dll/libusb-1.0.dll
mkdir build
cp -r libusb build
cd build
mkdir Release
mkdir Debug
cmake $cmake_params ..  -DCMAKE_DISABLE_FIND_PACKAGE_PkgConfig=ON -DBUILD_SHARED_LIBS=ON -DCMAKE_POLICY_VERSION_MINIMUM="3.5" -DLIBUSB_LIBRARIES="./libusb/MS64/dll"
ninja install
cd ../..

# HackRF
git clone https://github.com/greatscottgadgets/hackrf --depth 1 -b v2026.01.3
cd hackrf
mkdir build
cd build
cmake $cmake_params ../host -DCMAKE_WINDOWS_EXPORT_ALL_SYMBOLS=ON -DBUILD_SHARED_LIBS=ON -DCMAKE_POLICY_VERSION_MINIMUM="3.5" -DLIBUSB_INCLUDE_DIR="$output_folder/include/libusb-1.0" -DCMAKE_C_FLAGS="-I$output_folder/include" -DCMAKE_USE_PTHREADS_INIT=ON -DTHREADS_FOUND=TRUE -DCMAKE_THREAD_LIBS_INIT="$output_folder/lib/pthreadVC3.lib"
ninja install
cd ../..

# Boost
git clone https://github.com/boostorg/boost --depth 1 -b boost-1.92.0 --recursive
cd boost
mkdir build
cd build
cmake $cmake_params .. -DBUILD_SHARED_LIBS=ON
ninja install
cd ../..
cp -r $output_folder/include/boost-1_92/boost $output_folder/include

# Protobuf
git clone https://github.com/protocolbuffers/protobuf --depth 1 -b v36.1
cd protobuf
mkdir build
cd build
cmake $cmake_params .. -DBUILD_SHARED_LIBS=ON
ninja install
cd ../..

# LIBUHD
git clone https://github.com/ettusresearch/uhd --depth 1 -b v4.10.0.0
cd uhd
mkdir build
cd build
cmake $cmake_params ../host -DCMAKE_CXX_FLAGS="/EHsc /FIwinsock2.h" -DCMAKE_POLICY_VERSION_MINIMUM="3.5" -DBUILD_SHARED_LIBS=ON -DENABLE_MAN_PAGES=OFF -DENABLE_MANUAL=OFF -DENABLE_PYTHON_API=OFF -DENABLE_EXAMPLES=OFF -DENABLE_UTILS=OFF -DENABLE_TESTS=OFF 
ninja install
cd ../..

cd ..