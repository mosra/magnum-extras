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

FormatterTest::FormatterTest() {
    addTests({&FormatterTest::debugDecimalFlag,
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

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::FormatterTest)
