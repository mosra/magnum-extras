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
set(CMAKE_FOLDER "Magnum/Ui/Test")

# This, and all subsequent checks of the same variable, assumes that Magnum
# itself is built as static and plugins in turn as well. If only the UI library
# is built as static (i.e., MAGNUM_UI_BUILD_STATIC is ON but
# MAGNUM_BUILD_STATIC is OFF), then the plugins are assumed to be dynamic and
# thus don't need to be linked.
if(MAGNUM_BUILD_STATIC)
    find_package(Magnum COMPONENTS AnyImageImporter)
    find_package(MagnumPlugins COMPONENTS StbImageImporter StbTrueTypeFont)
endif()

corrade_add_test(UiAbstractAnimatorTest AbstractAnimatorTest.cpp LIBRARIES MagnumUiTestLib)
corrade_add_test(UiAbstractLayerTest AbstractLayerTest.cpp LIBRARIES MagnumUiTestLib)
corrade_add_test(UiAbstractLayouterTest AbstractLayouterTest.cpp LIBRARIES MagnumUiTestLib)
corrade_add_test(UiAbstractRendererTest AbstractRendererTest.cpp LIBRARIES MagnumUiTestLib)
corrade_add_test(UiAbstractStyleTest AbstractStyleTest.cpp LIBRARIES MagnumUiTestLib)
corrade_add_test(UiAbstractUserInterfaceTest AbstractUserInterfaceTest.cpp LIBRARIES MagnumUiTestLib)
corrade_add_test(UiAbstractVisualLayerTest AbstractVisualLayerTest.cpp LIBRARIES MagnumUiTestLib)
corrade_add_test(UiAbstractVisualLayerStyleAnima___Test AbstractVisualLayerStyleAnimatorTest.cpp LIBRARIES MagnumUiTestLib)
corrade_add_test(UiAnchorTest AnchorTest.cpp LIBRARIES MagnumUiTestLib)
corrade_add_test(UiApplicationTest ApplicationTest.cpp LIBRARIES MagnumUi)
corrade_add_test(UiBaseLayerTest BaseLayerTest.cpp LIBRARIES MagnumUiTestLib)
corrade_add_test(UiBaseLayerStyleAnimatorTest BaseLayerStyleAnimatorTest.cpp LIBRARIES MagnumUiTestLib)
corrade_add_test(UiBlurShaderTest BlurShaderTest.cpp LIBRARIES MagnumUi)
corrade_add_test(UiButtonTest ButtonTest.cpp LIBRARIES MagnumUi)
corrade_add_test(UiDebugLayerTest DebugLayerTest.cpp LIBRARIES MagnumUiTestLib)
corrade_add_test(UiEventTest EventTest.cpp LIBRARIES MagnumUiTestLib)
corrade_add_test(UiEventLayerTest EventLayerTest.cpp LIBRARIES MagnumUiTestLib)
corrade_add_test(UiGenericAnimatorTest GenericAnimatorTest.cpp LIBRARIES MagnumUiTestLib)
corrade_add_test(UiHandleTest HandleTest.cpp LIBRARIES MagnumUi)
corrade_add_test(UiInputTest InputTest.cpp LIBRARIES MagnumUi)
corrade_add_test(UiLabelTest LabelTest.cpp LIBRARIES MagnumUi)
corrade_add_test(UiLineLayerTest LineLayerTest.cpp LIBRARIES MagnumUiTestLib)
corrade_add_test(UiNodeAnimatorTest NodeAnimatorTest.cpp LIBRARIES MagnumUiTestLib)
corrade_add_test(UiNodeFlagsTest NodeFlagsTest.cpp LIBRARIES MagnumUi)
corrade_add_test(UiSnapLayouterTest SnapLayouterTest.cpp LIBRARIES MagnumUiTestLib)

