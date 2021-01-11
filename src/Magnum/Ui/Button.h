#ifndef Magnum_Ui_Button_h
#define Magnum_Ui_Button_h
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021 Vladimír Vondruš <mosra@centrum.cz>

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
 * @brief Class @ref Magnum::Ui::Button
 */

#include <Corrade/Interconnect/Emitter.h>

#include "Magnum/Ui/Widget.h"
#include "Magnum/Ui/Style.h"

namespace Magnum { namespace Ui {

/**
@brief Button widget

Text and foreground. Emits @ref tapped() signal on tap.

@section Ui-Button-styling Styling

Ignores @ref WidgetFlag::Active.
@experimental
*/
class MAGNUM_UI_EXPORT Button: public Widget, public Interconnect::Emitter {
    public:
        /**
         * @brief Constructor
         * @param plane         Plane this widget is a part of
         * @param anchor        Positioning anchor
         * @param text          Button text
         * @param capacity      Button text capacity (see @ref setText())
         * @param style         Widget style
         */
        explicit Button(Plane& plane, const Anchor& anchor, const std::string& text, std::size_t capacity = 0, Style style = Style::Default): Button{plane, anchor, Containers::ArrayView<const char>{text.data(), text.size()}, capacity, style} {}

        /** @overload */
        explicit Button(Plane& plane, const Anchor& anchor, const std::string& text, Style style): Button{plane, anchor, Containers::ArrayView<const char>{text.data(), text.size()}, 0, style} {}

        /** @overload */
        template<std::size_t size> explicit Button(Plane& plane, const Anchor& anchor, const char(&text)[size], std::size_t capacity = 0, Style style = Style::Default): Button{plane, anchor, Containers::ArrayView<const char>{text, size - 1}, capacity, style} {}

        /** @overload */
        template<std::size_t size> explicit Button(Plane& plane, const Anchor& anchor, const char(&text)[size], Style style): Button{plane, anchor, Containers::ArrayView<const char>{text, size - 1}, 0, style} {}

        /** @overload */
        explicit Button(Plane& plane, const Anchor& anchor, Containers::ArrayView<const char> text, std::size_t capacity = 0, Style style = Style::Default);

        /** @overload */
        explicit Button(Plane& plane, const Anchor& anchor, Containers::ArrayView<const char> text, Style style): Button{plane, anchor, text, 0, style} {}

        ~Button();

        /**
         * @brief Set widget style
         * @return Reference to self (for method chaining)
         */
        Button& setStyle(Style style);

        /**
         * @brief Set text
         * @return Reference to self (for method chaining)
         *
         * The text is expected to not exceed the capacity defined in the
         * constructor.
         */
        Button& setText(const std::string& text) {
            return setText(Containers::ArrayView<const char>{text.data(), text.size()});
        }

        /** @overload */
        Button& setText(Containers::ArrayView<const char> text);

        /** @overload */
        template<std::size_t size> Button& setText(const char(&text)[size]) {
            return setText(Containers::ArrayView<const char>{text, size - 1});
        }

        /** @brief The button was tapped */
        Signal tapped();

    private:
        void MAGNUM_UI_LOCAL update() override;

        bool MAGNUM_UI_LOCAL hoverEvent() override;
        bool MAGNUM_UI_LOCAL pressEvent() override;
        bool MAGNUM_UI_LOCAL releaseEvent() override;
        bool MAGNUM_UI_LOCAL focusEvent() override;

        Style _style;
        std::size_t _foregroundElementId,
            _textElementId;
};

}}

#endif
