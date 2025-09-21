/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023, 2024, 2025
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

#include <Corrade/Containers/BitArray.h>
#include <Corrade/Containers/Iterable.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/Reference.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/StridedBitArrayView.h>
#include <Corrade/Containers/String.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Container.h>
#include <Corrade/TestSuite/Compare/String.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/Animation/Easing.h>
#include <Magnum/Math/Time.h>
#include <Magnum/Text/AbstractGlyphCache.h>
#include <Magnum/Text/AbstractFont.h>
#include <Magnum/Text/AbstractShaper.h>
#include <Magnum/Text/Alignment.h>
#include <Magnum/Text/Feature.h>

#include "Magnum/Ui/AbstractUserInterface.h" /* for uiAdvance() */
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/TextLayer.h"
#include "Magnum/Ui/TextLayerAnimator.h"
#include "Magnum/Ui/TextProperties.h"
#include "Magnum/Ui/Implementation/textLayerState.h" /* for layerAdvance() */

namespace Magnum { namespace Ui { namespace Test { namespace {

struct TextLayerStyleAnimatorTest: TestSuite::Tester {
    explicit TextLayerStyleAnimatorTest();

    void debugAnimatorUpdate();
    void debugAnimatorUpdates();

    void construct();
    void constructCopy();
    void constructMove();

    void assignAnimator();
    void setDefaultStyleAnimator();
    /* There's no assert to trigger in assignAnimator() /
       setDefaultStyleAnimator() other than what's checked by
       AbstractVisualLayerStyleAnimatorTest::assignAnimatorInvalid() already */

    template<class T> void createRemove();
    void createRemoveHandleRecycle();
    void createInvalid();
    /* There's no assert to trigger in remove() other than what's checked by
       AbstractAnimator::remove() already */
    void propertiesInvalid();

    void advance();
    void advanceProperties();
    void advanceNoFreeDynamicStyles();
    void advanceConflictingAnimations();
    /* Nothing like BaseLayerStyleAnimatorTest::advanceExternalStyleChanges()
       as the whole logic is in the AbstractVisualLayerStyleAnimator already
       with nothing special in the subclasses. Population and reset of the
       expected style is tested sufficiently in createRemove() and
       createRemoveHandleRecycle(). */
    void advanceEmpty();
    void advanceInvalid();
    void advanceInvalidCursorSelection();

    void layerAdvance();
    void uiAdvance();
};

using namespace Math::Literals;

enum class Enum: UnsignedShort {};

Debug& operator<<(Debug& debug, Enum value) {
    return debug << UnsignedInt(value);
}

const struct {
    const char* name;
    bool samePaddingAfter, attachLaterAfter, cursorStyleBefore, selectionStyleBefore, cursorSelectionStyleAfter;
} CreateRemoveHandleRecycleData[]{
    {"",
        false, false, false, false, false},
    {"same paddings in recycled",
        true, false, false, false, false},
    {"attach recycled later",
        false, true, false, false, false},
    {"cursor style before but not after",
        false, false, true, false, false},
    {"selection style before but not after",
        false, false, false, true, false},
    {"cursor + selection style before but not after",
        false, false, true, true, false},
    {"cursor + selection style after but not before",
        false, false, false, false, true},
    {"cursor + selection style after but not before, same paddings",
        true, false, false, false, true},
};

const struct {
    const char* name;
    bool cursorStyles, selectionStyles;
} AdvanceData[]{
    {"", false, false},
    {"cursor styles", true, false},
    {"selection styles", false, true},
    {"cursor + selection styles", true, true},
};

const struct {
    const char* name;
    bool noAttachment;
    UnsignedInt uniform;
    Vector4 padding;
    Int cursorStyle, selectionStyle;
    UnsignedInt editingUniform;
    Int editingTextUniform1, editingTextUniform2;
    Vector4 editingPadding;
    TextLayerStyleAnimatorUpdates expectedUpdatesStart, expectedUpdatesMiddle;
    UnsignedInt expectedEditingTextUniform1, expectedEditingTextUniform2;
} AdvancePropertiesData[]{
    {"nothing changes, no editing styles",
        false, 1, Vector4{2.0f},
        -1, -1, 0, -1, -1, {},
        TextLayerStyleAnimatorUpdate::Uniform|TextLayerStyleAnimatorUpdate::Style,
        {}, 0, 0},
    {"nothing changes, no editing styles, no attachment",
        true, 1, Vector4{2.0f},
        -1, -1, 0, -1, -1, {},
        /* Uniform should be still set to trigger at least one upload of the
           dynamic style */
        TextLayerStyleAnimatorUpdate::Uniform,
        {}, 0, 0},
    {"nothing changes, cursor style",
        false, 1, Vector4{2.0f},
        1, -1, 0, -1, -1, Vector4{1.0f},
        TextLayerStyleAnimatorUpdate::Uniform|TextLayerStyleAnimatorUpdate::EditingUniform|TextLayerStyleAnimatorUpdate::Style,
        {}, 0, 0},
    {"nothing changes, cursor style, no attachment",
        true, 1, Vector4{2.0f},
        1, -1, 0, -1, -1, Vector4{1.0f},
        /* [Editing]Uniform should be still set to trigger at least one upload
           of the dynamic style */
        TextLayerStyleAnimatorUpdate::Uniform|TextLayerStyleAnimatorUpdate::EditingUniform,
        {}, 0, 0},
    {"nothing changes, selection style",
        false, 1, Vector4{2.0f},
        -1, 1, 2, 2, 2, Vector4{3.0f},
        TextLayerStyleAnimatorUpdate::Uniform|TextLayerStyleAnimatorUpdate::EditingUniform|TextLayerStyleAnimatorUpdate::Style,
        {}, 2, 2},
    {"nothing changes, selection style, no attachment",
        true, 1, Vector4{2.0f},
        -1, 1, 2, 2, 2, Vector4{3.0f},
        /* [Editing]Uniform should be still set to trigger at least one upload
           of the dynamic style */
        TextLayerStyleAnimatorUpdate::Uniform|TextLayerStyleAnimatorUpdate::EditingUniform,
        {}, 2, 2},
    {"nothing changes, selection style with unset text editing style",
        false, 1, Vector4{2.0f},
        /* Because the original uniform ID is unchanged, the text uniform ID
           (which falls back to the original uniform ID) is also unchanged */
        -1, 1, 2, -1, -1, Vector4{3.0f},
        TextLayerStyleAnimatorUpdate::Uniform|TextLayerStyleAnimatorUpdate::EditingUniform|TextLayerStyleAnimatorUpdate::Style,
        {}, 1, 1},
    {"nothing changes, selection style with one unset text editing style",
        false, 1, Vector4{2.0f},
        /* Same case */
        -1, 1, 2, 1, -1, Vector4{3.0f},
        TextLayerStyleAnimatorUpdate::Uniform|TextLayerStyleAnimatorUpdate::EditingUniform|TextLayerStyleAnimatorUpdate::Style,
        {}, 1, 1},
    {"nothing changes, selection style with another unset text editing style",
        false, 1, Vector4{2.0f},
        /* Same case */
        -1, 1, 2, -1, 1, Vector4{3.0f},
        TextLayerStyleAnimatorUpdate::Uniform|TextLayerStyleAnimatorUpdate::EditingUniform|TextLayerStyleAnimatorUpdate::Style,
        {}, 1, 1},

    {"uniform ID changes",
        false, 0, Vector4{2.0f},
        -1, -1, 0, -1, -1, {},
        TextLayerStyleAnimatorUpdate::Uniform|TextLayerStyleAnimatorUpdate::Style,
        TextLayerStyleAnimatorUpdate::Uniform, 0, 0},
    {"cursor uniform ID changes",
        false, 1, Vector4{2.0f},
        1, -1, 3, -1, -1, Vector4{1.0f},
        TextLayerStyleAnimatorUpdate::Uniform|TextLayerStyleAnimatorUpdate::EditingUniform|TextLayerStyleAnimatorUpdate::Style,
        TextLayerStyleAnimatorUpdate::EditingUniform, 0, 0},
    {"selection uniform ID changes",
        false, 1, Vector4{2.0f},
        -1, 1, 3, 2, 2, Vector4{3.0f},
        TextLayerStyleAnimatorUpdate::Uniform|TextLayerStyleAnimatorUpdate::EditingUniform|TextLayerStyleAnimatorUpdate::Style,
        TextLayerStyleAnimatorUpdate::EditingUniform, 2, 2},
    {"selection text uniform ID changes",
        false, 1, Vector4{2.0f},
        -1, 1, 2, 2, 1, Vector4{3.0f},
        TextLayerStyleAnimatorUpdate::Uniform|TextLayerStyleAnimatorUpdate::EditingUniform|TextLayerStyleAnimatorUpdate::Style,
        TextLayerStyleAnimatorUpdate::Uniform, 2, 1},
    {"selection text uniform ID changes, one unset",
        false, 1, Vector4{2.0f},
        -1, 1, 2, 2, -1, Vector4{3.0f},
        TextLayerStyleAnimatorUpdate::Uniform|TextLayerStyleAnimatorUpdate::EditingUniform|TextLayerStyleAnimatorUpdate::Style,
        TextLayerStyleAnimatorUpdate::Uniform, 2, 1},

    /* Still reports uniform change because comparing all values is unnecessary
       complexity */
    {"uniform ID changes but data stay the same",
        false, 3, Vector4{2.0f},
        -1, -1, 0, -1, -1, {},
        TextLayerStyleAnimatorUpdate::Uniform|TextLayerStyleAnimatorUpdate::Style,
        TextLayerStyleAnimatorUpdate::Uniform, 0, 0},
    {"cursor uniform ID changes but data stay the same",
        false, 1, Vector4{2.0f},
        1, -1, 4, -1, -1, Vector4{1.0f},
        TextLayerStyleAnimatorUpdate::Uniform|TextLayerStyleAnimatorUpdate::EditingUniform|TextLayerStyleAnimatorUpdate::Style,
        TextLayerStyleAnimatorUpdate::EditingUniform, 0, 0},
    {"selection uniform ID changes but data stay the same",
        false, 1, Vector4{2.0f},
        -1, 1, 3, 2, 2, Vector4{3.0f},
        TextLayerStyleAnimatorUpdate::Uniform|TextLayerStyleAnimatorUpdate::EditingUniform|TextLayerStyleAnimatorUpdate::Style,
        TextLayerStyleAnimatorUpdate::EditingUniform, 2, 2},
    {"selection text uniform ID changes but data stay the same",
        false, 1, Vector4{2.0f},
        -1, 1, 2, 2, 4, Vector4{3.0f},
        TextLayerStyleAnimatorUpdate::Uniform|TextLayerStyleAnimatorUpdate::EditingUniform|TextLayerStyleAnimatorUpdate::Style,
        TextLayerStyleAnimatorUpdate::Uniform, 2, 2}, /* text uniform 4 is same as 2 */

    {"padding changes",
        false, 1, Vector4{4.0f},
        -1, -1, 0, -1, -1, {},
        TextLayerStyleAnimatorUpdate::Uniform|TextLayerStyleAnimatorUpdate::Padding|TextLayerStyleAnimatorUpdate::Style,
        TextLayerStyleAnimatorUpdate::Padding, 0, 0},
    {"cursor padding changes",
        false, 1, Vector4{2.0f},
        1, -1, 0, -1, -1, Vector4{3.0f},
        TextLayerStyleAnimatorUpdate::Uniform|TextLayerStyleAnimatorUpdate::EditingUniform|TextLayerStyleAnimatorUpdate::EditingPadding|TextLayerStyleAnimatorUpdate::Style,
        TextLayerStyleAnimatorUpdate::EditingPadding, 0, 0},
    {"selection padding changes",
        false, 1, Vector4{2.0f},
        -1, 1, 2, 2, 2, Vector4{1.0f},
        TextLayerStyleAnimatorUpdate::Uniform|TextLayerStyleAnimatorUpdate::EditingUniform|TextLayerStyleAnimatorUpdate::EditingPadding|TextLayerStyleAnimatorUpdate::Style,
        TextLayerStyleAnimatorUpdate::EditingPadding, 2, 2},

    {"uniform ID + padding changes",
        false, 0, Vector4{4.0f},
        -1, -1, 0, -1, -1, {},
        TextLayerStyleAnimatorUpdate::Uniform|TextLayerStyleAnimatorUpdate::Padding|TextLayerStyleAnimatorUpdate::Style,
        TextLayerStyleAnimatorUpdate::Uniform|TextLayerStyleAnimatorUpdate::Padding, 0, 0},
    {"cursor uniform ID + cursor padding changes",
        false, 1, Vector4{2.0f},
        1, -1, 3, -1, -1, Vector4{3.0f},
        TextLayerStyleAnimatorUpdate::Uniform|TextLayerStyleAnimatorUpdate::EditingUniform|TextLayerStyleAnimatorUpdate::EditingPadding|TextLayerStyleAnimatorUpdate::Style,
        TextLayerStyleAnimatorUpdate::EditingUniform|TextLayerStyleAnimatorUpdate::EditingPadding, 0, 0},
    {"selection uniform ID + selection padding + selection text uniform changes",
        false, 1, Vector4{2.0f},
        -1, 1, 3, 2, 1, Vector4{1.0f},
        TextLayerStyleAnimatorUpdate::Uniform|TextLayerStyleAnimatorUpdate::EditingUniform|TextLayerStyleAnimatorUpdate::EditingPadding|TextLayerStyleAnimatorUpdate::Style,
        TextLayerStyleAnimatorUpdate::EditingUniform|TextLayerStyleAnimatorUpdate::EditingPadding|TextLayerStyleAnimatorUpdate::Uniform, 2, 1},
};

const struct {
    const char* name;
    AnimationFlags firstAnimationFlags;
    UnsignedInt firstAnimationRepeatCount;
    bool secondAnimationReverse, noFreeDynamicStyles;
    Containers::Optional<UnsignedInt> expectedSecondDynamicStyle;
    UnsignedInt expectedDynamicStyleCount;
} AdvanceConflictingAnimationsData[]{
    {"",
        {}, 1, false, false, 1, 1},
    {"no free dynamic styles",
        {}, 1, false, true, 0, 2},
    {"second animation reversed",
        {}, 1, true, false, 1, 1},
    {"second animation reversed, no free dynamic styles",
        {}, 1, true, true, 0, 2},
    {"first animation KeepOncePlayed",
        AnimationFlag::KeepOncePlayed, 1, false, false, 1, 1},
    {"first animation KeepOncePlayed, no free dynamic styles",
        AnimationFlag::KeepOncePlayed, 1, false, true, 0, 2},
    {"first animation endlessly repeating",
        {}, 0, false, false, 1, 1},
    {"first animation endlessly repeating, no free dynamic styles",
        {}, 0, false, true, 0, 2},
    {"first animation endlessly repeating, KeepOncePlayed",
        AnimationFlag::KeepOncePlayed, 0, false, false, 1, 2},
    {"first animation endlessly repeating, KeepOncePlayed, no free dynamic styles",
        AnimationFlag::KeepOncePlayed, 0, false, true, {}, 2},
};

const struct {
    const char* name;
    bool editingStyles;
    UnsignedInt uniform, editingUniform;
    Vector4 padding, editingPadding;
    bool expectDataChanges, expectCommonDataChanges;
} LayerAdvanceData[]{
    {"uniform changes",
        false, 0, 0, {}, {}, false, true},
    {"padding changes",
        false, 2, 0, Vector4{2.0f}, {}, true, false},
    {"uniform + padding changes",
        false, 0, 0, Vector4{2.0f}, {}, true, true},
    {"editing styles, uniform changes",
        true, 2, 0, {}, {}, false, true},
    {"editing styles, padding changes",
        true, 2, 1, {}, Vector4{2.0f}, true, false},
    {"editing styles, uniform + padding changes",
        true, 2, 0, Vector4{2.0f}, {}, true, true},
};

TextLayerStyleAnimatorTest::TextLayerStyleAnimatorTest() {
    addTests({&TextLayerStyleAnimatorTest::debugAnimatorUpdate,
              &TextLayerStyleAnimatorTest::debugAnimatorUpdates,

              &TextLayerStyleAnimatorTest::construct,
              &TextLayerStyleAnimatorTest::constructCopy,
              &TextLayerStyleAnimatorTest::constructMove,

              &TextLayerStyleAnimatorTest::assignAnimator,
              &TextLayerStyleAnimatorTest::setDefaultStyleAnimator,

              &TextLayerStyleAnimatorTest::createRemove<UnsignedInt>,
              &TextLayerStyleAnimatorTest::createRemove<Enum>});

    addInstancedTests({&TextLayerStyleAnimatorTest::createRemoveHandleRecycle},
        Containers::arraySize(CreateRemoveHandleRecycleData));

    addTests({&TextLayerStyleAnimatorTest::createInvalid,
              &TextLayerStyleAnimatorTest::propertiesInvalid});

    addInstancedTests({&TextLayerStyleAnimatorTest::advance},
        Containers::arraySize(AdvanceData));

    addInstancedTests({&TextLayerStyleAnimatorTest::advanceProperties},
        Containers::arraySize(AdvancePropertiesData));

    addTests({&TextLayerStyleAnimatorTest::advanceNoFreeDynamicStyles});

    addInstancedTests({&TextLayerStyleAnimatorTest::advanceConflictingAnimations},
        Containers::arraySize(AdvanceConflictingAnimationsData));

    addTests({&TextLayerStyleAnimatorTest::advanceEmpty,
              &TextLayerStyleAnimatorTest::advanceInvalid,
              &TextLayerStyleAnimatorTest::advanceInvalidCursorSelection});

    addInstancedTests({&TextLayerStyleAnimatorTest::layerAdvance},
        Containers::arraySize(LayerAdvanceData));

    addTests({&TextLayerStyleAnimatorTest::uiAdvance});
}

void TextLayerStyleAnimatorTest::debugAnimatorUpdate() {
    Containers::String out;
    Debug{&out} << TextLayerStyleAnimatorUpdate::Style << TextLayerStyleAnimatorUpdate(0xbe);
    CORRADE_COMPARE(out, "Ui::TextLayerStyleAnimatorUpdate::Style Ui::TextLayerStyleAnimatorUpdate(0xbe)\n");
}

void TextLayerStyleAnimatorTest::debugAnimatorUpdates() {
    Containers::String out;
    Debug{&out} << (TextLayerStyleAnimatorUpdate::Uniform|TextLayerStyleAnimatorUpdate(0xe0)) << TextLayerStyleAnimatorUpdates{};
    CORRADE_COMPARE(out, "Ui::TextLayerStyleAnimatorUpdate::Uniform|Ui::TextLayerStyleAnimatorUpdate(0xe0) Ui::TextLayerStyleAnimatorUpdates{}\n");
}

void TextLayerStyleAnimatorTest::construct() {
    TextLayerStyleAnimator animator{animatorHandle(0xab, 0x12)};

    CORRADE_COMPARE(animator.features(), AnimatorFeature::DataAttachment);
    CORRADE_COMPARE(animator.handle(), animatorHandle(0xab, 0x12));
    /* The rest is the same as in AbstractAnimatorTest::constructStyle() */
}

void TextLayerStyleAnimatorTest::constructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<TextLayerStyleAnimator>{});
    CORRADE_VERIFY(!std::is_copy_assignable<TextLayerStyleAnimator>{});
}

void TextLayerStyleAnimatorTest::constructMove() {
    /* Just verify that the subclass doesn't have the moves broken */

    TextLayerStyleAnimator a{animatorHandle(0xab, 0x12)};

    TextLayerStyleAnimator b{Utility::move(a)};
    CORRADE_COMPARE(b.handle(), animatorHandle(0xab, 0x12));

    TextLayerStyleAnimator c{animatorHandle(0xcd, 0x34)};
    c = Utility::move(b);
    CORRADE_COMPARE(c.handle(), animatorHandle(0xab, 0x12));

    CORRADE_VERIFY(std::is_nothrow_move_constructible<TextLayerStyleAnimator>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<TextLayerStyleAnimator>::value);
}

void TextLayerStyleAnimatorTest::assignAnimator() {
    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32}};

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{2}
        .setDynamicStyleCount(1)
    };

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    TextLayerStyleAnimator animator{animatorHandle(0, 1)};
    CORRADE_COMPARE(animator.layer(), LayerHandle::Null);

    layer.assignAnimator(animator);
    CORRADE_COMPARE(animator.layer(), layer.handle());
}

