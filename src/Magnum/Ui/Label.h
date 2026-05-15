#ifndef Magnum_Ui_Label_h
#define Magnum_Ui_Label_h
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
 * @brief Class @ref Magnum::Ui::Label, function @ref Magnum::Ui::label(), enum @ref Magnum::Ui::LabelStyle
 * @m_since_latest_{extras}
 */

#include "Magnum/Ui/Anchor.h" /* otherwise can't inline too many oneliners */
#include "Magnum/Ui/Widget.h"

namespace Magnum { namespace Ui {

/**
@brief Label style
@m_since_latest_{extras}

@see @ref Label, @ref label()
*/
enum class LabelStyle: UnsignedByte {
    Default,        /**< Default */
    Primary,        /**< Primary */
    Success,        /**< Success */
    Warning,        /**< Warning */
    Danger,         /**< Danger */
    Info,           /**< Info */
    Dim,            /**< Dim */
    Title,          /**< Title, with a larger text */
};

/**
@debugoperatorenum{LabelStyle}
@m_since_latest_{extras}
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, LabelStyle value);

/**
@brief Label widget
@m_since_latest_{extras}
*/
class MAGNUM_UI_EXPORT Label: public Widget {
    public:
        /**
         * @brief Construct an icon label
         * @param anchor            Positioning anchor
         * @param icon              Label icon. Passing @ref Icon::None makes
         *      the label empty.
         * @param style             Label style
         *
         * The label can be subsequently converted to a text label using
         * @ref setText().
         */
        explicit Label(Anchor anchor, Icon icon, LabelStyle style = LabelStyle::Default);

        /**
         * @brief Construct a text label
         * @param anchor            Positioning anchor
         * @param text              Label text. Passing an empty string makes
         *      the label empty.
         * @param textProperties    Text shaping and layouting properties
         * @param style             Label style
         *
         * The label can be subsequently converted to an icon label using
         * @ref setIcon().
         */
        explicit Label(Anchor anchor, Containers::StringView text, const TextProperties& textProperties, LabelStyle style = LabelStyle::Default);
        /** @overload */
        explicit Label(Anchor anchor, Containers::StringView text, LabelStyle style = LabelStyle::Default);

        /**
         * @brief Construct a label with an integer data binding
         * @param anchor            Positioning anchor
         * @param query             @ref DataLayer storage query
         * @param style             Label style
         *
         * Formats the value with a default-constructed @ref DecimalFormatter.
         * Use the @ref Label(Anchor, const StorageQuery<Int>&, const DecimalFormatter&, LabelStyle)
         * or @ref Label(Anchor, const StorageQuery<Int>&, const HexadecimalFormatter&, LabelStyle)
         * constructors and their overloads to explicitly supply a configured
         * @ref DecimalFormatter or @ref HexadecimalFormatter instance.
         *
         * Note that it's not possible to supply custom @ref TextProperties
         * this way. If you need to override these, supply the @p query along
         * with a lambda calling @ref setText() to @ref DataLayer::onUpdate().
         */
        explicit Label(Anchor anchor, const StorageQuery<Int>& query, LabelStyle style = LabelStyle::Default);
        /** @overload */
        explicit Label(Anchor anchor, const StorageQuery<UnsignedInt>& query, LabelStyle style = LabelStyle::Default);
        /** @overload */
        explicit Label(Anchor anchor, const StorageQuery<Long>& query, LabelStyle style = LabelStyle::Default);
        /** @overload */
        explicit Label(Anchor anchor, const StorageQuery<UnsignedLong>& query, LabelStyle style = LabelStyle::Default);