corrade_add_test(UiStyleTest StyleTest.cpp LIBRARIES MagnumUi)
if(MAGNUM_BUILD_STATIC)
    if(Magnum_AnyImageImporter_FOUND)
        target_link_libraries(UiStyleTest PRIVATE Magnum::AnyImageImporter)
    endif()
    if(MagnumPlugins_StbImageImporter_FOUND)
        target_link_libraries(UiStyleTest PRIVATE MagnumPlugins::StbImageImporter)
    endif()
    if(MagnumPlugins_StbTrueTypeFont_FOUND)
        target_link_libraries(UiStyleTest PRIVATE MagnumPlugins::StbTrueTypeFont)
    endif()
endif()

corrade_add_test(UiTextLayerTest TextLayerTest.cpp LIBRARIES MagnumUiTestLib)
corrade_add_test(UiTextLayerStyleAnimatorTest TextLayerStyleAnimatorTest.cpp LIBRARIES MagnumUiTestLib)
corrade_add_test(UiTextPropertiesTest TextPropertiesTest.cpp LIBRARIES MagnumUiTestLib)
corrade_add_test(UiUserInterfaceTest UserInterfaceTest.cpp LIBRARIES MagnumUiTestLib)
corrade_add_test(UiWidgetTest WidgetTest.cpp LIBRARIES MagnumUiTestLib)
if(CORRADE_TARGET_EMSCRIPTEN)
    if(CMAKE_VERSION VERSION_LESS 3.13)
        message(FATAL_ERROR "CMake 3.13+ is required in order to specify Emscripten linker options")
    endif()
    # It allocates 1M data / nodes to test boundary conditions, needs more
    # memory
    target_link_options(UiAbstractAnimatorTest PRIVATE "SHELL:-s ALLOW_MEMORY_GROWTH=1")
    target_link_options(UiAbstractLayerTest PRIVATE "SHELL:-s ALLOW_MEMORY_GROWTH=1")
    target_link_options(UiAbstractLayouterTest PRIVATE "SHELL:-s ALLOW_MEMORY_GROWTH=1")
    target_link_options(UiAbstractUserInterfaceTest PRIVATE "SHELL:-s ALLOW_MEMORY_GROWTH=1")
endif()

# Deliberately doesn't link to MagnumUi in order to be able to iterate on the
# implementation details in an isolated way without having to update the whole
# library as well
corrade_add_test(UiAbstractUserInt___ImplementationTest
        AbstractUserInterfaceImplementationTest.cpp
        ../Handle.cpp # for handle debug output
    LIBRARIES
        Magnum::Magnum)
target_include_directories(UiAbstractUserInt___ImplementationTest PRIVATE $<TARGET_PROPERTY:MagnumUi,INTERFACE_INCLUDE_DIRECTORIES>)
# So the debug output operators in Handle.cpp are not attempted to be
# dllimported on Windows
target_compile_definitions(UiAbstractUserInt___ImplementationTest PRIVATE "MagnumUi_EXPORTS")
# MSVC creates *.lib and *.exp files even for an exe if there's any dllexport.
# Don't want these. /NOIMPLIB will stop creating the *.lib in all versions, and
# *.exp as well with MSVC 2019+. MSVC 2015 however skips creating the *.lib but
# then attempts to create an *.exp out of it, dying.
# https://developercommunity.visualstudio.com/t/when-using-def-and-noimplib-with-linkexe-it-create/1495047
if(CORRADE_TARGET_MSVC AND NOT CORRADE_TARGET_CLANG_CL AND NOT CORRADE_MSVC2015_COMPATIBILITY AND NOT CMAKE_VERSION VERSION_LESS 3.13)
    target_link_options(UiAbstractUserInt___ImplementationTest PRIVATE /NOIMPLIB)
endif()

set_property(TARGET
    UiHandleTest
    UiDebugLayerTest
    UiTextLayerTest
    APPEND PROPERTY COMPILE_DEFINITIONS "CORRADE_GRACEFUL_ASSERT")