void TextLayerStyleAnimatorTest::setDefaultStyleAnimator() {
    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32}};

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{2}
        .setDynamicStyleCount(1)
    };

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    TextLayerStyleAnimator animator{animatorHandle(0, 1)};
    layer.assignAnimator(animator);
    CORRADE_COMPARE(animator.layer(), layer.handle());
    CORRADE_COMPARE(layer.defaultStyleAnimator(), nullptr);

    layer.setDefaultStyleAnimator(&animator);
    CORRADE_COMPARE(layer.defaultStyleAnimator(), &animator);
}

struct EmptyShaper: Text::AbstractShaper {
    using Text::AbstractShaper::AbstractShaper;

    UnsignedInt doShape(Containers::StringView, UnsignedInt, UnsignedInt, Containers::ArrayView<const Text::FeatureRange>) override {
        return 0;
    }
    void doGlyphIdsInto(const Containers::StridedArrayView1D<UnsignedInt>&) const override {}
    void doGlyphOffsetsAdvancesInto(const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&) const override {}
    void doGlyphClustersInto(const Containers::StridedArrayView1D<UnsignedInt>&) const override {}
};

template<class T> void TextLayerStyleAnimatorTest::createRemove() {
    setTestCaseTemplateName(std::is_same<T, Enum>::value ? "Enum" : "UnsignedInt");

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return Containers::pointer<EmptyShaper>(*this); }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(67, &font);

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{5, 6}
        .setEditingStyleCount(5, 4)
        .setDynamicStyleCount(1)
    };

    FontHandle fontHandle = shared.addFont(font, 1.0f);

    /* Have non-trivial uniform mapping to verify the data get correctly
       fetched. Has to be set early in order to call TextLayer::create() to
       attach the animation somewhere. TextLayerStyleAnimator::create() alone
       doesn't need the style to be set, which is tested elsewhere. */
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}                /* 0 */
            .setColor(0x112233_rgbf),
         TextLayerStyleUniform{}                /* 1 */
            .setColor(0xff3366_rgbf),
         TextLayerStyleUniform{}                /* 2 */
            .setColor(0xcc66aa_rgbf),
         TextLayerStyleUniform{}                /* 3, used by a selection */
            .setColor(0x111111_rgbf),
         TextLayerStyleUniform{}                /* 4 */
            .setColor(0x9933ff_rgbf)},
        {4, 1, 2, 4, 0, 1},
        {fontHandle, fontHandle, fontHandle,
         fontHandle, fontHandle, fontHandle},
        {Text::Alignment::MiddleCenter,
         Text::Alignment::MiddleCenter,
         Text::Alignment::MiddleCenter,
         Text::Alignment::MiddleCenter,
         Text::Alignment::MiddleCenter,
         Text::Alignment::MiddleCenter},
        {}, {}, {},
        /* Style 3 and 5 has associated cursor style */
        {-1, -1, -1, 2, -1, 0},
        /* Style 2 and 4 has a selection */
        {-1, -1, 1, -1, 3, -1},
        {Vector4{1.0f},
         {2.0f, 3.0f, 4.0f, 5.0f},
         {},
         {},
         Vector4{2.0f},
         Vector4{4.0f}});
    shared.setEditingStyle(TextLayerCommonEditingStyleUniform{},
        {TextLayerEditingStyleUniform{}         /* 0 */
            .setCornerRadius(4.0f),
         TextLayerEditingStyleUniform{},        /* 1 */
         TextLayerEditingStyleUniform{}         /* 2 */
            .setBackgroundColor(0x119900_rgbf),
         TextLayerEditingStyleUniform{}         /* 3 */
            .setBackgroundColor(0x337766_rgbf),
         TextLayerEditingStyleUniform{}         /* 4 */
            .setCornerRadius(5.0f)},
        {3, 0, 2, 4},
        /* Selection 1 overrides text color */
        {-1, 3, -1, -1},
        {{},
         Vector4{3.0f},
         {6.0f, 7.0f, 8.0f, 9.0f},
         Vector4{5.0f}});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    TextLayerStyleAnimator animator{animatorHandle(0, 1)};
    layer.assignAnimator(animator);

    /* The style used for the actual data shouldn't affect anything */
    DataHandle data1 = layer.create(1, "", {});
    DataHandle data2 = layer.create(2, "", {});
    DataHandle data3 = layer.create(0, "", {});

    /* The base overload. It shouldn't cause the data style to be changed to
       anything. */
    AnimationHandle first = animator.create(T(0), T(1), Animation::Easing::linear, 12_nsec, 13_nsec, data2, 10, AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.duration(first), 13_nsec);
    CORRADE_COMPARE(animator.repeatCount(first), 10);
    CORRADE_COMPARE(animator.flags(first), AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.started(first), 12_nsec);
    CORRADE_COMPARE(animator.data(first), data2);
    CORRADE_COMPARE(animator.styles(first), Containers::pair(0u, 1u));
    /* Can't use T, as the function restricts to enum types which would fail
       for T == UnsignedInt */
    CORRADE_COMPARE(animator.template styles<Enum>(first), Containers::pair(Enum(0), Enum(1)));
    CORRADE_COMPARE(animator.dynamicStyle(first), Containers::NullOpt);
    CORRADE_COMPARE(animator.easing(first), Animation::Easing::linear);
    /* Dynamic style is only allocated and switched to during advance() */
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 0);
    CORRADE_COMPARE(layer.style(data2), 2);

    /* Implicit repeat count, no data attachment (which thus shouldn't try to
       access anything data-related in the layer) */
    AnimationHandle second = animator.create(T(2), T(4), Animation::Easing::cubicIn, -15_nsec, 1_nsec, DataHandle::Null, AnimationFlag(0x40));
    CORRADE_COMPARE(animator.duration(second), 1_nsec);
    CORRADE_COMPARE(animator.repeatCount(second), 1);
    CORRADE_COMPARE(animator.flags(second), AnimationFlag(0x40));
    CORRADE_COMPARE(animator.started(second), -15_nsec);
    CORRADE_COMPARE(animator.data(second), DataHandle::Null);
    CORRADE_COMPARE(animator.styles(second), Containers::pair(2u, 4u));
    /* Can't use T, as the function restricts to enum types which would fail
       for T == UnsignedInt */
    CORRADE_COMPARE(animator.template styles<Enum>(second), Containers::pair(Enum(2), Enum(4)));
    CORRADE_COMPARE(animator.dynamicStyle(second), Containers::NullOpt);
    CORRADE_COMPARE(animator.easing(second), Animation::Easing::cubicIn);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 0);

    /* LayerDataHandle overload, verify also with AnimatorDataHandle */
    AnimationHandle third = animator.create(T(5), T(3), Animation::Easing::bounceInOut, 0_nsec, 100_nsec, dataHandleData(data3), 0, AnimationFlag(0x80));
    CORRADE_COMPARE(animator.duration(animationHandleData(third)), 100_nsec);
    CORRADE_COMPARE(animator.repeatCount(animationHandleData(third)), 0);
    CORRADE_COMPARE(animator.flags(animationHandleData(third)), AnimationFlag(0x80));
    CORRADE_COMPARE(animator.started(animationHandleData(third)), 0_nsec);
    CORRADE_COMPARE(animator.data(animationHandleData(third)), data3);
    CORRADE_COMPARE(animator.styles(animationHandleData(third)), Containers::pair(5u, 3u));
    /* Can't use T, as the function restricts to enum types which would fail
       for T == UnsignedInt */
    CORRADE_COMPARE(animator.template styles<Enum>(animationHandleData(third)), Containers::pair(Enum(5), Enum(3)));
    CORRADE_COMPARE(animator.dynamicStyle(animationHandleData(third)), Containers::NullOpt);
    CORRADE_COMPARE(animator.easing(animationHandleData(third)), Animation::Easing::bounceInOut);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 0);
    CORRADE_COMPARE(layer.style(data3), 0);

    /* LayerDataHandle overload with implicit repeat count */
    AnimationHandle fourth = animator.create(T(1), T(0), Animation::Easing::smoothstep, 20_nsec, 10_nsec, dataHandleData(data1), AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.duration(fourth), 10_nsec);
    CORRADE_COMPARE(animator.repeatCount(fourth), 1);
    CORRADE_COMPARE(animator.flags(fourth), AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.started(fourth), 20_nsec);
    CORRADE_COMPARE(animator.data(fourth), data1);
    CORRADE_COMPARE(animator.styles(fourth), Containers::pair(1u, 0u));
    /* Can't use T, as the function restricts to enum types which would fail
       for T == UnsignedInt */
    CORRADE_COMPARE(animator.template styles<Enum>(fourth), Containers::pair(Enum(1), Enum(0)));
    CORRADE_COMPARE(animator.dynamicStyle(fourth), Containers::NullOpt);
    CORRADE_COMPARE(animator.easing(fourth), Animation::Easing::smoothstep);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 0);
    CORRADE_COMPARE(layer.style(data1), 1);

    /* Removing an animation won't try to recycle the dynamic style, and won't
       attempt to switch the data style to anything else either */
    animator.remove(fourth);
    CORRADE_VERIFY(animator.isHandleValid(first));
    CORRADE_VERIFY(animator.isHandleValid(second));
    CORRADE_VERIFY(animator.isHandleValid(third));
    CORRADE_VERIFY(!animator.isHandleValid(fourth));
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 0);
    CORRADE_COMPARE(layer.style(data1), 1);

    /* Check the AnimatorDataHandle overload also. This one isn't attached to
       any data so it shouldn't attempt anything crazy either. */
    animator.remove(animationHandleData(second));
    CORRADE_VERIFY(animator.isHandleValid(first));
    CORRADE_VERIFY(!animator.isHandleValid(second));
    CORRADE_VERIFY(animator.isHandleValid(third));
    CORRADE_VERIFY(!animator.isHandleValid(fourth));
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 0);
}

void TextLayerStyleAnimatorTest::createRemoveHandleRecycle() {
    auto&& data = CreateRemoveHandleRecycleData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return Containers::pointer<EmptyShaper>(*this); }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(67, &font);

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{4}
        .setEditingStyleCount(data.cursorStyleBefore || data.selectionStyleBefore || data.cursorSelectionStyleAfter ? 2 : 0)
        .setDynamicStyleCount(1)
    };

    FontHandle fontHandle = shared.addFont(font, 1.0f);

    /* Has to be called early to be able to call TextLayer::create() which we
       want to ensure there's nothing accidentally skipped when recycling
       internally, TextLayerStyleAnimator::create() itself doesn't need
       setStyle() to be called */
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}
            .setColor(0xff3366_rgbf),
         TextLayerStyleUniform{}
            .setColor(0x9933ff_rgbf),
         TextLayerStyleUniform{}
            .setColor(0x337766_rgbf),
         TextLayerStyleUniform{}
            .setColor(0x112233_rgbf)},
        {fontHandle, fontHandle, fontHandle, fontHandle},
        {Text::Alignment::MiddleCenter,
         Text::Alignment::MiddleCenter,
         Text::Alignment::MiddleCenter,
         Text::Alignment::MiddleCenter},
        {}, {}, {},
        {data.cursorStyleBefore ? 0 : -1,
         data.cursorStyleBefore ? 1 : -1,
         data.cursorSelectionStyleAfter ? 0 : -1,
         data.cursorSelectionStyleAfter ? 1 : -1},
        {data.selectionStyleBefore ? 1 : -1,
         data.selectionStyleBefore ? 0 : -1,
         data.cursorSelectionStyleAfter ? 1 : -1,
         data.cursorSelectionStyleAfter ? 0 : -1},
        {Vector4{1.0f},
         Vector4{2.0f},
         Vector4{3.0f},
         Vector4{data.samePaddingAfter ? 3.0f : 4.0f}});
    if(data.cursorStyleBefore || data.selectionStyleBefore || data.cursorSelectionStyleAfter)
        shared.setEditingStyle(TextLayerCommonEditingStyleUniform{},
            {TextLayerEditingStyleUniform{}
                .setBackgroundColor(0x119900_rgbf),
             TextLayerEditingStyleUniform{}
                .setBackgroundColor(0xaabbcc_rgbf)},
            {},
            {Vector4{2.0f},
             Vector4{data.samePaddingAfter ? 2.0f : 3.0f}});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    TextLayerStyleAnimator animator{animatorHandle(0, 1)};
    layer.assignAnimator(animator);

    DataHandle layerData = layer.create(1, "", {});

    /* Allocate an animation */
    AnimationHandle first = animator.create(0, 1, Animation::Easing::linear, 0_nsec, 10_nsec, layerData);
    CORRADE_COMPARE(animator.styles(first), Containers::pair(0u, 1u));
    CORRADE_COMPARE(animator.dynamicStyle(first), Containers::NullOpt);
    CORRADE_COMPARE(animator.easing(first), Animation::Easing::linear);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 0);

    /* Let it advance to allocate the dynamic style and copy over style data.
       It should make use of all TextLayerStyleAnimatorUpdates. */
    Containers::BitArray activeStorage{NoInit, 1};
    Containers::BitArray startedStorage{NoInit, 1};
    Containers::BitArray stoppedStorage{NoInit, 1};
    Float factorStorage[1];
    Containers::BitArray removedStorage{NoInit, 1};
    TextLayerStyleUniform dynamicStyleUniforms[3];
    char dynamicStyleCursorStyles[1];
    char dynamicStyleSelectionStyles[1];
    Vector4 dynamicStylePaddings[1];
    TextLayerEditingStyleUniform dynamicEditingStyleUniforms[2];
    Vector4 dynamicEditingStylePaddings[2];
    UnsignedInt dataStyles[1];
    CORRADE_COMPARE(animator.advance(5_nsec,
            activeStorage,
            startedStorage,
            stoppedStorage,
            factorStorage,
            removedStorage,
            Containers::arrayView(dynamicStyleUniforms)
                .prefix(data.cursorStyleBefore || data.selectionStyleBefore || data.cursorSelectionStyleAfter ? 3 : 1),
            Containers::MutableBitArrayView{dynamicStyleCursorStyles, 0, 1},
            Containers::MutableBitArrayView{dynamicStyleSelectionStyles, 0, 1},
            dynamicStylePaddings,
            data.cursorStyleBefore || data.selectionStyleBefore || data.cursorSelectionStyleAfter ?
                Containers::arrayView(dynamicEditingStyleUniforms) : nullptr,
            data.cursorStyleBefore || data.selectionStyleBefore || data.cursorSelectionStyleAfter ?
                Containers::arrayView(dynamicEditingStylePaddings) : nullptr,
            dataStyles),
        TextLayerStyleAnimatorUpdate::Uniform|
        TextLayerStyleAnimatorUpdate::Padding|
        TextLayerStyleAnimatorUpdate::Style|
        (data.cursorStyleBefore || data.selectionStyleBefore ?
            TextLayerStyleAnimatorUpdate::EditingUniform|
            TextLayerStyleAnimatorUpdate::EditingPadding :
            TextLayerStyleAnimatorUpdates{}));
    CORRADE_COMPARE(animator.dynamicStyle(first), 0);
    /* Verify the AnimatorDataHandle overload correctly detecting a valid style
       also */
    CORRADE_COMPARE(animator.dynamicStyle(animationHandleData(first)), 0);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 1);

    /* Removal should free the dynamic style */
    animator.remove(first);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 0);

    /* Removal and new creation should reuse the same slot and overwrite
       everything including the dynamic style index. What's handled by
       AbstractAnimator is tested well enough in
       AbstractAnimatorTest::createRemoveHandleRecycle(). */
    AnimationHandle first2 = animator.create(3, 2, Animation::Easing::bounceInOut, -10_nsec, 30_nsec, data.attachLaterAfter ? DataHandle::Null : layerData);
    CORRADE_COMPARE(animationHandleId(first2), animationHandleId(first));
    CORRADE_COMPARE(animator.styles(first2), Containers::pair(3u, 2u));
    CORRADE_COMPARE(animator.dynamicStyle(first2), Containers::NullOpt);
    CORRADE_COMPARE(animator.easing(first2), Animation::Easing::bounceInOut);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 0);

    /* The recycled animation shouldn't inherit any info about uniform, padding
       or editing style animations. The padding is however checked against the
       current value, so update it to the expected new (constant) value
       first. */
    dynamicStylePaddings[0] = Vector4{3.0f};
    dynamicEditingStylePaddings[0] = Vector4{2.0f};
    dynamicEditingStylePaddings[1] = Vector4{2.0f};
    CORRADE_COMPARE(animator.advance(10_nsec,
            activeStorage,
            startedStorage,
            stoppedStorage,
            factorStorage,
            removedStorage,
            Containers::arrayView(dynamicStyleUniforms)
                .prefix(data.cursorStyleBefore || data.selectionStyleBefore || data.cursorSelectionStyleAfter ? 3 : 1),
            Containers::MutableBitArrayView{dynamicStyleCursorStyles, 0, 1},
            Containers::MutableBitArrayView{dynamicStyleSelectionStyles, 0, 1},
            dynamicStylePaddings,
            data.cursorStyleBefore || data.selectionStyleBefore || data.cursorSelectionStyleAfter ?
                Containers::arrayView(dynamicEditingStyleUniforms) : nullptr,
            data.cursorStyleBefore || data.selectionStyleBefore || data.cursorSelectionStyleAfter ?
                Containers::arrayView(dynamicEditingStylePaddings) : nullptr,
            dataStyles),
        TextLayerStyleAnimatorUpdate::Uniform|
        (data.samePaddingAfter ? TextLayerStyleAnimatorUpdates{} : TextLayerStyleAnimatorUpdate::Padding)|
        (data.cursorSelectionStyleAfter ? TextLayerStyleAnimatorUpdate::EditingUniform : TextLayerStyleAnimatorUpdates{})|
        (data.cursorSelectionStyleAfter && !data.samePaddingAfter ? TextLayerStyleAnimatorUpdate::EditingPadding : TextLayerStyleAnimatorUpdates{})|
        (data.attachLaterAfter ? TextLayerStyleAnimatorUpdate{} : TextLayerStyleAnimatorUpdate::Style));

    /* If the recycled animation wasn't attached initially, attaching it later
       should not inherit the original expected style and switch it to another
       but rather not switch at all */
    if(data.attachLaterAfter) {
        animator.attach(first2, layerData);
        /* The last remembered expected style is the dynamic one allocated
           previously. Set the data to it. */
        dataStyles[0] = 4;
        /* The animation will stop now. The animator should not update any
           styles as there was no attachment when it started and so it cannot
           know what's the expected style */
        CORRADE_COMPARE(animator.advance(30_nsec,
                activeStorage,
                startedStorage,
                stoppedStorage,
                factorStorage,
                removedStorage,
                Containers::arrayView(dynamicStyleUniforms).prefix(1),
                Containers::MutableBitArrayView{dynamicStyleCursorStyles, 0, 1},
                Containers::MutableBitArrayView{dynamicStyleSelectionStyles, 0, 1},
                dynamicStylePaddings,
                nullptr,
                nullptr,
                dataStyles),
            TextLayerStyleAnimatorUpdates{});
        CORRADE_COMPARE(dataStyles[0], 4);
    }
}

