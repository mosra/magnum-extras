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

#include "Magnum/Ui/AbstractUserInterface.h" /* for uiAdvance() */
#include "Magnum/Ui/BaseLayer.h"
#include "Magnum/Ui/BaseLayerAnimator.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/Implementation/baseLayerState.h" /* for layerAdvance() */

namespace Magnum { namespace Ui { namespace Test { namespace {

struct BaseLayerStyleAnimatorTest: TestSuite::Tester {
    explicit BaseLayerStyleAnimatorTest();

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
    void advanceExternalStyleChanges();
    void advanceEmpty();
    void advanceInvalid();

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
    bool samePaddingAfter, attachLaterAfter;
} CreateRemoveHandleRecycleData[]{
    {"", false, false},
    {"same paddings in recycled", true, false},
    {"attach recycled later", false, true},
};

const struct {
    const char* name;
    bool noAttachment;
    UnsignedInt uniform;
    Vector4 padding;
    BaseLayerStyleAnimatorUpdates expectedStart, expectedMiddle;
} AdvancePropertiesData[]{
    {"nothing changes",
        false, 1, Vector4{2.0f},
        BaseLayerStyleAnimatorUpdate::Uniform|BaseLayerStyleAnimatorUpdate::Style,
        {}},
    {"nothing changes, no attachment",
        true, 1, Vector4{2.0f},
        /* Uniform should be still set to trigger at least one upload of the
           dynamic style */
        BaseLayerStyleAnimatorUpdate::Uniform,
        {}},
    {"uniform ID changes",
        false, 0, Vector4{2.0f},
        BaseLayerStyleAnimatorUpdate::Uniform|BaseLayerStyleAnimatorUpdate::Style,
        BaseLayerStyleAnimatorUpdate::Uniform},
    /* Still reports uniform change because comparing all values is unnecessary
       complexity */
    {"uniform ID changes but data stay the same",
        false, 3, Vector4{2.0f},
        BaseLayerStyleAnimatorUpdate::Uniform|BaseLayerStyleAnimatorUpdate::Style,
        BaseLayerStyleAnimatorUpdate::Uniform},
    {"padding changes",
        false, 1, Vector4{4.0f},
        BaseLayerStyleAnimatorUpdate::Uniform|BaseLayerStyleAnimatorUpdate::Padding|BaseLayerStyleAnimatorUpdate::Style,
        BaseLayerStyleAnimatorUpdate::Padding},
    {"uniform ID + padding changes",
        false, 0, Vector4{4.0f},
        BaseLayerStyleAnimatorUpdate::Uniform|BaseLayerStyleAnimatorUpdate::Padding|BaseLayerStyleAnimatorUpdate::Style,
        BaseLayerStyleAnimatorUpdate::Uniform|BaseLayerStyleAnimatorUpdate::Padding},
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
    bool noAttachment,
        allocateDynamicStyleBeforeStart,
        freeDynamicStyleAfterStart;
    Containers::Optional<UnsignedInt> changeStyleBeforeStart,
        changeStyleAfterStart,
        changeStyleBeforeStop;
    UnsignedInt expectedStyleBegin,
        expectedStyleMiddle,
        expectedStyleEnd;
    BaseLayerStyleAnimatorUpdates expectedUpdatesBegin,
        expectedUpdatesMiddle,
        expectedUpdatesEnd;
    UnsignedInt expectedDynamicStyleUsedCountBegin,
        expectedDynamicStyleUsedCountMiddle,
        expectedDynamicStyleUsedCountEnd;
} AdvanceExternalStyleChangesData[]{
    {"no attachment",
        true, false, false, {}, {}, {},
        1, 1, 1,
        BaseLayerStyleAnimatorUpdate::Uniform,
        {},
        {},
        1, 1, 0},
    /* There's 10 styles and 1 dynamic style, so 10 is the dynamic style index
       if used */
    {"no changes",
        false, false, false, {}, {}, {},
        10, 10, 3,
        BaseLayerStyleAnimatorUpdate::Style|BaseLayerStyleAnimatorUpdate::Uniform,
        {},
        BaseLayerStyleAnimatorUpdate::Style,
        1, 1, 0},
    /* This results in the same, as the initial style is remembered only after
       the animation starts. Otherwise it'd be impossible to reuse the
       animations as they'd subsequently compare to a stale style that was set
       at creation time, not at the time the animation starts. */
    {"change style before start",
        false, false, false, 5u, {}, {},
        10, 10, 3,
        BaseLayerStyleAnimatorUpdate::Style|BaseLayerStyleAnimatorUpdate::Uniform,
        {},
        BaseLayerStyleAnimatorUpdate::Style,
        1, 1, 0},
    {"change style after start",
        false, false, false, {}, 5u, {},
        10, 5, 5,
        BaseLayerStyleAnimatorUpdate::Style|BaseLayerStyleAnimatorUpdate::Uniform,
        {},
        {},
        1, 1, 0},
    /* Transition to the source (7) and target (3) style happens but without
       any animation */
    {"no free dynamic styles",
        false, true, false, {}, {}, {},
        7, 7, 3,
        BaseLayerStyleAnimatorUpdate::Style,
        {},
        BaseLayerStyleAnimatorUpdate::Style,
        /* The dynamic style was allocated but not freed by the test case
           itself so it stays used even after the animation stops */
        1, 1, 1},
    {"no free dynamic styles, change style after start",
        false, true, false, {}, 9u, {},
        7, 9, 9,
        BaseLayerStyleAnimatorUpdate::Style,
        {},
        {},
        /* The dynamic style was allocated but not freed by the test case
           itself so it stays used even after the animation stops */
        1, 1, 1},
    {"free dynamic styles only after second advance",
        false, true, true, {}, {}, {},
        7, 10, 3,
        BaseLayerStyleAnimatorUpdate::Style,
        BaseLayerStyleAnimatorUpdate::Style|BaseLayerStyleAnimatorUpdate::Uniform,
        BaseLayerStyleAnimatorUpdate::Style,
        1, 1, 0},
    /* The dynamic style doesn't even get allocated in this case because it's
       not going to be used for anything anyway */
    {"free dynamic styles only after second advance, change style after start",
        false, true, true, {}, 8u, {},
        7, 8, 8,
        BaseLayerStyleAnimatorUpdate::Style,
        {},
        {},
        1, 0, 0},
    {"free dynamic styles only after second advance, change style before end",
        false, true, true, {}, {}, 8u,
        7, 10, 8,
        BaseLayerStyleAnimatorUpdate::Style,
        BaseLayerStyleAnimatorUpdate::Style|BaseLayerStyleAnimatorUpdate::Uniform,
        {},
        1, 1, 0},
    /* Even if the style changes back to the one that was there on animation
       start, the animation is considered stale and should not affect the data
       style anymore */
    {"free dynamic styles only after second advance, change style after start and then back to the expected before end",
        false, true, true, {}, 5u, 1u,
        7, 5, 1,
        BaseLayerStyleAnimatorUpdate::Style,
        {},
        {},
        1, 0, 0},
};

const struct {
    const char* name;
    Vector4 padding;
} LayerAdvanceData[]{
    {"", {}},
    {"padding changes as well", Vector4{2.0f}}
};

BaseLayerStyleAnimatorTest::BaseLayerStyleAnimatorTest() {
    addTests({&BaseLayerStyleAnimatorTest::debugAnimatorUpdate,
              &BaseLayerStyleAnimatorTest::debugAnimatorUpdates,

              &BaseLayerStyleAnimatorTest::construct,
              &BaseLayerStyleAnimatorTest::constructCopy,
              &BaseLayerStyleAnimatorTest::constructMove,

              &BaseLayerStyleAnimatorTest::assignAnimator,
              &BaseLayerStyleAnimatorTest::setDefaultStyleAnimator,

              &BaseLayerStyleAnimatorTest::createRemove<UnsignedInt>,
              &BaseLayerStyleAnimatorTest::createRemove<Enum>});

    addInstancedTests({&BaseLayerStyleAnimatorTest::createRemoveHandleRecycle},
        Containers::arraySize(CreateRemoveHandleRecycleData));

    addTests({&BaseLayerStyleAnimatorTest::createInvalid,
              &BaseLayerStyleAnimatorTest::propertiesInvalid,

              &BaseLayerStyleAnimatorTest::advance});

    addInstancedTests({&BaseLayerStyleAnimatorTest::advanceProperties},
        Containers::arraySize(AdvancePropertiesData));

    addTests({&BaseLayerStyleAnimatorTest::advanceNoFreeDynamicStyles});

    addInstancedTests({&BaseLayerStyleAnimatorTest::advanceConflictingAnimations},
        Containers::arraySize(AdvanceConflictingAnimationsData));

    addInstancedTests({&BaseLayerStyleAnimatorTest::advanceExternalStyleChanges},
        Containers::arraySize(AdvanceExternalStyleChangesData));

    addTests({&BaseLayerStyleAnimatorTest::advanceEmpty,
              &BaseLayerStyleAnimatorTest::advanceInvalid});

    addInstancedTests({&BaseLayerStyleAnimatorTest::layerAdvance},
        Containers::arraySize(LayerAdvanceData));

    addTests({&BaseLayerStyleAnimatorTest::uiAdvance});
}

void BaseLayerStyleAnimatorTest::debugAnimatorUpdate() {
    Containers::String out;
    Debug{&out} << BaseLayerStyleAnimatorUpdate::Style << BaseLayerStyleAnimatorUpdate(0xbe);
    CORRADE_COMPARE(out, "Ui::BaseLayerStyleAnimatorUpdate::Style Ui::BaseLayerStyleAnimatorUpdate(0xbe)\n");
}

void BaseLayerStyleAnimatorTest::debugAnimatorUpdates() {
    Containers::String out;
    Debug{&out} << (BaseLayerStyleAnimatorUpdate::Uniform|BaseLayerStyleAnimatorUpdate(0xe0)) << BaseLayerStyleAnimatorUpdates{};
    CORRADE_COMPARE(out, "Ui::BaseLayerStyleAnimatorUpdate::Uniform|Ui::BaseLayerStyleAnimatorUpdate(0xe0) Ui::BaseLayerStyleAnimatorUpdates{}\n");
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
    AnimationHandle second = animator.create(T(2), T(0), Animation::Easing::cubicIn, -15_nsec, 1_nsec, DataHandle::Null, AnimationFlag(0x40));
    CORRADE_COMPARE(animator.duration(second), 1_nsec);
    CORRADE_COMPARE(animator.repeatCount(second), 1);
    CORRADE_COMPARE(animator.flags(second), AnimationFlag(0x40));
    CORRADE_COMPARE(animator.started(second), -15_nsec);
    CORRADE_COMPARE(animator.data(second), DataHandle::Null);
    CORRADE_COMPARE(animator.styles(second), Containers::pair(2u, 0u));
    /* Can't use T, as the function restricts to enum types which would fail
       for T == UnsignedInt */
    CORRADE_COMPARE(animator.template styles<Enum>(second), Containers::pair(Enum(2), Enum(0)));
    CORRADE_COMPARE(animator.dynamicStyle(second), Containers::NullOpt);
    CORRADE_COMPARE(animator.easing(second), Animation::Easing::cubicIn);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 0);

