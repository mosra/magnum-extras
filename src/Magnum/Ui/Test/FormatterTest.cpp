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

#include <Corrade/Containers/ArrayView.h> /* arraySize() */
#include <Corrade/Containers/Function.h> /* construct*(), for in-place fit */
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/String.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/String.h>
#include <Corrade/Utility/Format.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/Text/AbstractFont.h>
#include <Magnum/Text/AbstractGlyphCache.h>
#include <Magnum/Text/AbstractShaper.h>

#include "Magnum/Ui/Formatter.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/TextLayer.h"
#include "Magnum/Ui/TextProperties.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct FormatterTest: TestSuite::Tester {
    explicit FormatterTest();

    void debugParseState();
    void debugDecimalFlag();
    void debugDecimalFlags();
    void debugHexadecimalFlag();
    void debugHexadecimalFlags();
    void debugFloatFlag();
    void debugFloatFlags();

    void constructDecimal();
    void constructHexadecimal();
    void constructFloat();
    template<class T> void constructAttached();
    /* Invalid construction is only when attaching to invalid data, which is
       checked in attachInvalid() below already */
    template<class T> void constructCopy();

    template<class T> void attach();
    template<class T> void attachInvalid();

    template<class T> void flags();
    template<class T> void decimalMinWidth();
    template<class T> void decimalMinWidthInvalid();
    void floatPrecision();
    void floatPrecisionInvalid();

    void formatDecimal();
    void formatHexadecimal();
    void formatFloat();
    template<class T, class U> void formatInvalid();

    void parseDecimal();
    void parseDecimalFailed();
    void parseHexadecimal();
    void parseHexadecimalFailed();
    void parseFloat();
    void parseFloatFailed();
    void parseInvalid();

    private:
        struct: Text::AbstractFont {
            Text::FontFeatures doFeatures() const override { return {}; }
            bool doIsOpened() const override { return true; }
            void doClose() override {}

            void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
            Vector2 doGlyphSize(UnsignedInt) override { return {}; }
            Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
            Containers::Pointer<Text::AbstractShaper> doCreateShaper() override {
                struct EmptyShaper: Text::AbstractShaper {
                    using Text::AbstractShaper::AbstractShaper;

                    UnsignedInt doShape(Containers::StringView, UnsignedInt, UnsignedInt, Containers::ArrayView<const Text::FeatureRange>) override {
                        return 0;
                    }
                    void doGlyphIdsInto(const Containers::StridedArrayView1D<UnsignedInt>&) const override {}
                    void doGlyphOffsetsAdvancesInto(const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&) const override {}
                    void doGlyphClustersInto(const Containers::StridedArrayView1D<UnsignedInt>&) const override {}
                };

                return Containers::pointer<EmptyShaper>(*this);
            }
        } _font;

        struct: Text::AbstractGlyphCache {
            using Text::AbstractGlyphCache::AbstractGlyphCache;

            Text::GlyphCacheFeatures doFeatures() const override { return {}; }
            void doSetImage(const Vector2i&, const ImageView2D&) override {}
        } _cache{PixelFormat::R8Unorm, {32, 32, 2}};

        struct LayerShared: TextLayer::Shared {
            explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

            void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
            void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
        } _shared{_cache, TextLayer::Shared::Configuration{1}};
};

using namespace Containers::Literals;

const struct {
    TestSuite::TestCaseDescriptionSourceLocation name;
    DecimalFormatter::Flags flags;
    Containers::Optional<Int> minWidth;
    /* Only one or the other should be non-zero. If valueUnsigned is 0 and the
       value is positive, both signed and unsigned variants are tested,
       otherwise the other variant is expected to produce an incorrect result.
       If value / valueUnsigned fits into 32 bits, both 32- and 64-bit variants
       are tested, otherwise the 32-bit value is expected to produce an
       incorrect result. */
    Long value;
    UnsignedLong valueUnsigned;
    const char* expected;
} FormatDecimalData[]{
    {"", {}, {},
        1337, 0,
        "1337"},
    {"explicit plus",
        DecimalFormatter::Flag::ExplicitPlus, {},
        +420, 0,
        "+420"},
    {"negative", {}, {},
        -1256, 0,
        "-1256"},
    /* This should cause no change compared to above */
    {"negative, explicit plus",
        DecimalFormatter::Flag::ExplicitPlus, {},
        -1256, 0,
        "-1256"},
    {"largest 32-bit signed value", {}, {},
        0x7fffffff, 0,
        "2147483647"},
    {"smallest 32-bit signed value", {}, {},
        /* C, you're stupid, DO YOU HEAR ME?! */
        -0x7fffffff - 1, 0,
        "-2147483648"},
    {"largest 32-bit unsigned value", {}, {},
        0xffffffff, 0,
        "4294967295"},
    {"largest 64-bit signed value", {}, {},
        0x7fffffffffffffff, 0,
        "9223372036854775807"},
    {"smallest 64-bit signed value", {}, {},
        /* C, you're stupid, DO YOU HEAR ME?! */
        -0x7fffffffffffffff - 1, 0,
        "-9223372036854775808"},
    {"largest 64-bit unsigned value", {}, {},
        0, 0xffffffffffffffff,
        "18446744073709551615"},

    {"min width", {}, 10,
        1337, 0,
      /* 1234567890 */
        "0000001337"},
    {"min width, negative", {}, 10,
        -42069, 0,
       /* 1234567890 */
        "-0000042069"},
    {"min width less than digit count", {}, 5,
        23245679, 0,
      /* 12345 */
        "23245679"},
    {"max min width", {}, 255,
        1337, 0,
      /* 123456789012345678901234567890123456789012345678901234567890   60 */
        "000000000000000000000000000000000000000000000000000000000000"
      /* 123456789012345678901234567890123456789012345678901234567890   120 */
        "000000000000000000000000000000000000000000000000000000000000"
      /* 123456789012345678901234567890123456789012345678901234567890   180 */
        "000000000000000000000000000000000000000000000000000000000000"
      /* 123456789012345678901234567890123456789012345678901234567890   240 */
        "000000000000000000000000000000000000000000000000000000000000"
      /* 123456789012345                                                255 */
        "000000000001337"},
    {"max min width, negative", {}, 255,
        -42069, 0,
      /* 123456789012345678901234567890123456789012345678901234567890   60 */
       "-000000000000000000000000000000000000000000000000000000000000"
      /* 123456789012345678901234567890123456789012345678901234567890   120 */
        "000000000000000000000000000000000000000000000000000000000000"
      /* 123456789012345678901234567890123456789012345678901234567890   180 */
        "000000000000000000000000000000000000000000000000000000000000"
      /* 123456789012345678901234567890123456789012345678901234567890   240 */
        "000000000000000000000000000000000000000000000000000000000000"
      /* 123456789012345                                                255 */
        "000000000042069"},
    {"max min width, explicit plus",
        DecimalFormatter::Flag::ExplicitPlus, 255,
        +42069, 0,
      /* 123456789012345678901234567890123456789012345678901234567890   60 */
       "+000000000000000000000000000000000000000000000000000000000000"
      /* 123456789012345678901234567890123456789012345678901234567890   120 */
        "000000000000000000000000000000000000000000000000000000000000"
      /* 123456789012345678901234567890123456789012345678901234567890   180 */
        "000000000000000000000000000000000000000000000000000000000000"
      /* 123456789012345678901234567890123456789012345678901234567890   240 */
        "000000000000000000000000000000000000000000000000000000000000"
      /* 123456789012345                                                255 */
        "000000000042069"},

    {"zero", {}, {},
        0, 0,
        "0"},
    {"zero, explicit plus",
        DecimalFormatter::Flag::ExplicitPlus, {},
        0, 0,
        "+0"},
    {"zero, min width", {}, 10,
        0, 0,
      /* 1234567890 */
        "0000000000"},
    /* This should format nothing at all */
    {"zero, zero min width", {}, 0,
        0, 0,
        ""},
    /* This should also format nothing at all */
    {"zero, zero min width, explicit plus",
        DecimalFormatter::Flag::ExplicitPlus, 0,
        0, 0,
        ""},
};

