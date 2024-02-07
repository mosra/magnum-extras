#
#   This file is part of Magnum.
#
#   Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
#               2020, 2021, 2022, 2023 Vladimír Vondruš <mosra@centrum.cz>
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
set(CMAKE_FOLDER "Magnum/Whee/Test")

corrade_add_test(WheeAbstractAnimatorTest AbstractAnimatorTest.cpp LIBRARIES MagnumWheeTestLib)
corrade_add_test(WheeAbstractLayerTest AbstractLayerTest.cpp LIBRARIES MagnumWheeTestLib)
corrade_add_test(WheeAbstractLayouterTest AbstractLayouterTest.cpp LIBRARIES MagnumWheeTestLib)
corrade_add_test(WheeAbstractRendererTest AbstractRendererTest.cpp LIBRARIES MagnumWheeTestLib)
corrade_add_test(WheeAbstractStyleTest AbstractStyleTest.cpp LIBRARIES MagnumWheeTestLib)
corrade_add_test(WheeAbstractUserInterfaceTest AbstractUserInterfaceTest.cpp LIBRARIES MagnumWheeTestLib)
corrade_add_test(WheeAbstractVisualLayerTest AbstractVisualLayerTest.cpp LIBRARIES MagnumWheeTestLib)
corrade_add_test(WheeBaseLayerTest BaseLayerTest.cpp LIBRARIES MagnumWheeTestLib)
corrade_add_test(WheeBlurShaderTest BlurShaderTest.cpp LIBRARIES MagnumWhee)
corrade_add_test(WheeEventTest EventTest.cpp LIBRARIES MagnumWhee)
corrade_add_test(WheeEventLayerTest EventLayerTest.cpp LIBRARIES MagnumWheeTestLib)
corrade_add_test(WheeHandleTest HandleTest.cpp LIBRARIES MagnumWhee)
corrade_add_test(WheeTextLayerTest TextLayerTest.cpp LIBRARIES MagnumWheeTestLib)
corrade_add_test(WheeTextPropertiesTest TextPropertiesTest.cpp LIBRARIES MagnumWheeTestLib)
corrade_add_test(WheeUserInterfaceTest UserInterfaceTest.cpp LIBRARIES MagnumWheeTestLib)
if(CORRADE_TARGET_EMSCRIPTEN)
    if(CMAKE_VERSION VERSION_LESS 3.13)
        message(FATAL_ERROR "CMake 3.13+ is required in order to specify Emscripten linker options")
    endif()
    # It allocates 1M data / nodes to test boundary conditions, needs more
    # memory
    target_link_options(WheeAbstractAnimatorTest PRIVATE "SHELL:-s ALLOW_MEMORY_GROWTH=1")
    target_link_options(WheeAbstractLayerTest PRIVATE "SHELL:-s ALLOW_MEMORY_GROWTH=1")
    target_link_options(WheeAbstractLayouterTest PRIVATE "SHELL:-s ALLOW_MEMORY_GROWTH=1")
    target_link_options(WheeAbstractUserInterfaceTest PRIVATE "SHELL:-s ALLOW_MEMORY_GROWTH=1")
endif()

# Deliberately doesn't link to MagnumWhee in order to be able to iterate on the
# implementation details in an isolated way without having to update the whole
# library as well
corrade_add_test(WheeAbstractUserI___ImplementationTest
        AbstractUserInterfaceImplementationTest.cpp
        ../Handle.cpp # for handle debug output
    LIBRARIES
        Magnum::Magnum)
target_include_directories(WheeAbstractUserI___ImplementationTest PRIVATE $<TARGET_PROPERTY:MagnumWhee,INTERFACE_INCLUDE_DIRECTORIES>)
# So the debug output operators in Handle.cpp are not attempted to be
# dllimported on Windows
target_compile_definitions(WheeAbstractUserI___ImplementationTest PRIVATE "MagnumWhee_EXPORTS")
# MSVC creates *.lib and *.exp files even for an exe if there's any dllexport.
# Don't want these. /NOIMPLIB will stop creating the *.lib in all versions, and
# *.exp as well with MSVC 2019+. MSVC 2015 however skips creating the *.lib but
# then attempts to create an *.exp out of it, dying.
# https://developercommunity.visualstudio.com/t/when-using-def-and-noimplib-with-linkexe-it-create/1495047
if(CORRADE_TARGET_MSVC AND NOT CORRADE_TARGET_CLANG_CL AND NOT CORRADE_MSVC2015_COMPATIBILITY AND NOT CMAKE_VERSION VERSION_LESS 3.13)
    target_link_options(WheeAbstractUserI___ImplementationTest PRIVATE /NOIMPLIB)
