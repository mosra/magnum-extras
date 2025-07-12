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
    -DCMAKE_BUILD_TYPE=Debug \
    -DCORRADE_WITH_INTERCONNECT=OFF \
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
    `# SwiftShader is used only on the Mac ES3 build. On Linux Mesa llvmpipe` \
    `# is used instead and SwiftShader is not even downloaded so this points` \
    `# to a non-existent location and does nothing.` \
    -DCMAKE_PREFIX_PATH="$HOME/swiftshader" \
    -DCMAKE_INSTALL_RPATH="$HOME/deps/lib;$HOME/swiftshader/lib" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DMAGNUM_TARGET_GLES=ON \
    -DMAGNUM_TARGET_GLES2=$TARGET_GLES2 \
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
    -DMAGNUM_WITH_GLFWAPPLICATION=$TARGET_GLES3 \
    -DMAGNUM_WITH_ANYIMAGEIMPORTER=$TARGET_GLES3 \
    -G Ninja
ninja install
cd ../..

# Magnum Plugins
git clone --depth 1 https://github.com/mosra/magnum-plugins.git
cd magnum-plugins
mkdir build && cd build
cmake .. \
    -DCMAKE_CXX_FLAGS="$CMAKE_CXX_FLAGS" \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_PREFIX_PATH="$HOME/swiftshader" \
    -DCMAKE_INSTALL_RPATH="$HOME/deps/lib;$HOME/swiftshader/lib" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DMAGNUM_WITH_STBIMAGEIMPORTER=$TARGET_GLES3 \
    -DMAGNUM_WITH_STBTRUETYPEFONT=$TARGET_GLES3 \
    -G Ninja
ninja install
cd ../..

# Build
mkdir build && cd build
cmake .. \
    -DCMAKE_CXX_FLAGS="$CMAKE_CXX_FLAGS" \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_PREFIX_PATH="$HOME/deps;$HOME/swiftshader" \
    -DCMAKE_INSTALL_RPATH="$HOME/deps/lib;$HOME/swiftshader/lib" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DMAGNUM_WITH_PLAYER=$TARGET_GLES3 \
    -DMAGNUM_WITH_UI=$TARGET_GLES3 \
    -DMAGNUM_WITH_UI_GALLERY=$TARGET_GLES3 \
    -DMAGNUM_BUILD_TESTS=ON \
    -DMAGNUM_BUILD_GL_TESTS=ON \
    -G Ninja
ninja $NINJA_JOBS

export CORRADE_TEST_COLOR=ON

# Not running GL benchmarks because I'm not interested in knowing speed of a
# random software GPU emulation, further offset by inherent randomness of a CI
# VM.
CORRADE_TEST_COLOR=ON ctest -V -E GLBenchmark
if [ "$TARGET_GLES2" == "ON" ]; then
    MAGNUM_DISABLE_EXTENSIONS="OES_vertex_array_object" ctest -V -R GLTest
fi

# Test install, after running the tests as for them it shouldn't be needed
ninja install
