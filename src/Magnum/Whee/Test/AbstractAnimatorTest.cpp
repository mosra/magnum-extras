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

#include <sstream>
#include <Corrade/Containers/BitArray.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/Pair.h>
#include <Corrade/Containers/StaticArray.h>
#include <Corrade/Containers/StridedBitArrayView.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/StringStl.h> /** @todo remove once Debug is stream-free */
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Container.h>
#include <Corrade/TestSuite/Compare/String.h>
#include <Corrade/Utility/DebugStl.h> /** @todo remove once Debug is stream-free */
#include <Corrade/Utility/Format.h>
#include <Magnum/Math/Constants.h>
#include <Magnum/Math/Time.h>

#include "Magnum/Whee/AbstractAnimator.h"
#include "Magnum/Whee/Handle.h"

namespace Magnum { namespace Whee { namespace Test { namespace {

struct AbstractAnimatorTest: TestSuite::Tester {
    explicit AbstractAnimatorTest();

    void debugFeature();
    void debugFeatures();
    void debugState();
    void debugStates();
    void debugAnimationFlag();
    void debugAnimationFlags();
    void debugAnimationState();

    void construct();
    void constructGeneric();
    void constructInvalidHandle();
    void constructCopy();
    void constructCopyGeneric();
    void constructMove();
    void constructMoveGeneric();

    void createRemove();
    void createRemoveHandleRecycle();
    void createRemoveHandleDisable();
    void createNoHandlesLeft();
    void createInvalid();
    void removeInvalid();
    void properties();
    void propertiesStateFactor();
    void propertiesInvalid();

    void clean();
    void cleanEmpty();
    void cleanNotImplemented();
    void cleanInvalid();

    void playPauseStop();
    void playPauseStopInvalid();
    void playPaused();

    void advance();
    void advanceEmpty();
    void advanceInvalid();

    void advanceGeneric();

    void state();
};

using namespace Math::Literals;

const struct {
    TestSuite::TestCaseDescriptionSourceLocation name;
    Nanoseconds duration, played;
    Containers::Optional<Nanoseconds> paused;
    Containers::Optional<Nanoseconds> stopped;
    Containers::Optional<UnsignedInt> repeatCount;
    AnimationState expectedState;
    Float expectedFactor;
} PropertiesStateFactorData[]{
    {"scheduled", 10_nsec, 100_nsec, {}, {}, {},
        AnimationState::Scheduled, 0.0f},
    {"scheduled, paused later", 10_nsec, 100_nsec, 108_nsec, {}, {},
        AnimationState::Scheduled, 0.0f},
    {"scheduled, stopped later", 10_nsec, 100_nsec, {}, 109_nsec, {},
        AnimationState::Scheduled, 0.0f},
    {"scheduled, paused + stopped later", 10_nsec, 100_nsec, 108_nsec, 109_nsec, {},
        AnimationState::Scheduled, 0.0f},
    {"scheduled, repeats", 10_nsec, 100_nsec, {}, {}, 10,
        AnimationState::Scheduled, 0.0f},
    {"playing begin", 10_nsec, 0_nsec, {}, {}, {},
        AnimationState::Playing, 0.0f},
    {"playing begin, paused later", 10_nsec, 0_nsec, 3_nsec, {}, {},
        AnimationState::Playing, 0.0f},
    {"playing begin, stopped later", 10_nsec, 0_nsec, {}, 4_nsec, {},
        AnimationState::Playing, 0.0f},
    {"playing begin, paused + stopped later", 10_nsec, 0_nsec, 3_nsec, 4_nsec, {},
        AnimationState::Playing, 0.0f},
    {"playing begin, repeat", 10_nsec, -90_nsec, {}, {}, 10,
        AnimationState::Playing, 0.0f},
    {"playing middle", 10_nsec, -3_nsec, {}, {}, {},
        AnimationState::Playing, 0.3f},
    {"playing middle, paused later", 10_nsec, -3_nsec, 8_nsec, {}, {},
        AnimationState::Playing, 0.3f},
    {"playing middle, stopped later", 10_nsec, -3_nsec, {}, 9_nsec, {},
        AnimationState::Playing, 0.3f},
    {"playing middle, paused + stopped later", 10_nsec, -3_nsec, 8_nsec, 9_nsec, {},
        AnimationState::Playing, 0.3f},
    {"playing middle, repeats", 10_nsec, -97_nsec, {}, {}, 10,
        AnimationState::Playing, 0.7f},
    {"playing end", 10_nsec, -10_nsec, {}, {}, {},
        AnimationState::Stopped, 1.0f},
    {"playing end, repeats", 10_nsec, -90_nsec, {}, {}, 9,
        AnimationState::Stopped, 1.0f},
    {"paused begin", 10_nsec, -10_nsec, Nanoseconds{-10_nsec}, {}, {},
        AnimationState::Paused, 0.0f},
    {"paused begin, stopped later", 10_nsec, -10_nsec, Nanoseconds{-10_nsec}, 3_nsec, {},
        AnimationState::Paused, 0.0f},
    {"paused begin, repeats", 10_nsec, -30_nsec, Nanoseconds{-10_nsec}, {}, 3,
        AnimationState::Paused, 0.0f},
    {"paused middle", 10_nsec, -10_nsec, Nanoseconds{-3_nsec}, {}, {},
        AnimationState::Paused, 0.7f},
    {"paused middle, repeats", 10_nsec, -30_nsec, Nanoseconds{-7_nsec}, {}, 3,
        AnimationState::Paused, 0.3f},
    {"paused end", 10_nsec, -10_nsec, 0_nsec, {}, {},
        AnimationState::Stopped, 1.0f},
    {"paused end, repeats", 10_nsec, -80_nsec, 0_nsec, {}, 8,
        AnimationState::Stopped, 1.0f},
    /* The animation isn't considered paused yet but scheduled, as it'll be
       advanced (and thus calculated) only once it reaches the actual paused
       state */
    {"paused, scheduled later", 10_nsec, 100_nsec, 90_nsec, {}, {},
        AnimationState::Scheduled, 0.0f},
    {"stopped", 10_nsec, -100_nsec, {}, {}, {},
        AnimationState::Stopped, 1.0f},
    {"stopped, repeats", 10_nsec, -100_nsec, {}, {}, 9,
        AnimationState::Stopped, 1.0f},
    {"stopped explicitly", 10_nsec, -100_nsec, {}, Nanoseconds{-95_nsec}, {},
        AnimationState::Stopped, 1.0f},
    {"stopped explicitly just now", 10_nsec, -5_nsec, {}, 0_nsec, {},
        AnimationState::Stopped, 1.0f},
    /* As this doesn't ever result in the animation running, it's Stopped
       already to not require a NeedsAdvance */
    {"stopped, scheduled later", 10_nsec, 100_nsec, {}, 90_nsec, {},
        AnimationState::Stopped, 1.0f},
    {"playing begin, one day duration",
        24ll*60ll*60ll*1.0_sec,
        0.0_sec, {}, {}, {},
        AnimationState::Playing, 0.0f},
    {"playing middle, one day duration",
        24ll*60ll*60ll*1.0_sec,
        -16ll*60ll*60ll*1.0_sec, {}, {}, {},
        AnimationState::Playing, 0.66667f},
    {"playing end, one day duration",
        24ll*60ll*60ll*1.0_sec,
        -24ll*60ll*60ll*1.0_sec, {}, {}, {},
        AnimationState::Stopped, 1.0f},
    {"playing begin, one year duration",
        365ll*24ll*60ll*60ll*1.0_sec,
        0.0_sec, {}, {}, {},
        AnimationState::Playing, 0.0f},
    {"playing middle, one year duration",
        365ll*24ll*60ll*60ll*1.0_sec,
        -365ll*16ll*60ll*60ll*1.0_sec, {}, {}, {},
        AnimationState::Playing, 0.66667f},
    {"playing end, one year duration",
        365ll*24ll*60ll*60ll*1.0_sec,
        -365ll*24ll*60ll*60ll*1.0_sec, {}, {}, {},
        AnimationState::Stopped, 1.0f},
    /* The duration is scaled by 29 in the test case, which makes this 290
       years, which is near to the maximum representable (signed) range of 292
       years */
    {"playing begin, 10 year duration",
        10ll*365ll*24ll*60ll*60ll*1.0_sec,
        0.0_sec, {}, {}, {},
        AnimationState::Playing, 0.0f},
    {"playing middle, 10 year duration",
        10ll*365ll*24ll*60ll*60ll*1.0_sec,
        -10ll*365ll*16ll*60ll*60ll*1.0_sec, {}, {}, {},
        AnimationState::Playing, 0.66667f},
    {"playing end, 10 year duration",
        10ll*365ll*24ll*60ll*60ll*1.0_sec,
        -10ll*365ll*24ll*60ll*60ll*1.0_sec, {}, {}, {},
        AnimationState::Stopped, 1.0f},
    {"playing begin, 1 second duration, 100 millionth repeat",
        1.0_sec,
        -100ll*1000ll*1000ll*1.0_sec, {}, {}, 0,
        AnimationState::Playing, 0.0f},
    {"playing middle, 1 second duration, 100 millionth repeat",
        1.0_sec,
        -100ll*1000ll*1000ll*1.0_sec + 0.376_sec, {}, {}, 0,
        AnimationState::Playing, 1.0f - 0.376f},
    {"playing end, 1 second duration, 100 millionth repeat",
        1.0_sec,
        -100ll*1000ll*1000ll*1.0_sec, {}, {}, 100*1000*1000,
        AnimationState::Stopped, 1.0f},
};

const struct {
    const char* name;
    Containers::Optional<Nanoseconds> stopped;
    Nanoseconds paused;
    Nanoseconds resumed;
    Nanoseconds expectedPlayed;
} PlayPausedData[]{
    /* Stopped at 40 nsec of play time, so resuming at 500 will push it back by
       40 */
    {"",
        {}, 50_nsec, 500_nsec, 460_nsec},
    /* The animation was paused before it was played, resuming it should be
       from the start */
    {"paused before a play",
        {}, -30_nsec, 500_nsec, 500_nsec},
    /* Resuming before a pause basically discards the pause that would happen
       in the future */
    {"resumed before a pause",
        {}, 50_nsec, 40_nsec, 40_nsec},
    /* Same, in this case it'đ moving the start of the playback further into
       the past */
    {"resumed before a play",
        {}, 50_nsec, -10_nsec, -10_nsec},
    /* The animation is considered stopped when it reaches a pause, so resuming
       it will play from the start */
    {"stopped before a pause",
        40_nsec, 50_nsec, 500_nsec, 500_nsec},
    /* Same, it's already stopped when resuming */
    {"stopped after a pause but before resume",
        90_nsec, 50_nsec, 500_nsec, 500_nsec},
    /* This is as if no stop happened yet */
    {"stopped after resume",
        600_nsec, 50_nsec, 500_nsec, 460_nsec},
};

AbstractAnimatorTest::AbstractAnimatorTest() {
    addTests({&AbstractAnimatorTest::debugFeature,
              &AbstractAnimatorTest::debugFeatures,
              &AbstractAnimatorTest::debugState,
              &AbstractAnimatorTest::debugStates,
              &AbstractAnimatorTest::debugAnimationFlag,
              &AbstractAnimatorTest::debugAnimationFlags,
              &AbstractAnimatorTest::debugAnimationState,

              &AbstractAnimatorTest::construct,
              &AbstractAnimatorTest::constructGeneric,
              &AbstractAnimatorTest::constructInvalidHandle,
              &AbstractAnimatorTest::constructCopy,
              &AbstractAnimatorTest::constructCopyGeneric,
              &AbstractAnimatorTest::constructMove,
              &AbstractAnimatorTest::constructMoveGeneric,

              &AbstractAnimatorTest::createRemove,
              &AbstractAnimatorTest::createRemoveHandleRecycle,
              &AbstractAnimatorTest::createRemoveHandleDisable,
              &AbstractAnimatorTest::createNoHandlesLeft,
              &AbstractAnimatorTest::createInvalid,
              &AbstractAnimatorTest::removeInvalid,
              &AbstractAnimatorTest::properties});

    addInstancedTests({&AbstractAnimatorTest::propertiesStateFactor},
        Containers::arraySize(PropertiesStateFactorData));

    addTests({&AbstractAnimatorTest::propertiesInvalid,

              &AbstractAnimatorTest::clean,
              &AbstractAnimatorTest::cleanEmpty,
              &AbstractAnimatorTest::cleanNotImplemented,
              &AbstractAnimatorTest::cleanInvalid,

              &AbstractAnimatorTest::playPauseStop,
              &AbstractAnimatorTest::playPauseStopInvalid});

    addInstancedTests({&AbstractAnimatorTest::playPaused},
        Containers::arraySize(PlayPausedData));

    addTests({&AbstractAnimatorTest::advance,
              &AbstractAnimatorTest::advanceEmpty,
              &AbstractAnimatorTest::advanceInvalid,

              &AbstractAnimatorTest::advanceGeneric,

              &AbstractAnimatorTest::state});
}

void AbstractAnimatorTest::debugFeature() {
    std::ostringstream out;
    Debug{&out} << AnimatorFeature(0xbe);
    CORRADE_COMPARE(out.str(), "Whee::AnimatorFeature(0xbe)\n");
}

void AbstractAnimatorTest::debugFeatures() {
    std::ostringstream out;
    Debug{&out} << AnimatorFeature(0xe0) << AnimatorFeatures{};
    CORRADE_COMPARE(out.str(), "Whee::AnimatorFeature(0xe0) Whee::AnimatorFeatures{}\n");
}

void AbstractAnimatorTest::debugState() {
    std::ostringstream out;
    Debug{&out} << AnimatorState::NeedsAdvance << AnimatorState(0xbe);
    CORRADE_COMPARE(out.str(), "Whee::AnimatorState::NeedsAdvance Whee::AnimatorState(0xbe)\n");
}

void AbstractAnimatorTest::debugStates() {
    std::ostringstream out;
    Debug{&out} << (AnimatorState::NeedsAdvance|AnimatorState(0xe0)) << AnimatorStates{};
    CORRADE_COMPARE(out.str(), "Whee::AnimatorState::NeedsAdvance|Whee::AnimatorState(0xe0) Whee::AnimatorStates{}\n");
}

void AbstractAnimatorTest::debugAnimationFlag() {
    std::ostringstream out;
    Debug{&out} << AnimationFlag::KeepOncePlayed << AnimationFlag(0xbe);
    CORRADE_COMPARE(out.str(), "Whee::AnimationFlag::KeepOncePlayed Whee::AnimationFlag(0xbe)\n");
}

void AbstractAnimatorTest::debugAnimationFlags() {
    std::ostringstream out;
    Debug{&out} << (AnimationFlag::KeepOncePlayed|AnimationFlag(0xe0)) << AnimationFlags{};
    CORRADE_COMPARE(out.str(), "Whee::AnimationFlag::KeepOncePlayed|Whee::AnimationFlag(0xe0) Whee::AnimationFlags{}\n");
}

void AbstractAnimatorTest::debugAnimationState() {
    std::ostringstream out;
    Debug{&out} << AnimationState::Paused << AnimationState(0xbe);
    CORRADE_COMPARE(out.str(), "Whee::AnimationState::Paused Whee::AnimationState(0xbe)\n");
}

void AbstractAnimatorTest::construct() {
    struct: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeatures{0xbc};
        }
    } animator{animatorHandle(0xab, 0x12)};

