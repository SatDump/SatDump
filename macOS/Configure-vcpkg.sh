#!/bin/bash

if [[ -z "$GITHUB_WORKSPACE" ]]
then
    GITHUB_WORKSPACE=".."
    cd $( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )/..
fi

if [[ -d vcpkg ]]
then
    rm -rf vcpkg
fi
git clone https://github.com/microsoft/vcpkg
cd vcpkg
git checkout ad3bae5
if [[ "$(uname -m)" == "arm64" ]]
then
    cp ../macOS/arm64-osx-satdump.cmake triplets/osx-satdump.cmake
    osx_target="11.0"
else
    cp ../macOS/x64-osx-satdump.cmake triplets/osx-satdump.cmake
    osx_target="10.15"
fi
./bootstrap-vcpkg.sh

echo "Installing vcpkg packages..."
./vcpkg install --triplet osx-satdump libjpeg-turbo tiff libpng glfw3 libusb fftw3 libxml2 portaudio jemalloc nng[mbedtls] zstd armadillo
mkdir build && cd build

echo "Setting up venv"
python3 -m venv venv
source venv/bin/activate
pip3 install mako

build_args="-DCMAKE_TOOLCHAIN_FILE=$(cd ../scripts/buildsystems && pwd)/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=osx-satdump -DCMAKE_INSTALL_PREFIX=$(cd ../installed/osx-satdump && pwd) -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_DEPLOYMENT_TARGET=$osx_target -DCMAKE_MACOSX_RPATH=ON"
libusb_include="$(cd ../installed/osx-satdump/include/libusb-1.0 && pwd)"
libusb_lib="$(cd ../installed/osx-satdump/lib && pwd)/libusb-1.0.0.dylib"

echo "Building OpenMP"
mkdir libomp && cd libomp
curl -LJ --output openmp-17.0.6.src.tar.xz https://github.com/llvm/llvm-project/releases/download/llvmorg-17.0.6/openmp-17.0.6.src.tar.xz
curl -LJ --output cmake-17.0.6.src.tar.xz https://github.com/llvm/llvm-project/releases/download/llvmorg-17.0.6/cmake-17.0.6.src.tar.xz
tar -xf openmp-17.0.6.src.tar.xz
tar -xf cmake-17.0.6.src.tar.xz
mv cmake-17.0.6.src cmake
cd openmp-17.0.6.src
mkdir build && cd build
cmake $build_args -DLIBOMP_INSTALL_ALIASES=OFF ..
make -j$(sysctl -n hw.logicalcpu)
make install
cd ../../..
rm -rf libomp

echo "Building orc"
git clone https://github.com/GStreamer/orc --depth 1 -b 0.4.38
cd orc
MACOSX_DEPLOYMENT_TARGET=$osx_target meson setup --buildtype=release --prefix=$(cd ../../installed/osx-satdump && pwd) -Dgtk_doc=disabled build
MACOSX_DEPLOYMENT_TARGET=$osx_target meson compile -C build --verbose
MACOSX_DEPLOYMENT_TARGET=$osx_target meson install -C build
cd ..
rm -rf orc

echo "Building cpu_features..."
git clone https://github.com/google/cpu_features --depth 1 -b v0.9.0
cd cpu_features
mkdir build && cd build
cmake $build_args -DBUILD_TESTING=OFF -DBUILD_EXECUTABLE=OFF ..
make -j$(sysctl -n hw.logicalcpu)
make install
cd ../..
rm -rf cpu_features

echo "Building Volk..."
git clone https://github.com/gnuradio/volk --depth 1 -b v3.1.2
cd volk
mkdir build && cd build
cmake $build_args -DENABLE_TESTING=OFF -DENABLE_MODTOOL=OFF ..
make -j$(sysctl -n hw.logicalcpu)
make install
cd ../..
rm -rf volk