const struct {
    TestSuite::TestCaseDescriptionSourceLocation name;
    HexadecimalFormatter::Flags flags;
    Containers::Optional<Int> minWidth;
    /* Only one or the other should be non-zero. If valueUnsigned is 0 and the
       value is positive, both signed and unsigned variants are tested,
       otherwise the other variant is expected to produce an incorrect result.
       If value / valueUnsigned fits into 32 bits, both 32- and 64-bit variants
       are tested, otherwise the 32-bit value is expected to produce an
       incorrect result. */
    Long value;
    UnsignedLong valueUnsigned;
    const char* expected;
} FormatHexadecimalData[]{
    {"", {}, {},
        0x1337cafe, 0,
        "1337cafe"},
    {"explicit plus",
        HexadecimalFormatter::Flag::ExplicitPlus, {},
        +0x420babe, 0,
        "+420babe"},
    {"uppercase",
        HexadecimalFormatter::Flag::Uppercase, {},
        0x1337cafe, 0,
        "1337CAFE"},
    {"base prefix",
        HexadecimalFormatter::Flag::BasePrefix, {},
        0xc0ffee, 0,
        "0xc0ffee"},
    /* The X is uppercased as well */
    {"base prefix, uppercase",
        HexadecimalFormatter::Flag::BasePrefix|HexadecimalFormatter::Flag::Uppercase, {},
        0xc0ffee, 0,
        "0XC0FFEE"},
    {"hash prefix",
        HexadecimalFormatter::Flag::HashPrefix, {},
        0x3bd267, 0,
        "#3bd267"},
    /* The hash isn't affected by uppercase */
    {"hash prefix, uppercase",
        HexadecimalFormatter::Flag::HashPrefix|HexadecimalFormatter::Flag::Uppercase, {},
        0x3bd267, 0,
        "#3BD267"},
    /* Hash prefix has a precedence over base prefix, so no change compared to
       above */
    {"base + hash prefix",
        HexadecimalFormatter::Flag::BasePrefix|HexadecimalFormatter::Flag::HashPrefix, {},
        0x3bd267, 0,
        "#3bd267"},
    {"negative", {}, {},
        -0xb0d1e5, 0,
        "-b0d1e5"},
    /* This should cause no change compared to above */
    {"negative, explicit plus",
        HexadecimalFormatter::Flag::ExplicitPlus, {},
        -0xb0d1e5, 0,
        "-b0d1e5"},
    {"negative, base prefix",
        HexadecimalFormatter::Flag::BasePrefix, {},
        -0xdead00d, 0,
        "-0xdead00d"},
    /* This should cause no change compared to above */
    {"negative, base prefix, explicit plus",
        HexadecimalFormatter::Flag::BasePrefix|HexadecimalFormatter::Flag::ExplicitPlus, {},
        -0xdead00d, 0,
        "-0xdead00d"},
    {"negative, hash prefix",
        HexadecimalFormatter::Flag::HashPrefix, {},
        -0xbadf00d, 0,
        "-#badf00d"},
    /* This should cause no change compared to above */
    {"negative, hash prefix, explicit plus",
        HexadecimalFormatter::Flag::HashPrefix|HexadecimalFormatter::Flag::ExplicitPlus, {},
        -0xbadf00d, 0,
        "-#badf00d"},

    {"largest 32-bit signed value", {}, {},
        0x7fffffff, 0,
        "7fffffff"},
    {"smallest 32-bit signed value", {}, {},
        /* C, you're stupid, DO YOU HEAR ME?! */
        -0x7fffffff - 1, 0,
        "-80000000"},
    {"largest 32-bit unsigned value", {}, {},
        0xffffffff, 0,
        "ffffffff"},
    {"largest 64-bit signed value", {}, {},
        0x7fffffffffffffff, 0,
        "7fffffffffffffff"},
    {"smallest 64-bit signed value", {}, {},
        /* C, you're stupid, DO YOU HEAR ME?! */
        -0x7fffffffffffffff - 1, 0,
        "-8000000000000000"},
    {"largest 64-bit unsigned value", {}, {},
        0, 0xffffffffffffffff,
        "ffffffffffffffff"},

    {"min width", {}, 10,
        0x2beef5, 0,
      /* 1234567890 */
        "00002beef5"},
    {"min width, negative", {}, 10,
        -0xdeaf, 0,
       /* 1234567890 */
        "-000000deaf"},
    {"min width, base prefix",
        HexadecimalFormatter::Flag::BasePrefix, 10,
        0x2beef5, 0,
        /* 1234567890 */
        "0x00002beef5"},
    {"min width, base prefix, negative",
        HexadecimalFormatter::Flag::BasePrefix, 10,
        -0xfaf0, 0,
         /* 1234567890 */
        "-0x000000faf0"},
    {"min width, hash prefix",
        HexadecimalFormatter::Flag::HashPrefix, 10,
        0x2beef5, 0,
        /*1234567890 */
        "#00002beef5"},
    {"min width less than digit count", {}, 5,
        0xcafebabe, 0,
      /* 12345 */
        "cafebabe"},
    {"max min width", {}, 255,
        0xdeafbed, 0,
      /* 123456789012345678901234567890123456789012345678901234567890   60 */
        "000000000000000000000000000000000000000000000000000000000000"
      /* 123456789012345678901234567890123456789012345678901234567890   120 */
        "000000000000000000000000000000000000000000000000000000000000"
      /* 123456789012345678901234567890123456789012345678901234567890   180 */
        "000000000000000000000000000000000000000000000000000000000000"
      /* 123456789012345678901234567890123456789012345678901234567890   240 */
        "000000000000000000000000000000000000000000000000000000000000"
      /* 123456789012345                                                255 */
        "00000000deafbed"},
    {"max min width, negative, base prefix",
        HexadecimalFormatter::Flag::BasePrefix, 255,
        -0xf0fa, 0,
      /* 123456789012345678901234567890123456789012345678901234567890   60 */
     "-0x000000000000000000000000000000000000000000000000000000000000"
      /* 123456789012345678901234567890123456789012345678901234567890   120 */
        "000000000000000000000000000000000000000000000000000000000000"
      /* 123456789012345678901234567890123456789012345678901234567890   180 */
        "000000000000000000000000000000000000000000000000000000000000"
      /* 123456789012345678901234567890123456789012345678901234567890   240 */
        "000000000000000000000000000000000000000000000000000000000000"
      /* 123456789012345                                                255 */
        "00000000000f0fa"},
    {"max min width, explicit plus, base prefix",
        HexadecimalFormatter::Flag::BasePrefix|HexadecimalFormatter::Flag::ExplicitPlus, 255,
        +0xcaffe, 0,
      /* 123456789012345678901234567890123456789012345678901234567890   60 */
     "+0x000000000000000000000000000000000000000000000000000000000000"
      /* 123456789012345678901234567890123456789012345678901234567890   120 */
        "000000000000000000000000000000000000000000000000000000000000"
      /* 123456789012345678901234567890123456789012345678901234567890   180 */
        "000000000000000000000000000000000000000000000000000000000000"
      /* 123456789012345678901234567890123456789012345678901234567890   240 */
        "000000000000000000000000000000000000000000000000000000000000"
      /* 123456789012345                                                255 */
        "0000000000caffe"},

    {"zero", {}, {},
        0, 0,
        "0"},
    {"zero, explicit plus",
        HexadecimalFormatter::Flag::ExplicitPlus, {},
        0, 0,
        "+0"},
    {"zero, min width", {}, 10,
        0, 0,
      /* 1234567890 */
        "0000000000"},
    /* This should format nothing at all */
    {"zero, zero min width", {}, 0,
        0, 0,
        ""},
    /* These should also format nothing at all */
    {"zero, zero min width, explicit plus",
        HexadecimalFormatter::Flag::ExplicitPlus, 0,
        0, 0,
        ""},
    {"zero, zero min width, base prefix",
        HexadecimalFormatter::Flag::BasePrefix, 0,
        0, 0,
        ""},
    {"zero, zero min width, hash prefix",
        HexadecimalFormatter::Flag::HashPrefix, 0,
        0, 0,
        ""},
    {"zero, zero min width, base + hash prefix, explicit plus",
        HexadecimalFormatter::Flag::BasePrefix|HexadecimalFormatter::Flag::HashPrefix|HexadecimalFormatter::Flag::ExplicitPlus, 0,
        0, 0,
        ""},
};

/* Yeah, sure, undefined behavior and all. Do I care? No. */
union FloatFromBits {
    explicit FloatFromBits(UnsignedInt bits): bits{bits} {}
    UnsignedInt bits;
    Float value;
};
union DoubleFromBits {
    explicit DoubleFromBits(UnsignedLong bits): bits{bits} {}
    UnsignedLong bits;
    Double value;
};

