call "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/vcvarsall.bat" x64 || exit /b
set PATH=%APPVEYOR_BUILD_FOLDER%\deps\bin;%PATH%

rem Build Corrade
git clone --depth 1 git://github.com/mosra/corrade.git || exit /b
cd corrade || exit /b
mkdir build && cd build || exit /b
cmake .. ^
    -DCMAKE_BUILD_TYPE=Debug ^
    -DCMAKE_INSTALL_PREFIX=%APPVEYOR_BUILD_FOLDER%/deps ^
    -DWITH_INTERCONNECT=ON ^
    -G Ninja || exit /b
cmake --build . || exit /b
cmake --build . --target install || exit /b
cd .. && cd ..

rem Build Magnum
git clone --depth 1 git://github.com/mosra/magnum.git || exit /b
cd magnum || exit /b
mkdir build && cd build || exit /b
cmake .. ^
    -DCMAKE_BUILD_TYPE=Debug ^
    -DCMAKE_INSTALL_PREFIX=%APPVEYOR_BUILD_FOLDER%/deps ^
    -DWITH_AUDIO=OFF ^
    -DWITH_DEBUGTOOLS=OFF ^
    -DWITH_MESHTOOLS=OFF ^
    -DWITH_PRIMITIVES=OFF ^
    -DWITH_SCENEGRAPH=OFF ^
    -DWITH_SHADERS=OFF ^
    -DWITH_SHAPES=OFF ^
    -DWITH_TEXT=ON ^
    -DWITH_TEXTURETOOLS=ON ^
    -DWITH_WINDOWLESSWGLAPPLICATION=ON ^
    -G Ninja || exit /b
cmake --build . || exit /b
cmake --build . --target install || exit /b
cd .. && cd ..

rem Build
mkdir build && cd build || exit /b
cmake .. ^
    -DCMAKE_BUILD_TYPE=Debug ^
    -DCMAKE_INSTALL_PREFIX=%APPVEYOR_BUILD_FOLDER%/deps ^
    -DWITH_UI=ON ^
    -DBUILD_TESTS=ON ^
    -DBUILD_GL_TESTS=ON ^
    -G Ninja || exit /b
cmake --build . || exit /b
cmake --build . --target install || exit /b

rem Test
ctest -V -E GLTest || exit /b