    /* LayerDataHandle overload, verify also with AnimatorDataHandle */
    AnimationHandle third = animator.create(T(1), T(2), Animation::Easing::bounceInOut, 0_nsec, 100_nsec, dataHandleData(data3), 0, AnimationFlag(0x80));
    CORRADE_COMPARE(animator.duration(animationHandleData(third)), 100_nsec);
    CORRADE_COMPARE(animator.repeatCount(animationHandleData(third)), 0);
    CORRADE_COMPARE(animator.flags(animationHandleData(third)), AnimationFlag(0x80));
    CORRADE_COMPARE(animator.started(animationHandleData(third)), 0_nsec);
    CORRADE_COMPARE(animator.data(animationHandleData(third)), data3);
    CORRADE_COMPARE(animator.styles(animationHandleData(third)), Containers::pair(1u, 2u));
    /* Can't use T, as the function restricts to enum types which would fail
       for T == UnsignedInt */
    CORRADE_COMPARE(animator.template styles<Enum>(animationHandleData(third)), Containers::pair(Enum(1), Enum(2)));
    CORRADE_COMPARE(animator.dynamicStyle(animationHandleData(third)), Containers::NullOpt);
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
    CORRADE_COMPARE(animator.styles(fourth), Containers::pair(0u, 2u));
    /* Can't use T, as the function restricts to enum types which would fail
       for T == UnsignedInt */
    CORRADE_COMPARE(animator.template styles<Enum>(fourth), Containers::pair(Enum(0), Enum(2)));
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

void BaseLayerStyleAnimatorTest::createRemoveHandleRecycle() {
    auto&& data = CreateRemoveHandleRecycleData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{BaseLayer::Shared::Configuration{4}
        .setDynamicStyleCount(1)
    };

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    BaseLayerStyleAnimator animator{animatorHandle(0, 1)};
    layer.assignAnimator(animator);

    DataHandle layerData = layer.create(1);

    /* Allocate an animation */
    AnimationHandle first = animator.create(0, 1, Animation::Easing::linear, 0_nsec, 10_nsec, layerData);
    CORRADE_COMPARE(animator.styles(first), Containers::pair(0u, 1u));
    CORRADE_COMPARE(animator.dynamicStyle(first), Containers::NullOpt);
    CORRADE_COMPARE(animator.easing(first), Animation::Easing::linear);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 0);

