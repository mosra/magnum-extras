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

#include <Corrade/Containers/ArrayView.h> /* for arraySize() */
#include <Corrade/Containers/BitArray.h>
#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/StaticArray.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/StridedBitArrayView.h>
#include <Corrade/Containers/String.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Container.h>
#include <Corrade/TestSuite/Compare/String.h>
#include <Magnum/Animation/Easing.h>
#include <Magnum/Math/Functions.h>
#include <Magnum/Math/Time.h>

#include "Magnum/Ui/AbstractUserInterface.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/NodeAnimator.h"
#include "Magnum/Ui/NodeFlags.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct NodeAnimatorTest: TestSuite::Tester {
    explicit NodeAnimatorTest();

    void animationConstruct();
    void animationConstructCopy();
    void animationSetters();
    void animationSettersNan();

    void construct();
    void constructCopy();
    void constructMove();

    void createRemove();
    void createRemoveHandleRecycle();
    void createInvalid();
    /* There's no assert to trigger in remove() other than what's checked by
       AbstractAnimator::remove() already */
    void propertiesInvalid();

    void advance();
    void advanceProperties();
    void advanceEmpty();

    void uiAdvance();
};

using namespace Math::Literals;

const struct {
    TestSuite::TestCaseDescriptionSourceLocation name;
    NodeAnimation animation;
    /* The animation always plays from 5_nsec to 25_nsec */
    Nanoseconds advance;
    NodeAnimatorUpdates expectedUpdates;
    Vector2 expectedOffset;
    Vector2 expectedSize;
    Float expectedOpacity;
    bool expectedRemove;
    NodeFlags initialFlags, expectedFlags;
} AdvancePropertiesData[]{
    {"nothing",
        NodeAnimation{},
        30_nsec, {},
        {100.0f, 100.0f},
        {10.0f, 10.0f},
        1.0f, false,
        {}, {}},
    {"offset to, 75%",
        NodeAnimation{}
            .toOffset({0.0f, 200.0f}),
        20_nsec, NodeAnimatorUpdate::OffsetSize,
        {25.0f, 175.0f},
        {10.0f, 10.0f},
        1.0f, false,
        {}, {}},
    {"offset from, 25%",
        NodeAnimation{}
            .fromOffset({0.0f, 200.0f}),
        10_nsec, NodeAnimatorUpdate::OffsetSize,
        {25.0f, 175.0f},
        {10.0f, 10.0f},
        1.0f, false,
        {}, {}},
    {"offset from to, 50%",
        NodeAnimation{}
            .fromOffset({1000.0f, 10.0f})
            .toOffset({2000.0f, 20.0f}),
        15_nsec, NodeAnimatorUpdate::OffsetSize,
        {1500.0f, 15.0f},
        {10.0f, 10.0f},
        1.0f, false,
        {}, {}},
    {"offset X from, 25%",
        NodeAnimation{}
            .fromOffsetX(0.0f),
        10_nsec, NodeAnimatorUpdate::OffsetSize,
        {25.0f, 100.0f},
        {10.0f, 10.0f},
        1.0f, false,
        {}, {}},
    {"offset X to, 75%",
        NodeAnimation{}
            .toOffsetX(0.0f),
        20_nsec, NodeAnimatorUpdate::OffsetSize,
        {25.0f, 100.0f},
        {10.0f, 10.0f},
        1.0f, false,
        {}, {}},
    {"offset Y from, 25%",
        NodeAnimation{}
            .fromOffsetY(200.0f),
        10_nsec, NodeAnimatorUpdate::OffsetSize,
        {100.0f, 175.0f},
        {10.0f, 10.0f},
        1.0f, false,
        {}, {}},
    {"offset Y to, 75%",
        NodeAnimation{}
            .toOffsetY(200.0f),
        20_nsec, NodeAnimatorUpdate::OffsetSize,
        {100.0f, 175.0f},
        {10.0f, 10.0f},
        1.0f, false,
        {}, {}},
    {"offset X from, offset Y to, 50%",
        NodeAnimation{}
            .fromOffsetX(0.0f)
            .toOffsetY(200.0f),
        15_nsec, NodeAnimatorUpdate::OffsetSize,
        {50.0f, 150.0f},
        {10.0f, 10.0f},
        1.0f, false,
        {}, {}},
    {"offset Y from, offset X to, 50%",
        NodeAnimation{}
            .fromOffsetY(200.0f)
            .toOffsetX(0.0f),
        15_nsec, NodeAnimatorUpdate::OffsetSize,
        {50.0f, 150.0f},
        {10.0f, 10.0f},
        1.0f, false,
        {}, {}},
    {"size to, 75%",
        NodeAnimation{}
            .toSize({0.0f, 20.0f}),
        20_nsec, NodeAnimatorUpdate::OffsetSize,
        {100.0f, 100.0f},
        {2.5f, 17.5f},
        1.0f, false,
        {}, {}},
    {"size from, 25%",
        NodeAnimation{}
            .fromSize({0.0f, 20.0f}),
        10_nsec, NodeAnimatorUpdate::OffsetSize,
        {100.0f, 100.0f},
        {2.5f, 17.5f},
        1.0f, false,
        {}, {}},
    {"size from to, 50%",
        NodeAnimation{}
            .fromSize({1000.0f, 10.0f})
            .toSize({2000.0f, 20.0f}),
        15_nsec, NodeAnimatorUpdate::OffsetSize,
        {100.0f, 100.0f},
        {1500.0f, 15.0f},
        1.0f, false,
        {}, {}},
    {"size X from, 25%",
        NodeAnimation{}
            .fromSizeX(0.0f),
        10_nsec, NodeAnimatorUpdate::OffsetSize,
        {100.0f, 100.0f},
        {2.5f, 10.0f},
        1.0f, false,
        {}, {}},
    {"size X to, 75%",
        NodeAnimation{}
            .toSizeX(0.0f),
        20_nsec, NodeAnimatorUpdate::OffsetSize,
        {100.0f, 100.0f},
        {2.5f, 10.0f},
        1.0f, false,
        {}, {}},
    {"size Y from, 25%",
        NodeAnimation{}
            .fromSizeY(20.0f),
        10_nsec, NodeAnimatorUpdate::OffsetSize,
        {100.0f, 100.0f},
        {10.0f, 17.5f},
        1.0f, false,
        {}, {}},
    {"size Y to, 75%",
        NodeAnimation{}
            .toSizeY(20.0f),
        20_nsec, NodeAnimatorUpdate::OffsetSize,
        {100.0f, 100.0f},
        {10.0f, 17.5f},
        1.0f, false,
        {}, {}},
    {"size X from, size Y to, 50%",
        NodeAnimation{}
            .fromSizeX(0.0f)
            .toSizeY(20.0f),
        15_nsec, NodeAnimatorUpdate::OffsetSize,
        {100.0f, 100.0f},
        {5.0f, 15.0f},
        1.0f, false,
        {}, {}},
    {"size Y from, size X to, 50%",
        NodeAnimation{}
            .fromSizeY(20.0f)
            .toSizeX(0.0f),
        15_nsec, NodeAnimatorUpdate::OffsetSize,
        {100.0f, 100.0f},
        {5.0f, 15.0f},
        1.0f, false,
        {}, {}},
    {"opacity to, 75%",
        NodeAnimation{}
            .toOpacity(0.0f),
        20_nsec, NodeAnimatorUpdate::Opacity,
        {100.0f, 100.0f},
        {10.0f, 10.0f},
        0.25f, false,
        {}, {}},
    {"opacity from, 25%",
        NodeAnimation{}
            .fromOpacity(0.0f),
        10_nsec, NodeAnimatorUpdate::Opacity,
        {100.0f, 100.0f},
        {10.0f, 10.0f},
        0.25f, false,
        {}, {}},
    {"opacity from to, 50%",
        NodeAnimation{}
            .fromOpacity(0.9f)
            .toOpacity(0.3f),
        15_nsec, NodeAnimatorUpdate::Opacity,
        {100.0f, 100.0f},
        {10.0f, 10.0f},
        0.6f, false,
        {}, {}},
    {"add FallthroughPointerEvents flag begin, 25%",
        NodeAnimation{}
            .addFlagsBegin(NodeFlag::FallthroughPointerEvents),
        10_nsec, NodeAnimatorUpdates{},
        {100.0f, 100.0f},
        {10.0f, 10.0f},
        1.0f, false,
        NodeFlag::Clip, NodeFlag::Clip|NodeFlag::FallthroughPointerEvents},
    {"add NoEvents flag begin, 25%",
        NodeAnimation{}
            .addFlagsBegin(NodeFlag::NoEvents),
        10_nsec, NodeAnimatorUpdate::Enabled,
        {100.0f, 100.0f},
        {10.0f, 10.0f},
        1.0f, false,
        NodeFlag::Clip, NodeFlag::Clip|NodeFlag::NoEvents},
    {"add Disabled flag begin, 25%",
        NodeAnimation{}
            .addFlagsBegin(NodeFlag::Disabled),
        10_nsec, NodeAnimatorUpdate::Enabled,
        {100.0f, 100.0f},
        {10.0f, 10.0f},
        1.0f, false,
        NodeFlag::Clip, NodeFlag::Clip|NodeFlag::Disabled},
    {"add Focusable flag begin, 25%",
        NodeAnimation{}
            .addFlagsBegin(NodeFlag::Focusable),
        10_nsec, NodeAnimatorUpdate::Enabled,
        {100.0f, 100.0f},
        {10.0f, 10.0f},
        1.0f, false,
        NodeFlag::Clip, NodeFlag::Clip|NodeFlag::Focusable},
    {"add Hidden flag begin, 25%",
        NodeAnimation{}
            .addFlagsBegin(NodeFlag::Hidden),
        10_nsec, NodeAnimatorUpdate::Visibility,
        {100.0f, 100.0f},
        {10.0f, 10.0f},
        1.0f, false,
        NodeFlag::Clip, NodeFlag::Clip|NodeFlag::Hidden},
    {"add multiple flags begin, 25%",
        NodeAnimation{}
            .addFlagsBegin(NodeFlag::Disabled|NodeFlag::NoBlur|NodeFlag::Hidden),
        10_nsec, NodeAnimatorUpdate::Enabled|NodeAnimatorUpdate::EventMask|NodeAnimatorUpdate::Visibility,
        {100.0f, 100.0f},
        {10.0f, 10.0f},
        1.0f, false,
        NodeFlag::Clip, NodeFlag::Clip|NodeFlag::Disabled|NodeFlag::NoBlur|NodeFlag::Hidden},
    {"add multiple flags begin, 125%",
        NodeAnimation{}
            .addFlagsBegin(NodeFlag::Disabled|NodeFlag::NoBlur|NodeFlag::Hidden),
        30_nsec, NodeAnimatorUpdate::Enabled|NodeAnimatorUpdate::EventMask|NodeAnimatorUpdate::Visibility,
        {100.0f, 100.0f},
        {10.0f, 10.0f},
        1.0f, false,
        NodeFlag::Clip, NodeFlag::Clip|NodeFlag::Disabled|NodeFlag::NoBlur|NodeFlag::Hidden},
    {"add multiple flags end, 25%",
        NodeAnimation{}
            .addFlagsEnd(NodeFlag::NoEvents|NodeFlag::NoBlur|NodeFlag::Hidden),
        10_nsec, NodeAnimatorUpdates{},
        {100.0f, 100.0f},
        {10.0f, 10.0f},
        1.0f, false,
        NodeFlag::Clip,
        NodeFlag::Clip},
    {"add FallthroughPointerEvents flag end, 125%",
        NodeAnimation{}
            .addFlagsEnd(NodeFlag::FallthroughPointerEvents),
        30_nsec, NodeAnimatorUpdates{},
        {100.0f, 100.0f},
        {10.0f, 10.0f},
        1.0f, false,
        NodeFlag::Clip, NodeFlag::Clip|NodeFlag::FallthroughPointerEvents},
    {"add NoEvents flag end, 125%",
        NodeAnimation{}
            .addFlagsEnd(NodeFlag::NoEvents),
        30_nsec, NodeAnimatorUpdate::Enabled,
        {100.0f, 100.0f},
        {10.0f, 10.0f},
        1.0f, false,
        NodeFlag::Clip, NodeFlag::Clip|NodeFlag::NoEvents},
    {"add Disabled flag end, 125%",
        NodeAnimation{}
            .addFlagsEnd(NodeFlag::Disabled),
        30_nsec, NodeAnimatorUpdate::Enabled,
        {100.0f, 100.0f},
        {10.0f, 10.0f},
        1.0f, false,
        NodeFlag::Clip, NodeFlag::Clip|NodeFlag::Disabled},
    {"add Focusable flag end, 125%",
        NodeAnimation{}
            .addFlagsEnd(NodeFlag::Focusable),
        30_nsec, NodeAnimatorUpdate::Enabled,
        {100.0f, 100.0f},
        {10.0f, 10.0f},
        1.0f, false,
        NodeFlag::Clip, NodeFlag::Clip|NodeFlag::Focusable},
    {"add Hidden flag end, 125%",
        NodeAnimation{}
            .addFlagsEnd(NodeFlag::Hidden),
        30_nsec, NodeAnimatorUpdate::Visibility,
        {100.0f, 100.0f},
        {10.0f, 10.0f},
        1.0f, false,
        NodeFlag::Clip, NodeFlag::Clip|NodeFlag::Hidden},
    {"add multiple flags end, 125%",
        NodeAnimation{}
            .addFlagsEnd(NodeFlag::NoEvents|NodeFlag::NoBlur|NodeFlag::Hidden),
        30_nsec, NodeAnimatorUpdate::Enabled|NodeAnimatorUpdate::EventMask|NodeAnimatorUpdate::Visibility,
        {100.0f, 100.0f},
        {10.0f, 10.0f},
        1.0f, false,
        NodeFlag::Clip, NodeFlag::Clip|NodeFlag::NoEvents|NodeFlag::NoBlur|NodeFlag::Hidden},
    {"clear FallthroughPointerEvents flag begin, 25%",
        NodeAnimation{}
            .clearFlagsBegin(NodeFlag::FallthroughPointerEvents),
        10_nsec, NodeAnimatorUpdates{},
        {100.0f, 100.0f},
        {10.0f, 10.0f},
        1.0f, false,
        NodeFlag::Clip|NodeFlag::FallthroughPointerEvents, NodeFlag::Clip},
    {"clear NoEvents flag begin, 25%",
        NodeAnimation{}
            .clearFlagsBegin(NodeFlag::NoEvents),
        10_nsec, NodeAnimatorUpdate::Enabled,
        {100.0f, 100.0f},
        {10.0f, 10.0f},
        1.0f, false,
        NodeFlag::Clip|NodeFlag::NoEvents, NodeFlag::Clip},
    {"clear Disabled flag begin, 25%",
        NodeAnimation{}
            .clearFlagsBegin(NodeFlag::Disabled),
        10_nsec, NodeAnimatorUpdate::Enabled,
        {100.0f, 100.0f},
        {10.0f, 10.0f},
        1.0f, false,
        NodeFlag::Clip|NodeFlag::Disabled, NodeFlag::Clip},
    {"clear Focusable flag begin, 25%",
        NodeAnimation{}
            .clearFlagsBegin(NodeFlag::Focusable),
        10_nsec, NodeAnimatorUpdate::Enabled,
        {100.0f, 100.0f},
        {10.0f, 10.0f},
        1.0f, false,
        NodeFlag::Clip|NodeFlag::Focusable, NodeFlag::Clip},
    {"clear Hidden flag begin, 25%",
        NodeAnimation{}
            .clearFlagsBegin(NodeFlag::Hidden),
        10_nsec, NodeAnimatorUpdate::Visibility,
        {100.0f, 100.0f},
        {10.0f, 10.0f},
        1.0f, false,
        NodeFlag::Clip|NodeFlag::Hidden, NodeFlag::Clip},
    {"clear multiple flags begin, 25%",
        NodeAnimation{}
            .clearFlagsBegin(NodeFlag::Disabled|NodeFlag::Hidden|NodeFlag::NoBlur),
        10_nsec, NodeAnimatorUpdate::Enabled|NodeAnimatorUpdate::EventMask|NodeAnimatorUpdate::Visibility,
        {100.0f, 100.0f},
        {10.0f, 10.0f},
        1.0f, false,
        NodeFlag::Clip|NodeFlag::Hidden|NodeFlag::Disabled|NodeFlag::NoBlur, NodeFlag::Clip},
    {"clear multiple flags begin, 125%",
        NodeAnimation{}
            .clearFlagsBegin(NodeFlag::Disabled|NodeFlag::Hidden|NodeFlag::NoBlur),
        30_nsec, NodeAnimatorUpdate::Enabled|NodeAnimatorUpdate::EventMask|NodeAnimatorUpdate::Visibility,
        {100.0f, 100.0f},
        {10.0f, 10.0f},
        1.0f, false,
        NodeFlag::Clip|NodeFlag::Hidden|NodeFlag::Disabled|NodeFlag::NoBlur, NodeFlag::Clip},
    {"clear multiple flags end, 25%",
        NodeAnimation{}
            .clearFlagsEnd(NodeFlag::Focusable|NodeFlag::NoBlur|NodeFlag::Hidden),
        10_nsec, NodeAnimatorUpdates{},
        {100.0f, 100.0f},
        {10.0f, 10.0f},
        1.0f, false,
        NodeFlag::Clip|NodeFlag::Focusable|NodeFlag::NoBlur|NodeFlag::Hidden,
        NodeFlag::Clip|NodeFlag::Focusable|NodeFlag::NoBlur|NodeFlag::Hidden},
    {"clear FallthroughPointerEvents flag end, 125%",
        NodeAnimation{}
            .clearFlagsEnd(NodeFlag::FallthroughPointerEvents),
        30_nsec, NodeAnimatorUpdates{},
        {100.0f, 100.0f},
        {10.0f, 10.0f},
        1.0f, false,
        NodeFlag::Clip|NodeFlag::FallthroughPointerEvents, NodeFlag::Clip},
    {"clear NoEvents flag end, 125%",
        NodeAnimation{}
            .clearFlagsEnd(NodeFlag::NoEvents),
        30_nsec, NodeAnimatorUpdate::Enabled,
        {100.0f, 100.0f},
        {10.0f, 10.0f},
        1.0f, false,
        NodeFlag::Clip|NodeFlag::NoEvents, NodeFlag::Clip},
    {"clear Disabled flag end, 125%",
        NodeAnimation{}
            .clearFlagsEnd(NodeFlag::Disabled),
        30_nsec, NodeAnimatorUpdate::Enabled,
        {100.0f, 100.0f},
        {10.0f, 10.0f},
        1.0f, false,
        NodeFlag::Clip|NodeFlag::Disabled, NodeFlag::Clip},
    {"clear Focusable flag end, 125%",
        NodeAnimation{}
            .clearFlagsEnd(NodeFlag::Focusable),
        30_nsec, NodeAnimatorUpdate::Enabled,
        {100.0f, 100.0f},
        {10.0f, 10.0f},
        1.0f, false,
        NodeFlag::Clip|NodeFlag::Focusable, NodeFlag::Clip},
    {"clear Hidden flag end, 125%",
        NodeAnimation{}
            .clearFlagsEnd(NodeFlag::Hidden),
        30_nsec, NodeAnimatorUpdate::Visibility,
        {100.0f, 100.0f},
        {10.0f, 10.0f},
        1.0f, false,
        NodeFlag::Clip|NodeFlag::Hidden, NodeFlag::Clip},
    {"clear multiple flags end, 125%",
        NodeAnimation{}
            .clearFlagsEnd(NodeFlag::Focusable|NodeFlag::NoBlur|NodeFlag::Hidden),
        30_nsec, NodeAnimatorUpdate::Enabled|NodeAnimatorUpdate::EventMask|NodeAnimatorUpdate::Visibility,
        {100.0f, 100.0f},
        {10.0f, 10.0f},
        1.0f, false,
        NodeFlag::Clip|NodeFlag::Focusable|NodeFlag::NoBlur|NodeFlag::Hidden, NodeFlag::Clip},
    /* These four should result in no NodeAnimatorUpdates being set */
    {"add flags that are already present at begin, 25%",
        NodeAnimation{}
            /* Only FallthroughPointerEvents is extra, which is the only that
               causes no NodeAnimatorUpdates to be set */
            .addFlagsBegin(NodeFlag::NoEvents|NodeFlag::Focusable|NodeFlag::Clip|NodeFlag::NoBlur|NodeFlag::FallthroughPointerEvents),
        10_nsec, NodeAnimatorUpdates{},
        {100.0f, 100.0f},
        {10.0f, 10.0f},
        1.0f, false,
        NodeFlag::NoEvents|NodeFlag::Focusable|NodeFlag::Clip|NodeFlag::NoBlur,
        NodeFlag::NoEvents|NodeFlag::Focusable|NodeFlag::Clip|NodeFlag::NoBlur|NodeFlag::FallthroughPointerEvents},
    {"add flags that are already present at end, 125%",
        NodeAnimation{}
            /* Only FallthroughPointerEvents is extra, which is the only that
               causes no NodeAnimatorUpdates to be set */
            .addFlagsEnd(NodeFlag::Disabled|NodeFlag::Focusable|NodeFlag::Clip|NodeFlag::NoBlur|NodeFlag::FallthroughPointerEvents),
        30_nsec, NodeAnimatorUpdates{},
        {100.0f, 100.0f},
        {10.0f, 10.0f},
        1.0f, false,
        NodeFlag::Disabled|NodeFlag::Focusable|NodeFlag::Clip|NodeFlag::NoBlur,
        NodeFlag::Disabled|NodeFlag::Focusable|NodeFlag::Clip|NodeFlag::NoBlur|NodeFlag::FallthroughPointerEvents},
    {"clear flags that are not present at begin, 25%",
        NodeAnimation{}
            /* Only FallthroughPointerEvents is removed, which is the only that
               causes no NodeAnimatorUpdates to be set */
            .clearFlagsBegin(NodeFlag::Disabled|NodeFlag::Focusable|NodeFlag::Clip|NodeFlag::NoBlur|NodeFlag::FallthroughPointerEvents),
        10_nsec, NodeAnimatorUpdates{},
        {100.0f, 100.0f},
        {10.0f, 10.0f},
        1.0f, false,
        NodeFlag(0x80)|NodeFlag::FallthroughPointerEvents,
        NodeFlag(0x80)},
    {"clear flags that are not present at end, 125%",
        NodeAnimation{}
            /* Only FallthroughPointerEvents is removed, which is the only that
               causes no NodeAnimatorUpdates to be set */
            .clearFlagsEnd(NodeFlag::NoEvents|NodeFlag::Focusable|NodeFlag::Clip|NodeFlag::NoBlur|NodeFlag::FallthroughPointerEvents),
        30_nsec, NodeAnimatorUpdates{},
        {100.0f, 100.0f},
        {10.0f, 10.0f},
        1.0f, false,
        NodeFlag(0x80)|NodeFlag::FallthroughPointerEvents,
        NodeFlag(0x80)},
    /* In both of those, it should clear first and only then add, otherwise
       it'd result in no flags set at all */
    {"clear all flags and add back a subset at begin, 25%",
        NodeAnimation{}
            .clearFlagsBegin(~NodeFlags{})
            .addFlagsBegin(NodeFlag::Clip|NodeFlag::Disabled),
        /* Enabled isn't present for Disabled because it's cleared but then
           added back */
        10_nsec, NodeAnimatorUpdate::EventMask|NodeAnimatorUpdate::Clip,
        {100.0f, 100.0f},
        {10.0f, 10.0f},
        1.0f, false,
        NodeFlag::Disabled|NodeFlag::NoBlur|NodeFlag::FallthroughPointerEvents,
        NodeFlag::Disabled|NodeFlag::Clip},
    {"clear all flags and add back a subset at end, 125%",
        NodeAnimation{}
            .clearFlagsEnd(~NodeFlags{})
            .addFlagsEnd(NodeFlag::Clip|NodeFlag::Disabled),
        /* Enabled isn't present for Focusable because it's cleared but then
           added back */
        30_nsec, NodeAnimatorUpdate::EventMask|NodeAnimatorUpdate::Clip,
        {100.0f, 100.0f},
        {10.0f, 10.0f},
        1.0f, false,
        NodeFlag::Disabled|NodeFlag::NoBlur|NodeFlag::FallthroughPointerEvents,
        NodeFlag::Disabled|NodeFlag::Clip},
    {"add Disabled flag with NoEvents present at begin, 25%",
        NodeAnimation{}
            .addFlagsBegin(NodeFlag::Disabled),
        /* It's now newly Disabled, before it was only NoEvents */
        10_nsec, NodeAnimatorUpdate::Enabled,
        {100.0f, 100.0f},
        {10.0f, 10.0f},
        1.0f, false,
        NodeFlag::NoEvents,
        NodeFlag::Disabled},
    {"add Disabled flag with NoEvents present at end, 125%",
        NodeAnimation{}
            .addFlagsEnd(NodeFlag::Disabled),
        /* It's now newly Disabled, before it was only NoEvents */
        30_nsec, NodeAnimatorUpdate::Enabled,
        {100.0f, 100.0f},
        {10.0f, 10.0f},
        1.0f, false,
        NodeFlag::NoEvents,
        NodeFlag::Disabled},
    {"clear Disabled flag and add NoEvents at begin, 25%",
        NodeAnimation{}
            .clearFlagsBegin(NodeFlag::Disabled)
            .addFlagsBegin(NodeFlag::NoEvents),
        /* It's now newly only NoEvents, before it was Disabled, which counts
           as an update */
        10_nsec, NodeAnimatorUpdate::Enabled,
        {100.0f, 100.0f},
        {10.0f, 10.0f},
        1.0f, false,
        NodeFlag::Disabled,
        NodeFlag::NoEvents},
    {"clear Disabled flag and add NoEvents at end, 125%",
        NodeAnimation{}
            .clearFlagsEnd(NodeFlag::Disabled)
            .addFlagsEnd(NodeFlag::NoEvents),
        /* It's now newly only NoEvents, before it was Disabled, which counts
           as an update */
        30_nsec, NodeAnimatorUpdate::Enabled,
        {100.0f, 100.0f},
        {10.0f, 10.0f},
        1.0f, false,
        NodeFlag::Disabled,
        NodeFlag::NoEvents},
    /* It should also clear/add at begin before clear/add at end */
    {"clear/add flags at begin and then at end, 125%",
        NodeAnimation{}
            /* If clearFlagsBegin() would be done after addFlagsEnd(), the
               result wouldn't have Focusable */
            .clearFlagsBegin(NodeFlag::Disabled|NodeFlag::Focusable)
            /* If addFlagsBegin() would be done after clearFlagsEnd(), the
               result would have extra NoEvents */
            .addFlagsBegin(NodeFlag::NoEvents|NodeFlag::NoBlur)
            .clearFlagsEnd(NodeFlag::NoEvents|NodeFlag::Clip)
            .addFlagsEnd(NodeFlag::FallthroughPointerEvents|NodeFlag::Focusable),
        30_nsec, NodeAnimatorUpdate::Enabled|NodeAnimatorUpdate::EventMask|NodeAnimatorUpdate::Clip,
        {100.0f, 100.0f},
        {10.0f, 10.0f},
        1.0f, false,
        NodeFlag::Disabled|NodeFlag::Focusable|NodeFlag::Hidden|NodeFlag::Clip,
        NodeFlag::Hidden|NodeFlag::NoBlur|NodeFlag::FallthroughPointerEvents|NodeFlag::Focusable},
    {"remove a node at the end, 25%",
        NodeAnimation{}
            .setRemoveNodeAfter(true),
        10_nsec, NodeAnimatorUpdates{},
        {100.0f, 100.0f},
        {10.0f, 10.0f},
        1.0f, false,
        {}, {}},
    {"remove a node at the end, 125%",
        NodeAnimation{}
            .setRemoveNodeAfter(true),
        30_nsec, NodeAnimatorUpdate::Removal,
        {100.0f, 100.0f},
        {10.0f, 10.0f},
        1.0f, true,
        {}, {}},
};

