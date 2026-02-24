#!/bin/bash
set -Eeo pipefail
if [[ -z "$GITHUB_WORKSPACE" ]]
then
    GITHUB_WORKSPACE=".."
    cd $( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )/../build
fi

HOMEBREW_LIB="/usr/local"
PLATFORM="Intel"
if [[ $(uname -m) == 'arm64' ]]; then
    PLATFORM="Silicon"
    HOMEBREW_LIB="/opt/homebrew"
fi


if [[ -n "$MACOS_CERTIFICATE" && -n "$MACOS_CERTIFICATE_PWD" ]]
then
    echo "Extracting signing certificate..."
    echo $MACOS_CERTIFICATE | base64 --decode > certificate.p12
    security create-keychain -p $MACOS_CERTIFICATE_PWD build.keychain
    security default-keychain -s build.keychain
    security unlock-keychain -p $MACOS_CERTIFICATE_PWD build.keychain
    security import certificate.p12 -k build.keychain -P $MACOS_CERTIFICATE_PWD -T /usr/bin/codesign
    security set-key-partition-list -S apple-tool:,apple:,codesign: -s -k $MACOS_CERTIFICATE_PWD build.keychain
fi

rm -rf MacApp
rm -rf SatDump-macOS-$PLATFORM.dmg

echo "Making app shell..." 
mkdir -p MacApp/SatDump.app/Contents/MacOS
mkdir -p MacApp/SatDump.app/Contents/Resources/plugins
cp -r $GITHUB_WORKSPACE/resources MacApp/SatDump.app/Contents/Resources/resources
cp $GITHUB_WORKSPACE/satdump_cfg.json MacApp/SatDump.app/Contents/Resources
cp $GITHUB_WORKSPACE/macOS/Info.plist MacApp/SatDump.app/Contents
cp $GITHUB_WORKSPACE/macOS/Readme.rtf MacApp/Readme.rtf

echo "Creating app icon..." 
mkdir macOSIcon.iconset
sips -z 16 16     $GITHUB_WORKSPACE/icon.png --out macOSIcon.iconset/icon_16x16.png
sips -z 32 32     $GITHUB_WORKSPACE/icon.png --out macOSIcon.iconset/icon_16x16@2x.png
sips -z 32 32     $GITHUB_WORKSPACE/icon.png --out macOSIcon.iconset/icon_32x32.png
sips -z 64 64     $GITHUB_WORKSPACE/icon.png --out macOSIcon.iconset/icon_32x32@2x.png
sips -z 128 128   $GITHUB_WORKSPACE/icon.png --out macOSIcon.iconset/icon_128x128.png
sips -z 256 256   $GITHUB_WORKSPACE/icon.png --out macOSIcon.iconset/icon_128x128@2x.png
sips -z 256 256   $GITHUB_WORKSPACE/icon.png --out macOSIcon.iconset/icon_256x256.png
sips -z 512 512   $GITHUB_WORKSPACE/icon.png --out macOSIcon.iconset/icon_256x256@2x.png
sips -z 512 512   $GITHUB_WORKSPACE/icon.png --out macOSIcon.iconset/icon_512x512.png
sips -z 1024 1024 $GITHUB_WORKSPACE/icon.png --out macOSIcon.iconset/icon_512x512@2x.png
iconutil -c icns -o MacApp/SatDump.app/Contents/Resources/icon.icns macOSIcon.iconset
rm -rf macOSIcon.iconset

