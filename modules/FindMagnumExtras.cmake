#.rst:
# Find Magnum extras
# ------------------
#
# Finds Magnum extras. Basic usage::
#
#  find_package(MagnumExtras REQUIRED)
#
# This command tries to find Magnum extras and then defines the following:
#
#  MagnumExtras_FOUND       - Whether Magnum extras were found
#
# This command alone is useless without specifying the components:
#
#  Ui                       - Ui library
#  ui-gallery               - magnum-ui-gallery executable
#  player                   - magnum-player executable
#
# Example usage with specifying additional components is:
#
#  find_package(MagnumExtras REQUIRED Ui)
#
# For each component is then defined:
#
#  MagnumExtras_*_FOUND     - Whether the component was found
#  MagnumExtras::*          - Component imported target
#
# The package is found if either debug or release version of each requested
# library is found. If both debug and release libraries are found, proper
# version is chosen based on actual build configuration of the project (i.e.
# Debug build is linked to debug libraries, Release build to release
# libraries).
#
# Additionally these variables are defined for internal usage:
#
#  MAGNUMEXTRAS_INCLUDE_DIR - Magnum extras include dir (w/o
#   dependencies)
#  MAGNUMEXTRAS_*_LIBRARY_DEBUG - Debug version of given library, if found
#  MAGNUMEXTRAS_*_LIBRARY_RELEASE - Release version of given library, if
#   found
#

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

# Corrade library dependencies
set(_MAGNUMEXTRAS_CORRADE_DEPENDENCIES )
foreach(_component ${MagnumExtras_FIND_COMPONENTS})
    list(APPEND _MAGNUMEXTRAS_CORRADE_DEPENDENCIES ${_MAGNUMEXTRAS_${_component}_CORRADE_DEPENDENCIES})
endforeach()
find_package(Corrade REQUIRED ${_MAGNUMEXTRAS_CORRADE_DEPENDENCIES})

# Magnum library dependencies
set(_MAGNUMEXTRAS_MAGNUM_DEPENDENCIES )
foreach(_component ${MagnumExtras_FIND_COMPONENTS})
    if(_component STREQUAL Ui)
        set(_MAGNUMEXTRAS_${_component}_MAGNUM_DEPENDENCIES Text GL Trade)
    endif()

    list(APPEND _MAGNUMEXTRAS_MAGNUM_DEPENDENCIES ${_MAGNUMEXTRAS_${_component}_MAGNUM_DEPENDENCIES})
endforeach()
find_package(Magnum REQUIRED ${_MAGNUMEXTRAS_MAGNUM_DEPENDENCIES})

# Global include dir that's unique to Magnum Extras. Often it will be installed
# alongside Magnum, which is why the hint, but if not, it shouldn't just pick
# MAGNUM_INCLUDE_DIR because then _MAGNUMEXTRAS_*_INCLUDE_DIR will fail to be
# found. In case of CMake subprojects the versionExtras.h is generated inside
# the build dir so this won't find it, instead src/CMakeLists.txt forcibly sets
# MAGNUMEXTRAS_INCLUDE_DIR as an internal cache value to make that work.
find_path(MAGNUMEXTRAS_INCLUDE_DIR Magnum/versionExtras.h
    HINTS ${MAGNUM_INCLUDE_DIR})
mark_as_advanced(MAGNUMEXTRAS_INCLUDE_DIR)

# CMake module dir for dependencies. It might not be present at all if no
# feature that needs them is enabled, in which case it'll be left at NOTFOUND.
# But in that case it should also not be subsequently needed for any
# find_package(). If this is called from a superproject, the
# _MAGNUMEXTRAS_DEPENDENCY_MODULE_DIR is already set by modules/CMakeLists.txt.
#
# There's no dependency Find modules so far. Once there are, uncomment this and
# list the modules in NAMES.
#find_path(_MAGNUMEXTRAS_DEPENDENCY_MODULE_DIR
#    NAMES
#    PATH_SUFFIXES share/cmake/MagnumExtras/dependencies)
#mark_as_advanced(_MAGNUMEXTRAS_DEPENDENCY_MODULE_DIR)

