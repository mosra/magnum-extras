/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023 Vladimír Vondruš <mosra@centrum.cz>

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

#include <new>
#include <sstream> /** @todo remove once Debug is stream-free */
#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/String.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Container.h>
#include <Corrade/TestSuite/Compare/String.h>
#include <Corrade/Utility/Algorithms.h>
#include <Corrade/Utility/DebugStl.h> /** @todo remove once Debug is stream-free */
#include <Magnum/PixelFormat.h>
#include <Magnum/Math/Range.h>
#include <Magnum/Text/AbstractGlyphCache.h>
#include <Magnum/Text/Alignment.h>
#include <Magnum/Text/Direction.h>
#include <Magnum/Text/Feature.h>
#include <Magnum/Text/Script.h>

#include "Magnum/Whee/AbstractUserInterface.h"
#include "Magnum/Whee/Event.h"
#include "Magnum/Whee/Handle.h"
#include "Magnum/Whee/TextLayer.h"
#include "Magnum/Whee/TextProperties.h"
/* for createRemoveSetText(), updateCleanDataOrder(), updateAlignment(),
   updateAlignmentGlyph(), updatePadding() and updatePaddingGlyph() */
#include "Magnum/Whee/Implementation/textLayerState.h"

namespace Magnum { namespace Whee { namespace Test { namespace {

struct TextLayerTest: TestSuite::Tester {
    explicit TextLayerTest();

    template<class T> void styleUniformSizeAlignment();

    void styleUniformCommonConstructDefault();
    void styleUniformCommonConstructNoInit();
    void styleUniformCommonSetters();

    void styleUniformConstructDefault();
    void styleUniformConstruct();
    void styleUniformConstructNoInit();
    void styleUniformSetters();

    void fontHandle();
    void fontHandleInvalid();
    void debugFontHandle();

    void sharedConfigurationConstruct();
    void sharedConfigurationConstructSameStyleUniformCount();
    void sharedConfigurationConstructZeroStyleCount();
    void sharedConfigurationConstructCopy();

    void sharedConstruct();
    void sharedConstructNoCreate();
    void sharedConstructCopy();
    void sharedConstructMove();

    void sharedSetGlyphCache();
    void sharedSetGlyphCacheAlreadySet();
    void sharedNoGlyphCache();

    void sharedAddFont();
    void sharedAddFontTakeOwnership();
    void sharedAddFontTakeOwnershipNull();
    void sharedAddFontNoCache();
    void sharedAddFontNotFoundInCache();
    void sharedAddFontNoHandlesLeft();
    void sharedAddInstancelessFontHasInstance();
    void sharedFontInvalidHandle();
    void sharedFontNoInstance();

    void sharedSetStyle();
    void sharedSetStyleImplicitPadding();
    void sharedSetStyleInvalidSize();
    void sharedSetStyleImplicitMapping();
    void sharedSetStyleImplicitMappingImplicitPadding();
    void sharedSetStyleImplicitMappingInvalidSize();
    void sharedSetStyleInvalidFontHandle();

    void construct();
    void constructCopy();
    void constructMove();

    /* remove() and setText() tested here as well */
    template<class StyleIndex, class GlyphIndex> void createRemoveSet();
    void createRemoveHandleRecycle();
    void createSetTextTextProperties();
    void createNoSharedGlyphCache();

    void setColor();
    void setPadding();

    void invalidHandle();
    void invalidFontHandle();
    void noSharedStyleFonts();
    void noFontInstance();
    void styleOutOfRange();
    void glyphOutOfRange();

    void updateEmpty();
    void updateCleanDataOrder();
    void updateAlignment();
    void updateAlignmentGlyph();
    void updatePadding();
    void updatePaddingGlyph();
    void updateNoStyleSet();
};

enum class Enum: UnsignedShort {};

Debug& operator<<(Debug& debug, Enum value) {
    return debug << UnsignedInt(value);
}

const struct {
    const char* name;
    NodeHandle node;
    LayerStates state;
    bool layerDataHandleOverloads, customFont, nullStyleFonts, noStyle;
} CreateRemoveSetData[]{
    {"create",
        NodeHandle::Null, LayerState::NeedsDataUpdate,
        false, false, false, false},
    {"create and attach",
        nodeHandle(9872, 0xbeb), LayerState::NeedsNodeOffsetSizeUpdate|LayerState::NeedsAttachmentUpdate|LayerState::NeedsDataUpdate,
        false, false, false, false},
    {"LayerDataHandle overloads",
        NodeHandle::Null, LayerState::NeedsDataUpdate,
        true, false, false, false},
    {"custom fonts",
        NodeHandle::Null, LayerState::NeedsDataUpdate,
        false, true, false, false},
    {"custom fonts, null style fonts",
        NodeHandle::Null, LayerState::NeedsDataUpdate,
        false, true, true, false},
    {"custom fonts, no style set",
        NodeHandle::Null, LayerState::NeedsDataUpdate,
        false, true, false, true},
    {"custom fonts, LayerDataHandle overloads",
        NodeHandle::Null, LayerState::NeedsDataUpdate,
        true, true, false, false},
};

const struct {
    const char* name;
    bool setStyle;
} NoSharedStyleFontsData[]{
    {"no style set", false},
    {"style with null font set", true}
};

const struct {
    const char* name;
    Text::Alignment alignment;
    /* Node offset is {50.5, 20.5}, size {200.8, 100.4}; bounding box {9, 11},
       ascent 7, descent -4 */
    Vector2 offset;
    /* Glyph ounding box is {6, 8}, offset {-4, -6} */
    Vector2 offsetGlyph;
} UpdateAlignmentPaddingData[]{
    {"line left", Text::Alignment::LineLeft,
        /* 20.5 + 100.4/2 = 70.7 */
        {50.5f, 70.7f},
        {50.5f, 76.7f}},
    {"line right", Text::Alignment::LineRight,
        {50.5f + 200.8f - 9.0f, 70.7f},
        {50.5f + 200.8f - 6.0f, 76.7f}},
    {"top center", Text::Alignment::TopCenter,
        {50.5f + 100.4f - 4.5f, 20.5f + 7.0f},
        {50.5f + 100.4f - 3.0f, 20.5f + 8.0f}},
    {"top center, interal", Text::Alignment::TopCenterIntegral,
        /* Only the offset inside the node and the bounding box is rounded,
           not the node offset itself; not the Y coordinate either */
        {50.5f + 100.0f - 5.0f, 20.5f + 7.0f},
        /* No change for the glyph as the glyph cache has integer sizes */
        {50.5f + 100.0f - 3.0f, 20.5f + 8.0f}},
    {"bottom left", Text::Alignment::BottomLeft,
        {50.5f, 120.9f - 4.0f},
        {50.5f, 120.9f}},
    {"middle right", Text::Alignment::MiddleRight,
        {50.5f + 200.8f - 9.0f, 20.5f + 50.2f - 5.5f + 7.0f},
        {50.5f + 200.8f - 6.0f, 20.5f + 50.2f - 4.0f + 8.0f}},
    {"middle right, integral", Text::Alignment::MiddleRightIntegral,
        /* Only the offset inside the node and the bounding box is rounded,
           not the node offset itself; not the X coordinate either. Note that
           the Y rounding is in the other direction compared to X because of Y
           flip. */
        {50.5f + 200.8f - 9.0f, 20.5f + 50.0f - 5.0f + 7.0f},
        {50.5f + 200.8f - 6.0f, 20.5f + 50.0f - 4.0f + 8.0f}},
    {"middle center", Text::Alignment::MiddleCenter,
        {50.5f + 100.4f - 4.5f, 20.5f + 50.2f - 5.5f + 7.0f},
        {50.5f + 100.4f - 3.0f, 20.5f + 50.2f - 4.0f + 8.0f}},
    {"middle center, integral", Text::Alignment::MiddleCenterIntegral,
        /* Only the offset inside the node and the bounding box is rounded,
           not the node offset itself. Note that the Y rounding is in the other
           direction compared to X because of Y flip. */
        {50.5f + 100.0f - 5.0f, 20.5f + 50.0f - 5.0f + 7.0f},
        {50.5f + 100.0f - 3.0f, 20.5f + 50.0f - 4.0f + 8.0f}},
};

const struct {
    const char* name;
    bool emptyUpdate;
    Vector2 node6Offset, node6Size;
    Vector4 paddingFromStyle;
    Vector4 paddingFromData;
} UpdateCleanDataOrderData[]{
    {"empty update", true,
        {}, {}, {}, {}},
    {"", false,
        {1.0f, 2.0f}, {10.0f, 15.0f}, {}, {}},
    {"padding from style", false,
        {-1.0f, 1.5f}, {13.0f, 17.0f},
        {2.0f, 0.5f, 1.0f, 1.5f}, {}},
    {"padding from data", false,
        {-1.0f, 1.5f}, {13.0f, 17.0f},
        {}, {2.0f, 0.5f, 1.0f, 1.5f}},
    {"padding from both style and data", false,
        {-1.0f, 1.5f}, {13.0f, 17.0f},
        {0.5f, 0.0f, 1.0f, 0.75f}, {1.5f, 0.5f, 0.0f, 0.75f}},
};

TextLayerTest::TextLayerTest() {
    addTests({&TextLayerTest::styleUniformSizeAlignment<TextLayerCommonStyleUniform>,
              &TextLayerTest::styleUniformSizeAlignment<TextLayerStyleUniform>,

              &TextLayerTest::styleUniformCommonConstructDefault,
              &TextLayerTest::styleUniformCommonConstructNoInit,
              &TextLayerTest::styleUniformCommonSetters,

              &TextLayerTest::styleUniformConstructDefault,
              &TextLayerTest::styleUniformConstruct,
              &TextLayerTest::styleUniformConstructNoInit,
              &TextLayerTest::styleUniformSetters,

              &TextLayerTest::fontHandle,
              &TextLayerTest::fontHandleInvalid,
              &TextLayerTest::debugFontHandle,

              &TextLayerTest::sharedConfigurationConstruct,
              &TextLayerTest::sharedConfigurationConstructSameStyleUniformCount,
              &TextLayerTest::sharedConfigurationConstructZeroStyleCount,
              &TextLayerTest::sharedConfigurationConstructCopy,

              &TextLayerTest::sharedConstruct,
              &TextLayerTest::sharedConstructNoCreate,
              &TextLayerTest::sharedConstructCopy,
              &TextLayerTest::sharedConstructMove,

              &TextLayerTest::sharedSetGlyphCache,
              &TextLayerTest::sharedSetGlyphCacheAlreadySet,
              &TextLayerTest::sharedNoGlyphCache,

              &TextLayerTest::sharedAddFont,
              &TextLayerTest::sharedAddFontTakeOwnership,
              &TextLayerTest::sharedAddFontTakeOwnershipNull,
              &TextLayerTest::sharedAddFontNoCache,
              &TextLayerTest::sharedAddFontNotFoundInCache,
              &TextLayerTest::sharedAddFontNoHandlesLeft,
              &TextLayerTest::sharedAddInstancelessFontHasInstance,
              &TextLayerTest::sharedFontInvalidHandle,
              &TextLayerTest::sharedFontNoInstance,

              &TextLayerTest::sharedSetStyle,
              &TextLayerTest::sharedSetStyleImplicitPadding,
              &TextLayerTest::sharedSetStyleInvalidSize,
              &TextLayerTest::sharedSetStyleImplicitMapping,
              &TextLayerTest::sharedSetStyleImplicitMappingImplicitPadding,
              &TextLayerTest::sharedSetStyleImplicitMappingInvalidSize,
              &TextLayerTest::sharedSetStyleInvalidFontHandle,

              &TextLayerTest::construct,
              &TextLayerTest::constructCopy,
              &TextLayerTest::constructMove});

    addInstancedTests<TextLayerTest>({
        &TextLayerTest::createRemoveSet<UnsignedInt, UnsignedInt>,
        &TextLayerTest::createRemoveSet<UnsignedInt, Enum>,
        &TextLayerTest::createRemoveSet<Enum, UnsignedInt>,
        &TextLayerTest::createRemoveSet<Enum, Enum>},
        Containers::arraySize(CreateRemoveSetData));

    addTests({&TextLayerTest::createRemoveHandleRecycle,
              &TextLayerTest::createSetTextTextProperties,
              &TextLayerTest::createNoSharedGlyphCache,

              &TextLayerTest::setColor,
              &TextLayerTest::setPadding,

              &TextLayerTest::invalidHandle,
              &TextLayerTest::invalidFontHandle});

    addInstancedTests({&TextLayerTest::noSharedStyleFonts},
        Containers::arraySize(NoSharedStyleFontsData));

    addTests({&TextLayerTest::noFontInstance,
              &TextLayerTest::styleOutOfRange,
              &TextLayerTest::glyphOutOfRange,

              &TextLayerTest::updateEmpty});

    addInstancedTests({&TextLayerTest::updateCleanDataOrder},
        Containers::arraySize(UpdateCleanDataOrderData));

    addInstancedTests({&TextLayerTest::updateAlignment,
                       &TextLayerTest::updateAlignmentGlyph,
                       &TextLayerTest::updatePadding,
                       &TextLayerTest::updatePaddingGlyph},
        Containers::arraySize(UpdateAlignmentPaddingData));

    addTests({&TextLayerTest::updateNoStyleSet});
}

using namespace Containers::Literals;
using namespace Math::Literals;

template<class> struct StyleTraits;
template<> struct StyleTraits<TextLayerCommonStyleUniform> {
    static const char* name() { return "TextLayerCommonStyleUniform"; }
};
template<> struct StyleTraits<TextLayerStyleUniform> {
    static const char* name() { return "TextLayerStyleUniform"; }
};

template<class T> void TextLayerTest::styleUniformSizeAlignment() {
    setTestCaseTemplateName(StyleTraits<T>::name());

    CORRADE_FAIL_IF(sizeof(T) % sizeof(Vector4) != 0, sizeof(T) << "is not a multiple of vec4 for UBO alignment.");

    /* 48-byte structures are fine, we'll align them to 768 bytes and not
       256, but warn about that */
    CORRADE_FAIL_IF(768 % sizeof(T) != 0, sizeof(T) << "can't fit exactly into 768-byte UBO alignment.");
    if(256 % sizeof(T) != 0)
        CORRADE_WARN(sizeof(T) << "can't fit exactly into 256-byte UBO alignment, only 768.");

    CORRADE_COMPARE(alignof(T), 4);
}

void TextLayerTest::styleUniformCommonConstructDefault() {
    TextLayerCommonStyleUniform a;
    TextLayerCommonStyleUniform b{DefaultInit};
    /* No actual fields yet */
    static_cast<void>(a);
    static_cast<void>(b);

    constexpr TextLayerCommonStyleUniform ca;
    constexpr TextLayerCommonStyleUniform cb{DefaultInit};
    /* No actual fields yet */
    static_cast<void>(ca);
    static_cast<void>(cb);

    CORRADE_VERIFY(std::is_nothrow_default_constructible<TextLayerCommonStyleUniform>::value);
    CORRADE_VERIFY(std::is_nothrow_constructible<TextLayerCommonStyleUniform, DefaultInitT>::value);

    /* Implicit construction is not allowed */
    CORRADE_VERIFY(!std::is_convertible<DefaultInitT, TextLayerCommonStyleUniform>::value);
}

void TextLayerTest::styleUniformCommonConstructNoInit() {
    /* Testing only some fields, should be enough */
    TextLayerCommonStyleUniform a;
    /* No actual fields yet */
    static_cast<void>(a);

    new(&a) TextLayerCommonStyleUniform{NoInit};
    {
        /* Explicitly check we're not on Clang because certain Clang-based IDEs
           inherit __GNUC__ if GCC is used instead of leaving it at 4 like
           Clang itself does */
        #if defined(CORRADE_TARGET_GCC) && !defined(CORRADE_TARGET_CLANG) && __GNUC__*100 + __GNUC_MINOR__ >= 601 && __OPTIMIZE__
        //CORRADE_EXPECT_FAIL("GCC 6.1+ misoptimizes and overwrites the value.");
        #endif
        /* No actual fields yet */
        CORRADE_VERIFY(true);
    }
}

void TextLayerTest::styleUniformCommonSetters() {
    TextLayerCommonStyleUniform a;
    static_cast<void>(a);
    CORRADE_SKIP("No actual fields yet");
}

void TextLayerTest::styleUniformConstructDefault() {
    TextLayerStyleUniform a;
    TextLayerStyleUniform b{DefaultInit};
    CORRADE_COMPARE(a.color, 0xffffffff_srgbaf);
    CORRADE_COMPARE(b.color, 0xffffffff_srgbaf);

    constexpr TextLayerStyleUniform ca;
    constexpr TextLayerStyleUniform cb{DefaultInit};
    CORRADE_COMPARE(ca.color, 0xffffffff_srgbaf);
    CORRADE_COMPARE(cb.color, 0xffffffff_srgbaf);

    CORRADE_VERIFY(std::is_nothrow_default_constructible<TextLayerStyleUniform>::value);
    CORRADE_VERIFY(std::is_nothrow_constructible<TextLayerStyleUniform, DefaultInitT>::value);

    /* Implicit construction is not allowed */
    CORRADE_VERIFY(!std::is_convertible<DefaultInitT, TextLayerStyleUniform>::value);
}

void TextLayerTest::styleUniformConstruct() {
    TextLayerStyleUniform a{0xff336699_rgbaf};
    CORRADE_COMPARE(a.color, 0xff336699_rgbaf);

    constexpr TextLayerStyleUniform ca{0xff336699_rgbaf};
    CORRADE_COMPARE(ca.color, 0xff336699_rgbaf);
}

void TextLayerTest::styleUniformConstructNoInit() {
    /* Testing only some fields, should be enough */
    TextLayerStyleUniform a;
    a.color = 0xff3366_rgbf;

    new(&a) TextLayerStyleUniform{NoInit};
    {
        /* Explicitly check we're not on Clang because certain Clang-based IDEs
           inherit __GNUC__ if GCC is used instead of leaving it at 4 like
           Clang itself does */
        #if defined(CORRADE_TARGET_GCC) && !defined(CORRADE_TARGET_CLANG) && __GNUC__*100 + __GNUC_MINOR__ >= 601 && __OPTIMIZE__
        CORRADE_EXPECT_FAIL("GCC 6.1+ misoptimizes and overwrites the value.");
        #endif
        CORRADE_COMPARE(a.color, 0xff3366_rgbf);
    }
}

void TextLayerTest::styleUniformSetters() {
    TextLayerStyleUniform a;
    a.setColor(0xff336699_rgbaf);
    CORRADE_COMPARE(a.color, 0xff336699_rgbaf);
}

void TextLayerTest::fontHandle() {
    CORRADE_COMPARE(FontHandle::Null, FontHandle{});
    CORRADE_COMPARE(Whee::fontHandle(0, 0), FontHandle::Null);
    CORRADE_COMPARE(Whee::fontHandle(0x2bcd, 0x1), FontHandle(0xabcd));
    CORRADE_COMPARE(Whee::fontHandle(0x7fff, 0x1), FontHandle(0xffff));
    CORRADE_COMPARE(fontHandleId(FontHandle::Null), 0);
    CORRADE_COMPARE(fontHandleId(FontHandle(0xabcd)), 0x2bcd);
    CORRADE_COMPARE(fontHandleGeneration(FontHandle::Null), 0);
    CORRADE_COMPARE(fontHandleGeneration(FontHandle(0xabcd)), 0x1);

    constexpr FontHandle handle = Whee::fontHandle(0x2bcd, 0x1);
    constexpr UnsignedInt id = fontHandleId(handle);
    constexpr UnsignedInt generation = fontHandleGeneration(handle);
    CORRADE_COMPARE(handle, FontHandle(0xabcd));
    CORRADE_COMPARE(id, 0x2bcd);
    CORRADE_COMPARE(generation, 0x1);
}

void TextLayerTest::fontHandleInvalid() {
    CORRADE_SKIP_IF_NO_DEBUG_ASSERT();

    std::ostringstream out;
    Error redirectError{&out};
    Whee::fontHandle(0x8000, 0x1);
    Whee::fontHandle(0x1, 0x2);
    CORRADE_COMPARE(out.str(),
        "Whee::fontHandle(): expected index to fit into 15 bits and generation into 1, got 0x8000 and 0x1\n"
        "Whee::fontHandle(): expected index to fit into 15 bits and generation into 1, got 0x1 and 0x2\n");
}

void TextLayerTest::debugFontHandle() {
    std::ostringstream out;
    Debug{&out} << FontHandle::Null << Whee::fontHandle(0x2bcd, 0x1);
    CORRADE_COMPARE(out.str(), "Whee::FontHandle::Null Whee::FontHandle(0x2bcd, 0x1)\n");
}

void TextLayerTest::sharedConfigurationConstruct() {
    TextLayer::Shared::Configuration configuration{3, 5};
    CORRADE_COMPARE(configuration.styleUniformCount(), 3);
    CORRADE_COMPARE(configuration.styleCount(), 5);
}

void TextLayerTest::sharedConfigurationConstructSameStyleUniformCount() {
    TextLayer::Shared::Configuration configuration{3};
    CORRADE_COMPARE(configuration.styleUniformCount(), 3);
    CORRADE_COMPARE(configuration.styleCount(), 3);
}

void TextLayerTest::sharedConfigurationConstructZeroStyleCount() {
    CORRADE_SKIP_IF_NO_ASSERT();

    std::ostringstream out;
    Error redirectError{&out};
    TextLayer::Shared::Configuration{0, 4};
    TextLayer::Shared::Configuration{4, 0};
    CORRADE_COMPARE(out.str(),
        "Whee::TextLayer::Shared::Configuration: expected non-zero style uniform count\n"
        "Whee::TextLayer::Shared::Configuration: expected non-zero style count\n");
}

void TextLayerTest::sharedConfigurationConstructCopy() {
    TextLayer::Shared::Configuration a{3, 5};

    TextLayer::Shared::Configuration b = a;
    CORRADE_COMPARE(b.styleUniformCount(), 3);
    CORRADE_COMPARE(b.styleCount(), 5);

    TextLayer::Shared::Configuration c{7, 9};
    c = b;
    CORRADE_COMPARE(c.styleUniformCount(), 3);
    CORRADE_COMPARE(c.styleCount(), 5);

    #ifndef CORRADE_NO_STD_IS_TRIVIALLY_TRAITS
    CORRADE_VERIFY(std::is_trivially_copy_constructible<TextLayer::Shared::Configuration>::value);
    CORRADE_VERIFY(std::is_trivially_copy_assignable<TextLayer::Shared::Configuration>::value);
    #endif
}

void TextLayerTest::sharedConstruct() {
    struct Shared: TextLayer::Shared {
        explicit Shared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{3, 5}};
    CORRADE_COMPARE(shared.styleUniformCount(), 3);
    CORRADE_COMPARE(shared.styleCount(), 5);
    CORRADE_COMPARE(shared.dynamicStyleCount(), 0);

    CORRADE_VERIFY(!shared.hasGlyphCache());

    CORRADE_COMPARE(shared.fontCount(), 0);
    CORRADE_VERIFY(!shared.isHandleValid(FontHandle::Null));
}

void TextLayerTest::sharedConstructNoCreate() {
    struct Shared: TextLayer::Shared {
        explicit Shared(NoCreateT): TextLayer::Shared{NoCreate} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{NoCreate};

    /* Shouldn't crash */
    CORRADE_VERIFY(true);

    /* Implicit construction is not allowed */
    CORRADE_VERIFY(!std::is_convertible<NoCreateT, TextLayer::Shared>::value);
}

void TextLayerTest::sharedConstructCopy() {
    struct Shared: TextLayer::Shared {
        /* Clang says the constructor is unused otherwise. However I fear that
           without the constructor the type would be impossible to construct
           (and thus also to copy), leading to false positives in the trait
           check below */
        explicit CORRADE_UNUSED Shared(const Configuration& configuration): TextLayer::Shared{Containers::pointer<TextLayer::Shared::State>(*this, configuration)} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    };

    CORRADE_VERIFY(!std::is_copy_constructible<Shared>{});
    CORRADE_VERIFY(!std::is_copy_assignable<Shared>{});
}

void TextLayerTest::sharedConstructMove() {
    struct Shared: TextLayer::Shared {
        explicit Shared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    };

    Shared a{TextLayer::Shared::Configuration{3, 5}};

    Shared b{Utility::move(a)};
    CORRADE_COMPARE(b.styleUniformCount(), 3);
    CORRADE_COMPARE(b.styleCount(), 5);

    Shared c{TextLayer::Shared::Configuration{5, 7}};
    c = Utility::move(b);
    CORRADE_COMPARE(c.styleUniformCount(), 3);
    CORRADE_COMPARE(c.styleCount(), 5);

    CORRADE_VERIFY(std::is_nothrow_move_constructible<Shared>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<Shared>::value);
}

void TextLayerTest::sharedSetGlyphCache() {
    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};

    struct Shared: TextLayer::Shared {
        explicit Shared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        using TextLayer::Shared::setGlyphCache;

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{3, 5}};
    CORRADE_VERIFY(!shared.hasGlyphCache());

    shared.setGlyphCache(cache);
    CORRADE_VERIFY(shared.hasGlyphCache());
    CORRADE_COMPARE(&shared.glyphCache(), &cache);
    /* Const overload */
    CORRADE_COMPARE(&const_cast<const Shared&>(shared).glyphCache(), &cache);
}

void TextLayerTest::sharedSetGlyphCacheAlreadySet() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};

