#ifndef Magnum_Ui_Input_h
#define Magnum_Ui_Input_h
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
 * @brief Class @ref Magnum::Ui::Input, @ref Magnum::Ui::PasswordInput, enum @ref Magnum::Ui::InputStyle
 * @m_since_latest_{extras}
 */

#include "Magnum/Ui/Anchor.h"
#include "Magnum/Ui/Widget.h"

namespace Magnum { namespace Ui {

/**
@brief Input style
@m_since_latest_{extras}

@see @ref Input
*/
enum class InputStyle: UnsignedByte {
    Default,        /**< Default */
    Success,        /**< Success */
    Warning,        /**< Warning */
    Danger,         /**< Danger */
    Flat,           /**< Flat */
};

/**
@debugoperatorenum{InputStyle}
@m_since_latest_{extras}
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, InputStyle value);

/**
@brief Input widget
@m_since_latest_{extras}

@see @ref PasswordInput
*/
class MAGNUM_UI_EXPORT Input: public Widget {
    public:
        /**
         * @brief Constructor
         * @param anchor            Positioning anchor
         * @param text              Pre-filled input text
         * @param textProperties    Text shaping and layouting properties
         * @param style             Input style
         */
        explicit Input(Anchor anchor, Containers::StringView text, const TextProperties& textProperties, InputStyle style = InputStyle::Default);
        /** @overload */
        explicit Input(Anchor anchor, Containers::StringView text, InputStyle style = InputStyle::Default);
        /** @overload */
        explicit Input(Anchor anchor, InputStyle style = InputStyle::Default);

        /**
         * @brief Construct an input with an integer data binding
         * @param anchor            Positioning anchor
         * @param query             @ref DataLayer storage query
         * @param style             Input style
         *
         * Formats the value with a default-constructed @ref DecimalFormatter
         * and uses a @ref DecimalFormatter::parse() overload matching the type
         * for parsing the value. Expects that @ref StorageQuery::operations()
         * list at least @ref StorageOperation::Set.
         *
         * Use the @ref Input() "Input(Anchor, const StorageQuery<Int>&, const DecimalFormatter&, ParseState(*)(Containers::StringView, Int&), InputStyle)"
         * or @ref Input() "Input(Anchor, const StorageQuery<Int>&, const HexadecimalFormatter&, ParseState(*)(Containers::StringView, Int&), InputStyle)"
         * constructors and their overloads to explicitly supply a configured
         * @ref DecimalFormatter or @ref HexadecimalFormatter instance and a
         * matching parser.
         *
         * Note that it's currently not possible to supply custom
         * @ref TextProperties when using a data binding.
         * @todoc fix the constructor links once Doxygen is capable of that
         */
        explicit Input(Anchor anchor, const StorageQuery<Int>& query, InputStyle style = InputStyle::Default);
        /** @overload */
        explicit Input(Anchor anchor, const StorageQuery<UnsignedInt>& query, InputStyle style = InputStyle::Default);
        /** @overload */
        explicit Input(Anchor anchor, const StorageQuery<Long>& query, InputStyle style = InputStyle::Default);
        /** @overload */
        explicit Input(Anchor anchor, const StorageQuery<UnsignedLong>& query, InputStyle style = InputStyle::Default);

        /**
         * @brief Construct an input with an integer data binding and a custom decimal formatter and parser
         * @param anchor            Positioning anchor
         * @param query             @ref DataLayer storage query
         * @param formatter         Formatter instance
         * @param parser            Parser function
         * @param style             Input style
         *
         * Expects that @ref StorageQuery::operations() list at least
         * @ref StorageOperation::Set. Passing a @cpp nullptr @ce @p parser is
         * equivalent to using a @ref DecimalFormatter::parse() overload
         * matching the query type.
         *
         * Use the @ref Input(Anchor, const StorageQuery<Int>&, const HexadecimalFormatter&, InputStyle)
         * constructor and its overloads to format and parse the value as
         * hexadecimal instead. Passing a default-constructed
         * @ref DecimalFormatter instance and @cpp nullptr @ce @p parser is
         * equivalent to using the @ref Input(Anchor, const StorageQuery<Int>&, InputStyle)
         * constructor.
         *
         * Note that it's currently not possible to supply custom
         * @ref TextProperties when using a data binding.
         * @todoc fix the constructor link once Doxygen is capable of that
         */
        explicit Input(Anchor anchor, const StorageQuery<Int>& query, const DecimalFormatter& formatter, ParseState(*parser)(Containers::StringView, Int&) = nullptr, InputStyle style = InputStyle::Default);
        /** @overload */
        explicit Input(Anchor anchor, const StorageQuery<UnsignedInt>& query, const DecimalFormatter& formatter, ParseState(*parser)(Containers::StringView, UnsignedInt&) = nullptr, InputStyle style = InputStyle::Default);
        /** @overload */
        explicit Input(Anchor anchor, const StorageQuery<Long>& query, const DecimalFormatter& formatter, ParseState(*parser)(Containers::StringView, Long&) = nullptr, InputStyle style = InputStyle::Default);
        /** @overload */
        explicit Input(Anchor anchor, const StorageQuery<UnsignedLong>& query, const DecimalFormatter& formatter, ParseState(*parser)(Containers::StringView, UnsignedLong&) = nullptr, InputStyle style = InputStyle::Default);
        /** @overload */
        explicit Input(Anchor anchor, const StorageQuery<Int>& query, const DecimalFormatter& formatter, InputStyle style): Input{anchor, query, formatter, nullptr, style} {}
        /** @overload */
        explicit Input(Anchor anchor, const StorageQuery<UnsignedInt>& query, const DecimalFormatter& formatter, InputStyle style): Input{anchor, query, formatter, nullptr, style} {}
        /** @overload */
        explicit Input(Anchor anchor, const StorageQuery<Long>& query, const DecimalFormatter& formatter, InputStyle style): Input{anchor, query, formatter, nullptr, style} {}
        /** @overload */
        explicit Input(Anchor anchor, const StorageQuery<UnsignedLong>& query, const DecimalFormatter& formatter, InputStyle style): Input{anchor, query, formatter, nullptr, style} {}

