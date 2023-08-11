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
void cullVisibleNodesInto(const Containers::StridedArrayView1D<const Vector2>& absoluteNodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::ArrayView<Containers::Triple<Vector2, Vector2, UnsignedInt>> clipStack, const Containers::StridedArrayView1D<const UnsignedInt>& visibleNodeIds, const Containers::StridedArrayView1D<const UnsignedInt>& visibleNodeChildrenCounts, const Containers::MutableBitArrayView visibleNodeMask) {
    CORRADE_INTERNAL_ASSERT(
        nodeSizes.size() == absoluteNodeOffsets.size() &&
        clipStack.size() == visibleNodeIds.size() &&
        visibleNodeChildrenCounts.size() == visibleNodeIds.size() &&
        visibleNodeMask.size() == absoluteNodeOffsets.size());

    /* Clear the visibility mask, individual bits will be set only if they're
       visible */
    visibleNodeMask.resetAll();

    /* Filter the visible node list and keep only nodes that are at least
       partially visible in the intersection of all parent clip rects */
    std::size_t i = 0;
    std::size_t depth = 0;
    while(i != visibleNodeIds.size()) {
        const UnsignedInt nodeId = visibleNodeIds[i];

        /* Calculate node clip rect min and max */
        const Vector2 min = absoluteNodeOffsets[nodeId];
        const Vector2 max = min + nodeSizes[nodeId];

        bool visible;
        /* If the rect has an empty area, skip it altogether */
        if(Math::equal(min, max).any()) {
            visible = false;

        /* If we're at a top-level node, save its clip rect for use by
           children */
        } else if(depth == 0) {
            clipStack[depth].first() = min;
            clipStack[depth].second() = max;
            visible = true;

        /* Otherwise intersect with the parent clip rect */
        } else {
            const Vector2 parentMin = clipStack[depth - 1].first();
            const Vector2 parentMax = clipStack[depth - 1].second();

            /* If the clip rects are completely disjoint, there's no point in
               continuing for any children. Logic follows Math::intersects()
               for Range. */
            /** @todo can't test & intersection calculation be done as a single
                operation? */
            visible = (parentMax > min).all() &&
                      (parentMin < max).all();

            /* If they intersect, calculate the clip rect intersection. Logic
               follows Math::intersect() for Range. */
            if(visible) {
                clipStack[depth].first() = Math::max(parentMin, min);
                clipStack[depth].second() = Math::min(parentMax, max);
            }
        }

        /* If the node is visible */
        if(visible) {
            /* Remember offset after all children of its node so we know when
               to pop this clip rect off the stack */
            clipStack[depth].third() = i + visibleNodeChildrenCounts[i] + 1;
            visibleNodeMask.set(nodeId);
            ++depth;

            /* Continue to the next entry */
            ++i;

        /* Otherwise there's no point in testing any children either */
        } else i += visibleNodeChildrenCounts[i] + 1;

        /* Pop the clip stack items for which all children were processed */
        while(depth && clipStack[depth - 1].third() == i)
            --depth;
    }
}

/* The `visibleNodeDataOffsets` and `visibleNodeData` arrays get filled with
   data handles for visible nodes, with `visibleNodeDataOffsets[i]` to
   `visibleNodeDataOffsets[i + 1]` being the range of data in
   `visibleNodeData` corresponding to visible node at index `i`.

   The `dataToUpdateLayerOffsets`, `dataToUpdateIds` and `dataToUpdateNodeIds`
   arrays get filled with data and node IDs in the desired draw order,
   clustered by layer ID, with `dataToUpdateLayerOffsets[i]` to
   `dataToUpdateLayerOffsets[i + 1]` being the range of data in
   `dataToUpdateIds` and `dataToUpdateNodeIds` corresponding to layer `i`.

   The `dataToDrawOffsets[j]` and `dataToDrawSizes[j]` is then a range in
   `dataToUpdateIds` and `dataToUpdateNodeIds` that should be drawn with layer
   `dataToDrawLayerIds[j]`, with their total count being the return value of
   this function.

   The `previousDataToUpdateLayerOffsets` array is temporary storage. The
   `visibleNodeDataOffsets` and `dataToUpdateLayerOffsets` arrays are expected
   to be zero-initialized. */
