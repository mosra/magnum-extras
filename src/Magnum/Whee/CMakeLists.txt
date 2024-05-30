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
set(CMAKE_FOLDER "Magnum/Whee")

find_package(Magnum REQUIRED)
if(MAGNUM_TARGET_GL)
    find_package(Magnum REQUIRED GL)
endif()

if(MAGNUM_BUILD_STATIC)
    set(MAGNUM_WHEE_BUILD_STATIC 1)
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/configure.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/configure.h)

set(MagnumWhee_SRCS
    Event.cpp
    Handle.cpp)

set(MagnumWhee_GracefulAssert_SRCS
    AbstractLayer.cpp
    AbstractUserInterface.cpp
    AbstractVisualLayer.cpp
    BaseLayer.cpp
    EventLayer.cpp)

set(MagnumWhee_HEADERS
    AbstractLayer.h
    AbstractUserInterface.h
    AbstractVisualLayer.h
    BaseLayer.h
    Event.h
    EventLayer.h
    Handle.h
    Whee.h
    visibility.h)

set(MagnumWhee_PRIVATE_HEADERS
    Implementation/abstractUserInterface.h
    Implementation/abstractVisualLayerState.h
    Implementation/baseLayerState.h)

if(MAGNUM_TARGET_GL)
    corrade_add_resource(MagnumWhee_RESOURCES resources.conf)
    list(APPEND MagnumWhee_SRCS
        ${MagnumWhee_RESOURCES})
    list(APPEND MagnumWhee_GracefulAssert_SRCS
        BaseLayerGL.cpp)
    list(APPEND MagnumWhee_HEADERS
        BaseLayerGL.h)
endif()

# Objects shared between main and test library
add_library(MagnumWheeObjects OBJECT
    ${MagnumWhee_SRCS}
    ${MagnumWhee_HEADERS}
    ${MagnumWhee_PRIVATE_HEADERS})
target_include_directories(MagnumWheeObjects PUBLIC
    $<TARGET_PROPERTY:Magnum::Magnum,INTERFACE_INCLUDE_DIRECTORIES>
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/src)
if(NOT MAGNUM_BUILD_STATIC)
    target_compile_definitions(MagnumWheeObjects PRIVATE "MagnumWheeObjects_EXPORTS")
endif()
if(NOT MAGNUM_BUILD_STATIC OR MAGNUM_BUILD_STATIC_PIC)
    set_target_properties(MagnumWheeObjects PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

# Main Whee library
add_library(MagnumWhee ${SHARED_OR_STATIC}
    $<TARGET_OBJECTS:MagnumWheeObjects>
    ${MagnumWhee_GracefulAssert_SRCS}
    ${MagnumWhee_HEADERS})
target_include_directories(MagnumWhee PUBLIC
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/src)
set_target_properties(MagnumWhee PROPERTIES DEBUG_POSTFIX "-d")
if(NOT MAGNUM_BUILD_STATIC)
    set_target_properties(MagnumWhee PROPERTIES VERSION ${MAGNUMEXTRAS_LIBRARY_VERSION} SOVERSION ${MAGNUMEXTRAS_LIBRARY_SOVERSION})
elseif(MAGNUM_BUILD_STATIC_PIC)
    set_target_properties(MagnumWhee PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()
target_link_libraries(MagnumWhee PUBLIC
    Magnum::Magnum)
if(MAGNUM_TARGET_GL)
    target_link_libraries(MagnumWhee PUBLIC Magnum::GL)
endif()

install(TARGETS MagnumWhee
    RUNTIME DESTINATION ${MAGNUM_BINARY_INSTALL_DIR}
    LIBRARY DESTINATION ${MAGNUM_LIBRARY_INSTALL_DIR}
    ARCHIVE DESTINATION ${MAGNUM_LIBRARY_INSTALL_DIR})
install(FILES ${MagnumWhee_HEADERS} DESTINATION ${MAGNUM_INCLUDE_INSTALL_DIR}/Whee)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/configure.h DESTINATION ${MAGNUM_INCLUDE_INSTALL_DIR}/Whee)

# Magnum Whee target alias for superprojects
add_library(MagnumExtras::Whee ALIAS MagnumWhee)

if(MAGNUM_BUILD_TESTS)
    # Library with graceful assert for testing
    add_library(MagnumWheeTestLib ${SHARED_OR_STATIC} ${EXCLUDE_FROM_ALL_IF_TEST_TARGET}
        $<TARGET_OBJECTS:MagnumWheeObjects>
        ${MagnumWhee_GracefulAssert_SRCS})
    target_include_directories(MagnumWheeTestLib PUBLIC
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_BINARY_DIR}/src)
    set_target_properties(MagnumWheeTestLib PROPERTIES DEBUG_POSTFIX "-d")
    target_compile_definitions(MagnumWheeTestLib PRIVATE
        "CORRADE_GRACEFUL_ASSERT" "MagnumWhee_EXPORTS")
    if(MAGNUM_BUILD_STATIC_PIC)
        set_target_properties(MagnumWheeTestLib PROPERTIES POSITION_INDEPENDENT_CODE ON)
    endif()
    target_link_libraries(MagnumWheeTestLib PUBLIC
        Magnum::Magnum)
    if(MAGNUM_TARGET_GL)
        target_link_libraries(MagnumWheeTestLib PUBLIC Magnum::GL)
    endif()

    add_subdirectory(Test ${EXCLUDE_FROM_ALL_IF_TEST_TARGET})
endif()