# If the module dir is found and is not present in CMAKE_MODULE_PATH already
# (such as when someone explicitly added it, or if it's the Magnum's modules/
# dir in case of a superproject), add it as the first before all other. Set a
# flag to remove it again at the end, so the modules don't clash with Find
# modules of the same name from other projects.
if(_MAGNUMEXTRAS_DEPENDENCY_MODULE_DIR AND NOT _MAGNUMEXTRAS_DEPENDENCY_MODULE_DIR IN_LIST CMAKE_MODULE_PATH)
    set(CMAKE_MODULE_PATH ${_MAGNUMEXTRAS_DEPENDENCY_MODULE_DIR} ${CMAKE_MODULE_PATH})
    set(_MAGNUMEXTRAS_REMOVE_DEPENDENCY_MODULE_DIR_FROM_CMAKE_PATH ON)
else()
    unset(_MAGNUMEXTRAS_REMOVE_DEPENDENCY_MODULE_DIR_FROM_CMAKE_PATH)
endif()

# Component distinction (listing them explicitly to avoid mistakes with finding
# components from other repositories)
set(_MAGNUMEXTRAS_LIBRARY_COMPONENTS Ui)
set(_MAGNUMEXTRAS_EXECUTABLE_COMPONENTS player ui-gallery)
# Nothing is enabled by default right now
set(_MAGNUMEXTRAS_IMPLICITLY_ENABLED_COMPONENTS )

# Inter-component dependencies
set(_MAGNUMEXTRAS_ui-gallery_DEPENDENCIES Ui)

# Ensure that all inter-component dependencies are specified as well
set(_MAGNUMEXTRAS_ADDITIONAL_COMPONENTS )
foreach(_component ${MagnumExtras_FIND_COMPONENTS})
    # Mark the dependencies as required if the component is also required
    if(MagnumExtras_FIND_REQUIRED_${_component})
        foreach(_dependency ${_MAGNUMEXTRAS_${_component}_DEPENDENCIES})
            set(MagnumExtras_FIND_REQUIRED_${_dependency} TRUE)
        endforeach()
    endif()

    list(APPEND _MAGNUMEXTRAS_ADDITIONAL_COMPONENTS ${_MAGNUMEXTRAS_${_component}_DEPENDENCIES})
endforeach()

# Join the lists, remove duplicate components
set(_MAGNUMEXTRAS_ORIGINAL_FIND_COMPONENTS ${MagnumExtras_FIND_COMPONENTS})
if(_MAGNUMEXTRAS_ADDITIONAL_COMPONENTS)
    list(INSERT MagnumExtras_FIND_COMPONENTS 0 ${_MAGNUMEXTRAS_ADDITIONAL_COMPONENTS})
endif()
if(MagnumExtras_FIND_COMPONENTS)
    list(REMOVE_DUPLICATES MagnumExtras_FIND_COMPONENTS)
endif()

