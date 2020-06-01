/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020 Vladimír Vondruš <mosra@centrum.cz>

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

#include "Input.h"

#include <Magnum/Text/AbstractFont.h>
#include <Magnum/Text/Alignment.h>

#include "Magnum/Ui/Anchor.h"
#include "Magnum/Ui/Plane.h"
#include "Magnum/Ui/UserInterface.h"
#include "Magnum/Ui/Implementation/TextUtility.h"

namespace Magnum { namespace Ui {

Input::Input(Plane& plane, const Anchor& anchor, std::string value, const std::size_t maxValueSize, const Style style): Widget{plane, anchor}, _value{std::move(value)}, _maxValueSize{maxValueSize}, _cursor{_value.size()}, _style{style} {
    _foregroundElementId = plane._foregroundLayer.addElement({rect(),
        Implementation::foregroundColorIndex(Type::Input, style,
        style == Style::Flat ? WidgetFlag::Hidden : flags())});

    _textElementId = plane.addText(
        Implementation::textColorIndex(Type::Input, style, flags()),
        plane.ui().styleConfiguration().fontSize(),
        {_value.data(), _value.size()},
        {rect().left() + plane.ui().styleConfiguration().padding().x(), rect().centerY() + Int(Implementation::lineAlignmentAdjustment(plane.ui()))},
        Text::Alignment::LineLeft,
        maxValueSize);
}

Input::Input(Plane& plane, const Anchor& anchor, const std::size_t maxValueSize, const Style style): Input{plane, anchor, {}, maxValueSize, style} {}

Input::~Input() = default;

Input& Input::setStyle(Style style) {
    _style = style;
    update();
    return *this;
}

Input& Input::setValue(const std::string& value) {
    _value = value;
    _cursor = _value.size();
    updateValue();
    return *this;
}

Input& Input::setValue(std::string&& value) {
    _value = std::move(value);
    _cursor = _value.size();
    updateValue();
    return *this;
}

Input& Input::setValue(const Containers::ArrayView<const char> value) {
    _value.assign(value.begin(), value.end());
    _cursor = _value.size();
    updateValue();
    return *this;
}

auto Input::focused() -> Signal {
    /* See https://github.com/mosra/corrade/issues/72 for details */
    return emit(&Input::focused);
}

auto Input::blurred() -> Signal {
    /* See https://github.com/mosra/corrade/issues/72 for details */
    return emit(&Input::blurred);
}

auto Input::valueChanged(const std::string& value) -> Signal {
    /* See https://github.com/mosra/corrade/issues/72 for details */
    return emit(&Input::valueChanged, value);
}

void Input::update() {
    auto& plane = static_cast<Plane&>(this->plane());

    plane._foregroundLayer.modifyElement(_foregroundElementId).colorIndex =
        Implementation::foregroundColorIndex(Type::Input, _style,
        _style == Style::Flat ? WidgetFlag::Hidden : flags());

    for(Implementation::TextVertex& v: plane._textLayer.modifyElement(_textElementId))
        v.colorIndex = Implementation::textColorIndex(Type::Input, _style, flags());
}

void Input::updateValue() {
    auto& plane = static_cast<Plane&>(this->plane());

    plane.setText(
        _textElementId,
        Implementation::textColorIndex(Type::Input, _style, flags()),
        plane.ui().styleConfiguration().fontSize(),
        {_value.data(), _value.size()},
        {rect().left() + plane.ui().styleConfiguration().padding().x(), rect().centerY() + Int(Implementation::lineAlignmentAdjustment(plane.ui()))},
        Text::Alignment::LineLeft);

    valueChanged(_value);
}

bool Input::hoverEvent() {
    update();
    return true;
}

bool Input::pressEvent() {
    update();
    return true;
}

bool Input::releaseEvent() {
    update();
    return true;
}

bool Input::focusEvent() {
    UserInterface& ui = static_cast<Plane&>(plane()).ui();
    CORRADE_INTERNAL_ASSERT(!ui._focusedInputWidget || ui._focusedInputWidget == this);
    ui._focusedInputWidget = this;

    update();
    focused();
    ui.inputWidgetFocused();
    return true;
}

bool Input::blurEvent() {
    UserInterface& ui = static_cast<Plane&>(plane()).ui();
    CORRADE_INTERNAL_ASSERT(ui._focusedInputWidget == this);
    ui._focusedInputWidget = nullptr;

    update();
    blurred();
    ui.inputWidgetBlurred();
    return true;
}

}}
