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

#include "BasicPlane.h"

#include "Magnum/Ui/Widget.h"
#include "Magnum/Ui/BasicUserInterface.h"
#include "Anchor.h"

namespace Magnum { namespace Ui {

AbstractPlane::AbstractPlane(AbstractUserInterface& ui, const Anchor& anchor, const Range2D& padding, const Vector2& margin): _ui(ui), _rect{anchor.rect(ui)}, _padding{padding}, _margin{margin} {
    if((_lastActivePlane = ui.addPlane(*this)))
        _flags |= Flag::Hidden;
}

AbstractPlane::~AbstractPlane() = default;

void AbstractPlane::activate() {
    _lastActivePlane = _ui._activePlane;
    _ui._activePlane = this;
    _flags &= ~Flag::Hidden;
}

void AbstractPlane::hide() {
    if(_flags & Flag::Hidden) return;

    CORRADE_ASSERT(_ui._activePlane == this, "Ui::AbstractPlane::hide(): can't hide plane that is not currently active", );

    _ui._activePlane = _lastActivePlane;
    _flags |= Flag::Hidden;
}

std::size_t AbstractPlane::addWidget(Widget& widget) {
    _widgets.emplace_back(widget.rect(), &widget);
    return _widgets.size() - 1;
}

void AbstractPlane::removeWidget(const std::size_t index) {
    CORRADE_INTERNAL_ASSERT(index < _widgets.size());
    _widgets[index].widget = nullptr;
}

Widget* AbstractPlane::handleEvent(const Vector2& position) {
    Widget* currentHoveredWidget = nullptr;

    /* Cursor stayed on the same widget */
    if(_lastHoveredWidget && _lastHoveredWidget->_rect.contains(position) && !(_lastHoveredWidget->_flags & StateFlag::Hidden))
        currentHoveredWidget = _lastHoveredWidget;

    /* Find new active widget if the cursor moved away */
    else for(auto it = _widgets.rbegin(); it != _widgets.rend(); ++it) {
        WidgetReference& widgetReference = *it;
        if(!widgetReference.widget || !widgetReference.rect.contains(position) || widgetReference.widget->_flags & StateFlag::Hidden)
            continue;

        currentHoveredWidget = widgetReference.widget;
        break;
    }

    /* Save cursor position for the next time */
    _lastCursorPosition = position;

    /* Return no widget in case the current one is disabled */
    return currentHoveredWidget && currentHoveredWidget->_flags & StateFlag::Disabled ?
        nullptr : currentHoveredWidget;
}

bool AbstractPlane::handleMoveEvent(const Vector2& position) {
    Widget* const currentHoveredWidget = handleEvent(position);

    bool accepted = false;

    /* If moved across widgets, emit hover out event for the previous one */
    if(_lastHoveredWidget && _lastHoveredWidget != currentHoveredWidget && (_lastHoveredWidget->_flags & StateFlag::Hovered)) {
        _lastHoveredWidget->_flags &= ~StateFlag::Hovered;
        _lastHoveredWidget->hoverEvent();
        accepted = true;
    }

    if(currentHoveredWidget) {
        /* Mark the widget as hovered and call hover event on it */
        currentHoveredWidget->_flags |= StateFlag::Hovered;
        accepted = currentHoveredWidget->hoverEvent();
    }

    /* Save the current widget for next time. Because we are just moving, the
       active widget doesn't change -- it changes only on press/release */
    _lastHoveredWidget = currentHoveredWidget;

    return accepted;
}

bool AbstractPlane::handlePressEvent(const Vector2& position) {
    Widget* const currentHoveredWidget = handleEvent(position);

    bool accepted = false;

    /* Pressed outside the previous widget, call blur event on it */
    if(_lastActiveWidget && _lastActiveWidget != currentHoveredWidget && (_lastActiveWidget->_flags & StateFlag::Active)) {
        _lastActiveWidget->_flags &= ~StateFlag::Active;
        _lastActiveWidget->blurEvent();
        accepted = true;
    }

    if(currentHoveredWidget) {
        /* Mark the widget as active and call press event on it */
        currentHoveredWidget->_flags |= StateFlag::Pressed;
        accepted = currentHoveredWidget->pressEvent();
    }

    /* Save the current widget for next time */
    _lastHoveredWidget = _lastActiveWidget = currentHoveredWidget;

    return accepted;
}

bool AbstractPlane::handleReleaseEvent(const Vector2& position) {
    Widget* const currentHoveredWidget = handleEvent(position);

    bool accepted = false;

    /* If moved across widgets during the mouse down, emit release event also
       for the previous one, but remove the active mark before doing so */
    if(_lastActiveWidget && _lastActiveWidget != currentHoveredWidget && (_lastActiveWidget->_flags & StateFlag::Pressed)) {
        _lastActiveWidget->_flags &= ~StateFlag::Pressed;
        _lastActiveWidget->releaseEvent();
        accepted = true;
    }

    if(currentHoveredWidget) {
        /* If the widget was pressed previously, it is active now. Remove the
           pressed flag, add the active flag and call the focus event. */
        if(currentHoveredWidget->_flags & StateFlag::Pressed) {
            currentHoveredWidget->_flags &= ~StateFlag::Pressed;
            currentHoveredWidget->_flags |= StateFlag::Active;
            accepted = currentHoveredWidget->focusEvent();
        }

        /* Call the release event in any case (even in case we might come from
           another widget) */
        accepted = currentHoveredWidget->releaseEvent();
    }

    /* Save the current widget for next time */
    _lastHoveredWidget = _lastActiveWidget = currentHoveredWidget;

    return accepted;
}

}}
