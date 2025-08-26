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
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/Pair.h>
#include <Corrade/Containers/StaticArray.h>
#include <Corrade/Containers/StridedBitArrayView.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/String.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Container.h>
#include <Corrade/TestSuite/Compare/String.h>
#include <Corrade/Utility/Format.h>
#include <Magnum/Math/Constants.h>
#include <Magnum/Math/Time.h>
#include <Magnum/Math/Vector2.h>

#include "Magnum/Ui/AbstractAnimator.h"
#include "Magnum/Ui/AbstractLayer.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/NodeFlags.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct AbstractAnimatorTest: TestSuite::Tester {
    explicit AbstractAnimatorTest();

    void debugFeature();
    void debugFeatures();
    void debugState();
    void debugStates();
    void debugAnimationFlag();
    void debugAnimationFlagPacked();
    void debugAnimationFlags();
    void debugAnimationFlagsPacked();
    void debugAnimationState();
    void debugAnimationStatePacked();
    void debugNodeAnimatorUpdate();
    void debugNodeAnimatorUpdates();
    void debugNodeAnimatorUpdatesSupersets();

    void construct();
    void constructGeneric();
    void constructNode();
    void constructData();
    void constructStyle();
    void constructInvalidHandle();
    void constructCopy();
    void constructCopyGeneric();
    void constructCopyNode();
    void constructCopyData();
    void constructCopyStyle();
    void constructMove();
    void constructMoveGeneric();
    void constructMoveNode();
    void constructMoveData();
    void constructMoveStyle();

    void featuresMutuallyExclusive();

    void genericSetLayer();
    void genericSetLayerInvalid();
    void genericSetLayerInvalidFeatures();

    void createRemove();
    void createRemoveHandleRecycle();
    void createRemoveHandleDisable();
    void createNoHandlesLeft();
    void createInvalid();
    void createNodeAttachment();
    void createNodeAttachmentInvalidFeatures();
    void createDataAttachment();
    void createDataAttachmentNoLayerSet();
    void createDataAttachmentInvalidLayer();
    void createDataAttachmentInvalidFeatures();
    void removeInvalid();
    void properties();
    void propertiesStateFactor();
    void propertiesInvalidHandle();
    void propertiesInvalid();
    void attachNode();
    void attachNodeInvalid();
    void attachNodeInvalidFeatures();
    void attachData();
    void attachDataInvalid();
    void attachDataNoLayerSet();
    void attachDataInvalidLayer();
    void attachDataInvalidFeatures();

    void clean();
    void cleanEmpty();
    void cleanNotImplemented();
    void cleanInvalid();

    void cleanNodes();
    void cleanNodesEmpty();
    void cleanNodesNotImplemented();
    void cleanNodesInvalidFeatures();

    void cleanData();
    void cleanDataEmpty();
    void cleanDataNotImplemented();
    void cleanDataInvalidFeatures();
    void cleanDataNoLayerSet();

    void playResumePauseStop();
    void toggleFlagsAtTime();
    void playPauseStopToggleFlagsInvalid();
    void playPaused();

    void update();
    void updateEmpty();
    void updateInvalid();

    void advanceGeneric();
    void advanceGenericInvalid();
    void advanceNode();
    void advanceNodeInvalid();

    void state();
};

using namespace Math::Literals;

const struct {
    const char* name;
    AnimatorFeatures features;
} CreateRemoveData[]{
    {"", {}},
    {"NodeAttachment", AnimatorFeature::NodeAttachment},
    {"DataAttachment", AnimatorFeature::DataAttachment},
};

const struct {
    TestSuite::TestCaseDescriptionSourceLocation name;
    Nanoseconds duration, start;
    Containers::Optional<Nanoseconds> paused;
    Containers::Optional<Nanoseconds> stopped;
    Containers::Optional<UnsignedInt> repeatCount;
    AnimationFlags flags;
    AnimationState expectedState;
    Float expectedFactor;
} PropertiesStateFactorData[]{
    {"scheduled",
        10_nsec, 100_nsec, {}, {}, {}, {},
        AnimationState::Scheduled, 0.0f},
    {"scheduled, paused later",
        10_nsec, 100_nsec, 108_nsec, {}, {}, {},
        AnimationState::Scheduled, 0.0f},
    {"scheduled, stopped later",
        10_nsec, 100_nsec, {}, 109_nsec, {}, {},
        AnimationState::Scheduled, 0.0f},
    {"scheduled, paused + stopped later",
        10_nsec, 100_nsec, 108_nsec, 109_nsec, {}, {},
        AnimationState::Scheduled, 0.0f},
    {"scheduled, repeat",
        10_nsec, 100_nsec, {}, {}, 10, {},
        AnimationState::Scheduled, 0.0f},
    {"playing begin",
        10_nsec, 0_nsec, {}, {}, {}, {},
        AnimationState::Playing, 0.0f},
    {"playing begin, reverse",
        10_nsec, 0_nsec, {}, {}, {}, AnimationFlag::Reverse,
        AnimationState::Playing, 1.0f},
    {"playing begin, reverse every other",
        10_nsec, 0_nsec, {}, {}, {}, AnimationFlag::ReverseEveryOther,
        AnimationState::Playing, 0.0f},
    {"playing begin, reverse + reverse every other",
        10_nsec, 0_nsec, {}, {}, {}, AnimationFlag::Reverse|AnimationFlag::ReverseEveryOther,
        AnimationState::Playing, 1.0f},
    {"playing begin, paused later",
        10_nsec, 0_nsec, 3_nsec, {}, {}, {},
        AnimationState::Playing, 0.0f},
    {"playing begin, stopped later",
        10_nsec, 0_nsec, {}, 4_nsec, {}, {},
        AnimationState::Playing, 0.0f},
    {"playing begin, paused + stopped later",
        10_nsec, 0_nsec, 3_nsec, 4_nsec, {}, {},
        AnimationState::Playing, 0.0f},
    /* Testing just one variant of reverse for the paused/stopped state, should
       be enough */
    {"playing begin, paused + stopped later, reverse + reverse every other",
        10_nsec, 0_nsec, 3_nsec, 4_nsec, {}, AnimationFlag::Reverse|AnimationFlag::ReverseEveryOther,
        AnimationState::Playing, 1.0f},
    {"playing begin, repeat",
        10_nsec, -90_nsec, {}, {}, 10, {},
        AnimationState::Playing, 0.0f},
    {"playing begin, repeat, reverse",
        10_nsec, -90_nsec, {}, {}, 10, AnimationFlag::Reverse,
        AnimationState::Playing, 1.0f},
    {"playing begin, repeat, even iteration, reverse every other",
        10_nsec, -90_nsec, {}, {}, 10, AnimationFlag::ReverseEveryOther,
        AnimationState::Playing, 1.0f},
    {"playing begin, repeat, even iteration, reverse + reverse every other",
        10_nsec, -90_nsec, {}, {}, 10, AnimationFlag::Reverse|AnimationFlag::ReverseEveryOther,
        AnimationState::Playing, 0.0f},
    {"playing begin, repeat, odd iteration, reverse every other",
        10_nsec, -80_nsec, {}, {}, 10, AnimationFlag::ReverseEveryOther,
        AnimationState::Playing, 0.0f},
    {"playing begin, repeat, odd iteration, reverse + reverse every other",
        10_nsec, -80_nsec, {}, {}, 10, AnimationFlag::Reverse|AnimationFlag::ReverseEveryOther,
        AnimationState::Playing, 1.0f},
    {"playing middle",
        10_nsec, -3_nsec, {}, {}, {}, {},
        AnimationState::Playing, 0.3f},
    {"playing middle, reverse",
        10_nsec, -3_nsec, {}, {}, {}, AnimationFlag::Reverse,
        AnimationState::Playing, 0.7f},
    {"playing middle, reverse every other",
        10_nsec, -3_nsec, {}, {}, {}, AnimationFlag::ReverseEveryOther,
        AnimationState::Playing, 0.3f},
    {"playing middle, reverse + reverse every other",
        10_nsec, -3_nsec, {}, {}, {}, AnimationFlag::Reverse|AnimationFlag::ReverseEveryOther,
        AnimationState::Playing, 0.7f},
    {"playing middle, paused later",
        10_nsec, -3_nsec, 8_nsec, {}, {}, {},
        AnimationState::Playing, 0.3f},
    {"playing middle, stopped later",
        10_nsec, -3_nsec, {}, 9_nsec, {}, {},
        AnimationState::Playing, 0.3f},
    {"playing middle, paused + stopped later",
        10_nsec, -3_nsec, 8_nsec, 9_nsec, {}, {},
        AnimationState::Playing, 0.3f},
    /* Again testing just one variant of reverse for the paused/stopped state,
       should be enough */
    {"playing middle, paused + stopped later, reverse + reverse every other",
        10_nsec, -3_nsec, 8_nsec, 9_nsec, {}, AnimationFlag::Reverse|AnimationFlag::ReverseEveryOther,
        AnimationState::Playing, 0.7f},
    {"playing middle, repeat",
        10_nsec, -97_nsec, {}, {}, 10, {},
        AnimationState::Playing, 0.7f},
    {"playing middle, repeat, reverse",
        10_nsec, -97_nsec, {}, {}, 10, AnimationFlag::Reverse,
        AnimationState::Playing, 0.3f},
    {"playing middle, repeat, even iteration, reverse every other",
        10_nsec, -97_nsec, {}, {}, 10, AnimationFlag::ReverseEveryOther,
        AnimationState::Playing, 0.3f},
    {"playing middle, repeat, even iteration, reverse + reverse every other",
        10_nsec, -97_nsec, {}, {}, 10, AnimationFlag::Reverse|AnimationFlag::ReverseEveryOther,
        AnimationState::Playing, 0.7f},
    {"playing middle, repeat, odd iteration, reverse every other",
        10_nsec, -87_nsec, {}, {}, 10, AnimationFlag::ReverseEveryOther,
        AnimationState::Playing, 0.7f},
    {"playing middle, repeat, odd iteration, reverse + reverse every other",
        10_nsec, -87_nsec, {}, {}, 10, AnimationFlag::Reverse|AnimationFlag::ReverseEveryOther,
        AnimationState::Playing, 0.3f},
    {"playing end",
        10_nsec, -10_nsec, {}, {}, {}, {},
        AnimationState::Stopped, 1.0f},
    {"playing end, reverse",
        10_nsec, -10_nsec, {}, {}, {}, AnimationFlag::Reverse,
        AnimationState::Stopped, 0.0f},
    {"playing end, reverse every other",
        10_nsec, -10_nsec, {}, {}, {}, AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 1.0f},
    {"playing end, reverse + reverse every other",
        10_nsec, -10_nsec, {}, {}, {}, AnimationFlag::Reverse|AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 0.0f},
    {"playing end, repeat",
        10_nsec, -90_nsec, {}, {}, 9, {},
        AnimationState::Stopped, 1.0f},
    {"playing end, repeat, reverse",
        10_nsec, -90_nsec, {}, {}, 9, AnimationFlag::Reverse,
        AnimationState::Stopped, 0.0f},
    {"playing end, repeat odd times, reverse every other",
        10_nsec, -90_nsec, {}, {}, 9, AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 1.0f},
    {"playing end, repeat odd times, reverse + reverse every other",
        10_nsec, -90_nsec, {}, {}, 9, AnimationFlag::Reverse|AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 0.0f},
    {"playing end, repeat even times, reverse every other",
        10_nsec, -100_nsec, {}, {}, 10, AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 0.0f},
    {"playing end, repeat even times, reverse + reverse every other",
        10_nsec, -100_nsec, {}, {}, 10, AnimationFlag::Reverse|AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 1.0f},
    {"paused begin",
        10_nsec, -10_nsec, {-10_nsec}, {}, {}, {},
        AnimationState::Paused, 0.0f},
    {"paused begin, reverse",
        10_nsec, -10_nsec, {-10_nsec}, {}, {}, AnimationFlag::Reverse,
        AnimationState::Paused, 1.0f},
    {"paused begin, reverse every other",
        10_nsec, -10_nsec, {-10_nsec}, {}, {}, AnimationFlag::ReverseEveryOther,
        AnimationState::Paused, 0.0f},
    {"paused begin, reverse + reverse every other",
        10_nsec, -10_nsec, {-10_nsec}, {}, {}, AnimationFlag::Reverse|AnimationFlag::ReverseEveryOther,
        AnimationState::Paused, 1.0f},
    {"paused begin, stopped later",
        10_nsec, -10_nsec, {-10_nsec}, 3_nsec, {}, {},
        AnimationState::Paused, 0.0f},
    /* Again testing just one variant of reverse for the paused + stopped
       state, should be enough */
    {"paused begin, stopped later, reverse",
        10_nsec, -10_nsec, {-10_nsec}, 3_nsec, {}, AnimationFlag::Reverse|AnimationFlag::ReverseEveryOther,
        AnimationState::Paused, 1.0f},
    {"paused begin, repeat",
        10_nsec, -30_nsec, {-10_nsec}, {}, 3, {},
        AnimationState::Paused, 0.0f},
    {"paused begin, repeat, reverse",
        10_nsec, -30_nsec, {-10_nsec}, {}, 3, AnimationFlag::Reverse,
        AnimationState::Paused, 1.0f},
    {"paused begin, repeat, even iteration, reverse every other",
        10_nsec, -30_nsec, {-10_nsec}, {}, 3, AnimationFlag::ReverseEveryOther,
        AnimationState::Paused, 0.0f},
    {"paused begin, repeat, even iteration, reverse + reverse every other",
        10_nsec, -30_nsec, {-10_nsec}, {}, 3, AnimationFlag::Reverse|AnimationFlag::ReverseEveryOther,
        AnimationState::Paused, 1.0f},
    {"paused begin, repeat, odd iteration, reverse every other",
        10_nsec, -30_nsec, {-20_nsec}, {}, 3, AnimationFlag::ReverseEveryOther,
        AnimationState::Paused, 1.0f},
    {"paused begin, repeat, odd iteration, reverse + reverse every other",
        10_nsec, -30_nsec, {-20_nsec}, {}, 3, AnimationFlag::Reverse|AnimationFlag::ReverseEveryOther,
        AnimationState::Paused, 0.0f},
    {"paused middle",
        10_nsec, -10_nsec, {-3_nsec}, {}, {}, {},
        AnimationState::Paused, 0.7f},
    {"paused middle, reverse",
        10_nsec, -10_nsec, {-3_nsec}, {}, {}, AnimationFlag::Reverse,
        AnimationState::Paused, 0.3f},
    {"paused middle, reverse every other",
        10_nsec, -10_nsec, {-3_nsec}, {}, {}, AnimationFlag::ReverseEveryOther,
        AnimationState::Paused, 0.7f},
    {"paused middle, reverse + reverse every other",
        10_nsec, -10_nsec, {-3_nsec}, {}, {}, AnimationFlag::Reverse|AnimationFlag::ReverseEveryOther,
        AnimationState::Paused, 0.3f},
    {"paused middle, repeat",
        10_nsec, -30_nsec, {-7_nsec}, {}, 3, {},
        AnimationState::Paused, 0.3f},
    {"paused middle, repeat, reverse",
        10_nsec, -30_nsec, {-7_nsec}, {}, 3, AnimationFlag::Reverse,
        AnimationState::Paused, 0.7f},
    {"paused middle, repeat, even iteration, reverse every other",
        10_nsec, -30_nsec, {-7_nsec}, {}, 3, AnimationFlag::ReverseEveryOther,
        AnimationState::Paused, 0.3f},
    {"paused middle, repeat, even iteration, reverse + reverse every other",
        10_nsec, -30_nsec, {-7_nsec}, {}, 3, AnimationFlag::Reverse|AnimationFlag::ReverseEveryOther,
        AnimationState::Paused, 0.7f},
    {"paused middle, repeat, odd iteration, reverse every other",
        10_nsec, -30_nsec, {-17_nsec}, {}, 3, AnimationFlag::ReverseEveryOther,
        AnimationState::Paused, 0.7f},
    {"paused middle, repeat, odd iteration, reverse + reverse every other",
        10_nsec, -30_nsec, {-17_nsec}, {}, 3, AnimationFlag::Reverse|AnimationFlag::ReverseEveryOther,
        AnimationState::Paused, 0.3f},
    {"paused end",
        10_nsec, -10_nsec, 0_nsec, {}, {}, {},
        AnimationState::Stopped, 1.0f},
    {"paused end, reverse",
        10_nsec, -10_nsec, 0_nsec, {}, {}, AnimationFlag::Reverse,
        AnimationState::Stopped, 0.0f},
    {"paused end, reverse every other",
        10_nsec, -10_nsec, 0_nsec, {}, {}, AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 1.0f},
    {"paused end, reverse + reverse every other",
        10_nsec, -10_nsec, 0_nsec, {}, {}, AnimationFlag::Reverse|AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 0.0f},
    {"paused end, repeat",
        10_nsec, -80_nsec, 0_nsec, {}, 8, {},
        AnimationState::Stopped, 1.0f},
    {"paused end, repeat, reverse",
        10_nsec, -80_nsec, 0_nsec, {}, 8, AnimationFlag::Reverse,
        AnimationState::Stopped, 0.0f},
    {"paused end, repeat even times, reverse every other",
        10_nsec, -80_nsec, 0_nsec, {}, 8, AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 0.0f},
    {"paused end, repeat even times, reverse + reverse every other",
        10_nsec, -80_nsec, 0_nsec, {}, 8, AnimationFlag::Reverse|AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 1.0f},
    {"paused end, repeat odd times, reverse every other",
        10_nsec, -70_nsec, 0_nsec, {}, 7, AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 1.0f},
    {"paused end, repeat odd times, reverse + reverse every other",
        10_nsec, -70_nsec, 0_nsec, {}, 7, AnimationFlag::Reverse|AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 0.0f},
    /* The animation isn't considered paused yet but scheduled, as it'll be
       advanced (and thus calculated) only once it reaches the actual paused
       state */
    {"paused, scheduled later",
        10_nsec, 100_nsec, 90_nsec, {}, {}, {},
        AnimationState::Scheduled, 0.0f},
    {"stopped",
        10_nsec, -100_nsec, {}, {}, {}, {},
        AnimationState::Stopped, 1.0f},
    {"stopped, reverse",
        10_nsec, -100_nsec, {}, {}, {}, AnimationFlag::Reverse,
        AnimationState::Stopped, 0.0f},
    {"stopped, reverse every other",
        10_nsec, -100_nsec, {}, {}, {}, AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 1.0f},
    {"stopped, reverse + reverse every other",
        10_nsec, -100_nsec, {}, {}, {}, AnimationFlag::Reverse|AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 0.0f},
    /* The time since which the animation stopped shouldn't matter */
    {"stopped, reverse every other, different time since stop",
        10_nsec, -110_nsec, {}, {}, {}, AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 1.0f},
    {"stopped, reverse + reverse every other, different time since stop",
        10_nsec, -110_nsec, {}, {}, {}, AnimationFlag::Reverse|AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 0.0f},
    {"stopped, repeat",
        10_nsec, -100_nsec, {}, {}, 9, {},
        AnimationState::Stopped, 1.0f},
    {"stopped, repeat, reverse",
        10_nsec, -100_nsec, {}, {}, 9, AnimationFlag::Reverse,
        AnimationState::Stopped, 0.0f},
    {"stopped, repeat odd times, reverse every other",
        10_nsec, -100_nsec, {}, {}, 9, AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 1.0f},
    {"stopped, repeat odd times, reverse + reverse every other",
        10_nsec, -100_nsec, {}, {}, 9, AnimationFlag::Reverse|AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 0.0f},
    {"stopped, repeat even times, reverse every other",
        10_nsec, -100_nsec, {}, {}, 8, AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 0.0f},
    {"stopped, repeat even times, reverse + reverse every other",
        10_nsec, -100_nsec, {}, {}, 8, AnimationFlag::Reverse|AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 1.0f},
    /* The time since which the animation stopped shouldn't matter */
    {"stopped, repeat odd times, reverse every other, different time since stop",
        10_nsec, -110_nsec, {}, {}, 9, AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 1.0f},
    {"stopped, repeat odd times, reverse + reverse every other, different time since stop",
        10_nsec, -110_nsec, {}, {}, 9, AnimationFlag::Reverse|AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 0.0f},
    {"stopped, repeat even times, reverse every other, different time since stop",
        10_nsec, -90_nsec, {}, {}, 8, AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 0.0f},
    {"stopped, repeat even times, reverse + reverse every other, different time since stop",
        10_nsec, -90_nsec, {}, {}, 8, AnimationFlag::Reverse|AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 1.0f},
    {"stopped explicitly",
        10_nsec, -100_nsec, {}, {-95_nsec}, {}, {},
        AnimationState::Stopped, 1.0f},
    {"stopped explicitly, reverse",
        10_nsec, -100_nsec, {}, {-95_nsec}, {}, AnimationFlag::Reverse,
        AnimationState::Stopped, 0.0f},
    {"stopped explicitly, reverse every other",
        10_nsec, -100_nsec, {}, {-95_nsec}, {}, AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 1.0f},
    {"stopped explicitly, reverse + reverse every other",
        10_nsec, -100_nsec, {}, {-95_nsec}, {}, AnimationFlag::Reverse|AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 0.0f},
    {"stopped explicitly, repeat",
        10_nsec, -100_nsec, {}, {-55_nsec}, 20, {},
        AnimationState::Stopped, 1.0f},
    {"stopped explicitly, repeat, reverse",
        10_nsec, -100_nsec, {}, {-55_nsec}, 20, AnimationFlag::Reverse,
        AnimationState::Stopped, 0.0f},
    {"stopped explicitly exactly at duration, repeat, even iteration, reverse every other",
        10_nsec, -100_nsec, {}, {-60_nsec}, 20, AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 0.0f},
    {"stopped explicitly right before duration, repeat, even iteration, reverse every other",
        10_nsec, -100_nsec, {}, {-69_nsec}, 20, AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 0.0f},
    {"stopped explicitly right after duration, repeat, even iteration, reverse every other",
        10_nsec, -100_nsec, {}, {-61_nsec}, 20, AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 0.0f},
    {"stopped explicitly, repeat, even iteration, reverse + reverse every other",
        10_nsec, -100_nsec, {}, {-65_nsec}, 20, AnimationFlag::Reverse|AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 1.0f},
    {"stopped explicitly exactly at duration, repeat, odd iteration, reverse every other",
        10_nsec, -100_nsec, {}, {-50_nsec}, 20, AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 1.0f},
    {"stopped explicitly right before duration, repeat, odd iteration, reverse every other",
        10_nsec, -100_nsec, {}, {-59_nsec}, 20, AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 1.0f},
    {"stopped explicitly right after duration, repeat, odd iteration, reverse every other",
        10_nsec, -100_nsec, {}, {-51_nsec}, 20, AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 1.0f},
    {"stopped explicitly, repeat, odd iteration, reverse + reverse every other",
        10_nsec, -100_nsec, {}, {-55_nsec}, 20, AnimationFlag::Reverse|AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 0.0f},
    /* Repeating infinite times should make no difference on reversing every
       other */
    {"stopped explicitly, repeat indefinitely, even iteration, reverse every other",
        10_nsec, -100_nsec, {}, {-65_nsec}, 0, AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 0.0f},
    {"stopped explicitly, repeat indefinitely, even iteration, reverse + reverse every other",
        10_nsec, -100_nsec, {}, {-65_nsec}, 0, AnimationFlag::Reverse|AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 1.0f},
    {"stopped explicitly, repeat indefinitely, odd iteration, reverse every other",
        10_nsec, -100_nsec, {}, {-55_nsec}, 0, AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 1.0f},
    {"stopped explicitly, repeat indefinitely, odd iteration, reverse + reverse every other",
        10_nsec, -100_nsec, {}, {-55_nsec}, 0, AnimationFlag::Reverse|AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 0.0f},
    /* The time since which the animation stopped shouldn't matter */
    {"stopped explicitly, repeat, even iteration, reverse every other, different time since stop",
        10_nsec, -110_nsec, {}, {-75_nsec}, 20, AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 0.0f},
    {"stopped explicitly, repeat, even iteration, reverse + reverse every other, different time since stop",
        10_nsec, -110_nsec, {}, {-75_nsec}, 20, AnimationFlag::Reverse|AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 1.0f},
    {"stopped explicitly, repeat, odd iteration, reverse every other, different time since stop",
        10_nsec, -110_nsec, {}, {-65_nsec}, 20, AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 1.0f},
    {"stopped explicitly, repeat, odd iteration, reverse + reverse every other, different time since stop",
        10_nsec, -110_nsec, {}, {-65_nsec}, 20, AnimationFlag::Reverse|AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 0.0f},
    /* This verifies just that the stop is taken into account even if it's the
       same as current time */
    {"stopped explicitly just now",
        10_nsec, -5_nsec, {}, 0_nsec, {}, {},
        AnimationState::Stopped, 1.0f},
    {"stopped explicitly just now, reverse",
        10_nsec, -5_nsec, {}, 0_nsec, {}, AnimationFlag::Reverse,
        AnimationState::Stopped, 0.0f},
    {"stopped explicitly just now, reverse every other",
        10_nsec, -5_nsec, {}, 0_nsec, {}, AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 1.0f},
    {"stopped explicitly just now, reverse + reverse every other",
        10_nsec, -5_nsec, {}, 0_nsec, {}, AnimationFlag::Reverse|AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 0.0f},
    {"stopped explicitly before the start",
        10_nsec, -100_nsec, {}, {-110_nsec}, {}, {},
        AnimationState::Stopped, 1.0f},
    {"stopped explicitly before the start, reverse",
        10_nsec, -100_nsec, {}, {-110_nsec}, {}, AnimationFlag::Reverse,
        AnimationState::Stopped, 0.0f},
    /** @todo these two are treated as being stopped in the iteration before
        the first one, and so if the first iteration is not reversed, this one
        is and vice versa; same in the zero duration case below */
    {"stopped explicitly before the start, reverse every other",
        10_nsec, -100_nsec, {}, {-110_nsec}, {}, AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 0.0f},
    {"stopped explicitly before the start, reverse + reverse every other",
        10_nsec, -100_nsec, {}, {-110_nsec}, {}, AnimationFlag::Reverse|AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 1.0f},
    /* These should take into account the actual duration end (so -90 / -50 ns,
       not the explicit stop, to calculate even/odd reverse */
    {"stopped explicitly but after the whole duration",
        10_nsec, -100_nsec, {}, {-85_nsec}, {}, {},
        AnimationState::Stopped, 1.0f},
    {"stopped explicitly but after the whole duration, reverse",
        10_nsec, -100_nsec, {}, {-85_nsec}, {}, AnimationFlag::Reverse,
        AnimationState::Stopped, 0.0f},
    {"stopped explicitly but after the whole duration, reverse every other",
        10_nsec, -100_nsec, {}, {-85_nsec}, {}, AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 1.0f},
    {"stopped explicitly but after the whole duration, repeat",
        10_nsec, -100_nsec, {}, {-45_nsec}, 5, {},
        AnimationState::Stopped, 1.0f},
    {"stopped explicitly but after the whole duration, repeat, reverse",
        10_nsec, -100_nsec, {}, {-45_nsec}, 5, AnimationFlag::Reverse,
        AnimationState::Stopped, 0.0f},
    {"stopped explicitly but after the whole duration, repeat odd times, reverse every other",
        10_nsec, -100_nsec, {}, {-45_nsec}, 5, AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 1.0f},
    {"stopped explicitly but after the whole duration, repeat odd times, reverse + reverse every other",
        10_nsec, -100_nsec, {}, {-45_nsec}, 5, AnimationFlag::Reverse|AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 0.0f},
    {"stopped explicitly but after the whole duration, repeat even times, reverse every other",
        10_nsec, -100_nsec, {}, {-55_nsec}, 4, AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 0.0f},
    {"stopped explicitly but after the whole duration, repeat even times, reverse + reverse every other",
        10_nsec, -100_nsec, {}, {-55_nsec}, 4, AnimationFlag::Reverse|AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 1.0f},
    /* As this doesn't ever result in the animation running, it's Stopped
       already to not require a NeedsAdvance */
    {"stopped, scheduled later",
        10_nsec, 100_nsec, {}, 90_nsec, {}, {},
        AnimationState::Stopped, 1.0f},
    {"playing begin, one day duration",
        24ll*60ll*60ll*1.0_sec,
        0.0_sec, {}, {}, {}, {},
        AnimationState::Playing, 0.0f},
    {"playing middle, one day duration",
        24ll*60ll*60ll*1.0_sec,
        -16ll*60ll*60ll*1.0_sec, {}, {}, {}, {},
        AnimationState::Playing, 0.66667f},
    {"playing end, one day duration",
        24ll*60ll*60ll*1.0_sec,
        -24ll*60ll*60ll*1.0_sec, {}, {}, {}, {},
        AnimationState::Stopped, 1.0f},
    {"playing begin, one year duration",
        365ll*24ll*60ll*60ll*1.0_sec,
        0.0_sec, {}, {}, {}, {},
        AnimationState::Playing, 0.0f},
    {"playing middle, one year duration",
        365ll*24ll*60ll*60ll*1.0_sec,
        -365ll*16ll*60ll*60ll*1.0_sec, {}, {}, {}, {},
        AnimationState::Playing, 0.66667f},
    {"playing end, one year duration",
        365ll*24ll*60ll*60ll*1.0_sec,
        -365ll*24ll*60ll*60ll*1.0_sec, {}, {}, {}, {},
        AnimationState::Stopped, 1.0f},
    /* The duration is scaled by 29 in the test case, which makes this 290
       years, which is near to the maximum representable (signed) range of 292
       years */
    {"playing begin, 10 year duration",
        10ll*365ll*24ll*60ll*60ll*1.0_sec,
        0.0_sec, {}, {}, {}, {},
        AnimationState::Playing, 0.0f},
    {"playing middle, 10 year duration",
        10ll*365ll*24ll*60ll*60ll*1.0_sec,
        -10ll*365ll*16ll*60ll*60ll*1.0_sec, {}, {}, {}, {},
        AnimationState::Playing, 0.66667f},
    {"playing end, 10 year duration",
        10ll*365ll*24ll*60ll*60ll*1.0_sec,
        -10ll*365ll*24ll*60ll*60ll*1.0_sec, {}, {}, {}, {},
        AnimationState::Stopped, 1.0f},
    {"playing begin, 1 second duration, 100 millionth repeat",
        1.0_sec,
        -100ll*1000ll*1000ll*1.0_sec, {}, {}, 0, {},
        AnimationState::Playing, 0.0f},
    {"playing middle, 1 second duration, 100 millionth repeat",
        1.0_sec,
        -100ll*1000ll*1000ll*1.0_sec + 0.376_sec, {}, {}, 0, {},
        AnimationState::Playing, 1.0f - 0.376f},
    {"playing end, 1 second duration, 100 millionth repeat",
        1.0_sec,
        -100ll*1000ll*1000ll*1.0_sec, {}, {}, 100*1000*1000, {},
        AnimationState::Stopped, 1.0f},
    /* Verify that evern repeat counts that go over 32 bits work correctly. Can
       only test begin & middle, not Stop, as there's no way to represent that
       many fixed repeats. */
    {"playing begin, 1 microsecond duration, 100 billionth repeat",
        1.0_usec,
        -100ll*1000ll*1000ll*1000ll*1.0_usec, {}, {}, 0, {},
        AnimationState::Playing, 0.0f},
    {"playing middle, 1 microsecond duration, 100 billionth repeat",
        1.0_usec,
        -100ll*1000ll*1000ll*1000ll*1.0_usec + 0.376_usec, {}, {}, 0, {},
        AnimationState::Playing, 1.0f - 0.376f},
    /* This verifies that there's no underflow or whatever happening when
       calculating even and odd repeats */
    {"playing begin, 1 microsecond duration, 100 billionth repeat, reverse every other",
        1.0_usec,
        -100ll*1000ll*1000ll*1000ll*1.0_usec, {}, {}, 0, AnimationFlag::ReverseEveryOther,
        AnimationState::Playing, 0.0f},
    {"playing begin, 1 microsecond duration, 100 billionth repeat - 1, reverse every other",
        1.0_usec,
        -100ll*1000ll*1000ll*1000ll*1.0_usec - 1.0_usec, {}, {}, 0, AnimationFlag::ReverseEveryOther,
        AnimationState::Playing, 1.0f},
    {"playing middle, 1 microsecond duration, 100 billionth repeat, reverse every other",
        1.0_usec,
        -100ll*1000ll*1000ll*1000ll*1.0_usec + 0.376_usec, {}, {}, 0, AnimationFlag::ReverseEveryOther,
        AnimationState::Playing, 0.376f},
    {"playing middle, 1 microsecond duration, 100 billionth repeat - 1, reverse every other",
        1.0_usec,
        -100ll*1000ll*1000ll*1000ll*1.0_usec + 0.376_usec - 1.0_usec, {}, {}, 0, AnimationFlag::ReverseEveryOther,
        AnimationState::Playing, 1.0f - 0.376f},
    {"playing end, 1 microsecond duration, 100 billionth repeat, reverse every other",
        1.0_usec,
        -100ll*1000ll*1000ll*1000ll*1.0_usec, {}, {}, 100*1000*1000, AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 0.0f},
    {"playing end, 1 microsecond duration, 100 billionth repeat - 1, reverse every other",
        1.0_usec,
        -100ll*1000ll*1000ll*1000ll*1.0_usec - 1.0_usec, {}, {}, 100*1000*1000 - 1, AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 1.0f},
    {"zero duration, scheduled",
        0_nsec, 100_nsec, {}, {}, {}, {},
        AnimationState::Scheduled, 0.0f},
    {"zero duration, scheduled, paused later",
        0_nsec, 100_nsec, 108_nsec, {}, {}, {},
        AnimationState::Scheduled, 0.0f},
    {"zero duration, scheduled, stopped later",
        0_nsec, 100_nsec, {}, 109_nsec, {}, {},
        AnimationState::Scheduled, 0.0f},
    {"zero duration, scheduled, paused + stopped later",
        0_nsec, 100_nsec, 108_nsec, 109_nsec, {}, {},
        AnimationState::Scheduled, 0.0f},
    {"zero duration, stopped",
        0_nsec, -100_nsec, {}, {}, {}, {},
        AnimationState::Stopped, 1.0f},
    {"zero duration, stopped, reverse",
        0_nsec, -100_nsec, {}, {}, {}, AnimationFlag::Reverse,
        AnimationState::Stopped, 0.0f},
    {"zero duration, stopped, reverse every other",
        0_nsec, -100_nsec, {}, {}, {}, AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 1.0f},
    {"zero duration, stopped, reverse + reverse every other",
        0_nsec, -100_nsec, {}, {}, {}, AnimationFlag::Reverse|AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 0.0f},
    {"zero duration, stopped explicitly before the start",
        0_nsec, -100_nsec, {}, {-110_nsec}, {}, {},
        AnimationState::Stopped, 1.0f},
    {"zero duration, stopped explicitly before the start, reverse",
        0_nsec, -100_nsec, {}, {-110_nsec}, {}, AnimationFlag::Reverse,
        AnimationState::Stopped, 0.0f},
    /** @todo these two are treated as being stopped in the iteration before
        the first one, and so if the first iteration is not reversed, this one
        is and vice versa; same in the non-zero duration case above */
    {"zero duration, stopped explicitly before the start, reverse every other",
        0_nsec, -100_nsec, {}, {-110_nsec}, {}, AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 0.0f},
    {"zero duration, stopped explicitly before the start, reverse + reverse every other",
        0_nsec, -100_nsec, {}, {-110_nsec}, {}, AnimationFlag::Reverse|AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 1.0f},
    {"zero duration, stopped explicitly but after the whole duration",
        0_nsec, -100_nsec, {}, {-50_nsec}, {}, {},
        AnimationState::Stopped, 1.0f},
    {"zero duration, stopped explicitly but after the whole duration, reverse",
        0_nsec, -100_nsec, {}, {-50_nsec}, {}, AnimationFlag::Reverse,
        AnimationState::Stopped, 0.0f},
    {"zero duration, stopped explicitly but after the whole duration, reverse every other",
        0_nsec, -100_nsec, {}, {-50_nsec}, {}, AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 1.0f},
    {"zero duration, stopped explicitly but after the whole duration, reverse + reverse every other",
        0_nsec, -100_nsec, {}, {-50_nsec}, {}, AnimationFlag::Reverse|AnimationFlag::ReverseEveryOther,
        AnimationState::Stopped, 0.0f},
};

