#ifndef Magnum_Whee_Implementation_abstractUserInterface_h
#define Magnum_Whee_Implementation_abstractUserInterface_h
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023 Vladimír Vondruš <mosra@centrum.cz>

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

#include <cstring> /* std::memset() */
#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/Triple.h>
#include <Corrade/Utility/Algorithms.h>
#include <Magnum/Magnum.h>
#include <Magnum/Math/Functions.h>

#include "Magnum/Whee/AbstractUserInterface.h"
#include "Magnum/Whee/Handle.h"

/* Contains algorithms used internally in AbstractUserInterface.cpp. Extracted
   here for easier testing and ability to iterate on them in isolation without
   having to make the library compile as well. */

namespace Magnum { namespace Whee { namespace Implementation { namespace {

/* The `nodeIds` array gets filled with with node IDs in the following
   order:

    -   the first item is always -1
    -   children IDs are always after their parent in the `nodeIds` array in a
        breadth-first order

   The `childrenOffsets` and `children` arrays are temporary storage. The
   `childrenOffsets` array has to be zero-initialized. Others don't need to
   be. */
void orderNodesBreadthFirstInto(const Containers::StridedArrayView1D<const NodeHandle>& nodeParentOrOrder, const Containers::ArrayView<UnsignedInt> childrenOffsets, const Containers::ArrayView<UnsignedInt> children, const Containers::ArrayView<Int> nodeIds) {
    CORRADE_INTERNAL_ASSERT(
        childrenOffsets.size() == nodeParentOrOrder.size() + 2 &&
        children.size() == nodeParentOrOrder.size() &&
        nodeIds.size() == nodeParentOrOrder.size() + 1);

    /* Children offset for each node including root (unparented) nodes. Handle
       generation is ignored here, so free and invalid nodes are counted as
       well.

       First calculate the count of children for each, skipping the first
       element (root is at index 1, first node at index 2) ... */
    for(const NodeHandle parentOrOrder: nodeParentOrOrder) {
        if(nodeHandleGeneration(parentOrOrder) != 0) {
            const UnsignedInt parentIndex = nodeHandleId(parentOrOrder);
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
        CORRADE_INTERNAL_ASSERT(offset == nodeParentOrOrder.size());
    }

    /* Go through the node list again, convert that to child ranges. The
       `childrenOffsets` array gets shifted by one element by the process, thus
       now `[childrenOffsets[i + 1], childrenOffsets[i + 2])` is a range in
       which the `children` array below contains a list of children for node
       `i`. The last array element is now containing the end offset. */
    for(std::size_t i = 0; i != nodeParentOrOrder.size(); ++i) {
        const NodeHandle parentOrOrder = nodeParentOrOrder[i];

        children[childrenOffsets[nodeHandleGeneration(parentOrOrder) == 0 ? 1 : nodeHandleId(parentOrOrder) + 2]++] = i;
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
    CORRADE_INTERNAL_ASSERT(outputOffset == nodeParentOrOrder.size());
}

/* The `visibleNodeIds` and `visibleNodeChildrenCounts` arrays get filled with
   visible node IDs and the count of their children in the following order,
   with the returned value being the size of the prefix filled:

    -   children IDs are always right after their parent in the
        `visibleNodeIds` array in a depth-first order, with the count stored in
        the corresponding item of the `visibleNodeChildrenCounts` array

   The `childrenOffsets`, `children` and `parentsToProcess` arrays are
   temporary storage. The `childrenOffsets` array has to be zero-initialized.
   Other outputs don't need to be. */
std::size_t orderVisibleNodesDepthFirstInto(const Containers::StridedArrayView1D<const NodeHandle>& nodeParentOrOrder, const Containers::StridedArrayView1D<const NodeFlags>& nodeFlags, const Containers::StridedArrayView1D<const NodeHandle>& nodeOrderNext, const NodeHandle firstNodeOrder, const Containers::ArrayView<UnsignedInt> childrenOffsets, const Containers::ArrayView<UnsignedInt> children, const Containers::ArrayView<Containers::Triple<UnsignedInt, UnsignedInt, UnsignedInt>> parentsToProcess, const Containers::StridedArrayView1D<UnsignedInt>& visibleNodeIds, const Containers::StridedArrayView1D<UnsignedInt>& visibleNodeChildrenCounts) {
    CORRADE_INTERNAL_ASSERT(
        nodeFlags.size() == nodeParentOrOrder.size() &&
        childrenOffsets.size() == nodeParentOrOrder.size() + 1 &&
        children.size() == nodeParentOrOrder.size() &&
        /* It only reaches nodeParentOrOrder.size() if the hierarchy is a
           single branch, usually it's shorter. */
        parentsToProcess.size() == nodeParentOrOrder.size() &&
        visibleNodeIds.size() == nodeParentOrOrder.size() &&
        visibleNodeChildrenCounts.size() == nodeParentOrOrder.size());

    /* If there are no top-level nodes, nothing is visible and thus nothing to
       do */
    if(firstNodeOrder == NodeHandle::Null)
        return 0;

    /* Children offset for each node excluding top-level nodes. Handle
       generation is ignored here, so invalid (free) nodes are counted as well.
       In order to avoid orphaned subtrees and cycles, the nodes are expected
       to be made root when freed.

       First calculate the count of children for each, skipping the first
       element ... */
    for(const NodeHandle parentOrOrder: nodeParentOrOrder) {
        if(nodeHandleGeneration(parentOrOrder) == 0)
            continue;

        const UnsignedInt parentIndex = nodeHandleId(parentOrOrder);
        ++childrenOffsets[parentIndex + 1];
    }

    /* ... then convert the counts to a running offset. Now
       `[childrenOffsets[i + 1], childrenOffsets[i + 2])` is a range in which
       the `children` array contains a list of children for node `i`. The
       last element (containing the end offset) is omitted at this step. */
    {
        UnsignedInt offset = 0;
        for(UnsignedInt& i: childrenOffsets) {
            const UnsignedInt nextOffset = offset + i;
            i = offset;
            offset = nextOffset;
        }
    }

    /* Go through the node list excluding top-level nodes again, convert that
       to child ranges. The `childrenOffsets` array gets shifted by one element
       by this, so now `[childrenOffsets[i], childrenOffsets[i + 1])` is a
       range in which the `children` array below contains a list of children
       for node `i`. The last array element is now containing the end
       offset. */
    for(std::size_t i = 0; i != nodeParentOrOrder.size(); ++i) {
        const NodeHandle parentOrOrder = nodeParentOrOrder[i];
        if(nodeHandleGeneration(parentOrOrder) == 0)
            continue;
        children[childrenOffsets[nodeHandleId(parentOrOrder) + 1]++] = i;
    }

    UnsignedInt outputOffset = 0;

    /* Go through the top-level node list. It's cyclic, so stop when reaching
       the first node again. */
    {
        NodeHandle topLevel = firstNodeOrder;
        do {
            /* Skip hidden top-level nodes */
            const UnsignedInt topLevelId = nodeHandleId(topLevel);
            if(!(nodeFlags[topLevelId] & NodeFlag::Hidden)) {
                /* Add the top-level node to the output, and to the list of
                   parents to process next */
                std::size_t parentsToProcessOffset = 0;
                visibleNodeIds[outputOffset] = topLevelId;
                parentsToProcess[parentsToProcessOffset++] = {topLevelId, outputOffset++, childrenOffsets[topLevelId]};

                while(parentsToProcessOffset) {
                    const UnsignedInt id = parentsToProcess[parentsToProcessOffset - 1].first();
                    UnsignedInt& childrenOffset = parentsToProcess[parentsToProcessOffset - 1].third();

                    /* If all children were processed, we're done with this
                       node */
                    if(childrenOffset == childrenOffsets[id + 1]) {
                        /* Save the total size */
                        const UnsignedInt firstChildOutputOffset = parentsToProcess[parentsToProcessOffset - 1].second();
                        visibleNodeChildrenCounts[firstChildOutputOffset] = outputOffset - firstChildOutputOffset - 1;

                        /* Remove from the processing stack and continue with
                           next */
                        --parentsToProcessOffset;
                        continue;
                    }

                    CORRADE_INTERNAL_DEBUG_ASSERT(childrenOffset < childrenOffsets[id + 1]);

                    /* Unless the current child is hidden, add it to the output
                       and to the list of parents to process next. Increment
                       all offsets for the next round. */
                    const UnsignedInt childId = children[childrenOffset];
                    if(!(nodeFlags[childId] & NodeFlag::Hidden)) {
                        visibleNodeIds[outputOffset] = childId;
                        parentsToProcess[parentsToProcessOffset++] = {childId, outputOffset++, childrenOffsets[childId]};
                    }

                    ++childrenOffset;
                }
            }

            topLevel = nodeOrderNext[nodeHandleId(nodeParentOrOrder[topLevelId])];
        } while(topLevel != firstNodeOrder);
    }
    CORRADE_INTERNAL_ASSERT(outputOffset <= nodeParentOrOrder.size());

    return outputOffset;
}

std::size_t visibleTopLevelNodeIndicesInto(const Containers::StridedArrayView1D<const UnsignedInt>& visibleNodeChildrenCounts, const Containers::StridedArrayView1D<UnsignedInt>& visibleTopLevelNodeIndices) {
    UnsignedInt offset = 0;
    for(UnsignedInt visibleTopLevelNodeIndex = 0; visibleTopLevelNodeIndex != visibleNodeChildrenCounts.size(); visibleTopLevelNodeIndex += visibleNodeChildrenCounts[visibleTopLevelNodeIndex] + 1)
        visibleTopLevelNodeIndices[offset++] = visibleTopLevelNodeIndex;

    return offset;
}

/* The `visibleNodeMask` has bits set for nodes in `visibleNodeIds` that are
   at least partially visible in the parent clip rects.

   The `clipStack` array is temporary storage. */
void cullVisibleNodesInto(const Containers::StridedArrayView1D<const Vector2>& absoluteNodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::StridedArrayView1D<const NodeFlags>& nodeFlags, const Containers::ArrayView<Containers::Triple<Vector2, Vector2, UnsignedInt>> clipStack, const Containers::StridedArrayView1D<const UnsignedInt>& visibleNodeIds, const Containers::StridedArrayView1D<const UnsignedInt>& visibleNodeChildrenCounts, const Containers::MutableBitArrayView visibleNodeMask) {
    CORRADE_INTERNAL_ASSERT(
        nodeSizes.size() == absoluteNodeOffsets.size() &&
        nodeFlags.size() == absoluteNodeOffsets.size() &&
        clipStack.size() == visibleNodeIds.size() &&
        visibleNodeChildrenCounts.size() == visibleNodeIds.size() &&
        visibleNodeMask.size() == absoluteNodeOffsets.size());

    /* Clear the visibility mask, individual bits will be set only if they're
       visible */
    visibleNodeMask.resetAll();

    /* Filter the visible node list and keep only nodes that are at least
       partially visible in the intersection of all parent clip rects */
    std::size_t i = 0;
    std::size_t clipStackDepth = 0;
    while(i != visibleNodeIds.size()) {
        const UnsignedInt nodeId = visibleNodeIds[i];

        /* Calculate node clip rect min and max */
        const Vector2 size = nodeSizes[nodeId];
        const Vector2 min = absoluteNodeOffsets[nodeId];
        const Vector2 max = min + size;

        /* If there's no clip rect, the node is visible */
        bool visible;
        Vector2 parentMin{NoInit}, parentMax{NoInit};
        if(clipStackDepth == 0) {
            visible = true;

        /* Otherwise check against the clip rect */
        } else {
            parentMin = clipStack[clipStackDepth - 1].first();
            parentMax = clipStack[clipStackDepth - 1].second();

            /* The node is visible if the clip rects overlap at least a bit.
               Logic follows Math::intersects() for Range. */
            /** @todo can't test & intersection calculation be done as a single
                operation? */
            visible = (parentMax > min).all() &&
                      (parentMin < max).all();
        }

        /* If the node is a clipping node, decide about a clip rect for its
           children */
        if(nodeFlags[nodeId] & NodeFlag::Clip) {
            /* If the rect has an empty area, the node isn't visible no matter
               whether it passed a clip test or not */
            if(size.x() < Math::TypeTraits<Float>::epsilon() ||
               size.y() < Math::TypeTraits<Float>::epsilon())
                visible = false;

            /* For a visible node, put the clip rect intersection onto the
               stack for children nodes */
            if(visible) {
                /* If there's a parent clip rect, calculate the clip rect
                   intersection. Logic follows Math::intersect() for Range. */
                if(clipStackDepth == 0) {
                    clipStack[clipStackDepth].first() = min;
                    clipStack[clipStackDepth].second() = max;
                } else {
                    /* GCC in a Release build spits out a useless "maybe
                       uninitialized" warning due to the NoInit even though it
                       cannot happen -- the variables *are* initialized if
                       `visible && clipStackDepth != 0`. */
                    #if defined(CORRADE_TARGET_GCC) && !defined(CORRADE_TARGET_CLANG)
                    #pragma GCC diagnostic push
                    #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
                    #endif
                    clipStack[clipStackDepth].first() = Math::max(parentMin, min);
                    clipStack[clipStackDepth].second() = Math::min(parentMax, max);
                    #if defined(CORRADE_TARGET_GCC) && !defined(CORRADE_TARGET_CLANG)
                    #pragma GCC diagnostic pop
                    #endif
                }

                /* Remember offset after all children of its node so we know
                   when to pop this clip rect off the stack */
                clipStack[clipStackDepth].third() = i + visibleNodeChildrenCounts[i] + 1;
                ++clipStackDepth;
                ++i;

            /* For an invisible node there's no point in testing any children
               as they'd be clipped away too */
            } else i += visibleNodeChildrenCounts[i] + 1;

        /* If the node isn't a clipping node, just continue to the next one
           after */
        } else ++i;

        /* Save the visibility status */
        if(visible)
            visibleNodeMask.set(nodeId);

        /* Pop the clip stack items for which all children were processed */
        while(clipStackDepth && clipStack[clipStackDepth - 1].third() == i)
            --clipStackDepth;
    }
}

/* The `visibleNodeEventDataCounts` array gets the visible per-node data counts
   added if `layerFeatures` contains `LayerFeature::Event`, otherwise it's left
   untouched. The `visibleNodeEventDataCounts` is meant to be
   `visibleNodeEventDataOffsets.exceptPrefix(1)` that's then passed to
   `orderNodeDataForEventHandling()` below.

   The `dataToUpdateLayerOffsets` and `dataToUpdateIds` arrays get filled with
   data and node IDs in the desired draw order, clustered by layer ID, with
   `dataToUpdateLayerOffsets[i]` to `dataToUpdateLayerOffsets[i + 1]` being the
   range of data in `dataToUpdateIds` corresponding to layer `i`.

   The `dataToDrawOffsets[j]` and `dataToDrawSizes[j]` is then a range in
   `dataToUpdateIds` that should be drawn with layer `dataToDrawLayerIds[j]`,
   with their total count being the return value of this function.

   The `visibleNodeDataOffsets` and `visibleNodeDataIds` arrays are temporary
   storage -- they get filled with data IDs for visible nodes, with  `visibleNodeDataOffsets[i]` to
   `visibleNodeDataOffsets[i + 1]` being the range of data in
   `visibleNodeDataIds` corresponding to visible node at index `i`. */
UnsignedInt orderVisibleNodeDataInto(const Containers::StridedArrayView1D<const UnsignedInt>& visibleNodeIds, const Containers::StridedArrayView1D<const UnsignedInt>& visibleNodeChildrenCounts, const Containers::StridedArrayView1D<const NodeHandle>& dataNodes, LayerFeatures layerFeatures, const Containers::BitArrayView visibleNodeMask, const Containers::ArrayView<UnsignedInt> visibleNodeDataOffsets, const Containers::ArrayView<UnsignedInt> visibleNodeEventDataCounts, const Containers::ArrayView<UnsignedInt> visibleNodeDataIds, const Containers::StridedArrayView1D<UnsignedInt>& dataToUpdateIds, UnsignedInt offset, const Containers::StridedArrayView1D<UnsignedInt>& dataToDrawOffsets, const Containers::StridedArrayView1D<UnsignedInt>& dataToDrawSizes) {
    CORRADE_INTERNAL_ASSERT(
        visibleNodeChildrenCounts.size() == visibleNodeIds.size() &&
        visibleNodeDataOffsets.size() == visibleNodeMask.size() + 1 &&
        visibleNodeEventDataCounts.size() == visibleNodeMask.size() &&
        visibleNodeDataIds.size() == dataNodes.size() &&
        /* These should have the size matching the top-level node count */
        dataToDrawSizes.size() == dataToDrawOffsets.size());

    /* Zero out the visibleNodeDataOffsets array */
    std::memset(visibleNodeDataOffsets.data(), 0, visibleNodeDataOffsets.size()*sizeof(UnsignedInt));

    /* Count how much data belongs to each visible node, skipping the first
       element ...*/
    for(const NodeHandle node: dataNodes) {
        if(node == NodeHandle::Null)
            continue;
        const UnsignedInt id = nodeHandleId(node);
        if(visibleNodeMask[id])
            ++visibleNodeDataOffsets[id + 1];
    }

    /* If this is an event layer, add those counts to the event data counters.
       After accumulating the counts across all layers, they'll get turned into
       a running offset and passed to orderNodeDataForEventHandling() below. */
    if(layerFeatures & LayerFeature::Event) for(std::size_t i = 0, iMax = visibleNodeMask.size(); i != iMax; ++i) {
        visibleNodeEventDataCounts[i] += visibleNodeDataOffsets[i + 1];
    }

    /* ... then convert the counts to a running offset. Now
       `[visibleNodeDataOffsets[i + 1], visibleNodeDataOffsets[i + 2])` is a
       range in which the `visibleNodeDataIds` array contains a list of data
       handles for visible node with ID `i`. The last element (containing the
       end offset) is omitted at this step. */
    {
        UnsignedInt visibleNodeDataCount = 0;
        for(UnsignedInt& i: visibleNodeDataOffsets) {
            const UnsignedInt nextOffset = visibleNodeDataCount + i;
            i = visibleNodeDataCount;
            visibleNodeDataCount = nextOffset;
        }
    }

    /* Go through the data list again, convert that to data handle ranges. The
       `visibleNodeDataOffsets` array gets shifted by one element by the
       process, thus now
       `[visibleNodeDataOffsets[i], visibleNodeDataOffsets[i + 1])` is a range
       in which the `visibleNodeDataIds` array contains a list of data handles
       for visible node with ID `i`. The last array element is now containing
       the end offset. */
    for(std::size_t i = 0; i != dataNodes.size(); ++i) {
        const NodeHandle node = dataNodes[i];
        if(node == NodeHandle::Null)
            continue;
        const UnsignedInt id = nodeHandleId(node);
        if(visibleNodeMask[id])
            visibleNodeDataIds[visibleNodeDataOffsets[id + 1]++] = i;
    }

    /* Now populate the "to update" and "to draw" arrays. The "to update"
       arrays contain a list of data IDs and corresponding node IDs for each
       layer, the "to draw" arrays then are ranges into these. The draws need
       to be first ordered by top-level node ID for correct back-to-front
       ordering, and then for each top-level node a draw for each layer again
       in a back-to-front order is issued.

       First go through each visible top-level node... */
    UnsignedInt drawOffset = 0;
    for(UnsignedInt visibleTopLevelNodeIndex = 0; visibleTopLevelNodeIndex != visibleNodeChildrenCounts.size(); visibleTopLevelNodeIndex += visibleNodeChildrenCounts[visibleTopLevelNodeIndex] + 1) {
        /* Remember how much data was drawn for the previous node so we can
           figure out the range to draw for this one... */
        const UnsignedInt previousOffset = offset;

        /* Go through all (direct and nested) children of the top-level node
           and then all data of each, and copy their IDs to the output range */
        for(UnsignedInt i = 0, iMax = visibleNodeChildrenCounts[visibleTopLevelNodeIndex] + 1; i != iMax; ++i) {
            const UnsignedInt visibleNodeId = visibleNodeIds[visibleTopLevelNodeIndex + i];

            for(UnsignedInt j = visibleNodeDataOffsets[visibleNodeId], jMax = visibleNodeDataOffsets[visibleNodeId + 1]; j != jMax; ++j) {
                dataToUpdateIds[offset] = visibleNodeDataIds[j];
                ++offset;
            }
        }

        /* If this layer is a drawing layer and there's any data to be drawn
           added by the above loop, save a range to the `dataToUpdateIds` and
           `dataToUpdateNodeIds` arrays. If there's no data to be drawn, put
           zeros there. */
        if(layerFeatures >= LayerFeature::Draw) {
            if(const UnsignedInt size = offset - previousOffset) {
                dataToDrawOffsets[drawOffset] = previousOffset;
                dataToDrawSizes[drawOffset] = offset - previousOffset;
            } else {
                dataToDrawOffsets[drawOffset] = 0;
                dataToDrawSizes[drawOffset] = 0;
            }
            ++drawOffset;
        }
    }

    return offset;
}

/* The `dataNodes` array is expected to be the same as passed into
   `orderVisibleNodeDataInto()`. The `dataGenerations` array is then matching
   generations and together with `layer` is used to form actual data handles in
   the output.

   The `visibleNodeEventDataOffsets` is expected to be the output of
   `orderVisibleNodeDataInto()` above with an additional first zero element,
   turned into an offset array. The process of calling this function for all
   event layers shifts the array by one element, with
   `visibleNodeEventDataOffsets[i]` to `visibleNodeEventDataOffsets[i + 1]`
   then being the range of data in `visibleNodeEventData` corresponding to node
   `i`. */
void orderNodeDataForEventHandlingInto(const LayerHandle layer, const Containers::StridedArrayView1D<const UnsignedShort>& dataGenerations, const Containers::StridedArrayView1D<const NodeHandle>& dataNodes, const Containers::ArrayView<UnsignedInt> visibleNodeEventDataOffsets, const Containers::BitArrayView visibleNodeMask, const Containers::ArrayView<DataHandle> visibleNodeEventData) {
    CORRADE_INTERNAL_ASSERT(
        dataNodes.size() == dataGenerations.size() &&
        visibleNodeEventDataOffsets.size() == visibleNodeMask.size() + 1);

    /* Go through the data list in reverse, convert that to data handle ranges.
       The `visibleNodeEventDataOffsets` array gets shifted by one element by
       the process, thus now
       `[visibleNodeEventDataOffsets[i], visibleNodeEventDataOffsets[i + 1])`
       is a range in which the `visibleNodeDataIds` array contains a list of
       data handles for visible node with ID `i`. The last array element is now
       containing the end offset. */
    for(std::size_t i = dataNodes.size(); i != 0; --i) {
        const NodeHandle node = dataNodes[i - 1];
        if(node == NodeHandle::Null)
            continue;
        const UnsignedInt id = nodeHandleId(node);
        if(visibleNodeMask[id])
            visibleNodeEventData[visibleNodeEventDataOffsets[id + 1]++] = dataHandle(layer, i - 1, dataGenerations[i - 1]);
    }
}

/* Reduces the three arrays by throwing away items where size is 0. Returns the
   resulting size. */
UnsignedInt compactDrawsInPlace(const Containers::StridedArrayView1D<UnsignedByte>& dataToDrawLayerIds, const Containers::StridedArrayView1D<UnsignedInt>& dataToDrawOffsets, const Containers::StridedArrayView1D<UnsignedInt>& dataToDrawSizes) {
    CORRADE_INTERNAL_ASSERT(
        dataToDrawOffsets.size() == dataToDrawLayerIds.size() &&
        dataToDrawSizes.size() == dataToDrawLayerIds.size());

    std::size_t offset = 0;
    for(std::size_t i = 0, iMax = dataToDrawLayerIds.size(); i != iMax; ++i) {
        if(!dataToDrawSizes[i])
            continue;

        /* Don't copy to itself */
        if(i != offset) {
            dataToDrawLayerIds[offset] = dataToDrawLayerIds[i];
            dataToDrawOffsets[offset] = dataToDrawOffsets[i];
            dataToDrawSizes[offset] = dataToDrawSizes[i];
        }

        ++offset;
    }

    /** @todo optimization step where draws of the same layer following each
        other are merged into one (with the assumption that the draw order is
        kept), that allows imgui-level efficiency where the whole UI with all
        widgets can be drawn in a single call, assuming most of the content
        (text, backgrounds, ...) is implemented in a single layer

        this would require that UserInterface populates the to-update data in
        the draw order, not in the layer ID order */

    /** @todo top-level nodes that have mutually disjoint bounding rect for all
        (clipped) subnodes can be also be drawn together without worrying about
        incorrect draw order -- however it needs some algorithm that is better
        than O(n^2) in finding mutually disjoint sets, plus also things like
        two top-level nodes being disjoint but between them is ordered another
        that overlaps with both

        so pick just nodes that are disjoint in a sequence, and stop when
        something overlaps? that's still O(n^2) though, every new node
        considered has to be checked with all previous */

    return offset;
}

}}}}

#endif