const struct {
    const char* name;
    FloatFormatter::Flags flags;
    Containers::Optional<Int> precision;
    /* Only one or the other should be non-zero. If valueDouble is zero, both
       float and double variants are tested, otherwise the float variant is
       expected to produce an incorrect result. */
    Float value;
    Double valueDouble;
    const char* expected;
    const char* xfail;
} FormatFloatData[]{
    {"", {}, {},
        13.37f, 0.0,
        "13.37", nullptr},
    {"explicit plus",
        FloatFormatter::Flag::ExplicitPlus, {},
        +3.1415f, 0.0,
        "+3.1415", nullptr},
    {"negative", {}, {},
        -12.26f, 0.0,
        "-12.26", nullptr},
    /* This should cause no change compared to above */
    {"negative, explicit plus",
        FloatFormatter::Flag::ExplicitPlus, {},
        -12.26f, 0.0,
        "-12.26", nullptr},

    {"small value, default", {}, {},
        42.069f, 0.0,
        "42.069", nullptr},
    /* Nothing to uppercase here */
    {"small value, default, uppercase",
        FloatFormatter::Flag::Uppercase, {},
        42.069f, 0.0,
        "42.069", nullptr},
    /* Should be the same as default */
    {"small value, decimal + exponent",
        FloatFormatter::Flag::Decimal|FloatFormatter::Flag::Exponent, {},
        42.069f, 0.0,
        "42.069", nullptr},
    {"small value, default, smaller precision", {}, 4,
        42.069f, 0.0,
        "42.07", nullptr},
    {"small value, default, even smaller precision", {}, 2,
        42.069f, 0.0,
        "42", nullptr},
    {"small value, decimal",
        FloatFormatter::Flag::Decimal, {},
        42.069f, 0.0,
        "42.069000", nullptr},
    {"small value, decimal, snaller precision",
        FloatFormatter::Flag::Decimal, 2,
        42.069f, 0.0,
        "42.07", nullptr},
    {"small value, exponent",
        FloatFormatter::Flag::Exponent, {},
        42.069f, 0.0,
        /* MinGW prints the exponent with three decimals always */
        #ifdef CORRADE_TARGET_MINGW
        "4.206900e+001",
        #else
        "4.206900e+01",
        #endif
        nullptr},
    {"small value, exponent, uppercase",
        FloatFormatter::Flag::Exponent|FloatFormatter::Flag::Uppercase, {},
        42.069f, 0.0,
        /* MinGW prints the exponent with three decimals always */
        #ifdef CORRADE_TARGET_MINGW
        "4.206900E+001",
        #else
        "4.206900E+01",
        #endif
        nullptr},
    {"small value, exponent, smaller precision",
        FloatFormatter::Flag::Exponent, 3,
        42.069f, 0.0,
        /* MinGW prints the exponent with three decimals always */
        #ifdef CORRADE_TARGET_MINGW
        "4.207e+001",
        #else
        "4.207e+01",
        #endif
        nullptr},

    {"large value, default", {}, {},
        1.25e10f, 0.0,
        /* MinGW prints the exponent with three decimals always */
        #ifdef CORRADE_TARGET_MINGW
        "1.25e+010",
        #else
        "1.25e+10",
        #endif
        nullptr},
    {"large value, default, uppercase",
        FloatFormatter::Flag::Uppercase, {},
        1.25e10f, 0.0,
        /* MinGW prints the exponent with three decimals always */
        #ifdef CORRADE_TARGET_MINGW
        "1.25E+010",
        #else
        "1.25E+10",
        #endif
        nullptr},
    {"large value, decimal",
        FloatFormatter::Flag::Decimal, {},
        /* Float causes a roundoff error here */
        0.0f, 1.25e10,
        "12500000000.000000", nullptr},
    /* This should cause no change compared to above */
    {"large value, decimal, uppercase",
        FloatFormatter::Flag::Decimal|FloatFormatter::Flag::Uppercase, {},
        0.0f, 1.25e10,
        "12500000000.000000", nullptr},
    {"large value, exponent",
        FloatFormatter::Flag::Exponent, {},
        1.25e10f, 0.0,
        /* MinGW prints the exponent with three decimals always */
        #ifdef CORRADE_TARGET_MINGW
        "1.250000e+010",
        #else
        "1.250000e+10",
        #endif
        nullptr},
    {"large value, exponent, uppercase",
        FloatFormatter::Flag::Exponent|FloatFormatter::Flag::Uppercase, {},
        1.25e10f, 0.0,
        /* MinGW prints the exponent with three decimals always */
        #ifdef CORRADE_TARGET_MINGW
        "1.250000E+010",
        #else
        "1.250000E+10",
        #endif
        nullptr},
    {"large double value, exponent",
        FloatFormatter::Flag::Exponent, {},
        0.0f, 1.25e100,
        "1.250000e+100", nullptr},
    {"large double value, exponent, uppercase",
        FloatFormatter::Flag::Exponent|FloatFormatter::Flag::Uppercase, {},
        0.0f, 1.25e100,
        "1.250000E+100", nullptr},

    {"largest 32-bit value, decimal, zero precision",
        FloatFormatter::Flag::Decimal, 0,
        /* https://en.wikipedia.org/wiki/Single-precision_floating-point_format */
        FloatFromBits{0x7f7fffffu}.value, 0.0,
        "340282346638528859811704183484516925440", nullptr},
    {"smallest 32-bit value, decimal, max precision",
        FloatFormatter::Flag::Decimal, 255,
        /* Like above, but flipping the highest sign bit */
        FloatFromBits{0xff7fffffu}.value, 0.0,
        "-340282346638528859811704183484516925440."
      /* 123456789012345678901234567890123456789012345678901234567890   60 */
        "000000000000000000000000000000000000000000000000000000000000"
      /* 123456789012345678901234567890123456789012345678901234567890   120 */
        "000000000000000000000000000000000000000000000000000000000000"
      /* 123456789012345678901234567890123456789012345678901234567890   180 */
        "000000000000000000000000000000000000000000000000000000000000"
      /* 123456789012345678901234567890123456789012345678901234567890   240 */
        "000000000000000000000000000000000000000000000000000000000000"
      /* 123456789012345                                                255 */
        "000000000000000", nullptr},
    {"largest 64-bit value, decimal, zero precision",
        FloatFormatter::Flag::Decimal, 0,
        /* https://en.wikipedia.org/wiki/Double-precision_floating-point_format */
        0.0f, DoubleFromBits{0x7fefffffffffffffull}.value,
        "179769313486231570814527423731704356798070567525844996598917476803157260780028538760589558632766878171540458953514382464234321326889464182768467546703537516986049910576551282076245490090389328944075868508455133942304583236903222948165808559332123348274797826204144723168738177180919299881250404026184124858368", nullptr},
    {"largest 64-bit value, decimal, max precision, explicit plus",
        FloatFormatter::Flag::Decimal|FloatFormatter::Flag::ExplicitPlus, 255,
        0.0f, DoubleFromBits{0x7fefffffffffffffull}.value,
        "+179769313486231570814527423731704356798070567525844996598917476803157260780028538760589558632766878171540458953514382464234321326889464182768467546703537516986049910576551282076245490090389328944075868508455133942304583236903222948165808559332123348274797826204144723168738177180919299881250404026184124858368."
      /* 123456789012345678901234567890123456789012345678901234567890   60 */
        "000000000000000000000000000000000000000000000000000000000000"
      /* 123456789012345678901234567890123456789012345678901234567890   120 */
        "000000000000000000000000000000000000000000000000000000000000"
      /* 123456789012345678901234567890123456789012345678901234567890   180 */
        "000000000000000000000000000000000000000000000000000000000000"
      /* 123456789012345678901234567890123456789012345678901234567890   240 */
        "000000000000000000000000000000000000000000000000000000000000"
      /* 123456789012345                                                255 */
        "000000000000000", nullptr},

    {"zero", {}, {},
        0.0f, 0.0,
        "0", nullptr},
    {"zero, explicit plus",
        FloatFormatter::Flag::ExplicitPlus, {},
        0.0f, 0.0,
        "+0", nullptr},
    {"negative zero", {}, {},
        -0.0f, 0.0,
        "-0", nullptr},
    /* Should have no difference compared to above */
    {"negative zero, explicit plus",
        FloatFormatter::Flag::ExplicitPlus, {},
        -0.0f, 0.0,
        "-0", nullptr},
    /* Interestingly enough, here zero precision doesn't cause the output to
       be empty */
    /** @todo make this consistent with ints? */
    {"zero, zero precision",
        {}, 0,
        0.0f, 0.0,
        "0", nullptr},
    {"zero, zero precision, explicit plus",
        FloatFormatter::Flag::ExplicitPlus, 0,
        0.0f, 0.0,
        "+0", nullptr},
    {"zero, decimal, smaller precision",
        FloatFormatter::Flag::Decimal, 1,
        0.0f, 0.0,
        "0.0", nullptr},
    {"zero, decimal, zero precision",
        FloatFormatter::Flag::Decimal, 0,
        0.0f, 0.0,
        "0", nullptr},

    {"infinity", {}, {},
        Constants::inf(), 0.0,
        "inf", nullptr},
    {"infinity, uppercase",
        FloatFormatter::Flag::Uppercase, {},
        Constants::inf(), 0.0,
        "INF", nullptr},
    {"infinity, decimal",
        FloatFormatter::Flag::Decimal, {},
        Constants::inf(), 0.0,
        "inf", nullptr},
    {"infinity, decimal, uppercase",
        FloatFormatter::Flag::Decimal|FloatFormatter::Flag::Uppercase, {},
        Constants::inf(), 0.0,
        "INF", nullptr},
    {"infinity, exponent",
        FloatFormatter::Flag::Exponent, {},
        Constants::inf(), 0.0,
        "inf", nullptr},
    {"infinity, exponent, uppercase",
        FloatFormatter::Flag::Exponent|FloatFormatter::Flag::Uppercase, {},
        Constants::inf(), 0.0,
        "INF", nullptr},
    {"infinity, explicit plus",
        FloatFormatter::Flag::ExplicitPlus, {},
        Constants::inf(), 0.0,
        "+inf", nullptr},
    {"negative infinity", {}, {},
        -Constants::inf(), 0.0,
        "-inf", nullptr},
    /* This should cause no change compared to above */
    {"negative infinity, explicit plus", {}, {},
        -Constants::inf(), 0.0,
        "-inf", nullptr},

    /** @todo effing hell, do my own float printing already, this is insane */
    {"NaN", {}, {},
        Constants::nan(), 0.0,
        /* MSVC before version 2019 16.10(11?) prints -nan(ind) instead of
           ±nan. Check matching DebugTools::CompareImage tests. */
        #if defined(CORRADE_TARGET_MSVC) && _MSC_VER < 1929 && !defined(CORRADE_TARGET_CLANG_CL)
        "-nan(ind)",
        #else
        "nan",
        #endif
        nullptr},
    {"NaN, uppercase",
        FloatFormatter::Flag::Uppercase, {},
        Constants::nan(), 0.0,
        #if defined(CORRADE_TARGET_MSVC) && _MSC_VER < 1929 && !defined(CORRADE_TARGET_CLANG_CL)
        "-NAN(IND)",
        #else
        "NAN",
        #endif
        nullptr},
    {"NaN, decimal",
        FloatFormatter::Flag::Decimal, {},
        Constants::nan(), 0.0,
        /* MSVC before version 2019 16.10(11?) prints -nan(ind) instead of
           ±nan. Check matching DebugTools::CompareImage tests. */
        #if defined(CORRADE_TARGET_MSVC) && _MSC_VER < 1929 && !defined(CORRADE_TARGET_CLANG_CL)
        "-nan(ind)",
        #else
        "nan",
        #endif
        nullptr},
    {"NaN, decimal, uppercase",
        FloatFormatter::Flag::Decimal|FloatFormatter::Flag::Uppercase, {},
        Constants::nan(), 0.0,
        #if defined(CORRADE_TARGET_MSVC) && _MSC_VER < 1929 && !defined(CORRADE_TARGET_CLANG_CL)
        "-NAN(IND)",
        #else
        "NAN",
        #endif
        nullptr},
    {"NaN, exponent",
        FloatFormatter::Flag::Exponent, {},
        Constants::nan(), 0.0,
        #if defined(CORRADE_TARGET_MSVC) && _MSC_VER < 1929 && !defined(CORRADE_TARGET_CLANG_CL)
        "-nan(ind)",
        #else
        "nan",
        #endif
        nullptr},
    {"NaN, exponent, uppercase",
        FloatFormatter::Flag::Exponent|FloatFormatter::Flag::Uppercase, {},
        Constants::nan(), 0.0,
        #if defined(CORRADE_TARGET_MSVC) && _MSC_VER < 1929 && !defined(CORRADE_TARGET_CLANG_CL)
        "-NAN(IND)",
        #else
        "NAN",
        #endif
        nullptr},
    {"NaN, explicit plus",
        FloatFormatter::Flag::ExplicitPlus, {},
        Constants::nan(), 0.0,
        #if defined(CORRADE_TARGET_MSVC) && _MSC_VER < 1929 && !defined(CORRADE_TARGET_CLANG_CL)
        "-nan(ind)",
        #else
        "+nan",
        #endif
        #ifdef CORRADE_TARGET_APPLE
        "On macOS the NaN sign is never printed."
        #else
        nullptr
        #endif
        },
    {"negative NaN", {}, {},
        -Constants::nan(), 0.0,
        /* MSVC adds (ind), but only after negative NaN. Interestingly, the
           behavior for MSVC before version 2019 16.10(11?) is inverted here,
           printing just "nan" without a sign. */
        #if defined(CORRADE_TARGET_MSVC) && _MSC_VER < 1929 && !defined(CORRADE_TARGET_CLANG_CL)
        "nan",
        #elif defined(CORRADE_TARGET_MSVC)
        "-nan(ind)",
        #else
        "-nan",
        #endif
        #if defined(CORRADE_TARGET_APPLE) || defined(CORRADE_TARGET_MINGW)
        "On macOS the NaN sign is never printed, MinGW never prints it if negative."
        #else
        nullptr
        #endif
        },
    /* This should cause no change compared to above */
    {"negative NaN, explicit plus", {}, {},
        -Constants::nan(), 0.0,
        #if defined(CORRADE_TARGET_MSVC) && _MSC_VER < 1929 && !defined(CORRADE_TARGET_CLANG_CL)
        "nan",
        #elif defined(CORRADE_TARGET_MSVC)
        "-nan(ind)",
        #else
        "-nan",
        #endif
        #if defined(CORRADE_TARGET_APPLE) || defined(CORRADE_TARGET_MINGW)
        "On macOS the NaN sign is never printed, MinGW never prints it if negative."
        #else
        nullptr
        #endif
        },
};

const struct {
    TestSuite::TestCaseDescriptionSourceLocation name;
    Containers::StringView text;
    /* Enumerating all four cases because anything else would make the test
       code too cryptic and prone to accidentally testing the wrong thing or
       not testing certain cases at all */
    ParseState expectedState;
    Int expected;
    ParseState expectedStateUnsigned;
    UnsignedInt expectedUnsigned;
    ParseState expectedStateLong;
    Long expectedLong;
    ParseState expectedStateUnsignedLong;
    UnsignedLong expectedUnsignedLong;
} ParseDecimalData[]{
    {"zero", "0",
        ParseState::Success, 0,
        ParseState::Success, 0,
        ParseState::Success, 0,
        ParseState::Success, 0},
    {"very many zeros",
        "000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000",
        ParseState::Success, 0,
        ParseState::Success, 0,
        ParseState::Success, 0,
        ParseState::Success, 0},
    {"positive zero", "+0",
        ParseState::Success, 0,
        ParseState::Success, 0,
        ParseState::Success, 0,
        ParseState::Success, 0},
    {"negative zero", "-0",
        ParseState::Success, 0,
        ParseState::Failed, 0,
        ParseState::Success, 0,
        ParseState::Failed, 0},
    {"value", "1337420",
        ParseState::Success, 1337420,
        ParseState::Success, 1337420,
        ParseState::Success, 1337420,
        ParseState::Success, 1337420},
    {"value, leading zeros", "000000000000001337420",
        ParseState::Success, 1337420,
        ParseState::Success, 1337420,
        ParseState::Success, 1337420,
        ParseState::Success, 1337420},
    {"positive value", "+1337420",
        ParseState::Success, 1337420,
        ParseState::Success, 1337420,
        ParseState::Success, 1337420,
        ParseState::Success, 1337420},
    {"negative value", "-1337420",
        ParseState::Success, -1337420,
        ParseState::Failed, 0,
        ParseState::Success, -1337420,
        ParseState::Failed, 0},

    {"leading whitespace", " \t \v\n\r  \f\f1337420",
        ParseState::Success, 1337420,
        ParseState::Success, 1337420,
        ParseState::Success, 1337420,
        ParseState::Success, 1337420},
    {"trailing whitespace", "1337420 \t\t \v\f\f  \r\n",
        ParseState::Success, 1337420,
        ParseState::Success, 1337420,
        ParseState::Success, 1337420,
        ParseState::Success, 1337420},
    {"leading and trailing whitespace", "\t \v\f  1337420 \t\f  \r\n",
        ParseState::Success, 1337420,
        ParseState::Success, 1337420,
        ParseState::Success, 1337420,
        ParseState::Success, 1337420},
    {"leading and trailing whitespace, positive value", "\t \v\f  +1337420 \t\f  \r\n",
        ParseState::Success, 1337420,
        ParseState::Success, 1337420,
        ParseState::Success, 1337420,
        ParseState::Success, 1337420},
    {"leading and trailing whitespace, negative value", "\t \v\f  -1337420 \t\f  \r\n",
        ParseState::Success, -1337420,
        ParseState::Failed, 0,
        ParseState::Success, -1337420,
        ParseState::Failed, 0},

    /* Value clamping. Comparing to TypeTraits values to make sure the text
       doesn't contain off-by-one values and such. */
    {"largest 32-bit signed value", "2147483647",
        ParseState::Success, Math::TypeTraits<Int>::max(),
        ParseState::Success, Math::TypeTraits<Int>::max(),
        ParseState::Success, Math::TypeTraits<Int>::max(),
        ParseState::Success, Math::TypeTraits<Int>::max()},
    {"largest 32-bit signed value plus one", "2147483648",
        ParseState::Clamped, Math::TypeTraits<Int>::max(),
        ParseState::Success, Math::TypeTraits<Int>::max() + 1u,
        ParseState::Success, Math::TypeTraits<Int>::max() + 1ll,
        ParseState::Success, Math::TypeTraits<Int>::max() + 1ull},
    {"smallest 32-bit signed value", "-2147483648",
        ParseState::Success, Math::TypeTraits<Int>::min(),
        ParseState::Failed, 0,
        ParseState::Success, Math::TypeTraits<Int>::min(),
        ParseState::Failed, 0},
    {"smallest 32-bit signed value minus one", "-2147483649",
        ParseState::Clamped, Math::TypeTraits<Int>::min(),
        ParseState::Failed, 0,
        ParseState::Success, Math::TypeTraits<Int>::min() -1ll,
        ParseState::Failed, 0},
    {"largest 32-bit unsigned value", "4294967295",
        ParseState::Clamped, Math::TypeTraits<Int>::max(),
        ParseState::Success, Math::TypeTraits<UnsignedInt>::max(),
        ParseState::Success, Math::TypeTraits<UnsignedInt>::max(),
        ParseState::Success, Math::TypeTraits<UnsignedInt>::max()},
    {"largest 32-bit unsigned value plus one", "4294967296",
        ParseState::Clamped, Math::TypeTraits<Int>::max(),
        ParseState::Clamped, Math::TypeTraits<UnsignedInt>::max(),
        ParseState::Success, Math::TypeTraits<UnsignedInt>::max() + 1ll,
        ParseState::Success, Math::TypeTraits<UnsignedInt>::max() + 1ull},

    {"largest 64-bit signed value", "9223372036854775807",
        ParseState::Clamped, Math::TypeTraits<Int>::max(),
        ParseState::Clamped, Math::TypeTraits<UnsignedInt>::max(),
        ParseState::Success, Math::TypeTraits<Long>::max(),
        ParseState::Success, Math::TypeTraits<Long>::max()},
    {"largest 64-bit signed value plus one", "9223372036854775808",
        ParseState::Clamped, Math::TypeTraits<Int>::max(),
        ParseState::Clamped, Math::TypeTraits<UnsignedInt>::max(),
        ParseState::Clamped, Math::TypeTraits<Long>::max(),
        ParseState::Success, Math::TypeTraits<Long>::max() + 1ull},
    {"smallest 64-bit signed value", "-9223372036854775808",
        ParseState::Clamped, Math::TypeTraits<Int>::min(),
        ParseState::Failed, 0,
        ParseState::Success, Math::TypeTraits<Long>::min(),
        ParseState::Failed, 0},
    {"smallest 64-bit signed value minus one", "-9223372036854775809",
        ParseState::Clamped, Math::TypeTraits<Int>::min(),
        ParseState::Failed, 0,
        ParseState::Clamped, Math::TypeTraits<Long>::min(),
        ParseState::Failed, 0},
    {"largest 64-bit unsigned value", "18446744073709551615",
        ParseState::Clamped, Math::TypeTraits<Int>::max(),
        ParseState::Clamped, Math::TypeTraits<UnsignedInt>::max(),
        ParseState::Clamped, Math::TypeTraits<Long>::max(),
        ParseState::Success, Math::TypeTraits<UnsignedLong>::max()},
    {"largest 64-bit unsigned value plus one", "18446744073709551616",
        ParseState::Clamped, Math::TypeTraits<Int>::max(),
        ParseState::Clamped, Math::TypeTraits<UnsignedInt>::max(),
        ParseState::Clamped, Math::TypeTraits<Long>::max(),
        ParseState::Clamped, Math::TypeTraits<UnsignedLong>::max()},
};