    CORRADE_COMPARE(animator.features(), AnimatorFeatures{0xbc});
    CORRADE_COMPARE(animator.handle(), animatorHandle(0xab, 0x12));
    CORRADE_COMPARE(animator.state(), AnimatorStates{});
    CORRADE_COMPARE(animator.time(), 0_nsec);
    CORRADE_COMPARE(animator.capacity(), 0);
    CORRADE_COMPARE(animator.usedCount(), 0);
    CORRADE_VERIFY(!animator.isHandleValid(AnimatorDataHandle::Null));
    CORRADE_VERIFY(!animator.isHandleValid(AnimationHandle::Null));
}

void AbstractAnimatorTest::constructGeneric() {
    struct: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeatures{0xbc};
        }
        void doAdvance(Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
    } animator{animatorHandle(0xab, 0x12)};

    CORRADE_COMPARE(animator.features(), AnimatorFeatures{0xbc});
    CORRADE_COMPARE(animator.handle(), animatorHandle(0xab, 0x12));
    /* The rest is the same as in construct() */
}

void AbstractAnimatorTest::constructInvalidHandle() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Animator: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;

        AnimatorFeatures doFeatures() const override { return {}; }
    };

    std::ostringstream out;
    Error redirectError{&out};
    Animator{AnimatorHandle::Null};
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractAnimator: handle is null\n");
}

void AbstractAnimatorTest::constructCopy() {
    struct Animator: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;

        AnimatorFeatures doFeatures() const override { return {}; }
    };

    CORRADE_VERIFY(!std::is_copy_constructible<Animator>{});
    CORRADE_VERIFY(!std::is_copy_assignable<Animator>{});
}

void AbstractAnimatorTest::constructCopyGeneric() {
    struct Animator: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;

        AnimatorFeatures doFeatures() const override { return {}; }
        void doAdvance(Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
    };

    CORRADE_VERIFY(!std::is_copy_constructible<Animator>{});
    CORRADE_VERIFY(!std::is_copy_assignable<Animator>{});
}

void AbstractAnimatorTest::constructMove() {
    struct Animator: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;

        AnimatorFeatures doFeatures() const override { return {}; }
    };

    /* The class has an internal state struct containing everything, so it's
       not needed to test each and every property */
    Animator a{animatorHandle(0xab, 0x12)};

    Animator b{Utility::move(a)};
    CORRADE_COMPARE(b.handle(), animatorHandle(0xab, 0x12));

    Animator c{animatorHandle(0xcd, 0x34)};
    c = Utility::move(b);
    CORRADE_COMPARE(c.handle(), animatorHandle(0xab, 0x12));

    CORRADE_VERIFY(std::is_nothrow_move_constructible<Animator>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<Animator>::value);
}

void AbstractAnimatorTest::constructMoveGeneric() {
    struct Animator: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;

        AnimatorFeatures doFeatures() const override { return {}; }
        void doAdvance(Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
    };

    /* Just verify that the subclass doesn't have the moves broken */
    Animator a{animatorHandle(0xab, 0x12)};

    Animator b{Utility::move(a)};
    CORRADE_COMPARE(b.handle(), animatorHandle(0xab, 0x12));

    Animator c{animatorHandle(0xcd, 0x34)};
    c = Utility::move(b);
    CORRADE_COMPARE(c.handle(), animatorHandle(0xab, 0x12));

    CORRADE_VERIFY(std::is_nothrow_move_constructible<Animator>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<Animator>::value);
}

void AbstractAnimatorTest::createRemove() {
    struct: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;
        using AbstractAnimator::create;
        using AbstractAnimator::remove;

        AnimatorFeatures doFeatures() const override { return {}; }
    } animator{animatorHandle(0xab, 0x12)};

    AnimationHandle first = animator.create(1337_nsec, 37588_nsec);
    CORRADE_COMPARE(first, animationHandle(animator.handle(), 0, 1));
    CORRADE_VERIFY(animator.isHandleValid(first));
    /* Animator state() is tested thoroughly in state() */
    CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);
    CORRADE_COMPARE(animator.capacity(), 1);
    CORRADE_COMPARE(animator.usedCount(), 1);
    CORRADE_COMPARE(animator.duration(first), 37588_nsec);
    CORRADE_COMPARE(animator.repeatCount(first), 1);
    CORRADE_COMPARE(animator.flags(first), AnimationFlags{});
    CORRADE_COMPARE(animator.played(first), 1337_nsec);
    CORRADE_COMPARE(animator.paused(first), Nanoseconds::max());
    CORRADE_COMPARE(animator.stopped(first), Nanoseconds::max());
    /* Animation state() is tested thoroughly in animationState() */
    CORRADE_COMPARE(animator.state(first), AnimationState::Scheduled);

    /* Specifying repeat count and flags, using the AnimatorDataHandle
       overload */
    AnimationHandle second = animator.create(-26_nsec, 3_nsec, 666, AnimationFlags{0x10});
    CORRADE_COMPARE(second, animationHandle(animator.handle(), 1, 1));
    CORRADE_VERIFY(animator.isHandleValid(second));
    /* Animator state() is tested thoroughly in state() */
    CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);
    CORRADE_COMPARE(animator.capacity(), 2);
    CORRADE_COMPARE(animator.usedCount(), 2);
    CORRADE_COMPARE(animator.duration(animationHandleData(second)), 3_nsec);
    CORRADE_COMPARE(animator.repeatCount(animationHandleData(second)), 666);
    CORRADE_COMPARE(animator.flags(animationHandleData(second)), AnimationFlags{0x10});
    CORRADE_COMPARE(animator.played(animationHandleData(second)), -26_nsec);
    CORRADE_COMPARE(animator.paused(animationHandleData(second)), Nanoseconds::max());
    CORRADE_COMPARE(animator.stopped(animationHandleData(second)), Nanoseconds::max());
    /* Animation state() is tested thoroughly in animationState() */
    CORRADE_COMPARE(animator.state(animationHandleData(second)), AnimationState::Playing);

    /* Overload without repeat count */
    AnimationHandle third = animator.create(111_nsec, 11_nsec, AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(third, animationHandle(animator.handle(), 2, 1));
    CORRADE_VERIFY(animator.isHandleValid(third));
    /* Animator state() is tested thoroughly in state() */
    CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);
    CORRADE_COMPARE(animator.capacity(), 3);
    CORRADE_COMPARE(animator.usedCount(), 3);
    CORRADE_COMPARE(animator.duration(third), 11_nsec);
    CORRADE_COMPARE(animator.repeatCount(third), 1);
    CORRADE_COMPARE(animator.flags(third), AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.played(third), 111_nsec);
    CORRADE_COMPARE(animator.paused(third), Nanoseconds::max());
    CORRADE_COMPARE(animator.stopped(third), Nanoseconds::max());
    /* Animation state() is tested thoroughly in animationState() */
    CORRADE_COMPARE(animator.state(third), AnimationState::Scheduled);

    animator.remove(first);
    CORRADE_VERIFY(!animator.isHandleValid(first));
    CORRADE_VERIFY(animator.isHandleValid(second));
    CORRADE_VERIFY(animator.isHandleValid(third));
    CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);
    CORRADE_COMPARE(animator.capacity(), 3);
    CORRADE_COMPARE(animator.usedCount(), 2);

    /* Using also the AnimatorDataHandle overload */
    animator.remove(animationHandleData(second));
    CORRADE_VERIFY(!animator.isHandleValid(first));
    CORRADE_VERIFY(!animator.isHandleValid(second));
    CORRADE_VERIFY(animator.isHandleValid(third));
    CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);
    CORRADE_COMPARE(animator.capacity(), 3);
    CORRADE_COMPARE(animator.usedCount(), 1);
}