const struct {
    const char* name;
    NodeAnimation animation;
    /* Node is at {20, 30} and has size {80, 100} initially, is animated to
       50% */
    Vector2 expectedOffset, expectedSize;
    Float expectedOpacity;
    NodeFlags expectedFlags;
    UserInterfaceStates expectedStates;
    /* Not testing offset / size / opacity at the end */
    NodeFlags expectedFlagsEnd;
    UserInterfaceStates expectedExtraStatesEnd;
    bool expectNodeRemovedEnd;
} UiAdvanceData[]{
    {"offset and size animation",
        NodeAnimation{}
            .fromSizeX(40.0f)
            .toSizeY(50.0f)
            .toOffset({10.0f, 10.0f}),
        {15.0f, 20.0f},
        {60.0f, 75.0f},
        1.0f, {}, UserInterfaceState::NeedsLayoutUpdate,
        {}, {}, false},
    {"opacity and clip animation",
        NodeAnimation{}
            .toOpacity(0.0f)
            .addFlagsEnd(NodeFlag::Clip),
        {20.0f, 30.0f},
        {80.0f, 100.0f},
        0.5f, {}, UserInterfaceState::NeedsNodeOpacityUpdate,
        NodeFlag::Clip, UserInterfaceState::NeedsNodeClipUpdate, false},
    {"disabled animation and removal",
        NodeAnimation{}
            .addFlagsBegin(NodeFlag::Disabled)
            .setRemoveNodeAfter(true),
        {20.0f, 30.0f},
        {80.0f, 100.0f},
        1.0f, NodeFlag::Disabled, UserInterfaceState::NeedsNodeEnabledUpdate,
        /* Cannot test the flag after as the node is removed */
        {}, UserInterfaceState::NeedsNodeClean, true},
};