const struct {
    TestSuite::TestCaseDescriptionSourceLocation name;
    Containers::StringView text;
    /** @todo expand with further diagnostic (cursor position and such) */
} ParseDecimalFailedData[]{
    {"empty string", ""},
    {"just spaces alone", " \t\t \v\f\f  \r\n"},
    {"invalid character at the begin", "_42069"},
    {"invalid character in the middle", "420#69"},
    {"invalid character at the end", "42069?"},
    /* This would pass if the string length wouldn't be correctly propagated
       all the way. Have to split in two literals because FUCKING C understands
       that as octal 06, ugh. */
    {"null terminator in the middle", "420\0" "69"_s},
    {"duplicated minus sign", "--1337"},
    {"duplicated plus sign", "++1337"},
    {"plus and minus sign", "+-1337"},
    {"minus and plus sign", "+-1337"},

    /* Cases that currently fail but maybe eventually shouldn't? */
    {"space in the middle", "420 69"},
    {"space after a plus sign", "+ 4201337"},
    {"space after a minus sign", "- 4201337"},
    /* This definitely shouldn't fail, sigh. Compare the vertical alignment of
       the two: −- */
    {"Unicode minus sign", "−42069"},
};

const struct {
    TestSuite::TestCaseDescriptionSourceLocation name;
    Containers::StringView text;
    /* Enumerating all four cases because anything else would make the test
       code too cryptic and prone to accidentally testing the wrong thing or
       not testing certain cases at all */
    ParseState expectedState;
    Int expected;
    ParseState expectedStateUnsigned;
    UnsignedInt expectedUnsigned;
    ParseState expectedStateLong;
    Long expectedLong;
    ParseState expectedStateUnsignedLong;
    UnsignedLong expectedUnsignedLong;
} ParseHexadecimalData[]{
    {"zero", "0",
        ParseState::Success, 0,
        ParseState::Success, 0,
        ParseState::Success, 0,
        ParseState::Success, 0},
    {"very many zeros",
        "000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000",
        ParseState::Success, 0,
        ParseState::Success, 0,
        ParseState::Success, 0,
        ParseState::Success, 0},
    {"positive zero", "+0",
        ParseState::Success, 0,
        ParseState::Success, 0,
        ParseState::Success, 0,
        ParseState::Success, 0},
    {"negative zero", "-0",
        ParseState::Success, 0,
        ParseState::Failed, 0,
        ParseState::Success, 0,
        ParseState::Failed, 0},
    {"value", "2cafe5",
        ParseState::Success, 0x2cafe5,
        ParseState::Success, 0x2cafe5,
        ParseState::Success, 0x2cafe5,
        ParseState::Success, 0x2cafe5},
    {"uppercase", "DEADB07",
        ParseState::Success, 0xdeadb07,
        ParseState::Success, 0xdeadb07,
        ParseState::Success, 0xdeadb07,
        ParseState::Success, 0xdeadb07},
    {"mixed case", "BADcafe",
        ParseState::Success, 0xBADcafe,
        ParseState::Success, 0xBADcafe,
        ParseState::Success, 0xBADcafe,
        ParseState::Success, 0xBADcafe},
    {"just numeric", "1337",
        ParseState::Success, 0x1337,
        ParseState::Success, 0x1337,
        ParseState::Success, 0x1337,
        ParseState::Success, 0x1337},
    {"hex prefix", "0x1ee7",
        ParseState::Success, 0x1ee7,
        ParseState::Success, 0x1ee7,
        ParseState::Success, 0x1ee7,
        ParseState::Success, 0x1ee7},
    {"uppercase hex prefix, mixed case after", "0XbadC0DE",
        ParseState::Success, 0xbadc0de,
        ParseState::Success, 0xbadc0de,
        ParseState::Success, 0xbadc0de,
        ParseState::Success, 0xbadc0de},
    {"hash prefix", "#3366cc",
        ParseState::Success, 0x3366cc,
        ParseState::Success, 0x3366cc,
        ParseState::Success, 0x3366cc,
        ParseState::Success, 0x3366cc},
    {"leading zeros", "000000000000000000000000000beef5",
        ParseState::Success, 0xbeef5,
        ParseState::Success, 0xbeef5,
        ParseState::Success, 0xbeef5,
        ParseState::Success, 0xbeef5},
    {"leading zeros with a hex prefix", "0x0000beef5",
        ParseState::Success, 0xbeef5,
        ParseState::Success, 0xbeef5,
        ParseState::Success, 0xbeef5,
        ParseState::Success, 0xbeef5},
    /* This one is too long to fit in a SSO so it allocates when changing the #
       to 0 and should still be parsed correctly */
    {"leading zeros with a hash prefix", "#000000000000000000000000000beef5",
        ParseState::Success, 0xbeef5,
        ParseState::Success, 0xbeef5,
        ParseState::Success, 0xbeef5,
        ParseState::Success, 0xbeef5},
    {"positive value", "+faad5",
        ParseState::Success, 0xfaad5,
        ParseState::Success, 0xfaad5,
        ParseState::Success, 0xfaad5,
        ParseState::Success, 0xfaad5},
    {"positive value, hex prefix", "+0xfaad5",
        ParseState::Success, 0xfaad5,
        ParseState::Success, 0xfaad5,
        ParseState::Success, 0xfaad5,
        ParseState::Success, 0xfaad5},
    {"negative value", "-deed1",
        ParseState::Success, -0xdeed1,
        ParseState::Failed, 0,
        ParseState::Success, -0xdeed1,
        ParseState::Failed, 0},
    {"negative value, hash prefix", "-#deed1",
        ParseState::Success, -0xdeed1,
        ParseState::Failed, 0,
        ParseState::Success, -0xdeed1,
        ParseState::Failed, 0},

    {"leading whitespace", " \t \v\n\r  \f\fbeef5",
        ParseState::Success, 0xbeef5,
        ParseState::Success, 0xbeef5,
        ParseState::Success, 0xbeef5,
        ParseState::Success, 0xbeef5},
    {"trailing whitespace", "beef5 \t\t \v\f\f  \r\n",
        ParseState::Success, 0xbeef5,
        ParseState::Success, 0xbeef5,
        ParseState::Success, 0xbeef5,
        ParseState::Success, 0xbeef5},
    {"leading and trailing whitespace", "\t \v\f  beef5 \t\f  \r\n",
        ParseState::Success, 0xbeef5,
        ParseState::Success, 0xbeef5,
        ParseState::Success, 0xbeef5,
        ParseState::Success, 0xbeef5},
    {"leading and trailing whitespace, positive value", "\t \v\f  +faad5 \t\f  \r\n",
        ParseState::Success, 0xfaad5,
        ParseState::Success, 0xfaad5,
        ParseState::Success, 0xfaad5,
        ParseState::Success, 0xfaad5},
    {"leading and trailing whitespace, negative value", "\t \v\f  -deed1 \t\f  \r\n",
        ParseState::Success, -0xdeed1,
        ParseState::Failed, 0,
        ParseState::Success, -0xdeed1,
        ParseState::Failed, 0},
    {"leading and trailing whitespace, negative value and a hash prefix", "\t \v\f  -#deed1 \t\f  \r\n",
        ParseState::Success, -0xdeed1,
        ParseState::Failed, 0,
        ParseState::Success, -0xdeed1,
        ParseState::Failed, 0},

    /* Value clamping. Comparing to TypeTraits values to make sure the text
       doesn't contain off-by-one values and such. */
    {"largest 32-bit signed value", "7fffffff",
        ParseState::Success, Math::TypeTraits<Int>::max(),
        ParseState::Success, Math::TypeTraits<Int>::max(),
        ParseState::Success, Math::TypeTraits<Int>::max(),
        ParseState::Success, Math::TypeTraits<Int>::max()},
    {"largest 32-bit signed value plus one", "80000000",
        ParseState::Clamped, Math::TypeTraits<Int>::max(),
        ParseState::Success, Math::TypeTraits<Int>::max() + 1u,
        ParseState::Success, Math::TypeTraits<Int>::max() + 1ll,
        ParseState::Success, Math::TypeTraits<Int>::max() + 1ull},
    {"smallest 32-bit signed value", "-80000000",
        ParseState::Success, Math::TypeTraits<Int>::min(),
        ParseState::Failed, 0,
        ParseState::Success, Math::TypeTraits<Int>::min(),
        ParseState::Failed, 0},
    {"smallest 32-bit signed value minus one", "-80000001",
        ParseState::Clamped, Math::TypeTraits<Int>::min(),
        ParseState::Failed, 0,
        ParseState::Success, Math::TypeTraits<Int>::min() -1ll,
        ParseState::Failed, 0},
    {"largest 32-bit unsigned value", "ffffffff",
        ParseState::Clamped, Math::TypeTraits<Int>::max(),
        ParseState::Success, Math::TypeTraits<UnsignedInt>::max(),
        ParseState::Success, Math::TypeTraits<UnsignedInt>::max(),
        ParseState::Success, Math::TypeTraits<UnsignedInt>::max()},
    {"largest 32-bit unsigned value plus one", "100000000",
        ParseState::Clamped, Math::TypeTraits<Int>::max(),
        ParseState::Clamped, Math::TypeTraits<UnsignedInt>::max(),
        ParseState::Success, Math::TypeTraits<UnsignedInt>::max() + 1ll,
        ParseState::Success, Math::TypeTraits<UnsignedInt>::max() + 1ull},

    {"largest 64-bit signed value", "7fffffffffffffff",
        ParseState::Clamped, Math::TypeTraits<Int>::max(),
        ParseState::Clamped, Math::TypeTraits<UnsignedInt>::max(),
        ParseState::Success, Math::TypeTraits<Long>::max(),
        ParseState::Success, Math::TypeTraits<Long>::max()},
    {"largest 64-bit signed value plus one", "8000000000000000",
        ParseState::Clamped, Math::TypeTraits<Int>::max(),
        ParseState::Clamped, Math::TypeTraits<UnsignedInt>::max(),
        ParseState::Clamped, Math::TypeTraits<Long>::max(),
        ParseState::Success, Math::TypeTraits<Long>::max() + 1ull},
    {"smallest 64-bit signed value", "-8000000000000000",
        ParseState::Clamped, Math::TypeTraits<Int>::min(),
        ParseState::Failed, 0,
        ParseState::Success, Math::TypeTraits<Long>::min(),
        ParseState::Failed, 0},
    {"smallest 64-bit signed value minus one", "-8000000000000001",
        ParseState::Clamped, Math::TypeTraits<Int>::min(),
        ParseState::Failed, 0,
        ParseState::Clamped, Math::TypeTraits<Long>::min(),
        ParseState::Failed, 0},
    {"largest 64-bit unsigned value", "ffffffffffffffff",
        ParseState::Clamped, Math::TypeTraits<Int>::max(),
        ParseState::Clamped, Math::TypeTraits<UnsignedInt>::max(),
        ParseState::Clamped, Math::TypeTraits<Long>::max(),
        ParseState::Success, Math::TypeTraits<UnsignedLong>::max()},
    {"largest 64-bit unsigned value plus one", "10000000000000000",
        ParseState::Clamped, Math::TypeTraits<Int>::max(),
        ParseState::Clamped, Math::TypeTraits<UnsignedInt>::max(),
        ParseState::Clamped, Math::TypeTraits<Long>::max(),
        ParseState::Clamped, Math::TypeTraits<UnsignedLong>::max()},
};

