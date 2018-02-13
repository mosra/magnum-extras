#!/bin/bash
set -ev

git submodule update --init

# Corrade
git clone --depth 1 git://github.com/mosra/corrade.git
cd corrade

# Build native corrade-rc
mkdir build && cd build
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps-native \
    -DCMAKE_INSTALL_RPATH=$HOME/deps-native/lib \
    -DWITH_INTERCONNECT=OFF \
    -DWITH_PLUGINMANAGER=OFF \
    -DWITH_TESTSUITE=OFF \
    -G Ninja
ninja install
cd ..

# Crosscompile Corrade
mkdir build-ios && cd build-ios
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../../toolchains/generic/iOS.cmake \
    -DCMAKE_OSX_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk \
    -DCMAKE_OSX_ARCHITECTURES="x86_64" \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DBUILD_STATIC=ON \
    -DTESTSUITE_TARGET_XCTEST=ON \
    -DWITH_INTERCONNECT=$TARGET_GLES3 \
    -G Xcode
set -o pipefail && cmake --build . --config Release --target install | xcpretty
cd ../..

# Crosscompile Magnum
git clone --depth 1 git://github.com/mosra/magnum.git
cd magnum
mkdir build-ios && cd build-ios
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../../toolchains/generic/iOS.cmake \
    -DCMAKE_OSX_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk \
    -DCMAKE_OSX_ARCHITECTURES="x86_64" \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_PREFIX_PATH=$TRAVIS_BUILD_DIR/sdl2 \
    -DWITH_AUDIO=OFF \
    -DWITH_DEBUGTOOLS=OFF \
    -DWITH_MESHTOOLS=OFF \
    -DWITH_PRIMITIVES=OFF \
    -DWITH_SCENEGRAPH=OFF \
    -DWITH_SHADERS=OFF \
    -DWITH_SHAPES=OFF \
    -DWITH_TEXT=$TARGET_GLES3 \
    -DWITH_TEXTURETOOLS=$TARGET_GLES3 \
    -DWITH_OPENGLTESTER=$TARGET_GLES3 \
    -DWITH_WINDOWLESSIOSAPPLICATION=ON \
    -DWITH_SDL2APPLICATION=ON \
    -DTARGET_GLES2=$TARGET_GLES2 \
    -DBUILD_STATIC=ON \
    -G Xcode
set -o pipefail && cmake --build . --config Release --target install | xcpretty
cd ../..

# Crosscompile Magnum Plugins
git clone --depth 1 git://github.com/mosra/magnum-plugins.git
cd magnum-plugins
mkdir build-ios && cd build-ios
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../../toolchains/generic/iOS.cmake \
    -DCMAKE_OSX_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk \
    -DCMAKE_OSX_ARCHITECTURES="x86_64" \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_BUILD_TYPE=Release \
    -DWITH_STBTRUETYPEFONT=$TARGET_GLES3 \
    -DBUILD_STATIC=ON \
    -G Xcode
set -o pipefail && cmake --build . --config Release --target install | xcpretty
cd ../..

# Crosscompile
mkdir build-ios && cd build-ios
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../toolchains/generic/iOS.cmake \
    -DCMAKE_OSX_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk \
    -DCMAKE_OSX_ARCHITECTURES="x86_64" \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DCMAKE_PREFIX_PATH="$HOME/deps;$TRAVIS_BUILD_DIR/sdl2" \
    -DBUILD_STATIC=ON \
    -DWITH_UI=$TARGET_GLES3 \
    -DWITH_UI_GALLERY=$TARGET_GLES3 \
    -DBUILD_TESTS=ON \
    -DBUILD_GL_TESTS=ON \
    -G Xcode
set -o pipefail && cmake --build . --config Release | xcpretty

# TODO: find a better way to avoid
# Library not loaded: /System/Library/Frameworks/OpenGLES.framework/OpenGLES
# error
DYLD_FALLBACK_LIBRARY_PATH=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk/System/Library/Frameworks/OpenGLES.framework/ DYLD_FALLBACK_FRAMEWORK_PATH=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk/System/Library/Frameworks CORRADE_TEST_COLOR=ON ctest -V -C Release -E GLTest