if(NOT CORRADE_TARGET_ANDROID)
    find_package(Magnum OPTIONAL_COMPONENTS Sdl2Application)
    if(Magnum_Sdl2Application_FOUND)
        add_executable(UiSdl2ApplicationTest Sdl2ApplicationTest.cpp)
        target_link_libraries(UiSdl2ApplicationTest PRIVATE
            MagnumUi
            Magnum::Sdl2Application)
        if(CORRADE_TARGET_EMSCRIPTEN)
            add_custom_command(TARGET UiSdl2ApplicationTest POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${MAGNUM_WEBAPPLICATION_CSS}
                    ${MAGNUM_EMSCRIPTENAPPLICATION_JS}
                    $<TARGET_FILE_DIR:UiSdl2ApplicationTest>
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CMAKE_CURRENT_SOURCE_DIR}/Sdl2ApplicationTest.html"
                    "$<TARGET_FILE_DIR:UiSdl2ApplicationTest>/UiSdl2ApplicationTest.html")
        endif()
    endif()
endif()

if(NOT CORRADE_TARGET_ANDROID AND NOT CORRADE_TARGET_IOS AND NOT CORRADE_TARGET_EMSCRIPTEN)
    find_package(Magnum OPTIONAL_COMPONENTS GlfwApplication)
    if(Magnum_GlfwApplication_FOUND)
        add_executable(UiGlfwApplicationTest GlfwApplicationTest.cpp)
        target_link_libraries(UiGlfwApplicationTest PRIVATE
            MagnumUi
            Magnum::GlfwApplication)
    endif()
endif()

if(CORRADE_TARGET_EMSCRIPTEN)
    find_package(Magnum OPTIONAL_COMPONENTS EmscriptenApplication)
    if(Magnum_EmscriptenApplication_FOUND)
        add_executable(UiEmscriptenApplicationTest EmscriptenApplicationTest.cpp)
        target_link_libraries(UiEmscriptenApplicationTest PRIVATE
            MagnumUi
            Magnum::EmscriptenApplication)
        add_custom_command(TARGET UiEmscriptenApplicationTest POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${MAGNUM_WEBAPPLICATION_CSS}
                ${MAGNUM_EMSCRIPTENAPPLICATION_JS}
                $<TARGET_FILE_DIR:UiEmscriptenApplicationTest>
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/EmscriptenApplicationTest.html"
                "$<TARGET_FILE_DIR:UiEmscriptenApplicationTest>/UiEmscriptenApplicationTest.html")
    endif()
endif()

