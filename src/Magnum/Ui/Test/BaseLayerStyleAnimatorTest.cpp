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
#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/Iterable.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/Reference.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/String.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Container.h>
#include <Corrade/TestSuite/Compare/String.h>
#include <Magnum/Animation/Easing.h>
#include <Magnum/Math/Time.h>

#include "Magnum/Ui/BaseLayer.h"
#include "Magnum/Ui/BaseLayerAnimator.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/Implementation/baseLayerState.h" /* for layerAdvance() */

namespace Magnum { namespace Ui { namespace Test { namespace {

struct BaseLayerStyleAnimatorTest: TestSuite::Tester {
    explicit BaseLayerStyleAnimatorTest();

    void debugAnimation();
    void debugAnimations();

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
    UnsignedInt uniform;
    Vector4 padding;
    BaseLayerStyleAnimations expected;
} AdvancePropertiesData[]{
    {"nothing changes", 1, Vector4{2.0f},
        BaseLayerStyleAnimation{}},
    {"uniform ID changes", 0, Vector4{2.0f},
        BaseLayerStyleAnimation::Uniform},
    /* Still reports uniform change because comparing all values is unnecessary
       complexity */
    {"uniform ID changes but data stay the same", 3, Vector4{2.0f},
        BaseLayerStyleAnimation::Uniform},
    {"padding changes", 1, Vector4{4.0f},
        BaseLayerStyleAnimation::Padding},
    {"uniform ID + padding changes", 0, Vector4{4.0f},
        BaseLayerStyleAnimation::Padding|BaseLayerStyleAnimation::Uniform},
};

const struct {
    const char* name;
    Vector4 padding;
} LayerAdvanceData[]{
    {"", {}},
    {"padding changes as well", Vector4{2.0f}}
};

BaseLayerStyleAnimatorTest::BaseLayerStyleAnimatorTest() {
    addTests({&BaseLayerStyleAnimatorTest::debugAnimation,
              &BaseLayerStyleAnimatorTest::debugAnimations,

              &BaseLayerStyleAnimatorTest::construct,
              &BaseLayerStyleAnimatorTest::constructCopy,
              &BaseLayerStyleAnimatorTest::constructMove,

              &BaseLayerStyleAnimatorTest::assignAnimator,
              &BaseLayerStyleAnimatorTest::setDefaultStyleAnimator,

              &BaseLayerStyleAnimatorTest::createRemove<UnsignedInt>,
              &BaseLayerStyleAnimatorTest::createRemove<Enum>,
              &BaseLayerStyleAnimatorTest::createRemoveHandleRecycle,
              &BaseLayerStyleAnimatorTest::createInvalid,
              &BaseLayerStyleAnimatorTest::propertiesInvalid,

              &BaseLayerStyleAnimatorTest::advance});

    addInstancedTests({&BaseLayerStyleAnimatorTest::advanceProperties},
        Containers::arraySize(AdvancePropertiesData));

    addTests({&BaseLayerStyleAnimatorTest::advanceNoFreeDynamicStyles,
              &BaseLayerStyleAnimatorTest::advanceEmpty,
              &BaseLayerStyleAnimatorTest::advanceInvalid});

    addInstancedTests({&BaseLayerStyleAnimatorTest::layerAdvance},
        Containers::arraySize(LayerAdvanceData));
}

void BaseLayerStyleAnimatorTest::debugAnimation() {
    Containers::String out;
    Debug{&out} << BaseLayerStyleAnimation::Style << BaseLayerStyleAnimation(0xbe);
    CORRADE_COMPARE(out, "Ui::BaseLayerStyleAnimation::Style Ui::BaseLayerStyleAnimation(0xbe)\n");
}

void BaseLayerStyleAnimatorTest::debugAnimations() {
    Containers::String out;
    Debug{&out} << (BaseLayerStyleAnimation::Uniform|BaseLayerStyleAnimation(0xe0)) << BaseLayerStyleAnimations{};
    CORRADE_COMPARE(out, "Ui::BaseLayerStyleAnimation::Uniform|Ui::BaseLayerStyleAnimation(0xe0) Ui::BaseLayerStyleAnimations{}\n");
}

void BaseLayerStyleAnimatorTest::construct() {
    BaseLayerStyleAnimator animator{animatorHandle(0xab, 0x12)};

    CORRADE_COMPARE(animator.features(), AnimatorFeature::DataAttachment);
    CORRADE_COMPARE(animator.handle(), animatorHandle(0xab, 0x12));
    /* The rest is the same as in AbstractAnimatorTest::constructStyle() */
}

void BaseLayerStyleAnimatorTest::constructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<BaseLayerStyleAnimator>{});
    CORRADE_VERIFY(!std::is_copy_assignable<BaseLayerStyleAnimator>{});
}

void BaseLayerStyleAnimatorTest::constructMove() {
    /* Just verify that the subclass doesn't have the moves broken */

    BaseLayerStyleAnimator a{animatorHandle(0xab, 0x12)};

    BaseLayerStyleAnimator b{Utility::move(a)};
    CORRADE_COMPARE(b.handle(), animatorHandle(0xab, 0x12));

    BaseLayerStyleAnimator c{animatorHandle(0xcd, 0x34)};
    c = Utility::move(b);
    CORRADE_COMPARE(c.handle(), animatorHandle(0xab, 0x12));

    CORRADE_VERIFY(std::is_nothrow_move_constructible<BaseLayerStyleAnimator>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<BaseLayerStyleAnimator>::value);
}

void BaseLayerStyleAnimatorTest::assignAnimator() {
    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{BaseLayer::Shared::Configuration{2}
        .setDynamicStyleCount(1)
    };

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    BaseLayerStyleAnimator animator{animatorHandle(0, 1)};
    CORRADE_COMPARE(animator.layer(), LayerHandle::Null);

    layer.assignAnimator(animator);
    CORRADE_COMPARE(animator.layer(), layer.handle());
}

void BaseLayerStyleAnimatorTest::setDefaultStyleAnimator() {
    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{BaseLayer::Shared::Configuration{2}
        .setDynamicStyleCount(1)
    };

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    BaseLayerStyleAnimator animator{animatorHandle(0, 1)};
    layer.assignAnimator(animator);
    CORRADE_COMPARE(animator.layer(), layer.handle());
    CORRADE_COMPARE(layer.defaultStyleAnimator(), nullptr);

    layer.setDefaultStyleAnimator(&animator);
    CORRADE_COMPARE(layer.defaultStyleAnimator(), &animator);
}

template<class T> void BaseLayerStyleAnimatorTest::createRemove() {
    setTestCaseTemplateName(std::is_same<T, Enum>::value ? "Enum" : "UnsignedInt");

    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{BaseLayer::Shared::Configuration{5, 3}
        .setDynamicStyleCount(1)
    };

    /* Have more uniforms that are sparsely indexed into to verify the data get
       correctly fetched */
    shared.setStyle(BaseLayerCommonStyleUniform{},
        {BaseLayerStyleUniform{},               /* 0 */
         BaseLayerStyleUniform{}                /* 1 */
            .setColor(0xff3366_rgbf),
         BaseLayerStyleUniform{}                /* 2 */
            .setOutlineWidth(Vector4{4.0f}),
         BaseLayerStyleUniform{},               /* 3 */
         BaseLayerStyleUniform{}                /* 4 */
            .setColor(0x9933ff_rgbf)
            .setOutlineWidth(Vector4{2.0f})},
        {4, 1, 2},
        {Vector4{1.0f},
         {2.0f, 3.0f, 4.0f, 5.0f},
         {}});

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    BaseLayerStyleAnimator animator{animatorHandle(0, 1)};
    layer.assignAnimator(animator);

    /* The style used for the actual data shouldn't affect anything */
    DataHandle data1 = layer.create(1);
    DataHandle data2 = layer.create(2);
    DataHandle data3 = layer.create(0);

    /* The base overload. It shouldn't cause the data style to be changed to
       anything. */
    AnimationHandle first = animator.create(T(0), T(1), Animation::Easing::linear, 12_nsec, 13_nsec, data2, 10, AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.duration(first), 13_nsec);
    CORRADE_COMPARE(animator.repeatCount(first), 10);
    CORRADE_COMPARE(animator.flags(first), AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.started(first), 12_nsec);
    CORRADE_COMPARE(animator.data(first), data2);
    CORRADE_COMPARE(animator.targetStyle(first), 1);
    /* Can't use T, as the function restricts to enum types which would fail
       for T == UnsignedInt */
    CORRADE_COMPARE(animator.template targetStyle<Enum>(first), Enum(1));
    CORRADE_COMPARE(animator.dynamicStyle(first), Containers::NullOpt);
    /* Styles 0 and 1 are uniforms 4 and 1 */
    CORRADE_COMPARE(animator.uniforms(first).first().topColor, 0x9933ff_rgbf);
    CORRADE_COMPARE(animator.uniforms(first).second().topColor, 0xff3366_rgbf);
    CORRADE_COMPARE(animator.uniforms(first).first().outlineWidth, Vector4{2.0f});
    CORRADE_COMPARE(animator.uniforms(first).second().outlineWidth, Vector4{0.0f});
    CORRADE_COMPARE(animator.paddings(first), Containers::pair(Vector4{1.0f}, Vector4{2.0f, 3.0f, 4.0f, 5.0f}));
    CORRADE_COMPARE(animator.easing(first), Animation::Easing::linear);
    /* Dynamic style is only allocated and switched to during advance() */
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 0);
    CORRADE_COMPARE(layer.style(data2), 2);

