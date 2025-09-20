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
#include <Corrade/Containers/Function.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/String.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/String.h>

#include "Magnum/Animation/Easing.h"
#include "Magnum/Math/Time.h"
#include "Magnum/Ui/AbstractLayer.h"
#include "Magnum/Ui/GenericAnimator.h"
#include "Magnum/Ui/Handle.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct GenericAnimatorTest: TestSuite::Tester {
    explicit GenericAnimatorTest();

    void debugAnimationState();
    void debugAnimationStates();

    void construct();
    void constructNode();
    void constructData();
    void constructCopy();
    void constructCopyNode();
    void constructCopyData();
    void constructMove();
    void constructMoveNode();
    void constructMoveData();

    void createRemove();
    void createRemoveNode();
    void createRemoveData();
    void createRemoveHandleRecycle();
    void createRemoveHandleRecycleNode();
    void createRemoveHandleRecycleData();
    void createInvalid();
    void createInvalidNode();
    void createInvalidData();
    /* There's no assert to trigger in remove() other than what's checked by
       AbstractAnimator::remove() already */
    void propertiesInvalid();
    void propertiesInvalidNode();
    void propertiesInvalidData();

    void clean();
    void cleanNode();
    void cleanData();

    void advance();
    void advanceNode();
    void advanceData();
    void advanceOnce();
    void advanceOnceNode();
    void advanceOnceData();
    void advanceEmpty();
    void advanceEmptyNode();
    void advanceEmptyData();
};

using namespace Math::Literals;

const struct {
    const char* name;
    bool states;
    bool once;
    bool implicitRepeat;
} CreateRemoveCleanData[]{
    {"", false, false, false},
    {"implicit repeat count", false, false, true},
    {"state overload", true, false, false},
    {"state overload, implicit repeat count", true, false, true},
    {"call once variant", false, true, true},
};

const struct {
    const char* name;
    AnimationFlags flags;
    bool states;
    Nanoseconds startFirst, durationFirst;
    Float expectedFactorFirst;
    GenericAnimationStates expectedStatesFirst;
    Nanoseconds startThird, durationThird;
    Float expectedFactorThird;
    GenericAnimationStates expectedStatesThird;
} AdvanceData[]{
    {"",
        {}, false,
        0_nsec, 20_nsec, 75.0f, {},
        0_nsec, 60_nsec, 25.0f, {}},
    {"state overload",
        {}, true,
        0_nsec, 20_nsec, 75.0f, {},
        0_nsec, 60_nsec, 25.0f, {}},
    {"state overload, reverse",
        AnimationFlag::Reverse, true,
        0_nsec, 20_nsec, 25.0f, {},
        0_nsec, 60_nsec, 75.0f, {}},
    {"state overload, first stopped, third started",
        {}, true,
        0_nsec, 10_nsec, 100.0f, GenericAnimationState::Stopped|GenericAnimationState::End,
        10_nsec, 20_nsec, 25.0f, GenericAnimationState::Started|GenericAnimationState::Begin},
    {"state overload, first stopped, third started, reverse",
        AnimationFlag::Reverse, true,
        0_nsec, 10_nsec, 0.0f, GenericAnimationState::Stopped|GenericAnimationState::Reverse|GenericAnimationState::Begin,
        10_nsec, 20_nsec, 75.0f, GenericAnimationState::Started|GenericAnimationState::Reverse|GenericAnimationState::End},
    {"state overload, first started & stopped",
        {}, true,
        5_nsec, 5_nsec, 100.0f,
        GenericAnimationState::Started|GenericAnimationState::Stopped|GenericAnimationState::Begin|GenericAnimationState::End,
        0_nsec, 60_nsec, 25.0f, {}},
    {"state overload, first started & stopped, reverse",
        AnimationFlag::Reverse, true,
        5_nsec, 5_nsec, 0.0f, GenericAnimationState::Started|GenericAnimationState::Stopped|GenericAnimationState::Reverse|GenericAnimationState::Begin|GenericAnimationState::End,
        0_nsec, 60_nsec, 75.0f, {}},
    {"state overload, third started & stopped",
        {}, true,
        0_nsec, 20_nsec, 75.0f, {},
        5_nsec, 5_nsec, 100.0f, GenericAnimationState::Started|GenericAnimationState::Stopped|GenericAnimationState::Begin|GenericAnimationState::End},
    {"state overload, third started & stopped, reverse",
        AnimationFlag::Reverse, true,
        0_nsec, 20_nsec, 25.0f, {},
        5_nsec, 5_nsec, 0.0f, GenericAnimationState::Started|GenericAnimationState::Stopped|GenericAnimationState::Reverse|GenericAnimationState::Begin|GenericAnimationState::End},
};

const struct {
    const char* name;
    AnimationFlags flags;
    Nanoseconds startFirst, startThird;
    Int expected;
} AdvanceOnceData[]{
    {"neither started or stopped",
        {}, 20_nsec, 30_nsec, 1},
    /* Since the animation is zero-length, there's no way to have an animation
       just started or just stopped */
    {"first started & stopped",
        {}, 5_nsec, 30_nsec, 2},
    /* Reversing has no effect on anything */
    {"first started & stopped, reverse",
        AnimationFlag::Reverse, 5_nsec, 30_nsec, 2},
    {"third started & stopped",
        {}, 20_nsec, 5_nsec, 3},
    /* Reversing has no effect on anything, same as above */
    {"third started & stopped, reverse",
        AnimationFlag::Reverse, 20_nsec, 5_nsec, 3},
    {"first & third started & stopped",
        {}, 5_nsec, 10_nsec, 6},
    /* Reversing has no effect on anything, same as above */
    {"first & third started & stopped, reverse",
        AnimationFlag::Reverse, 5_nsec, 10_nsec, 6},
};

GenericAnimatorTest::GenericAnimatorTest() {
    addTests({&GenericAnimatorTest::debugAnimationState,
              &GenericAnimatorTest::debugAnimationStates,

              &GenericAnimatorTest::construct,
              &GenericAnimatorTest::constructNode,
              &GenericAnimatorTest::constructData,
              &GenericAnimatorTest::constructCopy,
              &GenericAnimatorTest::constructCopyNode,
              &GenericAnimatorTest::constructCopyData,
              &GenericAnimatorTest::constructMove,
              &GenericAnimatorTest::constructMoveNode,
              &GenericAnimatorTest::constructMoveData});

    addInstancedTests({&GenericAnimatorTest::createRemove,
                       &GenericAnimatorTest::createRemoveNode,
                       &GenericAnimatorTest::createRemoveData,
                       &GenericAnimatorTest::createRemoveHandleRecycle,
                       &GenericAnimatorTest::createRemoveHandleRecycleNode,
                       &GenericAnimatorTest::createRemoveHandleRecycleData},
        Containers::arraySize(CreateRemoveCleanData));

    addTests({&GenericAnimatorTest::createInvalid,
              &GenericAnimatorTest::createInvalidNode,
              &GenericAnimatorTest::createInvalidData,
              &GenericAnimatorTest::propertiesInvalid,
              &GenericAnimatorTest::propertiesInvalidNode,
              &GenericAnimatorTest::propertiesInvalidData});

    addInstancedTests({&GenericAnimatorTest::clean,
                       &GenericAnimatorTest::cleanNode,
                       &GenericAnimatorTest::cleanData},
        Containers::arraySize(CreateRemoveCleanData));

    addInstancedTests({&GenericAnimatorTest::advance,
                       &GenericAnimatorTest::advanceNode,
                       &GenericAnimatorTest::advanceData},
        Containers::arraySize(AdvanceData));

    addInstancedTests({&GenericAnimatorTest::advanceOnce,
                       &GenericAnimatorTest::advanceOnceNode,
                       &GenericAnimatorTest::advanceOnceData},
        Containers::arraySize(AdvanceOnceData));

    addTests({&GenericAnimatorTest::advanceEmpty,
              &GenericAnimatorTest::advanceEmptyNode,
              &GenericAnimatorTest::advanceEmptyData});
}

void GenericAnimatorTest::debugAnimationState() {
    Containers::String out;
    Debug{&out} << GenericAnimationState::Stopped << GenericAnimationState(0xbe);
    CORRADE_COMPARE(out, "Ui::GenericAnimationState::Stopped Ui::GenericAnimationState(0xbe)\n");
}

void GenericAnimatorTest::debugAnimationStates() {
    Containers::String out;
    Debug{&out} << (GenericAnimationState::Started|GenericAnimationState::Stopped|GenericAnimationState(0x80)) << GenericAnimationStates{};
    CORRADE_COMPARE(out, "Ui::GenericAnimationState::Started|Ui::GenericAnimationState::Stopped|Ui::GenericAnimationState(0x80) Ui::GenericAnimationStates{}\n");
}

void GenericAnimatorTest::construct() {
    GenericAnimator animator{animatorHandle(0xab, 0x12)};

    CORRADE_COMPARE(animator.features(), AnimatorFeatures{});
    CORRADE_COMPARE(animator.handle(), animatorHandle(0xab, 0x12));
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 0);
    /* The rest is the same as in AbstractAnimatorTest::constructGeneric() */
}