        /**
         * @brief Construct an input with an integer data binding and a custom hexadecimal formatter and parser
         * @param anchor            Positioning anchor
         * @param query             @ref DataLayer storage query
         * @param formatter         Formatter instance
         * @param parser            Parser function
         * @param style             Input style
         *
         * Expects that @ref StorageQuery::operations() list at least
         * @ref StorageOperation::Set. Passing a @cpp nullptr @ce @p parser is
         * equivalent to using a @ref HexadecimalFormatter::parse() overload
         * matching the query type.
         *
         * Use the @ref Input() "Input(Anchor, const StorageQuery<Int>&, const DecimalFormatter&, ParseState(*)(Containers::StringView, Int&), InputStyle)"
         * constructor and its overloads to format and parse the value as
         * decimal instead, or @ref Input(Anchor, const StorageQuery<Int>&, InputStyle)
         * to use a default-constructed @ref DecimalFormatter instance and a
         * @ref DecimalFormatter::parse() overload matching the type for
         * parsing the value.
         *
         * Note that it's currently not possible to supply custom
         * @ref TextProperties when using a data binding.
         * @todoc fix the constructor link once Doxygen is capable of that
         */
        explicit Input(Anchor anchor, const StorageQuery<Int>& query, const HexadecimalFormatter& formatter, ParseState(*parser)(Containers::StringView, Int&) = nullptr, InputStyle style = InputStyle::Default);
        /** @overload */
        explicit Input(Anchor anchor, const StorageQuery<UnsignedInt>& query, const HexadecimalFormatter& formatter, ParseState(*parser)(Containers::StringView, UnsignedInt&) = nullptr, InputStyle style = InputStyle::Default);
        /** @overload */
        explicit Input(Anchor anchor, const StorageQuery<Long>& query, const HexadecimalFormatter& formatter, ParseState(*parser)(Containers::StringView, Long&) = nullptr, InputStyle style = InputStyle::Default);
        /** @overload */
        explicit Input(Anchor anchor, const StorageQuery<UnsignedLong>& query, const HexadecimalFormatter& formatter, ParseState(*parser)(Containers::StringView, UnsignedLong&) = nullptr, InputStyle style = InputStyle::Default);
        /** @overload */
        explicit Input(Anchor anchor, const StorageQuery<Int>& query, const HexadecimalFormatter& formatter, InputStyle style): Input{anchor, query, formatter, nullptr, style} {}
        /** @overload */
        explicit Input(Anchor anchor, const StorageQuery<UnsignedInt>& query, const HexadecimalFormatter& formatter, InputStyle style): Input{anchor, query, formatter, nullptr, style} {}
        /** @overload */
        explicit Input(Anchor anchor, const StorageQuery<Long>& query, const HexadecimalFormatter& formatter, InputStyle style): Input{anchor, query, formatter, nullptr, style} {}
        /** @overload */
        explicit Input(Anchor anchor, const StorageQuery<UnsignedLong>& query, const HexadecimalFormatter& formatter, InputStyle style): Input{anchor, query, formatter, nullptr, style} {}