    /* Set the style after animation creation to verify it isn't needed
       earlier */
    shared.setStyle(BaseLayerCommonStyleUniform{},
        {BaseLayerStyleUniform{}
            .setColor(0xff3366_rgbf),
         BaseLayerStyleUniform{}
            .setColor(0x9933ff_rgbf),
         BaseLayerStyleUniform{}
            .setColor(0x663399_rgbf),
         BaseLayerStyleUniform{}
            .setColor(0x996633_rgbf)},
        {Vector4{1.0f},
         Vector4{2.0f},
         Vector4{3.0f},
         Vector4{data.samePaddingAfter ? 3.0f : 4.0f}});

    /* Let it advance to allocate the dynamic style and copy over style data */
    Containers::BitArray activeStorage{NoInit, 1};
    Containers::BitArray startedStorage{NoInit, 1};
    Containers::BitArray stoppedStorage{NoInit, 1};
    Float factorStorage[1];
    Containers::BitArray removedStorage{NoInit, 1};
    BaseLayerStyleUniform dynamicStyleUniforms[1];
    Vector4 dynamicStylePaddings[1];
    UnsignedInt dataStyles[1];
    CORRADE_COMPARE(animator.advance(5_nsec,
            activeStorage,
            startedStorage,
            stoppedStorage,
            factorStorage,
            removedStorage,
            dynamicStyleUniforms, dynamicStylePaddings, dataStyles),
        BaseLayerStyleAnimatorUpdate::Uniform|
        BaseLayerStyleAnimatorUpdate::Padding|
        BaseLayerStyleAnimatorUpdate::Style);
    CORRADE_COMPARE(dataStyles[0], 4);
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
    AnimationHandle first2 = animator.create(2, 3, Animation::Easing::bounceInOut, -10_nsec, 30_nsec, data.attachLaterAfter ? DataHandle::Null : layerData);
    CORRADE_COMPARE(animationHandleId(first2), animationHandleId(first));
    CORRADE_COMPARE(animator.styles(first2), Containers::pair(2u, 3u));
    CORRADE_COMPARE(animator.dynamicStyle(first2), Containers::NullOpt);
    CORRADE_COMPARE(animator.easing(first2), Animation::Easing::bounceInOut);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 0);

    /* The recycled animation shouldn't inherit any info about uniform or
       padding style animations. The padding is however checked against the
       current value, so update it to the expected new (constant) value
       first. */
    dynamicStylePaddings[0] = Vector4{3.0f};
    CORRADE_COMPARE(animator.advance(10_nsec,
            activeStorage,
            startedStorage,
            stoppedStorage,
            factorStorage,
            removedStorage,
            dynamicStyleUniforms, dynamicStylePaddings, dataStyles),
        BaseLayerStyleAnimatorUpdate::Uniform|
        (data.samePaddingAfter ? BaseLayerStyleAnimatorUpdates{} : BaseLayerStyleAnimatorUpdate::Padding)|
        (data.attachLaterAfter ? BaseLayerStyleAnimatorUpdates{} : BaseLayerStyleAnimatorUpdate::Style));

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
                dynamicStyleUniforms, dynamicStylePaddings, dataStyles),
            BaseLayerStyleAnimatorUpdates{});
        CORRADE_COMPARE(dataStyles[0], 4);
    }
}

void BaseLayerStyleAnimatorTest::createInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{BaseLayer::Shared::Configuration{1, 5}
        .setDynamicStyleCount(1)};

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    BaseLayerStyleAnimator animatorNoLayerSet{animatorHandle(0, 1)};

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

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    BaseLayerStyleAnimator animator{animatorHandle(0, 1)};
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
        "Ui::BaseLayerStyleAnimator::easing(): invalid handle Ui::AnimationHandle::Null\n"

        "Ui::BaseLayerStyleAnimator::easing(): invalid handle Ui::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"

        "Ui::BaseLayerStyleAnimator::easing(): invalid handle Ui::AnimationHandle(Null, {0x0, 0x1})\n"

        "Ui::BaseLayerStyleAnimator::easing(): invalid handle Ui::AnimatorDataHandle(0xabcde, 0x123)\n",
        TestSuite::Compare::String);
}