void GenericAnimatorTest::constructNode() {
    GenericNodeAnimator animator{animatorHandle(0xab, 0x12)};

    CORRADE_COMPARE(animator.features(), AnimatorFeature::NodeAttachment);
    CORRADE_COMPARE(animator.handle(), animatorHandle(0xab, 0x12));
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 0);
    /* The rest is the same as in AbstractAnimatorTest::constructGeneric() */
}

void GenericAnimatorTest::constructData() {
    GenericDataAnimator animator{animatorHandle(0xab, 0x12)};

    CORRADE_COMPARE(animator.features(), AnimatorFeature::DataAttachment);
    CORRADE_COMPARE(animator.handle(), animatorHandle(0xab, 0x12));
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 0);
    CORRADE_COMPARE(animator.layer(), LayerHandle::Null);
    /* The rest is the same as in AbstractAnimatorTest::constructGeneric() */

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0xcd, 0x34)};
    animator.setLayer(layer);
    CORRADE_COMPARE(animator.layer(), layerHandle(0xcd, 0x34));
}

void GenericAnimatorTest::constructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<GenericAnimator>{});
    CORRADE_VERIFY(!std::is_copy_assignable<GenericAnimator>{});
}

void GenericAnimatorTest::constructCopyNode() {
    CORRADE_VERIFY(!std::is_copy_constructible<GenericNodeAnimator>{});
    CORRADE_VERIFY(!std::is_copy_assignable<GenericNodeAnimator>{});
}

void GenericAnimatorTest::constructCopyData() {
    CORRADE_VERIFY(!std::is_copy_constructible<GenericDataAnimator>{});
    CORRADE_VERIFY(!std::is_copy_assignable<GenericDataAnimator>{});
}

void GenericAnimatorTest::constructMove() {
    /* Just verify that the subclass doesn't have the moves broken */

    GenericAnimator a{animatorHandle(0xab, 0x12)};

    GenericAnimator b{Utility::move(a)};
    CORRADE_COMPARE(b.handle(), animatorHandle(0xab, 0x12));

    GenericAnimator c{animatorHandle(0xcd, 0x34)};
    c = Utility::move(b);
    CORRADE_COMPARE(c.handle(), animatorHandle(0xab, 0x12));

    CORRADE_VERIFY(std::is_nothrow_move_constructible<GenericAnimator>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<GenericAnimator>::value);
}

void GenericAnimatorTest::constructMoveNode() {
    /* Just verify that the subclass doesn't have the moves broken */

    GenericNodeAnimator a{animatorHandle(0xab, 0x12)};

    GenericNodeAnimator b{Utility::move(a)};
    CORRADE_COMPARE(b.handle(), animatorHandle(0xab, 0x12));

    GenericNodeAnimator c{animatorHandle(0xcd, 0x34)};
    c = Utility::move(b);
    CORRADE_COMPARE(c.handle(), animatorHandle(0xab, 0x12));

    CORRADE_VERIFY(std::is_nothrow_move_constructible<GenericNodeAnimator>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<GenericNodeAnimator>::value);
}

void GenericAnimatorTest::constructMoveData() {
    /* Just verify that the subclass doesn't have the moves broken */

    GenericDataAnimator a{animatorHandle(0xab, 0x12)};

    GenericDataAnimator b{Utility::move(a)};
    CORRADE_COMPARE(b.handle(), animatorHandle(0xab, 0x12));

    GenericDataAnimator c{animatorHandle(0xcd, 0x34)};
    c = Utility::move(b);
    CORRADE_COMPARE(c.handle(), animatorHandle(0xab, 0x12));

    CORRADE_VERIFY(std::is_nothrow_move_constructible<GenericDataAnimator>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<GenericDataAnimator>::value);
}

void GenericAnimatorTest::createRemove() {
    auto&& data = CreateRemoveCleanData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    Int destructedCount = 0;
    struct NonTrivial {
        explicit NonTrivial(int& output): destructedCount{&output} {}
        ~NonTrivial() {
            ++*destructedCount;
        }
        void operator()(Float) const {
            CORRADE_FAIL("This should never be called.");
        }
        void operator()() const { /* used for callOnce() */
            CORRADE_FAIL("This should never be called.");
        }

        Int* destructedCount;
    };
    struct NonTrivialStates {
        explicit NonTrivialStates(int& output): destructedCount{&output} {}
        ~NonTrivialStates() {
            ++*destructedCount;
        }
        void operator()(Float, GenericAnimationStates) const {
            CORRADE_FAIL("This should never be called.");
        }

        Int* destructedCount;
    };

    GenericAnimator animator{animatorHandle(0, 1)};

    AnimationHandle trivial;
    if(data.once)
        trivial = animator.callOnce([]{
            CORRADE_FAIL("This should never be called.");
        }, 137_nsec, AnimationFlag::KeepOncePlayed);
    else if(data.states) {
        if(data.implicitRepeat)
            trivial = animator.create([](Float, GenericAnimationStates) {
                CORRADE_FAIL("This should never be called.");
            }, Animation::Easing::bounceOut, 137_nsec, 277_nsec, AnimationFlag::KeepOncePlayed);
        else
            trivial = animator.create([](Float, GenericAnimationStates) {
                CORRADE_FAIL("This should never be called.");
            }, Animation::Easing::bounceOut, 137_nsec, 277_nsec, 3, AnimationFlag::KeepOncePlayed);
    } else {
        if(data.implicitRepeat)
            trivial = animator.create([](Float) {
                CORRADE_FAIL("This should never be called.");
            }, Animation::Easing::bounceOut, 137_nsec, 277_nsec, AnimationFlag::KeepOncePlayed);
        else
            trivial = animator.create([](Float) {
                CORRADE_FAIL("This should never be called.");
            }, Animation::Easing::bounceOut, 137_nsec, 277_nsec, 3, AnimationFlag::KeepOncePlayed);
    }
    CORRADE_COMPARE(animator.usedCount(), 1);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 0);
    CORRADE_COMPARE(animator.started(trivial), 137_nsec);
    CORRADE_COMPARE(animator.duration(trivial), data.once ? 0_nsec : 277_nsec);
    CORRADE_COMPARE(animator.repeatCount(trivial), data.implicitRepeat ? 1 : 3);
    CORRADE_COMPARE(animator.flags(trivial), AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.easing(trivial), data.once ? nullptr : Animation::Easing::bounceOut);

    /* The temporary gets destructed right away */
    AnimationHandle nonTrivial;
    if(data.once)
        nonTrivial = animator.callOnce(NonTrivial{destructedCount}, 226_nsec, AnimationFlags{0x80});
    else if(data.states) {
        if(data.implicitRepeat)
            nonTrivial = animator.create(NonTrivialStates{destructedCount}, Animation::Easing::smootherstep, 226_nsec, 191_nsec, AnimationFlags{0x80});
        else
            nonTrivial = animator.create(NonTrivialStates{destructedCount}, Animation::Easing::smootherstep, 226_nsec, 191_nsec, 0, AnimationFlags{0x80});
    } else {
        if(data.implicitRepeat)
            nonTrivial = animator.create(NonTrivial{destructedCount}, Animation::Easing::smootherstep, 226_nsec, 191_nsec, AnimationFlags{0x80});
        else
            nonTrivial = animator.create(NonTrivial{destructedCount}, Animation::Easing::smootherstep, 226_nsec, 191_nsec, 0, AnimationFlags{0x80});
    }
    CORRADE_COMPARE(animator.usedCount(), 2);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 1);
    CORRADE_COMPARE(animator.started(nonTrivial), 226_nsec);
    CORRADE_COMPARE(animator.duration(nonTrivial), data.once ? 0_nsec : 191_nsec);
    CORRADE_COMPARE(animator.repeatCount(nonTrivial), data.implicitRepeat ? 1 : 0);
    CORRADE_COMPARE(animator.flags(nonTrivial), AnimationFlags{0x80});
    /* Testing also the other overload. The other getters are tested in
       AbstractAnimatorTest already. */
    CORRADE_COMPARE(animator.easing(animationHandleData(nonTrivial)), data.once ? nullptr : Animation::Easing::smootherstep);
    CORRADE_COMPARE(destructedCount, 1);

    animator.remove(trivial);
    CORRADE_COMPARE(animator.usedCount(), 1);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 1);
    CORRADE_COMPARE(destructedCount, 1);

    /* Verifying also the other handle overload. They should both delegate into
       the same internal implementation. */
    animator.remove(animationHandleData(nonTrivial));
    CORRADE_COMPARE(animator.usedCount(), 0);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 0);
    CORRADE_COMPARE(destructedCount, 2);
}

