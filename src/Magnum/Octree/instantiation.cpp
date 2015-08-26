/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018
              Vladimír Vondruš <mosra@centrum.cz>
    Copyright © 2015 Andrea Capobianco <andrea.c.2205@gmail.com>
    Copyright © 2015, 2018 Jonathan Hale <squareys@googlemail.com>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#include "Magnum/Octree/Octree.h"
#include "Magnum/Octree/OctreeDrawableGroup.h"

namespace Corrade { namespace Utility {

#ifndef DOXYGEN_GENERATING_OUTPUT
#endif

}}

namespace Magnum { namespace Octree {

#if !defined(CORRADE_TARGET_WINDOWS) || defined(__MINGW32__)
#define MAGNUM_OCTREE_EXPORT_HPP MAGNUM_OCTREE_EXPORT
#else
#define MAGNUM_OCTREE_EXPORT_HPP
#endif

}

namespace SceneGraph {

/* On non-MinGW Windows the instantiations are already marked with extern
   template */
#if !defined(CORRADE_TARGET_WINDOWS) || defined(__MINGW32__)
#define MAGNUM_SCENEGRAPH_EXPORT_HPP MAGNUM_SCENEGRAPH_EXPORT
#else
#define MAGNUM_SCENEGRAPH_EXPORT_HPP
#endif

template class MAGNUM_SCENEGRAPH_EXPORT_HPP OctreeDrawableGroup<Float>;
template class MAGNUM_SCENEGRAPH_EXPORT_HPP OctreeDrawableGroup<Double>;

}}