    /* Implicit repeat count, no data attachment (which thus shouldn't try to
       access anything data-related in the layer) */
    AnimationHandle second = animator.create(T(2), T(0), Animation::Easing::cubicIn, -15_nsec, 1_nsec, DataHandle::Null, AnimationFlag(0x40));
    CORRADE_COMPARE(animator.duration(second), 1_nsec);
    CORRADE_COMPARE(animator.repeatCount(second), 1);
    CORRADE_COMPARE(animator.flags(second), AnimationFlag(0x40));
    CORRADE_COMPARE(animator.started(second), -15_nsec);
    CORRADE_COMPARE(animator.data(second), DataHandle::Null);
    CORRADE_COMPARE(animator.targetStyle(second), 0);
    /* Can't use T, as the function restricts to enum types which would fail
       for T == UnsignedInt */
    CORRADE_COMPARE(animator.template targetStyle<Enum>(second), Enum(0));
    CORRADE_COMPARE(animator.dynamicStyle(second), Containers::NullOpt);
    /* Styles 2 and 0 are uniforms 2 and 4 */
    CORRADE_COMPARE(animator.uniforms(second).first().topColor, 0xffffff_rgbf);
    CORRADE_COMPARE(animator.uniforms(second).second().topColor, 0x9933ff_rgbf);
    CORRADE_COMPARE(animator.uniforms(second).first().outlineWidth, Vector4{4.0f});
    CORRADE_COMPARE(animator.uniforms(second).second().outlineWidth, Vector4{2.0f});
    CORRADE_COMPARE(animator.paddings(second), Containers::pair(Vector4{0.0f}, Vector4{1.0f}));
    CORRADE_COMPARE(animator.easing(second), Animation::Easing::cubicIn);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 0);

    /* LayerDataHandle overload, verify also with AnimatorDataHandle */
    AnimationHandle third = animator.create(T(1), T(2), Animation::Easing::bounceInOut, 0_nsec, 100_nsec, dataHandleData(data3), 0, AnimationFlag(0x80));
    CORRADE_COMPARE(animator.duration(animationHandleData(third)), 100_nsec);
    CORRADE_COMPARE(animator.repeatCount(animationHandleData(third)), 0);
    CORRADE_COMPARE(animator.flags(animationHandleData(third)), AnimationFlag(0x80));
    CORRADE_COMPARE(animator.started(animationHandleData(third)), 0_nsec);
    CORRADE_COMPARE(animator.data(animationHandleData(third)), data3);
    CORRADE_COMPARE(animator.targetStyle(animationHandleData(third)), 2);
    /* Can't use T, as the function restricts to enum types which would fail
       for T == UnsignedInt */
    CORRADE_COMPARE(animator.template targetStyle<Enum>(animationHandleData(third)), Enum(2));
    CORRADE_COMPARE(animator.dynamicStyle(animationHandleData(third)), Containers::NullOpt);
    /* Styles 1 and 2 are uniforms 1 and 2 */
    CORRADE_COMPARE(animator.uniforms(animationHandleData(third)).first().topColor, 0xff3366_rgbf);
    CORRADE_COMPARE(animator.uniforms(animationHandleData(third)).second().topColor, 0xffffff_rgbf);
    CORRADE_COMPARE(animator.uniforms(animationHandleData(third)).first().outlineWidth, Vector4{0.0f});
    CORRADE_COMPARE(animator.uniforms(animationHandleData(third)).second().outlineWidth, Vector4{4.0f});
    CORRADE_COMPARE(animator.paddings(animationHandleData(third)), Containers::pair(Vector4{2.0f, 3.0f, 4.0f, 5.0f}, Vector4{0.0f}));
    CORRADE_COMPARE(animator.easing(animationHandleData(third)), Animation::Easing::bounceInOut);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 0);
    CORRADE_COMPARE(layer.style(data3), 0);

    /* LayerDataHandle overload with implicit repeat count */
    AnimationHandle fourth = animator.create(T(0), T(2), Animation::Easing::smoothstep, 20_nsec, 10_nsec, dataHandleData(data1), AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.duration(fourth), 10_nsec);
    CORRADE_COMPARE(animator.repeatCount(fourth), 1);
    CORRADE_COMPARE(animator.flags(fourth), AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.started(fourth), 20_nsec);
    CORRADE_COMPARE(animator.data(fourth), data1);
    CORRADE_COMPARE(animator.targetStyle(fourth), 2u);
    /* Can't use T, as the function restricts to enum types which would fail
       for T == UnsignedInt */
    CORRADE_COMPARE(animator.template targetStyle<Enum>(fourth), Enum(2));
    CORRADE_COMPARE(animator.dynamicStyle(fourth), Containers::NullOpt);
    /* Styles 0 and 2 are uniforms 4 and 2 */
    CORRADE_COMPARE(animator.uniforms(fourth).first().topColor, 0x9933ff_rgbf);
    CORRADE_COMPARE(animator.uniforms(fourth).second().topColor, 0xffffff_rgbf);
    CORRADE_COMPARE(animator.uniforms(fourth).first().outlineWidth, Vector4{2.0f});
    CORRADE_COMPARE(animator.uniforms(fourth).second().outlineWidth, Vector4{4.0f});
    CORRADE_COMPARE(animator.paddings(fourth), Containers::pair(Vector4{1.0f}, Vector4{0.0f}));
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

