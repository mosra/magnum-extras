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

#include <sstream> /** @todo remove once Debug is stream-free */
#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/Iterable.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/Reference.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/StringStl.h> /** @todo remove once Debug is stream-free */
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Container.h>
#include <Corrade/TestSuite/Compare/String.h>
#include <Corrade/Utility/DebugStl.h> /** @todo remove once Debug is stream-free */
#include <Magnum/PixelFormat.h>
#include <Magnum/Animation/Easing.h>
#include <Magnum/Math/Time.h>
#include <Magnum/Text/AbstractGlyphCache.h>
#include <Magnum/Text/AbstractFont.h>
#include <Magnum/Text/AbstractShaper.h>
#include <Magnum/Text/Alignment.h>

#include "Magnum/Whee/Handle.h"
#include "Magnum/Whee/TextLayer.h"
#include "Magnum/Whee/TextLayerAnimator.h"
#include "Magnum/Whee/TextProperties.h"
#include "Magnum/Whee/Implementation/textLayerState.h" /* for layerAdvance() */

namespace Magnum { namespace Whee { namespace Test { namespace {

struct TextLayerStyleAnimatorTest: TestSuite::Tester {
    explicit TextLayerStyleAnimatorTest();

    void debugAnimation();
    void debugAnimations();

    void construct();
    void constructCopy();
    void constructMove();

    void setAnimator();
    void setAnimatorInvalid();

    template<class T> void createRemove();
    void createRemoveHandleRecycle();
    void createInvalid();
    /* There's no assert to trigger in remove() other than what's checked by
       AbstractAnimator::remove() already */
    void propertiesInvalid();

    void clean();
    void cleanEmpty();
    /* There's no assert to trigger in clean() other than what's checked by
       AbstractAnimator::clean() already */

    void advance();
    void advanceNoFreeDynamicStyles();
    void advanceEmpty();
    void advanceInvalid();

    void layerAdvance();
};

using namespace Math::Literals;

enum class Enum: UnsignedShort {};

Debug& operator<<(Debug& debug, Enum value) {
    return debug << UnsignedInt(value);
}

const struct {
    const char* name;
    Vector4 padding;
} LayerAdvanceData[]{
    {"", {}},
    {"padding changes as well", Vector4{2.0f}}
};

TextLayerStyleAnimatorTest::TextLayerStyleAnimatorTest() {
    addTests({&TextLayerStyleAnimatorTest::debugAnimation,
              &TextLayerStyleAnimatorTest::debugAnimations,

              &TextLayerStyleAnimatorTest::construct,
              &TextLayerStyleAnimatorTest::constructCopy,
              &TextLayerStyleAnimatorTest::constructMove,

              &TextLayerStyleAnimatorTest::setAnimator,
              &TextLayerStyleAnimatorTest::setAnimatorInvalid,

              &TextLayerStyleAnimatorTest::createRemove<UnsignedInt>,
              &TextLayerStyleAnimatorTest::createRemove<Enum>,
              &TextLayerStyleAnimatorTest::createRemoveHandleRecycle,
              &TextLayerStyleAnimatorTest::createInvalid,
              &TextLayerStyleAnimatorTest::propertiesInvalid,

              &TextLayerStyleAnimatorTest::clean,
              &TextLayerStyleAnimatorTest::cleanEmpty,

              &TextLayerStyleAnimatorTest::advance,
              &TextLayerStyleAnimatorTest::advanceNoFreeDynamicStyles,
              &TextLayerStyleAnimatorTest::advanceEmpty,
              &TextLayerStyleAnimatorTest::advanceInvalid});

    addInstancedTests({&TextLayerStyleAnimatorTest::layerAdvance},
        Containers::arraySize(LayerAdvanceData));
}

void TextLayerStyleAnimatorTest::debugAnimation() {
    std::ostringstream out;
    Debug{&out} << TextLayerStyleAnimation::Style << TextLayerStyleAnimation(0xbe);
    CORRADE_COMPARE(out.str(), "Whee::TextLayerStyleAnimation::Style Whee::TextLayerStyleAnimation(0xbe)\n");
}

void TextLayerStyleAnimatorTest::debugAnimations() {
    std::ostringstream out;
    Debug{&out} << (TextLayerStyleAnimation::Uniform|TextLayerStyleAnimation(0xe0)) << TextLayerStyleAnimations{};
    CORRADE_COMPARE(out.str(), "Whee::TextLayerStyleAnimation::Uniform|Whee::TextLayerStyleAnimation(0xe0) Whee::TextLayerStyleAnimations{}\n");
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

void TextLayerStyleAnimatorTest::setAnimator() {
    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{2}
        .setDynamicStyleCount(1)
    };

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    TextLayerStyleAnimator animator{animatorHandle(0, 1)};
    CORRADE_COMPARE(animator.layer(), LayerHandle::Null);

    layer.setAnimator(animator);
    CORRADE_COMPARE(animator.layer(), layer.handle());
}

void TextLayerStyleAnimatorTest::setAnimatorInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{2}};

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    TextLayerStyleAnimator animator{animatorHandle(0, 1)};
    CORRADE_COMPARE(animator.layer(), LayerHandle::Null);

    std::ostringstream out;
    Error redirectError{&out};
    layer.setAnimator(animator);
    CORRADE_COMPARE(out.str(), "Whee::TextLayer::setAnimator(): can't animate a layer with zero dynamic styles\n");
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
        explicit LayerShared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        using TextLayer::Shared::setGlyphCache;

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{5, 3}
        .setDynamicStyleCount(1)
    };
    shared.setGlyphCache(cache);

    FontHandle fontHandle = shared.addFont(font, 1.0f);