        /**
         * @brief Construct an input with a float data binding
         * @param anchor            Positioning anchor
         * @param query             @ref DataLayer storage query
         * @param style             Input style
         *
         * Formats the value with a default-constructed @ref FloatFormatter and
         * uses a @ref FloatFormatter::parse() overload matching the type for
         * parsing the value. Expects that @ref StorageQuery::operations() list
         * at least @ref StorageOperation::Set.
         *
         * Use the @ref Input() "Input(Anchor, const StorageQuery<Float>&, ParseState(*)(Containers::StringView, Float&), const FloatFormatter&, InputStyle)"
         * constructor and its overloads to explicitly supply a configured
         * @ref FloatFormatter instance and a matching parser.
         *
         * Note that it's currently not possible to supply custom
         * @ref TextProperties when using a data binding.
         * @todoc fix the constructor link once Doxygen is capable of that
         */
        explicit Input(Anchor anchor, const StorageQuery<Float>& query, InputStyle style = InputStyle::Default);
        /** @overload */
        explicit Input(Anchor anchor, const StorageQuery<Double>& query, InputStyle style = InputStyle::Default);

        /**
         * @brief Construct an input with a float data binding and a custom formatter and parser
         * @param anchor            Positioning anchor
         * @param query             @ref DataLayer storage query
         * @param formatter         Formatter instance
         * @param parser            Parser function
         * @param style             Input style
         *
         * Expects that @ref StorageQuery::operations() list at least
         * @ref StorageOperation::Set. Passing a @cpp nullptr @ce @p parser is
         * equivalent to using a @ref FloatFormatter::parse() overload matching
         * the query type.
         *
         * Passing a default-constructed @ref FloatFormatter instance and
         * @cpp nullptr @ce @p parser is equivalent to using the
         * @ref Input(Anchor, const StorageQuery<Float>&, InputStyle)
         * constructor overloads.
         *
         * Note that it's currently not possible to supply custom
         * @ref TextProperties when using a data binding.
         * @todoc fix the constructor link once Doxygen is capable of that
         */
        explicit Input(Anchor anchor, const StorageQuery<Float>& query, const FloatFormatter& formatter, ParseState(*parser)(Containers::StringView, Float&) = nullptr, InputStyle style = InputStyle::Default);
        /** @overload */
        explicit Input(Anchor anchor, const StorageQuery<Double>& query, const FloatFormatter& formatter, ParseState(*parser)(Containers::StringView, Double&) = nullptr, InputStyle style = InputStyle::Default);
        /** @overload */
        explicit Input(Anchor anchor, const StorageQuery<Float>& query, const FloatFormatter& formatter, InputStyle style): Input{anchor, query, formatter, nullptr, style} {}
        /** @overload */
        explicit Input(Anchor anchor, const StorageQuery<Double>& query, const FloatFormatter& formatter, InputStyle style): Input{anchor, query, formatter, nullptr, style} {}

        /**
         * @brief Construct a non-owned input with an integer data binding
         *
         * Like @ref Input(Anchor, const StorageQuery<Int>&, InputStyle) but
         * the widget node doesn't get removed on destruction. Instead, it gets
         * removed either once any parent node is removed, or when
         * @ref AbstractUserInterface::removeNode() is explicitly called on
         * @ref node().
         * @see @ref isOwned(), @ref input(Anchor, const StorageQuery<Int>&, InputStyle)
         */
        explicit Input(NonOwnedT, Anchor anchor, const StorageQuery<Int>& query, InputStyle style = InputStyle::Default): Input{anchor, query, style} {
            makeNonOwned();
        }
        /** @overload */
        explicit Input(NonOwnedT, Anchor anchor, const StorageQuery<UnsignedInt>& query, InputStyle style = InputStyle::Default): Input{anchor, query, style} {
            makeNonOwned();
        }
        /** @overload */
        explicit Input(NonOwnedT, Anchor anchor, const StorageQuery<Long>& query, InputStyle style = InputStyle::Default): Input{anchor, query, style} {
            makeNonOwned();
        }
        /** @overload */
        explicit Input(NonOwnedT, Anchor anchor, const StorageQuery<UnsignedLong>& query, InputStyle style = InputStyle::Default): Input{anchor, query, style} {
            makeNonOwned();
        }