const struct {
    const char* name;
    AnimatorFeatures features;
} CleanData[]{
    {"", {}},
    {"node attachment", AnimatorFeature::NodeAttachment},
    {"data attachment", AnimatorFeature::DataAttachment},
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
    /* The animation was paused before it started, resuming it should be from
       the start */
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
              &AbstractAnimatorTest::debugAnimationFlagPacked,
              &AbstractAnimatorTest::debugAnimationFlags,
              &AbstractAnimatorTest::debugAnimationFlagsPacked,
              &AbstractAnimatorTest::debugAnimationState,
              &AbstractAnimatorTest::debugAnimationStatePacked,
              &AbstractAnimatorTest::debugNodeAnimatorUpdate,
              &AbstractAnimatorTest::debugNodeAnimatorUpdates,
              &AbstractAnimatorTest::debugNodeAnimatorUpdatesSupersets,

              &AbstractAnimatorTest::construct,
              &AbstractAnimatorTest::constructGeneric,
              &AbstractAnimatorTest::constructNode,
              &AbstractAnimatorTest::constructData,
              &AbstractAnimatorTest::constructStyle,
              &AbstractAnimatorTest::constructInvalidHandle,
              &AbstractAnimatorTest::constructCopy,
              &AbstractAnimatorTest::constructCopyGeneric,
              &AbstractAnimatorTest::constructCopyNode,
              &AbstractAnimatorTest::constructCopyData,
              &AbstractAnimatorTest::constructCopyStyle,
              &AbstractAnimatorTest::constructMove,
              &AbstractAnimatorTest::constructMoveGeneric,
              &AbstractAnimatorTest::constructMoveNode,
              &AbstractAnimatorTest::constructMoveData,
              &AbstractAnimatorTest::constructMoveStyle,

              &AbstractAnimatorTest::featuresMutuallyExclusive,

              &AbstractAnimatorTest::genericSetLayer,
              &AbstractAnimatorTest::genericSetLayerInvalid,
              &AbstractAnimatorTest::genericSetLayerInvalidFeatures});

    addInstancedTests({&AbstractAnimatorTest::createRemove,
                       &AbstractAnimatorTest::createRemoveHandleRecycle},
        Containers::arraySize(CreateRemoveData));

    addTests({&AbstractAnimatorTest::createRemoveHandleDisable,
              &AbstractAnimatorTest::createNoHandlesLeft,
              &AbstractAnimatorTest::createInvalid,
              &AbstractAnimatorTest::createNodeAttachment,
              &AbstractAnimatorTest::createNodeAttachmentInvalidFeatures,
              &AbstractAnimatorTest::createDataAttachment,
              &AbstractAnimatorTest::createDataAttachmentNoLayerSet,
              &AbstractAnimatorTest::createDataAttachmentInvalidLayer,
              &AbstractAnimatorTest::createDataAttachmentInvalidFeatures,
              &AbstractAnimatorTest::removeInvalid,
              &AbstractAnimatorTest::properties});

    addInstancedTests({&AbstractAnimatorTest::propertiesStateFactor},
        Containers::arraySize(PropertiesStateFactorData));

    addTests({&AbstractAnimatorTest::propertiesInvalidHandle,
              &AbstractAnimatorTest::propertiesInvalid,
              &AbstractAnimatorTest::attachNode,
              &AbstractAnimatorTest::attachNodeInvalid,
              &AbstractAnimatorTest::attachNodeInvalidFeatures,
              &AbstractAnimatorTest::attachData,
              &AbstractAnimatorTest::attachDataInvalid,
              &AbstractAnimatorTest::attachDataNoLayerSet,
              &AbstractAnimatorTest::attachDataInvalidLayer,
              &AbstractAnimatorTest::attachDataInvalidFeatures});

    addInstancedTests({&AbstractAnimatorTest::clean},
        Containers::arraySize(CleanData));

    addTests({&AbstractAnimatorTest::cleanEmpty,
              &AbstractAnimatorTest::cleanNotImplemented,
              &AbstractAnimatorTest::cleanInvalid,

              &AbstractAnimatorTest::cleanNodes,
              &AbstractAnimatorTest::cleanNodesEmpty,
              &AbstractAnimatorTest::cleanNodesNotImplemented,
              &AbstractAnimatorTest::cleanNodesInvalidFeatures,

              &AbstractAnimatorTest::cleanData,
              &AbstractAnimatorTest::cleanDataEmpty,
              &AbstractAnimatorTest::cleanDataNotImplemented,
              &AbstractAnimatorTest::cleanDataInvalidFeatures,
              &AbstractAnimatorTest::cleanDataNoLayerSet,

              &AbstractAnimatorTest::playResumePauseStop,
              &AbstractAnimatorTest::toggleFlagsAtTime,
              &AbstractAnimatorTest::playPauseStopToggleFlagsInvalid});

    addInstancedTests({&AbstractAnimatorTest::playPaused},
        Containers::arraySize(PlayPausedData));

    addTests({&AbstractAnimatorTest::update,
              &AbstractAnimatorTest::updateEmpty,
              &AbstractAnimatorTest::updateInvalid,

              &AbstractAnimatorTest::advanceGeneric,
              &AbstractAnimatorTest::advanceGenericInvalid,
              &AbstractAnimatorTest::advanceNode,
              &AbstractAnimatorTest::advanceNodeInvalid,

              &AbstractAnimatorTest::state});
}

void AbstractAnimatorTest::debugFeature() {
    Containers::String out;
    Debug{&out} << AnimatorFeature::NodeAttachment << AnimatorFeature(0xbe);
    CORRADE_COMPARE(out, "Ui::AnimatorFeature::NodeAttachment Ui::AnimatorFeature(0xbe)\n");
}

void AbstractAnimatorTest::debugFeatures() {
    Containers::String out;
    Debug{&out} << (AnimatorFeature::NodeAttachment|AnimatorFeature(0xe0)) << AnimatorFeatures{};
    CORRADE_COMPARE(out, "Ui::AnimatorFeature::NodeAttachment|Ui::AnimatorFeature(0xe0) Ui::AnimatorFeatures{}\n");
}

void AbstractAnimatorTest::debugState() {
    Containers::String out;
    Debug{&out} << AnimatorState::NeedsAdvance << AnimatorState(0xbe);
    CORRADE_COMPARE(out, "Ui::AnimatorState::NeedsAdvance Ui::AnimatorState(0xbe)\n");
}

void AbstractAnimatorTest::debugStates() {
    Containers::String out;
    Debug{&out} << (AnimatorState::NeedsAdvance|AnimatorState(0xe0)) << AnimatorStates{};
    CORRADE_COMPARE(out, "Ui::AnimatorState::NeedsAdvance|Ui::AnimatorState(0xe0) Ui::AnimatorStates{}\n");
}

void AbstractAnimatorTest::debugAnimationFlag() {
    Containers::String out;
    Debug{&out} << AnimationFlag::KeepOncePlayed << AnimationFlag(0xbe);
    CORRADE_COMPARE(out, "Ui::AnimationFlag::KeepOncePlayed Ui::AnimationFlag(0xbe)\n");
}

void AbstractAnimatorTest::debugAnimationFlagPacked() {
    Containers::String out;
    /* Last is not packed, ones before should not make any flags persistent */
    Debug{&out} << Debug::packed << AnimationFlag::KeepOncePlayed << Debug::packed << AnimationFlag(0xbe) << AnimationFlag::Reverse;
    CORRADE_COMPARE(out, "KeepOncePlayed 0xbe Ui::AnimationFlag::Reverse\n");
}

void AbstractAnimatorTest::debugAnimationFlags() {
    Containers::String out;
    Debug{&out} << (AnimationFlag::KeepOncePlayed|AnimationFlag::Reverse|AnimationFlag(0xe0)) << AnimationFlags{};
    CORRADE_COMPARE(out, "Ui::AnimationFlag::KeepOncePlayed|Ui::AnimationFlag::Reverse|Ui::AnimationFlag(0xe0) Ui::AnimationFlags{}\n");
}

void AbstractAnimatorTest::debugAnimationFlagsPacked() {
    Containers::String out;
    /* Last is not packed, ones before should not make any flags persistent */
    Debug{&out} << Debug::packed << (AnimationFlag::KeepOncePlayed|AnimationFlag::Reverse|AnimationFlag(0xe0)) << Debug::packed << AnimationFlags{} << (AnimationFlag::Reverse|AnimationFlag::ReverseEveryOther);
    CORRADE_COMPARE(out, "KeepOncePlayed|Reverse|0xe0 {} Ui::AnimationFlag::Reverse|Ui::AnimationFlag::ReverseEveryOther\n");
}

void AbstractAnimatorTest::debugAnimationState() {
    Containers::String out;
    Debug{&out} << AnimationState::Paused << AnimationState(0xbe);
    CORRADE_COMPARE(out, "Ui::AnimationState::Paused Ui::AnimationState(0xbe)\n");
}

void AbstractAnimatorTest::debugAnimationStatePacked() {
    Containers::String out;
    /* Last is not packed, ones before should not make any flags persistent */
    Debug{&out} << Debug::packed << AnimationState::Paused << Debug::packed << AnimationState(0xbe) << AnimationState::Stopped;
    CORRADE_COMPARE(out, "Paused 0xbe Ui::AnimationState::Stopped\n");
}

void AbstractAnimatorTest::debugNodeAnimatorUpdate() {
    Containers::String out;
    Debug{&out} << NodeAnimatorUpdate::Enabled << NodeAnimatorUpdate(0xbe);
    CORRADE_COMPARE(out, "Ui::NodeAnimatorUpdate::Enabled Ui::NodeAnimatorUpdate(0xbe)\n");
}

void AbstractAnimatorTest::debugNodeAnimatorUpdates() {
    Containers::String out;
    Debug{&out} << (NodeAnimatorUpdate::OffsetSize|NodeAnimatorUpdate(0x80)) << NodeAnimatorUpdates{};
    CORRADE_COMPARE(out, "Ui::NodeAnimatorUpdate::OffsetSize|Ui::NodeAnimatorUpdate(0x80) Ui::NodeAnimatorUpdates{}\n");
}

void AbstractAnimatorTest::debugNodeAnimatorUpdatesSupersets() {
    /* Enabled is a superset of EventMask, so only one should be printed */
    {
        Containers::String out;
        Debug{&out} << (NodeAnimatorUpdate::Enabled|NodeAnimatorUpdate::EventMask);
        CORRADE_COMPARE(out, "Ui::NodeAnimatorUpdate::Enabled\n");
    }
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
        void doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
    } animator{animatorHandle(0xab, 0x12)};

    CORRADE_COMPARE(animator.features(), AnimatorFeatures{0xbc});
    CORRADE_COMPARE(animator.handle(), animatorHandle(0xab, 0x12));
    /* The rest is the same as in construct() */
}

void AbstractAnimatorTest::constructNode() {
    struct: AbstractNodeAnimator {
        using AbstractNodeAnimator::AbstractNodeAnimator;

        NodeAnimatorUpdates doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Float>&, const Containers::StridedArrayView1D<NodeFlags>&, Containers::MutableBitArrayView) override {
            return {};
        }
    } animator{animatorHandle(0xab, 0x12)};

    CORRADE_COMPARE(animator.features(), AnimatorFeature::NodeAttachment);
    CORRADE_COMPARE(animator.handle(), animatorHandle(0xab, 0x12));
    /* The rest is the same as in construct() */
}

void AbstractAnimatorTest::constructData() {
     AbstractDataAnimator animator{animatorHandle(0xab, 0x12)};

    CORRADE_COMPARE(animator.features(), AnimatorFeature::DataAttachment);
    CORRADE_COMPARE(animator.handle(), animatorHandle(0xab, 0x12));
    /* The rest is the same as in construct() */
}

void AbstractAnimatorTest::constructStyle() {
    AbstractStyleAnimator animator{animatorHandle(0xab, 0x12)};

    CORRADE_COMPARE(animator.features(), AnimatorFeature::DataAttachment);
    CORRADE_COMPARE(animator.handle(), animatorHandle(0xab, 0x12));
    /* The rest is the same as in construct() */
}

void AbstractAnimatorTest::constructInvalidHandle() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Animator: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;

        AnimatorFeatures doFeatures() const override { return {}; }
    };

    Containers::String out;
    Error redirectError{&out};
    Animator{AnimatorHandle::Null};
    CORRADE_COMPARE(out,
        "Ui::AbstractAnimator: handle is null\n");
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
        void doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
    };

    CORRADE_VERIFY(!std::is_copy_constructible<Animator>{});
    CORRADE_VERIFY(!std::is_copy_assignable<Animator>{});
}

void AbstractAnimatorTest::constructCopyNode() {
    struct Animator: AbstractNodeAnimator {
        using AbstractNodeAnimator::AbstractNodeAnimator;

        AnimatorFeatures doFeatures() const override { return {}; }
        NodeAnimatorUpdates doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Float>&, const Containers::StridedArrayView1D<NodeFlags>&, Containers::MutableBitArrayView) override {
            return {};
        }
    };

    CORRADE_VERIFY(!std::is_copy_constructible<Animator>{});
    CORRADE_VERIFY(!std::is_copy_assignable<Animator>{});
}

void AbstractAnimatorTest::constructCopyData() {
    struct Animator: AbstractDataAnimator {
        using AbstractDataAnimator::AbstractDataAnimator;

        AnimatorFeatures doFeatures() const override { return {}; }
    };

    CORRADE_VERIFY(!std::is_copy_constructible<Animator>{});
    CORRADE_VERIFY(!std::is_copy_assignable<Animator>{});
}

void AbstractAnimatorTest::constructCopyStyle() {
    struct Animator: AbstractStyleAnimator {
        using AbstractStyleAnimator::AbstractStyleAnimator;

        AnimatorFeatures doFeatures() const override { return {}; }
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
        void doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
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

void AbstractAnimatorTest::constructMoveNode() {
    struct Animator: AbstractNodeAnimator {
        using AbstractNodeAnimator::AbstractNodeAnimator;

        AnimatorFeatures doFeatures() const override { return {}; }
        NodeAnimatorUpdates doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Float>&, const Containers::StridedArrayView1D<NodeFlags>&, Containers::MutableBitArrayView) override {
            return {};
        }
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

void AbstractAnimatorTest::constructMoveData() {
    struct Animator: AbstractDataAnimator {
        using AbstractDataAnimator::AbstractDataAnimator;

        AnimatorFeatures doFeatures() const override { return {}; }
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

void AbstractAnimatorTest::constructMoveStyle() {
    struct Animator: AbstractStyleAnimator {
        using AbstractStyleAnimator::AbstractStyleAnimator;

        AnimatorFeatures doFeatures() const override { return {}; }
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

void AbstractAnimatorTest::featuresMutuallyExclusive() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::NodeAttachment|AnimatorFeature::DataAttachment;
        }
    } animator{animatorHandle(0, 1)};

    Containers::String out;
    Error redirectError{&out};
    animator.features();
    CORRADE_COMPARE(out, "Ui::AbstractAnimator::features(): Ui::AnimatorFeature::NodeAttachment and Ui::AnimatorFeature::DataAttachment are mutually exclusive\n");
}

void AbstractAnimatorTest::genericSetLayer() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0xab, 0x12)};

    struct: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;
        using AbstractGenericAnimator::setLayer;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::DataAttachment;
        }
        void doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
    } animator{animatorHandle(0, 1)};
    CORRADE_COMPARE(animator.layer(), LayerHandle::Null);

    animator.setLayer(layer);
    CORRADE_COMPARE(animator.layer(), layer.handle());
}

void AbstractAnimatorTest::genericSetLayerInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;
        using AbstractGenericAnimator::setLayer;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::DataAttachment;
        }
        void doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
    } animator{animatorHandle(0, 1)};

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0xab, 0x12)};

    /* First time it passes */
    animator.setLayer(layer);
    CORRADE_COMPARE(animator.layer(), layer.handle());

    /* Second time it asserts, even if the layer is the same */
    Containers::String out;
    Error redirectError{&out};
    animator.setLayer(layer);
    CORRADE_COMPARE(out, "Ui::AbstractGenericAnimator::setLayer(): layer already set to Ui::LayerHandle(0xab, 0x12)\n");
}

void AbstractAnimatorTest::genericSetLayerInvalidFeatures() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;
        using AbstractGenericAnimator::setLayer;

        AnimatorFeatures doFeatures() const override {
            /* Not DataAttachment */
            return AnimatorFeature::NodeAttachment;
        }
        void doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
    } animator{animatorHandle(0, 1)};

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0, 1)};

    Containers::String out;
    Error redirectError{&out};
    animator.setLayer(layer);
    CORRADE_COMPARE(out, "Ui::AbstractGenericAnimator::setLayer(): feature not supported\n");
}