UnsignedInt orderVisibleNodeDataInto(const Containers::StridedArrayView1D<const UnsignedInt>& visibleNodeIds, const Containers::StridedArrayView1D<const UnsignedInt>& visibleNodeChildrenCounts, const Containers::StridedArrayView1D<const NodeHandle>& dataNodes, const Containers::StridedArrayView1D<const DataHandle>& data, const Containers::StridedArrayView1D<const LayerHandle>& layersNext, const LayerHandle firstLayer, const Containers::StridedArrayView1D<const LayerFeatures>& layerFeatures, const Containers::BitArrayView visibleNodeMask, const Containers::ArrayView<UnsignedInt> visibleNodeDataOffsets, const Containers::ArrayView<DataHandle> visibleNodeData, const Containers::ArrayView<UnsignedInt> dataToUpdateLayerOffsets, const Containers::ArrayView<UnsignedInt> previousDataToUpdateLayerOffsets, const Containers::StridedArrayView1D<UnsignedInt>& dataToUpdateIds, const Containers::StridedArrayView1D<UnsignedInt>& dataToUpdateNodeIds, const Containers::StridedArrayView1D<UnsignedByte>& dataToDrawLayerIds, const Containers::StridedArrayView1D<UnsignedInt>& dataToDrawOffsets, const Containers::StridedArrayView1D<UnsignedInt>& dataToDrawSizes) {
    CORRADE_INTERNAL_ASSERT(
        visibleNodeChildrenCounts.size() == visibleNodeIds.size() &&
        data.size() == dataNodes.size() &&
        visibleNodeDataOffsets.size() == visibleNodeMask.size() + 1 &&
        visibleNodeData.size() == dataNodes.size() &&
        dataToUpdateLayerOffsets.size() == layersNext.size() + 1 &&
        previousDataToUpdateLayerOffsets.size() == layersNext.size() + 1 &&
        dataToUpdateIds.size() == dataNodes.size() &&
        dataToUpdateNodeIds.size() == dataNodes.size() &&
        dataToDrawLayerIds.size() == dataNodes.size() &&
        dataToDrawOffsets.size() == dataNodes.size() &&
        dataToDrawSizes.size() == dataNodes.size());

    /* There are no valid layers, which means there's also no data to draw */
    if(firstLayer == LayerHandle::Null) {
        CORRADE_INTERNAL_ASSERT(data.isEmpty());
        /* There can however be a non-zero count of invalid layers (i.e.,
           present before but then removed), so be sure to set all running
           offsets to 0 */
        for(UnsignedInt& i: dataToUpdateLayerOffsets)
            i = 0;
        return 0;
    }

    /* Count how much data belongs to each visible node, skipping the first
       element ...*/
    for(const NodeHandle node: dataNodes) {
        const UnsignedInt id = nodeHandleId(node);
        if(visibleNodeMask[id])
            ++visibleNodeDataOffsets[id + 1];
    }

    /* ... then convert the counts to a running offset. Now
       `[visibleNodeDataOffsets[i + 1], visibleNodeDataOffsets[i + 2])` is a
       range in which the `visibleNodeData` array contains a list of data
       handles for visible node with ID `i`. The last element (containing the
       end offset) is omitted at this step. */
    UnsignedInt visibleNodeDataCount = 0;
    for(UnsignedInt& i: visibleNodeDataOffsets) {
        const UnsignedInt nextOffset = visibleNodeDataCount + i;
        i = visibleNodeDataCount;
        visibleNodeDataCount = nextOffset;
    }

    /* Go through the data list again, convert that to data handle ranges. The
       `visibleNodeDataOffsets` array gets shifted by one element by the
       process, thus now
       `[visibleNodeDataOffsets[i], visibleNodeDataOffsets[i + 1])` is a range
       in which the `visibleNodeData` array contains a list of data handles for
       visible node with ID `i`. The last array element is now containing the
       end offset. */
    for(std::size_t i = 0; i != data.size(); ++i) {
        const DataHandle handle = data[i];
        const NodeHandle node = dataNodes[i];
        const UnsignedInt id = nodeHandleId(node);
        if(visibleNodeMask[id])
            visibleNodeData[visibleNodeDataOffsets[id + 1]++] = handle;
    }

    /* Count how much data there is for each layer, skipping the first element
       ... */
    for(const DataHandle handle: visibleNodeData.prefix(visibleNodeDataCount))
        ++dataToUpdateLayerOffsets[dataHandleLayerId(handle) + 1];

    /* ... then convert the counts to a running offset. Now
       `[dataToUpdateLayerOffsets[i + 1], dataToUpdateLayerOffsets[i + 2])` is
       a range in which the `dataToUpdateIds` and `dataToUpdateNodeIds` arrays
       contains a list of data and node IDs for layer `i`. The last element
       (containing the end offset) is omitted at this step. */
    {
        UnsignedInt offset = 0;
        for(UnsignedInt& i: dataToUpdateLayerOffsets) {
            const UnsignedInt nextOffset = offset + i;
            i = offset;
            offset = nextOffset;
        }
        CORRADE_INTERNAL_ASSERT(offset == visibleNodeDataCount);
    }

    /* Now populate the "to update" and "to draw" arrays. The "to update"
       arrays contain a list of data IDs and corresponding node IDs for each
       layer, the "to draw" arrays then are ranges into these. The draws need
       to be first ordered by top-level node ID for correct back-to-front
       ordering, and then for each top-level node a draw for each layer again
       in a back-to-front order is issued.

       First go through each visible top-level node... */
    UnsignedInt drawCount = 0;
    for(UnsignedInt visibleTopLevelNodeIndex = 0; visibleTopLevelNodeIndex != visibleNodeChildrenCounts.size(); visibleTopLevelNodeIndex += visibleNodeChildrenCounts[visibleTopLevelNodeIndex] + 1) {
        /* Remember how much data was drawn for the previous node so we can
           figure out the per-layer ranges to draw for this one... */
        Utility::copy(dataToUpdateLayerOffsets, previousDataToUpdateLayerOffsets);

        /* Go through all (direct and nested) children of the top-level node
           and then all data of each, and copy their IDs to the per-layer
           ranges. The `dataToUpdateLayerOffsets` array gets shifted by one
           element by the process, thus now
           `[dataToUpdateLayerOffsets[i], dataToUpdateLayerOffsets[i + 1])` is
           a range in which the `visibleLayerNodeData` array contains a list of
           data handles for layer `i`, mapping back to node IDs is done from
           the second pair element. */
        for(UnsignedInt i = 0, iMax = visibleNodeChildrenCounts[visibleTopLevelNodeIndex] + 1; i != iMax; ++i) {
            const UnsignedInt visibleNodeId = visibleNodeIds[visibleTopLevelNodeIndex + i];

            for(UnsignedInt j = visibleNodeDataOffsets[visibleNodeId], jMax = visibleNodeDataOffsets[visibleNodeId + 1]; j != jMax; ++j) {
                const DataHandle handle = visibleNodeData[j];
                const UnsignedInt offset = dataToUpdateLayerOffsets[dataHandleLayerId(handle) + 1]++;
                dataToUpdateIds[offset] = dataHandleId(handle);
                dataToUpdateNodeIds[offset] = visibleNodeId;
            }
        }

        /* Go through the layer draw order. It's cyclic, so stop when reaching
           the first layer again. If there's any data to be drawn by this layer
           added by the above loop, add a draw, which is a range to the
           `dataToUpdateIds` and `dataToUpdateNodeIds` arrays. */
        LayerHandle layer = firstLayer;
        do {
            const UnsignedInt layerId = layerHandleId(layer);
            if(layerFeatures[layerId] >= LayerFeature::Draw) {
                if(const UnsignedInt size = dataToUpdateLayerOffsets[layerId + 1] - previousDataToUpdateLayerOffsets[layerId + 1]) {
                    dataToDrawLayerIds[drawCount] = layerId;
                    dataToDrawOffsets[drawCount] = previousDataToUpdateLayerOffsets[layerId + 1];
                    dataToDrawSizes[drawCount] = size;
                    ++drawCount;
                }
            }

            layer = layersNext[layerHandleId(layer)];
        } while(layer != firstLayer);
    }

    /** @todo optimization step where draws of the same layer following each
        other are merged into one (with the assumption that the draw order is
        kept), that allows imgui-level efficiency where the whole UI with all
        widgets can be drawn in a single call, assuming most of the content
        (text, backgrounds, ...) is implemented in a single layer */

    return drawCount;
}

