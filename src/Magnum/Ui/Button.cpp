/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016
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

#include "Button.h"

#include <Magnum/Text/AbstractFont.h>
#include <Magnum/Text/Alignment.h>

#include "Magnum/Ui/Anchor.h"
#include "Magnum/Ui/Plane.h"
#include "Magnum/Ui/UserInterface.h"
#include "Magnum/Ui/Implementation/TextUtility.h"

namespace Magnum { namespace Ui {

Button::Button(Plane& plane, const Anchor& anchor, const Containers::ArrayView<const char> text, const std::size_t capacity, const Style style): Widget{plane, anchor}, _style{style} {
    _foregroundElementId = plane._foregroundLayer.addElement({rect(),
        Implementation::foregroundColorIndex(Type::Button, style,
        style == Style::Flat ? StateFlag::Hidden : flags() & ~StateFlag::Active)});

    _textElementId = plane.addText(
        Implementation::textColorIndex(Type::Button, style, flags() & ~StateFlag::Active),
        plane.ui().styleConfiguration().fontSize(),
        text,
        rect().center() + Vector2::yAxis(Int(Implementation::lineAlignmentAdjustment(plane.ui()))),
        Text::Alignment::LineCenterIntegral,
        capacity);
}

Button::~Button() = default;

void Button::setStyle(Style style) {
    _style = style;
    update();
}

Button& Button::setText(const Containers::ArrayView<const char> text) {
    auto& plane = static_cast<Plane&>(this->plane());

    plane.setText(_textElementId,
        Implementation::textColorIndex(Type::Button, _style, flags() & ~(StateFlag::Active|StateFlag::Hovered|StateFlag::Pressed)),
        plane.ui().styleConfiguration().fontSize(),
        text,
        rect().center() + Vector2::yAxis(Int(Implementation::lineAlignmentAdjustment(plane.ui()))),
        Text::Alignment::LineCenterIntegral);
    return *this;
}

void Button::update() {
    auto& plane = static_cast<Plane&>(this->plane());

    plane._foregroundLayer.modifyElement(_foregroundElementId).colorIndex =
        Implementation::foregroundColorIndex(Type::Button, _style,
        _style == Style::Flat ? StateFlag::Hidden : flags() & ~StateFlag::Active);

    for(Implementation::TextVertex& v: plane._textLayer.modifyElement(_textElementId))
        v.colorIndex = Implementation::textColorIndex(Type::Button, _style, flags() & ~StateFlag::Active);
}

bool Button::hoverEvent() {
    update();
    return true;
}

bool Button::pressEvent() {
    update();
    return true;
}

bool Button::releaseEvent() {
    update();
    return true;
}

bool Button::focusEvent() {
    tapped();
    return true;
}

}}