void AbstractAnimatorTest::createRemove() {
    auto&& data = CreateRemoveData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct Animator: AbstractAnimator {
        explicit Animator(AnimatorHandle handle, AnimatorFeatures features): AbstractAnimator{handle}, _features{features} {}

        using AbstractAnimator::create;
        using AbstractAnimator::remove;

        AnimatorFeatures doFeatures() const override { return _features; }

        private:
            AnimatorFeatures _features;
    } animator{animatorHandle(0xab, 0x12), data.features};

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
    CORRADE_COMPARE(animator.started(first), 1337_nsec);
    CORRADE_COMPARE(animator.paused(first), Nanoseconds::max());
    CORRADE_COMPARE(animator.stopped(first), Nanoseconds::max());
    if(data.features & AnimatorFeature::NodeAttachment)
        CORRADE_COMPARE(animator.node(first), NodeHandle::Null);
    if(data.features & AnimatorFeature::DataAttachment)
        CORRADE_COMPARE(animator.data(first), DataHandle::Null);
    /* Animation state() is tested thoroughly in animationState() */
    CORRADE_COMPARE(animator.state(first), AnimationState::Scheduled);

    /* Specifying repeat count and flags, using the AnimatorDataHandle
       overload */
    AnimationHandle second = animator.create(-26_nsec, 3_nsec, 666, AnimationFlag::Reverse|AnimationFlag::ReverseEveryOther);
    CORRADE_COMPARE(second, animationHandle(animator.handle(), 1, 1));
    CORRADE_VERIFY(animator.isHandleValid(second));
    /* Animator state() is tested thoroughly in state() */
    CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);
    CORRADE_COMPARE(animator.capacity(), 2);
    CORRADE_COMPARE(animator.usedCount(), 2);
    CORRADE_COMPARE(animator.duration(animationHandleData(second)), 3_nsec);
    CORRADE_COMPARE(animator.repeatCount(animationHandleData(second)), 666);
    CORRADE_COMPARE(animator.flags(animationHandleData(second)), AnimationFlag::Reverse|AnimationFlag::ReverseEveryOther);
    CORRADE_COMPARE(animator.started(animationHandleData(second)), -26_nsec);
    CORRADE_COMPARE(animator.paused(animationHandleData(second)), Nanoseconds::max());
    CORRADE_COMPARE(animator.stopped(animationHandleData(second)), Nanoseconds::max());
    if(data.features & AnimatorFeature::NodeAttachment)
        CORRADE_COMPARE(animator.node(second), NodeHandle::Null);
    if(data.features & AnimatorFeature::DataAttachment)
        CORRADE_COMPARE(animator.data(second), DataHandle::Null);
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
    CORRADE_COMPARE(animator.started(third), 111_nsec);
    CORRADE_COMPARE(animator.paused(third), Nanoseconds::max());
    CORRADE_COMPARE(animator.stopped(third), Nanoseconds::max());
    if(data.features & AnimatorFeature::NodeAttachment)
        CORRADE_COMPARE(animator.node(third), NodeHandle::Null);
    if(data.features & AnimatorFeature::DataAttachment)
        CORRADE_COMPARE(animator.data(third), DataHandle::Null);
    /* Animation state() is tested thoroughly in animationState() */
    CORRADE_COMPARE(animator.state(third), AnimationState::Scheduled);

    /* The flags should be reflected in the batch getter as well. The nodes()
       and layerData() getters are tested in createNodeAttachment() and
       createDataAttachment() below. */
    CORRADE_COMPARE_AS(animator.flags(), Containers::arrayView<AnimationFlags>({
        {},
        AnimationFlag::Reverse|AnimationFlag::ReverseEveryOther,
        AnimationFlag::KeepOncePlayed
    }), TestSuite::Compare::Container);

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
    auto&& data = CreateRemoveData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0xab, 0x12)};

    /* Using AbstractGenericAnimator in order to access setLayer(), other than
       that it's testing the base AbstractAnimator APIs */
    struct Animator: AbstractGenericAnimator {
        explicit Animator(AnimatorHandle handle, AnimatorFeatures features): AbstractGenericAnimator{handle}, _features{features} {}

        using AbstractGenericAnimator::setLayer;
        using AbstractAnimator::create;
        using AbstractAnimator::remove;

        AnimatorFeatures doFeatures() const override { return _features; }
        void doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}

        private:
            AnimatorFeatures _features;
    } animator{animatorHandle(0xab, 0x12), data.features};
    if(data.features & AnimatorFeature::DataAttachment)
        animator.setLayer(layer);

    /* The actual intricancies of node/data attachment are tested in
       createNodeAttachment*() / createDataAttachment*() and
       attachNode*() / attachData*(), here it mainly just verifies that the
       assignment gets reset during recycle */

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
    CORRADE_COMPARE(animator.started(first), 0_nsec);
    CORRADE_COMPARE(animator.paused(first), Nanoseconds::max());
    CORRADE_COMPARE(animator.stopped(first), Nanoseconds::max());
    CORRADE_COMPARE(animator.duration(second), 1_nsec);
    CORRADE_COMPARE(animator.repeatCount(second), 1);
    CORRADE_COMPARE(animator.flags(second), AnimationFlags{});
    CORRADE_COMPARE(animator.started(second), 2_nsec);
    CORRADE_COMPARE(animator.paused(second), Nanoseconds::max());
    CORRADE_COMPARE(animator.stopped(second), Nanoseconds::max());
    CORRADE_COMPARE(animator.duration(third), 281698_nsec);
    CORRADE_COMPARE(animator.repeatCount(third), 666);
    CORRADE_COMPARE(animator.flags(third), AnimationFlags{});
    CORRADE_COMPARE(animator.started(third), 2782_nsec);
    CORRADE_COMPARE(animator.paused(third), Nanoseconds::max());
    CORRADE_COMPARE(animator.stopped(third), Nanoseconds::max());
    CORRADE_COMPARE(animator.duration(fourth), 78888_nsec);
    CORRADE_COMPARE(animator.repeatCount(fourth), 1);
    CORRADE_COMPARE(animator.flags(fourth), AnimationFlags{});
    CORRADE_COMPARE(animator.started(fourth), 166_nsec);
    CORRADE_COMPARE(animator.paused(fourth), Nanoseconds::max());
    CORRADE_COMPARE(animator.stopped(fourth), Nanoseconds::max());
    if(data.features & AnimatorFeature::NodeAttachment) {
        CORRADE_COMPARE(animator.node(first), NodeHandle::Null);
        CORRADE_COMPARE(animator.node(second), NodeHandle::Null);
        CORRADE_COMPARE(animator.node(third), NodeHandle::Null);
        CORRADE_COMPARE(animator.node(fourth), NodeHandle::Null);
    }
    if(data.features & AnimatorFeature::DataAttachment) {
        CORRADE_COMPARE(animator.data(first), DataHandle::Null);
        CORRADE_COMPARE(animator.data(second), DataHandle::Null);
        CORRADE_COMPARE(animator.data(third), DataHandle::Null);
        CORRADE_COMPARE(animator.data(fourth), DataHandle::Null);
    }

    /* Populate internals of some animations */
    animator.pause(first, 50_nsec);
    animator.stop(third, -30_nsec);
    if(data.features & AnimatorFeature::NodeAttachment) {
        animator.attach(second, NodeHandle(0xabc12345));
        animator.attach(fourth, NodeHandle(0x123abcde));
        CORRADE_COMPARE(animator.node(second), NodeHandle(0xabc12345));
        CORRADE_COMPARE(animator.node(fourth), NodeHandle(0x123abcde));
        CORRADE_COMPARE_AS(animator.nodes(), Containers::arrayView({
            NodeHandle::Null,
            NodeHandle(0xabc12345),
            NodeHandle::Null,
            NodeHandle(0x123abcde)
        }), TestSuite::Compare::Container);
    }
    if(data.features & AnimatorFeature::DataAttachment) {
        animator.attach(second, LayerDataHandle(0xabc12345));
        animator.attach(fourth, LayerDataHandle(0x123abcde));
        CORRADE_COMPARE(dataHandleData(animator.data(second)), LayerDataHandle(0xabc12345));
        CORRADE_COMPARE(dataHandleData(animator.data(fourth)), LayerDataHandle(0x123abcde));
        CORRADE_COMPARE_AS(animator.layerData(), Containers::arrayView({
            LayerDataHandle::Null,
            LayerDataHandle(0xabc12345),
            LayerDataHandle::Null,
            LayerDataHandle(0x123abcde)
        }), TestSuite::Compare::Container);
    }

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
    CORRADE_COMPARE(animator.started(second), 2_nsec);

    /* Internally all attachments should be set to a null handle after
       deletion */
    if(data.features & AnimatorFeature::NodeAttachment)
        CORRADE_COMPARE_AS(animator.nodes(), Containers::arrayView({
            NodeHandle::Null,
            NodeHandle(0xabc12345),
            NodeHandle::Null,
            NodeHandle::Null
        }), TestSuite::Compare::Container);
    if(data.features & AnimatorFeature::DataAttachment)
        CORRADE_COMPARE_AS(animator.layerData(), Containers::arrayView({
            LayerDataHandle::Null,
            LayerDataHandle(0xabc12345),
            LayerDataHandle::Null,
            LayerDataHandle::Null
        }), TestSuite::Compare::Container);

    /* Handles crafted with a manually incremented generation (i.e., the
       generation that will be used next) shouldn't be reported as valid */
    AnimationHandle firstNext = animationHandle(animator.handle(), animationHandleId(first), animationHandleGeneration(first) + 1);
    AnimationHandle thirdNext = animationHandle(animator.handle(), animationHandleId(third), animationHandleGeneration(third) + 1);
    AnimationHandle fourthNext = animationHandle(animator.handle(), animationHandleId(fourth), animationHandleGeneration(fourth) + 1);
    CORRADE_VERIFY(!animator.isHandleValid(firstNext));
    CORRADE_VERIFY(!animator.isHandleValid(thirdNext));
    CORRADE_VERIFY(!animator.isHandleValid(fourthNext));

    /* Allocating new handles should recycle the handles in the order they were
       removed (oldest first). They should be the same as the handles crafted
       above which should report as valid now. Their properties should be
       cleared.  */
    AnimationHandle fourth2 = animator.create(255_nsec, 8999_nsec);
    AnimationHandle first2 = animator.create(1_nsec, 14_nsec);
    AnimationHandle third2 = animator.create(2872_nsec, 896182_nsec, 333, AnimationFlags{0x40});
    CORRADE_COMPARE(first2, animationHandle(animator.handle(), 0, 2));
    CORRADE_COMPARE(third2, animationHandle(animator.handle(), 2, 2));
    CORRADE_COMPARE(fourth2, animationHandle(animator.handle(), 3, 2));
    CORRADE_COMPARE(first2, firstNext);
    CORRADE_COMPARE(third2, thirdNext);
    CORRADE_COMPARE(fourth2, fourthNext);
    CORRADE_VERIFY(animator.isHandleValid(firstNext));
    CORRADE_VERIFY(animator.isHandleValid(thirdNext));
    CORRADE_VERIFY(animator.isHandleValid(fourthNext));
    CORRADE_COMPARE(animator.capacity(), 4);
    CORRADE_COMPARE(animator.usedCount(), 4);
    CORRADE_COMPARE(animator.duration(first2), 14_nsec);
    CORRADE_COMPARE(animator.repeatCount(first2), 1);
    CORRADE_COMPARE(animator.flags(first2), AnimationFlags{});
    CORRADE_COMPARE(animator.started(first2), 1_nsec);
    CORRADE_COMPARE(animator.paused(first2), Nanoseconds::max());
    CORRADE_COMPARE(animator.stopped(first2), Nanoseconds::max());
    CORRADE_COMPARE(animator.duration(second), 1_nsec);
    CORRADE_COMPARE(animator.repeatCount(second), 1);
    CORRADE_COMPARE(animator.flags(second), AnimationFlags{});
    CORRADE_COMPARE(animator.started(second), 2_nsec);
    CORRADE_COMPARE(animator.paused(second), Nanoseconds::max());
    CORRADE_COMPARE(animator.stopped(second), Nanoseconds::max());
    CORRADE_COMPARE(animator.duration(third2), 896182_nsec);
    CORRADE_COMPARE(animator.repeatCount(third2), 333);
    CORRADE_COMPARE(animator.flags(third2), AnimationFlags{0x40});
    CORRADE_COMPARE(animator.started(third2), 2872_nsec);
    CORRADE_COMPARE(animator.paused(third2), Nanoseconds::max());
    CORRADE_COMPARE(animator.stopped(third2), Nanoseconds::max());
    CORRADE_COMPARE(animator.duration(fourth2), 8999_nsec);
    CORRADE_COMPARE(animator.repeatCount(fourth2), 1);
    CORRADE_COMPARE(animator.flags(fourth2), AnimationFlags{});
    CORRADE_COMPARE(animator.started(fourth2), 255_nsec);
    CORRADE_COMPARE(animator.paused(fourth2), Nanoseconds::max());
    CORRADE_COMPARE(animator.stopped(fourth2), Nanoseconds::max());
    if(data.features & AnimatorFeature::NodeAttachment) {
        CORRADE_COMPARE(animator.node(first2), NodeHandle::Null);
        CORRADE_COMPARE(animator.node(second), NodeHandle(0xabc12345));
        CORRADE_COMPARE(animator.node(third2), NodeHandle::Null);
        CORRADE_COMPARE(animator.node(fourth2), NodeHandle::Null);
    }
    if(data.features & AnimatorFeature::DataAttachment) {
        CORRADE_COMPARE(dataHandleData(animator.data(first2)), LayerDataHandle::Null);
        CORRADE_COMPARE(dataHandleData(animator.data(second)), LayerDataHandle(0xabc12345));
        CORRADE_COMPARE(dataHandleData(animator.data(third2)), LayerDataHandle::Null);
        CORRADE_COMPARE(dataHandleData(animator.data(fourth2)), LayerDataHandle::Null);
    }

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
    CORRADE_COMPARE(animator.started(third3), 12_nsec);
    CORRADE_COMPARE(animator.paused(third3), Nanoseconds::max());
    CORRADE_COMPARE(animator.stopped(third3), Nanoseconds::max());
    if(data.features & AnimatorFeature::NodeAttachment)
        CORRADE_COMPARE(animator.node(third3), NodeHandle::Null);
    if(data.features & AnimatorFeature::DataAttachment)
        CORRADE_COMPARE(animator.data(third3), DataHandle::Null);

    /* Allocating a new handle with the free list empty will grow it */
    AnimationHandle fifth = animator.create(2888_nsec, 8882_nsec);
    CORRADE_COMPARE(fifth, animationHandle(animator.handle(), 4, 1));
    CORRADE_VERIFY(animator.isHandleValid(fifth));
    CORRADE_COMPARE(animator.capacity(), 5);
    CORRADE_COMPARE(animator.usedCount(), 5);
    CORRADE_COMPARE(animator.duration(fifth), 8882_nsec);
    CORRADE_COMPARE(animator.repeatCount(fifth), 1);
    CORRADE_COMPARE(animator.flags(fifth), AnimationFlags{});
    CORRADE_COMPARE(animator.started(fifth), 2888_nsec);
    CORRADE_COMPARE(animator.paused(fifth), Nanoseconds::max());
    CORRADE_COMPARE(animator.stopped(fifth), Nanoseconds::max());
    if(data.features & AnimatorFeature::NodeAttachment)
        CORRADE_COMPARE(animator.node(fifth), NodeHandle::Null);
    if(data.features & AnimatorFeature::DataAttachment)
        CORRADE_COMPARE(animator.data(fifth), DataHandle::Null);

    /* The generation counter view should reflect the number of how much was
       given ID recycled */
    CORRADE_COMPARE_AS(animator.generations(), Containers::arrayView<UnsignedShort>({
        2,
        1,
        3,
        2,
        1
    }), TestSuite::Compare::Container);
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

    /* The generation counter view should have 0 for the disabled slot */
    CORRADE_COMPARE_AS(animator.generations(), Containers::arrayView<UnsignedShort>({
        1,
        0,
        1
    }), TestSuite::Compare::Container);
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

    Containers::String out;
    Error redirectError{&out};
    animator.create(17_nsec, 65_nsec);
    /* Number is hardcoded in the expected message but not elsewhere in order
       to give a heads-up when modifying the handle ID bit count */
    CORRADE_COMPARE(out,
        "Ui::AbstractAnimator::create(): can only have at most 1048576 animations\n");
}

void AbstractAnimatorTest::createInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;
        using AbstractAnimator::create;

        AnimatorFeatures doFeatures() const override { return {}; }
    } animator{animatorHandle(0, 1)};

    /* This is fine, defaulting to 1 repeat count */
    animator.create(15_nsec, 0_nsec);

    Containers::String out;
    Error redirectError{&out};
    animator.create(15_nsec, -1_nsec);
    animator.create(15_nsec, 0_nsec, 12);
    animator.create(15_nsec, 0_nsec, 0);
    CORRADE_COMPARE_AS(out,
        "Ui::AbstractAnimator::create(): expected non-negative duration, got Nanoseconds(-1)\n"
        "Ui::AbstractAnimator::create(): expected count to be 1 for an animation with zero duration but got 12\n"
        "Ui::AbstractAnimator::create(): expected count to be 1 for an animation with zero duration but got 0\n",
        TestSuite::Compare::String);
}

void AbstractAnimatorTest::createNodeAttachment() {
    /* Check just what the overload does on top of the base create(), to which
       it delegates */

    struct: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;
        using AbstractAnimator::create;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::NodeAttachment;
        }
    } animator{animatorHandle(0, 1)};

    /* Default overload */
    AnimationHandle first = animator.create(15_nsec, 37_nsec, NodeHandle(0xabcde123), 155, AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.duration(first), 37_nsec);
    CORRADE_COMPARE(animator.repeatCount(first), 155);
    CORRADE_COMPARE(animator.flags(first), AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.started(first), 15_nsec);
    CORRADE_COMPARE(animator.paused(first), Nanoseconds::max());
    CORRADE_COMPARE(animator.stopped(first), Nanoseconds::max());
    CORRADE_COMPARE(animator.node(first), NodeHandle(0xabcde123));

    /* Overload with implicit repeat count */
    AnimationHandle second = animator.create(-655_nsec, 12_nsec, NodeHandle(0x12345abc), AnimationFlag(0xe0));
    CORRADE_COMPARE(animator.duration(second), 12_nsec);
    CORRADE_COMPARE(animator.repeatCount(second), 1);
    CORRADE_COMPARE(animator.flags(second), AnimationFlag(0xe0));
    CORRADE_COMPARE(animator.started(second), -655_nsec);
    CORRADE_COMPARE(animator.paused(second), Nanoseconds::max());
    CORRADE_COMPARE(animator.stopped(second), Nanoseconds::max());
    CORRADE_COMPARE(animator.node(second), NodeHandle(0x12345abc));

    /* Null handles should be accepted too */
    AnimationHandle third = animator.create(12_nsec, 24_nsec, NodeHandle::Null, 0);
    CORRADE_COMPARE(animator.duration(third), 24_nsec);
    CORRADE_COMPARE(animator.repeatCount(third), 0);
    CORRADE_COMPARE(animator.flags(third), AnimationFlags{});
    CORRADE_COMPARE(animator.started(third), 12_nsec);
    CORRADE_COMPARE(animator.node(third), NodeHandle::Null);

    AnimationHandle fourth = animator.create(0_nsec, 1_nsec, NodeHandle::Null, AnimationFlag(0x10));
    CORRADE_COMPARE(animator.duration(fourth), 1_nsec);
    CORRADE_COMPARE(animator.repeatCount(fourth), 1);
    CORRADE_COMPARE(animator.flags(fourth), AnimationFlag(0x10));
    CORRADE_COMPARE(animator.started(fourth), 0_nsec);
    CORRADE_COMPARE(animator.node(fourth), NodeHandle::Null);

    /* The node attachments should be reflected here as well */
    CORRADE_COMPARE_AS(animator.nodes(), Containers::arrayView({
        NodeHandle(0xabcde123),
        NodeHandle(0x12345abc),
        NodeHandle::Null,
        NodeHandle::Null
    }), TestSuite::Compare::Container);
}

void AbstractAnimatorTest::createNodeAttachmentInvalidFeatures() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;
        using AbstractAnimator::create;

        AnimatorFeatures doFeatures() const override {
            /* Not NodeAttachment */
            return AnimatorFeature::DataAttachment;
        }
    } animator{animatorHandle(0, 1)};

    Containers::String out;
    Error redirectError{&out};
    animator.create(0_nsec, 1_nsec, NodeHandle::Null, 1);
    animator.create(0_nsec, 1_nsec, NodeHandle::Null, AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(out,
        "Ui::AbstractAnimator::create(): node attachment not supported\n"
        "Ui::AbstractAnimator::create(): node attachment not supported\n");
}

void AbstractAnimatorTest::createDataAttachment() {
    /* Check just what the overload does on top of the base create(), to which
       it delegates */

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0xab, 0x12)};

    /* Using AbstractGenericAnimator in order to access setLayer(), other than
       that it's testing the base AbstractAnimator APIs */
    struct: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;
        using AbstractGenericAnimator::setLayer;
        using AbstractAnimator::create;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::DataAttachment;
        }
        void doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
    } animator{animatorHandle(0, 1)};
    animator.setLayer(layer);

    /* Default overload */
    AnimationHandle first = animator.create(15_nsec, 37_nsec, dataHandle(animator.layer(), LayerDataHandle(0xabcde123)), 155, AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.duration(first), 37_nsec);
    CORRADE_COMPARE(animator.repeatCount(first), 155);
    CORRADE_COMPARE(animator.flags(first), AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.started(first), 15_nsec);
    CORRADE_COMPARE(animator.paused(first), Nanoseconds::max());
    CORRADE_COMPARE(animator.stopped(first), Nanoseconds::max());
    CORRADE_COMPARE(animator.data(first), dataHandle(animator.layer(), LayerDataHandle(0xabcde123)));

    /* LayerDataHandle variant */
    AnimationHandle second = animator.create(-37_nsec, 122_nsec, LayerDataHandle(0x123abcde), 12, AnimationFlag(0xc0));
    CORRADE_COMPARE(animator.duration(second), 122_nsec);
    CORRADE_COMPARE(animator.repeatCount(second), 12);
    CORRADE_COMPARE(animator.flags(second), AnimationFlag(0xc0));
    CORRADE_COMPARE(animator.started(second), -37_nsec);
    CORRADE_COMPARE(animator.paused(second), Nanoseconds::max());
    CORRADE_COMPARE(animator.stopped(second), Nanoseconds::max());
    CORRADE_COMPARE(animator.data(second), dataHandle(animator.layer(), LayerDataHandle(0x123abcde)));

    /* Overload with implicit repeat count */
    AnimationHandle third = animator.create(-655_nsec, 12_nsec, dataHandle(animator.layer(), LayerDataHandle(0x12345abc)), AnimationFlag(0xe0));
    CORRADE_COMPARE(animator.duration(third), 12_nsec);
    CORRADE_COMPARE(animator.repeatCount(third), 1);
    CORRADE_COMPARE(animator.flags(third), AnimationFlag(0xe0));
    CORRADE_COMPARE(animator.started(third), -655_nsec);
    CORRADE_COMPARE(animator.paused(third), Nanoseconds::max());
    CORRADE_COMPARE(animator.stopped(third), Nanoseconds::max());
    CORRADE_COMPARE(animator.data(third), dataHandle(animator.layer(), LayerDataHandle(0x12345abc)));

    /* LayerDataHandle variant */
    AnimationHandle fourth = animator.create(3_nsec, 777_nsec, LayerDataHandle(0xabc12345), AnimationFlag(0x70));
    CORRADE_COMPARE(animator.duration(fourth), 777_nsec);
    CORRADE_COMPARE(animator.repeatCount(fourth), 1);
    CORRADE_COMPARE(animator.flags(fourth), AnimationFlag(0x70));
    CORRADE_COMPARE(animator.started(fourth), 3_nsec);
    CORRADE_COMPARE(animator.paused(fourth), Nanoseconds::max());
    CORRADE_COMPARE(animator.stopped(fourth), Nanoseconds::max());
    CORRADE_COMPARE(animator.data(fourth), dataHandle(animator.layer(), LayerDataHandle(0xabc12345)));

    /* Null handles should be accepted too */
    AnimationHandle fifth1 = animator.create(12_nsec, 24_nsec, DataHandle::Null, 0);
    AnimationHandle fifth2 = animator.create(12_nsec, 24_nsec, LayerDataHandle::Null, 0);
    CORRADE_COMPARE(animator.duration(fifth1), 24_nsec);
    CORRADE_COMPARE(animator.duration(fifth2), 24_nsec);
    CORRADE_COMPARE(animator.repeatCount(fifth1), 0);
    CORRADE_COMPARE(animator.repeatCount(fifth2), 0);
    CORRADE_COMPARE(animator.flags(fifth1), AnimationFlags{});
    CORRADE_COMPARE(animator.flags(fifth2), AnimationFlags{});
    CORRADE_COMPARE(animator.started(fifth1), 12_nsec);
    CORRADE_COMPARE(animator.started(fifth2), 12_nsec);
    CORRADE_COMPARE(animator.data(fifth1), DataHandle::Null);
    CORRADE_COMPARE(animator.data(fifth2), DataHandle::Null);

    AnimationHandle sixth1 = animator.create(0_nsec, 1_nsec, DataHandle::Null, AnimationFlag(0x10));
    AnimationHandle sixth2 = animator.create(0_nsec, 1_nsec, LayerDataHandle::Null, AnimationFlag(0x10));
    CORRADE_COMPARE(animator.duration(sixth1), 1_nsec);
    CORRADE_COMPARE(animator.duration(sixth2), 1_nsec);
    CORRADE_COMPARE(animator.repeatCount(sixth1), 1);
    CORRADE_COMPARE(animator.repeatCount(sixth2), 1);
    CORRADE_COMPARE(animator.flags(sixth1), AnimationFlag(0x10));
    CORRADE_COMPARE(animator.flags(sixth2), AnimationFlag(0x10));
    CORRADE_COMPARE(animator.started(sixth1), 0_nsec);
    CORRADE_COMPARE(animator.started(sixth2), 0_nsec);
    CORRADE_COMPARE(animator.data(sixth1), DataHandle::Null);
    CORRADE_COMPARE(animator.data(sixth2), DataHandle::Null);

    /* The data attachments should be reflected here as well */
    CORRADE_COMPARE_AS(animator.layerData(), Containers::arrayView({
        LayerDataHandle(0xabcde123),
        LayerDataHandle(0x123abcde),
        LayerDataHandle(0x12345abc),
        LayerDataHandle(0xabc12345),
        LayerDataHandle::Null,
        LayerDataHandle::Null,
        LayerDataHandle::Null,
        LayerDataHandle::Null
    }), TestSuite::Compare::Container);
}

void AbstractAnimatorTest::createDataAttachmentNoLayerSet() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;
        using AbstractAnimator::create;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::DataAttachment;
        }
    } animator{animatorHandle(0, 1)};

    Containers::String out;
    Error redirectError{&out};
    animator.create(0_nsec, 1_nsec, DataHandle::Null, 1);
    animator.create(0_nsec, 1_nsec, DataHandle::Null, AnimationFlag::KeepOncePlayed);
    /* These don't work either even though there's no layer portion to compare,
       for consistency */
    animator.create(0_nsec, 1_nsec, LayerDataHandle::Null, 1);
    animator.create(0_nsec, 1_nsec, LayerDataHandle::Null, AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(out,
        "Ui::AbstractAnimator::create(): no layer set for data attachment\n"
        "Ui::AbstractAnimator::create(): no layer set for data attachment\n"
        "Ui::AbstractAnimator::create(): no layer set for data attachment\n"
        "Ui::AbstractAnimator::create(): no layer set for data attachment\n");
}

void AbstractAnimatorTest::createDataAttachmentInvalidLayer() {
    CORRADE_SKIP_IF_NO_ASSERT();

    /* Using AbstractGenericAnimator in order to access setLayer(), other than
       that it's testing the base AbstractAnimator APIs */
    struct: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;
        using AbstractGenericAnimator::setLayer;
        using AbstractAnimator::create;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::DataAttachment;
        }
        void doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
    } animator{animatorHandle(0, 1)};

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0xab, 0x12)};
    animator.setLayer(layer);

    /* Creating an animation with just a LayerDataHandle works even though
       there's no such data in the layer */
    animator.create(0_nsec, 1_nsec, layerDataHandle(0xabcde, 0x123), 1);
    animator.create(0_nsec, 1_nsec, layerDataHandle(0xabcde, 0x123), AnimationFlag::KeepOncePlayed);

    Containers::String out;
    Error redirectError{&out};
    animator.create(0_nsec, 1_nsec, dataHandle(layerHandle(0xab, 0x13), 0xabcde, 0x123), 1);
    animator.create(0_nsec, 1_nsec, dataHandle(layerHandle(0xab, 0x13), 0xabcde, 0x123), AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(out,
        "Ui::AbstractAnimator::create(): expected a data handle with Ui::LayerHandle(0xab, 0x12) but got Ui::DataHandle({0xab, 0x13}, {0xabcde, 0x123})\n"
        "Ui::AbstractAnimator::create(): expected a data handle with Ui::LayerHandle(0xab, 0x12) but got Ui::DataHandle({0xab, 0x13}, {0xabcde, 0x123})\n");
}

