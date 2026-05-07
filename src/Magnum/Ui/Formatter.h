#ifndef Magnum_Ui_Formatter_h
#define Magnum_Ui_Formatter_h
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
 * @brief Class @ref Magnum::Ui::DecimalFormatter, @ref Magnum::Ui::HexadecimalFormatter, @ref Magnum::Ui::FloatFormatter, enum @ref Magnum::Ui::ParseState
 * @m_since_latest_{extras}
 */

#include <Corrade/Containers/EnumSet.h>

#include "Magnum/Ui/Ui.h"
#include "Magnum/Ui/visibility.h"

namespace Magnum { namespace Ui {

/**
@brief Parse state
@m_since_latest_{extras}

@see @ref DecimalFormatter::parse(), @ref HexadecimalFormatter::parse(),
    @ref FloatFormatter::parse()
*/
/* Not called ParseResult as that's reserved for a class that wraps this enum
   together with additional information like error location */
enum class ParseState: UnsignedByte {
    /**
     * Parsing succeeded with no information loss, i.e. the value can fit into
     * the resulting type without being clamped.
     */
    Success,

    /**
     * Parsing succeeded but the parsed value had to be clamped to fit into the
     * resulting type. The output value is set to an appropriate min or max
     * representable value for given type.
     */
    Clamped,

    /**
     * Parsing the value failed, for example because it contains invalid
     * characters. The output value is left in an unspecified state in this
     * case.
     */
    Failed
};

/**
@debugoperatorenum{ParseState}
@m_since_latest_{extras}
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, ParseState value);

/**
@brief Formatter for decimal integer values
@m_since_latest_{extras}

@see @ref HexadecimalFormatter, @ref FloatFormatter
*/
class MAGNUM_UI_EXPORT DecimalFormatter {
    public:
        /**
         * @brief Flag
         *
         * @see @ref Flags, @ref DecimalFormatter(), @ref setFlags(),
         *      @ref addFlags(), @ref clearFlags()
        */
        enum class Flag: UnsignedByte {
            /** Prefix positive values with an explicit + sign */
            ExplicitPlus = 1 << 0
        };

        /**
         * @brief Flags
         * @m_since_latest_{extras}
         *
         * @see @ref DecimalFormatter(), @ref setFlags(), @ref addFlags(),
         *      @ref clearFlags()
         */
        typedef Containers::EnumSet<Flag> Flags;

        /**
         * @brief Parse text matching the formatter output
         * @param text      Text to parse
         * @param value     Where to put the parsed value
         * @return Result of the parsing operation
         *
         * Expects that @p text is
         * @relativeref{Corrade,Containers::StringViewFlag::NullTerminated}.
         * Leading and trailing whitespace is trimmed from the text and the
         * value is parsed assuming it's decimal with any amount of leading
         * zeros.
         *
         * If parsing succeeds, the function returns @ref ParseState::Success
         * and populates @p value. If the text is empty or all whitespace,
         * @ref ParseState::Failed is returned, same if the text contains a
         * negative value and the resulting type is unsigned. Otherwise, if the
         * value cannot fit into chosen resulting type, @p value is filled with
         * a corresponding min / max representable value and
         * @ref ParseState::Clamped is returned.
         * @see @relativeref{Corrade,Containers::StringView::trimmed()},
         *      @ref Math::TypeTraits::min(), @ref Math::TypeTraits::max()
         */
        static ParseState parse(Containers::StringView text, Int& value);
        /** @overload */
        static ParseState parse(Containers::StringView text, UnsignedInt& value);
        /** @overload */
        static ParseState parse(Containers::StringView text, Long& value);
        /** @overload */
        static ParseState parse(Containers::StringView text, UnsignedLong& value);

        /**
         * @brief Constructor
         * @param flags         Flags
         *
         * The formatter is only usable once it's attached to concrete text
         * data by calling @ref attach().
         */
        explicit DecimalFormatter(Flags flags = {}): _layer{}, _data{}, _flags{flags}, _minWidth{1} {}

        /**
         * @brief Construct and attach to a concrete text data
         * @param layer     Text layer instance
         * @param data      Text data handle
         * @param flags     Flags
         *
         * Equivalent to constructing with @ref DecimalFormatter(Flags) and
         * then calling @ref attach(TextLayer&, DataHandle). See their
         * documentation for more information.
         */
        explicit DecimalFormatter(TextLayer& layer, DataHandle data, Flags flags = {}): DecimalFormatter{flags} {
            attach(layer, data);
        }

