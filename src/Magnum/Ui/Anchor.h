#ifndef Magnum_Ui_Anchor_h
#define Magnum_Ui_Anchor_h
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019
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

/** @file
 * @brief Class @ref Magnum::Ui::Anchor, enum @ref Magnum::Ui::Snap, enum set @ref Magnum::Ui::Snaps
 */

#include <Corrade/Containers/EnumSet.h>
#include <Magnum/Magnum.h>
#include <Magnum/Math/Range.h>

#include "Magnum/Ui/Ui.h"
#include "Magnum/Ui/visibility.h"

namespace Magnum { namespace Ui {

/**
@brief Snap

See particular values for detailed documentation.

Specifying neither @ref Snap::Left nor @ref Snap::Right will result in
horizontal centering. Specifying both @ref Snap::Left and @ref Snap::Right will
cause the widget width to match width of reference widget, plane or user
interface. In case @ref Snap::InsideY is specified, horizontal
@ref BasicPlane::padding() or left/right-side @ref Widget::padding() is taken
into account.

Specifying neither @ref Snap::Bottom nor @ref Snap::Top will result in
verical centering. Specifying both @ref Snap::Bottom and @ref Snap::Top will
cause the widget height to match height of reference widget, plane or user
interface. In case @ref Snap::InsideX is specified, horizontal
@ref BasicPlane::padding() or bottom/top-side @ref Widget::padding() is taken
into account.

Specifying @ref Snap::NoSpaceX and/or @ref Snap::NoSpaceY will ignore
horizontal and/or vertical @ref BasicPlane::padding(), @ref Widget::padding()
and @ref BasicPlane::margin().
@see @ref Snaps, @ref Anchor
@experimental
*/
enum class Snap: UnsignedByte {
    /* 8 bits: nospaceX nospaceY insideX insideY bottom top left right */

    /**
     * If anchoring to a widget, snaps right side of a widget to left side of
     * the reference widget, taking horizontal @ref BasicPlane::margin() into
     * account.
     *
     * If combined with @ref Snap::InsideX or anchoring to a plane or user
     * interface, snaps left side of a widget to left side of reference widget,
     * plane or user interface, taking left-side @ref Widget::padding() or
     * horizontal @ref BasicPlane::padding() into account.
     */
    Left = 1 << 0,

    /**
     * If anchoring to a widget, snaps left side of a widget to right side of
     * the reference widget, taking horizontal @ref BasicPlane::margin() into
     * account.
     *
     * If combined with @ref Snap::InsideX or anchoring to a plane or user
     * interface, snaps left side of a widget to left side of reference widget,
     * plane or user interface, taking right-side @ref Widget::padding() or
     * horizontal @ref BasicPlane::padding() into account.
     */
    Right = 1 << 1,

    /**
     * If anchoring to a widget, snaps top side of a widget to bottom side of
     * the reference widget, taking vertical @ref BasicPlane::margin() into
     * account.
     *
     * If combined with @ref Snap::InsideY or anchoring to a plane or user
     * interface, snaps bottom side of a widget to bottom side of reference
     * widget, plane or user interface, taking bottom-side @ref Widget::padding()
     * or vertical @ref BasicPlane::padding() into account.
     */
    Bottom = 1 << 2,

    /**
     * If anchoring to a widget, snaps bottom side of a widget to top side of
     * the reference widget, taking vertical @ref BasicPlane::margin() into
     * account.
     *
     * If combined with @ref Snap::InsideY or anchoring to a plane or user
     * interface, snaps top side of a widget to top side of reference widget,
     * plane or user interface, taking top-side @ref Widget::padding() or
     * vertical @ref BasicPlane::padding() into account.
     */
    Top = 1 << 3,

    /**
     * When combined with @ref Snap::Left or @ref Snap::Right, snaps
     * horizontally inside reference widget instead of outside. Implicit when
     * anchoring to a plane or user interface.
     */
    InsideX = 1 << 4,

    /**
     * When combined with @ref Snap::Bottom or @ref Snap::Top, snaps vertically
     * inside reference widget instead of outside. Implicit when anchoring to a
     * plane or user interface.
     */
    InsideY = 1 << 5,

    /**
     * Ignore horizontal @ref BasicPlane::padding(), @ref Widget::padding() and
     * @ref BasicPlane::margin().
     */
    NoSpaceX = 1 << 6,

    /**
     * Ignore vertical @ref BasicPlane::padding(), @ref Widget::padding() and
     * @ref BasicPlane::margin().
     */
    NoSpaceY = 1 << 7
};

/**
@brief Set of snaps

@see @ref Anchor
@experimental
*/
typedef Containers::EnumSet<Snap> Snaps;

CORRADE_ENUMSET_OPERATORS(Snaps)

namespace Implementation {
    MAGNUM_UI_EXPORT Range2D anchorRect(Snaps snaps, const Range2D& referenceRect, const Range2D& referencePadding, const Vector2& referenceMargin, const Range2D& rect);
}

/**
@brief Anchor

Specifies widget position relative to the plane or another widget.
@experimental
*/
class MAGNUM_UI_EXPORT Anchor {
    public:
        /**
         * @brief Anchor relative to plane or user interface
         *
         * @ref Snap::InsideX and @ref Snap::InsideY is used implicitly.
         */
        /*implicit*/ Anchor(Snaps snaps, const Range2D& rect): _snaps{snaps}, _rect{rect} {}

        /** @overload */
        /*implicit*/ Anchor(Snaps snaps, const Vector2& size = {}): Anchor{snaps, {{}, size}} {}

        /**
         * @overload
         *
         * Equivalent to `Anchor{{}}`.
         */
        /*implicit*/ Anchor(): Anchor{{}} {}

        /** @brief Anchor relative to another widget */
        /*implicit*/ Anchor(Snaps snaps, const Widget& widget, const Range2D& rect);

        /** @overload */
        /*implicit*/ Anchor(Snaps snaps, const Widget& widget, const Vector2& size = {}): Anchor{snaps, widget, {{}, size}} {}

        /**
         * @brief Calculate final anchor rectangle relative to a plane
         *
         * Takes plane size, margin and padding into account.
         */
        Range2D rect(const AbstractPlane& plane) const;

        /**
         * @brief Calculate final anchor rectangle relative to an user interface
         *
         * Takes user interface size into account.
         */
        Range2D rect(const AbstractUserInterface& ui) const;

    private:
        Snaps _snaps;
        const Widget* _widget = nullptr;
        Range2D _rect;
};

}}

#endif
