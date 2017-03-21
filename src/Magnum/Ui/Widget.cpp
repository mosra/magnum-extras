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

#include "Widget.h"

#include "Magnum/Ui/BasicPlane.h"
#include "Magnum/Ui/Anchor.h"

namespace Magnum { namespace Ui {

void Widget::disable(const std::initializer_list<std::reference_wrapper<Widget>> widgets) {
    for(Widget& widget: widgets) widget.disable();
}

void Widget::enable(const std::initializer_list<std::reference_wrapper<Widget>> widgets) {
    for(Widget& widget: widgets) widget.enable();
}

void Widget::setEnabled(const bool enabled, const std::initializer_list<std::reference_wrapper<Widget>> widgets) {
    enabled ? enable(widgets) : disable(widgets);
}

void Widget::hide(const std::initializer_list<std::reference_wrapper<Widget>> widgets) {
    for(Widget& widget: widgets) widget.hide();
}

void Widget::show(const std::initializer_list<std::reference_wrapper<Widget>> widgets) {
    for(Widget& widget: widgets) widget.show();
}

void Widget::setVisible(const bool visible, const std::initializer_list<std::reference_wrapper<Widget>> widgets) {
    visible ? show(widgets) : hide(widgets);
}

Widget::Widget(AbstractPlane& plane, const Anchor& anchor, const Range2D& padding): _plane(plane), _rect{anchor.rect(_plane)}, _padding{padding}, _planeIndex{plane.addWidget(*this)} {}

Widget::~Widget() {
    _plane.removeWidget(_planeIndex);
}

Widget& Widget::disable() {
    _flags |= StateFlag::Disabled;
    update();
    return *this;
}

Widget& Widget::enable() {
    _flags &= ~StateFlag::Disabled;
    update();
    return *this;
}

Widget& Widget::setEnabled(const bool enabled) {
    return enabled ? enable() : disable();
}

Widget& Widget::hide() {
    _flags |= StateFlag::Hidden;
    update();
    return *this;
}

Widget& Widget::show() {
    _flags &= ~StateFlag::Hidden;
    update();
    return *this;
}

Widget& Widget::setVisible(const bool visible) {
    return visible ? show() : hide();
}

void Widget::update() {}

bool Widget::hoverEvent() { return false; }
bool Widget::pressEvent() { return false; }
bool Widget::releaseEvent() { return false; }
bool Widget::focusEvent() { return false; }
bool Widget::blurEvent() { return false; }

}}