void TextLayerStyleAnimatorTest::createInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32}};

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{1, 5}
        .setEditingStyleCount(1)
        .setDynamicStyleCount(1)
    };

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    TextLayerStyleAnimator animatorNoLayerSet{animatorHandle(0, 1)};

    TextLayerStyleAnimator animator{animatorHandle(0, 1)};
    layer.assignAnimator(animator);

    Containers::String out;
    Error redirectError{&out};
    /* Verify all four create() overloads check the layer being set early
       enough */
    animatorNoLayerSet.create(0, 1, Animation::Easing::linear, 12_nsec, 13_nsec, DataHandle::Null, 1, AnimationFlags{});
    animatorNoLayerSet.create(0, 1, Animation::Easing::linear, 12_nsec, 13_nsec, DataHandle::Null, AnimationFlags{});
    animatorNoLayerSet.create(0, 1, Animation::Easing::linear, 12_nsec, 13_nsec, LayerDataHandle::Null, 1, AnimationFlags{});
    animatorNoLayerSet.create(0, 1, Animation::Easing::linear, 12_nsec, 13_nsec, LayerDataHandle::Null, AnimationFlags{});
    animator.create(0, 5, Animation::Easing::linear, 12_nsec, 13_nsec, DataHandle::Null);
    animator.create(5, 0, Animation::Easing::linear, 12_nsec, 13_nsec, DataHandle::Null);
    animator.create(0, 1, nullptr, 12_nsec, 13_nsec, DataHandle::Null);
    /* Other things like data handle layer part not matching etc. tested in
       AbstractAnimatorTest already */
    CORRADE_COMPARE_AS(out,
        "Ui::TextLayerStyleAnimator::create(): no layer set\n"
        "Ui::TextLayerStyleAnimator::create(): no layer set\n"
        "Ui::TextLayerStyleAnimator::create(): no layer set\n"
        "Ui::TextLayerStyleAnimator::create(): no layer set\n"
        "Ui::TextLayerStyleAnimator::create(): expected source and target style to be in range for 5 styles but got 0 and 5\n"
        "Ui::TextLayerStyleAnimator::create(): expected source and target style to be in range for 5 styles but got 5 and 0\n"
        "Ui::TextLayerStyleAnimator::create(): easing is null\n",
        TestSuite::Compare::String);
}

void TextLayerStyleAnimatorTest::propertiesInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32}};

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{2}
        .setDynamicStyleCount(1)
    };

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    TextLayerStyleAnimator animator{animatorHandle(0, 1)};
    layer.assignAnimator(animator);

    AnimationHandle handle = animator.create(0, 1, Animation::Easing::linear, 12_nsec, 13_nsec, DataHandle::Null);

    Containers::String out;
    Error redirectError{&out};
    animator.easing(AnimationHandle::Null);
    /* Valid animator, invalid data */
    animator.easing(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)));
    /* Invalid animator, valid data */
    animator.easing(animationHandle(AnimatorHandle::Null, animationHandleData(handle)));
    /* AnimatorDataHandle directly */
    animator.easing(AnimatorDataHandle(0x123abcde));
    CORRADE_COMPARE_AS(out,
        "Ui::TextLayerStyleAnimator::easing(): invalid handle Ui::AnimationHandle::Null\n"

        "Ui::TextLayerStyleAnimator::easing(): invalid handle Ui::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"

        "Ui::TextLayerStyleAnimator::easing(): invalid handle Ui::AnimationHandle(Null, {0x0, 0x1})\n"

        "Ui::TextLayerStyleAnimator::easing(): invalid handle Ui::AnimatorDataHandle(0xabcde, 0x123)\n",
        TestSuite::Compare::String);
}

