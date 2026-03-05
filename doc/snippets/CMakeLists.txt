#
#   This file is part of Magnum.
#
#   Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
#               2020, 2021, 2022, 2023, 2024, 2025, 2026
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
set(CMAKE_FOLDER "Magnum/doc/snippets")

# On MSVC remove /W3, as we are replacing it with /W4
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
    string(REPLACE "/W3" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
endif()

set_directory_properties(PROPERTIES
    CORRADE_CXX_STANDARD 11
    CORRADE_USE_PEDANTIC_FLAGS ON)

find_package(Magnum REQUIRED DebugTools)
find_package(Magnum COMPONENTS Sdl2Application)

if(MAGNUM_WITH_UI)
    add_library(snippets-Ui STATIC ${EXCLUDE_FROM_ALL_IF_TEST_TARGET}
        Ui.cpp)
    target_link_libraries(snippets-Ui PRIVATE MagnumUi)
    if(CORRADE_TESTSUITE_TEST_TARGET)
        add_dependencies(${CORRADE_TESTSUITE_TEST_TARGET} snippets-Ui)
    endif()

    if(MAGNUM_TARGET_GL)
        add_library(snippets-Ui-gl STATIC ${EXCLUDE_FROM_ALL_IF_TEST_TARGET}
            Ui-gl.cpp)
        target_link_libraries(snippets-Ui-gl PRIVATE MagnumUi)
        if(CORRADE_TESTSUITE_TEST_TARGET)
            add_dependencies(${CORRADE_TESTSUITE_TEST_TARGET} snippets-Ui-gl)
        endif()
    endif()

    if(Magnum_Sdl2Application_FOUND)
        add_library(snippets-Ui-sdl2 STATIC ${EXCLUDE_FROM_ALL_IF_TEST_TARGET}
            Ui-sdl2.cpp)
        target_link_libraries(snippets-Ui-sdl2 PRIVATE MagnumUi Magnum::Sdl2Application)

        if(CORRADE_TESTSUITE_TEST_TARGET)
            add_dependencies(${CORRADE_TESTSUITE_TEST_TARGET} snippets-Ui-sdl2)
        endif()
    endif()

    if(MAGNUM_TARGET_GL AND Magnum_Sdl2Application_FOUND)
        add_executable(ui-getting-started ${EXCLUDE_FROM_ALL_IF_TEST_TARGET} ui-getting-started.cpp)
        target_link_libraries(ui-getting-started PRIVATE
            MagnumUi
            Magnum::Sdl2Application)

        if(MAGNUM_TARGET_EGL)
            find_package(Magnum REQUIRED WindowlessEglApplication)
        elseif(CORRADE_TARGET_IOS)
            find_package(Magnum REQUIRED WindowlessIosApplication)
        elseif(CORRADE_TARGET_APPLE)
            find_package(Magnum REQUIRED WindowlessCglApplication)
        elseif(CORRADE_TARGET_UNIX)
            find_package(Magnum REQUIRED WindowlessGlxApplication)
        elseif(CORRADE_TARGET_WINDOWS)
            find_package(Magnum REQUIRED WindowlessWglApplication)
        else()
            message(FATAL_ERROR "No windowless application available on this platform")
        endif()

        add_executable(ui-getting-started2 ${EXCLUDE_FROM_ALL_IF_TEST_TARGET} ui-getting-started2.cpp)
        target_link_libraries(ui-getting-started2 PRIVATE
            MagnumUi
            Magnum::DebugTools
            Magnum::Sdl2Application
            Magnum::WindowlessApplication)

        if(CORRADE_TESTSUITE_TEST_TARGET)
            add_dependencies(${CORRADE_TESTSUITE_TEST_TARGET}
                ui-getting-started
                ui-getting-started2)
        endif()
    endif()
endif()
