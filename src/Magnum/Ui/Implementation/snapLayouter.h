#ifndef Magnum_Ui_Implementation_snapLayouter_h
#define Magnum_Ui_Implementation_snapLayouter_h
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

#include <Magnum/Math/Functions.h>
#include <Magnum/Math/Swizzle.h>
#include <Magnum/Math/Vector4.h>

namespace Magnum { namespace Ui { namespace Implementation { namespace {

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

       Order of target margin (tm*) is left, top, right, bottom (xyzw), so it
       has to take the margin (m*) in right, bottom, left, top (zwxy). */
    const Vector4 maxMargin = Math::max(
        targetMargin,
        Math::gather<'z', 'w', 'x', 'y'>(margin));

    /* +----------------------+
       |     max(tpT, mT)     |
       | max( +--------+ max( |
       | tpL, |        | tpR, |
       |  mL) +--------+  mR) |
       |     max(tpB, mB)     |
       +----------------------+

       For target padding (tp*) and margin (m*) the sides are the same. */
    const Vector4 maxPadding = Math::max(
        targetPadding,
        margin);

    const Vector2 targetPaddedMin = targetOffset - Math::lerp(
        Math::lerp(maxMargin.xy(),
                  -maxPadding.xy(), snapInside),
        {},
        ignoreSpace);
    const Vector2 targetPaddedMax = targetOffset + targetSize + Math::lerp(
        Math::lerp(Math::gather<'z', 'w'>(maxMargin),
                  -Math::gather<'z', 'w'>(maxPadding), snapInside),
        {},
        ignoreSpace);

    /* Enlarge to target width */
    Float sizeX, offsetX;
    if(snap >= (Snap::Left|Snap::Right)) {
        sizeX = targetPaddedMax.x() - targetPaddedMin.x();
        offsetX = targetPaddedMin.x();

    /* Keep the original width */
    } else {
        sizeX = size.x();

        /* Snap to left */
        if(snap & Snap::Left) {
            if(snap & Snap::InsideX)
                offsetX = targetPaddedMin.x();
            else
                offsetX = targetPaddedMin.x() - size.x();

        /* Snap to right */
        } else if(snap & Snap::Right) {
            if(snap & Snap::InsideX)
                offsetX = targetPaddedMax.x() - size.x();
            else
                offsetX = targetPaddedMax.x();

        /* Snap to horizontal center */
        } else offsetX = (targetPaddedMin.x() + targetPaddedMax.x())*0.5f - size.x()*0.5f;
    }

    /* Enlarge to target height */
    Float sizeY, offsetY;
    if(snap >= (Snap::Top|Snap::Bottom)) {
        sizeY = targetPaddedMax.y() - targetPaddedMin.y();
        offsetY = targetPaddedMin.y();

    /* Keep the original width */
    } else {
        sizeY = size.y();

        /* Snap to top */
        if(snap & Snap::Top) {
            if(snap & Snap::InsideY)
                offsetY = targetPaddedMin.y();
            else
                offsetY = targetPaddedMin.y() - size.y();

        /* Snap to bottom */
        } else if(snap & Snap::Bottom) {
            if(snap & Snap::InsideY)
                offsetY = targetPaddedMax.y() - size.y();
            else
                offsetY = targetPaddedMax.y();

        /* Snap to vertical center */
        } else offsetY = (targetPaddedMin.y() + targetPaddedMax.y())*0.5f - size.y()*0.5f;
    }

    return {{offsetX, offsetY}, {sizeX, sizeY}};
}

/* Takes the `nodeIdsBreadthFirst` array (including the first -1 item)
   populated by orderNodesBreadthFirstInto() and fills a prefix of `layoutIds`
   in an order that matches the breadth-first order of layout target nodes.
   Returns the prefix of the `layoutIds` that got populated.

   Assumes that the masked layoutTargets are unique, which should hold because
   AbstractUserInterface has the same constraing. The `layoutOffsets` and
   `layouts` arrays are temporary storage, `layoutOffsets` is expected to be
   zero-initialized. */
std::size_t orderLayoutsBreadthFirstInto(const Containers::BitArrayView layoutIdsToUpdate, const Containers::StridedArrayView1D<const NodeHandle>& layoutTargets, const Containers::ArrayView<const Int> nodeIdsBreadthFirst, const Containers::ArrayView<UnsignedInt> layoutOffsets, const Containers::ArrayView<UnsignedInt> layouts, const Containers::ArrayView<UnsignedInt> layoutIds) {
    CORRADE_INTERNAL_ASSERT(
        layoutTargets.size() == layoutIdsToUpdate.size() &&
        layoutOffsets.size() == nodeIdsBreadthFirst.size() + 1 &&
        layouts.size() == layoutIdsToUpdate.size() &&
        layoutIds.size() == layoutIdsToUpdate.size());

    /* First calculate the count of layouts targeting each node, skipping the
       first element (targeting the UI itself is at index 1, targeting first
       node at index 2) ... */
    /** @todo some way to iterate set bits */
    for(std::size_t i = 0; i != layoutIdsToUpdate.size(); ++i) {
        if(!layoutIdsToUpdate[i])
            continue;
        const NodeHandle target = layoutTargets[i];
        ++layoutOffsets[target == NodeHandle::Null ? 1 : nodeHandleId(target) + 2];
    }

    /* ... then convert the counts to a running offset. Now
       `[layoutOffsets[i + 2], layoutOffsets[i + 3])` is a range in which
       the `layouts` array contains a list of layouts targeting node `i`. The
       last element (containing the end offset) is omitted at this step. */
    {
        UnsignedInt offset = 0;
        for(UnsignedInt& i: layoutOffsets) {
            const UnsignedInt nextOffset = offset + i;
            i = offset;
            offset = nextOffset;
        }
    }

    /* Go through the layout list again, convert that to layout ranges. The
       `layoutOffsets` array gets shifted by one element by the process, thus
       now `[layoutOffsets[i + 1], layoutOffsets[i + 2])` is a range in which
       the `layouts` array below contains a list of layouts targeting node `i`.
       The last filled array element is now containing the end offset. */
    /** @todo some way to iterate set bits */
    for(std::size_t i = 0; i != layoutIdsToUpdate.size(); ++i) {
        if(!layoutIdsToUpdate[i])
            continue;
        const NodeHandle target = layoutTargets[i];
        layouts[layoutOffsets[target == NodeHandle::Null ? 1 : nodeHandleId(target) + 2]++] = i;
    }

    /* Go through the breadth-first node order and put each that has a layout
       assigned to the output array */
    std::size_t offset = 0;
    for(const Int nodeId: nodeIdsBreadthFirst) {
        for(UnsignedInt i = layoutOffsets[nodeId + 1], end = layoutOffsets[nodeId + 2]; i != end; ++i)
            layoutIds[offset++] = layouts[i];
    }

    return offset;
}

}}}}

#endif