void BaseLayerStyleAnimatorTest::createRemoveHandleRecycle() {
    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{BaseLayer::Shared::Configuration{2}
        .setDynamicStyleCount(1)
    };

    shared.setStyle(BaseLayerCommonStyleUniform{},
        {BaseLayerStyleUniform{}
            .setColor(0xff3366_rgbf),
         BaseLayerStyleUniform{}
            .setColor(0x9933ff_rgbf)},
        {Vector4{1.0f},
         Vector4{2.0f}});

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    BaseLayerStyleAnimator animator{animatorHandle(0, 1)};
    layer.assignAnimator(animator);

    DataHandle data = layer.create(1);

    /* Allocate an animation */
    AnimationHandle first = animator.create(0, 1, Animation::Easing::linear, 0_nsec, 13_nsec, data);
    CORRADE_COMPARE(animator.targetStyle(first), 1u);
    CORRADE_COMPARE(animator.dynamicStyle(first), Containers::NullOpt);
    CORRADE_COMPARE(animator.uniforms(first).first().topColor, 0xff3366_rgbf);
    CORRADE_COMPARE(animator.uniforms(first).second().topColor, 0x9933ff_rgbf);
    CORRADE_COMPARE(animator.paddings(first), Containers::pair(Vector4{1.0f}, Vector4{2.0f}));
    CORRADE_COMPARE(animator.easing(first), Animation::Easing::linear);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 0);

    /* Let it advance to allocate the dynamic style */
    Containers::BitArray active{DirectInit, 1, true};
    Float factors[]{0.0f};
    Containers::BitArray remove{DirectInit, 1, false};
    BaseLayerStyleUniform dynamicStyleUniforms[1];
    Vector4 dynamicStylePaddings[1];
    UnsignedInt dataStyles[1];
    animator.advance(
        active,
        factors,
        remove,
        dynamicStyleUniforms, dynamicStylePaddings, dataStyles);
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
    CORRADE_COMPARE(animator.uniforms(first2).first().topColor, 0x9933ff_rgbf);
    CORRADE_COMPARE(animator.uniforms(first2).second().topColor, 0xff3366_rgbf);
    CORRADE_COMPARE(animator.paddings(first2), Containers::pair(Vector4{2.0f}, Vector4{1.0f}));
    CORRADE_COMPARE(animator.easing(first2), Animation::Easing::bounceInOut);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 0);
}

void BaseLayerStyleAnimatorTest::createInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } sharedNoStyleSet{BaseLayer::Shared::Configuration{5}
        .setDynamicStyleCount(1)},
      shared{BaseLayer::Shared::Configuration{1, 5}
        .setDynamicStyleCount(1)};

    shared.setStyle(
        BaseLayerCommonStyleUniform{},
        {BaseLayerStyleUniform{}},
        {0, 0, 0, 0, 0},
        {});

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    } layerNoStyleSet{layerHandle(0, 1), sharedNoStyleSet},
      layer{layerHandle(0, 1), shared};

    BaseLayerStyleAnimator animatorNoLayerSet{animatorHandle(0, 1)};

    BaseLayerStyleAnimator animatorNoLayerStyleSet{animatorHandle(0, 1)};
    layerNoStyleSet.assignAnimator(animatorNoLayerStyleSet);

    BaseLayerStyleAnimator animator{animatorHandle(0, 1)};
    layer.assignAnimator(animator);

    Containers::String out;
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
    CORRADE_COMPARE_AS(out,
        "Ui::BaseLayerStyleAnimator::create(): no layer set\n"
        "Ui::BaseLayerStyleAnimator::create(): no layer set\n"
        "Ui::BaseLayerStyleAnimator::create(): no layer set\n"
        "Ui::BaseLayerStyleAnimator::create(): no layer set\n"
        "Ui::BaseLayerStyleAnimator::create(): no style data was set on the layer\n"
        "Ui::BaseLayerStyleAnimator::create(): expected source and target style to be in range for 5 styles but got 0 and 5\n"
        "Ui::BaseLayerStyleAnimator::create(): expected source and target style to be in range for 5 styles but got 5 and 0\n"
        "Ui::BaseLayerStyleAnimator::create(): easing is null\n",
        TestSuite::Compare::String);
}