void GenericAnimatorTest::createRemoveNode() {
    auto&& data = CreateRemoveCleanData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    Int destructedCount = 0;
    struct NonTrivial {
        explicit NonTrivial(int& output): destructedCount{&output} {}
        ~NonTrivial() {
            ++*destructedCount;
        }
        void operator()(NodeHandle, Float) const {
            CORRADE_FAIL("This should never be called.");
        }
        void operator()(NodeHandle) const { /* used for callOnce() */
            CORRADE_FAIL("This should never be called.");
        }

        Int* destructedCount;
    };
    struct NonTrivialStates {
        explicit NonTrivialStates(int& output): destructedCount{&output} {}
        ~NonTrivialStates() {
            ++*destructedCount;
        }
        void operator()(NodeHandle, Float, GenericAnimationStates) const {
            CORRADE_FAIL("This should never be called.");
        }

        Int* destructedCount;
    };

    GenericNodeAnimator animator{animatorHandle(0, 1)};

    AnimationHandle trivial;
    if(data.once)
        trivial = animator.callOnce([](NodeHandle) {
            CORRADE_FAIL("This should never be called.");
        }, 137_nsec, nodeHandle(0x12345, 0xabc), AnimationFlag::KeepOncePlayed);
    else if(data.states) {
        if(data.implicitRepeat)
            trivial = animator.create([](NodeHandle, Float, GenericAnimationStates) {
                CORRADE_FAIL("This should never be called.");
            }, Animation::Easing::bounceOut, 137_nsec, 277_nsec, nodeHandle(0x12345, 0xabc), AnimationFlag::KeepOncePlayed);
        else
            trivial = animator.create([](NodeHandle, Float, GenericAnimationStates) {
                CORRADE_FAIL("This should never be called.");
            }, Animation::Easing::bounceOut, 137_nsec, 277_nsec, nodeHandle(0x12345, 0xabc), 3, AnimationFlag::KeepOncePlayed);
    } else {
        if(data.implicitRepeat)
            trivial = animator.create([](NodeHandle, Float) {
                CORRADE_FAIL("This should never be called.");
            }, Animation::Easing::bounceOut, 137_nsec, 277_nsec, nodeHandle(0x12345, 0xabc), AnimationFlag::KeepOncePlayed);
        else
            trivial = animator.create([](NodeHandle, Float) {
                CORRADE_FAIL("This should never be called.");
            }, Animation::Easing::bounceOut, 137_nsec, 277_nsec, nodeHandle(0x12345, 0xabc), 3, AnimationFlag::KeepOncePlayed);
    }
    CORRADE_COMPARE(animator.usedCount(), 1);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 0);
    CORRADE_COMPARE(animator.started(trivial), 137_nsec);
    CORRADE_COMPARE(animator.duration(trivial), data.once ? 0_nsec : 277_nsec);
    CORRADE_COMPARE(animator.repeatCount(trivial), data.implicitRepeat ? 1 : 3);
    CORRADE_COMPARE(animator.flags(trivial), AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.node(trivial), nodeHandle(0x12345, 0xabc));
    CORRADE_COMPARE(animator.easing(trivial), data.once ? nullptr : Animation::Easing::bounceOut);

    /* The temporary gets destructed right away */
    AnimationHandle nonTrivial;
    if(data.once)
        nonTrivial = animator.callOnce(NonTrivial{destructedCount}, 226_nsec, nodeHandle(0x67890, 0xdef), AnimationFlags{0x80});
    else if(data.states) {
        if(data.implicitRepeat)
            nonTrivial = animator.create(NonTrivialStates{destructedCount}, Animation::Easing::smootherstep, 226_nsec, 191_nsec, nodeHandle(0x67890, 0xdef), AnimationFlags{0x80});
        else
            nonTrivial = animator.create(NonTrivialStates{destructedCount}, Animation::Easing::smootherstep, 226_nsec, 191_nsec, nodeHandle(0x67890, 0xdef), 0, AnimationFlags{0x80});
    } else {
        if(data.implicitRepeat)
            nonTrivial = animator.create(NonTrivial{destructedCount}, Animation::Easing::smootherstep, 226_nsec, 191_nsec, nodeHandle(0x67890, 0xdef), AnimationFlags{0x80});
        else
            nonTrivial = animator.create(NonTrivial{destructedCount}, Animation::Easing::smootherstep, 226_nsec, 191_nsec, nodeHandle(0x67890, 0xdef), 0, AnimationFlags{0x80});
    }
    CORRADE_COMPARE(animator.usedCount(), 2);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 1);
    CORRADE_COMPARE(animator.started(nonTrivial), 226_nsec);
    CORRADE_COMPARE(animator.duration(nonTrivial), data.once ? 0_nsec : 191_nsec);
    CORRADE_COMPARE(animator.repeatCount(nonTrivial), data.implicitRepeat ? 1 : 0);
    CORRADE_COMPARE(animator.flags(nonTrivial), AnimationFlags{0x80});
    CORRADE_COMPARE(animator.node(nonTrivial), nodeHandle(0x67890, 0xdef));
    /* Testing also the other overload. The other getters are tested in
       AbstractAnimatorTest already. */
    CORRADE_COMPARE(animator.easing(animationHandleData(nonTrivial)), data.once ? nullptr : Animation::Easing::smootherstep);
    CORRADE_COMPARE(destructedCount, 1);

    animator.remove(trivial);
    CORRADE_COMPARE(animator.usedCount(), 1);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 1);
    CORRADE_COMPARE(destructedCount, 1);

    /* Verifying also the other handle overload. They should both delegate into
       the same internal implementation. */
    animator.remove(animationHandleData(nonTrivial));
    CORRADE_COMPARE(animator.usedCount(), 0);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 0);
    CORRADE_COMPARE(destructedCount, 2);
}

void GenericAnimatorTest::createRemoveData() {
    auto&& data = CreateRemoveCleanData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    Int destructedCount = 0;
    struct NonTrivial {
        explicit NonTrivial(int& output): destructedCount{&output} {}
        ~NonTrivial() {
            ++*destructedCount;
        }
        void operator()(DataHandle, Float) const {
            CORRADE_FAIL("This should never be called.");
        }
        void operator()(DataHandle) const { /* used for callOnce() */
            CORRADE_FAIL("This should never be called.");
        }

        Int* destructedCount;
    };
    struct NonTrivialStates {
        explicit NonTrivialStates(int& output): destructedCount{&output} {}
        ~NonTrivialStates() {
            ++*destructedCount;
        }
        void operator()(DataHandle, Float, GenericAnimationStates) const {
            CORRADE_FAIL("This should never be called.");
        }

        Int* destructedCount;
    };

    GenericDataAnimator animator{animatorHandle(0, 1)};

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0xcd, 0x34)};
    animator.setLayer(layer);

    AnimationHandle trivial;
    if(data.once)
        trivial = animator.callOnce([](DataHandle) {
            CORRADE_FAIL("This should never be called.");
        }, 137_nsec, dataHandle(layer.handle(), 0x12345, 0xabc), AnimationFlag::KeepOncePlayed);
    else if(data.states) {
        if(data.implicitRepeat)
            trivial = animator.create([](DataHandle, Float, GenericAnimationStates) {
                CORRADE_FAIL("This should never be called.");
            }, Animation::Easing::bounceOut, 137_nsec, 277_nsec, dataHandle(layer.handle(), 0x12345, 0xabc), AnimationFlag::KeepOncePlayed);
        else
            trivial = animator.create([](DataHandle, Float, GenericAnimationStates) {
                CORRADE_FAIL("This should never be called.");
            }, Animation::Easing::bounceOut, 137_nsec, 277_nsec, dataHandle(layer.handle(), 0x12345, 0xabc), 3, AnimationFlag::KeepOncePlayed);
    } else {
        if(data.implicitRepeat)
            trivial = animator.create([](DataHandle, Float) {
                CORRADE_FAIL("This should never be called.");
            }, Animation::Easing::bounceOut, 137_nsec, 277_nsec, dataHandle(layer.handle(), 0x12345, 0xabc), AnimationFlag::KeepOncePlayed);
        else
            trivial = animator.create([](DataHandle, Float) {
                CORRADE_FAIL("This should never be called.");
            }, Animation::Easing::bounceOut, 137_nsec, 277_nsec, dataHandle(layer.handle(), 0x12345, 0xabc), 3, AnimationFlag::KeepOncePlayed);
    }
    CORRADE_COMPARE(animator.usedCount(), 1);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 0);
    CORRADE_COMPARE(animator.started(trivial), 137_nsec);
    CORRADE_COMPARE(animator.duration(trivial), data.once ? 0_nsec : 277_nsec);
    CORRADE_COMPARE(animator.repeatCount(trivial), data.implicitRepeat ? 1 : 3);
    CORRADE_COMPARE(animator.flags(trivial), AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.data(trivial), dataHandle(layer.handle(), 0x12345, 0xabc));
    CORRADE_COMPARE(animator.easing(trivial), data.once ? nullptr : Animation::Easing::bounceOut);

    /* The temporary gets destructed right away. Testing also the
       LayerDataHandle overload. */
    AnimationHandle nonTrivial;
    if(data.once)
        nonTrivial = animator.callOnce(NonTrivial{destructedCount}, 226_nsec, layerDataHandle(0x67890, 0xdef), AnimationFlags{0x80});
    else if(data.states) {
        if(data.implicitRepeat)
            nonTrivial = animator.create(NonTrivialStates{destructedCount}, Animation::Easing::smootherstep, 226_nsec, 191_nsec, layerDataHandle(0x67890, 0xdef), AnimationFlags{0x80});
        else
            nonTrivial = animator.create(NonTrivialStates{destructedCount}, Animation::Easing::smootherstep, 226_nsec, 191_nsec, layerDataHandle(0x67890, 0xdef), 0, AnimationFlags{0x80});
    } else {
        if(data.implicitRepeat)
            nonTrivial = animator.create(NonTrivial{destructedCount}, Animation::Easing::smootherstep, 226_nsec, 191_nsec, layerDataHandle(0x67890, 0xdef), AnimationFlags{0x80});
        else
            nonTrivial = animator.create(NonTrivial{destructedCount}, Animation::Easing::smootherstep, 226_nsec, 191_nsec, layerDataHandle(0x67890, 0xdef), 0, AnimationFlags{0x80});
    }
    CORRADE_COMPARE(animator.usedCount(), 2);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 1);
    CORRADE_COMPARE(animator.started(nonTrivial), 226_nsec);
    CORRADE_COMPARE(animator.duration(nonTrivial), data.once ? 0_nsec : 191_nsec);
    CORRADE_COMPARE(animator.repeatCount(nonTrivial), data.implicitRepeat ? 1 : 0);
    CORRADE_COMPARE(animator.flags(nonTrivial), AnimationFlags{0x80});
    CORRADE_COMPARE(animator.data(nonTrivial), dataHandle(layer.handle(), 0x67890, 0xdef));
    /* Testing also the other overload. The other getters are tested in
       AbstractAnimatorTest already. */
    CORRADE_COMPARE(animator.easing(animationHandleData(nonTrivial)), data.once ? nullptr : Animation::Easing::smootherstep);
    CORRADE_COMPARE(destructedCount, 1);

    animator.remove(trivial);
    CORRADE_COMPARE(animator.usedCount(), 1);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 1);
    CORRADE_COMPARE(destructedCount, 1);

    /* Verifying also the other handle overload. They should both delegate into
       the same internal implementation. */
    animator.remove(animationHandleData(nonTrivial));
    CORRADE_COMPARE(animator.usedCount(), 0);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 0);
    CORRADE_COMPARE(destructedCount, 2);
}

