#ifndef Magnum_Ui_Implementation_abstractUserInterface_h
#define Magnum_Ui_Implementation_abstractUserInterface_h
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023, 2024, 2025
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

#include <cstring> /* std::memset() */
#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/Reference.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/Triple.h>
#include <Corrade/Utility/Algorithms.h>
#include <Magnum/Magnum.h>
#include <Magnum/Math/Functions.h>

#include "Magnum/Ui/AbstractAnimator.h" /* AnimatorFeatures */
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/NodeFlags.h"

/* Contains algorithms used internally in AbstractUserInterface.cpp. Extracted
   here for easier testing and ability to iterate on them in isolation without
   having to make the library compile as well. */

namespace Magnum { namespace Ui { namespace Implementation { namespace {

/* The `visibleNodeIds` and `visibleNodeChildrenCounts` arrays get filled with
   visible node IDs and the count of their children in the following order,
   with the returned value being the size of the prefix filled:

    -   children IDs are always right after their parent in the
        `visibleNodeIds` array in a depth-first order, with the count stored in
        the corresponding item of the `visibleNodeChildrenCounts` array

   The `visibleNodes`, `childrenOffsets`, `children` and `parentsToProcess`
   arrays are temporary storage. The `visibleNodes` and `childrenOffsets`
   arrays have to be zero-initialized. Other outputs don't need to be. */
std::size_t orderVisibleNodesDepthFirstInto(const Containers::StridedArrayView1D<const NodeHandle>& nodeParents, const Containers::StridedArrayView1D<const UnsignedInt>& nodeOrder, const Containers::StridedArrayView1D<const NodeFlags>& nodeFlags, const Containers::StridedArrayView1D<const NodeHandle>& nodeOrderNext, const NodeHandle firstNodeOrder, const Containers::MutableBitArrayView visibleNodes, const Containers::ArrayView<UnsignedInt> childrenOffsets, const Containers::ArrayView<UnsignedInt> children, const Containers::ArrayView<Containers::Triple<UnsignedInt, UnsignedInt, UnsignedInt>> parentsToProcess, const Containers::StridedArrayView1D<UnsignedInt>& visibleNodeIds, const Containers::StridedArrayView1D<UnsignedInt>& visibleNodeChildrenCounts) {
    CORRADE_INTERNAL_ASSERT(
        nodeOrder.size() == nodeParents.size() &&
        nodeFlags.size() == nodeParents.size() &&
        visibleNodes.size() == nodeParents.size() &&
        childrenOffsets.size() == nodeParents.size() + 1 &&
        children.size() == nodeParents.size() &&
        /* It only reaches nodeParents.size() if the hierarchy is a single
           branch, usually it's shorter. */
        parentsToProcess.size() == nodeParents.size() &&
        visibleNodeIds.size() == nodeParents.size() &&
        visibleNodeChildrenCounts.size() == nodeParents.size());

    /* If there are no top-level nodes, nothing is visible and thus nothing to
       do */
    if(firstNodeOrder == NodeHandle::Null)
        return 0;

    /* Children offset for each node excluding root and top-level nodes. Handle
       generation is ignored here, so invalid (free) nodes are counted as well.
       In order to avoid orphaned subtrees and cycles, the nodes are expected
       to be made root when freed.

       First calculate the count of children for each, skipping the first
       element ... */
    for(std::size_t i = 0; i != nodeParents.size(); ++i) {
        const NodeHandle parent = nodeParents[i];
        if(parent == NodeHandle::Null || nodeOrder[i] != ~UnsignedInt{})
            continue;
        ++childrenOffsets[nodeHandleId(parent) + 1];
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

    /* Go through the node list excluding root and top-level nodes again,
       convert that to child ranges. The `childrenOffsets` array gets shifted
       by one element by this, so now `[childrenOffsets[i], childrenOffsets[i + 1])`
       is a range in which the `children` array below contains a list of
       children for node `i`. The last array element is now containing the end
       offset. */
    for(std::size_t i = 0; i != nodeParents.size(); ++i) {
        const NodeHandle parent = nodeParents[i];
        if(parent == NodeHandle::Null || nodeOrder[i] != ~UnsignedInt{})
            continue;
        children[childrenOffsets[nodeHandleId(parent) + 1]++] = i;
    }

    UnsignedInt outputOffset = 0;

    /* Go through the top-level node list. It's cyclic, so stop when reaching
       the first node again. */
    {
        NodeHandle topLevel = firstNodeOrder;
        do {
            /* Skip hidden top-level nodes and also nested top-level nodes that
               have any parent hidden. This relies on the nested top-level
               nodes being always ordered after their parents, otherwise the
               visibleNodes mask won't be updated for those yet. */
            const UnsignedInt topLevelId = nodeHandleId(topLevel);
            if(!(nodeFlags[topLevelId] & NodeFlag::Hidden) && (nodeParents[topLevelId] == NodeHandle::Null || visibleNodes[nodeHandleId(nodeParents[topLevelId])])) {
                /* Add the top-level node to the output, mark it as visible,
                   and to the list of parents to process next */
                std::size_t parentsToProcessOffset = 0;
                visibleNodeIds[outputOffset] = topLevelId;
                visibleNodes.set(topLevelId);
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

                    /* Unless the current child is hidden, add it to the
                       output, mark it as visible, and to the list of parents
                       to process next. Increment all offsets for the next
                       round. */
                    const UnsignedInt childId = children[childrenOffset];
                    if(!(nodeFlags[childId] & NodeFlag::Hidden)) {
                        visibleNodeIds[outputOffset] = childId;
                        visibleNodes.set(childId);
                        parentsToProcess[parentsToProcessOffset++] = {childId, outputOffset++, childrenOffsets[childId]};
                    }

                    ++childrenOffset;
                }
            }

            CORRADE_INTERNAL_DEBUG_ASSERT(nodeOrder[topLevelId] != ~UnsignedInt{});
            topLevel = nodeOrderNext[nodeOrder[topLevelId]];
        } while(topLevel != firstNodeOrder);
    }
    CORRADE_INTERNAL_ASSERT(outputOffset <= nodeParents.size());

    return outputOffset;
}

std::size_t visibleTopLevelNodeIndicesInto(const Containers::StridedArrayView1D<const UnsignedInt>& visibleNodeChildrenCounts, const Containers::StridedArrayView1D<UnsignedInt>& visibleTopLevelNodeIndices) {
    UnsignedInt offset = 0;
    for(UnsignedInt visibleTopLevelNodeIndex = 0; visibleTopLevelNodeIndex != visibleNodeChildrenCounts.size(); visibleTopLevelNodeIndex += visibleNodeChildrenCounts[visibleTopLevelNodeIndex] + 1)
        visibleTopLevelNodeIndices[offset++] = visibleTopLevelNodeIndex;

    return offset;
}

/* The `visibleNodeIds` and `visibleNodeChildrenCounts` are outputs of
   orderVisibleNodesDepthFirstInto() above. The `mask` bits get set to 1 for
   all nodes that have a particular NodeFlag set, or any of their parents has it
   set. To be used for NodeFlag::NoEvent, Disabled and NoBlur.

   Only ever resets bits, never sets -- assumes the mask is initially set to
   1s (for example for visible and not culled nodes), and the operation results
   in fewer 1s being set. */
template<NodeFlag flag> void propagateNodeFlagToChildrenInto(const Containers::StridedArrayView1D<const NodeFlags>& nodeFlags, const Containers::StridedArrayView1D<const UnsignedInt>& visibleNodeIds, const Containers::StridedArrayView1D<const UnsignedInt>& visibleNodeChildrenCounts, const Containers::MutableBitArrayView mask) {
    CORRADE_INTERNAL_ASSERT(
        visibleNodeChildrenCounts.size() == visibleNodeIds.size() &&
        mask.size() == nodeFlags.size());

    /* The visible node IDs are ordered such that all children of a particular
       node are right behind it. Thus, in order to mark a node including all
       its children, we simply iterate the node IDs for all children and set
       corresponding bits, and then only continue after all children. That also
       means we don't redundantly check for the flag in nodes that are already
       marked transitively. */
    for(std::size_t i = 0; i != visibleNodeIds.size(); ++i) {
        if(nodeFlags[visibleNodeIds[i]] >= flag) {
            const std::size_t childrenCount = visibleNodeChildrenCounts[i];
            for(std::size_t j = i, jMax = i + 1 + childrenCount; j != jMax; ++j)
                mask.reset(visibleNodeIds[j]);
            i += childrenCount;
            continue;
        }
    }
}

/* The `visibleNodeIds` is the output of orderVisibleNodesDepthFirstInto()
   above. The `nodeLayouts` is meant to be a 2D array, where the first
   dimension is all node IDs and the second dimension is all layouters ordered
   by the layouter order, with items filled if given layouter has a layout for
   given node and null otherwise.

   The `topLevelLayoutIds` array gets filled with a subset of handle IDs
   from `nodeLayouts` which are considered top-level, i.e. nodes that act as
   roots from which layout is calculated. They're ordered by dependency, i.e.
   if a top-level layout node has its size calculated by another layout, it
   ensures that it's ordered after the layout it depends on. Prefix of the
   `topLevelLayoutOffsets` array, with prefix size being the second function
   return value, is filled with a running offset into the `topLevelLayoutIds`
   array, with `[topLevelLayoutOffsets[i], topLevelLayoutOffsets[i + 1])` being
   the range of IDs to submit to AbstractLayouter::layout() of a layouter
   `topLevelLayoutLayouterIds[i]`.

   The `nodeLayoutLevels`, `layoutLevelOffsets`, `topLevelLayouts`,
   `topLevelLayoutLevels` and `levelPartitionedTopLevelLayouts` arrays are
   temporary storage, the `nodeLayoutLevels` and `layoutLevelOffsets` arrays
   are expected to be zero-initialized. The first return value is meant to be
   subsequently used for sizing inputs to `fillLayoutUpdateMasksInto()`. */
Containers::Pair<UnsignedInt, std::size_t> discoverTopLevelLayoutNodesInto(const Containers::StridedArrayView1D<const NodeHandle>& nodeParents, const Containers::StridedArrayView1D<const UnsignedInt>& visibleNodeIds, const UnsignedInt layouterCount, const Containers::StridedArrayView2D<const LayoutHandle>& nodeLayouts, const Containers::StridedArrayView2D<UnsignedInt>& nodeLayoutLevels, const Containers::ArrayView<UnsignedInt> layoutLevelOffsets, const Containers::StridedArrayView1D<LayoutHandle>& topLevelLayouts, const Containers::StridedArrayView1D<UnsignedInt>& topLevelLayoutLevels, const Containers::StridedArrayView1D<LayoutHandle>& levelPartitionedTopLevelLayouts, const Containers::StridedArrayView1D<UnsignedInt>& topLevelLayoutOffsets, const Containers::StridedArrayView1D<UnsignedByte>& topLevelLayoutLayouterIds, const Containers::StridedArrayView1D<UnsignedInt>& topLevelLayoutIds) {
    CORRADE_INTERNAL_ASSERT(
        nodeLayouts.size()[0] == nodeParents.size() &&
        nodeLayouts.isContiguous<1>() &&
        nodeLayoutLevels.size() == nodeLayouts.size() &&
        nodeLayoutLevels.isContiguous<1>() &&
        /* Size of the topLevelLayouts array should be basically all data from
           all layouters, which unfortunately cannot be easily verified
           here. Then, worst case scenario is that each layout gets its own
           level, for example when they're all chained together, so the
           temporaries need to be the same size as the count of all top-level
           layout nodes. The running offset then needs one more element. */
        layoutLevelOffsets.size() == topLevelLayouts.size() + 1 &&
        topLevelLayoutLevels.size() == topLevelLayouts.size() &&
        levelPartitionedTopLevelLayouts.size() == topLevelLayouts.size() &&
        /* Worst case scenario is that each top-level layout is from a
           different layouter or from a different level, so the offsets have to
           have the same size as the count of all top-level layouts. The
           running offset then needs one more element. */
        topLevelLayoutOffsets.size() == topLevelLayouts.size() + 1 &&
        topLevelLayoutLayouterIds.size() == topLevelLayouts.size() &&
        topLevelLayoutIds.size() == topLevelLayouts.size());

    std::size_t topLevelLayoutIndex = 0;
    UnsignedInt maxLevel = 0;

    /* 1. Go through all layouts assigned to all nodes and collect top-level
       layouts, i.e. layouts which act as roots for a layout calculation.

       A layout is a top-level layout if it's assigned to a root node or its
       parent node doesn't have a layout from the same layouter. To ensure
       they're correctly ordered, a level index is calculated for each, where
       layouts with a higher index get always calculated after layouts with a
       lower index. */
    for(const UnsignedInt nodeId: visibleNodeIds) {
        CORRADE_INTERNAL_DEBUG_ASSERT(nodeId < nodeParents.size());

        const Containers::ArrayView<const LayoutHandle> layouts = nodeLayouts[nodeId].asContiguous();
        const Containers::ArrayView<UnsignedInt> layoutLevels = nodeLayoutLevels[nodeId].asContiguous();
        UnsignedInt nextFreeLevel = 0;

        /* Layout assigned to a root node is always a top-level layout. The
           first layout (first in the layout order, as supplied in the 2D
           nodeLayouts array passed to this function) assigned to a root node
           gets level 0, each subsequent layout assigned to the same node gets
           a higher level. */
        if(nodeParents[nodeId] == NodeHandle::Null) {
            for(std::size_t i = 0; i != layouts.size(); ++i) {
                if(layouts[i] != LayoutHandle::Null) {
                    /* The layoutLevels get the level + 1, 0 indicating the
                       layout (if non-null) isn't assigned to a visible node */
                    layoutLevels[i] = nextFreeLevel + 1;
                    topLevelLayouts[topLevelLayoutIndex] = layouts[i];
                    topLevelLayoutLevels[topLevelLayoutIndex] = nextFreeLevel;
                    ++nextFreeLevel;
                    ++topLevelLayoutIndex;
                }
            }

        /* Otherwise it might or might not be a top-level layout, and it gets a
           level depending on whether the parent node is assigned a layout from
           the same layouter or not */
        } else {
            const UnsignedInt parentNodeId = nodeHandleId(nodeParents[nodeId]);
            const Containers::ArrayView<const LayoutHandle> parentLayouts = nodeLayouts[parentNodeId].asContiguous();
            const Containers::ArrayView<const UnsignedInt> parentLayoutLevels = nodeLayoutLevels[parentNodeId].asContiguous();

            /* Go through all layouts for this node and inherit levels for
               layouts that have the same layouter in the parent node.
               The nextFreeLevel is used for potential other layouts that don't
               have the same layouter in the parent, and is higher than all
               inherited levels. */
            for(std::size_t i = 0; i != layouts.size(); ++i) {
                if(layouts[i] != LayoutHandle::Null &&
                   parentLayouts[i] != LayoutHandle::Null)
                {
                    /* The layoutLevels get the level + 1, subtract it back.
                       The next free level is then a level after. */
                    nextFreeLevel = Math::max(nextFreeLevel, parentLayoutLevels[i] - 1 + 1);
                    layoutLevels[i] = parentLayoutLevels[i];
                }
            }

            /* Go through the layouts again and assign next free levels to
               those that don't have the same layouter in the parent node.
               Those are then also treated as top-level layout nodes. */
            for(std::size_t i = 0; i != layouts.size(); ++i) {
                if(layouts[i] != LayoutHandle::Null &&
                   parentLayouts[i] == LayoutHandle::Null)
                {
                    /* The layoutLevels get the level + 1, 0 indicating the
                       layout (if non-null) isn't assigned to a visible node */
                    layoutLevels[i] = nextFreeLevel + 1;
                    topLevelLayouts[topLevelLayoutIndex] = layouts[i];
                    topLevelLayoutLevels[topLevelLayoutIndex] = nextFreeLevel;
                    ++nextFreeLevel;
                    ++topLevelLayoutIndex;
                }
            }
        }

        maxLevel = Math::max(maxLevel, nextFreeLevel);
    }

    CORRADE_INTERNAL_ASSERT(topLevelLayoutIndex <= topLevelLayouts.size());

    /* 2. Partition the top-level layout list by level. */
    CORRADE_INTERNAL_ASSERT(maxLevel <= layoutLevelOffsets.size());

    /* First calculate the count of layouts for each level, skipping the first
       element ... */
    for(const UnsignedInt level: topLevelLayoutLevels.prefix(topLevelLayoutIndex))
        ++layoutLevelOffsets[level + 1];

    /* ... then convert the counts to a running offset. Now
      `[layoutLevelOffsets[i + 1], layoutLevelOffsets[i + 2])` is a range in
      which the `levelPartitionedTopLevelLayouts` array will contain a list of
      layouts for level `i`. The last element (containing the end offset) is
      omitted at this step. */
    {
        UnsignedInt offset = 0;
        for(UnsignedInt& i: layoutLevelOffsets) {
            const UnsignedInt nextOffset = offset + i;
            i = offset;
            offset = nextOffset;
        }
        CORRADE_INTERNAL_ASSERT(offset == topLevelLayoutIndex);
    }

    /* Go through the (layout, level) list again, partition that to level
       ranges in a temporary storage. The `layoutLevelOffsets` array gets
       shifted by one element by the process, thus now
       `[layoutLevelOffsets[i], layoutLevelOffsets[i + 1])` is a range in which
       the `levelPartitionedTopLevelLayouts` array contains a list of layouts
       for level `i`. The last element is now containing the end offset.

       The temporary `topLevelLayoutLevels` array isn't needed for anything
       after this step, as the levels in `levelPartitionedTopLevelLayouts` are
       implicit from the `layoutLevelOffsets`. */
    for(std::size_t i = 0; i != topLevelLayoutIndex; ++i) {
        levelPartitionedTopLevelLayouts[layoutLevelOffsets[topLevelLayoutLevels[i] + 1]++] = topLevelLayouts[i];
    }

    /* 3. Partition each level by layouter and save the running offsets. */
    UnsignedInt offset = 0;
    topLevelLayoutOffsets[0] = 0;
    UnsignedInt outputTopLevelLayoutIndex = 1;
    for(UnsignedInt level = 0; level != maxLevel; ++level) {
        /* First calculate the count of layouts for each layouter, skipping the
           first element. The array is sized for the max layouter count but
           only `layouterCount + 1` elements get filled. Also only those get
           zero-initialized -- compared to a {} it makes a significant
           difference when there's just a few layouters but a ton of levels.

           Here it also doesn't need to take the layouter order into account,
           as top-level layout nodes within a single level don't depend on each
           other in any way, and thus the layouts for them can be calculated in
           an arbitrary order. */
        UnsignedInt layouterOffsets[(1 << Implementation::LayouterHandleIdBits) + 1];
        std::memset(layouterOffsets, 0, (layouterCount + 1)*sizeof(UnsignedInt));

        const std::size_t levelBegin = layoutLevelOffsets[level];
        const std::size_t levelEnd = layoutLevelOffsets[level + 1];
        for(std::size_t i = levelBegin; i != levelEnd; ++i) {
            const UnsignedInt layouterId = layoutHandleLayouterId(levelPartitionedTopLevelLayouts[i]);
            ++layouterOffsets[layouterId + 1];
        }

        /* ... then convert the first `layouterCount + 1` counts to a running
           offset. Now `[layouterOffsets[i + 1], layouterOffsets[i + 2])` is a
           range in which the `topLevelLayoutNodes` array contains a list of
           layouts for level `level` and layouter `i`. The last element
           (containing the end offset) is omitted at this step. */
        for(UnsignedInt& i: Containers::arrayView(layouterOffsets).prefix(layouterCount + 1)) {
            const UnsignedInt nextOffset = offset + i;
            i = offset;
            offset = nextOffset;
        }

        /* Go through the layout list again, convert that to per-layouter
           ranges. The `layouterOffsets` array gets shifted by one element by
           this, so now `[layouterOffsets[i], layouterOffsets[i + 1])` is a
           range in which the `topLevelLayoutNodes` array contains a list of
           layouts for level `level` and layouter `i`. The last element is now
           containing the end offset. */
        for(std::size_t i = levelBegin; i != levelEnd; ++i) {
            const LayoutHandle layout = levelPartitionedTopLevelLayouts[i];
            topLevelLayoutIds[layouterOffsets[layoutHandleLayouterId(layout) + 1]++] = layoutHandleId(layout);
        }

        /* Finally, take the non-empty layouter offsets and put them into the
           output array */
        for(UnsignedInt i = 0; i != layouterCount; ++i) {
            if(layouterOffsets[i] == layouterOffsets[i + 1])
                continue;

            topLevelLayoutOffsets[outputTopLevelLayoutIndex] = layouterOffsets[i + 1];
            topLevelLayoutLayouterIds[outputTopLevelLayoutIndex - 1] = i;
            ++outputTopLevelLayoutIndex;
        }
    }

    return {maxLevel, outputTopLevelLayoutIndex};
}

/* Assumes the `masks` size is a sum of layouter capacities for all entries in
   `topLevelLayoutLayouterIds`. For each entry in `topLevelLayoutLayouterIds`
   the `masks` will then contain a range corresponding to given layouter
   capacity, with bits being set for all layouts that are meant to be updated
   in given update() run.

   The `nodeLayouts`, `nodeLayoutLevels`, `layoutLevelOffsets`,
   `topLevelLayoutOffsets` and `topLevelLayoutLayouterIds` arrays are output of
   the `discoverTopLevelLayoutNodesInto()` call above. The
   `layouterLevelMaskOffset` array is temporary storage, the `masks` array is
   expected to be zero-initialized. */
void fillLayoutUpdateMasksInto(const Containers::StridedArrayView2D<const LayoutHandle>& nodeLayouts, const Containers::StridedArrayView2D<const UnsignedInt>& nodeLayoutLevels, const Containers::ArrayView<const UnsignedInt> layoutLevelOffsets, const Containers::StridedArrayView1D<const UnsignedInt>& topLevelLayoutOffsets, const Containers::StridedArrayView1D<const UnsignedByte>& topLevelLayoutLayouterIds, const Containers::ArrayView<const UnsignedInt> layouterCapacities, const Containers::StridedArrayView2D<std::size_t>& layouterLevelMaskOffsets, const Containers::MutableBitArrayView masks) {
    CORRADE_INTERNAL_ASSERT(
        nodeLayoutLevels.size() == nodeLayouts.size() &&
        /* Can't pin layoutLevelOffsets size to anything as
           discoverTopLevelLayoutNodesInto() conservatively expects it to be
           enough even if every layout would be its own level */
        topLevelLayoutOffsets.size() == topLevelLayoutLayouterIds.size() + 1 &&
        layouterLevelMaskOffsets.size()[1] == layouterCapacities.size());

    /* 1. Map each update() run to a range in the masks array, and create a
       mapping from the per-layouter level in nodeLayoutLevels to an offset
       in the masks array */
    UnsignedInt currentLevel = 0;
    std::size_t maskOffset = 0;
    for(std::size_t i = 0; i != topLevelLayoutOffsets.size() - 1; ++i) {
        /* Levels are associated with the content of topLevelLayoutIds coming
           from discoverTopLevelLayoutNodesInto() implicitly -- each update()
           run is fully contained within a range of particular level as it's
           partitioned from it by layouter ID. The per-layouter runs thus don't
           cross the level range boundaries. */
        if(topLevelLayoutOffsets[i] >= layoutLevelOffsets[currentLevel + 1]) {
            CORRADE_INTERNAL_DEBUG_ASSERT(topLevelLayoutOffsets[i] == layoutLevelOffsets[currentLevel + 1]);
            ++currentLevel;
        }

        const UnsignedInt layouterId = topLevelLayoutLayouterIds[i];
        layouterLevelMaskOffsets[{currentLevel, layouterId}] = maskOffset;
        maskOffset += layouterCapacities[layouterId];
    }

    CORRADE_INTERNAL_ASSERT(maskOffset == masks.size());

    /* 2. Set bits in the `masks` corresponding to items in nodeLayouts. */
    const std::size_t nodeCount = nodeLayouts.size()[0];
    const std::size_t layouterCount = nodeLayouts.size()[1];
    for(std::size_t node = 0; node != nodeCount; ++node) {
        for(std::size_t layouter = 0; layouter != layouterCount; ++layouter) {
            /* If the level is 0 it means that there's no layout assigned to
               given node from this layouter (thus nothing to set anywhere), or
               that the node isn't visible. Invisible nodes are not meant to be
               updated either, skip them. */
            const UnsignedInt level = nodeLayoutLevels[{node, layouter}];
            if(!level)
                continue;
            const LayoutHandle layout = nodeLayouts[{node, layouter}];
            const UnsignedInt layouterId = layoutHandleLayouterId(layout);
            masks.set(layouterLevelMaskOffsets[{level - 1, layouterId}] + layoutHandleId(layout));
        }
    }
}

/* The `visibleNodeMask` has bits set for nodes in `visibleNodeIds` that are
   at least partially visible in the parent clip rects, the `clipRects` is then
   a list of clip rects and count of nodes affected by them.

   The `clipStack` array is temporary storage. */
UnsignedInt cullVisibleNodesInto(const Vector2& uiOffset, const Vector2& uiSize, const Containers::StridedArrayView1D<const Vector2>& absoluteNodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::StridedArrayView1D<const NodeFlags>& nodeFlags, const Containers::ArrayView<Containers::Triple<Vector2, Vector2, UnsignedInt>> clipStack, const Containers::StridedArrayView1D<const UnsignedInt>& visibleNodeIds, const Containers::StridedArrayView1D<const UnsignedInt>& visibleNodeChildrenCounts, const Containers::MutableBitArrayView visibleNodeMask, const Containers::StridedArrayView1D<Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<Vector2>& clipRectSizes, const Containers::StridedArrayView1D<UnsignedInt>& clipRectNodeCounts) {
    CORRADE_INTERNAL_ASSERT(
        nodeSizes.size() == absoluteNodeOffsets.size() &&
        nodeFlags.size() == absoluteNodeOffsets.size() &&
        /* One more item for the actual UI offset + size */
        clipStack.size() == visibleNodeIds.size() + 1 &&
        visibleNodeChildrenCounts.size() == visibleNodeIds.size() &&
        visibleNodeMask.size() == absoluteNodeOffsets.size() &&
        clipRectSizes.size() == clipRectOffsets.size() &&
        clipRectNodeCounts.size() == clipRectOffsets.size());

    /* Clear the visibility mask, individual bits will be set only if they're
       visible */
    visibleNodeMask.resetAll();

    /* If there's no visible nodes to go through, bail. Otherwise it'd attempt
       to access out-of-bounds visibleNodeChildrenCounts etc below. */
    if(visibleNodeIds.isEmpty())
        return 0;

    /* The initial item on the clip stack is the UI clip rect. That one is
       used for culling away nodes out of the window / screen / ... area, but
       isn't included in the output list of clip rects -- instead, nodes
       clipped only by the UI are marked with a clip rect that has a zero
       offset and size. */
    std::size_t clipStackDepth = 1;
    clipStack[0] = {uiOffset, uiOffset + uiSize, UnsignedInt(visibleNodeIds.size())};

    /* The initial clip rect is with zero offset and size, indicating that
       nothing is clipped (apart from the implicit window / screen / ...
       clipping) */
    clipRectOffsets[0] = {};
    clipRectSizes[0] = {};
    clipRectNodeCounts[0] = 0;

    /* Filter the visible node list and keep only nodes that are at least
       partially visible in the intersection of all parent clip rects */
    std::size_t i = 0;
    std::size_t clipRectsOffset = 0;
    std::size_t topLevelNodeEnd = visibleNodeChildrenCounts[0] + 1;
    while(i != visibleNodeIds.size()) {
        const UnsignedInt nodeId = visibleNodeIds[i];

        /* Calculate node clip rect min and max */
        const Vector2 size = nodeSizes[nodeId];
        const Vector2 min = absoluteNodeOffsets[nodeId];
        const Vector2 max = min + size;

        /* There's always at least the UI clip rect in the clip stack, which we
           can check against */
        CORRADE_INTERNAL_DEBUG_ASSERT(clipStackDepth != 0);
        const Vector2 parentMin = clipStack[clipStackDepth - 1].first();
        const Vector2 parentMax = clipStack[clipStackDepth - 1].second();

        /* The node is visible if the clip rects overlap at least a bit. Logic
           follows Math::intersects() for Range. */
        /** @todo can't test & intersection calculation be done as a single
            operation? */
        bool visible = (parentMax > min).all() &&
                       (parentMin < max).all();

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
                /* Calculate the clip rect intersection. Logic follows
                   Math::intersect() for Range. */
                clipStack[clipStackDepth].first() = Math::max(parentMin, min);
                clipStack[clipStackDepth].second() = Math::min(parentMax, max);

                /* If the previous clip rect affected no nodes, replace it,
                   otherwise move to the next one. */
                if(clipRectNodeCounts[clipRectsOffset])
                    ++clipRectsOffset;

                /* Save the final clip rect to the output. Initially it affects
                   just the clipping node itself. */
                clipRectOffsets[clipRectsOffset] =
                    clipStack[clipStackDepth].first();
                clipRectSizes[clipRectsOffset] =
                    clipStack[clipStackDepth].second() -
                    clipStack[clipStackDepth].first();
                clipRectNodeCounts[clipRectsOffset] = 1;

                /* Remember offset after all children of its node so we know
                   when to pop this clip rect off the stack */
                clipStack[clipStackDepth].third() = i + visibleNodeChildrenCounts[i] + 1;
                ++clipStackDepth;
                ++i;

            /* For an invisible node there's no point in testing any children
               as they'd be clipped away too */
            } else {
                const UnsignedInt nodePlusChildrenCount = visibleNodeChildrenCounts[i] + 1;
                i += nodePlusChildrenCount;
                clipRectNodeCounts[clipRectsOffset] += nodePlusChildrenCount;
            }

        /* If the node isn't a clipping node, just continue to the next one
           after */
        } else {
            ++i;
            ++clipRectNodeCounts[clipRectsOffset];
        }