    /* Have more uniforms that are sparsely indexed into to verify the data get
       correctly fetched */
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{},               /* 0 */
         TextLayerStyleUniform{}                /* 1 */
            .setColor(0xff3366_rgbf),
         TextLayerStyleUniform{}                /* 2 */
            .setColor(0xcc66aa_rgbf),
         TextLayerStyleUniform{},               /* 3 */
         TextLayerStyleUniform{}                /* 4 */
            .setColor(0x9933ff_rgbf)},
        {4, 1, 2},
        {fontHandle, fontHandle, fontHandle},
        {Text::Alignment::MiddleCenter,
         Text::Alignment::MiddleCenter,
         Text::Alignment::MiddleCenter},
        {Vector4{1.0f},
         {2.0f, 3.0f, 4.0f, 5.0f},
         {}});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    TextLayerStyleAnimator animator{animatorHandle(0, 1)};
    layer.setAnimator(animator);

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
    CORRADE_COMPARE(animator.played(first), 12_nsec);
    CORRADE_COMPARE(animator.data(first), data2);
    CORRADE_COMPARE(animator.targetStyle(first), 1);
    /* Can't use T, as the function restricts to enum types which would fail
       for T == UnsignedInt */
    CORRADE_COMPARE(animator.template targetStyle<Enum>(first), Enum(1));
    CORRADE_COMPARE(animator.dynamicStyle(first), Containers::NullOpt);
    CORRADE_COMPARE(animator.easing(first), Animation::Easing::linear);
    /* Styles 0 and 1 are uniforms 4 and 1 */
    CORRADE_COMPARE(animator.uniforms(first).first().color, 0x9933ff_rgbf);
    CORRADE_COMPARE(animator.uniforms(first).second().color, 0xff3366_rgbf);
    CORRADE_COMPARE(animator.paddings(first), Containers::pair(Vector4{1.0f}, Vector4{2.0f, 3.0f, 4.0f, 5.0f}));
    /* Dynamic style is only allocated and switched to during advance() */
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 0);
    CORRADE_COMPARE(layer.style(data2), 2);

    /* Implicit repeat count, no data attachment (which thus shouldn't try to
       access anything data-related in the layer) */
    AnimationHandle second = animator.create(T(2), T(0), Animation::Easing::cubicIn, -15_nsec, 1_nsec, DataHandle::Null, AnimationFlag(0x40));
    CORRADE_COMPARE(animator.duration(second), 1_nsec);
    CORRADE_COMPARE(animator.repeatCount(second), 1);
    CORRADE_COMPARE(animator.flags(second), AnimationFlag(0x40));
    CORRADE_COMPARE(animator.played(second), -15_nsec);
    CORRADE_COMPARE(animator.data(second), DataHandle::Null);
    CORRADE_COMPARE(animator.targetStyle(second), 0);
    /* Can't use T, as the function restricts to enum types which would fail
       for T == UnsignedInt */
    CORRADE_COMPARE(animator.template targetStyle<Enum>(second), Enum(0));
    CORRADE_COMPARE(animator.dynamicStyle(second), Containers::NullOpt);
    CORRADE_COMPARE(animator.easing(second), Animation::Easing::cubicIn);
    /* Styles 2 and 0 are uniforms 2 and 4 */
    CORRADE_COMPARE(animator.uniforms(second).first().color, 0xcc66aa_rgbf);
    CORRADE_COMPARE(animator.uniforms(second).second().color, 0x9933ff_rgbf);
    CORRADE_COMPARE(animator.paddings(second), Containers::pair(Vector4{0.0f}, Vector4{1.0f}));
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 0);

    /* LayerDataHandle overload, verify also with AnimatorDataHandle */
    AnimationHandle third = animator.create(T(1), T(2), Animation::Easing::bounceInOut, 0_nsec, 100_nsec, dataHandleData(data3), 0, AnimationFlag(0x80));
    CORRADE_COMPARE(animator.duration(animationHandleData(third)), 100_nsec);
    CORRADE_COMPARE(animator.repeatCount(animationHandleData(third)), 0);
    CORRADE_COMPARE(animator.flags(animationHandleData(third)), AnimationFlag(0x80));
    CORRADE_COMPARE(animator.played(animationHandleData(third)), 0_nsec);
    CORRADE_COMPARE(animator.data(animationHandleData(third)), data3);
    CORRADE_COMPARE(animator.targetStyle(animationHandleData(third)), 2);
    /* Can't use T, as the function restricts to enum types which would fail
       for T == UnsignedInt */
    CORRADE_COMPARE(animator.template targetStyle<Enum>(animationHandleData(third)), Enum(2));
    CORRADE_COMPARE(animator.dynamicStyle(animationHandleData(third)), Containers::NullOpt);
    CORRADE_COMPARE(animator.easing(animationHandleData(third)), Animation::Easing::bounceInOut);
    /* Styles 1 and 2 are uniforms 1 and 2 */
    CORRADE_COMPARE(animator.uniforms(animationHandleData(third)).first().color, 0xff3366_rgbf);
    CORRADE_COMPARE(animator.uniforms(animationHandleData(third)).second().color, 0xcc66aa_rgbf);
    CORRADE_COMPARE(animator.paddings(animationHandleData(third)), Containers::pair(Vector4{2.0f, 3.0f, 4.0f, 5.0f}, Vector4{0.0f}));
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 0);
    CORRADE_COMPARE(layer.style(data3), 0);

    /* LayerDataHandle overload with implicit repeat count */
    AnimationHandle fourth = animator.create(T(0), T(2), Animation::Easing::smoothstep, 20_nsec, 10_nsec, dataHandleData(data1), AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.duration(fourth), 10_nsec);
    CORRADE_COMPARE(animator.repeatCount(fourth), 1);
    CORRADE_COMPARE(animator.flags(fourth), AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.played(fourth), 20_nsec);
    CORRADE_COMPARE(animator.data(fourth), data1);
    CORRADE_COMPARE(animator.targetStyle(fourth), 2u);
    /* Can't use T, as the function restricts to enum types which would fail
       for T == UnsignedInt */
    CORRADE_COMPARE(animator.template targetStyle<Enum>(fourth), Enum(2));
    CORRADE_COMPARE(animator.dynamicStyle(fourth), Containers::NullOpt);
    CORRADE_COMPARE(animator.easing(fourth), Animation::Easing::smoothstep);
    /* Styles 0 and 2 are uniforms 4 and 2 */
    CORRADE_COMPARE(animator.uniforms(fourth).first().color, 0x9933ff_rgbf);
    CORRADE_COMPARE(animator.uniforms(fourth).second().color, 0xcc66aa_rgbf);
    CORRADE_COMPARE(animator.paddings(fourth), Containers::pair(Vector4{1.0f}, Vector4{0.0f}));
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
        explicit LayerShared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        using TextLayer::Shared::setGlyphCache;

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{2}
        .setDynamicStyleCount(1)
    };
    shared.setGlyphCache(cache);

    FontHandle fontHandle = shared.addFont(font, 1.0f);

    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}
            .setColor(0xff3366_rgbf),
         TextLayerStyleUniform{}
            .setColor(0x9933ff_rgbf)},
        {fontHandle, fontHandle},
        {Text::Alignment::MiddleCenter,
         Text::Alignment::MiddleCenter},
        {Vector4{1.0f},
         Vector4{2.0f}});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    TextLayerStyleAnimator animator{animatorHandle(0, 1)};
    layer.setAnimator(animator);

    DataHandle data = layer.create(1, "", {});

    /* Allocate an animation */
    AnimationHandle first = animator.create(0, 1, Animation::Easing::linear, 0_nsec, 13_nsec, data);
    CORRADE_COMPARE(animator.targetStyle(first), 1u);
    CORRADE_COMPARE(animator.dynamicStyle(first), Containers::NullOpt);
    CORRADE_COMPARE(animator.easing(first), Animation::Easing::linear);
    CORRADE_COMPARE(animator.uniforms(first).first().color, 0xff3366_rgbf);
    CORRADE_COMPARE(animator.uniforms(first).second().color, 0x9933ff_rgbf);
    CORRADE_COMPARE(animator.paddings(first), Containers::pair(Vector4{1.0f}, Vector4{2.0f}));
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 0);

    /* Let it advance to allocate the dynamic style */
    TextLayerStyleUniform dynamicStyleUniforms[1];
    Vector4 dynamicStylePaddings[1];
    UnsignedInt dataStyles[1];
    animator.advance(0_nsec, dynamicStyleUniforms, dynamicStylePaddings, dataStyles);
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
    AnimationHandle first2 = animator.create(1, 0, Animation::Easing::bounceInOut, -10_nsec, 100_nsec, data);
    CORRADE_COMPARE(animationHandleId(first2), animationHandleId(first));
    CORRADE_COMPARE(animator.targetStyle(first2), 0u);
    CORRADE_COMPARE(animator.dynamicStyle(first2), Containers::NullOpt);
    CORRADE_COMPARE(animator.easing(first2), Animation::Easing::bounceInOut);
    CORRADE_COMPARE(animator.uniforms(first2).first().color, 0x9933ff_rgbf);
    CORRADE_COMPARE(animator.uniforms(first2).second().color, 0xff3366_rgbf);
    CORRADE_COMPARE(animator.paddings(first2), Containers::pair(Vector4{2.0f}, Vector4{1.0f}));
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 0);
}