void GenericAnimatorTest::createRemoveHandleRecycle() {
    auto&& data = CreateRemoveCleanData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    Int destructedCount1 = 0,
        destructedCount2 = 0;
    struct NonTrivial {
        explicit NonTrivial(int& output): destructedCount{&output} {}
        ~NonTrivial() {
            ++*destructedCount;
        }
        void operator()(Float) const {
            CORRADE_FAIL("This should never be called.");
        }
        void operator()() const { /* used for callOnce() */
            CORRADE_FAIL("This should never be called.");
        }

        Int* destructedCount;
    };
    struct NonTrivialStates {
        explicit NonTrivialStates(int& output): destructedCount{&output} {}
        ~NonTrivialStates() {
            ++*destructedCount;
        }
        void operator()(Float, GenericAnimationStates) const {
            CORRADE_FAIL("This should never be called.");
        }

        Int* destructedCount;
    };

    GenericAnimator animator{animatorHandle(0, 1)};
    animator.create([](Float){}, Animation::Easing::linear, 0_nsec, 1_nsec);

    /* The temporary gets destructed right away. Not using the States overload
       here to verify that switching overloads works as well. Removing a States
       overload is tested in createRemove() already. */
    AnimationHandle second = animator.create(NonTrivial{destructedCount1}, Animation::Easing::linear, 0_nsec, 1_nsec);
    CORRADE_COMPARE(destructedCount1, 1);

    animator.remove(second);
    CORRADE_COMPARE(destructedCount1, 2);

    /* Animation that reuses a previous slot should not call the destructor
       on the previous function again or some such crazy stuff */
    AnimationHandle second2;
    if(data.once)
        second2 = animator.callOnce(NonTrivial{destructedCount2}, 0_nsec);
    else if(data.states)
        second2 = animator.create(NonTrivialStates{destructedCount2}, Animation::Easing::step, 0_nsec, 1_nsec);
    else
        second2 = animator.create(NonTrivial{destructedCount2}, Animation::Easing::step, 0_nsec, 1_nsec);
    CORRADE_COMPARE(animationHandleId(second2), animationHandleId(second));
    CORRADE_COMPARE(destructedCount1, 2);
    CORRADE_COMPARE(destructedCount2, 1);
}

void GenericAnimatorTest::createRemoveHandleRecycleNode() {
    auto&& data = CreateRemoveCleanData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    Int destructedCount1 = 0,
        destructedCount2 = 0;
    struct NonTrivial {
        explicit NonTrivial(int& output): destructedCount{&output} {}
        ~NonTrivial() {
            ++*destructedCount;
        }
        void operator()(NodeHandle, Float) const {
            CORRADE_FAIL("This should never be called.");
        }
        void operator()(NodeHandle) const { /* used for callOnce() */
            CORRADE_FAIL("This should never be called.");
        }

        Int* destructedCount;
    };
    struct NonTrivialStates {
        explicit NonTrivialStates(int& output): destructedCount{&output} {}
        ~NonTrivialStates() {
            ++*destructedCount;
        }
        void operator()(NodeHandle, Float, GenericAnimationStates) const {
            CORRADE_FAIL("This should never be called.");
        }

        Int* destructedCount;
    };

    GenericNodeAnimator animator{animatorHandle(0, 1)};
    animator.create([](NodeHandle, Float){}, Animation::Easing::linear, 0_nsec, 1_nsec, NodeHandle::Null);

    /* The temporary gets destructed right away. Not using the States overload
       here to verify that switching overloads works as well. Removing a States
       overload is tested in createRemoveNode() already. */
    AnimationHandle second = animator.create(NonTrivial{destructedCount1}, Animation::Easing::linear, 0_nsec, 1_nsec, NodeHandle::Null);
    CORRADE_COMPARE(destructedCount1, 1);

    animator.remove(second);
    CORRADE_COMPARE(destructedCount1, 2);

    /* Animation that reuses a previous slot should not call the destructor
       on the previous function again or some such crazy stuff */
    AnimationHandle second2;
    if(data.once)
        second2 = animator.callOnce(NonTrivial{destructedCount2}, 0_nsec, NodeHandle::Null);
    else if(data.states)
        second2 = animator.create(NonTrivialStates{destructedCount2}, Animation::Easing::step, 0_nsec, 1_nsec, NodeHandle::Null);
    else
        second2 = animator.create(NonTrivial{destructedCount2}, Animation::Easing::step, 0_nsec, 1_nsec, NodeHandle::Null);
    CORRADE_COMPARE(animationHandleId(second2), animationHandleId(second));
    CORRADE_COMPARE(destructedCount1, 2);
    CORRADE_COMPARE(destructedCount2, 1);
}

void GenericAnimatorTest::createRemoveHandleRecycleData() {
    auto&& data = CreateRemoveCleanData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    Int destructedCount1 = 0,
        destructedCount2 = 0;
    struct NonTrivial {
        explicit NonTrivial(int& output): destructedCount{&output} {}
        ~NonTrivial() {
            ++*destructedCount;
        }
        void operator()(DataHandle, Float) const {
            CORRADE_FAIL("This should never be called.");
        }
        void operator()(DataHandle) const { /* used for callOnce() */
            CORRADE_FAIL("This should never be called.");
        }

        Int* destructedCount;
    };
    struct NonTrivialStates {
        explicit NonTrivialStates(int& output): destructedCount{&output} {}
        ~NonTrivialStates() {
            ++*destructedCount;
        }
        void operator()(DataHandle, Float, GenericAnimationStates) const {
            CORRADE_FAIL("This should never be called.");
        }

        Int* destructedCount;
    };

    GenericDataAnimator animator{animatorHandle(0, 1)};

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0xcd, 0x34)};
    animator.setLayer(layer);

    animator.create([](DataHandle, Float){}, Animation::Easing::linear, 0_nsec, 1_nsec, DataHandle::Null);

    /* The temporary gets destructed right away. Not using the States overload
       here to verify that switching overloads works as well. Removing a States
       overload is tested in createRemoveData() already. */
    AnimationHandle second = animator.create(NonTrivial{destructedCount1}, Animation::Easing::linear, 0_nsec, 1_nsec, DataHandle::Null);
    CORRADE_COMPARE(destructedCount1, 1);

    animator.remove(second);
    CORRADE_COMPARE(destructedCount1, 2);

    /* Animation that reuses a previous slot should not call the destructor
       on the previous function again or some such crazy stuff */
    AnimationHandle second2;
    if(data.once)
        second2 = animator.callOnce(NonTrivial{destructedCount2}, 0_nsec, DataHandle::Null);
    else if(data.states)
        second2 = animator.create(NonTrivialStates{destructedCount2}, Animation::Easing::step, 0_nsec, 1_nsec, DataHandle::Null);
    else
        second2 = animator.create(NonTrivial{destructedCount2}, Animation::Easing::step, 0_nsec, 1_nsec, DataHandle::Null);
    CORRADE_COMPARE(animationHandleId(second2), animationHandleId(second));
    CORRADE_COMPARE(destructedCount1, 2);
    CORRADE_COMPARE(destructedCount2, 1);
}