const struct {
    TestSuite::TestCaseDescriptionSourceLocation name;
    Containers::StringView text;
    /** @todo expand with further diagnostic (cursor position and such) */
} ParseHexadecimalFailedData[]{
    {"empty string", ""},
    {"just spaces alone", " \t\t \v\f\f  \r\n"},
    {"invalid character at the begin", "gfeed"},
    {"invalid character in the middle", "feegd"},
    {"invalid character at the end", "deefg"},
    /* This would pass if the string length wouldn't be correctly propagated
       all the way */
    {"null terminator in the middle", "fee\0d"_s},
    {"duplicated minus sign", "--feed"},
    {"duplicated plus sign", "++feed"},
    {"plus and minus sign", "+-feed"},
    {"minus and plus sign", "+-feed"},
    {"leading zeros before the hex prefix", "0000x0ab"},
    {"both hex and hash prefix", "#0xabcdef"},
    /* All cases enumerated to catch Emscripten / musl edge cases where it
       accepts this as a valid number. */
    {"hex prefix alone", "0x"},
    {"hex prefix alone with spaces around", " 0x   "},
    {"uppercase hex prefix alone", "0X"},
    {"positive hex prefix alone", "+0x"},
    {"positive uppercase hex prefix alone", "+0X"},
    {"negative hex prefix alone", "-0x"},
    {"negative uppercase hex prefix alone", "-0X"},
    /* Internally if a # is found, the string is copied and the # gets replaced
       with 0 before passing it further. But replacing it the wrong way could
       cause invalid inputs to be treated as valid, so test that these fail as
       they should. */
    {"multiple hash prefixes", "###abcdef"}, /* 000abcdef is valid */
    {"hash prefix in the middle", "a#bcdef"}, /* a0bcdef is valid */
    {"hash prefix alone", "#"}, /* 0 is valid */
    {"positive hash prefix alone", "+#"}, /* 0 is valid */
    {"negative hash prefix alone", "-#"}, /* 0 is valid */

    /* Cases that currently fail but maybe eventually shouldn't? */
    {"space in the middle", "feed cafe"},
    {"space after a plus sign", "+ feedcafe"},
    {"space after a minus sign", "- feedcafe"},
    {"space after a hex prefix", "0x feedcafe"},
    {"space after a hash", "# feedcafe"},
    /* Same as with decimal, maybe should be allowed? If so, then the hash
       detection logic needs updating as well, tho. */
    {"Unicode minus sign", "−deaf"},
};

const struct {
    TestSuite::TestCaseDescriptionSourceLocation name;
    Containers::StringView text;
    /* Enumerating both cases because anything else would make the test code
       too cryptic and prone to accidentally testing the wrong thing or not
       testing certain cases at all */
    ParseState expectedState;
    Float expected;
    ParseState expectedStateDouble;
    Double expectedDouble;
} ParseFloatData[]{
    {"zero", "0",
        ParseState::Success, 0.0f,
        ParseState::Success, 0.0},
    {"very many zeros",
        "000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000e100",
        ParseState::Success, 0.0f,
        ParseState::Success, 0.0},
    {"positive zero", "+0",
        ParseState::Success, 0.0f,
        ParseState::Success, 0.0},
    {"negative zero", "-0",
        ParseState::Success, -0.0f,
        ParseState::Success, -0.0},
    {"negative zero with an exponent", "-0e-100",
        ParseState::Success, -0.0f,
        ParseState::Success, -0.0},
    {"value", "1337.420",
        ParseState::Success, 1337.420f,
        ParseState::Success, 1337.420},
    {"leading zeros", "000000000000001337.420",
        ParseState::Success, 1337.420f,
        ParseState::Success, 1337.420},
    /** @todo make this fail? if yes, then disallow also +. and -. */
    {"leading zero omitted", ".420",
        ParseState::Success, 0.420f,
        ParseState::Success, 0.420},
    {"positive value", "+1337.420",
        ParseState::Success, 1337.420f,
        ParseState::Success, 1337.420},
    {"negative value", "-1337.420",
        ParseState::Success, -1337.420f,
        ParseState::Success, -1337.420},
    {"exponent", "1.33742e3",
        ParseState::Success, 1337.420f,
        ParseState::Success, 1337.420},
    {"uppercase exponent", "1.33742E3",
        ParseState::Success, 1337.420f,
        ParseState::Success, 1337.420},
    {"exponent with positive sign", "1.33742e+3",
        ParseState::Success, 1337.420f,
        ParseState::Success, 1337.420},
    {"exponent with negative sign", "1337420e-3",
        ParseState::Success, 1337.420f,
        ParseState::Success, 1337.420},

    {"leading whitespace", " \t \v\n\r  \f\f1337.420",
        ParseState::Success, 1337.420f,
        ParseState::Success, 1337.420},
    {"trailing whitespace", "1337.420 \t\t \v\f\f  \r\n",
        ParseState::Success, 1337.420f,
        ParseState::Success, 1337.420},
    {"leading and trailing whitespace", "\t \v\f  1337.420 \t\f  \r\n",
        ParseState::Success, 1337.420f,
        ParseState::Success, 1337.420},
    {"leading and trailing whitespace, positive value", "\t \v\f  +1337.420 \t\f  \r\n",
        ParseState::Success, 1337.420f,
        ParseState::Success, 1337.420},
    {"leading and trailing whitespace, negative value", "\t \v\f  -1337.420 \t\f  \r\n",
        ParseState::Success, -1337.420f,
        ParseState::Success, -1337.420},

    /* Overflow to positive/negative infninity */
    {"largest 32-bit value", "340282346638528859811704183484516925440",
        /* https://en.wikipedia.org/wiki/Single-precision_floating-point_format */
        ParseState::Success, FloatFromBits{0x7f7fffffu}.value,
        ParseState::Success, 340282346638528859811704183484516925440.0},
                      /* value changed here ---v to 6 from 4 */
    {"largest 32-bit value plus some", "340282366638528859811704183484516925440",
        ParseState::Clamped, Constants::inf(),
        ParseState::Success, 340282366638528859811704183484516925440.0},
    {"smallest 32-bit value", "-340282346638528859811704183484516925440",
        /* Like above, but flipping the highest sign bit */
        ParseState::Success, FloatFromBits{0xff7fffffu}.value,
        ParseState::Success, -340282346638528859811704183484516925440.0},
                         /* value changed here ---v to 6 from 4 */
    {"smallest 32-bit value minus some", "-340282366638528859811704183484516925440",
        ParseState::Clamped, -Constants::inf(),
        ParseState::Success, -340282366638528859811704183484516925440.0},

    {"largest 64-bit value",
        "179769313486231570814527423731704356798070567525844996598917476803157260780028538760589558632766878171540458953514382464234321326889464182768467546703537516986049910576551282076245490090389328944075868508455133942304583236903222948165808559332123348274797826204144723168738177180919299881250404026184124858368",
        ParseState::Clamped, Constants::inf(),
        /* https://en.wikipedia.org/wiki/Double-precision_floating-point_format */
        ParseState::Success, DoubleFromBits{0x7fefffffffffffffull}.value},
    {"largest 64-bit value plus some",
                      /* v--- value changed here to 8 from 7 */
        "179769313486231580814527423731704356798070567525844996598917476803157260780028538760589558632766878171540458953514382464234321326889464182768467546703537516986049910576551282076245490090389328944075868508455133942304583236903222948165808559332123348274797826204144723168738177180919299881250404026184124858368",
        ParseState::Clamped, Constants::inf(),
        ParseState::Clamped, Constantsd::inf()},
    {"smallest 64-bit value",
        "-179769313486231570814527423731704356798070567525844996598917476803157260780028538760589558632766878171540458953514382464234321326889464182768467546703537516986049910576551282076245490090389328944075868508455133942304583236903222948165808559332123348274797826204144723168738177180919299881250404026184124858368",
        ParseState::Clamped, -Constants::inf(),
        /* Like above, but flipping the highest sign bit */
        ParseState::Success, DoubleFromBits{0xffefffffffffffffull}.value},
    {"smallest 64-bit value plus one",
                       /* v--- value changed here to 8 from 7 */
        "-179769313486231580814527423731704356798070567525844996598917476803157260780028538760589558632766878171540458953514382464234321326889464182768467546703537516986049910576551282076245490090389328944075868508455133942304583236903222948165808559332123348274797826204144723168738177180919299881250404026184124858368",
        ParseState::Clamped, -Constants::inf(),
        ParseState::Clamped, -Constantsd::inf()},

    /* The string `infinity` is supported by std::strtof() as well but I don't
       intend to claim that as being supported so don't even test for that */
    {"infinity", "inf",
        /* It should *not* claim that a clamp happened since that's what we
           want to enter */
        ParseState::Success, Constants::inf(),
        ParseState::Success, Constantsd::inf()},
    {"positive infinity, mixed case", "+iNF",
        ParseState::Success, Constants::inf(),
        ParseState::Success, Constantsd::inf()},
    {"negative infinity, mixed case", "-Inf",
        ParseState::Success, -Constants::inf(),
        ParseState::Success, -Constantsd::inf()},
    /* Positive / negative NaN is ignored for practical purposes, so comparing
       to just NaN always */
    {"NaN", "nan",
        ParseState::Success, Constants::nan(),
        ParseState::Success, Constantsd::nan()},
    {"negative NaN, mixed case", "-nAn",
        ParseState::Success, Constants::nan(),
        ParseState::Success, Constantsd::nan()},
};