void AbstractAnimatorTest::createRemoveHandleRecycle() {
    struct: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;
        using AbstractAnimator::create;
        using AbstractAnimator::remove;

        AnimatorFeatures doFeatures() const override { return {}; }
    } animator{animatorHandle(0xab, 0x12)};

    AnimationHandle first = animator.create(0_nsec, 12_nsec, 0, AnimationFlag::KeepOncePlayed);
    AnimationHandle second = animator.create(2_nsec, 1_nsec);
    AnimationHandle third = animator.create(2782_nsec, 281698_nsec, 666);
    AnimationHandle fourth = animator.create(166_nsec, 78888_nsec);
    CORRADE_COMPARE(first, animationHandle(animator.handle(), 0, 1));
    CORRADE_COMPARE(second, animationHandle(animator.handle(), 1, 1));
    CORRADE_COMPARE(third, animationHandle(animator.handle(), 2, 1));
    CORRADE_COMPARE(fourth, animationHandle(animator.handle(), 3, 1));
    CORRADE_VERIFY(animator.isHandleValid(first));
    CORRADE_VERIFY(animator.isHandleValid(second));
    CORRADE_VERIFY(animator.isHandleValid(third));
    CORRADE_VERIFY(animator.isHandleValid(fourth));
    CORRADE_COMPARE(animator.capacity(), 4);
    CORRADE_COMPARE(animator.usedCount(), 4);
    CORRADE_COMPARE(animator.duration(first), 12_nsec);
    CORRADE_COMPARE(animator.repeatCount(first), 0);
    CORRADE_COMPARE(animator.flags(first), AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.played(first), 0_nsec);
    CORRADE_COMPARE(animator.paused(first), Nanoseconds::max());
    CORRADE_COMPARE(animator.stopped(first), Nanoseconds::max());
    CORRADE_COMPARE(animator.duration(second), 1_nsec);
    CORRADE_COMPARE(animator.repeatCount(second), 1);
    CORRADE_COMPARE(animator.flags(second), AnimationFlags{});
    CORRADE_COMPARE(animator.played(second), 2_nsec);
    CORRADE_COMPARE(animator.paused(second), Nanoseconds::max());
    CORRADE_COMPARE(animator.stopped(second), Nanoseconds::max());
    CORRADE_COMPARE(animator.duration(third), 281698_nsec);
    CORRADE_COMPARE(animator.repeatCount(third), 666);
    CORRADE_COMPARE(animator.flags(third), AnimationFlags{});
    CORRADE_COMPARE(animator.played(third), 2782_nsec);
    CORRADE_COMPARE(animator.paused(third), Nanoseconds::max());
    CORRADE_COMPARE(animator.stopped(third), Nanoseconds::max());
    CORRADE_COMPARE(animator.duration(fourth), 78888_nsec);
    CORRADE_COMPARE(animator.repeatCount(fourth), 1);
    CORRADE_COMPARE(animator.flags(fourth), AnimationFlags{});
    CORRADE_COMPARE(animator.played(fourth), 166_nsec);
    CORRADE_COMPARE(animator.paused(fourth), Nanoseconds::max());
    CORRADE_COMPARE(animator.stopped(fourth), Nanoseconds::max());

    /* Populate internals of some animations */
    animator.pause(first, 50_nsec);
    animator.stop(third, -30_nsec);

    /* Remove three out of the four in an arbitrary order */
    animator.remove(fourth);
    animator.remove(first);
    animator.remove(third);
    CORRADE_VERIFY(!animator.isHandleValid(first));
    CORRADE_VERIFY(animator.isHandleValid(second));
    CORRADE_VERIFY(!animator.isHandleValid(third));
    CORRADE_VERIFY(!animator.isHandleValid(fourth));
    CORRADE_COMPARE(animator.capacity(), 4);
    CORRADE_COMPARE(animator.usedCount(), 1);
    CORRADE_COMPARE(animator.duration(second), 1_nsec);
    CORRADE_COMPARE(animator.played(second), 2_nsec);

    /* Allocating new handles should recycle the handles in the order they were
       removed (oldest first). Their properties should be cleared. */
    AnimationHandle fourth2 = animator.create(255_nsec, 8999_nsec);
    AnimationHandle first2 = animator.create(1_nsec, 14_nsec);
    AnimationHandle third2 = animator.create(2872_nsec, 896182_nsec, 333, AnimationFlags{0x40});
    CORRADE_COMPARE(first2, animationHandle(animator.handle(), 0, 2));
    CORRADE_COMPARE(third2, animationHandle(animator.handle(), 2, 2));
    CORRADE_COMPARE(fourth2, animationHandle(animator.handle(), 3, 2));
    CORRADE_COMPARE(animator.capacity(), 4);
    CORRADE_COMPARE(animator.usedCount(), 4);
    CORRADE_COMPARE(animator.duration(first2), 14_nsec);
    CORRADE_COMPARE(animator.repeatCount(first2), 1);
    CORRADE_COMPARE(animator.flags(first2), AnimationFlags{});
    CORRADE_COMPARE(animator.played(first2), 1_nsec);
    CORRADE_COMPARE(animator.paused(first2), Nanoseconds::max());
    CORRADE_COMPARE(animator.stopped(first2), Nanoseconds::max());
    CORRADE_COMPARE(animator.duration(second), 1_nsec);
    CORRADE_COMPARE(animator.repeatCount(second), 1);
    CORRADE_COMPARE(animator.flags(second), AnimationFlags{});
    CORRADE_COMPARE(animator.played(second), 2_nsec);
    CORRADE_COMPARE(animator.paused(second), Nanoseconds::max());
    CORRADE_COMPARE(animator.stopped(second), Nanoseconds::max());
    CORRADE_COMPARE(animator.duration(third2), 896182_nsec);
    CORRADE_COMPARE(animator.repeatCount(third2), 333);
    CORRADE_COMPARE(animator.flags(third2), AnimationFlags{0x40});
    CORRADE_COMPARE(animator.played(third2), 2872_nsec);
    CORRADE_COMPARE(animator.paused(third2), Nanoseconds::max());
    CORRADE_COMPARE(animator.stopped(third2), Nanoseconds::max());
    CORRADE_COMPARE(animator.duration(fourth2), 8999_nsec);
    CORRADE_COMPARE(animator.repeatCount(fourth2), 1);
    CORRADE_COMPARE(animator.flags(fourth2), AnimationFlags{});
    CORRADE_COMPARE(animator.played(fourth2), 255_nsec);
    CORRADE_COMPARE(animator.paused(fourth2), Nanoseconds::max());
    CORRADE_COMPARE(animator.stopped(fourth2), Nanoseconds::max());

    /* Old handles shouldn't get valid again */
    CORRADE_VERIFY(!animator.isHandleValid(first));
    CORRADE_VERIFY(animator.isHandleValid(first2));
    CORRADE_VERIFY(!animator.isHandleValid(third));
    CORRADE_VERIFY(animator.isHandleValid(third2));
    CORRADE_VERIFY(!animator.isHandleValid(fourth));
    CORRADE_VERIFY(animator.isHandleValid(fourth2));

    /* Removing a single handle and creating a new one directly reuses it if
       there's just one in the free list */
    animator.remove(third2);
    AnimationHandle third3 = animator.create(12_nsec, 26_nsec);
    CORRADE_COMPARE(third3, animationHandle(animator.handle(), 2, 3));
    CORRADE_VERIFY(!animator.isHandleValid(third));
    CORRADE_VERIFY(!animator.isHandleValid(third2));
    CORRADE_VERIFY(animator.isHandleValid(third3));
    CORRADE_COMPARE(animator.capacity(), 4);
    CORRADE_COMPARE(animator.usedCount(), 4);
    CORRADE_COMPARE(animator.duration(third3), 26_nsec);
    CORRADE_COMPARE(animator.repeatCount(third3), 1);
    CORRADE_COMPARE(animator.flags(third3), AnimationFlags{});
    CORRADE_COMPARE(animator.played(third3), 12_nsec);
    CORRADE_COMPARE(animator.paused(third3), Nanoseconds::max());
    CORRADE_COMPARE(animator.stopped(third3), Nanoseconds::max());

    /* Allocating a new handle with the free list empty will grow it */
    AnimationHandle fifth = animator.create(2888_nsec, 8882_nsec);
    CORRADE_COMPARE(fifth, animationHandle(animator.handle(), 4, 1));
    CORRADE_VERIFY(animator.isHandleValid(fifth));
    CORRADE_COMPARE(animator.capacity(), 5);
    CORRADE_COMPARE(animator.usedCount(), 5);
    CORRADE_COMPARE(animator.duration(fifth), 8882_nsec);
    CORRADE_COMPARE(animator.repeatCount(fifth), 1);
    CORRADE_COMPARE(animator.flags(fifth), AnimationFlags{});
    CORRADE_COMPARE(animator.played(fifth), 2888_nsec);
    CORRADE_COMPARE(animator.paused(fifth), Nanoseconds::max());
    CORRADE_COMPARE(animator.stopped(fifth), Nanoseconds::max());
}

void AbstractAnimatorTest::createRemoveHandleDisable() {
    struct: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;
        using AbstractAnimator::create;
        using AbstractAnimator::remove;

        AnimatorFeatures doFeatures() const override { return {}; }
    } animator{animatorHandle(0xab, 0x12)};

    AnimationHandle first = animator.create(12_nsec, 78_nsec);
    CORRADE_COMPARE(first, animationHandle(animator.handle(), 0, 1));

    for(std::size_t i = 0; i != (1 << Implementation::AnimatorDataHandleGenerationBits) - 1; ++i) {
        AnimationHandle second = animator.create(56_nsec, 78_nsec);
        CORRADE_COMPARE(second, animationHandle(animator.handle(), 1, 1 + i));
        animator.remove(second);
    }

    /* The generation for the second slot is exhausted so the handle is not
       recycled */
    CORRADE_COMPARE(animator.capacity(), 2);
    CORRADE_COMPARE(animator.usedCount(), 2);

    /* It shouldn't think a handle from the second slot with generation 0 is
       valid */
    CORRADE_VERIFY(!animator.isHandleValid(animationHandle(animator.handle(), 1, 0)));

    /* There's nowhere to create a new handle from so the capacity is grown */
    AnimationHandle third = animator.create(62_nsec, 78_nsec);
    CORRADE_COMPARE(third, animationHandle(animator.handle(), 2, 1));
    CORRADE_COMPARE(animator.capacity(), 3);
    CORRADE_COMPARE(animator.usedCount(), 3);
}

void AbstractAnimatorTest::createNoHandlesLeft() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;
        using AbstractAnimator::create;

        AnimatorFeatures doFeatures() const override { return {}; }
    } animator{animatorHandle(0, 1)};

    for(std::size_t i = 0; i != 1 << Implementation::AnimatorDataHandleIdBits; ++i)
        animator.create(12_nsec, 35_nsec);

    CORRADE_COMPARE(animator.capacity(), 1 << Implementation::AnimatorDataHandleIdBits);
    CORRADE_COMPARE(animator.usedCount(), 1 << Implementation::AnimatorDataHandleIdBits);

    std::ostringstream out;
    Error redirectError{&out};
    animator.create(17_nsec, 65_nsec);
    /* Number is hardcoded in the expected message but not elsewhere in order
       to give a heads-up when modifying the handle ID bit count */
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractAnimator::create(): can only have at most 1048576 animations\n");
}

void AbstractAnimatorTest::createInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;
        using AbstractAnimator::create;

        AnimatorFeatures doFeatures() const override { return {}; }
    } animator{animatorHandle(0, 1)};

    std::ostringstream out;
    Error redirectError{&out};
    animator.create(15_nsec, 0_nsec);
    animator.create(15_nsec, -1_nsec);
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractAnimator::create(): expected positive duration, got Nanoseconds(0)\n"
        "Whee::AbstractAnimator::create(): expected positive duration, got Nanoseconds(-1)\n");
}

void AbstractAnimatorTest::removeInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;
        using AbstractAnimator::create;
        using AbstractAnimator::remove;

        AnimatorFeatures doFeatures() const override { return {}; }
    } animator{animatorHandle(0, 1)};

    AnimationHandle handle = animator.create(12_nsec, 13_nsec);

    std::ostringstream out;
    Error redirectError{&out};
    animator.remove(AnimationHandle::Null);
    /* Valid animator, invalid data */
    animator.remove(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)));
    /* Invalid animator, valid data */
    animator.remove(animationHandle(AnimatorHandle::Null, animationHandleData(handle)));
    /* AnimatorDataHandle directly */
    animator.remove(AnimatorDataHandle(0x123abcde));
    CORRADE_COMPARE_AS(out.str(),
        "Whee::AbstractAnimator::remove(): invalid handle Whee::AnimationHandle::Null\n"
        "Whee::AbstractAnimator::remove(): invalid handle Whee::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Whee::AbstractAnimator::remove(): invalid handle Whee::AnimationHandle(Null, {0x0, 0x1})\n"
        "Whee::AbstractAnimator::remove(): invalid handle Whee::AnimatorDataHandle(0xabcde, 0x123)\n",
        TestSuite::Compare::String);
}

void AbstractAnimatorTest::properties() {
    struct: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;
        using AbstractAnimator::create;

        AnimatorFeatures doFeatures() const override { return {}; }
    } animator{animatorHandle(0, 1)};

    /* So it doesn't always pick the first one */
    animator.create(7_nsec, 1_nsec);
    AnimationHandle handle = animator.create(12_nsec, 13_nsec);

    /* state() and factor() is tested thoroughly in propertiesStateFactor() */

    animator.setRepeatCount(handle, 777);
    CORRADE_COMPARE(animator.repeatCount(handle), 777);

    animator.setFlags(handle, AnimationFlag::KeepOncePlayed|AnimationFlags{0x20});
    CORRADE_COMPARE(animator.flags(handle), AnimationFlag::KeepOncePlayed|AnimationFlags{0x20});

    animator.addFlags(handle, AnimationFlags{0xe0});
    CORRADE_COMPARE(animator.flags(handle), AnimationFlag::KeepOncePlayed|AnimationFlags{0xe0});

    animator.clearFlags(handle, AnimationFlags{0xb0});
    CORRADE_COMPARE(animator.flags(handle), AnimationFlag::KeepOncePlayed|AnimationFlags{0x40});

    /* Using also the AnimatorDataHandle overload */
    animator.setRepeatCount(animationHandleData(handle), 444);
    CORRADE_COMPARE(animator.repeatCount(animationHandleData(handle)), 444);

    animator.setFlags(animationHandleData(handle), AnimationFlags{0x08});
    CORRADE_COMPARE(animator.flags(animationHandleData(handle)), AnimationFlags{0x08});

    animator.addFlags(animationHandleData(handle), AnimationFlag::KeepOncePlayed|AnimationFlags{0xe0});
    CORRADE_COMPARE(animator.flags(animationHandleData(handle)), AnimationFlag::KeepOncePlayed|AnimationFlags{0xe8});

    animator.clearFlags(animationHandleData(handle), AnimationFlag::KeepOncePlayed|AnimationFlags{0xb0});
    CORRADE_COMPARE(animator.flags(animationHandleData(handle)), AnimationFlags{0x48});
}