NodeAnimatorTest::NodeAnimatorTest() {
    addTests({&NodeAnimatorTest::animationConstruct,
              &NodeAnimatorTest::animationConstructCopy,
              &NodeAnimatorTest::animationSetters,
              &NodeAnimatorTest::animationSettersNan,

              &NodeAnimatorTest::construct,
              &NodeAnimatorTest::constructCopy,
              &NodeAnimatorTest::constructMove,

              &NodeAnimatorTest::createRemove,
              &NodeAnimatorTest::createRemoveHandleRecycle,
              &NodeAnimatorTest::createInvalid,
              &NodeAnimatorTest::propertiesInvalid,

              &NodeAnimatorTest::advance});

    addInstancedTests({&NodeAnimatorTest::advanceProperties},
        Containers::arraySize(AdvancePropertiesData));

    addTests({&NodeAnimatorTest::advanceEmpty});

    addInstancedTests({&NodeAnimatorTest::uiAdvance},
        Containers::arraySize(UiAdvanceData));
}

void NodeAnimatorTest::animationConstruct() {
    NodeAnimation a;
    /* NaN comparison works only for scalars, using isNan() for vectors
       instead */
    CORRADE_COMPARE(Math::isNan(a.offsets().first()), BitVector2{3});
    CORRADE_COMPARE(Math::isNan(a.offsets().second()), BitVector2{3});
    CORRADE_COMPARE(Math::isNan(a.sizes().first()), BitVector2{3});
    CORRADE_COMPARE(Math::isNan(a.sizes().second()), BitVector2{3});
    CORRADE_COMPARE(a.opacities().first(), Constants::nan());
    CORRADE_COMPARE(a.opacities().second(), Constants::nan());
    CORRADE_COMPARE(a.flagsAdd(), (Containers::Pair<NodeFlags, NodeFlags>{}));
    CORRADE_COMPARE(a.flagsClear(), (Containers::Pair<NodeFlags, NodeFlags>{}));
    CORRADE_COMPARE(a.hasRemoveNodeAfter(), false);

    constexpr NodeAnimation ca;
    CORRADE_COMPARE(Math::isNan(ca.offsets().first()), BitVector2{3});
    CORRADE_COMPARE(Math::isNan(ca.offsets().second()), BitVector2{3});
    CORRADE_COMPARE(Math::isNan(ca.sizes().first()), BitVector2{3});
    CORRADE_COMPARE(Math::isNan(ca.sizes().second()), BitVector2{3});
    CORRADE_COMPARE(ca.opacities().first(), Constants::nan());
    CORRADE_COMPARE(ca.opacities().second(), Constants::nan());
    CORRADE_COMPARE(ca.flagsAdd(), (Containers::Pair<NodeFlags, NodeFlags>{}));
    CORRADE_COMPARE(ca.flagsClear(), (Containers::Pair<NodeFlags, NodeFlags>{}));
    constexpr bool hasRemoveNodeAfter = ca.hasRemoveNodeAfter();
    CORRADE_COMPARE(hasRemoveNodeAfter, false);
}

void NodeAnimatorTest::animationConstructCopy() {
    /* Testing just some properties, it's an implicitly generated copy */
    NodeAnimation a;
    a.addFlagsBegin(NodeFlag::Clip)
     .toOffset({3.0f, 2.0f});

    NodeAnimation b = a;
    CORRADE_COMPARE(Math::isNan(b.offsets().first()), BitVector2{3});
    CORRADE_COMPARE(b.offsets().second(), (Vector2{3.0f, 2.0f}));
    CORRADE_COMPARE(b.flagsAdd(), (Containers::Pair<NodeFlags, NodeFlags>{NodeFlag::Clip, {}}));

    #ifndef CORRADE_NO_STD_IS_TRIVIALLY_TRAITS
    CORRADE_VERIFY(std::is_trivially_copy_constructible<NodeAnimation>::value);
    CORRADE_VERIFY(std::is_trivially_copy_assignable<NodeAnimation>::value);
    #endif
}

