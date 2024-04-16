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
    cp ../macOS/arm64-osx-satdump.cmake triplets
    triplet="arm64-osx-satdump"
else
    cp ../macOS/x64-osx-satdump.cmake triplets
    triplet="x64-osx-satdump"
fi
./bootstrap-vcpkg.sh

echo "Installing vcpkg packages..."
./vcpkg install --triplet $triplet libjpeg-turbo tiff libpng glfw3 libusb fftw3 portaudio jemalloc nng[mbedtls] zstd
mkdir build && cd build

echo "Setting up venv"
python3 -m venv venv
source venv/bin/activate
pip3 install mako

build_args="-DCMAKE_TOOLCHAIN_FILE=$(cd ../scripts/buildsystems && pwd)/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=$triplet -DCMAKE_INSTALL_PREFIX=$(cd ../installed/$triplet && pwd) -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15"
libusb_include="$(cd ../installed/$triplet/include/libusb-1.0 && pwd)"
libusb_lib="$(cd ../installed/$triplet/lib && pwd)/libusb-1.0.0.dylib"

echo "Building OpenMP"
mkdir libomp && cd libomp
curl -LJ --output openmp-17.0.6.src.tar.xz https://github.com/llvm/llvm-project/releases/download/llvmorg-17.0.6/openmp-17.0.6.src.tar.xz
curl -LJ --output cmake-17.0.6.src.tar.xz https://github.com/llvm/llvm-project/releases/download/llvmorg-17.0.6/cmake-17.0.6.src.tar.xz
tar -xf openmp-17.0.6.src.tar.xz
tar -xf cmake-17.0.6.src.tar.xz
mv cmake-17.0.6.src cmake
cd openmp-17.0.6.src
mkdir build && cd build
cmake build_args -DLIBOMP_INSTALL_ALIASES=OFF ..
make -j$(sysctl -n hw.logicalcpu)
make install
cd ../../..
rm -rf libomp

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
git clone https://github.com/gnuradio/volk --depth 1 -b v3.1.0
cd volk
mkdir build && cd build
cmake $build_args -DENABLE_TESTING=OFF -DENABLE_MODTOOL=OFF -DENABLE_ORC=OFF ..
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
cd ../..
rm -rf hackrf

deactivate #Exit the venv
echo "Done!"