void BaseLayerStyleAnimatorTest::advance() {
    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{BaseLayer::Shared::Configuration{4, 7}
        .setDynamicStyleCount(4)
    };

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    /* Assign data to styles that aren't used for animation */
    DataHandle data0 = layer.create(4);
    DataHandle data1 = layer.create(0);
    DataHandle data2 = layer.create(2);
    DataHandle data3 = layer.create(4);
    DataHandle data4 = layer.create(5);
    DataHandle data5 = layer.create(0);
    DataHandle data6 = layer.create(2);

    BaseLayerStyleAnimator animator{animatorHandle(0, 1)};
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

    /* Set the style after animation creation to verify it isn't needed
       earlier */
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

    /* Does what layer's advanceAnimations() is doing internally for all
       animators (as we need to test also the interaction with animation being
       removed, etc.), but with an ability to peek into the filled data to
       verify they're written only when they should be. Layer's
       advanceAnimations() is then tested in layerAdvance() below. */
    const auto advance = [&](Nanoseconds time, Containers::ArrayView<BaseLayerStyleUniform> dynamicStyleUniforms, const Containers::StridedArrayView1D<Vector4>& dynamicStylePaddings, const Containers::StridedArrayView1D<UnsignedInt>& dataStyles) {
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
            dynamicStyleUniforms, dynamicStylePaddings, dataStyles);
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
       switches the styles to them and fills the dynamic data. For the stopped
       & removed and stopped & kept animations it switches the style to the
       destination one. */
    {
        BaseLayerStyleUniform uniforms[4];
        CORRADE_COMPARE(advance(5_nsec, uniforms, paddings, dataStyles), BaseLayerStyleAnimatorUpdate::Uniform|BaseLayerStyleAnimatorUpdate::Style|BaseLayerStyleAnimatorUpdate::Padding);
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
            CORRADE_COMPARE(uniforms[i].topColor, Color4{0.5f});
            CORRADE_COMPARE(uniforms[i].bottomColor, Color4{0.875f});
            CORRADE_COMPARE(uniforms[i].outlineColor, Color4{5.0f});
            CORRADE_COMPARE(uniforms[i].outlineWidth, Vector4{28.0f});
            CORRADE_COMPARE(uniforms[i].cornerRadius, Vector4{9.0f});
            CORRADE_COMPARE(uniforms[i].innerOutlineCornerRadius, Vector4{18.0f});
            CORRADE_COMPARE(paddings[i], Vector4{2.0f});
        }
    }

    /* Reset the padding of the stopped & kept style to something else to
       verify it doesn't get touched anymore */
    paddings[3] = {};

    /* Advancing to 10 changes just the uniform to 1/2, nothing else */
    {
        BaseLayerStyleUniform uniforms[4];
        CORRADE_COMPARE(advance(10_nsec, uniforms, paddings, dataStyles), BaseLayerStyleAnimatorUpdate::Uniform);
        CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 3);
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
            CORRADE_COMPARE(uniforms[i].topColor, Color4{1.0f});
            CORRADE_COMPARE(uniforms[i].innerOutlineCornerRadius, Vector4{20.0f});
            CORRADE_COMPARE(paddings[i], Vector4{2.0f});
        }
    }

    /* Advancing to 15 plays also the scheduled animation without a data
       attachment, allocating a new dynamic style but not switching to it.
       I.e., no Style is set, only Uniform and Padding. */
    {
        BaseLayerStyleUniform uniforms[4];
        CORRADE_COMPARE(advance(15_nsec, uniforms, paddings, dataStyles), BaseLayerStyleAnimatorUpdate::Uniform|BaseLayerStyleAnimatorUpdate::Padding);
        CORRADE_COMPARE(animator.state(scheduledNullData), AnimationState::Playing);
        CORRADE_COMPARE(animator.dynamicStyle(scheduledNullData), 3);
        CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 4);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(0), playing);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(1), playingReverse);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(2), playingReverseEveryOther);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(3), scheduledNullData);
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
            CORRADE_COMPARE(uniforms[i].topColor, Color4{1.5f});
            CORRADE_COMPARE(paddings[i], Vector4{2.0f});
        }
        /* The null data animation is set to the value of style 1 */
        CORRADE_COMPARE(uniforms[3].topColor, Color4{2.0f});
        CORRADE_COMPARE(paddings[3], Vector4{2.0f});
    }

    /* Advancing to 20 stops the first two animations, recycling their dynamic
       style and changing the style to the target one (and source one for the
       Reverse animation). Uniform value is updated for the null data
       animation. */
    {
        BaseLayerStyleUniform uniforms[4];
        CORRADE_COMPARE(advance(20_nsec, uniforms, paddings, dataStyles), BaseLayerStyleAnimatorUpdate::Style|BaseLayerStyleAnimatorUpdate::Uniform);
        CORRADE_VERIFY(!animator.isHandleValid(playing));
        CORRADE_VERIFY(animator.isHandleValid(stoppedKept));
        CORRADE_VERIFY(animator.isHandleValid(scheduledChangesPadding));
        CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 1);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(0), AnimationHandle::Null);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(1), AnimationHandle::Null);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(2), AnimationHandle::Null);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(3), scheduledNullData);
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
        /* Uniform values of the recycled styles aren't touched anymore */
        for(UnsignedInt i: {0, 1, 2}) {
            CORRADE_ITERATION(i);
            CORRADE_COMPARE(uniforms[i].topColor, Color4{1.0f});
        }
        /* The null data animation is advanced to 1/2 between style 1 and 3 */
        CORRADE_COMPARE(uniforms[3].topColor, Color4{1.0f});
        CORRADE_COMPARE(paddings[3], Vector4{2.0f});
    }

    /* Advancing to 25 stops the null data animation, recycling its dynamic
       style. Leads to no other change, i.e. no Style set. */
    {
        BaseLayerStyleUniform uniforms[4];
        CORRADE_COMPARE(advance(25_nsec, uniforms, paddings, dataStyles), BaseLayerStyleAnimatorUpdates{});
        CORRADE_VERIFY(!animator.isHandleValid(scheduledNullData));
        CORRADE_VERIFY(animator.isHandleValid(stoppedKept));
        CORRADE_VERIFY(animator.isHandleValid(scheduledChangesPadding));
        CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 0);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(0), AnimationHandle::Null);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(1), AnimationHandle::Null);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(2), AnimationHandle::Null);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(3), AnimationHandle::Null);
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
        for(UnsignedInt i: {0, 1}) {
            CORRADE_ITERATION(i);
            CORRADE_COMPARE(uniforms[i].topColor, Color4{1.0f});
        }
        CORRADE_COMPARE(uniforms[2].topColor, Color4{1.0f});
    }

    /* Advancing to 35 plays the scheduled animation, allocating a new dynamic
       style and switching to it */
    {
        BaseLayerStyleUniform uniforms[4];
        CORRADE_COMPARE(advance(35_nsec, uniforms, paddings, dataStyles), BaseLayerStyleAnimatorUpdate::Uniform|BaseLayerStyleAnimatorUpdate::Style|BaseLayerStyleAnimatorUpdate::Padding);
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
        CORRADE_COMPARE(uniforms[0].topColor, Color4{0.0f});
        CORRADE_COMPARE(paddings[0], Vector4{3.5f});
    }

    /* Advancing to 45 advances the scheduled animation, changing both the
       uniform and the padding. No styles. */
    {
        BaseLayerStyleUniform uniforms[4];
        CORRADE_COMPARE(advance(45_nsec, uniforms, paddings, dataStyles), BaseLayerStyleAnimatorUpdate::Uniform|BaseLayerStyleAnimatorUpdate::Padding);
        CORRADE_VERIFY(animator.isHandleValid(stoppedKept));
        CORRADE_VERIFY(animator.isHandleValid(scheduledChangesPadding));
        CORRADE_COMPARE(animator.state(stoppedKept), AnimationState::Stopped);
        CORRADE_COMPARE(animator.state(scheduledChangesPadding), AnimationState::Playing);
        CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 1);
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
        CORRADE_COMPARE(uniforms[0].topColor, Color4{0.0f});
        CORRADE_COMPARE(paddings[0], Vector4{2.5f});
    }

    /* Stopping the remaining animation (even before it finishes at 50) makes
       it recycle the remaining dynamic style and switch to the target style at
       the next advance(). Not updating any uniforms or paddings. */
    {
        BaseLayerStyleUniform uniforms[4];
        animator.stop(scheduledChangesPadding, 46_nsec);
        CORRADE_COMPARE(advance(47_nsec, uniforms, paddings, dataStyles), BaseLayerStyleAnimatorUpdate::Style);
        CORRADE_VERIFY(!animator.isHandleValid(scheduledChangesPadding));
        CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 0);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(0), AnimationHandle::Null);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(1), AnimationHandle::Null);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(2), AnimationHandle::Null);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(3), AnimationHandle::Null);
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
        BaseLayerStyleUniform uniforms[4];
        animator.play(stoppedKept, 45_nsec);
        CORRADE_COMPARE(advance(50_nsec, uniforms, paddings, dataStyles), BaseLayerStyleAnimatorUpdate::Uniform|BaseLayerStyleAnimatorUpdate::Padding|BaseLayerStyleAnimatorUpdate::Style);
        CORRADE_VERIFY(animator.isHandleValid(stoppedKept));
        CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 1);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(0), stoppedKept);
        CORRADE_COMPARE(layer.dynamicStyleAnimation(1), AnimationHandle::Null);
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
        CORRADE_COMPARE(uniforms[0].topColor, Color4{0.0f});
        CORRADE_COMPARE(uniforms[0].bottomColor, Color4{1.0f/3.0f});
        CORRADE_COMPARE(paddings[0], Vector4{10.0f/3.0f});
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