    struct Shared: TextLayer::Shared {
        explicit Shared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        using TextLayer::Shared::setGlyphCache;

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{3, 5}};
    shared.setGlyphCache(cache);
    CORRADE_VERIFY(shared.hasGlyphCache());

    std::ostringstream out;
    Error redirectError{&out};
    shared.setGlyphCache(cache);
    CORRADE_COMPARE(out.str(), "Whee::TextLayer::Shared::setGlyphCache(): glyph cache already set\n");
}

void TextLayerTest::sharedNoGlyphCache() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Shared: TextLayer::Shared {
        explicit Shared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{3, 5}};

    std::ostringstream out;
    Error redirectError{&out};
    shared.glyphCache();
    /* Const overload */
    const_cast<const Shared&>(shared).glyphCache();
    CORRADE_COMPARE(out.str(),
        "Whee::TextLayer::Shared::glyphCache(): no glyph cache set\n"
        "Whee::TextLayer::Shared::glyphCache(): no glyph cache set\n");
}

void TextLayerTest::sharedAddFont() {
    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};

    struct Shared: TextLayer::Shared {
        explicit Shared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        using TextLayer::Shared::setGlyphCache;

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{3, 5}};
    shared.setGlyphCache(cache);
    CORRADE_COMPARE(shared.fontCount(), 0);

    struct
        /* MSVC 2017 (_MSC_VER == 191x) crashes at runtime accessing instance2
           in the following case. Not a problem in MSVC 2015 or 2019+.
            struct: SomeBase {
            } instance1, instance2;
           Simply naming the derived struct is enough to fix the crash, FFS. */
        #if defined(CORRADE_TARGET_MSVC) && _MSC_VER >= 1910 && _MSC_VER < 1920
        ThisNameAlonePreventsMSVC2017FromBlowingUp
        #endif
        : Text::AbstractFont
    {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return {}; }
    } font1, font2;

    /* First font */
    UnsignedInt firstFontId = cache.addFont(13, &font1);
    FontHandle first = shared.addFont(font1, 13.0f);
    CORRADE_COMPARE(first, Whee::fontHandle(0, 1));
    CORRADE_COMPARE(shared.fontCount(), 1);
    CORRADE_VERIFY(shared.isHandleValid(first));
    CORRADE_COMPARE(shared.glyphCacheFontId(first), firstFontId);
    CORRADE_VERIFY(shared.hasFontInstance(first));
    CORRADE_COMPARE(&shared.font(first), &font1);
    /* Const overload */
    CORRADE_COMPARE(&const_cast<const Shared&>(shared).font(first), &font1);

    /* Second font, instanceless */
    UnsignedInt secondFontId = cache.addFont(223);
    FontHandle second = shared.addInstancelessFont(secondFontId, 0.5f);
    CORRADE_COMPARE(second, Whee::fontHandle(1, 1));
    CORRADE_COMPARE(shared.fontCount(), 2);
    CORRADE_VERIFY(shared.isHandleValid(second));
    CORRADE_COMPARE(shared.glyphCacheFontId(second), secondFontId);
    CORRADE_VERIFY(!shared.hasFontInstance(second));

    /* Third font */
    UnsignedInt thirdFontId = cache.addFont(56, &font2);
    FontHandle third = shared.addFont(font2, 6.0f);
    CORRADE_COMPARE(third, Whee::fontHandle(2, 1));
    CORRADE_COMPARE(shared.fontCount(), 3);
    CORRADE_VERIFY(shared.isHandleValid(third));
    CORRADE_COMPARE(shared.glyphCacheFontId(third), thirdFontId);
    CORRADE_VERIFY(shared.hasFontInstance(third));
    CORRADE_COMPARE(&shared.font(third), &font2);
    /* Const overload */
    CORRADE_COMPARE(&const_cast<const Shared&>(shared).font(third), &font2);

    /* Fourth font, instanceless */
    UnsignedInt fourthFontId = cache.addFont(117);
    FontHandle fourth = shared.addInstancelessFont(fourthFontId, 2.0f);
    CORRADE_COMPARE(fourth, Whee::fontHandle(3, 1));
    CORRADE_COMPARE(shared.fontCount(), 4);
    CORRADE_VERIFY(shared.isHandleValid(fourth));
    CORRADE_COMPARE(shared.glyphCacheFontId(second), secondFontId);
    CORRADE_VERIFY(!shared.hasFontInstance(second));
}

void TextLayerTest::sharedAddFontTakeOwnership() {
    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};

    struct Font: Text::AbstractFont {
        explicit Font(Int& destructed): _destructed(destructed) {}

        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        ~Font() {
            ++_destructed;
        }

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return {}; }

        private:
            Int& _destructed;
    };

    Int destructed = 0;

    {
        struct Shared: TextLayer::Shared {
            explicit Shared(const Configuration& configuration): TextLayer::Shared{configuration} {}

            using TextLayer::Shared::setGlyphCache;

            void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        } shared{TextLayer::Shared::Configuration{3, 5}};
        shared.setGlyphCache(cache);
        CORRADE_COMPARE(shared.fontCount(), 0);

        Containers::Pointer<Font> font1{InPlaceInit, destructed};
        UnsignedInt firstFontId = cache.addFont(13, font1.get());
        Font* pointer1 = font1.get();
        FontHandle first = shared.addFont(Utility::move(font1), 13.0f);
        CORRADE_COMPARE(first, Whee::fontHandle(0, 1));
        CORRADE_COMPARE(shared.fontCount(), 1);
        CORRADE_VERIFY(shared.isHandleValid(first));
        CORRADE_COMPARE(shared.glyphCacheFontId(first), firstFontId);
        CORRADE_COMPARE(&shared.font(first), pointer1);

        /* It should be possible to add a second font using the same pointer
           but different options */
        FontHandle second = shared.addFont(*pointer1, 6.0f);
        CORRADE_COMPARE(second, Whee::fontHandle(1, 1));
        CORRADE_COMPARE(shared.fontCount(), 2);
        CORRADE_VERIFY(shared.isHandleValid(second));
        CORRADE_COMPARE(shared.glyphCacheFontId(second), firstFontId);
        CORRADE_COMPARE(&shared.font(second), pointer1);

        /* Add a second font, to verify both get deleted appropriately */
        Containers::Pointer<Font> font2{InPlaceInit, destructed};
        UnsignedInt thirdFontId = cache.addFont(13, font2.get());
        Font* pointer2 = font2.get();
        FontHandle third = shared.addFont(Utility::move(font2), 22.0f);
        CORRADE_COMPARE(third, Whee::fontHandle(2, 1));
        CORRADE_COMPARE(shared.fontCount(), 3);
        CORRADE_VERIFY(shared.isHandleValid(third));
        CORRADE_COMPARE(shared.glyphCacheFontId(third), thirdFontId);
        CORRADE_COMPARE(&shared.font(third), pointer2);
    }

    /* The owned instances should be destructed exactly once */
    CORRADE_COMPARE(destructed, 2);
}

void TextLayerTest::sharedAddFontTakeOwnershipNull() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Shared: TextLayer::Shared {
        explicit Shared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{3, 5}};
    CORRADE_COMPARE(shared.fontCount(), 0);

    std::ostringstream out;
    Error redirectError{&out};
    shared.addFont(nullptr, 13.0f);
    CORRADE_COMPARE(out.str(), "Whee::TextLayer::Shared::addFont(): font is null\n");
}

void TextLayerTest::sharedAddFontNoCache() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Shared: TextLayer::Shared {
        explicit Shared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{3, 5}};

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return false; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return {}; }
    } font;

    std::ostringstream out;
    Error redirectError{&out};
    shared.addFont(font, 1.0f);
    shared.addInstancelessFont(0, 0.5f);
    CORRADE_COMPARE(out.str(),
        "Whee::TextLayer::Shared::addFont(): no glyph cache set\n"
        "Whee::TextLayer::Shared::addInstancelessFont(): no glyph cache set\n");
}

