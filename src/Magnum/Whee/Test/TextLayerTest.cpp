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
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/String.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Container.h>
#include <Corrade/TestSuite/Compare/String.h>
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
/* for createRemoveSetText(), updateCleanDataOrder() and updateAlignment() */
#include "Magnum/Whee/Implementation/textLayerState.h"

namespace Magnum { namespace Whee { namespace Test { namespace {

struct TextLayerTest: TestSuite::Tester {
    explicit TextLayerTest();

    template<class T> void styleSizeAlignment();

    void styleCommonConstructDefault();
    void styleCommonConstructNoInit();
    void styleCommonSetters();

    void styleItemConstructDefault();
    void styleItemConstructNoInit();
    void styleItemSetters();

    void fontHandle();
    void fontHandleInvalid();
    void debugFontHandle();

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
    void sharedFontInvalidHandle();

    void sharedSetStyleFonts();
    void sharedSetStyleFontsInvalidSize();
    void sharedSetStyleFontsInvalidHandle();

    void construct();
    void constructCopy();
    void constructMove();

    /* remove() and setText() tested here as well */
    template<class T> void createRemoveSetText();
    void createSetTextTextProperties();
    void createNoSharedGlyphCache();

    void setColor();

    void invalidHandle();
    void invalidFontHandle();
    void noSharedStyleFonts();
    void styleOutOfRange();

