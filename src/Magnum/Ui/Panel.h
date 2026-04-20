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
 * @brief Class @ref Magnum::Ui::Panel, enum @ref Magnum::Ui::PanelStyle
 * @m_since_latest_{extras}
 */

#include "Magnum/Ui/Widget.h"

namespace Magnum { namespace Ui {

/**
@brief Panel style
@m_since_latest_{extras}

@see @ref Panel
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
         */
        explicit Panel(Anchor anchor, PanelStyle style = PanelStyle::Default);

        /**
         * @brief Construct a non-owned panel
         *
         * Like @ref Panel(Anchor, PanelStyle) but the widget node doesn't get
         * removed on destruction. Instead, it gets removed either once any
         * parent node is removed, or when
         * @ref AbstractUserInterface::removeNode() is explicitly called on
         * @ref node().
         * @see @ref isOwned()
         */
        explicit Panel(NonOwnedT, Anchor anchor, PanelStyle style = PanelStyle::Default);

        /** @copydoc AbstractWidget::AbstractWidget(NoCreateT) */
        explicit Panel(NoCreateT): Widget{NoCreate}, _style{}, _backgroundData{} {}

        /** @brief Style */
        PanelStyle style() const { return _style; }

        /**
         * @brief Set style
         * @return Reference to self (for method chaining)
         */
        Panel& setStyle(PanelStyle style);

        /**
         * @brief Set style at given time
         * @return Reference to self (for method chaining)
         *
         * Compared to @ref setStyle(PanelStyle) may animate the style
         * transition if the theme defines an animation for it. The
         * @ref style() getter is however updated immediately always.
         */
        Panel& setStyle(PanelStyle style, Nanoseconds time);

        /**
         * @brief Contents
         *
         * Use the returned anchor to put widgets inside the panel.
         */
        Anchor contents() const;

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
        template<class ...Args> MAGNUM_UI_LOCAL Panel& setStyleInternal(PanelStyle style, Args... args);

        PanelStyle _style;
        /* 2 bytes free (_style fits into padding of Widget) */
        LayerDataHandle _backgroundData;
};

}}

#endif