void TextLayerTest::sharedAddFontNotFoundInCache() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};

    /* Add some other fonts to the cache to verify it's not just checking for
       the cache being non-empty */
    cache.addFont(67);
    cache.addFont(36);

    struct Shared: TextLayer::Shared {
        explicit Shared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        using TextLayer::Shared::setGlyphCache;

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{3, 5}};
    shared.setGlyphCache(cache);

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return false; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return {}; }
    } font;

    std::ostringstream out;
    Error redirectError{&out};
    shared.addFont(font, 1.0f);
    shared.addInstancelessFont(2, 1.0f);
    CORRADE_COMPARE(out.str(),
        "Whee::TextLayer::Shared::addFont(): font not found among 2 fonts in set glyph cache\n"
        "Whee::TextLayer::Shared::addInstancelessFont(): index 2 out of range for 2 fonts in set glyph cache\n");
}

void TextLayerTest::sharedAddFontNoHandlesLeft() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return {}; }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(67, &font);

    UnsignedInt glyphCacheInstanceLessFontId = cache.addFont(223);

    struct Shared: TextLayer::Shared {
        explicit Shared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        using TextLayer::Shared::setGlyphCache;

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{3, 5}};
    shared.setGlyphCache(cache);

    FontHandle handle;
    for(std::size_t i = 0; i != 1 << Implementation::FontHandleIdBits; ++i)
        handle = shared.addFont(font, 1.0f);
    CORRADE_COMPARE(handle, Whee::fontHandle((1 << Implementation::FontHandleIdBits) - 1, 1));

    CORRADE_COMPARE(shared.fontCount(), 1 << Implementation::FontHandleIdBits);

    std::ostringstream out;
    Error redirectError{&out};
    shared.addFont(font, 1.0f);
    shared.addInstancelessFont(glyphCacheInstanceLessFontId, 1.0f);
    /* Number is hardcoded in the expected message but not elsewhere in order
       to give a heads-up when modifying the handle ID bit count */
    CORRADE_COMPARE(out.str(),
        "Whee::TextLayer::Shared::addFont(): can only have at most 32768 fonts\n"
        "Whee::TextLayer::Shared::addInstancelessFont(): can only have at most 32768 fonts\n");
}

void TextLayerTest::sharedAddInstancelessFontHasInstance() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return false; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return {}; }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};

    /* Add a font without an instance to check it's looking at the correct
       one */
    cache.addFont(223);
    UnsignedInt glyphCacheFontId = cache.addFont(67, &font);

    struct Shared: TextLayer::Shared {
        explicit Shared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        using TextLayer::Shared::setGlyphCache;

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{3, 5}};
    shared.setGlyphCache(cache);

    std::ostringstream out;
    Error redirectError{&out};
    shared.addInstancelessFont(glyphCacheFontId, 1.0f);
    CORRADE_COMPARE(out.str(),
        "Whee::TextLayer::Shared::addInstancelessFont(): glyph cache font 1 has an instance set\n");
}

void TextLayerTest::sharedFontInvalidHandle() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return {}; }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(67, &font);

    struct Shared: TextLayer::Shared {
        explicit Shared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        using TextLayer::Shared::setGlyphCache;

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{3, 5}};
    shared.setGlyphCache(cache);

    /* Need to add at least one font because the assertion returns the first
       font as a fallback */
    shared.addFont(font, 13.0f);

    std::ostringstream out;
    Error redirectError{&out};
    shared.glyphCacheFontId(FontHandle(0x12ab));
    shared.glyphCacheFontId(FontHandle::Null);
    shared.hasFontInstance(FontHandle(0x12ab));
    shared.hasFontInstance(FontHandle::Null);
    shared.font(FontHandle(0x12ab));
    shared.font(FontHandle::Null);
    /* Const overload */
    const_cast<const Shared&>(shared).font(FontHandle(0x12ab));
    const_cast<const Shared&>(shared).font(FontHandle::Null);
    CORRADE_COMPARE(out.str(),
        "Whee::TextLayer::Shared::glyphCacheFontId(): invalid handle Whee::FontHandle(0x12ab, 0x0)\n"
        "Whee::TextLayer::Shared::glyphCacheFontId(): invalid handle Whee::FontHandle::Null\n"
        "Whee::TextLayer::Shared::hasFontInstance(): invalid handle Whee::FontHandle(0x12ab, 0x0)\n"
        "Whee::TextLayer::Shared::hasFontInstance(): invalid handle Whee::FontHandle::Null\n"
        "Whee::TextLayer::Shared::font(): invalid handle Whee::FontHandle(0x12ab, 0x0)\n"
        "Whee::TextLayer::Shared::font(): invalid handle Whee::FontHandle::Null\n"
        "Whee::TextLayer::Shared::font(): invalid handle Whee::FontHandle(0x12ab, 0x0)\n"
        "Whee::TextLayer::Shared::font(): invalid handle Whee::FontHandle::Null\n");
}

void TextLayerTest::sharedFontNoInstance() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return {}; }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(67, &font);

    UnsignedInt glyphCacheInstanceLessFontId = cache.addFont(233);

    struct Shared: TextLayer::Shared {
        explicit Shared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        using TextLayer::Shared::setGlyphCache;

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{3, 5}};
    shared.setGlyphCache(cache);

    /* Need to add at least one font with an instance because the assertion
       returns the first font as a fallback */
    shared.addFont(font, 13.0f);

    FontHandle instanceless = shared.addInstancelessFont(glyphCacheInstanceLessFontId, 0.3f);
    CORRADE_VERIFY(!shared.hasFontInstance(instanceless));

    std::ostringstream out;
    Error redirectError{&out};
    shared.font(instanceless);
    CORRADE_COMPARE(out.str(), "Whee::TextLayer::Shared::font(): Whee::FontHandle(0x1, 0x1) is an instance-less font\n");
}

void TextLayerTest::sharedSetStyle() {
    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};

    struct Font: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return {}; }
    };

    struct Shared: TextLayer::Shared {
        explicit Shared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        State& state() { return static_cast<State&>(*_state); }
        using TextLayer::Shared::setGlyphCache;

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform> uniforms) override {
            /** @todo test the common style once it contains something */
            CORRADE_COMPARE(uniforms.size(), 3);
            CORRADE_COMPARE(uniforms[1].color, 0xc0ffee_rgbf);
            ++setStyleCalled;
        }

        Int setStyleCalled = 0;
    } shared{TextLayer::Shared::Configuration{3, 5}};
    shared.setGlyphCache(cache);

    /* By default the shared.state().styles array is empty, it gets only filled
       during the setStyle() call. The empty state is used to detect whether
       setStyle() was called at all when calling update(). */
    CORRADE_VERIFY(shared.state().styles.isEmpty());

    Font font1, font2;
    cache.addFont(67, &font1);
    cache.addFont(23, &font2);
    FontHandle first = shared.addFont(font1, 13.0f);
    FontHandle second = shared.addFont(font2, 6.0f);
    shared.setStyle(
        TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{},
         TextLayerStyleUniform{}
            .setColor(0xc0ffee_rgbf),
         TextLayerStyleUniform{}},
        {2, 1, 0, 0, 1},
        {first, second, first, second, second},
        {{1.0f, 2.0f, 3.0f, 4.0f},
         {4.0f, 3.0f, 2.0f, 1.0f},
         {2.0f, 1.0f, 4.0f, 3.0f},
         {1.0f, 3.0f, 2.0f, 4.0f},
         {4.0f, 1.0f, 3.0f, 2.0f}});
    CORRADE_COMPARE(shared.setStyleCalled, 1);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::uniform), Containers::stridedArrayView({
        2u, 1u, 0u, 0u, 1u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::font), Containers::stridedArrayView({
        first,
        second,
        first,
        second,
        second
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::padding), Containers::stridedArrayView({
        Vector4{1.0f, 2.0f, 3.0f, 4.0f},
        Vector4{4.0f, 3.0f, 2.0f, 1.0f},
        Vector4{2.0f, 1.0f, 4.0f, 3.0f},
        Vector4{1.0f, 3.0f, 2.0f, 4.0f},
        Vector4{4.0f, 1.0f, 3.0f, 2.0f}
    }), TestSuite::Compare::Container);
}

void TextLayerTest::sharedSetStyleImplicitPadding() {
    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};

    struct Font: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return {}; }
    };

    struct Shared: TextLayer::Shared {
        explicit Shared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        State& state() { return static_cast<State&>(*_state); }
        using TextLayer::Shared::setGlyphCache;

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform> uniforms) override {
            /** @todo test the common style once it contains something */
            CORRADE_COMPARE(uniforms.size(), 3);
            CORRADE_COMPARE(uniforms[1].color, 0xc0ffee_rgbf);
            ++setStyleCalled;
        }

        Int setStyleCalled = 0;
    } shared{TextLayer::Shared::Configuration{3, 5}};
    shared.setGlyphCache(cache);

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Font font1, font2;
    cache.addFont(67, &font1);
    cache.addFont(23, &font2);
    FontHandle first = shared.addFont(font1, 13.0f);
    FontHandle second = shared.addFont(font2, 6.0f);
    shared.setStyle(
        TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{},
         TextLayerStyleUniform{}
            .setColor(0xc0ffee_rgbf),
         TextLayerStyleUniform{}},
        {2, 1, 0, 0, 1},
        {first, second, first, second, second},
        {});
    CORRADE_COMPARE(shared.setStyleCalled, 1);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::uniform), Containers::stridedArrayView({
        2u, 1u, 0u, 0u, 1u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::font), Containers::stridedArrayView({
        first,
        second,
        first,
        second,
        second
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::padding), Containers::stridedArrayView({
        Vector4{},
        Vector4{},
        Vector4{},
        Vector4{},
        Vector4{}
    }), TestSuite::Compare::Container);

    /* Setting a style with implicit padding after a non-implicit padding was
       set should reset it back to zeros */
    shared.setStyle(
        TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{},
         TextLayerStyleUniform{}
            .setColor(0xc0ffee_rgbf),
         TextLayerStyleUniform{}},
        {2, 1, 0, 0, 1},
        {first, second, first, second, second},
        {{1.0f, 2.0f, 3.0f, 4.0f},
         {4.0f, 3.0f, 2.0f, 1.0f},
         {2.0f, 1.0f, 4.0f, 3.0f},
         {1.0f, 3.0f, 2.0f, 4.0f},
         {4.0f, 1.0f, 3.0f, 2.0f}});
    shared.setStyle(
        TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{},
         TextLayerStyleUniform{}
            .setColor(0xc0ffee_rgbf),
         TextLayerStyleUniform{}},
        {2, 1, 0, 0, 1},
        {first, second, first, second, second},
        {});
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::padding), Containers::stridedArrayView({
        Vector4{},
        Vector4{},
        Vector4{},
        Vector4{},
        Vector4{}
    }), TestSuite::Compare::Container);
}

void TextLayerTest::sharedSetStyleInvalidSize() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Shared: TextLayer::Shared {
        explicit Shared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{3, 5}};

    std::ostringstream out;
    Error redirectError{&out};
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}, TextLayerStyleUniform{}},
        {0, 1, 2, 3, 4},
        {FontHandle::Null, FontHandle::Null, FontHandle::Null, FontHandle::Null, FontHandle::Null},
        {{}, {}, {}, {}, {}});
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}, TextLayerStyleUniform{}, TextLayerStyleUniform{}},
        {0, 1, 2},
        {FontHandle::Null, FontHandle::Null, FontHandle::Null, FontHandle::Null, FontHandle::Null},
        {{}, {}, {}, {}, {}});
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}, TextLayerStyleUniform{}, TextLayerStyleUniform{}},
        {0, 1, 2, 3, 4},
        {FontHandle::Null, FontHandle::Null, FontHandle::Null},
        {{}, {}, {}, {}, {}});
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}, TextLayerStyleUniform{}, TextLayerStyleUniform{}},
        {0, 1, 2, 3, 4},
        {FontHandle::Null, FontHandle::Null, FontHandle::Null, FontHandle::Null, FontHandle::Null},
        {{}, {}, {}});
    CORRADE_COMPARE(out.str(),
        "Whee::TextLayer::Shared::setStyle(): expected 3 uniforms, got 2\n"
        "Whee::TextLayer::Shared::setStyle(): expected 5 style uniform indices, got 3\n"
        "Whee::TextLayer::Shared::setStyle(): expected 5 font handles, got 3\n"
        "Whee::TextLayer::Shared::setStyle(): expected either no or 5 paddings, got 3\n");
}

void TextLayerTest::sharedSetStyleImplicitMapping() {
    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};

    struct Font: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return {}; }
    };

    struct Shared: TextLayer::Shared {
        explicit Shared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        State& state() { return static_cast<State&>(*_state); }
        using TextLayer::Shared::setGlyphCache;

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform> uniforms) override {
            /** @todo test the common style once it contains something */
            CORRADE_COMPARE(uniforms.size(), 3);
            CORRADE_COMPARE(uniforms[1].color, 0xc0ffee_rgbf);
            ++setStyleCalled;
        }

        Int setStyleCalled = 0;
    } shared{TextLayer::Shared::Configuration{3}};
    shared.setGlyphCache(cache);

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Font font1, font2;
    cache.addFont(67, &font1);
    cache.addFont(23, &font2);
    FontHandle first = shared.addFont(font1, 13.0f);
    FontHandle second = shared.addFont(font2, 6.0f);
    shared.setStyle(
        TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{},
         TextLayerStyleUniform{}
            .setColor(0xc0ffee_rgbf),
         TextLayerStyleUniform{}},
        {first, second, first},
        {{1.0f, 2.0f, 3.0f, 4.0f},
         {4.0f, 3.0f, 2.0f, 1.0f},
         {2.0f, 1.0f, 4.0f, 3.0f}});
    CORRADE_COMPARE(shared.setStyleCalled, 1);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::uniform), Containers::stridedArrayView({
        0u, 1u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::font), Containers::stridedArrayView({
        first,
        second,
        first
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::padding), Containers::stridedArrayView({
        Vector4{1.0f, 2.0f, 3.0f, 4.0f},
        Vector4{4.0f, 3.0f, 2.0f, 1.0f},
        Vector4{2.0f, 1.0f, 4.0f, 3.0f}
    }), TestSuite::Compare::Container);
}

void TextLayerTest::sharedSetStyleImplicitMappingImplicitPadding() {
    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};

    struct Font: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return {}; }
    };

    struct Shared: TextLayer::Shared {
        explicit Shared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        State& state() { return static_cast<State&>(*_state); }
        using TextLayer::Shared::setGlyphCache;

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform> uniforms) override {
            /** @todo test the common style once it contains something */
            CORRADE_COMPARE(uniforms.size(), 3);
            CORRADE_COMPARE(uniforms[1].color, 0xc0ffee_rgbf);
            ++setStyleCalled;
        }

        Int setStyleCalled = 0;
    } shared{TextLayer::Shared::Configuration{3}};
    shared.setGlyphCache(cache);

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Font font1, font2;
    cache.addFont(67, &font1);
    cache.addFont(23, &font2);
    FontHandle first = shared.addFont(font1, 13.0f);
    FontHandle second = shared.addFont(font2, 6.0f);
    shared.setStyle(
        TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{},
         TextLayerStyleUniform{}
            .setColor(0xc0ffee_rgbf),
         TextLayerStyleUniform{}},
        {first, second, first},
        {});
    CORRADE_COMPARE(shared.setStyleCalled, 1);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::uniform), Containers::stridedArrayView({
        0u, 1u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::font), Containers::stridedArrayView({
        first,
        second,
        first
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::padding), Containers::stridedArrayView({
        Vector4{},
        Vector4{},
        Vector4{}
    }), TestSuite::Compare::Container);

    /* Setting a style with implicit padding after a non-implicit padding was
       set should reset it back to zeros */
    shared.setStyle(
        TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{},
         TextLayerStyleUniform{}
            .setColor(0xc0ffee_rgbf),
         TextLayerStyleUniform{}},
        {first, second, first},
        {{1.0f, 2.0f, 3.0f, 4.0f},
         {4.0f, 3.0f, 2.0f, 1.0f},
         {2.0f, 1.0f, 4.0f, 3.0f}});
    shared.setStyle(
        TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{},
         TextLayerStyleUniform{}
            .setColor(0xc0ffee_rgbf),
         TextLayerStyleUniform{}},
        {first, second, first},
        {});
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::padding), Containers::stridedArrayView({
        Vector4{},
        Vector4{},
        Vector4{}
    }), TestSuite::Compare::Container);
}