echo "Copying binaries..."
cp satdump-ui MacApp/SatDump.app/Contents/MacOS
cp satdump MacApp/SatDump.app/Contents/MacOS
cp satdump_sdr_server MacApp/SatDump.app/Contents/MacOS
cp plugins/*.dylib MacApp/SatDump.app/Contents/Resources/plugins

if [[ -n "$MACOS_SIGNING_SIGNATURE" ]]
then
    SIGN_FLAG="-ns"
fi

# Omp, openblas, and gfortran are not in $HOMEBREW_LIB/lib for some ungodly reason; we include their full paths instead
echo "Packaging and re-linking libraries..."
plugin_args=$(ls MacApp/SatDump.app/Contents/Resources/plugins | xargs printf -- '-x MacApp/SatDump.app/Contents/Resources/plugins/%s ')
dylibbundler $SIGN_FLAG \
  -cd \
  -b \
  -of \
  -s . \
  -s $GITHUB_WORKSPACE/deps/lib \
  -s $HOMEBREW_LIB/lib \
  -s $HOMEBREW_LIB/opt/libomp/lib \
  -s $HOMEBREW_LIB/opt/openblas/lib \
  -s $HOMEBREW_LIB/opt/gfortran/lib/gcc/current \
  -d MacApp/SatDump.app/Contents/libs \
  -x MacApp/SatDump.app/Contents/MacOS/satdump-ui \
  -x MacApp/SatDump.app/Contents/MacOS/satdump_sdr_server \
  -x MacApp/SatDump.app/Contents/MacOS/satdump \
  $plugin_args

# SDRPlay is custom, not staticaly linked; we can copy it manually
cp $GITHUB_WORKSPACE/deps/lib/libsdrplay*.dylib MacApp/SatDump.app/Contents/libs


# Some libraries are processed more than once, dylibbundler is silly and doesn't check whether
# it injects multiple LC_RPATH entries. MacOS is pissy about it and refuses to work with more than one,
# so we have to remove the duplicates manually.
echo "Removing duplicate RPATH entries..."
find MacApp/SatDump.app/Contents/libs -name "*.dylib" | while read lib; do
    rpaths=($(otool -l "$lib" | awk '/LC_RPATH/{getline; getline; sub(/.*path /,""); sub(/ .*/,""); print}'))
    seen=()
    for rp in "${rpaths[@]}"; do
        if [[ " ${seen[*]} " != *" $rp "* ]]; then
            seen+=("$rp")
        else
            install_name_tool -delete_rpath "$rp" "$lib"
        fi
    done

    # We have to resign the libraries afterwards as we changed their Mach-O headers
    codesign -v --force --timestamp --sign - "$lib"
done


if [[ -n "$MACOS_SIGNING_SIGNATURE" ]]
then
    echo "Signing code using proper signature..."
    for dylib in MacApp/SatDump.app/Contents/libs/*.dylib
    do
	    codesign -v --force --timestamp --sign "$MACOS_SIGNING_SIGNATURE" $dylib
    done

    for dylib in MacApp/SatDump.app/Contents/Resources/plugins/*.dylib
    do
	    codesign -v --force --timestamp --sign "$MACOS_SIGNING_SIGNATURE" $dylib
    done

    codesign -v --force --options runtime --entitlements $GITHUB_WORKSPACE/macOS/Entitlements.plist --timestamp --sign "$MACOS_SIGNING_SIGNATURE" MacApp/SatDump.app/Contents/MacOS/satdump
    codesign -v --force --options runtime --entitlements $GITHUB_WORKSPACE/macOS/Entitlements.plist --timestamp --sign "$MACOS_SIGNING_SIGNATURE" MacApp/SatDump.app/Contents/MacOS/satdump_sdr_server
    codesign -v --force --options runtime --entitlements $GITHUB_WORKSPACE/macOS/Entitlements.plist --timestamp --sign "$MACOS_SIGNING_SIGNATURE" MacApp/SatDump.app/Contents/MacOS/satdump-ui
else 
    echo "No signature found, signing with ad-hoc signature..."

    codesign -v --force --options runtime --entitlements $GITHUB_WORKSPACE/macOS/Entitlements.plist --timestamp --sign - MacApp/SatDump.app/Contents/MacOS/satdump
    codesign -v --force --options runtime --entitlements $GITHUB_WORKSPACE/macOS/Entitlements.plist --timestamp --sign - MacApp/SatDump.app/Contents/MacOS/satdump_sdr_server
    codesign -v --force --options runtime --entitlements $GITHUB_WORKSPACE/macOS/Entitlements.plist --timestamp --sign - MacApp/SatDump.app/Contents/MacOS/satdump-ui

    codesign --force --deep --sign - MacApp/SatDump.app
fi

echo "Creating SatDump.dmg..."
hdiutil create -srcfolder MacApp/ -volname SatDump SatDump-macOS-$PLATFORM.dmg

if [[ -n "$MACOS_SIGNING_SIGNATURE" ]]
then
    codesign -v --force --timestamp --sign "$MACOS_SIGNING_SIGNATURE" SatDump-macOS-$PLATFORM.dmg

    if [[ -n "$MACOS_NOTARIZATION_UN" && -n "$MACOS_NOTARIZATION_PWD" && -n "$MACOS_TEAM" ]]
    then
        echo "Notarizing DMG..."
        xcrun notarytool submit SatDump-macOS-$PLATFORM.dmg --apple-id $MACOS_NOTARIZATION_UN --password $MACOS_NOTARIZATION_PWD --team-id $MACOS_TEAM --wait
        xcrun stapler staple SatDump-macOS-$PLATFORM.dmg
    fi
fi

echo "Done!"