        /**
         * @brief Construct and attach to a concrete text data assuming it comes from given layer
         * @param layer     Text layer instance
         * @param data      Text data handle
         * @param flags     Flags
         *
         * Equivalent to constructing with @ref DecimalFormatter(Flags) and
         * then calling @ref attach(TextLayer&, LayerDataHandle). See their
         * documentation for more information.
         */
        explicit DecimalFormatter(TextLayer& layer, LayerDataHandle data, Flags flags = {}): DecimalFormatter{flags} {
            attach(layer, data);
        }

        /**
         * @brief Text layer instance
         *
         * If @ref attach() wasn't called yet, either directly or from the
         * constructor, returns @cpp nullptr @ce.
         */
        TextLayer* layer() const { return _layer; }

        /**
         * @brief Text data handle
         *
         * If @ref attach() wasn't called yet, either directly or from the
         * constructor, returns @ref DataHandle::Null.
         */
        DataHandle data() const;

        /**
         * @brief Attach the formatter to text data
         * @return Reference to self (for method chaining)
         *
         * Expects that @p data is valid in @p layer. Upon calling this
         * function, @ref operator()() can be used. Calling this function again
         * with different arguments will reassign the formatter elsewhere. It's
         * however not possible to detach the formatter once it has been
         * attached.
         * @see @ref TextLayer::isHandleValid(DataHandle) const
         */
        DecimalFormatter& attach(TextLayer& layer, DataHandle data);

        /**
         * @brief Attach the formatter to text data assuming it comes from given layer
         * @return Reference to self (for method chaining)
         *
         * Like @ref attach(TextLayer&, DataHandle) but without checking that
         * @p handle indeed belongs to @p layer. See its documentation
         * for more information.
         * @see @ref TextLayer::isHandleValid(LayerDataHandle) const,
         *      @ref dataHandleData()
         */
        DecimalFormatter& attach(TextLayer& layer, LayerDataHandle data);

        /** @brief Flags */
        Flags flags() const { return _flags; }

        /**
         * @brief Set flags
         * @return Reference to self (for method chaining)
         *
         * @see @ref addFlags(), @ref clearFlags()
         */
        DecimalFormatter& setFlags(Flags flags) {
            _flags = flags;
            return *this;
        }

        /**
         * @brief Add flags
         * @return Reference to self (for method chaining)
         *
         * Calls @ref setFlags() with the existing flags ORed with @p flags.
         * Useful for preserving previously set flags.
         * @see @ref clearFlags()
         */
        DecimalFormatter& addFlags(Flags flags) {
            return setFlags(_flags|flags);
        }

        /**
         * @brief Clear flags
         * @return Reference to self (for method chaining)
         *
         * Calls @ref setFlags() with the existing flags ANDed with the inverse
         * of @p flags. Useful for removing a subset of previously set flags.
         * @see @ref addFlags()
         */
        DecimalFormatter& clearFlags(Flags flags) {
            return setFlags(_flags & ~flags);
        }

        /** @brief Minimal number width */
        Int minWidth() const { return _minWidth; }

        /**
         * @brief Set minimal number width
         *
         * If the formatted number has less digits than specified @p width,
         * it's padded by leading zeros. Sign isn't counted into the width.
         * Expects that @p width is at least @cpp 0 @ce and at most
         * @cpp 255 @ce. Default value is @cpp 1 @ce.
         *
         * If @p width is @cpp 0 @ce, formatting a zero value results in an
         * empty text, regardless of whether @ref Flag::ExplicitPlus was
         * specified.
         */
        DecimalFormatter& setMinWidth(Int width);

        /**
         * @brief Format a value
         *
         * Expects that @ref attach() was called and the attached-to data
         * handle is still valid. Internally formats the @p value to a
         * stack-allocated string and passes it to @ref TextLayer::setText(),
         * see that function documentation for more information.
         */
        void operator()(Int value) const;
        void operator()(UnsignedInt value) const; /**< @overload */
        void operator()(Long value) const; /**< @overload */
        void operator()(UnsignedLong value) const; /**< @overload */

    private:
        template<class T> MAGNUM_UI_LOCAL void format(T value) const;

