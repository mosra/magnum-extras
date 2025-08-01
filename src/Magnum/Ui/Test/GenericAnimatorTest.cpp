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
} CreateRemoveCleanData[]{
    {"", false, false},
    {"state overload", true, false},
    {"call once variant", false, true},
};

const struct {
    const char* name;
    bool states;
    UnsignedByte started, stopped;
    GenericAnimationStates statesFirst, statesThird;
} AdvanceData[]{
    {"", false, 0, 0, {}, {}},
    {"state overload", true, 0, 0, {}, {}},
    {"state overload, first stopped, third started", true, 1 << 2, 1 << 0,
        GenericAnimationState::Stopped,
        GenericAnimationState::Started},
    {"state overload, first started & stopped", true, 1 << 0, 1 << 0,
        GenericAnimationState::Started|GenericAnimationState::Stopped,
        {}},
    /* Second isn't advanced at all, so this affects nothing */
    {"state overload, second started & stopped", true, 1 << 1, 1 << 1, {}, {}},
    {"state overload, third started & stopped", true, 1 << 2, 1 << 2,
        {},
        GenericAnimationState::Started|GenericAnimationState::Stopped},
};

const struct {
    const char* name;
    UnsignedByte started, stopped;
    Int expected;
} AdvanceOnceData[]{
    {"neither started or stopped", 0, 0, 1},
    /* Second isn't advanced at all, so this affects nothing */
    {"second started & stopped", 1 << 1, 1 << 1, 1},
    /* It reacts only to the stopped bit, so only third is called */
    {"first started, third stopped", 1 << 0, 1 << 2, 3},
    /* Having both the started and stopped bit should work also */
    {"first stopped, third started & stopped", 1 << 2, (1 << 0)|(1 << 2), 2*3},
    {"first started & stopped, third started", (1 << 0)|(1 << 3), 1 << 0, 2},
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
    else if(data.states)
        trivial = animator.create([](Float, GenericAnimationStates) {
            CORRADE_FAIL("This should never be called.");
        }, Animation::Easing::bounceOut, 137_nsec, 277_nsec, 3, AnimationFlag::KeepOncePlayed);
    else
        trivial = animator.create([](Float) {
            CORRADE_FAIL("This should never be called.");
        }, Animation::Easing::bounceOut, 137_nsec, 277_nsec, 3, AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.usedCount(), 1);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 0);
    CORRADE_COMPARE(animator.started(trivial), 137_nsec);
    CORRADE_COMPARE(animator.duration(trivial), data.once ? 0_nsec : 277_nsec);
    CORRADE_COMPARE(animator.repeatCount(trivial), data.once ? 1 : 3);
    CORRADE_COMPARE(animator.flags(trivial), AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.easing(trivial), data.once ? nullptr : Animation::Easing::bounceOut);

    /* The temporary gets destructed right away */
    AnimationHandle nonTrivial;
    if(data.once)
        nonTrivial = animator.callOnce(NonTrivial{destructedCount}, 226_nsec, AnimationFlags{0x80});
    else if(data.states)
        nonTrivial = animator.create(NonTrivialStates{destructedCount}, Animation::Easing::smootherstep, 226_nsec, 191_nsec, 0, AnimationFlags{0x80});
    else
        nonTrivial = animator.create(NonTrivial{destructedCount}, Animation::Easing::smootherstep, 226_nsec, 191_nsec, 0, AnimationFlags{0x80});
    CORRADE_COMPARE(animator.usedCount(), 2);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 1);
    CORRADE_COMPARE(animator.started(nonTrivial), 226_nsec);
    CORRADE_COMPARE(animator.duration(nonTrivial), data.once ? 0_nsec : 191_nsec);
    CORRADE_COMPARE(animator.repeatCount(nonTrivial), data.once ? 1 : 0);
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
    else if(data.states)
        trivial = animator.create([](NodeHandle, Float, GenericAnimationStates) {
            CORRADE_FAIL("This should never be called.");
        }, Animation::Easing::bounceOut, 137_nsec, 277_nsec, nodeHandle(0x12345, 0xabc), 3, AnimationFlag::KeepOncePlayed);
    else
        trivial = animator.create([](NodeHandle, Float) {
            CORRADE_FAIL("This should never be called.");
        }, Animation::Easing::bounceOut, 137_nsec, 277_nsec, nodeHandle(0x12345, 0xabc), 3, AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.usedCount(), 1);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 0);
    CORRADE_COMPARE(animator.started(trivial), 137_nsec);
    CORRADE_COMPARE(animator.duration(trivial), data.once ? 0_nsec : 277_nsec);
    CORRADE_COMPARE(animator.repeatCount(trivial), data.once ? 1 : 3);
    CORRADE_COMPARE(animator.flags(trivial), AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.node(trivial), nodeHandle(0x12345, 0xabc));
    CORRADE_COMPARE(animator.easing(trivial), data.once ? nullptr : Animation::Easing::bounceOut);

    /* The temporary gets destructed right away */
    AnimationHandle nonTrivial;
    if(data.once)
        nonTrivial = animator.callOnce(NonTrivial{destructedCount}, 226_nsec, nodeHandle(0x67890, 0xdef), AnimationFlags{0x80});
    else if(data.states)
        nonTrivial = animator.create(NonTrivialStates{destructedCount}, Animation::Easing::smootherstep, 226_nsec, 191_nsec, nodeHandle(0x67890, 0xdef), 0, AnimationFlags{0x80});
    else
        nonTrivial = animator.create(NonTrivial{destructedCount}, Animation::Easing::smootherstep, 226_nsec, 191_nsec, nodeHandle(0x67890, 0xdef), 0, AnimationFlags{0x80});
    CORRADE_COMPARE(animator.usedCount(), 2);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 1);
    CORRADE_COMPARE(animator.started(nonTrivial), 226_nsec);
    CORRADE_COMPARE(animator.duration(nonTrivial), data.once ? 0_nsec : 191_nsec);
    CORRADE_COMPARE(animator.repeatCount(nonTrivial), data.once ? 1 : 0);
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
    else if(data.states)
        trivial = animator.create([](DataHandle, Float, GenericAnimationStates) {
            CORRADE_FAIL("This should never be called.");
        }, Animation::Easing::bounceOut, 137_nsec, 277_nsec, dataHandle(layer.handle(), 0x12345, 0xabc), 3, AnimationFlag::KeepOncePlayed);
    else
        trivial = animator.create([](DataHandle, Float) {
            CORRADE_FAIL("This should never be called.");
        }, Animation::Easing::bounceOut, 137_nsec, 277_nsec, dataHandle(layer.handle(), 0x12345, 0xabc), 3, AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.usedCount(), 1);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 0);
    CORRADE_COMPARE(animator.started(trivial), 137_nsec);
    CORRADE_COMPARE(animator.duration(trivial), data.once ? 0_nsec : 277_nsec);
    CORRADE_COMPARE(animator.repeatCount(trivial), data.once ? 1 : 3);
    CORRADE_COMPARE(animator.flags(trivial), AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.data(trivial), dataHandle(layer.handle(), 0x12345, 0xabc));
    CORRADE_COMPARE(animator.easing(trivial), data.once ? nullptr : Animation::Easing::bounceOut);

    /* The temporary gets destructed right away. Testing also the
       LayerDataHandle overload. */
    AnimationHandle nonTrivial;
    if(data.once)
        nonTrivial = animator.callOnce(NonTrivial{destructedCount}, 226_nsec, layerDataHandle(0x67890, 0xdef), AnimationFlags{0x80});
    else if(data.states)
        nonTrivial = animator.create(NonTrivialStates{destructedCount}, Animation::Easing::smootherstep, 226_nsec, 191_nsec, layerDataHandle(0x67890, 0xdef), 0, AnimationFlags{0x80});
    else
        nonTrivial = animator.create(NonTrivial{destructedCount}, Animation::Easing::smootherstep, 226_nsec, 191_nsec, layerDataHandle(0x67890, 0xdef), 0, AnimationFlags{0x80});
    CORRADE_COMPARE(animator.usedCount(), 2);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 1);
    CORRADE_COMPARE(animator.started(nonTrivial), 226_nsec);
    CORRADE_COMPARE(animator.duration(nonTrivial), data.once ? 0_nsec : 191_nsec);
    CORRADE_COMPARE(animator.repeatCount(nonTrivial), data.once ? 1 : 0);
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
    animator.create(static_cast<void(*)(Float)>(nullptr), Animation::Easing::linear, 0_nsec, 1_nsec);
    animator.create(static_cast<void(*)(Float, GenericAnimationStates)>(nullptr), Animation::Easing::linear, 0_nsec, 1_nsec);
    animator.callOnce(nullptr, 0_nsec);
    animator.create([](Float) {}, nullptr, 0_nsec, 1_nsec);
    animator.create([](Float, GenericAnimationStates) {}, nullptr, 0_nsec, 1_nsec);
    CORRADE_COMPARE_AS(out,
        "Ui::GenericAnimator::create(): animation is null\n"
        "Ui::GenericAnimator::create(): animation is null\n"
        "Ui::GenericAnimator::callOnce(): callback is null\n"
        "Ui::GenericAnimator::create(): easing is null\n"
        "Ui::GenericAnimator::create(): easing is null\n",
        TestSuite::Compare::String);
}

