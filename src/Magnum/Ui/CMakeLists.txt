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

# IDE folder in VS, Xcode etc. CMake 3.12+, older versions have only the FOLDER
# property that would have to be set on each target separately.
set(CMAKE_FOLDER "Magnum/Ui")

find_package(Magnum REQUIRED Text Trade)
if(MAGNUM_TARGET_GL)
    find_package(Magnum REQUIRED GL)
endif()

if(MAGNUM_TARGET_GLES2)
    message(FATAL_ERROR "Magnum::Ui doesn't support OpenGL ES 2.0 or WebGL 1.0 builds. Switch to ES 3 or WebGL 2 if you want to use it.")
endif()

if(MAGNUM_BUILD_STATIC)
    set(MAGNUM_UI_BUILD_STATIC 1)
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/configure.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/configure.h)

set(MagnumUi_SRCS
    Button.cpp
    Handle.cpp
    Input.cpp
    Label.cpp
    NodeFlags.cpp
    Style.cpp)

set(MagnumUi_GracefulAssert_SRCS
    AbstractAnimator.cpp
    AbstractLayer.cpp
    AbstractLayouter.cpp
    AbstractRenderer.cpp
    AbstractStyle.cpp
    AbstractUserInterface.cpp
    AbstractVisualLayer.cpp
    AbstractVisualLayerAnimator.cpp
    Anchor.cpp
    BaseLayer.cpp
    BaseLayerAnimator.cpp
    DebugLayer.cpp
    Event.cpp
    EventLayer.cpp
    GenericAnimator.cpp
    LineLayer.cpp
    SnapLayouter.cpp
    TextLayer.cpp
    TextLayerAnimator.cpp
    TextProperties.cpp
    UserInterface.cpp
    Widget.cpp)

set(MagnumUi_HEADERS
    AbstractAnimator.h
    AbstractLayer.h
    AbstractLayouter.h
    AbstractRenderer.h
    AbstractStyle.h
    AbstractUserInterface.h
    AbstractVisualLayer.h
    AbstractVisualLayerAnimator.h
    Anchor.h
    Application.h
    BaseLayer.h
    BaseLayerAnimator.h
    Button.h
    DebugLayer.h
    Event.h
    EventLayer.h
    GenericAnimator.h
    Handle.h
    Input.h
    Label.h
    LineLayer.h
    NodeFlags.h
    SnapLayouter.h
    Style.h
    Style.hpp
    TextLayer.h
    TextLayerAnimator.h
    TextProperties.h
    UserInterface.h
    Ui.h
    Widget.h
    visibility.h)

set(MagnumUi_PRIVATE_HEADERS
    Implementation/abstractLayerState.h
    Implementation/abstractUserInterface.h
    Implementation/abstractVisualLayerState.h
    Implementation/abstractVisualLayerAnimatorState.h
    Implementation/baseLayerState.h
    Implementation/baseStyleUniformsMcssDark.h
    Implementation/debugLayerState.h
    Implementation/lineLayerState.h
    Implementation/lineMiterLimit.h
    Implementation/textLayerState.h
    Implementation/textStyleMcssDark.h
    Implementation/textStyleUniformsMcssDark.h
    Implementation/userInterfaceState.h)

if(MAGNUM_TARGET_GL)
    corrade_add_resource(MagnumUi_RESOURCES resources.conf)
    list(APPEND MagnumUi_SRCS
        DebugLayerGL.cpp
        # TODO this contains the font and icons as well, split into multiple
        #   files once non-GL / Vulkan builds become desirable
        ${MagnumUi_RESOURCES})
    list(APPEND MagnumUi_GracefulAssert_SRCS
        BaseLayerGL.cpp
        LineLayerGL.cpp
        RendererGL.cpp
        TextLayerGL.cpp
        UserInterfaceGL.cpp)
    list(APPEND MagnumUi_HEADERS
        BaseLayerGL.h
        DebugLayerGL.h
        LineLayerGL.h
        RendererGL.h
        TextLayerGL.h
        UserInterfaceGL.h)
    list(APPEND MagnumUi_PRIVATE_HEADERS
        Implementation/blurCoefficients.h
        Implementation/BlurShaderGL.h)
endif()

# Objects shared between main and test library
add_library(MagnumUiObjects OBJECT
    ${MagnumUi_SRCS}
    ${MagnumUi_HEADERS}
    ${MagnumUi_PRIVATE_HEADERS})
target_include_directories(MagnumUiObjects PUBLIC
    $<TARGET_PROPERTY:Magnum::Magnum,INTERFACE_INCLUDE_DIRECTORIES>
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/src)
if(NOT MAGNUM_UI_BUILD_STATIC)
    target_compile_definitions(MagnumUiObjects PRIVATE "MagnumUiObjects_EXPORTS")
