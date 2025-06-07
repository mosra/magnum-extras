#!/bin/bash
set -ev

git submodule update --init

# Crosscompile Corrade
git clone --depth 1 https://github.com/mosra/corrade.git
cd corrade
mkdir build-emscripten && cd build-emscripten
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE="../../toolchains/generic/Emscripten-wasm.cmake" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS_RELEASE="-DNDEBUG -O1" \
    -DCMAKE_EXE_LINKER_FLAGS_RELEASE="-O1" \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCORRADE_WITH_INTERCONNECT=OFF \
    $EXTRA_OPTS \
    -G Ninja
ninja install
cd ../..

# Crosscompile Magnum
git clone --depth 1 https://github.com/mosra/magnum.git
cd magnum
mkdir build-emscripten && cd build-emscripten
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE="../../toolchains/generic/Emscripten-wasm.cmake" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS_RELEASE="-DNDEBUG -O1" \
    -DCMAKE_EXE_LINKER_FLAGS_RELEASE="-O1" \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_FIND_ROOT_PATH=$HOME/deps \
    -DMAGNUM_WITH_AUDIO=OFF \
    -DMAGNUM_WITH_DEBUGTOOLS=$TARGET_GLES3 \
    -DMAGNUM_WITH_MATERIALTOOLS=OFF \
    -DMAGNUM_WITH_MESHTOOLS=ON \
    -DMAGNUM_WITH_OPENGLTESTER=$TARGET_GLES3 \
    -DMAGNUM_WITH_PRIMITIVES=$TARGET_GLES3 \
    -DMAGNUM_WITH_SCENEGRAPH=ON \
    -DMAGNUM_WITH_SCENETOOLS=OFF \
    -DMAGNUM_WITH_SHADERS=ON \
    -DMAGNUM_WITH_SHADERTOOLS=OFF \
    -DMAGNUM_WITH_TEXT=$TARGET_GLES3 \
    -DMAGNUM_WITH_TEXTURETOOLS=$TARGET_GLES3 \
    -DMAGNUM_WITH_EMSCRIPTENAPPLICATION=$TARGET_GLES3 \
    -DMAGNUM_WITH_SDL2APPLICATION=$TARGET_GLES3 \
    -DMAGNUM_WITH_WINDOWLESSEGLAPPLICATION=$TARGET_GLES3 \
    -DMAGNUM_WITH_ANYIMAGEIMPORTER=$TARGET_GLES3 \
    -DMAGNUM_WITH_ANYSCENEIMPORTER=$TARGET_GLES3 \
    -DMAGNUM_TARGET_GLES2=$TARGET_GLES2 \
    $EXTRA_OPTS \
    -G Ninja
ninja install
cd ../..

# Crosscompile Magnum Plugins
git clone --depth 1 https://github.com/mosra/magnum-plugins.git
cd magnum-plugins
mkdir build-emscripten && cd build-emscripten
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE="../../toolchains/generic/Emscripten-wasm.cmake" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS_RELEASE="-DNDEBUG -O1" \
    -DCMAKE_EXE_LINKER_FLAGS_RELEASE="-O1" \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_FIND_ROOT_PATH=$HOME/deps \
    -DMAGNUM_WITH_STBTRUETYPEFONT=$TARGET_GLES3 \
    -DMAGNUM_WITH_STBIMAGEIMPORTER=$TARGET_GLES3 \
    -DMAGNUM_WITH_GLTFIMPORTER=$TARGET_GLES3 \
    $EXTRA_OPTS \
    -G Ninja
ninja install
cd ../..

# Crosscompile
mkdir build-emscripten && cd build-emscripten
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE="../toolchains/generic/Emscripten-wasm.cmake" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS_RELEASE="-DNDEBUG -O1" \
    -DCMAKE_EXE_LINKER_FLAGS_RELEASE="-O1" \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_FIND_ROOT_PATH=$HOME/deps \
    -DMAGNUM_WITH_PLAYER=$TARGET_GLES3 \
    -DMAGNUM_WITH_UI=$TARGET_GLES3 \
    -DMAGNUM_WITH_UI_GALLERY=$TARGET_GLES3 \
    -DMAGNUM_BUILD_TESTS=ON \
    -DMAGNUM_BUILD_GL_TESTS=ON \
    $EXTRA_OPTS \
    -G Ninja
ninja $NINJA_JOBS

# Test
CORRADE_TEST_COLOR=ON ctest -V -E "GLTest|GLBenchmark"

# Test install, after running the tests as for them it shouldn't be needed
ninja install