void GenericAnimatorTest::createInvalidNode() {
    CORRADE_SKIP_IF_NO_ASSERT();

    GenericNodeAnimator animator{animatorHandle(0, 1)};

    Containers::String out;
    Error redirectError{&out};
    animator.create(static_cast<void(*)(NodeHandle, Float)>(nullptr), Animation::Easing::linear, 0_nsec, 1_nsec, NodeHandle::Null);
    animator.create(static_cast<void(*)(NodeHandle, Float, GenericAnimationStates)>(nullptr), Animation::Easing::linear, 0_nsec, 1_nsec, NodeHandle::Null);
    animator.callOnce(nullptr, 0_nsec, NodeHandle::Null);
    animator.create([](NodeHandle, Float) {}, nullptr, 0_nsec, 1_nsec, NodeHandle::Null);
    animator.create([](NodeHandle, Float, GenericAnimationStates) {}, nullptr, 0_nsec, 1_nsec, NodeHandle::Null);
    CORRADE_COMPARE_AS(out,
        "Ui::GenericNodeAnimator::create(): animation is null\n"
        "Ui::GenericNodeAnimator::create(): animation is null\n"
        "Ui::GenericNodeAnimator::callOnce(): callback is null\n"
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
    animator.create(static_cast<void(*)(DataHandle, Float)>(nullptr), Animation::Easing::linear, 0_nsec, 1_nsec, DataHandle::Null);
    animator.create(static_cast<void(*)(DataHandle, Float, GenericAnimationStates)>(nullptr), Animation::Easing::linear, 0_nsec, 1_nsec, DataHandle::Null);
    animator.create(static_cast<void(*)(DataHandle, Float)>(nullptr), Animation::Easing::linear, 0_nsec, 1_nsec, LayerDataHandle::Null);
    animator.create(static_cast<void(*)(DataHandle, Float, GenericAnimationStates)>(nullptr), Animation::Easing::linear, 0_nsec, 1_nsec, LayerDataHandle::Null);
    animator.callOnce(nullptr, 0_nsec, DataHandle::Null);
    animator.callOnce(nullptr, 0_nsec, LayerDataHandle::Null);
    animator.create([](DataHandle, Float) {}, nullptr, 0_nsec, 1_nsec, DataHandle::Null);
    animator.create([](DataHandle, Float, GenericAnimationStates) {}, nullptr, 0_nsec, 1_nsec, DataHandle::Null);
    animator.create([](DataHandle, Float) {}, nullptr, 0_nsec, 1_nsec, LayerDataHandle::Null);
    animator.create([](DataHandle, Float, GenericAnimationStates) {}, nullptr, 0_nsec, 1_nsec, LayerDataHandle::Null);
    CORRADE_COMPARE_AS(out,
        "Ui::GenericDataAnimator::create(): animation is null\n"
        "Ui::GenericDataAnimator::create(): animation is null\n"
        "Ui::GenericDataAnimator::create(): animation is null\n"
        "Ui::GenericDataAnimator::create(): animation is null\n"
        "Ui::GenericDataAnimator::callOnce(): callback is null\n"
        "Ui::GenericDataAnimator::callOnce(): callback is null\n"
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
        }, hundredTimes, 0_nsec, 10_nsec);
    else
        animator.create([&first](Float factor) {
            first += factor;
        }, hundredTimes, 0_nsec, 10_nsec);

    if(data.states)
        animator.create([](Float, GenericAnimationStates) {
            CORRADE_FAIL("This shouldn't be called");
        }, hundredTimes, 5_nsec, 15_nsec);
    else
        animator.create([](Float) {
            CORRADE_FAIL("This shouldn't be called");
        }, hundredTimes, 5_nsec, 15_nsec);

    Float third = 0.0f;
    GenericAnimationStates statesThird;
    if(data.states)
        animator.create([&third, &statesThird](Float factor, GenericAnimationStates states) {
            third += factor;
            statesThird |= states;
        }, hundredTimes, 10_nsec, 5_nsec);
    else
        animator.create([&third](Float factor) {
            third += factor;
        }, hundredTimes, 10_nsec, 5_nsec);

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    /* Should call just the first and third with given factors */
    UnsignedByte active[]{(1 << 0)|(1 << 2)};
    Float factors[]{0.75f, 0.42f, 0.25f};
    animator.advance(
        Containers::BitArrayView{active, 0, 3},
        Containers::BitArrayView{&data.started, 0, 3},
        Containers::BitArrayView{&data.stopped, 0, 3},
        factors);
    CORRADE_COMPARE(first, 75.0f);
    CORRADE_COMPARE(third, 25.0f);
    /* Comparing unconditionally to verify that we actually use the other
       overloads as well */
    CORRADE_COMPARE(statesFirst, data.statesFirst);
    CORRADE_COMPARE(statesThird, data.statesThird);
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
        }, hundredTimes, 0_nsec, 10_nsec, nodeHandle(0xabcde, 0x123));
    else
        animator.create([&first](NodeHandle node, Float factor) {
            CORRADE_COMPARE(node, nodeHandle(0xabcde, 0x123));
            first += factor;
        }, hundredTimes, 0_nsec, 10_nsec, nodeHandle(0xabcde, 0x123));

    if(data.states)
        animator.create([](NodeHandle, Float) {
            CORRADE_FAIL("This shouldn't be called");
        }, hundredTimes, 5_nsec, 15_nsec, nodeHandle(0xedcba, 0x321));
    else
        animator.create([](NodeHandle, Float) {
            CORRADE_FAIL("This shouldn't be called");
        }, hundredTimes, 5_nsec, 15_nsec, nodeHandle(0xedcba, 0x321));

    Float third = 0.0f;
    GenericAnimationStates statesThird;
    if(data.states)
        animator.create([&third, &statesThird](NodeHandle node, Float factor, GenericAnimationStates states) {
            CORRADE_COMPARE(node, NodeHandle::Null);
            third += factor;
            statesThird |= states;
        }, hundredTimes, 10_nsec, 5_nsec, NodeHandle::Null);
    else
        animator.create([&third](NodeHandle node, Float factor) {
            CORRADE_COMPARE(node, NodeHandle::Null);
            third += factor;
        }, hundredTimes, 10_nsec, 5_nsec, NodeHandle::Null);

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    /* Should call just the first and third with given factors. The
       started/stopped bits currently don't affect anything. */
    UnsignedByte active[]{(1 << 0)|(1 << 2)};
    Float factors[]{0.75f, 0.42f, 0.25f};
    animator.advance(
        Containers::BitArrayView{active, 0, 3},
        Containers::BitArrayView{&data.started, 0, 3},
        Containers::BitArrayView{&data.stopped, 0, 3},
        factors);
    CORRADE_COMPARE(first, 75.0f);
    CORRADE_COMPARE(third, 25.0f);
    /* Comparing unconditionally to verify that we actually use the other
       overloads as well */
    CORRADE_COMPARE(statesFirst, data.statesFirst);
    CORRADE_COMPARE(statesThird, data.statesThird);
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
        }, hundredTimes, 0_nsec, 10_nsec, dataHandle(layer.handle(), 0xabcde, 0x123));
    else
        animator.create([&first](DataHandle data, Float factor) {
            CORRADE_COMPARE(data, dataHandle(layerHandle(0xab, 0xcd), 0xabcde, 0x123));
            first += factor;
        }, hundredTimes, 0_nsec, 10_nsec, dataHandle(layer.handle(), 0xabcde, 0x123));

    if(data.states)
        animator.create([](DataHandle, Float, GenericAnimationStates) {
            CORRADE_FAIL("This shouldn't be called");
        }, hundredTimes, 5_nsec, 15_nsec, dataHandle(layer.handle(), 0xedcba, 0x321));
    else
        animator.create([](DataHandle, Float) {
            CORRADE_FAIL("This shouldn't be called");
        }, hundredTimes, 5_nsec, 15_nsec, dataHandle(layer.handle(), 0xedcba, 0x321));

    Float third = 0.0f;
    GenericAnimationStates statesThird;
    if(data.states)
        animator.create([&third, &statesThird](DataHandle data, Float factor, GenericAnimationStates states) {
            /* If there's no associated data, the layer handle shouldn't be
               added to the null LayerDataHandle */
            CORRADE_COMPARE(data, DataHandle::Null);
            third += factor;
            statesThird |= states;
        }, hundredTimes, 10_nsec, 5_nsec, LayerDataHandle::Null);
    else
        animator.create([&third](DataHandle data, Float factor) {
            /* If there's no associated data, the layer handle shouldn't be
               added to the null LayerDataHandle */
            CORRADE_COMPARE(data, DataHandle::Null);
            third += factor;
        }, hundredTimes, 10_nsec, 5_nsec, LayerDataHandle::Null);

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    /* Should call just the first and third with given factors. The
       started/stopped bits currently don't affect anything. */
    UnsignedByte active[]{(1 << 0)|(1 << 2)};
    Float factors[]{0.75f, 0.42f, 0.25f};
    animator.advance(
        Containers::BitArrayView{active, 0, 3},
        Containers::BitArrayView{&data.started, 0, 3},
        Containers::BitArrayView{&data.stopped, 0, 3},
        factors);
    CORRADE_COMPARE(first, 75.0f);
    CORRADE_COMPARE(third, 25.0f);
    /* Comparing unconditionally to verify that we actually use the other
       overloads as well */
    CORRADE_COMPARE(statesFirst, data.statesFirst);
    CORRADE_COMPARE(statesThird, data.statesThird);
}