        /**
         * @brief Construct a label with an integer data binding and a custom decimal formatter
         * @param anchor            Positioning anchor
         * @param query             @ref DataLayer storage query
         * @param formatter         Formatter instance
         * @param style             Label style
         *
         * Use the @ref Label(Anchor, const StorageQuery<Int>&, const HexadecimalFormatter&, LabelStyle)
         * constructor and its overloads to format the value as hexadecimal
         * instead. Passing a default-constructed @ref DecimalFormatter
         * instance is equivalent to using the @ref Label(Anchor, const StorageQuery<Int>&, LabelStyle)
         * constructor.
         *
         * Note that it's not possible to supply custom @ref TextProperties
         * this way. If you need to override these, supply the @p query along
         * with a lambda calling @ref setText() to @ref DataLayer::onUpdate().
         */
        explicit Label(Anchor anchor, const StorageQuery<Int>& query, const DecimalFormatter& formatter, LabelStyle style = LabelStyle::Default);
        /** @overload */
        explicit Label(Anchor anchor, const StorageQuery<UnsignedInt>& query, const DecimalFormatter& formatter, LabelStyle style = LabelStyle::Default);
        /** @overload */
        explicit Label(Anchor anchor, const StorageQuery<Long>& query, const DecimalFormatter& formatter, LabelStyle style = LabelStyle::Default);
        /** @overload */
        explicit Label(Anchor anchor, const StorageQuery<UnsignedLong>& query, const DecimalFormatter& formatter, LabelStyle style = LabelStyle::Default);

        /**
         * @brief Construct a label with an integer data binding and a custom hexadecimal formatter
         * @param anchor            Positioning anchor
         * @param query             @ref DataLayer storage query
         * @param formatter         Formatter instance
         * @param style             Label style
         *
         * Use the @ref Label(Anchor, const StorageQuery<Int>&, const DecimalFormatter&, LabelStyle)
         * constructor and its overloads to format the value as decimal
         * instead, or @ref Label(Anchor, const StorageQuery<Int>&, LabelStyle)
         * to use a default-constructed @ref DecimalFormatter instance.
         *
         * Note that it's not possible to supply custom @ref TextProperties
         * this way. If you need to override these, supply the @p query along
         * with a lambda calling @ref setText() to @ref DataLayer::onUpdate().
         */
        explicit Label(Anchor anchor, const StorageQuery<Int>& query, const HexadecimalFormatter& formatter, LabelStyle style = LabelStyle::Default);
        /** @overload */
        explicit Label(Anchor anchor, const StorageQuery<UnsignedInt>& query, const HexadecimalFormatter& formatter, LabelStyle style = LabelStyle::Default);
        /** @overload */
        explicit Label(Anchor anchor, const StorageQuery<Long>& query, const HexadecimalFormatter& formatter, LabelStyle style = LabelStyle::Default);
        /** @overload */
        explicit Label(Anchor anchor, const StorageQuery<UnsignedLong>& query, const HexadecimalFormatter& formatter, LabelStyle style = LabelStyle::Default);

        /**
         * @brief Construct a label with a floating-point data binding
         * @param anchor            Positioning anchor
         * @param query             @ref DataLayer storage query
         * @param style             Label style
         *
         * Formats the value with a default-constructed @ref FloatFormatter.
         * Use the @ref Label(Anchor, const StorageQuery<Float>&, const FloatFormatter&, LabelStyle)
         * constructor and its overloads to explicitly supply a configured
         * @ref FloatFormatter instance.
         *
         * Note that it's not possible to supply custom @ref TextProperties
         * this way. If you need to override these, supply the @p query along
         * with a lambda calling @ref setText() to @ref DataLayer::onUpdate().
         */
        explicit Label(Anchor anchor, const StorageQuery<Float>& query, LabelStyle style = LabelStyle::Default);
        /** @overload */
        explicit Label(Anchor anchor, const StorageQuery<Double>& query, LabelStyle style = LabelStyle::Default);

        /**
         * @brief Construct a label with a float data binding and a custom formatter
         * @param anchor            Positioning anchor
         * @param query             @ref DataLayer storage query
         * @param formatter         Formatter instance
         * @param style             Label style
         *
         * Passing a default-constructed @ref FloatFormatter instance is
         * equivalent to using the @ref Label(Anchor, const StorageQuery<Float>&, LabelStyle)
         * constructor overloads.
         *
         * Note that it's not possible to supply custom @ref TextProperties
         * this way. If you need to override these, supply the @p query along
         * with a lambda calling @ref setText() to @ref DataLayer::onUpdate().
         */
        explicit Label(Anchor anchor, const StorageQuery<Float>& query, const FloatFormatter& formatter, LabelStyle style = LabelStyle::Default);
        /** @overload */
        explicit Label(Anchor anchor, const StorageQuery<Double>& query, const FloatFormatter& formatter, LabelStyle style = LabelStyle::Default);