void GenericAnimatorTest::createInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    GenericAnimator animator{animatorHandle(0, 1)};

    Containers::String out;
    Error redirectError{&out};
    animator.create(static_cast<void(*)(Float)>(nullptr), Animation::Easing::linear, 0_nsec, 1_nsec, 0, AnimationFlags{});
    animator.create(static_cast<void(*)(Float)>(nullptr), Animation::Easing::linear, 0_nsec, 1_nsec, AnimationFlags{});
    animator.create(static_cast<void(*)(Float, GenericAnimationStates)>(nullptr), Animation::Easing::linear, 0_nsec, 1_nsec, 0, AnimationFlags{});
    animator.create(static_cast<void(*)(Float, GenericAnimationStates)>(nullptr), Animation::Easing::linear, 0_nsec, 1_nsec, AnimationFlags{});
    animator.callOnce(nullptr, 0_nsec);
    animator.create([](Float) {}, nullptr, 0_nsec, 1_nsec, 0, AnimationFlags{});
    animator.create([](Float) {}, nullptr, 0_nsec, 1_nsec, AnimationFlags{});
    animator.create([](Float, GenericAnimationStates) {}, nullptr, 0_nsec, 1_nsec, 0, AnimationFlags{});
    animator.create([](Float, GenericAnimationStates) {}, nullptr, 0_nsec, 1_nsec, AnimationFlags{});
    CORRADE_COMPARE_AS(out,
        "Ui::GenericAnimator::create(): animation is null\n"
        "Ui::GenericAnimator::create(): animation is null\n"
        "Ui::GenericAnimator::create(): animation is null\n"
        "Ui::GenericAnimator::create(): animation is null\n"
        "Ui::GenericAnimator::callOnce(): callback is null\n"
        "Ui::GenericAnimator::create(): easing is null\n"
        "Ui::GenericAnimator::create(): easing is null\n"
        "Ui::GenericAnimator::create(): easing is null\n"
        "Ui::GenericAnimator::create(): easing is null\n",
        TestSuite::Compare::String);
}

void GenericAnimatorTest::createInvalidNode() {
    CORRADE_SKIP_IF_NO_ASSERT();

    GenericNodeAnimator animator{animatorHandle(0, 1)};

    Containers::String out;
    Error redirectError{&out};
    animator.create(static_cast<void(*)(NodeHandle, Float)>(nullptr), Animation::Easing::linear, 0_nsec, 1_nsec, NodeHandle::Null, 0, AnimationFlags{});
    animator.create(static_cast<void(*)(NodeHandle, Float)>(nullptr), Animation::Easing::linear, 0_nsec, 1_nsec, NodeHandle::Null, AnimationFlags{});
    animator.create(static_cast<void(*)(NodeHandle, Float, GenericAnimationStates)>(nullptr), Animation::Easing::linear, 0_nsec, 1_nsec, NodeHandle::Null, 0, AnimationFlags{});
    animator.create(static_cast<void(*)(NodeHandle, Float, GenericAnimationStates)>(nullptr), Animation::Easing::linear, 0_nsec, 1_nsec, NodeHandle::Null, AnimationFlags{});
    animator.callOnce(nullptr, 0_nsec, NodeHandle::Null);
    animator.create([](NodeHandle, Float) {}, nullptr, 0_nsec, 1_nsec, NodeHandle::Null, 0, AnimationFlags{});
    animator.create([](NodeHandle, Float) {}, nullptr, 0_nsec, 1_nsec, NodeHandle::Null, AnimationFlags{});
    animator.create([](NodeHandle, Float, GenericAnimationStates) {}, nullptr, 0_nsec, 1_nsec, NodeHandle::Null, 0, AnimationFlags{});
    animator.create([](NodeHandle, Float, GenericAnimationStates) {}, nullptr, 0_nsec, 1_nsec, NodeHandle::Null, AnimationFlags{});
    CORRADE_COMPARE_AS(out,
        "Ui::GenericNodeAnimator::create(): animation is null\n"
        "Ui::GenericNodeAnimator::create(): animation is null\n"
        "Ui::GenericNodeAnimator::create(): animation is null\n"
        "Ui::GenericNodeAnimator::create(): animation is null\n"
        "Ui::GenericNodeAnimator::callOnce(): callback is null\n"
        "Ui::GenericNodeAnimator::create(): easing is null\n"
        "Ui::GenericNodeAnimator::create(): easing is null\n"
        "Ui::GenericNodeAnimator::create(): easing is null\n"
        "Ui::GenericNodeAnimator::create(): easing is null\n",
        TestSuite::Compare::String);
}

void GenericAnimatorTest::createInvalidData() {
    CORRADE_SKIP_IF_NO_ASSERT();

    GenericDataAnimator animator{animatorHandle(0, 1)};

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0, 1)};
    animator.setLayer(layer);

    Containers::String out;
    Error redirectError{&out};
    animator.create(static_cast<void(*)(DataHandle, Float)>(nullptr), Animation::Easing::linear, 0_nsec, 1_nsec, DataHandle::Null, 0, AnimationFlags{});
    animator.create(static_cast<void(*)(DataHandle, Float)>(nullptr), Animation::Easing::linear, 0_nsec, 1_nsec, DataHandle::Null, AnimationFlags{});
    animator.create(static_cast<void(*)(DataHandle, Float, GenericAnimationStates)>(nullptr), Animation::Easing::linear, 0_nsec, 1_nsec, DataHandle::Null, 0, AnimationFlags{});
    animator.create(static_cast<void(*)(DataHandle, Float, GenericAnimationStates)>(nullptr), Animation::Easing::linear, 0_nsec, 1_nsec, DataHandle::Null, AnimationFlags{});
    animator.create(static_cast<void(*)(DataHandle, Float)>(nullptr), Animation::Easing::linear, 0_nsec, 1_nsec, LayerDataHandle::Null, 0, AnimationFlags{});
    animator.create(static_cast<void(*)(DataHandle, Float)>(nullptr), Animation::Easing::linear, 0_nsec, 1_nsec, LayerDataHandle::Null, AnimationFlags{});
    animator.create(static_cast<void(*)(DataHandle, Float, GenericAnimationStates)>(nullptr), Animation::Easing::linear, 0_nsec, 1_nsec, LayerDataHandle::Null, 0, AnimationFlags{});
    animator.create(static_cast<void(*)(DataHandle, Float, GenericAnimationStates)>(nullptr), Animation::Easing::linear, 0_nsec, 1_nsec, LayerDataHandle::Null, AnimationFlags{});
    animator.callOnce(nullptr, 0_nsec, DataHandle::Null);
    animator.callOnce(nullptr, 0_nsec, LayerDataHandle::Null);
    animator.create([](DataHandle, Float) {}, nullptr, 0_nsec, 1_nsec, DataHandle::Null, 0, AnimationFlags{});
    animator.create([](DataHandle, Float) {}, nullptr, 0_nsec, 1_nsec, DataHandle::Null, AnimationFlags{});
    animator.create([](DataHandle, Float, GenericAnimationStates) {}, nullptr, 0_nsec, 1_nsec, DataHandle::Null, 0, AnimationFlags{});
    animator.create([](DataHandle, Float, GenericAnimationStates) {}, nullptr, 0_nsec, 1_nsec, DataHandle::Null, AnimationFlags{});
    animator.create([](DataHandle, Float) {}, nullptr, 0_nsec, 1_nsec, LayerDataHandle::Null, 0, AnimationFlags{});
    animator.create([](DataHandle, Float) {}, nullptr, 0_nsec, 1_nsec, LayerDataHandle::Null, AnimationFlags{});
    animator.create([](DataHandle, Float, GenericAnimationStates) {}, nullptr, 0_nsec, 1_nsec, LayerDataHandle::Null, 0, AnimationFlags{});
    animator.create([](DataHandle, Float, GenericAnimationStates) {}, nullptr, 0_nsec, 1_nsec, LayerDataHandle::Null, AnimationFlags{});
    CORRADE_COMPARE_AS(out,
        "Ui::GenericDataAnimator::create(): animation is null\n"
        "Ui::GenericDataAnimator::create(): animation is null\n"
        "Ui::GenericDataAnimator::create(): animation is null\n"
        "Ui::GenericDataAnimator::create(): animation is null\n"
        "Ui::GenericDataAnimator::create(): animation is null\n"
        "Ui::GenericDataAnimator::create(): animation is null\n"
        "Ui::GenericDataAnimator::create(): animation is null\n"
        "Ui::GenericDataAnimator::create(): animation is null\n"
        "Ui::GenericDataAnimator::callOnce(): callback is null\n"
        "Ui::GenericDataAnimator::callOnce(): callback is null\n"
        "Ui::GenericDataAnimator::create(): easing is null\n"
        "Ui::GenericDataAnimator::create(): easing is null\n"
        "Ui::GenericDataAnimator::create(): easing is null\n"
        "Ui::GenericDataAnimator::create(): easing is null\n"
        "Ui::GenericDataAnimator::create(): easing is null\n"
        "Ui::GenericDataAnimator::create(): easing is null\n"
        "Ui::GenericDataAnimator::create(): easing is null\n"
        "Ui::GenericDataAnimator::create(): easing is null\n",
        TestSuite::Compare::String);
}