void NodeAnimatorTest::animationSetters() {
    /* Keep some unset to verify that it can stay partially unset as well */
    NodeAnimation a;
    a.fromOffsetY(1.0f)
     .toOffsetX(2.0f)
     .fromSizeX(7.0f)
     .toSizeY(8.0f)
     .fromOpacity(0.25f)
     .clearFlagsBegin(NodeFlag::Disabled|NodeFlag::Focusable)
     .addFlagsEnd(NodeFlag::Hidden|NodeFlag::Disabled)
     .setRemoveNodeAfter(false);
    /* NaN comparison works only for scalars */
    CORRADE_COMPARE(a.offsets().first().x(), Constants::nan());
    CORRADE_COMPARE(a.offsets().first().y(), 1.0f);
    CORRADE_COMPARE(a.offsets().second().x(), 2.0f);
    CORRADE_COMPARE(a.offsets().second().y(), Constants::nan());
    CORRADE_COMPARE(a.sizes().first().x(), 7.0f);
    CORRADE_COMPARE(a.sizes().first().y(), Constants::nan());
    CORRADE_COMPARE(a.sizes().second().x(), Constants::nan());
    CORRADE_COMPARE(a.sizes().second().y(), 8.0f);
    CORRADE_COMPARE(a.opacities().first(), 0.25f);
    CORRADE_COMPARE(a.opacities().second(), Constants::nan());
    CORRADE_COMPARE(a.flagsAdd(), Containers::pair(NodeFlags{}, NodeFlag::Hidden|NodeFlag::Disabled));
    CORRADE_COMPARE(a.flagsClear(), Containers::pair(NodeFlag::Disabled|NodeFlag::Focusable, NodeFlags{}));
    CORRADE_COMPARE(a.hasRemoveNodeAfter(), false);

    NodeAnimation b;
    b.fromOffsetX(3.0f)
     .toOffsetY(4.0f)
     .fromSizeY(5.0f)
     .toSizeX(6.0f)
     .toOpacity(0.75f)
     .addFlagsBegin(NodeFlag::Clip|NodeFlag::NoEvents)
     .clearFlagsEnd(NodeFlag::FallthroughPointerEvents|NodeFlag::NoBlur)
     .setRemoveNodeAfter(true);
    /* NaN comparison works only for scalars */
    CORRADE_COMPARE(b.offsets().first().x(), 3.0f);
    CORRADE_COMPARE(b.offsets().first().y(), Constants::nan());
    CORRADE_COMPARE(b.offsets().second().x(), Constants::nan());
    CORRADE_COMPARE(b.offsets().second().y(), 4.0f);
    CORRADE_COMPARE(b.sizes().first().x(), Constants::nan());
    CORRADE_COMPARE(b.sizes().first().y(), 5.0f);
    CORRADE_COMPARE(b.sizes().second().x(), 6.0f);
    CORRADE_COMPARE(b.sizes().second().y(), Constants::nan());
    CORRADE_COMPARE(b.opacities().first(), Constants::nan());
    CORRADE_COMPARE(b.opacities().second(), 0.75f);
    CORRADE_COMPARE(b.flagsAdd(), Containers::pair(NodeFlag::Clip|NodeFlag::NoEvents, NodeFlags{}));
    CORRADE_COMPARE(b.flagsClear(), Containers::pair(NodeFlags{}, NodeFlag::FallthroughPointerEvents|NodeFlag::NoBlur));
    CORRADE_COMPARE(b.hasRemoveNodeAfter(), true);

    /* The X and Y setters shouldn't overwrite the other component, behaving
       the same as setting both at once */
    NodeAnimation c1, c2;
    c1.fromOffset({1.0f, 2.0f})
      .toSizeX(7.0f)
      .toSizeY(8.0f);
    c2.fromOffsetX(1.0f)
      .fromOffsetY(2.0f)
      .toSize({7.0f, 8.0f});
    CORRADE_COMPARE(c1.offsets().first(), (Vector2{1.0f, 2.0f}));
    CORRADE_COMPARE(c2.offsets().first(), (Vector2{1.0f, 2.0f}));
    CORRADE_COMPARE(Math::isNan(c1.offsets().second()), BitVector2{3});
    CORRADE_COMPARE(Math::isNan(c2.offsets().second()), BitVector2{3});
    CORRADE_COMPARE(Math::isNan(c1.sizes().first()), BitVector2{3});
    CORRADE_COMPARE(Math::isNan(c2.sizes().first()), BitVector2{3});
    CORRADE_COMPARE(c1.sizes().second(), (Vector2{7.0f, 8.0f}));
    CORRADE_COMPARE(c2.sizes().second(), (Vector2{7.0f, 8.0f}));

    /* Same for the other two */
    NodeAnimation d1, d2;
    d1.toOffsetX(3.0f)
      .toOffsetY(4.0f)
      .fromSize({5.0f, 6.0f});
    d2.toOffset({3.0f, 4.0f})
      .fromSizeX(5.0f)
      .fromSizeY(6.0f);
    CORRADE_COMPARE(Math::isNan(d1.offsets().first()), BitVector2{3});
    CORRADE_COMPARE(Math::isNan(d2.offsets().first()), BitVector2{3});
    CORRADE_COMPARE(d1.offsets().second(), (Vector2{3.0f, 4.0f}));
    CORRADE_COMPARE(d2.offsets().second(), (Vector2{3.0f, 4.0f}));
    CORRADE_COMPARE(d1.sizes().first(), (Vector2{5.0f, 6.0f}));
    CORRADE_COMPARE(d2.sizes().first(), (Vector2{5.0f, 6.0f}));
    CORRADE_COMPARE(Math::isNan(d1.sizes().second()), BitVector2{3});
    CORRADE_COMPARE(Math::isNan(d2.sizes().second()), BitVector2{3});
}

void NodeAnimatorTest::animationSettersNan() {
    /* Just to verify that explicitly setting NaNs doesn't make any difference
       compared to a default-constructed instance */
    NodeAnimation a1, a2, a3;
    a2.fromOffset({Constants::nan(), Constants::nan()})
      .toOffset({Constants::nan(), Constants::nan()})
      .fromSize({Constants::nan(), Constants::nan()})
      .toSize({Constants::nan(), Constants::nan()});
    a3.fromOffsetX(Constants::nan())
      .fromOffsetY(Constants::nan())
      .fromSizeX(Constants::nan())
      .fromSizeY(Constants::nan())
      .toOffsetX(Constants::nan())
      .toOffsetY(Constants::nan())
      .toSizeX(Constants::nan())
      .toSizeY(Constants::nan());
    for(const NodeAnimation& a: {a1, a2, a3}) {
        CORRADE_COMPARE(Math::isNan(a.offsets().first()), BitVector2{3});
        CORRADE_COMPARE(Math::isNan(a.offsets().second()), BitVector2{3});
        CORRADE_COMPARE(Math::isNan(a.sizes().first()), BitVector2{3});
        CORRADE_COMPARE(Math::isNan(a.sizes().second()), BitVector2{3});
        CORRADE_COMPARE(a.opacities().first(), Constants::nan());
        CORRADE_COMPARE(a.opacities().second(), Constants::nan());
    }
}

void NodeAnimatorTest::construct() {
    NodeAnimator animator{animatorHandle(0xab, 0x12)};

    CORRADE_COMPARE(animator.features(), AnimatorFeature::NodeAttachment);
    CORRADE_COMPARE(animator.handle(), animatorHandle(0xab, 0x12));
    /* The rest is the same as in AbstractAnimatorTest::constructStyle() */
}

void NodeAnimatorTest::constructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<NodeAnimator>{});
    CORRADE_VERIFY(!std::is_copy_assignable<NodeAnimator>{});
}

void NodeAnimatorTest::constructMove() {
    /* Just verify that the subclass doesn't have the moves broken */

    NodeAnimator a{animatorHandle(0xab, 0x12)};

    NodeAnimator b{Utility::move(a)};
    CORRADE_COMPARE(b.handle(), animatorHandle(0xab, 0x12));

    NodeAnimator c{animatorHandle(0xcd, 0x34)};
    c = Utility::move(b);
    CORRADE_COMPARE(c.handle(), animatorHandle(0xab, 0x12));

    CORRADE_VERIFY(std::is_nothrow_move_constructible<NodeAnimator>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<NodeAnimator>::value);
}