echo "Building Airspy..."
git clone https://github.com/airspy/airspyone_host --depth 1 -b v1.0.10
cd airspyone_host/libairspy
mkdir build && cd build
cmake $build_args -DLIBUSB_INCLUDE_DIR=$libusb_include -DLIBUSB_LIBRARIES=$libusb_lib ..
make -j$(sysctl -n hw.logicalcpu)
make install
cd ../../..
rm -rf airspyone_host

echo "Building Airspy HF..."
git clone https://github.com/airspy/airspyhf --depth 1 -b 1.6.8
cd airspyhf/libairspyhf
mkdir build && cd build
cmake $build_args -DLIBUSB_INCLUDE_DIR=$libusb_include -DLIBUSB_LIBRARIES=$libusb_lib ..
make -j$(sysctl -n hw.logicalcpu)
make install
cd ../../..
rm -rf airspyhf

echo "Building RTL-SDR..."
git clone https://github.com/osmocom/rtl-sdr --depth 1 -b v2.0.1
cd rtl-sdr
mkdir build && cd build
cmake $build_args -DLIBUSB_INCLUDE_DIRS=$libusb_include -DLIBUSB_LIBRARIES=$libusb_lib ..
make -j$(sysctl -n hw.logicalcpu)
make install
cd ../..
rm -rf rtl-sdr

echo "Building HackRF..."
git clone https://github.com/greatscottgadgets/hackrf --depth 1 -b v2024.02.1
cd hackrf/host/libhackrf
mkdir build && cd build
cmake $build_args -DLIBUSB_INCLUDE_DIR=$libusb_include -DLIBUSB_LIBRARIES=$libusb_lib ..
make -j$(sysctl -n hw.logicalcpu)
make install
cd ../../../..
rm -rf hackrf

echo "Building LimeSuite..."
git clone https://github.com/myriadrf/LimeSuite --depth 1 -b v23.11.0
cd LimeSuite
mkdir build-dir && cd build-dir
cmake $build_args -DENABLE_GUI=OFF ..
make -j$(sysctl -n hw.logicalcpu)
make install
cd ../..
rm -rf LimeSuite

echo "Building libiio..."
git clone https://github.com/analogdevicesinc/libiio --depth 1 -b v0.25
cd libiio
mkdir build && cd build
cmake $build_args -DWITH_IIOD=OFF -DOSX_FRAMEWORK=OFF -DWITH_TESTS=OFF -DWITH_ZSTD=ON ..
make -j$(sysctl -n hw.logicalcpu)
make install
cd ../..
rm -rf libiio

echo "Building libad9361-iio..."
git clone https://github.com/analogdevicesinc/libad9361-iio --depth 1 -b v0.3
cd libad9361-iio
sed -i '' 's/<iio\/iio.h>/<iio.h>/g' test/*.c    #Patch tests for macOS
sed -i '' 's/FRAMEWORK TRUE//' CMakeLists.txt    #Just a dylib, please!
mkdir build && cd build
cmake $build_args -DOSX_PACKAGE=OFF -DWITH_DOC=OFF -DENABLE_PACKAGING=OFF ..
make -j$(sysctl -n hw.logicalcpu)
make install
cd ../..
rm -rf libad9361-iio

echo "Building bladeRF..."
git clone https://github.com/Nuand/bladeRF
cd bladeRF
git checkout 2fbae2c
cd host && mkdir build && cd build
cmake $build_args -DTEST_LIBBLADERF=OFF ..
make -j$(sysctl -n hw.logicalcpu)
make install
cd ../../..
rm -rf bladeRF

echo "Adding SDRPlay Libs..."
curl -LJ --output sdrplay-macos.zip https://www.satdump.org/sdrplay-macos.zip
unzip sdrplay-macos.zip
cp sdrplay-macos/lib/* ../installed/osx-satdump/lib
cp sdrplay-macos/include/* ../installed/osx-satdump/include
cd ../installed/osx-satdump/lib
ln -s libsdrplay_api.3.14.dylib libsdrplay_api.dylib
cd -
rm -rf sdrplay-macos*

deactivate #Exit the venv
echo "Done!"
