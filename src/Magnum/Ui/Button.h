#ifndef Magnum_Ui_Button_h
#define Magnum_Ui_Button_h
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
 * @brief Class @ref Magnum::Ui::Button, function @ref Magnum::Ui::button(), enum @ref Magnum::Ui::ButtonStyle
 * @m_since_latest_{extras}
 */

#include <Corrade/Utility/Macros.h> /* CORRADE_NODISCARD() */

#include "Magnum/Ui/Widget.h"

namespace Magnum { namespace Ui {

/**
@brief Button style
@m_since_latest_{extras}

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
@m_since_latest_{extras}
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, ButtonStyle value);

/**
@brief Button widget
@m_since_latest_{extras}
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
         * using @ref setIcon() and @ref setText(). Use @ref onTrigger() to
         * connect the button to an action.
         */
        explicit Button(Anchor anchor, Icon icon, ButtonStyle style = ButtonStyle::Default);

        /**
         * @brief Construct a text button
         * @param anchor            Positioning anchor
         * @param text              Button text. Passing an empty string makes
         *      the button empty.
         * @param textProperties    Text shaping and layouting properties
         * @param style             Button style
         *
         * The button can be subsequently converted to icon-only or icon + text
         * using @ref setIcon() and @ref setText(). Use @ref onTrigger() to
         * connect the button to an action.
         */
        explicit Button(Anchor anchor, Containers::StringView text, const TextProperties& textProperties, ButtonStyle style = ButtonStyle::Default);
        /** @overload */
        explicit Button(Anchor anchor, Containers::StringView text, ButtonStyle style = ButtonStyle::Default);

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
         * using @ref setIcon() and @ref setText(). Use @ref onTrigger() to
         * connect the button to an action.
         */
        explicit Button(Anchor anchor, Icon icon, Containers::StringView text, const TextProperties& textProperties, ButtonStyle style = ButtonStyle::Default);
        /** @overload */
        explicit Button(Anchor anchor, Icon icon, Containers::StringView text, ButtonStyle style = ButtonStyle::Default);

        /**
         * @brief Construct a non-owned icon button
         *
         * Like @ref Button(Anchor, Icon, ButtonStyle) but the widget node
         * doesn't get removed on destruction. Instead, it gets removed either
         * once any parent node is removed, or when
         * @ref AbstractUserInterface::removeNode() is explicitly called on
         * @ref node().
         * @see @ref isOwned(),
         *      @ref button(Anchor, Icon, Containers::Function<void()>&&, ButtonStyle)
         */
        explicit Button(NonOwnedT, Anchor anchor, Icon icon, ButtonStyle style = ButtonStyle::Default);

        /**
         * @brief Construct a non-owned text button
         *
         * Like @ref Button(Anchor, Containers::StringView, const TextProperties&, ButtonStyle)
         * but the widget node doesn't get removed on destruction. Instead, it
         * gets removed either once any parent node is removed, or when
         * @ref AbstractUserInterface::removeNode() is explicitly called on
         * @ref node().
         * @see @ref isOwned(),
         *      @ref button(Anchor, Containers::StringView, const TextProperties&, Containers::Function<void()>&&, ButtonStyle)
         */
        explicit Button(NonOwnedT, Anchor anchor, Containers::StringView text, const TextProperties& textProperties, ButtonStyle style = ButtonStyle::Default);
        /** @overload */
        explicit Button(NonOwnedT, Anchor anchor, Containers::StringView text, ButtonStyle style = ButtonStyle::Default);

        /**
         * @brief Construct a non-owned icon + text button
         *
         * Like @ref Button(Anchor, Icon, Containers::StringView, const TextProperties&, ButtonStyle)
         * but the widget node doesn't get removed on destruction. Instead, it
         * gets removed either once any parent node is removed, or when
         * @ref AbstractUserInterface::removeNode() is explicitly called on
         * @ref node().
         * @see @ref isOwned(),
         *      @ref button(Anchor, Icon, Containers::StringView, const TextProperties&, Containers::Function<void()>&&, ButtonStyle)
         */
        explicit Button(NonOwnedT, Anchor anchor, Icon icon, Containers::StringView text, const TextProperties& textProperties, ButtonStyle style = ButtonStyle::Default);
        /** @overload */
        explicit Button(NonOwnedT, Anchor anchor, Icon icon, Containers::StringView text, ButtonStyle style = ButtonStyle::Default);

