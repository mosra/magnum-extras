#ifndef Magnum_Ui_Button_h
#define Magnum_Ui_Button_h
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023, 2024
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
 * @brief Class @ref Magnum::Ui::Button, function @ref Magnum::Ui::button(), enum @ref Magnum::Ui::ButtonStyle
 * @m_since_latest
 */

#include "Magnum/Ui/Widget.h"

namespace Magnum { namespace Ui {

/**
@brief Button style
@m_since_latest

@see @ref Button, @ref button()
*/
enum class ButtonStyle: UnsignedByte {
    Default,        /**< Default */
    Primary,        /**< Primary */
    Success,        /**< Success */
    Warning,        /**< Warning */
    Danger,         /**< Danger */
    Info,           /**< Info */
    Dim,            /**< Dim */
    Flat            /**< Flat */
};

/**
@debugoperatorenum{ButtonStyle}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, ButtonStyle value);

/**
@brief Button widget
@m_since_latest
*/
class MAGNUM_UI_EXPORT Button: public Widget {
    public:
        /**
         * @brief Construct an icon button
         * @param anchor            Positioning anchor
         * @param icon              Button icon. Passing @ref Icon::None makes
         *      the button empty.
         * @param style             Button style
         *
         * The button can be subsequently converted to text-only or icon + text
         * using @ref setIcon() and @ref setText().
         * @see @ref button(const Anchor&, Icon, ButtonStyle)
         */
        explicit Button(const Anchor& anchor, Icon icon, ButtonStyle style = ButtonStyle::Default);

        /**
         * @brief Construct a text button
         * @param anchor            Positioning anchor
         * @param text              Button text. Passing an empty string makes
         *      the button empty.
         * @param textProperties    Text shaping and layouting properties
         * @param style             Button style
         *
         * The button can be subsequently converted to icon-only or icon + text
         * using @ref setIcon() and @ref setText().
         * @see @ref button(const Anchor&, Containers::StringView, const TextProperties&, ButtonStyle)
         */
        explicit Button(const Anchor& anchor, Containers::StringView text, const TextProperties& textProperties, ButtonStyle style = ButtonStyle::Default);
        /** @overload */
        explicit Button(const Anchor& anchor, Containers::StringView text, ButtonStyle style = ButtonStyle::Default);

        /**
         * @brief Construct an icon + text button
         * @param anchor            Positioning anchor
         * @param icon              Button icon. Passing @ref Icon::None
         *      creates the button without an icon.
         * @param text              Button text. Passing an empty string
         *      creates the button without a text.
         * @param textProperties    Text shaping and layouting properties
         * @param style             Button style
         *
         * The button can be subsequently converted to icon-only or text-only
         * using @ref setIcon() and @ref setText().
         * @see @ref button(const Anchor&, Icon, Containers::StringView, const TextProperties&, ButtonStyle)
         */
        explicit Button(const Anchor& anchor, Icon icon, Containers::StringView text, const TextProperties& textProperties, ButtonStyle style = ButtonStyle::Default);
        /** @overload */
        explicit Button(const Anchor& anchor, Icon icon, Containers::StringView text, ButtonStyle style = ButtonStyle::Default);

        /**
         * @brief Construct with no underlying node
         *
         * The instance is equivalent to a moved-out state, i.e. not usable
         * for anything. Move another instance over it to make it useful.
         */
        explicit Button(NoCreateT, UserInterface& ui): Widget{NoCreate, ui}, _style{}, _icon{}, _backgroundData{}, _iconData{}, _textData{} {}

        /** @brief Style */
        ButtonStyle style() const { return _style; }

        /**
         * @brief Set style
         * @return Reference to self (for method chaining)
         *
         * Note that calling this function doesn't change the font if the new
         * style uses a different one, you have to call @ref setText()
         * afterwards to make it pick it up.
         * @see @ref setIcon(), @ref setText()
         */
        Button& setStyle(ButtonStyle style);

        /** @brief Icon */
        Icon icon() const { return _icon; }

        /**
         * @brief Set icon
         * @return Reference to self (for method chaining)
         *
         * Passing @ref Icon::None removes the icon.
         * @see @ref setText(), @ref setStyle()
         */
        Button& setIcon(Icon icon);

