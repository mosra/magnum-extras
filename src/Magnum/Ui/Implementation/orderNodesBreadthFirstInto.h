#ifndef Magnum_Ui_Implementation_orderNodesBreadthFirstInto_h
#define Magnum_Ui_Implementation_orderNodesBreadthFirstInto_h
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023, 2024
              Vladimír Vondruš <mosra@centrum.cz>

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

#include <Corrade/Containers/StridedArrayView.h>
#include <Magnum/Magnum.h>

#include "Magnum/Ui/Handle.h"

/* orderNodesBreadthFirstInto() is extracted out of abstractUserInterface.h to
   a dedicated header as it's (currently?) used by SnapLayouter internals (and
   tests) as well and including the whole thing would cause needless
   dependencies and warnings about the remaining functions unused */

namespace Magnum { namespace Ui { namespace Implementation { namespace {

/* The `nodeIds` array gets filled with with node IDs in the following
   order:

    -   the first item is always -1
    -   children IDs are always after their parent in the `nodeIds` array in a
        breadth-first order

   The `childrenOffsets` and `children` arrays are temporary storage. The
   `childrenOffsets` array has to be zero-initialized. Others don't need to
   be. */
void orderNodesBreadthFirstInto(const Containers::StridedArrayView1D<const NodeHandle>& nodeParents, const Containers::ArrayView<UnsignedInt> childrenOffsets, const Containers::ArrayView<UnsignedInt> children, const Containers::ArrayView<Int> nodeIds) {
    CORRADE_INTERNAL_ASSERT(
        childrenOffsets.size() == nodeParents.size() + 2 &&
        children.size() == nodeParents.size() &&
        nodeIds.size() == nodeParents.size() + 1);

    /* Children offset for each node including root (unparented) nodes. Handle
       generation is ignored here, so free and invalid nodes are counted as
       well.

       First calculate the count of children for each, skipping the first
       element (root is at index 1, first node at index 2) ... */
    for(const NodeHandle parent: nodeParents) {
        if(parent != NodeHandle::Null) {
            const UnsignedInt parentIndex = nodeHandleId(parent);
            ++childrenOffsets[parentIndex + 2];
        } else ++childrenOffsets[1];
    }

    /* ... then convert the counts to a running offset. Now
       `[childrenOffsets[i + 2], childrenOffsets[i + 3])` is a range in which
       the `children` array contains a list of children for node `i`. The last
       element (containing the end offset) is omitted at this step. */
    {
        UnsignedInt offset = 0;
        for(UnsignedInt& i: childrenOffsets) {
            const UnsignedInt nextOffset = offset + i;
            i = offset;
            offset = nextOffset;
        }
        CORRADE_INTERNAL_ASSERT(offset == nodeParents.size());
    }

    /* Go through the node list again, convert that to child ranges. The
       `childrenOffsets` array gets shifted by one element by the process, thus
       now `[childrenOffsets[i + 1], childrenOffsets[i + 2])` is a range in
       which the `children` array below contains a list of children for node
       `i`. The last array element is now containing the end offset. */
    for(std::size_t i = 0; i != nodeParents.size(); ++i) {
        const NodeHandle parent = nodeParents[i];

        children[childrenOffsets[parent == NodeHandle::Null ? 1 : nodeHandleId(parent) + 2]++] = i;
    }

    /* Go breadth-first (so we have items sharing the same parent next to each
       other, but that doesn't really matter, it's simpler than depth-first)
       and build a node IDs list where a parent node ID is always before its
       children. */
    std::size_t outputOffset = 0;
    nodeIds[0] = -1;
    for(std::size_t i = 0; i != outputOffset + 1; ++i) {
        const Int nodeId = nodeIds[i];
        for(std::size_t j = childrenOffsets[nodeId + 1], jMax = childrenOffsets[nodeId + 2]; j != jMax; ++j) {
            nodeIds[outputOffset + 1] = children[j];
            ++outputOffset;
        }
    }
    CORRADE_INTERNAL_ASSERT(outputOffset == nodeParents.size());
}

}}}}

#endif