void TextLayerTest::sharedSetStyleImplicitMappingInvalidSize() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Shared: TextLayer::Shared {
        explicit Shared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{3, 5}};

    std::ostringstream out;
    Error redirectError{&out};
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}, TextLayerStyleUniform{}, TextLayerStyleUniform{}},
        {FontHandle::Null, FontHandle::Null, FontHandle::Null, FontHandle::Null, FontHandle::Null},
        {{}, {}, {}, {}, {}});
    CORRADE_COMPARE(out.str(),
        "Whee::TextLayer::Shared::setStyle(): there's 3 uniforms for 5 styles, provide an explicit mapping\n");
}

void TextLayerTest::sharedSetStyleInvalidFontHandle() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return {}; }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(67, &font);

    struct Shared: TextLayer::Shared {
        explicit Shared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        using TextLayer::Shared::setGlyphCache;

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{4}};
    shared.setGlyphCache(cache);

    FontHandle handle = shared.addFont(font, 13.0f);

    /* Setting a null handle is okay, but create() etc that uses given style
       then has to explicitly pass a font handle */
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}, TextLayerStyleUniform{}, TextLayerStyleUniform{}, TextLayerStyleUniform{}},
        {handle, handle, FontHandle::Null, handle},
        {});

    std::ostringstream out;
    Error redirectError{&out};
    /* Testing just the implicit mapping variant, as both variants delegate to
       the same internal helper */
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}, TextLayerStyleUniform{}, TextLayerStyleUniform{}, TextLayerStyleUniform{}},
        {handle, FontHandle(0x12ab), handle, handle},
        {});
    CORRADE_COMPARE(out.str(),
        "Whee::TextLayer::Shared::setStyle(): invalid handle Whee::FontHandle(0x12ab, 0x0) at index 1\n");
}

void TextLayerTest::construct() {
    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{3, 5}};

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(137, 0xfe), shared};

    CORRADE_COMPARE(layer.handle(), layerHandle(137, 0xfe));
    CORRADE_COMPARE(&layer.shared(), &shared);
    /* Const overload */
    CORRADE_COMPARE(&static_cast<const Layer&>(layer).shared(), &shared);
}

void TextLayerTest::constructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<TextLayer>{});
    CORRADE_VERIFY(!std::is_copy_assignable<TextLayer>{});
}

void TextLayerTest::constructMove() {
    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    };

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, LayerShared& shared): TextLayer{handle, shared} {}
    };

    LayerShared shared{TextLayer::Shared::Configuration{1, 3}};
    LayerShared shared2{TextLayer::Shared::Configuration{5, 7}};

    Layer a{layerHandle(137, 0xfe), shared};

    Layer b{Utility::move(a)};
    CORRADE_COMPARE(b.handle(), layerHandle(137, 0xfe));
    CORRADE_COMPARE(&b.shared(), &shared);

    Layer c{layerHandle(0, 2), shared2};
    c = Utility::move(b);
    CORRADE_COMPARE(c.handle(), layerHandle(137, 0xfe));
    CORRADE_COMPARE(&c.shared(), &shared);

    CORRADE_VERIFY(std::is_nothrow_move_constructible<TextLayer>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<TextLayer>::value);
}

struct ThreeGlyphShaper: Text::AbstractShaper {
    using Text::AbstractShaper::AbstractShaper;

    UnsignedInt doShape(Containers::StringView text, UnsignedInt, UnsignedInt, Containers::ArrayView<const Text::FeatureRange>) override {
        return text.size();
    }
    void doGlyphIdsInto(const Containers::StridedArrayView1D<UnsignedInt>& ids) const override {
        /* Just cycling through three glyphs */
        for(std::size_t i = 0; i != ids.size(); ++i) {
            if(i % 3 == 0)
                ids[i] = 22;
            else if(i % 3 == 1)
                ids[i] = 13;
            else if(i % 3 == 2)
                ids[i] = 97;
            else CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        }
    }
    void doGlyphOffsetsAdvancesInto(const Containers::StridedArrayView1D<Vector2>& offsets, const Containers::StridedArrayView1D<Vector2>& advances) const override {
        /* Each next glyph has the advance and offset higher */
        for(std::size_t i = 0; i != offsets.size(); ++i) {
            offsets[i] = {Float(i), 1.0f + Float(i)};
            advances[i] = {2.0f + Float(i), 0.0f};
        }
    }
    void doGlyphClustersInto(const Containers::StridedArrayView1D<UnsignedInt>&) const override {
        /** @todo implement when it actually does get called for cursor /
            selection */
        CORRADE_FAIL("This shouldn't be called.");
    }
};

struct OneGlyphShaper: Text::AbstractShaper {
    using Text::AbstractShaper::AbstractShaper;

    UnsignedInt doShape(Containers::StringView, UnsignedInt, UnsignedInt, Containers::ArrayView<const Text::FeatureRange>) override {
        return 1;
    }
    void doGlyphIdsInto(const Containers::StridedArrayView1D<UnsignedInt>& ids) const override {
        ids[0] = 66;
    }
    void doGlyphOffsetsAdvancesInto(const Containers::StridedArrayView1D<Vector2>& offsets, const Containers::StridedArrayView1D<Vector2>& advances) const override {
        offsets[0] = {1.5f, -0.5f};
        advances[0] = {2.5f, 0.0f};
    }
    void doGlyphClustersInto(const Containers::StridedArrayView1D<UnsignedInt>&) const override {
        /** @todo implement when it actually does get called for cursor /
            selection */
        CORRADE_FAIL("This shouldn't be called.");
    }
};

