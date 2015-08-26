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

#include <Magnum/Math/Functions.h>
#include <Magnum/Math/Geometry/Intersection.h>

#include <stdio.h>
#include <stdlib.h>
#include "Octree.h"

namespace Magnum { namespace Octree {

template<class T, UnsignedInt dimensions> bool intersects(const Math::Range<dimensions, T>& a, const Math::Range<dimensions, T>& b) {
    if((b.max() < a.min()).any()) return false;
    if((b.min() > a.max()).any()) return false;

    return true;
}

template<class T, UnsignedInt dimensions> bool contains(const Math::Range<dimensions, T>& a, const Math::Range<dimensions, T>& b) {
    if((a.min() >= b.min()).any()) return false;
    if((a.max() <= b.max()).any()) return false;

    return true;
}

template<UnsignedInt dimensions, class T, class Data>
std::tuple<Int, Int, Int, Int> Tree<dimensions, T, Data>::findNodeFor(const Math::Range3D<T> boundingBox) const {
    Int nodeIndex = 0;
    Int childIndex = 0;
    Int childSubIndex = 0;
    Int depth = 0;

    /* Decend the tree until leaf node is reached */
    while((childIndex = _childrenStart[nodeIndex]) != 0 &&
          (childSubIndex = octantContainingBox(nodeIndex, boundingBox)) != -1) {
        nodeIndex = childIndex + childSubIndex;
        ++depth;
    }

    return std::make_tuple(nodeIndex, childIndex, childSubIndex, depth);
}

template<UnsignedInt dimensions, class T, class Data>
Tree<dimensions, T, Data>::Tree(
        const Containers::ArrayView<Math::Range<dimensions, T>> bounds,
        const Containers::ArrayView<Data> data, const Int maxDepth)
{
    CORRADE_ASSERT(data.size() == bounds.size(), "Bounds and data need to be same size.", );
    if(bounds.empty()) return;

    _bounds = Containers::Array<Math::Range3D<T>>(Containers::DefaultInit, data.size());
    _data = Containers::Array<Data>(Containers::DefaultInit, data.size());

    /* Per node attribute arrays allocations, preallocate for max amount of nodes.
       Will never be more than amount of data (TODO: You wish) */
    _maxDepth = maxDepth;
    const Int maxNodes = Math::min(Math::pow(Int(NodesPerLevel), maxDepth) + 1, Int(data.size()));
    _childrenStart = Containers::Array<Int>(Containers::ValueInit, maxNodes);
    // TODO Use NoInit where possible later
    _centers = Containers::Array<Math::Vector3<T>>(Containers::DefaultInit, maxNodes);
    _radii = Containers::Array<T>(Containers::DefaultInit, maxNodes);
    _perNodeData = Containers::Array<Math::Vector2<UnsignedInt>>(Containers::DefaultInit, maxNodes);

    /* Temporary vector of vectors to collect data elements. Later flattened to an array */
    std::vector<std::vector<Int>> entries;
    entries.reserve(maxNodes);
    entries.resize(1); /* Root node entries */

    /* Calculate bounds: find minimal and maximal x, y, z in the data points */
    Math::Vector3<T> min{bounds[0].min()};
    Math::Vector3<T> max{bounds[0].max()};

    for(auto boundingBox : bounds.suffix(1)) {
        min = Math::min(boundingBox.min(), min);
        max = Math::max(boundingBox.max(), max);
    }

    /* Bounds should be squared, not rectangular. Take the largest of the radi. */
    _radii[0] = (max - min).max()/2;

    /* Find the center of the bounding box */
    _centers[0] = min + Math::Vector3<T>{_radii[0]};

    Int lastNode = 0;
    Int dataIndex = 0;
    auto dataItor = data.begin();
    for(const auto& boundingBox : bounds) {
        Int nodeIndex, childrenStart, childSubIndex, depth;
        std::tie(nodeIndex, childrenStart, childSubIndex, depth) = findNodeFor(boundingBox);

        /* Use a simple code for making traversion simple by using bitwise or operations:
           <   childs coordinate is smaller than the coordinate of the center of the
               current node
           >   childs coordinate is greater than the coordinate of the center of the
               current node

           Example: child[3].x < center.x, child[3].y > center.y, child[3].z > center.z

           child:   0 1 2 3 4 5 6 7
           x:       < < < < > > > >
           y:       < < > > < < > >
           z:       < > < > < > < > */
        bool inserted = false;

        /* If this is a leaf node */
        if(childrenStart == 0) {
            /* As long as max depth is not reached yet, we may split the leaf node.
               We only split the node if if would contain at least two elements and one
               of them fits into a child. Otherwise all children would remain empty. */
           if(depth != 0 && entries[nodeIndex].size() != 0) {

                /* Get info of existing first entry in node. It may have been added only because
                   it was the only entry. All others are here because they did not fit into
                   one of the children. */
                const Int existingDataIndex = entries[nodeIndex][0];
                const Math::Range3D<T>& existingBoundingBox = bounds[existingDataIndex];

                const Int octContainingExisting = octantContainingBox(nodeIndex, existingBoundingBox);

                if(octContainingExisting != -1 || childSubIndex != -1) {
                    /* At least one entry will be put in a child */
                    const Int childrenStart = _childrenStart[nodeIndex] = lastNode + 1;
                    lastNode += NodesPerLevel;
                    CORRADE_ASSERT(lastNode <= maxNodes, "incorrect max nodes", );
                    const auto& center = _centers[nodeIndex];
                    const T radius = _radii[nodeIndex];
                    entries.resize(entries.size() + NodesPerLevel, {});
                    /* Create child nodes
                       {+0.5, +0.5, +0.5},
                       {+0.5, +0.5, -0.5},
                       {+0.5, -0.5, +0.5},
                       {+0.5, -0.5, -0.5},
                       {-0.5, +0.5, +0.5},
                       {-0.5, +0.5, -0.5},
                       {-0.5, -0.5, +0.5},
                       {-0.5, -0.5, -0.5} */
                    for(UnsignedByte i = 0; i < NodesPerLevel; ++i) {
                        const Int node = childrenStart + i;
                        _centers[node] = center +
                            radius*Math::lerp(Math::Vector3<T>{-0.5f},
                                              Math::Vector3<T>{0.5f},
                                              Math::BoolVector<3>{i});
                        _radii[node] = radius*0.5f;
                    }

                    /* If the existing entry fits into a child node, move it there */
                    if(octContainingExisting != -1) {
                        entries[childrenStart + octContainingExisting].push_back(existingDataIndex);
                        entries[nodeIndex].erase(entries[nodeIndex].begin());
                    } /* else: leave it in this node */

                    if(childSubIndex != -1) {
                        /* Move new data into child */
                        entries[childrenStart + childSubIndex].push_back(dataIndex);
                        inserted = true;
                    }
                }
            }

        /* Inner node, add to child node if it fits. */
        } else if(childSubIndex != -1) {
            entries[childrenStart + childSubIndex].push_back(dataIndex);
            inserted = true;
        }

        /* Insert here if not inserted into child node yet */
        if(!inserted)
            entries[nodeIndex].push_back(dataIndex);

        ++dataItor;
        ++dataIndex;
    }

    /* Flatten out vector of vectors to array, copying the input data */
    UnsignedInt i = 0;
    Int node = 0;
    for(auto& v: entries) {
        _perNodeData[node++] = {i, UnsignedInt(i + v.size())};
        for(const Int e: v) {
            CORRADE_ASSERT(i < _data.size(), "out of bounds", );
            _data[i] = data[e];
            _bounds[i] = bounds[e];
            ++i;
        }
    }
}

template<UnsignedInt dimensions, class T, class Data>
int Tree<dimensions, T, Data>::octantContainingBox(const Int nodeIndex, const Math::Range3D<T>& box) const {
    Int oct = 0;

    /* If one of max.x/y/z is greater than the center.x/y/z, but min.x/y/z is less than center.x/y/z,
       the AABB doesn't fit into any child node of this node, it therefore has to be stored
       into this node (-1 is returned). */
    const auto& center = _centers[nodeIndex];
    int bit = 1;
    for(int i = 0; i < dimensions; ++i, bit <<= 1) {
        if(box.min()[i] >= center[i]) {
            oct |= bit;
        } else if(box.max()[i] > center[i]) {
            /* Octant does not fully contain the box */
            return -1;
        }
    }

    return oct;
}

template<UnsignedInt dimensions, class T, class Data>
const Tree<dimensions, T, Data>& Tree<dimensions, T, Data>::points(std::vector<Data>& resultData, const Math::Range3D<T>& range, const Int nodeIndex) const {
    Int curNode = nodeIndex;
    Containers::Array<Int> nodeStack{size_t(_maxDepth)};
    Int stackFront = -1;

    do {
        const Math::Range<dimensions, T> nodeRange{_centers[curNode] - Math::Vector3<T>{_radii[curNode]},
                                                   _centers[curNode] + Math::Vector3<T>{_radii[curNode]}};
        if(contains(range, nodeRange)) {
            /* Node fully contained in range */
            points(resultData, curNode);
        } else if(intersects(range, nodeRange)) {
            /* Node only partially contained, check bounding box of each element */
            const auto& dataRange = _perNodeData[curNode];
            for(Int i = dataRange[0]; i < dataRange[1]; ++i) {
                if(intersects(range, _bounds[i])) {
                    resultData.push_back(_data[i]);
                }
            }

            const Int childrenStart = _childrenStart[curNode];
            if(childrenStart != 0) {
                nodeStack[++stackFront] = curNode;
                curNode = childrenStart;
                continue; /* Prevent side-stepping to sibling */
            }
        } /* else: Node fully outside of range */

        if (stackFront >= 0) {
            /* Set curNode to next sibling, if not the last node, otherwise parent */
            // TODO make number of bits "NodesPerLevel" dependent!
            curNode = ((curNode & 0b111) != 0) ? curNode + 1 : nodeStack[stackFront--];
        }
    } while(curNode != nodeIndex);

    return *this;
}

template<UnsignedInt dimensions, class T, class Data>
const Tree<dimensions, T, Data>& Tree<dimensions, T, Data>::points(std::vector<Data>& resultData, const Math::Frustum<T>& frustum, const Int nodeIndex) const {
    const OctantStatus octantStatus = cubeInFrustum(frustum, nodeIndex);

    if(octantStatus == OctantStatus::AllCorners) {
        points(resultData, nodeIndex);
    } else if(octantStatus == OctantStatus::SomeCorners) {
        const auto& dataRange = _perNodeData[nodeIndex];
        for(Int i = dataRange[0]; i < dataRange[1]; ++i) {
            // TODO: deprecated to rangeFrustum...
            if(Math::Geometry::Intersection::boxFrustum<T>(_bounds[i], frustum))
                resultData.push_back(_data[i]);
        }

        if(!isLeafNode(nodeIndex)) {
            const Int childrenStart = _childrenStart[nodeIndex];
            for(Int i = 0; i < NodesPerLevel; ++i) {
                points(resultData, childrenStart + i);
            }
        }
    }

    // For 100% accuracy additional test with the corners of the frustum is needed (planes are infinite)!

    return *this;
}

template<UnsignedInt dimensions, class T, class Data>
const Tree<dimensions, T, Data>& Tree<dimensions, T, Data>::points(std::vector<Data>& resultData, const Int nodeIndex) const {
    /* Root node means entire tree, copy all data */
    if(nodeIndex == 0) {
        std::copy(_data.begin(), _data.end(), std::back_inserter(resultData));
        return *this;
    }

    Containers::Array<Int> nodeStack{size_t(_maxDepth)};
    Int stackFront = -1;
    int curNode = nodeIndex;

    do {
        if(curNode != 0) {
            nodeStack[++stackFront] = curNode;
            while(curNode != 0) {
                curNode = nodeStack[++stackFront] = _childrenStart[nodeIndex];
            }
        }

        const Int poppedNode = nodeStack[stackFront--];
        if(isLeafNode(poppedNode)) {
            /* Shortcut: Add data of this leaf and all its siblings */
            const auto& dataRangeFirst = _perNodeData[poppedNode];
            const auto& dataRangeLast = _perNodeData[poppedNode+7];
            std::copy_n(_data.prefix(dataRangeFirst[0]).data(), dataRangeLast[1] - dataRangeFirst[0], resultData.end());

            curNode = 0;
        } else {
            /* Copy the data from this node */
            const auto& dataRange = _perNodeData[poppedNode];
            std::copy_n(_data.prefix(dataRange[0]).data(), dataRange[1] - dataRange[0], resultData.end());

            /* Set curNode to next sibling, if not the last node */
            // TODO make number of bits "NodesPerLevel" dependent!
            curNode = ((poppedNode & 0b111) != 0) ? poppedNode + 1 : 0;
        }
    } while(curNode != 0 || stackFront > 0);

    return *this;
}

template<UnsignedInt dimensions, class T, class Data>
OctantStatus Tree<dimensions, T, Data>::cubeInFrustum(const Math::Frustum<T>& frustum, const Int nodeIndex) const {
    /* Check for each corner of an octant whether it is inside the frustum.
       If only some of the corners are inside, the octant requires further checks. */
    Int planes = 0;

    const Math::Vector3<T> min = _centers[nodeIndex] - Math::Vector3<T>{_radii[nodeIndex]};
    const Math::Vector3<T> max = _centers[nodeIndex] + Math::Vector3<T>{_radii[nodeIndex]};

    for(const Math::Vector4<T>& plane: frustum.planes()) {
        int corners = 0;

        for(UnsignedByte c = 0; c < 8; ++c) {
            const Math::Vector3<T> corner = Math::lerp(min, max, Math::BoolVector<3>{c});

            if(Math::Geometry::Distance::pointPlaneScaled<T>(corner, plane) > 0.0f) {
                ++corners;
            }
        }

        if(corners == 0) {
            /* all corners are outside this plane */
            return OctantStatus::NoCorners;
        }

        if(corners == 8) {
            ++planes;
        }
    }

    if(planes == 6) {
        return OctantStatus::AllCorners;
    }

    return OctantStatus::SomeCorners;
}

}}