    void updateEmpty();
    void updateCleanDataOrder();
    void updateAlignment();
};

enum class Enum: UnsignedShort {};

Debug& operator<<(Debug& debug, Enum value) {
    return debug << UnsignedInt(value);
}

const struct {
    const char* name;
    NodeHandle node;
    LayerStates state;
    bool layerDataHandleOverloads, customFont, noStyleFonts;
} CreateRemoveSetTextData[]{
    {"create",
        NodeHandle::Null, LayerStates{},
        false, false, false},
    {"create and attach",
        nodeHandle(9872, 0xbeb), LayerState::NeedsAttachmentUpdate,
        false, false, false},
    {"LayerDataHandle overloads",
        NodeHandle::Null, LayerStates{},
        true, false, false},
    {"custom fonts",
        NodeHandle::Null, LayerStates{},
        false, true, false},
    {"custom fonts, no style fonts set",
        NodeHandle::Null, LayerStates{},
        false, true, true},
    {"custom fonts, LayerDataHandle overloads",
        NodeHandle::Null, LayerStates{},
        true, true, false},
};

const struct {
    const char* name;
    Text::Alignment alignment;
    /* Node offset is {50.5, 20.5}, size {200.8, 100.4}; bounding box {9, 11},
       ascent 7, descent -4 */
    Vector2 offset;
} UpdateAlignmentData[]{
    {"line left", Text::Alignment::LineLeft,
        {50.5f, 70.7f}},
    {"line right", Text::Alignment::LineRight,
        {50.5f + 200.8f - 9.0f, 70.7f}},
    {"top center", Text::Alignment::TopCenter,
        {50.5f + 100.4f - 4.5f, 20.5f + 7.0f}},
    {"top center, interal", Text::Alignment::TopCenterIntegral,
        /* Only the offset inside the node and the bounding box is rounded,
           not the node offset itself; not the Y coordinate either */
        {50.5f + 100.0f - 5.0f, 20.5f + 7.0f}},
    {"bottom left", Text::Alignment::BottomLeft,
        {50.5f, 120.9f - 4.0f}},
    {"middle right", Text::Alignment::MiddleRight,
        {50.5f + 200.8f - 9.0f, 20.5f + 50.2f - 5.5f + 7.0f}},
    {"middle right, integral", Text::Alignment::MiddleRightIntegral,
        /* Only the offset inside the node and the bounding box is rounded,
           not the node offset itself; not the X coordinate either. Note that
           the Y rounding is in the other direction compared to X because of Y
           flip. */
        {50.5f + 200.8f - 9.0f, 20.5f + 50.0f - 5.0f + 7.0f}},
    {"middle center", Text::Alignment::MiddleCenter,
        {50.5f + 100.4f - 4.5f, 20.5f + 50.2f - 5.5f + 7.0f}},
    {"middle center, integral", Text::Alignment::MiddleCenterIntegral,
        /* Only the offset inside the node and the bounding box is rounded,
           not the node offset itself. Note that the Y rounding is in the other
           direction compared to X because of Y flip. */
        {50.5f + 100.0f - 5.0f, 20.5f + 50.0f - 5.0f + 7.0f}},
};

const struct {
    const char* name;
    bool emptyUpdate;
} UpdateCleanDataOrderData[]{
    {"empty update", true},
    {"", false},
};

TextLayerTest::TextLayerTest() {
    addTests({&TextLayerTest::styleSizeAlignment<TextLayerStyleCommon>,
              &TextLayerTest::styleSizeAlignment<TextLayerStyleItem>,

              &TextLayerTest::styleCommonConstructDefault,
              &TextLayerTest::styleCommonConstructNoInit,
              &TextLayerTest::styleCommonSetters,

              &TextLayerTest::styleItemConstructDefault,
              &TextLayerTest::styleItemConstructNoInit,
              &TextLayerTest::styleItemSetters,

              &TextLayerTest::fontHandle,
              &TextLayerTest::fontHandleInvalid,
              &TextLayerTest::debugFontHandle,

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
              &TextLayerTest::sharedFontInvalidHandle,

              &TextLayerTest::sharedSetStyleFonts,
              &TextLayerTest::sharedSetStyleFontsInvalidSize,
              &TextLayerTest::sharedSetStyleFontsInvalidHandle,

              &TextLayerTest::construct,
              &TextLayerTest::constructCopy,
              &TextLayerTest::constructMove});

    addInstancedTests<TextLayerTest>({&TextLayerTest::createRemoveSetText<UnsignedInt>,
                                      &TextLayerTest::createRemoveSetText<Enum>},
        Containers::arraySize(CreateRemoveSetTextData));

    addTests({&TextLayerTest::createSetTextTextProperties,
              &TextLayerTest::createNoSharedGlyphCache,

              &TextLayerTest::setColor,

              &TextLayerTest::invalidHandle,
              &TextLayerTest::invalidFontHandle,
              &TextLayerTest::noSharedStyleFonts,
              &TextLayerTest::styleOutOfRange,

              &TextLayerTest::updateEmpty});

    addInstancedTests({&TextLayerTest::updateCleanDataOrder},
        Containers::arraySize(UpdateCleanDataOrderData));

    addInstancedTests({&TextLayerTest::updateAlignment},
        Containers::arraySize(UpdateAlignmentData));
}

using namespace Containers::Literals;
using namespace Math::Literals;

template<class> struct StyleTraits;
template<> struct StyleTraits<TextLayerStyleCommon> {
    static const char* name() { return "TextLayerStyleCommon"; }
};
template<> struct StyleTraits<TextLayerStyleItem> {
    static const char* name() { return "TextLayerStyleItem"; }
};

template<class T> void TextLayerTest::styleSizeAlignment() {
    setTestCaseTemplateName(StyleTraits<T>::name());

    CORRADE_FAIL_IF(sizeof(T) % sizeof(Vector4) != 0, sizeof(T) << "is not a multiple of vec4 for UBO alignment.");

    /* 48-byte structures are fine, we'll align them to 768 bytes and not
       256, but warn about that */
    CORRADE_FAIL_IF(768 % sizeof(T) != 0, sizeof(T) << "can't fit exactly into 768-byte UBO alignment.");
    if(256 % sizeof(T) != 0)
        CORRADE_WARN(sizeof(T) << "can't fit exactly into 256-byte UBO alignment, only 768.");

    CORRADE_COMPARE(alignof(T), 4);
}

void TextLayerTest::styleCommonConstructDefault() {
    TextLayerStyleCommon a;
    TextLayerStyleCommon b{DefaultInit};
    /* No actual fields yet */
    static_cast<void>(a);
    static_cast<void>(b);

    constexpr TextLayerStyleCommon ca;
    constexpr TextLayerStyleCommon cb{DefaultInit};
    /* No actual fields yet */
    static_cast<void>(ca);
    static_cast<void>(cb);

    CORRADE_VERIFY(std::is_nothrow_default_constructible<TextLayerStyleCommon>::value);
    CORRADE_VERIFY(std::is_nothrow_constructible<TextLayerStyleCommon, DefaultInitT>::value);

    /* Implicit construction is not allowed */
    CORRADE_VERIFY(!std::is_convertible<DefaultInitT, TextLayerStyleCommon>::value);
}

void TextLayerTest::styleCommonConstructNoInit() {
    /* Testing only some fields, should be enough */
    TextLayerStyleCommon a;
    /* No actual fields yet */
    static_cast<void>(a);

    new(&a) TextLayerStyleCommon{NoInit};
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

void TextLayerTest::styleCommonSetters() {
    TextLayerStyleCommon a;
    static_cast<void>(a);
    CORRADE_SKIP("No actual fields yet");
}

void TextLayerTest::styleItemConstructDefault() {
    TextLayerStyleItem a;
    TextLayerStyleItem b{DefaultInit};
    CORRADE_COMPARE(a.color, 0xffffffff_srgbaf);
    CORRADE_COMPARE(b.color, 0xffffffff_srgbaf);

    constexpr TextLayerStyleItem ca;
    constexpr TextLayerStyleItem cb{DefaultInit};
    CORRADE_COMPARE(ca.color, 0xffffffff_srgbaf);
    CORRADE_COMPARE(cb.color, 0xffffffff_srgbaf);

    CORRADE_VERIFY(std::is_nothrow_default_constructible<TextLayerStyleItem>::value);
    CORRADE_VERIFY(std::is_nothrow_constructible<TextLayerStyleItem, DefaultInitT>::value);

    /* Implicit construction is not allowed */
    CORRADE_VERIFY(!std::is_convertible<DefaultInitT, TextLayerStyleItem>::value);
}

void TextLayerTest::styleItemConstructNoInit() {
    /* Testing only some fields, should be enough */
    TextLayerStyleItem a;
    a.color = 0xff3366_rgbf;

    new(&a) TextLayerStyleItem{NoInit};
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

void TextLayerTest::styleItemSetters() {
    TextLayerStyleItem a;
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

void TextLayerTest::sharedConstruct() {
    struct Shared: TextLayer::Shared {
        explicit Shared(UnsignedInt styleCount): TextLayer::Shared{styleCount} {}
    } shared{3};
    CORRADE_COMPARE(shared.styleCount(), 3);

    CORRADE_COMPARE(shared.fontCount(), 0);
    CORRADE_VERIFY(!shared.isHandleValid(FontHandle::Null));
}

void TextLayerTest::sharedConstructNoCreate() {
    struct Shared: TextLayer::Shared {
        explicit Shared(NoCreateT): TextLayer::Shared{NoCreate} {}
    } shared{NoCreate};

    /* Shouldn't crash */
    CORRADE_VERIFY(true);

    /* Implicit construction is not allowed */
    CORRADE_VERIFY(!std::is_convertible<NoCreateT, TextLayer::Shared>::value);
}

void TextLayerTest::sharedConstructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<TextLayer::Shared>{});
    CORRADE_VERIFY(!std::is_copy_assignable<TextLayer::Shared>{});
}

void TextLayerTest::sharedConstructMove() {
    struct Shared: TextLayer::Shared {
        explicit Shared(UnsignedInt styleCount): TextLayer::Shared{styleCount} {}
    };

    Shared a{3};

    Shared b{Utility::move(a)};
    CORRADE_COMPARE(b.styleCount(), 3);

    Shared c{5};
    c = Utility::move(b);
    CORRADE_COMPARE(c.styleCount(), 3);

    CORRADE_VERIFY(std::is_nothrow_move_constructible<TextLayer::Shared>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<TextLayer::Shared>::value);
}

void TextLayerTest::sharedSetGlyphCache() {
    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};

    struct Shared: TextLayer::Shared {
        explicit Shared(UnsignedInt styleCount): TextLayer::Shared{styleCount} {}

        using TextLayer::Shared::setGlyphCache;
    } shared{3};
    shared.setGlyphCache(cache);
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
        explicit Shared(UnsignedInt styleCount): TextLayer::Shared{styleCount} {}

        using TextLayer::Shared::setGlyphCache;
    } shared{3};
    shared.setGlyphCache(cache);

    std::ostringstream out;
    Error redirectError{&out};
    shared.setGlyphCache(cache);
    CORRADE_COMPARE(out.str(), "Whee::TextLayer::Shared::setGlyphCache(): glyph cache already set\n");
}

void TextLayerTest::sharedNoGlyphCache() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Shared: TextLayer::Shared {
        explicit Shared(UnsignedInt styleCount): TextLayer::Shared{styleCount} {}
    } shared{3};

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
        explicit Shared(UnsignedInt styleCount): TextLayer::Shared{styleCount} {}

        using TextLayer::Shared::setGlyphCache;
    } shared{3};
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
        bool doIsOpened() const override { return false; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return {}; }
    } font1, font2;

