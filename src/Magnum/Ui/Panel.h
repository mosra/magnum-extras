#ifndef Magnum_Ui_Panel_h
#define Magnum_Ui_Panel_h
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023, 2024, 2025, 2026
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
 * @brief Class @ref Magnum::Ui::Panel, function @ref Magnum::Ui::panel(), enum @ref Magnum::Ui::PanelStyle
 * @m_since_latest_{extras}
 */

#include "Magnum/Ui/Widget.h"

namespace Magnum { namespace Ui {

/**
@brief Panel style
@m_since_latest_{extras}

@see @ref Panel, @ref panel()
*/
enum class PanelStyle: UnsignedByte {
    Default,    /** Default with no background */
    Filled,     /** With a filled background */
};

/**
@debugoperatorenum{PanelStyle}
@m_since_latest_{extras}
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, PanelStyle value);

/**
@brief Panel widget
@m_since_latest_{extras}
*/
class MAGNUM_UI_EXPORT Panel: public Widget {
    public:
        /**
         * @brief Constructor
         * @param anchor            Positioning anchor
         * @param style             Panel style
         *
         * @see @ref panel(Anchor, PanelStyle)
         */
        explicit Panel(Anchor anchor, PanelStyle style = PanelStyle::Default);

        /**
         * @brief Construct with no underlying node
         *
         * The instance is equivalent to a moved-out state, i.e. not usable
         * for anything. Move another instance over it to make it useful.
         */
        explicit Panel(NoCreateT, UserInterface& ui): Widget{NoCreate, ui}, _style{}, _backgroundData{} {}

        /** @brief Style */
        PanelStyle style() const { return _style; }

        /**
         * @brief Set style
         * @return Reference to self (for method chaining)
         */
        Panel& setStyle(PanelStyle style);

        /**
         * @brief Background data or @ref DataHandle::Null
         *
         * Exposed mainly for testing purposes, not meant to be modified
         * directly.
         */
        DataHandle backgroundData() const;

        #ifndef DOXYGEN_GENERATING_OUTPUT
        _MAGNUM_UI_WIDGET_SUBCLASS_IMPLEMENTATION(Panel) /* LCOV_EXCL_LINE */
        #endif

    private:
        PanelStyle _style;
        /* 3 bytes free */
        LayerDataHandle _backgroundData;
};

/**
@brief Stateless panel widget
@param anchor           Positioning anchor
@param style            Panel style
@return The @p anchor verbatim
@m_since_latest_{extras}

Compared to @ref Panel::Panel(Anchor, PanelStyle) this creates a
stateless panel that doesn't have any class instance that would need to be kept
in scope and eventually destructed, making it more lightweight. As a
consequence it can't have its style subsequently changed and is removed only
when the node or its parent get removed.
*/
MAGNUM_UI_EXPORT Anchor panel(Anchor anchor, PanelStyle style = PanelStyle::Default);

}}

#endif