void BaseLayerStyleAnimatorTest::advanceProperties() {
    auto&& data = AdvancePropertiesData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{BaseLayer::Shared::Configuration{4, 3}
        .setDynamicStyleCount(1)
    };

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    /* Assign data to a style that isn't used for animation */
    DataHandle layerData = layer.create(1);

    BaseLayerStyleAnimator animator{animatorHandle(0, 1)};
    layer.assignAnimator(animator);

    AnimationHandle animation = animator.create(2, 0, Animation::Easing::linear, 0_nsec, 20_nsec, data.noAttachment ? DataHandle::Null : layerData);

    /* Set the style after animation creation to verify it isn't needed
       earlier */
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

    /* Does what layer's advanceAnimations() is doing internally for all
       animators (as we need to test also the interaction with animation being
       removed, etc.), but with an ability to peek into the filled data to
       verify they're written only when they should be. Compared to the helper
       in advance() above it's not exposing all data. */
    const auto advance = [&](Nanoseconds time, Containers::ArrayView<BaseLayerStyleUniform> dynamicStyleUniforms, const Containers::StridedArrayView1D<Vector4>& dynamicStylePaddings, const Containers::StridedArrayView1D<UnsignedInt>& dataStyles) {
        UnsignedByte activeStorage[1];
        UnsignedByte startedStorage[1];
        UnsignedByte stoppedStorage[1];
        Float factorStorage[1];
        UnsignedByte removeStorage[1];

        return animator.advance(time,
            Containers::MutableBitArrayView{activeStorage, 0, 1},
            Containers::MutableBitArrayView{startedStorage, 0, 1},
            Containers::MutableBitArrayView{stoppedStorage, 0, 1},
            factorStorage,
            Containers::MutableBitArrayView{removeStorage, 0, 1},
            dynamicStyleUniforms, dynamicStylePaddings, dataStyles);
    };

    /* The padding resulting from the animation gets checked against these.
       Contrary to the advance() test case, set it to the initial padding value
       so the initial advance doesn't report padding as changed. */
    Vector4 paddings[]{
        Vector4{2.0f}
    };

    /* The dataStyles are compared against to not break animations and style
       changes that happened since the original animation started and so they
       need to be preserved across advances. Behavior with external style
       changes is tested in advanceExternalStyleChanges(). */
    UnsignedInt dataStyles[]{
        666
    };

    /* Advancing to 5 allocates a dynamic style, switches to it and fills the
       dynamic data. The Uniform is reported together with Style always in
       order to ensure the dynamic uniform is uploaded even though it won't
       subsequently change. */
    {
        BaseLayerStyleUniform uniforms[1];
        CORRADE_COMPARE(advance(5_nsec, uniforms, paddings, dataStyles), data.expectedStart);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Playing);
        CORRADE_COMPARE(animator.dynamicStyle(animation), 0);
        CORRADE_COMPARE(uniforms[0].topColor, Math::lerp(Color4{2.0f}, Color4{uniformColors[data.uniform]}, 0.25f));
        CORRADE_COMPARE(paddings[0], Math::lerp(Vector4{2.0f}, data.padding, 0.25f));
        CORRADE_COMPARE(dataStyles[0], data.noAttachment ? 666 : 3);

    /* Advancing to 15 changes only what's expected */
    } {
        BaseLayerStyleUniform uniforms[1];
        CORRADE_COMPARE(advance(15_nsec, uniforms, paddings, dataStyles), data.expectedMiddle);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Playing);
        CORRADE_COMPARE(animator.dynamicStyle(animation), 0);
        CORRADE_COMPARE(uniforms[0].topColor, Math::lerp(Color4{2.0f}, Color4{uniformColors[data.uniform]}, 0.75f));
        CORRADE_COMPARE(paddings[0], Math::lerp(Vector4{2.0f}, data.padding, 0.75f));
        CORRADE_COMPARE(dataStyles[0], data.noAttachment ? 666 : 3);

    /* Advancing to 25 changes only the Style if attached, the dynamic style
       values are unused now */
    } {
        BaseLayerStyleUniform uniforms[1];
        CORRADE_COMPARE(advance(25_nsec, uniforms, paddings, dataStyles), data.noAttachment ? BaseLayerStyleAnimatorUpdates{} : BaseLayerStyleAnimatorUpdate::Style);
        CORRADE_VERIFY(!animator.isHandleValid(animation));
        CORRADE_COMPARE(dataStyles[0], data.noAttachment ? 666 : 0);
    }
}

