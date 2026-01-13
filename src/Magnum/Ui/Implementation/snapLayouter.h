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
#include <Magnum/Math/Functions.h>
#include <Magnum/Math/Swizzle.h>
#include <Magnum/Math/Vector4.h>

namespace Magnum { namespace Ui { namespace Implementation { namespace {

/* Calculates total size of all child layouts, taking into account their margin
   as well as the parent node padding, and returns also the padding inside that
   node that's needed to be subsequently passed to snap() instead of
   nodePadding to position everything correctly */
Containers::Pair<Vector2, Vector4> childLayoutSizePadding(const Snaps childSnap, const Vector4& nodePadding, const Containers::StridedArrayView1D<const Vector2>& nodeMinSizes, const Containers::StridedArrayView1D<const Vector4>& nodeMargins, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const LayouterDataHandle firstChildLayout, const Containers::StridedArrayView1D<const NodeHandle>& layoutNodes, const Containers::StridedArrayView1D<const LayouterDataHandle>& nextLayout) {
    CORRADE_INTERNAL_ASSERT(
        nodeSizes.size() == nodeMinSizes.size() &&
        nodeMargins.size() == nodeMinSizes.size() &&
        firstChildLayout != LayouterDataHandle::Null &&
        layoutNodes.size() == nextLayout.size());

    /* First figure out padding and margin component indices based on child
       snap. The order is left (0), top (1), right (2), bottom (3). Parent
       padding (pp*) and child margin (cm*) share the same sides and thus the
       same index.

        +--------------------+
        |    ppT / cmT (1)   |
        | ppL +--------+ ppR |
        | cmL |        | cmR |
        | (0) +--------+ (2) |
        |    ppB / cmB (3)   |
        +--------------------+ */
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

    /* Indexed below with indexSizeForward / indexSizeSide, causes the padding
       to be not used in either direction if NoPad is specified. It's an extra
       multiplication but I suspect it's better than even more branching. */
    const Float padMultiplier[]{
        childSnap >= Snap::NoPadX ? 0.0f : 1.0f,
        childSnap >= Snap::NoPadY ? 0.0f : 1.0f
    };

    /* Calculate the size spanning all children along with their paddings and
       margins */
    LayouterDataHandle childLayout = firstChildLayout;
    UnsignedInt childNodeId = nodeHandleId(layoutNodes[layouterDataHandleId(childLayout)]);
    Float previousNodeMarginAfter = 0.0f;
    Float nextMargin = padMultiplier[indexSizeForward]*
        Math::max(nodeMargins[childNodeId][indexBefore],
                  nodePadding[indexBefore]);
    Vector4 outPadding{NoInit};
    outPadding[indexBefore] = nextMargin;
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
        /* Take the max of the actual node size and min size. The same
           operation will be done again later when passing arguments to snap()
           but doing it in a single place would mean iterating all nodes first
           (even those which aren't implicitly snapped children), which is
           likely going to be more expensive due to cache misses than just
           doing one extra max() */
        const Vector2 childNodeSize = Math::max(nodeSizes[childNodeId],
                                                nodeMinSizes[childNodeId]);
        /* For side margins take the max of the actual node margin and parent
           node padding. Before/after margins are combined either with the
           previous/next node margin or with the parent node padding below. */
        const Vector4 childNodeMargin = nodeMargins[childNodeId];
        const Float childNodeMarginSideL = padMultiplier[indexSizeSide]*
            Math::max(childNodeMargin[indexSideL],
                      nodePadding[indexSideL]);
        const Float childNodeMarginSideR = padMultiplier[indexSizeSide]*
            Math::max(childNodeMargin[indexSideR],
                      nodePadding[indexSideR]);

        /* Add the previously calculated next margin and current node size to
           the forward sizes */
        sizeForward += nextMargin + childNodeSize[indexSizeForward];

        /* Max side size is calculated out of individual pieces based on how
           the alignment is done */
        if(calculate == Calculate::SideLeft || calculate == Calculate::Fill)
            maxMarginSideL = Math::max(maxMarginSideL,
                                       childNodeMarginSideL);
        if(calculate == Calculate::SideRight || calculate == Calculate::Fill)
            maxMarginSideR = Math::max(maxMarginSideR,
                                       childNodeMarginSideR);
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
        previousNodeMarginAfter = childNodeMargin[indexAfter];
        childLayout = nextLayout[layouterDataHandleId(childLayout)];
        childNodeId = nodeHandleId(layoutNodes[layouterDataHandleId(childLayout)]);
        nextMargin = padMultiplier[indexSizeForward]*
            Math::max(previousNodeMarginAfter,
                      nodeMargins[childNodeId][indexBefore]);
    } while(childLayout != firstChildLayout);

    /* Calculate the final padding after, which includes the parent node
       padding again, and add it to the forward size */
    outPadding[indexAfter] = padMultiplier[indexSizeForward]*
        Math::max(previousNodeMarginAfter,
                  nodePadding[indexAfter]);
    sizeForward += outPadding[indexAfter];

    /* Figure out the output size and padding. Padding before got filled above
       already. */
    Vector2 outSize{NoInit};
    outSize[indexSizeForward] = sizeForward;
    if(calculate == Calculate::SideLeft) {
        outSize[indexSizeSide] = maxMarginSideL + maxSizeSideExceptMarginL;
        outPadding[indexSideL] = maxMarginSideL;
        outPadding[indexSideR] = maxSizeSideExceptMarginL - maxSizeSideExceptMargin;
    } else if(calculate == Calculate::SideRight) {
        outSize[indexSizeSide] = maxMarginSideR + maxSizeSideExceptMarginR;
        outPadding[indexSideL] = maxSizeSideExceptMarginR - maxSizeSideExceptMargin;
        outPadding[indexSideR] = maxMarginSideR;
    } else if(calculate == Calculate::Center) {
        outSize[indexSizeSide] = maxHalfSizeSideWithMarginL + maxHalfSizeSideWithMarginR;
        outPadding[indexSideL] = maxHalfSizeSideWithMarginL - maxSizeSideExceptMargin*0.5f;
        outPadding[indexSideR] = maxHalfSizeSideWithMarginR - maxSizeSideExceptMargin*0.5f;
    } else if(calculate == Calculate::Fill) {
        outSize[indexSizeSide] = maxMarginSideL + maxSizeSideExceptMargin + maxMarginSideR;
        outPadding[indexSideL] = maxMarginSideL;
        outPadding[indexSideR] = maxMarginSideR;
    } else CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */

    return {outSize, outPadding};
}

/* Used by SnapLayouter::setChildSnapInternal() but also tests that verify
   childLayoutSizePadding() together with snap(), so has to be here. Empty set
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
