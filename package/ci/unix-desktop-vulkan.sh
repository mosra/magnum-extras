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
    `# Vulkan Loader is custom-built on Mac` \
    -DCMAKE_PREFIX_PATH="$HOME/deps;$HOME/vulkan-loader" \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_INSTALL_RPATH=$HOME/deps/lib \
    -DCMAKE_BUILD_TYPE=Debug \
    -DMAGNUM_TARGET_EGL=$TARGET_EGL \
    -DMAGNUM_WITH_AUDIO=OFF \
    `# ColorMap used by Ui doc snippets` \
    -DMAGNUM_WITH_DEBUGTOOLS=ON \
    -DMAGNUM_WITH_GL=OFF \
    -DMAGNUM_WITH_MATERIALTOOLS=OFF \
    -DMAGNUM_WITH_MESHTOOLS=OFF \
    -DMAGNUM_WITH_PRIMITIVES=OFF \
    -DMAGNUM_WITH_SCENEGRAPH=OFF \
    -DMAGNUM_WITH_SCENETOOLS=OFF \
    -DMAGNUM_WITH_SHADERS=OFF \
    -DMAGNUM_WITH_SHADERTOOLS=OFF \
    -DMAGNUM_WITH_TEXT=ON \
    -DMAGNUM_WITH_TEXTURETOOLS=ON \
    -DMAGNUM_WITH_TRADE=ON \
    -DMAGNUM_WITH_VK=ON \
    `# Application libraries needed by various Ui tests` \
    -DMAGNUM_WITH_SDL2APPLICATION=ON \
    -DMAGNUM_WITH_GLFWAPPLICATION=ON \
    -DMAGNUM_WITH_VULKANTESTER=ON \
    -DMAGNUM_WITH_ANYIMAGEIMPORTER=ON \
    -G Ninja
ninja install

# The GL library shouldn't get built by accident
if ls Debug/lib/libMagnumGL*; then
    echo "The MagnumGL library was built even though it shouldn't"
    false
fi

cd ../..

# Magnum Plugins. Used by GL tests which aren't relevant here, but also by
# UiStyleTest, so build them always.
git clone --depth 1 https://github.com/mosra/magnum-plugins.git
cd magnum-plugins
mkdir build && cd build
cmake .. \
    -DCMAKE_CXX_FLAGS="$CMAKE_CXX_FLAGS" \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_INSTALL_RPATH=$HOME/deps/lib \
    -DCMAKE_BUILD_TYPE=Debug \
    -DMAGNUM_WITH_STBIMAGEIMPORTER=ON \
    -DMAGNUM_WITH_STBTRUETYPEFONT=ON \
    -G Ninja
ninja install
cd ../..

mkdir build && cd build
cmake .. \
    -DCMAKE_CXX_FLAGS="$CMAKE_CXX_FLAGS" \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_INSTALL_RPATH=$HOME/deps/lib \
    -DCMAKE_BUILD_TYPE=Debug \
    -DMAGNUM_WITH_PLAYER=OFF \
    -DMAGNUM_WITH_UI=ON \
    -DMAGNUM_WITH_UI_GALLERY=OFF \
    -DMAGNUM_BUILD_TESTS=ON \
    -DMAGNUM_BUILD_VK_TESTS=ON \
    -G Ninja
ninja $NINJA_JOBS

export VK_ICD_FILENAMES=$HOME/swiftshader/share/vulkan/icd.d/vk_swiftshader_icd.json
export CORRADE_TEST_COLOR=ON

# TODO once actual Vulkan variants exist, leep them in sync with PKGBUILD and
# PKGBUILD-coverage
ctest -V

# Test install, after running the tests as for them it shouldn't be needed
ninja install