const struct {
    TestSuite::TestCaseDescriptionSourceLocation name;
    Containers::StringView text;
    /** @todo expand with further diagnostic (cursor position and such) */
} ParseFloatFailedData[]{
    {"empty string", ""},
    {"just spaces alone", " \t\t \v\f\f  \r\n"},
    {"invalid character at the begin", "_42069"},
    {"invalid character in the middle", "420#69"},
    {"invalid character at the end", "42069?"},
    /* This would pass if the string length wouldn't be correctly propagated
       all the way. Have to split in two literals because FUCKING C understands
       that as octal 06, ugh. */
    {"null terminator in the middle", "420\0" "69"_s},
    {"duplicated minus sign", "--13.37"},
    {"duplicated plus sign", "++13.37"},
    {"plus and minus sign", "+-13.37"},
    {"minus and plus sign", "+-13.37"},

    /* I don't intend to support this weird hex representation once Corrade has
       own float parsers so disallowing it here already. (A hex representation
       of a float/double bit pattern is something else, supporting that makes
       sense, but that doesn't need a complex float parser.) Checking all
       possible variants that should fail. */
    {"hex representation", "0xfeed.beef"},
    {"hex representation, negative", "-0xfeed.beef"},
    {"hex representation, positive", "+0xfeed.beef"},
    {"hex representation, uppercase", "0XFEED.BEEF"},
    {"hex representation, negative uppercase", "-0XFEED.BEEF"},
    {"hex representation, positive uppercase", "-0XFEED.BEEF"},
    {"hex representation with an exponent and spaces around", "   -0x1.bc70a3d70a3d7p+6 "},
    {"hex representation with an exponent and spaces around, uppercase", "\t\b0X1.BC70A3D70A3D7P6  "},

    /* Cases that currently fail but maybe eventually shouldn't? */
    {"space in the middle", "420 69"},
    {"space after a plus sign", "+ 420.1337"},
    {"space after a minus sign", "- 420.1337"},
    {"space before an exponent", "4.201337e +2"},
    {"comma as a decimal separator", "13,37"},
    /* This definitely shouldn't fail, sigh. Compare the vertical alignment of
       the two: −- */
    {"Unicode minus sign", "−42069"},
};

FormatterTest::FormatterTest() {
    addTests({&FormatterTest::debugParseState,
              &FormatterTest::debugDecimalFlag,
              &FormatterTest::debugDecimalFlags,
              &FormatterTest::debugHexadecimalFlag,
              &FormatterTest::debugHexadecimalFlags,
              &FormatterTest::debugFloatFlag,
              &FormatterTest::debugFloatFlags,

              &FormatterTest::constructDecimal,
              &FormatterTest::constructHexadecimal,
              &FormatterTest::constructFloat,
              &FormatterTest::constructAttached<DecimalFormatter>,
              &FormatterTest::constructAttached<HexadecimalFormatter>,
              &FormatterTest::constructAttached<FloatFormatter>,
              &FormatterTest::constructCopy<DecimalFormatter>,
              &FormatterTest::constructCopy<HexadecimalFormatter>,
              &FormatterTest::constructCopy<FloatFormatter>,

              &FormatterTest::attach<DecimalFormatter>,
              &FormatterTest::attach<HexadecimalFormatter>,
              &FormatterTest::attach<FloatFormatter>,
              &FormatterTest::attachInvalid<DecimalFormatter>,
              &FormatterTest::attachInvalid<HexadecimalFormatter>,
              &FormatterTest::attachInvalid<FloatFormatter>,

              &FormatterTest::flags<DecimalFormatter>,
              &FormatterTest::flags<HexadecimalFormatter>,
              &FormatterTest::flags<FloatFormatter>,
              &FormatterTest::decimalMinWidth<DecimalFormatter>,
              &FormatterTest::decimalMinWidth<HexadecimalFormatter>,
              &FormatterTest::decimalMinWidthInvalid<DecimalFormatter>,
              &FormatterTest::decimalMinWidthInvalid<HexadecimalFormatter>,
              &FormatterTest::floatPrecision,
              &FormatterTest::floatPrecisionInvalid});

    addInstancedTests({&FormatterTest::formatDecimal},
        Containers::arraySize(FormatDecimalData));

    addInstancedTests({&FormatterTest::formatHexadecimal},
        Containers::arraySize(FormatHexadecimalData));

    addInstancedTests({&FormatterTest::formatFloat},
        Containers::arraySize(FormatFloatData));

    addTests<FormatterTest>({
        &FormatterTest::formatInvalid<DecimalFormatter, Int>,
        &FormatterTest::formatInvalid<DecimalFormatter, UnsignedInt>,
        &FormatterTest::formatInvalid<DecimalFormatter, Long>,
        &FormatterTest::formatInvalid<DecimalFormatter, UnsignedLong>,
        &FormatterTest::formatInvalid<HexadecimalFormatter, Int>,
        &FormatterTest::formatInvalid<HexadecimalFormatter, UnsignedInt>,
        &FormatterTest::formatInvalid<HexadecimalFormatter, Long>,
        &FormatterTest::formatInvalid<HexadecimalFormatter, UnsignedLong>,
        &FormatterTest::formatInvalid<FloatFormatter, Float>,
        &FormatterTest::formatInvalid<FloatFormatter, Double>});

    addInstancedTests({&FormatterTest::parseDecimal},
        Containers::arraySize(ParseDecimalData));

    addInstancedTests({&FormatterTest::parseDecimalFailed},
        Containers::arraySize(ParseDecimalFailedData));

    addInstancedTests({&FormatterTest::parseHexadecimal},
        Containers::arraySize(ParseHexadecimalData));

    addInstancedTests({&FormatterTest::parseHexadecimalFailed},
        Containers::arraySize(ParseHexadecimalFailedData));

    addInstancedTests({&FormatterTest::parseFloat},
        Containers::arraySize(ParseFloatData));

    addInstancedTests({&FormatterTest::parseFloatFailed},
        Containers::arraySize(ParseFloatFailedData));

    addTests({&FormatterTest::parseInvalid});

    /* The cache, font & shared instance are trivial and have no effect on the
       formatter output, so populate them just once to reduce overhead */
    _cache.addFont(67, &_font);
    _shared.setStyle(
        TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}},
        {_shared.addFont(_font, 1.0f)},
        {Text::Alignment{}},
        {}, {}, {}, {}, {}, {});
}

void FormatterTest::debugParseState() {
    Containers::String out;
    Debug{&out} << ParseState::Clamped << ParseState(0xbe);
    CORRADE_COMPARE(out, "Ui::ParseState::Clamped Ui::ParseState(0xbe)\n");
}

void FormatterTest::debugDecimalFlag() {
    Containers::String out;
    Debug{&out} << DecimalFormatter::Flag::ExplicitPlus << DecimalFormatter::Flag(0xbe);
    CORRADE_COMPARE(out, "Ui::DecimalFormatter::Flag::ExplicitPlus Ui::DecimalFormatter::Flag(0xbe)\n");
}

void FormatterTest::debugDecimalFlags() {
    Containers::String out;
    Debug{&out} << (DecimalFormatter::Flag::ExplicitPlus|DecimalFormatter::Flag(0x80)) << DecimalFormatter::Flags{};
    CORRADE_COMPARE(out, "Ui::DecimalFormatter::Flag::ExplicitPlus|Ui::DecimalFormatter::Flag(0x80) Ui::DecimalFormatter::Flags{}\n");
}

void FormatterTest::debugHexadecimalFlag() {
    Containers::String out;
    Debug{&out} << HexadecimalFormatter::Flag::HashPrefix << HexadecimalFormatter::Flag(0xbe);
    CORRADE_COMPARE(out, "Ui::HexadecimalFormatter::Flag::HashPrefix Ui::HexadecimalFormatter::Flag(0xbe)\n");
}

void FormatterTest::debugHexadecimalFlags() {
    Containers::String out;
    Debug{&out} << (HexadecimalFormatter::Flag::Uppercase|HexadecimalFormatter::Flag(0x80)) << HexadecimalFormatter::Flags{};
    CORRADE_COMPARE(out, "Ui::HexadecimalFormatter::Flag::Uppercase|Ui::HexadecimalFormatter::Flag(0x80) Ui::HexadecimalFormatter::Flags{}\n");
}

void FormatterTest::debugFloatFlag() {
    Containers::String out;
    Debug{&out} << FloatFormatter::Flag::Exponent << FloatFormatter::Flag(0xbe);
    CORRADE_COMPARE(out, "Ui::FloatFormatter::Flag::Exponent Ui::FloatFormatter::Flag(0xbe)\n");
}

void FormatterTest::debugFloatFlags() {
    Containers::String out;
    Debug{&out} << (FloatFormatter::Flag::Decimal|FloatFormatter::Flag(0x80)) << FloatFormatter::Flags{};
    CORRADE_COMPARE(out, "Ui::FloatFormatter::Flag::Decimal|Ui::FloatFormatter::Flag(0x80) Ui::FloatFormatter::Flags{}\n");
}

void FormatterTest::constructDecimal() {
    DecimalFormatter formatter{DecimalFormatter::Flag(0x80)};
    CORRADE_COMPARE(formatter.layer(), nullptr);
    CORRADE_COMPARE(formatter.data(), DataHandle::Null);
    CORRADE_COMPARE(formatter.flags(), DecimalFormatter::Flag(0x80));
    CORRADE_COMPARE(formatter.minWidth(), 1);

    /* The formatter should be small enough to fit in-place in a Function for
       use in DataLayer::onUpdate(). This may also fail if the formatter is not
       trivially copyable, which is verified in constructCopy() below. */
    CORRADE_VERIFY(!Containers::Function<void(Int)>{formatter}.isAllocated());
}

void FormatterTest::constructHexadecimal() {
    HexadecimalFormatter formatter{HexadecimalFormatter::Flag::HashPrefix|HexadecimalFormatter::Flag::Uppercase};
    CORRADE_COMPARE(formatter.layer(), nullptr);
    CORRADE_COMPARE(formatter.data(), DataHandle::Null);
    CORRADE_COMPARE(formatter.flags(), HexadecimalFormatter::Flag::HashPrefix|HexadecimalFormatter::Flag::Uppercase);
    CORRADE_COMPARE(formatter.minWidth(), 1);

    /* The formatter should be small enough to fit in-place in a Function for
       use in DataLayer::onUpdate(). This may also fail if the formatter is not
       trivially copyable, which is verified in constructCopy() below. */
    CORRADE_VERIFY(!Containers::Function<void(Int)>{formatter}.isAllocated());
}

void FormatterTest::constructFloat() {
    FloatFormatter formatter{FloatFormatter::Flag::Decimal|FloatFormatter::Flag::ExplicitPlus};
    CORRADE_COMPARE(formatter.layer(), nullptr);
    CORRADE_COMPARE(formatter.data(), DataHandle::Null);
    CORRADE_COMPARE(formatter.flags(), FloatFormatter::Flag::Decimal|FloatFormatter::Flag::ExplicitPlus);
    CORRADE_COMPARE(formatter.precision(), 6);

    /* The formatter should be small enough to fit in-place in a Function for
       use in DataLayer::onUpdate(). This may also fail if the formatter is not
       trivially copyable, which is verified in constructCopy() below. */
    CORRADE_VERIFY(!Containers::Function<void(Float)>{formatter}.isAllocated());
}

template<class> struct FormatTraits;
template<> struct FormatTraits<DecimalFormatter> {
    static const char* name() { return "DecimalFormatter"; }
};
template<> struct FormatTraits<HexadecimalFormatter> {
    static const char* name() { return "HexadecimalFormatter"; }
};
template<> struct FormatTraits<FloatFormatter> {
    static const char* name() { return "FloatFormatter"; }
};

