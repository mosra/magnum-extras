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

#include <cstdio> /** @todo throw away once Corrade has standalone formatters */
#include <cinttypes> /* PRIi64, PRIu64 */ /** @todo throw away also, ugh */
#include <Corrade/Containers/ArrayView.h>
#include <Corrade/Containers/EnumSet.hpp>
#include <Corrade/Utility/Assert.h>

#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/TextLayer.h"
#include "Magnum/Ui/TextProperties.h"

namespace Magnum { namespace Ui {

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

DataHandle DecimalFormatter::data() const {
    return _layer ? dataHandle(_layer->handle(), _data) : DataHandle::Null;
}

DataHandle HexadecimalFormatter::data() const {
    return _layer ? dataHandle(_layer->handle(), _data) : DataHandle::Null;
}

DataHandle FloatFormatter::data() const {
    return _layer ? dataHandle(_layer->handle(), _data) : DataHandle::Null;
}

DecimalFormatter& DecimalFormatter::attach(TextLayer& layer, const DataHandle data) {
    CORRADE_ASSERT(data != DataHandle::Null,
        "Ui::DecimalFormatter::attach(): invalid handle" << data, *this);
    CORRADE_ASSERT(layer.isHandleValid(data),
        "Ui::DecimalFormatter::attach():" << data << "not valid in" << layer.handle(), *this);
    _layer = &layer;
    _data = dataHandleData(data);
    return *this;
}

DecimalFormatter& DecimalFormatter::attach(TextLayer& layer, const LayerDataHandle data) {
    CORRADE_ASSERT(data != LayerDataHandle::Null,
        "Ui::DecimalFormatter::attach(): invalid handle" << data, *this);
    CORRADE_ASSERT(layer.isHandleValid(data),
        "Ui::DecimalFormatter::attach():" << data << "not valid in" << layer.handle(), *this);
    _layer = &layer;
    _data = data;
    return *this;
}

HexadecimalFormatter& HexadecimalFormatter::attach(TextLayer& layer, const DataHandle data) {
    CORRADE_ASSERT(data != DataHandle::Null,
        "Ui::HexadecimalFormatter::attach(): invalid handle" << data, *this);
    CORRADE_ASSERT(layer.isHandleValid(data),
        "Ui::HexadecimalFormatter::attach():" << data << "not valid in" << layer.handle(), *this);
    _layer = &layer;
    _data = dataHandleData(data);
    return *this;
}

HexadecimalFormatter& HexadecimalFormatter::attach(TextLayer& layer, const LayerDataHandle data) {
    CORRADE_ASSERT(data != LayerDataHandle::Null,
        "Ui::HexadecimalFormatter::attach(): invalid handle" << data, *this);
    CORRADE_ASSERT(layer.isHandleValid(data),
        "Ui::HexadecimalFormatter::attach():" << data << "not valid in" << layer.handle(), *this);
    _layer = &layer;
    _data = data;
    return *this;
}

FloatFormatter& FloatFormatter::attach(TextLayer& layer, const DataHandle data) {
    CORRADE_ASSERT(data != DataHandle::Null,
        "Ui::FloatFormatter::attach(): invalid handle" << data, *this);
    CORRADE_ASSERT(layer.isHandleValid(data),
        "Ui::FloatFormatter::attach():" << data << "not valid in" << layer.handle(), *this);
    _layer = &layer;
    _data = dataHandleData(data);
    return *this;
}

FloatFormatter& FloatFormatter::attach(TextLayer& layer, const LayerDataHandle data) {
    CORRADE_ASSERT(data != LayerDataHandle::Null,
        "Ui::FloatFormatter::attach(): invalid handle" << data, *this);
    CORRADE_ASSERT(layer.isHandleValid(data),
        "Ui::FloatFormatter::attach():" << data << "not valid in" << layer.handle(), *this);
    _layer = &layer;
    _data = data;
    return *this;
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

template<class T> inline void DecimalFormatter::format(const T value) const {
    CORRADE_ASSERT(_layer,
        "Ui::DecimalFormatter: formatter not attached", );
    CORRADE_ASSERT(_layer->isHandleValid(_data),
        "Ui::DecimalFormatter:" << dataHandle(_layer->handle(), _data) << "is no longer valid", );

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
    _layer->setText(_data, {output, size}, {});
}

void DecimalFormatter::operator()(const Int value) const {
    format(value);
}

void DecimalFormatter::operator()(const UnsignedInt value) const {
    format(value);
}

void DecimalFormatter::operator()(const Long value) const {
    format(value);
}

void DecimalFormatter::operator()(const UnsignedLong value) const {
    format(value);
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

template<class T> void HexadecimalFormatter::format(const T value) const {
    CORRADE_ASSERT(_layer,
        "Ui::HexadecimalFormatter: formatter not attached", );
    CORRADE_ASSERT(_layer->isHandleValid(_data),
        "Ui::HexadecimalFormatter:" << dataHandle(_layer->handle(), _data) << "is no longer valid", );

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
    _layer->setText(_data, {output, size}, {});
}

void HexadecimalFormatter::operator()(const Int value) const {
    format(value);
}

void HexadecimalFormatter::operator()(const UnsignedInt value) const {
    format(value);
}

void HexadecimalFormatter::operator()(const Long value) const {
    format(value);
}

void HexadecimalFormatter::operator()(const UnsignedLong value) const {
    format(value);
}

void FloatFormatter::operator()(const Float value) const {
    /** @todo stop delegating to Double once there's a dedicated code path for
        float printing (with smaller LUTs and such), printf doesn't do that */
    return operator()(Double(value));
}

void FloatFormatter::operator()(const Double value) const {
    CORRADE_ASSERT(_layer,
        "Ui::FloatFormatter: formatter not attached", );
    CORRADE_ASSERT(_layer->isHandleValid(_data),
        "Ui::FloatFormatter:" << dataHandle(_layer->handle(), _data) << "is no longer valid", );

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
    _layer->setText(_data, {output, size}, {});
}

}}