void GenericAnimatorTest::propertiesInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    GenericAnimator animator{animatorHandle(0, 1)};

    AnimationHandle handle = animator.create([](Float) {}, Animation::Easing::linear, 12_nsec, 13_nsec);

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
        "Ui::GenericAnimator::easing(): invalid handle Ui::AnimationHandle::Null\n"
        "Ui::GenericAnimator::easing(): invalid handle Ui::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Ui::GenericAnimator::easing(): invalid handle Ui::AnimationHandle(Null, {0x0, 0x1})\n"
        "Ui::GenericAnimator::easing(): invalid handle Ui::AnimatorDataHandle(0xabcde, 0x123)\n",
        TestSuite::Compare::String);
}

void GenericAnimatorTest::propertiesInvalidNode() {
    CORRADE_SKIP_IF_NO_ASSERT();

    GenericNodeAnimator animator{animatorHandle(0, 1)};

    AnimationHandle handle = animator.create([](NodeHandle, Float) {}, Animation::Easing::linear, 12_nsec, 13_nsec, NodeHandle::Null);

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
        "Ui::GenericNodeAnimator::easing(): invalid handle Ui::AnimationHandle::Null\n"
        "Ui::GenericNodeAnimator::easing(): invalid handle Ui::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Ui::GenericNodeAnimator::easing(): invalid handle Ui::AnimationHandle(Null, {0x0, 0x1})\n"
        "Ui::GenericNodeAnimator::easing(): invalid handle Ui::AnimatorDataHandle(0xabcde, 0x123)\n",
        TestSuite::Compare::String);
}

void GenericAnimatorTest::propertiesInvalidData() {
    CORRADE_SKIP_IF_NO_ASSERT();

    GenericDataAnimator animator{animatorHandle(0, 1)};

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0, 1)};
    animator.setLayer(layer);

    AnimationHandle handle = animator.create([](DataHandle, Float) {}, Animation::Easing::linear, 12_nsec, 13_nsec, DataHandle::Null);

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
        "Ui::GenericDataAnimator::easing(): invalid handle Ui::AnimationHandle::Null\n"
        "Ui::GenericDataAnimator::easing(): invalid handle Ui::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Ui::GenericDataAnimator::easing(): invalid handle Ui::AnimationHandle(Null, {0x0, 0x1})\n"
        "Ui::GenericDataAnimator::easing(): invalid handle Ui::AnimatorDataHandle(0xabcde, 0x123)\n",
        TestSuite::Compare::String);
}

void GenericAnimatorTest::clean() {
    auto&& data = CreateRemoveCleanData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    Int destructedCount = 0,
        anotherDestructedCount = 0;
    struct NonTrivial {
        explicit NonTrivial(int& output): destructedCount{&output} {}
        ~NonTrivial() {
            ++*destructedCount;
        }
        void operator()(Float) const {
            CORRADE_FAIL("This should never be called.");
        }
        void operator()() const { /* used for callOnce() */
            CORRADE_FAIL("This should never be called.");
        }

        Int* destructedCount;
    };
    struct NonTrivialStates {
        explicit NonTrivialStates(int& output): destructedCount{&output} {}
        ~NonTrivialStates() {
            ++*destructedCount;
        }
        void operator()(Float, GenericAnimationStates) const {
            CORRADE_FAIL("This should never be called.");
        }

        Int* destructedCount;
    };

    GenericAnimator animator{animatorHandle(0, 1)};

    AnimationHandle trivial = animator.create([](Float) {}, Animation::Easing::bounceOut, 137_nsec, 277_nsec, 3, AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.usedCount(), 1);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 0);

    /* The temporary gets destructed right away */
    AnimationHandle nonTrivial = animator.create(NonTrivial{destructedCount}, Animation::Easing::smootherstep, 226_nsec, 191_nsec, 0, AnimationFlags{0x80});
    CORRADE_COMPARE(animator.usedCount(), 2);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 1);
    CORRADE_COMPARE(destructedCount, 1);

    AnimationHandle another = animator.create([](Float) {}, Animation::Easing::bounceOut, 137_nsec, 277_nsec, 3, AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.usedCount(), 3);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 1);
    CORRADE_COMPARE(destructedCount, 1);

    /* The temporary gets destructed right away */
    AnimationHandle anotherNonTrivial;
    if(data.once)
        anotherNonTrivial = animator.callOnce(NonTrivial{anotherDestructedCount}, 226_nsec, AnimationFlags{0x80});
    else if(data.states)
        anotherNonTrivial = animator.create(NonTrivialStates{anotherDestructedCount}, Animation::Easing::smootherstep, 226_nsec, 191_nsec, 0, AnimationFlags{0x80});
    else
        anotherNonTrivial = animator.create(NonTrivial{anotherDestructedCount}, Animation::Easing::smootherstep, 226_nsec, 191_nsec, 0, AnimationFlags{0x80});
    CORRADE_COMPARE(animator.usedCount(), 4);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 2);
    CORRADE_COMPARE(anotherDestructedCount, 1);

    /* It should remove two but call just one destructor */
    UnsignedByte animationIdsToRemove[]{(1 << 0)|(1 << 3)};
    animator.clean(Containers::BitArrayView{animationIdsToRemove, 0, 4});
    CORRADE_COMPARE(animator.usedCount(), 2);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 1);
    CORRADE_COMPARE(destructedCount, 1);
    CORRADE_COMPARE(anotherDestructedCount, 2);
    CORRADE_VERIFY(!animator.isHandleValid(trivial));
    CORRADE_VERIFY(animator.isHandleValid(nonTrivial));
    CORRADE_VERIFY(animator.isHandleValid(another));
    CORRADE_VERIFY(!animator.isHandleValid(anotherNonTrivial));
}

void GenericAnimatorTest::cleanNode() {
    auto&& data = CreateRemoveCleanData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    Int destructedCount = 0,
        anotherDestructedCount = 0;
    struct NonTrivial {
        explicit NonTrivial(int& output): destructedCount{&output} {}
        ~NonTrivial() {
            ++*destructedCount;
        }
        void operator()(NodeHandle, Float) const {
            CORRADE_FAIL("This should never be called.");
        }
        void operator()(NodeHandle) const { /* used for callOnce() */
            CORRADE_FAIL("This should never be called.");
        }

        Int* destructedCount;
    };
    struct NonTrivialStates {
        explicit NonTrivialStates(int& output): destructedCount{&output} {}
        ~NonTrivialStates() {
            ++*destructedCount;
        }
        void operator()(NodeHandle, Float, GenericAnimationStates) const {
            CORRADE_FAIL("This should never be called.");
        }

        Int* destructedCount;
    };

    GenericNodeAnimator animator{animatorHandle(0, 1)};

    AnimationHandle trivial = animator.create([](NodeHandle, Float) {}, Animation::Easing::bounceOut, 137_nsec, 277_nsec, NodeHandle::Null, 3, AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.usedCount(), 1);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 0);

    /* The temporary gets destructed right away */
    AnimationHandle nonTrivial = animator.create(NonTrivial{destructedCount}, Animation::Easing::smootherstep, 226_nsec, 191_nsec, NodeHandle::Null, 0, AnimationFlags{0x80});
    CORRADE_COMPARE(animator.usedCount(), 2);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 1);
    CORRADE_COMPARE(destructedCount, 1);

    AnimationHandle another = animator.create([](NodeHandle, Float) {}, Animation::Easing::bounceOut, 137_nsec, 277_nsec, NodeHandle::Null, 3, AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.usedCount(), 3);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 1);
    CORRADE_COMPARE(destructedCount, 1);

    /* The temporary gets destructed right away */
    AnimationHandle anotherNonTrivial;
    if(data.once)
        anotherNonTrivial = animator.callOnce(NonTrivial{anotherDestructedCount}, 226_nsec, NodeHandle::Null, AnimationFlags{0x80});
    else if(data.states)
        anotherNonTrivial = animator.create(NonTrivialStates{anotherDestructedCount}, Animation::Easing::smootherstep, 226_nsec, 191_nsec, NodeHandle::Null, 0, AnimationFlags{0x80});
    else
        anotherNonTrivial = animator.create(NonTrivial{anotherDestructedCount}, Animation::Easing::smootherstep, 226_nsec, 191_nsec, NodeHandle::Null, 0, AnimationFlags{0x80});
    CORRADE_COMPARE(animator.usedCount(), 4);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 2);
    CORRADE_COMPARE(anotherDestructedCount, 1);

    /* It should remove two but call just one destructor */
    UnsignedByte animationIdsToRemove[]{(1 << 0)|(1 << 3)};
    animator.clean(Containers::BitArrayView{animationIdsToRemove, 0, 4});
    CORRADE_COMPARE(animator.usedCount(), 2);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 1);
    CORRADE_COMPARE(destructedCount, 1);
    CORRADE_COMPARE(anotherDestructedCount, 2);
    CORRADE_VERIFY(!animator.isHandleValid(trivial));
    CORRADE_VERIFY(animator.isHandleValid(nonTrivial));
    CORRADE_VERIFY(animator.isHandleValid(another));
    CORRADE_VERIFY(!animator.isHandleValid(anotherNonTrivial));
}

