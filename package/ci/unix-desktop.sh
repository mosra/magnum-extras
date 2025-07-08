#!/bin/bash
set -ev

# Corrade
git clone --depth 1 https://github.com/mosra/corrade.git
cd corrade
mkdir build && cd build
cmake .. \
    -DCMAKE_CXX_FLAGS="$CMAKE_CXX_FLAGS" \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_INSTALL_RPATH=$HOME/deps/lib \
    -DCMAKE_BUILD_TYPE=$CONFIGURATION \
    -DCORRADE_WITH_INTERCONNECT=OFF \
    -DCORRADE_BUILD_DEPRECATED=$BUILD_DEPRECATED \
    -DCORRADE_BUILD_STATIC=$BUILD_STATIC \
    -G Ninja
ninja install
cd ../..

# Magnum
git clone --depth 1 https://github.com/mosra/magnum.git
cd magnum
mkdir build && cd build
cmake .. \
    -DCMAKE_CXX_FLAGS="$CMAKE_CXX_FLAGS" \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_PREFIX_PATH=$HOME/sdl2 \
    -DCMAKE_INSTALL_RPATH=$HOME/deps/lib \
    -DCMAKE_BUILD_TYPE=$CONFIGURATION \
    -DMAGNUM_WITH_AUDIO=OFF \
    -DMAGNUM_WITH_DEBUGTOOLS=ON \
    -DMAGNUM_WITH_MATERIALTOOLS=OFF \
    -DMAGNUM_WITH_MESHTOOLS=ON \
    -DMAGNUM_WITH_PRIMITIVES=ON \
    -DMAGNUM_WITH_SCENEGRAPH=ON \
    -DMAGNUM_WITH_SCENETOOLS=OFF \
    -DMAGNUM_WITH_SHADERS=ON \
    -DMAGNUM_WITH_SHADERTOOLS=OFF \
    -DMAGNUM_WITH_TEXT=ON \
    -DMAGNUM_WITH_TEXTURETOOLS=ON \
    -DMAGNUM_WITH_OPENGLTESTER=ON \
    `# Application libraries needed by various Ui tests` \
    -DMAGNUM_WITH_SDL2APPLICATION=ON \
    -DMAGNUM_WITH_GLFWAPPLICATION=ON \
    -DMAGNUM_BUILD_DEPRECATED=$BUILD_DEPRECATED \
    -DMAGNUM_BUILD_STATIC=$BUILD_STATIC \
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
    -DMAGNUM_WITH_PLAYER=$BUILD_APPLICATIONS \
    -DMAGNUM_WITH_UI=ON \
    -DMAGNUM_WITH_UI_GALLERY=$BUILD_APPLICATIONS \
    -DMAGNUM_BUILD_TESTS=ON \
    -DMAGNUM_BUILD_GL_TESTS=ON \
    -DMAGNUM_BUILD_STATIC=$BUILD_STATIC \
    -G Ninja
ninja $NINJA_JOBS

export CORRADE_TEST_COLOR=ON
# Sanitizer options are used only in sanitizer builds, they do nothing
# elsewhere
export ASAN_OPTIONS="color=always"
export LSAN_OPTIONS="color=always"

ctest -V -E "GLTest|GLBenchmark"

# Test install, after running the tests as for them it shouldn't be needed
ninja install

cd ..

# Verify also compilation of the documentation image generators, if apps are
# built
if [ "$BUILD_APPLICATIONS" != "OFF" ]; then
    mkdir build-doc-generated && cd build-doc-generated
    cmake ../doc/generated \
        -DCMAKE_CXX_FLAGS="$CMAKE_CXX_FLAGS" \
        -DCMAKE_PREFIX_PATH=$HOME/deps \
        -DCMAKE_BUILD_TYPE=Debug \
        -G Ninja
    ninja
fi