    /* First font */
    cache.addFont(13, &font1);
    FontHandle first = shared.addFont(font1, 13.0f);
    CORRADE_COMPARE(first, Whee::fontHandle(0, 1));
    CORRADE_COMPARE(shared.fontCount(), 1);
    CORRADE_VERIFY(shared.isHandleValid(first));
    CORRADE_COMPARE(&shared.font(first), &font1);
    /* Const overload */
    CORRADE_COMPARE(&const_cast<const Shared&>(shared).font(first), &font1);

    /* Second font */
    cache.addFont(56, &font2);
    FontHandle second = shared.addFont(font2, 6.0f);
    CORRADE_COMPARE(second, Whee::fontHandle(1, 1));
    CORRADE_COMPARE(shared.fontCount(), 2);
    CORRADE_VERIFY(shared.isHandleValid(second));
    CORRADE_COMPARE(&shared.font(second), &font2);
    /* Const overload */
    CORRADE_COMPARE(&const_cast<const Shared&>(shared).font(second), &font2);
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
        bool doIsOpened() const override { return false; }
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
            explicit Shared(UnsignedInt styleCount): TextLayer::Shared{styleCount} {}

            using TextLayer::Shared::setGlyphCache;
        } shared{3};
        shared.setGlyphCache(cache);
        CORRADE_COMPARE(shared.fontCount(), 0);

        Containers::Pointer<Font> font1{InPlaceInit, destructed};
        cache.addFont(13, font1.get());
        Font* pointer1 = font1.get();
        FontHandle first = shared.addFont(Utility::move(font1), 13.0f);
        CORRADE_COMPARE(first, Whee::fontHandle(0, 1));
        CORRADE_COMPARE(shared.fontCount(), 1);
        CORRADE_VERIFY(shared.isHandleValid(first));
        CORRADE_COMPARE(&shared.font(first), pointer1);

        /* It should be possible to add a second font using the same pointer
           but different options */
        FontHandle second = shared.addFont(*pointer1, 6.0f);
        CORRADE_COMPARE(second, Whee::fontHandle(1, 1));
        CORRADE_COMPARE(shared.fontCount(), 2);
        CORRADE_VERIFY(shared.isHandleValid(second));
        CORRADE_COMPARE(&shared.font(second), pointer1);

        /* Add a second font, to verify both get deleted appropriately */
        Containers::Pointer<Font> font2{InPlaceInit, destructed};
        cache.addFont(13, font2.get());
        Font* pointer2 = font2.get();
        FontHandle third = shared.addFont(Utility::move(font2), 22.0f);
        CORRADE_COMPARE(third, Whee::fontHandle(2, 1));
        CORRADE_COMPARE(shared.fontCount(), 3);
        CORRADE_VERIFY(shared.isHandleValid(third));
        CORRADE_COMPARE(&shared.font(third), pointer2);
    }

    /* The owned instances should be destructed exactly once */
    CORRADE_COMPARE(destructed, 2);
}

void TextLayerTest::sharedAddFontTakeOwnershipNull() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Shared: TextLayer::Shared {
        explicit Shared(UnsignedInt styleCount): TextLayer::Shared{styleCount} {}
    } shared{3};
    CORRADE_COMPARE(shared.fontCount(), 0);

    std::ostringstream out;
    Error redirectError{&out};
    shared.addFont(nullptr, 13.0f);
    CORRADE_COMPARE(out.str(), "Whee::TextLayer::Shared::addFont(): font is null\n");
}

void TextLayerTest::sharedAddFontNoCache() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Shared: TextLayer::Shared {
        explicit Shared(UnsignedInt styleCount): TextLayer::Shared{styleCount} {}
    } shared{3};

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
    CORRADE_COMPARE(out.str(), "Whee::TextLayer::Shared::addFont(): no glyph cache set\n");
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
        explicit Shared(UnsignedInt styleCount): TextLayer::Shared{styleCount} {}

        using TextLayer::Shared::setGlyphCache;
    } shared{3};
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
    CORRADE_COMPARE(out.str(), "Whee::TextLayer::Shared::addFont(): font not found among 2 fonts in set glyph cache\n");
}

void TextLayerTest::sharedAddFontNoHandlesLeft() {
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
    cache.addFont(67, &font);

    struct Shared: TextLayer::Shared {
        explicit Shared(UnsignedInt styleCount): TextLayer::Shared{styleCount} {}

        using TextLayer::Shared::setGlyphCache;
    } shared{3};
    shared.setGlyphCache(cache);

    FontHandle handle;
    for(std::size_t i = 0; i != 1 << Implementation::FontHandleIdBits; ++i)
        handle = shared.addFont(font, 1.0f);
    CORRADE_COMPARE(handle, Whee::fontHandle((1 << Implementation::FontHandleIdBits) - 1, 1));

    CORRADE_COMPARE(shared.fontCount(), 1 << Implementation::FontHandleIdBits);

    std::ostringstream out;
    Error redirectError{&out};
    shared.addFont(font, 1.0f);
    /* Number is hardcoded in the expected message but not elsewhere in order
       to give a heads-up when modifying the handle ID bit count */
    CORRADE_COMPARE(out.str(),
        "Whee::TextLayer::Shared::addFont(): can only have at most 32768 fonts\n");
}

void TextLayerTest::sharedFontInvalidHandle() {
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
    cache.addFont(67, &font);

    struct Shared: TextLayer::Shared {
        explicit Shared(UnsignedInt styleCount): TextLayer::Shared{styleCount} {}

        using TextLayer::Shared::setGlyphCache;
    } shared{3};
    shared.setGlyphCache(cache);

    /* Need to add at least one font because the assertion returns the first
       font as a fallback */
    shared.addFont(font, 13.0f);

    std::ostringstream out;
    Error redirectError{&out};
    shared.font(FontHandle(0x12ab));
    shared.font(FontHandle::Null);
    /* Const overload */
    const_cast<const Shared&>(shared).font(FontHandle(0x12ab));
    const_cast<const Shared&>(shared).font(FontHandle::Null);
    CORRADE_COMPARE(out.str(),
        "Whee::TextLayer::Shared::font(): invalid handle Whee::FontHandle(0x12ab, 0x0)\n"
        "Whee::TextLayer::Shared::font(): invalid handle Whee::FontHandle::Null\n"
        "Whee::TextLayer::Shared::font(): invalid handle Whee::FontHandle(0x12ab, 0x0)\n"
        "Whee::TextLayer::Shared::font(): invalid handle Whee::FontHandle::Null\n");
}

