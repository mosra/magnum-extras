#
#   This file is part of Magnum.
#
#   Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
#               2020, 2021, 2022, 2023, 2024, 2025
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

find_package(Corrade REQUIRED Main)

set_directory_properties(PROPERTIES CORRADE_USE_PEDANTIC_FLAGS ON)

set(Magnum_EXTRA_COMPONENTS )
set(MagnumPlugins_EXTRA_COMPONENTS )
foreach(plugin ${MAGNUM_PLAYER_STATIC_PLUGINS})
    string(REGEX MATCH "(Magnum|MagnumPlugins)::(.+)" _plugin_match ${plugin})
    if(NOT _plugin_match)
        message(SEND_ERROR "Unrecognized plugin ${plugin} in MAGNUM_PLAYER_EXTRA_STATIC_PLUGINS")
    endif()
    if(CMAKE_MATCH_1 STREQUAL Magnum)
        list(APPEND Magnum_EXTRA_COMPONENTS ${CMAKE_MATCH_2})
    elseif(CMAKE_MATCH_1 STREQUAL MagnumPlugins)
        list(APPEND MagnumPlugins_EXTRA_COMPONENTS ${CMAKE_MATCH_2})
    else()
        message(FATAL_ERROR)
    endif()
endforeach()

find_package(Magnum REQUIRED
    DebugTools
    GL
    MeshTools
    Primitives
    Shaders
    SceneGraph
    Trade
    ${Magnum_EXTRA_COMPONENTS})

if(CORRADE_TARGET_EMSCRIPTEN)
    find_package(Magnum REQUIRED EmscriptenApplication AnySceneImporter)
    find_package(MagnumPlugins REQUIRED
        GltfImporter
        StbTrueTypeFont
        StbImageImporter
        ${MagnumPlugins_EXTRA_COMPONENTS})
else()
    find_package(Magnum REQUIRED Sdl2Application)
    if(MagnumPlugins_EXTRA_COMPONENTS)
        find_package(MagnumPlugins REQUIRED ${MagnumPlugins_EXTRA_COMPONENTS})
    endif()
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

add_executable(magnum-player WIN32 ${Player_SRCS})
if(CORRADE_TARGET_WINDOWS AND NOT CORRADE_TARGET_WINDOWS_RT)
    if(MSVC)
        target_sources(magnum-player PRIVATE WindowsHiDPI.manifest)
    elseif(MINGW)
        target_sources(magnum-player PRIVATE WindowsHiDPI.rc)
    endif()
endif()
target_link_libraries(magnum-player PRIVATE
    Corrade::Main
    Magnum::Application
    Magnum::DebugTools
    Magnum::GL
    Magnum::Magnum
    Magnum::MeshTools
    Magnum::Primitives
    Magnum::SceneGraph
    Magnum::Shaders
    Magnum::Trade
    MagnumUi
    ${MAGNUM_PLAYER_STATIC_PLUGINS})
if(CORRADE_TARGET_EMSCRIPTEN)
    target_link_libraries(magnum-player PRIVATE
        Magnum::AnySceneImporter
        MagnumPlugins::GltfImporter
        MagnumPlugins::StbTrueTypeFont
        MagnumPlugins::StbImageImporter)
endif()

if(CORRADE_TARGET_EMSCRIPTEN)
    if(CMAKE_VERSION VERSION_LESS 3.13)
        message(FATAL_ERROR "CMake 3.13+ is required in order to specify Emscripten linker options")
    endif()
    # EXTRA_EXPORTED_RUNTIME_METHODS got renamed to EXPORTED_RUNTIME_METHODS in
    # Emscripten 2.0.18
    if(EMSCRIPTEN_VERSION VERSION_LESS 2.0.18)
        target_link_options(magnum-player PRIVATE "SHELL:-s EXTRA_EXPORTED_RUNTIME_METHODS=[\"ccall\"]")
    # In 3.1.55+, one can explicitly include HEAPXX in order to access it
    # from outside (player.html in this case), and in 4.0.7+ it was made not
    # exported by default.
    # https://github.com/emscripten-core/emscripten/pull/21439
    # https://github.com/emscripten-core/emscripten/pull/24079
    elseif(EMSCRIPTEN_VERSION VERSION_LESS 3.1.55)
        target_link_options(magnum-player PRIVATE "SHELL:-s EXPORTED_RUNTIME_METHODS=[\"ccall\"]")
    else()
        target_link_options(magnum-player PRIVATE "SHELL:-s EXPORTED_RUNTIME_METHODS=[\"ccall\",\"HEAPU8\"]")
    endif()
    target_link_options(magnum-player PRIVATE
        "SHELL:$<$<CONFIG:Release>:--closure 1>"
        "SHELL:-s ALLOW_MEMORY_GROWTH=1"
        "SHELL:-s EXPORTED_FUNCTIONS=[\"_main\",\"_malloc\"]")
    add_custom_command(TARGET magnum-player POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${MAGNUM_EMSCRIPTENAPPLICATION_JS}
            ${MAGNUM_WEBAPPLICATION_CSS}
            $<TARGET_FILE_DIR:magnum-player>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/player.html"
            "$<TARGET_FILE_DIR:magnum-player>/magnum-player.html")

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