void AbstractAnimatorTest::createDataAttachmentInvalidFeatures() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;
        using AbstractAnimator::create;

        AnimatorFeatures doFeatures() const override {
            /* Not DataAttachment */
            return AnimatorFeature::NodeAttachment;
        }
    } animator{animatorHandle(0, 1)};

    Containers::String out;
    Error redirectError{&out};
    animator.create(0_nsec, 1_nsec, DataHandle::Null, 1);
    animator.create(0_nsec, 1_nsec, LayerDataHandle::Null, 1);
    animator.create(0_nsec, 1_nsec, DataHandle::Null, AnimationFlag::KeepOncePlayed);
    animator.create(0_nsec, 1_nsec, LayerDataHandle::Null, AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(out,
        "Ui::AbstractAnimator::create(): data attachment not supported\n"
        "Ui::AbstractAnimator::create(): data attachment not supported\n"
        "Ui::AbstractAnimator::create(): data attachment not supported\n"
        "Ui::AbstractAnimator::create(): data attachment not supported\n");
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

    Containers::String out;
    Error redirectError{&out};
    animator.remove(AnimationHandle::Null);
    /* Valid animator, invalid data */
    animator.remove(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)));
    /* Invalid animator, valid data */
    animator.remove(animationHandle(AnimatorHandle::Null, animationHandleData(handle)));
    /* AnimatorDataHandle directly */
    animator.remove(AnimatorDataHandle(0x123abcde));
    CORRADE_COMPARE_AS(out,
        "Ui::AbstractAnimator::remove(): invalid handle Ui::AnimationHandle::Null\n"
        "Ui::AbstractAnimator::remove(): invalid handle Ui::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Ui::AbstractAnimator::remove(): invalid handle Ui::AnimationHandle(Null, {0x0, 0x1})\n"
        "Ui::AbstractAnimator::remove(): invalid handle Ui::AnimatorDataHandle(0xabcde, 0x123)\n",
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

    /* The time-taking flag overloads should behave the same as above. Their
       side effects are tested in toggleFlagsAtTime(). */
    animator.setFlags(handle, AnimationFlag::KeepOncePlayed|AnimationFlags{0x20}, 0_nsec);
    CORRADE_COMPARE(animator.flags(handle), AnimationFlag::KeepOncePlayed|AnimationFlags{0x20});

    animator.addFlags(handle, AnimationFlags{0xe0}, 0_nsec);
    CORRADE_COMPARE(animator.flags(handle), AnimationFlag::KeepOncePlayed|AnimationFlags{0xe0});

    animator.clearFlags(handle, AnimationFlags{0xb0}, 0_nsec);
    CORRADE_COMPARE(animator.flags(handle), AnimationFlag::KeepOncePlayed|AnimationFlags{0x40});

    /* The AnimatorDataHandle overload of time-taking flags */
    animator.setFlags(animationHandleData(handle), AnimationFlags{0x08}, 0_nsec);
    CORRADE_COMPARE(animator.flags(animationHandleData(handle)), AnimationFlags{0x08});

    animator.addFlags(animationHandleData(handle), AnimationFlag::KeepOncePlayed|AnimationFlags{0xe0}, 0_nsec);
    CORRADE_COMPARE(animator.flags(animationHandleData(handle)), AnimationFlag::KeepOncePlayed|AnimationFlags{0xe8});

    animator.clearFlags(animationHandleData(handle), AnimationFlag::KeepOncePlayed|AnimationFlags{0xb0}, 0_nsec);
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
        animator.create(data.start*scale + offset, data.duration*scale, *data.repeatCount, AnimationFlag::KeepOncePlayed|data.flags) :
        animator.create(data.start*scale + offset, data.duration*scale, AnimationFlag::KeepOncePlayed|data.flags);
    if(data.paused)
        animator.pause(handle, *data.paused*scale + offset);
    if(data.stopped)
        animator.stop(handle, *data.stopped*scale + offset);

    Containers::BitArray mask{NoInit, 2};
    Float factors[2];
    animator.update(offset, mask, mask, mask, factors, mask);

    CORRADE_COMPARE(animator.state(handle), data.expectedState);
    CORRADE_COMPARE(animator.factor(handle), data.expectedFactor);
    /* Using also the AnimatorDataHandle overload */
    CORRADE_COMPARE(animator.state(animationHandleData(handle)), data.expectedState);
    CORRADE_COMPARE(animator.factor(animationHandleData(handle)), data.expectedFactor);
}

void AbstractAnimatorTest::propertiesInvalidHandle() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;
        using AbstractAnimator::create;

        AnimatorFeatures doFeatures() const override { return {}; }
    } animator{animatorHandle(0, 1)};

    AnimationHandle handle = animator.create(12_nsec, 13_nsec);

    Containers::String out;
    Error redirectError{&out};
    animator.duration(AnimationHandle::Null);
    animator.repeatCount(AnimationHandle::Null);
    animator.setRepeatCount(AnimationHandle::Null, 0);
    animator.flags(AnimationHandle::Null);
    animator.setFlags(AnimationHandle::Null, {});
    animator.addFlags(AnimationHandle::Null, {});
    animator.clearFlags(AnimationHandle::Null, {});
    animator.started(AnimationHandle::Null);
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
    animator.started(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)));
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
    animator.started(animationHandle(AnimatorHandle::Null, animationHandleData(handle)));
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
    animator.started(AnimatorDataHandle(0x123abcde));
    animator.paused(AnimatorDataHandle(0x123abcde));
    animator.stopped(AnimatorDataHandle(0x123abcde));
    animator.state(AnimatorDataHandle(0x123abcde));
    animator.factor(AnimatorDataHandle(0x123abcde));
    CORRADE_COMPARE_AS(out,
        "Ui::AbstractAnimator::duration(): invalid handle Ui::AnimationHandle::Null\n"
        "Ui::AbstractAnimator::repeatCount(): invalid handle Ui::AnimationHandle::Null\n"
        "Ui::AbstractAnimator::setRepeatCount(): invalid handle Ui::AnimationHandle::Null\n"
        "Ui::AbstractAnimator::flags(): invalid handle Ui::AnimationHandle::Null\n"
        "Ui::AbstractAnimator::setFlags(): invalid handle Ui::AnimationHandle::Null\n"
        "Ui::AbstractAnimator::addFlags(): invalid handle Ui::AnimationHandle::Null\n"
        "Ui::AbstractAnimator::clearFlags(): invalid handle Ui::AnimationHandle::Null\n"
        "Ui::AbstractAnimator::started(): invalid handle Ui::AnimationHandle::Null\n"
        "Ui::AbstractAnimator::paused(): invalid handle Ui::AnimationHandle::Null\n"
        "Ui::AbstractAnimator::stopped(): invalid handle Ui::AnimationHandle::Null\n"
        "Ui::AbstractAnimator::state(): invalid handle Ui::AnimationHandle::Null\n"
        "Ui::AbstractAnimator::factor(): invalid handle Ui::AnimationHandle::Null\n"

        "Ui::AbstractAnimator::duration(): invalid handle Ui::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Ui::AbstractAnimator::repeatCount(): invalid handle Ui::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Ui::AbstractAnimator::setRepeatCount(): invalid handle Ui::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Ui::AbstractAnimator::flags(): invalid handle Ui::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Ui::AbstractAnimator::setFlags(): invalid handle Ui::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Ui::AbstractAnimator::addFlags(): invalid handle Ui::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Ui::AbstractAnimator::clearFlags(): invalid handle Ui::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Ui::AbstractAnimator::started(): invalid handle Ui::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Ui::AbstractAnimator::paused(): invalid handle Ui::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Ui::AbstractAnimator::stopped(): invalid handle Ui::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Ui::AbstractAnimator::state(): invalid handle Ui::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Ui::AbstractAnimator::factor(): invalid handle Ui::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"

        "Ui::AbstractAnimator::duration(): invalid handle Ui::AnimationHandle(Null, {0x0, 0x1})\n"
        "Ui::AbstractAnimator::repeatCount(): invalid handle Ui::AnimationHandle(Null, {0x0, 0x1})\n"
        "Ui::AbstractAnimator::setRepeatCount(): invalid handle Ui::AnimationHandle(Null, {0x0, 0x1})\n"
        "Ui::AbstractAnimator::flags(): invalid handle Ui::AnimationHandle(Null, {0x0, 0x1})\n"
        "Ui::AbstractAnimator::setFlags(): invalid handle Ui::AnimationHandle(Null, {0x0, 0x1})\n"
        "Ui::AbstractAnimator::addFlags(): invalid handle Ui::AnimationHandle(Null, {0x0, 0x1})\n"
        "Ui::AbstractAnimator::clearFlags(): invalid handle Ui::AnimationHandle(Null, {0x0, 0x1})\n"
        "Ui::AbstractAnimator::started(): invalid handle Ui::AnimationHandle(Null, {0x0, 0x1})\n"
        "Ui::AbstractAnimator::paused(): invalid handle Ui::AnimationHandle(Null, {0x0, 0x1})\n"
        "Ui::AbstractAnimator::stopped(): invalid handle Ui::AnimationHandle(Null, {0x0, 0x1})\n"
        "Ui::AbstractAnimator::state(): invalid handle Ui::AnimationHandle(Null, {0x0, 0x1})\n"
        "Ui::AbstractAnimator::factor(): invalid handle Ui::AnimationHandle(Null, {0x0, 0x1})\n"

        "Ui::AbstractAnimator::duration(): invalid handle Ui::AnimatorDataHandle(0xabcde, 0x123)\n"
        "Ui::AbstractAnimator::repeatCount(): invalid handle Ui::AnimatorDataHandle(0xabcde, 0x123)\n"
        "Ui::AbstractAnimator::setRepeatCount(): invalid handle Ui::AnimatorDataHandle(0xabcde, 0x123)\n"
        "Ui::AbstractAnimator::flags(): invalid handle Ui::AnimatorDataHandle(0xabcde, 0x123)\n"
        "Ui::AbstractAnimator::setFlags(): invalid handle Ui::AnimatorDataHandle(0xabcde, 0x123)\n"
        "Ui::AbstractAnimator::addFlags(): invalid handle Ui::AnimatorDataHandle(0xabcde, 0x123)\n"
        "Ui::AbstractAnimator::clearFlags(): invalid handle Ui::AnimatorDataHandle(0xabcde, 0x123)\n"
        "Ui::AbstractAnimator::started(): invalid handle Ui::AnimatorDataHandle(0xabcde, 0x123)\n"
        "Ui::AbstractAnimator::paused(): invalid handle Ui::AnimatorDataHandle(0xabcde, 0x123)\n"
        "Ui::AbstractAnimator::stopped(): invalid handle Ui::AnimatorDataHandle(0xabcde, 0x123)\n"
        "Ui::AbstractAnimator::state(): invalid handle Ui::AnimatorDataHandle(0xabcde, 0x123)\n"
        "Ui::AbstractAnimator::factor(): invalid handle Ui::AnimatorDataHandle(0xabcde, 0x123)\n",
        TestSuite::Compare::String);
}

void AbstractAnimatorTest::propertiesInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;
        using AbstractAnimator::create;

        AnimatorFeatures doFeatures() const override { return {}; }
    } animator{animatorHandle(0, 1)};

    AnimationHandle zeroDuration = animator.create(12_nsec, 0_nsec);

    Containers::String out;
    Error redirectError{&out};
    animator.setRepeatCount(zeroDuration, 12);
    animator.setRepeatCount(animationHandleData(zeroDuration), 12);
    animator.setRepeatCount(zeroDuration, 0);
    animator.setRepeatCount(animationHandleData(zeroDuration), 0);
    CORRADE_COMPARE_AS(out,
        "Ui::AbstractAnimator::setRepeatCount(): expected count to be 1 for an animation with zero duration but got 12\n"
        "Ui::AbstractAnimator::setRepeatCount(): expected count to be 1 for an animation with zero duration but got 12\n"
        "Ui::AbstractAnimator::setRepeatCount(): expected count to be 1 for an animation with zero duration but got 0\n"
        "Ui::AbstractAnimator::setRepeatCount(): expected count to be 1 for an animation with zero duration but got 0\n",
        TestSuite::Compare::String);
}

void AbstractAnimatorTest::attachNode() {
    /* Mostly the same as AbstractLayerTest::attach() */

    struct: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;
        using AbstractAnimator::create;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::NodeAttachment;
        }
    } animator{animatorHandle(0xab, 0x12)};

    /* Create animations that are stoppped to not affect animator state */
    AnimationHandle first = animator.create(-10_nsec, 5_nsec, AnimationFlag::KeepOncePlayed);
    AnimationHandle second = animator.create(-100_nsec, 50_nsec, AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.node(first), NodeHandle::Null);
    CORRADE_COMPARE(animator.node(second), NodeHandle::Null);

    NodeHandle nodeFirst = nodeHandle(2865, 0xcec);
    NodeHandle nodeSecond = nodeHandle(9872, 0xbeb);
    NodeHandle nodeThird = nodeHandle(12, 0x888);

    /* Attaching shouldn't affect animator state */
    animator.attach(first, nodeSecond);
    CORRADE_COMPARE(animator.state(), AnimatorStates{});
    CORRADE_COMPARE(animator.node(first), nodeSecond);

    /* The attachment should be reflected in the view as well */
    CORRADE_COMPARE_AS(animator.nodes(), Containers::arrayView({
        nodeSecond,
        NodeHandle::Null
    }), TestSuite::Compare::Container);

    /* Calling with the animator-specific handles should work too */
    animator.attach(animationHandleData(second), nodeFirst);
    CORRADE_COMPARE(animator.state(), AnimatorStates{});
    CORRADE_COMPARE(animator.node(animationHandleData(second)), nodeFirst);

    /* Attaching to a new node should overwrite the previous */
    animator.attach(first, nodeThird);
    CORRADE_COMPARE(animator.state(), AnimatorStates{});
    CORRADE_COMPARE(animator.node(first), nodeThird);

    /* Attaching two animations to the same node should work too */
    animator.attach(second, nodeThird);
    CORRADE_COMPARE(animator.state(), AnimatorStates{});
    CORRADE_COMPARE(animator.node(first), nodeThird);
    CORRADE_COMPARE(animator.node(second), nodeThird);

    /* Detaching as well */
    animator.attach(first, NodeHandle::Null);
    CORRADE_COMPARE(animator.state(), AnimatorStates{});
    CORRADE_COMPARE(animator.node(first), NodeHandle::Null);
    CORRADE_COMPARE(animator.node(second), nodeThird);

    /* The cleared attachment should be reflected in the view as well */
    CORRADE_COMPARE_AS(animator.nodes(), Containers::arrayView({
        NodeHandle::Null,
        nodeThird
    }), TestSuite::Compare::Container);
}

void AbstractAnimatorTest::attachNodeInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;
        using AbstractAnimator::create;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::NodeAttachment;
        }
    } animator{animatorHandle(0xab, 0x12)};

    AnimationHandle handle = animator.create(0_nsec, 1_nsec);

    Containers::String out;
    Error redirectError{&out};
    animator.attach(AnimationHandle::Null, nodeHandle(2865, 0xcec));
    animator.node(AnimationHandle::Null);
    /* Valid animator, invalid data */
    animator.attach(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)), nodeHandle(2865, 0xcec));
    animator.node(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)));
    /* Invalid animator, valid data */
    animator.attach(animationHandle(AnimatorHandle::Null, animationHandleData(handle)), nodeHandle(2865, 0xcec));
    animator.node(animationHandle(AnimatorHandle::Null, animationHandleData(handle)));
    /* AnimatorDataHandle directly */
    animator.attach(AnimatorDataHandle(0x123abcde), nodeHandle(2865, 0xcec));
    animator.node(AnimatorDataHandle(0x123abcde));
    CORRADE_COMPARE_AS(out,
        "Ui::AbstractAnimator::attach(): invalid handle Ui::AnimationHandle::Null\n"
        "Ui::AbstractAnimator::node(): invalid handle Ui::AnimationHandle::Null\n"
        "Ui::AbstractAnimator::attach(): invalid handle Ui::AnimationHandle({0xab, 0x12}, {0xabcde, 0x123})\n"
        "Ui::AbstractAnimator::node(): invalid handle Ui::AnimationHandle({0xab, 0x12}, {0xabcde, 0x123})\n"
        "Ui::AbstractAnimator::attach(): invalid handle Ui::AnimationHandle(Null, {0x0, 0x1})\n"
        "Ui::AbstractAnimator::node(): invalid handle Ui::AnimationHandle(Null, {0x0, 0x1})\n"
        "Ui::AbstractAnimator::attach(): invalid handle Ui::AnimatorDataHandle(0xabcde, 0x123)\n"
        "Ui::AbstractAnimator::node(): invalid handle Ui::AnimatorDataHandle(0xabcde, 0x123)\n",
        TestSuite::Compare::String);
}

void AbstractAnimatorTest::attachNodeInvalidFeatures() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;
        using AbstractAnimator::create;

        AnimatorFeatures doFeatures() const override {
            /* Not NodeAttachment */
            return AnimatorFeature::DataAttachment;
        }
    } animator{animatorHandle(0, 1)};

    AnimationHandle handle = animator.create(0_nsec, 1_nsec);

    Containers::String out;
    Error redirectError{&out};
    animator.attach(handle, nodeHandle(2865, 0xcec));
    animator.attach(animationHandleData(handle), nodeHandle(2865, 0xcec));
    animator.node(handle);
    animator.node(animationHandleData(handle));
    animator.nodes();
    CORRADE_COMPARE_AS(out,
        "Ui::AbstractAnimator::attach(): node attachment not supported\n"
        "Ui::AbstractAnimator::attach(): node attachment not supported\n"
        "Ui::AbstractAnimator::node(): feature not supported\n"
        "Ui::AbstractAnimator::node(): feature not supported\n"
        "Ui::AbstractAnimator::nodes(): feature not supported\n",
        TestSuite::Compare::String);
}

void AbstractAnimatorTest::attachData() {
    /* Expands on attachData() with additional variants for DataHandle vs
       LayerDataHandle */

    /* Using AbstractGenericAnimator in order to access setLayer(), other than
       that it's testing the base AbstractAnimator APIs */
    struct: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;
        using AbstractGenericAnimator::setLayer;
        using AbstractAnimator::create;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::DataAttachment;
        }
        void doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
    } animator{animatorHandle(0xab, 0x12)};

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0xcd, 0x34)};
    animator.setLayer(layer);

    /* Create animations that are stoppped to not affect animator state */
    AnimationHandle first = animator.create(-10_nsec, 5_nsec, AnimationFlag::KeepOncePlayed);
    AnimationHandle second = animator.create(-100_nsec, 50_nsec, AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.data(first), DataHandle::Null);
    CORRADE_COMPARE(animator.data(second), DataHandle::Null);

    DataHandle dataFirst = dataHandle(animator.layer(), 2865, 0xcec);
    DataHandle dataSecond = dataHandle(animator.layer(), 9872, 0xbeb);
    DataHandle dataThird = dataHandle(animator.layer(), 12, 0x888);

    /* Attaching shouldn't affect animator state */
    animator.attach(first, dataSecond);
    CORRADE_COMPARE(animator.state(), AnimatorStates{});
    CORRADE_COMPARE(animator.data(first), dataSecond);

    /* The attachment should be reflected in the view as well */
    CORRADE_COMPARE_AS(animator.layerData(), Containers::arrayView({
        dataHandleData(dataSecond),
        LayerDataHandle::Null
    }), TestSuite::Compare::Container);

    /* Calling with the animator-specific handles should work too */
    animator.attach(animationHandleData(second), dataFirst);
    CORRADE_COMPARE(animator.state(), AnimatorStates{});
    CORRADE_COMPARE(animator.data(animationHandleData(second)), dataFirst);

    /* Attaching to a new data should overwrite the previous */
    animator.attach(first, dataThird);
    CORRADE_COMPARE(animator.state(), AnimatorStates{});
    CORRADE_COMPARE(animator.data(first), dataThird);

    /* Attaching two animations to the same node should work too */
    animator.attach(second, dataThird);
    CORRADE_COMPARE(animator.state(), AnimatorStates{});
    CORRADE_COMPARE(animator.data(first), dataThird);
    CORRADE_COMPARE(animator.data(second), dataThird);

    /* Detaching as well */
    animator.attach(first, DataHandle::Null);
    CORRADE_COMPARE(animator.state(), AnimatorStates{});
    CORRADE_COMPARE(animator.data(first), DataHandle::Null);
    CORRADE_COMPARE(animator.data(second), dataThird);

    /* The cleared attachment should be reflected in the view as well */
    CORRADE_COMPARE_AS(animator.layerData(), Containers::arrayView({
        LayerDataHandle::Null,
        dataHandleData(dataThird)
    }), TestSuite::Compare::Container);

    /* Verify the LayerDataHandle overloads work too */
    animator.attach(first, dataHandleData(dataSecond));
    animator.attach(second, LayerDataHandle::Null);
    CORRADE_COMPARE(animator.state(), AnimatorStates{});
    CORRADE_COMPARE(animator.data(first), dataSecond);
    CORRADE_COMPARE(animator.data(second), DataHandle::Null);
    CORRADE_COMPARE_AS(animator.layerData(), Containers::arrayView({
        dataHandleData(dataSecond),
        LayerDataHandle::Null
    }), TestSuite::Compare::Container);

    /* And the AnimatorDataHandle + LayerDataHandle overloads also */
    animator.attach(animationHandleData(first), LayerDataHandle::Null);
    animator.attach(animationHandleData(second), dataHandleData(dataFirst));
    CORRADE_COMPARE(animator.state(), AnimatorStates{});
    CORRADE_COMPARE(animator.data(first), DataHandle::Null);
    CORRADE_COMPARE(animator.data(second), dataFirst);
    CORRADE_COMPARE_AS(animator.layerData(), Containers::arrayView({
        LayerDataHandle::Null,
        dataHandleData(dataFirst)
    }), TestSuite::Compare::Container);
}

void AbstractAnimatorTest::attachDataInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;
        using AbstractAnimator::create;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::DataAttachment;
        }
    } animator{animatorHandle(0xab, 0x12)};

    /* Don't need to call setLayer() here as the animation handle validity is
       checked as the first thing, before everything else */

    AnimationHandle handle = animator.create(0_nsec, 1_nsec);

    Containers::String out;
    Error redirectError{&out};
    animator.attach(AnimationHandle::Null, dataHandle(animator.layer(), 2865, 0xcec));
    animator.attach(AnimationHandle::Null, layerDataHandle(2865, 0xcec));
    animator.data(AnimationHandle::Null);
    /* Valid animator, invalid data */
    animator.attach(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)), dataHandle(animator.layer(), 2865, 0xcec));
    animator.attach(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)), layerDataHandle(2865, 0xcec));
    animator.data(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)));
    /* Invalid animator, valid data */
    animator.attach(animationHandle(AnimatorHandle::Null, animationHandleData(handle)), dataHandle(animator.layer(), 2865, 0xcec));
    animator.attach(animationHandle(AnimatorHandle::Null, animationHandleData(handle)), layerDataHandle(2865, 0xcec));
    animator.data(animationHandle(AnimatorHandle::Null, animationHandleData(handle)));
    /* AnimatorDataHandle directly */
    animator.attach(AnimatorDataHandle(0x123abcde), dataHandle(animator.layer(), 2865, 0xcec));
    animator.attach(AnimatorDataHandle(0x123abcde), layerDataHandle(2865, 0xcec));
    animator.data(AnimatorDataHandle(0x123abcde));
    CORRADE_COMPARE_AS(out,
        "Ui::AbstractAnimator::attach(): invalid handle Ui::AnimationHandle::Null\n"
        "Ui::AbstractAnimator::attach(): invalid handle Ui::AnimationHandle::Null\n"
        "Ui::AbstractAnimator::data(): invalid handle Ui::AnimationHandle::Null\n"
        "Ui::AbstractAnimator::attach(): invalid handle Ui::AnimationHandle({0xab, 0x12}, {0xabcde, 0x123})\n"
        "Ui::AbstractAnimator::attach(): invalid handle Ui::AnimationHandle({0xab, 0x12}, {0xabcde, 0x123})\n"
        "Ui::AbstractAnimator::data(): invalid handle Ui::AnimationHandle({0xab, 0x12}, {0xabcde, 0x123})\n"
        "Ui::AbstractAnimator::attach(): invalid handle Ui::AnimationHandle(Null, {0x0, 0x1})\n"
        "Ui::AbstractAnimator::attach(): invalid handle Ui::AnimationHandle(Null, {0x0, 0x1})\n"
        "Ui::AbstractAnimator::data(): invalid handle Ui::AnimationHandle(Null, {0x0, 0x1})\n"
        "Ui::AbstractAnimator::attach(): invalid handle Ui::AnimatorDataHandle(0xabcde, 0x123)\n"
        "Ui::AbstractAnimator::attach(): invalid handle Ui::AnimatorDataHandle(0xabcde, 0x123)\n"
        "Ui::AbstractAnimator::data(): invalid handle Ui::AnimatorDataHandle(0xabcde, 0x123)\n",
        TestSuite::Compare::String);
}

void AbstractAnimatorTest::attachDataNoLayerSet() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;
        using AbstractAnimator::create;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::DataAttachment;
        }
    } animator{animatorHandle(0xab, 0x12)};

    AnimationHandle handle = animator.create(0_nsec, 1_nsec);

    /* Querying the attachment works in this case, it returns null */
    CORRADE_COMPARE(animator.data(handle), DataHandle::Null);
    CORRADE_COMPARE(animator.data(animationHandleData(handle)), DataHandle::Null);

    Containers::String out;
    Error redirectError{&out};
    animator.attach(handle, dataHandle(animator.layer(), 2865, 0xcec));
    animator.attach(handle, layerDataHandle(2865, 0xcec));
    animator.attach(animationHandleData(handle), dataHandle(animator.layer(), 2865, 0xcec));
    animator.attach(animationHandleData(handle), layerDataHandle(2865, 0xcec));
    CORRADE_COMPARE(out,
        "Ui::AbstractAnimator::attach(): no layer set for data attachment\n"
        "Ui::AbstractAnimator::attach(): no layer set for data attachment\n"
        "Ui::AbstractAnimator::attach(): no layer set for data attachment\n"
        "Ui::AbstractAnimator::attach(): no layer set for data attachment\n");
}

void AbstractAnimatorTest::attachDataInvalidLayer() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0xab, 0x12)};

    /* Using AbstractGenericAnimator in order to access setLayer(), other than
       that it's testing the base AbstractAnimator APIs */
    struct: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;
        using AbstractGenericAnimator::setLayer;
        using AbstractAnimator::create;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::DataAttachment;
        }
        void doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
    } animator{animatorHandle(0, 1)};
    animator.setLayer(layer);

    AnimationHandle handle = animator.create(0_nsec, 1_nsec);

    /* Attaching to just a LayerDataHandle works even though there's no such
       data in the layer */
    animator.attach(handle, layerDataHandle(0xabcde, 0x123));
    animator.attach(animationHandleData(handle), layerDataHandle(0xabcde, 0x123));

    Containers::String out;
    Error redirectError{&out};
    animator.attach(handle, dataHandle(layerHandle(0xab, 0x13), 0xabcde, 0x123));
    animator.attach(animationHandleData(handle), dataHandle(layerHandle(0xab, 0x13), 0xabcde, 0x123));
    CORRADE_COMPARE(out,
        "Ui::AbstractAnimator::attach(): expected a data handle with Ui::LayerHandle(0xab, 0x12) but got Ui::DataHandle({0xab, 0x13}, {0xabcde, 0x123})\n"
        "Ui::AbstractAnimator::attach(): expected a data handle with Ui::LayerHandle(0xab, 0x12) but got Ui::DataHandle({0xab, 0x13}, {0xabcde, 0x123})\n");
}

void AbstractAnimatorTest::attachDataInvalidFeatures() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;
        using AbstractAnimator::create;

        AnimatorFeatures doFeatures() const override {
            /* Not DataAttachment */
            return AnimatorFeature::NodeAttachment;
        }
    } animator{animatorHandle(0, 1)};

    AnimationHandle handle = animator.create(0_nsec, 1_nsec);

    Containers::String out;
    Error redirectError{&out};
    animator.attach(handle, DataHandle::Null);
    animator.attach(handle, LayerDataHandle::Null);
    animator.attach(animationHandleData(handle), DataHandle::Null);
    animator.attach(animationHandleData(handle), LayerDataHandle::Null);
    animator.data(handle);
    animator.data(animationHandleData(handle));
    animator.layerData();
    CORRADE_COMPARE_AS(out,
        "Ui::AbstractAnimator::attach(): data attachment not supported\n"
        "Ui::AbstractAnimator::attach(): data attachment not supported\n"
        "Ui::AbstractAnimator::attach(): data attachment not supported\n"
        "Ui::AbstractAnimator::attach(): data attachment not supported\n"
        "Ui::AbstractAnimator::data(): feature not supported\n"
        "Ui::AbstractAnimator::data(): feature not supported\n"
        "Ui::AbstractAnimator::layerData(): feature not supported\n",
        TestSuite::Compare::String);
}