void TextLayerStyleAnimatorTest::advance() {
    auto&& data = AdvanceData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return Containers::pointer<EmptyShaper>(*this); }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(67, &font);

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{4, 7}
        .setEditingStyleCount(
            data.cursorStyles || data.selectionStyles ? 4 : 0,
            data.cursorStyles || data.selectionStyles ? 5 : 0)
        .setDynamicStyleCount(4)
    };

    FontHandle fontHandle1 = shared.addFont(font, 1.0f);
    FontHandle fontHandle2 = shared.addFont(font, 2.0f);
    FontHandle fontHandle3 = shared.addFont(font, 3.0f);

    /* Has to be called early to be able to call TextLayer::create() which we
       need to to verify style ID updates, TextLayerStyleAnimator::create()
       itself doesn't need setStyle() to be called */
    shared.setStyle(
        TextLayerCommonStyleUniform{},
        /* Assuming the lerp works component-wise, just set them to mutually
           exclusive ranges to verify that correct values get interpolated */
        {TextLayerStyleUniform{}    /* 0, used by style 6. All zeros. */
            .setColor(Color4{0.0f}),
         TextLayerStyleUniform{}    /* 1, used by style 3 */
            .setColor(Color4{4.0f}),
         TextLayerStyleUniform{}    /* 2, used by style 1 */
            .setColor(Color4{2.0f}),
         TextLayerStyleUniform{}},  /* 3, not used for animation */
        {3, 2, 3, 1, 3, 3, 0},
        {fontHandle1,  /* 0, not used for animation */
         fontHandle2,  /* 1 */
         fontHandle1,  /* 2, not used for animation */
         fontHandle3,  /* 3 */
         fontHandle1,  /* 4, not used for animation */
         fontHandle1,  /* 5, not used for animation */
         fontHandle2}, /* 6 */
        {Text::Alignment::MiddleCenter, /* 0, not used for animation */
         Text::Alignment::TopLeft,      /* 1 */
         Text::Alignment::MiddleCenter, /* 2, not used for animation */
         Text::Alignment::LineRight,    /* 3 */
         Text::Alignment::MiddleCenter, /* 4, not used for animation */
         Text::Alignment::MiddleCenter, /* 5, not used for animation */
         Text::Alignment::BottomEnd},   /* 6 */
        {Text::Feature::HistoricalLigatures,
         Text::Feature::TabularFigures,
         Text::Feature::SlashedZero,
         {Text::Feature::StandardLigatures, false}},
        {0,     /* 0, not used for animation */
         3,     /* 1 */
         0,     /* 2, not used for animation */
         1,     /* 3 */
         0,     /* 4, not used for animation */
         0,     /* 5, not used for animation */
         0},    /* 6 */
        {0,     /* 0, not used for animation */
         1,     /* 1 */
         0,     /* 2, not used for animation */
         2,     /* 3 */
         0,     /* 4, not used for animation */
         0,     /* 5, not used for animation */
         1},    /* 6 */
        {-1,
         data.cursorStyles ? 2 : -1,
         -1,
         data.cursorStyles ? 1 : -1,
         -1,
         -1,
         data.cursorStyles ? 0 : -1},
        {-1,
         data.selectionStyles ? 3 : -1,
         -1,
         data.selectionStyles ? 2 : -1,
         -1,
         -1,
         data.selectionStyles ? 4 : -1},
        /* Paddings should not change between style 1 and 3 and should between
           style 3 and 6 */
        {{},                /* 0, not used for animation */
         Vector4{2.0f},     /* 1 */
         {},                /* 2, not used for animation */
         Vector4{2.0f},     /* 3 */
         {},                /* 4, not used for animation */
         {},                /* 5, not used for animation */
         Vector4{4.0f}}     /* 6 */
    );
    if(data.cursorStyles || data.selectionStyles) shared.setEditingStyle(
        TextLayerCommonEditingStyleUniform{},
        {TextLayerEditingStyleUniform{}         /* 0, used by style 1 and 4 */
            .setBackgroundColor(Color4{6.0f})
            .setCornerRadius(3.0f),
         TextLayerEditingStyleUniform{}         /* 1, used by style 2 */
            .setBackgroundColor(Color4{8.0f})
            .setCornerRadius(5.0f),
         TextLayerEditingStyleUniform{}         /* 2, used by style 3 */
            .setBackgroundColor(Color4{12.0f})
            .setCornerRadius(6.0f),
         TextLayerEditingStyleUniform{}         /* 3, used by style 0 */
            .setBackgroundColor(Color4{12.0f})
            .setCornerRadius(8.0f)},
        {3, 0, 1, 2, 0},
        {-1,            /* 0, used by a cursor style only */
         -1,            /* 1, used by a cursor style only */
         2,             /* 2, used by style 3 for selection */
         1,             /* 3, used by style 1 for selection */
         -1},           /* 4, used by style 6 for selection, resolves to 0 */
        /* Similarly here, paddings should not change between style 1, 2 and 3
           (referenced by style 1 and 3) and should between style 1, 0 and 2, 4
           (referenced by style 3 and 6) */
        {Vector4{16.0f},
         Vector4{32.0f},
         Vector4{32.0f},
         Vector4{32.0f},
         Vector4{24.0f}});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    /* Assign data to styles that aren't used for animation, and which have
       the font, alignment and features not used by any animation styles */
    DataHandle data0 = layer.create(4, "", {});
    DataHandle data1 = layer.create(0, "", {});
    DataHandle data2 = layer.create(2, "", {});
    DataHandle data3 = layer.create(4, "", {});
    DataHandle data4 = layer.create(5, "", {});
    DataHandle data5 = layer.create(0, "", {});
    DataHandle data6 = layer.create(2, "", {});

    TextLayerStyleAnimator animator{animatorHandle(0, 1)};
    layer.assignAnimator(animator);

    /* This one allocates a dynamic style, interpolates between uniforms 1 and
       2 with just Uniform set and when stopped sets the data2 style to 1 */
    AnimationHandle playing = animator.create(3, 1, Animation::Easing::linear, 0_nsec, 20_nsec, data2);
    AnimationHandle playingReverse = animator.create(1, 3, Animation::Easing::linear, 0_nsec, 20_nsec, data5, AnimationFlag::Reverse);
    /* The last iteration of this one will play, making it the same direction
       as the `playing` animation */
    AnimationHandle playingReverseEveryOther = animator.create(1, 3, Animation::Easing::linear, -60_nsec, 20_nsec, data6, 4, AnimationFlag::ReverseEveryOther);
    /* This one sets the data4 style to 3 and is removed without even
       allocating a dynamic style or marking Uniform or Padding as changed */
    AnimationHandle stopped = animator.create(1, 3, Animation::Easing::cubicOut, 0_nsec, 1_nsec, data4);
    /* This one is a reverse of the first, scheduled later and not attached to
       any data, thus it never marks Style as changed */
    AnimationHandle scheduledNullData = animator.create(1, 3, Animation::Easing::linear, 15_nsec, 10_nsec, DataHandle::Null);
    /* This one sets the data1 style to 3 and stays, without allocating a
       dynamic style at all, or marking Uniform or Padding as changed. Later on
       it's restarted and then it interpolates as usual. */
    AnimationHandle stoppedKept = animator.create(6, 3, Animation::Easing::linear, -20_nsec, 15_nsec, data1, AnimationFlag::KeepOncePlayed);
    /* This one sets both Uniform and Padding when animated. It's a linear
       easing but reverted. */
    AnimationHandle scheduledChangesPadding = animator.create(3, 6, [](Float a) { return 1.0f - a; }, 30_nsec, 20_nsec, data3);

    /* Initially there should be no styles changed and no dynamic styles
       used */
    CORRADE_COMPARE(layer.style(data0), 4);
    CORRADE_COMPARE(layer.style(data1), 0);
    CORRADE_COMPARE(layer.style(data2), 2);
    CORRADE_COMPARE(layer.style(data3), 4);
    CORRADE_COMPARE(layer.style(data4), 5);
    CORRADE_COMPARE(layer.style(data5), 0);
    CORRADE_COMPARE(layer.style(data6), 2);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 0);

    /* Does what layer's advanceAnimations() is doing internally for all
       animators (as we need to test also the interaction with animation being
       removed, etc.), but with an ability to peek into the filled data to
       verify they're written only when they should be. Layer's
       advanceAnimations() is then tested in layerAdvance() below. */
    const auto advance = [&](Nanoseconds time, Containers::ArrayView<TextLayerStyleUniform> dynamicStyleUniforms, Containers::MutableBitArrayView dynamicStyleCursorStyles, Containers::MutableBitArrayView dynamicStyleSelectionStyles, const Containers::StridedArrayView1D<Vector4>& dynamicStylePaddings, Containers::ArrayView<TextLayerEditingStyleUniform> dynamicEditingStyleUniforms, const Containers::StridedArrayView1D<Vector4>& dynamicEditingStylePaddings, const Containers::StridedArrayView1D<UnsignedInt>& dataStyles) {
        UnsignedByte activeStorage[1];
        UnsignedByte startedStorage[1];
        UnsignedByte stoppedStorage[1];
        Float factorStorage[7];
        UnsignedByte removeStorage[1];

        return animator.advance(time,
            Containers::MutableBitArrayView{activeStorage, 0, 7},
            Containers::MutableBitArrayView{startedStorage, 0, 7},
            Containers::MutableBitArrayView{stoppedStorage, 0, 7},
            factorStorage,
            Containers::MutableBitArrayView{removeStorage, 0, 7},
            dynamicStyleUniforms, dynamicStyleCursorStyles, dynamicStyleSelectionStyles, dynamicStylePaddings, dynamicEditingStyleUniforms, dynamicEditingStylePaddings, dataStyles);
    };

    /* The padding resulting from the animation gets checked against these
       values, so set them to something very different to make sure they get
       updated */
    Vector4 paddings[]{
        Vector4{Constants::nan()},
        Vector4{Constants::nan()},
        Vector4{Constants::nan()},
        Vector4{Constants::nan()}
    };
    Vector4 editingPaddings[]{
        Vector4{Constants::nan()},
        Vector4{Constants::nan()},
        Vector4{Constants::nan()},
        Vector4{Constants::nan()},
        Vector4{Constants::nan()},
        Vector4{Constants::nan()},
        Vector4{Constants::nan()},
        Vector4{Constants::nan()}
    };

    /* The dataStyles are compared against to not break animations and style
       changes that happened since the original animation started and so they
       need to be preserved across advances. Behavior with external style
       changes is tested in advanceExternalStyleChanges(). */
    UnsignedInt dataStyles[]{
        666,
        666,
        666,
        666,
        666,
        666,
        666
    };

    /* Advancing to 5 allocates dynamic styles for the playing animations,
       sets their font, alignment and features, switches the styles to them and
       fills the dynamic data. For the stopped & removed and stopped & kept
       animation it switches the style to the destination one. */
    {
        TextLayerStyleUniform uniforms[12];
        TextLayerEditingStyleUniform editingUniforms[8];
        /* Set to all 1s if non-editing, all 0s if editing. The advance()
           should then flip them to the other value only where expected. */
        Containers::BitArray cursorStyles{DirectInit, 4, data.cursorStyles ? false : true};
        Containers::BitArray selectionStyles{DirectInit, 4, data.selectionStyles ? false : true};
        CORRADE_COMPARE(advance(5_nsec,
                Containers::arrayView(uniforms).prefix(data.cursorStyles || data.selectionStyles ? 12 : 4),
                cursorStyles,
                selectionStyles,
                paddings,
                data.cursorStyles || data.selectionStyles ? Containers::arrayView(editingUniforms) : nullptr,
                data.cursorStyles || data.selectionStyles ? Containers::arrayView(editingPaddings) : nullptr,
                dataStyles),
            TextLayerStyleAnimatorUpdate::Uniform|TextLayerStyleAnimatorUpdate::Style|TextLayerStyleAnimatorUpdate::Padding|
            (data.cursorStyles || data.selectionStyles ? TextLayerStyleAnimatorUpdate::EditingUniform|TextLayerStyleAnimatorUpdate::EditingPadding : TextLayerStyleAnimatorUpdates{}));
        CORRADE_VERIFY(animator.isHandleValid(playing));
        CORRADE_VERIFY(!animator.isHandleValid(stopped));
        CORRADE_VERIFY(animator.isHandleValid(scheduledNullData));
        CORRADE_VERIFY(animator.isHandleValid(stoppedKept));
        CORRADE_VERIFY(animator.isHandleValid(scheduledChangesPadding));
        CORRADE_COMPARE(animator.state(playing), AnimationState::Playing);
        CORRADE_COMPARE(animator.state(scheduledNullData), AnimationState::Scheduled);
        CORRADE_COMPARE(animator.state(stoppedKept), AnimationState::Stopped);
        CORRADE_COMPARE(animator.state(scheduledChangesPadding), AnimationState::Scheduled);
        CORRADE_COMPARE(animator.dynamicStyle(playing), 0);
        CORRADE_COMPARE(animator.dynamicStyle(scheduledNullData), Containers::NullOpt);
        CORRADE_COMPARE(animator.dynamicStyle(stoppedKept), Containers::NullOpt);
        CORRADE_COMPARE(animator.dynamicStyle(scheduledChangesPadding), Containers::NullOpt);
        CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 3);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(0), playing);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(1), playingReverse);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(2), playingReverseEveryOther);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(3), AnimationHandle::Null);
        CORRADE_COMPARE_AS(layer.dynamicStyleFonts(), Containers::arrayView({
            fontHandle3,                    /* from style 3 */
            fontHandle3,                    /* from style 3 */
            /* The playingReverseEveryOther has ReverseEveryOther set but not
               Reverse and so the source and target style isn't swapped */
            fontHandle2,                    /* from style 1 */
            FontHandle::Null,
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(layer.dynamicStyleAlignments(), Containers::arrayView({
            Text::Alignment::LineRight,     /* from style 3 */
            Text::Alignment::LineRight,     /* from style 3 */
            /* The playingReverseEveryOther has ReverseEveryOther set but not
               Reverse and so the source and target style isn't swapped */
            Text::Alignment::TopLeft,       /* from style 1 */
            Text::Alignment::MiddleCenter,
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(layer.dynamicStyleFeatures(0))), (Containers::arrayView<const Containers::Pair<Text::Feature, UnsignedInt>>({
            {Text::Feature::TabularFigures, true},
            {Text::Feature::SlashedZero, true}
        })), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(layer.dynamicStyleFeatures(1))), (Containers::arrayView<const Containers::Pair<Text::Feature, UnsignedInt>>({
            {Text::Feature::TabularFigures, true},
            {Text::Feature::SlashedZero, true}
        })), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(layer.dynamicStyleFeatures(2))), (Containers::arrayView<const Containers::Pair<Text::Feature, UnsignedInt>>({
            {Text::Feature::StandardLigatures, false}
        })), TestSuite::Compare::Container);
        /* Style IDs in the layer aren't changed, the passed array is instead,
           and only where dynamic styles got allocated or the animation
           stopped */
        CORRADE_COMPARE(layer.style(data0), 4);
        CORRADE_COMPARE(layer.style(data1), 0);
        CORRADE_COMPARE(layer.style(data2), 2);
        CORRADE_COMPARE(layer.style(data3), 4);
        CORRADE_COMPARE(layer.style(data4), 5);
        CORRADE_COMPARE(layer.style(data5), 0);
        CORRADE_COMPARE(layer.style(data6), 2);
        /* Dynamic style 0, 1, 2 get the bits modified */
        CORRADE_COMPARE_AS(Containers::BitArrayView{cursorStyles}, Containers::stridedArrayView({
            data.cursorStyles ? true : false,
            data.cursorStyles ? true : false,
            data.cursorStyles ? true : false,
            data.cursorStyles ? false : true,
        }).sliceBit(0), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(Containers::BitArrayView{selectionStyles}, Containers::stridedArrayView({
            data.selectionStyles ? true : false,
            data.selectionStyles ? true : false,
            data.selectionStyles ? true : false,
            data.selectionStyles ? false : true,
        }).sliceBit(0), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
            666u,
            3u,
            shared.styleCount() + 0u,
            666u,
            3u,
            shared.styleCount() + 1u,
            shared.styleCount() + 2u,
        }), TestSuite::Compare::Container);
        /* The first dynamic style should get a 1/4 interpolation of uniforms 1
           and 2 and the constant padding value. The second dynamic style is
           the same uniforms swapped but played in reverse and the third is
           reversed in its second iteration, so all three should get the same
           output. */
        for(UnsignedInt i: {0, 1, 2}) {
            CORRADE_ITERATION(i);
            CORRADE_COMPARE(uniforms[i].color, Color4{3.5f});
            CORRADE_COMPARE(paddings[i], Vector4{2.0f});
            if(data.cursorStyles) {
                /* For the cursor styles 1 and 2 it's 1/4 of uniforms 0 and 1,
                   padding also constant */
                CORRADE_COMPARE(editingUniforms[i*2 + 1].backgroundColor, Color4{6.5f});
                CORRADE_COMPARE(editingUniforms[i*2 + 1].cornerRadius, 3.5f);
                CORRADE_COMPARE(editingPaddings[i*2 + 1], Vector4{32.0f});
            }
            if(data.selectionStyles) {
                /* For the selection styles 2 and 3 it's 1/4 of uniforms 1 and
                   2, padding again constant */
                CORRADE_COMPARE(editingUniforms[i*2 + 0].backgroundColor, Color4{9.0f});
                CORRADE_COMPARE(editingUniforms[i*2 + 0].cornerRadius, 5.25f);
                CORRADE_COMPARE(editingPaddings[i*2 + 0], Vector4{32.0f});
                /* 1/4 of text uniforms 2 and 1 */
                CORRADE_COMPARE(uniforms[shared.dynamicStyleCount() + 0*2 + 0].color, Color4{2.5f});
            }
        }
    }

    /* Reset the padding of the stopped & kept style to something else to
       verify it doesn't get touched anymore */
    paddings[3] = {};
    editingPaddings[3*2 + 0] = {};
    editingPaddings[3*2 + 1] = {};

    /* Advancing to 10 changes just the uniform to 1/2, nothing else */
    {
        TextLayerStyleUniform uniforms[12];
        Containers::BitArray cursorStyles{DirectInit, 4, data.cursorStyles ? false : true};
        Containers::BitArray selectionStyles{DirectInit, 4, data.selectionStyles ? false : true};
        TextLayerEditingStyleUniform editingUniforms[8];
        CORRADE_COMPARE(advance(10_nsec,
                Containers::arrayView(uniforms).prefix(data.cursorStyles || data.selectionStyles ? 12 : 4),
                cursorStyles,
                selectionStyles,
                paddings,
                data.cursorStyles || data.selectionStyles ? Containers::arrayView(editingUniforms) : nullptr,
                data.cursorStyles || data.selectionStyles ? Containers::arrayView(editingPaddings) : nullptr,
                dataStyles),
            TextLayerStyleAnimatorUpdate::Uniform|
            (data.cursorStyles || data.selectionStyles ? TextLayerStyleAnimatorUpdate::EditingUniform : TextLayerStyleAnimatorUpdates{}));
        CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 3);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(0), playing);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(1), playingReverse);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(2), playingReverseEveryOther);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(3), AnimationHandle::Null);
        /* Font, alignment and features isn't modified compared to last time as
           no new style got allocated */
        CORRADE_COMPARE_AS(layer.dynamicStyleFonts(), Containers::arrayView({
            fontHandle3,
            fontHandle3,
            fontHandle2,
            FontHandle::Null,
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(layer.dynamicStyleAlignments(), Containers::arrayView({
            Text::Alignment::LineRight,
            Text::Alignment::LineRight,
            Text::Alignment::TopLeft,
            Text::Alignment::MiddleCenter,
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(layer.dynamicStyleFeatures(0))), (Containers::arrayView<const Containers::Pair<Text::Feature, UnsignedInt>>({
            {Text::Feature::TabularFigures, true},
            {Text::Feature::SlashedZero, true}
        })), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(layer.dynamicStyleFeatures(1))), (Containers::arrayView<const Containers::Pair<Text::Feature, UnsignedInt>>({
            {Text::Feature::TabularFigures, true},
            {Text::Feature::SlashedZero, true}
        })), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(layer.dynamicStyleFeatures(2))), (Containers::arrayView<const Containers::Pair<Text::Feature, UnsignedInt>>({
            {Text::Feature::StandardLigatures, false}
        })), TestSuite::Compare::Container);
        /* No styles get the bits modified */
        CORRADE_COMPARE_AS(Containers::BitArrayView{cursorStyles}, Containers::stridedArrayView({
            data.cursorStyles ? false : true,
            data.cursorStyles ? false : true,
            data.cursorStyles ? false : true,
            data.cursorStyles ? false : true,
        }).sliceBit(0), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(Containers::BitArrayView{selectionStyles}, Containers::stridedArrayView({
            data.selectionStyles ? false : true,
            data.selectionStyles ? false : true,
            data.selectionStyles ? false : true,
            data.selectionStyles ? false : true,
        }).sliceBit(0), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
            666u,
            3u,
            shared.styleCount() + 0u,
            666u,
            3u,
            shared.styleCount() + 1u,
            shared.styleCount() + 2u,
        }), TestSuite::Compare::Container);
        /* Testing just a subset, assuming the rest is updated accordingly */
        for(UnsignedInt i: {0, 1, 2}) {
            CORRADE_ITERATION(i);
            CORRADE_COMPARE(uniforms[i].color, Color4{3.0f});
            CORRADE_COMPARE(paddings[i], Vector4{2.0f});
            if(data.cursorStyles) {
                /* For the cursor styles 1 and 2 it's 1/2 of uniforms 0 and 1,
                   padding also constant */
                CORRADE_COMPARE(editingUniforms[i*2 + 1].backgroundColor, Color4{7.0f});
                CORRADE_COMPARE(editingPaddings[i*2 + 1], Vector4{32.0f});
            }
            if(data.selectionStyles) {
                /* For the selection styles 2 and 3 it's 1/2 of uniforms 1 and
                   2, padding again constant */
                CORRADE_COMPARE(editingUniforms[i*2 + 0].backgroundColor, Color4{10.0f});
                CORRADE_COMPARE(editingPaddings[i*2 + 0], Vector4{32.0f});
                /* 1/2 of text uniforms 2 and 1 */
                CORRADE_COMPARE(uniforms[shared.dynamicStyleCount() + i*2 + 0].color, Color4{3.0f});
            }
        }
    }

    /* Advancing to 15 plays also the scheduled animation without a data
       attachment, allocating a new dynamic style but not switching to it.
       I.e., no Style is set, only Uniform and Padding. */
    {
        TextLayerStyleUniform uniforms[12];
        Containers::BitArray cursorStyles{DirectInit, 4, data.cursorStyles ? false : true};
        Containers::BitArray selectionStyles{DirectInit, 4, data.selectionStyles ? false : true};
        TextLayerEditingStyleUniform editingUniforms[8];
        CORRADE_COMPARE(advance(15_nsec,
                Containers::arrayView(uniforms).prefix(data.cursorStyles || data.selectionStyles ? 12 : 4),
                cursorStyles,
                selectionStyles,
                paddings,
                data.cursorStyles || data.selectionStyles ? Containers::arrayView(editingUniforms) : nullptr,
                data.cursorStyles || data.selectionStyles ? Containers::arrayView(editingPaddings) : nullptr,
                dataStyles),
            TextLayerStyleAnimatorUpdate::Uniform|TextLayerStyleAnimatorUpdate::Padding|
            (data.cursorStyles || data.selectionStyles ? TextLayerStyleAnimatorUpdate::EditingUniform|TextLayerStyleAnimatorUpdate::EditingPadding : TextLayerStyleAnimatorUpdates{}));
        CORRADE_COMPARE(animator.state(scheduledNullData), AnimationState::Playing);
        CORRADE_COMPARE(animator.dynamicStyle(scheduledNullData), 3);
        CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 4);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(0), playing);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(1), playingReverse);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(2), playingReverseEveryOther);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(3), scheduledNullData);
        CORRADE_COMPARE_AS(layer.dynamicStyleFonts(), Containers::arrayView({
            fontHandle3,
            fontHandle3,
            fontHandle2,
            fontHandle2,                    /* from style 1 */
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(layer.dynamicStyleAlignments(), Containers::arrayView({
            Text::Alignment::LineRight,
            Text::Alignment::LineRight,
            Text::Alignment::TopLeft,
            Text::Alignment::TopLeft,       /* from style 1 */
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(layer.dynamicStyleFeatures(0))), (Containers::arrayView<const Containers::Pair<Text::Feature, UnsignedInt>>({
            {Text::Feature::TabularFigures, true},
            {Text::Feature::SlashedZero, true}
        })), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(layer.dynamicStyleFeatures(1))), (Containers::arrayView<const Containers::Pair<Text::Feature, UnsignedInt>>({
            {Text::Feature::TabularFigures, true},
            {Text::Feature::SlashedZero, true}
        })), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(layer.dynamicStyleFeatures(2))), (Containers::arrayView<const Containers::Pair<Text::Feature, UnsignedInt>>({
            {Text::Feature::StandardLigatures, false}
        })), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(layer.dynamicStyleFeatures(3))), (Containers::arrayView<const Containers::Pair<Text::Feature, UnsignedInt>>({
            {Text::Feature::StandardLigatures, false}
        })), TestSuite::Compare::Container);
        /* Style 3 gets the bits modified */
        CORRADE_COMPARE_AS(Containers::BitArrayView{cursorStyles}, Containers::stridedArrayView({
            data.cursorStyles ? false : true,
            data.cursorStyles ? false : true,
            data.cursorStyles ? false : true,
            data.cursorStyles ? true : false,
        }).sliceBit(0), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(Containers::BitArrayView{selectionStyles}, Containers::stridedArrayView({
            data.selectionStyles ? false : true,
            data.selectionStyles ? false : true,
            data.selectionStyles ? false : true,
            data.selectionStyles ? true : false,
        }).sliceBit(0), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
            666u,
            3u,
            shared.styleCount() + 0u,
            666u,
            3u,
            shared.styleCount() + 1u,
            shared.styleCount() + 2u,
        }), TestSuite::Compare::Container);
        /* The playing animations are advanced to 3/4 */
        for(UnsignedInt i: {0, 1, 2}) {
            CORRADE_ITERATION(i);
            CORRADE_COMPARE(uniforms[i].color, Color4{2.5f});
            CORRADE_COMPARE(paddings[i], Vector4{2.0f});
        }
        /* The null data animation is set to the value of style 1 */
        CORRADE_COMPARE(uniforms[3].color, Color4{2.0f});
        CORRADE_COMPARE(paddings[3], Vector4{2.0f});
        if(data.cursorStyles) {
            /* For the cursor styles 1 and 2 it's 3/4 of uniforms 0 and 1,
               padding also constant */
            for(UnsignedInt i: {0, 1, 2}) {
                CORRADE_ITERATION(i);
                CORRADE_COMPARE(editingUniforms[i*2 + 1].backgroundColor, Color4{7.5f});
                CORRADE_COMPARE(editingPaddings[i*2 + 1], Vector4{32.0f});
            }
            /* The null data animation is set to the value of editing style 2 */
            CORRADE_COMPARE(editingUniforms[3*2 + 1].backgroundColor, Color4{8.0f});
            CORRADE_COMPARE(editingPaddings[3*2 + 1], Vector4{32.0f});
        }
        if(data.selectionStyles) {
            /* For the selection styles 2 and 3 it's 3/4 of uniforms 1 and 2,
               padding again constant */
            for(UnsignedInt i: {0, 1, 2}) {
                CORRADE_ITERATION(i);
                CORRADE_COMPARE(editingUniforms[i*2 + 0].backgroundColor, Color4{11.0f});
                CORRADE_COMPARE(editingPaddings[i*2 + 0], Vector4{32.0f});
                /* 3/4 of text uniforms 2 and 1 */
                CORRADE_COMPARE(uniforms[shared.dynamicStyleCount() + i*2 + 0].color, Color4{3.5f});
            }
            /* The null data animation is set to the value of editing style 3 */
            CORRADE_COMPARE(editingUniforms[3*2 + 0].backgroundColor, Color4{12.0f});
            CORRADE_COMPARE(editingPaddings[3*2 + 0], Vector4{32.0f});
            /* Exactly text uniform 1 */
            CORRADE_COMPARE(uniforms[shared.dynamicStyleCount() + 3*2 + 0].color, Color4{4.0f});
        }
    }

    /* Advancing to 20 stops the first two animations, recycling their dynamic
       style and changing the style to the target one (and source one for the
       Reverse animation). Uniform value is updated for the null data
       animation. */
    {
        TextLayerStyleUniform uniforms[12];
        Containers::BitArray cursorStyles{DirectInit, 4, data.cursorStyles ? false : true};
        Containers::BitArray selectionStyles{DirectInit, 4, data.selectionStyles ? false : true};
        TextLayerEditingStyleUniform editingUniforms[8];
        CORRADE_COMPARE(advance(20_nsec,
                Containers::arrayView(uniforms).prefix(data.cursorStyles || data.selectionStyles ? 12 : 4),
                cursorStyles,
                selectionStyles,
                paddings,
                data.cursorStyles || data.selectionStyles ? Containers::arrayView(editingUniforms) : nullptr,
                data.cursorStyles || data.selectionStyles ? Containers::arrayView(editingPaddings) : nullptr,
                dataStyles),
            TextLayerStyleAnimatorUpdate::Style|TextLayerStyleAnimatorUpdate::Uniform|
            (data.cursorStyles || data.selectionStyles ? TextLayerStyleAnimatorUpdate::EditingUniform : TextLayerStyleAnimatorUpdates{}));
        CORRADE_VERIFY(!animator.isHandleValid(playing));
        CORRADE_VERIFY(animator.isHandleValid(stoppedKept));
        CORRADE_VERIFY(animator.isHandleValid(scheduledChangesPadding));
        CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 1);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(0), AnimationHandle::Null);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(1), AnimationHandle::Null);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(2), AnimationHandle::Null);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(3), scheduledNullData);
        /* Font, alignment and features aren't modified compared to last time
           as no new style got allocated. In particular, the now-recycled
           dynamic style *isn't* changed to font, alignment and features of the
           target style, as the dynamic style is now unused. */
        CORRADE_COMPARE_AS(layer.dynamicStyleFonts(), Containers::arrayView({
            fontHandle3,
            fontHandle3,
            fontHandle2,
            fontHandle2,
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(layer.dynamicStyleAlignments(), Containers::arrayView({
            Text::Alignment::LineRight,
            Text::Alignment::LineRight,
            Text::Alignment::TopLeft,
            Text::Alignment::TopLeft,
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(layer.dynamicStyleFeatures(0))), (Containers::arrayView<const Containers::Pair<Text::Feature, UnsignedInt>>({
            {Text::Feature::TabularFigures, true},
            {Text::Feature::SlashedZero, true}
        })), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(layer.dynamicStyleFeatures(1))), (Containers::arrayView<const Containers::Pair<Text::Feature, UnsignedInt>>({
            {Text::Feature::TabularFigures, true},
            {Text::Feature::SlashedZero, true}
        })), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(layer.dynamicStyleFeatures(2))), (Containers::arrayView<const Containers::Pair<Text::Feature, UnsignedInt>>({
            {Text::Feature::StandardLigatures, false}
        })), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(layer.dynamicStyleFeatures(3))), (Containers::arrayView<const Containers::Pair<Text::Feature, UnsignedInt>>({
            {Text::Feature::StandardLigatures, false}
        })), TestSuite::Compare::Container);
        /* No styles get the bits modified */
        CORRADE_COMPARE_AS(Containers::BitArrayView{cursorStyles}, Containers::stridedArrayView({
            data.cursorStyles ? false : true,
            data.cursorStyles ? false : true,
            data.cursorStyles ? false : true,
            data.cursorStyles ? false : true,
        }).sliceBit(0), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(Containers::BitArrayView{selectionStyles}, Containers::stridedArrayView({
            data.selectionStyles ? false : true,
            data.selectionStyles ? false : true,
            data.selectionStyles ? false : true,
            data.selectionStyles ? false : true,
        }).sliceBit(0), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
            666u,
            3u,
            1u,
            666u,
            3u,
            1u,
            /* The playingReverseEveryOther has ReverseEveryOther set but not
               Reverse and so the source and target style isn't swapped */
            3u,
        }), TestSuite::Compare::Container);
        /* Uniform values of the recycled style aren't touched anymore */
        for(UnsignedInt i: {0, 1, 2}) {
            CORRADE_ITERATION(i);
            CORRADE_COMPARE(uniforms[i].color, Color4{1.0f});
        }
        /* The null data animation is advanced to 1/2 between style 1 and 3 */
        CORRADE_COMPARE(uniforms[3].color, Color4{3.0f});
        CORRADE_COMPARE(paddings[3], Vector4{2.0f});
        if(data.cursorStyles) {
            /* Uniform values of the recycled style aren't touched anymore */
            for(UnsignedInt i: {0, 1, 2}) {
                CORRADE_ITERATION(i);
                CORRADE_COMPARE(editingUniforms[i*2 + 1].backgroundColor, Color4{1.0f});
            }
            /* The null data animation is advanced to 1/2 between editing style
               2 and 1 */
            CORRADE_COMPARE(editingUniforms[3*2 + 1].backgroundColor, Color4{7.0f});
            CORRADE_COMPARE(editingPaddings[3*2 + 1], Vector4{32.0f});
        }
        if(data.selectionStyles) {
            /* Uniform values of the recycled style aren't touched anymore */
            for(UnsignedInt i: {0, 1, 2}) {
                CORRADE_ITERATION(i);
                CORRADE_COMPARE(editingUniforms[0*2 + 0].backgroundColor, Color4{1.0f});
            }
            /* The null data animation is advanced to 1/2 between editing style
               3 and 2 */
            CORRADE_COMPARE(editingUniforms[3*2 + 0].backgroundColor, Color4{10.0f});
            CORRADE_COMPARE(editingPaddings[3*2 + 0], Vector4{32.0f});
            /* And 1/2 of text uniform 1 and 2 */
            CORRADE_COMPARE(uniforms[shared.dynamicStyleCount() + 3*2 + 0].color, Color4{3.0f});
        }
    }

    /* Set dynamic style contents from outside to verify the font, alignment
       and features are not being set from each advance() */
    layer.setDynamicStyle(0,
        TextLayerStyleUniform{},
        FontHandle::Null,
        Text::Alignment::MiddleCenterIntegral,
        {}, {});
    layer.setDynamicStyle(1,
        TextLayerStyleUniform{},
        FontHandle::Null,
        Text::Alignment::MiddleCenterIntegral,
        {}, {});
    layer.setDynamicStyle(2,
        TextLayerStyleUniform{},
        FontHandle::Null,
        Text::Alignment::MiddleCenterIntegral,
        {}, {});

    /* Advancing to 25 stops the null data animation, recycling its dynamic
       style. Leads to no other change, i.e. no Style set. */
    {
        TextLayerStyleUniform uniforms[12];
        Containers::BitArray cursorStyles{DirectInit, 4, data.cursorStyles ? false : true};
        Containers::BitArray selectionStyles{DirectInit, 4, data.selectionStyles ? false : true};
        TextLayerEditingStyleUniform editingUniforms[8];
        CORRADE_COMPARE(advance(25_nsec,
                Containers::arrayView(uniforms).prefix(data.cursorStyles || data.selectionStyles ? 12 : 4),
                cursorStyles,
                selectionStyles,
                paddings,
                data.cursorStyles || data.selectionStyles ? Containers::arrayView(editingUniforms) : nullptr,
                data.cursorStyles || data.selectionStyles ? Containers::arrayView(editingPaddings) : nullptr,
                dataStyles),
            TextLayerStyleAnimatorUpdates{});
        CORRADE_VERIFY(!animator.isHandleValid(scheduledNullData));
        CORRADE_VERIFY(animator.isHandleValid(stoppedKept));
        CORRADE_VERIFY(animator.isHandleValid(scheduledChangesPadding));
        CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 0);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(0), AnimationHandle::Null);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(1), AnimationHandle::Null);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(2), AnimationHandle::Null);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(3), AnimationHandle::Null);
        /* Again font, alignment and features aren't modified, thus the reset
           values from above are staying */
        CORRADE_COMPARE_AS(layer.dynamicStyleFonts(), Containers::arrayView({
            FontHandle::Null,
            FontHandle::Null,
            FontHandle::Null,
            fontHandle2,
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(layer.dynamicStyleAlignments(), Containers::arrayView({
            Text::Alignment::MiddleCenterIntegral,
            Text::Alignment::MiddleCenterIntegral,
            Text::Alignment::MiddleCenterIntegral,
            Text::Alignment::TopLeft,
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(layer.dynamicStyleFeatures(0))), (Containers::arrayView<const Containers::Pair<Text::Feature, UnsignedInt>>({
        })), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(layer.dynamicStyleFeatures(1))), (Containers::arrayView<const Containers::Pair<Text::Feature, UnsignedInt>>({
        })), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(layer.dynamicStyleFeatures(2))), (Containers::arrayView<const Containers::Pair<Text::Feature, UnsignedInt>>({
        })), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(layer.dynamicStyleFeatures(3))), (Containers::arrayView<const Containers::Pair<Text::Feature, UnsignedInt>>({
            {Text::Feature::StandardLigatures, false}
        })), TestSuite::Compare::Container);
        /* No styles get the bits modified */
        CORRADE_COMPARE_AS(Containers::BitArrayView{cursorStyles}, Containers::stridedArrayView({
            data.cursorStyles ? false : true,
            data.cursorStyles ? false : true,
            data.cursorStyles ? false : true,
            data.cursorStyles ? false : true,
        }).sliceBit(0), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(Containers::BitArrayView{selectionStyles}, Containers::stridedArrayView({
            data.selectionStyles ? false : true,
            data.selectionStyles ? false : true,
            data.selectionStyles ? false : true,
            data.selectionStyles ? false : true,
        }).sliceBit(0), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
            666u,
            3u,
            1u,
            666u,
            3u,
            1u,
            3u
        }), TestSuite::Compare::Container);
        /* Uniform values of the recycled styles aren't touched anymore */
        for(UnsignedInt i: {0, 1, 2}) {
            CORRADE_ITERATION(i);
            CORRADE_COMPARE(uniforms[i].color, Color4{1.0f});
        }
        CORRADE_COMPARE(uniforms[2].color, Color4{1.0f});
        if(data.cursorStyles) {
            /* Uniform values of the recycled style aren't touched anymore */
            for(UnsignedInt i: {0, 1, 2}) {
                CORRADE_ITERATION(i);
                CORRADE_COMPARE(editingUniforms[i*2 + 1].backgroundColor, Color4{1.0f});
            }
            CORRADE_COMPARE(editingUniforms[3*2 + 1].backgroundColor, Color4{1.0f});
        }
        if(data.selectionStyles) {
            /* Uniform values of the recycled style aren't touched anymore */
            for(UnsignedInt i: {0, 1, 2}) {
                CORRADE_ITERATION(i);
                CORRADE_COMPARE(editingUniforms[i*2 + 0].backgroundColor, Color4{1.0f});
            }
            CORRADE_COMPARE(editingUniforms[3*2 + 0].backgroundColor, Color4{1.0f});
            CORRADE_COMPARE(uniforms[shared.dynamicStyleCount() + 3*2 + 0].color, Color4{1.0f});
        }
    }

    /* Advancing to 35 plays the scheduled animation, allocating a new dynamic
       style and switching to it */
    {
        TextLayerStyleUniform uniforms[12];
        Containers::BitArray cursorStyles{DirectInit, 4, data.cursorStyles ? false : true};
        Containers::BitArray selectionStyles{DirectInit, 4, data.selectionStyles ? false : true};
        TextLayerEditingStyleUniform editingUniforms[8];
        CORRADE_COMPARE(advance(35_nsec,
                Containers::arrayView(uniforms).prefix(data.cursorStyles || data.selectionStyles ? 12 : 4),
                cursorStyles,
                selectionStyles,
                paddings,
                data.cursorStyles || data.selectionStyles ? Containers::arrayView(editingUniforms) : nullptr,
                data.cursorStyles || data.selectionStyles ? Containers::arrayView(editingPaddings) : nullptr,
                dataStyles),
            TextLayerStyleAnimatorUpdate::Uniform|TextLayerStyleAnimatorUpdate::Style|TextLayerStyleAnimatorUpdate::Padding|
            (data.cursorStyles || data.selectionStyles ? TextLayerStyleAnimatorUpdate::EditingUniform|TextLayerStyleAnimatorUpdate::EditingPadding : TextLayerStyleAnimatorUpdates{}));
        CORRADE_VERIFY(animator.isHandleValid(stoppedKept));
        CORRADE_VERIFY(animator.isHandleValid(scheduledChangesPadding));
        CORRADE_COMPARE(animator.state(stoppedKept), AnimationState::Stopped);
        CORRADE_COMPARE(animator.state(scheduledChangesPadding), AnimationState::Playing);
        CORRADE_COMPARE(animator.dynamicStyle(scheduledChangesPadding), 0);
        CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 1);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(0), scheduledChangesPadding);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(1), AnimationHandle::Null);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(2), AnimationHandle::Null);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(3), AnimationHandle::Null);
        /* The newly allocated style is coincidentally again style 3 and again
           in slot 0, so this looks the same as before the setDynamicStyle()
           got called above */
        CORRADE_COMPARE_AS(layer.dynamicStyleFonts(), Containers::arrayView({
            fontHandle3,                    /* from style 3 again */
            FontHandle::Null,
            FontHandle::Null,
            fontHandle2,
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(layer.dynamicStyleAlignments(), Containers::arrayView({
            Text::Alignment::LineRight,     /* from style 3 again */
            Text::Alignment::MiddleCenterIntegral,
            Text::Alignment::MiddleCenterIntegral,
            Text::Alignment::TopLeft,
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(layer.dynamicStyleFeatures(0))), (Containers::arrayView<const Containers::Pair<Text::Feature, UnsignedInt>>({
            {Text::Feature::TabularFigures, true},
            {Text::Feature::SlashedZero, true}
        })), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(layer.dynamicStyleFeatures(1))), (Containers::arrayView<const Containers::Pair<Text::Feature, UnsignedInt>>({
        })), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(layer.dynamicStyleFeatures(2))), (Containers::arrayView<const Containers::Pair<Text::Feature, UnsignedInt>>({
        })), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(layer.dynamicStyleFeatures(3))), (Containers::arrayView<const Containers::Pair<Text::Feature, UnsignedInt>>({
            {Text::Feature::StandardLigatures, false}
        })), TestSuite::Compare::Container);
        /* Style 0 gets the bits modified */
        CORRADE_COMPARE_AS(Containers::BitArrayView{cursorStyles}, Containers::stridedArrayView({
            data.cursorStyles ? true : false,
            data.cursorStyles ? false : true,
            data.cursorStyles ? false : true,
            data.cursorStyles ? false : true,
        }).sliceBit(0), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(Containers::BitArrayView{selectionStyles}, Containers::stridedArrayView({
            data.selectionStyles ? true : false,
            data.selectionStyles ? false : true,
            data.selectionStyles ? false : true,
            data.selectionStyles ? false : true,
        }).sliceBit(0), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
            666u,
            3u,
            1u,
            shared.styleCount() + 0u,
            3u,
            1u,
            3u
        }), TestSuite::Compare::Container);
        /* The first dynamic style should get a 3/4 interpolation (i.e.,
           reverted from 1/4) of uniforms 1 and 0 and padding 3 and 6 */
        CORRADE_COMPARE(uniforms[0].color, Color4{1.0f});
        CORRADE_COMPARE(paddings[0], Vector4{3.5f});
        if(data.cursorStyles) {
            /* 3/4 interpolation of uniforms 0 and 3 and padding 1 and 0 */
            CORRADE_COMPARE(editingUniforms[0*2 + 1].backgroundColor, Color4{10.5f});
            CORRADE_COMPARE(editingPaddings[0*2 + 1], Vector4{20.0f});
        }
        if(data.selectionStyles) {
            /* 3/4 interpolation of uniforms 1 and 0 and padding 2 and 4 */
            CORRADE_COMPARE(editingUniforms[0*2 + 0].backgroundColor, Color4{6.5f});
            CORRADE_COMPARE(editingPaddings[0*2 + 0], Vector4{26.0f});
            /* 3/4 of text uniforms 2 and -1 resolved to 0 */
            CORRADE_COMPARE(uniforms[shared.dynamicStyleCount() + 0*2 + 0].color, Color4{0.5f});
        }
    }

    /* Advancing to 45 advances the scheduled animation, changing both the
       uniform and the padding. No styles. */
    {
        TextLayerStyleUniform uniforms[12];
        Containers::BitArray cursorStyles{DirectInit, 4, data.cursorStyles ? false : true};
        Containers::BitArray selectionStyles{DirectInit, 4, data.selectionStyles ? false : true};
        TextLayerEditingStyleUniform editingUniforms[8];
        CORRADE_COMPARE(advance(45_nsec,
                Containers::arrayView(uniforms).prefix(data.cursorStyles || data.selectionStyles ? 12 : 4),
                cursorStyles,
                selectionStyles,
                paddings,
                data.cursorStyles || data.selectionStyles ? Containers::arrayView(editingUniforms) : nullptr,
                data.cursorStyles || data.selectionStyles ? Containers::arrayView(editingPaddings) : nullptr,
                dataStyles),
            TextLayerStyleAnimatorUpdate::Uniform|TextLayerStyleAnimatorUpdate::Padding|
            (data.cursorStyles || data.selectionStyles ? TextLayerStyleAnimatorUpdate::EditingUniform|TextLayerStyleAnimatorUpdate::EditingPadding : TextLayerStyleAnimatorUpdates{}));
        CORRADE_VERIFY(animator.isHandleValid(stoppedKept));
        CORRADE_VERIFY(animator.isHandleValid(scheduledChangesPadding));
        CORRADE_COMPARE(animator.state(stoppedKept), AnimationState::Stopped);
        CORRADE_COMPARE(animator.state(scheduledChangesPadding), AnimationState::Playing);
        CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 1);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(0), scheduledChangesPadding);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(1), AnimationHandle::Null);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(2), AnimationHandle::Null);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(3), AnimationHandle::Null);
        /* No change to any of these again */
        CORRADE_COMPARE_AS(layer.dynamicStyleFonts(), Containers::arrayView({
            fontHandle3,
            FontHandle::Null,
            FontHandle::Null,
            fontHandle2,
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(layer.dynamicStyleAlignments(), Containers::arrayView({
            Text::Alignment::LineRight,
            Text::Alignment::MiddleCenterIntegral,
            Text::Alignment::MiddleCenterIntegral,
            Text::Alignment::TopLeft,
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(layer.dynamicStyleFeatures(0))), (Containers::arrayView<const Containers::Pair<Text::Feature, UnsignedInt>>({
            {Text::Feature::TabularFigures, true},
            {Text::Feature::SlashedZero, true}
        })), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(layer.dynamicStyleFeatures(1))), (Containers::arrayView<const Containers::Pair<Text::Feature, UnsignedInt>>({
        })), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(layer.dynamicStyleFeatures(2))), (Containers::arrayView<const Containers::Pair<Text::Feature, UnsignedInt>>({
        })), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(layer.dynamicStyleFeatures(3))), (Containers::arrayView<const Containers::Pair<Text::Feature, UnsignedInt>>({
            {Text::Feature::StandardLigatures, false}
        })), TestSuite::Compare::Container);
        /* No styles get the bits modified */
        CORRADE_COMPARE_AS(Containers::BitArrayView{cursorStyles}, Containers::stridedArrayView({
            data.cursorStyles ? false : true,
            data.cursorStyles ? false : true,
            data.cursorStyles ? false : true,
            data.cursorStyles ? false : true,
        }).sliceBit(0), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(Containers::BitArrayView{selectionStyles}, Containers::stridedArrayView({
            data.selectionStyles ? false : true,
            data.selectionStyles ? false : true,
            data.selectionStyles ? false : true,
            data.selectionStyles ? false : true,
        }).sliceBit(0), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
            666u,
            3u,
            1u,
            shared.styleCount() + 0u,
            3u,
            1u,
            3u
        }), TestSuite::Compare::Container);
        /* The first dynamic style should get a 1/4 interpolation (i.e.,
           reverted from 3/4) of uniforms 1 and 0 and padding 3 and 6 */
        CORRADE_COMPARE(uniforms[0].color, Color4{3.0f});
        CORRADE_COMPARE(paddings[0], Vector4{2.5f});
        if(data.cursorStyles) {
            /* 1/4 interpolation of uniforms 0 and 3 and padding 1 and 0 */
            CORRADE_COMPARE(editingUniforms[0*2 + 1].backgroundColor, Color4{7.5f});
            CORRADE_COMPARE(editingPaddings[0*2 + 1], Vector4{28.0f});
        }
        if(data.selectionStyles) {
            /* 1/4 interpolation of uniforms 1 and 0 and padding 2 and 4 */
            CORRADE_COMPARE(editingUniforms[0*2 + 0].backgroundColor, Color4{7.5f});
            CORRADE_COMPARE(editingPaddings[0*2 + 0], Vector4{30.0f});
            /* 1/4 of text uniforms 2 and -1 resolved to 0 */
            CORRADE_COMPARE(uniforms[shared.dynamicStyleCount() + 0*2 + 0].color, Color4{1.5f});
        }
    }

    /* Stopping the remaining animation (even before it finishes at 50) makes
       it recycle the remaining dynamic style and switch to the target style at
       the next advance(). Not updating any uniforms or paddings. */
    {
        TextLayerStyleUniform uniforms[12];
        Containers::BitArray cursorStyles{DirectInit, 4, data.cursorStyles ? false : true};
        Containers::BitArray selectionStyles{DirectInit, 4, data.selectionStyles ? false : true};
        TextLayerEditingStyleUniform editingUniforms[8];
        animator.stop(scheduledChangesPadding, 46_nsec);
        CORRADE_COMPARE(advance(47_nsec,
                Containers::arrayView(uniforms).prefix(data.cursorStyles || data.selectionStyles ? 12 : 4),
                cursorStyles,
                selectionStyles,
                paddings,
                data.cursorStyles || data.selectionStyles ? Containers::arrayView(editingUniforms) : nullptr,
                data.cursorStyles || data.selectionStyles ? Containers::arrayView(editingPaddings) : nullptr,
                dataStyles),
            TextLayerStyleAnimatorUpdate::Style);
        CORRADE_VERIFY(!animator.isHandleValid(scheduledChangesPadding));
        CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 0);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(0), AnimationHandle::Null);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(1), AnimationHandle::Null);
        /* No change to any of these again -- none of them are used anymore,
           and they stay at whatever they were before */
        CORRADE_COMPARE_AS(layer.dynamicStyleFonts(), Containers::arrayView({
            fontHandle3,
            FontHandle::Null,
            FontHandle::Null,
            fontHandle2,
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(layer.dynamicStyleAlignments(), Containers::arrayView({
            Text::Alignment::LineRight,
            Text::Alignment::MiddleCenterIntegral,
            Text::Alignment::MiddleCenterIntegral,
            Text::Alignment::TopLeft,
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(layer.dynamicStyleFeatures(0))), (Containers::arrayView<const Containers::Pair<Text::Feature, UnsignedInt>>({
            {Text::Feature::TabularFigures, true},
            {Text::Feature::SlashedZero, true}
        })), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(layer.dynamicStyleFeatures(1))), (Containers::arrayView<const Containers::Pair<Text::Feature, UnsignedInt>>({
        })), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(layer.dynamicStyleFeatures(2))), (Containers::arrayView<const Containers::Pair<Text::Feature, UnsignedInt>>({
        })), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(layer.dynamicStyleFeatures(3))), (Containers::arrayView<const Containers::Pair<Text::Feature, UnsignedInt>>({
            {Text::Feature::StandardLigatures, false}
        })), TestSuite::Compare::Container);
        /* No styles get the bits modified */
        CORRADE_COMPARE_AS(Containers::BitArrayView{cursorStyles}, Containers::stridedArrayView({
            data.cursorStyles ? false : true,
            data.cursorStyles ? false : true,
            data.cursorStyles ? false : true,
            data.cursorStyles ? false : true,
        }).sliceBit(0), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(Containers::BitArrayView{selectionStyles}, Containers::stridedArrayView({
            data.selectionStyles ? false : true,
            data.selectionStyles ? false : true,
            data.selectionStyles ? false : true,
            data.selectionStyles ? false : true,
        }).sliceBit(0), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
            666u,
            3u,
            1u,
            6u,
            3u,
            1u,
            3u
        }), TestSuite::Compare::Container);
    }

    /* Restarting the stopped animation makes it allocate a new dynamic
       style */
    {
        TextLayerStyleUniform uniforms[12];
        Containers::BitArray cursorStyles{DirectInit, 4, data.cursorStyles ? false : true};
        Containers::BitArray selectionStyles{DirectInit, 4, data.selectionStyles ? false : true};
        TextLayerEditingStyleUniform editingUniforms[8];
        animator.play(stoppedKept, 45_nsec);
        CORRADE_COMPARE(advance(50_nsec,
                Containers::arrayView(uniforms).prefix(data.cursorStyles || data.selectionStyles ? 12 : 4),
                cursorStyles,
                selectionStyles,
                paddings,
                data.cursorStyles || data.selectionStyles ? Containers::arrayView(editingUniforms) : nullptr,
                data.cursorStyles || data.selectionStyles ? Containers::arrayView(editingPaddings) : nullptr,
                dataStyles),
            TextLayerStyleAnimatorUpdate::Uniform|TextLayerStyleAnimatorUpdate::Padding|TextLayerStyleAnimatorUpdate::Style|
            (data.cursorStyles || data.selectionStyles ? TextLayerStyleAnimatorUpdate::EditingUniform|TextLayerStyleAnimatorUpdate::EditingPadding : TextLayerStyleAnimatorUpdates{}));
        CORRADE_VERIFY(!animator.isHandleValid(scheduledChangesPadding));
        CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 1);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(0), stoppedKept);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(1), AnimationHandle::Null);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(2), AnimationHandle::Null);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(3), AnimationHandle::Null);
        CORRADE_COMPARE_AS(layer.dynamicStyleFonts(), Containers::arrayView({
            fontHandle2,
            FontHandle::Null,
            FontHandle::Null,
            fontHandle2,
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(layer.dynamicStyleAlignments(), Containers::arrayView({
            Text::Alignment::BottomEnd,
            Text::Alignment::MiddleCenterIntegral,
            Text::Alignment::MiddleCenterIntegral,
            Text::Alignment::TopLeft,
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(layer.dynamicStyleFeatures(0))), (Containers::arrayView<const Containers::Pair<Text::Feature, UnsignedInt>>({
            {Text::Feature::HistoricalLigatures, true},
        })), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(layer.dynamicStyleFeatures(1))), (Containers::arrayView<const Containers::Pair<Text::Feature, UnsignedInt>>({
        })), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(layer.dynamicStyleFeatures(2))), (Containers::arrayView<const Containers::Pair<Text::Feature, UnsignedInt>>({
        })), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(layer.dynamicStyleFeatures(3))), (Containers::arrayView<const Containers::Pair<Text::Feature, UnsignedInt>>({
            {Text::Feature::StandardLigatures, false}
        })), TestSuite::Compare::Container);
        /* No styles get the bits modified */
        CORRADE_COMPARE_AS(Containers::BitArrayView{cursorStyles}, Containers::stridedArrayView({
            data.cursorStyles ? true : false,
            data.cursorStyles ? false : true,
            data.cursorStyles ? false : true,
            data.cursorStyles ? false : true,
        }).sliceBit(0), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(Containers::BitArrayView{selectionStyles}, Containers::stridedArrayView({
            data.selectionStyles ? true : false,
            data.selectionStyles ? false : true,
            data.selectionStyles ? false : true,
            data.selectionStyles ? false : true,
        }).sliceBit(0), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
            666u,
            shared.styleCount() + 0u,
            1u,
            6u,
            3u,
            1u,
            3u
        }), TestSuite::Compare::Container);
        /* The first dynamic style should get a 1/3 interpolation of uniforms 0
           and 1 and padding 6 and 3 */
        CORRADE_COMPARE(uniforms[0].color, Color4{4.0f/3.0f});
        CORRADE_COMPARE(paddings[0], Vector4{10.0f/3.0f});
        if(data.cursorStyles) {
            /* 1/3 interpolation of uniforms 3 and 0 and padding 0 and 1 */
            CORRADE_COMPARE(editingUniforms[0*2 + 1].backgroundColor, Color4{10.0f});
            CORRADE_COMPARE(editingPaddings[0*2 + 1], Vector4{64.0f/3.0f});
        }
        if(data.selectionStyles) {
            /* 1/3 interpolation of uniforms 0 and 1 and padding 4 and 2 */
            CORRADE_COMPARE(editingUniforms[0*2 + 0].backgroundColor, Color4{20.0f/3.0f});
            CORRADE_COMPARE(editingPaddings[0*2 + 0], Vector4{80.0f/3.0f});
            /* 1/3 of text uniforms -1 resolved to 0 and 2 */
            CORRADE_COMPARE(uniforms[shared.dynamicStyleCount() + 0*2 + 0].color, Color4{2.0f/3.0f});
        }
    }

    /* Removing the restarted animation recycles the dynamic style but doesn't
       switch the data style in any way, not even directly in the layer.
       Recycling inside AbstractVisualLayerStyleAnimator::doClean() is tested
       in uiAdvance() below. */
    animator.remove(stoppedKept);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 0);
    CORRADE_COMPARE(layer.style(data0), 4);
    CORRADE_COMPARE(layer.style(data1), 0);
    CORRADE_COMPARE(layer.style(data2), 2);
    CORRADE_COMPARE(layer.style(data3), 4);
    CORRADE_COMPARE(layer.style(data4), 5);
    CORRADE_COMPARE(layer.style(data5), 0);
    CORRADE_COMPARE(layer.style(data6), 2);
}

