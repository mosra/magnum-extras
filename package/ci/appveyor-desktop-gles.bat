if "%APPVEYOR_BUILD_WORKER_IMAGE%" == "Visual Studio 2022" call "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvarsall.bat" x64 || exit /b
if "%APPVEYOR_BUILD_WORKER_IMAGE%" == "Visual Studio 2019" call "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Auxiliary/Build/vcvarsall.bat" x64 || exit /b
if "%APPVEYOR_BUILD_WORKER_IMAGE%" == "Visual Studio 2017" call "C:/Program Files (x86)/Microsoft Visual Studio/2017/Community/VC/Auxiliary/Build/vcvarsall.bat" x64 || exit /b
set PATH=%APPVEYOR_BUILD_FOLDER%\deps\bin;%PATH%

rem Build Corrade
git clone --depth 1 https://github.com/mosra/corrade.git || exit /b
cd corrade || exit /b
mkdir build && cd build || exit /b
cmake .. ^
    -DCMAKE_BUILD_TYPE=Debug ^
    -DCMAKE_INSTALL_PREFIX=%APPVEYOR_BUILD_FOLDER%/deps ^
    -DCORRADE_WITH_INTERCONNECT=OFF ^
    -DCORRADE_UTILITY_USE_ANSI_COLORS=ON ^
    -DCORRADE_BUILD_DEPRECATED=OFF ^
    -G Ninja || exit /b
cmake --build . || exit /b
cmake --build . --target install || exit /b
cd .. && cd ..

rem Build Magnum. AnyImageImporter used by UiStyleTest.
git clone --depth 1 https://github.com/mosra/magnum.git || exit /b
cd magnum || exit /b
mkdir build && cd build || exit /b
cmake .. ^
    -DCMAKE_BUILD_TYPE=Debug ^
    -DCMAKE_INSTALL_PREFIX=%APPVEYOR_BUILD_FOLDER%/deps ^
    -DCMAKE_PREFIX_PATH=%APPVEYOR_BUILD_FOLDER%/SDL ^
    -DMAGNUM_TARGET_GLES=ON ^
    -DMAGNUM_TARGET_GLES2=%TARGET_GLES2% ^
    -DMAGNUM_TARGET_EGL=OFF ^
    -DMAGNUM_WITH_AUDIO=OFF ^
    -DMAGNUM_WITH_DEBUGTOOLS=%TARGET_GLES3% ^
    -DMAGNUM_WITH_MATERIALTOOLS=OFF ^
    -DMAGNUM_WITH_MESHTOOLS=ON ^
    -DMAGNUM_WITH_PRIMITIVES=%TARGET_GLES3% ^
    -DMAGNUM_WITH_SCENEGRAPH=ON ^
    -DMAGNUM_WITH_SCENETOOLS=OFF ^
    -DMAGNUM_WITH_SHADERS=ON ^
    -DMAGNUM_WITH_SHADERTOOLS=OFF ^
    -DMAGNUM_WITH_TEXT=%TARGET_GLES3% ^
    -DMAGNUM_WITH_TEXTURETOOLS=%TARGET_GLES3% ^
    -DMAGNUM_WITH_OPENGLTESTER=%TARGET_GLES3% ^
    -DMAGNUM_WITH_SDL2APPLICATION=%TARGET_GLES3% ^
    -DMAGNUM_WITH_GLFWAPPLICATION=%TARGET_GLES3% ^
    -DMAGNUM_WITH_ANYIMAGEIMPORTER=%TARGET_GLES3% ^
    -G Ninja || exit /b
cmake --build . || exit /b
cmake --build . --target install || exit /b
cd .. && cd ..

rem Build Magnum Plugins. Used by UiStyleTest.
git clone --depth 1 https://github.com/mosra/magnum-plugins.git || exit /b
cd magnum-plugins || exit /b
mkdir build && cd build || exit /b
cmake .. ^
    -DCMAKE_BUILD_TYPE=Debug ^
    -DCMAKE_INSTALL_PREFIX=%APPVEYOR_BUILD_FOLDER%/deps ^
    -DMAGNUM_WITH_STBIMAGEIMPORTER=ON ^
    -DMAGNUM_WITH_STBTRUETYPEFONT=ON ^
    -DMAGNUM_BUILD_STATIC=%BUILD_STATIC% ^
    -DMAGNUM_BUILD_PLUGINS_STATIC=%BUILD_STATIC% ^
    %COMPILER_EXTRA% -G Ninja || exit /b
cmake --build . || exit /b
cmake --build . --target install || exit /b
cd .. && cd ..

rem Build
mkdir build && cd build || exit /b
cmake .. ^
    -DCMAKE_BUILD_TYPE=Debug ^
    -DCMAKE_INSTALL_PREFIX=%APPVEYOR_BUILD_FOLDER%/deps ^
    -DCMAKE_PREFIX_PATH=%APPVEYOR_BUILD_FOLDER%/SDL ^
    -DMAGNUM_WITH_PLAYER=%TARGET_GLES3% ^
    -DMAGNUM_WITH_UI=%TARGET_GLES3% ^
    -DMAGNUM_WITH_UI_GALLERY=%TARGET_GLES3% ^
    -DMAGNUM_BUILD_TESTS=ON ^
    -DMAGNUM_BUILD_GL_TESTS=ON ^
    -G Ninja || exit /b
cmake --build . || exit /b

rem Test
set CORRADE_TEST_COLOR=ON
rem On Windows, if an assertion or other issue happens, A DIALOG WINDOWS POPS
rem UP FROM THE CONSOLE. And then, for fucks sake, IT WAITS ENDLESSLY FOR YOU
rem TO CLOSE IT!! Such behavior is utterly stupid in a non-interactive setting
rem such as on this very CI, so I'm setting a timeout to 60 seconds to avoid
rem the CI job being stuck for an hour if an assertion happens. CTest's default
rem timeout is somehow 10M seconds, which is as useful as nothing at all.
ctest -V -E "GLTest|GLBenchmark" --timeout 60 || exit /b

rem Test install, after running the tests as for them it shouldn't be needed
cmake --build . --target install || exit /b