void TextLayerTest::sharedSetStyleFonts() {
    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};

    struct Font: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return false; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return {}; }
    };

    struct Shared: TextLayer::Shared {
        explicit Shared(UnsignedInt styleCount): TextLayer::Shared{styleCount} {}

        State& state() { return static_cast<State&>(*_state); }

        using TextLayer::Shared::setGlyphCache;
    } shared{3};
    shared.setGlyphCache(cache);

    /* By default they're all null handles */
    CORRADE_COMPARE_AS(shared.state().styleFonts, Containers::arrayView({
        FontHandle::Null,
        FontHandle::Null,
        FontHandle::Null
    }), TestSuite::Compare::Container);

    Font font1, font2;
    cache.addFont(67, &font1);
    cache.addFont(23, &font2);
    FontHandle first = shared.addFont(font1, 13.0f);
    FontHandle second = shared.addFont(font2, 6.0f);
    shared.setStyleFonts({first, second, first});
    CORRADE_COMPARE_AS(shared.state().styleFonts, Containers::arrayView({
        first,
        second,
        first
    }), TestSuite::Compare::Container);
}

void TextLayerTest::sharedSetStyleFontsInvalidSize() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Shared: TextLayer::Shared {
        explicit Shared(UnsignedInt styleCount): TextLayer::Shared{styleCount} {}
    } shared{3};

    std::ostringstream out;
    Error redirectError{&out};
    shared.setStyleFonts({FontHandle::Null, FontHandle::Null});
    CORRADE_COMPARE(out.str(), "Whee::TextLayer::Shared::setStyleFonts(): expected 3 fonts, got 2\n");
}

void TextLayerTest::sharedSetStyleFontsInvalidHandle() {
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
    cache.addFont(67, &font);

    struct Shared: TextLayer::Shared {
        explicit Shared(UnsignedInt styleCount): TextLayer::Shared{styleCount} {}

        using TextLayer::Shared::setGlyphCache;
    } shared{4};
    shared.setGlyphCache(cache);

    FontHandle handle = shared.addFont(font, 13.0f);

    std::ostringstream out;
    Error redirectError{&out};
    shared.setStyleFonts({handle, FontHandle(0x12ab), handle, handle});
    shared.setStyleFonts({handle, handle, FontHandle::Null, handle});
    CORRADE_COMPARE(out.str(),
        "Whee::TextLayer::Shared::setStyleFonts(): invalid handle Whee::FontHandle(0x12ab, 0x0) at index 1\n"
        "Whee::TextLayer::Shared::setStyleFonts(): invalid handle Whee::FontHandle::Null at index 2\n");
}

void TextLayerTest::construct() {
    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(UnsignedInt styleCount): TextLayer::Shared{styleCount} {}
    } shared{3};

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(137, 0xfe), shared};

    /* There isn't anything to query on the TextLayer itself */
    CORRADE_COMPARE(layer.handle(), layerHandle(137, 0xfe));
}

void TextLayerTest::constructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<TextLayer>{});
    CORRADE_VERIFY(!std::is_copy_assignable<TextLayer>{});
}

void TextLayerTest::constructMove() {
    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(UnsignedInt styleCount): TextLayer::Shared{styleCount} {}
    };

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, LayerShared& shared): TextLayer{handle, shared} {}
    };

    LayerShared shared{3};
    LayerShared shared2{5};

    Layer a{layerHandle(137, 0xfe), shared};

    Layer b{Utility::move(a)};
    /* There isn't anything to query on the TextLayer itself */
    CORRADE_COMPARE(b.handle(), layerHandle(137, 0xfe));

    Layer c{layerHandle(0, 2), shared2};
    c = Utility::move(b);
    CORRADE_COMPARE(c.handle(), layerHandle(137, 0xfe));

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