template<class StyleIndex, class GlyphIndex> void TextLayerTest::createRemoveSet() {
    auto&& data = CreateRemoveSetData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName({
        std::is_same<StyleIndex, Enum>::value ? "Enum" : "UnsignedInt",
        std::is_same<GlyphIndex, Enum>::value ? "Enum" : "UnsignedInt"
    });

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return _opened; }
        Properties doOpenFile(Containers::StringView, Float size) override {
            _opened = true;
            return {size, 8.0f, -4.0f, 16.0f, 98};
        }
        void doClose() override { _opened = false; }

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return Containers::pointer<ThreeGlyphShaper>(*this); }

        bool _opened = false;
    } threeGlyphFont;
    threeGlyphFont.openFile({}, 16.0f);

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return _opened; }
        Properties doOpenFile(Containers::StringView, Float size) override {
            _opened = true;
            return {size, 1.0f, -0.5f, 2.0f, 67};
        }
        void doClose() override { _opened = false; }

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return Containers::pointer<OneGlyphShaper>(*this); }

        bool _opened = false;
    } oneGlyphFont;
    oneGlyphFont.openFile({}, 2.0f);

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    /* Default padding is 1, resetting to 0 for simplicity */
    } cache{PixelFormat::R8Unorm, {32, 32, 15}, {}};

    /* Glyph rectangle sizes in the glyph cache are only used for single-glyph
       data, text uses just the glyph ID mapping. Sizes, layers and offsets are
       only used in doUpdate() so they can be arbitrary. */
    cache.setInvalidGlyph({4, -2}, 7, {{16, 8}, {32, 32}});
    {
        UnsignedInt fontId = cache.addFont(threeGlyphFont.glyphCount(), &threeGlyphFont);
        cache.addGlyph(fontId, 97, {3000, 1000}, 13, {{7, 23}, {18, 30}});
        /* Glyph 22 deliberately omitted */
        cache.addGlyph(fontId, 13, {2, -4}, 6, {{8, 16}, {32, 32}});
    } {
        UnsignedInt fontId = cache.addFont(oneGlyphFont.glyphCount(), &oneGlyphFont);
        cache.addGlyph(fontId, 66, {2, -1}, 9, {{7, 8}, {15, 20}});
    }

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        using TextLayer::Shared::setGlyphCache;

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{3}};
    shared.setGlyphCache(cache);

    /* The three-glyph font is scaled to 0.5, the one-glyph to 2.0 */
    FontHandle threeGlyphFontHandle = shared.addFont(threeGlyphFont, 8.0f);
    FontHandle oneGlyphFontHandle = shared.addFont(oneGlyphFont, 4.0f);

    /* If using custom fonts, set the style to either something completely
       different or not set them at all -- they shouldn't get used for
       anything. Padding from the style is tested in setPadding() instead,
       effect of the style->uniform mapping in updateCleanDataOrder()
       instead, here they're both implicit. */
    if(!data.noStyle) {
        TextLayerStyleUniform uniforms[3];
        FontHandle fonts[3];
        if(!data.customFont)
            Utility::copy({threeGlyphFontHandle, threeGlyphFontHandle, oneGlyphFontHandle}, fonts);
        else if(data.nullStyleFonts)
            Utility::copy({FontHandle::Null, FontHandle::Null, FontHandle::Null}, fonts);
        else
            Utility::copy({oneGlyphFontHandle, oneGlyphFontHandle, threeGlyphFontHandle}, fonts);
        shared.setStyle(TextLayerCommonStyleUniform{},
            uniforms,
            fonts,
            {});
    }

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}

        const State& stateData() const {
            return static_cast<const State&>(*_state);
        }
    } layer{layerHandle(0, 1), shared};

    /* Default color */
    DataHandle first = layer.create(
        StyleIndex(1),
        "hello",
        TextProperties{}
            .setFont(data.customFont ? threeGlyphFontHandle : FontHandle::Null),
        data.node);
    CORRADE_COMPARE(layer.node(first), data.node);
    CORRADE_COMPARE(layer.style(first), 1);
    CORRADE_COMPARE(layer.glyphCount(first), 5);
    CORRADE_COMPARE(layer.size(first), (Vector2{10.0f, 6.0f}));
    CORRADE_COMPARE(layer.color(first), 0xffffff_rgbf);
    CORRADE_COMPARE(layer.padding(first), Vector4{0.0f});
    CORRADE_COMPARE(layer.state(), data.state);

    /* Single (invalid) glyph */
    DataHandle firstGlyph = layer.createGlyph(
        StyleIndex(1),
        GlyphIndex(22),
        TextProperties{}
            .setFont(data.customFont ? threeGlyphFontHandle : FontHandle::Null),
        data.node);
    CORRADE_COMPARE(layer.node(firstGlyph), data.node);
    CORRADE_COMPARE(layer.style(firstGlyph), 1);
    CORRADE_COMPARE(layer.glyphCount(firstGlyph), 1);
    CORRADE_COMPARE(layer.size(firstGlyph), (Vector2{8.0f, 12.0f}));
    CORRADE_COMPARE(layer.color(firstGlyph), 0xffffff_rgbf);
    CORRADE_COMPARE(layer.padding(firstGlyph), Vector4{0.0f});
    CORRADE_COMPARE(layer.state(), data.state);

    /* Custom color, testing also the getter overloads and templates */
    DataHandle second = layer.create(
        StyleIndex(2),
        "ahoy",
        TextProperties{}
            .setFont(data.customFont ? oneGlyphFontHandle : FontHandle::Null),
        0xff3366_rgbf,
        data.node);
    CORRADE_COMPARE(layer.node(second), data.node);
    if(data.layerDataHandleOverloads) {
        CORRADE_COMPARE(layer.style(dataHandleData(second)), 2);
        /* Can't use StyleIndex, as the function restricts to enum types which
           would fail for StyleIndex == UnsignedInt */
        CORRADE_COMPARE(layer.template style<Enum>(dataHandleData(second)), Enum(2));
        CORRADE_COMPARE(layer.glyphCount(dataHandleData(second)), 1);
        CORRADE_COMPARE(layer.size(dataHandleData(second)), (Vector2{5.0f, 3.0f}));
        CORRADE_COMPARE(layer.color(dataHandleData(second)), 0xff3366_rgbf);
        CORRADE_COMPARE(layer.padding(dataHandleData(second)), Vector4{0.0f});
    } else {
        CORRADE_COMPARE(layer.style(second), 2);
        /* Can't use StyleIndex, as the function restricts to enum types which
           would fail for StyleIndex == UnsignedInt */
        CORRADE_COMPARE(layer.template style<Enum>(second), Enum(2));
        CORRADE_COMPARE(layer.glyphCount(second), 1);
        CORRADE_COMPARE(layer.size(second), (Vector2{5.0f, 3.0f}));
        CORRADE_COMPARE(layer.color(second), 0xff3366_rgbf);
        CORRADE_COMPARE(layer.padding(second), Vector4{0.0f});
    }
    CORRADE_COMPARE(layer.state(), data.state);

    /* Single glyph with custom color */
    DataHandle secondGlyph = layer.createGlyph(
        StyleIndex(2),
        GlyphIndex(66),
        TextProperties{}
            .setFont(data.customFont ? oneGlyphFontHandle : FontHandle::Null),
        0xff3366_rgbf,
        data.node);
    CORRADE_COMPARE(layer.node(secondGlyph), data.node);
    CORRADE_COMPARE(layer.style(secondGlyph), 2);
    CORRADE_COMPARE(layer.glyphCount(secondGlyph), 1);
    CORRADE_COMPARE(layer.size(secondGlyph), (Vector2{16.0f, 24.0f}));
    CORRADE_COMPARE(layer.color(secondGlyph), 0xff3366_rgbf);
    CORRADE_COMPARE(layer.padding(secondGlyph), Vector4{0.0f});
    CORRADE_COMPARE(layer.state(), data.state);

    /* Empty text */
    DataHandle third = layer.create(
        StyleIndex(1),
        "",
        TextProperties{}
            .setFont(data.customFont ? threeGlyphFontHandle : FontHandle::Null),
        data.node);
    CORRADE_COMPARE(layer.node(third), data.node);
    CORRADE_COMPARE(layer.style(third), 1);
    CORRADE_COMPARE(layer.glyphCount(third), 0);
    CORRADE_COMPARE(layer.size(third), (Vector2{0.0f, 6.0f}));
    CORRADE_COMPARE(layer.color(third), 0xffffff_rgbf);
    CORRADE_COMPARE(layer.padding(third), Vector4{0.0f});
    CORRADE_COMPARE(layer.state(), data.state);

    DataHandle fourth = layer.create(
        StyleIndex(0),
        "hi",
        TextProperties{}
            .setFont(data.customFont ? threeGlyphFontHandle : FontHandle::Null),
        data.node);
    CORRADE_COMPARE(layer.node(fourth), data.node);
    CORRADE_COMPARE(layer.style(fourth), 0);
    CORRADE_COMPARE(layer.glyphCount(fourth), 2);
    CORRADE_COMPARE(layer.size(fourth), (Vector2{2.5f, 6.0f}));
    CORRADE_COMPARE(layer.color(fourth), 0xffffff_rgbf);
    CORRADE_COMPARE(layer.padding(fourth), Vector4{0.0f});
    CORRADE_COMPARE(layer.state(), data.state);

    /* There should be four glyph runs, assigned to the four data */
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().data).slice(&Implementation::TextLayerData::glyphRun), Containers::arrayView({
        0u, 1u, 2u, 3u, 4u, 5u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::glyphOffset), Containers::arrayView({
        /* The second and third data is a singly glyph, `second` text is using
           the OneGlyphShaper, so it's just one glyph; `third` is empty */
        0u, 5u, 6u, 7u, 8u, 8u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::glyphCount), Containers::arrayView({
        /* The second and third data is a singly glyph, `second` text is using
           the OneGlyphShaper, so it's just one glyph; `third` is empty */
        5u, 1u, 1u, 1u, 0u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::data), Containers::arrayView({
        0u, 1u, 2u, 3u, 4u, 5u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphData).slice(&Implementation::TextLayerGlyphData::glyphId), Containers::arrayView({
        /* Glyphs 22, 13, 97, 22, 13; glyph 22 isn't in the cache */
        0u, 2u, 1u, 0u, 2u,
        /* Single (invalid) glyph 22 */
        0u,
        /* Glyph 66 */
        3u,
        /* Single glyph 66 */
        3u,
        /* Nothing for third text */
        /* Glyphs 22, 13 */
        0u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphData).slice(&Implementation::TextLayerGlyphData::position), Containers::arrayView<Vector2>({
        /* "hello", aligned to MiddleCenter */
        {-5.0f, -0.5f},
        {-3.5f,  0.0f},
        {-1.5f,  0.5f},
        { 1.0f,  1.0f},
        { 4.0f,  1.5f},
        /* Single (invalid) glyph 22. Its size is {16, 24} and offset {4, -2},
           scaled to 0.5, aligned to MiddleCenter */
        {-6.0f, -5.0f},
        /* "ahoy", single glyph */
        { 0.5f, -1.5f},
        /* Single glyph 66. Its size is {8, 12} and offset {2, -1},
           scaled to 2.0, aligned to MiddleCenter */
        {-12.0f, -10.0f},
        /* Third text is empty */
        /* "hi", aligned to MiddleCenter */
        {-1.25f, -0.5f},
        { 0.25f,  0.0f}
    }), TestSuite::Compare::Container);

    /* Removing a text marks the original run as unused, and as it's not
       attached to any node, also not any state flag. The remaining data don't
       need any refresh, they still draw correctly. */
    data.layerDataHandleOverloads ?
        layer.remove(dataHandleData(fourth)) :
        layer.remove(fourth);
    CORRADE_COMPARE(layer.state(), data.state|LayerState::NeedsDataClean);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().data).slice(&Implementation::TextLayerData::glyphRun), Containers::arrayView({
        0u, 1u, 2u, 3u, 4u, 5u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::glyphOffset), Containers::arrayView({
        0u, 5u, 6u, 7u, 8u, 0xffffffffu
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::glyphCount), Containers::arrayView({
        5u, 1u, 1u, 1u, 0u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::data), Containers::arrayView({
        0u, 1u, 2u, 3u, 4u, 5u
    }), TestSuite::Compare::Container);

    /* Modifying a text creates a new run at the end, marks the original run as
       unused and marks the layer as needing an update. It's possible to switch
       to a different font, and between a single-glyph and text data as well.
       In this case the `second` text from the one-glyph font becomes a single
       glyph, and `secondGlyph` glyph from the one-glyph font becomes a text,
       and they optionally switch to the three-glyph font as well. */
    if(data.layerDataHandleOverloads) {
        layer.setText(dataHandleData(secondGlyph), "hey",
            TextProperties{}
                .setFont(data.customFont ? threeGlyphFontHandle : FontHandle::Null));
        layer.setGlyph(dataHandleData(second),
            data.customFont ? GlyphIndex(13) : GlyphIndex(66),
            TextProperties{}
                .setFont(data.customFont ? threeGlyphFontHandle : FontHandle::Null));
    } else {
        layer.setText(secondGlyph, "hey",
            TextProperties{}
                .setFont(data.customFont ? threeGlyphFontHandle : FontHandle::Null));
        layer.setGlyph(second,
            data.customFont ? GlyphIndex(13) : GlyphIndex(66),
            TextProperties{}
                .setFont(data.customFont ? threeGlyphFontHandle : FontHandle::Null));
    }

    CORRADE_COMPARE(layer.state(), data.state|LayerState::NeedsDataClean|LayerState::NeedsDataUpdate);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().data).slice(&Implementation::TextLayerData::glyphRun), Containers::arrayView({
        0u, 1u, 7u, 6u, 4u, 5u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::glyphCount), Containers::arrayView({
        5u, 1u, 1u, 1u, 0u, 2u, data.customFont ? 3u : 1u, 1u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::data), Containers::arrayView({
        0u, 1u, 2u, 3u, 4u, 5u, 3u, 2u
    }), TestSuite::Compare::Container);
    if(data.customFont) {
        CORRADE_COMPARE(layer.glyphCount(secondGlyph), 3);
        CORRADE_COMPARE(layer.glyphCount(second), 1);
        CORRADE_COMPARE(layer.size(secondGlyph), (Vector2{4.5f, 6.0f}));
        CORRADE_COMPARE(layer.size(second), (Vector2{12.0f, 8.0f}));
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::glyphOffset), Containers::arrayView({
            0u, 5u, 0xffffffffu, 0xffffffffu, 8u, 0xffffffffu, 10u, 13u
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphData).slice(&Implementation::TextLayerGlyphData::glyphId), Containers::arrayView({
            /* Glyphs 22, 13, 97, 22, 13; glyph 22 isn't in the cache */
            0u, 2u, 1u, 0u, 2u,
            /* Single (invalid) glyph 22 */
            0u,
            /* Now-unused "ahoy" text */
            3u,
            /* Now-unused single glyph 66 */
            3u,
            /* Nothing for third text */
            /* Glyphs 22, 13 */
            0u, 2u,
            /* Glyphs 22, 13, 97; glyph 22 isn't in the cache */
            0u, 2u, 1u,
            /* Glyph 13 */
            2u
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphData).slice(&Implementation::TextLayerGlyphData::position), Containers::arrayView<Vector2>({
            /* "hello", aligned to MiddleCenter */
            {-5.0f, -0.5f},
            {-3.5f,  0.0f},
            {-1.5f,  0.5f},
            { 1.0f,  1.0f},
            { 4.0f,  1.5f},
            /* Single (invalid) glyph 22 */
            {-6.0f, -5.0f},
            /* Now-unused "ahoy" text */
            { 0.5f, -1.5f},
            /* Now-unused single glyph 66 */
            {-12.0f, -10.0f},
            /* Third text is empty */
            /* "hi", aligned to MiddleCenter */
            {-1.25f, -0.5f},
            { 0.25f,  0.0f},
            /* "hey", aligned to MiddleCenter */
            {-2.25f, -0.5f},
            {-0.75f,  0.0f},
            { 1.25f,  0.5f},
            /* Single glyph 13. Its size is {24, 16} and offset {2, -4},
               scaled to 0.5, aligned to MiddleCenter */
            {-7.0f, -2.0f}
        }), TestSuite::Compare::Container);
    } else {
        CORRADE_COMPARE(layer.glyphCount(secondGlyph), 1);
        CORRADE_COMPARE(layer.glyphCount(second), 1);
        CORRADE_COMPARE(layer.size(secondGlyph), (Vector2{5.0f, 3.0f}));
        CORRADE_COMPARE(layer.size(second), (Vector2{16.0f, 24.0f}));
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::glyphOffset), Containers::arrayView({
            0u, 5u, 0xffffffffu, 0xffffffffu, 8u, 0xffffffffu, 10u, 11u
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphData).slice(&Implementation::TextLayerGlyphData::glyphId), Containers::arrayView({
            /* Glyphs 22, 13, 97, 22, 13; glyph 22 isn't in the cache */
            0u, 2u, 1u, 0u, 2u,
            /* Single (invalid) glyph 22 */
            0u,
            /* Now-unused "ahoy" text */
            3u,
            /* Now-unused single glyph 66 */
            3u,
            /* Nothing for third text */
            /* Glyphs 22, 13 */
            0u, 2u,
            /* Glyph 66 */
            3u,
            /* Single glyph 66 */
            3u,
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphData).slice(&Implementation::TextLayerGlyphData::position), Containers::arrayView<Vector2>({
            /* "hello", aligned to MiddleCenter */
            {-5.0f, -0.5f},
            {-3.5f,  0.0f},
            {-1.5f,  0.5f},
            { 1.0f,  1.0f},
            { 4.0f,  1.5f},
            /* Single (invalid) glyph 22 */
            {-6.0f, -5.0f},
            /* Now-unused "ahoy" text */
            { 0.5f, -1.5f},
            /* Now-unused single glyph 66 */
            {-12.0f, -10.0f},
            /* Third text is empty */
            /* "hi", aligned to MiddleCenter */
            {-1.25f, -0.5f},
            { 0.25f,  0.0f},
            /* "hey", aligned to MiddleCenter */
            { 0.5f, -1.5f},
            /* Single glyph 66 again, aligned to MiddleCenter */
            {-12.0f, -10.0f},
        }), TestSuite::Compare::Container);
    }
}

void TextLayerTest::createRemoveHandleRecycle() {
    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return Containers::pointer<OneGlyphShaper>(*this); }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(67, &font);

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        using TextLayer::Shared::setGlyphCache;

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{1}};
    shared.setGlyphCache(cache);
    /* Interestingly enough, these two can't be chained together as on some
       compilers it'd call addFont() before setGlyphCache(), causing an
       assert */
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}},
        {shared.addFont(font, 1.0f)},
        {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    DataHandle first = layer.create(0, "hello", {});
    DataHandle second = layer.create(0, "again", {});
    layer.setPadding(first, Vector4{15.0f});
    layer.setPadding(second, Vector4{5.0f});
    CORRADE_COMPARE(layer.padding(first), Vector4{15.0f});
    CORRADE_COMPARE(layer.padding(second), Vector4{5.0f});

    /* Data that reuses a previous slot should have the padding cleared */
    layer.remove(second);
    DataHandle second2 = layer.create(0, "yes", {});
    CORRADE_COMPARE(dataHandleId(second2), dataHandleId(second));
    CORRADE_COMPARE(layer.padding(second2), Vector4{0.0f});

    /* Same for a glyph */
    layer.remove(first);
    DataHandle first2 = layer.createGlyph(0, 0, {});
    CORRADE_COMPARE(dataHandleId(first2), dataHandleId(first));
    CORRADE_COMPARE(layer.padding(first2), Vector4{0.0f});
}

void TextLayerTest::createSetTextTextProperties() {
    /* A font that just checks what has been sent to the shaper */
    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return _opened; }
        Properties doOpenFile(Containers::StringView, Float) override {
            _opened = true;
            return {1.0f, 1.0f, 1.0f, 2.0f, 1};
        }
        void doClose() override { _opened = false; }

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override {
            struct Shaper: Text::AbstractShaper {
                explicit Shaper(Text::AbstractFont& font, int& setScriptCalled, int& setLanguageCalled, int& setDirectionCalled, int& shapeCalled): Text::AbstractShaper{font}, setScriptCalled(setScriptCalled), setLanguageCalled(setLanguageCalled), setDirectionCalled(setDirectionCalled), shapeCalled(shapeCalled) {}

                bool doSetScript(Text::Script script) override {
                    CORRADE_COMPARE(script, Text::Script::HanifiRohingya);
                    ++setScriptCalled;
                    return true;
                }
                bool doSetLanguage(Containers::StringView language) override {
                    CORRADE_COMPARE(language, "eh-UH");
                    ++setLanguageCalled;
                    return true;
                }
                bool doSetDirection(Text::ShapeDirection direction) override {
                    CORRADE_COMPARE(direction, Text::ShapeDirection::BottomToTop);
                    ++setDirectionCalled;
                    return true;
                }
                UnsignedInt doShape(Containers::StringView, UnsignedInt, UnsignedInt, Containers::ArrayView<const Text::FeatureRange> features) override {
                    CORRADE_COMPARE(features.size(), 2);
                    CORRADE_COMPARE(features[0].feature(), Text::Feature::DiscretionaryLigatures);
                    CORRADE_COMPARE(features[0].begin(), 3);
                    CORRADE_COMPARE(features[0].end(), 5);
                    CORRADE_COMPARE(features[1].feature(), Text::Feature::Kerning);
                    CORRADE_VERIFY(!features[1].isEnabled());
                    ++shapeCalled;
                    return 0;
                }
                void doGlyphIdsInto(const Containers::StridedArrayView1D<UnsignedInt>&) const override {}
                void doGlyphOffsetsAdvancesInto(const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&) const override {}
                void doGlyphClustersInto(const Containers::StridedArrayView1D<UnsignedInt>&) const override {}

                int& setScriptCalled;
                int& setLanguageCalled;
                int& setDirectionCalled;
                int& shapeCalled;
            };

            return Containers::pointer<Shaper>(*this, setScriptCalled, setLanguageCalled, setDirectionCalled, shapeCalled);
        }

        int setScriptCalled = 0;
        int setLanguageCalled = 0;
        int setDirectionCalled = 0;
        int shapeCalled = 0;

        bool _opened = false;
    } font;
    font.openFile({}, 16.0f);

    /* A trivial glyph cache */
    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    /* Default padding is 1, resetting to 0 for simplicity */
    } cache{PixelFormat::R8Unorm, {32, 32}, {}};
    cache.addGlyph(cache.addFont(1, &font), 0, {}, {});

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        using TextLayer::Shared::setGlyphCache;

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{1}};
    shared.setGlyphCache(cache);

    FontHandle fontHandle = shared.addFont(font, 16.0f);
    shared.setStyle(TextLayerCommonStyleUniform{}, {TextLayerStyleUniform{}}, {fontHandle}, {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    DataHandle text = layer.create(0, "hello", TextProperties{}
        .setScript(Text::Script::HanifiRohingya)
        .setLanguage("eh-UH")
        .setShapeDirection(Text::ShapeDirection::BottomToTop)
        .setFeatures({
            {Text::Feature::DiscretionaryLigatures, 3, 5},
            {Text::Feature::Kerning, false}
        }));
    CORRADE_COMPARE(font.setScriptCalled, 1);
    CORRADE_COMPARE(font.setLanguageCalled, 1);
    CORRADE_COMPARE(font.setDirectionCalled, 1);
    CORRADE_COMPARE(font.shapeCalled, 1);

    /* setText() should do the same */
    layer.setText(text, "hello", TextProperties{}
        .setScript(Text::Script::HanifiRohingya)
        .setLanguage("eh-UH")
        .setShapeDirection(Text::ShapeDirection::BottomToTop)
        .setFeatures({
            {Text::Feature::DiscretionaryLigatures, 3, 5},
            {Text::Feature::Kerning, false}
        }));
    CORRADE_COMPARE(font.setScriptCalled, 2);
    CORRADE_COMPARE(font.setLanguageCalled, 2);
    CORRADE_COMPARE(font.setDirectionCalled, 2);
    CORRADE_COMPARE(font.shapeCalled, 2);

    /* createGlyph() doesn't call shape() at all */
    DataHandle glyph = layer.createGlyph(0, 0, {});
    layer.setGlyph(glyph, 0, {});
    CORRADE_COMPARE(font.setScriptCalled, 2);
    CORRADE_COMPARE(font.setLanguageCalled, 2);
    CORRADE_COMPARE(font.setDirectionCalled, 2);
    CORRADE_COMPARE(font.shapeCalled, 2);
}

void TextLayerTest::createNoSharedGlyphCache() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{2, 3}};

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    std::ostringstream out;
    Error redirectError{&out};
    layer.create(2, "", {});
    layer.createGlyph(1, 0, {});
    CORRADE_COMPARE(out.str(),
        "Whee::TextLayer::create(): no glyph cache was set\n"
        "Whee::TextLayer::createGlyph(): no glyph cache was set\n");
}

void TextLayerTest::setColor() {
    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return Containers::pointer<OneGlyphShaper>(*this); }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(67, &font);

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        using TextLayer::Shared::setGlyphCache;

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{1}};
    shared.setGlyphCache(cache);
    /* Interestingly enough, these two can't be chained together as on some
       compilers it'd call addFont() before setGlyphCache(), causing an
       assert */
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}},
        {shared.addFont(font, 1.0f)},
        {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    /* Just to be sure the setters aren't picking up the first ever data
       always */
    layer.create(0, "", {});

    /* There's nothing that would work differently for createGlyph() */
    DataHandle data = layer.create(0, "", {}, 0xff3366_rgbf);
    CORRADE_COMPARE(layer.color(data), 0xff3366_rgbf);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Setting a color marks the layer as dirty */
    layer.setColor(data, 0xaabbcc_rgbf);
    CORRADE_COMPARE(layer.color(data), 0xaabbcc_rgbf);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Testing also the other overload */
    layer.setColor(dataHandleData(data), 0x112233_rgbf);
    CORRADE_COMPARE(layer.color(data), 0x112233_rgbf);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
}

void TextLayerTest::setPadding() {
    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return Containers::pointer<OneGlyphShaper>(*this); }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(67, &font);

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        using TextLayer::Shared::setGlyphCache;

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{1}};
    shared.setGlyphCache(cache);
    /* Interestingly enough, these two can't be chained together as on some
       compilers it'd call addFont() before setGlyphCache(), causing an
       assert */
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}},
        {shared.addFont(font, 1.0f)},
        {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    /* Just to be sure the setters aren't picking up the first ever data
       always */
    layer.create(0, "", {});

    /* There's nothing that would work differently for createGlyph() */
    DataHandle data = layer.create(0, "", {});
    CORRADE_COMPARE(layer.padding(data), Vector4{0.0f});
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Setting a padding marks the layer as dirty */
    layer.setPadding(data, {2.0f, 4.0f, 3.0f, 1.0f});
    CORRADE_COMPARE(layer.padding(data), (Vector4{2.0f, 4.0f, 3.0f, 1.0f}));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Testing also the other overload */
    layer.setPadding(dataHandleData(data), {1.0f, 2.0f, 3.0f, 4.0f});
    CORRADE_COMPARE(layer.padding(dataHandleData(data)), (Vector4{1.0f, 2.0f, 3.0f, 4.0f}));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Single-value padding */
    layer.setPadding(data, 4.0f);
    CORRADE_COMPARE(layer.padding(data), Vector4{4.0f});
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Testing also the other overload */
    layer.setPadding(dataHandleData(data), 3.0f);
    CORRADE_COMPARE(layer.padding(dataHandleData(data)), Vector4{3.0f});
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
}