template<class T> void FormatterTest::constructAttached() {
    setTestCaseTemplateName(FormatTraits<T>::name());

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0xab, 0x12), _shared};

    /* Create some extra data to have a non-trivial handle */
    layer.create(0, "", {});
    layer.create(0, "", {});
    layer.remove(layer.create(0, "", {}));
    layer.remove(layer.create(0, "", {}));
    DataHandle data = layer.create(0, "", {});

    T formatter1{layer, data, T::Flag::ExplicitPlus};
    T formatter2{layer, dataHandleData(data), T::Flag::ExplicitPlus};
    CORRADE_COMPARE(formatter1.layer(), &layer);
    CORRADE_COMPARE(formatter2.layer(), &layer);
    CORRADE_COMPARE(formatter1.data(), dataHandle(layer.handle(), 2, 3));
    CORRADE_COMPARE(formatter2.data(), dataHandle(layer.handle(), 2, 3));
    CORRADE_COMPARE(formatter1.flags(), T::Flag::ExplicitPlus);
    CORRADE_COMPARE(formatter2.flags(), T::Flag::ExplicitPlus);
}

template<class T> void FormatterTest::constructCopy() {
    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0xab, 0x12), _shared};

    DataHandle data = layer.create(0, "", {});

    /* The copy is implicit, so verify just a subset of the fields */

    T a{layer, data};
    T b = a;
    CORRADE_COMPARE(b.layer(), &layer);
    CORRADE_COMPARE(b.data(), data);

    /* If the formatters aren't trivially copyable, they can't be used in-place
       in a Function, which is bad */
    #ifndef CORRADE_NO_STD_IS_TRIVIALLY_TRAITS
    CORRADE_VERIFY(std::is_trivially_copy_constructible<T>::value);
    CORRADE_VERIFY(std::is_trivially_copy_assignable<T>::value);
    #endif
}

template<class T> void FormatterTest::attach() {
    setTestCaseTemplateName(FormatTraits<T>::name());

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0xab, 0x12), _shared};

    /* Create some extra data to have a non-trivial handle */
    layer.create(0, "", {});
    layer.create(0, "", {});
    layer.remove(layer.create(0, "", {}));
    layer.remove(layer.create(0, "", {}));
    DataHandle data = layer.create(0, "", {});

    T formatter1, formatter2;
    CORRADE_COMPARE(formatter1.layer(), nullptr);
    CORRADE_COMPARE(formatter2.layer(), nullptr);
    CORRADE_COMPARE(formatter1.data(), DataHandle::Null);
    CORRADE_COMPARE(formatter2.data(), DataHandle::Null);

    formatter1.attach(layer, data);
    formatter2.attach(layer, dataHandleData(data));
    CORRADE_COMPARE(formatter1.layer(), &layer);
    CORRADE_COMPARE(formatter2.layer(), &layer);
    CORRADE_COMPARE(formatter1.data(), dataHandle(layer.handle(), 2, 3));
    CORRADE_COMPARE(formatter2.data(), dataHandle(layer.handle(), 2, 3));
}

template<class T> void FormatterTest::attachInvalid() {
    setTestCaseTemplateName(FormatTraits<T>::name());

    CORRADE_SKIP_IF_NO_ASSERT();

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0xab, 0x12), _shared},
      anotherLayer{layerHandle(0x12, 0xab), _shared};

    T formatter;

    /* Create some extra data to have a non-trivial handle */
    layer.create(0, "", {});
    layer.create(0, "", {});
    layer.remove(layer.create(0, "", {}));
    layer.remove(layer.create(0, "", {}));
    DataHandle data = layer.create(0, "", {});

    Containers::String out;
    Error redirectError{&out};
    /* Null handle */
    formatter.attach(layer, DataHandle::Null);
    formatter.attach(layer, LayerDataHandle::Null);
    /* Invalid handle */
    formatter.attach(layer, dataHandle(layer.handle(), 0x12345, 0xabc));
    formatter.attach(layer, layerDataHandle(0x12345, 0xabc));
    /* Valid handle but different layer. With LayerDataHandle this cannot
       happen as the layer is assumed to match. */
    formatter.attach(anotherLayer, data);
    CORRADE_COMPARE_AS(out, Utility::format(
        "Ui::{0}::attach(): invalid handle Ui::DataHandle::Null\n"
        "Ui::{0}::attach(): invalid handle Ui::LayerDataHandle::Null\n"

        "Ui::{0}::attach(): Ui::DataHandle({{0xab, 0x12}}, {{0x12345, 0xabc}}) not valid in Ui::LayerHandle(0xab, 0x12)\n"
        "Ui::{0}::attach(): Ui::LayerDataHandle(0x12345, 0xabc) not valid in Ui::LayerHandle(0xab, 0x12)\n"

        "Ui::{0}::attach(): Ui::DataHandle({{0xab, 0x12}}, {{0x2, 0x3}}) not valid in Ui::LayerHandle(0x12, 0xab)\n", FormatTraits<T>::name()),
        TestSuite::Compare::String);
}

template<class T> void FormatterTest::flags() {
    setTestCaseTemplateName(FormatTraits<T>::name());

    T formatter;
    CORRADE_COMPARE(formatter.flags(), typename T::Flags{});

    formatter.setFlags(T::Flag::ExplicitPlus|typename T::Flags{0xc0});
    CORRADE_COMPARE(formatter.flags(), T::Flag::ExplicitPlus|typename T::Flags{0xc0});

    formatter.clearFlags(T::Flag::ExplicitPlus|typename T::Flags{0x80});
    CORRADE_COMPARE(formatter.flags(), typename T::Flags{0x40});

    formatter.addFlags(T::Flag::ExplicitPlus);
    CORRADE_COMPARE(formatter.flags(), T::Flag::ExplicitPlus|typename T::Flags{0x40});
}

template<class T> void FormatterTest::decimalMinWidth() {
    setTestCaseTemplateName(FormatTraits<T>::name());

    T formatter;
    CORRADE_COMPARE(formatter.minWidth(), 1);

    formatter.setMinWidth(107);
    CORRADE_COMPARE(formatter.minWidth(), 107);
}

template<class T> void FormatterTest::decimalMinWidthInvalid() {
    setTestCaseTemplateName(FormatTraits<T>::name());

    CORRADE_SKIP_IF_NO_ASSERT();

    T formatter;

    /* Shouldn't fail for exactly the boundaries */
    formatter.setMinWidth(0);
    formatter.setMinWidth(255);

    Containers::String out;
    Error redirectError{&out};
    formatter.setMinWidth(-1);
    formatter.setMinWidth(256);
    CORRADE_COMPARE_AS(out, Utility::format(
        "Ui::{0}::setMinWidth(): expected width to be between 0 and 255 but got -1\n"
        "Ui::{0}::setMinWidth(): expected width to be between 0 and 255 but got 256\n", FormatTraits<T>::name()),
        TestSuite::Compare::String);
}

void FormatterTest::floatPrecision() {
    FloatFormatter formatter;
    CORRADE_COMPARE(formatter.precision(), 6);

    formatter.setPrecision(107);
    CORRADE_COMPARE(formatter.precision(), 107);
}

void FormatterTest::floatPrecisionInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    FloatFormatter formatter;

    /* Shouldn't fail for exactly the boundaries */
    formatter.setPrecision(0);
    formatter.setPrecision(255);

    Containers::String out;
    Error redirectError{&out};
    formatter.setPrecision(-1);
    formatter.setPrecision(256);
    CORRADE_COMPARE_AS(out,
        "Ui::FloatFormatter::setPrecision(): expected precision to be between 0 and 255 but got -1\n"
        "Ui::FloatFormatter::setPrecision(): expected precision to be between 0 and 255 but got 256\n",
        TestSuite::Compare::String);
}

void FormatterTest::formatDecimal() {
    auto&& data = FormatDecimalData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Only one or the other should be non-zero */
    CORRADE_INTERNAL_ASSERT(!data.value || !data.valueUnsigned);

    /* The font has an empty shaper like elsewhere, the actual verification is
       done by making the text editable and comparing the saved string */

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0xab, 0x12), _shared};

    /* Create extra data to verify it's not always the first one updated */
    layer.create(0, "", {});

    DataHandle text = layer.create(0, "hello?", {}, TextDataFlag::Editable);
    CORRADE_COMPARE(layer.text(text), "hello?");

    DecimalFormatter formatter{layer, text, data.flags};
    if(data.minWidth)
        formatter.setMinWidth(*data.minWidth);

    /* Test with all possible type variants. If the value fits into given type,
       the formatted output should match, and if the value doesn't fit, the
       output shouldn't to verify we're actually testing the bounds properly.*/

    /* 32-bit signed type */
    formatter(data.valueUnsigned ? Int(data.valueUnsigned) : Int(data.value));
    if(!data.valueUnsigned && data.value >= -(1ll << 31) && data.value < (1ll << 31))
        CORRADE_COMPARE(layer.text(text), data.expected);
    else
        CORRADE_VERIFY(layer.text(text) != data.expected);

    /* 32-bit unsigned type */
    formatter(data.valueUnsigned ? UnsignedInt(data.valueUnsigned) : UnsignedInt(data.value));
    if(!data.valueUnsigned && data.value >= 0 && data.value < (1ll << 32))
        CORRADE_COMPARE(layer.text(text), data.expected);
    else
        CORRADE_VERIFY(layer.text(text) != data.expected);

    /* 64-bit signed type */
    formatter(data.valueUnsigned ? Long(data.valueUnsigned) : data.value);
    if(!data.valueUnsigned)
        CORRADE_COMPARE(layer.text(text), data.expected);
    else
        CORRADE_VERIFY(layer.text(text) != data.expected);

    /* 64-bit unsigned type */
    formatter(data.valueUnsigned ? data.valueUnsigned : UnsignedLong(data.value));
    if(data.valueUnsigned || data.value >= 0)
        CORRADE_COMPARE(layer.text(text), data.expected);
    else
        CORRADE_VERIFY(layer.text(text) != data.expected);
}

void FormatterTest::formatHexadecimal() {
    auto&& data = FormatHexadecimalData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Only one or the other should be non-zero */
    CORRADE_INTERNAL_ASSERT(!data.value || !data.valueUnsigned);

    /* The font has an empty shaper like elsewhere, the actual verification is
       done by making the text editable and comparing the saved string */

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0xab, 0x12), _shared};

    /* Create extra data to verify it's not always the first one updated */
    layer.create(0, "", {});

    DataHandle text = layer.create(0, "hello?", {}, TextDataFlag::Editable);
    CORRADE_COMPARE(layer.text(text), "hello?");

    HexadecimalFormatter formatter{layer, text, data.flags};
    if(data.minWidth)
        formatter.setMinWidth(*data.minWidth);

    /* Test with all possible type variants, same as in formatDecimal() */

    /* 32-bit signed type */
    formatter(data.valueUnsigned ? Int(data.valueUnsigned) : Int(data.value));
    if(!data.valueUnsigned && data.value >= -(1ll << 31) && data.value < (1ll << 31))
        CORRADE_COMPARE(layer.text(text), data.expected);
    else
        CORRADE_VERIFY(layer.text(text) != data.expected);

    /* 32-bit unsigned type */
    formatter(data.valueUnsigned ? UnsignedInt(data.valueUnsigned) : UnsignedInt(data.value));
    if(!data.valueUnsigned && data.value >= 0 && data.value < (1ll << 32))
        CORRADE_COMPARE(layer.text(text), data.expected);
    else
        CORRADE_VERIFY(layer.text(text) != data.expected);

    /* 64-bit signed type */
    formatter(data.valueUnsigned ? Long(data.valueUnsigned) : data.value);
    if(!data.valueUnsigned)
        CORRADE_COMPARE(layer.text(text), data.expected);
    else
        CORRADE_VERIFY(layer.text(text) != data.expected);

    /* 64-bit unsigned type */
    formatter(data.valueUnsigned ? data.valueUnsigned : UnsignedLong(data.value));
    if(data.valueUnsigned || data.value >= 0)
        CORRADE_COMPARE(layer.text(text), data.expected);
    else
        CORRADE_VERIFY(layer.text(text) != data.expected);
}