void BaseLayerStyleAnimatorTest::propertiesInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{BaseLayer::Shared::Configuration{2}
        .setDynamicStyleCount(1)
    };
    shared.setStyle(
        BaseLayerCommonStyleUniform{},
        {BaseLayerStyleUniform{}, BaseLayerStyleUniform{}},
        {});

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    BaseLayerStyleAnimator animator{animatorHandle(0, 1)};
    layer.assignAnimator(animator);

    AnimationHandle handle = animator.create(0, 1, Animation::Easing::linear, 12_nsec, 13_nsec, DataHandle::Null);

    Containers::String out;
    Error redirectError{&out};
    animator.uniforms(AnimationHandle::Null);
    animator.paddings(AnimationHandle::Null);
    animator.easing(AnimationHandle::Null);
    /* Valid animator, invalid data */
    animator.uniforms(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)));
    animator.paddings(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)));
    animator.easing(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)));
    /* Invalid animator, valid data */
    animator.uniforms(animationHandle(AnimatorHandle::Null, animationHandleData(handle)));
    animator.paddings(animationHandle(AnimatorHandle::Null, animationHandleData(handle)));
    animator.easing(animationHandle(AnimatorHandle::Null, animationHandleData(handle)));
    /* AnimatorDataHandle directly */
    animator.uniforms(AnimatorDataHandle(0x123abcde));
    animator.paddings(AnimatorDataHandle(0x123abcde));
    animator.easing(AnimatorDataHandle(0x123abcde));
    CORRADE_COMPARE_AS(out,
        "Ui::BaseLayerStyleAnimator::uniforms(): invalid handle Ui::AnimationHandle::Null\n"
        "Ui::BaseLayerStyleAnimator::paddings(): invalid handle Ui::AnimationHandle::Null\n"
        "Ui::BaseLayerStyleAnimator::easing(): invalid handle Ui::AnimationHandle::Null\n"

        "Ui::BaseLayerStyleAnimator::uniforms(): invalid handle Ui::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Ui::BaseLayerStyleAnimator::paddings(): invalid handle Ui::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Ui::BaseLayerStyleAnimator::easing(): invalid handle Ui::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"

        "Ui::BaseLayerStyleAnimator::uniforms(): invalid handle Ui::AnimationHandle(Null, {0x0, 0x1})\n"
        "Ui::BaseLayerStyleAnimator::paddings(): invalid handle Ui::AnimationHandle(Null, {0x0, 0x1})\n"
        "Ui::BaseLayerStyleAnimator::easing(): invalid handle Ui::AnimationHandle(Null, {0x0, 0x1})\n"

        "Ui::BaseLayerStyleAnimator::uniforms(): invalid handle Ui::AnimatorDataHandle(0xabcde, 0x123)\n"
        "Ui::BaseLayerStyleAnimator::paddings(): invalid handle Ui::AnimatorDataHandle(0xabcde, 0x123)\n"
        "Ui::BaseLayerStyleAnimator::easing(): invalid handle Ui::AnimatorDataHandle(0xabcde, 0x123)\n",
        TestSuite::Compare::String);
}