void AbstractAnimatorTest::clean() {
    auto&& data = CleanData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Using AbstractGenericAnimator in order to access setLayer(), other than
       that it's testing the base AbstractAnimator APIs */
    struct Animator: AbstractGenericAnimator {
        explicit Animator(AnimatorHandle handle, AnimatorFeatures features): AbstractGenericAnimator{handle}, _features{features} {}

        using AbstractGenericAnimator::setLayer;
        using AbstractAnimator::create;
        using AbstractAnimator::remove;

        AnimatorFeatures doFeatures() const override { return _features; }
        void doClean(Containers::BitArrayView animationIdsToRemove) override {
            ++called;

            /* The attachments should still be valid at this point, even though
               the animations get removed, to make it possible for the
               implementation to do cleanup based on those */
            if(_features >= AnimatorFeature::NodeAttachment) {
                CORRADE_COMPARE_AS(nodes(), Containers::arrayView({
                    nodeHandle(0x1234, 1),
                    NodeHandle::Null,
                    nodeHandle(0x5678, 1),
                    nodeHandle(0x9abc, 1),
                }), TestSuite::Compare::Container);
            } else if(_features >= AnimatorFeature::DataAttachment) {
                CORRADE_COMPARE_AS(layerData(), Containers::arrayView({
                    layerDataHandle(0x1234, 1),
                    LayerDataHandle::Null,
                    layerDataHandle(0x5678, 1),
                    layerDataHandle(0x9abc, 1),
                }), TestSuite::Compare::Container);
            }

            CORRADE_COMPARE_AS(animationIdsToRemove, Containers::stridedArrayView({
                true, false, true, false
            }).sliceBit(0), TestSuite::Compare::Container);
        }
        void doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}

        Int called = 0;

        private:
            AnimatorFeatures _features;

    } animator{animatorHandle(0, 1), data.features};

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0, 1)};
    if(data.features >= AnimatorFeature::DataAttachment)
        animator.setLayer(layer);

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    /* Create four animations to match the four bits, remove one of them */
    AnimationHandle first = animator.create(7_nsec, 65_nsec);
    AnimationHandle second = animator.create(2_nsec, 3_nsec);
    AnimationHandle third = animator.create(323_nsec, 2_nsec);
    AnimationHandle fourth = animator.create(0_nsec, 1_nsec);
    animator.remove(second);

    /* Attach them if supported */
    if(data.features >= AnimatorFeature::NodeAttachment) {
        animator.attach(first, nodeHandle(0x1234, 1));
        animator.attach(third, nodeHandle(0x5678, 1));
        animator.attach(fourth, nodeHandle(0x9abc, 1));
    } else if(data.features >= AnimatorFeature::DataAttachment) {
        animator.attach(first, layerDataHandle(0x1234, 1));
        animator.attach(third, layerDataHandle(0x5678, 1));
        animator.attach(fourth, layerDataHandle(0x9abc, 1));
    }

    /* Call clean() */
    animator.clean(Containers::BitArrayView{"\x05", 0, 4});
    CORRADE_COMPARE(animator.called, 1);

    /* Only the fourth data should stay afterwards */
    CORRADE_VERIFY(!animator.isHandleValid(first));
    CORRADE_VERIFY(!animator.isHandleValid(second));
    CORRADE_VERIFY(!animator.isHandleValid(third));
    CORRADE_VERIFY(animator.isHandleValid(fourth));

    /* The attachments should be cleared for removed animations */
    if(data.features >= AnimatorFeature::NodeAttachment) {
        CORRADE_COMPARE_AS(animator.nodes(), Containers::arrayView({
            NodeHandle::Null,
            NodeHandle::Null,
            NodeHandle::Null,
            nodeHandle(0x9abc, 1),
        }), TestSuite::Compare::Container);
    } else if(data.features >= AnimatorFeature::DataAttachment) {
        CORRADE_COMPARE_AS(animator.layerData(), Containers::arrayView({
            LayerDataHandle::Null,
            LayerDataHandle::Null,
            LayerDataHandle::Null,
            layerDataHandle(0x9abc, 1),
        }), TestSuite::Compare::Container);
    }
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

    Containers::String out;
    Error redirectError{&out};
    UnsignedByte data[1]{};
    animator.clean(Containers::BitArrayView{data, 0, 2});
    CORRADE_COMPARE(out, "Ui::AbstractAnimator::clean(): expected 3 bits but got 2\n");
}

void AbstractAnimatorTest::cleanNodes() {
    /* Mostly the same as AbstractLayerTest::clean() */

    struct: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;
        using AbstractAnimator::create;
        using AbstractAnimator::remove;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::NodeAttachment;
        }
        void doClean(Containers::BitArrayView dataIdsToRemove) override {
            ++called;

            /* Compared to regular clean(), the node attachments should be
               cleared for removed animations at this point already as
               cleanData() is meant to be called at a point where the original
               nodes don't exist anymore, thus keeping invalid handles wouldn't
               make sense */
            CORRADE_COMPARE_AS(nodes(), Containers::arrayView({
                NodeHandle::Null,
                NodeHandle::Null,
                NodeHandle::Null,
                NodeHandle::Null,
                nodeHandle(3, 0xaba),
                NodeHandle::Null,
                NodeHandle::Null,
            }), TestSuite::Compare::Container);

            CORRADE_COMPARE_AS(dataIdsToRemove, Containers::stridedArrayView({
                true, false, false, true, false, true, false
            }).sliceBit(0), TestSuite::Compare::Container);
        }

        Int called = 0;
    } animator{animatorHandle(0, 1)};

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    NodeHandle nodeFirst = nodeHandle(0, 0xcec);
    NodeHandle nodeSecond = nodeHandle(1, 0xded);
    NodeHandle nodeFourth = nodeHandle(3, 0xaba);
    NodeHandle nodeEighth = nodeHandle(7, 0xfef);

    /* Create seven animations to match the seven bits. Attach them to random
       handles, leave one unassigned, attach two animations to one node. */
    AnimationHandle first = animator.create(0_nsec, 1_nsec, nodeEighth);
    AnimationHandle second = animator.create(0_nsec, 1_nsec);
    AnimationHandle third = animator.create(0_nsec, 1_nsec, nodeSecond);
    AnimationHandle fourth = animator.create(0_nsec, 1_nsec, nodeFirst);
    AnimationHandle fifth = animator.create(0_nsec, 1_nsec, nodeFourth);
    AnimationHandle sixth = animator.create(0_nsec, 1_nsec, nodeFirst);
    AnimationHandle seventh = animator.create(0_nsec, 1_nsec, nodeFourth);

    /* Remove two of them */
    animator.remove(third);
    animator.remove(seventh);

    /* Call cleanNodes() with updated generation counters */
    animator.cleanNodes(Containers::arrayView({
        /* First node generation gets different, affecting fourth and sixth
           animation */
        UnsignedShort(nodeHandleGeneration(nodeFirst) + 1),
        /* Second node generation gets different but since the third animation
           is already removed it doesn't affect anything */
        UnsignedShort(nodeHandleGeneration(nodeSecond) - 1),
        /* Third node has no attachments so it can be arbitrary */
        UnsignedShort{0xbeb},
        /* Fourth node stays the same generation so the fifth animation stays.
           Seventh animation is already removed so they aren't set for deletion
           either. */
        UnsignedShort(nodeHandleGeneration(nodeFourth)),
        /* Fifth, sixth, seventh nodes have no attachments so they can be
           arbitrary again */
        UnsignedShort{0xaca},
        UnsignedShort{0x808},
        UnsignedShort{0xefe},
        /* Eighth node is now a zero generation, i.e. disabled, which should
           trigger removal of first animation */
        UnsignedShort{},
    }));
    CORRADE_COMPARE(animator.called, 1);

    /* Only the second and fifth data should stay afterwards */
    CORRADE_VERIFY(!animator.isHandleValid(first));
    CORRADE_VERIFY(animator.isHandleValid(second));
    CORRADE_VERIFY(!animator.isHandleValid(third));
    CORRADE_VERIFY(!animator.isHandleValid(fourth));
    CORRADE_VERIFY(animator.isHandleValid(fifth));
    CORRADE_VERIFY(!animator.isHandleValid(sixth));
    CORRADE_VERIFY(!animator.isHandleValid(seventh));

    /* The node attachments should be cleared for removed animations */
    CORRADE_COMPARE_AS(animator.nodes(), Containers::arrayView({
        NodeHandle::Null,
        NodeHandle::Null,
        NodeHandle::Null,
        NodeHandle::Null,
        nodeFourth,
        NodeHandle::Null,
        NodeHandle::Null,
    }), TestSuite::Compare::Container);
}

void AbstractAnimatorTest::cleanNodesEmpty() {
    /* Mostly the same as AbstractLayerTest::cleanEmpty() */

    struct: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::NodeAttachment;
        }
        void doClean(Containers::BitArrayView) override {
            ++called;
        }

        Int called = 0;
    } animator{animatorHandle(0, 1)};

    /* It should call the implementation even with empty contents */
    animator.cleanNodes({});
    CORRADE_COMPARE(animator.called, 1);
}

void AbstractAnimatorTest::cleanNodesNotImplemented() {
    /* Mostly the same as AbstractLayerTest::cleanNotImplemented() */

    struct: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::NodeAttachment;
        }
    } animator{animatorHandle(0, 1)};

    animator.cleanNodes({});

    /* Shouldn't crash or anything */
    CORRADE_VERIFY(true);
}

void AbstractAnimatorTest::cleanNodesInvalidFeatures() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;

        AnimatorFeatures doFeatures() const override { return {}; }
    } animator{animatorHandle(0, 1)};

    Containers::String out;
    Error redirectError{&out};
    animator.cleanNodes({});
    CORRADE_COMPARE(out, "Ui::AbstractAnimator::cleanNodes(): feature not supported\n");
}

void AbstractAnimatorTest::cleanData() {
    /* Like cleanNodes(), just handling data instead */

    /* Using AbstractGenericAnimator in order to access setLayer(), other than
       that it's testing the base AbstractAnimator APIs */
    struct: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;
        using AbstractGenericAnimator::setLayer;
        using AbstractAnimator::create;
        using AbstractAnimator::remove;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::DataAttachment;
        }
        void doClean(Containers::BitArrayView dataIdsToRemove) override {
            ++called;

            /* Compared to regular clean(), the data attachments should be
               cleared for removed animations at this point already as
               cleanData() is meant to be called at a point where the original
               data don't exist anymore, thus keeping invalid handles wouldn't
               make sense */
            CORRADE_COMPARE_AS(layerData(), Containers::arrayView({
                LayerDataHandle::Null,
                LayerDataHandle::Null,
                LayerDataHandle::Null,
                LayerDataHandle::Null,
                layerDataHandle(3, 0xaba),
                LayerDataHandle::Null,
                LayerDataHandle::Null,
            }), TestSuite::Compare::Container);

            CORRADE_COMPARE_AS(dataIdsToRemove, Containers::stridedArrayView({
                true, false, false, true, false, true, false
            }).sliceBit(0), TestSuite::Compare::Container);
        }
        void doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}

        Int called = 0;
    } animator{animatorHandle(0, 1)};

    /* Has to be called to actually be able to attach the data or clean them,
       but other than that doesn't affect the results in any way */
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0xab, 0x12)};
    animator.setLayer(layer);

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    LayerDataHandle dataFirst = layerDataHandle(0, 0xcec);
    LayerDataHandle dataSecond = layerDataHandle(1, 0xded);
    LayerDataHandle dataFourth = layerDataHandle(3, 0xaba);
    LayerDataHandle dataEighth = layerDataHandle(7, 0xfef);

    /* Create seven animations to match the seven bits. Attach them to random
       handles, leave one unassigned, attach two animations to one data. */
    AnimationHandle first = animator.create(0_nsec, 1_nsec, dataEighth);
    AnimationHandle second = animator.create(0_nsec, 1_nsec);
    AnimationHandle third = animator.create(0_nsec, 1_nsec, dataSecond);
    AnimationHandle fourth = animator.create(0_nsec, 1_nsec, dataFirst);
    AnimationHandle fifth = animator.create(0_nsec, 1_nsec, dataFourth);
    AnimationHandle sixth = animator.create(0_nsec, 1_nsec, dataFirst);
    AnimationHandle seventh = animator.create(0_nsec, 1_nsec, dataFourth);

    /* Remove two of them */
    animator.remove(third);
    animator.remove(seventh);

    /* Call cleanData() with updated generation counters */
    animator.cleanData(Containers::arrayView({
        /* First data generation gets different, affecting fourth and sixth
           animation */
        UnsignedShort(layerDataHandleGeneration(dataFirst) + 1),
        /* Second data generation gets different but since the third animation
           is already removed it doesn't affect anything */
        UnsignedShort(layerDataHandleGeneration(dataSecond) - 1),
        /* Third data has no attachments so it can be arbitrary */
        UnsignedShort{0xbeb},
        /* Fourth data stays the same generation so the fifth animation stays.
           Seventh animation is already removed so they aren't set for deletion
           either. */
        UnsignedShort(layerDataHandleGeneration(dataFourth)),
        /* Fifth, sixth, seventh data have no attachments so they can be
           arbitrary again */
        UnsignedShort{0xaca},
        UnsignedShort{0x808},
        UnsignedShort{0xefe},
        /* Eighth data is now a zero generation, i.e. disabled, which should
           trigger removal of first animation */
        UnsignedShort{},
    }));
    CORRADE_COMPARE(animator.called, 1);

    /* Only the second and fifth data should stay afterwards */
    CORRADE_VERIFY(!animator.isHandleValid(first));
    CORRADE_VERIFY(animator.isHandleValid(second));
    CORRADE_VERIFY(!animator.isHandleValid(third));
    CORRADE_VERIFY(!animator.isHandleValid(fourth));
    CORRADE_VERIFY(animator.isHandleValid(fifth));
    CORRADE_VERIFY(!animator.isHandleValid(sixth));
    CORRADE_VERIFY(!animator.isHandleValid(seventh));

    /* The data attachments should be cleared for removed animations */
    CORRADE_COMPARE_AS(animator.layerData(), Containers::arrayView({
        LayerDataHandle::Null,
        LayerDataHandle::Null,
        LayerDataHandle::Null,
        LayerDataHandle::Null,
        dataFourth,
        LayerDataHandle::Null,
        LayerDataHandle::Null,
    }), TestSuite::Compare::Container);
}

void AbstractAnimatorTest::cleanDataEmpty() {
    /* Like cleanNodesEmpty(), just handling data instead */

    /* Using AbstractGenericAnimator in order to access setLayer(), other than
       that it's testing the base AbstractAnimator APIs */
    struct: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;
        using AbstractGenericAnimator::setLayer;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::DataAttachment;
        }
        void doClean(Containers::BitArrayView) override {
            ++called;
        }
        void doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}

        Int called = 0;
    } animator{animatorHandle(0, 1)};

    /* Has to be called to actually be able to clean the data, but other than
       that doesn't affect the results in any way */
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0xab, 0x12)};
    animator.setLayer(layer);

    /* It should call the implementation even with empty contents */
    animator.cleanData({});
    CORRADE_COMPARE(animator.called, 1);
}

void AbstractAnimatorTest::cleanDataNotImplemented() {
    /* Like cleanNodesNotImplemented(), just handling data instead */

    /* Using AbstractGenericAnimator in order to access setLayer(), other than
       that it's testing the base AbstractAnimator APIs */
    struct: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;
        using AbstractGenericAnimator::setLayer;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::DataAttachment;
        }
        void doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
    } animator{animatorHandle(0, 1)};

    /* Has to be called to actually be able to clean the data, but other than
       that doesn't affect the results in any way */
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0xab, 0x12)};
    animator.setLayer(layer);

    animator.cleanData({});

    /* Shouldn't crash or anything */
    CORRADE_VERIFY(true);
}

void AbstractAnimatorTest::cleanDataInvalidFeatures() {
    /* Like cleanNodesInvalidFeatures(), just handling data instead */

    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;

        AnimatorFeatures doFeatures() const override { return {}; }
    } animator{animatorHandle(0, 1)};

    Containers::String out;
    Error redirectError{&out};
    animator.cleanData({});
    CORRADE_COMPARE(out, "Ui::AbstractAnimator::cleanData(): feature not supported\n");
}

void AbstractAnimatorTest::cleanDataNoLayerSet() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::DataAttachment;
        }
    } animator{animatorHandle(0, 1)};

    Containers::String out;
    Error redirectError{&out};
    animator.cleanData({});
    CORRADE_COMPARE(out, "Ui::AbstractAnimator::cleanData(): no layer set for data attachment\n");
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
    CORRADE_COMPARE(animator.started(handle), 1000_nsec);
    CORRADE_COMPARE(animator.duration(handle), 10_nsec);
    CORRADE_COMPARE(animator.paused(handle), Nanoseconds::max());
    CORRADE_COMPARE(animator.stopped(handle), Nanoseconds::max());

    animator.pause(handle, 1005_nsec);
    CORRADE_COMPARE(animator.started(handle), 1000_nsec);
    CORRADE_COMPARE(animator.duration(handle), 10_nsec);
    CORRADE_COMPARE(animator.paused(handle), 1005_nsec);
    CORRADE_COMPARE(animator.stopped(handle), Nanoseconds::max());

    animator.stop(handle, 1007_nsec);
    /* NeedsAdvance is only reset by advance(), not if any animations get
       stopped */
    CORRADE_COMPARE(animator.started(handle), 1000_nsec);
    CORRADE_COMPARE(animator.duration(handle), 10_nsec);
    CORRADE_COMPARE(animator.paused(handle), 1005_nsec);
    CORRADE_COMPARE(animator.stopped(handle), 1007_nsec);

    animator.play(handle, 400_nsec);
    CORRADE_COMPARE(animator.started(handle), 400_nsec);
    CORRADE_COMPARE(animator.duration(handle), 10_nsec);
    CORRADE_COMPARE(animator.paused(handle), Nanoseconds::max());
    CORRADE_COMPARE(animator.stopped(handle), Nanoseconds::max());

    /* Calling play() with different time restarts from that time */
    animator.play(handle, 700_nsec);
    CORRADE_COMPARE(animator.started(handle), 700_nsec);
    CORRADE_COMPARE(animator.duration(handle), 10_nsec);
    CORRADE_COMPARE(animator.paused(handle), Nanoseconds::max());
    CORRADE_COMPARE(animator.stopped(handle), Nanoseconds::max());

    /* Same even if the time is in the past */
    animator.play(handle, 500_nsec);
    CORRADE_COMPARE(animator.started(handle), 500_nsec);
    CORRADE_COMPARE(animator.duration(handle), 10_nsec);
    CORRADE_COMPARE(animator.paused(handle), Nanoseconds::max());
    CORRADE_COMPARE(animator.stopped(handle), Nanoseconds::max());

    /* Testing also the AnimatorDataHandle overloads */
    animator.pause(animationHandleData(handle), 990_nsec);
    CORRADE_COMPARE(animator.started(handle), 500_nsec);
    CORRADE_COMPARE(animator.duration(handle), 10_nsec);
    CORRADE_COMPARE(animator.paused(handle), 990_nsec);
    CORRADE_COMPARE(animator.stopped(handle), Nanoseconds::max());

    animator.stop(animationHandleData(handle), 550_nsec);
    CORRADE_COMPARE(animator.started(handle), 500_nsec);
    CORRADE_COMPARE(animator.duration(handle), 10_nsec);
    CORRADE_COMPARE(animator.paused(handle), 990_nsec);
    CORRADE_COMPARE(animator.stopped(handle), 550_nsec);

    animator.play(animationHandleData(handle), 400_nsec);
    CORRADE_COMPARE(animator.started(handle), 400_nsec);
    CORRADE_COMPARE(animator.duration(handle), 10_nsec);
    CORRADE_COMPARE(animator.paused(handle), Nanoseconds::max());
    CORRADE_COMPARE(animator.stopped(handle), Nanoseconds::max());
}

