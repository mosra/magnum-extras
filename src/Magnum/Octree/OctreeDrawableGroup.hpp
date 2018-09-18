#ifndef Magnum_SceneGraph_OctreeDrawableGroup_hpp
#define Magnum_SceneGraph_OctreeDrawableGroup_hpp
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015
              Vladimír Vondruš <mosra@centrum.cz>
    Copyright © 2015 Andrea Capobianco <andrea.c.2205@gmail.com>
    Copyright © 2015 Jonathan Hale <squareys@googlemail.com>

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
 * @brief @ref compilation-speedup-hpp "Template implementation" for @ref OctreeDrawableGroup.h
 */

#include "Magnum/Octree/Octree.hpp"

#include "Magnum/Octree/OctreeDrawableGroup.h"

namespace Magnum { namespace SceneGraph {

template<class T> OctreeDrawableGroup<T>& OctreeDrawableGroup<T>::add(Drawable<3, T>& drawable, const Range3D& boundingBox) {
    _drawables.push_back(&drawable);
    _boundingBoxes.push_back(boundingBox);

    /* Reset octree */
    if(!_octree.empty()) _octree = Octree::Octree<Drawable<3, T>*>();

    return *this;
}

template<class T> OctreeDrawableGroup<T>& OctreeDrawableGroup<T>::add(const DrawableGroup<3, T>& drawables, const std::vector<Range3D>& boundingBoxes) {
    CORRADE_ASSERT(drawables.size() == boundingBoxes.size(), "Drawables and bounding boxes have to be equal size.", *this);
    for(size_t i = 0; i < drawables.size(); ++i) {
        _drawables.push_back(drawables[i]);
        _boundingBoxes.push_back(boundingBoxes[i]);
    }

    /* Reset octree */
    if(!_octree.empty()) _octree = Octree::Octree<Drawable<3, T>*>();

    return *this;
}

template<class T> OctreeDrawableGroup<T>& OctreeDrawableGroup<T>::cull(Camera<3, T>& camera) {
    //CORRADE_ASSERT(_octree != nullptr, "Octree needs to be built before culling.", *this);

    /* Clear for next culling data */
    for(size_t i = 0; i < size(); ++i) {
        this->remove((*this)[i]);
    }

    /* Extract view frustum from view projection matrix */
    const Matrix4 mvp = Matrix4(camera.projectionMatrix()*camera.cameraMatrix());
    const Math::Frustum<Float> frustum = Math::Frustum<Float>::fromMatrix(mvp);
    std::vector<Drawable<3, T>*> visibleDrawables;

    _octree.points(visibleDrawables, frustum);

    for(auto drawable : visibleDrawables) {
        DrawableGroup<3, T>::add(*drawable);
    }

    return *this;
}

template<class T> OctreeDrawableGroup<T>& OctreeDrawableGroup<T>::buildOctree(int maxDepth) {
    if(_octree.size() != _boundingBoxes.size()) {
        _octree = Octree::Octree<Drawable<3, T>*>{
            Containers::arrayView(_boundingBoxes.data(), _boundingBoxes.size()),
                Containers::arrayView(_drawables.data(), _drawables.size()), maxDepth};
    } /* else: already built */

    return *this;
}


}}

#endif
