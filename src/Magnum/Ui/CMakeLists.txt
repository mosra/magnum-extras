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

set(MagnumUi_SRCS
    Anchor.cpp
    BasicPlane.cpp
    BasicUserInterface.cpp
    Widget.cpp)

set(MagnumUi_HEADERS
    AbstractUiShader.h
    Anchor.h
    BasicInstancedLayer.h
    BasicInstancedLayer.hpp
    BasicPlane.h
    BasicPlane.hpp
    BasicUserInterface.h
    BasicUserInterface.hpp
    Ui.h
    Widget.h

    visibility.h)

# Ui library
add_library(MagnumUi ${SHARED_OR_STATIC}
    ${MagnumUi_SRCS}
    ${MagnumUi_HEADERS}
    ${MagnumUi_RESOURCES})
target_include_directories(MagnumUi PUBLIC ${PROJECT_SOURCE_DIR}/src)
set_target_properties(MagnumUi PROPERTIES DEBUG_POSTFIX "-d")
if(BUILD_STATIC_PIC)
    set_target_properties(MagnumUi PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()
target_link_libraries(MagnumUi
    Magnum::Magnum)

install(TARGETS MagnumUi
    RUNTIME DESTINATION ${MAGNUM_BINARY_INSTALL_DIR}
    LIBRARY DESTINATION ${MAGNUM_LIBRARY_INSTALL_DIR}
    ARCHIVE DESTINATION ${MAGNUM_LIBRARY_INSTALL_DIR})
install(FILES ${MagnumUi_HEADERS} DESTINATION ${MAGNUM_INCLUDE_INSTALL_DIR}/Ui)

# Magnum Ui target alias for superprojects
add_library(MagnumExtras::Ui ALIAS MagnumUi)

if(BUILD_TESTS)
    add_subdirectory(Test)
endif()
