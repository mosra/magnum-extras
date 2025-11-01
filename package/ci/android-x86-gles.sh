#!/bin/bash
set -ev

git clone --depth 1 https://github.com/mosra/corrade.git
cd corrade

# Build native corrade-rc
mkdir build && cd build || exit /b
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
mkdir build-android-x86 && cd build-android-x86
cmake .. \
    -DCMAKE_SYSTEM_NAME=Android \
    -DCMAKE_SYSTEM_VERSION=29 \
    -DCMAKE_ANDROID_ARCH_ABI=x86 \
    -DCMAKE_ANDROID_NDK_TOOLCHAIN_VERSION=clang \
    -DCMAKE_ANDROID_STL_TYPE=c++_static \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DCORRADE_WITH_INTERCONNECT=OFF \
    -G Ninja
ninja install
cd ../..

# Crosscompile Magnum
git clone --depth 1 https://github.com/mosra/magnum.git
cd magnum
mkdir build-android-x86 && cd build-android-x86
cmake .. \
    -DCMAKE_SYSTEM_NAME=Android \
    -DCMAKE_SYSTEM_VERSION=29 \
    -DCMAKE_ANDROID_ARCH_ABI=x86 \
    -DCMAKE_ANDROID_NDK_TOOLCHAIN_VERSION=clang \
    -DCMAKE_ANDROID_STL_TYPE=c++_static \
    -DCMAKE_FIND_ROOT_PATH="/opt/android/sdk/ndk/21.4.7075529/toolchains/llvm/prebuilt/linux-x86_64/sysroot;$HOME/deps" \
    -DCMAKE_FIND_LIBRARY_CUSTOM_LIB_SUFFIX=/i686-linux-android/29 \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DMAGNUM_TARGET_GLES2=$TARGET_GLES2 \
    -DMAGNUM_WITH_AUDIO=OFF \
    -DMAGNUM_WITH_DEBUGTOOLS=$TARGET_GLES3 \
    -DMAGNUM_WITH_MATERIALTOOLS=OFF \
    -DMAGNUM_WITH_MESHTOOLS=OFF \
    -DMAGNUM_WITH_PRIMITIVES=$TARGET_GLES3 \
    -DMAGNUM_WITH_SCENEGRAPH=OFF \
    -DMAGNUM_WITH_SCENETOOLS=OFF \
    `# Shaders used by Ui doc snippets` \
    -DMAGNUM_WITH_SHADERS=$TARGET_GLES3 \
    -DMAGNUM_WITH_SHADERTOOLS=OFF \
    -DMAGNUM_WITH_TEXT=$TARGET_GLES3 \
    -DMAGNUM_WITH_TEXTURETOOLS=$TARGET_GLES3 \
    -DMAGNUM_WITH_SDL2APPLICATION=OFF \
    -DMAGNUM_WITH_GLFWAPPLICATION=OFF \
    -DMAGNUM_WITH_OPENGLTESTER=$TARGET_GLES3 \
    -DMAGNUM_WITH_ANYIMAGEIMPORTER=$TARGET_GLES3 \
    -G Ninja
ninja install
cd ../..

# Crosscompile Magnum Plugins
git clone --depth 1 https://github.com/mosra/magnum-plugins.git
cd magnum-plugins
mkdir build-android-x86 && cd build-android-x86
cmake .. \
    -DCMAKE_SYSTEM_NAME=Android \
    -DCMAKE_SYSTEM_VERSION=29 \
    -DCMAKE_ANDROID_ARCH_ABI=x86 \
    -DCMAKE_ANDROID_NDK_TOOLCHAIN_VERSION=clang \
    -DCMAKE_ANDROID_STL_TYPE=c++_static \
    -DCMAKE_FIND_ROOT_PATH="/opt/android/sdk/ndk/21.4.7075529/toolchains/llvm/prebuilt/linux-x86_64/sysroot;$HOME/deps" \
    -DCMAKE_FIND_LIBRARY_CUSTOM_LIB_SUFFIX=/i686-linux-android/29 \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DMAGNUM_WITH_STBIMAGEIMPORTER=$TARGET_GLES3 \
    -DMAGNUM_WITH_STBTRUETYPEFONT=$TARGET_GLES3 \
    -G Ninja
ninja install
cd ../..

# Crosscompile. Not compiling the Ui gallery because I don't want to mess with
# Android packaging just yet.
mkdir build-android-x86 && cd build-android-x86
cmake .. \
    -DCMAKE_SYSTEM_NAME=Android \
    -DCMAKE_SYSTEM_VERSION=29 \
    -DCMAKE_ANDROID_ARCH_ABI=x86 \
    -DCMAKE_ANDROID_NDK_TOOLCHAIN_VERSION=clang \
    -DCMAKE_ANDROID_STL_TYPE=c++_static \
    -DCMAKE_FIND_ROOT_PATH="/opt/android/sdk/ndk/21.4.7075529/toolchains/llvm/prebuilt/linux-x86_64/sysroot;$HOME/deps" \
    -DCMAKE_FIND_LIBRARY_CUSTOM_LIB_SUFFIX=/i686-linux-android/29 \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DMAGNUM_WITH_PLAYER=OFF \
    -DMAGNUM_WITH_UI=$TARGET_GLES3 \
    -DMAGNUM_WITH_UI_GALLERY=OFF \
    -DMAGNUM_BUILD_TESTS=ON \
    -DMAGNUM_BUILD_GL_TESTS=ON \
    -G Ninja
ninja $NINJA_JOBS

# Wait for emulator to start (done in parallel to build) and run tests
circle-android wait-for-boot
# SwiftShader on the Android 29 image just doesn't have a working GLES3
# implementation. See this file in the Magnum repository for details.
if [[ "$TARGET_GLES2" == "OFF" ]]; then
    EXCLUDE_GL_TESTS="-E (GLTest|GLBenchmark)"
fi
CORRADE_TEST_COLOR=ON ctest -V $EXCLUDE_GL_TESTS

# Test install, after running the tests as for them it shouldn't be needed
ninja install