        TextLayer* _layer;
        LayerDataHandle _data;
        Flags _flags;
        UnsignedByte _minWidth;
        /* 2 bytes free; 4/8 more bytes left to use for an in-place Function */
};

/**
@debugoperatorclassenum{DecimalFormatter,Flag}
@m_since_latest_{extras}
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, DecimalFormatter::Flag value);

/**
@debugoperatorclassenum{DecimalFormatter,Flags}
@m_since_latest_{extras}
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, DecimalFormatter::Flags value);

CORRADE_ENUMSET_OPERATORS(DecimalFormatter::Flags)

/**
@brief Formatter for hexadecimal integer values
@m_since_latest_{extras}

@see @ref DecimalFormatter, @ref FloatFormatter
*/
class MAGNUM_UI_EXPORT HexadecimalFormatter {
    public:
        /**
         * @brief Flag
         *
         * @see @ref Flags, @ref HexadecimalFormatter(), @ref setFlags(),
         *      @ref addFlags(), @ref clearFlags()
        */
        enum class Flag: UnsignedByte {
            /** Prefix positive values with an explicit + sign */
            ExplicitPlus = 1 << 0,

            /**
             * Use uppercase heaxadecimal letters and prefix if
             * @ref Flag::BasePrefix is enabled, instead of lowercase.
             */
            Uppercase = 1 << 1,

            /**
             * Format with a 0x prefix. If @ref Flag::Uppercase is enabled as
             * well, prefixes with 0X instead of 0x. If both
             * @ref Flag::BasePrefix and @relativeref{Flag,HashPrefix} is
             * enabled, @relativeref{Flag,HashPrefix} has a precedence.
             */
            BasePrefix = 1 << 2,

            /**
             * Format with a # prefix, such as for color values. If both
             * @ref Flag::BasePrefix and @relativeref{Flag,HashPrefix} is
             * enabled, @relativeref{Flag,HashPrefix} has a precedence.
             */
            HashPrefix = 1 << 3,
        };

        /**
         * @brief Flags
         * @m_since_latest_{extras}
         *
         * @see @ref HexadecimalFormatter(), @ref setFlags(), @ref addFlags(),
         *      @ref clearFlags()
         */
        typedef Containers::EnumSet<Flag> Flags;

        /**
         * @brief Parse text matching the formatter output
         * @param text      Text to parse
         * @param value     Where to put the parsed value
         * @return Result of the parsing operation
         *
         * Expects that @p text is
         * @relativeref{Corrade,Containers::StringViewFlag::NullTerminated}.
         * Leading and trailing whitespace is trimmed from the text and the
         * value is parsed assuming it's hexadecimal, potentially with a 0x /
         * 0X or # prefix. Both lowercase and uppercase input is supported.
         *
         * If parsing succeeds, the function returns @ref ParseState::Success
         * and populates @p value. If the text is empty or all whitespace,
         * @ref ParseState::Failed is returned, same if the text contains a
         * negative value and the resulting type is unsigned. Otherwise, if the
         * value cannot fit into chosen resulting type, @p value is filled with
         * a corresponding min / max representable value and
         * @ref ParseState::Clamped is returned.
         * @see @relativeref{Corrade,Containers::StringView::trimmed()},
         *      @ref Math::TypeTraits::min(), @ref Math::TypeTraits::max()
         */
        static ParseState parse(Containers::StringView text, Int& value);
        /** @overload */
        static ParseState parse(Containers::StringView text, UnsignedInt& value);
        /** @overload */
        static ParseState parse(Containers::StringView text, Long& value);
        /** @overload */
        static ParseState parse(Containers::StringView text, UnsignedLong& value);

        /**
         * @brief Constructor
         * @param flags         Flags
         *
         * The formatter is only usable once it's attached to concrete text
         * data by calling @ref attach().
         */
        explicit HexadecimalFormatter(Flags flags = {}): _layer{}, _data{}, _flags{flags}, _minWidth{1} {}

        /**
         * @brief Construct and attach to a concrete text data
         * @param layer     Text layer instance
         * @param data      Text data handle
         * @param flags     Flags
         *
         * Equivalent to constructing with @ref HexadecimalFormatter(Flags) and
         * then calling @ref attach(TextLayer&, DataHandle). See their
         * documentation for more information.
         */
        explicit HexadecimalFormatter(TextLayer& layer, DataHandle data, Flags flags = {}): HexadecimalFormatter{flags} {
            attach(layer, data);
        }

