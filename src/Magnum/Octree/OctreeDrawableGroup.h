#ifndef Magnum_SceneGraph_OctreeDrawableGroup_h
#define Magnum_SceneGraph_OctreeDrawableGroup_h
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

/** @file
 * @brief Class @ref Magnum::SceneGraph::OctreeDrawableGroup
 */

#include <functional>

#include "Magnum/SceneGraph/Drawable.h"
#include "Magnum/Octree/Octree.h"

namespace Magnum { namespace SceneGraph {

/**
@brief OctreeDrawableGroup

A @ref DrawableGroup which makes use of view frustum culling to avoid drawing meshes
outside of the cameras view using an @ref Magnum::Octree::Octree.

This only works for three dimensional scenes.

@todo doc
@todo 2D

@see @ref scenegraph, @ref BasicDrawable2D, @ref BasicDrawable3D,
    @ref Drawable2D, @ref Drawable3D, @ref DrawableGroup
*/
template<class T> class OctreeDrawableGroup: public DrawableGroup<3, T> {
    public:
        /**
         * @brief Constructor
         *
         * Adds @ref Drawable s from drawables if specified.
         * Otherwise you can use @ref OctreeDrawableGroup::add().
         */
        explicit OctreeDrawableGroup():
            DrawableGroup<3, T>(),
            _octree{}
        {}

        /**
         * @brief Add a Drawable
         * @param drawable The drawable
         * @param boundingBox Bounding box of the drawable
         *
         * The underlying @ref Magnum::Octree::Octree will need to be rebuilt
         * @ref buildOctree().
         */
        OctreeDrawableGroup<T>& add(Drawable<3, T>& drawable, const Range3D& boundingBox);

        /**
         * @brief Add the Drawables of a DrawableGroup
         * @param drawables The drawable group
         * @param boundingBoxes Bounding boxes of the drawables
         *
         * The underlying @ref Magnum::Octree::Octree will need to be rebuilt
         * @ref buildOctree().
         */
         OctreeDrawableGroup<T>& add(const DrawableGroup<3, T>& drawables, const std::vector<Range3D>& boundingBoxes);

        /**
         * @brief Update culling to the frustum of a camera
         * @param camera The camera to do view frustum culling for
         */
         OctreeDrawableGroup<T>& cull(Camera<3, T>& camera);

        /**
         * @brief Build the octree for the underlying drawables
         * @param maxDepth Maximum depth of the octree to build
         *
         * Does nothing if an octree already has been built.
         */
         OctreeDrawableGroup<T>& buildOctree(Int maxDepth = 4);

         /** @brief The underlying octree */
         Octree::Octree<Drawable<3, T>*>& octree() {
             return _octree;
         }

    private:

        Octree::Octree<Drawable<3, T>*> _octree;
        std::vector<Drawable<3, T>*> _drawables;
        std::vector<Range3D> _boundingBoxes;

};

}}

#endif
