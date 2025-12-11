#!/bin/bash
set -Eeo pipefail
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
git checkout 74e6536

if [[ "$(uname -m)" == "arm64" ]]
then
    cp ../macOS/arm64-osx-satdump.cmake triplets/osx-satdump.cmake
    osx_target="15.0"
else
    cp ../macOS/x64-osx-satdump.cmake triplets/osx-satdump.cmake
    osx_target="10.15"
fi
./bootstrap-vcpkg.sh

echo "Installing vcpkg packages..."

# Core packages. libxml2 is for libiio
./vcpkg install --triplet osx-satdump libjpeg-turbo tiff libpng  libusb fftw3 libxml2 portaudio jemalloc nng zstd armadillo hdf5[cpp] sqlite3

# Entirely for UHD...
./vcpkg install --triplet osx-satdump boost-chrono boost-date-time boost-filesystem boost-program-options boost-system boost-serialization boost-thread \
                                      boost-test boost-format boost-asio boost-math boost-graph boost-units boost-lockfree boost-circular-buffer        \
                                      boost-assign boost-dll

# Remove nested symlinks on known problematic libs
for dylib in libz.dylib libzstd.dylib libhdf5.dylib libhdf5_hl.dylib
do
    target_dylib=$(readlink installed/osx-satdump/lib/$dylib)
    final_dylib=$(readlink installed/osx-satdump/lib/$target_dylib)
    if [[ -n final_dylib ]]
    then
        mv installed/osx-satdump/lib/$final_dylib installed/osx-satdump/lib/$target_dylib
    fi
done

mkdir build && cd build

#Used for volk and uhd builds
echo "Setting up venv"
python3 -m venv venv
source venv/bin/activate
pip3 install mako

build_args="-DCMAKE_TOOLCHAIN_FILE=$(cd ../scripts/buildsystems && pwd)/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=osx-satdump -DCMAKE_INSTALL_PREFIX=$(cd ../installed/osx-satdump && pwd) -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_DEPLOYMENT_TARGET=$osx_target -DCMAKE_MACOSX_RPATH=ON -DCMAKE_POLICY_VERSION_MINIMUM=3.5"
standard_include="$(cd ../installed/osx-satdump/include && pwd)"
standard_lib="$(cd ../installed/osx-satdump/lib && pwd)"
libusb_include="$(cd $standard_include/libusb-1.0 && pwd)"
libusb_lib="$standard_lib/libusb-1.0.0.dylib"

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
git clone https://github.com/airspy/airspyone_host --depth 1 #-b v1.0.10
cd airspyone_host/libairspy
mkdir build && cd build
cmake $build_args -DLIBUSB_INCLUDE_DIR=$libusb_include -DLIBUSB_LIBRARIES=$libusb_lib ..
make -j$(sysctl -n hw.logicalcpu)
make install
cd ../../..
rm -rf airspyone_host

echo "Building Airspy HF..."
git clone https://github.com/airspy/airspyhf --depth 1 #-b 1.6.8
cd airspyhf/libairspyhf
mkdir build && cd build
cmake $build_args -DLIBUSB_INCLUDE_DIR=$libusb_include -DLIBUSB_LIBRARIES=$libusb_lib ..
make -j$(sysctl -n hw.logicalcpu)
make install
cd ../../..
rm -rf airspyhf

echo "Building RTL-SDR..."
git clone https://github.com/osmocom/rtl-sdr --depth 1 -b v2.0.2
cd rtl-sdr
mkdir build && cd build
cmake $build_args -DLIBUSB_INCLUDE_DIRS=$libusb_include -DLIBUSB_LIBRARIES=$libusb_lib ..
make -j$(sysctl -n hw.logicalcpu)
make install
cd ../..
rm -rf rtl-sdr

