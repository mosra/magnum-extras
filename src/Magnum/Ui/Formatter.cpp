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

#include "Formatter.h"

#include <cerrno> /** @todo throw away once Corrade has standalone formatters */
#include <cinttypes> /* PRIi64, PRIu64 */ /** @todo throw away also, ugh */
#include <cstdio> /** @todo throw away once Corrade has integer parsers */
#include <Corrade/Containers/ArrayView.h>
#include <Corrade/Containers/EnumSet.hpp>
#include <Corrade/Containers/String.h>
#include <Corrade/Utility/Assert.h>

#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/TextLayer.h"
#include "Magnum/Ui/TextProperties.h"

namespace Magnum { namespace Ui {

using namespace Containers::Literals;

Debug& operator<<(Debug& debug, const DecimalFormatter::Flag value) {
    debug << "Ui::DecimalFormatter::Flag" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case DecimalFormatter::Flag::value: return debug << "::" #value;
        _c(ExplicitPlus)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const DecimalFormatter::Flags value) {
    return Containers::enumSetDebugOutput(debug, value, "Ui::DecimalFormatter::Flags{}", {
        DecimalFormatter::Flag::ExplicitPlus,
    });
}

Debug& operator<<(Debug& debug, const HexadecimalFormatter::Flag value) {
    debug << "Ui::HexadecimalFormatter::Flag" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case HexadecimalFormatter::Flag::value: return debug << "::" #value;
        _c(ExplicitPlus)
        _c(Uppercase)
        _c(BasePrefix)
        _c(HashPrefix)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const HexadecimalFormatter::Flags value) {
    return Containers::enumSetDebugOutput(debug, value, "Ui::HexadecimalFormatter::Flags{}", {
        HexadecimalFormatter::Flag::ExplicitPlus,
        HexadecimalFormatter::Flag::Uppercase,
        HexadecimalFormatter::Flag::BasePrefix,
        HexadecimalFormatter::Flag::HashPrefix,
    });
}

Debug& operator<<(Debug& debug, const FloatFormatter::Flag value) {
    debug << "Ui::FloatFormatter::Flag" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case FloatFormatter::Flag::value: return debug << "::" #value;
        _c(ExplicitPlus)
        _c(Uppercase)
        _c(Decimal)
        _c(Exponent)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const FloatFormatter::Flags value) {
    return Containers::enumSetDebugOutput(debug, value, "Ui::FloatFormatter::Flags{}", {
        FloatFormatter::Flag::ExplicitPlus,
        FloatFormatter::Flag::Uppercase,
        FloatFormatter::Flag::Decimal,
        FloatFormatter::Flag::Exponent,
    });
}

Debug& operator<<(Debug& debug, const ParseState value) {
    debug << "Ui::ParseState" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case ParseState::value: return debug << "::" #value;
        _c(Success)
        _c(Clamped)
        _c(Failed)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

namespace {

template<class> struct ParseTraits;
/* It was
    constexpr static long long(*parser)(const char*, char**, int) = std::strtoll;
   etc. at first, but GCC 4.8 says such a thing isn't a constant expression. So
   making inline wrappers instead, which should be fine as this code isn't
   perf critical. Once Corrade has its own integer parsers, this all can go. */
template<> struct ParseTraits<Long> {
    static long long parser(const char* string, char** end, int base) {
        return std::strtoll(string, end, base);
    }
};
template<> struct ParseTraits<UnsignedLong> {
    static unsigned long long parser(const char* string, char** end, int base) {
        return std::strtoull(string, end, base);
    }
};
template<> struct ParseTraits<Float> {
    static Float parser(const char* string, char** end) {
        return std::strtof(string, end);
    }
};
template<> struct ParseTraits<Double> {
    static Double parser(const char* string, char** end) {
        return std::strtod(string, end);
    }
};

Containers::StringView parsePrepareDecimal(const Containers::StringView text) {
    /* Return the original view to not cause a crash inside std::strto[u]ll()
       when testing with graceful assertions, because of course it just crashes
       instead of doing something reasonable */
    CORRADE_ASSERT(text.flags() >= Containers::StringViewFlag::NullTerminated,
        "Ui::DecimalFormatter::parse(): text is not null-terminated", text);

    /* Trim whitespace from both ends of the string. Trimming of initial spaces
       is done by std::strto[u]ll() as well but as we need to do the same at
       the end, we can just do both for symmetry and more control. */
    return text.trimmed();
}

Containers::String parsePrepareHexadecimal(/*mutable*/ Containers::StringView text) {
    /* Compared to above, here a default-constructed String is already
       null-terminated so this doesn't cause a crash when testing with
       graceful assertions */
    CORRADE_ASSERT(text.flags() >= Containers::StringViewFlag::NullTerminated,
        "Ui::HexadecimalFormatter::parse(): text is not null-terminated", {});

    /* Trim whitespace from both ends of the string first, same as in the
       decimal case above */
    text = text.trimmed();

    /* On Emscripten (musl) the parser happily accepts just the 0x / 0X prefix
       alone, with no digits after. Other platforms treat that as an error and
       so should we. The code responsible for the parsing looks like this and
       ... I don't think I want to dig deeper.
        https://git.musl-libc.org/cgit/musl/tree/src/internal/intscan.c?id=5122f9f3c99fee366167c5de98b31546312921ab#n41 */
    #ifdef CORRADE_TARGET_EMSCRIPTEN
    if(text == "0x"_s || text == "0X"_s ||
       text == "+0x"_s || text == "+0X"_s ||
       text == "-0x"_s || text == "-0X"_s)
        /* Return an empty (null-terminated) string which should fail on its
           own */
        return {};
    #endif

    /* If the text has a hash prefix and it isn't just the prefix alone,
       replace it with a zero to make std::strto[u]ll() understand it. It
       understands the 0x / 0X prefix on its own, no need to do anything for
       that one. Alternatively I could also pass just the contents after the
       prefix, but that'd require me to handle negation manually, which is
       nasty given that signed types don't have a symmetric range. */
    std::size_t hashPosition = ~std::size_t{};
    if(text.hasPrefix('#'))
        hashPosition = 0;
    else if(text.hasPrefix("-#") || text.hasPrefix("+#"))
        hashPosition = 1;
    if(hashPosition != ~std::size_t{} && hashPosition + 1 != text.size()) {
        /* The longest representable hexadecimal value is 16 bytes long, which
           easily fits in the SSO along with a prefix and sign, and all spaces
           were trimmed above already. Worst case the string has many leading
           zeros, in which case the copy will allocate, but that should be
           rare. */
        Containers::String copy{text};
        copy[hashPosition] = '0';
        return copy;
    }

    /* Otherwise return a view wrapped in a String. As the input is always
       null-terminated this will never make a copy. */
    return Containers::String::nullTerminatedView(text);
}

Containers::StringView parsePrepareFloat(/*mutable*/ Containers::StringView text) {
    /* Return the original view to not cause a crash inside std::strto[df]()
       when testing with graceful assertions, because of course it just crashes
       instead of doing something reasonable */
    CORRADE_ASSERT(text.flags() >= Containers::StringViewFlag::NullTerminated,
        "Ui::FloatFormatter::parse(): text is not null-terminated", text);

    /* Trim whitespace from both ends of the string first, same as in the
       decimal case above */
    text = text.trimmed();

    /* I don't intend to support this weird hex representation once Corrade has
       own float parsers so disallowing it here already. (A hex representation
       of a float/double bit pattern is something else, supporting that makes
       sense, but that doesn't need a complex float parser.) The value can have
       a p / P character denoting the exponent but that's optional so instead I
       have to check for all possible variants. */
    if(text.hasPrefix("0x"_s) || text.hasPrefix("0X"_s) ||
       text.hasPrefix("+0x"_s) || text.hasPrefix("+0X"_s) ||
       text.hasPrefix("-0x"_s) || text.hasPrefix("-0X"_s))
        /* Returning an empty view pointing to the end so it's still
           null-terminated, otherwise std::strtof() and such crashes. */
        return text.suffix(text.end());

    /* Trim whitespace from both ends of the string. Trimming of initial spaces
       is done by std::strtof() as well but as we need to do the same at the
       end, we can just do both for symmetry and more control. */
    return text.trimmed();
}

/* The actual parsing is implemented just for 64-bit types, because that's what
   C++ provides, and std::atoi() is dangerous and useless because it has no
   error reporting or base specification whatsoever */
template<class T> ParseState parseLong(const Containers::StringView text, T& value, const Int base) {
    /* Assumes that text was processed by parsePrepareDecimal() or
       parsePrepareHexadecimal() first, in particular with all leading and
       trailing whitespace trimmed already */

    /* If we're parsing an unsigned type and this is a negative value, treat
       that as a failure. In comparison, std::strtoull() treats that just as a
       range error (and for -0 not even that) but for the UI feedback I feel it
       should be a more catastrophical failure. */
    if(std::is_unsigned<T>::value && text.hasPrefix('-'))
        return ParseState::Failed;

    /* Reset errno so we can detect an overflow below and attempt a parse. If
       the text was non-empty (and not just whitespace) in the first place and
       the parsing consumed it until the (potentially trimmed) end, we parsed
       successfully. Worst case it may have overflown. */
    errno = 0;
    char* end{};
    value = ParseTraits<T>::parser(text.begin(), &end, base);
    if(text && end == text.end()) {
        if(errno == ERANGE)
            return ParseState::Clamped;
        CORRADE_INTERNAL_ASSERT(errno == 0);
        return ParseState::Success;
    }

    /* If the text was empty or the parsing didn't consume everything, it's an
       error */
    return ParseState::Failed;
}

/* The 32-bit variants need to do extra bounds checks. It would be amazing if I
   had access to PROPER 32-bit variants of std::strto*() as well, not just a
   stupid `long` / `long long` distinction which in practice is useless. */
ParseState parseInt(const Containers::StringView text, Int& value, const Int base) {
    /* Parse into a 64bit value first */
    Long parsedValue;
    const ParseState state = parseLong(text, parsedValue, base);

    /* If the parse succeeded and the value fits into the target type (checking
       by converting to the smaller type and comparing to the original), return
       the value directly */
    if(state == ParseState::Success && Int(parsedValue) == parsedValue) {
        value = parsedValue;
        return ParseState::Success;
    }

    /* Otherwise, if the parse already clamped or succeeded (and the result
       cannot fit, which would be the case with Success since the above branch
       wasn't taken), return one of the extremes based on whether the parsed
       value is negative or positive. A zero value fits always so that's a case
       that shouldn't happen here. */
    if(state == ParseState::Clamped || state == ParseState::Success) {
        if(parsedValue < 0)
            value = Math::TypeTraits<Int>::min();
        else if(parsedValue > 0)
            value = Math::TypeTraits<Int>::max();
        else CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
        return ParseState::Clamped;
    }

    /* Otherwise, there was a failure */
    CORRADE_INTERNAL_ASSERT(state == ParseState::Failed);
    return ParseState::Failed;
}

/* Like above, but an unsigned case where the clamped value can be just one.
   Comments omitted for terseness. */
ParseState parseInt(const Containers::StringView text, UnsignedInt& value, const Int base) {
    UnsignedLong parsedValue;
    const ParseState state = parseLong(text, parsedValue, base);

    if(state == ParseState::Success && UnsignedInt(parsedValue) == parsedValue) {
        value = parsedValue;
        return ParseState::Success;
    }

    if(state == ParseState::Clamped || state == ParseState::Success) {
        value = Math::TypeTraits<UnsignedInt>::max();
        return ParseState::Clamped;
    }

    CORRADE_INTERNAL_ASSERT(state == ParseState::Failed);
    return ParseState::Failed;
}

template<class T> ParseState parseFloat(const Containers::StringView text, T& value) {
    /* Assumes that text was processed by parsePrepareFloat() first, in
       particular with all leading and trailing whitespace trimmed already */

    /* Reset errno so we can detect an overflow below and attempt a parse. If
       the text was non-empty (and not just whitespace) in the first place and
       the parsing consumed it until the (potentially trimmed) end, we parsed
       successfully. Worst case it may have overflown, in which case the value
       is positive or negative infinity. */
    errno = 0;
    char* end{};
    value = ParseTraits<T>::parser(text.begin(), &end);
    if(text && end == text.end()) {
        if(errno == ERANGE)
            return ParseState::Clamped;
        CORRADE_INTERNAL_ASSERT(errno == 0);
        return ParseState::Success;
    }

    /* If the text was empty or the parsing didn't consume everything, it's an
       error */
    return ParseState::Failed;
}

}

ParseState DecimalFormatter::parse(const Containers::StringView text, Long& value) {
    return parseLong(parsePrepareDecimal(text), value, 10);
}

ParseState DecimalFormatter::parse(const Containers::StringView text, UnsignedLong& value) {
    return parseLong(parsePrepareDecimal(text), value, 10);
}

ParseState DecimalFormatter::parse(const Containers::StringView text, Int& value) {
    return parseInt(parsePrepareDecimal(text), value, 10);
}

ParseState DecimalFormatter::parse(const Containers::StringView text, UnsignedInt& value) {
    return parseInt(parsePrepareDecimal(text), value, 10);
}

ParseState HexadecimalFormatter::parse(const Containers::StringView text, Long& value) {
    return parseLong(parsePrepareHexadecimal(text), value, 16);
}

ParseState HexadecimalFormatter::parse(const Containers::StringView text, UnsignedLong& value) {
    return parseLong(parsePrepareHexadecimal(text), value, 16);
}

ParseState HexadecimalFormatter::parse(const Containers::StringView text, Int& value) {
    return parseInt(parsePrepareHexadecimal(text), value, 16);
}

ParseState HexadecimalFormatter::parse(const Containers::StringView text, UnsignedInt& value) {
    return parseInt(parsePrepareHexadecimal(text), value, 16);
}

ParseState FloatFormatter::parse(const Containers::StringView text, Float& value) {
    return parseFloat(parsePrepareFloat(text), value);
}

ParseState FloatFormatter::parse(const Containers::StringView text, Double& value) {
    return parseFloat(parsePrepareFloat(text), value);
}

DecimalFormatter& DecimalFormatter::setMinWidth(const Int width) {
    CORRADE_ASSERT(width >= 0 && width <= 255,
        "Ui::DecimalFormatter::setMinWidth(): expected width to be between 0 and 255 but got" << width, *this);
    _minWidth = width;
    return *this;
}

HexadecimalFormatter& HexadecimalFormatter::setMinWidth(const Int width) {
    CORRADE_ASSERT(width >= 0 && width <= 255,
        "Ui::HexadecimalFormatter::setMinWidth(): expected width to be between 0 and 255 but got" << width, *this);
    _minWidth = width;
    return *this;
}

FloatFormatter& FloatFormatter::setPrecision(const Int precision) {
    CORRADE_ASSERT(precision >= 0 && precision <= 255,
        "Ui::FloatFormatter::setPrecision(): expected precision to be between 0 and 255 but got" << precision, *this);
    _precision = precision;
    return *this;
}

namespace {

/** @todo throw away all this once Corrade has standalone formatters */
template<class> constexpr const char* decimalFormatString();
template<> constexpr const char* decimalFormatString<UnsignedInt>() {
    return "%.*u";
}
template<> constexpr const char* decimalFormatString<UnsignedLong>() {
    return "%.*" PRIu64; /* how trash can we get, eh? */
}

}

template<class T> inline void DecimalFormatter::format(TextLayer& layer, const LayerDataHandle data, const T value) const {
    /** @todo throw away all this once Corrade has standalone formatters */
    /* The output size can be at most the largest possible _minWidth value,
       a plus or minus, and a null terminator */
    char output[255 + 1 + 1];
    /* While we can use + in the format string to have an explict plus, this is
       consistent with the hexadecimal case (where printf doesn't even support
       negative numbers) */
    typename std::make_unsigned<T>::type unsignedValue;
    std::size_t begin = 0;
    if(value < 0) {
        /* Interestingly enough this *does* seem to correctly handle
           -0x80000000 (and the corresponding 64-bit value), producing
           0x80000000 even though it doesn't fit into a signed type. For
           unsigned types this warns on MSVC ("warning C4146: unary minus
           operator applied to unsigned type") because ugh stupid thing. Yeah
           yeah if constexpr would solve this etc etc. I know. */
        #if defined(CORRADE_TARGET_MSVC) && !defined(CORRADE_TARGET_CLANG_CL)
        #pragma warning(push)
        #pragma warning(disable: 4146)
        #endif
        unsignedValue = -value;
        #if defined(CORRADE_TARGET_MSVC) && !defined(CORRADE_TARGET_CLANG_CL)
        #pragma warning(pop)
        #endif
        output[begin++] = '-';
    } else {
        unsignedValue = value;
        /* If the value is zero and min width is zero, nothing should be
           printed even with ExplicitPlus */
        if(_flags >= Flag::ExplicitPlus && (value || _minWidth))
            output[begin++] = '+';
    }
    const std::size_t size = begin + std::snprintf(output + begin, Containers::arraySize(output) - begin, decimalFormatString<typename std::make_unsigned<T>::type>(), Int(_minWidth), unsignedValue);
    CORRADE_INTERNAL_ASSERT(size < Containers::arraySize(output));
    layer.setText(data, {output, size}, {});
}

void DecimalFormatter::operator()(TextLayer& layer, const DataHandle data, const Int value) const {
    CORRADE_ASSERT(layer.isHandleValid(data),
        "Ui::DecimalFormatter: invalid handle" << data, );
    format(layer, dataHandleData(data), value);
}

void DecimalFormatter::operator()(TextLayer& layer, const DataHandle data, const UnsignedInt value) const {
    CORRADE_ASSERT(layer.isHandleValid(data),
        "Ui::DecimalFormatter: invalid handle" << data, );
    format(layer, dataHandleData(data), value);
}

void DecimalFormatter::operator()(TextLayer& layer, const DataHandle data, const Long value) const {
    CORRADE_ASSERT(layer.isHandleValid(data),
        "Ui::DecimalFormatter: invalid handle" << data, );
    format(layer, dataHandleData(data), value);
}

void DecimalFormatter::operator()(TextLayer& layer, const DataHandle data, const UnsignedLong value) const {
    CORRADE_ASSERT(layer.isHandleValid(data),
        "Ui::DecimalFormatter: invalid handle" << data, );
    format(layer, dataHandleData(data), value);
}

void DecimalFormatter::operator()(TextLayer& layer, const LayerDataHandle data, const Int value) const {
    CORRADE_ASSERT(layer.isHandleValid(data),
        "Ui::DecimalFormatter: invalid handle" << data, );
    format(layer, data, value);
}

void DecimalFormatter::operator()(TextLayer& layer, const LayerDataHandle data, const UnsignedInt value) const {
    CORRADE_ASSERT(layer.isHandleValid(data),
        "Ui::DecimalFormatter: invalid handle" << data, );
    format(layer, data, value);
}

void DecimalFormatter::operator()(TextLayer& layer, const LayerDataHandle data, const Long value) const {
    CORRADE_ASSERT(layer.isHandleValid(data),
        "Ui::DecimalFormatter: invalid handle" << data, );
    format(layer, data, value);
}

void DecimalFormatter::operator()(TextLayer& layer, const LayerDataHandle data, const UnsignedLong value) const {
    CORRADE_ASSERT(layer.isHandleValid(data),
        "Ui::DecimalFormatter: invalid handle" << data, );
    format(layer, data, value);
}

namespace {

/** @todo throw away all this once Corrade has standalone formatters */
template<class> constexpr const char* hexadecimalFormatString(bool);
template<> constexpr const char* hexadecimalFormatString<UnsignedInt>(bool uppercase) {
    return uppercase ? "%.*X" : "%.*x";
}
template<> constexpr const char* hexadecimalFormatString<UnsignedLong>(bool uppercase) {
    return uppercase ? "%.*llX" : "%.*llx";
}

}

template<class T> void HexadecimalFormatter::format(TextLayer& layer, const LayerDataHandle data, const T value) const {
    /** @todo throw away all this once Corrade has standalone formatters */
    /* The output size can be at most the largest possible _minWidth value,
       a plus or minus, a prefix, and a null terminator */
    char output[255 + 1 + 2 + 1];
    /* While we can use + in the format string to have an explict plus, it
       doesn't when prefix is involved, and as printf doesn't support signed
       hexadecimal numbers we'd have to handle - explicitly anyway */
    typename std::make_unsigned<T>::type unsignedValue;
    std::size_t begin = 0;
    if(value < 0) {
        /* Interestingly enough this *does* seem to correctly handle
           -0x80000000 (and the corresponding 64-bit value), producing
           0x80000000 even though it doesn't fit into a signed type. For
           unsigned types this warns on MSVC ("warning C4146: unary minus
           operator applied to unsigned type") because ugh stupid thing. Yeah
           yeah if constexpr would solve this etc etc. I know. */
        #if defined(CORRADE_TARGET_MSVC) && !defined(CORRADE_TARGET_CLANG_CL)
        #pragma warning(push)
        #pragma warning(disable: 4146)
        #endif
        unsignedValue = -value;
        #if defined(CORRADE_TARGET_MSVC) && !defined(CORRADE_TARGET_CLANG_CL)
        #pragma warning(pop)
        #endif
        output[begin++] = '-';
    } else {
        unsignedValue = value;
        /* If the value is zero and min width is zero, nothing should be
           printed even with ExplicitPlus */
        if(_flags >= Flag::ExplicitPlus && (value || _minWidth))
            output[begin++] = '+';
    }
    /* If the value is zero and min width is zero, nothing should be printed
       even with BasePrefix / HashPrefix */
    if(value || _minWidth) {
        if(_flags >= Flag::HashPrefix)
            output[begin++] = '#';
        else if(_flags >= Flag::BasePrefix) {
            output[begin++] = '0';
            output[begin++] = _flags >= Flag::Uppercase ? 'X' : 'x';
        }
    }
    const std::size_t size = begin + std::snprintf(output + begin, Containers::arraySize(output) - begin, hexadecimalFormatString<typename std::make_unsigned<T>::type>(_flags >= Flag::Uppercase), Int(_minWidth), unsignedValue);
    CORRADE_INTERNAL_ASSERT(size < Containers::arraySize(output));
    layer.setText(data, {output, size}, {});
}

void HexadecimalFormatter::operator()(TextLayer& layer, const DataHandle data, const Int value) const {
    CORRADE_ASSERT(layer.isHandleValid(data),
        "Ui::HexadecimalFormatter: invalid handle" << data, );
    format(layer, dataHandleData(data), value);
}

void HexadecimalFormatter::operator()(TextLayer& layer, const DataHandle data, const UnsignedInt value) const {
    CORRADE_ASSERT(layer.isHandleValid(data),
        "Ui::HexadecimalFormatter: invalid handle" << data, );
    format(layer, dataHandleData(data), value);
}

void HexadecimalFormatter::operator()(TextLayer& layer, const DataHandle data, const Long value) const {
    CORRADE_ASSERT(layer.isHandleValid(data),
        "Ui::HexadecimalFormatter: invalid handle" << data, );
    format(layer, dataHandleData(data), value);
}

void HexadecimalFormatter::operator()(TextLayer& layer, const DataHandle data, const UnsignedLong value) const {
    CORRADE_ASSERT(layer.isHandleValid(data),
        "Ui::HexadecimalFormatter: invalid handle" << data, );
    format(layer, dataHandleData(data), value);
}

void HexadecimalFormatter::operator()(TextLayer& layer, const LayerDataHandle data, const Int value) const {
    CORRADE_ASSERT(layer.isHandleValid(data),
        "Ui::HexadecimalFormatter: invalid handle" << data, );
    format(layer, data, value);
}

void HexadecimalFormatter::operator()(TextLayer& layer, const LayerDataHandle data, const UnsignedInt value) const {
    CORRADE_ASSERT(layer.isHandleValid(data),
        "Ui::HexadecimalFormatter: invalid handle" << data, );
    format(layer, data, value);
}

void HexadecimalFormatter::operator()(TextLayer& layer, const LayerDataHandle data, const Long value) const {
    CORRADE_ASSERT(layer.isHandleValid(data),
        "Ui::HexadecimalFormatter: invalid handle" << data, );
    format(layer, data, value);
}

void HexadecimalFormatter::operator()(TextLayer& layer, const LayerDataHandle data, const UnsignedLong value) const {
    CORRADE_ASSERT(layer.isHandleValid(data),
        "Ui::HexadecimalFormatter: invalid handle" << data, );
    format(layer, data, value);
}

void FloatFormatter::format(TextLayer& layer, const LayerDataHandle data, const Double value) const {
    /** @todo throw away all this once Corrade has standalone formatters */
    /* Format string. Either "%.*S" or "%+.*S", where `S` is picked below.
       Compared to DecimalFormatter / HexadecimalFormatter here printf() itself
       is tasked with formatting the explicit plus because we have no special
       cases to deal with. */
    char formatString[6]{'%', 0, 0, 0, 0, 0};
    std::size_t formatStringBegin = 1;
    if(_flags >= Flag::ExplicitPlus)
        formatString[formatStringBegin++] = '+';
    formatString[formatStringBegin++] = '.';
    formatString[formatStringBegin++] = '*';

    /* If neither decimal or exponent printing is forced (or both are, which is
       the same), use the general printer */
    if(!(_flags & (Flag::Decimal|Flag::Exponent)) ||
        _flags >= (Flag::Decimal|Flag::Exponent))
        formatString[formatStringBegin++] = _flags >= Flag::Uppercase ? 'G' : 'g';
    else if(_flags >= Flag::Decimal)
        formatString[formatStringBegin++] = _flags >= Flag::Uppercase ? 'F' : 'f';
    else if(_flags >= Flag::Exponent)
        formatString[formatStringBegin++] = _flags >= Flag::Uppercase ? 'E' : 'e';
    else CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */

    /* The output size can be at most the largest possible digit count (309
       digits for the 11-bit exponent in 64-bit doubles), a sign, a period,
       the max _precision value and a null terminator */
    char output[309 + 1 + 1 + 255 + 1];
    const std::size_t size = std::snprintf(output, Containers::arraySize(output), formatString, Int(_precision), value);
    CORRADE_INTERNAL_ASSERT(size < Containers::arraySize(output));
    layer.setText(data, {output, size}, {});
}

void FloatFormatter::operator()(TextLayer& layer, const DataHandle data, const Float value) const {
    CORRADE_ASSERT(layer.isHandleValid(data),
        "Ui::FloatFormatter: invalid handle" << data, );
    /** @todo stop delegating to Double once there's a dedicated code path for
        float printing (with smaller LUTs and such), printf doesn't do that */
    return format(layer, dataHandleData(data), Double(value));
}

void FloatFormatter::operator()(TextLayer& layer, const DataHandle data, const Double value) const {
    CORRADE_ASSERT(layer.isHandleValid(data),
        "Ui::FloatFormatter: invalid handle" << data, );
    return format(layer, dataHandleData(data), value);
}

void FloatFormatter::operator()(TextLayer& layer, const LayerDataHandle data, const Double value) const {
    CORRADE_ASSERT(layer.isHandleValid(data),
        "Ui::FloatFormatter: invalid handle" << data, );
    return format(layer, data, value);
}

void FloatFormatter::operator()(TextLayer& layer, const LayerDataHandle data, const Float value) const {
    CORRADE_ASSERT(layer.isHandleValid(data),
        "Ui::FloatFormatter: invalid handle" << data, );
    /** @todo stop delegating to Double once there's a dedicated code path for
        float printing (with smaller LUTs and such), printf doesn't do that */
    return format(layer, data, Double(value));
}

}}
