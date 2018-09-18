#ifndef Magnum_Octree_Octree_h
#define Magnum_Octree_Octree_h
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
 * @brief Class @ref Magnum::Octree::Octree
 */

#include <Magnum/Magnum.h>
#include <Magnum/Math/Frustum.h>
#include <Magnum/Math/Range.h>
#include <Magnum/Math/Vector3.h>
#include <Magnum/Math/Vector4.h>

#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/ArrayView.h>

#include <vector>
#include "Magnum/Octree/visibility.h"

namespace Magnum { namespace Octree {

enum class OctantStatus: Byte {
    NoCorners = 0,   /**< No corners of the octant are inside the testing volume */
    SomeCorners = 1, /**< Neither all nor no corners of the octant are inside the testing volume */
    AllCorners = 2,  /**< All corners of the octant are inside the testing volume */
};

/**
@brief Tree data structure for space partitioning.

$$ n $$-dimensional version of quadree or octree. Each node has exactly 0 or $$ 2^n $$
children.

# Performance

While construction is expensive, the data structure allows efficient query of elements
in range or elements in a frustum.

# Usage

TODO
*/
template<UnsignedInt dimensions, class T, class Data>
class Tree {
    public:

        enum {
            NodesPerLevel = 1 << dimensions
        };

        /** @brief Default constructor */
        Tree()=default;

        /** @brief Copying is not allowed */
        Tree(const Tree<dimensions, T, Data>&) = delete;

        /** @brief Move Constructor */
        Tree(Tree<dimensions, T, Data>&&)=default;

        /** @brief Default destructor */
        ~Tree()=default;

        /** @brief Copying is not allowed */
        Tree<dimensions, T, Data>& operator=(const Tree<dimensions, T, Data>&) = delete;

        /** @brief Move assignment */
        Tree<dimensions, T, Data>& operator=(Tree<dimensions, T, Data>&&);

        /**
         * @brief Build an octree from given bounds and data with given max depth
         *
         * @param bounds Axis aligned bounding shapes of each entry
         * @param data Data associated with each entry
         * @param maxDepth Maximum tree depth (levels in the tree)
         */
        Tree(const Containers::ArrayView<Math::Range<dimensions, T>> bounds,
             const Containers::ArrayView<Data> data, const Int maxDepth);

        /** @brief Get the bounding boxes in this octree node TODO: rename */
        const Containers::ArrayView<const Math::Range3D<T>> boundingBoxes() const { return _bounds; }

        /** @brief Get all the data contained in this octree */
        const Containers::ArrayView<const Data> data() const { return _data; }

        /** @brief Whether this node is empty */
        bool empty() const { return _data.empty(); }

        bool size() const { return _data.size(); }

        /**
         * @brief Check whether given node is a leaf node.
         *
         * An node is a leaf node if it has no children.
         */
        bool isLeafNode(Int nodeIndex) const { return _childrenStart[nodeIndex] == 0; }

        /**
         * @brief Get the child of the octree in which the point is stored, if the point exists
         * @param point    The point that should be found
         * @return The index of the child potentially containing the point
         */
        Int octantContainingBox(Int nodeIndex, const Math::Range3D<T>& point) const;

        /**
         * @brief Get all points contained of the octree within given bounds
         * @param found    All the points found in the specified bounds
         * @param min      The minimal corner of the search bounds
         * @param max      The maximal corner of the search bounds
         * @return Reference to self (for method chaining)
         */
        const Tree<dimensions, T, Data>& points(std::vector<Data>& resultData, const Math::Range3D<T>& range, Int nodeIndex=0) const;
        const Tree<dimensions, T, Data>& points(std::vector<Data>& resultData, const Math::Frustum<T>& frustum, Int nodeIndex=0) const;
        const Tree<dimensions, T, Data>& points(std::vector<Data>& resultData, Int nodeIndex=0) const;

    private:

        OctantStatus cubeInFrustum(const Math::Frustum<T>& frustum, Int nodeIndex=0) const;

        /* Find node which should hold entry with given bounding box.
           @returns Depth, and Index of that node as well as the child subindex this
                    entry would belong into, if the found node is a leaf node and finally
                    the depth in the tree the node is at */
        std::tuple<Int, Int, Int, Int> findNodeFor(const Math::Range3D<T> boundingBox) const;

        Containers::Array<Math::Vector3<T>> _centers;
        Containers::Array<T> _radii;

        Containers::Array<Math::Range3D<T>> _bounds;
        Containers::Array<Data> _data;
        /* Range of data entries in _data associated with each node */
        Containers::Array<Math::Vector2<UnsignedInt>> _perNodeData;

        Containers::Array<Int> _childrenStart;

        Int _maxDepth;
};

template<UnsignedInt dimensions, class T, class Data>
inline Tree<dimensions, T, Data>& Tree<dimensions, T, Data>::operator=(Tree<dimensions, T, Data>&& other) {
    using std::swap;
    swap(_centers, other._centers);
    swap(_radii, other._radii);
    swap(_bounds, other._bounds);
    swap(_data, other._data);
    swap(_perNodeData, other._perNodeData);
    swap(_childrenStart, other._childrenStart);
    return *this;
}

template<class Data> using Quadtree = Tree<2, Float, Data>;
template<class Data> using Quadtreed = Tree<2, Double, Data>;

template<class Data> using Octree = Tree<3, Float, Data>;
template<class Data> using Octreed = Tree<3, Double, Data>;

}}

#endif
