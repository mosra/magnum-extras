#ifndef Magnum_Ui_Implementation_snapLayouter_h
#define Magnum_Ui_Implementation_snapLayouter_h
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023, 2024, 2025, 2026
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

#include <Corrade/Containers/StridedBitArrayView.h>
#include <Corrade/Containers/Triple.h>
#include <Magnum/Math/Functions.h>
#include <Magnum/Math/Swizzle.h>
#include <Magnum/Math/Vector4.h>

namespace Magnum { namespace Ui { namespace Implementation { namespace {

/* Calculates total size of all child layouts along with their margin. The
   output size and margin is then meant to be passed to layoutSizePadding()
   which then calculates the actual node size and padding, which can be
   subsequently passed to snap() to position everything correctly. */
Containers::Pair<Vector2, Vector4> childLayoutSizeMargin(const Snaps childSnap, const Containers::StridedArrayView1D<const Vector4>& nodeMargins, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const LayouterDataHandle firstChildLayout, const Containers::StridedArrayView1D<const NodeHandle>& layoutNodes, const Containers::StridedArrayView1D<const LayouterDataHandle>& nextLayout) {
    CORRADE_INTERNAL_ASSERT(
        nodeSizes.size() == nodeMargins.size() &&
        firstChildLayout != LayouterDataHandle::Null &&
        nextLayout.size() == layoutNodes.size());

    /* First figure out margin component indices based on child snap. The order
       is left (0), top (1), right (2), bottom (3). */
    const Snaps snapNoNoPad = childSnap & ~Snap::NoPad;
    UnsignedInt indexBefore, indexAfter, indexSideL, indexSideR,
        indexSizeForward, indexSizeSide;
    enum Calculate {
        SideLeft,
        SideRight,
        Center,
        Fill,
    } calculate;

    /* +-------- 1 --------+
       |          sR       |
       |      +--+  +--+   |        +-- 0 -+
       0 a...a|  |ba|  |bb 2        1 size |
       |      +--+  +--+   |        +------+
       |          sL       |
       +-------- 3 --------+ */
    if(
        (snapNoNoPad|Snap::FillY|Snap::InsideY) == (Snap::Left|Snap::FillY|Snap::InsideY) ||
         snapNoNoPad == (Snap::TopLeft|Snap::InsideY) ||
         snapNoNoPad == (Snap::BottomLeft|Snap::InsideY)
    ) {
        indexBefore = 2;
        indexAfter = 0;
        indexSideL = 3;
        indexSideR = 1;
        indexSizeForward = 0;
        indexSizeSide = 1;

        if(snapNoNoPad >= (Snap::Left|Snap::FillY))
            calculate = Calculate::Fill;
        else if(snapNoNoPad >= Snap::TopLeft)
            calculate = Calculate::SideRight;
        else if(snapNoNoPad >= Snap::BottomLeft)
            calculate = Calculate::SideLeft;
        else if(!(snapNoNoPad & Snap::FillY))
            calculate = Calculate::Center;
        else CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */

    /* +-------- 1 --------+
       |       sL          |
       |   +--+  +--+      |        +-- 0 -+
       0 bb|  |ab|  |a...a 2        1 size |
       |   +--+  +--+      |        +------+
       |       sR          |
       +-------- 3 --------+ */
    } else if(
        (snapNoNoPad|Snap::FillY|Snap::InsideY) == (Snap::Right|Snap::FillY|Snap::InsideY) ||
         snapNoNoPad == (Snap::TopRight|Snap::InsideY) ||
         snapNoNoPad == (Snap::BottomRight|Snap::InsideY)
    ) {
        indexBefore = 0;
        indexAfter = 2;
        indexSideL = 1;
        indexSideR = 3;
        indexSizeForward = 0;
        indexSizeSide = 1;

        if(snapNoNoPad >= (Snap::Right|Snap::FillY))
            calculate = Calculate::Fill;
        else if(snapNoNoPad >= Snap::TopRight)
            calculate = Calculate::SideLeft;
        else if(snapNoNoPad >= Snap::BottomRight)
            calculate = Calculate::SideRight;
        else if(!(snapNoNoPad & Snap::FillY))
            calculate = Calculate::Center;
        else CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */

    /* +---- 1 ----+
       |     a     |
       |    ...    |
       |     a     |
       |   +---+   |
       |   |   |   |
       |   +---+   |        +-- 0 -+
       0     b     2        1 size |
       | sL  a  sR |        +------+
       |   +---+   |
       |   |   |   |
       |   +---+   |
       |     b     |
       |     b     |
       +---- 3 ----+ */
    } else if(
        (snapNoNoPad|Snap::FillX|Snap::InsideX) == (Snap::Top|Snap::FillX|Snap::InsideX) ||
         snapNoNoPad == (Snap::TopLeft|Snap::InsideX) ||
         snapNoNoPad == (Snap::TopRight|Snap::InsideX)
    ) {
        indexBefore = 3;
        indexAfter = 1;
        indexSideL = 0;
        indexSideR = 2;
        indexSizeForward = 1;
        indexSizeSide = 0;

        if(snapNoNoPad >= (Snap::Top|Snap::FillX))
            calculate = Calculate::Fill;
        else if(snapNoNoPad >= Snap::TopLeft)
            calculate = Calculate::SideLeft;
        else if(snapNoNoPad >= Snap::TopRight)
            calculate = Calculate::SideRight;
        else if(!(snapNoNoPad & Snap::FillX))
            calculate = Calculate::Center;
        else CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */

    /* +---- 1 ----+
       |     b     |
       |     b     |
       |   +---+   |
       |   |   |   |
       |   +---+   |
       | sR  a  sL |        +-- 0 -+
       0     b     2        1 size |
       |   +---+   |        +------+
       |   |   |   |
       |   +---+   |
       |     a     |
       |    ...    |
       |     a     |
       +---- 3 ----+ */
    } else if(
        (snapNoNoPad|Snap::FillX|Snap::InsideX) == (Snap::Bottom|Snap::FillX|Snap::InsideX) ||
         snapNoNoPad == (Snap::BottomLeft|Snap::InsideX) ||
         snapNoNoPad == (Snap::BottomRight|Snap::InsideX)
    ) {
        indexBefore = 1;
        indexAfter = 3;
        indexSideL = 2;
        indexSideR = 0;
        indexSizeForward = 1;
        indexSizeSide = 0;

        if(snapNoNoPad >= (Snap::Bottom|Snap::FillX))
            calculate = Calculate::Fill;
        else if(snapNoNoPad >= Snap::BottomLeft)
            calculate = Calculate::SideRight;
        else if(snapNoNoPad >= Snap::BottomRight)
            calculate = Calculate::SideLeft;
        else if(!(snapNoNoPad & Snap::FillX))
            calculate = Calculate::Center;
        else CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */

    /* The conditions above enumerate exactly the cases as in
       SnapLayouter::setChildSnapInternal(), thus there's no other variant that
       could happen. */
    } else CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */

    /* Whether to use the margin in the forward and size direction */
    const bool useMargin[]{
        !(childSnap >= Snap::NoPadX),
        !(childSnap >= Snap::NoPadY)
    };

    /* First child layout and corresponding node gives the first output margin
       value */
    LayouterDataHandle childLayout = firstChildLayout;
    UnsignedInt childNodeId = nodeHandleId(layoutNodes[layouterDataHandleId(childLayout)]);
    Vector4 outMargin{Math::ZeroInit};
    if(useMargin[indexSizeForward])
        outMargin[indexBefore] = nodeMargins[childNodeId][indexBefore];

    /* Calculate the size spanning all children along with their margins */
    Float previousNodeMarginAfter = 0.0f;
    Float nextMargin = 0.0f;
    Float sizeForward = 0.0f;
    /* For alignment to a side, only maxMarginSideL + maxSizeSideExceptMarginL
       or maxMarginSideR / maxSizeSideExceptMarginR are filled. For centered
       layout, only maxHalfSizeSideWithMarginL and maxHalfSizeSideWithMarginR
       are filled. For filled layout, maxSizeSideExceptMargin and
       maxMarginSideL + maxMarginSideR are filled. */
    Float maxMarginSideL = 0.0f;
    Float maxMarginSideR = 0.0f;
    Float maxSizeSideExceptMargin = 0.0f;
    Float maxSizeSideExceptMarginL = 0.0f;
    Float maxSizeSideExceptMarginR = 0.0f;
    Float maxHalfSizeSideWithMarginL = 0.0f;
    Float maxHalfSizeSideWithMarginR = 0.0f;
    do {
        const Vector2 childNodeSize = nodeSizes[childNodeId];
        /* Side margins. Before/after margins are combined with the previous /
           next node margin below. */
        const Float childNodeMarginSideL = nodeMargins[childNodeId][indexSideL];
        const Float childNodeMarginSideR = nodeMargins[childNodeId][indexSideR];

        /* Add the previously calculated next margin and current node size to
           the forward sizes. If useMargin[indexSizeForward] is false,
           nextMargin is always 0.0f. */
        sizeForward += nextMargin + childNodeSize[indexSizeForward];

        /* Max side size is calculated out of individual pieces based on how
           the alignment is done */
        if(calculate == Calculate::SideLeft || calculate == Calculate::Fill)
            maxMarginSideL = Math::max(maxMarginSideL, childNodeMarginSideL);
        if(calculate == Calculate::SideRight || calculate == Calculate::Fill)
            maxMarginSideR = Math::max(maxMarginSideR, childNodeMarginSideR);
        if(calculate == Calculate::SideLeft)
            maxSizeSideExceptMarginL = Math::max(maxSizeSideExceptMarginL,
                childNodeSize[indexSizeSide] + childNodeMarginSideR);
        if(calculate == Calculate::SideRight)
            maxSizeSideExceptMarginR = Math::max(maxSizeSideExceptMarginR,
                childNodeSize[indexSizeSide] + childNodeMarginSideL);
        if(calculate == Calculate::Center) {
            maxHalfSizeSideWithMarginL = Math::max(maxHalfSizeSideWithMarginL,
                childNodeSize[indexSizeSide]*0.5f + childNodeMarginSideL);
            maxHalfSizeSideWithMarginR = Math::max(maxHalfSizeSideWithMarginR,
                childNodeSize[indexSizeSide]*0.5f + childNodeMarginSideR);
        }

        /* Max size is used for Calculate::Fill but also below to figure out
           the max margin of all nodes, so calculate it always */
        maxSizeSideExceptMargin = Math::max(maxSizeSideExceptMargin,
                                            childNodeSize[indexSizeSide]);

        /* Advance to the next layout and calculate margin for it. If the next
           layout is firstChild again (i.e., we're at the end of the child
           list), the nextMargin value will stay unused and a value based on
           parentNodePadding will be used instead. */
        if(useMargin[indexSizeForward])
            previousNodeMarginAfter = nodeMargins[childNodeId][indexAfter];
        childLayout = nextLayout[layouterDataHandleId(childLayout)];
        childNodeId = nodeHandleId(layoutNodes[layouterDataHandleId(childLayout)]);
        if(useMargin[indexSizeForward]) nextMargin = Math::max(
            previousNodeMarginAfter,
            nodeMargins[childNodeId][indexBefore]);
    } while(childLayout != firstChildLayout);

    /* Figure out output size, which is just the forward and side size put into
       correct components */
    Vector2 outSize{NoInit};
    outSize[indexSizeForward] = sizeForward;
    outSize[indexSizeSide] = maxSizeSideExceptMargin;

    /* Fill in the remaining output margin, if it's meant to be used. If not,
       the fields stay zero-initialized. */
    if(useMargin[indexSizeForward])
        outMargin[indexAfter] = previousNodeMarginAfter;
    if(useMargin[indexSizeSide]) {
        if(calculate == Calculate::SideLeft) {
            outMargin[indexSideL] = maxMarginSideL;
            outMargin[indexSideR] = maxSizeSideExceptMarginL - maxSizeSideExceptMargin;
        } else if(calculate == Calculate::SideRight) {
            outMargin[indexSideL] = maxSizeSideExceptMarginR - maxSizeSideExceptMargin;
            outMargin[indexSideR] = maxMarginSideR;
        } else if(calculate == Calculate::Center) {
            outMargin[indexSideL] = maxHalfSizeSideWithMarginL - maxSizeSideExceptMargin*0.5f;
            outMargin[indexSideR] = maxHalfSizeSideWithMarginR - maxSizeSideExceptMargin*0.5f;
        } else if(calculate == Calculate::Fill) {
            outMargin[indexSideL] = maxMarginSideL;
            outMargin[indexSideR] = maxMarginSideR;
        } else CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
    }

    return {outSize, outMargin};
}

/* Used by explicitlySnappedChildLayoutSize() so has to be defined here */
constexpr SnapLayoutFlag SnapLayoutFlagExplicitSnapToParent = SnapLayoutFlag(0x80);

/* Calculates max size of direct explicitly snapped child layouts along with
   the corresponding padding and margin. Compared to childLayoutSizeMargin()
   this is basically just calculating the max value as the explicit snaps don't
   have any positioning constraints relative to each other. Explicitly snapped
   neighbors and neighbors of child layouts are deliberately ignored.

   The `flags` are also not used in any way except for the internal
   SnapLayoutFlagExplicitSnapToParent bit. */
Vector2 explicitlySnappedChildLayoutSize(const SnapLayoutFlags parentFlags, const Vector4& parentPadding, const Containers::StridedArrayView1D<const Vector4>& nodeMargins, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const LayouterDataHandle firstExplicitSnapLayout, const Containers::StridedArrayView1D<const NodeHandle>& layoutNodes, const Containers::StridedArrayView1D<const SnapLayoutFlags>& flags, const Containers::StridedArrayView1D<const Snaps>& explicitSnaps, const Containers::StridedArrayView1D<const LayouterDataHandle>& nextLayout) {
    CORRADE_INTERNAL_ASSERT(
        nodeSizes.size() == nodeMargins.size() &&
        firstExplicitSnapLayout != LayouterDataHandle::Null &&
        flags.size() == layoutNodes.size() &&
        explicitSnaps.size() == layoutNodes.size() &&
        nextLayout.size() == layoutNodes.size());

    LayouterDataHandle explicitSnapLayout = firstExplicitSnapLayout;
    Vector2 maxSizeWithMarginPadding;
    do {
        const UnsignedInt layoutId = layouterDataHandleId(explicitSnapLayout);

        /* Consider the layout only if it's snapped to a parent and not a
           sibling */
        if(flags[layoutId] >= SnapLayoutFlagExplicitSnapToParent) {
            const Snaps snap = explicitSnaps[layoutId];
            const UnsignedInt nodeId = nodeHandleId(layoutNodes[layoutId]);
            const Vector4 margin = nodeMargins[nodeId];
            const Vector2 size = nodeSizes[nodeId];

            /* If horizontal overflow is ignored, nothing to do here */
            if(!(parentFlags >= SnapLayoutFlag::IgnoreOverflowX)) {
                Float width = size.x();

                /* If all horizontal padding is ignored, use just the width
                   alone */
                if(snap >= Snap::NoPadX) {
                    /* Nothing */

                /* Otherwise, if margin is propagated outside, add just parent
                   horizontal padding alone */
                /** @todo collect the total margin and return it for
                    propagation, if such use case becomes important. Need to
                    differ between side/center/fill snapping for each, which
                    makes it rather complex for testing. */
                } else if(parentFlags >= SnapLayoutFlag::PropagateMarginX) {
                    width += Math::gather<0, 2>(parentPadding).sum();

                /* Otherwise add max of horizontal margin and padding */
                } else {
                    width += Math::max(Math::gather<0, 2>(margin),
                                       Math::gather<0, 2>(parentPadding)).sum();
                }

                maxSizeWithMarginPadding.x() = Math::max(maxSizeWithMarginPadding.x(), width);
            }

            /* If vertical overflow is ignored, nothing to do here */
            if(!(parentFlags >= SnapLayoutFlag::IgnoreOverflowY)) {
                Float height = size.y();

                /* If all vertical padding is ignored, use just the height
                   alone */
                if(snap >= Snap::NoPadY) {
                    /* Nothing */

                /* Otherwise, if margin is propagated outside, add just parent
                   vertical padding alone */
                /** @todo collect the total margin and return it for
                    propagation, if such use case becomes important. Need to
                    differ between side/center/fill snapping for each, which
                    makes it rather complex for testing. */
                } else if(parentFlags >= SnapLayoutFlag::PropagateMarginY) {
                    height += Math::gather<1, 3>(parentPadding).sum();

                /* Otherwise add max of vertical margin and padding */
                } else {
                    height += Math::max(Math::gather<1, 3>(margin),
                                        Math::gather<1, 3>(parentPadding)).sum();
                }

                maxSizeWithMarginPadding.y() = Math::max(maxSizeWithMarginPadding.y(), height);
            }
        }

        explicitSnapLayout = nextLayout[layoutId];
    } while(explicitSnapLayout != firstExplicitSnapLayout);

    return maxSizeWithMarginPadding;
}

/* Calculates the actual layout size, padding and margin from the layout and
   node properties and output from the childLayoutSizeMargin() above. Is a
   separate function because that makes it easier to test, and
   childLayoutSizeMargin() doesn't need to be called at all if flags contain
   IgnoreOverflow. */
Containers::Triple<Vector2, Vector4, Vector4> layoutSizePaddingMargin(const SnapLayoutFlags flags, const Snaps childSnap, const Vector2& nodeSize, const Vector4& nodePadding, const Vector4& nodeMargin, const Vector2& childLayoutSize, const Vector4& childLayoutMargin) {
    /* Calculate actual layout padding:
        - It's zero on given side if padding is ignored in matching direction
        - It's same as node padding in given direction if child layout overflow
          is ignored in that direction, or if the child layout margin is
          propagated outside
        - Otherwise it's the max of node padding and child layout margin in
          given direction */
    Vector4 layoutPadding{NoInit};
    Math::scatterInto<0, 2>(layoutPadding, childSnap >= Snap::NoPadX ?
        Vector2{} : Math::max(
            Math::gather<0, 2>(nodePadding),
            flags & (SnapLayoutFlag::IgnoreOverflowX|SnapLayoutFlag::PropagateMarginX) ?
                Vector2{} :
                Math::gather<0, 2>(childLayoutMargin)));
    Math::scatterInto<1, 3>(layoutPadding, childSnap >= Snap::NoPadY ?
        Vector2{} : Math::max(
            Math::gather<1, 3>(nodePadding),
            flags & (SnapLayoutFlag::IgnoreOverflowY|SnapLayoutFlag::PropagateMarginY) ?
                Vector2{} :
                Math::gather<1, 3>(childLayoutMargin)));

    /* Minimal layout size is the actual child layout size with padding from
       both sides added */
    const Vector2 minLayoutSize = childLayoutSize +
        Math::gather<0, 1>(layoutPadding) +
        Math::gather<2, 3>(layoutPadding);

    /* Calculate actual layout size:
        - It's same as node size in given direction if child layout overflow is
          ignored in that direction
        - Otherwise it's the max of node size and min layout size */
    const Vector2 layoutSize{
        flags >= SnapLayoutFlag::IgnoreOverflowX ?
            nodeSize.x() : Math::max(nodeSize.x(), minLayoutSize.x()),
        flags >= SnapLayoutFlag::IgnoreOverflowY ?
            nodeSize.y() : Math::max(nodeSize.y(), minLayoutSize.y())
    };

    /* Calculate actual layout margin. Initially it's the node margin. If
       propagating child layout margin, it has to take into account the extra
       available space between the parent layout padding and child layout. The
       diagrams below show parent and child layouts, parent layout padding
       denoted with # and child layout margin denoted with @, and the extra
       available space marked with ^^ arrows.

              +---------------+     When centering (i.e., Snaps{}), the child
              |              #|     layout is placed with the extra available
          @@@@@@@+--------+@@@@@    space divided in half between left and
          @   |  | Center |  #|@    right side, which is thus additionally
          @   |  |        |  #|@    subtracted from the child margins, along
               ^^          ^^       with parent padding.

              +---------------+     With left align, the left-side margin is
              |              #|     overflows outside in full, and the whole
        @@@@@@@+--------+@@@@@|     extra available space is subtracted from
        @     ||  Left  |    @|     the right-side margin. In this particular
        @     ||        |    @|     diagram the right-side child margin ends up
                         ^^^^       not overflowing the parent at all.

              +---------------+     With right align it's the opposite of
              |              #|     above, the right-side margin overflows
            @@@@@@@+--------+@@@@@  outside in full (again with parent padding
            @ |    | Right  |#|  @  subtracted), and the whole extra available
            @ |    |        |#|  @  space is subtracted from the left-side
               ^^^^                 margin.

              +---------------+     When filling, the child layout fills the
              |              #|     whole parent layout so there's no extra
        @@@@@@@+------------+@@@@@  available space to subtract from anywhere.
        @     ||    Fill    |#|  @  Both margins thus overflow outside in full,
        @     ||            |#|  @  with parent padding subtracted.

       The vertical case is equivalent, just with Top/Bottom instead of
       Left/Right. */
    Vector4 layoutMargin = nodeMargin;
    if((flags & SnapLayoutFlag::PropagateMargin) && !(childSnap >= Snap::NoPad)) {
        const Snaps snapNoNoPad = childSnap & ~Snap::NoPad;
        /* Horizontal margin propagation. If we're ignoring all horizontal
           padding, there's nothing left to do, and the parent node horizontal
           margin stays unchanged. */
        if(flags >= SnapLayoutFlag::PropagateMarginX && !(childSnap >= Snap::NoPadX)) {
            /* The extra available width is between the final layout width
               excluding the parent node horizontal padding and the child
               layout size. It cannot become negative because the layout size
               is always at least as large as what the child layout needs. */
            const Float extraAvailableWidth = layoutSize.x() - childLayoutSize.x() - nodePadding[0] - nodePadding[2];
            CORRADE_INTERNAL_DEBUG_ASSERT(extraAvailableWidth >= 0.0f);

            /* Margin overflow is the horizontal child layout margin with
               parent node horizontal padding subtracted. It's fine if it
               becomes negative. */
            Vector2 horizontalMarginOverflow = Math::gather<0, 2>(childLayoutMargin) - Math::gather<0, 2>(nodePadding);

            /* Further subtract the extra available space from the overflow,
               according to the diagrams above. Again this can make the
               overflow negative but that's fine. */

            /* Vertical centered snapping. The InsideX, if specified, is
               redundant. */
            if((snapNoNoPad|Snap::InsideX) == (Snap::Bottom|Snap::InsideX) ||
               (snapNoNoPad|Snap::InsideX) == (Snap::Top|Snap::InsideX))
                horizontalMarginOverflow -= Vector2{extraAvailableWidth*0.5f};
            /* Vertical snapping to the left or horizontal snapping starting
               from the left */
            else if(snapNoNoPad == (Snap::BottomLeft|Snap::InsideX) ||
                    snapNoNoPad == (Snap::TopLeft|Snap::InsideX) ||
                    /* Here InsideY is redundant if specified, and FillY is
                       treated the same as w/o */
                    (snapNoNoPad|Snap::FillY|Snap::InsideY) == (Snap::Right|Snap::FillY|Snap::InsideY) ||
                    snapNoNoPad == (Snap::TopRight|Snap::InsideY) ||
                    snapNoNoPad == (Snap::BottomRight|Snap::InsideY))
                horizontalMarginOverflow[1] -= extraAvailableWidth;
            /* Vertical snapping to the right or horizontal snapping starting
               from the right */
            else if(snapNoNoPad == (Snap::BottomRight|Snap::InsideX) ||
                    snapNoNoPad == (Snap::TopRight|Snap::InsideX) ||
                    /* Here InsideY is redundant if specified, and FillY is
                       treated the same as w/o */
                    (snapNoNoPad|Snap::FillY|Snap::InsideY) == (Snap::Left|Snap::FillY|Snap::InsideY) ||
                    snapNoNoPad == (Snap::TopLeft|Snap::InsideY) ||
                    snapNoNoPad == (Snap::BottomLeft|Snap::InsideY))
                horizontalMarginOverflow[0] -= extraAvailableWidth;
            /* Vertical filled snapping doesn't need any further adjustment.
               The InsideX, if specified, is redundant. */
            else if((snapNoNoPad|Snap::InsideX) != (Snap::Top|Snap::FillX|Snap::InsideX) &&
                    (snapNoNoPad|Snap::InsideX) != (Snap::Bottom|Snap::FillX|Snap::InsideX))
                CORRADE_INTERNAL_DEBUG_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */

            /* The final margin is the maximum of node margin and the
               overflow. Assuming the node margin is always at least 0, this
               handles the case where the overflow would be negative. */
            Math::scatterInto<0, 2>(layoutMargin, Math::max(
                Math::gather<0, 2>(layoutMargin),
                horizontalMarginOverflow));
        }

        /* Vertical margin propagation, equivalent to above */
        if(flags >= SnapLayoutFlag::PropagateMarginY && !(childSnap >= Snap::NoPadY)) {
            const Float extraAvailableHeight = layoutSize.y() - childLayoutSize.y() - nodePadding[1] - nodePadding[3];
            CORRADE_INTERNAL_DEBUG_ASSERT(extraAvailableHeight >= 0.0f);

            Vector2 verticalMarginOverflow = Math::gather<1, 3>(childLayoutMargin) - Math::gather<1, 3>(nodePadding);

            /* Horizontal centered snapping. The InsideY, if specified, is
               redundant. */
            if((snapNoNoPad|Snap::InsideY) == (Snap::Left|Snap::InsideY) ||
               (snapNoNoPad|Snap::InsideY) == (Snap::Right|Snap::InsideY))
                verticalMarginOverflow -= Vector2{extraAvailableHeight*0.5f};
            /* Horizontal snapping to the top or vertical snapping starting
               from the top */
            else if(snapNoNoPad == (Snap::TopLeft|Snap::InsideY) ||
                    snapNoNoPad == (Snap::TopRight|Snap::InsideY) ||
                    /* Here InsideX is redundant if specified, and FillX is
                       treated the same as w/o */
                    (snapNoNoPad|Snap::FillX|Snap::InsideX) == (Snap::Bottom|Snap::FillX|Snap::InsideX) ||
                    snapNoNoPad == (Snap::BottomLeft|Snap::InsideX) ||
                    snapNoNoPad == (Snap::BottomRight|Snap::InsideX))
                verticalMarginOverflow[1] -= extraAvailableHeight;
            /* Horizontal snapping to the bottom or vertical snapping starting
               from the bottom */
            else if(snapNoNoPad == (Snap::BottomLeft|Snap::InsideY) ||
                    snapNoNoPad == (Snap::BottomRight|Snap::InsideY) ||
                    /* Here InsideX is redundant if specified, and FillX is
                       treated the same as w/o */
                    (snapNoNoPad|Snap::FillX|Snap::InsideX) == (Snap::Top|Snap::FillX|Snap::InsideX) ||
                    snapNoNoPad == (Snap::TopLeft|Snap::InsideX) ||
                    snapNoNoPad == (Snap::TopRight|Snap::InsideX))
                verticalMarginOverflow[0] -= extraAvailableHeight;
            /* Horizontal filled snapping doesn't need any further adjustment.
               The InsideY, if specified, is redundant. */
            else if((snapNoNoPad|Snap::InsideY) != (Snap::Left|Snap::FillY|Snap::InsideY) &&
                    (snapNoNoPad|Snap::InsideY) != (Snap::Right|Snap::FillY|Snap::InsideY))
                CORRADE_INTERNAL_DEBUG_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */

            Math::scatterInto<1, 3>(layoutMargin, Math::max(
                Math::gather<1, 3>(layoutMargin),
                verticalMarginOverflow));
        }

        /* Just a sanity check that a negative overflow really didn't cause the
           parent node margin to shrink */
        CORRADE_INTERNAL_DEBUG_ASSERT((layoutMargin >= nodeMargin).all());
    }

    return {layoutSize, layoutPadding, layoutMargin};
}

/* Used by SnapLayouter::setChildSnapInternal() but also tests that verify
   childLayoutSizeMargin() together with snap(), so has to be here. Empty set
   returned is a failure that should be handled by the caller. */
Snaps firstChildSnap(const Snaps childSnap) {
    /* Make the first child snap inherit also any padding. Since it's snapping
       to a parent, Snap::Inside is implicit and doesn't need to be set. */
    const Snaps out = (childSnap & Snap::NoPad)|Snap::Inside;
    const Snaps snapNoNoPad = childSnap & ~Snap::NoPad;
    /* InsideY is implicit in these, allow it to be specified explicitly */
    if((snapNoNoPad|Snap::InsideY) == (Snap::Left|Snap::InsideY) ||
       (snapNoNoPad|Snap::InsideY) == (Snap::Left|Snap::FillY|Snap::InsideY))
        return out|Snap::Right|(snapNoNoPad & Snap::FillY);
    if((snapNoNoPad|Snap::InsideY) == (Snap::Right|Snap::InsideY) ||
       (snapNoNoPad|Snap::InsideY) == (Snap::Right|Snap::FillY|Snap::InsideY))
        return out|Snap::Left|(snapNoNoPad & Snap::FillY);
    /* InsideX is implicit in these, allow it to be specified explicitly */
    if((snapNoNoPad|Snap::InsideX) == (Snap::Top|Snap::InsideX) ||
       (snapNoNoPad|Snap::InsideX) == (Snap::Top|Snap::FillX|Snap::InsideX))
        return out|Snap::Bottom|(snapNoNoPad & Snap::FillX);
    if((snapNoNoPad|Snap::InsideX) == (Snap::Bottom|Snap::InsideX) ||
       (snapNoNoPad|Snap::InsideX) == (Snap::Bottom|Snap::FillX|Snap::InsideX))
        return out|Snap::Top|(snapNoNoPad & Snap::FillX);
    if(snapNoNoPad == (Snap::TopLeft|Snap::InsideX))
        return out|Snap::BottomLeft;
    if(snapNoNoPad == (Snap::TopLeft|Snap::InsideY))
        return out|Snap::TopRight;
    if(snapNoNoPad == (Snap::TopRight|Snap::InsideX))
        return out|Snap::BottomRight;
    if(snapNoNoPad == (Snap::TopRight|Snap::InsideY))
        return out|Snap::TopLeft;
    if(snapNoNoPad == (Snap::BottomLeft|Snap::InsideX))
        return out|Snap::TopLeft;
    if(snapNoNoPad == (Snap::BottomLeft|Snap::InsideY))
        return out|Snap::BottomRight;
    if(snapNoNoPad == (Snap::BottomRight|Snap::InsideX))
        return out|Snap::TopRight;
    if(snapNoNoPad == (Snap::BottomRight|Snap::InsideY))
        return out|Snap::BottomLeft;
    return {};
}

/* Snapping inside given direction is either explicitly or if either filling or
   centering in this direction */
BitVector2 snapInside(Snaps snap) {
    return BitVector2{UnsignedByte(
        ((snap & Snap::InsideX) || !(snap & Snap::Left) == !(snap & Snap::Right) ? 1 : 0)|
        ((snap & Snap::InsideY) || !(snap & Snap::Bottom) == !(snap & Snap::Top) ? 2 : 0)
    )};
}

/* Snaps rectangle of given `size` to a rectangle defined by `targetOffset`,
   `targetSize`, `targetPadding` inside the target rectangle in order left,
   top, right, bottom, `targetMargin` outside the target rectangle, and
   `margin` outside of the to-be-snapped rectangle */
Containers::Pair<Vector2, Vector2> snap(Snaps snap, const Vector2& targetOffset, const Vector2& targetSize, const Vector4& targetPadding, const Vector4& targetMargin, const Vector4& margin, const Vector2& size) {
    const BitVector2 snapInside = Implementation::snapInside(snap);

    /* Spacing in given direction is ignored either explicitly or if snapping
       inside in this direction and snapping outside in the opposite direction
       (that means also no center or fill in the opposite direction) */
    const BitVector2 ignoreSpace{UnsignedByte(
        ((snap & Snap::NoPadX) || (snapInside[0] && !snapInside[1] && (!(snap & Snap::Bottom) != !(snap & Snap::Top))) ? 1 : 0)|
        ((snap & Snap::NoPadY) || (snapInside[1] && !snapInside[0] && (!(snap & Snap::Left) != !(snap & Snap::Right))) ? 2 : 0)
    )};

    /*           +----+
              max(tmT, mB)
               +--------+
       -+ max( |        | max( +-
        | tmL, |        | tmR, |
       -+  mR) |        |  mL) +-
               +--------+
              max(rmB, mT)
                 +----+

       Order of target margin (tm*) is left, top, right, bottom (0123), so it
       has to take the margin (m*) in right, bottom, left, top (2301). */
    const Vector4 maxMargin = Math::max(
        targetMargin,
        Math::gather<2, 3, 0, 1>(margin));

    /* +----------------------+
       |     max(tpT, mT)     |
       | max( +--------+ max( |
       | tpL, |        | tpR, |
       |  mL) +--------+  mR) |
       |     max(tpB, mB)     |
       +----------------------+

       For target padding (tp*) and margin (m*) the sides are the same. */

    /* These take both target padding and margin into account and are used when
       aligning to sides or filling */
    const Vector4 maxPadding = Math::max(
        targetPadding,
        margin);
    const Vector2 targetMaxPaddedMin = targetOffset - Math::lerp(
        Math::lerp(Math::gather<0, 1>(maxMargin),
                  -Math::gather<0, 1>(maxPadding), snapInside),
        {},
        ignoreSpace);
    const Vector2 targetMaxPaddedMax = targetOffset + targetSize + Math::lerp(
        Math::lerp(Math::gather<2, 3>(maxMargin),
                  -Math::gather<2, 3>(maxPadding), snapInside),
        {},
        ignoreSpace);

    /* These take only the target padding into account when centering inside,
       ignoring margin, and taking into account neither margin nor padding when
       centering outside. The reason is that uneven margin would lead to uneven
       centering with no clear reason why, especially if there's enough room on
       either side to fit.*/
    const Vector2 targetPaddedMin = targetOffset - Math::lerp(
        Math::lerp(Math::gather<0, 1>(maxMargin),
                  -Math::gather<0, 1>(targetPadding), snapInside),
        {},
        ignoreSpace);
    const Vector2 targetPaddedMax = targetOffset + targetSize + Math::lerp(
        Math::lerp(Math::gather<2, 3>(maxMargin),
                  -Math::gather<2, 3>(targetPadding), snapInside),
        {},
        ignoreSpace);

    /* Enlarge to target width */
    Float sizeX, offsetX;
    if(snap >= (Snap::Left|Snap::Right)) {
        sizeX = targetMaxPaddedMax.x() - targetMaxPaddedMin.x();
        offsetX = targetMaxPaddedMin.x();

    /* Keep the original width */
    } else {
        sizeX = size.x();

        /* Snap to left */
        if(snap & Snap::Left) {
            if(snap & Snap::InsideX)
                offsetX = targetMaxPaddedMin.x();
            else
                offsetX = targetMaxPaddedMin.x() - size.x();

        /* Snap to right */
        } else if(snap & Snap::Right) {
            if(snap & Snap::InsideX)
                offsetX = targetMaxPaddedMax.x() - size.x();
            else
                offsetX = targetMaxPaddedMax.x();

        /* Snap to horizontal center */
        } else offsetX = (targetPaddedMin.x() + targetPaddedMax.x())*0.5f - size.x()*0.5f;
    }

    /* Enlarge to target height */
    Float sizeY, offsetY;
    if(snap >= (Snap::Top|Snap::Bottom)) {
        sizeY = targetMaxPaddedMax.y() - targetMaxPaddedMin.y();
        offsetY = targetMaxPaddedMin.y();

    /* Keep the original width */
    } else {
        sizeY = size.y();

        /* Snap to top */
        if(snap & Snap::Top) {
            if(snap & Snap::InsideY)
                offsetY = targetMaxPaddedMin.y();
            else
                offsetY = targetMaxPaddedMin.y() - size.y();

        /* Snap to bottom */
        } else if(snap & Snap::Bottom) {
            if(snap & Snap::InsideY)
                offsetY = targetMaxPaddedMax.y() - size.y();
            else
                offsetY = targetMaxPaddedMax.y();

        /* Snap to vertical center */
        } else offsetY = (targetPaddedMin.y() + targetPaddedMax.y())*0.5f - size.y()*0.5f;
    }

    return {{offsetX, offsetY}, {sizeX, sizeY}};
}

/* The `layoutIds` array gets filled with layout IDs in the following order:

    -   the first item is always -1
    -   children / nested layout IDs are always after their parent / target in
        the `layoutIds` array in a breadth-first order

   The `childrenOffsets` and `children` arrays are temporary storage. The
   `childrenOffsets` array has to be zero-initialized. Others don't need to
   be. */
void orderLayoutsBreadthFirstInto(const Containers::StridedArrayView1D<const LayouterDataHandle>& layoutParentsOrTargets, const Containers::StridedArrayView1D<const LayouterDataHandle>& layoutFirstChildren, const Containers::StridedArrayView1D<const LayouterDataHandle>& layoutFirstExplicitSnaps, const Containers::StridedArrayView1D<const LayouterDataHandle>& layoutNext, const Containers::ArrayView<UnsignedInt> childrenOffsets, const Containers::ArrayView<UnsignedInt> children, const Containers::ArrayView<Int> layoutIds) {
    CORRADE_INTERNAL_ASSERT(
        layoutFirstChildren.size() == layoutParentsOrTargets.size() &&
        layoutFirstExplicitSnaps.size() == layoutParentsOrTargets.size() &&
        layoutNext.size() == layoutParentsOrTargets.size() &&
        childrenOffsets.size() == layoutParentsOrTargets.size() + 2 &&
        children.size() == layoutParentsOrTargets.size() &&
        layoutIds.size() == layoutParentsOrTargets.size() + 1);

    /* Children offset for each layout including layouts with no parents or
       targets. Handle generation is ignored here, so free and invalid layouts
       are counted as well.

       First calculate the count of children / dependent layouts for each,
       skipping the first element (parent-/target-less layout is at index 1,
       first layout at index 2) ... */
    for(const LayouterDataHandle parentOrTarget: layoutParentsOrTargets) {
        if(parentOrTarget != LayouterDataHandle::Null)
            ++childrenOffsets[layouterDataHandleId(parentOrTarget) + 1];
        else
            ++childrenOffsets[0];
    }

    /* ... then convert the counts to a running offset. Now
       `[childrenOffsets[i + 1], childrenOffsets[i + 2])` is a range in which
       the `children` array contains a list of children for layout `i`. */
    {
        UnsignedInt offset = 0;
        for(UnsignedInt& i: childrenOffsets) {
            const UnsignedInt nextOffset = offset + i;
            i = offset;
            offset = nextOffset;
        }
        childrenOffsets.back() = offset;
        CORRADE_INTERNAL_ASSERT(offset == layoutParentsOrTargets.size());
    }

    /* Go through the layout list again. For each layout iterate its children
       and dependent layouts, filling the children list. Additionally put all
       layouts that don't have a parent / target at the start of the children
       list. */
    {
        UnsignedInt rootOffset = 0;
        for(std::size_t i = 0; i != layoutParentsOrTargets.size(); ++i) {
            const LayouterDataHandle parentOrTarget = layoutParentsOrTargets[i];

            /* If the layout has no parent or target, add it to the root layout
               list. Such layouts should not be part of any list. */
            if(parentOrTarget == LayouterDataHandle::Null) {
                children[rootOffset++] = i;
                CORRADE_INTERNAL_DEBUG_ASSERT(layoutNext[i] == LayouterDataHandle::Null);
            }

            /* If the layout has any children, put them all into the list in
               their respective order. */
            UnsignedInt childOffset = childrenOffsets[i + 1];
            const LayouterDataHandle firstChild = layoutFirstChildren[i];
            if(firstChild != LayouterDataHandle::Null) {
                LayouterDataHandle layout = firstChild;
                do {
                    const UnsignedInt layoutId = layouterDataHandleId(layout);
                    children[childOffset++] = layoutId;
                    layout = layoutNext[layoutId];
                } while(layout != firstChild);
            }

            /* If the layout has any dependent layouts, put them into the
               list. The order doesn't matter in this case. */
            const LayouterDataHandle firstExplicitSnap = layoutFirstExplicitSnaps[i];
            if(firstExplicitSnap != LayouterDataHandle::Null) {
                LayouterDataHandle layout = firstExplicitSnap;
                do {
                    const UnsignedInt layoutId = layouterDataHandleId(layout);
                    children[childOffset++] = layoutId;
                    layout = layoutNext[layoutId];
                } while(layout != firstExplicitSnap);
            }

            CORRADE_INTERNAL_DEBUG_ASSERT(childOffset == childrenOffsets[i + 2]);
        }

        CORRADE_INTERNAL_DEBUG_ASSERT(rootOffset == childrenOffsets[1]);
    }

    /* Go breadth-first (so we have items sharing the same parent next to each
       other, but that doesn't really matter, it's simpler than depth-first)
       and build a layout IDs list where a parent / target layout ID is always
       before its children. */
    std::size_t outputOffset = 0;
    layoutIds[0] = -1;
    for(std::size_t i = 0; i != outputOffset + 1; ++i) {
        const Int layoutId = layoutIds[i];
        for(std::size_t j = childrenOffsets[layoutId + 1], jMax = childrenOffsets[layoutId + 2]; j != jMax; ++j) {
            layoutIds[outputOffset + 1] = children[j];
            ++outputOffset;
        }
    }
    CORRADE_INTERNAL_ASSERT(outputOffset == layoutParentsOrTargets.size());
}

}}}}

#endif