# Build these only if there's no explicit -std= passed in the flags
if(NOT CMAKE_CXX_FLAGS MATCHES "-std=")
    # Copied verbatim from src/Corrade/Test/CMakeLists.txt, please keep in sync
    if((CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS "5.0") OR
       (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS "3.4") OR
       (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" AND NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS "6.0.1") OR
       (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS "19.10"))
        corrade_add_test(UiBaseLayerCpp14Test BaseLayerCpp14Test.cpp LIBRARIES MagnumUi)
        corrade_add_test(UiLineLayerCpp14Test LineLayerCpp14Test.cpp LIBRARIES MagnumUi)
        corrade_add_test(UiNodeAnimatorCpp14Test NodeAnimatorCpp14Test.cpp LIBRARIES MagnumUi)
        corrade_add_test(UiTextLayerCpp14Test TextLayerCpp14Test.cpp LIBRARIES MagnumUi)
        set_target_properties(
            UiBaseLayerCpp14Test
            UiLineLayerCpp14Test
            UiNodeAnimatorCpp14Test
            UiTextLayerCpp14Test
            PROPERTIES CORRADE_CXX_STANDARD 14)
   endif()
endif()

if(MAGNUM_TARGET_GL)
    find_package(Magnum REQUIRED DebugTools)

    corrade_add_test(UiBaseLayerGL_Test BaseLayerGL_Test.cpp LIBRARIES MagnumUiTestLib)
    corrade_add_test(UiLineLayerGL_Test LineLayerGL_Test.cpp LIBRARIES MagnumUiTestLib)
    corrade_add_test(UiRendererGL_Test RendererGL_Test.cpp LIBRARIES MagnumUiTestLib)
    corrade_add_test(UiTextLayerGL_Test TextLayerGL_Test.cpp LIBRARIES MagnumUiTestLib)
    corrade_add_test(UiUserInterfaceGL_Test UserInterfaceGL_Test.cpp LIBRARIES MagnumUi)

    if(NOT CORRADE_TARGET_ANDROID)
        find_package(Magnum REQUIRED Shaders)
        if(CORRADE_TARGET_EMSCRIPTEN)
            find_package(Magnum REQUIRED EmscriptenApplication)
        else()
            find_package(Magnum REQUIRED Sdl2Application)
        endif()

        add_executable(UiStressTest StressTest.cpp)
        target_link_libraries(UiStressTest PRIVATE
            MagnumUi
            Magnum::Application
            Magnum::DebugTools
            Magnum::Shaders)
        if(CORRADE_TARGET_EMSCRIPTEN)
            if(CMAKE_VERSION VERSION_LESS 3.12)
                message(FATAL_ERROR "CMake 3.12+ is required in order to specify Emscripten linker options")
            endif()
            target_link_options(UiStressTest PRIVATE "SHELL:-s ALLOW_MEMORY_GROWTH=1")
            add_custom_command(TARGET UiStressTest POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${MAGNUM_WEBAPPLICATION_CSS}
                    ${MAGNUM_EMSCRIPTENAPPLICATION_JS}
                    $<TARGET_FILE_DIR:UiStressTest>
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CMAKE_CURRENT_SOURCE_DIR}/StressTest.html"
                    "$<TARGET_FILE_DIR:UiStressTest>/UiStressTest.html")
        endif()
    endif()
endif()