void TextLayerStyleAnimatorTest::createInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } sharedNoStyleSet{TextLayer::Shared::Configuration{5}
        .setDynamicStyleCount(1)},
      shared{TextLayer::Shared::Configuration{1, 5}
        .setDynamicStyleCount(1)};

    shared.setStyle(
        TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}},
        {0, 0, 0, 0, 0},
        {FontHandle::Null, FontHandle::Null, FontHandle::Null, FontHandle::Null, FontHandle::Null},
        {Text::Alignment{}, Text::Alignment{}, Text::Alignment{}, Text::Alignment{}, Text::Alignment{}},
        {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layerNoStyleSet{layerHandle(0, 1), sharedNoStyleSet},
      layer{layerHandle(0, 1), shared};

    TextLayerStyleAnimator animatorNoLayerSet{animatorHandle(0, 1)};

    TextLayerStyleAnimator animatorNoLayerStyleSet{animatorHandle(0, 1)};
    layerNoStyleSet.setAnimator(animatorNoLayerStyleSet);

    TextLayerStyleAnimator animator{animatorHandle(0, 1)};
    layer.setAnimator(animator);

    std::ostringstream out;
    Error redirectError{&out};
    /* Verify all four create() overloads check the layer being set early
       enough */
    animatorNoLayerSet.create(0, 1, Animation::Easing::linear, 12_nsec, 13_nsec, DataHandle::Null, 1, AnimationFlags{});
    animatorNoLayerSet.create(0, 1, Animation::Easing::linear, 12_nsec, 13_nsec, DataHandle::Null, AnimationFlags{});
    animatorNoLayerSet.create(0, 1, Animation::Easing::linear, 12_nsec, 13_nsec, LayerDataHandle::Null, 1, AnimationFlags{});
    animatorNoLayerSet.create(0, 1, Animation::Easing::linear, 12_nsec, 13_nsec, LayerDataHandle::Null, AnimationFlags{});
    animatorNoLayerStyleSet.create(0, 1, Animation::Easing::linear, 12_nsec, 13_nsec, DataHandle::Null);
    animator.create(0, 5, Animation::Easing::linear, 12_nsec, 13_nsec, DataHandle::Null);
    animator.create(5, 0, Animation::Easing::linear, 12_nsec, 13_nsec, DataHandle::Null);
    animator.create(0, 1, nullptr, 12_nsec, 13_nsec, DataHandle::Null);
    /* Other things like data handle layer part not matching etc. tested in
       AbstractAnimatorTest already */
    CORRADE_COMPARE_AS(out.str(),
        "Whee::TextLayerStyleAnimator::create(): no layer set\n"
        "Whee::TextLayerStyleAnimator::create(): no layer set\n"
        "Whee::TextLayerStyleAnimator::create(): no layer set\n"
        "Whee::TextLayerStyleAnimator::create(): no layer set\n"
        "Whee::TextLayerStyleAnimator::create(): no style data was set on the layer\n"
        "Whee::TextLayerStyleAnimator::create(): expected source and target style to be in range for 5 styles but got 0 and 5\n"
        "Whee::TextLayerStyleAnimator::create(): expected source and target style to be in range for 5 styles but got 5 and 0\n"
        "Whee::TextLayerStyleAnimator::create(): easing is null\n",
        TestSuite::Compare::String);
}

