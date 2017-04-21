/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017
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

#include "Label.h"

#include <Magnum/Text/AbstractFont.h>
#include <Magnum/Text/Alignment.h>

#include "Magnum/Ui/Anchor.h"
#include "Magnum/Ui/Plane.h"
#include "Magnum/Ui/UserInterface.h"
#include "Magnum/Ui/Implementation/TextUtility.h"

namespace Magnum { namespace Ui {

Label::Label(Plane& plane, const Anchor& anchor, const Containers::ArrayView<const char> text, const Text::Alignment alignment, const std::size_t capacity, const Style style): Widget{plane, anchor}, _alignment{alignment}, _style{style} {
    /** @todo don't use implementation details */
    if((UnsignedByte(alignment) & Text::Implementation::AlignmentHorizontal) == Text::Implementation::AlignmentLeft)
        _cursor.x() = rect().left();
    else if((UnsignedByte(alignment) & Text::Implementation::AlignmentHorizontal) == Text::Implementation::AlignmentRight)
        _cursor.x() = rect().right();
    else _cursor.x() = rect().centerX();

    if((UnsignedByte(alignment) & Text::Implementation::AlignmentVertical) == Text::Implementation::AlignmentTop)
        _cursor.y() = rect().top();
    else if((UnsignedByte(alignment) & Text::Implementation::AlignmentVertical) == Text::Implementation::AlignmentLine)
        _cursor.y() = rect().centerY() + Int(Implementation::lineAlignmentAdjustment(plane.ui()));
    else _cursor.y() = rect().centerY();

    _textElementId = plane.addText(
        Implementation::textColorIndex(Type::Label, style, flags() & ~(WidgetFlag::Active|WidgetFlag::Hovered|WidgetFlag::Pressed)),
        plane.ui().styleConfiguration().fontSize(),
        text,
        _cursor, alignment, capacity);
}

Label::~Label() = default;

Label& Label::setStyle(const Style style) {
    _style = style;
    update();
    return *this;
}

Label& Label::setText(const Containers::ArrayView<const char> text) {
    auto& plane = static_cast<Plane&>(this->plane());

    plane.setText(_textElementId,
        Implementation::textColorIndex(Type::Label, _style, flags() & ~(WidgetFlag::Active|WidgetFlag::Hovered|WidgetFlag::Pressed)),
        plane.ui().styleConfiguration().fontSize(),
        text,
        _cursor, _alignment);
    return *this;
}

void Label::update() {
    for(Implementation::TextVertex& v: static_cast<Plane&>(plane())._textLayer.modifyElement(_textElementId))
        v.colorIndex = Implementation::textColorIndex(Type::Label, _style, flags() & ~(WidgetFlag::Active|WidgetFlag::Hovered|WidgetFlag::Pressed));
}

}}
