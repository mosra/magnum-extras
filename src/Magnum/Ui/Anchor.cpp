/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022 Vladimír Vondruš <mosra@centrum.cz>

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

#include "Anchor.h"

#include "Magnum/Ui/BasicPlane.h"
#include "Magnum/Ui/BasicUserInterface.h"
#include "Magnum/Ui/Widget.h"

namespace Magnum { namespace Ui {

namespace {

static_assert(UnsignedByte(Snap::InsideX) == 1 << 4, "");
static_assert(UnsignedByte(Snap::InsideY) == 1 << 5, "");
static_assert(UnsignedByte(Snap::NoSpaceX) == 1 << 6, "");
static_assert(UnsignedByte(Snap::NoSpaceY) == 1 << 7, "");

inline Math::BitVector<2> snapInside(Snaps snaps) {
    return (UnsignedByte(snaps) & UnsignedByte(Snap::InsideX|Snap::InsideY)) >> 4;
}

}

namespace Implementation {

Range2D anchorRect(Snaps snaps, const Range2D& referenceRect, const Range2D& referencePadding, const Vector2& referenceMargin, const Range2D& rect) {
    Vector2 size{rect.size()};
    Vector2 position;

    /* Snapping inside given direction is either explicitly or if either
       filling or centering in this direction */
    Math::BitVector<2> snapInside = Ui::snapInside(snaps);
    snapInside.set(0, snapInside[0] || !(snaps & Snap::Left) == !(snaps & Snap::Right));
    snapInside.set(1, snapInside[1] || !(snaps & Snap::Bottom) == !(snaps & Snap::Top));

    /* Spacing in given direction is ignored either explicitly or if snapping
       inside in this direction and snapping outside in the opposite direction
       (that means also no center or fill in the opposite direction) */
    Math::BitVector<2> ignoreSpace((UnsignedByte(snaps) & UnsignedByte(Snap::NoSpaceX|Snap::NoSpaceY)) >> 6);
    ignoreSpace.set(0, ignoreSpace[0] || (snapInside[0] && !snapInside[1] && (!(snaps & Snap::Bottom) != !(snaps & Snap::Top))));
    ignoreSpace.set(1, ignoreSpace[1] || (snapInside[1] && !snapInside[0] && (!(snaps & Snap::Left) != !(snaps & Snap::Right))));

    /* Padded reference rect */
    const Range2D referenceRectPadded{
        referenceRect.min() + Math::lerp(
            Math::lerp(-referenceMargin, referencePadding.min(), snapInside),
            {},
            ignoreSpace),
        referenceRect.max() + Math::lerp(
            Math::lerp(referenceMargin, referencePadding.max(), snapInside),
            {},
            ignoreSpace)};

    /* Enlarge to reference width */
    if(snaps >= (Snap::Left|Snap::Right)) {
        size.x() = referenceRectPadded.sizeX();
        position.x() = referenceRectPadded.left();

    /* Snap to left */
    } else if(snaps & Snap::Left) {
        if(snaps & Snap::InsideX)
            position.x() = referenceRectPadded.left();
        else
            position.x() = referenceRectPadded.left() - rect.sizeX();

    /* Snap to right */
    } else if(snaps & Snap::Right) {
        if(snaps & Snap::InsideX)
            position.x() = referenceRectPadded.right() - rect.sizeX();
        else
            position.x() = referenceRectPadded.right();

    /* Snap to horizontal center */
    } else position.x() = referenceRectPadded.centerX() - rect.sizeX()/2.0f;

    /* Enlarge to reference height */
    if(snaps >= (Snap::Top|Snap::Bottom)) {
        size.y() = referenceRectPadded.sizeY();
        position.y() = referenceRectPadded.bottom();

    /* Snap to top */
    } else if(snaps & Snap::Top) {
        if(snaps & Snap::InsideY)
            position.y() = referenceRectPadded.top() - rect.sizeY();
        else
            position.y() = referenceRectPadded.top();

    /* Snap to bottom */
    } else if(snaps & Snap::Bottom) {
        if(snaps & Snap::InsideY)
            position.y() = referenceRectPadded.bottom();
        else
            position.y() = referenceRectPadded.bottom() - rect.sizeY();

    /* Snap to vertical center */
    } else position.y() = referenceRectPadded.centerY() - rect.sizeY()/2.0f;

    return Range2D::fromSize(position + rect.min(), size);
}

}

Anchor::Anchor(Snaps snaps, const Widget& widget, const Range2D& rect): _snaps{snaps}, _widget{&widget}, _rect{rect} {}

Range2D Anchor::rect(const AbstractPlane& plane) const {
    const Snaps snaps = _snaps|(_widget ? Snaps{} : Snap::InsideX|Snap::InsideY);

    return Implementation::anchorRect(snaps,
        _widget ? _widget->rect() : Range2D{{}, plane.rect().size()},
        _widget ? Range2D{Math::lerp(_widget->padding().min(), plane.padding().min(), snapInside(snaps)),
                          Math::lerp(_widget->padding().max(), plane.padding().max(), snapInside(snaps))} : plane.padding(),
        plane.margin(), _rect);
}

Range2D Anchor::rect(const AbstractUserInterface& ui) const {
    CORRADE_ASSERT(!_widget, "Ui::Anchor: can't anchor a plane to a widget", {});

    const Snaps snaps = _snaps|Snap::InsideX|Snap::InsideY;

    return Implementation::anchorRect(snaps,
        Range2D{{}, ui.size()},
        Range2D{},
        {}, _rect);
}

}}