void NodeAnimatorTest::createRemove() {
    NodeAnimator animator{animatorHandle(0, 1)};

    /* Keep some unset to verify that it can stay partially unset as well */
    AnimationHandle first = animator.create(
        NodeAnimation{}
            .fromOffset({1.0f, 2.0f})
            .fromSizeX(7.0f)
            .fromOpacity(0.25f)
            .toOffsetY(8.0f)
            .clearFlagsBegin(NodeFlag::Disabled|NodeFlag::Focusable)
            .addFlagsEnd(NodeFlag::Hidden|NodeFlag::Disabled),
        Animation::Easing::bounceIn, 12_nsec, 13_nsec, nodeHandle(0xabcde, 0x123), 10, AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.usedCount(), 1);
    CORRADE_COMPARE(animator.duration(first), 13_nsec);
    CORRADE_COMPARE(animator.repeatCount(first), 10);
    CORRADE_COMPARE(animator.flags(first), AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.started(first), 12_nsec);
    CORRADE_COMPARE(animator.node(first), nodeHandle(0xabcde, 0x123));
    /* NaN comparison works only for scalars */
    CORRADE_COMPARE(animator.offsets(first).first().x(), 1.0f);
    CORRADE_COMPARE(animator.offsets(first).first().y(), 2.0f);
    CORRADE_COMPARE(animator.offsets(first).second().x(), Constants::nan());
    CORRADE_COMPARE(animator.offsets(first).second().y(), 8.0f);
    CORRADE_COMPARE(animator.sizes(first).first().x(), 7.0f);
    CORRADE_COMPARE(animator.sizes(first).first().y(), Constants::nan());
    CORRADE_COMPARE(animator.sizes(first).second().x(), Constants::nan());
    CORRADE_COMPARE(animator.sizes(first).second().y(), Constants::nan());
    CORRADE_COMPARE(animator.opacities(first).first(), 0.25f);
    CORRADE_COMPARE(animator.opacities(first).second(), Constants::nan());
    CORRADE_COMPARE(animator.flagsAdd(first), Containers::pair(NodeFlags{}, NodeFlag::Hidden|NodeFlag::Disabled));
    CORRADE_COMPARE(animator.flagsClear(first), Containers::pair(NodeFlag::Disabled|NodeFlag::Focusable, NodeFlags{}));
    CORRADE_COMPARE(animator.hasRemoveNodeAfter(first), false);
    CORRADE_COMPARE(animator.easing(first), Animation::Easing::bounceIn);

    /* Setting the other properties, using the repeat-less create() overload
       and verifying with AnimatorDataHandle getters */
    AnimationHandle second = animator.create(
        NodeAnimation{}
            .fromOffsetX(3.0f)
            .fromSizeY(4.0f)
            .toSize({5.0f, 6.0f})
            .toOpacity(0.75f)
            .addFlagsBegin(NodeFlag::Clip|NodeFlag::NoEvents)
            .clearFlagsEnd(NodeFlag::FallthroughPointerEvents|NodeFlag::NoBlur)
            .setRemoveNodeAfter(true),
        Animation::Easing::circularOut, 10_nsec, 20_nsec, nodeHandle(0x12345, 0xabc), AnimationFlag(0x80));
    CORRADE_COMPARE(animator.usedCount(), 2);
    CORRADE_COMPARE(animator.duration(animationHandleData(second)), 20_nsec);
    CORRADE_COMPARE(animator.repeatCount(animationHandleData(second)), 1);
    CORRADE_COMPARE(animator.flags(animationHandleData(second)), AnimationFlag(0x80));
    CORRADE_COMPARE(animator.started(animationHandleData(second)), 10_nsec);
    CORRADE_COMPARE(animator.node(animationHandleData(second)), nodeHandle(0x12345, 0xabc));
    /* NaN comparison works only for scalars */
    CORRADE_COMPARE(animator.offsets(animationHandleData(second)).first().x(), 3.0f);
    CORRADE_COMPARE(animator.offsets(animationHandleData(second)).first().y(), Constants::nan());
    CORRADE_COMPARE(animator.offsets(animationHandleData(second)).second().x(), Constants::nan());
    CORRADE_COMPARE(animator.offsets(animationHandleData(second)).second().y(), Constants::nan());
    CORRADE_COMPARE(animator.sizes(animationHandleData(second)).first().x(), Constants::nan());
    CORRADE_COMPARE(animator.sizes(animationHandleData(second)).first().y(), 4.0f);
    CORRADE_COMPARE(animator.sizes(animationHandleData(second)).second().x(), 5.0f);
    CORRADE_COMPARE(animator.sizes(animationHandleData(second)).second().y(), 6.0f);
    CORRADE_COMPARE(animator.opacities(animationHandleData(second)).first(), Constants::nan());
    CORRADE_COMPARE(animator.opacities(animationHandleData(second)).second(), 0.75f);
    CORRADE_COMPARE(animator.flagsAdd(animationHandleData(second)), Containers::pair(NodeFlag::Clip|NodeFlag::NoEvents, NodeFlags{}));
    CORRADE_COMPARE(animator.flagsClear(animationHandleData(second)), Containers::pair(NodeFlags{}, NodeFlag::FallthroughPointerEvents|NodeFlag::NoBlur));
    CORRADE_COMPARE(animator.hasRemoveNodeAfter(animationHandleData(second)), true);
    CORRADE_COMPARE(animator.easing(animationHandleData(second)), Animation::Easing::circularOut);

    /* Verifying more NaN combinations in getters ... */
    AnimationHandle third = animator.create(
        NodeAnimation{}
            .fromOffsetY(1.5f)
            .toOffset({2.5f, 3.5f})
            .toSizeX(4.5f),
        Animation::Easing::step, 14_nsec, 3_nsec, nodeHandle(0xecbda, 0x321), AnimationFlags{});
    CORRADE_COMPARE(animator.usedCount(), 3);
    CORRADE_COMPARE(animator.duration(third), 3_nsec);
    CORRADE_COMPARE(animator.repeatCount(third), 1);
    CORRADE_COMPARE(animator.flags(third), AnimationFlags{});
    CORRADE_COMPARE(animator.started(third), 14_nsec);
    CORRADE_COMPARE(animator.node(third), nodeHandle(0xecbda, 0x321));
    /* NaN comparison works only for scalars */
    CORRADE_COMPARE(animator.offsets(third).first().x(), Constants::nan());
    CORRADE_COMPARE(animator.offsets(third).first().y(), 1.5f);
    CORRADE_COMPARE(animator.offsets(third).second().x(), 2.5f);
    CORRADE_COMPARE(animator.offsets(third).second().y(), 3.5f);
    CORRADE_COMPARE(animator.sizes(third).first().x(), Constants::nan());
    CORRADE_COMPARE(animator.sizes(third).first().y(), Constants::nan());
    CORRADE_COMPARE(animator.sizes(third).second().x(), 4.5f);
    CORRADE_COMPARE(animator.sizes(third).second().y(), Constants::nan());
    CORRADE_COMPARE(animator.opacities(third).first(), Constants::nan());
    CORRADE_COMPARE(animator.opacities(third).second(), Constants::nan());
    CORRADE_COMPARE(animator.flagsAdd(third), Containers::pair(NodeFlags{}, NodeFlags{}));
    CORRADE_COMPARE(animator.flagsClear(third), Containers::pair(NodeFlags{}, NodeFlags{}));
    CORRADE_COMPARE(animator.hasRemoveNodeAfter(third), false);
    CORRADE_COMPARE(animator.easing(third), Animation::Easing::step);

    /* And more ... */
    AnimationHandle fourth = animator.create(
        NodeAnimation{}
            .fromSize({5.5f, 6.5f})
            .toOffsetX(7.5f)
            .toSizeY(8.5f),
        Animation::Easing::smootherstep, 40_nsec, 11_nsec, nodeHandle(0xfefe, 0x101), 33);
    CORRADE_COMPARE(animator.usedCount(), 4);
    CORRADE_COMPARE(animator.duration(fourth), 11_nsec);
    CORRADE_COMPARE(animator.repeatCount(fourth), 33);
    CORRADE_COMPARE(animator.flags(fourth), AnimationFlags{});
    CORRADE_COMPARE(animator.started(fourth), 40_nsec);
    CORRADE_COMPARE(animator.node(fourth), nodeHandle(0xfefe, 0x101));
    /* NaN comparison works only for scalars */
    CORRADE_COMPARE(animator.offsets(fourth).first().x(), Constants::nan());
    CORRADE_COMPARE(animator.offsets(fourth).first().y(), Constants::nan());
    CORRADE_COMPARE(animator.offsets(fourth).second().x(), 7.5f);
    CORRADE_COMPARE(animator.offsets(fourth).second().y(), Constants::nan());
    CORRADE_COMPARE(animator.sizes(fourth).first().x(), 5.5f);
    CORRADE_COMPARE(animator.sizes(fourth).first().y(), 6.5f);
    CORRADE_COMPARE(animator.sizes(fourth).second().x(), Constants::nan());
    CORRADE_COMPARE(animator.sizes(fourth).second().y(), 8.5f);
    CORRADE_COMPARE(animator.opacities(fourth).first(), Constants::nan());
    CORRADE_COMPARE(animator.opacities(fourth).second(), Constants::nan());
    CORRADE_COMPARE(animator.flagsAdd(fourth), Containers::pair(NodeFlags{}, NodeFlags{}));
    CORRADE_COMPARE(animator.flagsClear(fourth), Containers::pair(NodeFlags{}, NodeFlags{}));
    CORRADE_COMPARE(animator.hasRemoveNodeAfter(fourth), false);
    CORRADE_COMPARE(animator.easing(fourth), Animation::Easing::smootherstep);

    /* Only flags set in NodeAnimation, can omit the easing function. Using
       a null node handle, default repeat count and flags. */
    AnimationHandle fifth = animator.create(
        NodeAnimation{}
            .addFlagsBegin(NodeFlag::Clip|NodeFlag::NoEvents)
            .addFlagsEnd(NodeFlag::Hidden|NodeFlag::Disabled)
            .clearFlagsBegin(NodeFlag::Disabled|NodeFlag::Focusable)
            .clearFlagsEnd(NodeFlag::FallthroughPointerEvents|NodeFlag::NoBlur)
            .setRemoveNodeAfter(true),
        nullptr, 100_nsec, 10_nsec, NodeHandle::Null);
    CORRADE_COMPARE(animator.usedCount(), 5);
    CORRADE_COMPARE(animator.duration(fifth), 10_nsec);
    CORRADE_COMPARE(animator.repeatCount(fifth), 1);
    CORRADE_COMPARE(animator.flags(fifth), AnimationFlags{});
    CORRADE_COMPARE(animator.started(fifth), 100_nsec);
    CORRADE_COMPARE(animator.node(fifth), NodeHandle::Null);
    CORRADE_COMPARE(Math::isNan(animator.offsets(fifth).first()), BitVector2{3});
    CORRADE_COMPARE(Math::isNan(animator.offsets(fifth).second()), BitVector2{3});
    CORRADE_COMPARE(Math::isNan(animator.sizes(fifth).first()), BitVector2{3});
    CORRADE_COMPARE(Math::isNan(animator.sizes(fifth).second()), BitVector2{3});
    CORRADE_COMPARE(animator.opacities(fifth).first(), Constants::nan());
    CORRADE_COMPARE(animator.opacities(fifth).second(), Constants::nan());
    CORRADE_COMPARE(animator.flagsAdd(fifth), Containers::pair(NodeFlag::Clip|NodeFlag::NoEvents, NodeFlag::Hidden|NodeFlag::Disabled));
    CORRADE_COMPARE(animator.flagsClear(fifth), Containers::pair(NodeFlag::Disabled|NodeFlag::Focusable, NodeFlag::FallthroughPointerEvents|NodeFlag::NoBlur));
    CORRADE_COMPARE(animator.hasRemoveNodeAfter(fifth), true);
    CORRADE_COMPARE(animator.easing(fifth), nullptr);

    /* Nothing animated at all, the easing and node handle gets saved tho */
    AnimationHandle sixth = animator.create(
        NodeAnimation{},
        Animation::Easing::backIn, -50_nsec, 30_nsec, nodeHandle(0x12345, 0xabc), 20);
    CORRADE_COMPARE(animator.usedCount(), 6);
    CORRADE_COMPARE(animator.duration(sixth), 30_nsec);
    CORRADE_COMPARE(animator.repeatCount(sixth), 20);
    CORRADE_COMPARE(animator.flags(sixth), AnimationFlags{});
    CORRADE_COMPARE(animator.started(sixth), -50_nsec);
    CORRADE_COMPARE(animator.node(sixth), nodeHandle(0x12345, 0xabc));
    CORRADE_COMPARE(Math::isNan(animator.offsets(sixth).first()), BitVector2{3});
    CORRADE_COMPARE(Math::isNan(animator.offsets(sixth).second()), BitVector2{3});
    CORRADE_COMPARE(Math::isNan(animator.sizes(sixth).first()), BitVector2{3});
    CORRADE_COMPARE(Math::isNan(animator.sizes(sixth).second()), BitVector2{3});
    CORRADE_COMPARE(animator.opacities(sixth).first(), Constants::nan());
    CORRADE_COMPARE(animator.opacities(sixth).second(), Constants::nan());
    CORRADE_COMPARE(animator.flagsAdd(sixth), (Containers::Pair<NodeFlags, NodeFlags>{}));
    CORRADE_COMPARE(animator.flagsClear(sixth), (Containers::Pair<NodeFlags, NodeFlags>{}));
    CORRADE_COMPARE(animator.hasRemoveNodeAfter(sixth), false);
    CORRADE_COMPARE(animator.easing(sixth), Animation::Easing::backIn);

    /* There's no state to clean up, so just check that removal delegates
       correctly */
    animator.remove(second);
    animator.remove(animationHandleData(fifth));
    CORRADE_COMPARE(animator.usedCount(), 4);
    CORRADE_VERIFY(animator.isHandleValid(first));
    CORRADE_VERIFY(!animator.isHandleValid(second));
    CORRADE_VERIFY(animator.isHandleValid(third));
    CORRADE_VERIFY(animator.isHandleValid(fourth));
    CORRADE_VERIFY(!animator.isHandleValid(fifth));
    CORRADE_VERIFY(animator.isHandleValid(sixth));
}