        /* Save the visibility status */
        if(visible)
            visibleNodeMask.set(nodeId);

        /* Pop the clip stack items for which all children were processed */
        bool clipStackChanged = false;
        while(clipStackDepth && clipStack[clipStackDepth - 1].third() == i) {
            --clipStackDepth;
            clipStackChanged = true;
        }

        /* If we're at another top level node, it's a new draw, which means we
           need to start a new clip rect as well */
        if(i == topLevelNodeEnd && i != visibleNodeIds.size()) {
            topLevelNodeEnd = i + visibleNodeChildrenCounts[i] + 1;
            clipStackChanged = true;
        }

        /* If the clip stack changed, decide about the clip rect to use for the
           next items. Unless we're at the end of the node list, at which point
           there may not be any space for any more clip rects. */
        if(clipStackChanged && i != visibleNodeIds.size()) {
            /* Each iteration of the loop either increases the last
               clipRectNodeCounts or moves to the next element and sets it to
               1, so it's never 0 */
            CORRADE_INTERNAL_DEBUG_ASSERT(clipRectNodeCounts[clipRectsOffset]);
            ++clipRectsOffset;

            /* If there's no non-implicit clip rect available, use the "none"
               rect */
            if(clipStackDepth == 1) {
                clipRectOffsets[clipRectsOffset] = {};
                clipRectSizes[clipRectsOffset] = {};

            /* Otherwise go back to the parent clip rect */
            } else {
                CORRADE_INTERNAL_DEBUG_ASSERT(clipStackDepth > 1);

                clipRectOffsets[clipRectsOffset] =
                    clipStack[clipStackDepth - 1].first();
                clipRectSizes[clipRectsOffset] =
                    clipStack[clipStackDepth - 1].second() -
                    clipStack[clipStackDepth - 1].first();
            }

            /* There's no nodes to consume this clip rect yet */
            clipRectNodeCounts[clipRectsOffset] = 0;
        }
    }