        /**
         * @brief Construct a label with a string data binding
         * @param anchor            Positioning anchor
         * @param query             @ref DataLayer storage query
         * @param style             Label style
         *
         * Note that it's not possible to supply custom @ref TextProperties
         * this way. If you need to override these, supply the @p query along
         * with a lambda calling @ref setText() to @ref DataLayer::onUpdate().
         */
        explicit Label(Anchor anchor, const StorageQuery<Containers::StringView>& query, LabelStyle style = LabelStyle::Default);

        /**
         * @brief Construct a label with an icon data binding
         * @param anchor            Positioning anchor
         * @param query             @ref DataLayer storage query
         * @param style             Label style
         *
         * Note that it's not possible to supply custom @ref TextProperties
         * this way. If you need to override these, supply the @p query along
         * with a lambda calling @ref setIcon() to @ref DataLayer::onUpdate().
         */
        explicit Label(Anchor anchor, const StorageQuery<Icon>& query, LabelStyle style = LabelStyle::Default);

        /**
         * @brief Construct a non-owned icon label
         *
         * Like @ref Label(Anchor, Icon, LabelStyle) but the widget node
         * doesn't get removed on destruction. Instead, it gets removed either
         * once any parent node is removed, or when
         * @ref AbstractUserInterface::removeNode() is explicitly called on
         * @ref node().
         * @see @ref isOwned(), @ref label(Anchor, Icon, LabelStyle)
         */
        explicit Label(NonOwnedT, Anchor anchor, Icon icon, LabelStyle style = LabelStyle::Default);

        /**
         * @brief Construct a non-owned text label
         *
         * Like @ref Label(Anchor, Containers::StringView, const TextProperties&, LabelStyle)
         * but the widget node doesn't get removed on destruction. Instead, it
         * gets removed either once any parent node is removed, or when
         * @ref AbstractUserInterface::removeNode() is explicitly called on
         * @ref node().
         * @see @ref isOwned(), @ref label(Anchor, Containers::StringView, const TextProperties&, LabelStyle)
         */
        explicit Label(NonOwnedT, Anchor anchor, Containers::StringView text, const TextProperties& textProperties, LabelStyle style = LabelStyle::Default);
        /** @overload */
        explicit Label(NonOwnedT, Anchor anchor, Containers::StringView text, LabelStyle style = LabelStyle::Default);

        /**
         * @brief Construct a non-owned label with an integer data binding
         *
         * Like @ref Label(Anchor, const StorageQuery<Int>&, LabelStyle) but
         * the widget node doesn't get removed on destruction. Instead, it gets
         * removed either once any parent node is removed, or when
         * @ref AbstractUserInterface::removeNode() is explicitly called on
         * @ref node().
         * @see @ref isOwned(), @ref label(Anchor, const StorageQuery<Int>&, LabelStyle)
         */
        explicit Label(NonOwnedT, Anchor anchor, const StorageQuery<Int>& query, LabelStyle style = LabelStyle::Default): Label{anchor, query, style} {
            makeNonOwned();
        }
        /** @overload */
        explicit Label(NonOwnedT, Anchor anchor, const StorageQuery<UnsignedInt>& query, LabelStyle style = LabelStyle::Default): Label{anchor, query, style} {
            makeNonOwned();
        }
        /** @overload */
        explicit Label(NonOwnedT, Anchor anchor, const StorageQuery<Long>& query, LabelStyle style = LabelStyle::Default): Label{anchor, query, style} {
            makeNonOwned();
        }
        /** @overload */
        explicit Label(NonOwnedT, Anchor anchor, const StorageQuery<UnsignedLong>& query, LabelStyle style = LabelStyle::Default): Label{anchor, query, style} {
            makeNonOwned();
        }