void AbstractAnimatorTest::propertiesStateFactor() {
    auto&& data = PropertiesStateFactorData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;
        using AbstractAnimator::create;

        AnimatorFeatures doFeatures() const override { return {}; }
    } animator{animatorHandle(0, 1)};

    /* So it doesn't always pick the first one */
    animator.create(7_nsec, 1_nsec);

    /* All time is scaled and offset by a non-trivial amount to test corner
       cases. The resulting factors should be still same with it. */
    Long scale = 29;
    Nanoseconds offset = 77777_nsec;

    AnimationHandle handle = data.repeatCount ?
        animator.create(data.played*scale + offset, data.duration*scale, *data.repeatCount, AnimationFlag::KeepOncePlayed) :
        animator.create(data.played*scale + offset, data.duration*scale, AnimationFlag::KeepOncePlayed);
    if(data.paused)
        animator.pause(handle, *data.paused*scale + offset);
    if(data.stopped)
        animator.stop(handle, *data.stopped*scale + offset);

    Containers::BitArray mask{NoInit, 2};
    Float factors[2];
    animator.advance(offset, mask, factors, mask);

    CORRADE_COMPARE(animator.state(handle), data.expectedState);
    CORRADE_COMPARE(animator.factor(handle), data.expectedFactor);
    /* Using also the AnimatorDataHandle overload */
    CORRADE_COMPARE(animator.state(animationHandleData(handle)), data.expectedState);
    CORRADE_COMPARE(animator.factor(animationHandleData(handle)), data.expectedFactor);
}

void AbstractAnimatorTest::propertiesInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;
        using AbstractAnimator::create;

        AnimatorFeatures doFeatures() const override { return {}; }
    } animator{animatorHandle(0, 1)};

    AnimationHandle handle = animator.create(12_nsec, 13_nsec);

    std::ostringstream out;
    Error redirectError{&out};
    animator.duration(AnimationHandle::Null);
    animator.repeatCount(AnimationHandle::Null);
    animator.setRepeatCount(AnimationHandle::Null, 0);
    animator.flags(AnimationHandle::Null);
    animator.setFlags(AnimationHandle::Null, {});
    animator.addFlags(AnimationHandle::Null, {});
    animator.clearFlags(AnimationHandle::Null, {});
    animator.played(AnimationHandle::Null);
    animator.paused(AnimationHandle::Null);
    animator.stopped(AnimationHandle::Null);
    animator.state(AnimationHandle::Null);
    animator.factor(AnimationHandle::Null);
    /* Valid animator, invalid data */
    animator.duration(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)));
    animator.repeatCount(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)));
    animator.setRepeatCount(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)), 0);
    animator.flags(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)));
    animator.setFlags(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)), {});
    animator.addFlags(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)), {});
    animator.clearFlags(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)), {});
    animator.played(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)));
    animator.paused(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)));
    animator.stopped(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)));
    animator.state(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)));
    animator.factor(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)));
    /* Invalid animator, valid data */
    animator.duration(animationHandle(AnimatorHandle::Null, animationHandleData(handle)));
    animator.repeatCount(animationHandle(AnimatorHandle::Null, animationHandleData(handle)));
    animator.setRepeatCount(animationHandle(AnimatorHandle::Null, animationHandleData(handle)), 0);
    animator.flags(animationHandle(AnimatorHandle::Null, animationHandleData(handle)));
    animator.setFlags(animationHandle(AnimatorHandle::Null, animationHandleData(handle)), {});
    animator.addFlags(animationHandle(AnimatorHandle::Null, animationHandleData(handle)), {});
    animator.clearFlags(animationHandle(AnimatorHandle::Null, animationHandleData(handle)), {});
    animator.played(animationHandle(AnimatorHandle::Null, animationHandleData(handle)));
    animator.paused(animationHandle(AnimatorHandle::Null, animationHandleData(handle)));
    animator.stopped(animationHandle(AnimatorHandle::Null, animationHandleData(handle)));
    animator.state(animationHandle(AnimatorHandle::Null, animationHandleData(handle)));
    animator.factor(animationHandle(AnimatorHandle::Null, animationHandleData(handle)));
    /* AnimatorDataHandle directly */
    animator.duration(AnimatorDataHandle(0x123abcde));
    animator.repeatCount(AnimatorDataHandle(0x123abcde));
    animator.setRepeatCount(AnimatorDataHandle(0x123abcde), 0);
    animator.flags(AnimatorDataHandle(0x123abcde));
    animator.setFlags(AnimatorDataHandle(0x123abcde), {});
    animator.addFlags(AnimatorDataHandle(0x123abcde), {});
    animator.clearFlags(AnimatorDataHandle(0x123abcde), {});
    animator.played(AnimatorDataHandle(0x123abcde));
    animator.paused(AnimatorDataHandle(0x123abcde));
    animator.stopped(AnimatorDataHandle(0x123abcde));
    animator.state(AnimatorDataHandle(0x123abcde));
    animator.factor(AnimatorDataHandle(0x123abcde));
    CORRADE_COMPARE_AS(out.str(),
        "Whee::AbstractAnimator::duration(): invalid handle Whee::AnimationHandle::Null\n"
        "Whee::AbstractAnimator::repeatCount(): invalid handle Whee::AnimationHandle::Null\n"
        "Whee::AbstractAnimator::setRepeatCount(): invalid handle Whee::AnimationHandle::Null\n"
        "Whee::AbstractAnimator::flags(): invalid handle Whee::AnimationHandle::Null\n"
        "Whee::AbstractAnimator::setFlags(): invalid handle Whee::AnimationHandle::Null\n"
        "Whee::AbstractAnimator::addFlags(): invalid handle Whee::AnimationHandle::Null\n"
        "Whee::AbstractAnimator::clearFlags(): invalid handle Whee::AnimationHandle::Null\n"
        "Whee::AbstractAnimator::played(): invalid handle Whee::AnimationHandle::Null\n"
        "Whee::AbstractAnimator::paused(): invalid handle Whee::AnimationHandle::Null\n"
        "Whee::AbstractAnimator::stopped(): invalid handle Whee::AnimationHandle::Null\n"
        "Whee::AbstractAnimator::state(): invalid handle Whee::AnimationHandle::Null\n"
        "Whee::AbstractAnimator::factor(): invalid handle Whee::AnimationHandle::Null\n"

        "Whee::AbstractAnimator::duration(): invalid handle Whee::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Whee::AbstractAnimator::repeatCount(): invalid handle Whee::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Whee::AbstractAnimator::setRepeatCount(): invalid handle Whee::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Whee::AbstractAnimator::flags(): invalid handle Whee::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Whee::AbstractAnimator::setFlags(): invalid handle Whee::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Whee::AbstractAnimator::addFlags(): invalid handle Whee::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Whee::AbstractAnimator::clearFlags(): invalid handle Whee::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Whee::AbstractAnimator::played(): invalid handle Whee::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Whee::AbstractAnimator::paused(): invalid handle Whee::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Whee::AbstractAnimator::stopped(): invalid handle Whee::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Whee::AbstractAnimator::state(): invalid handle Whee::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Whee::AbstractAnimator::factor(): invalid handle Whee::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"

        "Whee::AbstractAnimator::duration(): invalid handle Whee::AnimationHandle(Null, {0x0, 0x1})\n"
        "Whee::AbstractAnimator::repeatCount(): invalid handle Whee::AnimationHandle(Null, {0x0, 0x1})\n"
        "Whee::AbstractAnimator::setRepeatCount(): invalid handle Whee::AnimationHandle(Null, {0x0, 0x1})\n"
        "Whee::AbstractAnimator::flags(): invalid handle Whee::AnimationHandle(Null, {0x0, 0x1})\n"
        "Whee::AbstractAnimator::setFlags(): invalid handle Whee::AnimationHandle(Null, {0x0, 0x1})\n"
        "Whee::AbstractAnimator::addFlags(): invalid handle Whee::AnimationHandle(Null, {0x0, 0x1})\n"
        "Whee::AbstractAnimator::clearFlags(): invalid handle Whee::AnimationHandle(Null, {0x0, 0x1})\n"
        "Whee::AbstractAnimator::played(): invalid handle Whee::AnimationHandle(Null, {0x0, 0x1})\n"
        "Whee::AbstractAnimator::paused(): invalid handle Whee::AnimationHandle(Null, {0x0, 0x1})\n"
        "Whee::AbstractAnimator::stopped(): invalid handle Whee::AnimationHandle(Null, {0x0, 0x1})\n"
        "Whee::AbstractAnimator::state(): invalid handle Whee::AnimationHandle(Null, {0x0, 0x1})\n"
        "Whee::AbstractAnimator::factor(): invalid handle Whee::AnimationHandle(Null, {0x0, 0x1})\n"

        "Whee::AbstractAnimator::duration(): invalid handle Whee::AnimatorDataHandle(0xabcde, 0x123)\n"
        "Whee::AbstractAnimator::repeatCount(): invalid handle Whee::AnimatorDataHandle(0xabcde, 0x123)\n"
        "Whee::AbstractAnimator::setRepeatCount(): invalid handle Whee::AnimatorDataHandle(0xabcde, 0x123)\n"
        "Whee::AbstractAnimator::flags(): invalid handle Whee::AnimatorDataHandle(0xabcde, 0x123)\n"
        "Whee::AbstractAnimator::setFlags(): invalid handle Whee::AnimatorDataHandle(0xabcde, 0x123)\n"
        "Whee::AbstractAnimator::addFlags(): invalid handle Whee::AnimatorDataHandle(0xabcde, 0x123)\n"
        "Whee::AbstractAnimator::clearFlags(): invalid handle Whee::AnimatorDataHandle(0xabcde, 0x123)\n"
        "Whee::AbstractAnimator::played(): invalid handle Whee::AnimatorDataHandle(0xabcde, 0x123)\n"
        "Whee::AbstractAnimator::paused(): invalid handle Whee::AnimatorDataHandle(0xabcde, 0x123)\n"
        "Whee::AbstractAnimator::stopped(): invalid handle Whee::AnimatorDataHandle(0xabcde, 0x123)\n"
        "Whee::AbstractAnimator::state(): invalid handle Whee::AnimatorDataHandle(0xabcde, 0x123)\n"
        "Whee::AbstractAnimator::factor(): invalid handle Whee::AnimatorDataHandle(0xabcde, 0x123)\n",
        TestSuite::Compare::String);
}

void AbstractAnimatorTest::clean() {
    struct: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;
        using AbstractAnimator::create;
        using AbstractAnimator::remove;

        AnimatorFeatures doFeatures() const override { return {}; }
        void doClean(Containers::BitArrayView animationIdsToRemove) override {
            ++called;
            CORRADE_COMPARE_AS(animationIdsToRemove, Containers::stridedArrayView({
                true, false, true, false
            }).sliceBit(0), TestSuite::Compare::Container);
        }

        Int called = 0;
    } animator{animatorHandle(0, 1)};

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    /* Create four animations to match the four bits, remove one of them */
    AnimationHandle first = animator.create(7_nsec, 65_nsec);
    AnimationHandle second = animator.create(2_nsec, 3_nsec);
    AnimationHandle third = animator.create(323_nsec, 2_nsec);
    AnimationHandle fourth = animator.create(0_nsec, 1_nsec);
    animator.remove(second);

    /* Call clean() */
    UnsignedByte data[]{0x05};
    animator.clean(Containers::BitArrayView{data, 0, 4});
    CORRADE_COMPARE(animator.called, 1);

    /* Only the fourth data should stay afterwards */
    CORRADE_VERIFY(!animator.isHandleValid(first));
    CORRADE_VERIFY(!animator.isHandleValid(second));
    CORRADE_VERIFY(!animator.isHandleValid(third));
    CORRADE_VERIFY(animator.isHandleValid(fourth));
}