endif()
if(NOT MAGNUM_UI_BUILD_STATIC OR MAGNUM_BUILD_STATIC_PIC)
    set_target_properties(MagnumUiObjects PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

# Main Ui library
add_library(MagnumUi ${SHARED_OR_STATIC}
    $<TARGET_OBJECTS:MagnumUiObjects>
    ${MagnumUi_GracefulAssert_SRCS}
    ${MagnumUi_HEADERS})
target_include_directories(MagnumUi PUBLIC
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/src)
set_target_properties(MagnumUi PROPERTIES DEBUG_POSTFIX "-d")
if(NOT MAGNUM_UI_BUILD_STATIC)
    set_target_properties(MagnumUi PROPERTIES VERSION ${MAGNUMEXTRAS_LIBRARY_VERSION} SOVERSION ${MAGNUMEXTRAS_LIBRARY_SOVERSION})
elseif(MAGNUM_BUILD_STATIC_PIC)
    set_target_properties(MagnumUi PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()
target_link_libraries(MagnumUi PUBLIC
    Magnum::Magnum
    Magnum::Text
    Magnum::Trade)
if(MAGNUM_TARGET_GL)
    target_link_libraries(MagnumUi PUBLIC Magnum::GL)
endif()

install(TARGETS MagnumUi
    RUNTIME DESTINATION ${MAGNUM_BINARY_INSTALL_DIR}
    LIBRARY DESTINATION ${MAGNUM_LIBRARY_INSTALL_DIR}
    ARCHIVE DESTINATION ${MAGNUM_LIBRARY_INSTALL_DIR})
install(FILES ${MagnumUi_HEADERS} DESTINATION ${MAGNUM_INCLUDE_INSTALL_DIR}/Ui)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/configure.h DESTINATION ${MAGNUM_INCLUDE_INSTALL_DIR}/Ui)

# Magnum Ui target alias for superprojects
add_library(MagnumExtras::Ui ALIAS MagnumUi)

if(MAGNUM_WITH_UI_GALLERY)
    find_package(Magnum REQUIRED DebugTools)
    if(CORRADE_TARGET_EMSCRIPTEN)
        find_package(Magnum REQUIRED
            AnyImageImporter
            EmscriptenApplication)
        find_package(MagnumPlugins REQUIRED
            StbImageImporter
            StbTrueTypeFont)
    else()
        find_package(Magnum REQUIRED Sdl2Application)
    endif()

    add_executable(magnum-ui-gallery
        ui-gallery.cpp)
    if(CORRADE_TARGET_WINDOWS AND NOT CORRADE_TARGET_WINDOWS_RT)
        if(MSVC)
            target_sources(magnum-ui-gallery PRIVATE WindowsHiDPI.manifest)
        elseif(MINGW)
            target_sources(magnum-ui-gallery PRIVATE WindowsHiDPI.rc)
        endif()
    endif()
    target_link_libraries(magnum-ui-gallery PRIVATE
        MagnumUi
        Magnum::Application
        Magnum::DebugTools)
    if(CORRADE_TARGET_IOS)
        set_target_properties(magnum-ui-gallery PROPERTIES
            MACOSX_BUNDLE ON
            MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/MacOSXBundleInfo.plist.in
            XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "YES")
    elseif(CORRADE_TARGET_EMSCRIPTEN)
        target_link_libraries(magnum-ui-gallery PRIVATE
            Magnum::AnyImageImporter
            MagnumPlugins::StbImageImporter
            MagnumPlugins::StbTrueTypeFont)
        if(CMAKE_VERSION VERSION_LESS 3.13)
            message(FATAL_ERROR "CMake 3.13+ is required in order to specify Emscripten linker options")
        endif()
        target_link_options(magnum-ui-gallery PRIVATE
            "SHELL:$<$<CONFIG:Release>:--closure 1>")
        add_custom_command(TARGET magnum-ui-gallery POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${MAGNUM_WEBAPPLICATION_CSS}
                ${MAGNUM_EMSCRIPTENAPPLICATION_JS}
                $<TARGET_FILE_DIR:magnum-ui-gallery>
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/ui-gallery.html"
                "$<TARGET_FILE_DIR:magnum-ui-gallery>/magnum-ui-gallery.html")
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

if(MAGNUM_BUILD_TESTS)
    # Library with graceful assert for testing
    add_library(MagnumUiTestLib ${SHARED_OR_STATIC} ${EXCLUDE_FROM_ALL_IF_TEST_TARGET}
        $<TARGET_OBJECTS:MagnumUiObjects>
        ${MagnumUi_GracefulAssert_SRCS})
    target_include_directories(MagnumUiTestLib PUBLIC
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_BINARY_DIR}/src)
    set_target_properties(MagnumUiTestLib PROPERTIES DEBUG_POSTFIX "-d")
    target_compile_definitions(MagnumUiTestLib PRIVATE
        "CORRADE_GRACEFUL_ASSERT" "MagnumUi_EXPORTS")
    if(MAGNUM_BUILD_STATIC_PIC)
        set_target_properties(MagnumUiTestLib PROPERTIES POSITION_INDEPENDENT_CODE ON)
    endif()
    target_link_libraries(MagnumUiTestLib PUBLIC
        Magnum::Magnum
        Magnum::Text
        Magnum::Trade)
    if(MAGNUM_TARGET_GL)
        target_link_libraries(MagnumUiTestLib PUBLIC Magnum::GL)
    endif()

    add_subdirectory(Test ${EXCLUDE_FROM_ALL_IF_TEST_TARGET})
endif()
