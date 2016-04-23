#ifndef Magnum_Ui_Input_h
#define Magnum_Ui_Input_h
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

/** @file
 * @brief Class @ref Magnum::Ui::Input
 */

#include <Corrade/Interconnect/Emitter.h>
#include <Corrade/Utility/Unicode.h>
#include <Magnum/Text/Text.h>

#include "Magnum/Ui/Widget.h"
#include "Magnum/Ui/Style.h"

namespace Magnum { namespace Ui {

/**
@brief Input widget

Can attach to application-specific keyboard and text input.
*/
class MAGNUM_UI_EXPORT Input: public Widget, public Interconnect::Emitter {
    public:
        /**
         * @brief Constructor
         * @param plane         Plane this widget is a part of
         * @param anchor        Positioning anchor
         * @param value         Initial input value
         * @param maxValueSize  Max input text size
         * @param style         Widget style
         */
        explicit Input(Plane& plane, const Anchor& anchor, std::string value, std::size_t maxValueSize, Style style = Style::Default);

        /** @overload */
        explicit Input(Plane& plane, const Anchor& anchor, std::size_t maxValueSize, Style style = Style::Default);

        ~Input();

        /**
         * @brief Set widget style
         * @return Reference to self (for method chaining)
         */
        Input& setStyle(Style style);

        /** @brief Max value size */
        std::size_t maxValueSize() const { return _maxValueSize; }

        /** @brief Value */
        const std::string& value() const { return _value; }

        /**
         * @brief Set value
         * @return Reference to self (for method chaining)
         *
         * The value is expected to not have more glyphs than the capacity set
         * in constructor.
         */
        Input& setValue(const std::string& value);
        Input& setValue(std::string&& value); /**< @overload */
        Input& setValue(Containers::ArrayView<const char> value); /**< @overload */

        /**
         * @brief Handle key press from an application
         *
         * Handles keyboard input like cursor movement or backspace/delete
         * keys. Should be called from within
         * @ref Platform::Sdl2Application::keyPressEvent() "Platform::*Application::keyPressEvent()".
         * Returns `true` if the event was accepted (and thus the input value
         * changed), `false` otherwise.
         * @see @ref UserInterface::focusedInputWidget()
         */
        template<class KeyEvent> bool handleKeyPress(KeyEvent& keyEvent);

        /**
         * @brief Handle text input from an application
         *
         * Handles UTF-8 text input. Input that makes the value exceed
         * @ref maxValueSize is ignored. Should be called from within
         * @ref Platform::Sdl2Application::textInputEvent() "Platform::*Application::textInputEvent()".
         * Returns `true` if the event was accepted (and thus the input value
         * changed), `false` otherwise.
         * @see @ref UserInterface::focusedInputWidget()
         */
        template<class TextInputEvent> bool handleTextInput(TextInputEvent& textInputEvent);

        /**
         * @brief The widget was focused
         *
         * Text input from the application should be started upon signalling
         * this and passed to @ref handleKeyPress() and @ref handleTextInput().
         * @see @ref UserInterface::inputWidgetFocused(),
         *      @ref Platform::Sdl2Application::startTextInput() "Platform::*Application::startTextInput()"
         */
        Signal focused() {
            return emit(&Input::focused);
        }

        /**
         * @brief The widget was blurred
         *
         * Text input from the application should be stopped upon signalling
         * this.
         * @see @ref UserInterface::inputWidgetBlurred(),
         *      @ref Platform::Sdl2Application::stopTextInput() "Platform::*Application::stopTextInput()"
         */
        Signal blurred() {
            return emit(&Input::blurred);
        }

        /**
         * @brief The input value changed
         *
         * Useful for attaching input validators.
         */
        Signal valueChanged(const std::string& value) {
            return emit(&Input::valueChanged, value);
        }

    private:
        void MAGNUM_UI_LOCAL update() override;
        void updateValue();

        bool MAGNUM_UI_LOCAL hoverEvent() override;
        bool MAGNUM_UI_LOCAL pressEvent() override;
        bool MAGNUM_UI_LOCAL releaseEvent() override;
        bool MAGNUM_UI_LOCAL focusEvent() override;
        bool MAGNUM_UI_LOCAL blurEvent() override;

        std::string _value;
        std::size_t _maxValueSize, _cursor;
        Style _style;
        std::size_t _foregroundElementId,
            _textElementId;
};

template<class KeyEvent> bool Input::handleKeyPress(KeyEvent& event) {
    /* Cursor left */
    if(event.key() == KeyEvent::Key::Left && _cursor > 0) {
        std::tie(std::ignore, _cursor) = Utility::Unicode::prevChar(_value, _cursor);

    /* Backspace */
    } else if(event.key() == KeyEvent::Key::Backspace && _cursor > 0) {
        const std::size_t prevCursor = _cursor;
        std::tie(std::ignore, _cursor) = Utility::Unicode::prevChar(_value, _cursor);
        _value.erase(_cursor, prevCursor - _cursor);

    /* Cursor right */
    } else if(event.key() == KeyEvent::Key::Right && _cursor < _value.size()) {
        std::tie(std::ignore, _cursor) = Utility::Unicode::nextChar(_value, _cursor);

    /* Delete */
    } else if(event.key() == KeyEvent::Key::Delete && _cursor < _value.size()) {
        std::size_t nextCursor;
        std::tie(std::ignore, nextCursor) = Utility::Unicode::nextChar(_value, _cursor);
        _value.erase(_cursor, nextCursor - _cursor);

    /* Everything else didn't contribute to the state */
    } else return false;

    /** @todo Insert */
    /** @todo Home/end */
    /** @todo Enter to finish editing, hide the keyboard, signal "finished" (action upon that) and blur the element */

    updateValue();
    event.setAccepted();
    return true;
}

template<class TextInputEvent> bool Input::handleTextInput(TextInputEvent& event) {
    /* Too long, ignore */
    if(event.text().size() + _value.size() > _maxValueSize)
        return false;

    _value.insert(_cursor, event.text().data(), event.text().size());
    _cursor += event.text().size();
    updateValue();
    event.setAccepted();
    return true;
}

}}

#endif