void BaseLayerStyleAnimatorTest::advanceNoFreeDynamicStyles() {
    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{BaseLayer::Shared::Configuration{4}
        .setDynamicStyleCount(1)
    };

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    BaseLayerStyleAnimator animator{animatorHandle(0, 1)};
    layer.assignAnimator(animator);

    DataHandle data1 = layer.create(2);
    DataHandle data2 = layer.create(2);

    AnimationHandle first = animator.create(0, 1, Animation::Easing::linear, 0_nsec, 20_nsec, data2);
    AnimationHandle second = animator.create(2, 1, Animation::Easing::linear, 10_nsec, 40_nsec, data1);

    /* Set the style after animation creation to verify it isn't needed
       earlier */
    shared.setStyle(
        BaseLayerCommonStyleUniform{},
        {BaseLayerStyleUniform{}
            .setColor(Color4{0.25f}),
         BaseLayerStyleUniform{}
            .setColor(Color4{0.75f}),
         BaseLayerStyleUniform{}
            .setColor(Color4{1.25f}),
         BaseLayerStyleUniform{}},
        {});

    /* Does what layer's advanceAnimations() is doing internally for all
       animators (as we need to test also the interaction with animation being
       removed, etc.), but with an ability to peek into the filled data to
       verify they're written only when they should be. Compared to the helper
       in advance() above it exposes only some data. */
    const auto advance = [&](Nanoseconds time, Containers::ArrayView<BaseLayerStyleUniform> dynamicStyleUniforms, const Containers::StridedArrayView1D<UnsignedInt>& dataStyles) {
        UnsignedByte activeStorage[1];
        UnsignedByte startedStorage[1];
        UnsignedByte stoppedStorage[1];
        Float factorStorage[2];
        UnsignedByte removeStorage[1];
        Vector4 paddings[1];

        return animator.advance(time,
            Containers::MutableBitArrayView{activeStorage, 0, 2},
            Containers::MutableBitArrayView{startedStorage, 0, 2},
            Containers::MutableBitArrayView{stoppedStorage, 0, 2},
            factorStorage,
            Containers::MutableBitArrayView{removeStorage, 0, 2}, dynamicStyleUniforms, paddings, dataStyles);
    };

    BaseLayerStyleUniform uniforms[1];
    UnsignedInt dataStyles[]{666, 666};

    /* First advance takes the only dynamic style and switches to it */
    CORRADE_COMPARE(advance(5_nsec, uniforms, dataStyles), BaseLayerStyleAnimatorUpdate::Uniform|BaseLayerStyleAnimatorUpdate::Style);
    CORRADE_COMPARE(animator.dynamicStyle(first), 0);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 1);
    CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
        666u,
        shared.styleCount() + 0u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(uniforms[0].topColor, Color4{0.375f});

    /* Next advance plays the other animation also, but isn't able to take any
       other dynamic style, so it updates the style index only to the initial
       style */
    CORRADE_COMPARE(advance(10_nsec, uniforms, dataStyles), BaseLayerStyleAnimatorUpdate::Uniform|BaseLayerStyleAnimatorUpdate::Style);
    CORRADE_COMPARE(animator.dynamicStyle(first), 0);
    CORRADE_COMPARE(animator.dynamicStyle(second), Containers::NullOpt);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 1);
    CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
        2u,
        shared.styleCount() + 0u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(uniforms[0].topColor, Color4{0.5f});

    /* Another advance still doesn't have any dynamic style to switch to, so
       it's just uniforms */
    CORRADE_COMPARE(advance(15_nsec, uniforms, dataStyles), BaseLayerStyleAnimatorUpdate::Uniform);
    CORRADE_COMPARE(animator.dynamicStyle(first), 0);
    CORRADE_COMPARE(animator.dynamicStyle(second), Containers::NullOpt);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 1);
    CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
        2u,
        shared.styleCount() + 0u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(uniforms[0].topColor, Color4{0.625f});

    /* Next advance finishes the first animation and recycles its dynamic
       style, which allows the second animation to take over it */
    CORRADE_COMPARE(advance(20_nsec, uniforms, dataStyles), BaseLayerStyleAnimatorUpdate::Uniform|BaseLayerStyleAnimatorUpdate::Style);
    CORRADE_VERIFY(!animator.isHandleValid(first));
    CORRADE_COMPARE(animator.dynamicStyle(second), 0);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 1);
    CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
        shared.styleCount() + 0u,
        1u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(uniforms[0].topColor, Color4{1.125f});
}

void BaseLayerStyleAnimatorTest::advanceConflictingAnimations() {
    auto&& data = AdvanceConflictingAnimationsData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{BaseLayer::Shared::Configuration{4}
        .setDynamicStyleCount(2)
    };

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    BaseLayerStyleAnimator animator{animatorHandle(0, 1)};
    layer.assignAnimator(animator);

    /* Create a second data just to ensure the zero index isn't updated by
       accident always */
    layer.create(3);
    DataHandle data2 = layer.create(3);

    AnimationHandle first = animator.create(0, 1, Animation::Easing::linear, 0_nsec, 20_nsec, data2, data.firstAnimationRepeatCount, data.firstAnimationFlags);
    /* If there are no free dynamic styles, the data should get style 2 both in
       the forward and reverse case */
    AnimationHandle second = animator.create(
        data.secondAnimationReverse ? 1 : 2,
        data.secondAnimationReverse ? 2 : 1,
        Animation::Easing::linear, 10_nsec, 40_nsec, data2,
        data.secondAnimationReverse ? AnimationFlag::Reverse : AnimationFlags{});

    /* Set the style after animation creation to verify it isn't needed
       earlier */
    shared.setStyle(
        BaseLayerCommonStyleUniform{},
        {BaseLayerStyleUniform{}
            .setColor(Color4{0.25f}),
         BaseLayerStyleUniform{}
            .setColor(Color4{0.75f}),
         BaseLayerStyleUniform{}
            .setColor(Color4{1.25f}),
         BaseLayerStyleUniform{}},
        {});

    /* Does what layer's advanceAnimations() is doing internally for all
       animators (as we need to test also the interaction with animation being
       removed, etc.), but with an ability to peek into the filled data to
       verify they're written only when they should be. Compared to the helper
       in advance() above it exposes only some data. */
    const auto advance = [&](Nanoseconds time, Containers::ArrayView<BaseLayerStyleUniform> dynamicStyleUniforms, const Containers::StridedArrayView1D<UnsignedInt>& dataStyles) {
        UnsignedByte activeStorage[1];
        UnsignedByte startedStorage[1];
        UnsignedByte stoppedStorage[1];
        Float factorStorage[2];
        UnsignedByte removeStorage[1];
        Vector4 paddings[2];

        return animator.advance(time,
            Containers::MutableBitArrayView{activeStorage, 0, 2},
            Containers::MutableBitArrayView{startedStorage, 0, 2},
            Containers::MutableBitArrayView{stoppedStorage, 0, 2},
            factorStorage,
            Containers::MutableBitArrayView{removeStorage, 0, 2}, dynamicStyleUniforms, paddings, dataStyles);
    };

    BaseLayerStyleUniform uniforms[2];
    UnsignedInt dataStyles[]{666, 666};

    /* First advance takes the dynamic style and switches to it */
    CORRADE_COMPARE(advance(5_nsec, uniforms, dataStyles), BaseLayerStyleAnimatorUpdate::Uniform|BaseLayerStyleAnimatorUpdate::Style);
    CORRADE_COMPARE(animator.dynamicStyle(first), 0);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 1);
    CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
        666u,
        shared.styleCount() + 0u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(uniforms[0].topColor, Color4{0.375f});

    /* Allocate the other dynamic style if testing the case where the other
       animation has none */
    if(data.noFreeDynamicStyles)
        layer.allocateDynamicStyle();

    /* Next advance plays the other animation affecting the same data. If
       there's no dynamic style left, it updates the index to the initial style
       instead. The first animation thus no longer affects the data anymore. */
    CORRADE_COMPARE(advance(10_nsec, uniforms, dataStyles), BaseLayerStyleAnimatorUpdate::Uniform|BaseLayerStyleAnimatorUpdate::Style);
    CORRADE_COMPARE(animator.dynamicStyle(first), 0);
    CORRADE_COMPARE(animator.dynamicStyle(second), data.noFreeDynamicStyles ? Containers::NullOpt : Containers::optional(1u));
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 2);
    CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
        666u,
        data.noFreeDynamicStyles ? 2u : shared.styleCount() + 1u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(uniforms[0].topColor, Color4{0.5f});

    /* Next advance either finishes or discards & removes the first animation
       and recycles its dynamic style, which allows the second animation to
       take over if it didn't have a dynamic style already. If the first
       animation isn't finishing yet and it's KeepOncePlayed, it's left
       untouched including its dynamic style. */
    CORRADE_COMPARE(advance(20_nsec, uniforms, dataStyles), BaseLayerStyleAnimatorUpdate::Uniform|(data.noFreeDynamicStyles && data.expectedSecondDynamicStyle ? BaseLayerStyleAnimatorUpdate::Style : BaseLayerStyleAnimatorUpdate{}));
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
        CORRADE_COMPARE(uniforms[*data.expectedSecondDynamicStyle].topColor, Color4{1.125f});
}