void TextLayerTest::invalidHandle() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        using TextLayer::Shared::setGlyphCache;

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{1}};

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    std::ostringstream out;
    Error redirectError{&out};
    layer.glyphCount(DataHandle::Null);
    layer.glyphCount(LayerDataHandle::Null);
    layer.size(DataHandle::Null);
    layer.size(LayerDataHandle::Null);
    layer.setText(DataHandle::Null, "", {});
    layer.setText(LayerDataHandle::Null, "", {});
    layer.setGlyph(DataHandle::Null, 0, {});
    layer.setGlyph(LayerDataHandle::Null, 0, {});
    layer.color(DataHandle::Null);
    layer.color(LayerDataHandle::Null);
    layer.setColor(DataHandle::Null, {});
    layer.setColor(LayerDataHandle::Null, {});
    layer.padding(DataHandle::Null);
    layer.padding(LayerDataHandle::Null);
    layer.setPadding(DataHandle::Null, {});
    layer.setPadding(LayerDataHandle::Null, {});
    CORRADE_COMPARE_AS(out.str(),
        "Whee::TextLayer::glyphCount(): invalid handle Whee::DataHandle::Null\n"
        "Whee::TextLayer::glyphCount(): invalid handle Whee::LayerDataHandle::Null\n"
        "Whee::TextLayer::size(): invalid handle Whee::DataHandle::Null\n"
        "Whee::TextLayer::size(): invalid handle Whee::LayerDataHandle::Null\n"
        "Whee::TextLayer::setText(): invalid handle Whee::DataHandle::Null\n"
        "Whee::TextLayer::setText(): invalid handle Whee::LayerDataHandle::Null\n"
        "Whee::TextLayer::setGlyph(): invalid handle Whee::DataHandle::Null\n"
        "Whee::TextLayer::setGlyph(): invalid handle Whee::LayerDataHandle::Null\n"
        "Whee::TextLayer::color(): invalid handle Whee::DataHandle::Null\n"
        "Whee::TextLayer::color(): invalid handle Whee::LayerDataHandle::Null\n"
        "Whee::TextLayer::setColor(): invalid handle Whee::DataHandle::Null\n"
        "Whee::TextLayer::setColor(): invalid handle Whee::LayerDataHandle::Null\n"
        "Whee::TextLayer::padding(): invalid handle Whee::DataHandle::Null\n"
        "Whee::TextLayer::padding(): invalid handle Whee::LayerDataHandle::Null\n"
        "Whee::TextLayer::setPadding(): invalid handle Whee::DataHandle::Null\n"
        "Whee::TextLayer::setPadding(): invalid handle Whee::LayerDataHandle::Null\n",
        TestSuite::Compare::String);
}

void TextLayerTest::invalidFontHandle() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return Containers::pointer<OneGlyphShaper>(*this); }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(67, &font);

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        using TextLayer::Shared::setGlyphCache;

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{1}};
    shared.setGlyphCache(cache);
    /* Interestingly enough, these two can't be chained together as on some
       compilers it'd call addFont() before setGlyphCache(), causing an
       assert */
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}},
        {shared.addFont(font, 1.0f)},
        {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    DataHandle data = layer.create(0, "", {});

    std::ostringstream out;
    Error redirectError{&out};
    layer.create(0, "", FontHandle(0x12ab));
    layer.createGlyph(0, 0, FontHandle(0x12ab));
    layer.setText(data, "", FontHandle(0x12ab));
    layer.setGlyph(data, 0, FontHandle(0x12ab));
    CORRADE_COMPARE(out.str(),
        "Whee::TextLayer::create(): invalid handle Whee::FontHandle(0x12ab, 0x0)\n"
        "Whee::TextLayer::createGlyph(): invalid handle Whee::FontHandle(0x12ab, 0x0)\n"
        "Whee::TextLayer::setText(): invalid handle Whee::FontHandle(0x12ab, 0x0)\n"
        "Whee::TextLayer::setGlyph(): invalid handle Whee::FontHandle(0x12ab, 0x0)\n");
}

void TextLayerTest::noSharedStyleFonts() {
    auto&& data = NoSharedStyleFontsData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return Containers::pointer<OneGlyphShaper>(*this); }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(67, &font);

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        using TextLayer::Shared::setGlyphCache;

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{4}};
    shared.setGlyphCache(cache);

    FontHandle fontHandle = shared.addFont(font, 1.0f);

    if(data.setStyle) shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}, TextLayerStyleUniform{}, TextLayerStyleUniform{}, TextLayerStyleUniform{}},
        {fontHandle, FontHandle::Null, fontHandle, FontHandle::Null},
        {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    DataHandle layerData = layer.create(1, "", fontHandle);

    std::ostringstream out;
    Error redirectError{&out};
    layer.create(1, "", {});
    layer.createGlyph(3, 0, {});
    layer.setText(layerData, "", {});
    layer.setGlyph(layerData, 1, {});
    CORRADE_COMPARE(out.str(),
        "Whee::TextLayer::create(): style 1 has no font set and no custom font was supplied\n"
        "Whee::TextLayer::createGlyph(): style 3 has no font set and no custom font was supplied\n"
        "Whee::TextLayer::setText(): style 1 has no font set and no custom font was supplied\n"
        "Whee::TextLayer::setGlyph(): style 1 has no font set and no custom font was supplied\n");
}

void TextLayerTest::noFontInstance() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    UnsignedInt glyphCacheInstanceLessFontId = cache.addFont(233);

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        using TextLayer::Shared::setGlyphCache;

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{1}};
    shared.setGlyphCache(cache);

    FontHandle fontHandle1 = shared.addInstancelessFont(glyphCacheInstanceLessFontId, 0.1f);
    FontHandle fontHandle2 = shared.addInstancelessFont(glyphCacheInstanceLessFontId, 0.1f);
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}},
        {fontHandle1},
        {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    DataHandle data = layer.createGlyph(0, 0, {});

    std::ostringstream out;
    Error redirectError{&out};
    layer.create(0, "", {});
    layer.create(0, "", fontHandle2);
    layer.setText(data, "", {});
    layer.setText(data, "", fontHandle2);
    CORRADE_COMPARE(out.str(),
        "Whee::TextLayer::create(): Whee::FontHandle(0x0, 0x1) is an instance-less font\n"
        "Whee::TextLayer::create(): Whee::FontHandle(0x1, 0x1) is an instance-less font\n"
        "Whee::TextLayer::setText(): Whee::FontHandle(0x0, 0x1) is an instance-less font\n"
        "Whee::TextLayer::setText(): Whee::FontHandle(0x1, 0x1) is an instance-less font\n");
}

void TextLayerTest::styleOutOfRange() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return {}; }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(56, &font);

    /* In this case the uniform count is higher than the style count, which is
       unlikely to happen in practice. It's to verify the check happens against
       the style count, not uniform count. */
    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        using TextLayer::Shared::setGlyphCache;

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{6, 3}};
    shared.setGlyphCache(cache);

    FontHandle fontHandle = shared.addFont(font, 1.0f);

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    std::ostringstream out;
    Error redirectError{&out};
    layer.create(3, "", TextProperties{}.setFont(fontHandle));
    layer.createGlyph(3, 0, TextProperties{}.setFont(fontHandle));
    CORRADE_COMPARE(out.str(),
        "Whee::TextLayer::create(): style 3 out of range for 3 styles\n"
        "Whee::TextLayer::createGlyph(): style 3 out of range for 3 styles\n");
}

void TextLayerTest::glyphOutOfRange() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};

    /* Add one more font to verify it's checking the right one */
    cache.addFont(57);
    UnsignedInt glyphCacheFontId = cache.addFont(56);

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        using TextLayer::Shared::setGlyphCache;

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{2, 3}};
    shared.setGlyphCache(cache);

    FontHandle fontHandle = shared.addInstancelessFont(glyphCacheFontId, 1.0f);
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}, TextLayerStyleUniform{}},
        {0, 1, 0},
        {fontHandle, fontHandle, fontHandle},
        {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    DataHandle data = layer.createGlyph(2, 55, {});

    std::ostringstream out;
    Error redirectError{&out};
    layer.createGlyph(2, 56, {});
    layer.setGlyph(data, 56, {});
    CORRADE_COMPARE(out.str(),
        "Whee::TextLayer::createGlyph(): glyph 56 out of range for 56 glyphs in glyph cache font 1\n"
        "Whee::TextLayer::setGlyph(): glyph 56 out of range for 56 glyphs in glyph cache font 1\n");
}

void TextLayerTest::updateEmpty() {
    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return {}; }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(56, &font);

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        using TextLayer::Shared::setGlyphCache;

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{1}};
    shared.setGlyphCache(cache);

    FontHandle fontHandle = shared.addFont(font, 1.0f);
    shared.setStyle(
        TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}},
        {fontHandle},
        {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0, 1), shared};

    /* Shouldn't crash or do anything weird */
    layer.update(LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsNodeOrderUpdate|LayerState::NeedsNodeOffsetSizeUpdate|LayerState::NeedsAttachmentUpdate|LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate|LayerState::NeedsSharedDataUpdate, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_VERIFY(true);
}