        /**
         * @brief Construct and attach to a concrete text data assuming it comes from given layer
         * @param layer     Text layer instance
         * @param data      Text data handle
         * @param flags     Flags
         *
         * Equivalent to constructing with @ref HexadecimalFormatter(Flags) and
         * then calling @ref attach(TextLayer&, LayerDataHandle). See their
         * documentation for more information.
         */
        explicit HexadecimalFormatter(TextLayer& layer, LayerDataHandle data, Flags flags = {}): HexadecimalFormatter{flags} {
            attach(layer, data);
        }

        /**
         * @brief Text layer instance
         *
         * If @ref attach() wasn't called yet, either directly or from the
         * constructor, returns @cpp nullptr @ce.
         */
        TextLayer* layer() const { return _layer; }

        /**
         * @brief Text data handle
         *
         * If @ref attach() wasn't called yet, either directly or from the
         * constructor, returns @ref DataHandle::Null.
         */
        DataHandle data() const;

        /**
         * @brief Attach the formatter to text data
         * @return Reference to self (for method chaining)
         *
         * Expects that @p data is valid in @p layer. Upon calling this
         * function, @ref operator()() can be used. Calling this function again
         * with different arguments will reassign the formatter elsewhere. It's
         * however not possible to detach the formatter once it has been
         * attached.
         * @see @ref TextLayer::isHandleValid(DataHandle) const
         */
        HexadecimalFormatter& attach(TextLayer& layer, DataHandle data);

        /**
         * @brief Attach the formatter to text data assuming it comes from given layer
         * @return Reference to self (for method chaining)
         *
         * Like @ref attach(TextLayer&, DataHandle) but without checking that
         * @p handle indeed belongs to @p layer. See its documentation
         * for more information.
         * @see @ref TextLayer::isHandleValid(LayerDataHandle) const,
         *      @ref dataHandleData()
         */
        HexadecimalFormatter& attach(TextLayer& layer, LayerDataHandle data);

        /** @brief Flags */
        Flags flags() const { return _flags; }

        /**
         * @brief Set flags
         * @return Reference to self (for method chaining)
         *
         * @see @ref addFlags(), @ref clearFlags()
         */
        HexadecimalFormatter& setFlags(Flags flags) {
            _flags = flags;
            return *this;
        }

        /**
         * @brief Add flags
         * @return Reference to self (for method chaining)
         *
         * Calls @ref setFlags() with the existing flags ORed with @p flags.
         * Useful for preserving previously set flags.
         * @see @ref clearFlags()
         */
        HexadecimalFormatter& addFlags(Flags flags) {
            return setFlags(_flags|flags);
        }

        /**
         * @brief Clear flags
         * @return Reference to self (for method chaining)
         *
         * Calls @ref setFlags() with the existing flags ANDed with the inverse
         * of @p flags. Useful for removing a subset of previously set flags.
         * @see @ref addFlags()
         */
        HexadecimalFormatter& clearFlags(Flags flags) {
            return setFlags(_flags & ~flags);
        }

        /** @brief Minimal number width */
        Int minWidth() const { return _minWidth; }

        /**
         * @brief Set minimal number width
         *
         * If the formatted number has less digits than specified @p width,
         * it's padded by leading zeros. Sign or prefix isn't counted into the
         * width. Expects that @p width is at least @cpp 0 @ce and at most
         * @cpp 255 @ce. Default value is @cpp 1 @ce.
         *
         * If @p width is @cpp 0 @ce, formatting a zero value results in an
         * empty text, regardless of whether @ref Flag::ExplicitPlus,
         * @relativeref{Flag,BasePrefix} or @relativeref{Flag,HashPrefix} was
         * specified.
         */
        HexadecimalFormatter& setMinWidth(Int width);

        /**
         * @brief Format a value
         *
         * Expects that @ref attach() was called and the attached-to data
         * handle is still valid. Internally formats the @p value to a
         * stack-allocated string and passes it to @ref TextLayer::setText(),
         * see that function documentation for more information.
         */
        void operator()(Int value) const;
        void operator()(UnsignedInt value) const; /**< @overload */
        void operator()(Long value) const; /**< @overload */
        void operator()(UnsignedLong value) const; /**< @overload */

    private:
        template<class T> MAGNUM_UI_LOCAL void format(T value) const;