void FormatterTest::formatFloat() {
    auto&& data = FormatFloatData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Only one or the other should be non-zero */
    CORRADE_INTERNAL_ASSERT(!data.value || !data.valueDouble);

    /* The font has an empty shaper like elsewhere, the actual verification is
       done by making the text editable and comparing the saved string */

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0xab, 0x12), _shared};

    /* Create extra data to verify it's not always the first one updated */
    layer.create(0, "", {});

    DataHandle text = layer.create(0, "hello?", {}, TextDataFlag::Editable);
    CORRADE_COMPARE(layer.text(text), "hello?");

    FloatFormatter formatter{layer, text, data.flags};
    if(data.precision)
        formatter.setPrecision(*data.precision);

    /* 32-bit type. Comparing as string to have a diff because the output can
       get *long*. */
    formatter(data.valueDouble ? Float(data.valueDouble) : data.value);
    if(!data.valueDouble) {
        CORRADE_EXPECT_FAIL_IF(data.xfail, data.xfail);
        CORRADE_COMPARE_AS(layer.text(text),
            data.expected,
            TestSuite::Compare::String);
    } else
        CORRADE_VERIFY(layer.text(text) != data.expected);

    /* 64-bit type. This should pass always. */
    formatter(data.valueDouble ? data.valueDouble : Double(data.value));
    CORRADE_EXPECT_FAIL_IF(data.xfail, data.xfail);
    CORRADE_COMPARE_AS(layer.text(text),
        data.expected,
        TestSuite::Compare::String);
}

template<class T, class U> void FormatterTest::formatInvalid() {
    setTestCaseTemplateName({FormatTraits<T>::name(), Math::TypeTraits<U>::name()});

    CORRADE_SKIP_IF_NO_ASSERT();

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0xab, 0x12), _shared};

    /* Create some extra data to have a non-trivial handle */
    layer.create(0, "", {});
    layer.create(0, "", {});
    layer.remove(layer.create(0, "", {}));
    layer.remove(layer.create(0, "", {}));
    DataHandle removed = layer.create(0, "", {});

    T formatterNotAttached;
    T formatterRemoved{layer, removed};
    layer.remove(removed);

    Containers::String out;
    Error redirectError{&out};
    formatterNotAttached(U{});
    formatterRemoved(U{});
    CORRADE_COMPARE_AS(out, Utility::format(
        "Ui::{0}: formatter not attached\n"
        "Ui::{0}: Ui::DataHandle({{0xab, 0x12}}, {{0x2, 0x3}}) is no longer valid\n", FormatTraits<T>::name()),
        TestSuite::Compare::String);
}

void FormatterTest::parseDecimal() {
    auto&& data = ParseDecimalData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    Int value;
    CORRADE_COMPARE(DecimalFormatter::parse(data.text, value), data.expectedState);
    if(data.expectedState != ParseState::Failed)
        CORRADE_COMPARE(value, data.expected);

    UnsignedInt valueUnsigned;
    CORRADE_COMPARE(DecimalFormatter::parse(data.text, valueUnsigned), data.expectedStateUnsigned);
    if(data.expectedStateUnsigned != ParseState::Failed)
        CORRADE_COMPARE(valueUnsigned, data.expectedUnsigned);

    Long valueLong;
    CORRADE_COMPARE(DecimalFormatter::parse(data.text, valueLong), data.expectedStateLong);
    if(data.expectedStateLong != ParseState::Failed)
        CORRADE_COMPARE(valueLong, data.expectedLong);

    UnsignedLong valueUnsignedLong;
    CORRADE_COMPARE(DecimalFormatter::parse(data.text, valueUnsignedLong), data.expectedStateUnsignedLong);
    if(data.expectedStateUnsignedLong != ParseState::Failed)
        CORRADE_COMPARE(valueUnsignedLong, data.expectedUnsignedLong);
}

void FormatterTest::parseDecimalFailed() {
    auto&& data = ParseDecimalFailedData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* All failures tested here should behave the same for all cases. Failures
       affecting only a subset are tested in parseDecimal() above. */
    Int value;
    UnsignedInt valueUnsigned;
    Long valueLong;
    UnsignedLong valueUnsignedLong;
    CORRADE_COMPARE(DecimalFormatter::parse(data.text, value), ParseState::Failed);
    CORRADE_COMPARE(DecimalFormatter::parse(data.text, valueUnsigned), ParseState::Failed);
    CORRADE_COMPARE(DecimalFormatter::parse(data.text, valueLong), ParseState::Failed);
    CORRADE_COMPARE(DecimalFormatter::parse(data.text, valueUnsignedLong), ParseState::Failed);
    /** @todo expand with further diagnostic (cursor position and such) */
}

void FormatterTest::parseHexadecimal() {
    auto&& data = ParseHexadecimalData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    Int value;
    CORRADE_COMPARE(HexadecimalFormatter::parse(data.text, value), data.expectedState);
    if(data.expectedState != ParseState::Failed)
        CORRADE_COMPARE(value, data.expected);

    UnsignedInt valueUnsigned;
    CORRADE_COMPARE(HexadecimalFormatter::parse(data.text, valueUnsigned), data.expectedStateUnsigned);
    if(data.expectedStateUnsigned != ParseState::Failed)
        CORRADE_COMPARE(valueUnsigned, data.expectedUnsigned);

    Long valueLong;
    CORRADE_COMPARE(HexadecimalFormatter::parse(data.text, valueLong), data.expectedStateLong);
    if(data.expectedStateLong != ParseState::Failed)
        CORRADE_COMPARE(valueLong, data.expectedLong);

    UnsignedLong valueUnsignedLong;
    CORRADE_COMPARE(HexadecimalFormatter::parse(data.text, valueUnsignedLong), data.expectedStateUnsignedLong);
    if(data.expectedStateUnsignedLong != ParseState::Failed)
        CORRADE_COMPARE(valueUnsignedLong, data.expectedUnsignedLong);
}

void FormatterTest::parseHexadecimalFailed() {
    auto&& data = ParseHexadecimalFailedData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* All failures tested here should behave the same for all cases. Failures
       affecting only a subset are tested in parseHexadecimal() above. */
    Int value;
    UnsignedInt valueUnsigned;
    Long valueLong;
    UnsignedLong valueUnsignedLong;
    CORRADE_COMPARE(HexadecimalFormatter::parse(data.text, value), ParseState::Failed);
    CORRADE_COMPARE(HexadecimalFormatter::parse(data.text, valueUnsigned), ParseState::Failed);
    CORRADE_COMPARE(HexadecimalFormatter::parse(data.text, valueLong), ParseState::Failed);
    CORRADE_COMPARE(HexadecimalFormatter::parse(data.text, valueUnsignedLong), ParseState::Failed);
    /** @todo expand with further diagnostic (cursor position and such) */
}

void FormatterTest::parseFloat() {
    auto&& data = ParseFloatData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    Float value;
    {
        #if defined(__GLIBC__) && __GLIBC__*1000 + __GLIBC_MINOR__ < 2028
        /* https://github.com/bminor/glibc/commit/fcd6b5ac36a49e83e27e9186ded04329d3b0b0d9,
           or so I think. The 32-bit Clamped cases are picked to be the
           smallest possible single-digit change triggering it, they work on
           2.42, on MSVC and on Emscripten as well. Only large 64-bit values
           get detected correctly. */
        CORRADE_EXPECT_FAIL_IF(data.expectedState == ParseState::Clamped && Containers::StringView{data.name}.contains("32-bit"),
            "glibc before version 2.28 has an off-by-one error in overflow detection and doesn't set the errno correctly");
        #endif
        CORRADE_COMPARE(FloatFormatter::parse(data.text, value), data.expectedState);
    }
    if(data.expectedState != ParseState::Failed) {
        CORRADE_COMPARE(value, data.expected);
        /*  Verify that the result is really negative if it should be, i.e.
            that -0 is preserved as such */
        CORRADE_COMPARE(value < 0.0f, data.expected < 0.0f);
    }

    Double valueDouble;
    {
        #if defined(__GLIBC__) && __GLIBC__*1000 + __GLIBC_MINOR__ < 2028
        /* Similar to above, here it fails to produce Clamped in all cases
           where a 64-bit value is meant to overflow */
        CORRADE_EXPECT_FAIL_IF(data.expectedStateDouble == ParseState::Clamped,
            "glibc before version 2.28 has an off-by-one error in overflow detection and doesn't set the errno correctly");
        #endif
        CORRADE_COMPARE(FloatFormatter::parse(data.text, valueDouble), data.expectedStateDouble);
    }
    if(data.expectedStateDouble != ParseState::Failed) {
        CORRADE_COMPARE(valueDouble, data.expectedDouble);
        /*  Verify that the result is really negative if it should be, i.e.
            that -0 is preserved as such */
        CORRADE_COMPARE(valueDouble < 0.0, data.expectedDouble < 0.0);
    }
}

void FormatterTest::parseFloatFailed() {
    auto&& data = ParseFloatFailedData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* All failures tested here should behave the same for all cases. Failures
       affecting only a subset are tested in parseFloat() above. */
    Float value;
    Double valueDouble;
    CORRADE_COMPARE(FloatFormatter::parse(data.text, value), ParseState::Failed);
    CORRADE_COMPARE(FloatFormatter::parse(data.text, valueDouble), ParseState::Failed);
}

void FormatterTest::parseInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    /* Just a sanity check that an empty or view doesn't have NullTerminated
       set. Can't test with that as it crashes inside std::strto[u]ll() because
       of course, it's the STL we're talking about. */
    CORRADE_VERIFY(!(Containers::StringView{}.flags() >= Containers::StringViewFlag::NullTerminated));
    CORRADE_VERIFY(!(Containers::StringView{nullptr}.flags() >= Containers::StringViewFlag::NullTerminated));

    Int value;
    UnsignedInt valueUnsigned;
    Long valueLong;
    UnsignedLong valueUnsignedLong;
    Float valueFloat;
    Double valueDouble;

    Containers::String out;
    Error redirectError{&out};
    /* Be sure to still pass a string that eventually null terminates to avoid
       std::strto[u]ll() runaway reading into random memory after. Sigh. */
    DecimalFormatter::parse("12345"_s.exceptSuffix(1), value);
    DecimalFormatter::parse("12345"_s.exceptSuffix(1), valueUnsigned);
    DecimalFormatter::parse("12345"_s.exceptSuffix(1), valueLong);
    DecimalFormatter::parse("12345"_s.exceptSuffix(1), valueUnsignedLong);
    HexadecimalFormatter::parse("12345"_s.exceptSuffix(1), value);
    HexadecimalFormatter::parse("12345"_s.exceptSuffix(1), valueUnsigned);
    HexadecimalFormatter::parse("12345"_s.exceptSuffix(1), valueLong);
    HexadecimalFormatter::parse("12345"_s.exceptSuffix(1), valueUnsignedLong);
    FloatFormatter::parse("12345"_s.exceptSuffix(1), valueFloat);
    FloatFormatter::parse("12345"_s.exceptSuffix(1), valueDouble);
    CORRADE_COMPARE_AS(out,
        "Ui::DecimalFormatter::parse(): text is not null-terminated\n"
        "Ui::DecimalFormatter::parse(): text is not null-terminated\n"
        "Ui::DecimalFormatter::parse(): text is not null-terminated\n"
        "Ui::DecimalFormatter::parse(): text is not null-terminated\n"
        "Ui::HexadecimalFormatter::parse(): text is not null-terminated\n"
        "Ui::HexadecimalFormatter::parse(): text is not null-terminated\n"
        "Ui::HexadecimalFormatter::parse(): text is not null-terminated\n"
        "Ui::HexadecimalFormatter::parse(): text is not null-terminated\n"
        "Ui::FloatFormatter::parse(): text is not null-terminated\n"
        "Ui::FloatFormatter::parse(): text is not null-terminated\n",
        TestSuite::Compare::String);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::FormatterTest)
