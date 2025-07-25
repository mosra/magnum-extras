#!/bin/bash
set -ev

git submodule update --init

# Corrade
git clone --depth 1 https://github.com/mosra/corrade.git
cd corrade

# Build native corrade-rc
mkdir build && cd build
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps-native \
    -DCORRADE_WITH_INTERCONNECT=OFF \
    -DCORRADE_WITH_PLUGINMANAGER=OFF \
    -DCORRADE_WITH_TESTSUITE=OFF \
    -DCORRADE_WITH_UTILITY=OFF \
    -G Ninja
ninja install
cd ..

# Crosscompile Corrade
mkdir build-ios && cd build-ios
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../../toolchains/generic/iOS.cmake \
    -DCMAKE_OSX_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk \
    -DCMAKE_OSX_ARCHITECTURES="x86_64" \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DCORRADE_BUILD_STATIC=ON \
    -DCORRADE_TESTSUITE_TARGET_XCTEST=ON \
    -DCORRADE_WITH_INTERCONNECT=OFF \
    -G Xcode
set -o pipefail && cmake --build . --config Release --target install -j$XCODE_JOBS | xcbeautify
cd ../..

# Crosscompile SDL. On 2022-14-02 curl says the certificate is expired, so
# ignore that.
# TODO use a CMake build instead
curl --insecure -O https://www.libsdl.org/release/SDL2-2.0.10.tar.gz
tar -xzvf SDL2-2.0.10.tar.gz
cd SDL2-2.0.10/Xcode-iOS/SDL
set -o pipefail && xcodebuild -sdk iphonesimulator -jobs $XCODE_JOBS -parallelizeTargets | xcbeautify
cp build/Release-iphonesimulator/libSDL2.a $HOME/deps/lib
mkdir -p $HOME/deps/include/SDL2
cp -R ../../include/* $HOME/deps/include/SDL2
cd ../../..

# Crosscompile Magnum
git clone --depth 1 https://github.com/mosra/magnum.git
cd magnum
mkdir build-ios && cd build-ios
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../../toolchains/generic/iOS.cmake \
    -DCMAKE_OSX_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk \
    -DCMAKE_OSX_ARCHITECTURES="x86_64" \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DMAGNUM_WITH_AUDIO=OFF \
    -DMAGNUM_WITH_DEBUGTOOLS=$TARGET_GLES3 \
    -DMAGNUM_WITH_MATERIALTOOLS=OFF \
    -DMAGNUM_WITH_MESHTOOLS=ON \
    -DMAGNUM_WITH_PRIMITIVES=$TARGET_GLES3 \
    -DMAGNUM_WITH_SCENEGRAPH=ON \
    -DMAGNUM_WITH_SCENETOOLS=OFF \
    -DMAGNUM_WITH_SHADERS=ON \
    -DMAGNUM_WITH_SHADERTOOLS=OFF \
    -DMAGNUM_WITH_TEXT=$TARGET_GLES3 \
    -DMAGNUM_WITH_TEXTURETOOLS=$TARGET_GLES3 \
    -DMAGNUM_WITH_OPENGLTESTER=$TARGET_GLES3 \
    -DMAGNUM_WITH_SDL2APPLICATION=$TARGET_GLES3 \
    -DMAGNUM_TARGET_GLES2=$TARGET_GLES2 \
    -DMAGNUM_BUILD_STATIC=ON \
    -G Xcode
set -o pipefail && cmake --build . --config Release --target install -j$XCODE_JOBS | xcbeautify
cd ../..

# Crosscompile Magnum Plugins
git clone --depth 1 https://github.com/mosra/magnum-plugins.git
cd magnum-plugins
mkdir build-ios && cd build-ios
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../../toolchains/generic/iOS.cmake \
    -DCMAKE_OSX_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk \
    -DCMAKE_OSX_ARCHITECTURES="x86_64" \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_BUILD_TYPE=Release \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DMAGNUM_WITH_STBIMAGEIMPORTER=$TARGET_GLES3 \
    -DMAGNUM_WITH_STBTRUETYPEFONT=$TARGET_GLES3 \
    -DMAGNUM_BUILD_STATIC=ON \
    -G Xcode
set -o pipefail && cmake --build . --config Release --target install -j$XCODE_JOBS | xcbeautify
cd ../..

# Crosscompile
mkdir build-ios && cd build-ios
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../toolchains/generic/iOS.cmake \
    -DCMAKE_OSX_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk \
    -DCMAKE_OSX_ARCHITECTURES="x86_64" \
    -DCMAKE_PREFIX_PATH=$HOME/deps \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DMAGNUM_BUILD_STATIC=ON \
    -DMAGNUM_WITH_PLAYER=$TARGET_GLES3 \
    -DMAGNUM_WITH_UI=$TARGET_GLES3 \
    -DMAGNUM_WITH_UI_GALLERY=$TARGET_GLES3 \
    -DMAGNUM_BUILD_TESTS=ON \
    -DMAGNUM_BUILD_GL_TESTS=ON \
    -G Xcode
set -o pipefail && cmake --build . --config Release -j$XCODE_JOBS | xcbeautify

# TODO enable again once https://github.com/mosra/corrade/pull/176 is resolved
# TODO: find a better way to avoid
# Library not loaded: /System/Library/Frameworks/OpenGLES.framework/OpenGLES
# error
# DYLD_FALLBACK_LIBRARY_PATH="/Library/Developer/CoreSimulator/Profiles/Runtimes/iOS 12.4.simruntime/Contents/Resources/RuntimeRoot/System/Library/Frameworks/OpenGLES.framework" DYLD_FALLBACK_FRAMEWORK_PATH="/Library/Developer/CoreSimulator/Profiles/Runtimes/iOS 12.4.simruntime/Contents/Resources/RuntimeRoot/System/Library/Frameworks" CORRADE_TEST_COLOR=ON ctest -V -C Release -E GLTest

# Test install, after running the tests as for them it shouldn't be needed
set -o pipefail && cmake --build . --config Release --target install -j$XCODE_JOBS | xcbeautify