endif()

set_property(TARGET
    WheeHandleTest
    WheeTextLayerTest
    APPEND PROPERTY COMPILE_DEFINITIONS "CORRADE_GRACEFUL_ASSERT")

if(MAGNUM_TARGET_GL)
    find_package(Magnum REQUIRED DebugTools)

    corrade_add_test(WheeBaseLayerGL_Test BaseLayerGL_Test.cpp LIBRARIES MagnumWheeTestLib)
    corrade_add_test(WheeRendererGL_Test RendererGL_Test.cpp LIBRARIES MagnumWheeTestLib)
    corrade_add_test(WheeTextLayerGL_Test TextLayerGL_Test.cpp LIBRARIES MagnumWheeTestLib)
    corrade_add_test(WheeUserInterfaceGL_Test UserInterfaceGL_Test.cpp LIBRARIES MagnumWhee)

    if(NOT CORRADE_TARGET_ANDROID)
        find_package(Magnum REQUIRED Shaders)
        if(CORRADE_TARGET_EMSCRIPTEN)
            find_package(Magnum REQUIRED EmscriptenApplication)
        else()
            find_package(Magnum REQUIRED Sdl2Application)
        endif()

        add_executable(WheeStressTest StressTest.cpp)
        target_link_libraries(WheeStressTest PRIVATE
            MagnumWhee
            Magnum::Application
            Magnum::DebugTools
            Magnum::Shaders)
        if(CORRADE_TARGET_EMSCRIPTEN)
            if(CMAKE_VERSION VERSION_LESS 3.12)
                message(FATAL_ERROR "CMake 3.12+ is required in order to specify Emscripten linker options")
            endif()
            target_link_options(WheeStressTest PRIVATE "SHELL:-s ALLOW_MEMORY_GROWTH=1")
            add_custom_command(TARGET WheeStressTest POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${MAGNUM_WEBAPPLICATION_CSS}
                    ${MAGNUM_EMSCRIPTENAPPLICATION_JS}
                    $<TARGET_FILE_DIR:WheeStressTest>
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CMAKE_CURRENT_SOURCE_DIR}/StressTest.html"
                    "$<TARGET_FILE_DIR:WheeStressTest>/WheeStressTest.html")
        endif()
    endif()
endif()