void TextLayerStyleAnimatorTest::propertiesInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{2}
        .setDynamicStyleCount(1)
    };
    shared.setStyle(
        TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}, TextLayerStyleUniform{}},
        {FontHandle::Null, FontHandle::Null},
        {Text::Alignment{}, Text::Alignment{}},
        {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    TextLayerStyleAnimator animator{animatorHandle(0, 1)};
    layer.setAnimator(animator);

    AnimationHandle handle = animator.create(0, 1, Animation::Easing::linear, 12_nsec, 13_nsec, DataHandle::Null);

    std::ostringstream out;
    Error redirectError{&out};
    animator.targetStyle(AnimationHandle::Null);
    animator.dynamicStyle(AnimationHandle::Null);
    animator.easing(AnimationHandle::Null);
    animator.uniforms(AnimationHandle::Null);
    animator.paddings(AnimationHandle::Null);
    /* Valid animator, invalid data */
    animator.targetStyle(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)));
    animator.dynamicStyle(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)));
    animator.easing(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)));
    animator.uniforms(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)));
    animator.paddings(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)));
    /* Invalid animator, valid data */
    animator.targetStyle(animationHandle(AnimatorHandle::Null, animationHandleData(handle)));
    animator.dynamicStyle(animationHandle(AnimatorHandle::Null, animationHandleData(handle)));
    animator.easing(animationHandle(AnimatorHandle::Null, animationHandleData(handle)));
    animator.uniforms(animationHandle(AnimatorHandle::Null, animationHandleData(handle)));
    animator.paddings(animationHandle(AnimatorHandle::Null, animationHandleData(handle)));
    /* AnimatorDataHandle directly */
    animator.targetStyle(AnimatorDataHandle(0x123abcde));
    animator.dynamicStyle(AnimatorDataHandle(0x123abcde));
    animator.easing(AnimatorDataHandle(0x123abcde));
    animator.uniforms(AnimatorDataHandle(0x123abcde));
    animator.paddings(AnimatorDataHandle(0x123abcde));
    CORRADE_COMPARE_AS(out.str(),
        "Whee::TextLayerStyleAnimator::targetStyle(): invalid handle Whee::AnimationHandle::Null\n"
        "Whee::TextLayerStyleAnimator::dynamicStyle(): invalid handle Whee::AnimationHandle::Null\n"
        "Whee::TextLayerStyleAnimator::easing(): invalid handle Whee::AnimationHandle::Null\n"
        "Whee::TextLayerStyleAnimator::uniforms(): invalid handle Whee::AnimationHandle::Null\n"
        "Whee::TextLayerStyleAnimator::paddings(): invalid handle Whee::AnimationHandle::Null\n"

        "Whee::TextLayerStyleAnimator::targetStyle(): invalid handle Whee::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Whee::TextLayerStyleAnimator::dynamicStyle(): invalid handle Whee::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Whee::TextLayerStyleAnimator::easing(): invalid handle Whee::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Whee::TextLayerStyleAnimator::uniforms(): invalid handle Whee::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Whee::TextLayerStyleAnimator::paddings(): invalid handle Whee::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"

        "Whee::TextLayerStyleAnimator::targetStyle(): invalid handle Whee::AnimationHandle(Null, {0x0, 0x1})\n"
        "Whee::TextLayerStyleAnimator::dynamicStyle(): invalid handle Whee::AnimationHandle(Null, {0x0, 0x1})\n"
        "Whee::TextLayerStyleAnimator::easing(): invalid handle Whee::AnimationHandle(Null, {0x0, 0x1})\n"
        "Whee::TextLayerStyleAnimator::uniforms(): invalid handle Whee::AnimationHandle(Null, {0x0, 0x1})\n"
        "Whee::TextLayerStyleAnimator::paddings(): invalid handle Whee::AnimationHandle(Null, {0x0, 0x1})\n"

        "Whee::TextLayerStyleAnimator::targetStyle(): invalid handle Whee::AnimatorDataHandle(0xabcde, 0x123)\n"
        "Whee::TextLayerStyleAnimator::dynamicStyle(): invalid handle Whee::AnimatorDataHandle(0xabcde, 0x123)\n"
        "Whee::TextLayerStyleAnimator::easing(): invalid handle Whee::AnimatorDataHandle(0xabcde, 0x123)\n"
        "Whee::TextLayerStyleAnimator::uniforms(): invalid handle Whee::AnimatorDataHandle(0xabcde, 0x123)\n"
        "Whee::TextLayerStyleAnimator::paddings(): invalid handle Whee::AnimatorDataHandle(0xabcde, 0x123)\n",
        TestSuite::Compare::String);
}

void TextLayerStyleAnimatorTest::clean() {
    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{2}
        .setDynamicStyleCount(3)
    };
    shared.setStyle(
        TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}, TextLayerStyleUniform{}},
        {FontHandle::Null, FontHandle::Null},
        {Text::Alignment{}, Text::Alignment{}},
        {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    TextLayerStyleAnimator animator{animatorHandle(0, 1)};
    layer.setAnimator(animator);

    /* Creating animations doesn't allocate dynamic styles just yet, only
       advance() does */
    AnimationHandle first = animator.create(0, 1, Animation::Easing::linear, 12_nsec, 13_nsec, DataHandle::Null);
    AnimationHandle second = animator.create(0, 1, Animation::Easing::linear, 12_nsec, 13_nsec, DataHandle::Null);
    AnimationHandle third = animator.create(0, 1, Animation::Easing::linear, 12_nsec, 13_nsec, DataHandle::Null);
    CORRADE_COMPARE(animator.usedCount(), 3);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 0);

    /* So cleaning them shouldn't try to recycle them either. Cleaning
       animations with allocated dynamic styles is tested in advance(). */
    UnsignedByte animationIdsToRemove[]{0x05}; /* 0b101 */
    animator.clean(Containers::BitArrayView{animationIdsToRemove, 0, 3});
    CORRADE_COMPARE(animator.usedCount(), 1);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 0);
    CORRADE_VERIFY(!animator.isHandleValid(first));
    CORRADE_VERIFY(animator.isHandleValid(second));
    CORRADE_VERIFY(!animator.isHandleValid(third));
}

