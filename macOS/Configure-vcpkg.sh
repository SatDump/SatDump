#!/bin/bash

if [[ -z "$GITHUB_WORKSPACE" ]]
then
    GITHUB_WORKSPACE=".."
    cd $( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )/..
fi

echo "Cloning vcpkg..."
if [[ -d vcpkg ]]
then
    rm -rf vcpkg #TODO: Replace with an "already set up" message
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

echo "Installing distributed packages..."
./vcpkg install --triplet $triplet libjpeg-turbo tiff libpng glfw3 libusb fftw3 portaudio jemalloc nng[mbedtls] zstd
mkdir build && cd build

echo "Setting up venv"
python3 -m venv venv
source venv/bin/activate
pip3 install mako

echo "Building OpenMP"
curl -LJ --output openmp-17.0.6.src.tar.xz https://github.com/llvm/llvm-project/releases/download/llvmorg-17.0.6/openmp-17.0.6.src.tar.xz
tar -xf openmp-17.0.6.src.tar.xz
cd openmp-17.0.6.src
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../../../scripts/buildsystems/vcpkg.cmake -DCMAKE_INSTALL_PREFIX=../../../installed/$triplet -DLIBOMP_INSTALL_ALIASES=OFF ..
make -j$(sysctl -n hw.logicalcpu)
make install
cd ../..
rm -rf openmp-17.0.6.src*

echo "Building cpu_features..."
git clone https://github.com/google/cpu_features --depth 1 -b v0.9.0
cd cpu_features
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../../../scripts/buildsystems/vcpkg.cmake -DBUILD_TESTING=OFF -DBUILD_EXECUTABLE=OFF -DCMAKE_INSTALL_PREFIX=../../../installed/$triplet ..
make -j$(sysctl -n hw.logicalcpu)
make install
cd ../..
rm -rf cpu_features

echo "Building Volk..."
git clone https://github.com/gnuradio/volk --depth 1 -b v3.1.0
cd volk
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../../../scripts/buildsystems/vcpkg.cmake -DCMAKE_FIND_ROOT_PATH=../../../installed/$triplet -DENABLE_TESTING=OFF -DENABLE_MODTOOL=OFF -DCMAKE_INSTALL_PREFIX=../../../installed/$triplet ..
make -j$(sysctl -n hw.logicalcpu)
make install
cd ../..
rm -rf volk

echo "Building Airspy..."
https://github.com/airspy/airspyone_host --depth 1 -b v1.0.10
cd airspyone_host/libairspy
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../../../../scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../../../../installed/$triplet ..
make -j$(sysctl -n hw.logicalcpu)
make install
cd ../../..
rm -rf airspyone_host

echo "Building Airspy HF..."
https://github.com/airspy/airspyhf --depth 1 -b 1.6.8
cd airspyhf/libairspyhf
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../../../../scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../../../../installed/$triplet ..
make -j$(sysctl -n hw.logicalcpu)
make install
cd ../../..
rm -rf airspyhf

echo "Building RTL-SDR..."
#TODO

echo "Building HackRF..."
#TODO

deactivate #Exit the venv
echo "Done!"
