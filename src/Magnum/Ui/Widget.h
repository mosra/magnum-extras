#ifndef Magnum_Ui_Widget_h
#define Magnum_Ui_Widget_h
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

/** @file
 * @brief Class @ref Magnum::Ui::Widget
 */

#include <functional>
#include <Magnum/Magnum.h>
#include <Magnum/Math/Range.h>

#include "Magnum/Ui/Ui.h"
#include "Magnum/Ui/visibility.h"

namespace Magnum { namespace Ui {

/**
@brief Widget flag

@see @ref WidgetFlags, @ref Widget::flags()
@experimental
*/
enum class WidgetFlag: UnsignedInt {
    /**
     * The widget is currently under mouse cursor.
     * @see @ref Widget::hoverEvent()
     */
    Hovered = 1 << 0,

    /**
     * The widget is currently pressed.
     * @see @ref Widget::pressEvent()
     */
    Pressed = 1 << 1,

    /**
     * The widget has been tapped, meaning both press event and
     * release event happened on it and the user didn't blur it since.
     * @see @ref Widget::pressEvent()
     */
    Active = 1 << 2,

    /**
     * The widget was disabled, it is visible but it is not receiving any
     * events until it's re-enabled again using @ref Widget::enable() /
     * @ref Widget::setEnabled().
     */
    Disabled = 1 << 3,

    /**
     * The widget was hidden, is not visible and it is not receiving any
     * events until it's shown again using @ref Widget::show() /
     * @ref Widget::setVisible().
     */
    Hidden = 1 << 4
};

/** @debugoperatorenum{WidgetFlag}
 * @experimental
 */
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, WidgetFlag value);

/**
@brief Widget flags

@see @ref Widget::flags()
@experimental
*/
typedef Containers::EnumSet<WidgetFlag> WidgetFlags;

/** @debugoperatorenum{WidgetFlags}
 * @experimental
 */
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, WidgetFlags value);

CORRADE_ENUMSET_OPERATORS(WidgetFlags)

/**
@brief Base for widgets

@experimental
*/
class MAGNUM_UI_EXPORT Widget {
    friend class AbstractPlane;

    public:
        /**
         * @brief Disable a set of widgets
         *
         * Convenience batch alternative to @ref disable().
         */
        static void disable(std::initializer_list<std::reference_wrapper<Widget>> widgets);

        /**
         * @brief Enable a set of widgets
         *
         * Convenience batch alternative to @ref enable().
         */
        static void enable(std::initializer_list<std::reference_wrapper<Widget>> widgets);

        /**
         * @brief Enable or disable a set of widgets
         *
         * Convenience batch alternative to @ref setEnabled().
         */
        static void setEnabled(bool enabled, std::initializer_list<std::reference_wrapper<Widget>> widgets);

        /**
         * @brief Hide a set of widgets
         *
         * Convenience batch alternative to @ref hide().
         */
        static void hide(std::initializer_list<std::reference_wrapper<Widget>> widgets);

        /**
         * @brief Show a set of widgets
         *
         * Convenience batch alternative to @ref show().
         */
        static void show(std::initializer_list<std::reference_wrapper<Widget>> widgets);

        /**
         * @brief Set a set of widgets visible
         *
         * Convenience batch alternative to @ref setVisible().
         */
        static void setVisible(bool visible, std::initializer_list<std::reference_wrapper<Widget>> widgets);

        /**
         * @brief Constructor
         * @param plane         Plane this widget is a part of
         * @param anchor        Positioning anchor
         * @param padding       Padding for widgets inside
         */
        explicit Widget(AbstractPlane& plane, const Anchor& anchor, const Range2D& padding = {});

        /** @brief Widget rectangle */
        Range2D rect() const { return _rect; }

        /** @brief Padding for widgets inside */
        Range2D padding() const { return _padding; }

        /** @brief Flags */
        WidgetFlags flags() const { return _flags; }