        /**
         * @brief Construct a non-owned input with an integer data binding and a custom decimal formatter and parser
         *
         * Like @ref Input() "Input(Anchor, const StorageQuery<Int>&, const DecimalFormatter&, ParseState(*)(Containers::StringView, Int&), InputStyle)"
         * but the widget node doesn't get removed on destruction. Instead, it
         * gets removed either once any parent node is removed, or when
         * @ref AbstractUserInterface::removeNode() is explicitly called on
         * @ref node().
         * @see @ref isOwned(), @ref input() "input(Anchor, const StorageQuery<Int>&, const DecimalFormatter&, ParseState(*)(Containers::StringView, Int&), InputStyle)"
         * @todoc fix the constructor and function links once Doxygen is
         *      capable of that
         */
        explicit Input(NonOwnedT, Anchor anchor, const StorageQuery<Int>& query, const DecimalFormatter& formatter, ParseState(*parser)(Containers::StringView, Int&) = nullptr, InputStyle style = InputStyle::Default): Input{anchor, query, formatter, parser, style} {
            makeNonOwned();
        }
        /** @overload */
        explicit Input(NonOwnedT, Anchor anchor, const StorageQuery<UnsignedInt>& query, const DecimalFormatter& formatter, ParseState(*parser)(Containers::StringView, UnsignedInt&) = nullptr, InputStyle style = InputStyle::Default): Input{anchor, query, formatter, parser, style} {
            makeNonOwned();
        }
        /** @overload */
        explicit Input(NonOwnedT, Anchor anchor, const StorageQuery<Long>& query, const DecimalFormatter& formatter, ParseState(*parser)(Containers::StringView, Long&) = nullptr, InputStyle style = InputStyle::Default): Input{anchor, query, formatter, parser, style} {
            makeNonOwned();
        }
        /** @overload */
        explicit Input(NonOwnedT, Anchor anchor, const StorageQuery<UnsignedLong>& query, const DecimalFormatter& formatter, ParseState(*parser)(Containers::StringView, UnsignedLong&) = nullptr, InputStyle style = InputStyle::Default): Input{anchor, query, formatter, parser, style} {
            makeNonOwned();
        }
        /** @overload */
        explicit Input(NonOwnedT, Anchor anchor, const StorageQuery<Int>& query, const DecimalFormatter& formatter, InputStyle style): Input{anchor, query, formatter, style} {
            makeNonOwned();
        }
        /** @overload */
        explicit Input(NonOwnedT, Anchor anchor, const StorageQuery<UnsignedInt>& query, const DecimalFormatter& formatter, InputStyle style): Input{anchor, query, formatter, style} {
            makeNonOwned();
        }
        /** @overload */
        explicit Input(NonOwnedT, Anchor anchor, const StorageQuery<Long>& query, const DecimalFormatter& formatter, InputStyle style): Input{anchor, query, formatter, style} {
            makeNonOwned();
        }
        /** @overload */
        explicit Input(NonOwnedT, Anchor anchor, const StorageQuery<UnsignedLong>& query, const DecimalFormatter& formatter, InputStyle style): Input{anchor, query, formatter, style} {
            makeNonOwned();
        }

        /**
         * @brief Construct a non-owned input with an integer data binding and a custom hexadecimal formatter and parser
         *
         * Like @ref Input() "Input(Anchor, const StorageQuery<Int>&, const HexadecimalFormatter&, ParseState(*)(Containers::StringView, Int&), InputStyle)"
         * but the widget node doesn't get removed on destruction. Instead, it
         * gets removed either once any parent node is removed, or when
         * @ref AbstractUserInterface::removeNode() is explicitly called on
         * @ref node().
         * @see @ref isOwned(), @ref input() "input(Anchor, const StorageQuery<Int>&, const HexadecimalFormatter&, ParseState(*)(Containers::StringView, Int&), InputStyle)"
         * @todoc fix the constructor and function links once Doxygen is
         *      capable of that
         */
        explicit Input(NonOwnedT, Anchor anchor, const StorageQuery<Int>& query, const HexadecimalFormatter& formatter, ParseState(*parser)(Containers::StringView, Int&) = nullptr, InputStyle style = InputStyle::Default): Input{anchor, query, formatter, parser, style} {
            makeNonOwned();
        }
        /** @overload */
        explicit Input(NonOwnedT, Anchor anchor, const StorageQuery<UnsignedInt>& query, const HexadecimalFormatter& formatter, ParseState(*parser)(Containers::StringView, UnsignedInt&) = nullptr, InputStyle style = InputStyle::Default): Input{anchor, query, formatter, parser, style} {
            makeNonOwned();
        }
        /** @overload */
        explicit Input(NonOwnedT, Anchor anchor, const StorageQuery<Long>& query, const HexadecimalFormatter& formatter, ParseState(*parser)(Containers::StringView, Long&) = nullptr, InputStyle style = InputStyle::Default): Input{anchor, query, formatter, parser, style} {
            makeNonOwned();
        }
        /** @overload */
        explicit Input(NonOwnedT, Anchor anchor, const StorageQuery<UnsignedLong>& query, const HexadecimalFormatter& formatter, ParseState(*parser)(Containers::StringView, UnsignedLong&) = nullptr, InputStyle style = InputStyle::Default): Input{anchor, query, formatter, parser, style} {
            makeNonOwned();
        }
        /** @overload */
        explicit Input(NonOwnedT, Anchor anchor, const StorageQuery<Int>& query, const HexadecimalFormatter& formatter, InputStyle style): Input{anchor, query, formatter, style} {
            makeNonOwned();
        }
        /** @overload */
        explicit Input(NonOwnedT, Anchor anchor, const StorageQuery<UnsignedInt>& query, const HexadecimalFormatter& formatter, InputStyle style): Input{anchor, query, formatter, style} {
            makeNonOwned();
        }
        /** @overload */
        explicit Input(NonOwnedT, Anchor anchor, const StorageQuery<Long>& query, const HexadecimalFormatter& formatter, InputStyle style): Input{anchor, query, formatter, style} {
            makeNonOwned();
        }
        /** @overload */
        explicit Input(NonOwnedT, Anchor anchor, const StorageQuery<UnsignedLong>& query, const HexadecimalFormatter& formatter, InputStyle style): Input{anchor, query, formatter, style} {
            makeNonOwned();
        }