void BaseLayerStyleAnimatorTest::advance() {
    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{BaseLayer::Shared::Configuration{4, 7}
        .setDynamicStyleCount(3)
    };
    shared.setStyle(
        BaseLayerCommonStyleUniform{},
        /* Assuming the lerp works component-wise, just set them to mutually
           exclusive ranges to verify that correct values get interpolated */
        {BaseLayerStyleUniform{}    /* 0, used by style 6. All zeros. */
            .setColor(Color4{0.0f})
            .setOutlineColor(Color4{0.0f}),
         BaseLayerStyleUniform{}    /* 1, used by style 3 */
            .setColor(Color4{0.0f}, Color4{1.0f})
            .setOutlineColor(Color4{4.0f})
            .setOutlineWidth(Vector4{32.0f})
            .setCornerRadius(Vector4{8.0f})
            .setInnerOutlineCornerRadius(Vector4{16.0f}),
         BaseLayerStyleUniform{}    /* 2, used by style 1 */
            .setColor(Color4{2.0f}, Color4{0.5f})
            .setOutlineColor(Color4{8.0f})
            .setOutlineWidth(Vector4{16.0f})
            .setCornerRadius(Vector4{12.0f})
            .setInnerOutlineCornerRadius(Vector4{24.0f}),
         BaseLayerStyleUniform{}},  /* 3, not used for animation */
        {3, 2, 3, 1, 3, 3, 0},
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

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    /* Assign data to styles that aren't used for animation */
    DataHandle data0 = layer.create(4);
    DataHandle data1 = layer.create(0);
    DataHandle data2 = layer.create(2);
    DataHandle data3 = layer.create(4);
    DataHandle data4 = layer.create(5);

    BaseLayerStyleAnimator animator{animatorHandle(0, 1)};
    layer.assignAnimator(animator);

    /* This one allocates a dynamic style, interpolates between uniforms 1 and
       2 with just Uniform set and when stopped sets the data2 style to 1 */
    AnimationHandle playing = animator.create(3, 1, Animation::Easing::linear, 0_nsec, 20_nsec, data2);
    /* This one sets the data4 style to 3 and is removed without even
       allocating a dynamic style or marking Uniform or Padding as changed */
    AnimationHandle stopped = animator.create(1, 3, Animation::Easing::cubicOut, 0_nsec, 1_nsec, data4);
    /* This one is a reverse of the first, scheduled later and not attached to
       any data, thus it never marks Style as changed */
    AnimationHandle scheduledNullData = animator.create(1, 3, Animation::Easing::linear, 15_nsec, 10_nsec, DataHandle::Null);
    /* This one allocates a dynamic style once started, interpolates all the
       way to 3 and stays */
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

    /* Does what layer's advanceAnimations() is doing internally for all
       animators (as we need to test also the interaction with animation being
       removed, etc.), but with an ability to peek into the filled data to
       verify they're written only when they should be. Layer's
       advanceAnimations() is then tested in layerAdvance() below. */
    const auto advance = [&](Nanoseconds time, Containers::ArrayView<BaseLayerStyleUniform> dynamicStyleUniforms, const Containers::StridedArrayView1D<Vector4>& dynamicStylePaddings, const Containers::StridedArrayView1D<UnsignedInt>& dataStyles) {
        UnsignedByte activeData[1];
        Containers::MutableBitArrayView active{activeData, 0, 5};
        UnsignedByte startedData[1];
        Containers::MutableBitArrayView started{startedData, 0, 5};
        UnsignedByte stoppedData[1];
        Containers::MutableBitArrayView stopped{stoppedData, 0, 5};
        Float factors[5];
        UnsignedByte removeData[1];
        Containers::MutableBitArrayView remove{removeData, 0, 5};

        Containers::Pair<bool, bool> needsAdvanceClean = animator.update(time, active, started, stopped, factors, remove);
        BaseLayerStyleAnimations animations;
        if(needsAdvanceClean.first())
            animations = animator.advance(active, factors, remove, dynamicStyleUniforms, dynamicStylePaddings, dataStyles);
        if(needsAdvanceClean.second())
            animator.clean(remove);
        return animations;
    };

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
        BaseLayerStyleUniform uniforms[3];
        UnsignedInt dataStyles[]{666, 666, 666, 666, 666};
        CORRADE_COMPARE(advance(5_nsec, uniforms, paddings, dataStyles), BaseLayerStyleAnimation::Uniform|BaseLayerStyleAnimation::Style|BaseLayerStyleAnimation::Padding);
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
        CORRADE_COMPARE(layer.dynamicStyleAnimation(0), playing);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(1), stoppedKept);
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
        CORRADE_COMPARE(uniforms[0].topColor, Color4{0.5f});
        CORRADE_COMPARE(uniforms[0].bottomColor, Color4{0.875f});
        CORRADE_COMPARE(uniforms[0].outlineColor, Color4{5.0f});
        CORRADE_COMPARE(uniforms[0].outlineWidth, Vector4{28.0f});
        CORRADE_COMPARE(uniforms[0].cornerRadius, Vector4{9.0f});
        CORRADE_COMPARE(uniforms[0].innerOutlineCornerRadius, Vector4{18.0f});
        CORRADE_COMPARE(paddings[0], Vector4{2.0f});
        /* The stopped but kept style should get exactly the uniform 1 value,
           and the constant padding */
        CORRADE_COMPARE(uniforms[1].topColor, Color4{0.0f});
        CORRADE_COMPARE(uniforms[1].bottomColor, Color4{1.0f});
        CORRADE_COMPARE(uniforms[1].outlineColor, Color4{4.0f});
        CORRADE_COMPARE(uniforms[1].outlineWidth, Vector4{32.0f});
        CORRADE_COMPARE(uniforms[1].cornerRadius, Vector4{8.0f});
        CORRADE_COMPARE(uniforms[1].innerOutlineCornerRadius, Vector4{16.0f});
        CORRADE_COMPARE(paddings[1], Vector4{2.0f});
    }

    /* Reset the padding of the stopped & kept style to something else to
       verify it doesn't get touched anymore */
    paddings[1] = {};

    /* Advancing to 10 changes just the uniform to 1/2, nothing else. In
       particular, the style values aren't touched even though they're now
       different. */
    {
        BaseLayerStyleUniform uniforms[3];
        UnsignedInt dataStyles[]{666, 666, 666, 666, 666};
        CORRADE_COMPARE(advance(10_nsec, uniforms, paddings, dataStyles), BaseLayerStyleAnimation::Uniform);
        CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 2);
        CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
            666u, 666u, 666u, 666u, 666u,
        }), TestSuite::Compare::Container);
        /* Testing just a subset, assuming the rest is updated accordingly */
        CORRADE_COMPARE(uniforms[0].topColor, Color4{1.0f});
        CORRADE_COMPARE(uniforms[0].innerOutlineCornerRadius, Vector4{20.0f});
        CORRADE_COMPARE(paddings[0], Vector4{2.0f});
        /* The stopped & kept style isn't touched anymore, staying at the reset
           defaults */
        CORRADE_COMPARE(uniforms[1].topColor, Color4{1.0f});
        CORRADE_COMPARE(paddings[1], Vector4{0.0f});
    }

    /* Advancing to 15 plays the also scheduled animation without a data
       attachment, allocating a new dynamic style but not switching to it.
       I.e., no Style is set, only Uniform and Padding. */
    {
        BaseLayerStyleUniform uniforms[3];
        UnsignedInt dataStyles[]{666, 666, 666, 666, 666};
        CORRADE_COMPARE(advance(15_nsec, uniforms, paddings, dataStyles), BaseLayerStyleAnimation::Uniform|BaseLayerStyleAnimation::Padding);
        CORRADE_COMPARE(animator.state(scheduledNullData), AnimationState::Playing);
        CORRADE_COMPARE(animator.dynamicStyle(scheduledNullData), 2);
        CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 3);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(2), scheduledNullData);
        CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
            666u, 666u, 666u, 666u, 666u,
        }), TestSuite::Compare::Container);
        /* The playing animation is advanced to 3/4 */
        CORRADE_COMPARE(uniforms[0].topColor, Color4{1.5f});
        CORRADE_COMPARE(paddings[0], Vector4{2.0f});
        /* The stopped & kept style isn't touched anymore, staying at the reset
           defaults */
        CORRADE_COMPARE(uniforms[1].topColor, Color4{1.0f});
        CORRADE_COMPARE(paddings[1], Vector4{0.0f});
        /* The null data animation is set to the value of style 1 */
        CORRADE_COMPARE(uniforms[2].topColor, Color4{2.0f});
        CORRADE_COMPARE(paddings[2], Vector4{2.0f});
    }

    /* Advancing to 20 stops the first animation, recycling its dynamic style
       and changing the style to the target one. Uniform value is updated for
       the null data animation. */
    {
        BaseLayerStyleUniform uniforms[3];
        UnsignedInt dataStyles[]{666, 666, 666, 666, 666};
        CORRADE_COMPARE(advance(20_nsec, uniforms, paddings, dataStyles), BaseLayerStyleAnimation::Style|BaseLayerStyleAnimation::Uniform);
        CORRADE_VERIFY(!animator.isHandleValid(playing));
        CORRADE_VERIFY(animator.isHandleValid(stoppedKept));
        CORRADE_VERIFY(animator.isHandleValid(scheduledChangesPadding));
        CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 2);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(0), AnimationHandle::Null);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(1), stoppedKept);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(2), scheduledNullData);
        CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
            666u,
            666u,
            1u,
            666u,
            666u,
        }), TestSuite::Compare::Container);
        /* Uniform values of the recycled style aren't touched anymore */
        CORRADE_COMPARE(uniforms[0].topColor, Color4{1.0f});
        /* The stopped & kept style isn't touched anymore, staying at the reset
           defaults */
        CORRADE_COMPARE(uniforms[1].topColor, Color4{1.0f});
        CORRADE_COMPARE(paddings[1], Vector4{0.0f});
        /* The null data animation is advanced to 1/2 between style 1 and 3 */
        CORRADE_COMPARE(uniforms[2].topColor, Color4{1.0f});
        CORRADE_COMPARE(paddings[2], Vector4{2.0f});
    }

    /* Advancing to 25 stops the null data animation, recycling its dynamic
       style. Leads to no other change, i.e. no Style set. */
    {
        BaseLayerStyleUniform uniforms[3];
        UnsignedInt dataStyles[]{666, 666, 666, 666, 666};
        CORRADE_COMPARE(advance(25_nsec, uniforms, paddings, dataStyles), BaseLayerStyleAnimations{});
        CORRADE_VERIFY(!animator.isHandleValid(scheduledNullData));
        CORRADE_VERIFY(animator.isHandleValid(stoppedKept));
        CORRADE_VERIFY(animator.isHandleValid(scheduledChangesPadding));
        CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 1);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(0), AnimationHandle::Null);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(1), stoppedKept);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(2), AnimationHandle::Null);
        CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
            666u, 666u, 666u, 666u, 666u,
        }), TestSuite::Compare::Container);
        /* Uniform values of the recycled styles aren't touched anymore */
        CORRADE_COMPARE(uniforms[0].topColor, Color4{1.0f});
        CORRADE_COMPARE(uniforms[2].topColor, Color4{1.0f});
        /* The stopped & kept style isn't touched anymore, staying at the reset
           defaults */
        CORRADE_COMPARE(uniforms[1].topColor, Color4{1.0f});
        CORRADE_COMPARE(paddings[1], Vector4{0.0f});
    }

    /* Advancing to 35 plays the scheduled animation, allocating a new dynamic
       style and switching to it */
    {
        BaseLayerStyleUniform uniforms[3];
        UnsignedInt dataStyles[]{666, 666, 666, 666, 666};
        CORRADE_COMPARE(advance(35_nsec, uniforms, paddings, dataStyles), BaseLayerStyleAnimation::Uniform|BaseLayerStyleAnimation::Style|BaseLayerStyleAnimation::Padding);
        CORRADE_VERIFY(animator.isHandleValid(stoppedKept));
        CORRADE_VERIFY(animator.isHandleValid(scheduledChangesPadding));
        CORRADE_COMPARE(animator.state(stoppedKept), AnimationState::Stopped);
        CORRADE_COMPARE(animator.state(scheduledChangesPadding), AnimationState::Playing);
        CORRADE_COMPARE(animator.dynamicStyle(scheduledChangesPadding), 0);
        CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 2);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(0), scheduledChangesPadding);
        CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
            666u,
            666u,
            666u,
            shared.styleCount() + 0u,
            666u
        }), TestSuite::Compare::Container);
        /* The first dynamic style should get a 3/4 interpolation (i.e.,
           reverted from 1/4) of uniforms 1 and 0 and padding 3 and 6 */
        CORRADE_COMPARE(uniforms[0].topColor, Color4{0.0f});
        CORRADE_COMPARE(paddings[0], Vector4{3.5f});
        /* The stopped & kept style isn't touched anymore, staying at the reset
           defaults */
        CORRADE_COMPARE(uniforms[1].topColor, Color4{1.0f});
        CORRADE_COMPARE(paddings[1], Vector4{0.0f});
    }

    /* Advancing to 45 advances the scheduled animation, changing both the
       uniform and the padding. No styles. */
    {
        BaseLayerStyleUniform uniforms[3];
        UnsignedInt dataStyles[]{666, 666, 666, 666, 666};
        CORRADE_COMPARE(advance(45_nsec, uniforms, paddings, dataStyles), BaseLayerStyleAnimation::Uniform|BaseLayerStyleAnimation::Padding);
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
        CORRADE_COMPARE(uniforms[0].topColor, Color4{0.0f});
        CORRADE_COMPARE(paddings[0], Vector4{2.5f});
        /* The stopped & kept style isn't touched anymore, staying at the reset
           defaults */
        CORRADE_COMPARE(uniforms[1].topColor, Color4{1.0f});
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
        BaseLayerStyleUniform uniforms[3];
        UnsignedInt dataStyles[]{666, 666, 666, 666, 666};
        animator.stop(scheduledChangesPadding, 46_nsec);
        CORRADE_COMPARE(advance(47_nsec, uniforms, paddings, dataStyles), BaseLayerStyleAnimation::Style);
        CORRADE_VERIFY(!animator.isHandleValid(scheduledChangesPadding));
        CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 0);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(0), AnimationHandle::Null);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(1), AnimationHandle::Null);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(2), AnimationHandle::Null);
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