if(MAGNUM_BUILD_GL_TESTS)
    find_package(Magnum REQUIRED OpenGLTester)
    if(MAGNUM_BUILD_STATIC)
        find_package(Magnum COMPONENTS AnyImageImporter)
        find_package(MagnumPlugins COMPONENTS StbImageImporter StbTrueTypeFont)
    endif()

    if(CORRADE_TARGET_EMSCRIPTEN OR CORRADE_TARGET_ANDROID)
        set(WHEE_TEST_DIR .)
        set(UI_DIR .)
    else()
        set(WHEE_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR})
        set(UI_DIR ${PROJECT_SOURCE_DIR}/src/Magnum/Ui)
    endif()

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/configure.h.cmake
                   ${CMAKE_CURRENT_BINARY_DIR}/configure.h)

    corrade_add_test(WheeBaseLayerGLTest BaseLayerGLTest.cpp
        LIBRARIES
            MagnumWheeTestLib
            Magnum::DebugTools
            Magnum::OpenGLTester
        FILES
            # Scaled-down mage from https://en.wikipedia.org/wiki/Gaussian_blur
            # by IkamusumeFan - Own work, CC BY-SA 4.0, https://commons.wikimedia.org/w/index.php?curid=41790217
            BaseLayerTestFiles/blur-input.png
            BaseLayerTestFiles/clipping-disabled.png
            BaseLayerTestFiles/clipping-enabled.png
            BaseLayerTestFiles/composite-background-blur-0.png
            BaseLayerTestFiles/composite-background-blur-50.png
            BaseLayerTestFiles/composite-background-blur-50-r31.png
            BaseLayerTestFiles/composite-background-blur-50-r31-80.png
            BaseLayerTestFiles/composite-background-blur-75-colored.png
            BaseLayerTestFiles/composite-default-50.png
            BaseLayerTestFiles/default.png
            BaseLayerTestFiles/draw-order.png
            BaseLayerTestFiles/draw-order-composite-background-blur.png
            BaseLayerTestFiles/draw-order-composite-default.png
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
            BaseLayerTestFiles/textured.png)
    target_include_directories(WheeBaseLayerGLTest PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    if(MAGNUM_BUILD_STATIC)
        if(Magnum_AnyImageImporter_FOUND)
            target_link_libraries(WheeBaseLayerGLTest PRIVATE Magnum::AnyImageImporter)
        endif()
        if(MagnumPlugins_StbImageImporter_FOUND)
            target_link_libraries(WheeBaseLayerGLTest PRIVATE MagnumPlugins::StbImageImporter)
        endif()
    endif()

    corrade_add_test(WheeBlurShaderGLTest BlurShaderGLTest.cpp
        LIBRARIES
            # There are no asserts used but we need to use a symbol that's only
            # exported in the test lib
            MagnumWheeTestLib
            Magnum::DebugTools
            Magnum::OpenGLTester
        FILES
            # Scaled-down mage from https://en.wikipedia.org/wiki/Gaussian_blur
            # by IkamusumeFan - Own work, CC BY-SA 4.0, https://commons.wikimedia.org/w/index.php?curid=41790217
            BaseLayerTestFiles/blur-input.png
            BaseLayerTestFiles/blur-3.png
            BaseLayerTestFiles/blur-16.png
            BaseLayerTestFiles/blur-31.png)
    target_include_directories(WheeBlurShaderGLTest PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    if(MAGNUM_BUILD_STATIC)
        if(Magnum_AnyImageImporter_FOUND)
            target_link_libraries(WheeBlurShaderGLTest PRIVATE Magnum::AnyImageImporter)
        endif()
        if(MagnumPlugins_StbImageImporter_FOUND)
            target_link_libraries(WheeBlurShaderGLTest PRIVATE MagnumPlugins::StbImageImporter)
        endif()
    endif()

    corrade_add_test(WheeRendererGLTest RendererGLTest.cpp
        LIBRARIES
            MagnumWheeTestLib
            Magnum::OpenGLTester)

    corrade_add_test(WheeTextLayerGLTest TextLayerGLTest.cpp
        LIBRARIES
            MagnumWheeTestLib
            Magnum::DebugTools
            Magnum::OpenGLTester
        FILES
            ../../Ui/SourceSansPro-Regular.ttf
            TextLayerTestFiles/clipping-disabled.png
            TextLayerTestFiles/clipping-enabled.png
            TextLayerTestFiles/colored.png
            TextLayerTestFiles/colored-glyph.png
            TextLayerTestFiles/default.png
            TextLayerTestFiles/default-glyph.png
            TextLayerTestFiles/draw-order.png)
    target_include_directories(WheeTextLayerGLTest PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    if(MAGNUM_BUILD_STATIC)
        if(Magnum_AnyImageImporter_FOUND)
            target_link_libraries(WheeTextLayerGLTest PRIVATE Magnum::AnyImageImporter)
        endif()
        if(MagnumPlugins_StbImageImporter_FOUND)
            target_link_libraries(WheeTextLayerGLTest PRIVATE MagnumPlugins::StbImageImporter)
        endif()
        if(MagnumPlugins_StbTrueTypeFont_FOUND)
            target_link_libraries(WheeTextLayerGLTest PRIVATE MagnumPlugins::StbTrueTypeFont)
        endif()
    endif()

    corrade_add_test(WheeUserInterfaceGLTest UserInterfaceGLTest.cpp
        LIBRARIES
            MagnumWheeTestLib
            Magnum::OpenGLTester)

    if(CORRADE_TARGET_EMSCRIPTEN)
        if(CMAKE_VERSION VERSION_LESS 3.13)
            message(FATAL_ERROR "CMake 3.13+ is required in order to specify Emscripten linker options")
        endif()
        # It allocates a large image to benchmark the blur ops
        target_link_options(WheeBlurShaderGLTest PRIVATE "SHELL:-s ALLOW_MEMORY_GROWTH=1")
    endif()
endif()