void TextLayerStyleAnimatorTest::cleanEmpty() {
    /* This should work even with no layer being set */
    TextLayerStyleAnimator animator{animatorHandle(0, 1)};
    animator.clean({});

    CORRADE_VERIFY(true);
}

void TextLayerStyleAnimatorTest::advance() {
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
        explicit LayerShared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        using TextLayer::Shared::setGlyphCache;

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{4, 7}
        .setDynamicStyleCount(3)
    };
    shared.setGlyphCache(cache);

    FontHandle fontHandle = shared.addFont(font, 1.0f);

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
        {fontHandle, fontHandle, fontHandle, fontHandle,
         fontHandle, fontHandle, fontHandle},
        {Text::Alignment::MiddleCenter,
         Text::Alignment::MiddleCenter,
         Text::Alignment::MiddleCenter,
         Text::Alignment::MiddleCenter,
         Text::Alignment::MiddleCenter,
         Text::Alignment::MiddleCenter,
         Text::Alignment::MiddleCenter},
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

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    /* Assign data to styles that aren't used for animation */
    DataHandle data0 = layer.create(4, "", {});
    DataHandle data1 = layer.create(0, "", {});
    DataHandle data2 = layer.create(2, "", {});
    DataHandle data3 = layer.create(4, "", {});
    DataHandle data4 = layer.create(5, "", {});

    TextLayerStyleAnimator animator{animatorHandle(0, 1)};
    layer.setAnimator(animator);

    /* This one allocates a dynamic style, interpolates between uniforms 1 and
       2 with just Uniform set and when stopped sets the data2 style to 1 */
    AnimationHandle playing = animator.create(3, 1, Animation::Easing::linear, 0_nsec, 20_nsec, data2);
    /* This one sets the data4 style to 3 and is removed without even
       allocating a dynamic style or marking Uniform or Padding as changed */
    AnimationHandle stopped = animator.create(1, 3, Animation::Easing::cubicOut, 0_nsec, 1_nsec, data4);
    /* This one is a reverse of the first, scheduled later and not attached to
       any data, thus it never marks Style as changed */
    AnimationHandle scheduledNullData = animator.create(1, 3, Animation::Easing::linear, 15_nsec, 10_nsec, DataHandle::Null);
    /* This one allocates a dynamic style once played, interpolates all the way
       to 3 and stays */
    AnimationHandle stoppedKept = animator.create(6, 3, Animation::Easing::cubicIn, 0_nsec, 1_nsec, data1, AnimationFlag::KeepOncePlayed);
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
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 0);

    /* The padding resulting from the animation gets checked against these
       values, so set them to something very different to make sure they get
       updated */
    Vector4 paddings[]{
        Vector4{Constants::nan()},
        Vector4{Constants::nan()},
        Vector4{Constants::nan()}
    };

    /* Advancing to 5 allocates a dynamic style for the playing animation,
       switches the style to it and fills the dynamic data. For the stopped
       & removed animation it switches the style to the destination one, for
       the stopped & kept it allocates a dynamic style, transitions to the
       final style but doesn't recycle it. */
    {
        TextLayerStyleUniform uniforms[3];
        UnsignedInt dataStyles[]{666, 666, 666, 666, 666};
        CORRADE_COMPARE(animator.advance(5_nsec, uniforms, paddings, dataStyles), TextLayerStyleAnimation::Uniform|TextLayerStyleAnimation::Style|TextLayerStyleAnimation::Padding);
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
        CORRADE_COMPARE(animator.dynamicStyle(stoppedKept), 1);
        CORRADE_COMPARE(animator.dynamicStyle(scheduledChangesPadding), Containers::NullOpt);
        CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 2);
        /* Style IDs in the layer aren't changed, the passed array is instead,
           and only where dynamic styles got allocated or the animation
           stopped */
        CORRADE_COMPARE(layer.style(data0), 4);
        CORRADE_COMPARE(layer.style(data1), 0);
        CORRADE_COMPARE(layer.style(data2), 2);
        CORRADE_COMPARE(layer.style(data3), 4);
        CORRADE_COMPARE(layer.style(data4), 5);
        CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
            666u,
            shared.styleCount() + 1u,
            shared.styleCount() + 0u,
            666u,
            3u,
        }), TestSuite::Compare::Container);
        /* The first dynamic style should get a 1/4 interpolation of uniforms 1
           and 2 and the constant padding value */
        CORRADE_COMPARE(uniforms[0].color, Color4{3.5f});
        CORRADE_COMPARE(paddings[0], Vector4{2.0f});
        /* The stopped but kept style should get exactly the uniform 1 value,
           and the constant padding */
        CORRADE_COMPARE(uniforms[1].color, Color4{4.0f});
        CORRADE_COMPARE(paddings[1], Vector4{2.0f});
    }

    /* Reset the padding of the stopped & kept style to something else to
       verify it doesn't get touched anymore */
    paddings[1] = {};

    /* Advancing to 10 changes just the uniform to 1/2, nothing else. In
       particular, the style values aren't touched even though they're now
       different. */
    {
        TextLayerStyleUniform uniforms[3];
        UnsignedInt dataStyles[]{666, 666, 666, 666, 666};
        CORRADE_COMPARE(animator.advance(10_nsec, uniforms, paddings, dataStyles), TextLayerStyleAnimation::Uniform);
        CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 2);
        CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
            666u, 666u, 666u, 666u, 666u,
        }), TestSuite::Compare::Container);
        /* Testing just a subset, assuming the rest is updated accordingly */
        CORRADE_COMPARE(uniforms[0].color, Color4{3.0f});
        CORRADE_COMPARE(paddings[0], Vector4{2.0f});
        /* The stopped & kept style isn't touched anymore, staying at the reset
           defaults */
        CORRADE_COMPARE(uniforms[1].color, Color4{1.0f});
        CORRADE_COMPARE(paddings[1], Vector4{0.0f});
    }

    /* Advancing to 15 plays the also scheduled animation without a data
       attachment, allocating a new dynamic style but not switching to it.
       I.e., no Style is set, only Uniform and Padding. */
    {
        TextLayerStyleUniform uniforms[3];
        UnsignedInt dataStyles[]{666, 666, 666, 666, 666};
        CORRADE_COMPARE(animator.advance(15_nsec, uniforms, paddings, dataStyles), TextLayerStyleAnimation::Uniform|TextLayerStyleAnimation::Padding);
        CORRADE_COMPARE(animator.state(scheduledNullData), AnimationState::Playing);
        CORRADE_COMPARE(animator.dynamicStyle(scheduledNullData), 2);
        CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 3);
        CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
            666u, 666u, 666u, 666u, 666u,
        }), TestSuite::Compare::Container);
        /* The playing animation is advanced to 3/4 */
        CORRADE_COMPARE(uniforms[0].color, Color4{2.5f});
        CORRADE_COMPARE(paddings[0], Vector4{2.0f});
        /* The stopped & kept style isn't touched anymore, staying at the reset
           defaults */
        CORRADE_COMPARE(uniforms[1].color, Color4{1.0f});
        CORRADE_COMPARE(paddings[1], Vector4{0.0f});
        /* The null data animation is set to the value of style 1 */
        CORRADE_COMPARE(uniforms[2].color, Color4{2.0f});
        CORRADE_COMPARE(paddings[2], Vector4{2.0f});
    }

    /* Advancing to 20 stops the first animation, recycling its dynamic style
       and changing the style to the target one. Uniform value is updated for
       the null data animation. */
    {
        TextLayerStyleUniform uniforms[3];
        UnsignedInt dataStyles[]{666, 666, 666, 666, 666};
        CORRADE_COMPARE(animator.advance(20_nsec, uniforms, paddings, dataStyles), TextLayerStyleAnimation::Style|TextLayerStyleAnimation::Uniform);
        CORRADE_VERIFY(!animator.isHandleValid(playing));
        CORRADE_VERIFY(animator.isHandleValid(stoppedKept));
        CORRADE_VERIFY(animator.isHandleValid(scheduledChangesPadding));
        CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 2);
        CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
            666u,
            666u,
            1u,
            666u,
            666u,
        }), TestSuite::Compare::Container);
        /* Uniform values of the recycled style aren't touched anymore */
        CORRADE_COMPARE(uniforms[0].color, Color4{1.0f});
        /* The stopped & kept style isn't touched anymore, staying at the reset
           defaults */
        CORRADE_COMPARE(uniforms[1].color, Color4{1.0f});
        CORRADE_COMPARE(paddings[1], Vector4{0.0f});
        /* The null data animation is advanced to 1/2 between style 1 and 3 */
        CORRADE_COMPARE(uniforms[2].color, Color4{3.0f});
        CORRADE_COMPARE(paddings[2], Vector4{2.0f});
    }

    /* Advancing to 25 stops the null data animation, recycling its dynamic
       style. Leads to no other change, i.e. no Style set. */
    {
        TextLayerStyleUniform uniforms[3];
        UnsignedInt dataStyles[]{666, 666, 666, 666, 666};
        CORRADE_COMPARE(animator.advance(25_nsec, uniforms, paddings, dataStyles), TextLayerStyleAnimations{});
        CORRADE_VERIFY(!animator.isHandleValid(scheduledNullData));
        CORRADE_VERIFY(animator.isHandleValid(stoppedKept));
        CORRADE_VERIFY(animator.isHandleValid(scheduledChangesPadding));
        CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 1);
        CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
            666u, 666u, 666u, 666u, 666u,
        }), TestSuite::Compare::Container);
        /* Uniform values of the recycled styles aren't touched anymore */
        CORRADE_COMPARE(uniforms[0].color, Color4{1.0f});
        CORRADE_COMPARE(uniforms[2].color, Color4{1.0f});
        /* The stopped & kept style isn't touched anymore, staying at the reset
           defaults */
        CORRADE_COMPARE(uniforms[1].color, Color4{1.0f});
        CORRADE_COMPARE(paddings[1], Vector4{0.0f});
    }

    /* Advancing to 35 plays the scheduled animation, allocating a new dynamic
       style and switching to it */
    {
        TextLayerStyleUniform uniforms[3];
        UnsignedInt dataStyles[]{666, 666, 666, 666, 666};
        CORRADE_COMPARE(animator.advance(35_nsec, uniforms, paddings, dataStyles), TextLayerStyleAnimation::Uniform|TextLayerStyleAnimation::Style|TextLayerStyleAnimation::Padding);
        CORRADE_VERIFY(animator.isHandleValid(stoppedKept));
        CORRADE_VERIFY(animator.isHandleValid(scheduledChangesPadding));
        CORRADE_COMPARE(animator.state(stoppedKept), AnimationState::Stopped);
        CORRADE_COMPARE(animator.state(scheduledChangesPadding), AnimationState::Playing);
        CORRADE_COMPARE(animator.dynamicStyle(scheduledChangesPadding), 0);
        CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 2);
        CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
            666u,
            666u,
            666u,
            shared.styleCount() + 0u,
            666u
        }), TestSuite::Compare::Container);
        /* The first dynamic style should get a 3/4 interpolation (i.e.,
           reverted from 1/4) of uniforms 1 and 0 and padding 3 and 6 */
        CORRADE_COMPARE(uniforms[0].color, Color4{1.0f});
        CORRADE_COMPARE(paddings[0], Vector4{3.5f});
        /* The stopped & kept style isn't touched anymore, staying at the reset
           defaults */
        CORRADE_COMPARE(uniforms[1].color, Color4{1.0f});
        CORRADE_COMPARE(paddings[1], Vector4{0.0f});
    }

    /* Advancing to 45 advances the scheduled animation, changing both the
       uniform and the padding. No styles. */
    {
        TextLayerStyleUniform uniforms[3];
        UnsignedInt dataStyles[]{666, 666, 666, 666, 666};
        CORRADE_COMPARE(animator.advance(45_nsec, uniforms, paddings, dataStyles), TextLayerStyleAnimation::Uniform|TextLayerStyleAnimation::Padding);
        CORRADE_VERIFY(animator.isHandleValid(stoppedKept));
        CORRADE_VERIFY(animator.isHandleValid(scheduledChangesPadding));
        CORRADE_COMPARE(animator.state(stoppedKept), AnimationState::Stopped);
        CORRADE_COMPARE(animator.state(scheduledChangesPadding), AnimationState::Playing);
        CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 2);
        CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
            666u, 666u, 666u, 666u, 666u,
        }), TestSuite::Compare::Container);
        /* The first dynamic style should get a 1/4 interpolation (i.e.,
           reverted from 3/4) of uniforms 1 and 0 and padding 3 and 6 */
        CORRADE_COMPARE(uniforms[0].color, Color4{3.0f});
        CORRADE_COMPARE(paddings[0], Vector4{2.5f});
        /* The stopped & kept style isn't touched anymore, staying at the reset
           defaults */
        CORRADE_COMPARE(uniforms[1].color, Color4{1.0f});
        CORRADE_COMPARE(paddings[1], Vector4{0.0f});
    }

    /* Removing the stopped & kept animation recycles the dynamic style but
       doesn't switch the data style in any way, not even directly in the
       layer */
    animator.remove(stoppedKept);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 1);
    CORRADE_COMPARE(layer.style(data0), 4);
    CORRADE_COMPARE(layer.style(data1), 0);
    CORRADE_COMPARE(layer.style(data2), 2);
    CORRADE_COMPARE(layer.style(data3), 4);
    CORRADE_COMPARE(layer.style(data4), 5);

    /* Stopping the remaining animation (even before it finishes at 50) makes
       it recycle the remaining dynamic style and switch to the target style at
       the next advance(). Not updating any uniforms or paddings. */
    {
        TextLayerStyleUniform uniforms[3];
        UnsignedInt dataStyles[]{666, 666, 666, 666, 666};
        animator.stop(scheduledChangesPadding, 46_nsec);
        CORRADE_COMPARE(animator.advance(47_nsec, uniforms, paddings, dataStyles), TextLayerStyleAnimation::Style);
        CORRADE_VERIFY(!animator.isHandleValid(scheduledChangesPadding));
        CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 0);
        CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
            666u,
            666u,
            666u,
            6u,
            666u
        }), TestSuite::Compare::Container);
    }

    /* Final verification that styles in the layer aren't directly changed */
    CORRADE_COMPARE(layer.style(data0), 4);
    CORRADE_COMPARE(layer.style(data1), 0);
    CORRADE_COMPARE(layer.style(data2), 2);
    CORRADE_COMPARE(layer.style(data3), 4);
    CORRADE_COMPARE(layer.style(data4), 5);
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
        explicit LayerShared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        using TextLayer::Shared::setGlyphCache;

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{3}
        .setDynamicStyleCount(1)
    };
    shared.setGlyphCache(cache);

    FontHandle fontHandle = shared.addFont(font, 1.0f);

    shared.setStyle(
        TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}
            .setColor(Color4{0.25f}),
         TextLayerStyleUniform{}
            .setColor(Color4{0.75f}),
         TextLayerStyleUniform{}},
        {fontHandle, fontHandle, fontHandle},
        {Text::Alignment::MiddleCenter,
         Text::Alignment::MiddleCenter,
         Text::Alignment::MiddleCenter},
        {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    TextLayerStyleAnimator animator{animatorHandle(0, 1)};
    layer.setAnimator(animator);

    DataHandle data1 = layer.create(2, "", {});
    DataHandle data2 = layer.create(2, "", {});

    AnimationHandle first = animator.create(0, 1, Animation::Easing::linear, 0_nsec, 20_nsec, data2);
    AnimationHandle second = animator.create(1, 0, Animation::Easing::linear, 10_nsec, 20_nsec, data1);

    TextLayerStyleUniform uniforms[1];
    Vector4 paddings[1];
    UnsignedInt dataStyles[]{666, 666};

    /* First advance takes the only dynamic style and switches to it */
    {
        CORRADE_COMPARE(animator.advance(5_nsec, uniforms, paddings, dataStyles), TextLayerStyleAnimation::Uniform|TextLayerStyleAnimation::Style);
        CORRADE_COMPARE(animator.dynamicStyle(first), 0);
        CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 1);
        CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
            666u,
            shared.styleCount() + 0u
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE(uniforms[0].color, Color4{0.375f});

    /* Next advance plays the other animation also, but isn't able to take any
       other dynamic style, so it doesn't update any style index */
    } {
        CORRADE_COMPARE(animator.advance(10_nsec, uniforms, paddings, dataStyles), TextLayerStyleAnimation::Uniform);
        CORRADE_COMPARE(animator.dynamicStyle(first), 0);
        CORRADE_COMPARE(animator.dynamicStyle(second), Containers::NullOpt);
        CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 1);
        CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
            666u,
            shared.styleCount() + 0u
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE(uniforms[0].color, Color4{0.5f});

    /* Next advance finishes the first animation and recycles its dynamic
       style. But the recycling is done after the allocation, so the second
       animation still isn't doing anything. */
    } {
        CORRADE_COMPARE(animator.advance(20_nsec, uniforms, paddings, dataStyles), TextLayerStyleAnimation::Style);
        CORRADE_VERIFY(!animator.isHandleValid(first));
        CORRADE_COMPARE(animator.dynamicStyle(second), Containers::NullOpt);
        CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 0);
        CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
            666u,
            1u
        }), TestSuite::Compare::Container);
        /* No uniforms updated in this case */

    /* Advancing right after is finally able to allocate the recycled style */
    } {
        CORRADE_COMPARE(animator.advance(25_nsec, uniforms, paddings, dataStyles), TextLayerStyleAnimation::Uniform|TextLayerStyleAnimation::Style);
        CORRADE_COMPARE(animator.dynamicStyle(second), 0);
        CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 1);
        CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
            shared.styleCount() + 0u,
            1u
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE(uniforms[0].color, Color4{0.375f});
    }
}