void BaseLayerStyleAnimatorTest::advanceProperties() {
    auto&& data = AdvancePropertiesData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{BaseLayer::Shared::Configuration{4, 3}
        .setDynamicStyleCount(3)
    };

    Float uniformColors[]{
        4.0f, 2.0f, 0.0f, 2.0f
    };
    shared.setStyle(
        BaseLayerCommonStyleUniform{},
        {BaseLayerStyleUniform{}
            .setColor(Color4{uniformColors[0]}),
         BaseLayerStyleUniform{}
            .setColor(Color4{uniformColors[1]}),
         BaseLayerStyleUniform{}
            .setColor(Color4{uniformColors[2]}),
         BaseLayerStyleUniform{} /* same data as uniform 1, different index */
            .setColor(Color4{uniformColors[3]})},
        {data.uniform, 2, 1},
        {data.padding,
         Vector4{4.0f},
         Vector4{2.0f}});

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    /* Assign data to a style that isn't used for animation */
    DataHandle layerData = layer.create(1);

    BaseLayerStyleAnimator animator{animatorHandle(0, 1)};
    layer.assignAnimator(animator);

    AnimationHandle animation = animator.create(2, 0, Animation::Easing::linear, 0_nsec, 20_nsec, layerData);

    /* Does what layer's advanceAnimations() is doing internally for all
       animators (as we need to test also the interaction with animation being
       removed, etc.), but with an ability to peek into the filled data to
       verify they're written only when they should be. Compared to the helper
       in advance() above it's not exposing all data. */
    const auto advance = [&](Nanoseconds time, Containers::ArrayView<BaseLayerStyleUniform> dynamicStyleUniforms, const Containers::StridedArrayView1D<Vector4>& dynamicStylePaddings, const Containers::StridedArrayView1D<UnsignedInt>& dataStyles) {
        UnsignedByte activeData[1];
        Containers::MutableBitArrayView active{activeData, 0, 1};
        UnsignedByte startedData[1];
        Containers::MutableBitArrayView started{startedData, 0, 1};
        UnsignedByte stoppedData[1];
        Containers::MutableBitArrayView stopped{stoppedData, 0, 1};
        Float factors[1];
        UnsignedByte removeData[1];
        Containers::MutableBitArrayView remove{removeData, 0, 1};

        Containers::Pair<bool, bool> needsAdvanceClean = animator.update(time, active, started, stopped, factors, remove);
        BaseLayerStyleAnimations animations;
        if(needsAdvanceClean.first()) {
            animations = animator.advance(
                active, factors, remove, dynamicStyleUniforms,
                dynamicStylePaddings, dataStyles);
        }
        if(needsAdvanceClean.second())
            animator.clean(remove);
        return animations;
    };

    /* The padding resulting from the animation gets checked against these.
       Contrary to the advance() test case, set it to the initial padding value
       so the initial advance doesn't report padding as changed. */
    Vector4 paddings[]{
        Vector4{2.0f}
    };

    /* Advancing to 5 allocates a dynamic style, switches to it and fills the
       dynamic data. The Uniform is reported together with Style always in
       order to ensure the dynamic uniform is uploaded even though it won't
       subsequently change. */
    {
        BaseLayerStyleUniform uniforms[1];
        UnsignedInt dataStyles[]{666};
        CORRADE_COMPARE(advance(5_nsec, uniforms, paddings, dataStyles), BaseLayerStyleAnimation::Uniform|BaseLayerStyleAnimation::Style|data.expected);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Playing);
        CORRADE_COMPARE(animator.dynamicStyle(animation), 0);
        CORRADE_COMPARE(uniforms[0].topColor, Math::lerp(Color4{2.0f}, Color4{uniformColors[data.uniform]}, 0.25f));
        CORRADE_COMPARE(paddings[0], Math::lerp(Vector4{2.0f}, data.padding, 0.25f));
        CORRADE_COMPARE(dataStyles[0], 3);

    /* Advancing to 15 changes only what's expected */
    } {
        BaseLayerStyleUniform uniforms[1];
        UnsignedInt dataStyles[]{666};
        CORRADE_COMPARE(advance(15_nsec, uniforms, paddings, dataStyles), data.expected);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Playing);
        CORRADE_COMPARE(animator.dynamicStyle(animation), 0);
        CORRADE_COMPARE(uniforms[0].topColor, Math::lerp(Color4{2.0f}, Color4{uniformColors[data.uniform]}, 0.75f));
        CORRADE_COMPARE(paddings[0], Math::lerp(Vector4{2.0f}, data.padding, 0.75f));
        CORRADE_COMPARE(dataStyles[0], 666);

    /* Advancing to 25 changes only the Style, the dynamic style values are
       unused now */
    } {
        BaseLayerStyleUniform uniforms[1];
        UnsignedInt dataStyles[]{666};
        CORRADE_COMPARE(advance(25_nsec, uniforms, paddings, dataStyles), BaseLayerStyleAnimation::Style);
        CORRADE_VERIFY(!animator.isHandleValid(animation));
        CORRADE_COMPARE(dataStyles[0], 0);
    }
}