void AbstractAnimatorTest::toggleFlagsAtTime() {
    /* Tests behavior of time-taking setFlags() / addFlags() / clearFlags(),
       especially with AnimationFlag::Repeat. The non-time-taking flag APIs are
       tested in properties() instead. */

    struct: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;
        using AbstractAnimator::create;

        AnimatorFeatures doFeatures() const override { return {}; }
    } animator{animatorHandle(0, 1)};

    AnimationHandle start = animator.create(50_nsec, 20_nsec);
    AnimationHandle startRepeat = animator.create(50_nsec, 20_nsec, 2);
    AnimationHandle zeroDuration = animator.create(50_nsec, 0_nsec);
    AnimationHandle quarter = animator.create(35_nsec, 20_nsec, AnimationFlag::Reverse);
    AnimationHandle quarterPaused = animator.create(-25_nsec, 20_nsec);
    animator.pause(quarterPaused, -20_nsec);
    AnimationHandle quarterRepeated = animator.create(-15_nsec, 20_nsec, 0);
    AnimationHandle quarterRepeatedReverseEveryOtherEven = animator.create(-35_nsec, 20_nsec, 0, AnimationFlag::ReverseEveryOther);
    AnimationHandle quarterRepeatedReverseEveryOtherOdd = animator.create(-25_nsec, 20_nsec, 0, AnimationFlag::ReverseEveryOther);
    AnimationHandle middle = animator.create(40_nsec, 20_nsec, AnimationFlag::ReverseEveryOther|AnimationFlag::Reverse);
    AnimationHandle scheduled = animator.create(100_nsec, 10_nsec);
    /* This one is stopped exactly at the point of the update() call */
    AnimationHandle stop = animator.create(30_nsec, 20_nsec);
    /* Also stopped but earlier, not exactly at 50_nsec */
    AnimationHandle stopped = animator.create(-35_nsec, 20_nsec);
    AnimationHandle stoppedReverse = animator.create(-35_nsec, 20_nsec, AnimationFlag::Reverse);

    Containers::BitArray mask{NoInit, 13};
    Containers::StaticArray<13, Float> factors{DirectInit, Constants::nan()};

    /* Initial state. Just as a sanity check, factor() should return the same
       as the result from update(), apart from values update() doesn't touch
       at all. */
    animator.update(50_nsec, mask, mask, mask, factors, mask);
    CORRADE_COMPARE(factors[animationHandleId(start)], 0.0f);
    CORRADE_COMPARE(factors[animationHandleId(startRepeat)], 0.0f);
    CORRADE_COMPARE(factors[animationHandleId(zeroDuration)], 1.0f);
    CORRADE_COMPARE(factors[animationHandleId(quarter)], 0.25f);
    CORRADE_COMPARE(factors[animationHandleId(quarterRepeated)], 0.25f);
    CORRADE_COMPARE(factors[animationHandleId(quarterRepeatedReverseEveryOtherEven)], 0.25f);
    CORRADE_COMPARE(factors[animationHandleId(quarterRepeatedReverseEveryOtherOdd)], 0.25f);
    CORRADE_COMPARE(factors[animationHandleId(quarterPaused)], Constants::nan());
    CORRADE_COMPARE(factors[animationHandleId(middle)], 0.5f);
    CORRADE_COMPARE(factors[animationHandleId(scheduled)], Constants::nan());
    CORRADE_COMPARE(factors[animationHandleId(stop)], 1.0f);
    CORRADE_COMPARE(factors[animationHandleId(stopped)], Constants::nan());
    CORRADE_COMPARE(factors[animationHandleId(stoppedReverse)], Constants::nan());
    CORRADE_COMPARE(animator.factor(start), 0.0f);
    CORRADE_COMPARE(animator.factor(startRepeat), 0.0f);
    CORRADE_COMPARE(animator.factor(zeroDuration), 1.0f);
    CORRADE_COMPARE(animator.factor(quarter), 0.25f);
    CORRADE_COMPARE(animator.factor(quarterRepeated), 0.25f);
    CORRADE_COMPARE(animator.factor(quarterRepeatedReverseEveryOtherEven), 0.25f);
    CORRADE_COMPARE(animator.factor(quarterRepeatedReverseEveryOtherOdd), 0.25f);
    CORRADE_COMPARE(animator.factor(quarterPaused), 0.25f);
    CORRADE_COMPARE(animator.factor(middle), 0.5f);
    CORRADE_COMPARE(animator.factor(scheduled), 0.0f);
    CORRADE_COMPARE(animator.factor(stop), 1.0f);
    CORRADE_COMPARE(animator.factor(stopped), 1.0f);
    CORRADE_COMPARE(animator.factor(stoppedReverse), 0.0f);

    /* Flipping the Reverse flag at current time. Verify that the start time
       adjustment is performed in all variants, independent of other flags
       being present, and the AnimatorDataHandle overloads as well. */
    animator.addFlags(start, AnimationFlag::Reverse, 50_nsec);
    animator.addFlags(startRepeat, AnimationFlag::Reverse|AnimationFlag::KeepOncePlayed, 50_nsec);
    animator.addFlags(animationHandleData(zeroDuration), AnimationFlag::Reverse, 50_nsec);
    animator.clearFlags(quarter, AnimationFlag::Reverse, 50_nsec);
    animator.setFlags(quarterRepeated, AnimationFlag::Reverse|AnimationFlag::KeepOncePlayed, 50_nsec);
    animator.addFlags(quarterRepeatedReverseEveryOtherEven, AnimationFlag::Reverse, 50_nsec);
    animator.addFlags(quarterRepeatedReverseEveryOtherOdd, AnimationFlag::Reverse, 50_nsec);
    animator.setFlags(animationHandleData(quarterPaused), AnimationFlag::Reverse, 50_nsec);
    animator.clearFlags(animationHandleData(middle), AnimationFlag::Reverse, 50_nsec);
    animator.addFlags(scheduled, AnimationFlag::Reverse, 50_nsec);
    animator.addFlags(stop, AnimationFlag::Reverse, 50_nsec);
    animator.addFlags(stopped, AnimationFlag::Reverse, 50_nsec);
    animator.clearFlags(stoppedReverse, AnimationFlag::Reverse, 50_nsec);
    /* The animation that just started is now at the end of its play time (but
       still at the same factor 0), and thus stoppped */
    CORRADE_COMPARE(animator.started(start), 30_nsec);
    CORRADE_COMPARE(animator.state(start), AnimationState::Stopped);
    CORRADE_COMPARE(animator.factor(start), 0.0f);
    /* In comparison, the animation that just started but has an additional
       repeat gets the same start time adjustment but doesn't get stopped */
    CORRADE_COMPARE(animator.started(startRepeat), 30_nsec);
    CORRADE_COMPARE(animator.state(startRepeat), AnimationState::Playing);
    CORRADE_COMPARE(animator.factor(startRepeat), 1.0f);
    /* The zero duration animation gets stopped as well, but now it's at 0.0f
       instead of 1.0f */
    CORRADE_COMPARE(animator.started(zeroDuration), 50_nsec);
    CORRADE_COMPARE(animator.state(zeroDuration), AnimationState::Stopped);
    CORRADE_COMPARE(animator.factor(zeroDuration), 0.0f);
    /* Quarter factor animation stays at the quarter factor, still playing */
    CORRADE_COMPARE(animator.started(quarter), 45_nsec);
    CORRADE_COMPARE(animator.state(quarter), AnimationState::Playing);
    CORRADE_COMPARE(animator.factor(quarter), 0.25f);
    /* With repeat the start time is adjusted just within a single iteration */
    CORRADE_COMPARE(animator.started(quarterRepeated), -25_nsec);
    CORRADE_COMPARE(animator.state(quarterRepeated), AnimationState::Playing);
    CORRADE_COMPARE(animator.factor(quarterRepeated), 0.25f);
    /* ReverseEveryOther being enabled doesn't have any effect on the
       adjustment */
    CORRADE_COMPARE(animator.started(quarterRepeatedReverseEveryOtherEven), -45_nsec);
    CORRADE_COMPARE(animator.state(quarterRepeatedReverseEveryOtherEven), AnimationState::Playing);
    CORRADE_COMPARE(animator.factor(quarterRepeatedReverseEveryOtherEven), 0.25f);
    CORRADE_COMPARE(animator.started(quarterRepeatedReverseEveryOtherOdd), -15_nsec);
    CORRADE_COMPARE(animator.state(quarterRepeatedReverseEveryOtherOdd), AnimationState::Playing);
    CORRADE_COMPARE(animator.factor(quarterRepeatedReverseEveryOtherOdd), 0.25f);
    /* Pause is still paused, just adjusted to be still at a quarter */
    CORRADE_COMPARE(animator.started(quarterPaused), -35_nsec);
    CORRADE_COMPARE(animator.state(quarterPaused), AnimationState::Paused);
    CORRADE_COMPARE(animator.factor(quarterPaused), 0.25f);
    /* Middle stays at the same start time as before */
    CORRADE_COMPARE(animator.started(middle), 40_nsec); /* as before */
    CORRADE_COMPARE(animator.state(middle), AnimationState::Playing);
    CORRADE_COMPARE(animator.factor(middle), 0.5f);
    /* For Scheduled there is no change in start time, state or factor */
    CORRADE_COMPARE(animator.started(scheduled), 100_nsec);
    CORRADE_COMPARE(animator.state(scheduled), AnimationState::Scheduled);
    CORRADE_COMPARE(animator.factor(scheduled), 0.0f);
    /* For Stopped the start time or state doesn't change but factor flips
       between 0 and 1. This is the same behavior as if changing the flags
       directly without time adjustment. */
    CORRADE_COMPARE(animator.started(stop), 30_nsec);
    CORRADE_COMPARE(animator.state(stop), AnimationState::Stopped);
    CORRADE_COMPARE(animator.factor(stop), 0.0f); /* was 1 before */
    CORRADE_COMPARE(animator.started(stopped), -35_nsec);
    CORRADE_COMPARE(animator.state(stopped), AnimationState::Stopped);
    CORRADE_COMPARE(animator.factor(stopped), 0.0f); /* was 1 before */
    CORRADE_COMPARE(animator.started(stoppedReverse), -35_nsec);
    CORRADE_COMPARE(animator.state(stoppedReverse), AnimationState::Stopped);
    CORRADE_COMPARE(animator.factor(stoppedReverse), 1.0f); /* was 0 before */

    /* Flipping the Reverse for playing and paused animations goes back to the
       original time specified at creation */
    animator.clearFlags(start, AnimationFlag::Reverse, 50_nsec);
    animator.clearFlags(startRepeat, AnimationFlag::Reverse, 50_nsec);
    animator.clearFlags(zeroDuration, AnimationFlag::Reverse, 50_nsec);
    animator.addFlags(quarter, AnimationFlag::Reverse, 50_nsec);
    animator.clearFlags(quarterPaused, AnimationFlag::Reverse, 50_nsec);
    animator.clearFlags(quarterRepeated, AnimationFlag::Reverse, 50_nsec);
    animator.clearFlags(quarterRepeatedReverseEveryOtherEven, AnimationFlag::Reverse, 50_nsec);
    animator.clearFlags(quarterRepeatedReverseEveryOtherOdd, AnimationFlag::Reverse, 50_nsec);
    animator.addFlags(middle, AnimationFlag::Reverse, 50_nsec);
    /* The originally starting animation transitioned to Stopped so it doesn't
       change anymore besides the factor flipping back to 1 */
    CORRADE_COMPARE(animator.started(start), 30_nsec);
    CORRADE_COMPARE(animator.state(start), AnimationState::Stopped);
    CORRADE_COMPARE(animator.factor(start), 1.0f);
    /* In this case, because there's another repeat iteration after, it doesn't
       stop but goes back. For this there's a special case in the code to
       ensure it properly roundtrips instead of the start time going to 10_nsec
       and causing the animation to stop as well. */
    CORRADE_COMPARE(animator.started(startRepeat), 50_nsec);
    CORRADE_COMPARE(animator.state(startRepeat), AnimationState::Playing);
    CORRADE_COMPARE(animator.factor(startRepeat), 0.0f);
    /* The zero-duration is also Stopped and doesn't change besides the factor
       flipping back to 1 */
    CORRADE_COMPARE(animator.started(zeroDuration), 50_nsec);
    CORRADE_COMPARE(animator.state(zeroDuration), AnimationState::Stopped);
    CORRADE_COMPARE(animator.factor(zeroDuration), 1.0f);
    /* These match what was passed to create() */
    CORRADE_COMPARE(animator.started(quarter), 35_nsec);
    CORRADE_COMPARE(animator.state(quarter), AnimationState::Playing);
    CORRADE_COMPARE(animator.factor(quarter), 0.25f);
    CORRADE_COMPARE(animator.started(quarterPaused), -25_nsec);
    CORRADE_COMPARE(animator.state(quarterPaused), AnimationState::Paused);
    CORRADE_COMPARE(animator.factor(quarterPaused), 0.25f);
    CORRADE_COMPARE(animator.started(quarterRepeated), -15_nsec);
    CORRADE_COMPARE(animator.state(quarterRepeated), AnimationState::Playing);
    CORRADE_COMPARE(animator.factor(quarterRepeated), 0.25f);
    CORRADE_COMPARE(animator.started(quarterRepeatedReverseEveryOtherEven), -35_nsec);
    CORRADE_COMPARE(animator.state(quarterRepeatedReverseEveryOtherEven), AnimationState::Playing);
    CORRADE_COMPARE(animator.factor(quarterRepeatedReverseEveryOtherEven), 0.25f);
    CORRADE_COMPARE(animator.started(quarterRepeatedReverseEveryOtherOdd), -25_nsec);
    CORRADE_COMPARE(animator.state(quarterRepeatedReverseEveryOtherOdd), AnimationState::Playing);
    CORRADE_COMPARE(animator.factor(quarterRepeatedReverseEveryOtherOdd), 0.25f);
    CORRADE_COMPARE(animator.started(middle), 40_nsec);
    CORRADE_COMPARE(animator.state(middle), AnimationState::Playing);
    CORRADE_COMPARE(animator.factor(middle), 0.5f);
    /* The remaining scheduled and stopped animations aren't tested anymore as
       they don't change */

    /* Flipping the ReverseEveryOther flag (currently?) doesn't lead to any
       time adjustment so the factor jumps (unless we're at an even iteration).
       I.e., the behavior would be the same as if changing the flag directly. */
    animator.clearFlags(quarterRepeatedReverseEveryOtherEven, AnimationFlag::ReverseEveryOther, 50_nsec);
    animator.clearFlags(quarterRepeatedReverseEveryOtherOdd, AnimationFlag::ReverseEveryOther, 50_nsec);
    CORRADE_COMPARE(animator.started(quarterRepeatedReverseEveryOtherEven), -35_nsec);
    CORRADE_COMPARE(animator.state(quarterRepeatedReverseEveryOtherEven), AnimationState::Playing);
    CORRADE_COMPARE(animator.factor(quarterRepeatedReverseEveryOtherEven), 0.25f);
    CORRADE_COMPARE(animator.started(quarterRepeatedReverseEveryOtherOdd), -25_nsec);
    CORRADE_COMPARE(animator.state(quarterRepeatedReverseEveryOtherOdd), AnimationState::Playing);
    CORRADE_COMPARE(animator.factor(quarterRepeatedReverseEveryOtherOdd), 0.75f);

    /* Flipping other flags doesn't do any adjustment */
    animator.addFlags(quarter, AnimationFlag::KeepOncePlayed, 50_nsec);
    CORRADE_COMPARE(animator.started(quarter), 35_nsec);
    CORRADE_COMPARE(animator.state(quarter), AnimationState::Playing);
    CORRADE_COMPARE(animator.factor(quarter), 0.25f);

    /* Setting or clearing a flag that's already there doesn't do anything */
    CORRADE_COMPARE(animator.started(quarter), 35_nsec);
    CORRADE_COMPARE(animator.flags(quarter), AnimationFlag::KeepOncePlayed|AnimationFlag::Reverse);
    animator.addFlags(quarter, AnimationFlag::Reverse, 50_nsec);
    CORRADE_COMPARE(animator.started(quarter), 35_nsec);

    CORRADE_COMPARE(animator.started(quarterRepeated), -15_nsec);
    CORRADE_COMPARE(animator.flags(quarterRepeated), AnimationFlag::KeepOncePlayed);
    animator.clearFlags(quarterRepeated, AnimationFlag::Reverse, 50_nsec);
    CORRADE_COMPARE(animator.started(quarterRepeated), -15_nsec);

    /* Flipping at a time different from the time at last update will make it
       so the factor stays the same when it reaches given time. In this case,
       the `quarter` reaches a factor of 0.15 at 52 nsec (so -0.2), so when
       going back from there to 50 nsec it'd be 0.05. */
    animator.clearFlags(quarter, AnimationFlag::Reverse, 52_nsec);
    CORRADE_COMPARE(animator.started(quarter), 49_nsec);
    CORRADE_COMPARE(animator.state(quarter), AnimationState::Playing);
    CORRADE_COMPARE(animator.factor(quarter), 0.05f);
    animator.update(52_nsec, mask, mask, mask, factors, mask);
    CORRADE_COMPARE(animator.factor(quarter), 0.15f);
}

void AbstractAnimatorTest::playPauseStopToggleFlagsInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;
        using AbstractAnimator::create;

        AnimatorFeatures doFeatures() const override { return {}; }
    } animator{animatorHandle(0, 1)};

    AnimationHandle handle = animator.create(12_nsec, 13_nsec);

    Containers::String out;
    Error redirectError{&out};
    animator.play(AnimationHandle::Null, 0_nsec);
    animator.pause(AnimationHandle::Null, 0_nsec);
    animator.stop(AnimationHandle::Null, 0_nsec);
    animator.setFlags(AnimationHandle::Null, {}, 0_nsec);
    animator.addFlags(AnimationHandle::Null, {}, 0_nsec);
    animator.clearFlags(AnimationHandle::Null, {}, 0_nsec);
    /* Valid animator, invalid data */
    animator.play(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)), 0_nsec);
    animator.pause(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)), 0_nsec);
    animator.stop(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)), 0_nsec);
    animator.setFlags(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)), {}, 0_nsec);
    animator.addFlags(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)), {}, 0_nsec);
    animator.clearFlags(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)), {}, 0_nsec);
    /* Invalid animator, valid data */
    animator.play(animationHandle(AnimatorHandle::Null, animationHandleData(handle)), 0_nsec);
    animator.pause(animationHandle(AnimatorHandle::Null, animationHandleData(handle)), 0_nsec);
    animator.stop(animationHandle(AnimatorHandle::Null, animationHandleData(handle)), 0_nsec);
    animator.setFlags(animationHandle(AnimatorHandle::Null, animationHandleData(handle)), {}, 0_nsec);
    animator.addFlags(animationHandle(AnimatorHandle::Null, animationHandleData(handle)), {}, 0_nsec);
    animator.clearFlags(animationHandle(AnimatorHandle::Null, animationHandleData(handle)), {}, 0_nsec);
    /* AnimatorDataHandle directly */
    animator.play(AnimatorDataHandle(0x123abcde), 0_nsec);
    animator.pause(AnimatorDataHandle(0x123abcde), 0_nsec);
    animator.stop(AnimatorDataHandle(0x123abcde), 0_nsec);
    animator.setFlags(AnimatorDataHandle(0x123abcde), {}, 0_nsec);
    animator.addFlags(AnimatorDataHandle(0x123abcde), {}, 0_nsec);
    animator.clearFlags(AnimatorDataHandle(0x123abcde), {}, 0_nsec);
    CORRADE_COMPARE_AS(out,
        "Ui::AbstractAnimator::play(): invalid handle Ui::AnimationHandle::Null\n"
        "Ui::AbstractAnimator::pause(): invalid handle Ui::AnimationHandle::Null\n"
        "Ui::AbstractAnimator::stop(): invalid handle Ui::AnimationHandle::Null\n"
        "Ui::AbstractAnimator::setFlags(): invalid handle Ui::AnimationHandle::Null\n"
        "Ui::AbstractAnimator::addFlags(): invalid handle Ui::AnimationHandle::Null\n"
        "Ui::AbstractAnimator::clearFlags(): invalid handle Ui::AnimationHandle::Null\n"

        "Ui::AbstractAnimator::play(): invalid handle Ui::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Ui::AbstractAnimator::pause(): invalid handle Ui::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Ui::AbstractAnimator::stop(): invalid handle Ui::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Ui::AbstractAnimator::setFlags(): invalid handle Ui::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Ui::AbstractAnimator::addFlags(): invalid handle Ui::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Ui::AbstractAnimator::clearFlags(): invalid handle Ui::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"

        "Ui::AbstractAnimator::play(): invalid handle Ui::AnimationHandle(Null, {0x0, 0x1})\n"
        "Ui::AbstractAnimator::pause(): invalid handle Ui::AnimationHandle(Null, {0x0, 0x1})\n"
        "Ui::AbstractAnimator::stop(): invalid handle Ui::AnimationHandle(Null, {0x0, 0x1})\n"
        "Ui::AbstractAnimator::setFlags(): invalid handle Ui::AnimationHandle(Null, {0x0, 0x1})\n"
        "Ui::AbstractAnimator::addFlags(): invalid handle Ui::AnimationHandle(Null, {0x0, 0x1})\n"
        "Ui::AbstractAnimator::clearFlags(): invalid handle Ui::AnimationHandle(Null, {0x0, 0x1})\n"

        "Ui::AbstractAnimator::play(): invalid handle Ui::AnimatorDataHandle(0xabcde, 0x123)\n"
        "Ui::AbstractAnimator::pause(): invalid handle Ui::AnimatorDataHandle(0xabcde, 0x123)\n"
        "Ui::AbstractAnimator::stop(): invalid handle Ui::AnimatorDataHandle(0xabcde, 0x123)\n"
        "Ui::AbstractAnimator::setFlags(): invalid handle Ui::AnimatorDataHandle(0xabcde, 0x123)\n"
        "Ui::AbstractAnimator::addFlags(): invalid handle Ui::AnimatorDataHandle(0xabcde, 0x123)\n"
        "Ui::AbstractAnimator::clearFlags(): invalid handle Ui::AnimatorDataHandle(0xabcde, 0x123)\n",
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
    CORRADE_COMPARE(animator.started(handle), 10_nsec);
    CORRADE_COMPARE(animator.duration(handle), 100_nsec);
    CORRADE_COMPARE(animator.paused(handle), Nanoseconds::max());
    CORRADE_COMPARE(animator.stopped(handle), Nanoseconds::max());

    /* Stopping the animation shouldn't affect anything here */
    if(data.stopped)
        animator.stop(handle, *data.stopped);

    /* Pausing only records how long the animation have been playing, doesn't
       touch anything else */
    animator.pause(handle, data.paused);
    CORRADE_COMPARE(animator.started(handle), 10_nsec);
    CORRADE_COMPARE(animator.duration(handle), 100_nsec);
    CORRADE_COMPARE(animator.paused(handle), data.paused);
    CORRADE_COMPARE(animator.stopped(handle), data.stopped ? *data.stopped : Nanoseconds::max());

    /* Playing either adjusts the started time to resume from where it was
       paused, or plays from the start. The paused and stopped time gets reset
       always, unconditionally. */
    animator.play(handle, data.resumed);
    CORRADE_COMPARE(animator.started(handle), data.expectedPlayed);
    CORRADE_COMPARE(animator.duration(handle), 100_nsec);
    CORRADE_COMPARE(animator.paused(handle), Nanoseconds::max());
    CORRADE_COMPARE(animator.stopped(handle), Nanoseconds::max());
}