void AbstractAnimatorTest::cleanEmpty() {
    struct: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;

        AnimatorFeatures doFeatures() const override { return {}; }
        void doClean(Containers::BitArrayView) override {
            ++called;
        }

        Int called = 0;
    } animator{animatorHandle(0, 1)};

    /* It should call the implementation even with empty contents */
    animator.clean({});
    CORRADE_COMPARE(animator.called, 1);
}

void AbstractAnimatorTest::cleanNotImplemented() {
    struct: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;

        AnimatorFeatures doFeatures() const override { return {}; }
    } animator{animatorHandle(0, 1)};

    animator.clean({});

    /* Shouldn't crash or anything */
    CORRADE_VERIFY(true);
}

void AbstractAnimatorTest::cleanInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;
        using AbstractAnimator::create;

        AnimatorFeatures doFeatures() const override { return {}; }
        void doClean(Containers::BitArrayView) override {
            CORRADE_FAIL("This shouldn't get called.");
        }
    } animator{animatorHandle(0, 1)};

    animator.create(0_nsec, 1_nsec);
    animator.create(0_nsec, 1_nsec);
    animator.create(0_nsec, 1_nsec);

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    std::ostringstream out;
    Error redirectError{&out};
    UnsignedByte data[1]{};
    animator.clean(Containers::BitArrayView{data, 0, 2});
    CORRADE_COMPARE(out.str(), "Whee::AbstractAnimator::clean(): expected 3 bits but got 2\n");
}

void AbstractAnimatorTest::playPauseStop() {
    struct: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;
        using AbstractAnimator::create;

        AnimatorFeatures doFeatures() const override { return {}; }
    } animator{animatorHandle(0, 1)};

    /* The NeedsAdvance flag is thoroughly tested in state() instead, animation
       state and factor in propertiesStateFactor() instead; unpausing behavior
       in playPaused() and playPausedStopped(). This solely verifies that the
       internal data get correctly updated after all API call variants. */

    /* So it doesn't always pick the first one */
    animator.create(10_nsec, 50_nsec);

    AnimationHandle handle = animator.create(1000_nsec, 10_nsec);
    CORRADE_COMPARE(animator.played(handle), 1000_nsec);
    CORRADE_COMPARE(animator.duration(handle), 10_nsec);
    CORRADE_COMPARE(animator.paused(handle), Nanoseconds::max());
    CORRADE_COMPARE(animator.stopped(handle), Nanoseconds::max());

    animator.pause(handle, 1005_nsec);
    CORRADE_COMPARE(animator.played(handle), 1000_nsec);
    CORRADE_COMPARE(animator.duration(handle), 10_nsec);
    CORRADE_COMPARE(animator.paused(handle), 1005_nsec);
    CORRADE_COMPARE(animator.stopped(handle), Nanoseconds::max());

    animator.stop(handle, 1007_nsec);
    /* NeedsAdvance is only reset by advance(), not if any animations get
       stopped */
    CORRADE_COMPARE(animator.played(handle), 1000_nsec);
    CORRADE_COMPARE(animator.duration(handle), 10_nsec);
    CORRADE_COMPARE(animator.paused(handle), 1005_nsec);
    CORRADE_COMPARE(animator.stopped(handle), 1007_nsec);

    animator.play(handle, 500_nsec);
    CORRADE_COMPARE(animator.played(handle), 500_nsec);
    CORRADE_COMPARE(animator.duration(handle), 10_nsec);
    CORRADE_COMPARE(animator.paused(handle), Nanoseconds::max());
    CORRADE_COMPARE(animator.stopped(handle), Nanoseconds::max());

    /* Testing also the AnimatorDataHandle overloads */
    animator.pause(animationHandleData(handle), 990_nsec);
    CORRADE_COMPARE(animator.played(handle), 500_nsec);
    CORRADE_COMPARE(animator.duration(handle), 10_nsec);
    CORRADE_COMPARE(animator.paused(handle), 990_nsec);
    CORRADE_COMPARE(animator.stopped(handle), Nanoseconds::max());

    animator.stop(animationHandleData(handle), 550_nsec);
    CORRADE_COMPARE(animator.played(handle), 500_nsec);
    CORRADE_COMPARE(animator.duration(handle), 10_nsec);
    CORRADE_COMPARE(animator.paused(handle), 990_nsec);
    CORRADE_COMPARE(animator.stopped(handle), 550_nsec);

    animator.play(animationHandleData(handle), 400_nsec);
    CORRADE_COMPARE(animator.played(handle), 400_nsec);
    CORRADE_COMPARE(animator.duration(handle), 10_nsec);
    CORRADE_COMPARE(animator.paused(handle), Nanoseconds::max());
    CORRADE_COMPARE(animator.stopped(handle), Nanoseconds::max());
}

void AbstractAnimatorTest::playPauseStopInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;
        using AbstractAnimator::create;

        AnimatorFeatures doFeatures() const override { return {}; }
    } animator{animatorHandle(0, 1)};

    AnimationHandle handle = animator.create(12_nsec, 13_nsec);

    std::ostringstream out;
    Error redirectError{&out};
    animator.play(AnimationHandle::Null, 0_nsec);
    animator.pause(AnimationHandle::Null, 0_nsec);
    animator.stop(AnimationHandle::Null, 0_nsec);
    /* Valid animator, invalid data */
    animator.play(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)), 0_nsec);
    animator.pause(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)), 0_nsec);
    animator.stop(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)), 0_nsec);
    /* Invalid animator, valid data */
    animator.play(animationHandle(AnimatorHandle::Null, animationHandleData(handle)), 0_nsec);
    animator.pause(animationHandle(AnimatorHandle::Null, animationHandleData(handle)), 0_nsec);
    animator.stop(animationHandle(AnimatorHandle::Null, animationHandleData(handle)), 0_nsec);
    /* AnimatorDataHandle directly */
    animator.play(AnimatorDataHandle(0x123abcde), 0_nsec);
    animator.pause(AnimatorDataHandle(0x123abcde), 0_nsec);
    animator.stop(AnimatorDataHandle(0x123abcde), 0_nsec);
    CORRADE_COMPARE_AS(out.str(),
        "Whee::AbstractAnimator::play(): invalid handle Whee::AnimationHandle::Null\n"
        "Whee::AbstractAnimator::pause(): invalid handle Whee::AnimationHandle::Null\n"
        "Whee::AbstractAnimator::stop(): invalid handle Whee::AnimationHandle::Null\n"

        "Whee::AbstractAnimator::play(): invalid handle Whee::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Whee::AbstractAnimator::pause(): invalid handle Whee::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Whee::AbstractAnimator::stop(): invalid handle Whee::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"

        "Whee::AbstractAnimator::play(): invalid handle Whee::AnimationHandle(Null, {0x0, 0x1})\n"
        "Whee::AbstractAnimator::pause(): invalid handle Whee::AnimationHandle(Null, {0x0, 0x1})\n"
        "Whee::AbstractAnimator::stop(): invalid handle Whee::AnimationHandle(Null, {0x0, 0x1})\n"

        "Whee::AbstractAnimator::play(): invalid handle Whee::AnimatorDataHandle(0xabcde, 0x123)\n"
        "Whee::AbstractAnimator::pause(): invalid handle Whee::AnimatorDataHandle(0xabcde, 0x123)\n"
        "Whee::AbstractAnimator::stop(): invalid handle Whee::AnimatorDataHandle(0xabcde, 0x123)\n",
        TestSuite::Compare::String);
}

void AbstractAnimatorTest::playPaused() {
    auto&& data = PlayPausedData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;
        using AbstractAnimator::create;

        AnimatorFeatures doFeatures() const override { return {}; }
    } animator{animatorHandle(0, 1)};

    AnimationHandle handle = animator.create(10_nsec, 100_nsec);
    CORRADE_COMPARE(animator.played(handle), 10_nsec);
    CORRADE_COMPARE(animator.duration(handle), 100_nsec);
    CORRADE_COMPARE(animator.paused(handle), Nanoseconds::max());
    CORRADE_COMPARE(animator.stopped(handle), Nanoseconds::max());

    /* Stopping the animation shouldn't affect anything here */
    if(data.stopped)
        animator.stop(handle, *data.stopped);

    /* Pausing only records how long the animation have been playing, doesn't
       touch anything else */
    animator.pause(handle, data.paused);
    CORRADE_COMPARE(animator.played(handle), 10_nsec);
    CORRADE_COMPARE(animator.duration(handle), 100_nsec);
    CORRADE_COMPARE(animator.paused(handle), data.paused);
    CORRADE_COMPARE(animator.stopped(handle), data.stopped ? *data.stopped : Nanoseconds::max());

    /* Playing either adjusts the played time to resume from where it was
       paused, or plays from the start. The paused and stopped time gets reset
       always, unconditionally. */
    animator.play(handle, data.resumed);
    CORRADE_COMPARE(animator.played(handle), data.expectedPlayed);
    CORRADE_COMPARE(animator.duration(handle), 100_nsec);
    CORRADE_COMPARE(animator.paused(handle), Nanoseconds::max());
    CORRADE_COMPARE(animator.stopped(handle), Nanoseconds::max());
}