void TextLayerStyleAnimatorTest::advanceProperties() {
    auto&& data = AdvancePropertiesData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return Containers::pointer<EmptyShaper>(*this); }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(67, &font);

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{5, 3}
        .setEditingStyleCount(5, 3)
        .setDynamicStyleCount(1)
    };

    FontHandle fontHandle = shared.addFont(font, 1.0f);

    /* Has to be called early to be able to call TextLayer::create() which we
       need to to verify style ID updates, TextLayerStyleAnimator::create()
       itself doesn't need setStyle() to be called */
    Float uniformColors[]{
        4.0f, 2.0f, 0.0f, 2.0f, 0.0f
    };
    shared.setStyle(
        TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}
            .setColor(Color4{uniformColors[0]}),
         TextLayerStyleUniform{}
            .setColor(Color4{uniformColors[1]}),
         TextLayerStyleUniform{}
            .setColor(Color4{uniformColors[2]}),
         TextLayerStyleUniform{} /* same data as uniform 1, different index */
            .setColor(Color4{uniformColors[3]}),
         TextLayerStyleUniform{} /* same data as uniform 2, different index */
            .setColor(Color4{uniformColors[4]})},
        {data.uniform, 2, 1},
        {fontHandle, fontHandle, fontHandle},
        {Text::Alignment::MiddleCenter,
         Text::Alignment::MiddleCenter,
         Text::Alignment::MiddleCenter},
        {}, {}, {},
        {data.cursorStyle,
         -1,
         data.cursorStyle == -1 ? -1 : 2},
        {data.selectionStyle,
         -1,
         data.selectionStyle == -1 ? -1 : 0},
        {data.padding,
         Vector4{4.0f},
         Vector4{2.0f}});

    Float editingUniformColors[]{
        3.0f, 1.0f, 5.0f, 3.0f, 5.0f
    };
    shared.setEditingStyle(
        TextLayerCommonEditingStyleUniform{},
        {TextLayerEditingStyleUniform{}
            .setBackgroundColor(Color4{editingUniformColors[0]}),
         TextLayerEditingStyleUniform{}
            .setBackgroundColor(Color4{editingUniformColors[1]}),
         TextLayerEditingStyleUniform{}
            .setBackgroundColor(Color4{editingUniformColors[2]}),
         TextLayerEditingStyleUniform{} /* same data as uniform 0, diff index */
            .setBackgroundColor(Color4{editingUniformColors[3]}),
         TextLayerEditingStyleUniform{} /* same data as uniform 2, diff index */
            .setBackgroundColor(Color4{editingUniformColors[4]})},
        {2, data.editingUniform, 0},
        {data.editingTextUniform1,
         data.editingTextUniform2,
         -1},
        {Vector4{3.0f},
         data.editingPadding,
         Vector4{1.0f}});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    /* Assign data to a style that isn't used for animation */
    DataHandle layerData = layer.create(1, "", {});

    TextLayerStyleAnimator animator{animatorHandle(0, 1)};
    layer.assignAnimator(animator);

    AnimationHandle animation = animator.create(2, 0, Animation::Easing::linear, 0_nsec, 20_nsec, data.noAttachment ? DataHandle::Null : layerData);

    /* Does what layer's advanceAnimations() is doing internally for all
       animators (as we need to test also the interaction with animation being
       removed, etc.), but with an ability to peek into the filled data to
       verify they're written only when they should be. Compared to the helper
       in advance() above it's not exposing all data. */
    const auto advance = [&](Nanoseconds time, Containers::ArrayView<TextLayerStyleUniform> dynamicStyleUniforms, const Containers::StridedArrayView1D<Vector4>& dynamicStylePaddings, Containers::ArrayView<TextLayerEditingStyleUniform> dynamicEditingStyleUniforms, const Containers::StridedArrayView1D<Vector4>& dynamicEditingStylePaddings, const Containers::StridedArrayView1D<UnsignedInt>& dataStyles) {
        UnsignedByte activeStorage[1];
        UnsignedByte startedStorage[1];
        UnsignedByte stoppedStorage[1];
        Float factorStorage[1];
        UnsignedByte removeStorage[1];
        /* Those two being set or not being set are tested thoroughly
           enough in advance() */
        char cursorStyles[1];
        char selectionStyles[1];

        return animator.advance(time,
            Containers::MutableBitArrayView{activeStorage, 0, 1},
            Containers::MutableBitArrayView{startedStorage, 0, 1},
            Containers::MutableBitArrayView{stoppedStorage, 0, 1},
            factorStorage,
            Containers::MutableBitArrayView{removeStorage, 0, 1},
            dynamicStyleUniforms,
            Containers::MutableBitArrayView{cursorStyles, 0, 1},
            Containers::MutableBitArrayView{selectionStyles, 0, 1},
            dynamicStylePaddings, dynamicEditingStyleUniforms, dynamicEditingStylePaddings, dataStyles);
    };

    /* The padding resulting from the animation gets checked against these.
       Contrary to the advance() test case, set it to the initial padding value
       so the initial advance doesn't report padding as changed. */
    Vector4 paddings[]{
        Vector4{2.0f}
    };
    Vector4 editingPaddings[2]{
        Vector4{3.0f}, /* selection */
        Vector4{1.0f}  /* cursor */
    };

    /* The dataStyles are compared against to not break animations and style
       changes that happened since the original animation started and so they
       need to be preserved across advances. Behavior with external style
       changes is tested in advanceExternalStyleChanges(). */
    UnsignedInt dataStyles[]{
        666
    };

    /* Advancing to 5 allocates a dynamic style, switches to it and fills the
       dynamic data. The (Editing)Uniform is reported together with Style
       always in order to ensure the dynamic uniform is uploaded even though it
       won't subsequently change. */
    {
        TextLayerStyleUniform uniforms[3];
        TextLayerEditingStyleUniform editingUniforms[2];
        CORRADE_COMPARE(advance(5_nsec,
                uniforms,
                paddings,
                editingUniforms,
                editingPaddings,
                dataStyles),
            data.expectedUpdatesStart);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Playing);
        CORRADE_COMPARE(animator.dynamicStyle(animation), 0);
        CORRADE_COMPARE(uniforms[0].color, Math::lerp(Color4{2.0f}, Color4{uniformColors[data.uniform]}, 0.25f));
        CORRADE_COMPARE(paddings[0], Math::lerp(Vector4{2.0f}, data.padding, 0.25f));
        if(data.cursorStyle != -1) {
            CORRADE_COMPARE(editingUniforms[1].backgroundColor, Math::lerp(Color4{3.0f}, Color4{editingUniformColors[data.editingUniform]}, 0.25f));
            CORRADE_COMPARE(editingPaddings[1], Math::lerp(Vector4{1.0f}, data.editingPadding, 0.25f));
        }
        if(data.selectionStyle != -1) {
            CORRADE_COMPARE(editingUniforms[0].backgroundColor, Math::lerp(Color4{5.0f}, Color4{editingUniformColors[data.editingUniform]}, 0.25f));
            CORRADE_COMPARE(editingPaddings[0], Math::lerp(Vector4{3.0f}, data.editingPadding, 0.25f));
            CORRADE_COMPARE(uniforms[shared.dynamicStyleCount() + 0*2 + 0].color, Math::lerp(Color4{uniformColors[data.expectedEditingTextUniform1]}, Color4{uniformColors[data.expectedEditingTextUniform2]}, 0.25f));
        }
        CORRADE_COMPARE(dataStyles[0], data.noAttachment ? 666 : 3);

    /* Advancing to 15 changes only what's expected */
    } {
        TextLayerStyleUniform uniforms[3];
        TextLayerEditingStyleUniform editingUniforms[2];
        CORRADE_COMPARE(advance(15_nsec,
                uniforms,
                paddings,
                editingUniforms,
                editingPaddings,
                dataStyles),
            data.expectedUpdatesMiddle);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Playing);
        CORRADE_COMPARE(animator.dynamicStyle(animation), 0);
        CORRADE_COMPARE(uniforms[0].color, Math::lerp(Color4{2.0f}, Color4{uniformColors[data.uniform]}, 0.75f));
        CORRADE_COMPARE(paddings[0], Math::lerp(Vector4{2.0f}, data.padding, 0.75f));
        if(data.cursorStyle != -1) {
            CORRADE_COMPARE(editingUniforms[1].backgroundColor, Math::lerp(Color4{3.0f}, Color4{editingUniformColors[data.editingUniform]}, 0.75f));
            CORRADE_COMPARE(editingPaddings[1], Math::lerp(Vector4{1.0f}, data.editingPadding, 0.75f));
        }
        if(data.selectionStyle != -1) {
            CORRADE_COMPARE(editingUniforms[0].backgroundColor, Math::lerp(Color4{5.0f}, Color4{editingUniformColors[data.editingUniform]}, 0.75f));
            CORRADE_COMPARE(editingPaddings[0], Math::lerp(Vector4{3.0f}, data.editingPadding, 0.75f));
            CORRADE_COMPARE(uniforms[shared.dynamicStyleCount() + 0*2 + 0].color, Math::lerp(Color4{uniformColors[data.expectedEditingTextUniform1]}, Color4{uniformColors[data.expectedEditingTextUniform2]}, 0.75f));
        }
        CORRADE_COMPARE(dataStyles[0], data.noAttachment ? 666 : 3);

    /* Advancing to 25 changes only the Style if attached, the dynamic style
       values are unused now */
    } {
        TextLayerStyleUniform uniforms[3];
        TextLayerEditingStyleUniform editingUniforms[2];
        CORRADE_COMPARE(advance(25_nsec,
                uniforms,
                paddings,
                editingUniforms,
                editingPaddings,
                dataStyles),
        data.noAttachment ? TextLayerStyleAnimatorUpdates{} : TextLayerStyleAnimatorUpdate::Style);
        CORRADE_VERIFY(!animator.isHandleValid(animation));
        CORRADE_COMPARE(dataStyles[0], data.noAttachment ? 666 : 0);
    }
}