void GenericAnimatorTest::cleanData() {
    auto&& data = CreateRemoveCleanData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    Int destructedCount = 0,
        anotherDestructedCount = 0;
    struct NonTrivial {
        explicit NonTrivial(int& output): destructedCount{&output} {}
        ~NonTrivial() {
            ++*destructedCount;
        }
        void operator()(DataHandle, Float) const {
            CORRADE_FAIL("This should never be called.");
        }
        void operator()(DataHandle) const { /* used for callOnce() */
            CORRADE_FAIL("This should never be called.");
        }

        Int* destructedCount;
    };
    struct NonTrivialStates {
        explicit NonTrivialStates(int& output): destructedCount{&output} {}
        ~NonTrivialStates() {
            ++*destructedCount;
        }
        void operator()(DataHandle, Float, GenericAnimationStates) const {
            CORRADE_FAIL("This should never be called.");
        }

        Int* destructedCount;
    };

    GenericDataAnimator animator{animatorHandle(0, 1)};

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0, 1)};
    animator.setLayer(layer);

    AnimationHandle trivial = animator.create([](DataHandle, Float) {}, Animation::Easing::bounceOut, 137_nsec, 277_nsec, DataHandle::Null, 3, AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.usedCount(), 1);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 0);

    /* The temporary gets destructed right away */
    AnimationHandle nonTrivial = animator.create(NonTrivial{destructedCount}, Animation::Easing::smootherstep, 226_nsec, 191_nsec, DataHandle::Null, 0, AnimationFlags{0x80});
    CORRADE_COMPARE(animator.usedCount(), 2);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 1);
    CORRADE_COMPARE(destructedCount, 1);

    AnimationHandle another = animator.create([](DataHandle, Float) {}, Animation::Easing::bounceOut, 137_nsec, 277_nsec, DataHandle::Null, 3, AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.usedCount(), 3);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 1);
    CORRADE_COMPARE(destructedCount, 1);

    /* The temporary gets destructed right away */
    AnimationHandle anotherNonTrivial;
    if(data.once)
        anotherNonTrivial = animator.callOnce(NonTrivial{anotherDestructedCount}, 226_nsec, DataHandle::Null, AnimationFlags{0x80});
    else if(data.states)
        anotherNonTrivial = animator.create(NonTrivialStates{anotherDestructedCount}, Animation::Easing::smootherstep, 226_nsec, 191_nsec, DataHandle::Null, 0, AnimationFlags{0x80});
    else
        anotherNonTrivial = animator.create(NonTrivial{anotherDestructedCount}, Animation::Easing::smootherstep, 226_nsec, 191_nsec, DataHandle::Null, 0, AnimationFlags{0x80});
    CORRADE_COMPARE(animator.usedCount(), 4);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 2);
    CORRADE_COMPARE(anotherDestructedCount, 1);

    /* It should remove two but call just one destructor */
    UnsignedByte animationIdsToRemove[]{(1 << 0)|(1 << 3)};
    animator.clean(Containers::BitArrayView{animationIdsToRemove, 0, 4});
    CORRADE_COMPARE(animator.usedCount(), 2);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 1);
    CORRADE_COMPARE(destructedCount, 1);
    CORRADE_COMPARE(anotherDestructedCount, 2);
    CORRADE_VERIFY(!animator.isHandleValid(trivial));
    CORRADE_VERIFY(animator.isHandleValid(nonTrivial));
    CORRADE_VERIFY(animator.isHandleValid(another));
    CORRADE_VERIFY(!animator.isHandleValid(anotherNonTrivial));
}

Float hundredTimes(Float value) { return value*100; }

void GenericAnimatorTest::advance() {
    auto&& data = AdvanceData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    GenericAnimator animator{animatorHandle(0, 1)};

    Float first = 0.0f;
    GenericAnimationStates statesFirst;
    if(data.states)
        animator.create([&first, &statesFirst](Float factor, GenericAnimationStates states) {
            first += factor;
            statesFirst |= states;
        }, hundredTimes, data.startFirst, data.durationFirst, data.flags);
    else
        animator.create([&first](Float factor) {
            first += factor;
        }, hundredTimes, data.startFirst, data.durationFirst, data.flags);

    if(data.states)
        animator.create([](Float, GenericAnimationStates) {
            CORRADE_FAIL("This shouldn't be called");
        }, hundredTimes, 20_nsec, 10_nsec, data.flags);
    else
        animator.create([](Float) {
            CORRADE_FAIL("This shouldn't be called");
        }, hundredTimes, 20_nsec, 10_nsec, data.flags);

    Float third = 0.0f;
    GenericAnimationStates statesThird;
    if(data.states)
        animator.create([&third, &statesThird](Float factor, GenericAnimationStates states) {
            third += factor;
            statesThird |= states;
        }, hundredTimes, data.startThird, data.durationThird, data.flags);
    else
        animator.create([&third](Float factor) {
            third += factor;
        }, hundredTimes, data.startThird, data.durationThird, data.flags);

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::BitArray activeStorage{NoInit, 3};
    Containers::BitArray startedStorage{NoInit, 3};
    Containers::BitArray stoppedStorage{NoInit, 3};
    Float factorStorage[3];
    Containers::BitArray removeStorage{NoInit, 3};

    /* Advance at 0 so it's possible to even have a state that has neither
       Started nor Stopped set, clean everything after */
    animator.advance(0_nsec,
        activeStorage,
        startedStorage,
        stoppedStorage,
        factorStorage,
        removeStorage);
    first = 0.0f;
    third = 0.0f;
    statesFirst = {};
    statesThird = {};

    /* Should call just the first and third with appropriate factors */
    animator.advance(15_nsec,
        activeStorage,
        startedStorage,
        stoppedStorage,
        factorStorage,
        removeStorage);
    CORRADE_COMPARE(first, data.expectedFactorFirst);
    CORRADE_COMPARE(third, data.expectedFactorThird);
    /* Comparing unconditionally to verify that we actually use the other
       overloads as well */
    CORRADE_COMPARE(statesFirst, data.expectedStatesFirst);
    CORRADE_COMPARE(statesThird, data.expectedStatesThird);
}

void GenericAnimatorTest::advanceNode() {
    auto&& data = AdvanceData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    GenericNodeAnimator animator{animatorHandle(0, 1)};

    Float first = 0.0f;
    GenericAnimationStates statesFirst;
    if(data.states)
        animator.create([&first, &statesFirst](NodeHandle node, Float factor, GenericAnimationStates states) {
            CORRADE_COMPARE(node, nodeHandle(0xabcde, 0x123));
            first += factor;
            statesFirst |= states;
        }, hundredTimes, data.startFirst, data.durationFirst, nodeHandle(0xabcde, 0x123), data.flags);
    else
        animator.create([&first](NodeHandle node, Float factor) {
            CORRADE_COMPARE(node, nodeHandle(0xabcde, 0x123));
            first += factor;
        }, hundredTimes, data.startFirst, data.durationFirst, nodeHandle(0xabcde, 0x123), data.flags);

    if(data.states)
        animator.create([](NodeHandle, Float) {
            CORRADE_FAIL("This shouldn't be called");
        }, hundredTimes, 20_nsec, 10_nsec, nodeHandle(0xedcba, 0x321), data.flags);
    else
        animator.create([](NodeHandle, Float) {
            CORRADE_FAIL("This shouldn't be called");
        }, hundredTimes, 20_nsec, 10_nsec, nodeHandle(0xedcba, 0x321), data.flags);

    Float third = 0.0f;
    GenericAnimationStates statesThird;
    if(data.states)
        animator.create([&third, &statesThird](NodeHandle node, Float factor, GenericAnimationStates states) {
            CORRADE_COMPARE(node, NodeHandle::Null);
            third += factor;
            statesThird |= states;
        }, hundredTimes, data.startThird, data.durationThird, NodeHandle::Null, data.flags);
    else
        animator.create([&third](NodeHandle node, Float factor) {
            CORRADE_COMPARE(node, NodeHandle::Null);
            third += factor;
        }, hundredTimes, data.startThird, data.durationThird, NodeHandle::Null, data.flags);

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::BitArray activeStorage{NoInit, 3};
    Containers::BitArray startedStorage{NoInit, 3};
    Containers::BitArray stoppedStorage{NoInit, 3};
    Float factorStorage[3];
    Containers::BitArray removeStorage{NoInit, 3};

    /* Advance at 0 so it's possible to even have a state that has neither
       Started nor Stopped set, clean everything after */
    animator.advance(0_nsec,
        activeStorage,
        startedStorage,
        stoppedStorage,
        factorStorage,
        removeStorage);
    first = 0.0f;
    third = 0.0f;
    statesFirst = {};
    statesThird = {};

    /* Should call just the first and third with appropriate factors */
    animator.advance(15_nsec,
        activeStorage,
        startedStorage,
        stoppedStorage,
        factorStorage,
        removeStorage);
    CORRADE_COMPARE(first, data.expectedFactorFirst);
    CORRADE_COMPARE(third, data.expectedFactorThird);
    /* Comparing unconditionally to verify that we actually use the other
       overloads as well */
    CORRADE_COMPARE(statesFirst, data.expectedStatesFirst);
    CORRADE_COMPARE(statesThird, data.expectedStatesThird);
}