        /**
         * @brief Construct a non-owned input with a float data binding
         *
         * Like @ref Input(Anchor, const StorageQuery<Float>&, InputStyle) but
         * the widget node doesn't get removed on destruction. Instead, it gets
         * removed either once any parent node is removed, or when
         * @ref AbstractUserInterface::removeNode() is explicitly called on
         * @ref node().
         * @see @ref isOwned(), @ref input(Anchor, const StorageQuery<Float>&, InputStyle)
         */
        explicit Input(NonOwnedT, Anchor anchor, const StorageQuery<Float>& query, InputStyle style = InputStyle::Default): Input{anchor, query, style} {
            makeNonOwned();
        }
        /** @overload */
        explicit Input(NonOwnedT, Anchor anchor, const StorageQuery<Double>& query, InputStyle style = InputStyle::Default): Input{anchor, query, style} {
            makeNonOwned();
        }

        /**
         * @brief Construct a non-owned input with a float data binding and a custom formatter and parser
         *
         * Like @ref Input() "Input(Anchor, const StorageQuery<Float>&, const FloatFormatter&, ParseState(*)(Containers::StringView, Float&), InputStyle)"
         * but the widget node doesn't get removed on destruction. Instead, it
         * gets removed either once any parent node is removed, or when
         * @ref AbstractUserInterface::removeNode() is explicitly called on
         * @ref node().
         * @see @ref isOwned(), @ref input() "input(Anchor, const StorageQuery<Float>&, const DecimalFormatter&, ParseState(*)(Containers::StringView, Float&), InputStyle)"
         * @todoc fix the constructor and function links once Doxygen is
         *      capable of that
         */
        explicit Input(NonOwnedT, Anchor anchor, const StorageQuery<Float>& query, const FloatFormatter& formatter, ParseState(*parser)(Containers::StringView, Float&) = nullptr, InputStyle style = InputStyle::Default): Input{anchor, query, formatter, parser, style} {
            makeNonOwned();
        }
        /** @overload */
        explicit Input(NonOwnedT, Anchor anchor, const StorageQuery<Double>& query, const FloatFormatter& formatter, ParseState(*parser)(Containers::StringView, Double&) = nullptr, InputStyle style = InputStyle::Default): Input{anchor, query, formatter, parser, style} {
            makeNonOwned();
        }
        /** @overload */
        explicit Input(NonOwnedT, Anchor anchor, const StorageQuery<Float>& query, const FloatFormatter& formatter, InputStyle style): Input{anchor, query, formatter, style} {
            makeNonOwned();
        }
        /** @overload */
        explicit Input(NonOwnedT, Anchor anchor, const StorageQuery<Double>& query, const FloatFormatter& formatter, InputStyle style): Input{anchor, query, formatter, style} {
            makeNonOwned();
        }

        /** @copydoc AbstractWidget::AbstractWidget(NoCreateT) */
        explicit Input(NoCreateT): Widget{NoCreate}, _style{}, _backgroundData{}, _textData{}, _dataBindingData{} {}

        /**
         * @brief Whether the input has a data binding
         *
         * Returns @cpp true @ce if the input was constructed using a
         * @ref StorageQuery, @cpp false @ce otherwise.
         */
        bool hasDataBinding() const { return _dataBindingData != DataHandle{}; }

        /** @brief Style */
        InputStyle style() const { return _style; }

        /**
         * @brief Set style
         * @return Reference to self (for method chaining)
         *
         * Expects that the input doesn't have a data binding, as in that case
         * the input style changes are driven from outside. Note that calling
         * this function doesn't change the font if the new style uses a
         * different one, you have to call @ref setText() afterwards to make it
         * pick it up.
         * @see @ref hasDataBinding(), @ref setStyle(InputStyle, Nanoseconds),
         *      @ref setText()
         */
        Input& setStyle(InputStyle style);

        /**
         * @brief Set style at given time
         * @return Reference to self (for method chaining)
         *
         * Compared to @ref setStyle(InputStyle) may animate the style
         * transition if the theme defines an animation for it. The
         * @ref style() getter is however updated immediately always.
         * Additionally, if the function is called while the @ref Input is
         * focused, ensures that a blinking cursor animation is playing for the
         * new style as well.
         */
        Input& setStyle(InputStyle style, Nanoseconds time);