void NodeAnimatorTest::createRemoveHandleRecycle() {
    NodeAnimator animator{animatorHandle(0, 1)};

    /* Allocate an animation that uses all properties */
    AnimationHandle first = animator.create(
        NodeAnimation{}
            .fromOffset({1.0f, 2.0f})
            .toOffset({3.0f, 4.0f})
            .fromSize({5.0f, 6.0f})
            .toSize({7.0f, 8.0f})
            .fromOpacity(0.25f)
            .toOpacity(0.75f)
            .addFlagsBegin(NodeFlag::Clip|NodeFlag::NoEvents)
            .clearFlagsBegin(NodeFlag::Disabled|NodeFlag::Focusable)
            .addFlagsEnd(NodeFlag::Hidden|NodeFlag::Disabled)
            .clearFlagsEnd(NodeFlag::FallthroughPointerEvents|NodeFlag::NoBlur)
            .setRemoveNodeAfter(true),
        Animation::Easing::bounceIn, 12_nsec, 13_nsec, nodeHandle(0xabcde, 0x123), 10, AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.node(first), nodeHandle(0xabcde, 0x123));
    CORRADE_COMPARE(animator.offsets(first), Containers::pair(Vector2{1.0f, 2.0f}, Vector2{3.0f, 4.0f}));
    CORRADE_COMPARE(animator.sizes(first), Containers::pair(Vector2{5.0f, 6.0f}, Vector2{7.0f, 8.0f}));
    CORRADE_COMPARE(animator.opacities(first), Containers::pair(0.25f, 0.75f));
    CORRADE_COMPARE(animator.flagsAdd(first), Containers::pair(NodeFlag::Clip|NodeFlag::NoEvents, NodeFlag::Hidden|NodeFlag::Disabled));
    CORRADE_COMPARE(animator.flagsClear(first), Containers::pair(NodeFlag::Disabled|NodeFlag::Focusable, NodeFlag::FallthroughPointerEvents|NodeFlag::NoBlur));
    CORRADE_COMPARE(animator.hasRemoveNodeAfter(first), true);
    CORRADE_COMPARE(animator.easing(first), Animation::Easing::bounceIn);

    /* Removal and new creation with no properties set should reuse the same
       slot and reset everything. What's handled by AbstractAnimator is tested
       well enough in AbstractAnimatorTest::createRemoveHandleRecycle(). */
    animator.remove(first);
    AnimationHandle first2 = animator.create(
        NodeAnimation{},
        nullptr, 12_nsec, 13_nsec, NodeHandle::Null, 10, AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animationHandleId(first2), animationHandleId(first));
    CORRADE_COMPARE(animator.node(first2), NodeHandle::Null);
    CORRADE_COMPARE(Math::isNan(animator.offsets(first2).first()), BitVector2{3});
    CORRADE_COMPARE(Math::isNan(animator.offsets(first2).second()), BitVector2{3});
    CORRADE_COMPARE(Math::isNan(animator.sizes(first2).first()), BitVector2{3});
    CORRADE_COMPARE(Math::isNan(animator.sizes(first2).second()), BitVector2{3});
    CORRADE_COMPARE(animator.opacities(first2).first(), Constants::nan());
    CORRADE_COMPARE(animator.opacities(first2).second(), Constants::nan());
    CORRADE_COMPARE(animator.flagsAdd(first2), (Containers::Pair<NodeFlags, NodeFlags>{}));
    CORRADE_COMPARE(animator.flagsClear(first2), (Containers::Pair<NodeFlags, NodeFlags>{}));
    CORRADE_COMPARE(animator.hasRemoveNodeAfter(first2), false);
    CORRADE_COMPARE(animator.easing(first2), nullptr);

    /* The other way around should work as well, i.e. the NaNs shouldn't get
       stuck from before but should be replaced with new values */
    animator.remove(first2);
    AnimationHandle first3 = animator.create(
        NodeAnimation{}
            .fromOffset({1.0f, 2.0f})
            .toOffset({3.0f, 4.0f})
            .fromSize({5.0f, 6.0f})
            .toSize({7.0f, 8.0f})
            .fromOpacity(0.25f)
            .toOpacity(0.75f)
            .addFlagsBegin(NodeFlag::Clip|NodeFlag::NoEvents)
            .clearFlagsBegin(NodeFlag::Disabled|NodeFlag::Focusable)
            .addFlagsEnd(NodeFlag::Hidden|NodeFlag::Disabled)
            .clearFlagsEnd(NodeFlag::FallthroughPointerEvents|NodeFlag::NoBlur)
            .setRemoveNodeAfter(true),
        Animation::Easing::bounceIn, 12_nsec, 13_nsec, nodeHandle(0xabcde, 0x123), 10, AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animationHandleId(first3), animationHandleId(first));
    CORRADE_COMPARE(animator.node(first3), nodeHandle(0xabcde, 0x123));
    CORRADE_COMPARE(animator.offsets(first3), Containers::pair(Vector2{1.0f, 2.0f}, Vector2{3.0f, 4.0f}));
    CORRADE_COMPARE(animator.sizes(first3), Containers::pair(Vector2{5.0f, 6.0f}, Vector2{7.0f, 8.0f}));
    CORRADE_COMPARE(animator.opacities(first3), Containers::pair(0.25f, 0.75f));
    CORRADE_COMPARE(animator.flagsAdd(first3), Containers::pair(NodeFlag::Clip|NodeFlag::NoEvents, NodeFlag::Hidden|NodeFlag::Disabled));
    CORRADE_COMPARE(animator.flagsClear(first3), Containers::pair(NodeFlag::Disabled|NodeFlag::Focusable, NodeFlag::FallthroughPointerEvents|NodeFlag::NoBlur));
    CORRADE_COMPARE(animator.hasRemoveNodeAfter(first3), true);
    CORRADE_COMPARE(animator.easing(first3), Animation::Easing::bounceIn);
}

void NodeAnimatorTest::createInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    NodeAnimator animator{animatorHandle(0, 1)};

    /* Null easing if nothing is interpolated is fine. Tested more thoroughly
       in createRemove() above. */
    animator.create(NodeAnimation{}, nullptr, 0_nsec, 1_nsec, NodeHandle::Null);

    Containers::String out;
    Error redirectError{&out};
    animator.create(NodeAnimation{}.fromSize({}), nullptr, 0_nsec, 1_nsec, NodeHandle::Null);
    animator.create(NodeAnimation{}.fromSizeX({}), nullptr, 0_nsec, 1_nsec, NodeHandle::Null);
    animator.create(NodeAnimation{}.fromSizeY({}), nullptr, 0_nsec, 1_nsec, NodeHandle::Null);
    animator.create(NodeAnimation{}.toSize({}), nullptr, 0_nsec, 1_nsec, NodeHandle::Null);
    animator.create(NodeAnimation{}.toSizeX({}), nullptr, 0_nsec, 1_nsec, NodeHandle::Null);
    animator.create(NodeAnimation{}.toSizeY({}), nullptr, 0_nsec, 1_nsec, NodeHandle::Null);
    animator.create(NodeAnimation{}.fromOffset({}), nullptr, 0_nsec, 1_nsec, NodeHandle::Null);
    animator.create(NodeAnimation{}.fromOffsetX({}), nullptr, 0_nsec, 1_nsec, NodeHandle::Null);
    animator.create(NodeAnimation{}.fromOffsetY({}), nullptr, 0_nsec, 1_nsec, NodeHandle::Null);
    animator.create(NodeAnimation{}.toOffset({}), nullptr, 0_nsec, 1_nsec, NodeHandle::Null);
    animator.create(NodeAnimation{}.toOffsetX({}), nullptr, 0_nsec, 1_nsec, NodeHandle::Null);
    animator.create(NodeAnimation{}.toOffsetY({}), nullptr, 0_nsec, 1_nsec, NodeHandle::Null);
    animator.create(NodeAnimation{}.fromOpacity({}), nullptr, 0_nsec, 1_nsec, NodeHandle::Null);
    animator.create(NodeAnimation{}.toOpacity({}), nullptr, 0_nsec, 1_nsec, NodeHandle::Null);
    CORRADE_COMPARE_AS(out,
        "Ui::NodeAnimator::create(): easing expected to be non-null if animating offset, size or opacity\n"
        "Ui::NodeAnimator::create(): easing expected to be non-null if animating offset, size or opacity\n"
        "Ui::NodeAnimator::create(): easing expected to be non-null if animating offset, size or opacity\n"
        "Ui::NodeAnimator::create(): easing expected to be non-null if animating offset, size or opacity\n"
        "Ui::NodeAnimator::create(): easing expected to be non-null if animating offset, size or opacity\n"
        "Ui::NodeAnimator::create(): easing expected to be non-null if animating offset, size or opacity\n"
        "Ui::NodeAnimator::create(): easing expected to be non-null if animating offset, size or opacity\n"
        "Ui::NodeAnimator::create(): easing expected to be non-null if animating offset, size or opacity\n"
        "Ui::NodeAnimator::create(): easing expected to be non-null if animating offset, size or opacity\n"
        "Ui::NodeAnimator::create(): easing expected to be non-null if animating offset, size or opacity\n"
        "Ui::NodeAnimator::create(): easing expected to be non-null if animating offset, size or opacity\n"
        "Ui::NodeAnimator::create(): easing expected to be non-null if animating offset, size or opacity\n"
        "Ui::NodeAnimator::create(): easing expected to be non-null if animating offset, size or opacity\n"
        "Ui::NodeAnimator::create(): easing expected to be non-null if animating offset, size or opacity\n",
        TestSuite::Compare::String);
}

void NodeAnimatorTest::propertiesInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    NodeAnimator animator{animatorHandle(0, 1)};

    AnimationHandle handle = animator.create(
        NodeAnimation{},
        nullptr, 0_nsec, 0_nsec, NodeHandle::Null);

    Containers::String out;
    Error redirectError{&out};
    animator.offsets(AnimationHandle::Null);
    animator.sizes(AnimationHandle::Null);
    animator.opacities(AnimationHandle::Null);
    animator.flagsAdd(AnimationHandle::Null);
    animator.flagsClear(AnimationHandle::Null);
    animator.hasRemoveNodeAfter(AnimationHandle::Null);
    animator.easing(AnimationHandle::Null);
    /* Valid animator, invalid data */
    animator.offsets(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)));
    animator.sizes(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)));
    animator.opacities(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)));
    animator.flagsAdd(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)));
    animator.flagsClear(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)));
    animator.hasRemoveNodeAfter(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)));
    animator.easing(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)));
    /* Invalid animator, valid data */
    animator.offsets(animationHandle(AnimatorHandle::Null, animationHandleData(handle)));
    animator.sizes(animationHandle(AnimatorHandle::Null, animationHandleData(handle)));
    animator.opacities(animationHandle(AnimatorHandle::Null, animationHandleData(handle)));
    animator.flagsAdd(animationHandle(AnimatorHandle::Null, animationHandleData(handle)));
    animator.flagsClear(animationHandle(AnimatorHandle::Null, animationHandleData(handle)));
    animator.hasRemoveNodeAfter(animationHandle(AnimatorHandle::Null, animationHandleData(handle)));
    animator.easing(animationHandle(AnimatorHandle::Null, animationHandleData(handle)));
    /* AnimatorDataHandle directly */
    animator.offsets(AnimatorDataHandle(0x123abcde));
    animator.sizes(AnimatorDataHandle(0x123abcde));
    animator.opacities(AnimatorDataHandle(0x123abcde));
    animator.flagsAdd(AnimatorDataHandle(0x123abcde));
    animator.flagsClear(AnimatorDataHandle(0x123abcde));
    animator.hasRemoveNodeAfter(AnimatorDataHandle(0x123abcde));
    animator.easing(AnimatorDataHandle(0x123abcde));
    CORRADE_COMPARE_AS(out,
        "Ui::NodeAnimator::offsets(): invalid handle Ui::AnimationHandle::Null\n"
        "Ui::NodeAnimator::sizes(): invalid handle Ui::AnimationHandle::Null\n"
        "Ui::NodeAnimator::opacities(): invalid handle Ui::AnimationHandle::Null\n"
        "Ui::NodeAnimator::flagsAdd(): invalid handle Ui::AnimationHandle::Null\n"
        "Ui::NodeAnimator::flagsClear(): invalid handle Ui::AnimationHandle::Null\n"
        "Ui::NodeAnimator::hasRemoveNodeAfter(): invalid handle Ui::AnimationHandle::Null\n"
        "Ui::NodeAnimator::easing(): invalid handle Ui::AnimationHandle::Null\n"

        "Ui::NodeAnimator::offsets(): invalid handle Ui::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Ui::NodeAnimator::sizes(): invalid handle Ui::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Ui::NodeAnimator::opacities(): invalid handle Ui::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Ui::NodeAnimator::flagsAdd(): invalid handle Ui::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Ui::NodeAnimator::flagsClear(): invalid handle Ui::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Ui::NodeAnimator::hasRemoveNodeAfter(): invalid handle Ui::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Ui::NodeAnimator::easing(): invalid handle Ui::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"

        "Ui::NodeAnimator::offsets(): invalid handle Ui::AnimationHandle(Null, {0x0, 0x1})\n"
        "Ui::NodeAnimator::sizes(): invalid handle Ui::AnimationHandle(Null, {0x0, 0x1})\n"
        "Ui::NodeAnimator::opacities(): invalid handle Ui::AnimationHandle(Null, {0x0, 0x1})\n"
        "Ui::NodeAnimator::flagsAdd(): invalid handle Ui::AnimationHandle(Null, {0x0, 0x1})\n"
        "Ui::NodeAnimator::flagsClear(): invalid handle Ui::AnimationHandle(Null, {0x0, 0x1})\n"
        "Ui::NodeAnimator::hasRemoveNodeAfter(): invalid handle Ui::AnimationHandle(Null, {0x0, 0x1})\n"
        "Ui::NodeAnimator::easing(): invalid handle Ui::AnimationHandle(Null, {0x0, 0x1})\n"

        "Ui::NodeAnimator::offsets(): invalid handle Ui::AnimatorDataHandle(0xabcde, 0x123)\n"
        "Ui::NodeAnimator::sizes(): invalid handle Ui::AnimatorDataHandle(0xabcde, 0x123)\n"
        "Ui::NodeAnimator::opacities(): invalid handle Ui::AnimatorDataHandle(0xabcde, 0x123)\n"
        "Ui::NodeAnimator::flagsAdd(): invalid handle Ui::AnimatorDataHandle(0xabcde, 0x123)\n"
        "Ui::NodeAnimator::flagsClear(): invalid handle Ui::AnimatorDataHandle(0xabcde, 0x123)\n"
        "Ui::NodeAnimator::hasRemoveNodeAfter(): invalid handle Ui::AnimatorDataHandle(0xabcde, 0x123)\n"
        "Ui::NodeAnimator::easing(): invalid handle Ui::AnimatorDataHandle(0xabcde, 0x123)\n",
        TestSuite::Compare::String);
}

