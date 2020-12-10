#!/bin/bash
set -ev

# Corrade
git clone --depth 1 git://github.com/mosra/corrade.git
cd corrade
mkdir build && cd build
cmake .. \
    -DCMAKE_CXX_FLAGS="$CMAKE_CXX_FLAGS" \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_INSTALL_RPATH=$HOME/deps/lib \
    -DCMAKE_BUILD_TYPE=$CONFIGURATION \
    -DWITH_INTERCONNECT=ON \
    -DBUILD_DEPRECATED=$BUILD_DEPRECATED \
    -DBUILD_STATIC=$BUILD_STATIC \
    -G Ninja
ninja install
cd ../..

# Magnum
git clone --depth 1 git://github.com/mosra/magnum.git
cd magnum
mkdir build && cd build
cmake .. \
    -DCMAKE_CXX_FLAGS="$CMAKE_CXX_FLAGS" \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_PREFIX_PATH=$HOME/sdl2 \
    -DCMAKE_INSTALL_RPATH=$HOME/deps/lib \
    -DCMAKE_BUILD_TYPE=$CONFIGURATION \
    -DWITH_AUDIO=OFF \
    -DWITH_DEBUGTOOLS=ON \
    -DWITH_MESHTOOLS=ON \
    -DWITH_PRIMITIVES=ON \
    -DWITH_SCENEGRAPH=ON \
    -DWITH_SHADERS=ON \
    -DWITH_SHADERTOOLS=OFF \
    -DWITH_TEXT=ON \
    -DWITH_TEXTURETOOLS=ON \
    -DWITH_OPENGLTESTER=ON \
    -DWITH_WINDOWLESS${PLATFORM_GL_API}APPLICATION=ON \
    -DWITH_SDL2APPLICATION=$BUILD_APPLICATIONS \
    -DBUILD_DEPRECATED=$BUILD_DEPRECATED \
    -DBUILD_STATIC=$BUILD_STATIC \
    -G Ninja
ninja install
cd ../..

mkdir build && cd build
cmake .. \
    -DCMAKE_CXX_FLAGS="$CMAKE_CXX_FLAGS" \
    -DCMAKE_PREFIX_PATH="$HOME/deps;$HOME/sdl2" \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_INSTALL_RPATH=$HOME/deps/lib \
    -DCMAKE_BUILD_TYPE=$CONFIGURATION \
    -DWITH_PLAYER=$BUILD_APPLICATIONS \
    -DWITH_UI=ON \
    -DWITH_UI_GALLERY=$BUILD_APPLICATIONS \
    -DBUILD_TESTS=ON \
    -DBUILD_GL_TESTS=ON \
    -DBUILD_STATIC=$BUILD_STATIC \
    -G Ninja
# Otherwise the job gets killed (probably because using too much memory)
ninja -j4
ASAN_OPTIONS="color=always" LSAN_OPTIONS="color=always" CORRADE_TEST_COLOR=ON ctest -V -E GLTest