void AbstractAnimatorTest::update() {
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
    AnimationHandle scheduledToPlayingReverse = animator.create(5_nsec, 10_nsec, AnimationFlag::Reverse);
    AnimationHandle scheduledToPaused = animator.create(5_nsec, 10_nsec);
    AnimationHandle scheduledToStopped = animator.create(5_nsec, 10_nsec);
    AnimationHandle removed = animator.create(0_nsec, 6_nsec);
    AnimationHandle playingMiddleKeep = animator.create(-20_nsec, 40_nsec, AnimationFlag::KeepOncePlayed);
    AnimationHandle playingToPausedKeep = animator.create(-10_nsec, 20_nsec, AnimationFlag::KeepOncePlayed);
    AnimationHandle playingEndToStopped = animator.create(0_nsec, 10_nsec);
    AnimationHandle playingToStoppedKeep = animator.create(0_nsec, 5_nsec, AnimationFlag::KeepOncePlayed);
    AnimationHandle playingRepeated = animator.create(-4_nsec, 10_nsec, 3);
    AnimationHandle playingRepeatedReverseReverseEveryOther = animator.create(-3_nsec, 10_nsec, 3, AnimationFlag::Reverse|AnimationFlag::ReverseEveryOther);
    AnimationHandle paused = animator.create(-40_nsec, 10_nsec);
    AnimationHandle pausedToStopped = animator.create(-40_nsec, 10_nsec);
    AnimationHandle stoppedRemove = animator.create(-40_nsec, 30_nsec);
    AnimationHandle stoppedKeep = animator.create(-40_nsec, 30_nsec, AnimationFlag::KeepOncePlayed);
    AnimationHandle zeroDurationScheduled = animator.create(20_nsec, 0_nsec);
    AnimationHandle zeroDurationScheduledReverseKeep = animator.create(20_nsec, 0_nsec, AnimationFlag::Reverse|AnimationFlag::KeepOncePlayed);
    AnimationHandle zeroDurationStopped = animator.create(-20_nsec, 0_nsec);
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
    CORRADE_COMPARE(animator.state(scheduledToPlayingReverse), AnimationState::Scheduled);
    CORRADE_COMPARE(animator.state(scheduledToPaused), AnimationState::Scheduled);
    CORRADE_COMPARE(animator.state(scheduledToStopped), AnimationState::Scheduled);
    /* removed is gone */
    CORRADE_COMPARE(animator.state(playingMiddleKeep), AnimationState::Playing);
    CORRADE_COMPARE(animator.state(playingToPausedKeep), AnimationState::Playing);
    /* playingEndToStopped is gone */
    CORRADE_COMPARE(animator.state(playingEndToStopped), AnimationState::Playing);
    CORRADE_COMPARE(animator.state(playingToStoppedKeep), AnimationState::Playing);
    CORRADE_COMPARE(animator.state(playingRepeated), AnimationState::Playing);
    CORRADE_COMPARE(animator.state(playingRepeatedReverseReverseEveryOther), AnimationState::Playing);
    CORRADE_COMPARE(animator.state(paused), AnimationState::Paused);
    CORRADE_COMPARE(animator.state(pausedToStopped), AnimationState::Paused);
    CORRADE_COMPARE(animator.state(stoppedRemove), AnimationState::Stopped);
    CORRADE_COMPARE(animator.state(stoppedKeep), AnimationState::Stopped);
    CORRADE_COMPARE(animator.state(zeroDurationScheduled), AnimationState::Scheduled);
    CORRADE_COMPARE(animator.state(zeroDurationScheduledReverseKeep), AnimationState::Scheduled);
    CORRADE_COMPARE(animator.state(zeroDurationStopped), AnimationState::Stopped);

    constexpr Float unused = Constants::inf();
    constexpr std::size_t animationCount = 19;

    /* Call to update(10) advances also stopped and paused animations that
       changed their state compared to last time (i.e., time 0) */
    {
        Containers::BitArray active{NoInit, animationCount};
        Containers::BitArray started{NoInit, animationCount};
        Containers::BitArray stopped{NoInit, animationCount};
        Containers::StaticArray<animationCount, Float> factors{DirectInit, unused};
        Containers::BitArray remove{NoInit, animationCount};
        CORRADE_COMPARE(animator.update(10_nsec, active, started, stopped, factors, remove), Containers::pair(true, true));
        CORRADE_COMPARE_AS(Containers::BitArrayView{active}, Containers::stridedArrayView({
            false,  /*  0 scheduledKeep */
            true,   /*  1 scheduledToPlayingBegin */
            true,   /*  2 scheduledToPlayingReverse */
            true,   /*  3 scheduledToPaused */
            true,   /*  4 scheduledToStopped */
            false,  /*  5 removed */
            true,   /*  6 playingMiddleKeep */
            true,   /*  7 playingToPausedKeep */
            true,   /*  8 playingEndToStopped */
            true,   /*  9 playingToStoppedKeep */
            true,   /* 10 playingRepeated */
            true,   /* 11 playingRepeatedReverseReverseEveryOther */
            false,  /* 12 paused */
            true,   /* 13 pausedToStopped */
            false,  /* 14 stoppedRemove */
            false,  /* 15 stoppedKeep */
            false,  /* 16 zeroDurationScheduled */
            false,  /* 17 zeroDurationScheduledReverseKeep */
            false,  /* 18 zeroDurationStopped */
        }).sliceBit(0), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(Containers::BitArrayView{started}, Containers::stridedArrayView({
            false,  /*  0 scheduledKeep */
            true,   /*  1 scheduledToPlayingBegin */
            true,   /*  2 scheduledToPlayingReverse */
            true,   /*  3 scheduledToPaused */
            true,   /*  4 scheduledToStopped */
            false,  /*  5 removed */
            false,  /*  6 playingMiddleKeep */
            false,  /*  7 playingToPausedKeep */
            false,  /*  8 playingEndToStopped */
            false,  /*  9 playingToStoppedKeep */
            false,  /* 10 playingRepeated */
            false,  /* 11 playingRepeatedReverseReverseEveryOther */
            false,  /* 12 paused */
            false,  /* 13 pausedToStopped */
            false,  /* 14 stoppedRemove */
            false,  /* 15 stoppedKeep */
            false,  /* 16 zeroDurationScheduled */
            false,  /* 17 zeroDurationScheduledReverseKeep */
            false,  /* 18 zeroDurationStopped */
        }).sliceBit(0), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(Containers::BitArrayView{stopped}, Containers::stridedArrayView({
            false,  /*  0 scheduledKeep */
            false,  /*  1 scheduledToPlayingBegin */
            false,  /*  2 scheduledToPlayingReverse */
            false,  /*  3 scheduledToPaused */
            true,   /*  4 scheduledToStopped */
            false,  /*  5 removed */
            false,  /*  6 playingMiddleKeep */
            false,  /*  7 playingToPausedKeep */
            true,   /*  8 playingEndToStopped */
            true,   /*  9 playingToStoppedKeep */
            false,  /* 10 playingRepeated */
            false,  /* 11 playingRepeatedReverseReverseEveryOther */
            false,  /* 12 paused */
            true,   /* 13 pausedToStopped */
            false,  /* 14 stoppedRemove */
            false,  /* 15 stoppedKeep */
            false,  /* 16 zeroDurationScheduled */
            false,  /* 17 zeroDurationScheduledReverseKeep */
            false,  /* 18 zeroDurationStopped */
        }).sliceBit(0), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(factors, Containers::arrayView({
            unused, /*  0 scheduledKeep */
            0.0f,   /*  1 scheduledToPlayingBegin */
            0.5f,   /*  2 scheduledToPlayingReverse */
            0.3f,   /*  3 scheduledToPaused */
            1.0f,   /*  4 scheduledToStopped */
            unused, /*  5 removed */
            0.75f,  /*  6 playingMiddleKeep */
            0.75f,  /*  7 playingToPausedKeep */
            1.0f,   /*  8 playingEndToStopped */
            1.0f,   /*  9 playingToStoppedKeep */
            0.4f,   /* 10 playingRepeated */
            0.3f,   /* 11 playingRepeatedReverseReverseEveryOther */
            unused, /* 12 paused */
            1.0f,   /* 13 pausedToStopped */
            unused, /* 14 stoppedRemove */
            unused, /* 15 stoppedKeep */
            unused, /* 16 zeroDurationScheduled */
            unused, /* 17 zeroDurationScheduledReverseKeep */
            unused, /* 18 zeroDurationStopped */
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(Containers::BitArrayView{remove}, Containers::stridedArrayView({
            false,  /*  0 scheduledKeep */
            false,  /*  1 scheduledToPlayingBegin */
            false,  /*  2 scheduledToPlayingReverse */
            false,  /*  3 scheduledToPaused */
            true,   /*  4 scheduledToStopped */
            false,  /*  5 removed */
            false,  /*  6 playingMiddleKeep */
            false,  /*  7 playingToPausedKeep */
            true,   /*  8 playingEndToStopped */
            false,  /*  9 playingToStoppedKeep */
            false,  /* 10 playingRepeated */
            false,  /* 11 playingRepeatedReverseReverseEveryOther */
            false,  /* 12 paused */
            true,   /* 13 pausedToStopped */
            true,   /* 14 stoppedRemove */
            false,  /* 15 stoppedKeep */
            false,  /* 16 zeroDurationScheduled */
            false,  /* 17 zeroDurationScheduledReverseKeep */
            true,   /* 18 zeroDurationStopped */
        }).sliceBit(0), TestSuite::Compare::Container);

        /* Need to call this ourselves to not have the removed animations
           picked up again next time */
        animator.clean(remove);
    }
    CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);

    /* All animations that stopped and aren't KeepOncePlayed are removed now */
    CORRADE_VERIFY(animator.isHandleValid(scheduledKeep));
    CORRADE_VERIFY(animator.isHandleValid(scheduledToPlayingBegin));
    CORRADE_VERIFY(animator.isHandleValid(scheduledToPlayingReverse));
    CORRADE_VERIFY(animator.isHandleValid(scheduledToPaused));
    CORRADE_VERIFY(!animator.isHandleValid(scheduledToStopped));
    CORRADE_VERIFY(!animator.isHandleValid(removed));
    CORRADE_VERIFY(animator.isHandleValid(playingMiddleKeep));
    CORRADE_VERIFY(animator.isHandleValid(playingToPausedKeep));
    CORRADE_VERIFY(!animator.isHandleValid(playingEndToStopped));
    CORRADE_VERIFY(animator.isHandleValid(playingToStoppedKeep));
    CORRADE_VERIFY(animator.isHandleValid(playingRepeated));
    CORRADE_VERIFY(animator.isHandleValid(playingRepeatedReverseReverseEveryOther));
    CORRADE_VERIFY(animator.isHandleValid(paused));
    CORRADE_VERIFY(!animator.isHandleValid(pausedToStopped));
    CORRADE_VERIFY(!animator.isHandleValid(stoppedRemove));
    CORRADE_VERIFY(animator.isHandleValid(stoppedKeep));
    CORRADE_VERIFY(animator.isHandleValid(zeroDurationScheduled));
    CORRADE_VERIFY(animator.isHandleValid(zeroDurationScheduledReverseKeep));
    CORRADE_VERIFY(!animator.isHandleValid(zeroDurationStopped));

    CORRADE_COMPARE(animator.state(scheduledKeep), AnimationState::Scheduled);
    CORRADE_COMPARE(animator.state(scheduledToPlayingBegin), AnimationState::Playing);
    CORRADE_COMPARE(animator.state(scheduledToPlayingReverse), AnimationState::Playing);
    CORRADE_COMPARE(animator.state(scheduledToPaused), AnimationState::Paused);
    /* scheduledToStopped is gone */
    /* removed is gone */
    CORRADE_COMPARE(animator.state(playingMiddleKeep), AnimationState::Playing);
    CORRADE_COMPARE(animator.state(playingToPausedKeep), AnimationState::Paused);
    /* playingEndToStopped is gone */
    CORRADE_COMPARE(animator.state(playingToStoppedKeep), AnimationState::Stopped);
    CORRADE_COMPARE(animator.state(playingRepeated), AnimationState::Playing);
    CORRADE_COMPARE(animator.state(playingRepeatedReverseReverseEveryOther), AnimationState::Playing);
    CORRADE_COMPARE(animator.state(paused), AnimationState::Paused);
    /* pausedToStopped is gone */
    /* stoppedRemove is gone */
    CORRADE_COMPARE(animator.state(stoppedKeep), AnimationState::Stopped);
    CORRADE_COMPARE(animator.state(zeroDurationScheduled), AnimationState::Scheduled);
    CORRADE_COMPARE(animator.state(zeroDurationScheduledReverseKeep), AnimationState::Scheduled);
    /* zeroDurationStopped is gone */

    /* Call to update(10) again marks only the currently playing animations as
       active. As there's no difference in current and previous state and all
       stopped animations got already removed, started and stoppped are all 0s
       and clean() isn't meant to be called. */
    {
        Containers::BitArray active{NoInit, animationCount};
        Containers::BitArray started{NoInit, animationCount};
        Containers::BitArray stopped{NoInit, animationCount};
        Containers::StaticArray<animationCount, Float> factors{DirectInit, unused};
        Containers::BitArray remove{NoInit, animationCount};
        CORRADE_COMPARE(animator.update(10_nsec, active, started, stopped, factors, remove), Containers::pair(true, false));
        CORRADE_COMPARE_AS(Containers::BitArrayView{active}, Containers::stridedArrayView({
            false,  /*  0 scheduledKeep */
            true,   /*  1 scheduledToPlayingBegin */
            true,   /*  2 scheduledToPlayingReverse */
            false,  /*  3 scheduledToPaused */
            false,  /*  4 scheduledToStopped */
            false,  /*  5 removed */
            true,   /*  6 playingMiddleKeep */
            false,  /*  7 playingToPausedKeep */
            false,  /*  8 playingEndToStopped */
            false,  /*  9 playingToStoppedKeep */
            true,   /* 10 playingRepeated */
            true,   /* 11 playingRepeatedReverseReverseEveryOther */
            false,  /* 12 paused */
            false,  /* 13 pausedToStopped */
            false,  /* 14 stoppedRemove */
            false,  /* 15 stoppedKeep */
            false,  /* 16 zeroDurationScheduled */
            false,  /* 17 zeroDurationScheduledReverseKeep */
            false,  /* 18 zeroDurationStopped */
        }).sliceBit(0), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(Containers::BitArrayView{started}, Containers::stridedArrayView({
            false,  /*  0 scheduledKeep */
            false,  /*  1 scheduledToPlayingBegin */
            false,  /*  2 scheduledToPlayingReverse */
            false,  /*  3 scheduledToPaused */
            false,  /*  4 scheduledToStopped */
            false,  /*  5 removed */
            false,  /*  6 playingMiddleKeep */
            false,  /*  7 playingToPausedKeep */
            false,  /*  8 playingEndToStopped */
            false,  /*  9 playingToStoppedKeep */
            false,  /* 10 playingRepeated */
            false,  /* 11 playingRepeatedReverseReverseEveryOther */
            false,  /* 12 paused */
            false,  /* 13 pausedToStopped */
            false,  /* 14 stoppedRemove */
            false,  /* 15 stoppedKeep */
            false,  /* 16 zeroDurationScheduled */
            false,  /* 17 zeroDurationScheduledReverseKeep */
            false,  /* 18 zeroDurationStopped */
        }).sliceBit(0), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(Containers::BitArrayView{stopped}, Containers::stridedArrayView({
            false,  /*  0 scheduledKeep */
            false,  /*  1 scheduledToPlayingBegin */
            false,  /*  2 scheduledToPlayingReverse */
            false,  /*  3 scheduledToPaused */
            false,  /*  4 scheduledToStopped */
            false,  /*  5 removed */
            false,  /*  6 playingMiddleKeep */
            false,  /*  7 playingToPausedKeep */
            false,  /*  8 playingEndToStopped */
            false,  /*  9 playingToStoppedKeep */
            false,  /* 10 playingRepeated */
            false,  /* 11 playingRepeatedReverseReverseEveryOther */
            false,  /* 12 paused */
            false,  /* 13 pausedToStopped */
            false,  /* 14 stoppedRemove */
            false,  /* 15 stoppedKeep */
            false,  /* 16 zeroDurationScheduled */
            false,  /* 17 zeroDurationScheduledReverseKeep */
            false,  /* 18 zeroDurationStopped */
        }).sliceBit(0), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(factors, Containers::arrayView({
            unused, /*  0 scheduledKeep */
            0.0f,   /*  1 scheduledToPlayingBegin */
            0.5f,   /*  2 scheduledToPlayingReverse */
            unused, /*  3 scheduledToPaused */
            unused, /*  4 scheduledToStopped */
            unused, /*  5 removed */
            0.75f,  /*  6 playingMiddleKeep */
            unused, /*  7 playingToPausedKeep */
            unused, /*  8 playingEndToStopped */
            unused, /*  9 playingToStoppedKeep */
            0.4f,   /* 10 playingRepeated */
            0.3f,   /* 11 playingRepeatedReverseReverseEveryOther */
            unused, /* 12 paused */
            unused, /* 13 pausedToStopped */
            unused, /* 14 stoppedRemove */
            unused, /* 15 stoppedKeep */
            unused, /* 16 zeroDurationScheduled */
            unused, /* 17 zeroDurationScheduledReverseKeep */
            unused, /* 18 zeroDurationStopped */
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(remove,
            (Containers::BitArray{DirectInit, animationCount, false}),
            TestSuite::Compare::Container);

        /* Need to call this ourselves to not have the removed animations
           picked up again next time */
        animator.clean(remove);
    }
    CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);

    /* Same as before */
    CORRADE_VERIFY(animator.isHandleValid(scheduledKeep));
    CORRADE_VERIFY(animator.isHandleValid(scheduledToPlayingBegin));
    CORRADE_VERIFY(animator.isHandleValid(scheduledToPlayingReverse));
    CORRADE_VERIFY(animator.isHandleValid(scheduledToPaused));
    CORRADE_VERIFY(!animator.isHandleValid(scheduledToStopped));
    CORRADE_VERIFY(!animator.isHandleValid(removed));
    CORRADE_VERIFY(animator.isHandleValid(playingMiddleKeep));
    CORRADE_VERIFY(animator.isHandleValid(playingToPausedKeep));
    CORRADE_VERIFY(!animator.isHandleValid(playingEndToStopped));
    CORRADE_VERIFY(animator.isHandleValid(playingToStoppedKeep));
    CORRADE_VERIFY(animator.isHandleValid(playingRepeated));
    CORRADE_VERIFY(animator.isHandleValid(playingRepeatedReverseReverseEveryOther));
    CORRADE_VERIFY(animator.isHandleValid(paused));
    CORRADE_VERIFY(!animator.isHandleValid(pausedToStopped));
    CORRADE_VERIFY(!animator.isHandleValid(stoppedRemove));
    CORRADE_VERIFY(animator.isHandleValid(stoppedKeep));
    CORRADE_VERIFY(animator.isHandleValid(zeroDurationScheduled));
    CORRADE_VERIFY(animator.isHandleValid(zeroDurationScheduledReverseKeep));
    CORRADE_VERIFY(!animator.isHandleValid(zeroDurationStopped));

    /* Same as before */
    CORRADE_COMPARE(animator.state(scheduledKeep), AnimationState::Scheduled);
    CORRADE_COMPARE(animator.state(scheduledToPlayingBegin), AnimationState::Playing);
    CORRADE_COMPARE(animator.state(scheduledToPlayingReverse), AnimationState::Playing);
    CORRADE_COMPARE(animator.state(scheduledToPaused), AnimationState::Paused);
    /* scheduledToStopped is gone */
    /* removed is gone */
    CORRADE_COMPARE(animator.state(playingMiddleKeep), AnimationState::Playing);
    CORRADE_COMPARE(animator.state(playingToPausedKeep), AnimationState::Paused);
    /* playingEndToStopped is gone */
    CORRADE_COMPARE(animator.state(playingToStoppedKeep), AnimationState::Stopped);
    CORRADE_COMPARE(animator.state(playingRepeated), AnimationState::Playing);
    CORRADE_COMPARE(animator.state(playingRepeatedReverseReverseEveryOther), AnimationState::Playing);
    CORRADE_COMPARE(animator.state(paused), AnimationState::Paused);
    /* pausedToStopped is gone */
    /* stoppedRemove is gone */
    CORRADE_COMPARE(animator.state(stoppedKeep), AnimationState::Stopped);
    CORRADE_COMPARE(animator.state(zeroDurationScheduled), AnimationState::Scheduled);
    CORRADE_COMPARE(animator.state(zeroDurationScheduledReverseKeep), AnimationState::Scheduled);
    /* zeroDurationStopped is gone */

    /* Call at 20 advances animations that weren't stopped and paused before as
       well. There's just zeroDurationScheduled now being active that wasn't
       before, and it's both started, stopped and removed in this frame. The
       playingRepeated* do *not* get the started / stopped bits set for
       successive iterations. */
    {
        Containers::BitArray active{NoInit, animationCount};
        Containers::BitArray started{NoInit, animationCount};
        Containers::BitArray stopped{NoInit, animationCount};
        Containers::StaticArray<animationCount, Float> factors{DirectInit, unused};
        Containers::BitArray remove{NoInit, animationCount};
        CORRADE_COMPARE(animator.update(20_nsec, active, started, stopped, factors, remove), Containers::pair(true, true));
        CORRADE_COMPARE_AS(Containers::BitArrayView{active}, Containers::stridedArrayView({
            false,  /*  0 scheduledKeep */
            true,   /*  1 scheduledToPlayingBegin */
            true,   /*  2 scheduledToPlayingReverse */
            false,  /*  3 scheduledToPaused */
            false,  /*  4 scheduledToStopped */
            false,  /*  5 removed */
            true,   /*  6 playingMiddleKeep */
            false,  /*  7 playingToPausedKeep */
            false,  /*  8 playingEndToStopped */
            false,  /*  9 playingToStoppedKeep */
            true,   /* 10 playingRepeated */
            true,   /* 11 playingRepeatedReverseReverseEveryOther */
            false,  /* 12 paused */
            false,  /* 13 pausedToStopped */
            false,  /* 14 stoppedRemove */
            false,  /* 15 stoppedKeep */
            true,   /* 16 zeroDurationScheduled */
            true,   /* 17 zeroDurationScheduledReverseKeep */
            false,  /* 18 zeroDurationStopped */
        }).sliceBit(0), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(Containers::BitArrayView{started}, Containers::stridedArrayView({
            false,  /*  0 scheduledKeep */
            false,  /*  1 scheduledToPlayingBegin */
            false,  /*  2 scheduledToPlayingReverse */
            false,  /*  3 scheduledToPaused */
            false,  /*  4 scheduledToStopped */
            false,  /*  5 removed */
            false,  /*  6 playingMiddleKeep */
            false,  /*  7 playingToPausedKeep */
            false,  /*  8 playingEndToStopped */
            false,  /*  9 playingToStoppedKeep */
            false,  /* 10 playingRepeated */
            false,  /* 11 playingRepeatedReverseReverseEveryOther */
            false,  /* 12 paused */
            false,  /* 13 pausedToStopped */
            false,  /* 14 stoppedRemove */
            false,  /* 15 stoppedKeep */
            true,   /* 16 zeroDurationScheduled */
            true,   /* 17 zeroDurationScheduledReverseKeep */
            false,  /* 18 zeroDurationStopped */
        }).sliceBit(0), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(Containers::BitArrayView{stopped}, Containers::stridedArrayView({
            false,  /*  0 scheduledKeep */
            true,   /*  1 scheduledToPlayingBegin */
            true,   /*  2 scheduledToPlayingReverse */
            false,  /*  3 scheduledToPaused */
            false,  /*  4 scheduledToStopped */
            false,  /*  5 removed */
            true,   /*  6 playingMiddleKeep */
            false,  /*  7 playingToPausedKeep */
            false,  /*  8 playingEndToStopped */
            false,  /*  9 playingToStoppedKeep */
            false,  /* 10 playingRepeated */
            false,  /* 11 playingRepeatedReverseReverseEveryOther */
            false,  /* 12 paused */
            false,  /* 13 pausedToStopped */
            false,  /* 14 stoppedRemove */
            false,  /* 15 stoppedKeep */
            true,   /* 16 zeroDurationScheduled */
            true,   /* 17 zeroDurationScheduledReverseKeep */
            false,  /* 18 zeroDurationStopped */
        }).sliceBit(0), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(factors, Containers::arrayView({
            unused, /*  0 scheduledKeep */
            1.0f,   /*  1 scheduledToPlayingBegin */
            0.0f,   /*  2 scheduledToPlayingReverse */
            unused, /*  3 scheduledToPaused */
            unused, /*  4 scheduledToStopped */
            unused, /*  5 removed */
            1.0f,   /*  6 playingMiddleKeep */
            unused, /*  7 playingToPausedKeep */
            unused, /*  8 playingEndToStopped */
            unused, /*  9 playingToStoppedKeep */
            0.4f,   /* 10 playingRepeated */
            0.7f,   /* 11 playingRepeatedReverseReverseEveryOther */
            unused, /* 12 paused */
            unused, /* 13 pausedToStopped */
            unused, /* 14 stoppedRemove */
            unused, /* 15 stoppedKeep */
            1.0f,   /* 16 zeroDurationScheduled */
            0.0f,   /* 17 zeroDurationScheduledReverseKeep */
            unused, /* 18 zeroDurationStopped */
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(Containers::BitArrayView{remove}, Containers::stridedArrayView({
            false,  /*  0 scheduledKeep */
            true,   /*  1 scheduledToPlayingBegin */
            true,   /*  2 scheduledToPlayingReverse */
            false,  /*  3 scheduledToPaused */
            false,  /*  4 scheduledToStopped */
            false,  /*  5 removed */
            false,  /*  6 playingMiddleKeep */
            false,  /*  7 playingToPausedKeep */
            false,  /*  8 playingEndToStopped */
            false,  /*  9 playingToStoppedKeep */
            false,  /* 10 playingRepeated */
            false,  /* 11 playingRepeatedReverseReverseEveryOther */
            false,  /* 12 paused */
            false,  /* 13 pausedToStopped */
            false,  /* 14 stoppedRemove */
            false,  /* 15 stoppedKeep */
            true,   /* 16 zeroDurationScheduled */
            false,  /* 17 zeroDurationScheduledReverseKeep */
            false,  /* 18 zeroDurationStopped */
        }).sliceBit(0), TestSuite::Compare::Container);

        /* Need to call this ourselves to not have the removed animations
           picked up again next time */
        animator.clean(remove);
    }
    CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);

    /* The scheduledToPlayingBegin and zeroDurationScheduled gets removed,
       playingMiddleKeep not because is marked as such */
    CORRADE_VERIFY(animator.isHandleValid(scheduledKeep));
    CORRADE_VERIFY(!animator.isHandleValid(scheduledToPlayingBegin));
    CORRADE_VERIFY(!animator.isHandleValid(scheduledToPlayingReverse));
    CORRADE_VERIFY(animator.isHandleValid(scheduledToPaused));
    CORRADE_VERIFY(!animator.isHandleValid(scheduledToStopped));
    CORRADE_VERIFY(!animator.isHandleValid(removed));
    CORRADE_VERIFY(animator.isHandleValid(playingMiddleKeep));
    CORRADE_VERIFY(animator.isHandleValid(playingToPausedKeep));
    CORRADE_VERIFY(!animator.isHandleValid(playingEndToStopped));
    CORRADE_VERIFY(animator.isHandleValid(playingToStoppedKeep));
    CORRADE_VERIFY(animator.isHandleValid(playingRepeated));
    CORRADE_VERIFY(animator.isHandleValid(playingRepeatedReverseReverseEveryOther));
    CORRADE_VERIFY(animator.isHandleValid(paused));
    CORRADE_VERIFY(!animator.isHandleValid(pausedToStopped));
    CORRADE_VERIFY(!animator.isHandleValid(stoppedRemove));
    CORRADE_VERIFY(animator.isHandleValid(stoppedKeep));
    CORRADE_VERIFY(!animator.isHandleValid(zeroDurationScheduled));
    CORRADE_VERIFY(animator.isHandleValid(zeroDurationScheduledReverseKeep));
    CORRADE_VERIFY(!animator.isHandleValid(zeroDurationStopped));

    CORRADE_COMPARE(animator.state(scheduledKeep), AnimationState::Scheduled);
    /* scheduledToPlayingBegin is gone */
    /* scheduledToPlayingReverse is gone */
    CORRADE_COMPARE(animator.state(scheduledToPaused), AnimationState::Paused);
    /* scheduledToStopped is gone */
    /* removed is gone */
    CORRADE_COMPARE(animator.state(playingMiddleKeep), AnimationState::Stopped);
    CORRADE_COMPARE(animator.state(playingToPausedKeep), AnimationState::Paused);
    /* playingEndToStopped is gone */
    CORRADE_COMPARE(animator.state(playingToStoppedKeep), AnimationState::Stopped);
    CORRADE_COMPARE(animator.state(playingRepeated), AnimationState::Playing);
    CORRADE_COMPARE(animator.state(playingRepeatedReverseReverseEveryOther), AnimationState::Playing);
    CORRADE_COMPARE(animator.state(paused), AnimationState::Paused);
    /* pausedToStopped is gone */
    /* stoppedRemove is gone */
    CORRADE_COMPARE(animator.state(stoppedKeep), AnimationState::Stopped);
    /* zeroDurationScheduled is gone */
    CORRADE_COMPARE(animator.state(zeroDurationScheduledReverseKeep), AnimationState::Stopped);
    /* zeroDurationStopped is gone */

    /* After stopping what's paused, call at 30 advances the remaining
       animations including the last iteration of the repeated ones, after that
       there's nothing to advance anymore */
    animator.stop(scheduledToPaused, 30_nsec);
    animator.stop(playingToPausedKeep, 30_nsec);
    animator.stop(paused, 30_nsec);
    {
        Containers::BitArray active{NoInit, animationCount};
        Containers::BitArray started{NoInit, animationCount};
        Containers::BitArray stopped{NoInit, animationCount};
        Containers::StaticArray<animationCount, Float> factors{DirectInit, unused};
        Containers::BitArray remove{NoInit, animationCount};
        CORRADE_COMPARE(animator.update(30_nsec, active, started, stopped, factors, remove), Containers::pair(true, true));
        CORRADE_COMPARE_AS(Containers::BitArrayView{active}, Containers::stridedArrayView({
            true,   /*  0 scheduledKeep */
            false,  /*  1 scheduledToPlayingBegin */
            false,  /*  2 scheduledToPlayingReverse */
            true,   /*  3 scheduledToPaused */
            false,  /*  4 scheduledToStopped */
            false,  /*  5 removed */
            false,  /*  6 playingMiddleKeep */
            true,   /*  7 playingToPausedKeep */
            false,  /*  8 playingEndToStopped */
            false,  /*  9 playingToStoppedKeep */
            true,   /* 10 playingRepeated */
            true,   /* 11 playingRepeatedReverseReverseEveryOther */
            true,   /* 12 paused */
            false,  /* 13 pausedToStopped */
            false,  /* 14 stoppedRemove */
            false,  /* 15 stoppedKeep */
            false,  /* 16 zeroDurationScheduled */
            false,  /* 17 zeroDurationScheduledReverseKeep */
            false,  /* 18 zeroDurationStopped */
        }).sliceBit(0), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(Containers::BitArrayView{started}, Containers::stridedArrayView({
            true,   /*  0 scheduledKeep */
            false,  /*  1 scheduledToPlayingBegin */
            false,  /*  2 scheduledToPlayingReverse */
            false,  /*  3 scheduledToPaused */
            false,  /*  4 scheduledToStopped */
            false,  /*  5 removed */
            false,  /*  6 playingMiddleKeep */
            false,  /*  7 playingToPausedKeep */
            false,  /*  8 playingEndToStopped */
            false,  /*  9 playingToStoppedKeep */
            false,  /* 10 playingRepeated */
            false,  /* 11 playingRepeatedReverseReverseEveryOther */
            false,  /* 12 paused */
            false,  /* 13 pausedToStopped */
            false,  /* 14 stoppedRemove */
            false,  /* 15 stoppedKeep */
            false,  /* 16 zeroDurationScheduled */
            false,  /* 17 zeroDurationScheduledReverseKeep */
            false,  /* 18 zeroDurationStopped */
        }).sliceBit(0), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(Containers::BitArrayView{stopped}, Containers::stridedArrayView({
            false,  /*  0 scheduledKeep */
            false,  /*  1 scheduledToPlayingBegin */
            false,  /*  2 scheduledToPlayingReverse */
            true,   /*  3 scheduledToPaused */
            false,  /*  4 scheduledToStopped */
            false,  /*  5 removed */
            false,  /*  6 playingMiddleKeep */
            true,   /*  7 playingToPausedKeep */
            false,  /*  8 playingEndToStopped */
            false,  /*  9 playingToStoppedKeep */
            true,   /* 10 playingRepeated */
            true,   /* 11 playingRepeatedReverseReverseEveryOther */
            true,   /* 12 paused */
            false,  /* 13 pausedToStopped */
            false,  /* 14 stoppedRemove */
            false,  /* 15 stoppedKeep */
            false,  /* 16 zeroDurationScheduled */
            false,  /* 17 zeroDurationScheduledReverseKeep */
            false,  /* 18 zeroDurationStopped */
        }).sliceBit(0), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(factors, Containers::arrayView({
            0.0f,   /*  0 scheduledKeep */
            unused, /*  1 scheduledToPlayingBegin */
            unused, /*  2 scheduledToPlayingReverse */
            1.0f,   /*  3 scheduledToPaused */
            unused, /*  4 scheduledToStopped */
            unused, /*  5 removed */
            unused, /*  6 playingMiddleKeep */
            1.0f,   /*  7 playingToPausedKeep */
            unused, /*  8 playingEndToStopped */
            unused, /*  9 playingToStoppedKeep */
            1.0f,   /* 10 playingRepeated */
            0.0f,   /* 11 playingRepeatedReverseReverseEveryOther */
            1.0f,   /* 12 paused */
            unused, /* 13 pausedToStopped */
            unused, /* 14 stoppedRemove */
            unused, /* 15 stoppedKeep */
            unused, /* 16 zeroDurationScheduled */
            unused, /* 17 zeroDurationScheduledReverseKeep */
            unused, /* 18 zeroDurationStopped */
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(Containers::BitArrayView{remove}, Containers::stridedArrayView({
            false,  /*  0 scheduledKeep */
            false,  /*  1 scheduledToPlayingBegin */
            false,  /*  2 scheduledToPlayingReverse */
            true,   /*  3 scheduledToPaused */
            false,  /*  4 scheduledToStopped */
            false,  /*  5 removed */
            false,  /*  6 playingMiddleKeep */
            false,  /*  7 playingToPausedKeep */
            false,  /*  8 playingEndToStopped */
            false,  /*  9 playingToStoppedKeep */
            true,   /* 10 playingRepeated */
            true,   /* 11 playingRepeatedReverseReverseEveryOther */
            true,   /* 12 paused */
            false,  /* 13 pausedToStopped */
            false,  /* 14 stoppedRemove */
            false,  /* 15 stoppedKeep */
            false,  /* 16 zeroDurationScheduled */
            false,  /* 17 zeroDurationScheduledReverseKeep */
            false,  /* 18 zeroDurationStopped */
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
    CORRADE_VERIFY(!animator.isHandleValid(scheduledToPlayingReverse));
    CORRADE_VERIFY(!animator.isHandleValid(scheduledToPaused));
    CORRADE_VERIFY(!animator.isHandleValid(scheduledToStopped));
    CORRADE_VERIFY(!animator.isHandleValid(removed));
    CORRADE_VERIFY(animator.isHandleValid(playingMiddleKeep));
    CORRADE_VERIFY(animator.isHandleValid(playingToPausedKeep));
    CORRADE_VERIFY(!animator.isHandleValid(playingEndToStopped));
    CORRADE_VERIFY(animator.isHandleValid(playingToStoppedKeep));
    CORRADE_VERIFY(!animator.isHandleValid(playingRepeated));
    CORRADE_VERIFY(!animator.isHandleValid(playingRepeatedReverseReverseEveryOther));
    CORRADE_VERIFY(!animator.isHandleValid(paused));
    CORRADE_VERIFY(!animator.isHandleValid(pausedToStopped));
    CORRADE_VERIFY(!animator.isHandleValid(stoppedRemove));
    CORRADE_VERIFY(animator.isHandleValid(stoppedKeep));
    CORRADE_VERIFY(!animator.isHandleValid(zeroDurationScheduled));
    CORRADE_VERIFY(animator.isHandleValid(zeroDurationScheduledReverseKeep));
    CORRADE_VERIFY(!animator.isHandleValid(zeroDurationStopped));

    CORRADE_COMPARE(animator.state(scheduledKeep), AnimationState::Playing);
    /* scheduledToPlayingBegin is gone */
    /* scheduledToPlayingReverse is gone */
    /* scheduledToPaused is gone */
    /* scheduledToStopped is gone */
    /* removed is gone */
    CORRADE_COMPARE(animator.state(playingMiddleKeep), AnimationState::Stopped);
    CORRADE_COMPARE(animator.state(playingToPausedKeep), AnimationState::Stopped);
    /* playingEndToStopped is gone */
    CORRADE_COMPARE(animator.state(playingToStoppedKeep), AnimationState::Stopped);
    /* playingRepeated is gone */
    /* playingRepeatedReverseReverseEveryOther is gone */
    /* paused is gone */
    /* pausedToStopped is gone */
    /* stoppedRemove is gone */
    CORRADE_COMPARE(animator.state(stoppedKeep), AnimationState::Stopped);
    /* zeroDurationScheduled is gone */
    CORRADE_COMPARE(animator.state(zeroDurationScheduledReverseKeep), AnimationState::Stopped);
    /* zeroDurationStopped is gone */

    /* Call at 40 doesn't need to delegeate to clean() anymore */
    {
        Containers::BitArray active{NoInit, animationCount};
        Containers::BitArray started{NoInit, animationCount};
        Containers::BitArray stopped{NoInit, animationCount};
        Containers::StaticArray<animationCount, Float> factors{DirectInit, unused};
        Containers::BitArray remove{NoInit, animationCount};
        CORRADE_COMPARE(animator.update(40_nsec, active, started, stopped, factors, remove), Containers::pair(true, false));
        CORRADE_COMPARE_AS(Containers::BitArrayView{active}, Containers::stridedArrayView({
            true,   /*  0 scheduledKeep */
            false,  /*  1 scheduledToPlayingBegin */
            false,  /*  2 scheduledToPlayingReverse */
            false,  /*  3 scheduledToPaused */
            false,  /*  4 scheduledToStopped */
            false,  /*  5 removed */
            false,  /*  6 playingMiddleKeep */
            false,  /*  7 playingToPausedKeep */
            false,  /*  8 playingEndToStopped */
            false,  /*  9 playingToStoppedKeep */
            false,  /* 10 playingRepeated */
            false,  /* 11 playingRepeatedReverseReverseEveryOther */
            false,  /* 12 paused */
            false,  /* 13 pausedToStopped */
            false,  /* 14 stoppedRemove */
            false,  /* 15 stoppedKeep */
            false,  /* 16 zeroDurationScheduled */
            false,  /* 17 zeroDurationScheduledReverseKeep */
            false,  /* 18 zeroDurationStopped */
        }).sliceBit(0), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(Containers::BitArrayView{started}, Containers::stridedArrayView({
            false,  /*  0 scheduledKeep */
            false,  /*  1 scheduledToPlayingBegin */
            false,  /*  2 scheduledToPlayingReverse */
            false,  /*  3 scheduledToPaused */
            false,  /*  4 scheduledToStopped */
            false,  /*  5 removed */
            false,  /*  6 playingMiddleKeep */
            false,  /*  7 playingToPausedKeep */
            false,  /*  8 playingEndToStopped */
            false,  /*  9 playingToStoppedKeep */
            false,  /* 10 playingRepeated */
            false,  /* 11 playingRepeatedReverseReverseEveryOther */
            false,  /* 12 paused */
            false,  /* 13 pausedToStopped */
            false,  /* 14 stoppedRemove */
            false,  /* 15 stoppedKeep */
            false,  /* 16 zeroDurationScheduled */
            false,  /* 17 zeroDurationScheduledReverseKeep */
            false,  /* 18 zeroDurationStopped */
        }).sliceBit(0), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(Containers::BitArrayView{stopped}, Containers::stridedArrayView({
            true,   /*  0 scheduledKeep */
            false,  /*  1 scheduledToPlayingBegin */
            false,  /*  2 scheduledToPlayingReverse */
            false,  /*  3 scheduledToPaused */
            false,  /*  4 scheduledToStopped */
            false,  /*  5 removed */
            false,  /*  6 playingMiddleKeep */
            false,  /*  7 playingToPausedKeep */
            false,  /*  8 playingEndToStopped */
            false,  /*  9 playingToStoppedKeep */
            false,  /* 10 playingRepeated */
            false,  /* 11 playingRepeatedReverseReverseEveryOther */
            false,  /* 12 paused */
            false,  /* 13 pausedToStopped */
            false,  /* 14 stoppedRemove */
            false,  /* 15 stoppedKeep */
            false,  /* 16 zeroDurationScheduled */
            false,  /* 17 zeroDurationScheduledReverseKeep */
            false,  /* 18 zeroDurationStopped */
        }).sliceBit(0), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(factors, Containers::arrayView({
            1.0f,   /*  0 scheduledKeep */
            unused, /*  1 scheduledToPlayingBegin */
            unused, /*  2 scheduledToPlayingReverse */
            unused, /*  3 scheduledToPaused */
            unused, /*  4 scheduledToStopped */
            unused, /*  5 removed */
            unused, /*  6 playingMiddleKeep */
            unused, /*  7 playingToPausedKeep */
            unused, /*  8 playingEndToStopped */
            unused, /*  9 playingToStoppedKeep */
            unused, /* 10 playingRepeated */
            unused, /* 11 playingRepeatedReverseReverseEveryOther */
            unused, /* 12 paused */
            unused, /* 13 pausedToStopped */
            unused, /* 14 stoppedRemove */
            unused, /* 15 stoppedKeep */
            unused, /* 16 zeroDurationScheduled */
            unused, /* 17 zeroDurationScheduledReverseKeep */
            unused, /* 18 zeroDurationStopped */
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(remove,
            (Containers::BitArray{DirectInit, animationCount, false}),
            TestSuite::Compare::Container);

        /* Nothing to remove, not calling clean() */
    }
    /* It also doesn't need to advance anything after this */
    CORRADE_COMPARE(animator.state(), AnimatorStates{});

    /* Same as before */
    CORRADE_VERIFY(animator.isHandleValid(scheduledKeep));
    CORRADE_VERIFY(!animator.isHandleValid(scheduledToPlayingBegin));
    CORRADE_VERIFY(!animator.isHandleValid(scheduledToPlayingReverse));
    CORRADE_VERIFY(!animator.isHandleValid(scheduledToPaused));
    CORRADE_VERIFY(!animator.isHandleValid(scheduledToStopped));
    CORRADE_VERIFY(!animator.isHandleValid(removed));
    CORRADE_VERIFY(animator.isHandleValid(playingMiddleKeep));
    CORRADE_VERIFY(animator.isHandleValid(playingToPausedKeep));
    CORRADE_VERIFY(!animator.isHandleValid(playingEndToStopped));
    CORRADE_VERIFY(animator.isHandleValid(playingToStoppedKeep));
    CORRADE_VERIFY(!animator.isHandleValid(playingRepeated));
    CORRADE_VERIFY(!animator.isHandleValid(playingRepeatedReverseReverseEveryOther));
    CORRADE_VERIFY(!animator.isHandleValid(paused));
    CORRADE_VERIFY(!animator.isHandleValid(pausedToStopped));
    CORRADE_VERIFY(!animator.isHandleValid(stoppedRemove));
    CORRADE_VERIFY(animator.isHandleValid(stoppedKeep));
    CORRADE_VERIFY(!animator.isHandleValid(zeroDurationScheduled));
    CORRADE_VERIFY(animator.isHandleValid(zeroDurationScheduledReverseKeep));
    CORRADE_VERIFY(!animator.isHandleValid(zeroDurationStopped));

    CORRADE_COMPARE(animator.state(scheduledKeep), AnimationState::Stopped);
    /* scheduledToPlayingBegin is gone */
    /* scheduledToPlayingReverse is gone */
    /* scheduledToPaused is gone */
    /* scheduledToStopped is gone */
    /* removed is gone */
    CORRADE_COMPARE(animator.state(playingMiddleKeep), AnimationState::Stopped);
    CORRADE_COMPARE(animator.state(playingToPausedKeep), AnimationState::Stopped);
    /* playingEndToStopped is gone */
    CORRADE_COMPARE(animator.state(playingToStoppedKeep), AnimationState::Stopped);
    /* playingRepeated is gone */
    /* playingRepeatedReverseReverseEveryOther is gone */
    /* paused is gone */
    /* pausedToStopped is gone */
    /* stoppedRemove is gone */
    CORRADE_COMPARE(animator.state(stoppedKeep), AnimationState::Stopped);
    /* zeroDurationScheduled is gone */
    CORRADE_COMPARE(animator.state(zeroDurationScheduledReverseKeep), AnimationState::Stopped);
    /* zeroDurationStopped is gone */

    /* Call at 50 needs neither advance nor clean anymore */
    {
        Containers::BitArray active{NoInit, animationCount};
        Containers::BitArray started{NoInit, animationCount};
        Containers::BitArray stopped{NoInit, animationCount};
        Containers::StaticArray<animationCount, Float> factors{DirectInit, unused};
        Containers::BitArray remove{NoInit, animationCount};
        CORRADE_COMPARE(animator.update(50_nsec, active, started, stopped, factors, remove), Containers::pair(false, false));
        CORRADE_COMPARE_AS(active,
            (Containers::BitArray{DirectInit, animationCount, false}),
            TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(started,
            (Containers::BitArray{DirectInit, animationCount, false}),
            TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stopped,
            (Containers::BitArray{DirectInit, animationCount, false}),
            TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(factors,
            (Containers::StaticArray<animationCount, Float>{DirectInit, unused}),
            TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(remove,
            (Containers::BitArray{DirectInit, animationCount, false}),
            TestSuite::Compare::Container);

        /* Nothing to remove, not calling clean() */
    }
    CORRADE_COMPARE(animator.state(), AnimatorStates{});

    /* Same as before */
    CORRADE_VERIFY(animator.isHandleValid(scheduledKeep));
    CORRADE_VERIFY(!animator.isHandleValid(scheduledToPlayingBegin));
    CORRADE_VERIFY(!animator.isHandleValid(scheduledToPlayingReverse));
    CORRADE_VERIFY(!animator.isHandleValid(scheduledToPaused));
    CORRADE_VERIFY(!animator.isHandleValid(scheduledToStopped));
    CORRADE_VERIFY(!animator.isHandleValid(removed));
    CORRADE_VERIFY(animator.isHandleValid(playingMiddleKeep));
    CORRADE_VERIFY(animator.isHandleValid(playingToPausedKeep));
    CORRADE_VERIFY(!animator.isHandleValid(playingEndToStopped));
    CORRADE_VERIFY(animator.isHandleValid(playingToStoppedKeep));
    CORRADE_VERIFY(!animator.isHandleValid(playingRepeated));
    CORRADE_VERIFY(!animator.isHandleValid(playingRepeatedReverseReverseEveryOther));
    CORRADE_VERIFY(!animator.isHandleValid(paused));
    CORRADE_VERIFY(!animator.isHandleValid(pausedToStopped));
    CORRADE_VERIFY(!animator.isHandleValid(stoppedRemove));
    CORRADE_VERIFY(animator.isHandleValid(stoppedKeep));
    CORRADE_VERIFY(!animator.isHandleValid(zeroDurationScheduled));
    CORRADE_VERIFY(animator.isHandleValid(zeroDurationScheduledReverseKeep));
    CORRADE_VERIFY(!animator.isHandleValid(zeroDurationStopped));

    /* Same as before */
    CORRADE_COMPARE(animator.state(scheduledKeep), AnimationState::Stopped);
    /* scheduledToPlayingBegin is gone */
    /* scheduledToPlayingReverse is gone */
    /* scheduledToPaused is gone */
    /* scheduledToStopped is gone */
    /* removed is gone */
    CORRADE_COMPARE(animator.state(playingMiddleKeep), AnimationState::Stopped);
    CORRADE_COMPARE(animator.state(playingToPausedKeep), AnimationState::Stopped);
    /* playingEndToStopped is gone */
    CORRADE_COMPARE(animator.state(playingToStoppedKeep), AnimationState::Stopped);
    /* playingRepeated is gone */
    /* playingRepeatedReverseReverseEveryOther is gone */
    /* paused is gone */
    /* pausedToStopped is gone */
    /* stoppedRemove is gone */
    CORRADE_COMPARE(animator.state(stoppedKeep), AnimationState::Stopped);
    /* zeroDurationScheduled is gone */
    CORRADE_COMPARE(animator.state(zeroDurationScheduledReverseKeep), AnimationState::Stopped);
    /* zeroDurationStopped is gone */
}

void AbstractAnimatorTest::updateEmpty() {
    struct: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;

        AnimatorFeatures doFeatures() const override { return {}; }
    } animator{animatorHandle(0, 1)};
    CORRADE_COMPARE(animator.time(), 0_nsec);
    CORRADE_COMPARE(animator.state(), AnimatorStates{});

    CORRADE_COMPARE(animator.update(56_nsec, {}, {}, {}, {}, {}), Containers::pair(false, false));
    CORRADE_COMPARE(animator.time(), 56_nsec);
    CORRADE_COMPARE(animator.state(), AnimatorStates{});
}

void AbstractAnimatorTest::updateInvalid() {
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
    animator.update(46_nsec, mask, mask, mask, factors, mask);
    animator.update(46_nsec, mask, mask, mask, factors, mask);
    CORRADE_COMPARE(animator.time(), 46_nsec);

    Containers::String out;
    Error redirectError{&out};
    animator.update(45_nsec, mask, mask, mask, factors, mask);
    animator.update(46_nsec, mask, mask, mask, factors, maskIncorrect);
    animator.update(46_nsec, mask, mask, mask, factorsIncorrect, mask);
    animator.update(46_nsec, mask, mask, maskIncorrect, factors, mask);
    animator.update(46_nsec, mask, maskIncorrect, mask, factors, mask);
    animator.update(46_nsec, maskIncorrect, mask, mask, factors, mask);
    CORRADE_COMPARE_AS(out,
        "Ui::AbstractAnimator::update(): expected a time at least Nanoseconds(46) but got Nanoseconds(45)\n"
        "Ui::AbstractAnimator::update(): expected active, started, stopped, factors and remove views to have a size of 3 but got 3, 3, 3, 3 and 4\n"
        "Ui::AbstractAnimator::update(): expected active, started, stopped, factors and remove views to have a size of 3 but got 3, 3, 3, 4 and 3\n"
        "Ui::AbstractAnimator::update(): expected active, started, stopped, factors and remove views to have a size of 3 but got 3, 3, 4, 3 and 3\n"
        "Ui::AbstractAnimator::update(): expected active, started, stopped, factors and remove views to have a size of 3 but got 3, 4, 3, 3 and 3\n"
        "Ui::AbstractAnimator::update(): expected active, started, stopped, factors and remove views to have a size of 3 but got 4, 3, 3, 3 and 3\n",
        TestSuite::Compare::String);
}

void AbstractAnimatorTest::advanceGeneric() {
    struct: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;
        using AbstractGenericAnimator::create;

        AnimatorFeatures doFeatures() const override { return {}; }
        void doAdvance(Containers::BitArrayView active, Containers::BitArrayView started, Containers::BitArrayView stopped, const Containers::StridedArrayView1D<const Float>& factors) override {
            CORRADE_COMPARE_AS(active, Containers::stridedArrayView({
                true,
                false,
                true
            }).sliceBit(0), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(started, Containers::stridedArrayView({
                false,
                false,
                true
            }).sliceBit(0), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(stopped, Containers::stridedArrayView({
                false,
                true,
                true
            }).sliceBit(0), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(factors, Containers::arrayView({
                1.0f,
                0.5f,
                0.75f
            }), TestSuite::Compare::Container);
            ++advanceCallCount;
        }
        Int advanceCallCount = 0;
    } animator{animatorHandle(0, 1)};

    animator.create(0_nsec, 1_nsec);
    animator.create(0_nsec, 1_nsec);
    animator.create(0_nsec, 1_nsec);

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::BitArray active{DirectInit, 3, true};
    Containers::BitArray started{DirectInit, 3, false};
    Containers::BitArray stopped{DirectInit, 3, true};
    active.reset(1);
    started.set(2);
    stopped.reset(0);
    Float factors[]{
        1.0f,
        0.5f,
        0.75f
    };
    animator.advance(active, started, stopped, factors);
    CORRADE_COMPARE(animator.advanceCallCount, 1);
}

void AbstractAnimatorTest::advanceGenericInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;
        using AbstractGenericAnimator::create;

        AnimatorFeatures doFeatures() const override { return {}; }
        void doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
    } animator{animatorHandle(0, 1)};

    animator.create(0_nsec, 1_nsec);
    animator.create(0_nsec, 1_nsec);
    animator.create(0_nsec, 1_nsec);

    Containers::BitArray mask{NoInit, 3};
    Containers::BitArray maskInvalid{NoInit, 4};
    Float factors[3];
    Float factorsInvalid[4];

    Containers::String out;
    Error redirectError{&out};
    animator.advance(mask, mask, mask, factorsInvalid);
    animator.advance(mask, mask, maskInvalid, factors);
    animator.advance(mask, maskInvalid, mask, factors);
    animator.advance(maskInvalid, mask, mask, factors);
    CORRADE_COMPARE_AS(out,
        "Ui::AbstractGenericAnimator::advance(): expected active, started, stopped and factors views to have a size of 3 but got 3, 3, 3 and 4\n"
        "Ui::AbstractGenericAnimator::advance(): expected active, started, stopped and factors views to have a size of 3 but got 3, 3, 4 and 3\n"
        "Ui::AbstractGenericAnimator::advance(): expected active, started, stopped and factors views to have a size of 3 but got 3, 4, 3 and 3\n"
        "Ui::AbstractGenericAnimator::advance(): expected active, started, stopped and factors views to have a size of 3 but got 4, 3, 3 and 3\n",
        TestSuite::Compare::String);
}

void AbstractAnimatorTest::advanceNode() {
    struct: AbstractNodeAnimator {
        using AbstractNodeAnimator::AbstractNodeAnimator;
        using AbstractNodeAnimator::create;

        AnimatorFeatures doFeatures() const override { return {}; }
        NodeAnimatorUpdates doAdvance(Containers::BitArrayView active, Containers::BitArrayView started, Containers::BitArrayView stopped, const Containers::StridedArrayView1D<const Float>& factors, const Containers::StridedArrayView1D<Vector2>& nodeOffsets, const Containers::StridedArrayView1D<Vector2>& nodeSizes, const Containers::StridedArrayView1D<Float>& nodeOpacities, const Containers::StridedArrayView1D<NodeFlags>& nodeFlags, Containers::MutableBitArrayView nodesRemove) override {
            CORRADE_COMPARE_AS(active, Containers::stridedArrayView({
                true,
                false,
                true
            }).sliceBit(0), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(started, Containers::stridedArrayView({
                false,
                false,
                true
            }).sliceBit(0), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(stopped, Containers::stridedArrayView({
                false,
                true,
                true
            }).sliceBit(0), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(factors, Containers::arrayView({
                1.0f,
                0.5f,
                0.75f
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(nodeOffsets, Containers::stridedArrayView<Vector2>({
                {1.0f, 2.0f},
                {3.0f, 4.0f},
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(nodeSizes, Containers::stridedArrayView<Vector2>({
                {5.0f, 6.0f},
                {8.0f, 8.0f},
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(nodeOpacities, Containers::stridedArrayView<Float>({
                0.75f,
                0.25f
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(nodeFlags, Containers::stridedArrayView({
                NodeFlags{},
                NodeFlag::Clip|NodeFlag::Disabled,
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(nodesRemove, Containers::stridedArrayView({
                false,
                true
            }).sliceBit(0), TestSuite::Compare::Container);
            ++advanceCallCount;

            return NodeAnimatorUpdates{0xc0};
        }
        Int advanceCallCount = 0;
    } animator{animatorHandle(0, 1)};

    animator.create(0_nsec, 1_nsec);
    animator.create(0_nsec, 1_nsec);
    animator.create(0_nsec, 1_nsec);

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::BitArray active{DirectInit, 3, true};
    Containers::BitArray started{DirectInit, 3, false};
    Containers::BitArray stopped{DirectInit, 3, true};
    active.reset(1);
    started.set(2);
    stopped.reset(0);
    Float factors[]{
        1.0f,
        0.5f,
        0.75f
    };
    Vector2 nodeOffsets[]{
        {1.0f, 2.0f},
        {3.0f, 4.0f},
    };
    Vector2 nodeSizes[]{
        {5.0f, 6.0f},
        {8.0f, 8.0f},
    };
    Float nodeOpacities[]{
        0.75f,
        0.25f
    };
    NodeFlags nodeFlags[]{
        {},
        NodeFlag::Clip|NodeFlag::Disabled,
    };
    Containers::BitArray nodesRemove{ValueInit, 2};
    nodesRemove.set(1);
    CORRADE_COMPARE(animator.advance(active, started, stopped, factors, nodeOffsets, nodeSizes, nodeOpacities, nodeFlags, nodesRemove), NodeAnimatorUpdates{0xc0});
    CORRADE_COMPARE(animator.advanceCallCount, 1);
}

void AbstractAnimatorTest::advanceNodeInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractNodeAnimator {
        using AbstractNodeAnimator::AbstractNodeAnimator;
        using AbstractNodeAnimator::create;

        AnimatorFeatures doFeatures() const override { return {}; }
        NodeAnimatorUpdates doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Float>&, const Containers::StridedArrayView1D<NodeFlags>&, Containers::MutableBitArrayView) override {
            CORRADE_FAIL("This shouldn't be called.");
            return {};
        }
    } animator{animatorHandle(0, 1)};

    animator.create(0_nsec, 1_nsec);
    animator.create(0_nsec, 1_nsec);

    Containers::BitArray mask{NoInit, 2};
    Containers::BitArray maskInvalid{NoInit, 3};
    Float factors[2];
    Float factorsInvalid[3];
    Containers::BitArray nodesEnabled{NoInit, 3};
    Containers::BitArray nodesEnabledInvalid{NoInit, 4};
    Vector2 nodeOffsetsSizes[3];
    Vector2 nodeOffsetsSizesInvalid[4];
    Float nodeOpacities[3];
    Float nodeOpacitiesInvalid[4];
    NodeFlags nodeFlags[3];
    NodeFlags nodeFlagsInvalid[4];

    Containers::String out;
    Error redirectError{&out};
    animator.advance(mask, mask, mask, factorsInvalid, nodeOffsetsSizes, nodeOffsetsSizes, nodeOpacities, nodeFlags, nodesEnabled);
    animator.advance(mask, mask, maskInvalid, factors, nodeOffsetsSizes, nodeOffsetsSizes, nodeOpacities, nodeFlags, nodesEnabled);
    animator.advance(mask, maskInvalid, mask, factors, nodeOffsetsSizes, nodeOffsetsSizes, nodeOpacities, nodeFlags, nodesEnabled);
    animator.advance(maskInvalid, mask, mask, factors, nodeOffsetsSizes, nodeOffsetsSizes, nodeOpacities, nodeFlags, nodesEnabled);
    animator.advance(mask, mask, mask, factors, nodeOffsetsSizes, nodeOffsetsSizes, nodeOpacities, nodeFlags, nodesEnabledInvalid);
    animator.advance(mask, mask, mask, factors, nodeOffsetsSizes, nodeOffsetsSizes, nodeOpacities, nodeFlagsInvalid, nodesEnabled);
    animator.advance(mask, mask, mask, factors, nodeOffsetsSizes, nodeOffsetsSizes, nodeOpacitiesInvalid, nodeFlags, nodesEnabled);
    animator.advance(mask, mask, mask, factors, nodeOffsetsSizes, nodeOffsetsSizesInvalid, nodeOpacities, nodeFlags, nodesEnabled);
    animator.advance(mask, mask, mask, factors, nodeOffsetsSizesInvalid, nodeOffsetsSizes, nodeOpacities, nodeFlags, nodesEnabled);
    CORRADE_COMPARE_AS(out,
        "Ui::AbstractNodeAnimator::advance(): expected active, started, stopped and factors views to have a size of 2 but got 2, 2, 2 and 3\n"
        "Ui::AbstractNodeAnimator::advance(): expected active, started, stopped and factors views to have a size of 2 but got 2, 2, 3 and 2\n"
        "Ui::AbstractNodeAnimator::advance(): expected active, started, stopped and factors views to have a size of 2 but got 2, 3, 2 and 2\n"
        "Ui::AbstractNodeAnimator::advance(): expected active, started, stopped and factors views to have a size of 2 but got 3, 2, 2 and 2\n"
        "Ui::AbstractNodeAnimator::advance(): expected node offset, size, opacity, flags and remove views to have the same size but got 3, 3, 3, 3 and 4\n"
        "Ui::AbstractNodeAnimator::advance(): expected node offset, size, opacity, flags and remove views to have the same size but got 3, 3, 3, 4 and 3\n"
        "Ui::AbstractNodeAnimator::advance(): expected node offset, size, opacity, flags and remove views to have the same size but got 3, 3, 4, 3 and 3\n"
        "Ui::AbstractNodeAnimator::advance(): expected node offset, size, opacity, flags and remove views to have the same size but got 3, 4, 3, 3 and 3\n"
        "Ui::AbstractNodeAnimator::advance(): expected node offset, size, opacity, flags and remove views to have the same size but got 4, 3, 3, 3 and 3\n",
        TestSuite::Compare::String);
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

    /* Animation that's created scheduled sets a state, removal & update()
       clears it */
    {
        AnimationHandle animation = animator.create(10_nsec, 10_nsec);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Scheduled);
        CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);
        animator.remove(animation);
        CORRADE_COMPARE(animator.update(0_nsec, mask, mask, mask, factors, mask), Containers::pair(false, false));
        CORRADE_COMPARE(animator.state(), AnimatorStates{});

    /* Animation that's created playing sets a state, removal & update()
       clears it */
    } {
        AnimationHandle animation = animator.create(0_nsec, 10_nsec);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Playing);
        CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);
        animator.remove(animation);
        CORRADE_COMPARE(animator.update(0_nsec, mask, mask, mask, factors, mask), Containers::pair(false, false));
        CORRADE_COMPARE(animator.state(), AnimatorStates{});

    /* Animation that's created stopped and with KeepOncePlayed doesn't set
       anything */
    } {
        AnimationHandle animation = animator.create(-10_nsec, 10_nsec, AnimationFlag::KeepOncePlayed);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Stopped);
        CORRADE_COMPARE(animator.state(), AnimatorStates{});
        animator.remove(animation);

    /* Animation that's created stopped sets a state, update() then marks it
       for removal and clears the state */
    } {
        AnimationHandle animation = animator.create(-10_nsec, 10_nsec);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Stopped);
        CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);
        Containers::BitArray remove{NoInit, 1};
        CORRADE_COMPARE(animator.update(0_nsec, mask, mask, mask, factors, remove), Containers::pair(false, true));
        CORRADE_COMPARE(remove[0], true);
        animator.remove(animation);
        CORRADE_VERIFY(!animator.isHandleValid(animation));
        CORRADE_COMPARE(animator.state(), AnimatorStates{});

    /* Animation that's scheduled after play() sets a state, removal & update()
       clears it */
    } {
        AnimationHandle animation = animator.create(-10_nsec, 10_nsec, AnimationFlag::KeepOncePlayed);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Stopped);
        CORRADE_COMPARE(animator.state(), AnimatorStates{});

        animator.play(animation, 10_nsec);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Scheduled);
        CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);
        animator.remove(animation);
        CORRADE_COMPARE(animator.update(0_nsec, mask, mask, mask, factors, mask), Containers::pair(false, false));
        CORRADE_COMPARE(animator.state(), AnimatorStates{});

    /* Animation that's playing after play() sets a state, removal & update()
       clears it */
    } {
        AnimationHandle animation = animator.create(-10_nsec, 10_nsec, AnimationFlag::KeepOncePlayed);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Stopped);
        CORRADE_COMPARE(animator.state(), AnimatorStates{});

        animator.play(animation, 0_nsec);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Playing);
        CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);
        animator.remove(animation);
        CORRADE_COMPARE(animator.update(0_nsec, mask, mask, mask, factors, mask), Containers::pair(false, false));
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
       update() clears it */
    } {
        AnimationHandle animation = animator.create(10_nsec, 10_nsec);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Scheduled);
        CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);

        animator.pause(animation, 10_nsec);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Scheduled);
        CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);
        animator.remove(animation);
        CORRADE_COMPARE(animator.update(0_nsec, mask, mask, mask, factors, mask), Containers::pair(false, false));
        CORRADE_COMPARE(animator.state(), AnimatorStates{});

    /* Animation that stays playing after pause() keeps the state, removal &
       update() clears it */
    } {
        AnimationHandle animation = animator.create(0_nsec, 10_nsec);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Playing);
        CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);

        animator.pause(animation, 5_nsec);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Playing);
        CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);
        animator.remove(animation);
        CORRADE_COMPARE(animator.update(0_nsec, mask, mask, mask, factors, mask), Containers::pair(false, false));
        CORRADE_COMPARE(animator.state(), AnimatorStates{});

    /* Animation that's paused after pause() keeps the state, removal &
       update() clears it */
    } {
        AnimationHandle animation = animator.create(0_nsec, 10_nsec);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Playing);
        CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);

        animator.pause(animation, 0_nsec);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Paused);
        CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);
        animator.remove(animation);
        CORRADE_COMPARE(animator.update(0_nsec, mask, mask, mask, factors, mask), Containers::pair(false, false));
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
       update() clears it */
    } {
        AnimationHandle animation = animator.create(10_nsec, 10_nsec);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Scheduled);
        CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);

        animator.stop(animation, 20_nsec);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Scheduled);
        CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);
        animator.remove(animation);
        CORRADE_COMPARE(animator.update(0_nsec, mask, mask, mask, factors, mask), Containers::pair(false, false));
        CORRADE_COMPARE(animator.state(), AnimatorStates{});

    /* Animation that stays playing after stop() keeps the state, removal &
       update() clears it */
    } {
        AnimationHandle animation = animator.create(0_nsec, 10_nsec);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Playing);
        CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);

        animator.stop(animation, 10_nsec);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Playing);
        CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);
        animator.remove(animation);
        CORRADE_COMPARE(animator.update(0_nsec, mask, mask, mask, factors, mask), Containers::pair(false, false));
        CORRADE_COMPARE(animator.state(), AnimatorStates{});

    /* Animation that stays paused after stop() keeps the state, removal &
       update() clears it */
    } {
        AnimationHandle animation = animator.create(0_nsec, 10_nsec, AnimationFlag::KeepOncePlayed);
        animator.pause(animation, 0_nsec);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Paused);
        CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);

        animator.stop(animation, 10_nsec);
        CORRADE_COMPARE(animator.state(animation), AnimationState::Paused);
        CORRADE_COMPARE(animator.state(), AnimatorState::NeedsAdvance);
        animator.remove(animation);
        CORRADE_COMPARE(animator.update(0_nsec, mask, mask, mask, factors, mask), Containers::pair(false, false));
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

CORRADE_TEST_MAIN(Magnum::Ui::Test::AbstractAnimatorTest)