        /**
         * @brief Construct a non-owned label with an integer data binding and a custom decimal formatter
         *
         * Like @ref Label(Anchor, const StorageQuery<Int>&, const DecimalFormatter&, LabelStyle)
         * but the widget node doesn't get removed on destruction. Instead, it
         * gets removed either once any parent node is removed, or when
         * @ref AbstractUserInterface::removeNode() is explicitly called on
         * @ref node().
         * @see @ref isOwned(), @ref label(Anchor, const StorageQuery<Int>&, const DecimalFormatter&, LabelStyle)
         */
        explicit Label(NonOwnedT, Anchor anchor, const StorageQuery<Int>& query, const DecimalFormatter& formatter, LabelStyle style = LabelStyle::Default): Label{anchor, query, formatter, style} {
            makeNonOwned();
        }
        /** @overload */
        explicit Label(NonOwnedT, Anchor anchor, const StorageQuery<UnsignedInt>& query, const DecimalFormatter& formatter, LabelStyle style = LabelStyle::Default): Label{anchor, query, formatter, style} {
            makeNonOwned();
        }
        /** @overload */
        explicit Label(NonOwnedT, Anchor anchor, const StorageQuery<Long>& query, const DecimalFormatter& formatter, LabelStyle style = LabelStyle::Default): Label{anchor, query, formatter, style} {
            makeNonOwned();
        }
        /** @overload */
        explicit Label(NonOwnedT, Anchor anchor, const StorageQuery<UnsignedLong>& query, const DecimalFormatter& formatter, LabelStyle style = LabelStyle::Default): Label{anchor, query, formatter, style} {
            makeNonOwned();
        }

        /**
         * @brief Construct a non-owned label with an integer data binding and a custom hexadecimal formatter
         *
         * Like @ref Label(Anchor, const StorageQuery<Int>&, const HexadecimalFormatter&, LabelStyle)
         * but the widget node doesn't get removed on destruction. Instead, it
         * gets removed either once any parent node is removed, or when
         * @ref AbstractUserInterface::removeNode() is explicitly called on
         * @ref node().
         * @see @ref isOwned(), @ref label(Anchor, const StorageQuery<Int>&, const HexadecimalFormatter&, LabelStyle)
         */
        explicit Label(NonOwnedT, Anchor anchor, const StorageQuery<Int>& query, const HexadecimalFormatter& formatter, LabelStyle style = LabelStyle::Default): Label{anchor, query, formatter, style} {
            makeNonOwned();
        }
        /** @overload */
        explicit Label(NonOwnedT, Anchor anchor, const StorageQuery<UnsignedInt>& query, const HexadecimalFormatter& formatter, LabelStyle style = LabelStyle::Default): Label{anchor, query, formatter, style} {
            makeNonOwned();
        }
        /** @overload */
        explicit Label(NonOwnedT, Anchor anchor, const StorageQuery<Long>& query, const HexadecimalFormatter& formatter, LabelStyle style = LabelStyle::Default): Label{anchor, query, formatter, style} {
            makeNonOwned();
        }
        /** @overload */
        explicit Label(NonOwnedT, Anchor anchor, const StorageQuery<UnsignedLong>& query, const HexadecimalFormatter& formatter, LabelStyle style = LabelStyle::Default): Label{anchor, query, formatter, style} {
            makeNonOwned();
        }

        /**
         * @brief Construct a non-owned label with a float data binding
         *
         * Like @ref Label(Anchor, const StorageQuery<Float>&, LabelStyle) but
         * the widget node doesn't get removed on destruction. Instead, it gets
         * removed either once any parent node is removed, or when
         * @ref AbstractUserInterface::removeNode() is explicitly called on
         * @ref node().
         * @see @ref isOwned(), @ref label(Anchor, const StorageQuery<Float>&, LabelStyle)
         */
        explicit Label(NonOwnedT, Anchor anchor, const StorageQuery<Float>& query, LabelStyle style = LabelStyle::Default): Label{anchor, query, style} {
            makeNonOwned();
        }
        /** @overload */
        explicit Label(NonOwnedT, Anchor anchor, const StorageQuery<Double>& query, LabelStyle style = LabelStyle::Default): Label{anchor, query, style} {
            makeNonOwned();
        }