        /**
         * @brief Text
         *
         * The returned view is valid only until any text is created or updated
         * on the user interface text layer.
         */
        Containers::StringView text() const;

        /**
         * @brief Set text
         * @return Reference to self (for method chaining)
         *
         * Expects that the input doesn't have a data binding, as in that case
         * the input updates are driven from outside.
         * @see @ref hasDataBinding(), @ref setStyle()
         */
        #ifdef DOXYGEN_GENERATING_OUTPUT
        Input& setText(Containers::StringView text, const TextProperties& textProperties = {});
        #else
        /* To avoid having to include TextProperties.h */
        Input& setText(Containers::StringView text, const TextProperties& textProperties);
        Input& setText(Containers::StringView text);
        #endif

        /**
         * @brief Background data
         *
         * Exposed mainly for testing purposes, not meant to be modified
         * directly.
         */
        DataHandle backgroundData() const;

        /**
         * @brief Text data or @ref DataHandle::Null
         *
         * Exposed mainly for testing purposes, not meant to be modified
         * directly.
         */
        DataHandle textData() const;

        /**
         * @brief Data binding data or @ref DataHandle::Null
         *
         * Exposed mainly for testing purposes, not meant to be modified
         * directly.
         */
        DataHandle dataBindingData() const { return _dataBindingData; }

        #ifndef DOXYGEN_GENERATING_OUTPUT
        _MAGNUM_UI_WIDGET_SUBCLASS_IMPLEMENTATION(Input) /* LCOV_EXCL_LINE */
        #endif

    #ifdef DOXYGEN_GENERATING_OUTPUT
    private:
    #else
    protected:
    #endif
        explicit MAGNUM_UI_LOCAL Input(Anchor anchor, Containers::StringView text, const TextProperties& textProperties, InputStyle style, Implementation::TextStyle(*textStyle)(InputStyle));
        /* The nullptr is just to avoid accidental conflicts with public
           constructors */
        explicit MAGNUM_UI_LOCAL Input(std::nullptr_t, Anchor anchor, InputStyle style);
        template<class T, class Formatter> explicit MAGNUM_UI_LOCAL Input(std::nullptr_t, Anchor anchor, const StorageQuery<T>& query, const Formatter& formatter, ParseState(*parser)(Containers::StringView, T&), InputStyle style);
        /* Delegated to from the above templated constructor to reduce
           generated code duplication */
        template<class T> MAGNUM_UI_LOCAL void createInternal(ParseState(*parser)(Containers::StringView, T&));
        MAGNUM_UI_LOCAL void createInternal();
        template<class ...Args> MAGNUM_UI_LOCAL Input& setStyleInternal(InputStyle style, Implementation::TextStyle(*textStyle)(InputStyle), Args... args);

    private:
        InputStyle _style;
        /* 2 bytes free (_style fits into padding of Widget) */
        LayerDataHandle _backgroundData, _textData;
        /* Unlike with other data, here the layer can be arbitrary, so storing
           a full handle */
        DataHandle _dataBindingData;
};

/**
@brief Password input widget
@m_since_latest_{extras}

Compared to @ref Input displays the entered text as a sequence of dots.
*/
class MAGNUM_UI_EXPORT PasswordInput: public Input {
    public:
        /**
         * @brief Constructor
         * @param anchor            Positioning anchor
         * @param text              Pre-filled input text
         * @param textProperties    Text shaping and layouting properties
         * @param style             Input style
         */
        explicit PasswordInput(Anchor anchor, Containers::StringView text, const TextProperties& textProperties, InputStyle style = InputStyle::Default);
        /** @overload */
        explicit PasswordInput(Anchor anchor, Containers::StringView text, InputStyle style = InputStyle::Default);
        /** @overload */
        explicit PasswordInput(Anchor anchor, InputStyle style = InputStyle::Default);

        /** @copydoc AbstractWidget::AbstractWidget(NoCreateT) */
        explicit PasswordInput(NoCreateT): Input{NoCreate} {}

