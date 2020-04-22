#
#   This file is part of Magnum.
#
#   Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019
#             Vladimír Vondruš <mosra@centrum.cz>
#
#   Permission is hereby granted, free of charge, to any person obtaining a
#   copy of this software and associated documentation files (the "Software"),
#   to deal in the Software without restriction, including without limitation
#   the rights to use, copy, modify, merge, publish, distribute, sublicense,
#   and/or sell copies of the Software, and to permit persons to whom the
#   Software is furnished to do so, subject to the following conditions:
#
#   The above copyright notice and this permission notice shall be included
#   in all copies or substantial portions of the Software.
#
#   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
#   DEALINGS IN THE SOFTWARE.
#

find_package(Magnum REQUIRED
    DebugTools
    GL
    MeshTools
    Primitives
    Shaders
    SceneGraph
    Trade)

if(CORRADE_TARGET_EMSCRIPTEN)
    find_package(Magnum REQUIRED EmscriptenApplication)
else()
    find_package(Magnum REQUIRED Sdl2Application)
endif()

set_directory_properties(PROPERTIES CORRADE_USE_PEDANTIC_FLAGS ON)

if(CORRADE_TARGET_EMSCRIPTEN)
    find_package(MagnumPlugins REQUIRED
        BasisImporter
        TinyGltfImporter
        StbTrueTypeFont
        StbImageImporter)
endif()

set(Player_SRCS
    Player.cpp
    ImagePlayer.cpp
    LoadImage.cpp
    ScenePlayer.cpp)

if(MAGNUM_TARGET_WEBGL)
    corrade_add_resource(Player_RESOURCES resources.conf)
    list(APPEND Player_SRCS ${Player_RESOURCES})
endif()

add_executable(magnum-player ${Player_SRCS})
if(CORRADE_TARGET_WINDOWS AND NOT CORRADE_TARGET_WINDOWS_RT)
    if(MSVC)
        target_sources(magnum-player PRIVATE WindowsHiDPI.manifest)
    elseif(MINGW)
        target_sources(magnum-player PRIVATE WindowsHiDPI.rc)
    endif()
endif()
target_link_libraries(magnum-player PRIVATE
    Magnum::Application
    Magnum::DebugTools
    Magnum::GL
    Magnum::Magnum
    Magnum::MeshTools
    Magnum::Primitives
    Magnum::SceneGraph
    Magnum::Shaders
    Magnum::Trade
    MagnumUi)
if(CORRADE_TARGET_EMSCRIPTEN)
    target_link_libraries(magnum-player PRIVATE
        MagnumPlugins::BasisImporter
        MagnumPlugins::TinyGltfImporter
        MagnumPlugins::StbTrueTypeFont
        MagnumPlugins::StbImageImporter)
endif()

if(CORRADE_TARGET_EMSCRIPTEN)
    # globals yay :/
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s 'EXTRA_EXPORTED_RUNTIME_METHODS=[\"ccall\"]' -s ALLOW_MEMORY_GROWTH=1")

    install(FILES player.html DESTINATION ${MAGNUM_DEPLOY_PREFIX}/magnum-player RENAME index.html)
    install(TARGETS magnum-player DESTINATION ${MAGNUM_DEPLOY_PREFIX}/magnum-player)
    install(FILES
        $<TARGET_FILE_DIR:magnum-player>/magnum-player.js.mem
        $<TARGET_FILE_DIR:magnum-player>/magnum-player.wasm
        DESTINATION ${MAGNUM_DEPLOY_PREFIX}/magnum-player OPTIONAL)
    install(FILES
        ${MAGNUM_EMSCRIPTENAPPLICATION_JS}
        ${MAGNUM_WEBAPPLICATION_CSS}
        DESTINATION ${MAGNUM_DEPLOY_PREFIX}/magnum-player)
else()
    install(TARGETS magnum-player DESTINATION ${MAGNUM_BINARY_INSTALL_DIR})
endif()

# Install a desktop file for Linux, BSD, ... (basically all Unix except Apple
# and Android)
if(CORRADE_TARGET_UNIX AND NOT CORRADE_TARGET_APPLE AND NOT CORRADE_TARGET_ANDROID)
    install(FILES magnum-player.desktop DESTINATION share/applications)
endif()