void AbstractAnimatorTest::advance() {
    struct: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;
        using AbstractAnimator::create;
        using AbstractAnimator::remove;

        AnimatorFeatures doFeatures() const override { return {}; }
        /* doClean() gets called by us, no need to check anything */
    } animator{animatorHandle(0, 1)};
    CORRADE_COMPARE(animator.time(), 0_nsec);
    CORRADE_COMPARE(animator.state(), AnimatorStates{});

    /* State should change at 0, 10, 20, 30, 40. Tests mainly the interaction
       between previous and current state, the actual interpolation is tested
       in propertiesStateFactor(). */
    AnimationHandle scheduledKeep = animator.create(30_nsec, 10_nsec, AnimationFlag::KeepOncePlayed);
    AnimationHandle scheduledToPlayingBegin = animator.create(10_nsec, 10_nsec);
    AnimationHandle scheduledToPaused = animator.create(5_nsec, 10_nsec);
    AnimationHandle scheduledToStopped = animator.create(5_nsec, 10_nsec);
    AnimationHandle removed = animator.create(0_nsec, 6_nsec);
    AnimationHandle playingMiddleKeep = animator.create(-20_nsec, 40_nsec, AnimationFlag::KeepOncePlayed);
    AnimationHandle playingToPausedKeep = animator.create(-10_nsec, 20_nsec, AnimationFlag::KeepOncePlayed);
    AnimationHandle playingEndToStopped = animator.create(0_nsec, 10_nsec);
    AnimationHandle playingToStoppedKeep = animator.create(0_nsec, 5_nsec, AnimationFlag::KeepOncePlayed);
    AnimationHandle paused = animator.create(-40_nsec, 10_nsec);
    AnimationHandle pausedToStopped = animator.create(-40_nsec, 10_nsec);
    AnimationHandle stopped = animator.create(-40_nsec, 30_nsec);
    AnimationHandle stoppedKeep = animator.create(-40_nsec, 30_nsec, AnimationFlag::KeepOncePlayed);
    animator.remove(removed);
    animator.pause(scheduledToPaused, 8_nsec); /* pauses at 3/10 */
    animator.pause(playingToPausedKeep, 5_nsec); /* pauses at 15/20 */
    animator.pause(paused, -35_nsec); /* pauses at 5/10 */
    animator.pause(pausedToStopped, -35_nsec);
    animator.stop(scheduledToStopped, 8_nsec);
    animator.stop(pausedToStopped, 8_nsec);
    CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);

    CORRADE_COMPARE(animator.state(scheduledKeep), AnimationState::Scheduled);
    CORRADE_COMPARE(animator.state(scheduledToPlayingBegin), AnimationState::Scheduled);
    CORRADE_COMPARE(animator.state(scheduledToPaused), AnimationState::Scheduled);
    CORRADE_COMPARE(animator.state(scheduledToStopped), AnimationState::Scheduled);
    /* removed is gone */
    CORRADE_COMPARE(animator.state(playingMiddleKeep), AnimationState::Playing);
    CORRADE_COMPARE(animator.state(playingToPausedKeep), AnimationState::Playing);
    /* playingEndToStopped is gone */
    CORRADE_COMPARE(animator.state(playingEndToStopped), AnimationState::Playing);
    CORRADE_COMPARE(animator.state(playingToStoppedKeep), AnimationState::Playing);
    CORRADE_COMPARE(animator.state(paused), AnimationState::Paused);
    CORRADE_COMPARE(animator.state(pausedToStopped), AnimationState::Paused);
    CORRADE_COMPARE(animator.state(stopped), AnimationState::Stopped);
    CORRADE_COMPARE(animator.state(stoppedKeep), AnimationState::Stopped);

    constexpr Float unused = Constants::inf();

    /* Call to advance(10) advances also stopped and paused animations that
       changed their state compared to last time (i.e., time 0) */
    {
        Containers::BitArray active{ValueInit, 13};
        Containers::StaticArray<13, Float> factors{DirectInit, unused};
        Containers::BitArray remove{ValueInit, 13};
        CORRADE_COMPARE(animator.advance(10_nsec, active, factors, remove), Containers::pair(true, true));
        CORRADE_COMPARE_AS(Containers::BitArrayView{active}, Containers::stridedArrayView({
            false,  /*  0 scheduledKeep */
            true,   /*  1 scheduledToPlayingBegin */
            true,   /*  2 scheduledToPaused */
            true,   /*  3 scheduledToStopped */
            false,  /*  4 removed */
            true,   /*  5 playingMiddleKeep */
            true,   /*  6 playingToPausedKeep */
            true,   /*  7 playingEndToStopped */
            true,   /*  8 playingToStoppedKeep */
            false,  /*  9 paused */
            true,   /* 10 pausedToStopped */
            false,  /* 11 stopped */
            false,  /* 12 stoppedKeep */
        }).sliceBit(0), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(factors, Containers::arrayView({
            unused, /*  0 scheduledKeep */
            0.0f,   /*  1 scheduledToPlayingBegin */
            0.3f,   /*  2 scheduledToPaused */
            1.0f,   /*  3 scheduledToStopped */
            unused, /*  4 removed */
            0.75f,  /*  5 playingMiddleKeep */
            0.75f,  /*  6 playingToPausedKeep */
            1.0f,   /*  7 playingEndToStopped */
            1.0f,   /*  8 playingToStoppedKeep */
            unused, /*  9 paused */
            1.0f,   /* 10 pausedToStopped */
            unused, /* 11 stopped */
            unused, /* 12 stoppedKeep */
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(Containers::BitArrayView{remove}, Containers::stridedArrayView({
            false,  /*  0 scheduledKeep */
            false,  /*  1 scheduledToPlayingBegin */
            false,  /*  2 scheduledToPaused */
            true,   /*  3 scheduledToStopped */
            false,  /*  4 removed */
            false,  /*  5 playingMiddleKeep */
            false,  /*  6 playingToPausedKeep */
            true,   /*  7 playingEndToStopped */
            false,  /*  8 playingToStoppedKeep */
            false,  /*  9 paused */
            true,   /* 10 pausedToStopped */
            true,   /* 11 stopped */
            false,  /* 12 stoppedKeep */
        }).sliceBit(0), TestSuite::Compare::Container);

        /* Need to call this ourselves to not have the removed animations
           picked up again next time */
        animator.clean(remove);
    }
    CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);

    CORRADE_VERIFY(animator.isHandleValid(scheduledKeep));
    CORRADE_VERIFY(animator.isHandleValid(scheduledToPlayingBegin));
    CORRADE_VERIFY(animator.isHandleValid(scheduledToPaused));
    CORRADE_VERIFY(!animator.isHandleValid(scheduledToStopped));
    CORRADE_VERIFY(!animator.isHandleValid(removed));
    CORRADE_VERIFY(animator.isHandleValid(playingMiddleKeep));
    CORRADE_VERIFY(animator.isHandleValid(playingToPausedKeep));
    CORRADE_VERIFY(!animator.isHandleValid(playingEndToStopped));
    CORRADE_VERIFY(animator.isHandleValid(playingToStoppedKeep));
    CORRADE_VERIFY(animator.isHandleValid(paused));
    CORRADE_VERIFY(!animator.isHandleValid(pausedToStopped));
    CORRADE_VERIFY(!animator.isHandleValid(stopped));
    CORRADE_VERIFY(animator.isHandleValid(stoppedKeep));

    CORRADE_COMPARE(animator.state(scheduledKeep), AnimationState::Scheduled);
    CORRADE_COMPARE(animator.state(scheduledToPlayingBegin), AnimationState::Playing);
    CORRADE_COMPARE(animator.state(scheduledToPaused), AnimationState::Paused);
    /* scheduledToStopped is gone */
    /* removed is gone */
    CORRADE_COMPARE(animator.state(playingMiddleKeep), AnimationState::Playing);
    CORRADE_COMPARE(animator.state(playingToPausedKeep), AnimationState::Paused);
    /* playingEndToStopped is gone */
    CORRADE_COMPARE(animator.state(playingToStoppedKeep), AnimationState::Stopped);
    CORRADE_COMPARE(animator.state(paused), AnimationState::Paused);
    /* pausedToStopped is gone */
    /* stopped is gone */
    CORRADE_COMPARE(animator.state(stoppedKeep), AnimationState::Stopped);

    /* Call to advance(10) again marks only the currently playing animations as
       active. As there's no difference in current and previous state and all
       stopped animations got already removed, clean() isn't meant to be
       called. */
    {
        Containers::BitArray active{ValueInit, 13};
        Containers::StaticArray<13, Float> factors{DirectInit, unused};
        Containers::BitArray remove{ValueInit, 13};
        CORRADE_COMPARE(animator.advance(10_nsec, active, factors, remove), Containers::pair(true, false));
        CORRADE_COMPARE_AS(Containers::BitArrayView{active}, Containers::stridedArrayView({
            false,  /*  0 scheduledKeep */
            true,   /*  1 scheduledToPlayingBegin */
            false,  /*  2 scheduledToPaused */
            false,  /*  3 scheduledToStopped */
            false,  /*  4 removed */
            true,   /*  5 playingMiddleKeep */
            false,  /*  6 playingToPausedKeep */
            false,  /*  7 playingEndToStopped */
            false,  /*  8 playingToStoppedKeep */
            false,  /*  9 paused */
            false,  /* 10 pausedToStopped */
            false,  /* 11 stopped */
            false,  /* 12 stoppedKeep */
        }).sliceBit(0), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(factors, Containers::arrayView({
            unused, /*  0 scheduledKeep */
            0.0f,   /*  1 scheduledToPlayingBegin */
            unused, /*  2 scheduledToPaused */
            unused, /*  3 scheduledToStopped */
            unused, /*  4 removed */
            0.75f,  /*  5 playingMiddleKeep */
            unused, /*  6 playingToPausedKeep */
            unused, /*  7 playingEndToStopped */
            unused, /*  8 playingToStoppedKeep */
            unused, /*  9 paused */
            unused, /* 10 pausedToStopped */
            unused, /* 11 stopped */
            unused, /* 12 stoppedKeep */
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(remove,
            (Containers::BitArray{DirectInit, 13, false}),
            TestSuite::Compare::Container);

        /* Need to call this ourselves to not have the removed animations
           picked up again next time */
        animator.clean(remove);
    }
    CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);

    /* Same as before */
    CORRADE_VERIFY(animator.isHandleValid(scheduledKeep));
    CORRADE_VERIFY(animator.isHandleValid(scheduledToPlayingBegin));
    CORRADE_VERIFY(animator.isHandleValid(scheduledToPaused));
    CORRADE_VERIFY(!animator.isHandleValid(scheduledToStopped));
    CORRADE_VERIFY(!animator.isHandleValid(removed));
    CORRADE_VERIFY(animator.isHandleValid(playingMiddleKeep));
    CORRADE_VERIFY(animator.isHandleValid(playingToPausedKeep));
    CORRADE_VERIFY(!animator.isHandleValid(playingEndToStopped));
    CORRADE_VERIFY(animator.isHandleValid(playingToStoppedKeep));
    CORRADE_VERIFY(animator.isHandleValid(paused));
    CORRADE_VERIFY(!animator.isHandleValid(pausedToStopped));
    CORRADE_VERIFY(!animator.isHandleValid(stopped));
    CORRADE_VERIFY(animator.isHandleValid(stoppedKeep));

    /* Same as before */
    CORRADE_COMPARE(animator.state(scheduledKeep), AnimationState::Scheduled);
    CORRADE_COMPARE(animator.state(scheduledToPlayingBegin), AnimationState::Playing);
    CORRADE_COMPARE(animator.state(scheduledToPaused), AnimationState::Paused);
    /* scheduledToStopped is gone */
    /* removed is gone */
    CORRADE_COMPARE(animator.state(playingMiddleKeep), AnimationState::Playing);
    CORRADE_COMPARE(animator.state(playingToPausedKeep), AnimationState::Paused);
    /* playingEndToStopped is gone */
    CORRADE_COMPARE(animator.state(playingToStoppedKeep), AnimationState::Stopped);
    CORRADE_COMPARE(animator.state(paused), AnimationState::Paused);
    /* pausedToStopped is gone */
    /* stopped is gone */
    CORRADE_COMPARE(animator.state(stoppedKeep), AnimationState::Stopped);

    /* Call at 20 advances only animations that weren't stopped and paused
       before as well. The active mask is thus the same as the second call at
       10. */
    {
        Containers::BitArray active{ValueInit, 13};
        Containers::StaticArray<13, Float> factors{DirectInit, unused};
        Containers::BitArray remove{ValueInit, 13};
        CORRADE_COMPARE(animator.advance(20_nsec, active, factors, remove), Containers::pair(true, true));
        CORRADE_COMPARE_AS(Containers::BitArrayView{active}, Containers::stridedArrayView({
            false,  /*  0 scheduledKeep */
            true,   /*  1 scheduledToPlayingBegin */
            false,  /*  2 scheduledToPaused */
            false,  /*  3 scheduledToStopped */
            false,  /*  4 removed */
            true,   /*  5 playingMiddleKeep */
            false,  /*  6 playingToPausedKeep */
            false,  /*  7 playingEndToStopped */
            false,  /*  8 playingToStoppedKeep */
            false,  /*  9 paused */
            false,  /* 10 pausedToStopped */
            false,  /* 11 stopped */
            false,  /* 12 stoppedKeep */
        }).sliceBit(0), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(factors, Containers::arrayView({
            unused, /*  0 scheduledKeep */
            1.0f,   /*  1 scheduledToPlayingBegin */
            unused, /*  2 scheduledToPaused */
            unused, /*  3 scheduledToStopped */
            unused, /*  4 removed */
            1.0f,   /*  5 playingMiddleKeep */
            unused, /*  6 playingToPausedKeep */
            unused, /*  7 playingEndToStopped */
            unused, /*  8 playingToStoppedKeep */
            unused, /*  9 paused */
            unused, /* 10 pausedToStopped */
            unused, /* 11 stopped */
            unused, /* 12 stoppedKeep */
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(Containers::BitArrayView{remove}, Containers::stridedArrayView({
            false,  /*  0 scheduledKeep */
            true,   /*  1 scheduledToPlayingBegin */
            false,  /*  2 scheduledToPaused */
            false,  /*  3 scheduledToStopped */
            false,  /*  4 removed */
            false,  /*  5 playingMiddleKeep */
            false,  /*  6 playingToPausedKeep */
            false,  /*  7 playingEndToStopped */
            false,  /*  8 playingToStoppedKeep */
            false,  /*  9 paused */
            false,  /* 10 pausedToStopped */
            false,  /* 11 stopped */
            false,  /* 12 stoppedKeep */
        }).sliceBit(0), TestSuite::Compare::Container);

        /* Need to call this ourselves to not have the removed animations
           picked up again next time */
        animator.clean(remove);
    }
    CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);

    /* The scheduledToPlayingBegin gets removed, playingMiddleKeep not because
       is marked as such */
    CORRADE_VERIFY(animator.isHandleValid(scheduledKeep));
    CORRADE_VERIFY(!animator.isHandleValid(scheduledToPlayingBegin));
    CORRADE_VERIFY(animator.isHandleValid(scheduledToPaused));
    CORRADE_VERIFY(!animator.isHandleValid(scheduledToStopped));
    CORRADE_VERIFY(!animator.isHandleValid(removed));
    CORRADE_VERIFY(animator.isHandleValid(playingMiddleKeep));
    CORRADE_VERIFY(animator.isHandleValid(playingToPausedKeep));
    CORRADE_VERIFY(!animator.isHandleValid(playingEndToStopped));
    CORRADE_VERIFY(animator.isHandleValid(playingToStoppedKeep));
    CORRADE_VERIFY(animator.isHandleValid(paused));
    CORRADE_VERIFY(!animator.isHandleValid(pausedToStopped));
    CORRADE_VERIFY(!animator.isHandleValid(stopped));
    CORRADE_VERIFY(animator.isHandleValid(stoppedKeep));

    CORRADE_COMPARE(animator.state(scheduledKeep), AnimationState::Scheduled);
    /* scheduledToPlayingBegin is gone */
    CORRADE_COMPARE(animator.state(scheduledToPaused), AnimationState::Paused);
    /* scheduledToStopped is gone */
    /* removed is gone */
    CORRADE_COMPARE(animator.state(playingMiddleKeep), AnimationState::Stopped);
    CORRADE_COMPARE(animator.state(playingToPausedKeep), AnimationState::Paused);
    /* playingEndToStopped is gone */
    CORRADE_COMPARE(animator.state(playingToStoppedKeep), AnimationState::Stopped);
    CORRADE_COMPARE(animator.state(paused), AnimationState::Paused);
    /* pausedToStopped is gone */
    /* stopped is gone */
    CORRADE_COMPARE(animator.state(stoppedKeep), AnimationState::Stopped);

    /* After stopping what's paused, call at 30 advances the remaining
       animation, after that there's nothing to advance anymore */
    animator.stop(scheduledToPaused, 30_nsec);
    animator.stop(playingToPausedKeep, 30_nsec);
    animator.stop(paused, 30_nsec);
    {
        Containers::BitArray active{ValueInit, 13};
        Containers::StaticArray<13, Float> factors{DirectInit, unused};
        Containers::BitArray remove{ValueInit, 13};
        CORRADE_COMPARE(animator.advance(30_nsec, active, factors, remove), Containers::pair(true, true));
        CORRADE_COMPARE_AS(Containers::BitArrayView{active}, Containers::stridedArrayView({
            true,   /*  0 scheduledKeep */
            false,  /*  1 scheduledToPlayingBegin */
            true,   /*  2 scheduledToPaused */
            false,  /*  3 scheduledToStopped */
            false,  /*  4 removed */
            false,  /*  5 playingMiddleKeep */
            true,   /*  6 playingToPausedKeep */
            false,  /*  7 playingEndToStopped */
            false,  /*  8 playingToStoppedKeep */
            true,   /*  9 paused */
            false,  /* 10 pausedToStopped */
            false,  /* 11 stopped */
            false,  /* 12 stoppedKeep */
        }).sliceBit(0), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(factors, Containers::arrayView({
            0.0f,   /*  0 scheduledKeep */
            unused, /*  1 scheduledToPlayingBegin */
            1.0f,   /*  2 scheduledToPaused */
            unused, /*  3 scheduledToStopped */
            unused, /*  4 removed */
            unused, /*  5 playingMiddleKeep */
            1.0f,   /*  6 playingToPausedKeep */
            unused, /*  7 playingEndToStopped */
            unused, /*  8 playingToStoppedKeep */
            1.0f,   /*  9 paused */
            unused, /* 10 pausedToStopped */
            unused, /* 11 stopped */
            unused, /* 12 stoppedKeep */
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(Containers::BitArrayView{remove}, Containers::stridedArrayView({
            false,  /*  0 scheduledKeep */
            false,  /*  1 scheduledToPlayingBegin */
            true,   /*  2 scheduledToPaused */
            false,  /*  3 scheduledToStopped */
            false,  /*  4 removed */
            false,  /*  5 playingMiddleKeep */
            false,  /*  6 playingToPausedKeep */
            false,  /*  7 playingEndToStopped */
            false,  /*  8 playingToStoppedKeep */
            true,   /*  9 paused */
            false,  /* 10 pausedToStopped */
            false,  /* 11 stopped */
            false,  /* 12 stoppedKeep */
        }).sliceBit(0), TestSuite::Compare::Container);

        /* Need to call this ourselves to not have the removed animations
           picked up again next time */
        animator.clean(remove);
    }
    CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);

    /* The scheduledToPaused and paused gets removed, playingToPausedKeep not
       because is marked as such */
    CORRADE_VERIFY(animator.isHandleValid(scheduledKeep));
    CORRADE_VERIFY(!animator.isHandleValid(scheduledToPlayingBegin));
    CORRADE_VERIFY(!animator.isHandleValid(scheduledToPaused));
    CORRADE_VERIFY(!animator.isHandleValid(scheduledToStopped));
    CORRADE_VERIFY(!animator.isHandleValid(removed));
    CORRADE_VERIFY(animator.isHandleValid(playingMiddleKeep));
    CORRADE_VERIFY(animator.isHandleValid(playingToPausedKeep));
    CORRADE_VERIFY(!animator.isHandleValid(playingEndToStopped));
    CORRADE_VERIFY(animator.isHandleValid(playingToStoppedKeep));
    CORRADE_VERIFY(!animator.isHandleValid(paused));
    CORRADE_VERIFY(!animator.isHandleValid(pausedToStopped));
    CORRADE_VERIFY(!animator.isHandleValid(stopped));
    CORRADE_VERIFY(animator.isHandleValid(stoppedKeep));

    CORRADE_COMPARE(animator.state(scheduledKeep), AnimationState::Playing);
    /* scheduledToPlayingBegin is gone */
    /* scheduledToPaused is gone */
    /* scheduledToStopped is gone */
    /* removed is gone */
    CORRADE_COMPARE(animator.state(playingMiddleKeep), AnimationState::Stopped);
    CORRADE_COMPARE(animator.state(playingToPausedKeep), AnimationState::Stopped);
    /* playingEndToStopped is gone */
    CORRADE_COMPARE(animator.state(playingToStoppedKeep), AnimationState::Stopped);
    /* paused is gone */
    /* pausedToStopped is gone */
    /* stopped is gone */
    CORRADE_COMPARE(animator.state(stoppedKeep), AnimationState::Stopped);

    /* Call at 40 doesn't need to delegeate to clean() anymore */
    {
        Containers::BitArray active{ValueInit, 13};
        Containers::StaticArray<13, Float> factors{DirectInit, unused};
        Containers::BitArray remove{ValueInit, 13};
        CORRADE_COMPARE(animator.advance(40_nsec, active, factors, remove), Containers::pair(true, false));
        CORRADE_COMPARE_AS(Containers::BitArrayView{active}, Containers::stridedArrayView({
            true,   /*  0 scheduledKeep */
            false,  /*  1 scheduledToPlayingBegin */
            false,  /*  2 scheduledToPaused */
            false,  /*  3 scheduledToStopped */
            false,  /*  4 removed */
            false,  /*  5 playingMiddleKeep */
            false,  /*  6 playingToPausedKeep */
            false,  /*  7 playingEndToStopped */
            false,  /*  8 playingToStoppedKeep */
            false,  /*  9 paused */
            false,  /* 10 pausedToStopped */
            false,  /* 11 stopped */
            false,  /* 12 stoppedKeep */
        }).sliceBit(0), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(factors, Containers::arrayView({
            1.0f,   /*  0 scheduledKeep */
            unused, /*  1 scheduledToPlayingBegin */
            unused, /*  2 scheduledToPaused */
            unused, /*  3 scheduledToStopped */
            unused, /*  4 removed */
            unused, /*  5 playingMiddleKeep */
            unused, /*  6 playingToPausedKeep */
            unused, /*  7 playingEndToStopped */
            unused, /*  8 playingToStoppedKeep */
            unused, /*  9 paused */
            unused, /* 10 pausedToStopped */
            unused, /* 11 stopped */
            unused, /* 12 stoppedKeep */
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(remove,
            (Containers::BitArray{DirectInit, 13, false}),
            TestSuite::Compare::Container);

        /* Nothing to remove, not calling clean() */
    }
    /* It also doesn't need to advance anything after this */
    CORRADE_COMPARE(animator.state(), AnimatorStates{});

    /* Same as before */
    CORRADE_VERIFY(animator.isHandleValid(scheduledKeep));
    CORRADE_VERIFY(!animator.isHandleValid(scheduledToPlayingBegin));
    CORRADE_VERIFY(!animator.isHandleValid(scheduledToPaused));
    CORRADE_VERIFY(!animator.isHandleValid(scheduledToStopped));
    CORRADE_VERIFY(!animator.isHandleValid(removed));
    CORRADE_VERIFY(animator.isHandleValid(playingMiddleKeep));
    CORRADE_VERIFY(animator.isHandleValid(playingToPausedKeep));
    CORRADE_VERIFY(!animator.isHandleValid(playingEndToStopped));
    CORRADE_VERIFY(animator.isHandleValid(playingToStoppedKeep));
    CORRADE_VERIFY(!animator.isHandleValid(paused));
    CORRADE_VERIFY(!animator.isHandleValid(pausedToStopped));
    CORRADE_VERIFY(!animator.isHandleValid(stopped));
    CORRADE_VERIFY(animator.isHandleValid(stoppedKeep));

    CORRADE_COMPARE(animator.state(scheduledKeep), AnimationState::Stopped);
    /* scheduledToPlayingBegin is gone */
    /* scheduledToPaused is gone */
    /* scheduledToStopped is gone */
    /* removed is gone */
    CORRADE_COMPARE(animator.state(playingMiddleKeep), AnimationState::Stopped);
    CORRADE_COMPARE(animator.state(playingToPausedKeep), AnimationState::Stopped);
    /* playingEndToStopped is gone */
    CORRADE_COMPARE(animator.state(playingToStoppedKeep), AnimationState::Stopped);
    /* paused is gone */
    /* pausedToStopped is gone */
    /* stopped is gone */
    CORRADE_COMPARE(animator.state(stoppedKeep), AnimationState::Stopped);

    /* Call at 50 needs neither advance nor clean anymore */
    {
        Containers::BitArray active{ValueInit, 13};
        Containers::StaticArray<13, Float> factors{DirectInit, unused};
        Containers::BitArray remove{ValueInit, 13};
        CORRADE_COMPARE(animator.advance(50_nsec, active, factors, remove), Containers::pair(false, false));
        CORRADE_COMPARE_AS(active,
            (Containers::BitArray{DirectInit, 13, false}),
            TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(factors,
            (Containers::StaticArray<13, Float>{DirectInit, unused}),
            TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(remove,
            (Containers::BitArray{DirectInit, 13, false}),
            TestSuite::Compare::Container);

        /* Nothing to remove, not calling clean() */
    }
    CORRADE_COMPARE(animator.state(), AnimatorStates{});

    /* Same as before */
    CORRADE_VERIFY(animator.isHandleValid(scheduledKeep));
    CORRADE_VERIFY(!animator.isHandleValid(scheduledToPlayingBegin));
    CORRADE_VERIFY(!animator.isHandleValid(scheduledToPaused));
    CORRADE_VERIFY(!animator.isHandleValid(scheduledToStopped));
    CORRADE_VERIFY(!animator.isHandleValid(removed));
    CORRADE_VERIFY(animator.isHandleValid(playingMiddleKeep));
    CORRADE_VERIFY(animator.isHandleValid(playingToPausedKeep));
    CORRADE_VERIFY(!animator.isHandleValid(playingEndToStopped));
    CORRADE_VERIFY(animator.isHandleValid(playingToStoppedKeep));
    CORRADE_VERIFY(!animator.isHandleValid(paused));
    CORRADE_VERIFY(!animator.isHandleValid(pausedToStopped));
    CORRADE_VERIFY(!animator.isHandleValid(stopped));
    CORRADE_VERIFY(animator.isHandleValid(stoppedKeep));

    /* Same as before */
    CORRADE_COMPARE(animator.state(scheduledKeep), AnimationState::Stopped);
    /* scheduledToPlayingBegin is gone */
    /* scheduledToPaused is gone */
    /* scheduledToStopped is gone */
    /* removed is gone */
    CORRADE_COMPARE(animator.state(playingMiddleKeep), AnimationState::Stopped);
    CORRADE_COMPARE(animator.state(playingToPausedKeep), AnimationState::Stopped);
    /* playingEndToStopped is gone */
    CORRADE_COMPARE(animator.state(playingToStoppedKeep), AnimationState::Stopped);
    /* paused is gone */
    /* pausedToStopped is gone */
    /* stopped is gone */
    CORRADE_COMPARE(animator.state(stoppedKeep), AnimationState::Stopped);
}

void AbstractAnimatorTest::advanceEmpty() {
    struct: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;

        AnimatorFeatures doFeatures() const override { return {}; }
    } animator{animatorHandle(0, 1)};
    CORRADE_COMPARE(animator.time(), 0_nsec);
    CORRADE_COMPARE(animator.state(), AnimatorStates{});

    CORRADE_COMPARE(animator.advance(56_nsec, {}, {}, {}), Containers::pair(false, false));
    CORRADE_COMPARE(animator.time(), 56_nsec);
    CORRADE_COMPARE(animator.state(), AnimatorStates{});
}

void AbstractAnimatorTest::advanceInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;
        using AbstractAnimator::create;
        using AbstractAnimator::remove;

        AnimatorFeatures doFeatures() const override { return {}; }
    } animator{animatorHandle(0, 1)};

    /* Create three animations, remove one, capacity should be still 3 */
    animator.create(0_nsec, 15_nsec);
    animator.create(0_nsec, 15_nsec);
    animator.remove(animator.create(0_nsec, 15_nsec));
    CORRADE_COMPARE(animator.capacity(), 3);

    Containers::BitArray mask{NoInit, 3};
    Containers::BitArray maskIncorrect{NoInit, 4};
    Float factors[3];
    Float factorsIncorrect[4];

    /* Same time should be okay */
    animator.advance(46_nsec, mask, factors, mask);
    animator.advance(46_nsec, mask, factors, mask);
    CORRADE_COMPARE(animator.time(), 46_nsec);

    std::ostringstream out;
    Error redirectError{&out};
    animator.advance(45_nsec, mask, factors, mask);
    animator.advance(46_nsec, mask, factors, maskIncorrect);
    animator.advance(46_nsec, mask, factorsIncorrect, mask);
    animator.advance(46_nsec, maskIncorrect, factors, mask);
    CORRADE_COMPARE_AS(out.str(),
        "Whee::AbstractAnimator::advance(): expected a time at least Nanoseconds(46) but got Nanoseconds(45)\n"
        "Whee::AbstractAnimator::advance(): expected active, factors and remove views to have a size of 3 but got 3, 3 and 4\n"
        "Whee::AbstractAnimator::advance(): expected active, factors and remove views to have a size of 3 but got 3, 4 and 3\n"
        "Whee::AbstractAnimator::advance(): expected active, factors and remove views to have a size of 3 but got 4, 3 and 3\n",
        TestSuite::Compare::String);
}

void AbstractAnimatorTest::advanceGeneric() {
    struct: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;
        using AbstractGenericAnimator::create;

        AnimatorFeatures doFeatures() const override { return {}; }
        void doAdvance(Containers::BitArrayView active, const Containers::StridedArrayView1D<const Float>& factors) override {
            CORRADE_COMPARE_AS(active,
                expectedActive,
                TestSuite::Compare::Container);
            for(std::size_t i = 0; i != active.size(); ++i) if(active[i]) {
                CORRADE_ITERATION(i);
                CORRADE_COMPARE(factors[i], expectedFactors[i]);
            }
            ++advanceCallCount;
        }
        void doClean(Containers::BitArrayView animationIdsToRemove) override {
            CORRADE_COMPARE_AS(animationIdsToRemove,
                expectedAnimationIdsToRemove,
                TestSuite::Compare::Container);
            ++cleanCallCount;
        }

        Containers::StridedBitArrayView1D expectedActive;
        Containers::StridedBitArrayView1D expectedAnimationIdsToRemove;
        Containers::StridedArrayView1D<const Float> expectedFactors;
        Int advanceCallCount = 0;
        Int cleanCallCount = 0;
    } animator{animatorHandle(0, 1)};

    /* The mask and factor calculation is thoroughly tested in advance() and
       propertiesStateFactor(), so just create some non-trivial state to verify
       it gets correctly passed through. */

    /* Call to advance(5) advances the first, nothing to clean */
    animator.create(0_nsec, 10_nsec);
    animator.create(-20_nsec, 10_nsec, AnimationFlag::KeepOncePlayed);
    animator.create(6_nsec, 4_nsec);
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
        bool active[]{
            true,
            false,
            false
        };
        Float factors[]{
            0.5f,
            0.0f, /* unused */
            0.0f, /* unused */
        };
        animator.expectedActive = Containers::stridedArrayView(active).sliceBit(0);
        animator.expectedFactors = factors;
        animator.expectedAnimationIdsToRemove = {};
        animator.advance(5_nsec);
    }
    CORRADE_COMPARE(animator.advanceCallCount, 1);
    CORRADE_COMPARE(animator.cleanCallCount, 0);

    /* Call to advance(10) advances the first and last to end, both get
       cleaned afterwards */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
        bool active[]{
            true,
            false,
            true
        };
        Float factors[]{
            1.0f,
            0.0f, /* unused */
            1.0f,
        };
        bool animationIdsToRemove[]{
            true,
            false,
            true
        };
        animator.expectedActive = Containers::stridedArrayView(active).sliceBit(0);
        animator.expectedFactors = factors;
        animator.expectedAnimationIdsToRemove = Containers::stridedArrayView(animationIdsToRemove).sliceBit(0);
        animator.advance(10_nsec);
    }
    CORRADE_COMPARE(animator.advanceCallCount, 2);
    CORRADE_COMPARE(animator.cleanCallCount, 1);

    /* Call to advance(20) does nothing */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
        animator.expectedActive = {};
        animator.expectedFactors = {};
        animator.expectedAnimationIdsToRemove = {};
        animator.advance(20_nsec);
    }
    CORRADE_COMPARE(animator.advanceCallCount, 2);
    CORRADE_COMPARE(animator.cleanCallCount, 1);
}

void AbstractAnimatorTest::state() {
    struct: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;
        using AbstractAnimator::create;
        using AbstractAnimator::remove;

        AnimatorFeatures doFeatures() const override { return {}; }
    } animator{animatorHandle(0, 1)};
    CORRADE_COMPARE(animator.state(), AnimatorStates{});

    Containers::BitArray mask{NoInit, 1};
    Float factors[1];

    /* Animation that's created scheduled sets a state, removal & advance
       clears it */
    {
        AnimationHandle animation = animator.create(10_nsec, 10_nsec);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Scheduled);
        CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);
        animator.remove(animation);
        CORRADE_COMPARE(animator.advance(0_nsec, mask, factors, mask), Containers::pair(false, false));
        CORRADE_COMPARE(animator.state(), AnimatorStates{});

    /* Animation that's created playing sets a state, removal & advance
       clears it */
    } {
        AnimationHandle animation = animator.create(0_nsec, 10_nsec);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Playing);
        CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);
        animator.remove(animation);
        CORRADE_COMPARE(animator.advance(0_nsec, mask, factors, mask), Containers::pair(false, false));
        CORRADE_COMPARE(animator.state(), AnimatorStates{});

    /* Animation that's created stopped and with KeepOncePlayed doesn't set
       anything */
    } {
        AnimationHandle animation = animator.create(-10_nsec, 10_nsec, AnimationFlag::KeepOncePlayed);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Stopped);
        CORRADE_COMPARE(animator.state(), AnimatorStates{});
        animator.remove(animation);

    /* Animation that's created stopped sets a state, advance then marks it for
       removal and clears the state */
    } {
        AnimationHandle animation = animator.create(-10_nsec, 10_nsec);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Stopped);
        CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);
        CORRADE_COMPARE(animator.advance(0_nsec, mask, factors, mask), Containers::pair(false, true));
        CORRADE_COMPARE(mask[0], true);
        animator.remove(animation);
        CORRADE_VERIFY(!animator.isHandleValid(animation));
        CORRADE_COMPARE(animator.state(), AnimatorStates{});

    /* Animation that's scheduled after play() sets a state, removal & advance
       clears it */
    } {
        AnimationHandle animation = animator.create(-10_nsec, 10_nsec, AnimationFlag::KeepOncePlayed);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Stopped);
        CORRADE_COMPARE(animator.state(), AnimatorStates{});

        animator.play(animation, 10_nsec);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Scheduled);
        CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);
        animator.remove(animation);
        CORRADE_COMPARE(animator.advance(0_nsec, mask, factors, mask), Containers::pair(false, false));
        CORRADE_COMPARE(animator.state(), AnimatorStates{});

    /* Animation that's playing after play() sets a state, removal & advance
       clears it */
    } {
        AnimationHandle animation = animator.create(-10_nsec, 10_nsec, AnimationFlag::KeepOncePlayed);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Stopped);
        CORRADE_COMPARE(animator.state(), AnimatorStates{});

        animator.play(animation, 0_nsec);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Playing);
        CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);
        animator.remove(animation);
        CORRADE_COMPARE(animator.advance(0_nsec, mask, factors, mask), Containers::pair(false, false));
        CORRADE_COMPARE(animator.state(), AnimatorStates{});

    /* Animation that's stopped after play() doesn't set anything */
    } {
        AnimationHandle animation = animator.create(-10_nsec, 10_nsec, AnimationFlag::KeepOncePlayed);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Stopped);
        CORRADE_COMPARE(animator.state(), AnimatorStates{});

        animator.play(animation, -20_nsec);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Stopped);
        CORRADE_COMPARE(animator.state(), AnimatorStates{});
        animator.remove(animation);

    /* Animation that stays scheduled after pause() keeps the state, removal &
       advance clears it */
    } {
        AnimationHandle animation = animator.create(10_nsec, 10_nsec);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Scheduled);
        CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);

        animator.pause(animation, 10_nsec);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Scheduled);
        CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);
        animator.remove(animation);
        CORRADE_COMPARE(animator.advance(0_nsec, mask, factors, mask), Containers::pair(false, false));
        CORRADE_COMPARE(animator.state(), AnimatorStates{});

    /* Animation that stays playing after pause() keeps the state, removal &
       advance clears it */
    } {
        AnimationHandle animation = animator.create(0_nsec, 10_nsec);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Playing);
        CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);

        animator.pause(animation, 5_nsec);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Playing);
        CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);
        animator.remove(animation);
        CORRADE_COMPARE(animator.advance(0_nsec, mask, factors, mask), Containers::pair(false, false));
        CORRADE_COMPARE(animator.state(), AnimatorStates{});

    /* Animation that's paused after pause() keeps the state, removal & advance
       clears it */
    } {
        AnimationHandle animation = animator.create(0_nsec, 10_nsec);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Playing);
        CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);

        animator.pause(animation, 0_nsec);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Paused);
        CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);
        animator.remove(animation);
        CORRADE_COMPARE(animator.advance(0_nsec, mask, factors, mask), Containers::pair(false, false));
        CORRADE_COMPARE(animator.state(), AnimatorStates{});

    /* Animation that stays stopped after pause() doesn't set anything */
    } {
        AnimationHandle animation = animator.create(-10_nsec, 10_nsec, AnimationFlag::KeepOncePlayed);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Stopped);
        CORRADE_COMPARE(animator.state(), AnimatorStates{});

        animator.pause(animation, 10_nsec);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Stopped);
        CORRADE_COMPARE(animator.state(), AnimatorStates{});
        animator.remove(animation);

    /* Animation that stays scheduled after stop() keeps the state, removal &
       advance clears it */
    } {
        AnimationHandle animation = animator.create(10_nsec, 10_nsec);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Scheduled);
        CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);

        animator.stop(animation, 20_nsec);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Scheduled);
        CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);
        animator.remove(animation);
        CORRADE_COMPARE(animator.advance(0_nsec, mask, factors, mask), Containers::pair(false, false));
        CORRADE_COMPARE(animator.state(), AnimatorStates{});

    /* Animation that stays playing after stop() keeps the state, removal &
       advance clears it */
    } {
        AnimationHandle animation = animator.create(0_nsec, 10_nsec);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Playing);
        CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);

        animator.stop(animation, 10_nsec);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Playing);
        CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);
        animator.remove(animation);
        CORRADE_COMPARE(animator.advance(0_nsec, mask, factors, mask), Containers::pair(false, false));
        CORRADE_COMPARE(animator.state(), AnimatorStates{});

    /* Animation that stays paused after stop() keeps the state, removal &
       advance clears it */
    } {
        AnimationHandle animation = animator.create(0_nsec, 10_nsec, AnimationFlag::KeepOncePlayed);
        animator.pause(animation, 0_nsec);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Paused);
        CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);

        animator.stop(animation, 10_nsec);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Paused);
        CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);
        animator.remove(animation);
        CORRADE_COMPARE(animator.advance(0_nsec, mask, factors, mask), Containers::pair(false, false));
        CORRADE_COMPARE(animator.state(), AnimatorStates{});

    /* Animation that stays stopped after stop() doesn't set anything */
    } {
        AnimationHandle animation = animator.create(-10_nsec, 10_nsec, AnimationFlag::KeepOncePlayed);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Stopped);
        CORRADE_COMPARE(animator.state(), AnimatorStates{});

        animator.stop(animation, -20_nsec);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Stopped);
        CORRADE_COMPARE(animator.state(), AnimatorStates{});
        animator.remove(animation);
    }
}

}}}}

CORRADE_TEST_MAIN(Magnum::Whee::Test::AbstractAnimatorTest)