echo "Building HackRF..."
git clone https://github.com/greatscottgadgets/hackrf --depth 1
cd hackrf/host/libhackrf
mkdir build && cd build
cmake $build_args -DLIBUSB_INCLUDE_DIR=$libusb_include -DLIBUSB_LIBRARIES=$libusb_lib ..
make -j$(sysctl -n hw.logicalcpu)
make install
cd ../../../..
rm -rf hackrf

echo "Building HydraSDR..."
git clone https://github.com/hydrasdr/rfone_host -b v1.0.1
cd rfone_host/libhydrasdr
mkdir build && cd build
cmake $build_args -DLIBUSB_INCLUDE_DIR=$libusb_include -DLIBUSB_LIBRARIES=$libusb_lib ..
make -j$(sysctl -n hw.logicalcpu)
make install
cd ../../..
rm -rf rfone_host

echo "Building FobosSDR..."
git clone https://github.com/rigexpert/libfobos -b v.2.2.2
cd libfobos
sed -i '' 's/#target_link_libraries/target_link_libraries/' CMakeLists.txt
sed -i '' 's/"udev"/"udev" EXCLUDE_FROM_ALL/' CMakeLists.txt
mkdir build && cd build
cmake $build_args -DLIBUSB_INCLUDE_DIRS=$standard_include -DCMAKE_INSTALL_LIBDIR=$standard_lib ..
make -j$(sysctl -n hw.logicalcpu)
make install
cd ../..
rm -rf libfobos

echo "Building LimeSuite..."
git clone https://github.com/myriadrf/LimeSuite --depth 1
cd LimeSuite
mkdir build-dir && cd build-dir
cmake $build_args -DENABLE_GUI=OFF ..
make -j$(sysctl -n hw.logicalcpu)
make install
cd ../..
rm -rf LimeSuite

echo "Building libiio..."
git clone https://github.com/analogdevicesinc/libiio
cd libiio
git checkout a0eca0d
mkdir build && cd build
cmake $build_args -DWITH_IIOD=OFF -DOSX_FRAMEWORK=OFF -DWITH_TESTS=OFF -DWITH_ZSTD=ON ..
make -j$(sysctl -n hw.logicalcpu)
make install
cd ../..
rm -rf libiio

echo "Building libad9361-iio..."
git clone https://github.com/analogdevicesinc/libad9361-iio
cd libad9361-iio
git checkout 2264074
sed -i '' 's/<iio\/iio.h>/<iio.h>/g' test/*.c    #Patch tests for macOS
sed -i '' 's/FRAMEWORK TRUE//' CMakeLists.txt    #Just a dylib, please!
sed -i '' 's|/Library/Frameworks|'"$PWD/../installed/osx-satdump"'|g' CMakeLists.txt  #No, don't build into /Library
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

echo "Building UHD..."
git clone https://github.com/EttusResearch/uhd --depth 1 -b v4.9.0.0
cd uhd/host
sed -i '' 's/ appropriately or"/");/g' lib/utils/paths.cpp                          #Disable non-applicable help
sed -i '' '/follow the below instructions to download/{N;d;}' lib/utils/paths.cpp
mkdir build && cd build
cmake $build_args -DENABLE_MAN_PAGES=OFF -DENABLE_MANUAL=OFF -DENABLE_PYTHON_API=OFF -DENABLE_EXAMPLES=OFF -DENABLE_UTILS=OFF -DENABLE_TESTS=OFF ..
make -j$(sysctl -n hw.logicalcpu)
make install
cd ../../..
rm -rf uhd

echo "Adding SDRPlay Library..."
curl -LJ --output sdrplay-macos.zip https://www.satdump.org/sdrplay-macos.zip
unzip sdrplay-macos.zip
cp sdrplay-macos/lib/* ../installed/osx-satdump/lib
cp sdrplay-macos/include/* ../installed/osx-satdump/include
cd ../installed/osx-satdump/lib
ln -s libsdrplay_api.3.15.dylib libsdrplay_api.dylib
cd -
rm -rf sdrplay-macos*

deactivate #Exit the venv
echo "Done!"
