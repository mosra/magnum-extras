rem Workaround for CMake not wanting sh.exe on PATH for MinGW. AARGH.
set PATH=%PATH:C:\Program Files\Git\usr\bin;=%
set PATH=C:\mingw-w64\x86_64-7.2.0-posix-seh-rt_v5-rev1\mingw64\bin;%APPVEYOR_BUILD_FOLDER%\deps\bin;%PATH%

rem Build Corrade
git clone --depth 1 https://github.com/mosra/corrade.git || exit /b
cd corrade || exit /b
mkdir build && cd build || exit /b
cmake .. ^
    -DCMAKE_CXX_FLAGS="--coverage" ^
    -DCMAKE_BUILD_TYPE=Debug ^
    -DCMAKE_INSTALL_PREFIX=%APPVEYOR_BUILD_FOLDER%/deps ^
    -DCORRADE_WITH_INTERCONNECT=ON ^
    -DCORRADE_UTILITY_USE_ANSI_COLORS=ON ^
    -G Ninja || exit /b
cmake --build . || exit /b
cmake --build . --target install || exit /b
cd .. && cd ..

rem Build Magnum
git clone --depth 1 https://github.com/mosra/magnum.git || exit /b
cd magnum || exit /b
mkdir build && cd build || exit /b
cmake .. ^
    -DCMAKE_CXX_FLAGS="--coverage" ^
    -DCMAKE_BUILD_TYPE=Debug ^
    -DCMAKE_INSTALL_PREFIX=%APPVEYOR_BUILD_FOLDER%/deps ^
    -DCMAKE_PREFIX_PATH=%APPVEYOR_BUILD_FOLDER%/SDL ^
    -DMAGNUM_WITH_AUDIO=OFF ^
    -DMAGNUM_WITH_DEBUGTOOLS=ON ^
    -DMAGNUM_WITH_MATERIALTOOLS=OFF ^
    -DMAGNUM_WITH_MESHTOOLS=ON ^
    -DMAGNUM_WITH_PRIMITIVES=ON ^
    -DMAGNUM_WITH_SCENEGRAPH=ON ^
    -DMAGNUM_WITH_SCENETOOLS=OFF ^
    -DMAGNUM_WITH_SHADERS=ON ^
    -DMAGNUM_WITH_SHADERTOOLS=OFF ^
    -DMAGNUM_WITH_TEXT=ON ^
    -DMAGNUM_WITH_TEXTURETOOLS=ON ^
    -DMAGNUM_WITH_OPENGLTESTER=ON ^
    -DMAGNUM_WITH_WINDOWLESSWGLAPPLICATION=ON ^
    -DMAGNUM_WITH_SDL2APPLICATION=ON ^
    -G Ninja || exit /b
cmake --build . || exit /b
cmake --build . --target install || exit /b
cd .. && cd ..

rem Build
mkdir build && cd build || exit /b
cmake .. ^
    -DCMAKE_CXX_FLAGS="--coverage" ^
    -DCMAKE_BUILD_TYPE=Debug ^
    -DCMAKE_INSTALL_PREFIX=%APPVEYOR_BUILD_FOLDER%/deps ^
    -DCMAKE_PREFIX_PATH=%APPVEYOR_BUILD_FOLDER%/SDL ^
    -DMAGNUM_WITH_PLAYER=ON ^
    -DMAGNUM_WITH_UI=ON ^
    -DMAGNUM_WITH_UI_GALLERY=ON ^
    -DMAGNUM_BUILD_TESTS=ON ^
    -DMAGNUM_BUILD_GL_TESTS=ON ^
    -G Ninja || exit /b
cmake --build . || exit /b

rem Test
set CORRADE_TEST_COLOR=ON
ctest -V -E GLTest || exit /b

rem Test install, after running the tests as for them it shouldn't be needed
cmake --build . --target install || exit /b

rem Coverage upload
set PATH=C:\msys64\usr\bin;%PATH%
bash %APPVEYOR_BUILD_FOLDER%\package\ci\appveyor-lcov.sh || exit /b
codecov -f coverage.info -X gcov