void TextLayerTest::updateCleanDataOrder() {
    auto&& data = UpdateCleanDataOrderData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Does just extremely basic verification that the vertex and index data
       get filled with correct contents and in correct order. The actual visual
       output is checked in TextLayerGLTest. */

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return _opened; }
        Properties doOpenFile(Containers::StringView, Float size) override {
            _opened = true;
            return {size, 8.0f, -4.0f, 16.0f, 98};
        }
        void doClose() override { _opened = false; }

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return Containers::pointer<ThreeGlyphShaper>(*this); }

        bool _opened = false;
    } threeGlyphFont;
    threeGlyphFont.openFile({}, 16.0f);

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return _opened; }
        Properties doOpenFile(Containers::StringView, Float size) override {
            _opened = true;
            return {size, 1.0f, -0.5f, 2.0f, 67};
        }
        void doClose() override { _opened = false; }

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return Containers::pointer<OneGlyphShaper>(*this); }

        bool _opened = false;
    } oneGlyphFont;
    oneGlyphFont.openFile({}, 2.0f);

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    /* Default padding is 1, resetting to 0 for simplicity */
    } cache{PixelFormat::R8Unorm, {32, 32, 3}, {}};

    /* +--+--+
       |66|13|
       +-----+
       | 97  |
       +-----+ */
    {
        UnsignedInt fontId = cache.addFont(threeGlyphFont.glyphCount(), &threeGlyphFont);
        cache.addGlyph(fontId, 97, {8, 4}, 2, {{}, {32, 16}});
        /* Glyph 22 deliberately omitted */
        cache.addGlyph(fontId, 13, {4, -8}, 0, {{16, 16}, {32, 32}});
    } {
        UnsignedInt fontId = cache.addFont(oneGlyphFont.glyphCount(), &oneGlyphFont);
        cache.addGlyph(fontId, 66, {}, 1, {{0, 16}, {16, 32}});
    }

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        using TextLayer::Shared::setGlyphCache;

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{3, 6}};
    shared.setGlyphCache(cache);

    /* The three-glyph font is scaled to 0.5, the one-glyph to 2.0 */
    FontHandle threeGlyphFontHandle = shared.addFont(threeGlyphFont, 8.0f);
    FontHandle oneGlyphFontHandle = shared.addFont(oneGlyphFont, 4.0f);
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}, TextLayerStyleUniform{}, TextLayerStyleUniform{}},
        /* Style 5 doesn't get used (gets transitioned to 2), use a weird
           uniform index and padding to verify it doesn't get picked. The font
           handle should however match style 2 as it can't be transitioned. */
        {1, 2, 0, 1, 1, 666},
        {oneGlyphFontHandle, oneGlyphFontHandle, threeGlyphFontHandle, threeGlyphFontHandle, threeGlyphFontHandle, threeGlyphFontHandle},
        {{}, {}, data.paddingFromStyle, {}, data.paddingFromStyle, Vector4{666}});
    shared.setStyleTransition(
        nullptr,
        nullptr,
        [](UnsignedInt style) {
            return style == 5 ? 2u : style;
        }
    );

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}

        const State& stateData() const {
            return static_cast<const State&>(*_state);
        }
    } layer{layerHandle(0, 1), shared};

    /* Two node handles to attach the data to */
    NodeHandle node6 = nodeHandle(6, 0);
    NodeHandle node15 = nodeHandle(15, 0);

    /* Create 10 data handles. Only four get filled and actually used. */
    layer.create(0, "", {});                            /* 0, quad 0 */
    layer.create(0, "", {});                            /* 1, quad 1 */
    layer.create(0, "", {});                            /* 2, quad 2 */
    /* Node 6 is disabled, so style 5 should get transitioned to 2 */
    DataHandle data3 = layer.create(5, "hello", {}, 0xff3366_rgbf, node6);
                                                        /* 3, quad 3 to 7 */
    layer.create(0, "", {});                            /* 4, quad 8 */
    /* Node 6 is disabled, but style 4 has no disabled transition so this stays
       the same */
    DataHandle data5 = layer.createGlyph(4, 13, {}, 0xcceeff_rgbf, node6);
                                                        /* 5, quad 9 */
    layer.create(0, "", {});                            /* 6, quad 10 */
    DataHandle data7 = layer.create(1, "ahoy", {}, 0x112233_rgbf, node15);
                                                        /* 7, quad 11 */
    layer.create(0, "", {});                            /* 8, quad 12 */
    DataHandle data9 = layer.create(3, "hi", {}, 0x663399_rgbf, node15);
                                                        /* 9, quad 13 to 14 */

    if(!data.paddingFromData.isZero()) {
        layer.setPadding(data3, data.paddingFromData);
        layer.setPadding(data5, data.paddingFromData);
    }

    /* There should be 10 glyph runs, assigned to the 10 data */
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().data).slice(&Implementation::TextLayerData::glyphRun), Containers::arrayView({
        0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::glyphOffset), Containers::arrayView({
        0u, 1u, 2u, 3u, 8u, 9u, 10u, 11u, 12u, 13u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::glyphCount), Containers::arrayView({
        1u, 1u, 1u, 5u, 1u, 1u, 1u, 1u, 1u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::data), Containers::arrayView({
        0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u
    }), TestSuite::Compare::Container);

    Vector2 nodeOffsets[16];
    Vector2 nodeSizes[16];
    UnsignedByte nodesEnabledData[2]{};
    Containers::MutableBitArrayView nodesEnabled{nodesEnabledData, 0, 16};
    nodeOffsets[6] = data.node6Offset;
    nodeSizes[6] = data.node6Size;
    nodeOffsets[15] = {3.0f, 4.0f};
    nodeSizes[15] = {20.0f, 5.0f};
    nodesEnabled.set(15);

    /* An empty update should generate an empty draw list */
    if(data.emptyUpdate) {
        layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, nodeOffsets, nodeSizes, nodesEnabled, {}, {});
        CORRADE_COMPARE_AS(layer.stateData().indices,
            Containers::ArrayView<const UnsignedInt>{},
            TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(layer.stateData().indexDrawOffsets, Containers::arrayView({
            0u
        }), TestSuite::Compare::Container);
        return;
    }

    /* Just the filled subset is getting updated */
    UnsignedInt dataIds[]{9, 5, 7, 3};
    layer.update(LayerState::NeedsDataUpdate, dataIds, {}, {}, nodeOffsets, nodeSizes, nodesEnabled, {}, {});

    /* The indices should be filled just for the four items */
    CORRADE_COMPARE_AS(layer.stateData().indices, Containers::arrayView<UnsignedInt>({
        /* Text 9, "hi", quads 13 to 14 */
        13*4 + 0, 13*4 + 1, 13*4 + 2, 13*4 + 2, 13*4 + 1, 13*4 + 3,
        14*4 + 0, 14*4 + 1, 14*4 + 2, 14*4 + 2, 14*4 + 1, 14*4 + 3,
        /* Glyph 5, quad 9 */
         9*4 + 0,  9*4 + 1,  9*4 + 2,  9*4 + 2,  9*4 + 1,  9*4 + 3,
        /* Text 7, "ahoy", quad 11 */
        11*4 + 0, 11*4 + 1, 11*4 + 2, 11*4 + 2, 11*4 + 1, 11*4 + 3,
        /* Text 3, "hello", quads 3 to 7 */
         3*4 + 0,  3*4 + 1,  3*4 + 2,  3*4 + 2,  3*4 + 1,  3*4 + 3,
         4*4 + 0,  4*4 + 1,  4*4 + 2,  4*4 + 2,  4*4 + 1,  4*4 + 3,
         5*4 + 0,  5*4 + 1,  5*4 + 2,  5*4 + 2,  5*4 + 1,  5*4 + 3,
         6*4 + 0,  6*4 + 1,  6*4 + 2,  6*4 + 2,  6*4 + 1,  6*4 + 3,
         7*4 + 0,  7*4 + 1,  7*4 + 2,  7*4 + 2,  7*4 + 1,  7*4 + 3,
    }), TestSuite::Compare::Container);

    /* The vertices are there for all data, but only the actually used are
       filled */
    CORRADE_COMPARE(layer.stateData().vertices.size(), 15*4);
    for(std::size_t i = 0; i != 5*4; ++i) {
        CORRADE_ITERATION(i);
        CORRADE_COMPARE(layer.stateData().vertices[3*4 + i].color, 0xff3366_rgbf);
        /* Created with style 2, transitioned from 5, which is mapped to
           uniform 0 */
        CORRADE_COMPARE(layer.stateData().vertices[3*4 + i].styleUniform, 0);
    }
    for(std::size_t i = 0; i != 1*4; ++i) {
        CORRADE_ITERATION(i);
        CORRADE_COMPARE(layer.stateData().vertices[9*4 + i].color, 0xcceeff_rgbf);
        /* Created with style 4, which is mapped to uniform 1 */
        CORRADE_COMPARE(layer.stateData().vertices[9*4 + i].styleUniform, 1);
    }
    for(std::size_t i = 0; i != 1*4; ++i) {
        CORRADE_ITERATION(i);
        CORRADE_COMPARE(layer.stateData().vertices[11*4 + i].color, 0x112233_rgbf);
        /* Created with style 1, which is mapped to uniform 2 */
        CORRADE_COMPARE(layer.stateData().vertices[11*4 + i].styleUniform, 2);
    }
    for(std::size_t i = 0; i != 2*4; ++i) {
        CORRADE_ITERATION(i);
        CORRADE_COMPARE(layer.stateData().vertices[13*4 + i].color, 0x663399_rgbf);
        /* Created with style 3, which is mapped to uniform 1 */
        CORRADE_COMPARE(layer.stateData().vertices[13*4 + i].styleUniform, 1);
    }

    Containers::StridedArrayView1D<const Vector2> positions = stridedArrayView(layer.stateData().vertices).slice(&Implementation::TextLayerVertex::position);
    Containers::StridedArrayView1D<const Vector3> textureCoordinates = stridedArrayView(layer.stateData().vertices).slice(&Implementation::TextLayerVertex::textureCoordinates);

    /* Text 3 and glyph 5 are attached to node 6, which has a center of
       {6.0, 9.5}. Shaped positions should match what's in create() however as
       the coordinate system has Y up, the glyph positions have Y flipped
       compared in comparison to create():

        2--3
        |  |
        0--1 */
    CORRADE_COMPARE_AS(positions.sliceSize(3*4, 5*4), Containers::arrayView<Vector2>({
        /* Glyph 22, not in cache */
        {6.0f - 5.0f,               9.5f + 0.5f},
        {6.0f - 5.0f,               9.5f + 0.5f},
        {6.0f - 5.0f,               9.5f + 0.5f},
        {6.0f - 5.0f,               9.5f + 0.5f},

        /* Glyph 13. Offset {4, -8}, size {16, 16}, scaled to 0.5. */
        {6.0f - 3.5f + 2.0f + 0.0f, 9.5f - 0.0f + 4.0f - 0.0f},
        {6.0f - 3.5f + 2.0f + 8.0f, 9.5f - 0.0f + 4.0f - 0.0f},
        {6.0f - 3.5f + 2.0f + 0.0f, 9.5f - 0.0f + 4.0f - 8.0f},
        {6.0f - 3.5f + 2.0f + 8.0f, 9.5f - 0.0f + 4.0f - 8.0f},

        /* Glyph 97. Offset {8, 4}, size {32, 16}, scaled to 0.5. */
        {6.0f - 1.5f + 4.0f + 0.0f, 9.5f - 0.5f - 2.0f - 0.0f},
        {6.0f - 1.5f + 4.0f + 16.f, 9.5f - 0.5f - 2.0f - 0.0f},
        {6.0f - 1.5f + 4.0f + 0.0f, 9.5f - 0.5f - 2.0f - 8.0f},
        {6.0f - 1.5f + 4.0f + 16.f, 9.5f - 0.5f - 2.0f - 8.0f},

        /* Glyph 22, not in cache */
        {6.0f + 1.0f,               9.5f - 1.0f},
        {6.0f + 1.0f,               9.5f - 1.0f},
        {6.0f + 1.0f,               9.5f - 1.0f},
        {6.0f + 1.0f,               9.5f - 1.0f},

        /* Glyph 13 again */
        {6.0f + 4.0f + 2.0f + 0.0f, 9.5f - 1.5f + 4.0f - 0.0f},
        {6.0f + 4.0f + 2.0f + 8.0f, 9.5f - 1.5f + 4.0f - 0.0f},
        {6.0f + 4.0f + 2.0f + 0.0f, 9.5f - 1.5f + 4.0f - 8.0f},
        {6.0f + 4.0f + 2.0f + 8.0f, 9.5f - 1.5f + 4.0f - 8.0f},
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(positions.sliceSize(9*4, 1*4), Containers::arrayView<Vector2>({
        /* Glyph 13 again, centered */
        {6.0f - 4.0f        + 0.0f, 9.5f + 4.0f        - 0.0f},
        {6.0f - 4.0f        + 8.0f, 9.5f + 4.0f        - 0.0f},
        {6.0f - 4.0f        + 0.0f, 9.5f + 4.0f        - 8.0f},
        {6.0f - 4.0f        + 8.0f, 9.5f + 4.0f        - 8.0f},
    }), TestSuite::Compare::Container);

    /* Text 7 and 9 are both attached to node 15, which has a center of
       {13.0, 6.5} */
    CORRADE_COMPARE_AS(positions.sliceSize(11*4, 1*4), Containers::arrayView<Vector2>({
        /* Glyph 66. No offset, size {16, 16}, scaled to 2.0. */
        {13.f + 0.5f        + 0.0f, 6.5f + 1.5f        - 0.0f},
        {13.f + 0.5f        + 32.f, 6.5f + 1.5f        - 0.0f},
        {13.f + 0.5f        + 0.0f, 6.5f + 1.5f        - 32.f},
        {13.f + 0.5f        + 32.f, 6.5f + 1.5f        - 32.f},
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(positions.sliceSize(13*4, 2*4), Containers::arrayView<Vector2>({
        /* Glyph 22, not in cache */
        {13.f - 1.25f,              6.5f + 0.5f},
        {13.f - 1.25f,              6.5f + 0.5f},
        {13.f - 1.25f,              6.5f + 0.5f},
        {13.f - 1.25f,              6.5f + 0.5f},

        /* Glyph 13. Offset {4, -8}, size {16, 16}, scaled to 0.5. */
        {13.f + 0.25f + 2.f + 0.0f, 6.5f - 0.0f + 4.0f - 0.0f},
        {13.f + 0.25f + 2.f + 8.0f, 6.5f - 0.0f + 4.0f - 0.0f},
        {13.f + 0.25f + 2.f + 0.0f, 6.5f - 0.0f + 4.0f - 8.0f},
        {13.f + 0.25f + 2.f + 8.0f, 6.5f - 0.0f + 4.0f - 8.0f},
    }), TestSuite::Compare::Container);

    /* Texture coordinates however stay the same, with Y up:

        +--+--+
        |66|13|
        3-----2
        | 97  |
        0-----1 */

    /* Glyph 22, at quad 3, 6, 13, isn't in cache */
    for(std::size_t i: {3, 6, 13}) {
        CORRADE_COMPARE_AS(textureCoordinates.sliceSize(i*4, 4), Containers::arrayView<Vector3>({
            {},
            {},
            {},
            {},
        }), TestSuite::Compare::Container);
    }

    /* Glyph 13, at quad 4, 7, 9, 14 */
    for(std::size_t i: {4, 7, 9, 14}) {
        CORRADE_COMPARE_AS(textureCoordinates.sliceSize(i*4, 4), Containers::arrayView<Vector3>({
            {0.5f, 0.5f, 0.0f},
            {1.0f, 0.5f, 0.0f},
            {0.5f, 1.0f, 0.0f},
            {1.0f, 1.0f, 0.0f},
        }), TestSuite::Compare::Container);
    }

    /* Glyph 66, at quad 11 */
    CORRADE_COMPARE_AS(textureCoordinates.sliceSize(11*4, 4), Containers::arrayView<Vector3>({
        {0.0f, 0.5f, 1.0f},
        {0.5f, 0.5f, 1.0f},
        {0.0f, 1.0f, 1.0f},
        {0.5f, 1.0f, 1.0f},
    }), TestSuite::Compare::Container);

    /* Glyph 97, at quad 5 */
    CORRADE_COMPARE_AS(textureCoordinates.sliceSize(5*4, 4), Containers::arrayView<Vector3>({
        {0.0f, 0.0f, 2.0f},
        {1.0f, 0.0f, 2.0f},
        {0.0f, 0.5f, 2.0f},
        {1.0f, 0.5f, 2.0f},
    }), TestSuite::Compare::Container);

    /* For drawing data 9, 5, 7, 3 it needs to draw the first 2 quads in the
       index buffer, then next 1 quad, then next 1, then next 5 */
    CORRADE_COMPARE_AS(layer.stateData().indexDrawOffsets, Containers::arrayView({
        0u, 2u*6, 3u*6, 4u*6, 9u*6
    }), TestSuite::Compare::Container);

    /* Removing a node with cleanNodes() marks the corresponding run as unused,
       and update() recompacts again */
    {
        UnsignedShort nodeGenerations[16];
        nodeGenerations[6] = nodeHandleGeneration(node6) + 1;
        nodeGenerations[15] = nodeHandleGeneration(node15);
        layer.cleanNodes(nodeGenerations);
        /* Node 6 was disabled before already, so the nodesEnabled mask doesn't
           need to be updated */
    }

    /* The run corresponding to the removed data should be marked as unused,
       the rest stays the same */
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().data).slice(&Implementation::TextLayerData::glyphRun), Containers::arrayView({
        0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::glyphOffset), Containers::arrayView({
        0u, 1u, 2u, 0xffffffffu, 8u, 0xffffffffu, 10u, 11u, 12u, 13u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::glyphCount), Containers::arrayView({
        1u, 1u, 1u, 5u, 1u, 1u, 1u, 1u, 1u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::data), Containers::arrayView({
        0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u
    }), TestSuite::Compare::Container);

    UnsignedInt dataIdsPostClean[]{9, 7};
    layer.update(LayerState::NeedsDataUpdate, dataIdsPostClean, {}, {}, nodeOffsets, nodeSizes, nodesEnabled, {}, {});

    /* There should be just 9 glyph runs, assigned to the remaining 9 data */
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().data).slice(&Implementation::TextLayerData::glyphRun), Containers::arrayView({
        0u, 1u, 2u, 3u /* free data */, 3u, 5u /* free data */, 4u, 5u, 6u, 7u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::glyphOffset), Containers::arrayView({
        0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::glyphCount), Containers::arrayView({
        1u, 1u, 1u, 1u, 1u, 1u, 1u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::data), Containers::arrayView({
        0u, 1u, 2u, 4u, 6u, 7u, 8u, 9u
    }), TestSuite::Compare::Container);

    /* The glyph count queries should still match */
    CORRADE_COMPARE(layer.glyphCount(data7), 1u);
    CORRADE_COMPARE(layer.glyphCount(data9), 2u);

    /* Indices for remaining 3 visible glyphs */
    CORRADE_COMPARE_AS(layer.stateData().indices, Containers::arrayView<UnsignedInt>({
        /* Text 9, "hi", quads 7 to 8 */
        7*4 + 0, 7*4 + 1, 7*4 + 2, 7*4 + 2, 7*4 + 1, 7*4 + 3,
        8*4 + 0, 8*4 + 1, 8*4 + 2, 8*4 + 2, 8*4 + 1, 8*4 + 3,
        /* Text 7, "ahoy", quad 5 */
        5*4 + 0, 5*4 + 1, 5*4 + 2, 5*4 + 2, 5*4 + 1, 5*4 + 3,
        /* Text 3, "hello" is removed now */
        /* Glyph 5 is removed now */
    }), TestSuite::Compare::Container);

    /* Vertices for all remaining 9 glyphs */
    CORRADE_COMPARE(layer.stateData().vertices.size(), 9*4);
    for(std::size_t i = 0; i != 1*4; ++i) {
        CORRADE_ITERATION(i);
        CORRADE_COMPARE(layer.stateData().vertices[5*4 + i].color, 0x112233_rgbf);
        /* Created with style 1, which is mapped to uniform 2 */
        CORRADE_COMPARE(layer.stateData().vertices[5*4 + i].styleUniform, 2);
    }
    for(std::size_t i = 0; i != 2*4; ++i) {
        CORRADE_ITERATION(i);
        CORRADE_COMPARE(layer.stateData().vertices[7*4 + i].color, 0x663399_rgbf);
        /* Created with style 3, which is mapped to uniform 1 */
        CORRADE_COMPARE(layer.stateData().vertices[7*4 + i].styleUniform, 1);
    }

    /* Text 7 and 9, now quads 5 and 7 to 8 */
    CORRADE_COMPARE_AS(positions.sliceSize(5*4, 1*4), Containers::arrayView<Vector2>({
        {13.f + 0.5f        + 0.0f, 6.5f + 1.5f        - 0.0f},
        {13.f + 0.5f        + 32.f, 6.5f + 1.5f        - 0.0f},
        {13.f + 0.5f        + 0.0f, 6.5f + 1.5f        - 32.f},
        {13.f + 0.5f        + 32.f, 6.5f + 1.5f        - 32.f},
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(positions.sliceSize(7*4, 2*4), Containers::arrayView<Vector2>({
        {13.f - 1.25f,              6.5f + 0.5f},
        {13.f - 1.25f,              6.5f + 0.5f},
        {13.f - 1.25f,              6.5f + 0.5f},
        {13.f - 1.25f,              6.5f + 0.5f},

        {13.f + 0.25f + 2.f + 0.0f, 6.5f - 0.0f + 4.0f - 0.0f},
        {13.f + 0.25f + 2.f + 8.0f, 6.5f - 0.0f + 4.0f - 0.0f},
        {13.f + 0.25f + 2.f + 0.0f, 6.5f - 0.0f + 4.0f - 8.0f},
        {13.f + 0.25f + 2.f + 8.0f, 6.5f - 0.0f + 4.0f - 8.0f},
    }), TestSuite::Compare::Container);

    /* Glyph 22, now only at quad 7 */
    CORRADE_COMPARE_AS(textureCoordinates.sliceSize(7*4, 4), Containers::arrayView<Vector3>({
        {},
        {},
        {},
        {},
    }), TestSuite::Compare::Container);

    /* Glyph 13, now only at quad 8 */
    CORRADE_COMPARE_AS(textureCoordinates.sliceSize(8*4, 4), Containers::arrayView<Vector3>({
        {0.5f, 0.5f, 0.0f},
        {1.0f, 0.5f, 0.0f},
        {0.5f, 1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
    }), TestSuite::Compare::Container);

    /* Glyph 66, now at quad 5 */
    CORRADE_COMPARE_AS(textureCoordinates.sliceSize(5*4, 4), Containers::arrayView<Vector3>({
        {0.0f, 0.5f, 1.0f},
        {0.5f, 0.5f, 1.0f},
        {0.0f, 1.0f, 1.0f},
        {0.5f, 1.0f, 1.0f},
    }), TestSuite::Compare::Container);

    /* For drawing data 9 and 7 it needs to draw the first 2 quads in the
       index buffer, then next 1 quad */
    CORRADE_COMPARE_AS(layer.stateData().indexDrawOffsets, Containers::arrayView({
        0u, 2u*6, 3u*6
    }), TestSuite::Compare::Container);

    /* Removing a text marks the corresponding run as unused, the next update()
       then recompacts it */
    layer.remove(data7);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsNodeOffsetSizeUpdate|LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsAttachmentUpdate|LayerState::NeedsDataClean);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::glyphOffset), Containers::arrayView({
        0u, 1u, 2u, 3u, 4u, 0xffffffffu, 6u, 7u
    }), TestSuite::Compare::Container);

    UnsignedInt dataIdsPostRemoval[]{9};
    layer.update(LayerState::NeedsDataUpdate, dataIdsPostRemoval, {}, {}, nodeOffsets, nodeSizes, nodesEnabled, {}, {});

    /* There should be just 7 glyph runs, assigned to the remaining 7 data */
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().data).slice(&Implementation::TextLayerData::glyphRun), Containers::arrayView({
        0u, 1u, 2u, 3u /* free data */, 3u, 5u /* free data */, 4u, 5u /* free data */, 5u, 6u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::glyphOffset), Containers::arrayView({
        0u, 1u, 2u, 3u, 4u, 5u, 6u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::glyphCount), Containers::arrayView({
        1u, 1u, 1u, 1u, 1u, 1u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::data), Containers::arrayView({
        0u, 1u, 2u, 4u, 6u, 8u, 9u
    }), TestSuite::Compare::Container);

    /* The glyph count queries should still match */
    CORRADE_COMPARE(layer.glyphCount(data9), 2u);

    /* Indices for remaining 2 visible glyphs */
    CORRADE_COMPARE_AS(layer.stateData().indices, Containers::arrayView<UnsignedInt>({
        /* Text 9, "hi", quads 6 to 7 */
        6*4 + 0, 6*4 + 1, 6*4 + 2, 6*4 + 2, 6*4 + 1, 6*4 + 3,
        7*4 + 0, 7*4 + 1, 7*4 + 2, 7*4 + 2, 7*4 + 1, 7*4 + 3,
        /* Text 7, "ahoy", is removed now */
        /* Text 3, "hello", is removed now */
        /* Glyph 5 is removed now */
    }), TestSuite::Compare::Container);

    /* Vertices for all remaining 8 glyphs */
    CORRADE_COMPARE(layer.stateData().vertices.size(), 8*4);
    for(std::size_t i = 0; i != 2*4; ++i) {
        CORRADE_ITERATION(i);
        CORRADE_COMPARE(layer.stateData().vertices[6*4 + i].color, 0x663399_rgbf);
        /* Created with style 3, which is mapped to uniform 1 */
        CORRADE_COMPARE(layer.stateData().vertices[6*4 + i].styleUniform, 1);
    }

    /* Text 9, now quad 6 to 7 */
    CORRADE_COMPARE_AS(positions.sliceSize(6*4, 2*4), Containers::arrayView<Vector2>({
        {13.f - 1.25f,              6.5f + 0.5f},
        {13.f - 1.25f,              6.5f + 0.5f},
        {13.f - 1.25f,              6.5f + 0.5f},
        {13.f - 1.25f,              6.5f + 0.5f},

        {13.f + 0.25f + 2.f + 0.0f, 6.5f - 0.0f + 4.0f - 0.0f},
        {13.f + 0.25f + 2.f + 8.0f, 6.5f - 0.0f + 4.0f - 0.0f},
        {13.f + 0.25f + 2.f + 0.0f, 6.5f - 0.0f + 4.0f - 8.0f},
        {13.f + 0.25f + 2.f + 8.0f, 6.5f - 0.0f + 4.0f - 8.0f},
    }), TestSuite::Compare::Container);

    /* Glyph 22, now only at quad 6 */
    CORRADE_COMPARE_AS(textureCoordinates.sliceSize(6*4, 4), Containers::arrayView<Vector3>({
        {},
        {},
        {},
        {},
    }), TestSuite::Compare::Container);

    /* Glyph 13, now only at quad 7 */
    CORRADE_COMPARE_AS(textureCoordinates.sliceSize(7*4, 4), Containers::arrayView<Vector3>({
        {0.5f, 0.5f, 0.0f},
        {1.0f, 0.5f, 0.0f},
        {0.5f, 1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
    }), TestSuite::Compare::Container);

    /* For drawing data 9 it needs to draw the first 2 quads in the index
       buffer */
    CORRADE_COMPARE_AS(layer.stateData().indexDrawOffsets, Containers::arrayView({
        0u, 2u*6
    }), TestSuite::Compare::Container);
}

void TextLayerTest::updateAlignment() {
    auto&& data = UpdateAlignmentPaddingData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return _opened; }
        Properties doOpenFile(Containers::StringView, Float) override {
            _opened = true;
            /* Font size and line height shouldn't be used for any alignment,
               ascent / descent should */
            return {100.0f, 3.5f, -2.0f, 200.0f, 1};
        }
        void doClose() override { _opened = false; }

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override {
            struct Shaper: Text::AbstractShaper {
                using Text::AbstractShaper::AbstractShaper;

                UnsignedInt doShape(Containers::StringView text, UnsignedInt, UnsignedInt, Containers::ArrayView<const Text::FeatureRange>) override {
                    return text.size();
                }
                void doGlyphIdsInto(const Containers::StridedArrayView1D<UnsignedInt>& ids) const override {
                    for(std::size_t i = 0; i != ids.size(); ++i)
                        ids[i] = 0;
                }
                void doGlyphOffsetsAdvancesInto(const Containers::StridedArrayView1D<Vector2>& offsets, const Containers::StridedArrayView1D<Vector2>& advances) const override {
                    for(std::size_t i = 0; i != offsets.size(); ++i) {
                        offsets[i] = {};
                        advances[i] = {1.5f, 0.0f};
                    }
                }
                void doGlyphClustersInto(const Containers::StridedArrayView1D<UnsignedInt>&) const override {
                    /** @todo implement when it actually does get called for
                        cursor / selection */
                    CORRADE_FAIL("This shouldn't be called.");
                }
            };
            return Containers::pointer<Shaper>(*this);
        }

        bool _opened = false;
    } font;
    font.openFile({}, 100.0f);

    /* A trivial glyph cache. While font's ascent/descent goes both above and
       below the line, this is just above. */
    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    /* Default padding is 1, resetting to 0 for simplicity */
    } cache{PixelFormat::R8Unorm, {32, 32}, {}};
    cache.addGlyph(cache.addFont(1, &font), 0, {}, {{}, {1, 2}});

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        using TextLayer::Shared::setGlyphCache;

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{1}};
    shared.setGlyphCache(cache);

    /* Font scaled 2x, so all metrics coming from the font or the cache should
       be scaled 2x */
    FontHandle fontHandle = shared.addFont(font, 200.0f);
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}},
        {fontHandle},
        {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}

        const State& stateData() const {
            return static_cast<const State&>(*_state);
        }
    } layer{layerHandle(0, 1), shared};

    NodeHandle node3 = nodeHandle(3, 0);

    /* 3 chars, size x2, so the bounding box is 9x11 */
    layer.create(0, "hey", data.alignment, node3);

    Vector2 nodeOffsets[4];
    Vector2 nodeSizes[4];
    UnsignedByte nodesEnabledData[1]{};
    Containers::BitArrayView nodesEnabled{nodesEnabledData, 0, 4};
    nodeOffsets[3] = {50.5f, 20.5f};
    nodeSizes[3] = {200.8f, 100.4f};
    UnsignedInt dataIds[]{0};
    layer.update(LayerState::NeedsDataUpdate, dataIds, {}, {}, nodeOffsets, nodeSizes, nodesEnabled, {}, {});

    /* 2--3
       |  |
       0--1 */
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().vertices).slice(&Implementation::TextLayerVertex::position), Containers::arrayView<Vector2>({
        Vector2{0.0f, 0.0f} + data.offset,
        Vector2{2.0f, 0.0f} + data.offset,
        Vector2{0.0f, -4.0f} + data.offset,
        Vector2{2.0f, -4.0f} + data.offset,

        Vector2{3.0f, 0.0f} + data.offset,
        Vector2{5.0f, 0.0f} + data.offset,
        Vector2{3.0f, -4.0f} + data.offset,
        Vector2{5.0f, -4.0f} + data.offset,

        Vector2{6.0f, 0.0f} + data.offset,
        Vector2{8.0f, 0.0f} + data.offset,
        Vector2{6.0f, -4.0f} + data.offset,
        Vector2{8.0f, -4.0f} + data.offset,
    }), TestSuite::Compare::Container);
}

void TextLayerTest::updateAlignmentGlyph() {
    auto&& data = UpdateAlignmentPaddingData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* A trivial glyph cache. Goes both above and below the line to verify
       vertical alignment. */
    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    /* Default padding is 1, resetting to 0 for simplicity */
    } cache{PixelFormat::R8Unorm, {32, 32}, {}};

    UnsignedInt glyphCacheFontId = cache.addFont(18);
    cache.addGlyph(glyphCacheFontId, 17, {-2, -3}, {{}, {3, 4}});

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        using TextLayer::Shared::setGlyphCache;

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{1}};
    shared.setGlyphCache(cache);

    /* Font scaled 2x, so all metrics coming from the the cache should be
       scaled 2x */
    FontHandle fontHandle = shared.addInstancelessFont(glyphCacheFontId, 2.0f);
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}},
        {fontHandle},
        {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}

        const State& stateData() const {
            return static_cast<const State&>(*_state);
        }
    } layer{layerHandle(0, 1), shared};

    NodeHandle node3 = nodeHandle(3, 0);

    /* Size x2, so the bounding box is 6x8 */
    layer.createGlyph(
        0,
        17,
        TextProperties{}.setAlignment(data.alignment),
        node3);

    Vector2 nodeOffsets[4];
    Vector2 nodeSizes[4];
    UnsignedByte nodesEnabledData[1]{};
    Containers::BitArrayView nodesEnabled{nodesEnabledData, 0, 4};
    nodeOffsets[3] = {50.5f, 20.5f};
    nodeSizes[3] = {200.8f, 100.4f};
    UnsignedInt dataIds[]{0};
    layer.update(LayerState::NeedsDataUpdate, dataIds, {}, {}, nodeOffsets, nodeSizes, nodesEnabled, {}, {});

    /* 2--3
       |  |
       0--1 */
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().vertices).slice(&Implementation::TextLayerVertex::position), Containers::arrayView<Vector2>({
        Vector2{0.0f, 0.0f} + data.offsetGlyph,
        Vector2{6.0f, 0.0f} + data.offsetGlyph,
        Vector2{0.0f, -8.0f} + data.offsetGlyph,
        Vector2{6.0f, -8.0f} + data.offsetGlyph
    }), TestSuite::Compare::Container);
}