void GenericAnimatorTest::advanceData() {
    auto&& data = AdvanceData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    GenericDataAnimator animator{animatorHandle(0, 1)};

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0xab, 0xcd)};
    animator.setLayer(layer);

    Float first = 0.0f;
    GenericAnimationStates statesFirst;
    if(data.states)
        animator.create([&first, &statesFirst](DataHandle data, Float factor, GenericAnimationStates states) {
            CORRADE_COMPARE(data, dataHandle(layerHandle(0xab, 0xcd), 0xabcde, 0x123));
            first += factor;
            statesFirst |= states;
        }, hundredTimes, data.startFirst, data.durationFirst, dataHandle(layer.handle(), 0xabcde, 0x123), data.flags);
    else
        animator.create([&first](DataHandle data, Float factor) {
            CORRADE_COMPARE(data, dataHandle(layerHandle(0xab, 0xcd), 0xabcde, 0x123));
            first += factor;
        }, hundredTimes, data.startFirst, data.durationFirst, dataHandle(layer.handle(), 0xabcde, 0x123), data.flags);

    if(data.states)
        animator.create([](DataHandle, Float, GenericAnimationStates) {
            CORRADE_FAIL("This shouldn't be called");
        }, hundredTimes, 20_nsec, 10_nsec, dataHandle(layer.handle(), 0xedcba, 0x321), data.flags);
    else
        animator.create([](DataHandle, Float) {
            CORRADE_FAIL("This shouldn't be called");
        }, hundredTimes, 20_nsec, 10_nsec, dataHandle(layer.handle(), 0xedcba, 0x321), data.flags);

    Float third = 0.0f;
    GenericAnimationStates statesThird;
    if(data.states)
        animator.create([&third, &statesThird](DataHandle data, Float factor, GenericAnimationStates states) {
            /* If there's no associated data, the layer handle shouldn't be
               added to the null LayerDataHandle */
            CORRADE_COMPARE(data, DataHandle::Null);
            third += factor;
            statesThird |= states;
        }, hundredTimes, data.startThird, data.durationThird, LayerDataHandle::Null, data.flags);
    else
        animator.create([&third](DataHandle data, Float factor) {
            /* If there's no associated data, the layer handle shouldn't be
               added to the null LayerDataHandle */
            CORRADE_COMPARE(data, DataHandle::Null);
            third += factor;
        }, hundredTimes, data.startThird, data.durationThird, LayerDataHandle::Null, data.flags);

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::BitArray activeStorage{NoInit, 3};
    Containers::BitArray startedStorage{NoInit, 3};
    Containers::BitArray stoppedStorage{NoInit, 3};
    Float factorStorage[3];
    Containers::BitArray removeStorage{NoInit, 3};

    /* Advance at 0 so it's possible to even have a state that has neither
       Started nor Stopped set, clean everything after */
    animator.advance(0_nsec,
        activeStorage,
        startedStorage,
        stoppedStorage,
        factorStorage,
        removeStorage);
    first = 0.0f;
    third = 0.0f;
    statesFirst = {};
    statesThird = {};

    /* Should call just the first and third with appropriate factors */
    animator.advance(15_nsec,
        activeStorage,
        startedStorage,
        stoppedStorage,
        factorStorage,
        removeStorage);
    CORRADE_COMPARE(first, data.expectedFactorFirst);
    CORRADE_COMPARE(third, data.expectedFactorThird);
    /* Comparing unconditionally to verify that we actually use the other
       overloads as well */
    CORRADE_COMPARE(statesFirst, data.expectedStatesFirst);
    CORRADE_COMPARE(statesThird, data.expectedStatesThird);
}

void GenericAnimatorTest::advanceOnce() {
    auto&& data = AdvanceOnceData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    GenericAnimator animator{animatorHandle(0, 1)};

    Int called = 1;
    animator.callOnce([&called]() {
        called *= 2;
    }, data.startFirst, data.flags);

    animator.callOnce([]() {
        CORRADE_FAIL("This shouldn't be called");
    }, 20_nsec, data.flags);

    animator.callOnce([&called]() {
        called *= 3;
    }, data.startThird, data.flags);

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::BitArray activeStorage{NoInit, 3};
    Containers::BitArray startedStorage{NoInit, 3};
    Containers::BitArray stoppedStorage{NoInit, 3};
    Float factorStorage[3];
    Containers::BitArray removeStorage{NoInit, 3};

    /* Advance at 0 so it's possible to even have a state that has neither
       Started nor Stopped set, clean everything after */
    animator.advance(0_nsec,
        activeStorage,
        startedStorage,
        stoppedStorage,
        factorStorage,
        removeStorage);
    called = 1;

    /* Should call just the first and third. Factors are not used, only the
       started/stopped bits should affect the output. */
    animator.advance(15_nsec,
        activeStorage,
        startedStorage,
        stoppedStorage,
        factorStorage,
        removeStorage);
    CORRADE_COMPARE(called, data.expected);
}

void GenericAnimatorTest::advanceOnceNode() {
    auto&& data = AdvanceOnceData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    GenericNodeAnimator animator{animatorHandle(0, 1)};

    Int called = 1;
    animator.callOnce([&called](NodeHandle node) {
        CORRADE_COMPARE(node, nodeHandle(0xabcde, 0x123));
        called *= 2;
    }, data.startFirst, nodeHandle(0xabcde, 0x123), data.flags);

    animator.callOnce([](NodeHandle) {
        CORRADE_FAIL("This shouldn't be called");
    }, 20_nsec, nodeHandle(0xedcba, 0x321), data.flags);

    animator.callOnce([&called](NodeHandle node) {
        CORRADE_COMPARE(node, NodeHandle::Null);
        called *= 3;
    }, data.startThird, NodeHandle::Null, data.flags);

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::BitArray activeStorage{NoInit, 3};
    Containers::BitArray startedStorage{NoInit, 3};
    Containers::BitArray stoppedStorage{NoInit, 3};
    Float factorStorage[3];
    Containers::BitArray removeStorage{NoInit, 3};

    /* Advance at 0 so it's possible to even have a state that has neither
       Started nor Stopped set, clean everything after */
    animator.advance(0_nsec,
        activeStorage,
        startedStorage,
        stoppedStorage,
        factorStorage,
        removeStorage);
    called = 1;

    /* Should call just the first and third. Factors are not used, only the
       started/stopped bits should affect the output. */
    animator.advance(15_nsec,
        activeStorage,
        startedStorage,
        stoppedStorage,
        factorStorage,
        removeStorage);
    CORRADE_COMPARE(called, data.expected);
}

void GenericAnimatorTest::advanceOnceData() {
    auto&& data = AdvanceOnceData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    GenericDataAnimator animator{animatorHandle(0, 1)};

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0xab, 0xcd)};
    animator.setLayer(layer);

    Int called = 1;
    animator.callOnce([&called](DataHandle data) {
        CORRADE_COMPARE(data, dataHandle(layerHandle(0xab, 0xcd), 0xabcde, 0x123));
        called *= 2;
    }, data.startFirst, dataHandle(layer.handle(), 0xabcde, 0x123), data.flags);

    animator.callOnce([](DataHandle) {
        CORRADE_FAIL("This shouldn't be called");
    }, 20_nsec, dataHandle(layer.handle(), 0xedcba, 0x321), data.flags);

    animator.callOnce([&called](DataHandle data) {
        /* If there's no associated data, the layer handle shouldn't be added
           to the null LayerDataHandle */
        CORRADE_COMPARE(data, DataHandle::Null);
        called *= 3;
    }, data.startThird, LayerDataHandle::Null, data.flags);

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::BitArray activeStorage{NoInit, 3};
    Containers::BitArray startedStorage{NoInit, 3};
    Containers::BitArray stoppedStorage{NoInit, 3};
    Float factorStorage[3];
    Containers::BitArray removeStorage{NoInit, 3};

    /* Advance at 0 so it's possible to even have a state that has neither
       Started nor Stopped set, clean everything after */
    animator.advance(0_nsec,
        activeStorage,
        startedStorage,
        stoppedStorage,
        factorStorage,
        removeStorage);
    called = 1;

    /* Should call just the first and third. Factors are not used, only the
       started/stopped bits should affect the output. */
    animator.advance(15_nsec,
        activeStorage,
        startedStorage,
        stoppedStorage,
        factorStorage,
        removeStorage);
    CORRADE_COMPARE(called, data.expected);
}

void GenericAnimatorTest::advanceEmpty() {
    GenericAnimator animator{animatorHandle(0, 1)};
    animator.advance({}, {}, {}, {}, {}, {});

    /* Shouldn't crash or anything */
    CORRADE_VERIFY(true);
}

void GenericAnimatorTest::advanceEmptyNode() {
    GenericNodeAnimator animator{animatorHandle(0, 1)};
    animator.advance({}, {}, {}, {}, {}, {});

    /* Shouldn't crash or anything */
    CORRADE_VERIFY(true);
}

void GenericAnimatorTest::advanceEmptyData() {
    /* This should work even with no layer being set */
    GenericDataAnimator animator{animatorHandle(0, 1)};
    animator.advance({}, {}, {}, {}, {}, {});

    /* Shouldn't crash or anything */
    CORRADE_VERIFY(true);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::GenericAnimatorTest)