template<class T> void TextLayerTest::createRemoveSetText() {
    auto&& data = CreateRemoveSetTextData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName(std::is_same<T, Enum>::value ? "Enum" : "UnsignedInt");

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

    /* Sizes, layers and offsets in the glyph cache are only used in
       doUpdate() so they can be arbitrary. Here it uses just the glyph ID
       mapping. */
    {
        UnsignedInt fontId = cache.addFont(threeGlyphFont.glyphCount(), &threeGlyphFont);
        cache.addGlyph(fontId, 97, {3000, 1000}, 13, {{7, 23}, {18, 30}});
        /* Glyph 22 deliberately omitted */
        cache.addGlyph(fontId, 13, {2000, 2000}, 6, {{30, 2}, {32, 26}});
    } {
        UnsignedInt fontId = cache.addFont(oneGlyphFont.glyphCount(), &oneGlyphFont);
        cache.addGlyph(fontId, 66, {1000, 3000}, 9, {{7, 8}, {9, 10}});
    }

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(UnsignedInt styleCount): TextLayer::Shared{styleCount} {}

        using TextLayer::Shared::setGlyphCache;
    } shared{3};
    shared.setGlyphCache(cache);

    /* The three-glyph font is scaled to 0.5, the one-glyph to 2.0 */
    FontHandle threeGlyphFontHandle = shared.addFont(threeGlyphFont, 8.0f);
    FontHandle oneGlyphFontHandle = shared.addFont(oneGlyphFont, 4.0f);

    /* If using custom fonts, set the style to either something completely
       different or not set them at all -- they shouldn't get used for
       anything */
    if(!data.customFont)
        shared.setStyleFonts({threeGlyphFontHandle, threeGlyphFontHandle, oneGlyphFontHandle});
    else if(!data.noStyleFonts)
        shared.setStyleFonts({oneGlyphFontHandle, oneGlyphFontHandle, threeGlyphFontHandle});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}

        const State& stateData() const {
            return static_cast<const State&>(*_state);
        }
    } layer{layerHandle(0, 1), shared};

    /* Default color */
    DataHandle first = layer.create(
        T(1),
        "hello",
        TextProperties{}
            .setFont(data.customFont ? threeGlyphFontHandle : FontHandle::Null),
        data.node);
    CORRADE_COMPARE(layer.node(first), data.node);
    CORRADE_COMPARE(layer.style(first), 1);
    CORRADE_COMPARE(layer.color(first), 0xffffff_rgbf);
    CORRADE_COMPARE(layer.state(), data.state);

    /* Custom color, testing also the getter overloads and templates */
    DataHandle second = layer.create(
        T(2),
        "ahoy",
        TextProperties{}
            .setFont(data.customFont ? oneGlyphFontHandle : FontHandle::Null),
        0xff3366_rgbf,
        data.node);
    CORRADE_COMPARE(layer.node(second), data.node);
    if(data.layerDataHandleOverloads) {
        CORRADE_COMPARE(layer.style(dataHandleData(second)), 2);
        /* Can't use T, as the function restricts to enum types which would
           fail for T == UnsignedInt */
        CORRADE_COMPARE(layer.template style<Enum>(dataHandleData(second)), Enum(2));
        CORRADE_COMPARE(layer.color(dataHandleData(second)), 0xff3366_rgbf);
    } else {
        CORRADE_COMPARE(layer.style(second), 2);
        /* Can't use T, as the function restricts to enum types which would
           fail for T == UnsignedInt */
        CORRADE_COMPARE(layer.template style<Enum>(second), Enum(2));
        CORRADE_COMPARE(layer.color(second), 0xff3366_rgbf);
    }
    CORRADE_COMPARE(layer.state(), data.state);

    /* Empty text */
    DataHandle third = layer.create(
        T(1),
        "",
        TextProperties{}
            .setFont(data.customFont ? threeGlyphFontHandle : FontHandle::Null),
        data.node);
    CORRADE_COMPARE(layer.node(third), data.node);
    CORRADE_COMPARE(layer.style(third), 1);
    CORRADE_COMPARE(layer.color(third), 0xffffff_rgbf);
    CORRADE_COMPARE(layer.state(), data.state);

    DataHandle fourth = layer.create(
        T(0),
        "hi",
        TextProperties{}
            .setFont(data.customFont ? threeGlyphFontHandle : FontHandle::Null),
        data.node);
    CORRADE_COMPARE(layer.node(fourth), data.node);
    CORRADE_COMPARE(layer.style(fourth), 0);
    CORRADE_COMPARE(layer.color(fourth), 0xffffff_rgbf);
    CORRADE_COMPARE(layer.state(), data.state);

    /* There should be four glyph runs, assigned to the four data */
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().data).slice(&Implementation::TextLayerData::glyphRun), Containers::arrayView({
        0u, 1u, 2u, 3u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::glyphOffset), Containers::arrayView({
        /* The second text is using the OneGlyphShaper, so it's just one glyph;
           third is empty */
        0u, 5u, 6u, 6u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::glyphCount), Containers::arrayView({
        /* The second text is using the OneGlyphShaper, so it's just one glyph;
           third is empty */
        5u, 1u, 0u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::data), Containers::arrayView({
        0u, 1u, 2u, 3u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphData).slice(&Implementation::TextLayerGlyphData::glyphId), Containers::arrayView({
        /* Glyphs 22, 13, 97, 22, 13; glyph 22 isn't in the cache */
        0u, 2u, 1u, 0u, 2u,
        /* Glyph 66 */
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
        /* "ahoy", single glyph */
        { 0.5f, -1.5f},
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
    CORRADE_COMPARE(layer.state(), data.state);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().data).slice(&Implementation::TextLayerData::glyphRun), Containers::arrayView({
        0u, 1u, 2u, 3u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::glyphOffset), Containers::arrayView({
        0u, 5u, 6u, 0xffffffffu
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::glyphCount), Containers::arrayView({
        5u, 1u, 0u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::data), Containers::arrayView({
        0u, 1u, 2u, 3u
    }), TestSuite::Compare::Container);

    /* Modifying a text creates a new run at the end, marks the original run as
       unused and marks the layer as needing an update. It's possible to switch
       to a different font, in this case from the one-glyph font to the
       three-glyph one. */
    if(data.layerDataHandleOverloads) layer.setText(
        dataHandleData(second),
        "hey",
        TextProperties{}
            .setFont(data.customFont ? threeGlyphFontHandle : FontHandle::Null));
    else layer.setText(
        second,
        "hey",
        TextProperties{}
            .setFont(data.customFont ? threeGlyphFontHandle : FontHandle::Null));
    CORRADE_COMPARE(layer.state(), data.state|LayerState::NeedsUpdate);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().data).slice(&Implementation::TextLayerData::glyphRun), Containers::arrayView({
        0u, 4u, 2u, 3u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::glyphOffset), Containers::arrayView({
        0u, 0xffffffffu, 6u, 0xffffffffu, 8u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::glyphCount), Containers::arrayView({
        5u, 1u, 0u, 2u, data.customFont ? 3u : 1u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::data), Containers::arrayView({
        0u, 1u, 2u, 3u, 1u
    }), TestSuite::Compare::Container);
    if(data.customFont) {
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphData).slice(&Implementation::TextLayerGlyphData::glyphId), Containers::arrayView({
            /* Glyphs 22, 13, 97, 22, 13; glyph 22 isn't in the cache */
            0u, 2u, 1u, 0u, 2u,
            /* Now-unused "ahoy" text */
            3u,
            /* Nothing for third text */
            /* Glyphs 22, 13 */
            0u, 2u,
            /* Glyphs 22, 13, 97; glyph 22 isn't in the cache */
            0u, 2u, 1u,
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphData).slice(&Implementation::TextLayerGlyphData::position), Containers::arrayView<Vector2>({
            /* "hello", aligned to MiddleCenter */
            {-5.0f, -0.5f},
            {-3.5f,  0.0f},
            {-1.5f,  0.5f},
            { 1.0f,  1.0f},
            { 4.0f,  1.5f},
            /* Now-unused "ahoy" text */
            { 0.5f, -1.5f},
            /* Third text is empty */
            /* "hi", aligned to MiddleCenter */
            {-1.25f, -0.5f},
            { 0.25f,  0.0f},
            /* "hey", aligned to MiddleCenter */
            {-2.25f, -0.5f},
            {-0.75f,  0.0f},
            { 1.25f,  0.5f},
        }), TestSuite::Compare::Container);
    } else {
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphData).slice(&Implementation::TextLayerGlyphData::glyphId), Containers::arrayView({
            /* Glyphs 22, 13, 97, 22, 13; glyph 22 isn't in the cache */
            0u, 2u, 1u, 0u, 2u,
            /* Now-unused "ahoy" text */
            3u,
            /* Nothing for third text */
            /* Glyphs 22, 13 */
            0u, 2u,
            /* Glyph 66 */
            3u,
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphData).slice(&Implementation::TextLayerGlyphData::position), Containers::arrayView<Vector2>({
            /* "hello", aligned to MiddleCenter */
            {-5.0f, -0.5f},
            {-3.5f,  0.0f},
            {-1.5f,  0.5f},
            { 1.0f,  1.0f},
            { 4.0f,  1.5f},
            /* Now-unused "ahoy" text */
            { 0.5f, -1.5f},
            /* Third text is empty */
            /* "hi", aligned to MiddleCenter */
            {-1.25f, -0.5f},
            { 0.25f,  0.0f},
            /* "hey", aligned to MiddleCenter */
            { 0.5f, -1.5f},
        }), TestSuite::Compare::Container);
    }
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
        explicit LayerShared(UnsignedInt styleCount): TextLayer::Shared{styleCount} {}

        using TextLayer::Shared::setGlyphCache;
    } shared{1};
    shared.setGlyphCache(cache);

    FontHandle fontHandle = shared.addFont(font, 16.0f);
    shared.setStyleFonts({fontHandle});

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
}

void TextLayerTest::createNoSharedGlyphCache() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(UnsignedInt styleCount): TextLayer::Shared{styleCount} {}
    } shared{3};

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    std::ostringstream out;
    Error redirectError{&out};
    layer.create(2, "", {});
    CORRADE_COMPARE(out.str(), "Whee::TextLayer::create(): no glyph cache was set\n");
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
        explicit LayerShared(UnsignedInt styleCount): TextLayer::Shared{styleCount} {}

        using TextLayer::Shared::setGlyphCache;
    } shared{1};
    shared.setGlyphCache(cache);
    /* Interestingly enough, these two can't be chained together as on some
       compilers it'd call addFont() before setGlyphCache(), causing an
       assert */
    shared.setStyleFonts({shared.addFont(font, 1.0f)});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    /* Just to be sure the setters aren't picking up the first ever data
       always */
    layer.create(0, "", {});

    DataHandle data = layer.create(0, "", {}, 0xff3366_rgbf);
    CORRADE_COMPARE(layer.color(data), 0xff3366_rgbf);
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Setting a color marks the layer as dirty */
    layer.setColor(data, 0xaabbcc_rgbf);
    CORRADE_COMPARE(layer.color(data), 0xaabbcc_rgbf);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsUpdate);

    /* Testing also the other overload */
    layer.setColor(dataHandleData(data), 0x112233_rgbf);
    CORRADE_COMPARE(layer.color(data), 0x112233_rgbf);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsUpdate);
}