        /* Mostly just overloads to remove a WTF factor from method chaining
           order */
        #ifndef DOXYGEN_GENERATING_OUTPUT
        PasswordInput& setStyle(InputStyle style);
        PasswordInput& setStyle(InputStyle style, Nanoseconds time);
        PasswordInput& setText(Containers::StringView text, const TextProperties& textProperties);
        PasswordInput& setText(Containers::StringView text);
        #endif
};

/**
@brief Stateless input widget with an integer data binding
@param anchor           Positioning anchor
@param query            @ref DataLayer storage query
@param style            Input style
@return The @p anchor verbatim
@m_since_latest_{extras}

Equivalent to constructing a non-owned @ref Input using
@ref Input::Input(NonOwnedT, Anchor, const StorageQuery<Int>&, InputStyle) and
discarding the stateful instance. See its documentation for more information.
*/
inline Anchor input(Anchor anchor, const StorageQuery<Int>& query, InputStyle style = InputStyle::Default) {
    Input{NonOwned, anchor, query, style};
    return anchor;
}
/** @overload */
inline Anchor input(Anchor anchor, const StorageQuery<UnsignedInt>& query, InputStyle style = InputStyle::Default) {
    Input{NonOwned, anchor, query, style};
    return anchor;
}
/** @overload */
inline Anchor input(Anchor anchor, const StorageQuery<Long>& query, InputStyle style = InputStyle::Default) {
    Input{NonOwned, anchor, query, style};
    return anchor;
}
/** @overload */
inline Anchor input(Anchor anchor, const StorageQuery<UnsignedLong>& query, InputStyle style = InputStyle::Default) {
    Input{NonOwned, anchor, query, style};
    return anchor;
}

/**
@brief Stateless input widget with an integer data binding and a custom decimal formatter and parser
@param anchor           Positioning anchor
@param query            @ref DataLayer storage query
@param formatter        Formatter instance
@param parser           Parser function
@param style            Input style
@return The @p anchor verbatim
@m_since_latest_{extras}

Equivalent to constructing a non-owned @ref Input using
@ref Input::Input() "Input::Input(NonOwnedT, Anchor, const StorageQuery<Int>&, const DecimalFormatter&, ParseState(*)(Containers::StringView, Int&), InputStyle)"
and discarding the stateful instance. See its documentation for more
information.
@todoc fix the constructor link once Doxygen is capable of that
*/
inline Anchor input(Anchor anchor, const StorageQuery<Int>& query, const DecimalFormatter& formatter, ParseState(*parser)(Containers::StringView, Int&) = nullptr, InputStyle style = InputStyle::Default) {
    Input{NonOwned, anchor, query, formatter, parser, style};
    return anchor;
}
/** @overload */
inline Anchor input(Anchor anchor, const StorageQuery<UnsignedInt>& query, const DecimalFormatter& formatter, ParseState(*parser)(Containers::StringView, UnsignedInt&) = nullptr, InputStyle style = InputStyle::Default) {
    Input{NonOwned, anchor, query, formatter, parser, style};
    return anchor;
}
/** @overload */
inline Anchor input(Anchor anchor, const StorageQuery<Long>& query, const DecimalFormatter& formatter, ParseState(*parser)(Containers::StringView, Long&) = nullptr, InputStyle style = InputStyle::Default) {
    Input{NonOwned, anchor, query, formatter, parser, style};
    return anchor;
}
/** @overload */
inline Anchor input(Anchor anchor, const StorageQuery<UnsignedLong>& query, const DecimalFormatter& formatter, ParseState(*parser)(Containers::StringView, UnsignedLong&) = nullptr, InputStyle style = InputStyle::Default) {
    Input{NonOwned, anchor, query, formatter, parser, style};
    return anchor;
}
/** @overload */
inline Anchor input(Anchor anchor, const StorageQuery<Int>& query, const DecimalFormatter& formatter, InputStyle style) {
    Input{NonOwned, anchor, query, formatter, style};
    return anchor;
}
/** @overload */
inline Anchor input(Anchor anchor, const StorageQuery<UnsignedInt>& query, const DecimalFormatter& formatter, InputStyle style) {
    Input{NonOwned, anchor, query, formatter, style};
    return anchor;
}
/** @overload */
inline Anchor input(Anchor anchor, const StorageQuery<Long>& query, const DecimalFormatter& formatter, InputStyle style) {
    Input{NonOwned, anchor, query, formatter, style};
    return anchor;
}
/** @overload */
inline Anchor input(Anchor anchor, const StorageQuery<UnsignedLong>& query, const DecimalFormatter& formatter, InputStyle style) {
    Input{NonOwned, anchor, query, formatter, style};
    return anchor;
}

/**
@brief Stateless input widget with an integer data binding and a custom hexadecimal formatter and parser
@param anchor           Positioning anchor
@param query            @ref DataLayer storage query
@param formatter        Formatter instance
@param parser           Parser function
@param style            Input style
@return The @p anchor verbatim
@m_since_latest_{extras}

Equivalent to constructing a non-owned @ref Input using
@ref Input::Input() "Input::Input(NonOwnedT, Anchor, const StorageQuery<Int>&, const HexadecimalFormatter&, ParseState(*)(Containers::StringView, Int&), InputStyle)"
and discarding the stateful instance. See its documentation for more
information.
@todoc fix the constructor link once Doxygen is capable of that
*/
inline Anchor input(Anchor anchor, const StorageQuery<Int>& query, const HexadecimalFormatter& formatter, ParseState(*parser)(Containers::StringView, Int&) = nullptr, InputStyle style = InputStyle::Default) {
    Input{NonOwned, anchor, query, formatter, parser, style};
    return anchor;
}
/** @overload */
inline Anchor input(Anchor anchor, const StorageQuery<UnsignedInt>& query, const HexadecimalFormatter& formatter, ParseState(*parser)(Containers::StringView, UnsignedInt&) = nullptr, InputStyle style = InputStyle::Default) {
    Input{NonOwned, anchor, query, formatter, parser, style};
    return anchor;
}
/** @overload */
inline Anchor input(Anchor anchor, const StorageQuery<Long>& query, const HexadecimalFormatter& formatter, ParseState(*parser)(Containers::StringView, Long&) = nullptr, InputStyle style = InputStyle::Default) {
    Input{NonOwned, anchor, query, formatter, parser, style};
    return anchor;
}
/** @overload */
inline Anchor input(Anchor anchor, const StorageQuery<UnsignedLong>& query, const HexadecimalFormatter& formatter, ParseState(*parser)(Containers::StringView, UnsignedLong&) = nullptr, InputStyle style = InputStyle::Default) {
    Input{NonOwned, anchor, query, formatter, parser, style};
    return anchor;
}
/** @overload */
inline Anchor input(Anchor anchor, const StorageQuery<Int>& query, const HexadecimalFormatter& formatter, InputStyle style) {
    Input{NonOwned, anchor, query, formatter, style};
    return anchor;
}
/** @overload */
inline Anchor input(Anchor anchor, const StorageQuery<UnsignedInt>& query, const HexadecimalFormatter& formatter, InputStyle style) {
    Input{NonOwned, anchor, query, formatter, style};
    return anchor;
}
/** @overload */
inline Anchor input(Anchor anchor, const StorageQuery<Long>& query, const HexadecimalFormatter& formatter, InputStyle style) {
    Input{NonOwned, anchor, query, formatter, style};
    return anchor;
}
/** @overload */
inline Anchor input(Anchor anchor, const StorageQuery<UnsignedLong>& query, const HexadecimalFormatter& formatter, InputStyle style) {
    Input{NonOwned, anchor, query, formatter, style};
    return anchor;
}

/**
@brief Stateless input widget with a float data binding
@param anchor           Positioning anchor
@param query            @ref DataLayer storage query
@param style            Input style
@return The @p anchor verbatim
@m_since_latest_{extras}

Equivalent to constructing a non-owned @ref Input using
@ref Input::Input(NonOwnedT, Anchor, const StorageQuery<Float>&, InputStyle)
and the stateful instance. See its documentation for more information.
*/
inline Anchor input(Anchor anchor, const StorageQuery<Float>& query, InputStyle style = InputStyle::Default) {
    Input{NonOwned, anchor, query, style};
    return anchor;
}
/** @overload */
inline Anchor input(Anchor anchor, const StorageQuery<Double>& query, InputStyle style = InputStyle::Default) {
    Input{NonOwned, anchor, query, style};
    return anchor;
}

/**
@brief Stateless input widget with a float data binding and a custom formatter and parser
@param anchor           Positioning anchor
@param query            @ref DataLayer storage query
@param formatter        Formatter instance
@param parser           Parser function
@param style            Input style
@return The @p anchor verbatim
@m_since_latest_{extras}

Equivalent to constructing a non-owned @ref Input using
@ref Input::Input() "Input::Input(NonOwnedT, Anchor, const StorageQuery<Float>&, const FloatFormatter&, ParseState(*)(Containers::StringView, Float&), InputStyle)"
and discarding the stateful instance. See its documentation for more
information.
@todoc fix the constructor link once Doxygen is capable of that
*/
inline Anchor input(Anchor anchor, const StorageQuery<Float>& query, const FloatFormatter& formatter, ParseState(*parser)(Containers::StringView, Float&) = nullptr, InputStyle style = InputStyle::Default) {
    Input{NonOwned, anchor, query, formatter, parser, style};
    return anchor;
}
/** @overload */
inline Anchor input(Anchor anchor, const StorageQuery<Double>& query, const FloatFormatter& formatter, ParseState(*parser)(Containers::StringView, Double&) = nullptr, InputStyle style = InputStyle::Default) {
    Input{NonOwned, anchor, query, formatter, parser, style};
    return anchor;
}
/** @overload */
inline Anchor input(Anchor anchor, const StorageQuery<Float>& query, const FloatFormatter& formatter, InputStyle style) {
    Input{NonOwned, anchor, query, formatter, style};
    return anchor;
}
/** @overload */
inline Anchor input(Anchor anchor, const StorageQuery<Double>& query, const FloatFormatter& formatter, InputStyle style) {
    Input{NonOwned, anchor, query, formatter, style};
    return anchor;
}

}}

#endif