void BaseLayerStyleAnimatorTest::advanceExternalStyleChanges() {
    auto&& data = AdvanceExternalStyleChangesData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Verifies the logic in AbstractVisualLayerStyleAnimator that deals with
       externally changed styles. There's nothing in BaseLayerStyleAnimator
       that'd affect this, so it doesn't verify anything specific to it.
       Similar test case is in TextLayerStyleAnimatorTest. */

    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{BaseLayer::Shared::Configuration{1, 10}
        .setDynamicStyleCount(1)
    };

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    BaseLayerStyleAnimator animator{animatorHandle(0, 1)};
    layer.assignAnimator(animator);

    /* Create extra data & animations just to ensure it's not always targeting
       the first index in various corner cases */
    layer.create(2);
    DataHandle data1 = layer.create(1);
    DataHandle data2 = layer.create(4);
    animator.create(0, 1, Animation::Easing::linear, Nanoseconds::max(), 40_nsec, data2);
    /* This one is actually getting animated */
    AnimationHandle animation = animator.create(
        7, 3, Animation::Easing::linear, 7_nsec, 15_nsec,
        data.noAttachment ? DataHandle::Null : data1,
        AnimationFlag::KeepOncePlayed);

    /* The style has all uniforms and paddings the same so all the advancing
       should do is just allocating dynamic styles and updating the style
       index */
    shared.setStyle(
        BaseLayerCommonStyleUniform{},
        {BaseLayerStyleUniform{}},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {});

    /* Does what layer's advanceAnimations() is doing internally for all
       animators (as we need to test also the interaction with animation being
       removed, etc.), but with an ability to peek into the filled data to
       verify they're written only when they should be. Compared to the helper
       in advance() above it exposes only style IDs. */
    const auto advance = [&](Nanoseconds time, const Containers::StridedArrayView1D<UnsignedInt>& dataStyles) {
        UnsignedByte activeStorage[1];
        UnsignedByte startedStorage[1];
        UnsignedByte stoppedStorage[1];
        Float factorStorage[2];
        UnsignedByte removeStorage[1];
        Vector4 paddings[1];
        BaseLayerStyleUniform uniforms[1];

        return animator.advance(time,
            Containers::MutableBitArrayView{activeStorage, 0, 2},
            Containers::MutableBitArrayView{startedStorage, 0, 2},
            Containers::MutableBitArrayView{stoppedStorage, 0, 2},
            factorStorage,
            Containers::MutableBitArrayView{removeStorage, 0, 2},
            uniforms, paddings, dataStyles);
    };

    Containers::Optional<UnsignedInt> dynamicStyle;
    if(data.allocateDynamicStyleBeforeStart)
        CORRADE_VERIFY((dynamicStyle = layer.allocateDynamicStyle()));

    UnsignedInt dataStyles[]{
        666,
        data.changeStyleBeforeStart ?
            *data.changeStyleBeforeStart : layer.style(data1),
        666
    };

    /* First advance start the animation, allocates a dynamic style and
       switches to it, if available. If no dynamic style is available, nothing
       is done. */
    CORRADE_COMPARE(advance(10_nsec, dataStyles), data.expectedUpdatesBegin);
    CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
        666u, data.expectedStyleBegin, 666u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), data.expectedDynamicStyleUsedCountBegin);

    if(data.freeDynamicStyleAfterStart) {
        layer.recycleDynamicStyle(*dynamicStyle);
        dynamicStyle = {};
    }
    if(data.changeStyleAfterStart)
        dataStyles[1] = *data.changeStyleAfterStart;

    /* Second advance allocates a dynamic style and switches to it if it didn't
       manage before and if there wasn't any change to the layer styles */
    CORRADE_COMPARE(advance(15_nsec, dataStyles), data.expectedUpdatesMiddle);
    CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
        666u, data.expectedStyleMiddle, 666u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), data.expectedDynamicStyleUsedCountMiddle);

    /* Third advance in the middle should cause no changes at all compared to
       the second, as nothing changed externally either */
    CORRADE_COMPARE(advance(20_nsec, dataStyles), BaseLayerStyleAnimatorUpdates{});
    CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
        666u, data.expectedStyleMiddle, 666u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), data.expectedDynamicStyleUsedCountMiddle);

    if(data.changeStyleBeforeStop)
        dataStyles[1] = *data.changeStyleBeforeStop;

    /* Fourth advance stops the animation, recycling the dynamic style (if any)
       and switches to the target style, unless changed */
    CORRADE_COMPARE(advance(25_nsec, dataStyles), data.expectedUpdatesEnd);
    CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
        666u, data.expectedStyleEnd, 666u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), data.expectedDynamicStyleUsedCountEnd);

    if(dynamicStyle)
        layer.recycleDynamicStyle(*dynamicStyle);

    /* Restarting the animation with a completely different style ID at start
       should pick that one up as the expected style and continue as usual */
    dataStyles[1] = data.noAttachment ? 666 : 2;
    animator.play(animation, 25_nsec);
    CORRADE_COMPARE(advance(30_nsec, dataStyles),
        (data.noAttachment ? BaseLayerStyleAnimatorUpdates{} : BaseLayerStyleAnimatorUpdate::Style)|
        BaseLayerStyleAnimatorUpdate::Uniform);
    CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
        666u, data.noAttachment ? 666u : 10u, 666u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 1);

    /* Same should happen with removal & recycling in the same animation
       slot */
    dataStyles[1] = data.noAttachment ? 666 : 2;
    animator.remove(animation);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 0);
    AnimationHandle animation2 = animator.create(
        7, 3, Animation::Easing::linear, 30_nsec, 10_nsec,
        data.noAttachment ? DataHandle::Null : data1,
        AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animationHandleId(animation2), animationHandleId(animation));
    CORRADE_COMPARE(advance(35_nsec, dataStyles),
        (data.noAttachment ? BaseLayerStyleAnimatorUpdates{} : BaseLayerStyleAnimatorUpdate::Style)|
        BaseLayerStyleAnimatorUpdate::Uniform);
    CORRADE_COMPARE_AS(Containers::arrayView(dataStyles), Containers::arrayView({
        666u, data.noAttachment ? 666u : 10u, 666u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 1);
}

void BaseLayerStyleAnimatorTest::advanceEmpty() {
    /* This should work even with no layer being set */
    BaseLayerStyleAnimator animator{animatorHandle(0, 1)};
    animator.advance({}, {}, {}, {}, {}, {}, {}, {}, {});

    CORRADE_VERIFY(true);
}

void BaseLayerStyleAnimatorTest::advanceInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{BaseLayer::Shared::Configuration{2}
        .setDynamicStyleCount(2)
    };

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
    BaseLayerStyleUniform dynamicStyleUniformsInvalid[3];
    Vector4 dynamicStylePaddings[2];
    Vector4 dynamicStylePaddingsInvalid[3];

    Containers::String out;
    Error redirectError{&out};
    animator.advance({}, mask, mask, mask, factors, maskInvalid, dynamicStyleUniforms, dynamicStylePaddings, {});
    animator.advance({}, mask, mask, mask, factorsInvalid, mask, dynamicStyleUniforms, dynamicStylePaddings, {});
    animator.advance({}, mask, mask, maskInvalid, factors, mask, dynamicStyleUniforms, dynamicStylePaddings, {});
    animator.advance({}, mask, maskInvalid, mask, factors, mask, dynamicStyleUniforms, dynamicStylePaddings, {});
    animator.advance({}, maskInvalid, mask, mask, factors, mask, dynamicStyleUniforms, dynamicStylePaddings, {});

    animator.advance({}, mask, mask, mask, factors, mask, dynamicStyleUniforms, dynamicStylePaddingsInvalid, {});
    animator.advance({}, mask, mask, mask, factors, mask, dynamicStyleUniformsInvalid, dynamicStylePaddings, {});
    /* All views correct but the layer doesn't have styles set */
    animator.advance({}, mask, mask, mask, factors, mask, dynamicStyleUniforms, dynamicStylePaddings, {});
    CORRADE_COMPARE_AS(out,
        /* These are caught by update() already, no need to repeat the
           assertion for the subclass. Verifying them here to ensure it doesn't
           accidentally blow up something earlier. */
        "Ui::AbstractAnimator::update(): expected active, started, stopped, factors and remove views to have a size of 3 but got 3, 3, 3, 3 and 4\n"
        "Ui::AbstractAnimator::update(): expected active, started, stopped, factors and remove views to have a size of 3 but got 3, 3, 3, 4 and 3\n"
        "Ui::AbstractAnimator::update(): expected active, started, stopped, factors and remove views to have a size of 3 but got 3, 3, 4, 3 and 3\n"
        "Ui::AbstractAnimator::update(): expected active, started, stopped, factors and remove views to have a size of 3 but got 3, 4, 3, 3 and 3\n"
        "Ui::AbstractAnimator::update(): expected active, started, stopped, factors and remove views to have a size of 3 but got 4, 3, 3, 3 and 3\n"

        "Ui::BaseLayerStyleAnimator::advance(): expected dynamic style uniform and padding views to have a size of 2 but got 2 and 3\n"
        "Ui::BaseLayerStyleAnimator::advance(): expected dynamic style uniform and padding views to have a size of 2 but got 3 and 2\n"
        "Ui::BaseLayerStyleAnimator::advance(): no style data was set on the layer\n",
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

    animator1.create(0, 1, Animation::Easing::linear, 0_nsec, 20_nsec, data2, AnimationFlag::KeepOncePlayed);
    animator2.create(1, 0, Animation::Easing::linear, 13_nsec, 1_nsec, data1);

    /* Set the style after animation creation to verify it isn't needed
       earlier */
    shared.setStyle(
        BaseLayerCommonStyleUniform{},
        {BaseLayerStyleUniform{}
            .setColor(Color4{0.25f}),
         BaseLayerStyleUniform{}
            .setColor(Color4{0.75f}),
         BaseLayerStyleUniform{}},
        {{}, Vector4{data.padding}, {}});

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
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 0);
    CORRADE_COMPARE(layer.style(data2), 1);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
    CORRADE_VERIFY(!layer.stateData().dynamicStyleChanged);
}