void TextLayerTest::invalidHandle() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(UnsignedInt styleCount): TextLayer::Shared{styleCount} {}

        using TextLayer::Shared::setGlyphCache;
    } shared{1};

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    std::ostringstream out;
    Error redirectError{&out};
    layer.setText(DataHandle::Null, "", {});
    layer.setText(LayerDataHandle::Null, "", {});
    layer.color(DataHandle::Null);
    layer.color(LayerDataHandle::Null);
    layer.setColor(DataHandle::Null, {});
    layer.setColor(LayerDataHandle::Null, {});
    CORRADE_COMPARE_AS(out.str(),
        "Whee::TextLayer::setText(): invalid handle Whee::DataHandle::Null\n"
        "Whee::TextLayer::setText(): invalid handle Whee::LayerDataHandle::Null\n"
        "Whee::TextLayer::color(): invalid handle Whee::DataHandle::Null\n"
        "Whee::TextLayer::color(): invalid handle Whee::LayerDataHandle::Null\n"
        "Whee::TextLayer::setColor(): invalid handle Whee::DataHandle::Null\n"
        "Whee::TextLayer::setColor(): invalid handle Whee::LayerDataHandle::Null\n",
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
        explicit LayerShared(UnsignedInt styleCount): TextLayer::Shared{styleCount} {}

        using TextLayer::Shared::setGlyphCache;
    } shared{1};
    shared.setGlyphCache(cache);
    /* Interestingly enough, these two can't be chained together as on some
       compilers it'd call addFont() before setGlyphCache(), causing an
       assert */
    shared.setStyleFonts({shared.addFont(font, 1.0f)});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    DataHandle data = layer.create(0, "", {});

    std::ostringstream out;
    Error redirectError{&out};
    layer.create(0, "", FontHandle(0x12ab));
    layer.setText(data, "", FontHandle(0x12ab));
    CORRADE_COMPARE(out.str(),
        "Whee::TextLayer::create(): invalid handle Whee::FontHandle(0x12ab, 0x0)\n"
        "Whee::TextLayer::setText(): invalid handle Whee::FontHandle(0x12ab, 0x0)\n");
}

void TextLayerTest::noSharedStyleFonts() {
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
        explicit LayerShared(UnsignedInt styleCount): TextLayer::Shared{styleCount} {}

        using TextLayer::Shared::setGlyphCache;
    } shared{1};
    shared.setGlyphCache(cache);

    FontHandle fontHandle = shared.addFont(font, 1.0f);

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    DataHandle data = layer.create(0, "", fontHandle);

    std::ostringstream out;
    Error redirectError{&out};
    layer.create(0, "", {});
    layer.setText(data, "", {});
    CORRADE_COMPARE(out.str(),
        "Whee::TextLayer::create(): no fonts were assigned to styles and no custom font was supplied\n"
        "Whee::TextLayer::setText(): no fonts were assigned to styles and no custom font was supplied\n");
}

void TextLayerTest::styleOutOfRange() {
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
    cache.addFont(56, &font);

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(UnsignedInt styleCount): TextLayer::Shared{styleCount} {}

        using TextLayer::Shared::setGlyphCache;
    } shared{3};
    shared.setGlyphCache(cache);

    FontHandle fontHandle = shared.addFont(font, 1.0f);

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    std::ostringstream out;
    Error redirectError{&out};
    layer.create(3, "", TextProperties{}.setFont(fontHandle));
    /* All overloads delegate to the same one, no need to check all */
    CORRADE_COMPARE(out.str(), "Whee::TextLayer::create(): style 3 out of range for 3 styles\n");
}