void BaseLayerStyleAnimatorTest::advanceNoFreeDynamicStyles() {
    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{BaseLayer::Shared::Configuration{3}
        .setDynamicStyleCount(1)
    };
    shared.setStyle(
        BaseLayerCommonStyleUniform{},
        {BaseLayerStyleUniform{}
            .setColor(Color4{0.25f}),
         BaseLayerStyleUniform{}
            .setColor(Color4{0.75f}),
         BaseLayerStyleUniform{}},
        {});

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    BaseLayerStyleAnimator animator{animatorHandle(0, 1)};
    layer.assignAnimator(animator);

    DataHandle data1 = layer.create(2);
    DataHandle data2 = layer.create(2);

    AnimationHandle first = animator.create(0, 1, Animation::Easing::linear, 0_nsec, 20_nsec, data2);
    AnimationHandle second = animator.create(1, 0, Animation::Easing::linear, 10_nsec, 20_nsec, data1);

    /* Does what layer's advanceAnimations() is doing internally for all
       animators (as we need to test also the interaction with animation being
       removed, etc.), but with an ability to peek into the filled data to
       verify they're written only when they should be. Compared to the helper
       in advance() above it exposes only some data. */
    const auto advance = [&](Nanoseconds time, Containers::ArrayView<BaseLayerStyleUniform> dynamicStyleUniforms, const Containers::StridedArrayView1D<UnsignedInt>& dataStyles) {
        UnsignedByte activeData[1];
        Containers::MutableBitArrayView active{activeData, 0, 2};
        UnsignedByte startedData[1];
        Containers::MutableBitArrayView started{startedData, 0, 2};
        UnsignedByte stoppedData[1];
        Containers::MutableBitArrayView stopped{stoppedData, 0, 2};
        Float factors[2];
        UnsignedByte removeData[1];
        Containers::MutableBitArrayView remove{removeData, 0, 2};

        Containers::Pair<bool, bool> needsAdvanceClean = animator.update(time, active, started, stopped, factors, remove);
        BaseLayerStyleAnimations animations;
        if(needsAdvanceClean.first()) {
            Vector4 paddings[1];
            animations = animator.advance(active, factors, remove, dynamicStyleUniforms, paddings, dataStyles);
        } if(needsAdvanceClean.second())
            animator.clean(remove);
        return animations;
    };

    BaseLayerStyleUniform uniforms[1];
    UnsignedInt dataStyles[]{666, 666};

    /* First advance takes the only dynamic style and switches to it */
    {
        CORRADE_COMPARE(advance(5_nsec, uniforms, dataStyles), BaseLayerStyleAnimation::Uniform|BaseLayerStyleAnimation::Style);
        CORRADE_COMPARE(animator.dynamicStyle(first), 0);
        CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 1);
        CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
            666u,
            shared.styleCount() + 0u
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE(uniforms[0].topColor, Color4{0.375f});

    /* Next advance plays the other animation also, but isn't able to take any
       other dynamic style, so it doesn't update any style index */
    } {
        CORRADE_COMPARE(advance(10_nsec, uniforms, dataStyles), BaseLayerStyleAnimation::Uniform);
        CORRADE_COMPARE(animator.dynamicStyle(first), 0);
        CORRADE_COMPARE(animator.dynamicStyle(second), Containers::NullOpt);
        CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 1);
        CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
            666u,
            shared.styleCount() + 0u
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE(uniforms[0].topColor, Color4{0.5f});

    /* Next advance finishes the first animation and recycles its dynamic
       style. But the recycling is done after the allocation, so the second
       animation still isn't doing anything. */
    } {
        CORRADE_COMPARE(advance(20_nsec, uniforms, dataStyles), BaseLayerStyleAnimation::Style);
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
        CORRADE_COMPARE(advance(25_nsec, uniforms, dataStyles), BaseLayerStyleAnimation::Uniform|BaseLayerStyleAnimation::Style);
        CORRADE_COMPARE(animator.dynamicStyle(second), 0);
        CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 1);
        CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
            shared.styleCount() + 0u,
            1u
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE(uniforms[0].topColor, Color4{0.375f});
    }
}

void BaseLayerStyleAnimatorTest::advanceEmpty() {
    /* This should work even with no layer being set */
    BaseLayerStyleAnimator animator{animatorHandle(0, 1)};
    animator.advance({}, {}, {}, {}, {}, {});

    CORRADE_VERIFY(true);
}