/* The `visibleNodeEventDataOffsets` and `visibleNodeEventData` arrays get
   filled with a subset of `visibleNodeDataOffsets` and `visibleNodeData` with
   only layers that have LayerFeature::Event, and in a front-to-back order. */
void orderNodeDataForEventHandling(const Containers::ArrayView<const UnsignedInt> visibleNodeDataOffsets, const Containers::ArrayView<const DataHandle> visibleNodeData, const Containers::StridedArrayView1D<const LayerHandle>& layersPrevious, const LayerHandle lastLayer, const Containers::StridedArrayView1D<const LayerFeatures>& layerFeatures, const Containers::ArrayView<UnsignedInt> visibleNodeEventDataOffsets, const Containers::ArrayView<DataHandle> visibleNodeEventData) {
    CORRADE_INTERNAL_ASSERT(
        layerFeatures.size() == layersPrevious.size() &&
        !visibleNodeEventDataOffsets.isEmpty() &&
        visibleNodeEventDataOffsets.size() == visibleNodeDataOffsets.size());

    /* There are no layers, which means there's also no data to draw */
    if(lastLayer == LayerHandle::Null) {
        /* There can however be a non-zero count of nodes (though with no data
           attached), so be sure to set all running offsets to 0 */
        for(UnsignedInt& i: visibleNodeEventDataOffsets)
            i = 0;
        return;
    }

    /* Build a layer ID -> layer order mapping. Elements for which there aren't
       any layer IDs are left uninitialized. Layers that don't support
       LayerFeature::Event are not handled in any special way here, as data
       corresponding to those have to be handled below in any case. */
    UnsignedByte order[1 << Implementation::LayerHandleIdBits];
    UnsignedInt layerCount = 0;
    {
        LayerHandle layer = lastLayer;
        do {
            const UnsignedInt layerId = layerHandleId(layer);
            order[layerId] = layerCount++;
            layer = layersPrevious[layerId];
        } while(layer != lastLayer);
    }

    /* Go through data for all visible nodes */
    UnsignedInt offset = 0;
    visibleNodeEventDataOffsets[0] = 0;
    for(std::size_t i = 0; i != visibleNodeDataOffsets.size() - 1; ++i) {
        /* First calculate how much data there is for each layer order index,
           skipping data in layers without LayerFeature::Event. The array is
           sized for the max layer count but only `layerCount + 1` elements get
           filled.  */
        UnsignedInt dataOffsets[(1 << Implementation::LayerHandleIdBits) + 1]{};
        for(std::size_t j = visibleNodeDataOffsets[i]; j != visibleNodeDataOffsets[i + 1]; ++j) {
            const UnsignedInt layerId = dataHandleLayerId(visibleNodeData[j]);
            /* Might possbily be faster to do this just once for every layer
               than for every data, but as we're going node-by-node here that's
               not really possible I think. And then what in the second loop
               below? */
            if(!(layerFeatures[layerId] >= LayerFeature::Event))
                continue;
            ++dataOffsets[order[layerId] + 1];
        }

        /* Then convert the first `layerCount + 1` counts to a running offset.
           Now `[dataOffsets[k + 1], dataOffsets[k + 2])` is a range in which
           the `visibleNodeEventData` array contains a list of data for node
           `i` and layer `k`. The last element (containing the end offset) is
           omitted at this step. */
        for(UnsignedInt& i: Containers::arrayView(dataOffsets).prefix(layerCount + 1)) {
            const UnsignedInt nextOffset = offset + i;
            i = offset;
            offset = nextOffset;
        }

        /* Go through the data list again, convert that to per-layer ranges.
           The `dataOffsets` array gets shifted by one element by this, so now
           `[dataOffsets[k], dataOffsets[k + 1])` is a range in which the
           `visibleNodeEventData` array below contains a list of data for node
           `i` and layer `k`. We don't need the per-layer offsets for anything
           though, only the end offset for the whole node, which is saved to
           the `visibleNodeEventDataOffsets` array.

           Populate in reverse order so we're consistent (data get drawn in the
           order they were added, first being at the bottom, so the events
           should get processed in the other direction). */
        for(std::size_t j = visibleNodeDataOffsets[i + 1], jMin = visibleNodeDataOffsets[i]; j != jMin; --j) {
            const DataHandle data = visibleNodeData[j - 1];
            const UnsignedInt layerId = dataHandleLayerId(data);
            if(!(layerFeatures[layerId] >= LayerFeature::Event))
                continue;
            visibleNodeEventData[dataOffsets[order[layerId] + 1]++] = data;
        }
        visibleNodeEventDataOffsets[i + 1] = offset;
    }
}

}}}}

#endif
