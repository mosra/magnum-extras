#!/bin/bash
set -ev

# Corrade
git clone --depth 1 git://github.com/mosra/corrade.git
cd corrade
mkdir build && cd build
cmake .. \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_INSTALL_RPATH=$HOME/deps/lib \
    -DCMAKE_BUILD_TYPE=Release \
    -DWITH_INTERCONNECT=$TARGET_GLES3
make -j install
cd ../..

# Magnum
git clone --depth 1 git://github.com/mosra/magnum.git
cd magnum
mkdir build && cd build
cmake .. \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_PREFIX_PATH=$HOME/sdl2 \
    -DCMAKE_INSTALL_RPATH=$HOME/deps/lib \
    -DCMAKE_BUILD_TYPE=Release \
    -DTARGET_GLES=ON \
    -DTARGET_GLES2=$TARGET_GLES2 \
    -DTARGET_DESKTOP_GLES=ON \
    -DWITH_AUDIO=OFF \
    -DWITH_DEBUGTOOLS=OFF \
    -DWITH_MESHTOOLS=OFF \
    -DWITH_PRIMITIVES=OFF \
    -DWITH_SCENEGRAPH=OFF \
    -DWITH_SHADERS=OFF \
    -DWITH_SHAPES=OFF \
    -DWITH_TEXT=$TARGET_GLES3 \
    -DWITH_TEXTURETOOLS=$TARGET_GLES3 \
    -DWITH_WINDOWLESS${PLATFORM_GL_API}APPLICATION=ON \
    -DWITH_SDL2APPLICATION=$TARGET_GLES3
make -j install
cd ../..

mkdir build && cd build
cmake .. \
    -DCMAKE_PREFIX_PATH="$HOME/deps;$HOME/sdl2" \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_INSTALL_RPATH=$HOME/deps/lib \
    -DCMAKE_BUILD_TYPE=Release \
    -DWITH_UI=$TARGET_GLES3 \
    -DWITH_UI_GALLERY=$TARGET_GLES3 \
    -DBUILD_TESTS=ON \
    -DBUILD_GL_TESTS=ON
# Otherwise the job gets killed (probably because using too much memory)
make -j4
CORRADE_TEST_COLOR=ON ctest -V -E GLTest