if(MAGNUM_BUILD_GL_TESTS)
    find_package(Magnum REQUIRED OpenGLTester)

    if(CORRADE_TARGET_EMSCRIPTEN OR CORRADE_TARGET_ANDROID)
        set(UI_TEST_DIR .)
        set(UI_DIR .)
    else()
        set(UI_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR})
        set(UI_DIR ${PROJECT_SOURCE_DIR}/src/Magnum/Ui)
    endif()

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/configure.h.cmake
                   ${CMAKE_CURRENT_BINARY_DIR}/configure.h)

    corrade_add_test(UiApplicationGLTest ApplicationGLTest.cpp
        LIBRARIES
            MagnumUi
            Magnum::OpenGLTester)
    corrade_add_test(UiBaseLayerGLTest BaseLayerGLTest.cpp
        LIBRARIES
            MagnumUiTestLib
            Magnum::DebugTools
            Magnum::OpenGLTester
        FILES
            # Scaled-down mage from https://en.wikipedia.org/wiki/Gaussian_blur
            # by IkamusumeFan - Own work, CC BY-SA 4.0, https://commons.wikimedia.org/w/index.php?curid=41790217
            BaseLayerTestFiles/blur-input.png
            # Exported by Inkscape from mask.svg as (non-premultiplied) RGBA8.
            # Then processed with PngImporter and
            #   magnum-imageconverter --in-place -i alphaMode=premultipliedLinear mask-premultiplied.png
            # to have the alpha channel equal to the RGB channels because we're
            # not generally sRGB-aware yet, and then ultimately with
            #   pngcrush -ow mask-premultiplied.png
            # which turns the RGBA8 to RG8 to save space, and which
            # StbImageImporter will then expand back to RGBA8 on import.
            BaseLayerTestFiles/mask-premultiplied.png
            # Same, but the colored layer of mask.svg instead
            BaseLayerTestFiles/mask-colored-premultiplied.png
            BaseLayerTestFiles/clipping-disabled.png
            BaseLayerTestFiles/clipping-enabled.png
            BaseLayerTestFiles/composite-background-blur-0.png
            BaseLayerTestFiles/composite-background-blur-50.png
            BaseLayerTestFiles/composite-background-blur-50-smooth.png
            BaseLayerTestFiles/composite-background-blur-50-r31.png
            BaseLayerTestFiles/composite-background-blur-50-r31-mask-default.png
            BaseLayerTestFiles/composite-background-blur-50-r31-mask-mask.png
            BaseLayerTestFiles/composite-background-blur-50-r31-mask-colored-default.png
            BaseLayerTestFiles/composite-background-blur-50-r31-mask-colored-mask.png
            BaseLayerTestFiles/composite-background-blur-50-r31-mask-outline-mask.png
            BaseLayerTestFiles/composite-background-blur-50-r31-80.png
            BaseLayerTestFiles/composite-background-blur-75-colored.png
            BaseLayerTestFiles/composite-default-50.png
            BaseLayerTestFiles/composite-node-rects-background-blur-r0.png
            BaseLayerTestFiles/composite-node-rects-background-blur-r1.png
            BaseLayerTestFiles/composite-node-rects-background-blur-r30.png
            BaseLayerTestFiles/default.png
            BaseLayerTestFiles/default-smooth.png
            BaseLayerTestFiles/draw-order.png
            BaseLayerTestFiles/draw-order-composite-background-blur.png
            BaseLayerTestFiles/draw-order-composite-default.png
            BaseLayerTestFiles/edge-smoothness-same.png
            BaseLayerTestFiles/edge-smoothness-inner-larger.png
            BaseLayerTestFiles/edge-smoothness-inner-smaller.png
            BaseLayerTestFiles/gradient.png
            BaseLayerTestFiles/outline-different.png
            BaseLayerTestFiles/outline-gradient.png
            BaseLayerTestFiles/outline-rounded-corners-both-different.png
            BaseLayerTestFiles/outline-rounded-corners-different-smoothness.png
            BaseLayerTestFiles/outline-rounded-corners-inside.png
            BaseLayerTestFiles/outline-same.png
            BaseLayerTestFiles/rounded-corners-different.png
            BaseLayerTestFiles/rounded-corners-same-hard.png
            BaseLayerTestFiles/rounded-corners-same.png
            BaseLayerTestFiles/textured-colored.png
            BaseLayerTestFiles/textured-default.png
            BaseLayerTestFiles/textured-mask.png
            BaseLayerTestFiles/textured-mask-colored.png
            BaseLayerTestFiles/textured-mask-outline-default.png
            BaseLayerTestFiles/textured-mask-outline-mask.png
            BaseLayerTestFiles/textured.png)
    target_include_directories(UiBaseLayerGLTest PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    if(MAGNUM_BUILD_STATIC)
        if(Magnum_AnyImageImporter_FOUND)
            target_link_libraries(UiBaseLayerGLTest PRIVATE Magnum::AnyImageImporter)
        endif()
        if(MagnumPlugins_StbImageImporter_FOUND)
            target_link_libraries(UiBaseLayerGLTest PRIVATE MagnumPlugins::StbImageImporter)
        endif()
    endif()

    corrade_add_test(UiBaseLayerGLBenchmark BaseLayerGLBenchmark.cpp
        LIBRARIES
            MagnumUi
            Magnum::OpenGLTester)

    corrade_add_test(UiBlurShaderGLTest BlurShaderGLTest.cpp
        LIBRARIES
            # There are no asserts used but we need to use a symbol that's only
            # exported in the test lib
            MagnumUiTestLib
            Magnum::DebugTools
            Magnum::OpenGLTester
        FILES
            # Scaled-down mage from https://en.wikipedia.org/wiki/Gaussian_blur
            # by IkamusumeFan - Own work, CC BY-SA 4.0, https://commons.wikimedia.org/w/index.php?curid=41790217
            BaseLayerTestFiles/blur-input.png
            BaseLayerTestFiles/blur-3.png
            BaseLayerTestFiles/blur-16.png
            BaseLayerTestFiles/blur-31.png)
    target_include_directories(UiBlurShaderGLTest PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    if(MAGNUM_BUILD_STATIC)
        if(Magnum_AnyImageImporter_FOUND)
            target_link_libraries(UiBlurShaderGLTest PRIVATE Magnum::AnyImageImporter)
        endif()
        if(MagnumPlugins_StbImageImporter_FOUND)
            target_link_libraries(UiBlurShaderGLTest PRIVATE MagnumPlugins::StbImageImporter)
        endif()
    endif()

    corrade_add_test(UiBlurShaderGLBenchmark BlurShaderGLBenchmark.cpp
        LIBRARIES
            # There are no asserts used but we need to use a symbol that's only
            # exported in the test lib
            MagnumUiTestLib
            Magnum::DebugTools
            Magnum::OpenGLTester)

    corrade_add_test(UiDebugLayerGLTest DebugLayerGLTest.cpp
        LIBRARIES
            MagnumUi
            Magnum::DebugTools
            Magnum::OpenGLTester
        FILES
            DebugLayerTestFiles/draw-order-blue.png
            DebugLayerTestFiles/draw-order-green.png
            DebugLayerTestFiles/empty.png
            DebugLayerTestFiles/node-inspect.png
            DebugLayerTestFiles/node-inspect-color.png
            # Yes, reusing BaseLayer test file for draw order
            BaseLayerTestFiles/draw-order.png)
    target_include_directories(UiDebugLayerGLTest PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    if(MAGNUM_BUILD_STATIC)
        if(Magnum_AnyImageImporter_FOUND)
            target_link_libraries(UiDebugLayerGLTest PRIVATE Magnum::AnyImageImporter)
        endif()
        if(MagnumPlugins_StbImageImporter_FOUND)
            target_link_libraries(UiDebugLayerGLTest PRIVATE MagnumPlugins::StbImageImporter)
        endif()
    endif()

    corrade_add_test(UiLineLayerGLTest LineLayerGLTest.cpp
        LIBRARIES
            MagnumUiTestLib
            Magnum::DebugTools
            Magnum::OpenGLTester
        FILES
            LineLayerTestFiles/butt-bevel.png
            LineLayerTestFiles/color.png
            LineLayerTestFiles/default.png
            LineLayerTestFiles/draw-order.png
            LineLayerTestFiles/loop.png
            LineLayerTestFiles/round-bevel.png
            LineLayerTestFiles/smooth.png
            LineLayerTestFiles/square-miter.png
            LineLayerTestFiles/strip.png
            LineLayerTestFiles/triangle-miter-limit.png)
    target_include_directories(UiLineLayerGLTest PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    if(MAGNUM_BUILD_STATIC)
        if(Magnum_AnyImageImporter_FOUND)
            target_link_libraries(UiLineLayerGLTest PRIVATE Magnum::AnyImageImporter)
        endif()
        if(MagnumPlugins_StbImageImporter_FOUND)
            target_link_libraries(UiLineLayerGLTest PRIVATE MagnumPlugins::StbImageImporter)
        endif()
    endif()

    corrade_add_test(UiRendererGLTest RendererGLTest.cpp
        LIBRARIES
            MagnumUiTestLib
            Magnum::OpenGLTester)

    corrade_add_test(UiStyleGLTest StyleGLTest.cpp
        LIBRARIES
            MagnumUi
            Magnum::DebugTools
            Magnum::OpenGLTester
        FILES
            StyleTestFiles/mcss-dark-button-text-icon.png
            StyleTestFiles/mcss-dark-button-text.png
            StyleTestFiles/mcss-dark-button-icon.png
            StyleTestFiles/mcss-dark-input.png
            StyleTestFiles/mcss-dark-label-text.png
            StyleTestFiles/mcss-dark-label-icon.png)
    target_include_directories(UiStyleGLTest PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    if(MAGNUM_BUILD_STATIC)
        if(Magnum_AnyImageImporter_FOUND)
            target_link_libraries(UiStyleGLTest PRIVATE Magnum::AnyImageImporter)
        endif()
        if(MagnumPlugins_StbImageImporter_FOUND)
            target_link_libraries(UiStyleGLTest PRIVATE MagnumPlugins::StbImageImporter)
        endif()
        if(MagnumPlugins_StbTrueTypeFont_FOUND)
            target_link_libraries(UiStyleGLTest PRIVATE MagnumPlugins::StbTrueTypeFont)
        endif()
    endif()

    corrade_add_test(UiTextLayerGLTest TextLayerGLTest.cpp
        LIBRARIES
            MagnumUiTestLib
            Magnum::DebugTools
            Magnum::OpenGLTester
        FILES
            ../../Ui/SourceSans3-Regular.otf
            TextLayerTestFiles/clipping-disabled.png
            TextLayerTestFiles/clipping-disabled-editable.png
            TextLayerTestFiles/clipping-enabled.png
            TextLayerTestFiles/clipping-enabled-editable.png
            TextLayerTestFiles/colored.png
            TextLayerTestFiles/colored-cursor.png
            TextLayerTestFiles/colored-cursor-selection.png
            TextLayerTestFiles/colored-cursor-selection-rounded-smooth.png
            TextLayerTestFiles/colored-cursor-selection-text.png
            TextLayerTestFiles/colored-cursor-selection-text-rounded.png
            TextLayerTestFiles/colored-selection.png
            TextLayerTestFiles/colored-glyph.png
            TextLayerTestFiles/default.png
            TextLayerTestFiles/default-glyph.png
            TextLayerTestFiles/default-selection.png
            TextLayerTestFiles/distancefield-colored.png
            TextLayerTestFiles/distancefield-default.png
            TextLayerTestFiles/distancefield-dilate.png
            TextLayerTestFiles/distancefield-dilate-oob.png
            TextLayerTestFiles/distancefield-dilate-outline.png
            TextLayerTestFiles/distancefield-dilate-outline-cursor-selection-text.png
            TextLayerTestFiles/distancefield-empty.png
            TextLayerTestFiles/distancefield-erode.png
            TextLayerTestFiles/distancefield-smooth.png
            TextLayerTestFiles/distancefield-width-smoothness.png
            TextLayerTestFiles/draw-order.png
            TextLayerTestFiles/draw-order-editable.png)
    target_include_directories(UiTextLayerGLTest PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    if(MAGNUM_BUILD_STATIC)
        if(Magnum_AnyImageImporter_FOUND)
            target_link_libraries(UiTextLayerGLTest PRIVATE Magnum::AnyImageImporter)
        endif()
        if(MagnumPlugins_StbImageImporter_FOUND)
            target_link_libraries(UiTextLayerGLTest PRIVATE MagnumPlugins::StbImageImporter)
        endif()
        if(MagnumPlugins_StbTrueTypeFont_FOUND)
            target_link_libraries(UiTextLayerGLTest PRIVATE MagnumPlugins::StbTrueTypeFont)
        endif()
    endif()

    corrade_add_test(UiTextLayerGLBenchmark TextLayerGLBenchmark.cpp
        LIBRARIES
            MagnumUi
            Magnum::OpenGLTester)

    corrade_add_test(UiUserInterfaceGLTest UserInterfaceGLTest.cpp
        LIBRARIES
            MagnumUiTestLib
            Magnum::OpenGLTester)

    if(CORRADE_TARGET_EMSCRIPTEN)
        if(CMAKE_VERSION VERSION_LESS 3.13)
            message(FATAL_ERROR "CMake 3.13+ is required in order to specify Emscripten linker options")
        endif()
        # It allocates a large image to benchmark the blur ops
        target_link_options(UiBlurShaderGLTest PRIVATE "SHELL:-s ALLOW_MEMORY_GROWTH=1")
    endif()
endif()