void TextLayerStyleAnimatorTest::advanceEmpty() {
    /* This should work even with no layer being set */
    TextLayerStyleAnimator animator{animatorHandle(0, 1)};
    animator.advance({}, {}, {}, {});

    CORRADE_VERIFY(true);
}

void TextLayerStyleAnimatorTest::advanceInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{2}
        .setDynamicStyleCount(1)
    };

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    TextLayerStyleAnimator animator{animatorHandle(0, 1)};
    layer.setAnimator(animator);

    TextLayerStyleUniform dynamicStyleUniforms[2];
    Vector4 dynamicStylePaddingsInvalid[3];

    std::ostringstream out;
    Error redirectError{&out};
    animator.advance(12_nsec, dynamicStyleUniforms, dynamicStylePaddingsInvalid, {});
    CORRADE_COMPARE(out.str(), "Whee::TextLayerStyleAnimator::advance(): expected dynamic style uniform and padding views to have the same size but got 2 and 3\n");
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
        explicit LayerShared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        using TextLayer::Shared::setGlyphCache;

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{3}
        .setDynamicStyleCount(1)
    };
    shared.setGlyphCache(cache);

    FontHandle fontHandle = shared.addFont(font, 1.0f);

    shared.setStyle(
        TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}
            .setColor(Color4{0.25f}),
         TextLayerStyleUniform{}
            .setColor(Color4{0.75f}),
         TextLayerStyleUniform{}},
        {fontHandle, fontHandle, fontHandle},
        {Text::Alignment::MiddleCenter,
         Text::Alignment::MiddleCenter,
         Text::Alignment::MiddleCenter},
        {{}, Vector4{data.padding}, {}});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}

        TextLayer::State& stateData() {
            return static_cast<TextLayer::State&>(*_state);
        }
    } layer{layerHandle(0, 1), shared};

    DataHandle data1 = layer.create(2, "", {});
    DataHandle data2 = layer.create(2, "", {});

    TextLayerStyleAnimator animator1{animatorHandle(0, 1)};
    TextLayerStyleAnimator animatorEmpty{animatorHandle(0, 1)};
    TextLayerStyleAnimator animator2{animatorHandle(0, 1)};
    layer.setAnimator(animator1);
    layer.setAnimator(animatorEmpty);
    layer.setAnimator(animator2);

    animator1.create(0, 1, Animation::Easing::linear, 0_nsec, 20_nsec, data2);
    animator2.create(1, 0, Animation::Easing::linear, 13_nsec, 1_nsec, data1);

    /* Advancing just the first animation to 1/4, which sets the style,
       uniform and optionally padding */
    layer.advanceAnimations(5_nsec, std::initializer_list<Containers::AnyReference<AbstractStyleAnimator>>{animator2, animatorEmpty, animator1});
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 1);
    CORRADE_COMPARE(layer.style(data2), shared.styleCount() + 0);
    CORRADE_COMPARE(layer.dynamicStyleUniforms()[0].color, Color4{0.375f});
    CORRADE_COMPARE(layer.dynamicStylePaddings()[0], Vector4{data.padding}*0.25f);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate);
    CORRADE_VERIFY(layer.stateData().dynamicStyleChanged);

    /* Advancing the first animation to 1/2, which sets just the uniform and
       optionally padding */
    layer.update(LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    layer.stateData().dynamicStyleChanged = false;
    layer.advanceAnimations(10_nsec, std::initializer_list<Containers::AnyReference<AbstractStyleAnimator>>{animator2, animatorEmpty, animator1});
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 1);
    CORRADE_COMPARE(layer.style(data2), shared.styleCount() + 0);
    CORRADE_COMPARE(layer.dynamicStyleUniforms()[0].color, Color4{0.5f});
    CORRADE_COMPARE(layer.dynamicStylePaddings()[0], Vector4{data.padding}*0.5f);
    CORRADE_COMPARE(layer.state(), (data.padding.isZero() ? LayerStates{} : LayerState::NeedsDataUpdate)|LayerState::NeedsCommonDataUpdate);
    CORRADE_VERIFY(layer.stateData().dynamicStyleChanged);

    /* Advancing both the first animation to 3/4 and second animation directly
       to the final style. It should thus set both the update and the style
       change. */
    layer.update(LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    layer.stateData().dynamicStyleChanged = false;
    layer.advanceAnimations(15_nsec, std::initializer_list<Containers::AnyReference<AbstractStyleAnimator>>{animator2, animatorEmpty, animator1});
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 1);
    CORRADE_COMPARE(layer.style(data1), 0);
    CORRADE_COMPARE(layer.style(data2), shared.styleCount() + 0);
    CORRADE_COMPARE(layer.dynamicStyleUniforms()[0].color, Color4{0.625f});
    CORRADE_COMPARE(layer.dynamicStylePaddings()[0], Vector4{data.padding}*0.75f);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate);
    CORRADE_VERIFY(layer.stateData().dynamicStyleChanged);

    /* Advancing the first animation to the end & the final style. Only the
       style data is updated, no uniforms or paddings. */
    layer.update(LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    layer.stateData().dynamicStyleChanged = false;
    layer.advanceAnimations(20_nsec, std::initializer_list<Containers::AnyReference<AbstractStyleAnimator>>{animator2, animatorEmpty, animator1});
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 0);
    CORRADE_COMPARE(layer.style(data2), 1);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
    CORRADE_VERIFY(!layer.stateData().dynamicStyleChanged);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Whee::Test::TextLayerStyleAnimatorTest)