        TextLayer* _layer;
        LayerDataHandle _data;
        Flags _flags;
        UnsignedByte _minWidth;
        /* 2 bytes free; 4/8 more bytes left to use for an in-place Function */
};

/**
@debugoperatorclassenum{HexadecimalFormatter,Flag}
@m_since_latest_{extras}
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, HexadecimalFormatter::Flag value);

/**
@debugoperatorclassenum{HexadecimalFormatter,Flags}
@m_since_latest_{extras}
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, HexadecimalFormatter::Flags value);

CORRADE_ENUMSET_OPERATORS(HexadecimalFormatter::Flags)

/**
@brief Formatter for floating-point values
@m_since_latest_{extras}

@see @ref DecimalFormatter, @ref HexadecimalFormatter
*/
class MAGNUM_UI_EXPORT FloatFormatter {
    public:
        /**
         * @brief Flag
         *
         * @see @ref Flags, @ref FloatFormatter(), @ref setFlags(),
         *      @ref addFlags(), @ref clearFlags()
        */
        enum class Flag: UnsignedByte {
            /** Prefix positive values with an explicit + sign */
            ExplicitPlus = 1 << 0,

            /**
             * Use uppercase for exponents, infinity and NaN values. Default is
             * lowercase.
             */
            Uppercase = 1 << 1,

            /**
             * Force decimal notation. The precision set by @ref setPrecision()
             * then specifies the number of digits after the decimal point.
             * Default is either decimal or exponent notation depending on
             * value an precision. Setting both @ref Flag::Decimal and
             * @relativeref{Flag,Exponent} behaves as settting neither of the
             * two.
             */
            Decimal = 1 << 2,

            /**
             * Force decimal exponent notation. The precision set by
             * @ref setPrecision() then specifies the number of digits after
             * the decimal point. Default is either decimal or exponent
             * notation depending on value an precision. Setting both
             * @ref Flag::Decimal and @relativeref{Flag,Exponent} behaves as
             * settting neither of the two.
             */
            Exponent = 1 << 3,
        };

        /**
         * @brief Flags
         * @m_since_latest_{extras}
         *
         * @see @ref FloatFormatter(), @ref setFlags(), @ref addFlags(),
         *      @ref clearFlags()
         */
        typedef Containers::EnumSet<Flag> Flags;

        /**
         * @brief Parse text matching the formatter output
         * @param text      Text to parse
         * @param value     Where to put the parsed value
         * @return Result of the parsing operation
         *
         * Expects that @p text is
         * @relativeref{Corrade,Containers::StringViewFlag::NullTerminated}.
         * Leading and trailing whitespace is trimmed from the text and the
         * value is parsed assuming it's floating-point with an optional
         * exponent or an inf or nan, preceded with an optional sign.
         *
         * If parsing succeeds, the function returns @ref ParseState::Success
         * and populates @p value. If the text is empty or all whitespace,
         * @ref ParseState::Failed is returned, same if the text contains a
         * negative value and the resulting type is unsigned. Otherwise, if the
         * value cannot fit into chosen resulting type, @p value is filled with
         * a corresponding positive or negative infinity and
         * @ref ParseState::Clamped is returned. Parsing a literal inf or nan
         * itself results in @ref ParseState::Success, not
         * @ref ParseState::Clamped.
         * @see @relativeref{Corrade,Containers::StringView::trimmed()}
         */
        static ParseState parse(Containers::StringView text, Float& value);
        /** @overload */
        static ParseState parse(Containers::StringView text, Double& value);

        /**
         * @brief Constructor
         * @param flags         Flags
         *
         * The formatter is only usable once it's attached to concrete text
         * data by calling @ref attach().
         */
        explicit FloatFormatter(Flags flags = {}): _layer{}, _data{}, _flags{flags}, _precision{6} {}

        /**
         * @brief Construct and attach to a concrete text data
         * @param layer     Text layer instance
         * @param data      Text data handle
         * @param flags     Flags
         *
         * Equivalent to constructing with @ref FloatFormatter(Flags) and then
         * calling @ref attach(TextLayer&, DataHandle). See their documentation
         * for more information.
         */
        explicit FloatFormatter(TextLayer& layer, DataHandle data, Flags flags = {}): FloatFormatter{flags} {
            attach(layer, data);
        }

        /**
         * @brief Construct and attach to a concrete text data assuming it comes from given layer
         * @param layer     Text layer instance
         * @param data      Text data handle
         * @param flags     Flags
         *
         * Equivalent to constructing with @ref FloatFormatter(Flags) and then
         * calling @ref attach(TextLayer&, LayerDataHandle). See their
         * documentation for more information.
         */
        explicit FloatFormatter(TextLayer& layer, LayerDataHandle data, Flags flags = {}): FloatFormatter{flags} {
            attach(layer, data);
        }