void BaseLayerStyleAnimatorTest::advanceInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{BaseLayer::Shared::Configuration{2}
        .setDynamicStyleCount(1)
    };
    shared.setStyle(BaseLayerCommonStyleUniform{},
        {BaseLayerStyleUniform{}, BaseLayerStyleUniform{}},
        {});

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    BaseLayerStyleAnimator animator{animatorHandle(0, 1)};
    layer.assignAnimator(animator);

    DataHandle data = layer.create(0);
    animator.create(0, 1, Animation::Easing::linear, 0_nsec, 1_nsec, data);
    animator.create(0, 1, Animation::Easing::linear, 0_nsec, 1_nsec, data);
    animator.create(0, 1, Animation::Easing::linear, 0_nsec, 1_nsec, data);

    Containers::BitArray mask{NoInit, 3};
    Containers::BitArray maskInvalid{NoInit, 4};
    Float factors[3];
    Float factorsInvalid[4];
    BaseLayerStyleUniform dynamicStyleUniforms[2];
    Vector4 dynamicStylePaddings[2];
    Vector4 dynamicStylePaddingsInvalid[3];

    Containers::String out;
    Error redirectError{&out};
    animator.advance(mask, factors, maskInvalid, dynamicStyleUniforms, dynamicStylePaddings, {});
    animator.advance(mask, factorsInvalid, mask, dynamicStyleUniforms, dynamicStylePaddings, {});
    animator.advance(maskInvalid, factors, mask, dynamicStyleUniforms, dynamicStylePaddings, {});
    animator.advance(mask, factors, mask, dynamicStyleUniforms, dynamicStylePaddingsInvalid, {});
    CORRADE_COMPARE_AS(out,
        "Ui::BaseLayerStyleAnimator::advance(): expected active, factors and remove views to have a size of 3 but got 3, 3 and 4\n"
        "Ui::BaseLayerStyleAnimator::advance(): expected active, factors and remove views to have a size of 3 but got 3, 4 and 3\n"
        "Ui::BaseLayerStyleAnimator::advance(): expected active, factors and remove views to have a size of 3 but got 4, 3 and 3\n"
        "Ui::BaseLayerStyleAnimator::advance(): expected dynamic style uniform and padding views to have the same size but got 2 and 3\n",
        TestSuite::Compare::String);
}

void BaseLayerStyleAnimatorTest::layerAdvance() {
    auto&& data = LayerAdvanceData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{BaseLayer::Shared::Configuration{3}
        .setDynamicStyleCount(1)
    };
    shared.setStyle(
        BaseLayerCommonStyleUniform{},
        {BaseLayerStyleUniform{}
            .setColor(Color4{0.25f}),
         BaseLayerStyleUniform{}
            .setColor(Color4{0.75f}),
         BaseLayerStyleUniform{}},
        {{}, Vector4{data.padding}, {}});

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}

        BaseLayer::State& stateData() {
            return static_cast<BaseLayer::State&>(*_state);
        }
    } layer{layerHandle(0, 1), shared};

    /* Required to be called before update() (because AbstractUserInterface
       guarantees the same on a higher level), not needed for anything here */
    layer.setSize({1, 1}, {1, 1});

    DataHandle data1 = layer.create(2);
    DataHandle data2 = layer.create(2);

    BaseLayerStyleAnimator animator1{animatorHandle(0, 1)};
    BaseLayerStyleAnimator animatorEmpty{animatorHandle(0, 1)};
    BaseLayerStyleAnimator animator2{animatorHandle(0, 1)};
    layer.assignAnimator(animator1);
    layer.assignAnimator(animatorEmpty);
    layer.assignAnimator(animator2);

    animator1.create(0, 1, Animation::Easing::linear, 0_nsec, 20_nsec, data2);
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
    CORRADE_COMPARE(layer.dynamicStyleUniforms()[0].topColor, Color4{0.375f});
    CORRADE_COMPARE(layer.dynamicStylePaddings()[0], Vector4{data.padding}*0.25f);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate);
    CORRADE_VERIFY(layer.stateData().dynamicStyleChanged);

    /* Advancing the first animation to 1/2, which sets just the uniform and
       optionally padding */
    layer.update(LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    layer.stateData().dynamicStyleChanged = false;
    layer.advanceAnimations(10_nsec, activeStorage, startedStorage, stoppedStorage, factorStorage, removeStorage, {animator2, animatorEmpty, animator1});
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 1);
    CORRADE_COMPARE(layer.style(data2), shared.styleCount() + 0);
    CORRADE_COMPARE(layer.dynamicStyleUniforms()[0].topColor, Color4{0.5f});
    CORRADE_COMPARE(layer.dynamicStylePaddings()[0], Vector4{data.padding}*0.5f);
    CORRADE_COMPARE(layer.state(), (data.padding.isZero() ? LayerStates{} : LayerState::NeedsDataUpdate)|LayerState::NeedsCommonDataUpdate);
    CORRADE_VERIFY(layer.stateData().dynamicStyleChanged);

    /* Advancing both the first animation to 3/4 and second animation directly
       to the final style. It should thus set both the update and the style
       change. */
    layer.update(LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    layer.stateData().dynamicStyleChanged = false;
    layer.advanceAnimations(15_nsec, activeStorage, startedStorage, stoppedStorage, factorStorage, removeStorage, {animator2, animatorEmpty, animator1});
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 1);
    CORRADE_COMPARE(layer.style(data1), 0);
    CORRADE_COMPARE(layer.style(data2), shared.styleCount() + 0);
    CORRADE_COMPARE(layer.dynamicStyleUniforms()[0].topColor, Color4{0.625f});
    CORRADE_COMPARE(layer.dynamicStylePaddings()[0], Vector4{data.padding}*0.75f);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate);
    CORRADE_VERIFY(layer.stateData().dynamicStyleChanged);

    /* Advancing the first animation to the end & the final style. Only the
       style data is updated, no uniforms or paddings.  */
    layer.update(LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    layer.stateData().dynamicStyleChanged = false;
    layer.advanceAnimations(20_nsec, activeStorage, startedStorage, stoppedStorage, factorStorage, removeStorage, {animator2, animatorEmpty, animator1});
    /* If clean() wouldn't be called, the dynamic style won't get recycled */
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 0);
    CORRADE_COMPARE(layer.style(data2), 1);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
    CORRADE_VERIFY(!layer.stateData().dynamicStyleChanged);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::BaseLayerStyleAnimatorTest)