void TextLayerTest::updatePadding() {
    auto&& data = UpdateAlignmentPaddingData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Same as updateAlignment(), except that the node offset & size is
       different and only matches the original if padding is applied
       correctly from both the data and the style */

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return _opened; }
        Properties doOpenFile(Containers::StringView, Float) override {
            _opened = true;
            /* Font size and line height shouldn't be used for any alignment,
               ascent / descent should */
            return {100.0f, 3.5f, -2.0f, 200.0f, 1};
        }
        void doClose() override { _opened = false; }

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override {
            struct Shaper: Text::AbstractShaper {
                using Text::AbstractShaper::AbstractShaper;

                UnsignedInt doShape(Containers::StringView text, UnsignedInt, UnsignedInt, Containers::ArrayView<const Text::FeatureRange>) override {
                    return text.size();
                }
                void doGlyphIdsInto(const Containers::StridedArrayView1D<UnsignedInt>& ids) const override {
                    for(std::size_t i = 0; i != ids.size(); ++i)
                        ids[i] = 0;
                }
                void doGlyphOffsetsAdvancesInto(const Containers::StridedArrayView1D<Vector2>& offsets, const Containers::StridedArrayView1D<Vector2>& advances) const override {
                    for(std::size_t i = 0; i != offsets.size(); ++i) {
                        offsets[i] = {};
                        advances[i] = {1.5f, 0.0f};
                    }
                }
                void doGlyphClustersInto(const Containers::StridedArrayView1D<UnsignedInt>&) const override {
                    /** @todo implement when it actually does get called for
                        cursor / selection */
                    CORRADE_FAIL("This shouldn't be called.");
                }
            };
            return Containers::pointer<Shaper>(*this);
        }

        bool _opened = false;
    } font;
    font.openFile({}, 100.0f);

    /* A trivial glyph cache. While font's ascent/descent goes both above and
       below the line, this is just above. */
    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    /* Default padding is 1, resetting to 0 for simplicity */
    } cache{PixelFormat::R8Unorm, {32, 32}, {}};
    cache.addGlyph(cache.addFont(1, &font), 0, {}, {{}, {1, 2}});

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        using TextLayer::Shared::setGlyphCache;

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{1}};
    shared.setGlyphCache(cache);

    /* Font scaled 2x, so all metrics coming from the font or the cache should
       be scaled 2x */
    FontHandle fontHandle = shared.addFont(font, 200.0f);
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}},
        {fontHandle},
        {{10.0f, 5.0f, 20.0f, 10.0f}});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}

        const State& stateData() const {
            return static_cast<const State&>(*_state);
        }
    } layer{layerHandle(0, 1), shared};

    NodeHandle node3 = nodeHandle(3, 0);

    /* 3 chars, size x2, so the bounding box is 9x11 */
    DataHandle node3Data = layer.create(0, "hey", data.alignment, node3);
    layer.setPadding(node3Data, {20.0f, 5.0f, 50.0f, 30.0f});

    Vector2 nodeOffsets[4];
    Vector2 nodeSizes[4];
    UnsignedByte nodesEnabledData[1]{};
    Containers::BitArrayView nodesEnabled{nodesEnabledData, 0, 4};
    nodeOffsets[3] = {20.5f, 10.5f};
    nodeSizes[3] = {300.8f, 150.4f};
    UnsignedInt dataIds[]{0};
    layer.update(LayerState::NeedsDataUpdate, dataIds, {}, {}, nodeOffsets, nodeSizes, nodesEnabled, {}, {});

    /* 2--3
       |  |
       0--1 */
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().vertices).slice(&Implementation::TextLayerVertex::position), Containers::arrayView<Vector2>({
        Vector2{0.0f, 0.0f} + data.offset,
        Vector2{2.0f, 0.0f} + data.offset,
        Vector2{0.0f, -4.0f} + data.offset,
        Vector2{2.0f, -4.0f} + data.offset,

        Vector2{3.0f, 0.0f} + data.offset,
        Vector2{5.0f, 0.0f} + data.offset,
        Vector2{3.0f, -4.0f} + data.offset,
        Vector2{5.0f, -4.0f} + data.offset,

        Vector2{6.0f, 0.0f} + data.offset,
        Vector2{8.0f, 0.0f} + data.offset,
        Vector2{6.0f, -4.0f} + data.offset,
        Vector2{8.0f, -4.0f} + data.offset,
    }), TestSuite::Compare::Container);
}

void TextLayerTest::updatePaddingGlyph() {
    auto&& data = UpdateAlignmentPaddingData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Same as updateAlignmentGlyph(), except that the node offset & size is
       different and only matches the original if padding is applied
       correctly from both the data and the style */

    /* A trivial glyph cache. Goes both above and below the line to verify
       vertical alignment. */
    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    /* Default padding is 1, resetting to 0 for simplicity */
    } cache{PixelFormat::R8Unorm, {32, 32}, {}};

    UnsignedInt glyphCacheFontId = cache.addFont(18);
    cache.addGlyph(glyphCacheFontId, 17, {-2, -3}, {{}, {3, 4}});

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        using TextLayer::Shared::setGlyphCache;

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{1}};
    shared.setGlyphCache(cache);

    /* Font scaled 2x, so all metrics coming from the the cache should be
       scaled 2x */
    FontHandle fontHandle = shared.addInstancelessFont(glyphCacheFontId, 2.0f);
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}},
        {fontHandle},
        {{10.0f, 5.0f, 20.0f, 10.0f}});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}

        const State& stateData() const {
            return static_cast<const State&>(*_state);
        }
    } layer{layerHandle(0, 1), shared};

    NodeHandle node3 = nodeHandle(3, 0);

    /* Size x2, so the bounding box is 6x8 */
    DataHandle node3Data = layer.createGlyph(0, 17, data.alignment, node3);
    layer.setPadding(node3Data, {20.0f, 5.0f, 50.0f, 30.0f});

    Vector2 nodeOffsets[4];
    Vector2 nodeSizes[4];
    UnsignedByte nodesEnabledData[1]{};
    Containers::BitArrayView nodesEnabled{nodesEnabledData, 0, 4};
    nodeOffsets[3] = {20.5f, 10.5f};
    nodeSizes[3] = {300.8f, 150.4f};
    UnsignedInt dataIds[]{0};
    layer.update(LayerState::NeedsDataUpdate, dataIds, {}, {}, nodeOffsets, nodeSizes, nodesEnabled, {}, {});

    /* 2--3
       |  |
       0--1 */
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().vertices).slice(&Implementation::TextLayerVertex::position), Containers::arrayView<Vector2>({
        Vector2{0.0f, 0.0f} + data.offsetGlyph,
        Vector2{6.0f, 0.0f} + data.offsetGlyph,
        Vector2{0.0f, -8.0f} + data.offsetGlyph,
        Vector2{6.0f, -8.0f} + data.offsetGlyph
    }), TestSuite::Compare::Container);
}

void TextLayerTest::updateNoStyleSet() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{1}};

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    std::ostringstream out;
    Error redirectError{&out};
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(out.str(), "Whee::TextLayer::update(): no style data was set\n");
}

}}}}

CORRADE_TEST_MAIN(Magnum::Whee::Test::TextLayerTest)