void TextLayerTest::updateEmpty() {
    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(UnsignedInt styleCount): TextLayer::Shared{styleCount} {}
    } shared{3};

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0, 1), shared};

    /* Shouldn't crash or do anything weird */
    layer.update({}, {}, {}, {}, {}, {}, {});
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
        explicit LayerShared(UnsignedInt styleCount): TextLayer::Shared{styleCount} {}

        using TextLayer::Shared::setGlyphCache;
    } shared{4};
    shared.setGlyphCache(cache);

    /* The three-glyph font is scaled to 0.5, the one-glyph to 2.0 */
    FontHandle threeGlyphFontHandle = shared.addFont(threeGlyphFont, 8.0f);
    FontHandle oneGlyphFontHandle = shared.addFont(oneGlyphFont, 4.0f);
    shared.setStyleFonts({oneGlyphFontHandle, oneGlyphFontHandle, threeGlyphFontHandle, threeGlyphFontHandle});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}

        const State& stateData() const {
            return static_cast<const State&>(*_state);
        }
    } layer{layerHandle(0, 1), shared};

    /* Two node handles to attach the data to */
    NodeHandle node6 = nodeHandle(6, 0);
    NodeHandle node15 = nodeHandle(15, 0);

    /* Create 10 data handles. Only three get filled and actually used. */
    layer.create(0, "", {});                            /* 0, quad 0 */
    layer.create(0, "", {});                            /* 1, quad 1 */
    layer.create(0, "", {});                            /* 2, quad 2 */
    layer.create(2, "hello", {}, 0xff3366_rgbf, node6); /* 3, quad 3 to 7 */
    layer.create(0, "", {});                            /* 4, quad 8 */
    layer.create(0, "", {});                            /* 5, quad 9 */
    layer.create(0, "", {});                            /* 6, quad 10 */
    DataHandle data7 = layer.create(1, "ahoy", {}, 0x112233_rgbf, node15);
                                                        /* 7, quad 11 */
    layer.create(0, "", {});                            /* 8, quad 12 */
    layer.create(3, "hi", {}, 0x663399_rgbf, node15);   /* 9, quad 13 to 14 */

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
    nodeOffsets[6] = {1.0f, 2.0f};
    nodeSizes[6] = {10.0f, 15.0f};
    nodeOffsets[15] = {3.0f, 4.0f};
    nodeSizes[15] = {20.0f, 5.0f};

    /* An empty update should generate an empty draw list */
    if(data.emptyUpdate) {
        layer.update({}, {}, {}, nodeOffsets, nodeSizes, {}, {});
        CORRADE_COMPARE_AS(layer.stateData().indices,
            Containers::ArrayView<const UnsignedInt>{},
            TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(layer.stateData().indexDrawOffsets, Containers::arrayView({
            0u
        }), TestSuite::Compare::Container);
        return;
    }

    /* Just the filled subset is getting updated */
    UnsignedInt dataIds[]{9, 7, 3};
    layer.update(dataIds, {}, {}, nodeOffsets, nodeSizes, {}, {});

    /* The indices should be filled just for the three items */
    CORRADE_COMPARE_AS(layer.stateData().indices, Containers::arrayView<UnsignedInt>({
        /* Text 9, "hi", quads 13 to 14 */
        13*4 + 0, 13*4 + 1, 13*4 + 2, 13*4 + 2, 13*4 + 1, 13*4 + 3,
        14*4 + 0, 14*4 + 1, 14*4 + 2, 14*4 + 2, 14*4 + 1, 14*4 + 3,
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
        CORRADE_COMPARE(layer.stateData().vertices[3*4 + i].style, 2);
    }
    for(std::size_t i = 0; i != 1*4; ++i) {
        CORRADE_ITERATION(i);
        CORRADE_COMPARE(layer.stateData().vertices[11*4 + i].color, 0x112233_rgbf);
        CORRADE_COMPARE(layer.stateData().vertices[11*4 + i].style, 1);
    }
    for(std::size_t i = 0; i != 2*4; ++i) {
        CORRADE_ITERATION(i);
        CORRADE_COMPARE(layer.stateData().vertices[13*4 + i].color, 0x663399_rgbf);
        CORRADE_COMPARE(layer.stateData().vertices[13*4 + i].style, 3);
    }

    Containers::StridedArrayView1D<const Vector2> positions = stridedArrayView(layer.stateData().vertices).slice(&Implementation::TextLayerVertex::position);
    Containers::StridedArrayView1D<const Vector3> textureCoordinates = stridedArrayView(layer.stateData().vertices).slice(&Implementation::TextLayerVertex::textureCoordinates);

    /* Text 3 is attached to node 6, which has a center of {6.0, 9.5}. Shaped
       positions should match what's in create() however as the coordinate
       system has Y up, the glyph positions have Y flipped compared in
       comparison to create():

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

    /* Glyph 13, at quad 4, 7, 14 */
    for(std::size_t i: {4, 7, 14}) {
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

    /* For drawing data 9, 7, 3 it needs to draw the first 2 quads in the
       index buffer, then next 1 quad, then next 5 */
    CORRADE_COMPARE_AS(layer.stateData().indexDrawOffsets, Containers::arrayView({
        0u, 2u*6, 3u*6, 8u*6
    }), TestSuite::Compare::Container);

    /* Removing a node with cleanNodes() marks the corresponding run as unused,
       and update() recompacts again */
    {
        UnsignedShort nodeGenerations[16];
        nodeGenerations[6] = nodeHandleGeneration(node6) + 1;
        nodeGenerations[15] = nodeHandleGeneration(node15);
        layer.cleanNodes(nodeGenerations);
    }

    /* The run corresponding to the removed data should be marked as unused,
       the rest stays the same */
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().data).slice(&Implementation::TextLayerData::glyphRun), Containers::arrayView({
        0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::glyphOffset), Containers::arrayView({
        0u, 1u, 2u, 0xffffffffu, 8u, 9u, 10u, 11u, 12u, 13u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::glyphCount), Containers::arrayView({
        1u, 1u, 1u, 5u, 1u, 1u, 1u, 1u, 1u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::data), Containers::arrayView({
        0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u
    }), TestSuite::Compare::Container);

    UnsignedInt dataIdsPostClean[]{9, 7};
    layer.update(dataIdsPostClean, {}, {}, nodeOffsets, nodeSizes, {}, {});

    /* There should be just 9 glyph runs, assigned to the remaining 9 data */
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().data).slice(&Implementation::TextLayerData::glyphRun), Containers::arrayView({
        0u, 1u, 2u, 3u /* free data */, 3u, 4u, 5u, 6u, 7u, 8u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::glyphOffset), Containers::arrayView({
        0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::glyphCount), Containers::arrayView({
        1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::data), Containers::arrayView({
        0u, 1u, 2u, 4u, 5u, 6u, 7u, 8u, 9u
    }), TestSuite::Compare::Container);

    /* Indices for remaining 3 visible glyphs */
    CORRADE_COMPARE_AS(layer.stateData().indices, Containers::arrayView<UnsignedInt>({
        /* Text 9, "hi", quads 8 to 9 */
        8*4 + 0, 8*4 + 1, 8*4 + 2, 8*4 + 2, 8*4 + 1, 8*4 + 3,
        9*4 + 0, 9*4 + 1, 9*4 + 2, 9*4 + 2, 9*4 + 1, 9*4 + 3,
        /* Text 7, "ahoy", quad 6 */
        6*4 + 0, 6*4 + 1, 6*4 + 2, 6*4 + 2, 6*4 + 1, 6*4 + 3,
        /* Text 3, "hello", is removed now */
    }), TestSuite::Compare::Container);

    /* Vertices for all remaining 10 glyphs */
    CORRADE_COMPARE(layer.stateData().vertices.size(), 10*4);
    for(std::size_t i = 0; i != 1*4; ++i) {
        CORRADE_ITERATION(i);
        CORRADE_COMPARE(layer.stateData().vertices[6*4 + i].color, 0x112233_rgbf);
        CORRADE_COMPARE(layer.stateData().vertices[6*4 + i].style, 1);
    }
    for(std::size_t i = 0; i != 2*4; ++i) {
        CORRADE_ITERATION(i);
        CORRADE_COMPARE(layer.stateData().vertices[8*4 + i].color, 0x663399_rgbf);
        CORRADE_COMPARE(layer.stateData().vertices[8*4 + i].style, 3);
    }

    /* Text 7 and 9, now quads 6 and 8 to 9 */
    CORRADE_COMPARE_AS(positions.sliceSize(6*4, 1*4), Containers::arrayView<Vector2>({
        {13.f + 0.5f        + 0.0f, 6.5f + 1.5f        - 0.0f},
        {13.f + 0.5f        + 32.f, 6.5f + 1.5f        - 0.0f},
        {13.f + 0.5f        + 0.0f, 6.5f + 1.5f        - 32.f},
        {13.f + 0.5f        + 32.f, 6.5f + 1.5f        - 32.f},
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(positions.sliceSize(8*4, 2*4), Containers::arrayView<Vector2>({
        {13.f - 1.25f,              6.5f + 0.5f},
        {13.f - 1.25f,              6.5f + 0.5f},
        {13.f - 1.25f,              6.5f + 0.5f},
        {13.f - 1.25f,              6.5f + 0.5f},

        {13.f + 0.25f + 2.f + 0.0f, 6.5f - 0.0f + 4.0f - 0.0f},
        {13.f + 0.25f + 2.f + 8.0f, 6.5f - 0.0f + 4.0f - 0.0f},
        {13.f + 0.25f + 2.f + 0.0f, 6.5f - 0.0f + 4.0f - 8.0f},
        {13.f + 0.25f + 2.f + 8.0f, 6.5f - 0.0f + 4.0f - 8.0f},
    }), TestSuite::Compare::Container);

    /* Glyph 22, now only at quad 8 */
    CORRADE_COMPARE_AS(textureCoordinates.sliceSize(8*4, 4), Containers::arrayView<Vector3>({
        {},
        {},
        {},
        {},
    }), TestSuite::Compare::Container);

    /* Glyph 13, now only at quad 9 */
    CORRADE_COMPARE_AS(textureCoordinates.sliceSize(9*4, 4), Containers::arrayView<Vector3>({
        {0.5f, 0.5f, 0.0f},
        {1.0f, 0.5f, 0.0f},
        {0.5f, 1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
    }), TestSuite::Compare::Container);

    /* Glyph 66, now at quad 6 */
    CORRADE_COMPARE_AS(textureCoordinates.sliceSize(6*4, 4), Containers::arrayView<Vector3>({
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
    CORRADE_COMPARE(layer.state(), LayerState::NeedsAttachmentUpdate);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::glyphOffset), Containers::arrayView({
        0u, 1u, 2u, 3u, 4u, 5u, 0xffffffffu, 7u, 8u
    }), TestSuite::Compare::Container);

    UnsignedInt dataIdsPostRemoval[]{9};
    layer.update(dataIdsPostRemoval, {}, {}, nodeOffsets, nodeSizes, {}, {});

    /* There should be just 8 glyph runs, assigned to the remaining 8 data */
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().data).slice(&Implementation::TextLayerData::glyphRun), Containers::arrayView({
        0u, 1u, 2u, 3u /* free data */, 3u, 4u, 5u, 6u /* free data */, 6u, 7u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::glyphOffset), Containers::arrayView({
        0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::glyphCount), Containers::arrayView({
        1u, 1u, 1u, 1u, 1u, 1u, 1u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::data), Containers::arrayView({
        0u, 1u, 2u, 4u, 5u, 6u, 8u, 9u
    }), TestSuite::Compare::Container);

    /* Indices for remaining 2 visible glyphs */
    CORRADE_COMPARE_AS(layer.stateData().indices, Containers::arrayView<UnsignedInt>({
        /* Text 9, "hi", quads 7 to 8 */
        7*4 + 0, 7*4 + 1, 7*4 + 2, 7*4 + 2, 7*4 + 1, 7*4 + 3,
        8*4 + 0, 8*4 + 1, 8*4 + 2, 8*4 + 2, 8*4 + 1, 8*4 + 3,
        /* Text 7, "ahoy", is removed now */
        /* Text 3, "hello", is removed now */
    }), TestSuite::Compare::Container);

    /* Vertices for all remaining 9 glyphs */
    CORRADE_COMPARE(layer.stateData().vertices.size(), 9*4);
    for(std::size_t i = 0; i != 2*4; ++i) {
        CORRADE_ITERATION(i);
        CORRADE_COMPARE(layer.stateData().vertices[7*4 + i].color, 0x663399_rgbf);
        CORRADE_COMPARE(layer.stateData().vertices[7*4 + i].style, 3);
    }

    /* Text 9, now quad 7 to 8 */
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

    /* For drawing data 9 it needs to draw the first 2 quads in the index
       buffer */
    CORRADE_COMPARE_AS(layer.stateData().indexDrawOffsets, Containers::arrayView({
        0u, 2u*6
    }), TestSuite::Compare::Container);
}

void TextLayerTest::updateAlignment() {
    auto&& data = UpdateAlignmentData[testCaseInstanceId()];
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
        explicit LayerShared(UnsignedInt styleCount): TextLayer::Shared{styleCount} {}

        using TextLayer::Shared::setGlyphCache;
    } shared{1};
    shared.setGlyphCache(cache);

    /* Font scaled 2x, so all metrics coming from the font or the cache should
       be scaled 2x */
    FontHandle fontHandle = shared.addFont(font, 200.0f);
    shared.setStyleFonts({fontHandle});

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
    nodeOffsets[3] = {50.5f, 20.5f};
    nodeSizes[3] = {200.8f, 100.4f};
    UnsignedInt dataIds[]{0};
    layer.update(dataIds, {}, {}, nodeOffsets, nodeSizes, {}, {});

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

}}}}

CORRADE_TEST_MAIN(Magnum::Whee::Test::TextLayerTest)
