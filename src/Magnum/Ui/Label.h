#ifndef Magnum_Ui_Label_h
#define Magnum_Ui_Label_h
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

/** @file
 * @brief Class @ref Magnum::Ui::Label
 */

#include <Magnum/Text/Text.h>

#include "Magnum/Ui/Widget.h"
#include "Magnum/Ui/Style.h"

namespace Magnum { namespace Ui {

/**
@brief Label widget

Just a text with no interactivity.

@section Ui-Label-styling Styling

Ignores @ref WidgetFlag::Hovered, @ref WidgetFlag::Pressed and
@ref WidgetFlag::Active, @ref Style::Flat.
@experimental
*/
class MAGNUM_UI_EXPORT Label: public Widget {
    public:
        /**
         * @brief Label
         * @param plane         Plane this widget is a part of
         * @param anchor        Positioning anchor
         * @param text          Label text
         * @param alignment     Label text alignment
         * @param capacity      Label text capacity (see @ref setText())
         * @param style         Widget style
         */
        explicit Label(Plane& plane, const Anchor& anchor, const std::string& text, Text::Alignment alignment, std::size_t capacity = 0, Style style = Style::Default): Label{plane, anchor, Containers::ArrayView<const char>{text.data(), text.size()}, alignment, capacity, style} {}

        /** @overload */
        explicit Label(Plane& plane, const Anchor& anchor, const std::string& text, Text::Alignment alignment, Style style): Label{plane, anchor, Containers::ArrayView<const char>{text.data(), text.size()}, alignment, 0, style} {}

        /** @overload */
        template<std::size_t size> explicit Label(Plane& plane, const Anchor& anchor, const char(&text)[size], Text::Alignment alignment, std::size_t capacity = 0, Style style = Style::Default): Label{plane, anchor, Containers::ArrayView<const char>{text, size - 1}, alignment, capacity, style} {}

        /** @overload */
        template<std::size_t size> explicit Label(Plane& plane, const Anchor& anchor, const char(&text)[size], Text::Alignment alignment, Style style): Label{plane, anchor, Containers::ArrayView<const char>{text, size - 1}, alignment, 0, style} {}

        /** @overload */
        explicit Label(Plane& plane, const Anchor& anchor, Containers::ArrayView<const char> text, Text::Alignment alignment, std::size_t capacity = 0, Style style = Style::Default);

        /** @overload */
        explicit Label(Plane& plane, const Anchor& anchor, Containers::ArrayView<const char> text, Text::Alignment alignment, Style style = Style::Default): Label{plane, anchor, text, alignment, 0, style} {}

        ~Label();

        /**
         * @brief Set widget style
         * @return Reference to self (for method chaining)
         */
        Label& setStyle(Style style);

        /**
         * @brief Set text
         * @return Reference to self (for method chaining)
         *
         * The text is expected to not exceed the capacity defined in the
         * constructor.
         */
        Label& setText(const std::string& text) {
            return setText(Containers::ArrayView<const char>{text.data(), text.size()});
        }

        /** @overload */
        Label& setText(Containers::ArrayView<const char> text);

        /** @overload */
        template<std::size_t size> Label& setText(const char(&text)[size]) {
            return setText(Containers::ArrayView<const char>{text, size - 1});
        }

    private:
        void MAGNUM_UI_LOCAL update() override;

        Text::Alignment _alignment;
        Style _style;
        Vector2 _cursor;
        std::size_t _textElementId;
};

}}

#endif
