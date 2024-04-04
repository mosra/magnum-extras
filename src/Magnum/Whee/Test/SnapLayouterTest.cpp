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
#include <Corrade/Containers/ArrayView.h>
#include <Corrade/Containers/StringStl.h> /** @todo remove once Debug is stream-free */
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/Utility/DebugStl.h> /** @todo remove once Debug is stream-free */

#include "Magnum/Whee/SnapLayouter.h"
#include "Magnum/Whee/Implementation/snapLayouter.h"

namespace Magnum { namespace Whee { namespace Test { namespace {

struct SnapLayouterTest: TestSuite::Tester {
    explicit SnapLayouterTest();

    void debugSnap();
    void debugSnapPacked();
    void debugSnaps();
    void debugSnapsPacked();
    void debugSnapsSupersets();

    void snap();
};

const Vector2 Size{20.0f, 30.0f};

const struct {
    Snaps snaps;
    Vector2 expectedOffset, expectedSize;
} SnapRectData[]{
    /*     100   500
        200 +-----+                   margin
          BD|F    |                     3
            | CN  |             +---------------+
          O | PU  |             |       5       |
            |   I |           7 | 10 padding 15 | 7
            |    L|JH           |       25      |
        500 +-----+             +---------------+
                 K  T                   3
                 G  S                                */
    {Snap::Top|Snap::Left|Snap::InsideX,
        {100.0f, 167.0f}, Size},                            /* A */
    {Snap::Top|Snap::Left|Snap::NoSpaceX|Snap::InsideX,
        {100.0f, 167.0f}, Size},                            /* A again */
    {Snap::Top|Snap::Left|Snap::InsideY,
        {73.0f, 200.0f}, Size},                             /* B */
    {Snap::Top|Snap::Left|Snap::NoSpaceY|Snap::InsideY,
        {73.0f, 200.0f}, Size},                             /* B again */
    {Snap::Top|Snap::Left|Snap::InsideX|Snap::InsideY,
        {110.0f, 205.0f}, Size},                            /* C */
    {Snap::Top|Snap::Left|Snap::NoSpaceX|Snap::InsideY,
        {80.0f, 200.0f}, Size},                             /* D */
    {Snap::Top|Snap::Left|Snap::NoSpaceY|Snap::InsideX,
        {100.0f, 170.0f}, Size},                            /* E */
    {Snap::Top|Snap::Left|Snap::NoSpaceX|Snap::NoSpaceY|Snap::InsideX|Snap::InsideY,
        {100.0f, 200.0f}, Size},                            /* F */
    {Snap::Bottom|Snap::Right|Snap::InsideX,
        {480.0f, 503.0f}, Size},                            /* G */
    {Snap::Bottom|Snap::Right|Snap::NoSpaceX|Snap::InsideX,
        {480.0f, 503.0f}, Size},                            /* G again */
    {Snap::Bottom|Snap::Right|Snap::InsideY,
        {507.0f, 470.0f}, Size},                            /* H */
    {Snap::Bottom|Snap::Right|Snap::NoSpaceY|Snap::InsideY,
        {507.0f, 470.0f}, Size},                            /* H again */
    {Snap::Bottom|Snap::Right|Snap::InsideX|Snap::InsideY,
        {465.0f, 445.0f}, Size},                            /* I */
    {Snap::Bottom|Snap::Right|Snap::NoSpaceX|Snap::InsideY,
        {500.0f, 470.0f}, Size},                            /* J */
    {Snap::Bottom|Snap::Right|Snap::NoSpaceY|Snap::InsideX,
        {480.0f, 500.0f}, Size},                            /* K */
    {Snap::Bottom|Snap::Right|Snap::NoSpaceX|Snap::NoSpaceY|Snap::InsideX|Snap::InsideY,
        {480.0f, 470.0f}, Size},                            /* L */
    {Snap::Top,
        {290.0f, 167.0f}, Size},                            /* M */
    {Snap::Top|Snap::NoSpaceX,
        {290.0f, 167.0f}, Size},                            /* M */
    {Snap::Top|Snap::InsideY,
        {287.5f, 205.0f}, Size},                            /* N */
    {Snap::Top|Snap::InsideY|Snap::NoSpaceX,
        {290.0f, 205.0f}, Size},                            /* N, no space X */
    {Snap::Left,
        {73.0f, 335.0f}, Size},                             /* O */
    {Snap::Left|Snap::NoSpaceY,
        {73.0f, 335.0f}, Size},                             /* O again */
    {Snap::Left|Snap::InsideX,
        {110.0f, 325.0f}, Size},                            /* P */
    {Snap::Left|Snap::InsideX|Snap::NoSpaceY,
        {110.0f, 335.0f}, Size},                            /* P, no space Y */
    {Snap::Top|Snap::Left,
        {73.0f, 167.0f}, Size},                             /* Q */
    {Snap::Top|Snap::Left|Snap::NoSpaceX,
        {80.0f, 167.0f}, Size},                             /* R */
    {Snap::Bottom|Snap::Right,
        {507.0f, 503.0f}, Size},                            /* S */
    {Snap::Bottom|Snap::Right|Snap::NoSpaceY,
        {507.0f, 500.0f}, Size},                            /* T */
    {{},
        {287.5f, 325.0f}, Size},                            /* U */
    {Snap::NoSpaceX|Snap::NoSpaceY,
        {290.0f, 335.0f}, Size},                            /* U, no space XY */

    /*     100   500
        200 +-----+     +-----+      +-----+
            |     |     |   f | d    |hhhhh|
            |     |     |   e | d    |hgggh|
            |     |     |   e | d    |hgggh|
            |cbbbc|     |   e | d    |hgggh|
            |     |     |   f | d    |hhhhh|
        500 +-----+     +-----+      +-----+
             aaaaa                           */
    {Snap::Bottom|Snap::Left|Snap::Right,
        {100.0f, 503.0f}, {400.0f, Size.y()}},              /* aaa */
    {Snap::Bottom|Snap::Left|Snap::Right|Snap::InsideY,
        {110.0f, 445.0f}, {375.0f, Size.y()}},              /* bbb */
    {Snap::Bottom|Snap::Left|Snap::Right|Snap::InsideY|Snap::NoSpaceX,
        {100.0f, 445.0f}, {400.0f, Size.y()}},              /* cbc */
    {Snap::Top|Snap::Bottom|Snap::Right,
        {507.0f, 200.0f}, {Size.x(), 300.0f}},              /* ddd */
    {Snap::Top|Snap::Bottom|Snap::Right|Snap::InsideX,
        {465.0f, 205.0f}, {Size.x(), 270.0f}},              /* eee */
    {Snap::Top|Snap::Bottom|Snap::Right|Snap::InsideX|Snap::NoSpaceY,
        {465.0f, 200.0f}, {Size.x(), 300.0f}},              /* fef */
    {Snap::Top|Snap::Bottom|Snap::Left|Snap::Right,
        {110.0f, 205.0f}, {375.0f, 270.0f}},                /* ggg */
    {Snap::Top|Snap::Bottom|Snap::Left|Snap::Right|Snap::NoSpaceX|Snap::NoSpaceY,
        {100.0f, 200.0f}, {400.0f, 300.0f}}                 /* hgh */
};

SnapLayouterTest::SnapLayouterTest() {
    addTests({&SnapLayouterTest::debugSnap,
              &SnapLayouterTest::debugSnapPacked,
              &SnapLayouterTest::debugSnaps,
              &SnapLayouterTest::debugSnapsPacked,
              &SnapLayouterTest::debugSnapsSupersets});

    addInstancedTests({&SnapLayouterTest::snap},
        Containers::arraySize(SnapRectData));
}

void SnapLayouterTest::debugSnap() {
    std::ostringstream out;
    Debug{&out} << Snap::InsideX << Snap(0xbe);
    CORRADE_COMPARE(out.str(), "Whee::Snap::InsideX Whee::Snap(0xbe)\n");
}

void SnapLayouterTest::debugSnapPacked() {
    std::ostringstream out;
    /* Last is not packed, ones before should not make any flags persistent */
    Debug{&out} << Debug::packed << Snap::InsideX << Debug::packed << Snap(0xbe) << Snap::Right;
    CORRADE_COMPARE(out.str(), "InsideX 0xbe Whee::Snap::Right\n");
}

void SnapLayouterTest::debugSnaps() {
    std::ostringstream out;
    /* There isn't any bit free to test how the remains get printed */
    Debug{&out} << (Snap::Left|Snap::Right) << Snaps{};
    CORRADE_COMPARE(out.str(), "Whee::Snap::Left|Whee::Snap::Right Whee::Snaps{}\n");
}

void SnapLayouterTest::debugSnapsPacked() {
    std::ostringstream out;
    /* There isn't any bit free to test how the remains get printed. Last is
       not packed, ones before should not make any flags persistent. */
    Debug{&out} << Debug::packed << (Snap::Left|Snap::Right) << Debug::packed << Snaps{} << (Snap::InsideX|Snap::NoSpaceY);
    CORRADE_COMPARE(out.str(), "Left|Right {} Whee::Snap::InsideX|Whee::Snap::NoSpaceY\n");
}

void SnapLayouterTest::debugSnapsSupersets() {
    /* Inside is InsideX and InsideY combined */
    {
        std::ostringstream out;
        Debug{&out} << (Snap::InsideX|Snap::InsideY);
        CORRADE_COMPARE(out.str(), "Whee::Snap::Inside\n");

    /* NoSpace is NoSpaceX and NoSpaceY combined */
    } {
        std::ostringstream out;
        Debug{&out} << (Snap::NoSpaceX|Snap::NoSpaceY);
        CORRADE_COMPARE(out.str(), "Whee::Snap::NoSpace\n");
    }
}

void SnapLayouterTest::snap() {
    auto&& data = SnapRectData[testCaseInstanceId()];
    {
        std::ostringstream out;
        Debug{&out, Debug::Flag::NoNewlineAtTheEnd|Debug::Flag::Packed} << data.snaps;
        setTestCaseDescription(out.str());
    }

    Containers::Pair<Vector2, Vector2> out = Implementation::snap(data.snaps,
        {100.0f, 200.0f}, {400.0f, 300.0f},
        /* Left, top, right, bottom */
        {10.0f, 5.0f, 15.0f, 25.0f},
        {7.0f, 3.0f},
        Size);

    CORRADE_COMPARE(out, Containers::pair(data.expectedOffset, data.expectedSize));
}

}}}}

CORRADE_TEST_MAIN(Magnum::Whee::Test::SnapLayouterTest)
