# - Find Magnum extras
#
# Basic usage:
#  find_package(MagnumExtras [REQUIRED])
# This command tries to find Magnum extras and then defines:
#  MAGNUMEXTRAS_FOUND          - Whether Magnum extras were found
# This command alone is useless without specifying the components:
#  (none yet)
# Example usage with specifying the components is:
#  find_package(MagnumExtras [REQUIRED|COMPONENTS]
#               SomeLibraryThatDoesntExistYet)
# For each component is then defined:
#  MAGNUM_*_FOUND           - Whether the component was found
#  MAGNUM_*_LIBRARIES       - Component library and dependent libraries
#  MAGNUM_*_INCLUDE_DIRS    - Include dirs of dependencies
#
# The package is found if either debug or release version of each requested
# library is found. If both debug and release libraries are found, proper
# version is chosen based on actual build configuration of the project (i.e.
# Debug build is linked to debug libraries, Release build to release
# libraries).
#
# Additionally these variables are defined for internal usage:
#  MAGNUM_*_LIBRARY         - Component library (w/o dependencies)
#  MAGNUM_*_LIBRARY_DEBUG   - Debug version of given library, if found
#  MAGNUM_*_LIBRARY_RELEASE - Release version of given library, if found
#

#
#   This file is part of Magnum.
#
#   Copyright © 2010, 2011, 2012, 2013, 2014, 2015
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

# Ensure that all inter-component dependencies are specified as well
set(_MAGNUMEXTRAS_ADDITIONAL_COMPONENTS )
foreach(component ${MagnumExtras_FIND_COMPONENTS})
    string(TOUPPER ${component} _COMPONENT)

    # The dependencies need to be sorted by their dependency order as well
    # (no inter-component dependencies yet)

    list(APPEND _MAGNUMEXTRAS_ADDITIONAL_COMPONENTS ${_MAGNUMEXTRAS_${_COMPONENT}_DEPENDENCIES})
endforeach()

# Join the lists, remove duplicate components
if(_MAGNUMEXTRAS_ADDITIONAL_COMPONENTS)
    list(INSERT MagnumExtras_FIND_COMPONENTS 0 ${_MAGNUMEXTRAS_ADDITIONAL_COMPONENTS})
endif()
if(MagnumExtras_FIND_COMPONENTS)
    list(REMOVE_DUPLICATES MagnumExtras_FIND_COMPONENTS)
endif()

# Magnum library dependencies
set(_MAGNUMEXTRAS_DEPENDENCIES )
foreach(component ${MagnumExtras_FIND_COMPONENTS})
    string(TOUPPER ${component} _COMPONENT)

    # (none yet)

    list(APPEND _MAGNUMEXTRAS_DEPENDENCIES ${_MAGNUMEXTRAS_${_COMPONENT}_MAGNUM_DEPENDENCY})
endforeach()
find_package(Magnum REQUIRED ${_MAGNUMEXTRAS_DEPENDENCIES})

# Additional components
foreach(component ${MagnumExtras_FIND_COMPONENTS})
    string(TOUPPER ${component} _COMPONENT)

    # Try to find both debug and release version of the library
    find_library(MAGNUM_${_COMPONENT}_LIBRARY_DEBUG Magnum${component}-d)
    find_library(MAGNUM_${_COMPONENT}_LIBRARY_RELEASE Magnum${component})

    # Set the _LIBRARY variable based on what was found
    if(MAGNUM_${_COMPONENT}_LIBRARY_DEBUG AND MAGNUM_${_COMPONENT}_LIBRARY_RELEASE)
        set(MAGNUM_${_COMPONENT}_LIBRARY
            debug ${MAGNUM_${_COMPONENT}_LIBRARY_DEBUG}
            optimized ${MAGNUM_${_COMPONENT}_LIBRARY_RELEASE})
    elseif(MAGNUM_${_COMPONENT}_LIBRARY_DEBUG)
        set(MAGNUM_${_COMPONENT}_LIBRARY ${MAGNUM_${_COMPONENT}_LIBRARY_DEBUG})
    elseif(MAGNUM_${_COMPONENT}_LIBRARY_RELEASE)
        set(MAGNUM_${_COMPONENT}_LIBRARY ${MAGNUM_${_COMPONENT}_LIBRARY_RELEASE})
    endif()

    set(_MAGNUM_${_COMPONENT}_INCLUDE_PATH_SUFFIX ${component})

    # (none yet)

    # Try to find the includes
    if(_MAGNUM_${_COMPONENT}_INCLUDE_PATH_NAMES)
        find_path(_MAGNUM_${_COMPONENT}_INCLUDE_DIR
            NAMES ${_MAGNUM_${_COMPONENT}_INCLUDE_PATH_NAMES}
            PATHS ${MAGNUM_INCLUDE_DIR}/Magnum/${_MAGNUM_${_COMPONENT}_INCLUDE_PATH_SUFFIX})
    endif()

    # Add Magnum library dependency, if there is any
    if(_MAGNUM_${_COMPONENT}_MAGNUM_DEPENDENCY)
        string(TOUPPER ${_MAGNUM_${_COMPONENT}_MAGNUM_DEPENDENCY} _DEPENDENCY)
        set(_MAGNUM_${_COMPONENT}_LIBRARIES ${_MAGNUM_${_COMPONENT}_LIBRARIES} ${MAGNUM_${_DEPENDENCY}_LIBRARIES})
        set(_MAGNUM_${_COMPONENT}_INCLUDE_DIRS ${_MAGNUM_${_COMPONENT}_INCLUDE_DIRS} ${MAGNUM_${_DEPENDENCY}_INCLUDE_DIRS})
    endif()

    # Decide if the library was found
    if(MAGNUM_${_COMPONENT}_LIBRARY AND _MAGNUM_${_COMPONENT}_INCLUDE_DIR)
        set(MAGNUM_${_COMPONENT}_LIBRARIES
            ${MAGNUM_${_COMPONENT}_LIBRARY}
            ${_MAGNUM_${_COMPONENT}_LIBRARIES})
        set(MAGNUM_${_COMPONENT}_INCLUDE_DIRS
            ${_MAGNUM_${_COMPONENT}_INCLUDE_DIRS})

        set(MagnumExtras_${component}_FOUND TRUE)

        # Don't expose variables w/o dependencies to end users
        mark_as_advanced(FORCE
            MAGNUM_${_COMPONENT}_LIBRARY_DEBUG
            MAGNUM_${_COMPONENT}_LIBRARY_RELEASE
            MAGNUM_${_COMPONENT}_LIBRARY
            _MAGNUM_${_COMPONENT}_INCLUDE_DIR)
    else()
        set(MagnumExtras_${component}_FOUND FALSE)
    endif()
endforeach()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MagnumExtras
    REQUIRED_VARS MAGNUM_LIBRARY MAGNUM_INCLUDE_DIR
    HANDLE_COMPONENTS)