void TextLayerStyleAnimatorTest::advanceNoFreeDynamicStyles() {
    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return Containers::pointer<EmptyShaper>(*this); }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(67, &font);

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{4}
        .setDynamicStyleCount(1)
    };

    FontHandle fontHandle = shared.addFont(font, 1.0f);

    /* Has to be called early to be able to call TextLayer::create() which we
       need to to verify style ID updates, TextLayerStyleAnimator::create()
       itself doesn't need setStyle() to be called */
    shared.setStyle(
        TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}
            .setColor(Color4{0.25f}),
         TextLayerStyleUniform{}
            .setColor(Color4{0.75f}),
         TextLayerStyleUniform{}
            .setColor(Color4{1.25f}),
         TextLayerStyleUniform{}},
        {fontHandle, fontHandle, fontHandle, fontHandle},
        {Text::Alignment::MiddleCenter,
         Text::Alignment::MiddleCenter,
         Text::Alignment::MiddleCenter,
         Text::Alignment::MiddleCenter},
        {}, {}, {},
        /* Editing style presence has no effect on dynamic style recycling */
        {}, {},
        {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    TextLayerStyleAnimator animator{animatorHandle(0, 1)};
    layer.assignAnimator(animator);

    DataHandle data1 = layer.create(2, "", {});
    DataHandle data2 = layer.create(2, "", {});

    AnimationHandle first = animator.create(0, 1, Animation::Easing::linear, 0_nsec, 20_nsec, data2);
    AnimationHandle second = animator.create(2, 1, Animation::Easing::linear, 10_nsec, 40_nsec, data1);

    /* Does what layer's advanceAnimations() is doing internally for all
       animators (as we need to test also the interaction with animation being
       removed, etc.), but with an ability to peek into the filled data to
       verify they're written only when they should be. Compared to the helper
       in advance() above it exposes only some data. */
    const auto advance = [&](Nanoseconds time, Containers::ArrayView<TextLayerStyleUniform> dynamicStyleUniforms, const Containers::StridedArrayView1D<UnsignedInt>& dataStyles) {
        UnsignedByte activeStorage[1];
        UnsignedByte startedStorage[1];
        UnsignedByte stoppedStorage[1];
        Float factorStorage[2];
        UnsignedByte removeStorage[1];
        char cursorStyles[1];
        char selectionStyles[1];
        Vector4 paddings[1];

        return animator.advance(time,
            Containers::MutableBitArrayView{activeStorage, 0, 2},
            Containers::MutableBitArrayView{startedStorage, 0, 2},
            Containers::MutableBitArrayView{stoppedStorage, 0, 2},
            factorStorage,
            Containers::MutableBitArrayView{removeStorage, 0, 2},
            dynamicStyleUniforms,
            Containers::MutableBitArrayView{cursorStyles, 0, 1},
            Containers::MutableBitArrayView{selectionStyles, 0, 1},
            paddings, nullptr, nullptr, dataStyles);
    };

    TextLayerStyleUniform uniforms[1];
    UnsignedInt dataStyles[]{666, 666};

    /* First advance takes the only dynamic style and switches to it */
    CORRADE_COMPARE(advance(5_nsec, uniforms, dataStyles), TextLayerStyleAnimatorUpdate::Uniform|TextLayerStyleAnimatorUpdate::Style);
    CORRADE_COMPARE(animator.dynamicStyle(first), 0);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 1);
    CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
        666u,
        shared.styleCount() + 0u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(uniforms[0].color, Color4{0.375f});

    /* Next advance plays the other animation also, but isn't able to take any
       other dynamic style, so it updates the style index only to the initial
       style */
    CORRADE_COMPARE(advance(10_nsec, uniforms, dataStyles), TextLayerStyleAnimatorUpdate::Uniform|TextLayerStyleAnimatorUpdate::Style);
    CORRADE_COMPARE(animator.dynamicStyle(first), 0);
    CORRADE_COMPARE(animator.dynamicStyle(second), Containers::NullOpt);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 1);
    CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
        2u,
        shared.styleCount() + 0u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(uniforms[0].color, Color4{0.5f});

    /* Another advance still doesn't have any dynamic style to switch to, so
       it's just uniforms */
    CORRADE_COMPARE(advance(15_nsec, uniforms, dataStyles), TextLayerStyleAnimatorUpdate::Uniform);
    CORRADE_COMPARE(animator.dynamicStyle(first), 0);
    CORRADE_COMPARE(animator.dynamicStyle(second), Containers::NullOpt);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 1);
    CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
        2u,
        shared.styleCount() + 0u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(uniforms[0].color, Color4{0.625f});

    /* Next advance finishes the first animation and recycles its dynamic
       style, which allows the second animation to take over it */
    CORRADE_COMPARE(advance(20_nsec, uniforms, dataStyles), TextLayerStyleAnimatorUpdate::Uniform|TextLayerStyleAnimatorUpdate::Style);
    CORRADE_VERIFY(!animator.isHandleValid(first));
    CORRADE_COMPARE(animator.dynamicStyle(second), 0);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 1);
    CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
        shared.styleCount() + 0u,
        1u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(uniforms[0].color, Color4{1.125f});
}