# Additional components
foreach(_component ${MagnumExtras_FIND_COMPONENTS})
    string(TOUPPER ${_component} _COMPONENT)

    # Create imported target in case the library is found. If the project is
    # added as subproject to CMake, the target already exists and all the
    # required setup is already done from the build tree.
    if(TARGET "MagnumExtras::${_component}") # Quotes to "fix" KDE's higlighter
        set(MagnumExtras_${_component}_FOUND TRUE)
    else()
        # Find library includes. Each has a configure.h file so there doesn't
        # need to be any specialized per-library handling.
        if(_component IN_LIST _MAGNUMEXTRAS_LIBRARY_COMPONENTS)
            find_file(_MAGNUMEXTRAS_${_COMPONENT}_CONFIGURE_FILE configure.h
                HINTS ${MAGNUMEXTRAS_INCLUDE_DIR}/Magnum/${_component})
            mark_as_advanced(_MAGNUMEXTRAS_${_COMPONENT}_CONFIGURE_FILE)
        endif()

        # Library components
        if(_component IN_LIST _MAGNUMEXTRAS_LIBRARY_COMPONENTS)
            # Try to find both debug and release version
            find_library(MAGNUMEXTRAS_${_COMPONENT}_LIBRARY_DEBUG Magnum${_component}-d)
            find_library(MAGNUMEXTRAS_${_COMPONENT}_LIBRARY_RELEASE Magnum${_component})
            mark_as_advanced(MAGNUMEXTRAS_${_COMPONENT}_LIBRARY_DEBUG
                MAGNUMEXTRAS_${_COMPONENT}_LIBRARY_RELEASE)

            # Determine if the library is static or dynamic by reading the
            # per-library config file. If the file wasn't found, skip this so
            # it fails on the FPHSA below and not right here.
            if(_MAGNUMEXTRAS_${_COMPONENT}_CONFIGURE_FILE)
                file(READ ${_MAGNUMEXTRAS_${_COMPONENT}_CONFIGURE_FILE} _magnumExtrasConfigure)
                string(REGEX REPLACE ";" "\\\\;" _magnumExtrasConfigure "${_magnumExtrasConfigure}")
                string(REGEX REPLACE "\n" ";" _magnumExtrasConfigure "${_magnumExtrasConfigure}")
                list(FIND _magnumExtrasConfigure "#define MAGNUM_${_COMPONENT}_BUILD_STATIC" _magnumExtrasBuildStatic)
                if(NOT _magnumExtrasBuildStatic EQUAL -1)
                    # The variable is inconsistently named between C++ and
                    # CMake, so keep it underscored / private
                    set(_MAGNUMEXTRAS_${_COMPONENT}_BUILD_STATIC ON)
                endif()
            endif()

            # On Windows, if we have a dynamic build of given library, find the
            # DLLs as well. Abuse find_program() since the DLLs should be
            # alongside usual executables. On MinGW they however have a lib
            # prefix.
            if(CORRADE_TARGET_WINDOWS AND NOT _MAGNUMEXTRAS_${_COMPONENT}_BUILD_STATIC)
                find_program(MAGNUMEXTRAS_${_COMPONENT}_DLL_DEBUG ${CMAKE_SHARED_LIBRARY_PREFIX}Magnum${_component}-d.dll)
                find_program(MAGNUMEXTRAS_${_COMPONENT}_DLL_RELEASE ${CMAKE_SHARED_LIBRARY_PREFIX}Magnum${_component}.dll)
                mark_as_advanced(MAGNUMEXTRAS_${_COMPONENT}_DLL_DEBUG
                    MAGNUMEXTRAS_${_COMPONENT}_DLL_RELEASE)
            # If not on Windows or on a static build, unset the DLL variables
            # to avoid leaks when switching shared and static builds
            else()
                unset(MAGNUMEXTRAS_${_COMPONENT}_DLL_DEBUG CACHE)
                unset(MAGNUMEXTRAS_${_COMPONENT}_DLL_RELEASE CACHE)
            endif()

        # Executables
        elseif(_component IN_LIST _MAGNUMEXTRAS_EXECUTABLE_COMPONENTS)
            find_program(MAGNUMEXTRAS_${_COMPONENT}_EXECUTABLE magnum-${_component})
            mark_as_advanced(MAGNUMEXTRAS_${_COMPONENT}_EXECUTABLE)

        # Something unknown, skip. FPHSA will take care of handling this below.
        else()
            continue()
        endif()

        # Decide if the library was found. If not, skip the rest, which
        # populates the target properties and finds additional dependencies.
        # This means that the rest can also rely on that some FindXYZ.cmake is
        # present in _MAGNUMEXTRAS_DEPENDENCY_MODULE_DIR -- given that the
        # library needing XYZ was found, it likely also installed FindXYZ for
        # itself.
        if(
            # If the component is a library, it should have the configure file
            (_component IN_LIST _MAGNUMEXTRAS_LIBRARY_COMPONENTS AND _MAGNUMEXTRAS_${_COMPONENT}_CONFIGURE_FILE AND (
                # And it should have a debug library, and a DLL found if
                # expected
                (MAGNUMEXTRAS_${_COMPONENT}_LIBRARY_DEBUG AND (
                    NOT DEFINED MAGNUMEXTRAS_${_COMPONENT}_DLL_DEBUG OR
                    MAGNUMEXTRAS_${_COMPONENT}_DLL_DEBUG)) OR
                # Or have a release library, and a DLL found if expected
                (MAGNUMEXTRAS_${_COMPONENT}_LIBRARY_RELEASE AND (
                    NOT DEFINED MAGNUMEXTRAS_${_COMPONENT}_DLL_RELEASE OR
                    MAGNUMEXTRAS_${_COMPONENT}_DLL_RELEASE)))) OR
            # If the component is an executable, it should have just the
            # location
            (_component IN_LIST _MAGNUMEXTRAS_EXECUTABLE_COMPONENTS AND MAGNUMEXTRAS_${_COMPONENT}_EXECUTABLE)
        )
            set(MagnumExtras_${_component}_FOUND TRUE)
        else()
            set(MagnumExtras_${_component}_FOUND FALSE)
            continue()
        endif()

        # Target and location for libraries
        if(_component IN_LIST _MAGNUMEXTRAS_LIBRARY_COMPONENTS)
            if(_MAGNUMEXTRAS_${_COMPONENT}_BUILD_STATIC)
                add_library(MagnumExtras::${_component} STATIC IMPORTED)
            else()
                add_library(MagnumExtras::${_component} SHARED IMPORTED)
            endif()

            foreach(_CONFIG DEBUG RELEASE)
                if(NOT MAGNUMEXTRAS_${_COMPONENT}_LIBRARY_${_CONFIG})
                    continue()
                endif()

                set_property(TARGET MagnumExtras::${_component} APPEND PROPERTY
                    IMPORTED_CONFIGURATIONS ${_CONFIG})
                # Unfortunately for a DLL the two properties are swapped out,
                # *.lib goes to IMPLIB, so it's duplicated like this
                if(DEFINED MAGNUMEXTRAS_${_COMPONENT}_DLL_${_CONFIG})
                    # Quotes to "fix" KDE's higlighter
                    set_target_properties("MagnumExtras::${_component}" PROPERTIES
                        IMPORTED_LOCATION_${_CONFIG} ${MAGNUMEXTRAS_${_COMPONENT}_DLL_${_CONFIG}}
                        IMPORTED_IMPLIB_${_CONFIG} ${MAGNUMEXTRAS_${_COMPONENT}_LIBRARY_${_CONFIG}})
                else()
                    set_property(TARGET MagnumExtras::${_component} PROPERTY
                        IMPORTED_LOCATION_${_CONFIG} ${MAGNUMEXTRAS_${_COMPONENT}_LIBRARY_${_CONFIG}})
                endif()
            endforeach()

        # Target and location for executable components
        elseif(_component IN_LIST _MAGNUMEXTRAS_EXECUTABLE_COMPONENTS)
            add_executable(MagnumExtras::${_component} IMPORTED)

            set_property(TARGET MagnumExtras::${_component} PROPERTY
                IMPORTED_LOCATION ${MAGNUMEXTRAS_${_COMPONENT}_EXECUTABLE})
        endif()

        # No special setup required for Ui library

        if(_component IN_LIST _MAGNUMEXTRAS_LIBRARY_COMPONENTS)
            # Link to core Magnum library, add inter-library dependencies
            foreach(_dependency ${_MAGNUMEXTRAS_${_component}_CORRADE_DEPENDENCIES})
                set_property(TARGET MagnumExtras::${_component} APPEND PROPERTY
                    INTERFACE_LINK_LIBRARIES Corrade::${_dependency})
            endforeach()
            set_property(TARGET MagnumExtras::${_component} APPEND PROPERTY
                INTERFACE_LINK_LIBRARIES Magnum::Magnum)
            foreach(_dependency ${_MAGNUMEXTRAS_${_component}_MAGNUM_DEPENDENCIES})
                set_property(TARGET MagnumExtras::${_component} APPEND PROPERTY
                    INTERFACE_LINK_LIBRARIES Magnum::${_dependency})
            endforeach()

            # Add inter-project dependencies
            foreach(_dependency ${_MAGNUMEXTRAS_${_component}_DEPENDENCIES})
                set_property(TARGET MagnumExtras::${_component} APPEND PROPERTY
                    INTERFACE_LINK_LIBRARIES MagnumExtras::${_dependency})
            endforeach()
        endif()
    endif()