        /**
         * @brief Construct a non-owned label with a float data binding and a custom formatter
         *
         * Like @ref Label(Anchor, const StorageQuery<Float>&, const FloatFormatter&, LabelStyle)
         * but the widget node doesn't get removed on destruction. Instead, it
         * gets removed either once any parent node is removed, or when
         * @ref AbstractUserInterface::removeNode() is explicitly called on
         * @ref node().
         * @see @ref isOwned(), @ref label(Anchor, const StorageQuery<Float>&, const FloatFormatter&, LabelStyle)
         */
        explicit Label(NonOwnedT, Anchor anchor, const StorageQuery<Float>& query, const FloatFormatter& formatter, LabelStyle style = LabelStyle::Default): Label{anchor, query, formatter, style} {
            makeNonOwned();
        }
        /** @overload */
        explicit Label(NonOwnedT, Anchor anchor, const StorageQuery<Double>& query, const FloatFormatter& formatter, LabelStyle style = LabelStyle::Default): Label{anchor, query, formatter, style} {
            makeNonOwned();
        }

        /**
         * @brief Construct a non-owned label with a string data binding
         *
         * Like @ref Label(Anchor, const StorageQuery<Containers::StringView>&, LabelStyle)
         * but the widget node doesn't get removed on destruction. Instead, it
         * gets removed either once any parent node is removed, or when
         * @ref AbstractUserInterface::removeNode() is explicitly called on
         * @ref node().
         * @see @ref isOwned(), @ref label(Anchor, const StorageQuery<Containers::StringView>&, LabelStyle)
         */
        explicit Label(NonOwnedT, Anchor anchor, const StorageQuery<Containers::StringView>& query, LabelStyle style = LabelStyle::Default): Label{anchor, query, style} {
            makeNonOwned();
        }

        /**
         * @brief Construct a non-owned label with an icon data binding
         *
         * Like @ref Label(Anchor, const StorageQuery<Icon>&, LabelStyle) but
         * the widget node doesn't get removed on destruction. Instead, it gets
         * removed either once any parent node is removed, or when
         * @ref AbstractUserInterface::removeNode() is explicitly called on
         * @ref node().
         * @see @ref isOwned(), @ref label(Anchor, const StorageQuery<Icon>&, LabelStyle)
         */
        explicit Label(NonOwnedT, Anchor anchor, const StorageQuery<Icon>& query, LabelStyle style = LabelStyle::Default): Label{anchor, query, style} {
            makeNonOwned();
        }

        /** @copydoc AbstractWidget::AbstractWidget(NoCreateT) */
        explicit Label(NoCreateT): Widget{NoCreate}, _style{}, _icon{}, _data{}, _dataBindingData{}, _layoutData{} {}

        /**
         * @brief Whether the label has a data binding
         *
         * Returns @cpp true @ce if the label was constructed using a
         * @ref StorageQuery, @cpp false @ce otherwise.
         */
        bool hasDataBinding() const { return _dataBindingData != DataHandle{}; }

        /** @brief Style */
        LabelStyle style() const { return _style; }

        /**
         * @brief Set style
         * @return Reference to self (for method chaining)
         *
         * Note that calling this function doesn't change the font if the new
         * style uses a different one, you have to call @ref setText()
         * afterwards to make it pick it up. This is the case especially when
         * switching to or from @ref LabelStyle::Title.
         * @see @ref setStyle(LabelStyle, Nanoseconds), @ref setIcon(),
         *      @ref setText()
         */
        Label& setStyle(LabelStyle style);

        /**
         * @brief Set style at given time
         * @return Reference to self (for method chaining)
         *
         * Compared to @ref setStyle(LabelStyle) may animate the style
         * transition if the theme defines an animation for it. The
         * @ref style() getter is however updated immediately always.
         */
        Label& setStyle(LabelStyle style, Nanoseconds time);