void TextLayerStyleAnimatorTest::advanceConflictingAnimations() {
    auto&& data = AdvanceConflictingAnimationsData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return Containers::pointer<EmptyShaper>(*this); }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(67, &font);

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{4}
        .setDynamicStyleCount(2)
    };

    FontHandle fontHandle = shared.addFont(font, 1.0f);

    /* Has to be called early to be able to call TextLayer::create() which we
       need to to verify style ID updates, TextLayerStyleAnimator::create()
       itself doesn't need setStyle() to be called */
    shared.setStyle(
        TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}
            .setColor(Color4{0.25f}),
         TextLayerStyleUniform{}
            .setColor(Color4{0.75f}),
         TextLayerStyleUniform{}
            .setColor(Color4{1.25f}),
         TextLayerStyleUniform{}},
        {fontHandle, fontHandle, fontHandle, fontHandle},
        {Text::Alignment::MiddleCenter,
         Text::Alignment::MiddleCenter,
         Text::Alignment::MiddleCenter,
         Text::Alignment::MiddleCenter},
        {}, {}, {},
        /* Editing style presence has no effect on dynamic style recycling */
        {}, {},
        {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    TextLayerStyleAnimator animator{animatorHandle(0, 1)};
    layer.assignAnimator(animator);

    /* Create a second data just to ensure the zero index isn't updated by
       accident always */
    layer.create(3, "", {});
    DataHandle data2 = layer.create(3, "", {});

    AnimationHandle first = animator.create(0, 1, Animation::Easing::linear, 0_nsec, 20_nsec, data2, data.firstAnimationRepeatCount, data.firstAnimationFlags);
    /* If there are no free dynamic styles, the data should get style 2 both in
       the forward and reverse case */
    AnimationHandle second = animator.create(
        data.secondAnimationReverse ? 1 : 2,
        data.secondAnimationReverse ? 2 : 1,
        Animation::Easing::linear, 10_nsec, 40_nsec, data2,
        data.secondAnimationReverse ? AnimationFlag::Reverse : AnimationFlags{});

    /* Does what layer's advanceAnimations() is doing internally for all
       animators (as we need to test also the interaction with animation being
       removed, etc.), but with an ability to peek into the filled data to
       verify they're written only when they should be. Compared to the helper
       in advance() above it exposes only some data. */
    const auto advance = [&](Nanoseconds time, Containers::ArrayView<TextLayerStyleUniform> dynamicStyleUniforms, const Containers::StridedArrayView1D<UnsignedInt>& dataStyles) {
        UnsignedByte activeStorage[1];
        UnsignedByte startedStorage[1];
        UnsignedByte stoppedStorage[1];
        Float factorStorage[2];
        UnsignedByte removeStorage[1];
        char cursorStyles[1];
        char selectionStyles[1];
        Vector4 paddings[2];

        return animator.advance(time,
            Containers::MutableBitArrayView{activeStorage, 0, 2},
            Containers::MutableBitArrayView{startedStorage, 0, 2},
            Containers::MutableBitArrayView{stoppedStorage, 0, 2},
            factorStorage,
            Containers::MutableBitArrayView{removeStorage, 0, 2}, dynamicStyleUniforms,
            Containers::MutableBitArrayView{cursorStyles, 0, 2},
            Containers::MutableBitArrayView{selectionStyles, 0, 2},
            paddings, nullptr, nullptr, dataStyles);
    };

    TextLayerStyleUniform uniforms[2];
    UnsignedInt dataStyles[]{666, 666};

    /* First advance takes the dynamic style and switches to it */
    CORRADE_COMPARE(advance(5_nsec, uniforms, dataStyles), TextLayerStyleAnimatorUpdate::Uniform|TextLayerStyleAnimatorUpdate::Style);
    CORRADE_COMPARE(animator.dynamicStyle(first), 0);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 1);
    CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
        666u,
        shared.styleCount() + 0u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(uniforms[0].color, Color4{0.375f});

    /* Allocate the other dynamic style if testing the case where the other
       animation has none */
    if(data.noFreeDynamicStyles)
        layer.allocateDynamicStyle();

    /* Next advance plays the other animation affecting the same data. If
       there's no dynamic style left, it updates the index to the initial style
       instead. The first animation thus no longer affects the data anymore. */
    CORRADE_COMPARE(advance(10_nsec, uniforms, dataStyles), TextLayerStyleAnimatorUpdate::Uniform|TextLayerStyleAnimatorUpdate::Style);
    CORRADE_COMPARE(animator.dynamicStyle(first), 0);
    CORRADE_COMPARE(animator.dynamicStyle(second), data.noFreeDynamicStyles ? Containers::NullOpt : Containers::optional(1u));
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 2);
    CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
        666u,
        data.noFreeDynamicStyles ? 2u : shared.styleCount() + 1u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(uniforms[0].color, Color4{0.5f});

    /* Next advance either finishes or discards & removes the first animation
       and recycles its dynamic style, which allows the second animation to
       take over if it didn't have a dynamic style already. If the first
       animation isn't finishing yet and it's KeepOncePlayed, it's left
       untouched including its dynamic style. */
    CORRADE_COMPARE(advance(20_nsec, uniforms, dataStyles), TextLayerStyleAnimatorUpdate::Uniform|(data.noFreeDynamicStyles && data.expectedSecondDynamicStyle ? TextLayerStyleAnimatorUpdate::Style : TextLayerStyleAnimatorUpdate{}));
    CORRADE_COMPARE(animator.isHandleValid(first), data.firstAnimationFlags >= AnimationFlag::KeepOncePlayed);
    if(data.firstAnimationRepeatCount == 0 && data.firstAnimationFlags >= AnimationFlag::KeepOncePlayed)
        CORRADE_COMPARE(animator.dynamicStyle(first), 0);
    CORRADE_COMPARE(animator.dynamicStyle(second), data.expectedSecondDynamicStyle);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), data.expectedDynamicStyleCount);
    CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
        666u,
        data.expectedSecondDynamicStyle ? shared.styleCount() + *data.expectedSecondDynamicStyle : 2u
    }), TestSuite::Compare::Container);
    if(data.expectedSecondDynamicStyle)
        CORRADE_COMPARE(uniforms[*data.expectedSecondDynamicStyle].color, Color4{1.125f});
}

void TextLayerStyleAnimatorTest::advanceEmpty() {
    /* This should work even with no layer being set */
    TextLayerStyleAnimator animator{animatorHandle(0, 1)};
    animator.advance({}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});

    CORRADE_VERIFY(true);
}

void TextLayerStyleAnimatorTest::advanceInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return Containers::pointer<EmptyShaper>(*this); }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(67, &font);

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{2}
        .setDynamicStyleCount(2)
    }, sharedEditing{cache, TextLayer::Shared::Configuration{2}
        .setEditingStyleCount(1)
        .setDynamicStyleCount(2)
    };

    /* The editing layer has only the non-editing style set to check both
       style assertions. The non-editing layer has no style set. */
    FontHandle fontHandleEditing = sharedEditing.addFont(font, 1.0f);
    sharedEditing.setStyle(
        TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}, TextLayerStyleUniform{}},
        {fontHandleEditing, fontHandleEditing},
        {Text::Alignment{}, Text::Alignment{}},
        {}, {}, {}, {}, {}, {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared},
      layerEditing{layerHandle(0, 1), sharedEditing};

    TextLayerStyleAnimator animator{animatorHandle(0, 1)};
    TextLayerStyleAnimator animatorEditing{animatorHandle(0, 1)};
    layer.assignAnimator(animator);
    layerEditing.assignAnimator(animatorEditing);

    animator.create(0, 1, Animation::Easing::linear, 0_nsec, 1_nsec, DataHandle::Null);
    animator.create(0, 1, Animation::Easing::linear, 0_nsec, 1_nsec, DataHandle::Null);
    animator.create(0, 1, Animation::Easing::linear, 0_nsec, 1_nsec, DataHandle::Null);

    animatorEditing.create(0, 1, Animation::Easing::linear, 0_nsec, 1_nsec, DataHandle::Null);
    animatorEditing.create(0, 1, Animation::Easing::linear, 0_nsec, 1_nsec, DataHandle::Null);
    animatorEditing.create(0, 1, Animation::Easing::linear, 0_nsec, 1_nsec, DataHandle::Null);

    Containers::BitArray mask{NoInit, 3};
    Containers::BitArray maskInvalid{NoInit, 4};
    Float factors[3];
    Float factorsInvalid[4];
    TextLayerStyleUniform dynamicStyleUniforms[2];
    TextLayerStyleUniform dynamicStyleUniformsInvalid[3];
    TextLayerStyleUniform dynamicStyleUniformsEditing[6];
    TextLayerStyleUniform dynamicStyleUniformsEditingInvalid[5];
    char dynamicStyleCursorStyles[1];
    char dynamicStyleSelectionStyles[1];
    Vector4 dynamicStylePaddings[2];
    Vector4 dynamicStylePaddingsInvalid[3];
    TextLayerEditingStyleUniform dynamicEditingStyleUniforms[4];
    TextLayerEditingStyleUniform dynamicEditingStyleUniformsInvalid[3];
    Vector4 dynamicEditingStylePaddings[4];
    Vector4 dynamicEditingStylePaddingsInvalid[3];

    Containers::String out;
    Error redirectError{&out};
    animator.advance({}, mask, mask, mask, factors, maskInvalid,
        dynamicStyleUniforms,
        Containers::MutableBitArrayView{dynamicStyleCursorStyles, 0, 2},
        Containers::MutableBitArrayView{dynamicStyleSelectionStyles, 0, 2},
        dynamicStylePaddings,
        {}, {}, {});
    animator.advance({}, mask, mask, mask, factorsInvalid, mask,
        dynamicStyleUniforms,
        Containers::MutableBitArrayView{dynamicStyleCursorStyles, 0, 2},
        Containers::MutableBitArrayView{dynamicStyleSelectionStyles, 0, 2},
        dynamicStylePaddings,
        {}, {}, {});
    animator.advance({}, mask, mask, maskInvalid, factors, mask,
        dynamicStyleUniforms,
        Containers::MutableBitArrayView{dynamicStyleCursorStyles, 0, 2},
        Containers::MutableBitArrayView{dynamicStyleSelectionStyles, 0, 2},
        dynamicStylePaddings,
        {}, {}, {});
    animator.advance({}, mask, maskInvalid, mask, factors, mask,
        dynamicStyleUniforms,
        Containers::MutableBitArrayView{dynamicStyleCursorStyles, 0, 2},
        Containers::MutableBitArrayView{dynamicStyleSelectionStyles, 0, 2},
        dynamicStylePaddings,
        {}, {}, {});
    animator.advance({}, maskInvalid, mask, mask, factors, mask,
        dynamicStyleUniforms,
        Containers::MutableBitArrayView{dynamicStyleCursorStyles, 0, 2},
        Containers::MutableBitArrayView{dynamicStyleSelectionStyles, 0, 2},
        dynamicStylePaddings,
        {}, {}, {});
    /* Non-editing case */
    animator.advance({}, mask, mask, mask, factors, mask,
        dynamicStyleUniformsInvalid,
        Containers::MutableBitArrayView{dynamicStyleCursorStyles, 0, 2},
        Containers::MutableBitArrayView{dynamicStyleSelectionStyles, 0, 2},
        dynamicStylePaddings,
        {}, {}, {});
    animator.advance({}, mask, mask, mask, factors, mask,
        dynamicStyleUniforms,
        Containers::MutableBitArrayView{dynamicStyleCursorStyles, 0, 3},
        Containers::MutableBitArrayView{dynamicStyleSelectionStyles, 0, 2},
        dynamicStylePaddings,
        {}, {}, {});
    animator.advance({}, mask, mask, mask, factors, mask,
        dynamicStyleUniforms,
        Containers::MutableBitArrayView{dynamicStyleCursorStyles, 0, 2},
        Containers::MutableBitArrayView{dynamicStyleSelectionStyles, 0, 3},
        dynamicStylePaddings,
        {}, {}, {});
    animator.advance({}, mask, mask, mask, factors, mask,
        dynamicStyleUniforms,
        Containers::MutableBitArrayView{dynamicStyleCursorStyles, 0, 2},
        Containers::MutableBitArrayView{dynamicStyleSelectionStyles, 0, 2},
        dynamicStylePaddingsInvalid,
        {}, {}, {});
    /* Non-editing getting editing styles passed by accident */
    animator.advance({}, mask, mask, mask, factors, mask,
        dynamicStyleUniforms,
        Containers::MutableBitArrayView{dynamicStyleCursorStyles, 0, 2},
        Containers::MutableBitArrayView{dynamicStyleSelectionStyles, 0, 2},
        dynamicStylePaddings,
        {},
        dynamicEditingStylePaddings, {});
    animator.advance({}, mask, mask, mask, factors, mask,
        dynamicStyleUniforms,
        Containers::MutableBitArrayView{dynamicStyleCursorStyles, 0, 2},
        Containers::MutableBitArrayView{dynamicStyleSelectionStyles, 0, 2},
        dynamicStylePaddings,
        dynamicEditingStyleUniforms,
        {}, {});
    animator.advance({}, mask, mask, mask, factors, mask,
        dynamicStyleUniforms,
        Containers::MutableBitArrayView{dynamicStyleCursorStyles, 0, 2},
        Containers::MutableBitArrayView{dynamicStyleSelectionStyles, 0, 2},
        dynamicStylePaddings,
        dynamicEditingStyleUniforms,
        dynamicEditingStylePaddings, {});
    /* Editing case */
    animatorEditing.advance({}, mask, mask, mask, factors, mask,
        dynamicStyleUniformsEditingInvalid,
        Containers::MutableBitArrayView{dynamicStyleCursorStyles, 0, 2},
        Containers::MutableBitArrayView{dynamicStyleSelectionStyles, 0, 2},
        dynamicStylePaddings,
        dynamicEditingStyleUniforms,
        dynamicEditingStylePaddings, {});
    animatorEditing.advance({}, mask, mask, mask, factors, mask,
        dynamicStyleUniformsEditing,
        Containers::MutableBitArrayView{dynamicStyleCursorStyles, 0, 3},
        Containers::MutableBitArrayView{dynamicStyleSelectionStyles, 0, 2},
        dynamicStylePaddings,
        dynamicEditingStyleUniforms,
        dynamicEditingStylePaddings, {});
    animatorEditing.advance({}, mask, mask, mask, factors, mask,
        dynamicStyleUniformsEditing,
        Containers::MutableBitArrayView{dynamicStyleCursorStyles, 0, 2},
        Containers::MutableBitArrayView{dynamicStyleSelectionStyles, 0, 3},
        dynamicStylePaddings,
        dynamicEditingStyleUniforms,
        dynamicEditingStylePaddings, {});
    animatorEditing.advance({}, mask, mask, mask, factors, mask,
        dynamicStyleUniformsEditing,
        Containers::MutableBitArrayView{dynamicStyleCursorStyles, 0, 2},
        Containers::MutableBitArrayView{dynamicStyleSelectionStyles, 0, 2},
        dynamicStylePaddingsInvalid,
        dynamicEditingStyleUniforms,
        dynamicEditingStylePaddings, {});
    animatorEditing.advance({}, mask, mask, mask, factors, mask,
        dynamicStyleUniformsEditing,
        Containers::MutableBitArrayView{dynamicStyleCursorStyles, 0, 2},
        Containers::MutableBitArrayView{dynamicStyleSelectionStyles, 0, 2},
        dynamicStylePaddings,
        dynamicEditingStyleUniformsInvalid,
        dynamicEditingStylePaddings, {});
    animatorEditing.advance({}, mask, mask, mask, factors, mask,
        dynamicStyleUniformsEditing,
        Containers::MutableBitArrayView{dynamicStyleCursorStyles, 0, 2},
        Containers::MutableBitArrayView{dynamicStyleSelectionStyles, 0, 2},
        dynamicStylePaddings,
        dynamicEditingStyleUniforms,
        dynamicEditingStylePaddingsInvalid, {});
    /* Editing not getting editing styles passed by accident */
    animatorEditing.advance({}, mask, mask, mask, factors, mask,
        dynamicStyleUniformsEditing,
        Containers::MutableBitArrayView{dynamicStyleCursorStyles, 0, 2},
        Containers::MutableBitArrayView{dynamicStyleSelectionStyles, 0, 2},
        dynamicStylePaddings,
        {}, {}, {});
    /* All views correct but the layer doesn't have styles set */
    animator.advance({}, mask, mask, mask, factors, mask,
        dynamicStyleUniforms,
        Containers::MutableBitArrayView{dynamicStyleCursorStyles, 0, 2},
        Containers::MutableBitArrayView{dynamicStyleSelectionStyles, 0, 2},
        dynamicStylePaddings,
        {}, {}, {});
    /* This one doesn't have just editing styles set */
    animatorEditing.advance({}, mask, mask, mask, factors, mask,
        dynamicStyleUniformsEditing,
        Containers::MutableBitArrayView{dynamicStyleCursorStyles, 0, 2},
        Containers::MutableBitArrayView{dynamicStyleSelectionStyles, 0, 2},
        dynamicStylePaddings,
        dynamicEditingStyleUniforms,
        dynamicEditingStylePaddings, {});

    CORRADE_COMPARE_AS(out,
        /* These are caught by update() already, no need to repeat the
           assertion for the subclass. Verifying them here to ensure it doesn't
           accidentally blow up something earlier. */
        "Ui::AbstractAnimator::update(): expected active, started, stopped, factors and remove views to have a size of 3 but got 3, 3, 3, 3 and 4\n"
        "Ui::AbstractAnimator::update(): expected active, started, stopped, factors and remove views to have a size of 3 but got 3, 3, 3, 4 and 3\n"
        "Ui::AbstractAnimator::update(): expected active, started, stopped, factors and remove views to have a size of 3 but got 3, 3, 4, 3 and 3\n"
        "Ui::AbstractAnimator::update(): expected active, started, stopped, factors and remove views to have a size of 3 but got 3, 4, 3, 3 and 3\n"
        "Ui::AbstractAnimator::update(): expected active, started, stopped, factors and remove views to have a size of 3 but got 4, 3, 3, 3 and 3\n"

        "Ui::TextLayerStyleAnimator::advance(): expected dynamic style uniform, cursor style, selection style and padding views to have a size of 2, and the dynamic editing style uniform and paddings empty, but got 3, 2, 2, 2; 0 and 0\n"
        "Ui::TextLayerStyleAnimator::advance(): expected dynamic style uniform, cursor style, selection style and padding views to have a size of 2, and the dynamic editing style uniform and paddings empty, but got 2, 3, 2, 2; 0 and 0\n"
        "Ui::TextLayerStyleAnimator::advance(): expected dynamic style uniform, cursor style, selection style and padding views to have a size of 2, and the dynamic editing style uniform and paddings empty, but got 2, 2, 3, 2; 0 and 0\n"
        "Ui::TextLayerStyleAnimator::advance(): expected dynamic style uniform, cursor style, selection style and padding views to have a size of 2, and the dynamic editing style uniform and paddings empty, but got 2, 2, 2, 3; 0 and 0\n"
        "Ui::TextLayerStyleAnimator::advance(): expected dynamic style uniform, cursor style, selection style and padding views to have a size of 2, and the dynamic editing style uniform and paddings empty, but got 2, 2, 2, 2; 0 and 4\n"
        "Ui::TextLayerStyleAnimator::advance(): expected dynamic style uniform, cursor style, selection style and padding views to have a size of 2, and the dynamic editing style uniform and paddings empty, but got 2, 2, 2, 2; 4 and 0\n"
        "Ui::TextLayerStyleAnimator::advance(): expected dynamic style uniform, cursor style, selection style and padding views to have a size of 2, and the dynamic editing style uniform and paddings empty, but got 2, 2, 2, 2; 4 and 4\n"

        "Ui::TextLayerStyleAnimator::advance(): expected dynamic style cursor style, selection style and padding views to have a size of 2, the dynamic style uniform view a size of 6, and the dynamic editing style uniform and padding views a size of 4, but got 2, 2, 2; 5; 4 and 4\n"
        "Ui::TextLayerStyleAnimator::advance(): expected dynamic style cursor style, selection style and padding views to have a size of 2, the dynamic style uniform view a size of 6, and the dynamic editing style uniform and padding views a size of 4, but got 3, 2, 2; 6; 4 and 4\n"
        "Ui::TextLayerStyleAnimator::advance(): expected dynamic style cursor style, selection style and padding views to have a size of 2, the dynamic style uniform view a size of 6, and the dynamic editing style uniform and padding views a size of 4, but got 2, 3, 2; 6; 4 and 4\n"
        "Ui::TextLayerStyleAnimator::advance(): expected dynamic style cursor style, selection style and padding views to have a size of 2, the dynamic style uniform view a size of 6, and the dynamic editing style uniform and padding views a size of 4, but got 2, 2, 3; 6; 4 and 4\n"
        "Ui::TextLayerStyleAnimator::advance(): expected dynamic style cursor style, selection style and padding views to have a size of 2, the dynamic style uniform view a size of 6, and the dynamic editing style uniform and padding views a size of 4, but got 2, 2, 2; 6; 3 and 4\n"
        "Ui::TextLayerStyleAnimator::advance(): expected dynamic style cursor style, selection style and padding views to have a size of 2, the dynamic style uniform view a size of 6, and the dynamic editing style uniform and padding views a size of 4, but got 2, 2, 2; 6; 4 and 3\n"
        "Ui::TextLayerStyleAnimator::advance(): expected dynamic style cursor style, selection style and padding views to have a size of 2, the dynamic style uniform view a size of 6, and the dynamic editing style uniform and padding views a size of 4, but got 2, 2, 2; 6; 0 and 0\n"

        "Ui::TextLayerStyleAnimator::advance(): no style data was set on the layer\n"
        "Ui::TextLayerStyleAnimator::advance(): no editing style data was set on the layer\n",
        TestSuite::Compare::String);
}