void BaseLayerStyleAnimatorTest::uiAdvance() {
    /* Verifies that removing a data with an animation attached properly cleans
       the attached dynamic style (if there's any) in
       AbstractVisualLayerStyleAnimator::doClean() */

    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{BaseLayer::Shared::Configuration{3}
        .setDynamicStyleCount(1)
    };
    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    };

    AbstractUserInterface ui{{100, 100}};

    BaseLayer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared));

    Containers::Pointer<BaseLayerStyleAnimator> animatorInstance{InPlaceInit, ui.createAnimator()};
    layer.assignAnimator(*animatorInstance);
    BaseLayerStyleAnimator& animator = ui.setStyleAnimatorInstance(Utility::move(animatorInstance));

    DataHandle data = layer.create(2);

    /* Creating animations doesn't allocate dynamic styles just yet, only
       advance() does */
    AnimationHandle withoutDynamicStyle = animator.create(0, 1, Animation::Easing::linear, 10_nsec, 10_nsec, data);
    AnimationHandle withDynamicStyle = animator.create(1, 0, Animation::Easing::linear, 0_nsec, 10_nsec, data);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 0);
    CORRADE_COMPARE(animator.usedCount(), 2);

    /* Set the style after animation creation to verify it isn't needed
       earlier */
    shared.setStyle(
        BaseLayerCommonStyleUniform{},
        {BaseLayerStyleUniform{}
            .setColor(Color4{0.25f}),
         BaseLayerStyleUniform{}
            .setColor(Color4{0.75f}),
         BaseLayerStyleUniform{}},
         {});

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

CORRADE_TEST_MAIN(Magnum::Ui::Test::BaseLayerStyleAnimatorTest)
