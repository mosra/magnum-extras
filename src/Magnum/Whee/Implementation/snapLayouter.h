#ifndef Magnum_Whee_Implementation_snapLayouter_h
#define Magnum_Whee_Implementation_snapLayouter_h
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

#include <Magnum/Math/Functions.h>
#include <Magnum/Math/Swizzle.h>
#include <Magnum/Math/Vector4.h>

namespace Magnum { namespace Whee { namespace Implementation { namespace {

/* Snaps rectangle of given `size` to a rectangle defined by `referenceOffset`,
   `referenceSize`, `padding` inside in order left, top, right, bottom and
   `margin` outside */
Containers::Pair<Vector2, Vector2> snap(Snaps snap, const Vector2& referenceOffset, const Vector2& referenceSize, const Vector4& padding, const Vector2& margin, const Vector2& size) {
    /* Snapping inside given direction is either explicitly or if either
       filling or centering in this direction */
    const BitVector2 snapInside{UnsignedByte(
        ((snap & Snap::InsideX) || !(snap & Snap::Left) == !(snap & Snap::Right) ? 1 : 0)|
        ((snap & Snap::InsideY) || !(snap & Snap::Bottom) == !(snap & Snap::Top) ? 2 : 0)
    )};

    /* Spacing in given direction is ignored either explicitly or if snapping
       inside in this direction and snapping outside in the opposite direction
       (that means also no center or fill in the opposite direction) */
    const BitVector2 ignoreSpace{UnsignedByte(
        ((snap & Snap::NoSpaceX) || (snapInside[0] && !snapInside[1] && (!(snap & Snap::Bottom) != !(snap & Snap::Top))) ? 1 : 0)|
        ((snap & Snap::NoSpaceY) || (snapInside[1] && !snapInside[0] && (!(snap & Snap::Left) != !(snap & Snap::Right))) ? 2 : 0)
    )};

    const Vector2 referencePaddedMin = referenceOffset - Math::lerp(
        Math::lerp(margin, -padding.xy(), snapInside),
        {},
        ignoreSpace);
    const Vector2 referencePaddedMax = referenceOffset + referenceSize + Math::lerp(
        Math::lerp(margin, -Math::gather<'z', 'w'>(padding), snapInside),
        {},
        ignoreSpace);

    /* Enlarge to reference width */
    Float sizeX, offsetX;
    if(snap >= (Snap::Left|Snap::Right)) {
        sizeX = referencePaddedMax.x() - referencePaddedMin.x();
        offsetX = referencePaddedMin.x();

    /* Keep the original width */
    } else {
        sizeX = size.x();

        /* Snap to left */
        if(snap & Snap::Left) {
            if(snap & Snap::InsideX)
                offsetX = referencePaddedMin.x();
            else
                offsetX = referencePaddedMin.x() - size.x();

        /* Snap to right */
        } else if(snap & Snap::Right) {
            if(snap & Snap::InsideX)
                offsetX = referencePaddedMax.x() - size.x();
            else
                offsetX = referencePaddedMax.x();

        /* Snap to horizontal center */
        } else offsetX = (referencePaddedMin.x() + referencePaddedMax.x())*0.5f - size.x()*0.5f;
    }

    /* Enlarge to reference height */
    Float sizeY, offsetY;
    if(snap >= (Snap::Top|Snap::Bottom)) {
        sizeY = referencePaddedMax.y() - referencePaddedMin.y();
        offsetY = referencePaddedMin.y();

    /* Keep the original width */
    } else {
        sizeY = size.y();

        /* Snap to top */
        if(snap & Snap::Top) {
            if(snap & Snap::InsideY)
                offsetY = referencePaddedMin.y();
            else
                offsetY = referencePaddedMin.y() - size.y();

        /* Snap to bottom */
        } else if(snap & Snap::Bottom) {
            if(snap & Snap::InsideY)
                offsetY = referencePaddedMax.y() - size.y();
            else
                offsetY = referencePaddedMax.y();

        /* Snap to vertical center */
        } else offsetY = (referencePaddedMin.y() + referencePaddedMax.y())*0.5f - size.y()*0.5f;
    }

    return {{offsetX, offsetY}, {sizeX, sizeY}};
}

}}}}

#endif