    /* Expect the top-level node range were correctly matched. There shouldn't
       be any empty clip rect at the end, as an empty one is only added when
       `i` isn't at the end. */
    CORRADE_INTERNAL_ASSERT(i == topLevelNodeEnd && clipRectNodeCounts[clipRectsOffset]);

    return clipRectsOffset + 1;
}

/* The `dataToUpdateLayerOffsets` and `dataToUpdateIds` arrays get filled with
   data and node IDs in the desired draw order, clustered by layer ID, with
   `dataToUpdateLayerOffsets[i]` to `dataToUpdateLayerOffsets[i + 1]` being the
   range of data in `dataToUpdateIds` corresponding to layer `i`.

   The `dataToDrawOffsets[j]` and `dataToDrawSizes[j]` is then a range in
   `dataToUpdateIds` that should be drawn with layer `dataToDrawLayerIds[j]`,
   with their total count being the first return value of this function.

   The `dataToDrawClipRectOffsets[k]` and `dataToDrawClipRectSizes[k]` is a
   range in `dataToUpdateClipRectIds` that should be passed together with the
   draw data, with their total count being the second return value of this
   function.

   The `visibleNodeDataOffsets` and `visibleNodeDataIds` arrays are temporary
   storage -- they get filled with data IDs for visible nodes, with  `visibleNodeDataOffsets[i]` to
   `visibleNodeDataOffsets[i + 1]` being the range of data in
   `visibleNodeDataIds` corresponding to visible node at index `i`. */
Containers::Pair<UnsignedInt, UnsignedInt> orderVisibleNodeDataInto(const Containers::StridedArrayView1D<const UnsignedInt>& visibleNodeIds, const Containers::StridedArrayView1D<const UnsignedInt>& visibleNodeChildrenCounts, const Containers::StridedArrayView1D<const NodeHandle>& dataNodes, LayerFeatures layerFeatures, const Containers::BitArrayView visibleNodeMask, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectNodeCounts, const Containers::ArrayView<UnsignedInt> visibleNodeDataOffsets, const Containers::ArrayView<UnsignedInt> visibleNodeDataIds, const Containers::StridedArrayView1D<UnsignedInt>& dataToUpdateIds, const Containers::StridedArrayView1D<UnsignedInt>& dataToUpdateClipRectIds, const Containers::StridedArrayView1D<UnsignedInt>& dataToUpdateClipRectDataCounts, UnsignedInt offset, UnsignedInt clipRectOffset, const Containers::StridedArrayView1D<UnsignedInt>& dataToDrawOffsets, const Containers::StridedArrayView1D<UnsignedInt>& dataToDrawSizes, const Containers::StridedArrayView1D<UnsignedInt>& dataToDrawClipRectOffsets, const Containers::StridedArrayView1D<UnsignedInt>& dataToDrawClipRectSizes) {
    CORRADE_INTERNAL_ASSERT(
        visibleNodeChildrenCounts.size() == visibleNodeIds.size() &&
        visibleNodeDataOffsets.size() == visibleNodeMask.size() + 1 &&
        visibleNodeDataIds.size() == dataNodes.size() &&
        offset <= dataToUpdateIds.size() &&
        dataToUpdateClipRectDataCounts.size() == dataToUpdateClipRectIds.size()  &&
        clipRectOffset <= dataToUpdateClipRectIds.size() &&
        /* These should have the size matching the top-level node count */
        dataToDrawSizes.size() == dataToDrawOffsets.size() &&
        dataToDrawClipRectOffsets.size() == dataToDrawOffsets.size() &&
        dataToDrawClipRectSizes.size() == dataToDrawOffsets.size());

    /* If there's no visible nodes to go through, bail. Otherwise it'd attempt
       to access out-of-bounds dataToUpdateClipRectIds etc below. */
    if(visibleNodeIds.isEmpty()) {
        CORRADE_INTERNAL_ASSERT(
            offset == 0 &&
            clipRectNodeCounts.isEmpty() &&
            clipRectOffset == 0);
        return {};
    }

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
    UnsignedInt clipRectInputOffset = 0;
    dataToUpdateClipRectIds[clipRectOffset] = 0;
    dataToUpdateClipRectDataCounts[clipRectOffset] = 0;
    for(UnsignedInt visibleTopLevelNodeIndex = 0; visibleTopLevelNodeIndex != visibleNodeChildrenCounts.size(); visibleTopLevelNodeIndex += visibleNodeChildrenCounts[visibleTopLevelNodeIndex] + 1) {
        /* Remember how much data was drawn for the previous node so we can
           figure out the range to draw for this one... */
        const UnsignedInt previousOffset = offset;
        const UnsignedInt previousClipRectOutputOffset = clipRectOffset;

        /* Go through all (direct and nested) children of the top-level node
           and then all data of each, and copy their IDs to the output range */
        for(UnsignedInt i = 0, iMax = visibleNodeChildrenCounts[visibleTopLevelNodeIndex] + 1; i != iMax; ++i) {
            const UnsignedInt visibleNodeId = visibleNodeIds[visibleTopLevelNodeIndex + i];

            for(UnsignedInt j = visibleNodeDataOffsets[visibleNodeId], jMax = visibleNodeDataOffsets[visibleNodeId + 1]; j != jMax; ++j) {
                dataToUpdateIds[offset] = visibleNodeDataIds[j];
                ++offset;
            }
        }

        /* Convert the "clip rect affects N next visible nodes" counts to
           "clip rect affects N next data attached to visible nodes" counts */
        UnsignedInt clipRectNodeCount = 0;
        for(UnsignedInt i = 0, iMax = visibleNodeChildrenCounts[visibleTopLevelNodeIndex] + 1; i != iMax; ++i) {
            const UnsignedInt visibleNodeId = visibleNodeIds[visibleTopLevelNodeIndex + i];

            /* For each node, add the count of data attached to that node to
               the output. Which, on the other hand, *can* be zero. */
            dataToUpdateClipRectDataCounts[clipRectOffset] += visibleNodeDataOffsets[visibleNodeId + 1] - visibleNodeDataOffsets[visibleNodeId];
            ++clipRectNodeCount;

            /* If we exhausted all nodes for this clip rect, move to the next
               one. In order for this to work, it assumes all input counts are
               non-zero. */
            CORRADE_INTERNAL_DEBUG_ASSERT(clipRectNodeCounts[clipRectInputOffset]);
            if(clipRectNodeCount == clipRectNodeCounts[clipRectInputOffset]) {
                ++clipRectInputOffset;
                if(dataToUpdateClipRectDataCounts[clipRectOffset])
                    ++clipRectOffset;
                /* If we're at the end of the input, the dataToUpdateClipRect*
                   may not have any space left for the rest. Don't attempt to
                   write there in that case. */
                if(clipRectInputOffset != clipRectNodeCounts.size()) {
                    dataToUpdateClipRectIds[clipRectOffset] = clipRectInputOffset;
                    dataToUpdateClipRectDataCounts[clipRectOffset] = 0;
                }
                clipRectNodeCount = 0;
            }
        }

        /* If this layer is a drawing layer and there's any data to be drawn
           added by the above loop, save a range to the `dataToUpdateIds` and
           `dataToUpdateNodeIds` arrays, and a corresponding clip rect range as
           well. If there's no data to be drawn, put zeros there. */
        if(layerFeatures >= LayerFeature::Draw) {
            if(offset - previousOffset) {
                dataToDrawOffsets[drawOffset] = previousOffset;
                dataToDrawSizes[drawOffset] = offset - previousOffset;
                dataToDrawClipRectOffsets[drawOffset] = previousClipRectOutputOffset;
                dataToDrawClipRectSizes[drawOffset] = clipRectOffset - previousClipRectOutputOffset;
            } else {
                dataToDrawOffsets[drawOffset] = 0;
                dataToDrawSizes[drawOffset] = 0;
                dataToDrawClipRectOffsets[drawOffset] = 0;
                dataToDrawClipRectSizes[drawOffset] = 0;
            }
            ++drawOffset;
        }
    }

    /* After all top-level nodes we should have the clip rect array fully
       exhausted */
    CORRADE_INTERNAL_ASSERT(clipRectInputOffset == clipRectNodeCounts.size());

    return {offset, clipRectOffset};
}

/* Counts how much data belongs to each visible node, skipping the first
   element. Should be called for `dataNodes` from all layers that have
   LayerFeature::Event, the `visibleNodeEventDataOffsets` array then converted
   to an offset array and passed to `orderNodeDataForEventHandling()` below. */
void countNodeDataForEventHandlingInto(const Containers::StridedArrayView1D<const NodeHandle>& dataNodes, const Containers::ArrayView<UnsignedInt> visibleNodeEventDataOffsets, const Containers::BitArrayView visibleNodeMask) {
    CORRADE_INTERNAL_ASSERT(
        visibleNodeEventDataOffsets.size() == visibleNodeMask.size() + 1);

    for(const NodeHandle node: dataNodes) {
        if(node == NodeHandle::Null)
            continue;
        const UnsignedInt id = nodeHandleId(node);
        if(visibleNodeMask[id])
            ++visibleNodeEventDataOffsets[id + 1];
    }
}

/* The `dataNodes` array is expected to be the same as passed into
   `orderVisibleNodeDataInto()`. The array indices together with `layer` are
   used to form (generation-less) data handles in the output.

   The `visibleNodeEventDataOffsets` is expected to be the output of
   `orderVisibleNodeDataInto()` above with an additional first zero element,
   turned into an offset array. The process of calling this function for all
   event layers shifts the array by one element, with
   `visibleNodeEventDataOffsets[i]` to `visibleNodeEventDataOffsets[i + 1]`
   then being the range of data in `visibleNodeEventData` corresponding to node
   `i`. */
void orderNodeDataForEventHandlingInto(const LayerHandle layer, const Containers::StridedArrayView1D<const NodeHandle>& dataNodes, const Containers::ArrayView<UnsignedInt> visibleNodeEventDataOffsets, const Containers::BitArrayView visibleEventNodeMask, const Containers::ArrayView<DataHandle> visibleNodeEventData) {
    CORRADE_INTERNAL_ASSERT(
        visibleNodeEventDataOffsets.size() == visibleEventNodeMask.size() + 1);

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
        if(visibleEventNodeMask[id])
            /* The DataHandle generation isn't used for anything, only data and
               layer ID is extracted out of the handle, so can be arbitrary
               (but not 0, as that'd make dataHandleId() assert). */
            visibleNodeEventData[visibleNodeEventDataOffsets[id + 1]++] = dataHandle(layer, i - 1, 0xfff);
    }
}

/* Reduces the three arrays by throwing away items where size is 0. Returns the
   resulting size. */
UnsignedInt compactDrawsInPlace(const Containers::StridedArrayView1D<UnsignedByte>& dataToDrawLayerIds, const Containers::StridedArrayView1D<UnsignedInt>& dataToDrawOffsets, const Containers::StridedArrayView1D<UnsignedInt>& dataToDrawSizes, const Containers::StridedArrayView1D<UnsignedInt>& dataToDrawClipRectOffsets, const Containers::StridedArrayView1D<UnsignedInt>& dataToDrawClipRectSizes) {
    CORRADE_INTERNAL_ASSERT(
        dataToDrawOffsets.size() == dataToDrawLayerIds.size() &&
        dataToDrawSizes.size() == dataToDrawLayerIds.size() &&
        dataToDrawClipRectSizes.size() == dataToDrawClipRectOffsets.size());

    std::size_t offset = 0;
    for(std::size_t i = 0, iMax = dataToDrawLayerIds.size(); i != iMax; ++i) {
        if(!dataToDrawSizes[i]) {
            CORRADE_INTERNAL_DEBUG_ASSERT(!dataToDrawClipRectSizes[i]);
            continue;
        }

        /* Don't copy to itself */
        if(i != offset) {
            dataToDrawLayerIds[offset] = dataToDrawLayerIds[i];
            dataToDrawOffsets[offset] = dataToDrawOffsets[i];
            dataToDrawSizes[offset] = dataToDrawSizes[i];
            dataToDrawClipRectOffsets[offset] = dataToDrawClipRectOffsets[i];
            dataToDrawClipRectSizes[offset] = dataToDrawClipRectSizes[i];
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

/* Calculates compositing rectangles for all nodes referenced by drawn data,
   intersecting them with corresponding clip rectangles. The `dataIds` and
   `compositeRectOffsets` + `compositeRectSizes` views are are meant to be the
   same that's passed to `update()` but sliced to `offset` + `count` for a
   particular draw; the `clipRectIds` + `clipRectDataCounts` then
   similarly the views that are passed to `update()` but sliced to
   `clipRectOffset` + `clipRectCount` for a particular draw. */
void compositeRectsInto(const Vector2& uiOffset, const Vector2& uiSize, const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const Containers::StridedArrayView1D<const NodeHandle>& dataNodes, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes, const Containers::StridedArrayView1D<Vector2>& compositeRectOffsets, const Containers::StridedArrayView1D<Vector2>& compositeRectSizes) {
    CORRADE_INTERNAL_ASSERT(
        clipRectDataCounts.size() == clipRectIds.size() &&
        nodeSizes.size() == nodeOffsets.size() &&
        clipRectOffsets.size() == clipRectSizes.size() &&
        compositeRectOffsets.size() == dataIds.size() &&
        compositeRectSizes.size() == dataIds.size());

    /** @todo handle the case of multiple data per the same node, which means
        filling less than `dataIds.size()` rects and returning the size, then
        the internals have to keep some running offset for slicing into
        doComposite(), can't reuse the doDraw() running offset anymore */

    std::size_t dataOffset = 0;
    for(std::size_t i = 0; i != clipRectIds.size(); ++i) {
        const UnsignedInt clipRectId = clipRectIds[i];
        const Vector2 clipRectSize = clipRectSizes[clipRectId];

        Vector2 clipRectMin{NoInit}, clipRectMax{NoInit};
        if(clipRectSize.isZero()) {
            clipRectMin = uiOffset;
            clipRectMax = uiOffset + uiSize;
        } else {
            clipRectMin = clipRectOffsets[clipRectId];
            clipRectMax = clipRectMin + clipRectSize;
            /* The clip rects should already include the UI size at this
               point, as done by cullVisibleNodesInto() */
            CORRADE_INTERNAL_DEBUG_ASSERT((clipRectMin >= uiOffset).all() &&
                                          (clipRectMax <= uiOffset + uiSize).all());
        }

        const std::size_t clipRectDataCount = clipRectDataCounts[i];
        for(std::size_t j = 0; j != clipRectDataCount; ++j) {
            const NodeHandle node = dataNodes[dataIds[dataOffset + j]];
            const UnsignedInt nodeId = nodeHandleId(node);
            const Vector2 nodeMin = nodeOffsets[nodeId];
            const Vector2 nodeMax = nodeMin + nodeSizes[nodeId];
            const Vector2 compositingRectMin = Math::max(nodeMin, clipRectMin);
            const Vector2 compositingRectMax = Math::min(nodeMax, clipRectMax);
            compositeRectOffsets[dataOffset + j] = compositingRectMin;
            compositeRectSizes[dataOffset + j] = compositingRectMax - compositingRectMin;
        }

        dataOffset += clipRectDataCount;
    }
    CORRADE_INTERNAL_ASSERT(dataOffset == dataIds.size());
}

/* Query a list of animators partitioned into the following groups:
   - Animators with no NodeAttachment
   - Animators with NodeAttachment
   - Animators with DataAttachment, further partitioned by layer */
Containers::ArrayView<const Containers::Reference<AbstractAnimator>> partitionedAnimatorsNone(const Containers::ArrayView<const Containers::Reference<AbstractAnimator>> instances, const UnsignedInt nodeAttachmentAnimatorOffset) {
    CORRADE_INTERNAL_ASSERT(nodeAttachmentAnimatorOffset <= instances.size());
    return instances.prefix(nodeAttachmentAnimatorOffset);
}
Containers::ArrayView<const Containers::Reference<AbstractAnimator>> partitionedAnimatorsAnyNodeAttachment(const Containers::ArrayView<const Containers::Reference<AbstractAnimator>> instances, const UnsignedInt nodeAttachmentAnimatorOffset, const Containers::StridedArrayView1D<const UnsignedShort>& dataAttachmentAnimatorOffsets) {
    const std::size_t end = dataAttachmentAnimatorOffsets.isEmpty() ? instances.size() : dataAttachmentAnimatorOffsets.front();
    CORRADE_INTERNAL_ASSERT(nodeAttachmentAnimatorOffset <= end);

    return instances.slice(nodeAttachmentAnimatorOffset, end);
}
Containers::ArrayView<const Containers::Reference<AbstractAnimator>> partitionedAnimatorsGenericNodeAttachment(const Containers::ArrayView<const Containers::Reference<AbstractAnimator>> instances, const UnsignedInt nodeAttachmentAnimatorOffset, const UnsignedInt nodeAnimatorOffset, const Containers::StridedArrayView1D<const UnsignedShort>& /*dataAttachmentAnimatorOffsets*/) {
    CORRADE_INTERNAL_ASSERT(nodeAttachmentAnimatorOffset <= nodeAnimatorOffset);
    return instances.slice(nodeAttachmentAnimatorOffset, nodeAnimatorOffset);
}
Containers::ArrayView<const Containers::Reference<AbstractAnimator>> partitionedAnimatorsNodeNodeAttachment(const Containers::ArrayView<const Containers::Reference<AbstractAnimator>> instances, UnsignedInt /*nodeAttachmentAnimatorOffset*/, const UnsignedInt nodeAnimatorOffset, const Containers::StridedArrayView1D<const UnsignedShort>& dataAttachmentAnimatorOffsets) {
    const std::size_t end = dataAttachmentAnimatorOffsets.isEmpty() ? instances.size() : dataAttachmentAnimatorOffsets.front();
    CORRADE_INTERNAL_ASSERT(nodeAnimatorOffset <= end);

    return instances.slice(nodeAnimatorOffset, end);
}
Containers::ArrayView<const Containers::Reference<AbstractAnimator>> partitionedAnimatorsAnyDataAttachment(const Containers::ArrayView<const Containers::Reference<AbstractAnimator>> instances, const Containers::StridedArrayView1D<const UnsignedShort>& dataAttachmentAnimatorOffsets, const LayerHandle layer) {
    const UnsignedInt layerId = layerHandleId(layer);
    CORRADE_INTERNAL_ASSERT(layerId < dataAttachmentAnimatorOffsets.size());
    if(layerId == dataAttachmentAnimatorOffsets.size() - 1)
        return instances.exceptPrefix(dataAttachmentAnimatorOffsets.back());

    return instances.slice(dataAttachmentAnimatorOffsets[layerId], dataAttachmentAnimatorOffsets[layerId + 1]);
}
Containers::ArrayView<const Containers::Reference<AbstractAnimator>> partitionedAnimatorsGenericDataAttachment(const Containers::ArrayView<const Containers::Reference<AbstractAnimator>> instances, const Containers::StridedArrayView1D<const UnsignedShort>& dataAttachmentAnimatorOffsets, const Containers::StridedArrayView1D<const UnsignedShort>& dataAnimatorOffsets, const Containers::StridedArrayView1D<const UnsignedShort>& /*styleAnimatorOffsets*/, const LayerHandle layer) {
    const UnsignedInt layerId = layerHandleId(layer);
    CORRADE_INTERNAL_ASSERT(layerId < dataAttachmentAnimatorOffsets.size() && dataAttachmentAnimatorOffsets[layerId] <= dataAnimatorOffsets[layerId]);
    return instances.slice(dataAttachmentAnimatorOffsets[layerId], dataAnimatorOffsets[layerId]);
}
Containers::ArrayView<const Containers::Reference<AbstractAnimator>> partitionedAnimatorsDataDataAttachment(const Containers::ArrayView<const Containers::Reference<AbstractAnimator>> instances, const Containers::StridedArrayView1D<const UnsignedShort>& /*dataAttachmentAnimatorOffsets*/, const Containers::StridedArrayView1D<const UnsignedShort>& dataAnimatorOffsets, const Containers::StridedArrayView1D<const UnsignedShort>& styleAnimatorOffsets, const LayerHandle layer) {
    const UnsignedInt layerId = layerHandleId(layer);
    CORRADE_INTERNAL_ASSERT(layerId < dataAnimatorOffsets.size());
    CORRADE_INTERNAL_ASSERT(dataAnimatorOffsets[layerId] <= styleAnimatorOffsets[layerId]);
    return instances.slice(dataAnimatorOffsets[layerId], styleAnimatorOffsets[layerId]);
}
Containers::ArrayView<const Containers::Reference<AbstractAnimator>> partitionedAnimatorsStyleDataAttachment(const Containers::ArrayView<const Containers::Reference<AbstractAnimator>> instances, const Containers::StridedArrayView1D<const UnsignedShort>& dataAttachmentAnimatorOffsets, const Containers::StridedArrayView1D<const UnsignedShort>& /*dataAnimatorOffsets*/, const Containers::StridedArrayView1D<const UnsignedShort>& styleAnimatorOffsets, const LayerHandle layer) {
    const UnsignedInt layerId = layerHandleId(layer);
    CORRADE_INTERNAL_ASSERT(layerId < dataAttachmentAnimatorOffsets.size());
    const std::size_t end = layerId == dataAttachmentAnimatorOffsets.size() - 1 ?
        instances.size() : dataAttachmentAnimatorOffsets[layerId + 1];
    CORRADE_INTERNAL_ASSERT(styleAnimatorOffsets[layerId] <= end);
    return instances.slice(styleAnimatorOffsets[layerId], end);
}

enum class AnimatorType {
    Generic,
    Node,
    Data,
    Style
};

/* Insert into the partitioned animator list and update the offsets
   accordingly. The features and layer are taken by value to not need to
   include the whole AbstractAnimator here. */
void partitionedAnimatorsInsert(Containers::Array<Containers::Reference<AbstractAnimator>>& instances, AbstractAnimator& instance, const AnimatorType type, const AnimatorFeatures features, const LayerHandle layer, UnsignedInt& nodeAttachmentAnimatorOffset, UnsignedInt& nodeAnimatorOffset, const Containers::StridedArrayView1D<UnsignedShort>& dataAttachmentAnimatorOffsets, const Containers::StridedArrayView1D<UnsignedShort>& dataAnimatorOffsets, const Containers::StridedArrayView1D<UnsignedShort>& styleAnimatorOffsets) {
    /* Find the slice the animator should be in and directly take that as an
       oppportunity to update the offsets */
    std::size_t dataAttachmentAnimatorOffsetUpdateOffset, dataAnimatorOffsetUpdateOffset, styleAnimatorOffsetUpdateOffset;
    Containers::ArrayView<const Containers::Reference<AbstractAnimator>> slice;
    if(features >= AnimatorFeature::DataAttachment) {
        dataAttachmentAnimatorOffsetUpdateOffset = layerHandleId(layer) + 1;
        if(type == AnimatorType::Style) {
            slice = partitionedAnimatorsStyleDataAttachment(instances, dataAttachmentAnimatorOffsets, dataAnimatorOffsets, styleAnimatorOffsets, layer);
            dataAnimatorOffsetUpdateOffset = layerHandleId(layer) + 1;
            styleAnimatorOffsetUpdateOffset = layerHandleId(layer) + 1;
        } else if(type == AnimatorType::Data) {
            slice = partitionedAnimatorsDataDataAttachment(instances, dataAttachmentAnimatorOffsets, dataAnimatorOffsets, styleAnimatorOffsets, layer);
            dataAnimatorOffsetUpdateOffset = layerHandleId(layer) + 1;
            styleAnimatorOffsetUpdateOffset = layerHandleId(layer);
        } else {
            CORRADE_INTERNAL_ASSERT(type == AnimatorType::Generic);
            slice = partitionedAnimatorsGenericDataAttachment(instances, dataAttachmentAnimatorOffsets, dataAnimatorOffsets, styleAnimatorOffsets, layer);
            dataAnimatorOffsetUpdateOffset = layerHandleId(layer);
            styleAnimatorOffsetUpdateOffset = layerHandleId(layer);
        }
    } else {
        dataAttachmentAnimatorOffsetUpdateOffset = 0;
        dataAnimatorOffsetUpdateOffset = 0;
        styleAnimatorOffsetUpdateOffset = 0;
        CORRADE_INTERNAL_ASSERT(layer == LayerHandle::Null);
        if(features >= AnimatorFeature::NodeAttachment) {
            if(type == AnimatorType::Node) {
                slice = partitionedAnimatorsNodeNodeAttachment(instances, nodeAttachmentAnimatorOffset, nodeAnimatorOffset, dataAttachmentAnimatorOffsets);
            } else {
                CORRADE_INTERNAL_ASSERT(type == AnimatorType::Generic);
                slice = partitionedAnimatorsGenericNodeAttachment(instances, nodeAttachmentAnimatorOffset, nodeAnimatorOffset, dataAttachmentAnimatorOffsets);
                ++nodeAnimatorOffset;
            }
        } else {
            CORRADE_INTERNAL_ASSERT(type == AnimatorType::Generic);
            slice = partitionedAnimatorsNone(instances, nodeAttachmentAnimatorOffset);
            ++nodeAttachmentAnimatorOffset;
            ++nodeAnimatorOffset;
        }
    }
    for(UnsignedShort& i: dataAttachmentAnimatorOffsets.exceptPrefix(dataAttachmentAnimatorOffsetUpdateOffset))
        ++i;
    for(UnsignedShort& i: dataAnimatorOffsets.exceptPrefix(dataAnimatorOffsetUpdateOffset))
        ++i;
    for(UnsignedShort& i: styleAnimatorOffsets.exceptPrefix(styleAnimatorOffsetUpdateOffset))
        ++i;

    /* Insert at the end of given slice */
    arrayInsert(instances, slice.end() - instances.begin(), instance);
}

/* Remove from the partitioned animator list and update the offsets
   accordingly */
void partitionedAnimatorsRemove(Containers::Array<Containers::Reference<AbstractAnimator>>& instances, const AbstractAnimator& instance, const AnimatorFeatures features, const LayerHandle layer, UnsignedInt& nodeAttachmentAnimatorOffset, UnsignedInt& nodeAnimatorOffset, const Containers::StridedArrayView1D<UnsignedShort>& dataAttachmentAnimatorOffsets, const Containers::StridedArrayView1D<UnsignedShort>& dataAnimatorOffsets, const Containers::StridedArrayView1D<UnsignedShort>& styleAnimatorOffsets) {
    /* Find the slice the animator should be in and directly take that as an
       oppportunity to update the offsets */
    std::size_t dataAttachmentAnimatorOffsetUpdateOffset;
    Containers::ArrayView<const Containers::Reference<AbstractAnimator>> slice;
    if(features >= AnimatorFeature::DataAttachment) {
        slice = partitionedAnimatorsAnyDataAttachment(instances, dataAttachmentAnimatorOffsets, layer);
        dataAttachmentAnimatorOffsetUpdateOffset = layerHandleId(layer) + 1;
    } else {
        dataAttachmentAnimatorOffsetUpdateOffset = 0;
        CORRADE_INTERNAL_ASSERT(layer == LayerHandle::Null);
        if(features >= AnimatorFeature::NodeAttachment) {
            slice = partitionedAnimatorsAnyNodeAttachment(instances, nodeAttachmentAnimatorOffset, dataAttachmentAnimatorOffsets);
        } else {
            slice = partitionedAnimatorsNone(instances, nodeAttachmentAnimatorOffset);
            CORRADE_INTERNAL_ASSERT(nodeAttachmentAnimatorOffset != 0);
            --nodeAttachmentAnimatorOffset;
        }
    }
    for(UnsignedShort& i: dataAttachmentAnimatorOffsets.exceptPrefix(dataAttachmentAnimatorOffsetUpdateOffset)) {
        CORRADE_INTERNAL_ASSERT(i != 0);
        --i;
    }
    for(UnsignedShort& i: dataAnimatorOffsets.exceptPrefix(dataAttachmentAnimatorOffsetUpdateOffset)) {
        CORRADE_INTERNAL_ASSERT(i != 0);
        --i;
    }
    for(UnsignedShort& i: styleAnimatorOffsets.exceptPrefix(dataAttachmentAnimatorOffsetUpdateOffset)) {
        CORRADE_INTERNAL_ASSERT(i != 0);
        --i;
    }

    /* Find the actual instance in given slice. Yes, this is a linear search,
       but I don't expect there being that many animators in every slice (the
       cap is 256) and that many being added and removed all the time, so this
       should be fine. */
    std::size_t found = ~std::size_t{};
    for(const Containers::Reference<AbstractAnimator>& i: slice) {
        if(&*i == &instance) {
            found = &i - instances;
            break;
        }
    }

    /* If the instance was found before AbstractDataAnimators /
       AbstractStyleAnimators for given layer, decrement the corresponding
       offset as well */
    if(features >= AnimatorFeature::DataAttachment) {
        if(found < dataAnimatorOffsets[layerHandleId(layer)]) {
            CORRADE_INTERNAL_ASSERT(dataAnimatorOffsets[layerHandleId(layer)] <= styleAnimatorOffsets[layerHandleId(layer)]);
            --styleAnimatorOffsets[layerHandleId(layer)];
            --dataAnimatorOffsets[layerHandleId(layer)];
        } else if(found < styleAnimatorOffsets[layerHandleId(layer)])
            --styleAnimatorOffsets[layerHandleId(layer)];
    }

    /* If the instance was found before AbstractNodeAnimators, decrement that
       offset as well */
    if(found < nodeAnimatorOffset)
        --nodeAnimatorOffset;

    /* Remove it from the array. The animator should always be in given slice
       if it has an instance. */
    CORRADE_INTERNAL_ASSERT(found != ~std::size_t{});
    arrayRemove(instances, found);
}

void partitionedAnimatorsCreateLayer(const Containers::ArrayView<const Containers::Reference<AbstractAnimator>> instances, const Containers::StridedArrayView1D<UnsignedShort>& dataAttachmentAnimatorOffsets, const Containers::StridedArrayView1D<UnsignedShort>& dataAnimatorOffsets, const Containers::StridedArrayView1D<UnsignedShort>& styleAnimatorOffsets, const LayerHandle layer) {
    const UnsignedInt layerId = layerHandleId(layer);
    CORRADE_INTERNAL_ASSERT(layerId < dataAttachmentAnimatorOffsets.size());

    /* If this is a new layer at the end, set its offset to after all previous
       instances */
    if(layerId == dataAttachmentAnimatorOffsets.size() - 1) {
        dataAttachmentAnimatorOffsets[layerId] = instances.size();
        dataAnimatorOffsets[layerId] = instances.size();
        styleAnimatorOffsets[layerId] = instances.size();
    /* Otherwise it should already have a correct value, ensured by
       partitionedAnimatorsRemoveLayer() and subsequent Insert() / Remove()
       calls */
    } else CORRADE_INTERNAL_ASSERT(
        dataAttachmentAnimatorOffsets[layerId] == dataAttachmentAnimatorOffsets[layerId + 1] &&
        dataAnimatorOffsets[layerId] == dataAttachmentAnimatorOffsets[layerId + 1] &&
        styleAnimatorOffsets[layerId] == dataAttachmentAnimatorOffsets[layerId + 1]);
}

void partitionedAnimatorsRemoveLayer(Containers::Array<Containers::Reference<AbstractAnimator>>& instances, const Containers::StridedArrayView1D<UnsignedShort>& dataAttachmentAnimatorOffsets, const Containers::StridedArrayView1D<UnsignedShort>& dataAnimatorOffsets, const Containers::StridedArrayView1D<UnsignedShort>& styleAnimatorOffsets, const LayerHandle layer) {
    const UnsignedInt layerId = layerHandleId(layer);
    CORRADE_INTERNAL_ASSERT(layerId < dataAttachmentAnimatorOffsets.size());

    /* There are no animators in this layer anymore, do the data / style
       animator offset is the same as the data attachment animator offset */
    dataAnimatorOffsets[layerId] = dataAttachmentAnimatorOffsets[layerId];
    styleAnimatorOffsets[layerId] = dataAttachmentAnimatorOffsets[layerId];

    /* Remove all instances that belonged to this layer, and update all offsets
       after to exclude the now-removed range */
    const std::size_t count = (layerId == dataAttachmentAnimatorOffsets.size() - 1 ? instances.size() : dataAttachmentAnimatorOffsets[layerId + 1]) - dataAttachmentAnimatorOffsets[layerId];
    arrayRemove(instances, dataAttachmentAnimatorOffsets[layerId], count);
    for(UnsignedShort& i: dataAttachmentAnimatorOffsets.exceptPrefix(layerId + 1))
        i -= count;
    for(UnsignedShort& i: dataAnimatorOffsets.exceptPrefix(layerId + 1))
        i -= count;
    for(UnsignedShort& i: styleAnimatorOffsets.exceptPrefix(layerId + 1))
        i -= count;
}

}}}}

#endif