        /**
         * @brief Icon
         *
         * If the label is text-only or has neither an icon nor a text, returns
         * @ref Icon::None.
         *
         * @m_class{m-note m-warning}
         *
         * @par
         *      Note that @ref Icon::None is returned also for a label
         *      constructed using the @ref Label(Anchor, const StorageQuery<Icon>&, LabelStyle)
         *      constructor and equivalent, as the data binding has no way to
         *      know if the @ref Label instance is still alive in order to
         *      update this field there.
         */
        Icon icon() const { return _icon; }

        /**
         * @brief Set icon
         * @return Reference to self (for method chaining)
         *
         * Expects that the label doesn't have a data binding, as in that case
         * the label updates are driven from outside. If the label had a text
         * before, it's replaced with the icon. Passing @ref Icon::None makes
         * the label empty.
         * @see @ref hasDataBinding(), @ref setText(), @ref setStyle()
         */
        Label& setIcon(Icon icon);

        /**
         * @brief Set text
         * @return Reference to self (for method chaining)
         *
         * Expects that the label doesn't have a data binding, as in that case
         * the label updates are driven from outside. If the label had an icon
         * before, it's replaced with a text. Passing an empty @p text makes
         * the label empty.
         * @see @ref hasDataBinding(), @ref setIcon(), @ref setStyle()
         */
        #ifdef DOXYGEN_GENERATING_OUTPUT
        Label& setText(Containers::StringView text, const TextProperties& textProperties = {});
        #else
        /* To avoid having to include TextProperties.h */
        Label& setText(Containers::StringView text, const TextProperties& textProperties);
        Label& setText(Containers::StringView text);
        #endif

        /**
         * @brief Icon / text data or @ref DataHandle::Null
         *
         * Exposed mainly for testing purposes, not meant to be modified
         * directly.
         */
        DataHandle data() const;

        /**
         * @brief Layout data
         *
         * Exposed mainly for testing purposes, not meant to be modified
         * directly.
         */
        DataHandle layoutData() const;

        /**
         * @brief Data binding data or @ref DataHandle::Null
         *
         * Exposed mainly for testing purposes, not meant to be modified
         * directly.
         */
        DataHandle dataBindingData() const { return _dataBindingData; }

        #ifndef DOXYGEN_GENERATING_OUTPUT
        _MAGNUM_UI_WIDGET_SUBCLASS_IMPLEMENTATION(Label) /* LCOV_EXCL_LINE */
        #endif

    private:
        /* The nullptr is just to avoid accidental conflicts with public
           constructors */
        template<class T, class Formatter> MAGNUM_UI_LOCAL Label(std::nullptr_t, Anchor anchor, const StorageQuery<T>& query, const Formatter& formatter, LabelStyle style);
        template<class ...Args> MAGNUM_UI_LOCAL Label& setStyleInternal(LabelStyle style, Args... args);

