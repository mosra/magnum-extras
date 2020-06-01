#
#   This file is part of Magnum.
#
#   Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
#               2020 Vladimír Vondruš <mosra@centrum.cz>
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

find_package(Corrade REQUIRED Interconnect)
find_package(Magnum REQUIRED GL Text)

if(MAGNUM_TARGET_GLES2)
    message(FATAL_ERROR "Magnum::Ui doesn't work on OpenGL ES 2.0 or WebGL 1.0 builds at the moment. Switch to ES 3 or WebGL 2 if you want to use it.")
endif()

if(BUILD_STATIC)
    set(MAGNUM_UI_BUILD_STATIC 1)
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/configure.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/configure.h)

set(MagnumUi_SRCS
    Anchor.cpp
    BasicPlane.cpp
    BasicUserInterface.cpp
    Widget.cpp

    Button.cpp
    Input.cpp
    Label.cpp
    Modal.cpp
    Plane.cpp
    Style.cpp
    UserInterface.cpp
    ValidatedInput.cpp
    instantiation.cpp)

set(MagnumUi_HEADERS
    AbstractUiShader.h
    Anchor.h
    BasicInstancedGLLayer.h
    BasicInstancedGLLayer.hpp
    BasicInstancedLayer.h
    BasicInstancedLayer.hpp
    BasicGLLayer.h
    BasicGLLayer.hpp
    BasicLayer.h
    BasicLayer.hpp
    BasicPlane.h
    BasicPlane.hpp
    BasicUserInterface.h
    BasicUserInterface.hpp
    Ui.h
    Widget.h
    visibility.h

    Button.h
    Input.h
    Label.h
    Modal.h
    Plane.h
    Style.h
    UserInterface.h
    ValidatedInput.h)

corrade_add_resource(MagnumUi_RESOURCES resources.conf)
set_target_properties(MagnumUi_RESOURCES-dependencies PROPERTIES FOLDER "Magnum/Ui")

# Ui library
add_library(MagnumUi ${SHARED_OR_STATIC}
    ${MagnumUi_SRCS}
    ${MagnumUi_HEADERS}
    ${MagnumUi_RESOURCES})
target_include_directories(MagnumUi PUBLIC
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/src)
set_target_properties(MagnumUi PROPERTIES
    DEBUG_POSTFIX "-d"
    FOLDER "Magnum/Ui")
if(NOT BUILD_STATIC)
    set_target_properties(MagnumUi PROPERTIES VERSION ${MAGNUM_LIBRARY_VERSION} SOVERSION ${MAGNUM_LIBRARY_SOVERSION})
elseif(BUILD_STATIC_PIC)
    set_target_properties(MagnumUi PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()
target_link_libraries(MagnumUi
    Corrade::Interconnect
    Magnum::GL
    Magnum::Magnum
    Magnum::Text)

install(TARGETS MagnumUi
    RUNTIME DESTINATION ${MAGNUM_BINARY_INSTALL_DIR}
    LIBRARY DESTINATION ${MAGNUM_LIBRARY_INSTALL_DIR}
    ARCHIVE DESTINATION ${MAGNUM_LIBRARY_INSTALL_DIR})
install(FILES ${MagnumUi_HEADERS} DESTINATION ${MAGNUM_INCLUDE_INSTALL_DIR}/Ui)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/configure.h DESTINATION ${MAGNUM_INCLUDE_INSTALL_DIR}/Ui)

# Magnum Ui target alias for superprojects
add_library(MagnumExtras::Ui ALIAS MagnumUi)

if(WITH_UI_GALLERY)
    if(CORRADE_TARGET_ANDROID)
        find_package(Magnum REQUIRED AndroidApplication)
    elseif(CORRADE_TARGET_EMSCRIPTEN)
        find_package(Magnum REQUIRED EmscriptenApplication)
    else()
        find_package(Magnum REQUIRED Sdl2Application)
    endif()
    if(CORRADE_TARGET_IOS OR CORRADE_TARGET_EMSCRIPTEN OR CORRADE_TARGET_ANDROID)
        find_package(MagnumPlugins REQUIRED StbTrueTypeFont)
    endif()

    if(NOT CORRADE_TARGET_ANDROID)
        add_executable(magnum-ui-gallery ui-gallery.cpp)
        if(CORRADE_TARGET_WINDOWS AND NOT CORRADE_TARGET_WINDOWS_RT)
            if(MSVC)
                target_sources(magnum-ui-gallery PRIVATE WindowsHiDPI.manifest)
            elseif(MINGW)
                target_sources(magnum-ui-gallery PRIVATE WindowsHiDPI.rc)
            endif()
        endif()
    else()
        add_library(magnum-ui-gallery SHARED ui-gallery.cpp)
    endif()
    set_target_properties(magnum-ui-gallery PROPERTIES FOLDER "Magnum/Ui")
    target_link_libraries(magnum-ui-gallery PRIVATE
        MagnumUi
        Magnum::Application)
    if(CORRADE_TARGET_IOS OR CORRADE_TARGET_EMSCRIPTEN OR CORRADE_TARGET_ANDROID)
        target_link_libraries(magnum-ui-gallery PRIVATE MagnumPlugins::StbTrueTypeFont)
    endif()
    if(CORRADE_TARGET_IOS)
        set_target_properties(magnum-ui-gallery PROPERTIES
            MACOSX_BUNDLE ON
            MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/MacOSXBundleInfo.plist.in
            XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "YES")
    elseif(CORRADE_TARGET_EMSCRIPTEN)
        install(FILES ui-gallery.html DESTINATION ${MAGNUM_DEPLOY_PREFIX}/magnum-ui-gallery RENAME index.html)
        install(TARGETS magnum-ui-gallery DESTINATION ${MAGNUM_DEPLOY_PREFIX}/magnum-ui-gallery)
        install(FILES
            $<TARGET_FILE_DIR:magnum-ui-gallery>/magnum-ui-gallery.js.mem
            $<TARGET_FILE_DIR:magnum-ui-gallery>/magnum-ui-gallery.wasm
            DESTINATION ${MAGNUM_DEPLOY_PREFIX}/magnum-ui-gallery OPTIONAL)
        install(FILES
            ${MAGNUM_EMSCRIPTENAPPLICATION_JS}
            ${MAGNUM_WEBAPPLICATION_CSS}
            DESTINATION ${MAGNUM_DEPLOY_PREFIX}/magnum-ui-gallery)
    else()
        install(TARGETS magnum-ui-gallery DESTINATION ${MAGNUM_BINARY_INSTALL_DIR})
    endif()
endif()

if(BUILD_TESTS)
    add_subdirectory(Test)
endif()