void NodeAnimatorTest::advance() {
    /* Tests mainly behavior of advancing just active animations and
       propagating the removed etc. status correctly. The particular values are
       tested extensively in advanceProperties(). */

    NodeAnimator animator{animatorHandle(0, 1)};

    /* This one interpolates size and offset, adds flags at the beginning and
       clears the same flags at the end, and marks the node for deletion at the
       end */
    AnimationHandle playing = animator.create(
        NodeAnimation{}
            .fromOffset({-10.0f, -100.0f})
            .toOffset({10.0f, 100.0f})
            .fromSize({0.0f, 5.0f})
            .toSize({5.0f, 0.0f})
            .addFlagsBegin(NodeFlag::Focusable|NodeFlag::Clip)
            .clearFlagsEnd(NodeFlag::Focusable|NodeFlag::Clip)
            .setRemoveNodeAfter(true),
        /* An easing that goes in reverse to verify it's being used */
        [](Float a) { return 1.0f - a; }, 5_nsec, 20_nsec, nodeHandle(2, 0xacf));
    /* This one performs both begin and end flag adjustment at once, and sets
       opacity to the final value */
    AnimationHandle stopped = animator.create(
        NodeAnimation{}
            .fromOpacity(0.25f)
            .toOpacity(0.75f)
            .clearFlagsBegin(NodeFlag::Focusable)
            .addFlagsEnd(NodeFlag::NoBlur),
        /** @todo In order to correctly have the animation marked as `started`
            and properly have the start flags etc. applied, it currently has to
            start at time that's greater than the UI animationTime() default
            0_nsec. Revert back to 0_nsec once this is fixed in the UI
            itself. */
        Animation::Easing::cubicOut, 1_nsec, 1_nsec, nodeHandle(4, 0x113));
    /* This one is a variant of the first, scheduled later and not attached to
       any node, thus it never marks any updates */
    AnimationHandle scheduledNullNode = animator.create(
        NodeAnimation{}
            .fromOffset({-10.0f, -100.0f})
            .fromSize({0.0f, 5.0f})
            .setRemoveNodeAfter(true),
        Animation::Easing::linear, 20_nsec, 10_nsec, NodeHandle::Null);
    /* This one interpolates all the way and stays */
    AnimationHandle stoppedKept = animator.create(
        NodeAnimation{}
            .toOffset({3.0f, 333.0f})
            .toSize({33.0f, 3333.0f}),
        /** @todo Same as above, in order to correctly have the animation
            marked as `started` and properly have the fromOffset / fromSize
            fetched, it currently has to start at time that's greater than the
            UI animationTime() default 0_nsec. Without that, the interpolation
            is done from a random value, which could be a NaN, causing the test
            to fail. Revert back to 0_nsec once this is fixed in the UI
            itself. */
        Animation::Easing::cubicIn, 1_nsec, 1_nsec, nodeHandle(1, 0xaca), AnimationFlag::KeepOncePlayed);

    /* Does what UI's advanceAnimations() is doing internally for all animators
       (as we need to test also the interaction with animation being removed,
       etc.), but with an ability to peek into the filled data to verify
       they're written only when they should be. UI's advanceAnimations() is
       then tested in uiAdvance() below. */
    const auto advance = [&](Nanoseconds time, const Containers::StridedArrayView1D<Vector2>& nodeOffsets, const Containers::StridedArrayView1D<Vector2>& nodeSizes, const Containers::StridedArrayView1D<Float>& nodeOpacities, const Containers::StridedArrayView1D<NodeFlags>& nodeFlags, const Containers::MutableBitArrayView& nodesRemove) {
        UnsignedByte activeData[1];
        Containers::MutableBitArrayView active{activeData, 0, 4};
        UnsignedByte startedData[1];
        Containers::MutableBitArrayView started{startedData, 0, 4};
        UnsignedByte stoppedData[1];
        Containers::MutableBitArrayView stopped{stoppedData, 0, 4};
        Float factors[4];
        UnsignedByte removeData[1];
        Containers::MutableBitArrayView remove{removeData, 0, 4};

        Containers::Pair<bool, bool> needsAdvanceClean = animator.update(time, active, started, stopped, factors, remove);
        NodeAnimatorUpdates updates;
        if(needsAdvanceClean.first())
            updates = animator.advance(active, started, stopped, factors, nodeOffsets, nodeSizes, nodeOpacities, nodeFlags, nodesRemove);
        if(needsAdvanceClean.second())
            animator.clean(remove);
        return updates;
    };

    /* Advancing to 10 sets begin flags for the playing animation and
       interpolates its offset and size. For the stopped & removed animation it
       performs both begin and end flag changes and sets the final opacity, for
       the stopped & kept it uses just the final offset and size */
    {
        Containers::StaticArray<5, Vector2> nodeOffsets{DirectInit, Vector2{-999.9f}};
        Containers::StaticArray<5, Vector2> nodeSizes{DirectInit, Vector2{-999.9f}};
        Containers::StaticArray<5, Float> nodeOpacities{DirectInit, -999.9f};
        Containers::StaticArray<5, NodeFlags> nodeFlags{{
            ~NodeFlags{},
            ~NodeFlags{},
            {},
            ~NodeFlags{},
            ~NodeFlag::NoBlur,
        }};
        Containers::BitArray nodesRemove{ValueInit, 5};

        CORRADE_COMPARE(advance(10_nsec, nodeOffsets, nodeSizes, nodeOpacities, nodeFlags, nodesRemove), NodeAnimatorUpdate::OffsetSize|NodeAnimatorUpdate::Opacity|NodeAnimatorUpdate::Enabled|NodeAnimatorUpdate::EventMask|NodeAnimatorUpdate::Clip);
        CORRADE_VERIFY(animator.isHandleValid(playing));
        CORRADE_VERIFY(!animator.isHandleValid(stopped));
        CORRADE_VERIFY(animator.isHandleValid(scheduledNullNode));
        CORRADE_VERIFY(animator.isHandleValid(stoppedKept));
        CORRADE_COMPARE(animator.state(playing), AnimationState::Playing);
        CORRADE_COMPARE(animator.state(scheduledNullNode), AnimationState::Scheduled);
        CORRADE_COMPARE(animator.state(stoppedKept), AnimationState::Stopped);
        CORRADE_COMPARE_AS(nodeOffsets, Containers::arrayView<Vector2>({
            Vector2{-999.9f},
            {3.0f, 333.0f},     /* changed by stoppedKept */
            {5.0f, 50.0f},      /* changed by playing (easing in reverse) */
            Vector2{-999.9f},
            Vector2{-999.9f},
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(nodeSizes, Containers::arrayView<Vector2>({
            Vector2{-999.9f},
            {33.0f, 3333.0f},   /* changed by stoppedKept */
            {3.75f, 1.25f},     /* changed by playing (easing in reverse) */
            Vector2{-999.9f},
            Vector2{-999.9f},
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(nodeOpacities, Containers::arrayView({
            -999.9f,
            -999.9f,
            -999.9f,
            -999.9f,
            0.75f,              /* changed by stopped */
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(nodeFlags, Containers::arrayView<NodeFlags>({
            ~NodeFlags{},
            ~NodeFlags{},
            NodeFlag::Focusable|NodeFlag::Clip,
            ~NodeFlags{},
            ~NodeFlag::Focusable, /* replaced from ~NoBlur by stopped */
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(Containers::BitArrayView{nodesRemove}, Containers::stridedArrayView({
            false,
            false,
            false,
            false,
            false,
        }).sliceBit(0), TestSuite::Compare::Container);

    /* Advancing to 15 changes just the offset/size to a 50% interpolation,
       nothing else. In particular, the flags or opacities aren't touched even
       though they're now different. */
    } {
        Containers::StaticArray<5, Vector2> nodeOffsets{DirectInit, Vector2{-999.9f}};
        Containers::StaticArray<5, Vector2> nodeSizes{DirectInit, Vector2{-999.9f}};
        Containers::StaticArray<5, Float> nodeOpacities{DirectInit, -999.9f};
        Containers::StaticArray<5, NodeFlags> nodeFlags{{
            ~NodeFlags{},
            ~NodeFlags{},
            ~NodeFlags{},
            ~NodeFlags{},
            ~NodeFlags{},
        }};
        Containers::BitArray nodesRemove{ValueInit, 5};

        CORRADE_COMPARE(advance(15_nsec, nodeOffsets, nodeSizes, nodeOpacities, nodeFlags, nodesRemove), NodeAnimatorUpdate::OffsetSize);
        CORRADE_VERIFY(animator.isHandleValid(playing));
        CORRADE_VERIFY(animator.isHandleValid(scheduledNullNode));
        CORRADE_VERIFY(animator.isHandleValid(stoppedKept));
        CORRADE_COMPARE(animator.state(playing), AnimationState::Playing);
        CORRADE_COMPARE(animator.state(scheduledNullNode), AnimationState::Scheduled);
        CORRADE_COMPARE(animator.state(stoppedKept), AnimationState::Stopped);
        CORRADE_COMPARE_AS(nodeOffsets, Containers::arrayView<Vector2>({
            Vector2{-999.9f},
            Vector2{-999.9f},
            {0.0f, 0.0f},       /* changed by playing */
            Vector2{-999.9f},
            Vector2{-999.9f},
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(nodeSizes, Containers::arrayView<Vector2>({
            Vector2{-999.9f},
            Vector2{-999.9f},
            {2.5f, 2.5f},       /* changed by playing */
            Vector2{-999.9f},
            Vector2{-999.9f},
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(nodeOpacities, Containers::arrayView({
            -999.9f,
            -999.9f,
            -999.9f,
            -999.9f,
            -999.9f,
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(nodeFlags, Containers::arrayView<NodeFlags>({
            ~NodeFlags{},
            ~NodeFlags{},
            ~NodeFlags{},
            ~NodeFlags{},
            ~NodeFlags{},
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(Containers::BitArrayView{nodesRemove}, Containers::stridedArrayView({
            false,
            false,
            false,
            false,
            false,
        }).sliceBit(0), TestSuite::Compare::Container);

    /* Advancing to 20 plays also the scheduled animation without a node
       attachment, but as there's no node to write the data to it's a no-op,
       so it's again just the `playing` animation changing things */
    } {
        Containers::StaticArray<5, Vector2> nodeOffsets{DirectInit, Vector2{-999.9f}};
        Containers::StaticArray<5, Vector2> nodeSizes{DirectInit, Vector2{-999.9f}};
        Containers::StaticArray<5, Float> nodeOpacities{DirectInit, -999.9f};
        Containers::StaticArray<5, NodeFlags> nodeFlags{{
            ~NodeFlags{},
            ~NodeFlags{},
            ~NodeFlags{},
            ~NodeFlags{},
            ~NodeFlags{},
        }};
        Containers::BitArray nodesRemove{ValueInit, 5};

        CORRADE_COMPARE(advance(20_nsec, nodeOffsets, nodeSizes, nodeOpacities, nodeFlags, nodesRemove), NodeAnimatorUpdate::OffsetSize);
        CORRADE_VERIFY(animator.isHandleValid(playing));
        CORRADE_VERIFY(animator.isHandleValid(scheduledNullNode));
        CORRADE_VERIFY(animator.isHandleValid(stoppedKept));
        CORRADE_COMPARE(animator.state(playing), AnimationState::Playing);
        CORRADE_COMPARE(animator.state(scheduledNullNode), AnimationState::Playing);
        CORRADE_COMPARE(animator.state(stoppedKept), AnimationState::Stopped);
        CORRADE_COMPARE_AS(nodeOffsets, Containers::arrayView<Vector2>({
            Vector2{-999.9f},
            Vector2{-999.9f},
            {-5.0f, -50.0f},    /* changed by playing (easing in reverse) */
            Vector2{-999.9f},
            Vector2{-999.9f},
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(nodeSizes, Containers::arrayView<Vector2>({
            Vector2{-999.9f},
            Vector2{-999.9f},
            {1.25f, 3.75f},     /* changed by playing (easing in reverse) */
            Vector2{-999.9f},
            Vector2{-999.9f},
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(nodeOpacities, Containers::arrayView({
            -999.9f,
            -999.9f,
            -999.9f,
            -999.9f,
            -999.9f,
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(nodeFlags, Containers::arrayView<NodeFlags>({
            ~NodeFlags{},
            ~NodeFlags{},
            ~NodeFlags{},
            ~NodeFlags{},
            ~NodeFlags{},
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(Containers::BitArrayView{nodesRemove}, Containers::stridedArrayView({
            false,
            false,
            false,
            false,
            false,
        }).sliceBit(0), TestSuite::Compare::Container);

    /* Advancing to 25 stops the first animation, applying the final flags. It
       marks both the animation and the node for removal. */
    } {
        Containers::StaticArray<5, Vector2> nodeOffsets{DirectInit, Vector2{-999.9f}};
        Containers::StaticArray<5, Vector2> nodeSizes{DirectInit, Vector2{-999.9f}};
        Containers::StaticArray<5, Float> nodeOpacities{DirectInit, -999.9f};
        Containers::StaticArray<5, NodeFlags> nodeFlags{{
            ~NodeFlags{},
            ~NodeFlags{},
            ~NodeFlags{},
            ~NodeFlags{},
            ~NodeFlags{},
        }};
        Containers::BitArray nodesRemove{ValueInit, 5};

        CORRADE_COMPARE(advance(25_nsec, nodeOffsets, nodeSizes, nodeOpacities, nodeFlags, nodesRemove), NodeAnimatorUpdate::OffsetSize|NodeAnimatorUpdate::Enabled|NodeAnimatorUpdate::Clip|NodeAnimatorUpdate::Removal);
        CORRADE_VERIFY(!animator.isHandleValid(playing));
        CORRADE_VERIFY(animator.isHandleValid(scheduledNullNode));
        CORRADE_VERIFY(animator.isHandleValid(stoppedKept));
        CORRADE_COMPARE(animator.state(scheduledNullNode), AnimationState::Playing);
        CORRADE_COMPARE(animator.state(stoppedKept), AnimationState::Stopped);
        CORRADE_COMPARE_AS(nodeOffsets, Containers::arrayView<Vector2>({
            Vector2{-999.9f},
            Vector2{-999.9f},
            {-10.0f, -100.0f},  /* changed by playing (easing in reverse) */
            Vector2{-999.9f},
            Vector2{-999.9f},
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(nodeSizes, Containers::arrayView<Vector2>({
            Vector2{-999.9f},
            Vector2{-999.9f},
            {0.0f, 5.0f},       /* changed by playing (easing in reverse) */
            Vector2{-999.9f},
            Vector2{-999.9f},
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(nodeOpacities, Containers::arrayView({
            -999.9f,
            -999.9f,
            -999.9f,
            -999.9f,
            -999.9f,
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(nodeFlags, Containers::arrayView<NodeFlags>({
            ~NodeFlags{},
            ~NodeFlags{},
            ~(NodeFlag::Focusable|NodeFlag::Clip),
            ~NodeFlags{},
            ~NodeFlags{},
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(Containers::BitArrayView{nodesRemove}, Containers::stridedArrayView({
            false,
            false,
            true,
            false,
            false,
        }).sliceBit(0), TestSuite::Compare::Container);

    /* Advancing to 30 stops the null node animation, but it again results in
       nothing besides it being removed. There's nothing else that would change
       anything. */
    } {
        Containers::StaticArray<5, Vector2> nodeOffsets{DirectInit, Vector2{-999.9f}};
        Containers::StaticArray<5, Vector2> nodeSizes{DirectInit, Vector2{-999.9f}};
        Containers::StaticArray<5, Float> nodeOpacities{DirectInit, -999.9f};
        Containers::StaticArray<5, NodeFlags> nodeFlags{{
            ~NodeFlags{},
            ~NodeFlags{},
            ~NodeFlags{},
            ~NodeFlags{},
            ~NodeFlags{},
        }};
        Containers::BitArray nodesRemove{ValueInit, 5};

        CORRADE_COMPARE(advance(30_nsec, nodeOffsets, nodeSizes, nodeOpacities, nodeFlags, nodesRemove), NodeAnimatorUpdates{});
        CORRADE_VERIFY(!animator.isHandleValid(scheduledNullNode));
        CORRADE_VERIFY(animator.isHandleValid(stoppedKept));
        CORRADE_COMPARE(animator.state(stoppedKept), AnimationState::Stopped);
        CORRADE_COMPARE_AS(nodeOffsets, Containers::arrayView<Vector2>({
            Vector2{-999.9f},
            Vector2{-999.9f},
            Vector2{-999.9f},
            Vector2{-999.9f},
            Vector2{-999.9f},
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(nodeSizes, Containers::arrayView<Vector2>({
            Vector2{-999.9f},
            Vector2{-999.9f},
            Vector2{-999.9f},
            Vector2{-999.9f},
            Vector2{-999.9f},
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(nodeOpacities, Containers::arrayView({
            -999.9f,
            -999.9f,
            -999.9f,
            -999.9f,
            -999.9f,
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(nodeFlags, Containers::arrayView<NodeFlags>({
            ~NodeFlags{},
            ~NodeFlags{},
            ~NodeFlags{},
            ~NodeFlags{},
            ~NodeFlags{},
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(Containers::BitArrayView{nodesRemove}, Containers::stridedArrayView({
            false,
            false,
            false,
            false,
            false,
        }).sliceBit(0), TestSuite::Compare::Container);

    /* Advancing to 35 does nothing at all */
    } {
        Containers::StaticArray<5, Vector2> nodeOffsets{DirectInit, Vector2{-999.9f}};
        Containers::StaticArray<5, Vector2> nodeSizes{DirectInit, Vector2{-999.9f}};
        Containers::StaticArray<5, Float> nodeOpacities{DirectInit, -999.9f};
        Containers::StaticArray<5, NodeFlags> nodeFlags{{
            ~NodeFlags{},
            ~NodeFlags{},
            ~NodeFlags{},
            ~NodeFlags{},
            ~NodeFlags{},
        }};
        Containers::BitArray nodesRemove{ValueInit, 5};

        CORRADE_COMPARE(advance(35_nsec, nodeOffsets, nodeSizes, nodeOpacities, nodeFlags, nodesRemove), NodeAnimatorUpdates{});
        CORRADE_VERIFY(!animator.isHandleValid(scheduledNullNode));
        CORRADE_VERIFY(animator.isHandleValid(stoppedKept));
        CORRADE_COMPARE(animator.state(stoppedKept), AnimationState::Stopped);
        CORRADE_COMPARE_AS(nodeOffsets, Containers::arrayView<Vector2>({
            Vector2{-999.9f},
            Vector2{-999.9f},
            Vector2{-999.9f},
            Vector2{-999.9f},
            Vector2{-999.9f},
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(nodeSizes, Containers::arrayView<Vector2>({
            Vector2{-999.9f},
            Vector2{-999.9f},
            Vector2{-999.9f},
            Vector2{-999.9f},
            Vector2{-999.9f},
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(nodeOpacities, Containers::arrayView({
            -999.9f,
            -999.9f,
            -999.9f,
            -999.9f,
            -999.9f,
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(nodeFlags, Containers::arrayView<NodeFlags>({
            ~NodeFlags{},
            ~NodeFlags{},
            ~NodeFlags{},
            ~NodeFlags{},
            ~NodeFlags{},
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(Containers::BitArrayView{nodesRemove}, Containers::stridedArrayView({
            false,
            false,
            false,
            false,
            false,
        }).sliceBit(0), TestSuite::Compare::Container);
    }
}

void NodeAnimatorTest::advanceProperties() {
    auto&& data = AdvancePropertiesData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    NodeAnimator animator{animatorHandle(0, 1)};
    animator.create(data.animation, Animation::Easing::linear, 5_nsec, 20_nsec, nodeHandle(2, 0xcac));

    UnsignedByte activeData[1];
    Containers::MutableBitArrayView active{activeData, 0, 1};
    UnsignedByte startedData[1];
    Containers::MutableBitArrayView started{startedData, 0, 1};
    UnsignedByte stoppedData[1];
    Containers::MutableBitArrayView stopped{stoppedData, 0, 1};
    Float factors[1];
    UnsignedByte removeData[1];
    Containers::MutableBitArrayView remove{removeData, 0, 1};
    CORRADE_VERIFY(animator.update(data.advance, active, started, stopped, factors, remove).first());

    Vector2 nodeOffsets[]{
        {},
        {},
        {100.0f, 100.0f},
    };
    Vector2 nodeSizes[]{
        {},
        {},
        {10.0f, 10.0f}
    };
    Float nodeOpacities[]{
        0.0f,
        0.0f,
        1.0f
    };
    NodeFlags nodeFlags[]{
        {},
        {},
        data.initialFlags
    };
    Containers::BitArray nodesRemove{ValueInit, 3};
    CORRADE_COMPARE(animator.advance(active, started, stopped, factors, nodeOffsets, nodeSizes, nodeOpacities, nodeFlags, nodesRemove), data.expectedUpdates);
    CORRADE_COMPARE(nodeOffsets[2], data.expectedOffset);
    CORRADE_COMPARE(nodeSizes[2], data.expectedSize);
    CORRADE_COMPARE(nodeOpacities[2], data.expectedOpacity);
    CORRADE_COMPARE(nodeFlags[2], data.expectedFlags);
    CORRADE_COMPARE(nodesRemove[2], data.expectedRemove);
}

void NodeAnimatorTest::advanceEmpty() {
    /* This should work */
    NodeAnimator animator{animatorHandle(0, 1)};
    animator.advance({}, {}, {}, {}, {}, {}, {}, {}, {});

    CORRADE_VERIFY(true);
}

void NodeAnimatorTest::uiAdvance() {
    auto&& data = UiAdvanceData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Just an integration test verifying that it all comes together. Detailed
       test for node animators being used by the UI is in AbstractAnimatorTest,
       detailed behavior of all properties is tested in advanceProperties(). */

    AbstractUserInterface ui{{100, 100}};

    ui.createNode({}, {});
    ui.createNode({}, {});
    NodeHandle node = ui.createNode({20.0f, 30.0f}, {80.0f, 100.0f});

    NodeAnimator& animator = ui.setNodeAnimatorInstance(Containers::pointer<NodeAnimator>(ui.createAnimator()));

    ui.update();
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

    animator.create(data.animation, Animation::Easing::linear, 5_nsec, 10_nsec, node);

    ui.advanceAnimations(10_nsec);
    CORRADE_VERIFY(ui.isHandleValid(node));
    CORRADE_COMPARE(ui.nodeOffset(node), data.expectedOffset);
    CORRADE_COMPARE(ui.nodeSize(node), data.expectedSize);
    CORRADE_COMPARE(ui.nodeOpacity(node), data.expectedOpacity);
    CORRADE_COMPARE(ui.nodeFlags(node), data.expectedFlags);
    CORRADE_COMPARE(ui.state(), data.expectedStates|UserInterfaceState::NeedsAnimationAdvance);

    ui.update();
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsAnimationAdvance);

    ui.advanceAnimations(20_nsec);
    CORRADE_COMPARE(ui.isHandleValid(node), !data.expectNodeRemovedEnd);
    if(!data.expectNodeRemovedEnd)
        CORRADE_COMPARE(ui.nodeFlags(node), data.expectedFlagsEnd);
    CORRADE_COMPARE(ui.state(), data.expectedStates|data.expectedExtraStatesEnd);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::NodeAnimatorTest)
