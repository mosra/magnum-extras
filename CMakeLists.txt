#
#   This file is part of Magnum.
#
#   Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016
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

cmake_minimum_required(VERSION 2.8.12)
project(MagnumExtras)

# CMake policies: enable MACOSX_RPATH by default
if(POLICY CMP0042)
    cmake_policy(SET CMP0042 NEW)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/modules/")
find_package(Magnum REQUIRED)

include(CMakeDependentOption)

option(BUILD_TESTS "Build unit tests" OFF)
cmake_dependent_option(BUILD_GL_TESTS "Build unit tests for OpenGL code" OFF "BUILD_TESTS" OFF)

option(BUILD_STATIC "Build static libraries (default are dynamic)" OFF)
cmake_dependent_option(BUILD_STATIC_PIC "Build static libraries with position-independent code" ON "BUILD_STATIC" OFF)

# Check compiler compatibility
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.7.0")
    message(FATAL_ERROR "Compatibility branch of Magnum Extras is needed for use with GCC < 4.7. See the documentation for more information.")
elseif(MSVC)
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "19.0")
        message(FATAL_ERROR "Compatibility branch of Magnum Extras is needed for use with MSVC < 2015. See the documentation for more information.")
    endif()
endif()

# Newlib toolchain supports only static linking
if(CORRADE_TARGET_NACL_NEWLIB OR CORRADE_TARGET_EMSCRIPTEN)
    set(BUILD_STATIC ON)
endif()

if(BUILD_TESTS)
    find_package(Corrade REQUIRED TestSuite)
    enable_testing()
endif()

if(BUILD_GL_TESTS)
    if(CORRADE_TARGET_APPLE)
        find_package(Magnum REQUIRED WindowlessCglApplication)
    elseif(CORRADE_TARGET_UNIX AND (NOT MAGNUM_TARGET_GLES OR MAGNUM_TARGET_DESKTOP_GLES))
        find_package(Magnum REQUIRED WindowlessGlxApplication)
    elseif(CORRADE_TARGET_WINDOWS)
        find_package(Magnum REQUIRED WindowlessWglApplication)
    else()
        message(FATAL_ERROR "Cannot run tests for OpenGL code on this platform. Set BUILD_GL_TESTS to OFF to skip building them.")
    endif()
endif()

if(NOT BUILD_STATIC)
    set(SHARED_OR_STATIC SHARED)
else()
    set(SHARED_OR_STATIC STATIC)
endif()

set(MAGNUMEXTRAS_CMAKE_MODULE_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/share/cmake/MagnumExtras)

add_subdirectory(modules)
add_subdirectory(src)