        LabelStyle _style;
        /* 2 bytes free (_style fits into padding of Widget) */
        Icon _icon;
        LayerDataHandle _data;
        /* Unlike with other data, here the layer can be arbitrary, so storing
           a full handle */
        DataHandle _dataBindingData;
        LayerDataHandle _layoutData;
};

/**
@brief Stateless icon label widget
@param anchor           Positioning anchor
@param icon             Label icon. Passing @ref Icon::None makes the label
    empty.
@param style            Label style
@return The @p anchor verbatim
@m_since_latest_{extras}

Equivalent to constructing a non-owned @ref Label using
@ref Label::Label(NonOwnedT, Anchor, Icon, LabelStyle) and discarding the
stateful instance. See its documentation for more information.
*/
MAGNUM_UI_EXPORT Anchor label(Anchor anchor, Icon icon, LabelStyle style = LabelStyle::Default);

/**
@brief Stateless text label widget
@param anchor           Positioning anchor
@param text             Label text. Passing an empty string makes the label
    empty.
@param textProperties   Text shaping and layouting properties
@param style            Label style
@return The @p anchor verbatim
@m_since_latest_{extras}

Equivalent to constructing a non-owned @ref Label using
@ref Label::Label(NonOwnedT, Anchor, Containers::StringView, const TextProperties&, LabelStyle)
and discarding the stateful instance. See its documentation for more
information.
*/
MAGNUM_UI_EXPORT Anchor label(Anchor anchor, Containers::StringView text, const TextProperties& textProperties, LabelStyle style = LabelStyle::Default);
/**
@overload
@m_since_latest_{extras}
*/
MAGNUM_UI_EXPORT Anchor label(Anchor anchor, Containers::StringView text, LabelStyle style = LabelStyle::Default);

/**
@brief Stateless label widget with an integer data binding
@param anchor           Positioning anchor
@param query            @ref DataLayer storage query
@param style            Label style
@return The @p anchor verbatim
@m_since_latest_{extras}

Equivalent to constructing a non-owned @ref Label using
@ref Label::Label(NonOwnedT, Anchor, const StorageQuery<Int>&, LabelStyle) and
discarding the stateful instance. See its documentation for more information.
*/
inline Anchor label(Anchor anchor, const StorageQuery<Int>& query, LabelStyle style = LabelStyle::Default) {
    Label{NonOwned, anchor, query, style};
    return anchor;
}
/** @overload */
inline Anchor label(Anchor anchor, const StorageQuery<UnsignedInt>& query, LabelStyle style = LabelStyle::Default) {
    Label{NonOwned, anchor, query, style};
    return anchor;
}
/** @overload */
inline Anchor label(Anchor anchor, const StorageQuery<Long>& query, LabelStyle style = LabelStyle::Default) {
    Label{NonOwned, anchor, query, style};
    return anchor;
}
/** @overload */
inline Anchor label(Anchor anchor, const StorageQuery<UnsignedLong>& query, LabelStyle style = LabelStyle::Default) {
    Label{NonOwned, anchor, query, style};
    return anchor;
}

/**
@brief Stateless label widget with an integer data binding and a custom decimal formatter
@param anchor           Positioning anchor
@param query            @ref DataLayer storage query
@param formatter        Formatter instance
@param style            Label style
@return The @p anchor verbatim
@m_since_latest_{extras}

Equivalent to constructing a non-owned @ref Label using
@ref Label::Label(NonOwnedT, Anchor, const StorageQuery<Int>&, const DecimalFormatter&, LabelStyle)
and discarding the stateful instance. See its documentation for more
information.
*/
inline Anchor label(Anchor anchor, const StorageQuery<Int>& query, const DecimalFormatter& formatter, LabelStyle style = LabelStyle::Default) {
    Label{NonOwned, anchor, query, formatter, style};
    return anchor;
}
/** @overload */
inline Anchor label(Anchor anchor, const StorageQuery<UnsignedInt>& query, const DecimalFormatter& formatter, LabelStyle style = LabelStyle::Default) {
    Label{NonOwned, anchor, query, formatter, style};
    return anchor;
}
/** @overload */
inline Anchor label(Anchor anchor, const StorageQuery<Long>& query, const DecimalFormatter& formatter, LabelStyle style = LabelStyle::Default) {
    Label{NonOwned, anchor, query, formatter, style};
    return anchor;
}
/** @overload */
inline Anchor label(Anchor anchor, const StorageQuery<UnsignedLong>& query, const DecimalFormatter& formatter, LabelStyle style = LabelStyle::Default) {
    Label{NonOwned, anchor, query, formatter, style};
    return anchor;
}

/**
@brief Stateless label widget with an integer data binding and a custom hexadecimal formatter
@param anchor           Positioning anchor
@param query            @ref DataLayer storage query
@param formatter        Formatter instance
@param style            Label style
@return The @p anchor verbatim
@m_since_latest_{extras}

Equivalent to constructing a non-owned @ref Label using
@ref Label::Label(NonOwnedT, Anchor, const StorageQuery<Int>&, const HexadecimalFormatter&, LabelStyle)
and discarding the stateful instance. See its documentation for more
information.
*/
inline Anchor label(Anchor anchor, const StorageQuery<Int>& query, const HexadecimalFormatter& formatter, LabelStyle style = LabelStyle::Default) {
    Label{NonOwned, anchor, query, formatter, style};
    return anchor;
}
/** @overload */
inline Anchor label(Anchor anchor, const StorageQuery<UnsignedInt>& query, const HexadecimalFormatter& formatter, LabelStyle style = LabelStyle::Default) {
    Label{NonOwned, anchor, query, formatter, style};
    return anchor;
}
/** @overload */
inline Anchor label(Anchor anchor, const StorageQuery<Long>& query, const HexadecimalFormatter& formatter, LabelStyle style = LabelStyle::Default) {
    Label{NonOwned, anchor, query, formatter, style};
    return anchor;
}
/** @overload */
inline Anchor label(Anchor anchor, const StorageQuery<UnsignedLong>& query, const HexadecimalFormatter& formatter, LabelStyle style = LabelStyle::Default) {
    Label{NonOwned, anchor, query, formatter, style};
    return anchor;
}

/**
@brief Stateless label widget with a floating-point data binding
@param anchor           Positioning anchor
@param query            @ref DataLayer storage query
@param style            Label style
@return The @p anchor verbatim
@m_since_latest_{extras}

Equivalent to constructing a non-owned @ref Label using
@ref Label::Label(NonOwnedT, Anchor, const StorageQuery<Float>&, LabelStyle)
and discarding the stateful instance. See its documentation for more
information.
*/
inline Anchor label(Anchor anchor, const StorageQuery<Float>& query, LabelStyle style = LabelStyle::Default) {
    Label{NonOwned, anchor, query, style};
    return anchor;
}
/** @overload */
inline Anchor label(Anchor anchor, const StorageQuery<Double>& query, LabelStyle style = LabelStyle::Default) {
    Label{NonOwned, anchor, query, style};
    return anchor;
}

/**
@brief Stateless label widget with a floating-point data binding and a custom formatter
@param anchor           Positioning anchor
@param query            @ref DataLayer storage query
@param formatter        Formatter instanc
@param style            Label style
@return The @p anchor verbatim
@m_since_latest_{extras}

Equivalent to constructing a non-owned @ref Label using
@ref Label::Label(NonOwnedT, Anchor, const StorageQuery<Float>&, const FloatFormatter&, LabelStyle)
and discarding the stateful instance. See its documentation for more
information.
*/
inline Anchor label(Anchor anchor, const StorageQuery<Float>& query, const FloatFormatter& formatter, LabelStyle style = LabelStyle::Default) {
    Label{NonOwned, anchor, query, formatter, style};
    return anchor;
}
/** @overload */
inline Anchor label(Anchor anchor, const StorageQuery<Double>& query, const FloatFormatter& formatter, LabelStyle style = LabelStyle::Default) {
    Label{NonOwned, anchor, query, formatter, style};
    return anchor;
}

/**
@brief Stateless label widget with a string data binding
@param anchor           Positioning anchor
@param query            @ref DataLayer storage query
@param style            Label style
@return The @p anchor verbatim
@m_since_latest_{extras}

Equivalent to constructing a non-owned @ref Label using
@ref Label::Label(NonOwnedT, Anchor, const StorageQuery<Containers::StringView>&, LabelStyle)
and discarding the stateful instance. See its documentation for more
information.
*/
inline Anchor label(Anchor anchor, const StorageQuery<Containers::StringView>& query, LabelStyle style = LabelStyle::Default) {
    Label{NonOwned, anchor, query, style};
    return anchor;
}

/**
@brief Stateless label widget with an icon data binding
@param anchor           Positioning anchor
@param query            @ref DataLayer storage query
@param style            Label style
@return The @p anchor verbatim
@m_since_latest_{extras}

Equivalent to constructing a non-owned @ref Label using
@ref Label::Label(NonOwnedT, Anchor, const StorageQuery<Icon>&, LabelStyle) and
discarding the stateful instance. See its documentation for more information.
*/
inline Anchor label(Anchor anchor, const StorageQuery<Icon>& query, LabelStyle style = LabelStyle::Default) {
    Label{NonOwned, anchor, query, style};
    return anchor;
}

}}

#endif