        /**
         * @brief Text layer instance
         *
         * If @ref attach() wasn't called yet, either directly or from the
         * constructor, returns @cpp nullptr @ce.
         */
        TextLayer* layer() const { return _layer; }

        /**
         * @brief Text data handle
         *
         * If @ref attach() wasn't called yet, either directly or from the
         * constructor, returns @ref DataHandle::Null.
         */
        DataHandle data() const;

        /**
         * @brief Attach the formatter to text data
         * @return Reference to self (for method chaining)
         *
         * Expects that @p data is valid in @p layer. Upon calling this
         * function, @ref operator()() can be used. Calling this function again
         * with different arguments will reassign the formatter elsewhere. It's
         * however not possible to detach the formatter once it has been
         * attached.
         * @see @ref TextLayer::isHandleValid(DataHandle) const
         */
        FloatFormatter& attach(TextLayer& layer, DataHandle data);

        /**
         * @brief Attach the formatter to text data assuming it comes from given layer
         * @return Reference to self (for method chaining)
         *
         * Like @ref attach(TextLayer&, DataHandle) but without checking that
         * @p handle indeed belongs to @p layer. See its documentation
         * for more information.
         * @see @ref TextLayer::isHandleValid(LayerDataHandle) const,
         *      @ref dataHandleData()
         */
        FloatFormatter& attach(TextLayer& layer, LayerDataHandle data);

        /** @brief Flags */
        Flags flags() const { return _flags; }

        /**
         * @brief Set flags
         * @return Reference to self (for method chaining)
         *
         * @see @ref addFlags(), @ref clearFlags()
         */
        FloatFormatter& setFlags(Flags flags) {
            _flags = flags;
            return *this;
        }

        /**
         * @brief Add flags
         * @return Reference to self (for method chaining)
         *
         * Calls @ref setFlags() with the existing flags ORed with @p flags.
         * Useful for preserving previously set flags.
         * @see @ref clearFlags()
         */
        FloatFormatter& addFlags(Flags flags) {
            return setFlags(_flags|flags);
        }

        /**
         * @brief Clear flags
         * @return Reference to self (for method chaining)
         *
         * Calls @ref setFlags() with the existing flags ANDed with the inverse
         * of @p flags. Useful for removing a subset of previously set flags.
         * @see @ref addFlags()
         */
        FloatFormatter& clearFlags(Flags flags) {
            return setFlags(_flags & ~flags);
        }

        /** @brief Number precision */
        Int precision() const { return _precision; }

        /**
         * @brief Set number precision
         *
         * Expects that @p precision is at least @cpp 0 @ce and at most
         * @cpp 255 @ce. Note that the precision choice along with the value
         * itself affects whether the value is printed with or without an
         * exponent, enable @ref Flag::Decimal or @ref Flag::Exponent to force
         * one or the other representation. Default value is @cpp 6 @ce, which
         * is consistent with how @relativeref{Corrade::Utility,Debug} or
         * @relativeref{Corrade,Utility::format()} prints
         * @relativeref{Magnum,Float} values.
         *
         * @m_class{m-note m-warning}
         *
         * @par
         *      Note that the same precision is used for printing both
         *      @relativeref{Magnum,Float} and @relativeref{Magnum,Double}
         *      values. The default @relativeref{Corrade::Utility,Debug} and
         *      @relativeref{Corrade,Utility::format()} precision for printing
         *      @relativeref{Magnum,Double} values is @cpp 15 @ce instead.
         */
        FloatFormatter& setPrecision(Int precision);

        /**
         * @brief Format a value
         *
         * Expects that @ref attach() was called and the attached-to data
         * handle is still valid. Internally formats the @p value to a
         * stack-allocated string and passes it to @ref TextLayer::setText(),
         * see that function documentation for more information.
         */
        void operator()(Float value) const;
        void operator()(Double value) const; /**< @overload */

    private:
        TextLayer* _layer;
        LayerDataHandle _data;
        Flags _flags;
        UnsignedByte _precision;
        /* 2 bytes free; 4/8 more bytes left to use for an in-place Function */
};

/**
@debugoperatorclassenum{FloatFormatter,Flag}
@m_since_latest_{extras}
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, FloatFormatter::Flag value);

/**
@debugoperatorclassenum{FloatFormatter,Flags}
@m_since_latest_{extras}
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, FloatFormatter::Flags value);

CORRADE_ENUMSET_OPERATORS(FloatFormatter::Flags)

}}

#endif