        /**
         * @brief Set text
         * @return Reference to self (for method chaining)
         *
         * Passing an empty @p text removes the text.
         * @see @ref setIcon(), @ref setStyle()
         */
        #ifdef DOXYGEN_GENERATING_OUTPUT
        Button& setText(Containers::StringView text, const TextProperties& textProperties = {});
        #else
        /* To avoid having to include TextProperties.h */
        Button& setText(Containers::StringView text, const TextProperties& textProperties);
        Button& setText(Containers::StringView text);
        #endif

        /**
         * @brief Background data
         *
         * Exposed mainly for testing purposes, not meant to be modified
         * directly.
         */
        DataHandle backgroundData() const;

        /**
         * @brief Icon data or @ref DataHandle::Null
         *
         * Exposed mainly for testing purposes, not meant to be modified
         * directly.
         */
        DataHandle iconData() const;

        /**
         * @brief Text data or @ref DataHandle::Null
         *
         * Exposed mainly for testing purposes, not meant to be modified
         * directly.
         */
        DataHandle textData() const;

        #ifndef DOXYGEN_GENERATING_OUTPUT
        _MAGNUM_UI_WIDGET_SUBCLASS_IMPLEMENTATION(Button) /* LCOV_EXCL_LINE */
        #endif

    private:
        ButtonStyle _style;
        /* 3 bytes free */
        Icon _icon;
        LayerDataHandle _backgroundData, _iconData, _textData;
};

/**
@brief Stateless icon button widget
@param anchor           Positioning anchor
@param icon             Button icon. Passing @ref Icon::None makes the button
    empty.
@param style            Button style
@return The @p anchor verbatim
@m_since_latest

Compared to @ref Button::Button(const Anchor&, Icon, ButtonStyle)
this creates a stateless button that doesn't have any class instance that would
need to be kept in scope and eventually destructed, making it more lightweight.
As a consequence it can't have its style, icon or text subsequently changed and
is removed only when the node or its parent get removed.
*/
MAGNUM_UI_EXPORT Anchor button(const Anchor& anchor, Icon icon, ButtonStyle style = ButtonStyle::Default);

/**
@brief Stateless text button widget
@param anchor           Positioning anchor
@param text             Button text. Passing an empty string makes the button
    empty.
@param style            Button style
@param textProperties   Text shaping and layouting properties
@return The @p anchor verbatim
@m_since_latest

Compared to @ref Button::Button(const Anchor&, Containers::StringView, const TextProperties&, ButtonStyle)
this creates a stateless button that doesn't have any class instance that would
need to be kept in scope and eventually destructed, making it more lightweight.
As a consequence it can't have its style, icon or text subsequently changed and
is removed only when the node or its parent get removed.
*/
MAGNUM_UI_EXPORT Anchor button(const Anchor& anchor, Containers::StringView text, const TextProperties& textProperties, ButtonStyle style = ButtonStyle::Default);
/** @overload */
MAGNUM_UI_EXPORT Anchor button(const Anchor& anchor, Containers::StringView text, ButtonStyle style = ButtonStyle::Default);

/**
@brief Stateless icon + text button widget
@param anchor           Positioning anchor
@param icon             Button icon. Passing @ref Icon::None creates the button
    without an icon.
@param text             Button text. Passing an empty string creates the button
    without a text.
@param textProperties   Text shaping and layouting properties
@param style            Button style
@return The @p anchor verbatim
@m_since_latest

Compared to @ref Button::Button(const Anchor&, Icon, Containers::StringView, const TextProperties&, ButtonStyle)
this creates a stateless button that doesn't have any class instance that would
need to be kept in scope and eventually destructed, making it more lightweight.
As a consequence it can't have its style, icon or text subsequently changed and
is removed only when the node or its parent get removed.
*/
MAGNUM_UI_EXPORT Anchor button(const Anchor& anchor, Icon icon, Containers::StringView text, const TextProperties& textProperties, ButtonStyle style = ButtonStyle::Default);
/** @overload */
MAGNUM_UI_EXPORT Anchor button(const Anchor& anchor, Icon icon, Containers::StringView text, ButtonStyle style = ButtonStyle::Default);

}}

#endif
