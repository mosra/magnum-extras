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

# Matches Corrade requirement, see its root CMakeLists for more information.
cmake_minimum_required(VERSION 3.5...3.10)

project(MagnumExtrasDocumentationImageGenerator CXX)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/../../modules/" ${CMAKE_MODULE_PATH})

find_package(Magnum REQUIRED GL MeshTools Primitives Trade)
find_package(MagnumExtras REQUIRED Ui)

set_directory_properties(PROPERTIES CORRADE_USE_PEDANTIC_FLAGS ON)

if(MAGNUM_TARGET_EGL)
    find_package(Magnum REQUIRED WindowlessEglApplication)
elseif(CORRADE_TARGET_APPLE AND NOT MAGNUM_TARGET_GLES)
    find_package(Magnum REQUIRED WindowlessCglApplication)
elseif(CORRADE_TARGET_UNIX)
    find_package(Magnum REQUIRED WindowlessGlxApplication)
elseif(CORRADE_TARGET_WINDOWS)
    find_package(Magnum REQUIRED WindowlessWglApplication)
else()
    message(FATAL_ERROR "No windowless application available on this platform")
endif()

add_executable(ui-baselayer ui-baselayer.cpp)
target_link_libraries(ui-baselayer PRIVATE
    Magnum::GL
    Magnum::Trade
    Magnum::WindowlessApplication
    MagnumExtras::Ui)

add_executable(ui-debuglayer ui-debuglayer.cpp)
target_link_libraries(ui-debuglayer PRIVATE
    Magnum::GL
    Magnum::Trade
    Magnum::WindowlessApplication
    MagnumExtras::Ui)

add_executable(ui-linelayer ui-linelayer.cpp)
target_link_libraries(ui-linelayer PRIVATE
    Magnum::GL
    Magnum::MeshTools
    Magnum::Primitives
    Magnum::Trade
    Magnum::WindowlessApplication
    MagnumExtras::Ui)

add_executable(ui-textlayer ui-textlayer.cpp)
target_link_libraries(ui-textlayer PRIVATE
    Magnum::GL
    Magnum::Trade
    Magnum::WindowlessApplication
    MagnumExtras::Ui)