        /**
         * @brief Disable widget
         * @return Reference to self (for method chaining)
         *
         * Disabled widget does not receive any input events until it is
         * enabled again using @ref enable().
         * @see @ref disable(std::initializer_list<std::reference_wrapper<Widget>>),
         *      @ref hide()
         */
        Widget& disable();

        /**
         * @brief Enable widget
         * @return Reference to self (for method chaining)
         *
         * Enables the widget again after it was disabled using @ref disable().
         * @see @ref enable(std::initializer_list<std::reference_wrapper<Widget>>),
         *      @ref show()
         */
        Widget& enable();

        /**
         * @brief Enable or disable widget
         * @return Reference to self (for method chaining)
         *
         * @see @ref setEnabled(bool, std::initializer_list<std::reference_wrapper<Widget>>),
         *      @ref enable(), @ref disable(), @ref setVisible()
         */
        Widget& setEnabled(bool enabled);

        /**
         * @brief Hide widget
         * @return Reference to self (for method chaining)
         *
         * Hidden widget is not visible and doesn't receive any input events
         * until it is shown again using @ref show().
         * @see @ref hide(std::initializer_list<std::reference_wrapper<Widget>>),
         *      @ref disable()
         */
        Widget& hide();

        /**
         * @brief Show widget
         * @return Reference to self (for method chaining)
         *
         * Shows the widget again after it was hidden using @ref hide().
         * @see @ref show(std::initializer_list<std::reference_wrapper<Widget>>),
         *      @ref enable()
         */
        Widget& show();

        /**
         * @brief Set widget visible
         * @return Reference to self (for method chaining)
         *
         * @see @ref setVisible(bool, std::initializer_list<std::reference_wrapper<Widget>>),
         *      @ref show(), @ref hide(), @ref setEnabled()
         */
        Widget& setVisible(bool visible);

    protected:
        ~Widget();

        /** @brief Plane this widget is a part of */
        AbstractPlane& plane() { return _plane; }
        const AbstractPlane& plane() const { return _plane; } /**< @overload */

    #ifdef DOXYGEN_GENERATING_OUTPUT
    protected:
    #else
    private:
    #endif
        /**
         * @brief Update the widget after its state changed
         *
         * Expects to do only visual update, should not do any expensive
         * operations like text relayouting.
         */
        virtual void update();

        /**
         * @brief Hover event
         * @return `True` if the event was accepted, `false` if it was ignored
         *
         * Called when the widget is hovered by the mouse or when the mouse
         * leaves it again. Use @ref Flag::Hovered to check for the state.
         * Default implementation does nothing and returns `false`.
         */
        virtual bool hoverEvent();

        /**
         * @brief Press event
         * @return `True` if the event was accepted, `false` if it was ignored
         *
         * When the widget is pressed, @ref Flag::Pressed is set and this
         * function is called. Default implementation does nothing and returns
         * `false`.
         */
        virtual bool pressEvent();

        /**
         * @brief Release event
         * @return `True` if the event was accepted, `false` if it was ignored
         *
         * On release after previous press event, @ref Flag::Pressed flag is
         * removed and this function is called. Default implementation does
         * nothing and returns `false`.
         */
        virtual bool releaseEvent();

        /**
         * @brief Focus event
         * @return `True` if the event was accepted, `false` if it was ignored
         *
         * If both press and release event happened on the same widget,
         * @ref Flag::Active is set and this function is called. Default
         * implementation does nothing and returns `false`.
         */
        virtual bool focusEvent();

        /**
         * @brief Blur event
         * @return `True` if the event was accepted, `false` if it was ignored
         *
         * If the widget was active previously and the user tapped outside,
         * @ref Flag::Active is removed and this function is called. Default
         * implementation does nothing and returns `false`.
         */
        virtual bool blurEvent();

    private:
        AbstractPlane& _plane;
        Range2D _rect, _padding;
        WidgetFlags _flags;
        std::size_t _planeIndex;
};

}}

#endif