        /** @copydoc AbstractWidget::AbstractWidget(NoCreateT) */
        explicit Button(NoCreateT): Widget{NoCreate}, _style{}, _icon{}, _backgroundData{}, _iconData{}, _textData{} {}

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
         * @brief Connect to the button being triggered
         * @return New data handle
         *
         * Expects that the @p function is not @cpp nullptr @ce. Unless
         * explicitly removed through the returned @ref DataHandle, the
         * callback is active until the button itself is removed. See
         * @ref onTriggerScoped() for an alternative.
         *
         * Subsequent calls to this function don't replace existing calls but
         * add to them. There is no guarantee on the order in which multiple
         * functions are called.
         *
         * Delegates to @ref EventLayer::onTapOrClick(), see its documentation
         * for detailed behavior description.
         */
        DataHandle onTrigger(Containers::Function<void()>&& function);

        /**
         * @brief Scoped connection to the button being triggered
         * @return Event connection instance
         *
         * Expects that the @p function is not @cpp nullptr @ce. Compared to
         * @ref onTrigger(), the callback is removed when the returned
         * @ref EventConnection is destructed.
         */
        CORRADE_NODISCARD("the call is removed when the returned EventConnection is destructed") EventConnection onTriggerScoped(Containers::Function<void()>&& function);

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
        /* 2 bytes free (_style fits into padding of Widget) */
        Icon _icon;
        LayerDataHandle _backgroundData, _iconData, _textData;
};

/**
@brief Stateless icon button widget
@param anchor           Positioning anchor
@param icon             Button icon. Passing @ref Icon::None makes the button
    empty.
@param trigger          Function to execute when the button is triggered or
    @cpp nullptr @ce
@param style            Button style
@return The @p anchor verbatim
@m_since_latest_{extras}

Equivalent to constructing a non-owned @ref Button using
@ref Button::Button(NonOwnedT, Anchor, Icon, ButtonStyle), passing @p trigger to
@ref Button::onTrigger() if it's not @cpp nullptr @ce, and discarding the
stateful instance. See documentation of these functions for more information.
*/
MAGNUM_UI_EXPORT Anchor button(Anchor anchor, Icon icon, Containers::Function<void()>&& trigger, ButtonStyle style = ButtonStyle::Default);

/**
@brief Stateless text button widget
@param anchor           Positioning anchor
@param text             Button text. Passing an empty string makes the button
    empty.
@param textProperties   Text shaping and layouting properties
@param trigger          Function to execute when the button is triggered or
    @cpp nullptr @ce
@param style            Button style
@return The @p anchor verbatim
@m_since_latest_{extras}

Equivalent to constructing a non-owned @ref Button using
@ref Button::Button(NonOwnedT, Anchor, Containers::StringView, const TextProperties&, ButtonStyle),
passing @p trigger to @ref Button::onTrigger() if it's not @cpp nullptr @ce,
and discarding the stateful instance. See documentation of these functions for
more information.
*/
MAGNUM_UI_EXPORT Anchor button(Anchor anchor, Containers::StringView text, const TextProperties& textProperties, Containers::Function<void()>&& trigger, ButtonStyle style = ButtonStyle::Default);
/**
@overload
@m_since_latest_{extras}
*/
MAGNUM_UI_EXPORT Anchor button(Anchor anchor, Containers::StringView text, Containers::Function<void()>&& trigger, ButtonStyle style = ButtonStyle::Default);

/**
@brief Stateless icon + text button widget
@param anchor           Positioning anchor
@param icon             Button icon. Passing @ref Icon::None creates the button
    without an icon.
@param text             Button text. Passing an empty string creates the button
    without a text.
@param textProperties   Text shaping and layouting properties
@param trigger          Function to execute when the button is triggered or
    @cpp nullptr @ce
@param style            Button style
@return The @p anchor verbatim
@m_since_latest_{extras}

Equivalent to constructing a non-owned @ref Button using
@ref Button::Button(NonOwnedT, Anchor, Icon, Containers::StringView, const TextProperties&, ButtonStyle),
passing @p trigger to @ref Button::onTrigger() if it's not @cpp nullptr @ce,
and discarding the stateful instance. See documentation of these functions for
more information.
*/
MAGNUM_UI_EXPORT Anchor button(Anchor anchor, Icon icon, Containers::StringView text, const TextProperties& textProperties, Containers::Function<void()>&& trigger, ButtonStyle style = ButtonStyle::Default);
/**
@overload
@m_since_latest_{extras}
*/
MAGNUM_UI_EXPORT Anchor button(Anchor anchor, Icon icon, Containers::StringView text, Containers::Function<void()>&& trigger, ButtonStyle style = ButtonStyle::Default);

}}

#endif