void TextLayerStyleAnimatorTest::advanceInvalidCursorSelection() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return Containers::pointer<EmptyShaper>(*this); }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(67, &font);

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{2, 5}
        .setEditingStyleCount(1)
        .setDynamicStyleCount(2)
    };

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    TextLayerStyleAnimator animator{animatorHandle(0, 1)};
    layer.assignAnimator(animator);

    animator.create(0, 1, Animation::Easing::linear, 0_nsec, 5_nsec, DataHandle::Null);
    animator.create(2, 3, Animation::Easing::linear, 10_nsec, 15_nsec, DataHandle::Null);
    animator.create(3, 2, Animation::Easing::linear, 20_nsec, 25_nsec, DataHandle::Null);
    animator.create(2, 4, Animation::Easing::linear, 30_nsec, 35_nsec, DataHandle::Null);
    animator.create(4, 2, Animation::Easing::linear, 40_nsec, 45_nsec, DataHandle::Null);

    /* Set the style after animation creation to verify it isn't needed
       earlier */
    FontHandle fontHandle = shared.addFont(font, 1.0f);
    shared.setStyle(
        TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}, TextLayerStyleUniform{}},
        {0, 1, 0, 0, 0},
        {fontHandle, fontHandle, fontHandle, fontHandle, fontHandle},
        {Text::Alignment{}, Text::Alignment{}, Text::Alignment{}, Text::Alignment{}, Text::Alignment{}},
        {}, {}, {},
        /* Style 2 references both cursor and selection styles, 3 just
           selection, 4 just cursor */
        {-1, -1, 0, -1, 0}, {-1, -1, 0, 0, -1},
        {});
    shared.setEditingStyle(
        TextLayerCommonEditingStyleUniform{},
        {TextLayerEditingStyleUniform{}},
        {}, {{}});

    Containers::BitArray activeStorage{NoInit, 5};
    Containers::BitArray startedStorage{NoInit, 5};
    Containers::BitArray stoppedStorage{NoInit, 5};
    Float factorStorage[5];
    Containers::BitArray removeStorage{NoInit, 5};
    TextLayerStyleUniform dynamicStyleUniforms[6];
    char dynamicStyleCursorStyles[1];
    char dynamicStyleSelectionStyles[1];
    Vector4 dynamicStylePaddings[2];
    TextLayerEditingStyleUniform dynamicEditingStyleUniforms[4];
    Vector4 dynamicEditingStylePaddings[4];

    /* This advance() should be fine, the views are all sized properly and the
       first animation is correct */
    {
        CORRADE_COMPARE(animator.advance(0_nsec,
            activeStorage, startedStorage, stoppedStorage, factorStorage, removeStorage,
            dynamicStyleUniforms,
            Containers::MutableBitArrayView{dynamicStyleCursorStyles, 0, 2},
            Containers::MutableBitArrayView{dynamicStyleSelectionStyles, 0, 2},
            dynamicStylePaddings,
            dynamicEditingStyleUniforms,
            dynamicEditingStylePaddings, {}), TextLayerStyleAnimatorUpdate::Uniform);
    }

    Containers::String out;
    Error redirectError{&out};
    {
        animator.advance(10_nsec,
            activeStorage, startedStorage, stoppedStorage, factorStorage, removeStorage,
            dynamicStyleUniforms,
            Containers::MutableBitArrayView{dynamicStyleCursorStyles, 0, 2},
            Containers::MutableBitArrayView{dynamicStyleSelectionStyles, 0, 2},
            dynamicStylePaddings,
            dynamicEditingStyleUniforms,
            dynamicEditingStylePaddings, {});
    } {
        animator.advance(20_nsec,
            activeStorage, startedStorage, stoppedStorage, factorStorage, removeStorage,
            dynamicStyleUniforms,
            Containers::MutableBitArrayView{dynamicStyleCursorStyles, 0, 2},
            Containers::MutableBitArrayView{dynamicStyleSelectionStyles, 0, 2},
            dynamicStylePaddings,
            dynamicEditingStyleUniforms,
            dynamicEditingStylePaddings, {});
    } {
        animator.advance(30_nsec,
            activeStorage, startedStorage, stoppedStorage, factorStorage, removeStorage,
            dynamicStyleUniforms,
            Containers::MutableBitArrayView{dynamicStyleCursorStyles, 0, 2},
            Containers::MutableBitArrayView{dynamicStyleSelectionStyles, 0, 2},
            dynamicStylePaddings,
            dynamicEditingStyleUniforms,
            dynamicEditingStylePaddings, {});
    } {
        animator.advance(40_nsec,
            activeStorage, startedStorage, stoppedStorage, factorStorage, removeStorage,
            dynamicStyleUniforms,
            Containers::MutableBitArrayView{dynamicStyleCursorStyles, 0, 2},
            Containers::MutableBitArrayView{dynamicStyleSelectionStyles, 0, 2},
            dynamicStylePaddings,
            dynamicEditingStyleUniforms,
            dynamicEditingStylePaddings, {});
    }
    CORRADE_COMPARE_AS(out,
        "Ui::TextLayerStyleAnimator::advance(): expected style 3 to reference a cursor style like style 2 for Ui::AnimationHandle({0x0, 0x1}, {0x1, 0x1})\n"
        "Ui::TextLayerStyleAnimator::advance(): expected style 2 to not reference a cursor style like style 3 for Ui::AnimationHandle({0x0, 0x1}, {0x2, 0x1})\n"
        "Ui::TextLayerStyleAnimator::advance(): expected style 4 to reference a selection style like style 2 for Ui::AnimationHandle({0x0, 0x1}, {0x3, 0x1})\n"
        "Ui::TextLayerStyleAnimator::advance(): expected style 2 to not reference a selection style like style 4 for Ui::AnimationHandle({0x0, 0x1}, {0x4, 0x1})\n",
        TestSuite::Compare::String);
}

void TextLayerStyleAnimatorTest::layerAdvance() {
    auto&& data = LayerAdvanceData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return Containers::pointer<EmptyShaper>(*this); }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(67, &font);

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{3}
        .setEditingStyleCount(data.editingStyles ? 2 : 0)
        .setDynamicStyleCount(1)
    };

    FontHandle fontHandle = shared.addFont(font, 1.0f);

    /* Has to be called early to be able to call TextLayer::create() which we
       need to to verify style ID updates, TextLayerStyleAnimator::create()
       itself doesn't need setStyle() to be called */
    shared.setStyle(
        TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}
            .setColor(Color4{0.75f}),
         TextLayerStyleUniform{},
         TextLayerStyleUniform{}
            .setColor(Color4{0.25f})},
        {2, data.uniform, 1},
        {fontHandle, fontHandle, fontHandle},
        {Text::Alignment::MiddleCenter,
         Text::Alignment::MiddleCenter,
         Text::Alignment::MiddleCenter},
        {}, {}, {},
        {data.editingStyles ? 1 : -1,
         data.editingStyles ? 0 : -1,
         -1},
        {-1, -1, -1},
        {{}, Vector4{data.padding}, {}});
    if(data.editingStyles) shared.setEditingStyle(
        TextLayerCommonEditingStyleUniform{},
        {TextLayerEditingStyleUniform{}
            .setBackgroundColor(Color4{0.5f}),
         TextLayerEditingStyleUniform{}
            .setBackgroundColor(Color4{1.0f})},
        {1, data.editingUniform},
        {},
        {Vector4{data.editingPadding}, {}});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}

        TextLayer::State& stateData() {
            return static_cast<TextLayer::State&>(*_state);
        }
    } layer{layerHandle(0, 1), shared};

    /* Required to be called before update() (because AbstractUserInterface
       guarantees the same on a higher level), not needed for anything here */
    layer.setSize({1, 1}, {1, 1});

    DataHandle data1 = layer.create(2, "", {});
    DataHandle data2 = layer.create(2, "", {});

    TextLayerStyleAnimator animator1{animatorHandle(0, 1)};
    TextLayerStyleAnimator animatorEmpty{animatorHandle(0, 1)};
    TextLayerStyleAnimator animator2{animatorHandle(0, 1)};
    layer.assignAnimator(animator1);
    layer.assignAnimator(animatorEmpty);
    layer.assignAnimator(animator2);

    animator1.create(0, 1, Animation::Easing::linear, 0_nsec, 20_nsec, data2, AnimationFlag::KeepOncePlayed);
    animator2.create(1, 0, Animation::Easing::linear, 13_nsec, 1_nsec, data1);

    /* The storage can be bigger than needed, the layer should slice it for
       each animator */
    Containers::BitArray activeStorage{NoInit, 7};
    Containers::BitArray startedStorage{NoInit, 7};
    Containers::BitArray stoppedStorage{NoInit, 7};
    Float factorStorage[7];
    Containers::BitArray removeStorage{NoInit, 7};

    /* Advancing just the first animation to 1/4, which sets the style,
       uniform and optionally padding */
    layer.advanceAnimations(5_nsec, activeStorage, startedStorage, stoppedStorage, factorStorage, removeStorage, {animator2, animatorEmpty, animator1});
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 1);
    CORRADE_COMPARE(layer.style(data2), shared.styleCount() + 0);
    CORRADE_COMPARE(layer.dynamicStyleUniforms()[0].color, !data.editingStyles && data.expectCommonDataChanges ? Color4{0.375f} : Color4{0.25f});
    CORRADE_COMPARE(layer.dynamicStylePaddings()[0], Vector4{data.padding}*0.25f);
    if(data.editingStyles) {
        CORRADE_COMPARE(layer.dynamicEditingStyleUniforms()[2*0 + 1].backgroundColor, data.expectCommonDataChanges ? Color4{0.625f} : Color4{1.0f});
        CORRADE_COMPARE(layer.dynamicEditingStylePaddings()[2*0 + 1], Vector4{data.editingPadding}*0.25f);
    }
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate);
    CORRADE_VERIFY(layer.stateData().dynamicStyleChanged);
    CORRADE_COMPARE(layer.stateData().dynamicEditingStyleChanged, data.editingStyles);

    /* Advancing the first animation to 1/2, which sets just what's expected */
    layer.update(LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    layer.stateData().dynamicStyleChanged = false;
    layer.stateData().dynamicEditingStyleChanged = false;
    layer.advanceAnimations(10_nsec, activeStorage, startedStorage, stoppedStorage, factorStorage, removeStorage, {animator2, animatorEmpty, animator1});
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 1);
    CORRADE_COMPARE(layer.style(data2), shared.styleCount() + 0);
    CORRADE_COMPARE(layer.dynamicStyleUniforms()[0].color, !data.editingStyles && data.expectCommonDataChanges ? Color4{0.5f} : Color4{0.25f});
    CORRADE_COMPARE(layer.dynamicStylePaddings()[0], Vector4{data.padding}*0.5f);
    if(data.editingStyles) {
        CORRADE_COMPARE(layer.dynamicEditingStyleUniforms()[2*0 + 1].backgroundColor, data.expectCommonDataChanges ? Color4{0.75f} : Color4{1.0f});
        CORRADE_COMPARE(layer.dynamicEditingStylePaddings()[2*0 + 1], Vector4{data.editingPadding}*0.5f);
    }
    CORRADE_COMPARE(layer.state(),
        (data.expectDataChanges ? LayerState::NeedsDataUpdate : LayerStates{})|
        (data.expectCommonDataChanges ? LayerState::NeedsCommonDataUpdate : LayerStates{}));
    CORRADE_COMPARE(layer.stateData().dynamicStyleChanged, !data.editingStyles && data.expectCommonDataChanges);
    CORRADE_COMPARE(layer.stateData().dynamicEditingStyleChanged, data.editingStyles && data.expectCommonDataChanges);

    /* Advancing both the first animation to 3/4 and second animation directly
       to the final style. It should thus set both the update and the style
       change. */
    layer.update(LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    layer.stateData().dynamicStyleChanged = false;
    layer.stateData().dynamicEditingStyleChanged = false;
    layer.advanceAnimations(15_nsec, activeStorage, startedStorage, stoppedStorage, factorStorage, removeStorage, {animator2, animatorEmpty, animator1});
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 1);
    CORRADE_COMPARE(layer.style(data1), 0);
    CORRADE_COMPARE(layer.style(data2), shared.styleCount() + 0);
    CORRADE_COMPARE(layer.dynamicStyleUniforms()[0].color, !data.editingStyles && data.expectCommonDataChanges ? Color4{0.625f} : Color4{0.25f});
    CORRADE_COMPARE(layer.dynamicStylePaddings()[0], Vector4{data.padding}*0.75f);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate|(data.expectCommonDataChanges ? LayerState::NeedsCommonDataUpdate : LayerStates{}));
    CORRADE_COMPARE(layer.stateData().dynamicStyleChanged, !data.editingStyles && data.expectCommonDataChanges);
    CORRADE_COMPARE(layer.stateData().dynamicEditingStyleChanged, data.editingStyles && data.expectCommonDataChanges);

    /* Advancing the first animation to the end & the final style. Only the
       style data is updated, no uniforms or paddings. */
    layer.update(LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    layer.stateData().dynamicStyleChanged = false;
    layer.stateData().dynamicEditingStyleChanged = false;
    layer.advanceAnimations(20_nsec, activeStorage, startedStorage, stoppedStorage, factorStorage, removeStorage, {animator2, animatorEmpty, animator1});
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 0);
    CORRADE_COMPARE(layer.style(data2), 1);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
    CORRADE_VERIFY(!layer.stateData().dynamicStyleChanged);
    CORRADE_VERIFY(!layer.stateData().dynamicEditingStyleChanged);
}

void TextLayerStyleAnimatorTest::uiAdvance() {
    /* Verifies that removing a data with an animation attached properly cleans
       the attached dynamic style (if there's any) in
       AbstractVisualLayerStyleAnimator::doClean() */

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return Containers::pointer<EmptyShaper>(*this); }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(67, &font);

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{3}
        .setDynamicStyleCount(1)
    };

    FontHandle fontHandle = shared.addFont(font, 1.0f);

    /* Has to be called early to be able to call TextLayer::create() which we
       need to to verify style ID updates, TextLayerStyleAnimator::create()
       itself doesn't need setStyle() to be called */
    shared.setStyle(
        TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}
            .setColor(Color4{0.75f}),
         TextLayerStyleUniform{},
         TextLayerStyleUniform{}
            .setColor(Color4{0.25f})},
        {fontHandle, fontHandle, fontHandle},
        {Text::Alignment::MiddleCenter,
         Text::Alignment::MiddleCenter,
         Text::Alignment::MiddleCenter},
        {}, {}, {}, {}, {}, {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    };

    AbstractUserInterface ui{{100, 100}};

    TextLayer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared));

    Containers::Pointer<TextLayerStyleAnimator> animatorInstance{InPlaceInit, ui.createAnimator()};
    layer.assignAnimator(*animatorInstance);
    TextLayerStyleAnimator& animator = ui.setStyleAnimatorInstance(Utility::move(animatorInstance));

    DataHandle data = layer.create(2, "", {});

    /* Creating animations doesn't allocate dynamic styles just yet, only
       advance() does */
    AnimationHandle withoutDynamicStyle = animator.create(0, 1, Animation::Easing::linear, 10_nsec, 10_nsec, data);
    AnimationHandle withDynamicStyle = animator.create(1, 0, Animation::Easing::linear, 0_nsec, 10_nsec, data);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 0);
    CORRADE_COMPARE(animator.usedCount(), 2);

    ui.advanceAnimations(5_nsec);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 1);
    CORRADE_COMPARE(animator.usedCount(), 2);
    CORRADE_COMPARE(animator.dynamicStyle(withoutDynamicStyle), Containers::NullOpt);
    CORRADE_COMPARE(animator.dynamicStyle(withDynamicStyle), 0);

    /* Removing data and then advancing again calls appropriate clean() to
       recycle the used dynamic style */
    layer.remove(data);
    ui.advanceAnimations(6_nsec);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 0);
    CORRADE_COMPARE(animator.usedCount(), 0);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::TextLayerStyleAnimatorTest)
