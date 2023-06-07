#ifndef Magnum_Ui_Modal_h
#define Magnum_Ui_Modal_h
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023 Vladimír Vondruš <mosra@centrum.cz>

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
 * @brief Class @ref Magnum::Ui::Modal
 */

#include <Magnum/Text/Text.h>

#include "Magnum/Ui/Widget.h"
#include "Magnum/Ui/Style.h"

namespace Magnum { namespace Ui {

/**
@brief Modal widget

Should be snapped to whole area of a plane. Gives the plane a background and
fills the rest with semi-transparent dim color to suggest modality.

@section Ui-Modal-styling Styling

Ignores @ref WidgetFlag::Hovered, @ref WidgetFlag::Pressed and
@ref WidgetFlag::Active, @ref Style::Flat.
@experimental
*/
class MAGNUM_UI_EXPORT Modal: public Widget {
    public:
        /**
         * @brief Label
         * @param plane         Plane this widget is a part of
         * @param anchor        Positioning anchor
         * @param style         Widget style
         */
        explicit Modal(Plane& plane, const Anchor& anchor, Style style = Style::Default);

        ~Modal();

        /**
         * @brief Set widget style
         * @return Reference to self (for method chaining)
         */
        Modal& setStyle(Style style);

    private:
        void MAGNUM_UI_LOCAL update() override;

        std::size_t _dimElementId, _backgroundElementId;
        Style _style;
};

}}

#endif
