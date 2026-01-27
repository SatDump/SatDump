#!/bin/bash
set -Eeo pipefail

if [[ -z "$GITHUB_WORKSPACE" ]]
then
    GITHUB_WORKSPACE=".."
    cd $( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )/../build

    echo "Workspace $GITHUB_WORKSPACE"
fi

echo "---prefix"
brew --prefix liboomp

echo "--omp"
ls /opt/homebrew/opt/liboomp/

# TODOREWORK: SHIM

exit 1


# This is where the libraries are pulled & built in
working_dir="$GITHUB_WORKSPACE/deps-temp"

# Libusb from homebrew is used
HOMEBREW_LIB="/usr/local"
if [[ $(uname -m) == 'arm64' ]]; then
  echo "Configured Homebrew for Silicon"
  HOMEBREW_LIB="/opt/homebrew"
fi

# Cleans up working space
rm -rf $working_dir
rm -rf $GITHUB_WORKSPACE/deps

mkdir $working_dir
cd $working_dir
working_dir=$(pwd)
echo "Dependencies will be built in: $working_dir"

mkdir $working_dir/deps
mkdir $working_dir/deps/lib
mkdir $working_dir/deps/include

echo "Setting up venv"
python3 -m venv venv
source venv/bin/activate
pip3 install mako

build_args="-DCMAKE_INSTALL_PREFIX=$(cd ./deps && pwd) -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_DEPLOYMENT_TARGET=$osx_target -DCMAKE_MACOSX_RPATH=ON -DCMAKE_POLICY_VERSION_MINIMUM=3.5"
standard_include="$(cd ./deps/include && pwd)"
standard_lib="$(cd ./deps/lib && pwd)"
libusb_include="$HOMEBREW_LIB/opt/libusb/include/libusb-1.0"
libusb_lib="$HOMEBREW_LIB/opt/libusb/lib/libusb-1.0.0.dylib"

echo "Building cpu_features..."
git clone https://github.com/google/cpu_features --depth 1 -b v0.9.0
cd cpu_features
mkdir build && cd build
cmake $build_args -DBUILD_TESTING=OFF -DBUILD_EXECUTABLE=OFF ..
make -j$(sysctl -n hw.logicalcpu)
make install
cd ../..
rm -rf cpu_features

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
# Fobos is incredibly picky about USB dirs for some reason, when it doesn't complain,
# everything else does - hardcoded them to make it shut up
cmake $build_args -DLIBUSB_LIBRARIES=$HOMEBREW_LIB/opt/libusb/lib/libusb-1.0.dylib -DLIBUSB_INCLUDE_DIRS=$HOMEBREW_LIB/opt/libusb/include \
 -DCMAKE_INSTALL_LIBDIR=$standard_lib ..
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
sed -i '' 's/option(OSX_FRAMEWORK "Create a OSX_FRAMEWORK" ON)/option(OSX_FRAMEWORK "Create a OSX_FRAMEWORK" OFF)/' CMakeLists.txt    #Just a dylib, please!
sed -i '' 's|/Library/Frameworks|'"$PWD/deps"'|g' CMakeLists.txt  #No, don't build into /Library
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
# boost_system isn't shipped standalone anymore, this makes sure it doesn't try finding it
sed -i '' '/^set(UHD_BOOST_REQUIRED_COMPONENTS/,/^[[:space:]]*)/{/^[[:space:]]*system[[:space:]]*$/d;}' CMakeLists.txt
mkdir build && cd build
cmake $build_args -Dboost_system_DIR=$HOMEBREW_LIB/opt/boost/lib/cmake/Boost-1.90.0 -DENABLE_MAN_PAGES=OFF -DENABLE_MANUAL=OFF -DENABLE_PYTHON_API=OFF -DENABLE_EXAMPLES=OFF -DENABLE_UTILS=OFF -DENABLE_TESTS=OFF ..
make -j$(sysctl -n hw.logicalcpu)
make install
cd ../../..
rm -rf uhd

echo "Adding SDRPlay Library..."
curl -LJ --output sdrplay-macos.zip https://www.satdump.org/sdrplay-macos.zip
unzip sdrplay-macos.zip
cp sdrplay-macos/lib/* ./deps/lib
cp sdrplay-macos/include/* ./deps/include
cd ./deps/lib
ln -s libsdrplay_api.3.15.dylib libsdrplay_api.dylib
cd -
rm -rf sdrplay-macos*

deactivate #Exit the venv


echo "Cleaning up..."

mv $working_dir/deps $GITHUB_WORKSPACE/deps
rm -r $working_dir

# Sanity check
if [ ! -d $GITHUB_WORKSPACE/deps ] ; then
    echo "Something went wrong, deps directory does not exist!"
    exit 1
fi
echo "Done!"
