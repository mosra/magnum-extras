if "%APPVEYOR_BUILD_WORKER_IMAGE%" == "Visual Studio 2017" call "C:/Program Files (x86)/Microsoft Visual Studio/2017/Community/VC/Auxiliary/Build/vcvarsall.bat" x64 || exit /b
if "%APPVEYOR_BUILD_WORKER_IMAGE%" == "Visual Studio 2017" set GENERATOR=Visual Studio 15 2017
rem This is what should make the *native* corrade-rc getting found. Corrade
rem should not cross-compile it on this platform, because then it'd get found
rem instead of the native version with no way to distinguish the two, and all
rem hell breaks loose. Thus also not passing CORRADE_RC_EXECUTABLE anywhere
rem below to ensure this doesn't regress.
set PATH=%APPVEYOR_BUILD_FOLDER%\deps-native\bin;%PATH%

git clone --depth 1 https://github.com/mosra/corrade.git || exit /b
cd corrade || exit /b

rem Build native corrade-rc
mkdir build && cd build || exit /b
cmake .. ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX=%APPVEYOR_BUILD_FOLDER%/deps-native ^
    -DCORRADE_WITH_INTERCONNECT=OFF ^
    -DCORRADE_WITH_PLUGINMANAGER=OFF ^
    -DCORRADE_WITH_TESTSUITE=OFF ^
    -DCORRADE_WITH_UTILITY=OFF ^
    -G Ninja || exit /b
cmake --build . --target install || exit /b
cd .. || exit /b

rem Crosscompile Corrade
mkdir build-rt && cd build-rt || exit /b
cmake .. ^
    -DCMAKE_SYSTEM_NAME=WindowsStore ^
    -DCMAKE_SYSTEM_VERSION=10.0 ^
    -DCMAKE_INSTALL_PREFIX=%APPVEYOR_BUILD_FOLDER%/deps ^
    -DCORRADE_WITH_INTERCONNECT=%TARGET_GLES3% ^
    -DCORRADE_BUILD_STATIC=ON ^
    -G "%GENERATOR%" -A x64 || exit /b
cmake --build . --config Release --target install -- /m /v:m || exit /b
cd .. && cd ..

rem Crosscompile Magnum
git clone --depth 1 https://github.com/mosra/magnum.git || exit /b
cd magnum || exit /b
mkdir build-rt && cd build-rt || exit /b
cmake .. ^
    -DCMAKE_SYSTEM_NAME=WindowsStore ^
    -DCMAKE_SYSTEM_VERSION=10.0 ^
    -DCMAKE_PREFIX_PATH=%APPVEYOR_BUILD_FOLDER%/deps ^
    -DCMAKE_INSTALL_PREFIX=%APPVEYOR_BUILD_FOLDER%/deps ^
    -DMAGNUM_WITH_AUDIO=OFF ^
    -DMAGNUM_WITH_DEBUGTOOLS=OFF ^
    -DMAGNUM_WITH_MATERIALTOOLS=OFF ^
    -DMAGNUM_WITH_MESHTOOLS=OFF ^
    -DMAGNUM_WITH_PRIMITIVES=OFF ^
    -DMAGNUM_WITH_SCENEGRAPH=OFF ^
    -DMAGNUM_WITH_SCENETOOLS=OFF ^
    -DMAGNUM_WITH_SHADERS=OFF ^
    -DMAGNUM_WITH_SHADERTOOLS=OFF ^
    -DMAGNUM_WITH_TEXT=%TARGET_GLES3% ^
    -DMAGNUM_WITH_TEXTURETOOLS=%TARGET_GLES3% ^
    -DMAGNUM_WITH_SDL2APPLICATION=OFF ^
    -DMAGNUM_WITH_GLFWAPPLICATION=OFF ^
    -DMAGNUM_TARGET_GLES2=%TARGET_GLES2% ^
    -DMAGNUM_BUILD_STATIC=ON ^
    -G "%GENERATOR%" -A x64 || exit /b
cmake --build . --config Release --target install -- /m /v:m || exit /b
cd .. && cd ..

rem Crosscompile. Not building the Ui gallery because I don't want to mess with
rem the app signing just yet. Similarly no tests because they take ages to
rem build, each executable is a msix file, and they can't be reasonably run
rem either. F this platform.
mkdir build-rt && cd build-rt || exit /b
cmake .. ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_PREFIX_PATH=%APPVEYOR_BUILD_FOLDER%/deps ^
    -DMAGNUM_BUILD_STATIC=ON ^
    -DMAGNUM_WITH_PLAYER=OFF ^
    -DMAGNUM_WITH_UI=%TARGET_GLES3% ^
    -DMAGNUM_WITH_UI_GALLERY=OFF ^
    -G "%GENERATOR%" -A x64 || exit /b
cmake --build . --config Release -- /m /v:m || exit /b

rem Test install, after running the tests as for them it shouldn't be needed
cmake --build . --config Release --target install || exit /b