endforeach()

# For CMake 3.16+ with REASON_FAILURE_MESSAGE, provide additional potentially
# useful info about the failed components.
if(NOT CMAKE_VERSION VERSION_LESS 3.16)
    set(_MAGNUMEXTRAS_REASON_FAILURE_MESSAGE )
    # Go only through the originally specified find_package() components, not
    # the dependencies added by us afterwards
    foreach(_component ${_MAGNUMEXTRAS_ORIGINAL_FIND_COMPONENTS})
        if(MagnumExtras_${_component}_FOUND)
            continue()
        endif()

        # If it's not known at all, tell the user -- it might be a new library
        # and an old Find module, or something platform-specific.
        if(NOT _component IN_LIST _MAGNUMEXTRAS_LIBRARY_COMPONENTS AND NOT _component IN_LIST _MAGNUMEXTRAS_EXECUTABLE_COMPONENTS)
            list(APPEND _MAGNUMEXTRAS_REASON_FAILURE_MESSAGE "${_component} is not a known component on this platform.")
        # Otherwise, if it's not among implicitly built components, hint that
        # the user may need to enable it
        # TODO: currently, the _FOUND variable doesn't reflect if dependencies
        #   were found. When it will, this needs to be updated to avoid
        #   misleading messages.
        elseif(NOT _component IN_LIST _MAGNUMEXTRAS_IMPLICITLY_ENABLED_COMPONENTS)
            string(TOUPPER ${_component} _COMPONENT)
            list(APPEND _MAGNUMEXTRAS_REASON_FAILURE_MESSAGE "${_component} is not built by default. Make sure you enabled MAGNUM_WITH_${_COMPONENT} when building Magnum Extras.")
        # Otherwise we have no idea. Better be silent than to print something
        # misleading.
        else()
        endif()
    endforeach()

    string(REPLACE ";" " " _MAGNUMEXTRAS_REASON_FAILURE_MESSAGE "${_MAGNUMEXTRAS_REASON_FAILURE_MESSAGE}")
    set(_MAGNUMEXTRAS_REASON_FAILURE_MESSAGE REASON_FAILURE_MESSAGE "${_MAGNUMEXTRAS_REASON_FAILURE_MESSAGE}")
endif()

# Remove Magnum Extras dependency module dir from CMAKE_MODULE_PATH again. Do
# it before the FPHSA call which may exit early in case of a failure.
if(_MAGNUMEXTRAS_REMOVE_DEPENDENCY_MODULE_DIR_FROM_CMAKE_PATH)
    list(REMOVE_ITEM CMAKE_MODULE_PATH ${_MAGNUMEXTRAS_DEPENDENCY_MODULE_DIR})
    unset(_MAGNUMEXTRAS_REMOVE_DEPENDENCY_MODULE_DIR_FROM_CMAKE_PATH)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MagnumExtras
    REQUIRED_VARS MAGNUMEXTRAS_INCLUDE_DIR
    HANDLE_COMPONENTS
    ${_MAGNUMEXTRAS_REASON_FAILURE_MESSAGE})
