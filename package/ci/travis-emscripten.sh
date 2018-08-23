#!/bin/bash
set -ev

git submodule update --init

git clone --depth 1 git://github.com/mosra/corrade.git
cd corrade

# Build native corrade-rc
mkdir build && cd build || exit /b
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps-native \
    -DWITH_INTERCONNECT=OFF \
    -DWITH_PLUGINMANAGER=OFF \
    -DWITH_TESTSUITE=OFF \
    -DWITH_UTILITY=OFF \
    -G Ninja
ninja install
cd ..

# Crosscompile Corrade
mkdir build-emscripten && cd build-emscripten
cmake .. \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DCMAKE_TOOLCHAIN_FILE="../../toolchains/generic/Emscripten.cmake" \
    -DEMSCRIPTEN_PREFIX=$(echo /usr/local/Cellar/emscripten/*/libexec) \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS_RELEASE="-DNDEBUG -O1" \
    -DCMAKE_EXE_LINKER_FLAGS_RELEASE="-O1" \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DWITH_INTERCONNECT=ON \
    -G Ninja
ninja install
cd ../..

# Crosscompile Magnum
git clone --depth 1 git://github.com/mosra/magnum.git
cd magnum
mkdir build-emscripten && cd build-emscripten
cmake .. \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DCMAKE_TOOLCHAIN_FILE="../../toolchains/generic/Emscripten.cmake" \
    -DEMSCRIPTEN_PREFIX=$(echo /usr/local/Cellar/emscripten/*/libexec) \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS_RELEASE="-DNDEBUG -O1" \
    -DCMAKE_EXE_LINKER_FLAGS_RELEASE="-O1" \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_FIND_ROOT_PATH=$HOME/deps \
    -DWITH_AUDIO=OFF \
    -DWITH_DEBUGTOOLS=OFF \
    -DWITH_MESHTOOLS=ON \
    -DWITH_PRIMITIVES=OFF \
    -DWITH_SCENEGRAPH=ON \
    -DWITH_SHADERS=ON \
    -DWITH_SHAPES=OFF \
    -DWITH_TEXT=$TARGET_GLES3 \
    -DWITH_TEXTURETOOLS=$TARGET_GLES3 \
    -DWITH_SDL2APPLICATION=$TARGET_GLES3 \
    -DWITH_ANYIMAGEIMPORTER=$TARGET_GLES3 \
    -DTARGET_GLES2=$TARGET_GLES2 \
    -G Ninja
ninja install
cd ../..

# Crosscompile Magnum Plugins
git clone --depth 1 git://github.com/mosra/magnum-plugins.git
cd magnum-plugins
mkdir build-emscripten && cd build-emscripten
cmake .. \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DCMAKE_TOOLCHAIN_FILE="../../toolchains/generic/Emscripten.cmake" \
    -DEMSCRIPTEN_PREFIX=$(echo /usr/local/Cellar/emscripten/*/libexec) \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS_RELEASE="-DNDEBUG -O1" \
    -DCMAKE_EXE_LINKER_FLAGS_RELEASE="-O1" \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_FIND_ROOT_PATH=$HOME/deps \
    -DWITH_STBTRUETYPEFONT=$TARGET_GLES3 \
    -DWITH_STBIMAGEIMPORTER=$TARGET_GLES3 \
    -DWITH_TINYGLTFIMPORTER=$TARGET_GLES3 \
    -G Ninja
ninja install
cd ../..

# Crosscompile
mkdir build-emscripten && cd build-emscripten
cmake .. \
    -DCORRADE_RC_EXECUTABLE=$HOME/deps-native/bin/corrade-rc \
    -DCMAKE_TOOLCHAIN_FILE="../toolchains/generic/Emscripten.cmake" \
    -DEMSCRIPTEN_PREFIX=$(echo /usr/local/Cellar/emscripten/*/libexec) \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS_RELEASE="-DNDEBUG -O1" \
    -DCMAKE_EXE_LINKER_FLAGS_RELEASE="-O1" \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_FIND_ROOT_PATH=$HOME/deps \
    -DWITH_PLAYER=$TARGET_GLES3 \
    -DWITH_UI=$TARGET_GLES3 \
    -DWITH_UI_GALLERY=$TARGET_GLES3 \
    -DBUILD_TESTS=ON \
    -G Ninja
# Otherwise the job gets killed (probably because using too much memory)
ninja -j4

# Test
CORRADE_TEST_COLOR=ON ctest -V