void GenericAnimatorTest::advanceOnce() {
    auto&& data = AdvanceOnceData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    GenericAnimator animator{animatorHandle(0, 1)};

    Int called = 1;
    animator.callOnce([&called]() {
        called *= 2;
    }, 9_nsec);

    animator.callOnce([]() {
        CORRADE_FAIL("This shouldn't be called");
    }, 2_nsec);

    animator.callOnce([&called]() {
        called *= 3;
    }, 7_nsec);

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    /* Should call just the first and third. Factors are not used, only the
       started/stopped bits should affect the output. */
    UnsignedByte active[]{(1 << 0)|(1 << 2)};
    Float factors[3]{};
    animator.advance(
        Containers::BitArrayView{active, 0, 3},
        Containers::BitArrayView{&data.started, 0, 3},
        Containers::BitArrayView{&data.stopped, 0, 3},
        factors);
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
    }, 9_nsec, nodeHandle(0xabcde, 0x123));

    animator.callOnce([](NodeHandle) {
        CORRADE_FAIL("This shouldn't be called");
    }, 2_nsec, nodeHandle(0xedcba, 0x321));

    animator.callOnce([&called](NodeHandle node) {
        CORRADE_COMPARE(node, NodeHandle::Null);
        called *= 3;
    }, 7_nsec, NodeHandle::Null);

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    /* Should call just the first and third. Factors are not used, only the
       started/stopped bits should affect the output. */
    UnsignedByte active[]{(1 << 0)|(1 << 2)};
    Float factors[3]{};
    animator.advance(
        Containers::BitArrayView{active, 0, 3},
        Containers::BitArrayView{&data.started, 0, 3},
        Containers::BitArrayView{&data.stopped, 0, 3},
        factors);
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
    }, 9_nsec, dataHandle(layer.handle(), 0xabcde, 0x123));

    animator.callOnce([](DataHandle) {
        CORRADE_FAIL("This shouldn't be called");
    }, 2_nsec, dataHandle(layer.handle(), 0xedcba, 0x321));

    animator.callOnce([&called](DataHandle data) {
        /* If there's no associated data, the layer handle shouldn't be added
           to the null LayerDataHandle */
        CORRADE_COMPARE(data, DataHandle::Null);
        called *= 3;
    }, 7_nsec, LayerDataHandle::Null);

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    /* Should call just the first and third. Factors are not used, only the
       started/stopped bits should affect the output. */
    UnsignedByte active[]{(1 << 0)|(1 << 2)};
    Float factors[3]{};
    animator.advance(
        Containers::BitArrayView{active, 0, 3},
        Containers::BitArrayView{&data.started, 0, 3},
        Containers::BitArrayView{&data.stopped, 0, 3},
        factors);
    CORRADE_COMPARE(called, data.expected);
}

void GenericAnimatorTest::advanceEmpty() {
    GenericAnimator animator{animatorHandle(0, 1)};
    animator.advance({}, {}, {}, {});

    /* Shouldn't crash or anything */
    CORRADE_VERIFY(true);
}

void GenericAnimatorTest::advanceEmptyNode() {
    GenericNodeAnimator animator{animatorHandle(0, 1)};
    animator.advance({}, {}, {}, {});

    /* Shouldn't crash or anything */
    CORRADE_VERIFY(true);
}

void GenericAnimatorTest::advanceEmptyData() {
    /* This should work even with no layer being set */
    GenericDataAnimator animator{animatorHandle(0, 1)};
    animator.advance({}, {}, {}, {});

    /* Shouldn't crash or anything */
    CORRADE_VERIFY(true);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::GenericAnimatorTest)
