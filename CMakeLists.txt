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

# CMake 3.12+ uses the policy max version specified in
# cmake_minimum_required(), meaning that with ...3.10, everything until CMP0071
# gets set to NEW implicitly. We however want to keep compatibility with
# versions before 3.12, so the NEW policies are still being hand-picked. Also
# don't want to do a blanket cmake_policy(VERSION) because that may break
# behavior for existing projects that rely on the OLD behavior.

# Don't restrict INTERPROCEDURAL_OPTIMIZATION only for icc on Linux
if(POLICY CMP0069)
    cmake_policy(SET CMP0069 NEW)
endif()
# If CMAKE_AUTOMOC is set, all uses of corrade_add_resource() would otherwise
# complain on 3.10 that AUTOMOC is not processing GENERATED files
if(POLICY CMP0071)
    cmake_policy(SET CMP0071 NEW)
endif()
# Allow <PackageName>_ROOT to be used on 3.12+ to point to per-package install
# locations that find_package(PackageName) subsequently picks up
if(POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
endif()
# Allow also <PACKAGENAME>_ROOT (i.e., uppercase), on 3.27+
if(POLICY CMP0144)
    cmake_policy(SET CMP0144 NEW)
endif()
# Superprojects can use just set(MAGNUM_WITH_BLAH ON) without FORCE CACHE on
# 3.13+
if(POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif()

project(MagnumExtras CXX)

# Use folders for nice tree in Visual Studio and XCode
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/modules/" ${CMAKE_MODULE_PATH})
find_package(Magnum REQUIRED)

include(CMakeDependentOption)

# Options that used to be unprefixed. New options shouldn't be added to this
# list.
set(_MAGNUMEXTRAS_DEPRECATED_UNPREFIXED_OPTIONS
    WITH_PLAYER
    WITH_UI
    WITH_UI_GALLERY
    BUILD_STATIC
    BUILD_STATIC_PIC
    BUILD_TESTS
    BUILD_GL_TESTS)
# If during the first run (i.e., when the variable isn't in cache yet), check
# if any of the prefixed options are already set, except for BUILD_STATIC which
# may already be coming from Magnum's configure.h. If so, we assume the user is
# already switched to the prefixed options and won't accept the deprecated
# unprefixed options for backwards compatibility. This way it's possible for
# projects to reuse these variables for other purposes without affecting
# Corrade in any way.
if(NOT DEFINED _MAGNUMEXTRAS_ACCEPT_DEPRECATED_UNPREFIXED_OPTIONS)
    set(_MAGNUMEXTRAS_ACCEPT_DEPRECATED_UNPREFIXED_OPTIONS ON CACHE INTERNAL "")
    foreach(option ${_MAGNUMEXTRAS_DEPRECATED_UNPREFIXED_OPTIONS})
        if(NOT "${option}" STREQUAL "BUILD_STATIC" AND DEFINED MAGNUM_${option})
            set(_MAGNUMEXTRAS_ACCEPT_DEPRECATED_UNPREFIXED_OPTIONS OFF CACHE INTERNAL "")
            break()
        endif()
    endforeach()
endif()

option(MAGNUM_BUILD_TESTS "Build unit tests" OFF)
cmake_dependent_option(MAGNUM_BUILD_GL_TESTS "Build unit tests for OpenGL code" OFF "MAGNUM_BUILD_TESTS" OFF)

# It's inconvenient to manually load all shared libs using Android / JNI,
# similarly on Emscripten, so there default to static.
if(CORRADE_TARGET_EMSCRIPTEN OR CORRADE_TARGET_ANDROID)
    set(OFF_EXCEPT_ANDROID_EMSCRIPTEN ON)
else()
    set(OFF_EXCEPT_ANDROID_EMSCRIPTEN OFF)
endif()
option(MAGNUM_BUILD_STATIC "Build static libraries" ${OFF_EXCEPT_ANDROID_EMSCRIPTEN})
# Disable PIC on Emscripten by default (but still allow it to be enabled
# explicitly if one so desires). Currently causes linker errors related to
# __memory_base etc.: https://github.com/emscripten-core/emscripten/issues/8761
if(CORRADE_TARGET_EMSCRIPTEN)
    set(ON_EXCEPT_EMSCRIPTEN OFF)
else()
    set(ON_EXCEPT_EMSCRIPTEN ON)
endif()
cmake_dependent_option(MAGNUM_BUILD_STATIC_PIC "Build static libraries with position-independent code" ${ON_EXCEPT_EMSCRIPTEN} "MAGNUM_BUILD_STATIC" OFF)

option(MAGNUM_WITH_PLAYER "Build the magnum-player application" OFF)
if(MAGNUM_WITH_PLAYER AND MAGNUM_BUILD_STATIC)
    set(MAGNUM_PLAYER_STATIC_PLUGINS "" CACHE STRING "Static plugins to link to the magnum-player application")
endif()

cmake_dependent_option(MAGNUM_WITH_UI "Build Ui library" OFF "NOT MAGNUM_WITH_PLAYER" ON)
cmake_dependent_option(MAGNUM_WITH_UI_GALLERY "Build magnum-ui-gallery executable" OFF "MAGNUM_WITH_UI" OFF)

# Backwards compatibility for unprefixed CMake options. If the user isn't
# explicitly using prefixed options in the first run already, accept the
# unprefixed options, and remember this decision for subsequent runs
if(NOT DEFINED _MAGNUMEXTRAS_ACCEPT_DEPRECATED_UNPREFIXED_OPTIONS)
    set(_MAGNUMEXTRAS_ACCEPT_DEPRECATED_UNPREFIXED_OPTIONS ON CACHE INTERNAL "")
endif()
# If the user wasn't explicitly using prefixed options in the first run and the
# MAGNUM_BUILD_DEPRECATED option is not currently disabled (which can get
# changed subsequently), accept the unprefixed options and print a warning if
# they're different from the prefixed ones.
if(_MAGNUMEXTRAS_ACCEPT_DEPRECATED_UNPREFIXED_OPTIONS AND MAGNUM_BUILD_DEPRECATED)
    # The following variables need extra care -- mirroring what's done in
    # cmake_dependent_option(), but only the cases that enable a dependency
    # that is off by default, not cases that hide an option from the GUI when
    # it's needed by something else. In other words, if the original user flag
    # setup didn't make sense before but the option dependency fixed it, it'll
    # be broken now, but if it made sense, it should keep working.
    #
    # Doing this before propagating the unprefixed options to avoid a
    # false-positive warning when e.g. MAGNUM_BUILD_STATIC_PIC is implicitly ON
    # but BUILD_STATIC_PIC not yet.
    if(BUILD_STATIC)
        if(NOT CORRADE_TARGET_EMSCRIPTEN AND NOT DEFINED BUILD_STATIC_PIC)
            set(BUILD_STATIC_PIC ON)
        endif()
    endif()
    if(WITH_PLAYER OR WITH_UI_GALLERY)
        if(NOT DEFINED WITH_UI)
            set(WITH_UI ON)
        endif()
    endif()

    set(_MAGNUMEXTRAS_WARN_DEPRECATED_UNPREFIXED_OPTION )
    foreach(option ${_MAGNUMEXTRAS_DEPRECATED_UNPREFIXED_OPTIONS})
        if(DEFINED ${option})
            # CMake has no comparison of boolean values (EQUAL returns false if
            # comparing ON and 1 or OFF and FALSE, STREQUAL also), so we have
            # to do it this way. Also warn only on the first encountered
            # variable so people can fix it, reconfigure and go to the next one
            # that warns.
            if((${option} AND NOT MAGNUM_${option}) OR
               (NOT ${option} AND MAGNUM_${option}) AND NOT _MAGNUMEXTRAS_WARN_DEPRECATED_UNPREFIXED_OPTION)
                set(_MAGNUMEXTRAS_WARN_DEPRECATED_UNPREFIXED_OPTION ${option})
            endif()
            set(MAGNUM_${option} ${${option}})
            # If variables specified on the command line don't match any
            # options, they're kept in cache but set as UNINITIALIZED, meaning
            # they don't appear in cmake-gui or ccmake, so there's no way to
            # fix the warning apart from hand-enditing the CMakeCache.txt or
            # recreating the build dir. Update their cached type to be BOOL to
            # make them appear.
            set(${option} ${${option}} CACHE BOOL "Deprecated, use MAGNUM_${option} instead" FORCE)
        endif()
    endforeach()

    if(_MAGNUMEXTRAS_WARN_DEPRECATED_UNPREFIXED_OPTION)
        message(DEPRECATION "Unprefixed options such as ${_MAGNUMEXTRAS_WARN_DEPRECATED_UNPREFIXED_OPTION} are deprecated, use MAGNUM_${_MAGNUMEXTRAS_WARN_DEPRECATED_UNPREFIXED_OPTION} instead. Delete the unprefixed variable from CMake cache or set both to the same value to silence this warning.")
    endif()
endif()

if(MAGNUM_BUILD_TESTS)
    find_package(Corrade REQUIRED TestSuite)
    if(CORRADE_TARGET_IOS)
        set(CORRADE_TESTSUITE_BUNDLE_IDENTIFIER_PREFIX "cz.mosra.magnum-extras")
    endif()
    enable_testing()

    # If CORRADE_TESTSUITE_TEST_TARGET is set, tests aren't built by default
    # (in the ALL target) but instead set as dependencies of a target named
    # after the value of CORRADE_TESTSUITE_TEST_TARGET. This is a copy of
    # what's done in corrade_add_test(), because we also build various test
    # libraries and plugins in addition to the test executables.
    if(CORRADE_TESTSUITE_TEST_TARGET)
        if(NOT TARGET ${CORRADE_TESTSUITE_TEST_TARGET})
            add_custom_target(${CORRADE_TESTSUITE_TEST_TARGET})
        endif()
        set(EXCLUDE_FROM_ALL_IF_TEST_TARGET EXCLUDE_FROM_ALL)
    endif()
endif()

if(MAGNUM_BUILD_GL_TESTS)
    find_package(Magnum REQUIRED OpenGLTester)
endif()

if(NOT MAGNUM_BUILD_STATIC)
    set(SHARED_OR_STATIC SHARED)
else()
    set(SHARED_OR_STATIC STATIC)
endif()

set(MAGNUMEXTRAS_CMAKE_MODULE_INSTALL_DIR share/cmake/MagnumExtras)

# Library version. MAGNUMEXTRAS_VERSION_YEAR/MONTH is used in
# src/Magnum/CMakeLists.txt to generate the versionExtras.h header.
set(MAGNUMEXTRAS_LIBRARY_VERSION 2.4)
set(MAGNUMEXTRAS_LIBRARY_SOVERSION 2)
set(MAGNUMEXTRAS_VERSION_YEAR 2020)
set(MAGNUMEXTRAS_VERSION_MONTH 6)

# A single output location. After a decade of saying NO THIS IS A NON-SOLUTION
# TO A NON-PROBLEM I reconsidered my views and enabled this, because:
#
# - On Windows (which don't have RPATH), this makes test execution finally
#   possible without having to install all the stuff first (including the
#   test-only libs, which is ugh).
# - With CMake subprojects, this makes it finally possible to use dynamic
#   plugins directly from the build dir (again without installing anything) ---
#   all plugins are put into the same place, so PluginManager has a single
#   place to look into; and thanks to the dynamic libraries being there as
#   well, this location can be automagically detected as relative to
#   Utility::Path::libraryLocation().
# - Thanks to the $<CONFIG> being part of the output path, you are always sure
#   you never accidentally mix up debug/release libraries when switching
#   CMAKE_BUILD_TYPE in an existing build dir.
#
# The runtime location is set to CMAKE_BINARY_DIR and not PROJECT_BINARY_DIR
# because have one runtime location per CMake subproject would not solve much
# either. If the user already provides CMAKE_RUNTIME_OUTPUT_DIRECTORY (even
# empty), it's respected and nothing is being done.
#
# Explicitly using a generator expression to ensure plugins are added to e.g.
# <CONFIG>/lib/magnum/importers/ instead of lib/magnum/importers/<CONFIG>. Also
# adding this to cache, making superprojects pick that up implicitly as well,
# without forcing them to explicitly mirror this setting.
if(NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY AND NOT DEFINED CMAKE_LIBRARY_OUTPUT_DIRECTORY AND NOT DEFINED CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/bin CACHE PATH "" FORCE)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/lib CACHE PATH "" FORCE)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/lib CACHE PATH "" FORCE)
    # There should be no need for the "90% use case" user to adjust these, so
    # don't show them in the default view
    mark_as_advanced(
        CMAKE_RUNTIME_OUTPUT_DIRECTORY
        CMAKE_LIBRARY_OUTPUT_DIRECTORY
        CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
endif()

add_subdirectory(modules)
add_subdirectory(src)

# Build snippets as part of testing. Unlike all other Test/ directories, this
# one isn't added with ${EXCLUDE_FROM_ALL_IF_TEST_TARGET} because the targets
# from there aren't referenced by CTest and thus it'd likely happen that they
# accidentally don't get added as a dependency to that target, causing them to
# be never built. Instead, each target there is handled separately, to minimize
# the chance of an accident.
if(MAGNUM_BUILD_TESTS)
    add_subdirectory(doc/snippets)
endif()
