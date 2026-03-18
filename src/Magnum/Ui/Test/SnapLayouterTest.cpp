/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023, 2024, 2025, 2026
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
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/String.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Container.h>
#include <Corrade/TestSuite/Compare/Numeric.h>
#include <Corrade/TestSuite/Compare/String.h>
#include <Magnum/Math/Swizzle.h>

#include "Magnum/Ui/AbstractLayer.h"
#include "Magnum/Ui/Anchor.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/NodeFlags.h"
#include "Magnum/Ui/SnapLayouter.h"
#include "Magnum/Ui/UserInterface.h"
#include "Magnum/Ui/Implementation/snapLayouter.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct SnapLayouterTest: TestSuite::Tester {
    explicit SnapLayouterTest();

    void debugSnap();
    void debugSnapPacked();
    void debugSnaps();
    void debugSnapsPacked();
    void debugSnapsSupersets();
    void debugFlag();
    void debugFlags();
    void debugFlagsSupersets();

    void snapInsideImplementation();
    void snapImplementation();

    void layoutSizePaddingMargin();
    void layoutSizePaddingMarginPropagate();

    /* The result is fed into layoutSizePaddingMargin() and snap(), so this
       subsequently tests that these two do what's expected */
    void childLayoutSizeSide();
    void childLayoutSizeForward();

    void explicitlySnappedChildLayoutSize();

    void expandChildLayouts();

    void orderLayoutsBreadthFirstEmpty();
    void orderLayoutsBreadthFirst();

    void construct();
    void constructCopy();
    void constructMove();

    void addRemove();
    void addRemoveExplicitSnap();
    void addRemoveHandleRecycle();
    void addInvalid();
    void removeInvalid();

    void flags();
    void flagsInvalid();

    void snap();
    void snapInvalid();

    void childSnap();
    void childSnapInvalid();

    void invalidHandle();
    void invalidExplicitSnap();

    void setSize();

    void clean();
    void cleanInvalid();

    void layoutEmpty();
    void layoutDataOrder();
    void layoutLayoutProperties();
    void layoutChildSnap();
    void layoutPropagateChildSizes();
    void layoutExpandChildLayouts();
    void layoutExpandChildLayoutsOverflow();
};

const struct {
    Snaps snap;
    BitVector2 expected;
} SnapInsideImplementationData[]{
    {Snaps{}, BitVector2{3}},
    {Snap::Left|Snap::Right, BitVector2{3}},
    {Snap::Top|Snap::Bottom, BitVector2{3}},
    {Snap::Left|Snap::Top, BitVector2{0}},
    {Snap::Right|Snap::Bottom, BitVector2{0}},
    {Snap::Left|Snap::InsideX, BitVector2{3}},
    {Snap::Left|Snap::InsideY, BitVector2{2}},
    {Snap::Right|Snap::InsideX, BitVector2{3}},
    {Snap::Right|Snap::InsideY, BitVector2{2}},
    {Snap::Top|Snap::InsideX, BitVector2{1}},
    {Snap::Top|Snap::InsideY, BitVector2{3}},
    {Snap::Bottom|Snap::InsideX, BitVector2{1}},
    {Snap::Bottom|Snap::InsideY, BitVector2{3}},
};

const Vector2 Size{20.0f, 30.0f};

/* Used inside SnapData below but also in updateDataOrder() */
constexpr Float Nan = Constants::nan();
constexpr Vector4 Nan4{Constants::nan()};

const struct {
    Snaps snap;
    const char* name;
    /* left, top, right, bottom; components that should not be used are NaN */
    Vector4 referencePadding, referenceMargin, margin;
    Vector2 expectedOffset, expectedSize;
} SnapImplementationData[]{
    /*     100   500        in all cases below, the final margin and padding
          QR A M            should be either the following or NaN if not used
             E
        200 +-----+                   margin
          BD|F    |                     3
            | CN  |             +---------------+
          O | PU  |             |       5       |
            |   I |           7 | 10 padding 15 | 7
            |    L|JH           |       25      |
        500 +-----+             +---------------+
                 K  T                   3
                 G  S                                */
    {Snap::TopLeft|Snap::InsideX, "ref Y margin larger",
        Nan4, {Nan, 3.0f, Nan, Nan}, {Nan, Nan, Nan, 2.0f},
        {100.0f, 167.0f}, Size},                            /* A */
    {Snap::TopLeft|Snap::InsideX, nullptr,
        Nan4, {Nan, 1.0f, Nan, Nan}, {Nan, Nan, Nan, 3.0f},
        {100.0f, 167.0f}, Size},                            /* A */
    {Snap::TopLeft|Snap::NoPadX|Snap::InsideX, "ref Y margin larger",
        Nan4, {Nan, 3.0f, Nan, Nan}, {Nan, Nan, Nan, 2.0f},
        {100.0f, 167.0f}, Size},                            /* A again */
    {Snap::TopLeft|Snap::NoPadX|Snap::InsideX, nullptr,
        Nan4, {Nan, 1.0f, Nan, Nan}, {Nan, Nan, Nan, 3.0f},
        {100.0f, 167.0f}, Size},                            /* A again */
    {Snap::TopLeft|Snap::InsideY, "ref X margin larger",
        Nan4, {7.0f, Nan, Nan, Nan}, {Nan, Nan, 5.0f, Nan},
        {73.0f, 200.0f}, Size},                             /* B */
    {Snap::TopLeft|Snap::InsideY, nullptr,
        Nan4, {6.0f, Nan, Nan, Nan}, {Nan, Nan, 7.0f, Nan},
        {73.0f, 200.0f}, Size},                             /* B */
    {Snap::TopLeft|Snap::NoPadY|Snap::InsideY, "ref X margin larger",
        Nan4, {7.0f, Nan, Nan, Nan}, {Nan, Nan, 5.0f, Nan},
        {73.0f, 200.0f}, Size},                             /* B again */
    {Snap::TopLeft|Snap::NoPadY|Snap::InsideY, nullptr,
        Nan4, {6.0f, Nan, Nan, Nan}, {Nan, Nan, 7.0f, Nan},
        {73.0f, 200.0f}, Size},                             /* B again */
    {Snap::TopLeft|Snap::InsideX|Snap::InsideY, "ref X padding larger",
        {10.0f, 4.0f, Nan, Nan}, Nan4, {9.0f, 5.0f, Nan, Nan},
        {110.0f, 205.0f}, Size},                            /* C */
    {Snap::TopLeft|Snap::InsideX|Snap::InsideY, "ref Y padding larger",
        {8.0f, 5.0f, Nan, Nan}, Nan4, {10.0f, 3.0f, Nan, Nan},
        {110.0f, 205.0f}, Size},                            /* C */
    {Snap::TopLeft|Snap::InsideX|Snap::InsideY, "ref XY padding larger",
        {10.0f, 5.0f, Nan, Nan}, Nan4, {4.0f, 3.0f, Nan, Nan},
        {110.0f, 205.0f}, Size},                            /* C */
    {Snap::TopLeft|Snap::InsideX|Snap::InsideY, nullptr,
        {3.0f, 2.0f, Nan, Nan}, Nan4, {10.0f, 5.0f, Nan, Nan},
        {110.0f, 205.0f}, Size},                            /* C */
    {Snap::TopLeft|Snap::NoPadX|Snap::InsideY, nullptr,
        Nan4, Nan4, Nan4,
        {80.0f, 200.0f}, Size},                             /* D */
    {Snap::TopLeft|Snap::NoPadY|Snap::InsideX, nullptr,
        Nan4, Nan4, Nan4,
        {100.0f, 170.0f}, Size},                            /* E */
    {Snap::TopLeft|Snap::NoPad|Snap::Inside, nullptr,
        Nan4, Nan4, Nan4,
        {100.0f, 200.0f}, Size},                            /* F */
    {Snap::BottomRight|Snap::InsideX, "ref Y margin larger",
        Nan4, {Nan, Nan, Nan, 3.0f}, {Nan, 2.0f, Nan, Nan},
        {480.0f, 503.0f}, Size},                            /* G */
    {Snap::BottomRight|Snap::InsideX, nullptr,
        Nan4, {Nan, Nan, Nan, 1.0f}, {Nan, 3.0f, Nan, Nan},
        {480.0f, 503.0f}, Size},                            /* G */
    {Snap::BottomRight|Snap::NoPadX|Snap::InsideX, "ref Y margin larger",
        Nan4, {Nan, Nan, Nan, 3.0f}, {Nan, 2.0f, Nan, Nan},
        {480.0f, 503.0f}, Size},                            /* G again */
    {Snap::BottomRight|Snap::NoPadX|Snap::InsideX, nullptr,
        Nan4, {Nan, Nan, Nan, 1.0f}, {Nan, 3.0f, Nan, Nan},
        {480.0f, 503.0f}, Size},                            /* G again */
    {Snap::BottomRight|Snap::InsideY, "ref X margin larger",
        Nan4, {Nan, Nan, 7.0f, Nan}, {4.0f, Nan, Nan, Nan},
        {507.0f, 470.0f}, Size},                            /* H */
    {Snap::BottomRight|Snap::InsideY, nullptr,
        Nan4, {Nan, Nan, 6.0f, Nan}, {7.0f, Nan, Nan, Nan},
        {507.0f, 470.0f}, Size},                            /* H */
    {Snap::BottomRight|Snap::NoPadY|Snap::InsideY, "ref X margin larger",
        Nan4, {Nan, Nan, 7.0f, Nan}, {4.0f, Nan, Nan, Nan},
        {507.0f, 470.0f}, Size},                            /* H again */
    {Snap::BottomRight|Snap::NoPadY|Snap::InsideY, nullptr,
        Nan4, {Nan, Nan, 6.0f, Nan}, {7.0f, Nan, Nan, Nan},
        {507.0f, 470.0f}, Size},                            /* H again */
    {Snap::BottomRight|Snap::Inside, "ref X padding larger",
        {Nan, Nan, 15.0f, 23.0f}, Nan4, {Nan, Nan, 14.0f, 25.0f},
        {465.0f, 445.0f}, Size},                            /* I */
    {Snap::BottomRight|Snap::Inside, "ref Y padding larger",
        {Nan, Nan, 10.0f, 25.0f}, Nan4, {Nan, Nan, 15.0f, 20.0f},
        {465.0f, 445.0f}, Size},                            /* I */
    {Snap::BottomRight|Snap::Inside, "ref XY padding larger",
        {Nan, Nan, 15.0f, 25.0f}, Nan4, {Nan, Nan, 14.0f, 22.0f},
        {465.0f, 445.0f}, Size},                            /* I */
    {Snap::BottomRight|Snap::Inside, nullptr,
        {Nan, Nan, 11.0f, 23.0f}, Nan4, {Nan, Nan, 15.0f, 25.0f},
        {465.0f, 445.0f}, Size},                            /* I */
    {Snap::BottomRight|Snap::NoPadX|Snap::InsideY, nullptr,
        Nan4, Nan4, Nan4,
        {500.0f, 470.0f}, Size},                            /* J */
    {Snap::BottomRight|Snap::NoPadY|Snap::InsideX, nullptr,
        Nan4, Nan4, Nan4,
        {480.0f, 500.0f}, Size},                            /* K */
    {Snap::BottomRight|Snap::NoPad|Snap::Inside, nullptr,
        Nan4, Nan4, Nan4,
        {480.0f, 470.0f}, Size},                            /* L */
    {Snap::Top, "ref Y margin larger",
        Nan4, {Nan, 3.0f, Nan, Nan}, {Nan, Nan, Nan, 2.0f},
        {290.0f, 167.0f}, Size},                            /* M */
    {Snap::Top, nullptr,
        Nan4, {Nan, 1.0f, Nan, Nan}, {Nan, Nan, Nan, 3.0f},
        {290.0f, 167.0f}, Size},                            /* M */
    {Snap::Top|Snap::NoPadX, "ref Y margin larger",
        Nan4, {Nan, 3.0f, Nan, Nan}, {Nan, Nan, Nan, 2.0f},
        {290.0f, 167.0f}, Size},                            /* M again */
    {Snap::Top|Snap::NoPadX, nullptr,
        Nan4, {Nan, 1.0f, Nan, Nan}, {Nan, Nan, Nan, 3.0f},
        {290.0f, 167.0f}, Size},                            /* M again */
    {Snap::Top|Snap::InsideY, "ref Y padding larger",
        {10.0f, 5.0f, 15.0f, Nan}, Nan4, {Nan, 3.0f, Nan, Nan},
        {287.5f, 205.0f}, Size},                            /* N */
    {Snap::Top|Snap::InsideY, nullptr,
        {10.0f, 3.0f, 15.0f, Nan}, Nan4, {Nan, 5.0f, Nan, Nan},
        {287.5f, 205.0f}, Size},                            /* N */
    {Snap::Top|Snap::InsideY|Snap::NoPadX, "ref Y padding larger",
        {Nan, 5.0f, Nan, Nan}, Nan4, {Nan, 3.0f, Nan, Nan},
        {290.0f, 205.0f}, Size},                            /* N, no pad X */
    {Snap::Top|Snap::InsideY|Snap::NoPadX, nullptr,
        {Nan, 4.0f, Nan, Nan}, Nan4, {Nan, 5.0f, Nan, Nan},
        {290.0f, 205.0f}, Size},                            /* N, no pad X */
    {Snap::Left, "ref X margin larger",
        Nan4, {7.0f, Nan, Nan, Nan}, {Nan, Nan, 6.0f, Nan},
        {73.0f, 335.0f}, Size},                             /* O */
    {Snap::Left, nullptr,
        Nan4, {3.0f, Nan, Nan, Nan}, {Nan, Nan, 7.0f, Nan},
        {73.0f, 335.0f}, Size},                             /* O */
    {Snap::Left|Snap::NoPadY, "ref X margin larger",
        Nan4, {7.0f, Nan, Nan, Nan}, {Nan, Nan, 6.0f, Nan},
        {73.0f, 335.0f}, Size},                             /* O again */
    {Snap::Left|Snap::NoPadY, nullptr,
        Nan4, {3.0f, Nan, Nan, Nan}, {Nan, Nan, 7.0f, Nan},
        {73.0f, 335.0f}, Size},                             /* O again */
    {Snap::Left|Snap::InsideX, "ref X padding larger",
        {10.0f, 5.0f, Nan, 25.0f}, Nan4, {7.0f, Nan, Nan, Nan},
        {110.0f, 325.0f}, Size},                            /* P */
    {Snap::Left|Snap::InsideX, nullptr,
        {9.0f, 5.0f, Nan, 25.0f}, Nan4, {10.0f, Nan, Nan, Nan},
        {110.0f, 325.0f}, Size},                            /* P */
    {Snap::Left|Snap::InsideX|Snap::NoPadY, "ref X padding larger",
        {10.0f, Nan, Nan, Nan}, Nan4, {9.0f, Nan, Nan, Nan},
        {110.0f, 335.0f}, Size},                            /* P, no pad Y */
    {Snap::Left|Snap::InsideX|Snap::NoPadY, nullptr,
        {8.0f, Nan, Nan, Nan}, Nan4, {10.0f, Nan, Nan, Nan},
        {110.0f, 335.0f}, Size},                            /* P, no pad Y */
    {Snap::TopLeft, "ref X margin larger",
        Nan4, {7.0f, 2.0f, Nan, Nan}, {Nan, Nan, 6.0f, 3.0f},
        {73.0f, 167.0f}, Size},                             /* Q */
    {Snap::TopLeft, "ref Y margin larger",
        Nan4, {5.0f, 3.0f, Nan, Nan}, {Nan, Nan, 7.0f, 1.0f},
        {73.0f, 167.0f}, Size},                             /* Q */
    {Snap::TopLeft, "ref XY margin larger",
        Nan4, {7.0f, 3.0f, Nan, Nan}, {Nan, Nan, 4.0f, 1.0f},
        {73.0f, 167.0f}, Size},                             /* Q */
    {Snap::TopLeft, nullptr,
        Nan4, {4.0f, 1.0f, Nan, Nan}, {Nan, Nan, 7.0f, 3.0f},
        {73.0f, 167.0f}, Size},                             /* Q */
    {Snap::TopLeft|Snap::NoPadX, "ref Y margin larger",
        Nan4, {Nan, 3.0f, Nan, Nan}, {Nan, Nan, Nan, 2.0f},
        {80.0f, 167.0f}, Size},                             /* R */
    {Snap::TopLeft|Snap::NoPadX, nullptr,
        Nan4, {Nan, 1.0f, Nan, Nan}, {Nan, Nan, Nan, 3.0f},
        {80.0f, 167.0f}, Size},                             /* R */
    {Snap::BottomRight, "ref X margin larger",
        Nan4, {Nan, Nan, 7.0f, 2.0f}, {6.0f, 3.0f, Nan, Nan},
        {507.0f, 503.0f}, Size},                            /* S */
    {Snap::BottomRight, "ref X margin larger",
        Nan4, {Nan, Nan, 5.0f, 3.0f}, {7.0f, 2.0f, Nan, Nan},
        {507.0f, 503.0f}, Size},                            /* S */
    {Snap::BottomRight, "ref XY margin larger",
        Nan4, {Nan, Nan, 7.0f, 3.0f}, {4.0f, 1.0f, Nan, Nan},
        {507.0f, 503.0f}, Size},                            /* S */
    {Snap::BottomRight, nullptr,
        Nan4, {Nan, Nan, 3.0f, 1.0f}, {7.0f, 3.0f, Nan, Nan},
        {507.0f, 503.0f}, Size},                            /* S */
    {Snap::BottomRight|Snap::NoPadY, "ref X margin larger",
        Nan4, {Nan, Nan, 7.0f, Nan}, {6.0f, Nan, Nan, Nan},
        {507.0f, 500.0f}, Size},                            /* T */
    {Snap::BottomRight|Snap::NoPadY, nullptr,
        Nan4, {Nan, Nan,5.0f, Nan}, {7.0f, Nan, Nan, Nan},
        {507.0f, 500.0f}, Size},                            /* T */
    {{}, nullptr,
        {10.0f, 5.0f, 15.0f, 25.0f}, Nan4, Nan4,
        {287.5f, 325.0f}, Size},                            /* U */
    {Snap::NoPad, nullptr,
        Nan4, Nan4, Nan4,
        {290.0f, 335.0f}, Size},                            /* U, no pad XY */

    /*     100   500
        200 +-----+     +-----+      +-----+
            |     |     |   f | d    |hhhhh|
            |     |     |   e | d    |hgggh|
            |     |     |   e | d    |hgggh|
            |cbbbc|     |   e | d    |hgggh|
            |     |     |   f | d    |hhhhh|
        500 +-----+     +-----+      +-----+
             aaaaa

        Per-component max() is tested sufficiently above, verifying just that
        both sides are taken into account. */
    {Snap::Bottom|Snap::FillX, "ref margin larger",
        Nan4, {Nan, Nan, Nan, 3.0f}, {Nan, 2.0f, Nan, Nan},
        {100.0f, 503.0f}, {400.0f, Size.y()}},              /* aaa */
    {Snap::Bottom|Snap::FillX, nullptr,
        Nan4, {Nan, Nan, Nan, 1.0f}, {Nan, 3.0f, Nan, Nan},
        {100.0f, 503.0f}, {400.0f, Size.y()}},              /* aaa */
    {Snap::Bottom|Snap::FillX|Snap::InsideY, "ref padding larger",
        {10.0f, Nan, 15.0f, 25.0f}, Nan4, {9.0f, Nan, 11.0f, 22.0f},
        {110.0f, 445.0f}, {375.0f, Size.y()}},              /* bbb */
    {Snap::Bottom|Snap::FillX|Snap::InsideY, nullptr,
        {9.0f, Nan, 11.0f, 22.0f}, Nan4, {10.0f, Nan, 15.0f, 25.0f},
        {110.0f, 445.0f}, {375.0f, Size.y()}},              /* bbb */
    {Snap::Bottom|Snap::FillX|Snap::InsideY|Snap::NoPadX, "ref padding larger",
        {Nan, Nan, Nan, 25.0f}, Nan4, {Nan, Nan, Nan, 22.0f},
        {100.0f, 445.0f}, {400.0f, Size.y()}},              /* cbc */
    {Snap::Bottom|Snap::FillX|Snap::InsideY|Snap::NoPadX, nullptr,
        {Nan, Nan, Nan, 24.0f}, Nan4, {Nan, Nan, Nan, 25.0f},
        {100.0f, 445.0f}, {400.0f, Size.y()}},              /* cbc */
    {Snap::FillY|Snap::Right, "ref margin larger",
        Nan4, {Nan, Nan, 7.0f, Nan}, {5.0f, Nan, Nan, Nan},
        {507.0f, 200.0f}, {Size.x(), 300.0f}},              /* ddd */
    {Snap::FillY|Snap::Right, nullptr,
        Nan4, {Nan, Nan, 6.0f, Nan}, {7.0f, Nan, Nan, Nan},
        {507.0f, 200.0f}, {Size.x(), 300.0f}},              /* ddd */
    {Snap::FillY|Snap::Right|Snap::InsideX, "ref padding larger",
        {Nan, 5.0f, 15.0f, 25.0f}, Nan4, {Nan, 4.0f, 11.0f, 22.0f},
        {465.0f, 205.0f}, {Size.x(), 270.0f}},              /* eee */
    {Snap::FillY|Snap::Right|Snap::InsideX, nullptr,
        {Nan, 4.0f, 11.0f, 22.0f}, Nan4, {Nan, 5.0f, 15.0f, 25.0f},
        {465.0f, 205.0f}, {Size.x(), 270.0f}},              /* eee */
    {Snap::FillY|Snap::Right|Snap::InsideX|Snap::NoPadY, "ref padding larger",
        {Nan, Nan, 15.0f, Nan}, Nan4, {Nan, Nan, 13.0f, Nan},
        {465.0f, 200.0f}, {Size.x(), 300.0f}},              /* fef */
    {Snap::FillY|Snap::Right|Snap::InsideX|Snap::NoPadY, nullptr,
        {Nan, Nan, 14.0f, Nan}, Nan4, {Nan, Nan, 15.0f, Nan},
        {465.0f, 200.0f}, {Size.x(), 300.0f}},              /* fef */
    {Snap::Fill, "ref padding larger",
        {10.0f, 5.0f, 15.0f, 25.0f}, Nan4, {9.0f, 4.0f, 11.0f, 22.0f},
        {110.0f, 205.0f}, {375.0f, 270.0f}},                /* ggg */
    {Snap::Fill, nullptr,
        {9.0f, 4.0f, 11.0f, 22.0f}, Nan4, {10.0f, 5.0f, 15.0f, 25.0f},
        {110.0f, 205.0f}, {375.0f, 270.0f}},                /* ggg */
    {Snap::Fill|Snap::NoPad, nullptr,
        Nan4, Nan4, Nan4,
        {100.0f, 200.0f}, {400.0f, 300.0f}}                 /* hgh */
};

const struct {
    TestSuite::TestCaseDescriptionSourceLocation name;
    SnapLayoutFlags flags;
    Snaps extraChildSnap;
    Vector2 nodeSize;
    Vector4 nodePadding;
    Vector2 childLayoutSize;
    Vector4 childLayoutMargin;
    Vector2 expectedPaddedChildLayoutSize, expectedLayoutSize;
    Vector4 expectedLayoutPadding;
} LayoutSizePaddingMarginData[]{
    {"no padding or margin, node size only", {}, {},
        {30.0f, 20.0f}, {},
        {}, {},
        {}, {30.0f, 20.0f}, {}},
    {"no padding or margin, child layout size only", {}, {},
        {}, {},
        {30.0f, 20.0f}, {},
        {30.0f, 20.0f}, {30.0f, 20.0f}, {}},
    {"padding, node size only", {}, {},
        {30.0f, 20.0f}, {1.0f, 2.0f, 3.0f, 4.0f},
        {}, {},
        /* The padding is applied inside so it doesn't expand the node size */
        {4.0f, 6.0f}, {30.0f, 20.0f}, {1.0f, 2.0f, 3.0f, 4.0f}},
    {"padding, child layout size only", {}, {},
        {}, {1.0f, 2.0f, 3.0f, 4.0f},
        {30.0f, 20.0f}, {},
        {34.0f, 26.0f}, {34.0f, 26.0f}, {1.0f, 2.0f, 3.0f, 4.0f}},
    {"margin, node size only", {}, {},
        {30.0f, 20.0f}, {},
        {}, {1.0f, 2.0f, 3.0f, 4.0f},
        /* The child margin is applied inside so it doesn't expand the node
           size */
        {4.0f, 6.0f}, {30.0f, 20.0f}, {1.0f, 2.0f, 3.0f, 4.0f}},
    {"margin, child layout size only", {}, {},
        {}, {},
        {30.0f, 20.0f}, {1.0f, 2.0f, 3.0f, 4.0f},
        {34.0f, 26.0f}, {34.0f, 26.0f}, {1.0f, 2.0f, 3.0f, 4.0f}},
    {"padding, margin, node size, child layout size, variant 1", {}, {},
        /* The node X size is larger than child layout X size but padding +
           margin makes the child size bigger so it's picked */
        {29.0f, 20.0f}, {1.0f, 1.0f, 3.0f, 3.0f},
      /* v      ^        ^     v     ^     v     */
        {26.0f, 10.0f}, {0.0f, 2.0f, 2.0f, 4.0f},
        {30.0f, 16.0f}, {30.0f, 20.0f}, {1.0f, 2.0f, 3.0f, 4.0f}},
    {"padding, margin, node size, child layout size, variant 2", {}, {},
        /* The node Y size is larger than child layout Y size but padding +
           margin makes the child size bigger so it's picked */
        {30.0f, 15.0f}, {0.0f, 2.0f, 2.0f, 4.0f},
      /* ^      v        v     ^     v     ^     */
        {20.0f, 14.0f}, {1.0f, 1.0f, 3.0f, 3.0f},
        {24.0f, 20.0f}, {30.0f, 20.0f}, {1.0f, 2.0f, 3.0f, 4.0f}},
    {"padding, margin, node size, child layout size, no pad, variant 1",
        {}, Snap::NoPad,
        {30.0f, 15.0f}, {1.0f, 2.0f, 3.0f, 4.0f},
      /* ^      v */
        {20.0f, 20.0f}, {1.0f, 2.0f, 3.0f, 4.0f},
        {20.0f, 20.0f}, {30.0f, 20.0f}, {}},
    {"padding, margin, node size, child layout size, no pad, variant 2",
        {}, Snap::NoPad,
        {20.0f, 20.0f}, {1.0f, 2.0f, 3.0f, 4.0f},
      /* v      ^ */
        {30.0f, 15.0f}, {1.0f, 2.0f, 3.0f, 4.0f},
        {30.0f, 15.0f}, {30.0f, 20.0f}, {}},
    {"padding, margin, node size, child layout size, no pad X, variant 1",
        {}, Snap::NoPadX,
        /* The node Y size is larger than child layout Y size but padding +
           margin makes the child size bigger so it's picked */
        {30.0f, 15.0f}, {1.0f, 2.0f, 3.0f, 3.0f},
      /* ^      v              ^           v     */
        {20.0f, 14.0f}, {1.0f, 1.0f, 3.0f, 4.0f},
        {20.0f, 20.0f}, {30.0f, 20.0f}, {0.0f, 2.0f, 0.0f, 4.0f}},
    {"padding, margin, node size, child layout size, no pad X, variant 2",
        {}, Snap::NoPadX,
        {20.0f, 20.0f}, {1.0f, 1.0f, 3.0f, 4.0f},
      /* v      ^              v           ^     */
        {30.0f, 10.0f}, {1.0f, 2.0f, 3.0f, 3.0f},
        {30.0f, 16.0f}, {30.0f, 20.0f}, {0.0f, 2.0f, 0.0f, 4.0f}},
    {"padding, margin, node size, child layout size, no pad Y, variant 1",
        {}, Snap::NoPadY,
        /* The node X size is larger than child layout X size but padding +
           margin makes the child size bigger so it's picked */
        {29.0f, 20.0f}, {1.0f, 2.0f, 2.0f, 4.0f},
      /* v      ^        ^           v           */
        {26.0f, 10.0f}, {0.0f, 2.0f, 3.0f, 4.0f},
        {30.0f, 10.0f}, {30.0f, 20.0f}, {1.0f, 0.0f, 3.0f, 0.0f}},
    {"padding, margin, node size, child layout size, no pad Y, variant 2",
        {}, Snap::NoPadY,
        {30.0f, 10.0f}, {0.0f, 2.0f, 3.0f, 4.0f},
      /* ^      v        v           ^           */
        {20.0f, 20.0f}, {1.0f, 2.0f, 2.0f, 4.0f},
        {24.0f, 20.0f}, {30.0f, 20.0f}, {1.0f, 0.0f, 3.0f, 0.0f}},
    {"padding, margin, node size, child layout size, ignore overflow",
        SnapLayoutFlag::IgnoreOverflow, {},
        {30.0f, 20.0f}, {1.0f, 2.0f, 3.0f, 4.0f},
        /* The child layout size or margin isn't taken into account at all for
           the outer size */
        {50.0f, 40.0f}, {9.0f, 8.0f, 7.0f, 6.0f},
        {54.0f, 46.0f}, {30.0f, 20.0f}, {1.0f, 2.0f, 3.0f, 4.0f}},
    {"padding, margin, node size, child layout size, ignore overflow, no pad",
        SnapLayoutFlag::IgnoreOverflow, Snap::NoPad,
        {30.0f, 20.0f}, {1.0f, 2.0f, 3.0f, 4.0f},
        /* The child layout size or margin isn't taken into account at all for
           the outer size */
        {50.0f, 40.0f}, {9.0f, 8.0f, 7.0f, 6.0f},
        {50.0f, 40.0f}, {30.0f, 20.0f}, {}},
    {"padding, margin, node size, child layout size, ignore overflow, no pad X",
        SnapLayoutFlag::IgnoreOverflow, Snap::NoPadX,
        {30.0f, 20.0f}, {1.0f, 2.0f, 3.0f, 4.0f},
        /* The child layout size or margin isn't taken into account at all for
           the outer size */
        {50.0f, 40.0f}, {9.0f, 8.0f, 7.0f, 6.0f},
        {50.0f, 46.0f}, {30.0f, 20.0f}, {0.0f, 2.0f, 0.0f, 4.0f}},
    {"padding, margin, node size, child layout size, ignore overflow, no pad Y",
        SnapLayoutFlag::IgnoreOverflow, Snap::NoPadY,
        {30.0f, 20.0f}, {1.0f, 2.0f, 3.0f, 4.0f},
        /* The child layout size or margin isn't taken into account at all for
           the outer size */
        {50.0f, 40.0f}, {9.0f, 8.0f, 7.0f, 6.0f},
        {54.0f, 40.0f}, {30.0f, 20.0f}, {1.0f, 0.0f, 3.0f, 0.0f}},
    {"padding, margin, node size, child layout size, ignore overflow X",
        SnapLayoutFlag::IgnoreOverflowX, {},
        /* The node Y size is larger than child layout Y size but padding +
           margin makes the child size bigger so it's picked */
        {30.0f, 15.0f}, {1.0f, 1.0f, 3.0f, 3.0f},
        {50.0f, 14.0f}, {9.0f, 2.0f, 7.0f, 4.0f},
        {54.0f, 20.0f}, {30.0f, 20.0f}, {1.0f, 2.0f, 3.0f, 4.0f}},
    {"padding, margin, node size, child layout size, ignore overflow X, no pad",
        SnapLayoutFlag::IgnoreOverflowX, Snap::NoPad,
        {30.0f, 10.0f}, {1.0f, 2.0f, 3.0f, 3.0f},
        {50.0f, 20.0f}, {9.0f, 8.0f, 7.0f, 6.0f},
        {50.0f, 20.0f}, {30.0f, 20.0f}, {}},
    {"padding, margin, node size, child layout size, ignore overflow X, no pad X",
        SnapLayoutFlag::IgnoreOverflowX, Snap::NoPadX,
        /* The node Y size is larger than child layout Y size but padding +
           margin makes the child size bigger so it's picked */
        {30.0f, 15.0f}, {1.0f, 1.0f, 3.0f, 3.0f},
        {50.0f, 14.0f}, {9.0f, 2.0f, 7.0f, 4.0f},
        {50.0f, 20.0f}, {30.0f, 20.0f}, {0.0f, 2.0f, 0.0f, 4.0f}},
    {"padding, margin, node size, child layout size, ignore overflow X, no pad Y",
        SnapLayoutFlag::IgnoreOverflowX, Snap::NoPadY,
        {30.0f, 19.0f}, {1.0f, 2.0f, 3.0f, 4.0f},
        {50.0f, 20.0f}, {9.0f, 8.0f, 7.0f, 6.0f},
        {54.0f, 20.0f}, {30.0f, 20.0f}, {1.0f, 0.0f, 3.0f, 0.0f}},
    {"padding, margin, node size, child layout size, ignore overflow Y",
        SnapLayoutFlag::IgnoreOverflowY, {},
        /* The node X size is larger than child layout X size but padding +
           margin makes the child size bigger so it's picked */
        {29.0f, 20.0f}, {0.0f, 2.0f, 2.0f, 4.0f},
        {26.0f, 40.0f}, {1.0f, 8.0f, 3.0f, 6.0f},
        {30.0f, 46.0f}, {30.0f, 20.0f}, {1.0f, 2.0f, 3.0f, 4.0f}},
    {"padding, margin, node size, child layout size, ignore overflow Y, no pad",
        SnapLayoutFlag::IgnoreOverflowY, Snap::NoPad,
        {20.0f, 20.0f}, {1.0f, 2.0f, 3.0f, 4.0f},
        {30.0f, 40.0f}, {9.0f, 8.0f, 7.0f, 6.0f},
        {30.0f, 40.0f}, {30.0f, 20.0f}, {}},
    {"padding, margin, node size, child layout size, ignore overflow Y, no pad X",
        SnapLayoutFlag::IgnoreOverflowY, Snap::NoPadX,
        {20.0f, 20.0f}, {1.0f, 2.0f, 3.0f, 4.0f},
        {30.0f, 40.0f}, {9.0f, 8.0f, 7.0f, 6.0f},
        {30.0f, 46.0f}, {30.0f, 20.0f}, {0.0f, 2.0f, 0.0f, 4.0f}},
    {"padding, margin, node size, child layout size, ignore overflow Y, no pad Y",
        SnapLayoutFlag::IgnoreOverflowY, Snap::NoPadY,
        /* The node X size is larger than child layout X size but padding +
           margin makes the child size bigger so it's picked */
        {29.0f, 20.0f}, {0.0f, 2.0f, 2.0f, 4.0f},
        {26.0f, 40.0f}, {1.0f, 8.0f, 3.0f, 6.0f},
        {30.0f, 40.0f}, {30.0f, 20.0f}, {1.0f, 0.0f, 3.0f, 0.0f}},
};

const struct {
    TestSuite::TestCaseDescriptionSourceLocation name;
    SnapLayoutFlags flags[2];
    Snaps snaps[4];
    BitVector2 hasFillChildren;
    Vector2 nodeSize;
    Vector4 nodeMargin;
    /* +----------------------+
       |          2           |
       |          5           |
       |     +----------+     |     Parent node padding, child layout margin
       | 3 7 | {10, 16} | 3 1 |     and child layout size
       |     +----------+     |
       |          9           |
       |          4           |
       +----------------------+ */
    Vector2 expectedPaddedChildLayoutSize, expectedLayoutSize;
    Vector4 expectedLayoutPadding, expectedLayoutMargin;
} LayoutSizePaddingMarginPropagateData[]{
    {"propagate both directions, node padding smaller than child margin, no node margin, center",
        {SnapLayoutFlag::PropagateMargin,
         SnapLayoutFlag::PropagateMargin},
        {Snap::Right,
         Snap::Bottom,
         Snap::Left,
         Snap::Top}, {},
        /* The left-side margin is fully propagated outside, the centering has
           extra space of 4 on each side, which causes the top margin to
           underflow by 1, bottom to overflow by 1. Right-side margin fits
           exactly without overflow. */
        {16.0f, 30.0f}, {},
        {14.0f, 22.0f}, {16.0f, 30.0f},
        {3.0f, 2.0f, 1.0f, 4.0f}, {4.0f, 0.0f, 0.0f, 1.0f}},
    /* Should be exactly the same as above */
    {"propagate both directions, node padding smaller than child margin, no node margin, center, redundant Inside",
        {SnapLayoutFlag::PropagateMargin,
         SnapLayoutFlag::PropagateMargin},
        {Snap::Right|Snap::InsideY,
         Snap::Bottom|Snap::InsideX,
         Snap::Left|Snap::InsideY,
         Snap::Top|Snap::InsideX}, {},
        {16.0f, 30.0f}, {},
        {14.0f, 22.0f}, {16.0f, 30.0f},
        {3.0f, 2.0f, 1.0f, 4.0f}, {4.0f, 0.0f, 0.0f, 1.0f}},
    {"propagate both directions, node padding smaller than child margin, no node margin, center, no pad",
        {SnapLayoutFlag::PropagateMargin,
         SnapLayoutFlag::PropagateMargin},
        {Snap::Right|Snap::NoPad,
         Snap::Bottom|Snap::NoPad,
         Snap::Left|Snap::NoPad,
         Snap::Top|Snap::NoPad}, {},
        {16.0f, 30.0f}, {},
        {10.0f, 16.0f}, {16.0f, 30.0f}, {}, {}},
    {"propagate both directions, node padding smaller than child margin, no node margin, center, no pad forward",
        {SnapLayoutFlag::PropagateMargin,
         SnapLayoutFlag::PropagateMargin},
        {Snap::Right|Snap::NoPadX,
         Snap::Bottom|Snap::NoPadY,
         Snap::Left|Snap::NoPadX,
         Snap::Top|Snap::NoPadY}, {},
        {16.0f, 30.0f}, {},
        {10.0f, 22.0f}, {16.0f, 30.0f},
        {0.0f, 2.0f, 0.0f, 4.0f}, {0.0f, 0.0f, 0.0f, 1.0f}},
    {"propagate both directions, node padding smaller than child margin, no node margin, center, no pad side",
        {SnapLayoutFlag::PropagateMargin,
         SnapLayoutFlag::PropagateMargin},
        {Snap::Right|Snap::NoPadY,
         Snap::Bottom|Snap::NoPadX,
         Snap::Left|Snap::NoPadY,
         Snap::Top|Snap::NoPadX}, {},
        {16.0f, 30.0f}, {},
        {14.0f, 16.0f}, {16.0f, 30.0f},
        {3.0f, 0.0f, 1.0f, 0.0f}, {4.0f, 0.0f, 0.0f, 0.0f}},
    {"propagate forward direction, node padding smaller than child margin, no node margin, center",
        {SnapLayoutFlag::PropagateMarginX,
         SnapLayoutFlag::PropagateMarginY},
        {Snap::Right,
         Snap::Bottom,
         Snap::Left,
         Snap::Top}, {},
        /* Left and right same as above, vertically the margin fits exactly
           and thus affects inner padding instead of outer margin */
        {16.0f, 30.0f}, {},
        {14.0f, 30.0f}, {16.0f, 30.0f},
        {3.0f, 5.0f, 1.0f, 9.0f}, {4.0f, 0.0f, 0.0f, 0.0f}},
    {"propagate side direction, node padding smaller than child margin, no node margin, center",
        {SnapLayoutFlag::PropagateMarginY,
         SnapLayoutFlag::PropagateMarginX},
        {Snap::Right,
         Snap::Bottom,
         Snap::Left,
         Snap::Top}, {},
        /* Top and bottom same as above, horizontally the margin doesn't fit
           and thus affects inner padding and enlarges the width instead of
           overflowing into outer margin */
        {20.0f, 30.0f}, {},
        {20.0f, 22.0f}, {20.0f, 30.0f},
        {7.0f, 2.0f, 3.0f, 4.0f}, {0.0f, 0.0f, 0.0f, 1.0f}},

    {"propagate both directions, node padding smaller than child margin, no node margin, one side",
        {SnapLayoutFlag::PropagateMargin,
         SnapLayoutFlag::PropagateMargin},
        {Snap::Right|Snap::Top|Snap::InsideY,
         Snap::Bottom|Snap::Right|Snap::InsideX,
         Snap::Left|Snap::Bottom|Snap::InsideY,
         Snap::Top|Snap::Left|Snap::InsideX}, {},
        /* The left-side and top-side margin is fully propagated outside,
           there's extra space of 7 on the other side, which causes the bottom
           margin to overflow by 2. Right-side margin overflows by 1. */
        {15.0f, 25.0f}, {},
        {14.0f, 22.0f}, {15.0f, 25.0f},
        {3.0f, 2.0f, 1.0f, 4.0f}, {4.0f, 3.0f, 1.0f, 2.0f}},
    {"propagate forward direction, node padding smaller than child margin, no node margin, one side",
        {SnapLayoutFlag::PropagateMarginX,
         SnapLayoutFlag::PropagateMarginY},
        {Snap::Right|Snap::Top|Snap::InsideY,
         Snap::Bottom|Snap::Right|Snap::InsideX,
         Snap::Left|Snap::Bottom|Snap::InsideY,
         Snap::Top|Snap::Left|Snap::InsideX}, {},
        {15.0f, 25.0f}, {},
        /* Left and right same as above, vertically the margin doesn't fit and
           thus affects inner padding and enlarges the height instead of
           overflowing into outer margin */
        {14.0f, 30.0f}, {15.0f, 30.0f},
        {3.0f, 5.0f, 1.0f, 9.0f}, {4.0f, 0.0f, 1.0f, 0.0f}},
    /* No point in verifying forward direction not propagated, as that's the
       same output as with the center variant above. Similarly not verifying
       the NoPad cases, as they should have been sufficiently tested with the
       center variant. */

    {"propagate both directions, node padding smaller than child margin, no node margin, other side",
        {SnapLayoutFlag::PropagateMargin,
         SnapLayoutFlag::PropagateMargin},
        {Snap::Right|Snap::Bottom|Snap::InsideY,
         Snap::Bottom|Snap::Left|Snap::InsideX,
         Snap::Left|Snap::Top|Snap::InsideY,
         Snap::Top|Snap::Right|Snap::InsideX}, {},
        /* The left-side and bottom-side margin is fully propagated outside,
           there's extra space of 2 on the other side, which causes the top
           margin to overflow by 3. Right-side margin underflows by 1. */
        {17.0f, 22.0f}, {},
        {14.0f, 22.0f}, {17.0f, 22.0f},
        {3.0f, 2.0f, 1.0f, 4.0f}, {4.0f, 3.0f, 0.0f, 5.0f}},
    {"propagate forward direction, node padding smaller than child margin, no node margin, other side",
        {SnapLayoutFlag::PropagateMarginX,
         SnapLayoutFlag::PropagateMarginY},
        {Snap::Right|Snap::Bottom|Snap::InsideY,
         Snap::Bottom|Snap::Left|Snap::InsideX,
         Snap::Left|Snap::Top|Snap::InsideY,
         Snap::Top|Snap::Right|Snap::InsideX}, {},
        {17.0f, 22.0f}, {},
        {14.0f, 30.0f}, {17.0f, 30.0f},
        /* Left and right same as above, vertically the same as with the other
           side above */
        {3.0f, 5.0f, 1.0f, 9.0f}, {4.0f, 0.0f, 0.0f, 0.0f}},
    /* No point in verifying forward direction not propagated, as that's the
       same output as with the center variant above. Similarly not verifying
       the NoPad cases, as they should have been sufficiently tested with the
       center variant. */

    {"propagate both directions, node padding smaller than child margin, no node margin, fill",
        {SnapLayoutFlag::PropagateMargin,
         SnapLayoutFlag::PropagateMargin},
        {Snap::Right|Snap::FillY,
         Snap::Bottom|Snap::FillX,
         Snap::Left|Snap::FillY,
         Snap::Top|Snap::FillX}, {},
        /* All margins except the right-side one are fully propagated outside,
           right-side margin overflows by 2. */
        {14.0f, 30.0f}, {},
        {14.0f, 22.0f}, {14.0f, 30.0f},
        {3.0f, 2.0f, 1.0f, 4.0f}, {4.0f, 3.0f, 2.0f, 5.0f}},
    /* Should be exactly the same as above */
    {"propagate both directions, node padding smaller than child margin, no node margin, fill, redundant Inside",
        {SnapLayoutFlag::PropagateMargin,
         SnapLayoutFlag::PropagateMargin},
        {Snap::Right|Snap::FillY|Snap::InsideY,
         Snap::Bottom|Snap::FillX|Snap::InsideX,
         Snap::Left|Snap::FillY|Snap::InsideY,
         Snap::Top|Snap::FillX|Snap::InsideX}, {},
        {14.0f, 30.0f}, {},
        {14.0f, 22.0f}, {14.0f, 30.0f},
        {3.0f, 2.0f, 1.0f, 4.0f}, {4.0f, 3.0f, 2.0f, 5.0f}},
    {"propagate forward direction, node padding smaller than child margin, no node margin, fill",
        {SnapLayoutFlag::PropagateMarginX,
         SnapLayoutFlag::PropagateMarginY},
        {Snap::Right|Snap::FillY,
         Snap::Bottom|Snap::FillX,
         Snap::Left|Snap::FillY,
         Snap::Top|Snap::FillX}, {},
        /* Left and right same as above, vertically the margin affects inner
           padding instead of overflowing into outer margin */
        {14.0f, 30.0f}, {},
        {14.0f, 30.0f}, {14.0f, 30.0f},
        {3.0f, 5.0f, 1.0f, 9.0f}, {4.0f, 0.0f, 2.0f, 0.0f}},
    /* No point in verifying forward direction not propagated, as that's the
       same output as with the center variant above. Similarly not verifying
       the NoPad cases, as they should have been sufficiently tested with the
       center variant. */

    /* Like the "fill" cases above but with the fill coming from children
       instead */
    {"propagate both directions, node padding smaller than child margin, no node margin, has fill children in forward direction",
        {SnapLayoutFlag::PropagateMargin,
         SnapLayoutFlag::PropagateMargin},
         /* The snap is rotated compared to above so the children fill
            corresponds to Right|FillY etc. above */
        {Snap::Bottom,
         Snap::Left,
         Snap::Top,
         Snap::Right}, 2,
        {14.0f, 30.0f}, {},
        {14.0f, 22.0f}, {14.0f, 30.0f},
        {3.0f, 2.0f, 1.0f, 4.0f}, {4.0f, 3.0f, 2.0f, 5.0f}},

    {"propagate both directions, node size zero forward, large side, no node margin",
        {SnapLayoutFlag::PropagateMargin,
         SnapLayoutFlag::PropagateMargin},
        {Snap::Right,
         Snap::Bottom,
         Snap::Left,
         Snap::Top}, {},
        {0.0f, 100.0f}, {},
        {14.0f, 22.0f}, {14.0f, 100.0f},
        {3.0f, 2.0f, 1.0f, 4.0f}, {4.0f, 0.0f, 2.0f, 0.0f}},
    {"propagate both directions, node size large forward, zero side, no node margin",
        {SnapLayoutFlag::PropagateMargin,
         SnapLayoutFlag::PropagateMargin},
        {Snap::Right,
         Snap::Bottom,
         Snap::Left,
         Snap::Top}, {},
        {100.0f, 0.0f}, {},
        /* The margin before is propagated always, even with enough width */
        {14.0f, 22.0f}, {100.0f, 22.0f},
        {3.0f, 2.0f, 1.0f, 4.0f}, {4.0f, 3.0f, 0.0f, 5.0f}},

    {"propagate both directions, zero node size, node margin, variant 1",
        {SnapLayoutFlag::PropagateMargin,
         SnapLayoutFlag::PropagateMargin},
        {Snap::Right,
         Snap::Bottom,
         Snap::Left,
         Snap::Top}, {},
        {}, {2.0f, 7.0f, 3.0f, 3.0f},
        /* Margin overflow is {4, 3, 2, 5}, so a bigger value gets picked on
           top and right */
        {14.0f, 22.0f}, {14.0f, 22.0f},
        {3.0f, 2.0f, 1.0f, 4.0f}, {4.0f, 7.0f, 3.0f, 5.0f}},
    {"propagate both directions, zero node size, node margin, variant 2",
        {SnapLayoutFlag::PropagateMargin,
         SnapLayoutFlag::PropagateMargin},
        {Snap::Right,
         Snap::Bottom,
         Snap::Left,
         Snap::Top}, {},
        {}, {5.0f, 2.0f, 1.0f, 7.0f},
        /* Margin overflow is {4, 3, 2, 5}, so a bigger value gets picked on
           left and bottom */
        {14.0f, 22.0f}, {14.0f, 22.0f},
        {3.0f, 2.0f, 1.0f, 4.0f}, {5.0f, 3.0f, 2.0f, 7.0f}},

    {"propagate both directions, zero node size, node margin all smaller, no pad forward",
        {SnapLayoutFlag::PropagateMargin,
         SnapLayoutFlag::PropagateMargin},
        {Snap::Right|Snap::NoPadX,
         Snap::Bottom|Snap::NoPadY,
         Snap::Left|Snap::NoPadX,
         Snap::Top|Snap::NoPadY}, {},
        {}, {2.0f, 2.0f, 1.0f, 3.0f},
        /* Margin overflow is {4, 3, 2, 5}, but gets picked only on top and
           bottom. The parent node margin is used unchanged otherwise. Only the
           inner padding and size is affected by NoPad, margin isn't zeroed. */
        {10.0f, 22.0f}, {10.0f, 22.0f},
        {0.0f, 2.0f, 0.0f, 4.0f}, {2.0f, 3.0f, 1.0f, 5.0f}},
    {"propagate both directions, zero node size, node margin all smaller, no pad side",
        {SnapLayoutFlag::PropagateMargin,
         SnapLayoutFlag::PropagateMargin},
        {Snap::Right|Snap::NoPadY,
         Snap::Bottom|Snap::NoPadX,
         Snap::Left|Snap::NoPadY,
         Snap::Top|Snap::NoPadX}, {},
        {}, {2.0f, 2.0f, 1.0f, 3.0f},
        /* Margin overflow is {4, 3, 2, 5}, but gets picked only on left and
           right. The parent node margin is used unchanged otherwise. Only the
           inner padding and size is affected by NoPad, margin isn't zeroed. */
        {14.0f, 16.0f}, {14.0f, 16.0f},
        {3.0f, 0.0f, 1.0f, 0.0f}, {4.0f, 2.0f, 2.0f, 3.0f}},
};

const struct {
    TestSuite::TestCaseDescriptionSourceLocation name;
    Snaps childSnaps[4];
    Vector2 marginsSide[3];
    Float sizesSide[3];
    Float expectedSizeSide;
    Vector2 expectedPaddingSide;
    Float expectedOffsetsSide[3];
    Float expectedSizesSide[3]; /* If left at {}, sizeSide is used */
} ChildLayoutSizeSideData[]{
    /* All diagrams for left-to-right direction (first in childSnap), @ denotes
       child margin */

    /* +---+ +---+ +---+
       |   | |   | |   |
       +---+ +---+ +---+ */
    {"same sizes, no margins, center",
        {Snap::Right, Snap::Bottom, Snap::Left, Snap::Top},
        {},
        {20.0f, 20.0f, 20.0f},
        20.0f, {}, {0.0f, 0.0f, 0.0f}, {}},
    /* @@@@@ @@@@@ @@@@@
       +---+ +---+ +---+
       |   | |   | |   |
       +---+ +---+ +---+
       @@@@@ @@@@@ @@@@@ */
    {"same sizes, even margins, center",
        {Snap::Right, Snap::Bottom, Snap::Left, Snap::Top},
        {{5.0f, 5.0f}, {5.0f, 5.0f}, {5.0f, 5.0f}},
        {20.0f, 20.0f, 20.0f},
        20.0f, {5.0f, 5.0f}, {5.0f, 5.0f, 5.0f}, {}},
    /* +---+ +---+ +---+
       |   | |   | |   |
       +---+ +---+ +---+ */
    {"same sizes, even margin, center, no pad",
        {Snap::Right|Snap::NoPadY,
         Snap::Bottom|Snap::NoPadX,
         Snap::Left|Snap::NoPadY,
         Snap::Top|Snap::NoPadX},
        {{5.0f, 5.0f}, {5.0f, 5.0f}, {5.0f, 5.0f}},
        {20.0f, 20.0f, 20.0f},
        20.0f, {}, {0.0f, 0.0f, 0.0f}, {}},
    /* @@@@@
       @@@@@
       @@@@@       @@@@@
       +---+ +---+ +---+    (same for all four cases below)
       |   | |   | |   |
       +---+ +---+ +---+
             @@@@@
             @@@@@ */
    {"same sizes, uneven margin, center",
        {Snap::Right, Snap::Bottom, Snap::Left, Snap::Top},
        {{15.0f, 0.0f}, {0.0f, 10.0f}, {5.0f, 0.0f}},
        {20.0f, 20.0f, 20.0f},
        20.0f, {15.0f, 10.0f}, {15.0f, 15.0f, 15.0f}, {}},
    {"same sizes, uneven margin, fill",
        {Snap::Right|Snap::FillY,
         Snap::Bottom|Snap::FillX,
         Snap::Left|Snap::FillY,
         Snap::Top|Snap::FillX},
        {{15.0f, 0.0f}, {0.0f, 10.0f}, {5.0f, 0.0f}},
        {20.0f, 20.0f, 20.0f},
        20.0f, {15.0f, 10.0f}, {15.0f, 15.0f, 15.0f}, {}},
    {"same sizes, uneven margin, side 1",
        {Snap::TopRight|Snap::InsideY,
         Snap::BottomRight|Snap::InsideX,
         Snap::BottomLeft|Snap::InsideY,
         Snap::TopLeft|Snap::InsideX},
        {{15.0f, 0.0f}, {0.0f, 10.0f}, {5.0f, 0.0f}},
        {20.0f, 20.0f, 20.0f},
        20.0f, {15.0f, 10.0f}, {15.0f, 15.0f, 15.0f}, {}},
    {"same sizes, uneven padding and margin, side 2",
        {Snap::BottomRight|Snap::InsideY,
         Snap::BottomLeft|Snap::InsideX,
         Snap::TopLeft|Snap::InsideY,
         Snap::TopRight|Snap::InsideX},
        {{15.0f, 0.0f}, {0.0f, 10.0f}, {5.0f, 0.0f}},
        {20.0f, 20.0f, 20.0f},
        20.0f, {15.0f, 10.0f}, {15.0f, 15.0f, 15.0f}, {}},
    /* @@@@@
       +---+ +---+ +---+
       |   | |   | !   |
       +---+ |   | +---+
             |   |
             +---+
             @@@@@       */
    {"one node larger, side 1",
        {Snap::TopRight|Snap::InsideY,
         Snap::BottomRight|Snap::InsideX,
         Snap::BottomLeft|Snap::InsideY,
         Snap::TopLeft|Snap::InsideX},
        {{5.0f, 0.0f}, {0.0f, 5.0f}, {}},
        {20.0f, 40.0f, 20.0f},
        40.0f, {5.0f, 5.0f}, {5.0f, 5.0f, 5.0f}, {}},
    /*       +---+
       @@@@@ |   |
       +---+ |   | +---+
       |   | |   | |   |
       +---+ +---+ +---+
             @@@@@       */
    {"one node larger, side 2",
        {Snap::BottomRight|Snap::InsideY,
         Snap::BottomLeft|Snap::InsideX,
         Snap::TopLeft|Snap::InsideY,
         Snap::TopRight|Snap::InsideX},
        {{5.0f, 0.0f}, {0.0f, 5.0f}, {}},
        {20.0f, 40.0f, 20.0f},
        40.0f, {0.0f, 5.0f}, {20.0f, 0.0f, 20.0f}, {}},
    /* @@@@@ +---+
       +---+ |   | +---+
       |   | |   | |   |
       +---+ |   | +---+
             +---+
             @@@@@       */
    {"one node larger, center",
        {Snap::Right, Snap::Bottom, Snap::Left, Snap::Top},
        {{5.0f, 0.0f}, {0.0f, 5.0f}, {}},
        {20.0f, 40.0f, 20.0f},
        40.0f, {0.0f, 5.0f}, {10.0f, 0.0f, 10.0f}, {}},
    /* @@@@@
       +---+ +---+ +---+
       |   | |   | !   |
       |   | |   | !   |
       |   | |   | !   |
       +---+ +---+ +---+
             @@@@@       */
    {"one node larger, fill",
        {Snap::Right|Snap::FillY,
         Snap::Bottom|Snap::FillX,
         Snap::Left|Snap::FillY,
         Snap::Top|Snap::FillX},
        {{5.0f, 0.0f}, {0.0f, 5.0f}, {}},
        {20.0f, 40.0f, 20.0f},
        40.0f, {5.0f, 5.0f}, {5.0f, 5.0f, 5.0f}, {40.0f, 40.0f, 40.0f}},
    /*       @@@@@
       +---+ @@@@@ +---+
       |   | +---+ |   |
       |   | |   | |   |
       |   | +---+ |   |
       +---+ @@@@@ +---+
             @@@@@
             @@@@@       */
    {"one node smaller with excessive margin, center",
        {Snap::Right, Snap::Bottom, Snap::Left, Snap::Top},
        {{}, {10.0f, 15.0f}, {}},
        {30.0f, 20.0f, 30.0f},
        30.0f, {5.0f, 10.0f}, {5.0f, 10.0f, 5.0f}, {}},
    /*       @@@@@
             @@@@@
       +---+ +---+ +---+
       |   | |   | |   |
       |   | |   | |   |
       |   | |   | |   |
       +---+ +---+ +---+
             @@@@@
             @@@@@
             @@@@@       */
    {"one node smaller with excessive margin, fill",
        {Snap::Right|Snap::FillY,
         Snap::Bottom|Snap::FillX,
         Snap::Left|Snap::FillY,
         Snap::Top|Snap::FillX},
        {{}, {10.0f, 15.0f}, {}},
        {30.0f, 20.0f, 30.0f},
        30.0f, {10.0f, 15.0f}, {10.0f, 10.0f, 10.0f}, {30.0f, 30.0f, 30.0f}},
    /*       @@@@@
       +---+ @@@@@ +---+
       |   | +---+ |   |
       |   | |   | |   |
       |   | +---+ |   |
       +---+ @@@@@ +---+
             @@@@@
             @@@@@ */
    {"one node smaller with excessive margin, center",
        {Snap::Right, Snap::Bottom, Snap::Left, Snap::Top},
        {{}, {10.0f, 15.0f}, {}},
        {30.0f, 20.0f, 30.0f},
        30.0f, {5.0f, 10.0f}, {5.0f, 10.0f, 5.0f}, {}},
    /* +---+       +---+
       |   | +---+ |   |
       |   | |   | |   |
       |   | +---+ |   |
       +---+       +---+ */
    {"one node smaller with excessive margin, center, no pad",
        {Snap::Right|Snap::NoPadY,
         Snap::Bottom|Snap::NoPadX,
         Snap::Left|Snap::NoPadY,
         Snap::Top|Snap::NoPadX},
        {{}, {10.0f, 15.0f}, {}},
        {30.0f, 20.0f, 30.0f},
        30.0f, {}, {0.0f, 5.0f, 0.0f}, {}},
};

const struct {
    TestSuite::TestCaseDescriptionSourceLocation name;
    Snaps extraChildSnaps[2]; /* for X and Y direction */
    Vector2 marginsForward[3];
    Float sizesForward[3];
    Float expectedSizeForward;
    Vector2 expectedPaddingForward;
    Float expectedOffsetsForward[3];
} ChildLayoutSizeForwardData[]{
    /* All diagrams for left-to-right direction, @ denotes child margin */

    /* +---+ +---+ +---+
       |   | |   | |   |
       +---+ +---+ +---+ */
    {"same sizes, no padding or margins",
        {}, {},
        {20.0f, 20.0f, 20.0f},
        60.0f, {}, {0.0f, 20.0f, 40.0f}},
    /* @ +---+ @ +---+ @ +---+ @
       @ |   | @ |   | @ |   | @
       @ +---+ @ +---+ @ +---+ @ */
    {"same sizes, even margins",
        {}, {{5.0f, 5.0f}, {5.0f, 5.0f}, {5.0f, 5.0f}},
        {20.0f, 20.0f, 20.0f},
        /* Size doesn't include the margin before/after */
        70.0f, {5.0f, 5.0f}, {5.0f, 30.0f, 55.0f}},
    /* +---+ +---+ +---+
       |   | |   | |   |
       +---+ +---+ +---+ */
    {"same sizes, even margins, no pad",
        {Snap::NoPadX, Snap::NoPadY},
        {{5.0f, 5.0f}, {5.0f, 5.0f}, {5.0f, 5.0f}},
        {20.0f, 20.0f, 20.0f},
        60.0f, {}, {0.0f, 20.0f, 40.0f}},
    /* @@@ +-----+ @@ +-+ @@@@ +---+ @
       @@@ |     | @@ | | @@@@ |   | @
       @@@ +-----+ @@ +-+ @@@@ +---+ @ */
    {"uneven sizes amd margins, variant 1",
        {}, {{15.0f, 10.0f}, {}, {20.0f, 5.0f}},
        {40.0f, 10.0f, 20.0f},
        /* Size doesn't include the margin before/after */
        100.0f, {15.0f, 5.0f}, {15.0f, 65.0f, 95.0f}},
    /* @@@ +-----+ @@ +-+ @@@@ +---+ @
       @@@ |     | @@ | | @@@@ |   | @
       @@@ +-----+ @@ +-+ @@@@ +---+ @ */
    {"uneven sizes amd margins, variant 2",
        {}, {{15.0f, 0.0f}, {10.0f, 20.0f}, {0.0f, 5.0f}},
        {40.0f, 10.0f, 20.0f},
        /* Size doesn't include the margin before/after */
        100.0f, {15.0f, 5.0f}, {15.0f, 65.0f, 95.0f}},
};

const struct {
    const char* name;
    SnapLayoutFlags parentFlags;
    Vector4 parentPadding;
    Snaps snaps[3];
    Vector2 nodeSizes[3];
    Vector4 nodeMargins[3];
    Vector2 expected;
} ExplicitlySnappedChildLayoutSizeData[]{
    {"no padding or margin",
        {}, {},
        {{}, {}, {}},
        /* Max X in second, max Y in last */
        {{20.0f, 10.0f}, {30.0f, 15.0f}, {10.0f, 40.0f}},
        {{}, {}, {}},
        {30.0f, 40.0f}},
    {"padding only",
        {}, {1.0f, 2.0f, 3.0f, 4.0f},
        {{}, {}, {}},
        /* Max X in second, max Y in last */
        {{20.0f, 10.0f}, {26.0f, 15.0f}, {10.0f, 34.0f}},
        {{}, {}, {}},
        {30.0f, 40.0f}},
    {"margin only",
        {}, {},
        {{}, {}, {}},
        /* Max X in second, max Y in last, but only if margin is applied */
        {{20.0f, 10.0f}, {10.0f, 15.0f}, {10.0f, 10.0f}},
        {{}, {10.0f, 0.0f, 10.0f, 0.0f}, {0.0f, 20.0f, 0.0f, 10.0f}},
        {30.0f, 40.0f}},
    {"padding + margin",
        {}, {10.0f, 0.0f, 0.0f, 5.0f},
        {{}, {}, {}},
        /* Max X in second, max Y in last, but only with max(margin, padding);
           left and bottom padding is larger, top and right margin is larger */
        {{20.0f, 10.0f}, {15.0f, 15.0f}, {10.0f, 10.0f}},
        {{}, {0.0f, 0.0f, 5.0f, 0.0f}, {0.0f, 25.0f, 0.0f, 0.0f}},
        {30.0f, 40.0f}},
    {"padding + margin, IgnoreOverflow",
        SnapLayoutFlag::IgnoreOverflow, Vector4{3.0f},
        {{}, {}, {}},
        /* Max X in second, max Y in last */
        {{20.0f, 10.0f}, {24.0f, 15.0f}, {10.0f, 34.0f}},
        {Vector4{3.0f}, Vector4{3.0f}, Vector4{3.0f}},
        {0.0f, 0.0f}},
    {"padding + margin, IgnoreOverflowX",
        SnapLayoutFlag::IgnoreOverflowX, Vector4{3.0f},
        {{}, {}, {}},
        /* Max X in second, max Y in last */
        {{20.0f, 10.0f}, {30.0f, 15.0f}, {10.0f, 34.0f}},
        {Vector4{3.0f}, Vector4{3.0f}, Vector4{3.0f}},
        {0.0f, 40.0f}},
    {"padding + margin, IgnoreOverflowY",
        SnapLayoutFlag::IgnoreOverflowY, Vector4{3.0f},
        {{}, {}, {}},
        /* Max X in second, max Y in last */
        {{20.0f, 10.0f}, {24.0f, 15.0f}, {10.0f, 40.0f}},
        {Vector4{3.0f}, Vector4{3.0f}, Vector4{3.0f}},
        {30.0f, 0.0f}},
    {"padding + margin, PropagateMargin",
        SnapLayoutFlag::PropagateMargin, Vector4{3.0f},
        {{}, {}, {}},
        /* Max X in second, max Y in last, padding included but margin not */
        {{20.0f, 10.0f}, {24.0f, 15.0f}, {10.0f, 34.0f}},
        {Vector4{7.0f}, Vector4{7.0f}, Vector4{7.0f}},
        {30.0f, 40.0f}},
    {"padding + margin, PropagateMarginX",
        SnapLayoutFlag::PropagateMarginX, Vector4{3.0f},
        {{}, {}, {}},
        /* Max X in second, max Y in last, padding included but margin not */
        {{20.0f, 10.0f}, {24.0f, 15.0f}, {10.0f, 26.0f}},
        {Vector4{7.0f}, Vector4{7.0f}, Vector4{7.0f}},
        {30.0f, 40.0f}},
    {"padding + margin, PropagateMarginY",
        SnapLayoutFlag::PropagateMarginY, Vector4{3.0f},
        {{}, {}, {}},
        /* Max X in second, max Y in last, padding included but margin not */
        {{15.0f, 10.0f}, {16.0f, 15.0f}, {10.0f, 34.0f}},
        {Vector4{7.0f}, Vector4{7.0f}, Vector4{7.0f}},
        {30.0f, 40.0f}},
    {"padding + margin, NoPad",
        {}, Vector4{3.0f},
        {{}, Snap::NoPad, Snap::NoPad},
        /* Max X in second, max Y in last */
        {{20.0f, 10.0f}, {30.0f, 15.0f}, {10.0f, 40.0f}},
        {Vector4{3.0f}, Vector4{3.0f}, Vector4{3.0f}},
        {30.0f, 40.0f}},
    {"padding + margin, NoPadX/Y",
        {}, Vector4{3.0f},
        {{}, Snap::NoPadX, Snap::NoPadY},
        /* Max X in second, max Y in last */
        {{20.0f, 10.0f}, {30.0f, 15.0f}, {10.0f, 40.0f}},
        {Vector4{3.0f}, Vector4{3.0f}, Vector4{3.0f}},
        {30.0f, 40.0f}},
    {"padding + margin, arbitrary snaps",
        {}, Vector4{3.0f},
        {Snap::Fill, Snap::Right, Snap::Top},
        /* Max X in second, max Y in last. The snaps don't affect the size in
           any way */
        {{20.0f, 10.0f}, {24.0f, 15.0f}, {10.0f, 34.0f}},
        {Vector4{3.0f}, Vector4{3.0f}, Vector4{3.0f}},
        {30.0f, 40.0f}},
};

const struct {
    const char* name;
    Snaps snapMask;
    Float extraAvailableSize;
    Float sizes[4];
    Float expectedSizes[4];
} ExpandChildLayoutsData[]{
    {"no expanding children", {}, 100.0f,
        {30.0f, 5.0f, 15.0f, 10.0f},
        {30.0f, 5.0f, 15.0f, 10.0f}},
    {"no extra available size", ~Snaps{}, 0.0f,
        {30.0f, 5.0f, 15.0f, 10.0f},
        {30.0f, 5.0f, 15.0f, 10.0f}},
    {"all sizes the same", ~Snaps{}, 40.0f,
        {30.0f, 30.0f, 30.0f, 30.0f},
        {40.0f, 40.0f, 40.0f, 40.0f}},
    {"all sizes different, expanded the same", ~Snaps{}, 100.0f,
        {30.0f, 5.0f, 15.0f, 10.0f},
        {40.0f, 40.0f, 40.0f, 40.0f}},
    {"one size too large", ~Snaps{}, 45.0f,
        {30.0f, 5.0f, 15.0f, 10.0f},
        {30.0f, 25.0f, 25.0f, 25.0f}},
    {"one size too large, different order", ~Snaps{}, 45.0f,
        {5.0f, 15.0f, 10.0f, 30.0f},
        {25.0f, 25.0f, 25.0f, 30.0f}},
    /* This should internally do just the simple logic without sorting */
    {"one size too large, others expanded to exactly match it", ~Snaps{}, 60.0f,
        {30.0f, 5.0f, 15.0f, 10.0f},
        {30.0f, 30.0f, 30.0f, 30.0f}},
    /* This should internally adjust just two (and not three) */
    {"two sizes too large, remaining two expanded to exactly match the second", ~Snaps{}, 15.0f,
        {30.0f, 5.0f, 15.0f, 10.0f},
        {30.0f, 15.0f, 15.0f, 15.0f}},
    {"all sizes except one too large", ~Snaps{}, 4.0f,
        {5.0f, 30.0f, 15.0f, 10.0f},
        {9.0f, 30.0f, 15.0f, 10.0f}},
    {"all sizes except one too large, different order", ~Snaps{}, 4.0f,
        {10.0f, 15.0f, 30.0f, 5.0f},
        {10.0f, 15.0f, 30.0f, 9.0f}},
};

const struct {
    const char* name;
    bool clean;
} AddRemoveData[]{
    {"", false},
    {"clean to remove", true},
};

const struct {
    const char* name;
    bool recycledLayouts, shuffledChildLayouts, removedLayouts;
} LayoutDataOrderData[]{
    {"",
        false, false, false},
    {"layouts recycled in shuffled order",
        true, false, false},
    {"child layouts added in shuffled order",
        false, true, false},
    {"layouts recycled in shuffled order, child layouts added in shuffled order",
        true, true, false},
    {"with removed layouts",
        false, false, true},
};

const struct {
    const char* name;
    /* Individual edges tested sufficiently in snap() */
    Float paddingTarget, marginTarget, margin;
    Vector2 minSize;
    Containers::Optional<Snaps> explicitSnap;
    /* The UI has a size of {500, 600} */
    bool child, targetUi, xfailMinSize;
    /* The target is at {300, 400} with a size of {100, 200}, snapped node size
       is {50, 100}. A constant node offset is additionally supplied by the
       test case, which is added to the expectedOffset. */
    Vector2 expectedOffset, expectedSize;
} LayoutLayoutPropertiesData[]{
    /* Tests just that the padding / margin properties get used at all.
       Complete behavior tested in snap(), an "integration test" with multiple
       nodes having different paddings and margins is in updateDataOrder(). */
    {"target node padding, fill",
        10.0f, 0.0f, 0.0f, {},
        {Snap::Fill}, true, false, false,
        /* The node is a child, so its offset is relative to parent */
        {10.0f, 10.0f}, {80.0f, 180.0f}},
    {"margin, fill",
        0.0f, 0.0f, 10.0f, {},
        {Snap::Fill}, true, false, false,
        /* The node is a child, so its offset is relative to parent */
        {10.0f, 10.0f}, {80.0f, 180.0f}},
    {"margin, fill, target UI instead of a node",
        0.0f, 0.0f, 10.0f, {},
        {Snap::Fill}, false, true, false,
        {10.0f, 10.0f}, {480.0f, 580.0f}},
    {"target margin, outside",
        0.0f, 10.0f, 0.0f, {},
        {Snap::BottomRight}, false, false, false,
        {410.0f, 610.0f}, {50.0f, 100.0f}},
    {"margin, outside",
        0.0f, 0.0f, 10.0f, {},
        {Snap::BottomRight}, false, false, false,
        {410.0f, 610.0f}, {50.0f, 100.0f}},
    {"margin, min size, fill",
        0.0f, 0.0f, 10.0f, {150.0f, 300.0f},
        {Snap::Fill}, true, false, false,
        /* The node is a child, so its offset is relative to parent. The min
           size causes the parent to be enlarged, so the node gets exactly
           what was specified as minimum. */
        {10.0f, 10.0f}, {150.0f, 300.0f}},
    {"min size, centered",
        0.0f, 0.0f, 0.0f, {70.0f, 120.0f},
        Snaps{}, true, false, false,
        /* The node is a child, so its offset is relative to parent */
        {15.0f, 40.0f}, {70.0f, 120.0f}},
    {"min size, implicitly snapped as a child",
        0.0f, 0.0f, 0.0f, {70.0f, 120.0f},
        {}, true, false, false,
        /* The default child snap is Bottom, so this is centered on the top
           edge */
        {15.0f, 0.0f}, {70.0f, 120.0f}},
    {"min size, implicitly snapped with no parent layout",
        0.0f, 0.0f, 0.0f, {70.0f, 120.0f},
        {}, false, false, false,
        /* There's no parent layout, so the offset is untouched */
        {}, {70.0f, 120.0f}},
    {"large, min size, centered",
        0.0f, 0.0f, 0.0f, {150.0f, 300.0f},
        Snaps{}, true, false, false,
        /* The min size causes the parent to be enlarged, so the node gets
           exactly what was specified as minimum. And because the parent is as
           large as the child, there's no offset at all. */
        {0.0f, 0.0f}, {150.0f, 300.0f}},
    {"min X size, outside, margin, fill Y",
        0.0f, 0.0f, 10.0f, {350.0f, 0.0f},
        Snap::Right|Snap::FillY, false, false, false,
        {410.0f, 400.0f}, {350.0f, 200.0f}},
    {"min size, outside, margin, fill Y",
        0.0f, 0.0f, 10.0f, {350.0f, 1500.0f},
        Snap::Right|Snap::FillY, false, false, true,
        /* The Y size is ignored in this case as well */
        /** @todo should probably enlarge the target node as well if snapping
            a sibling to it with Fill? */
        {410.0f, 400.0f}, {350.0f, 200.0f}},
    {"min Y size, outside, margin, fill X",
        0.0f, 0.0f, 10.0f, {0.0f, 150.0f},
        Snap::Bottom|Snap::FillX, false, false, false,
        {300.0f, 610.0f}, {100.0f, 150.0f}},
    {"min size, outside, margin, fill X",
        0.0f, 0.0f, 10.0f, {900.0f, 150.0f},
        Snap::Bottom|Snap::FillX, false, false, true,
        /* The X size is ignored in this case as well */
        /** @todo should probably enlarge the target node as well? here it
            doesn't have a layout to affect */
        {300.0f, 610.0f}, {100.0f, 150.0f}},
};

const struct {
    Snaps snap;
    const char* name;
    Float padding, childMargin;
    /* Parent is {100, 200}, the three children are {20, 30} originally */
    Vector2 offset, advance, childSize;
} LayoutChildSnapData[]{
    {Snap::Left,
        nullptr, 0.0f, 0.0f,
        {80.0f, 85.0f}, {-20.0f, 0.0f}, {20.0f, 30.0f}},
    {Snap::Left|Snap::InsideY, /* InsideY is implicit, same as above */
        nullptr, 0.0f, 0.0f,
        {80.0f, 85.0f}, {-20.0f, 0.0f}, {20.0f, 30.0f}},
    {Snap::Left|Snap::FillY,
        nullptr, 0.0f, 0.0f,
        {80.0f, 0.0f}, {-20.0f, 0.0f}, {20.0f, 200.0f}},
    {Snap::Left|Snap::FillY|Snap::InsideY, /* InsideY is implicit, same as above */
        nullptr, 0.0f, 0.0f,
        {80.0f, 0.0f}, {-20.0f, 0.0f}, {20.0f, 200.0f}},
    {Snap::Right,
        nullptr, 0.0f, 0.0f,
        {0.0f, 85.0f}, {20.0f, 0.0f}, {20.0f, 30.0f}},
    {Snap::Right|Snap::InsideY, /* InsideY is implicit, same as above */
        nullptr, 0.0f, 0.0f,
        {0.0f, 85.0f}, {20.0f, 0.0f}, {20.0f, 30.0f}},
    {Snap::Right|Snap::FillY,
        nullptr, 0.0f, 0.0f,
        {0.0f, 0.0f}, {20.0f, 0.0f}, {20.0f, 200.0f}},
    {Snap::Right|Snap::FillY|Snap::InsideY, /* InsideY is implicit, same as above */
        nullptr, 0.0f, 0.0f,
        {0.0f, 0.0f}, {20.0f, 0.0f}, {20.0f, 200.0f}},
    {Snap::Top,
        nullptr, 0.0f, 0.0f,
        {40.0f, 170.0f}, {0.0f, -30.0f}, {20.0f, 30.0f}},
    {Snap::Top|Snap::InsideX, /* InsideX is implicit, same as above */
        nullptr, 0.0f, 0.0f,
        {40.0f, 170.0f}, {0.0f, -30.0f}, {20.0f, 30.0f}},
    {Snap::Top|Snap::FillX,
        nullptr, 0.0f, 0.0f,
        {0.0f, 170.0f}, {0.0f, -30.0f}, {100.0f, 30.0f}},
    {Snap::Top|Snap::FillX|Snap::InsideX, /* InsideX is implicit, same as above */
        nullptr, 0.0f, 0.0f,
        {0.0f, 170.0f}, {0.0f, -30.0f}, {100.0f, 30.0f}},
    {Snap::Bottom,
        nullptr, 0.0f, 0.0f,
        {40.0f, 0.0f}, {0.0f, 30.0f}, {20.0f, 30.0f}},
    {{}, /* default, should be same as Bottom */
        "default", 0.0f, 0.0f,
        {40.0f, 0.0f}, {0.0f, 30.0f}, {20.0f, 30.0f}},
    {~Snaps{}, /* default, should be same as Bottom, explicit snap overload */
        "default, explicit snap overload", 0.0f, 0.0f,
        {40.0f, 0.0f}, {0.0f, 30.0f}, {20.0f, 30.0f}},
    {Snap::Bottom|Snap::InsideX, /* InsideX is implicit, same as above */
        nullptr, 0.0f, 0.0f,
        {40.0f, 0.0f}, {0.0f, 30.0f}, {20.0f, 30.0f}},
    {Snap::Bottom|Snap::FillX,
        nullptr, 0.0f, 0.0f,
        {0.0f, 0.0f}, {0.0f, 30.0f}, {100.0f, 30.0f}},
    {Snap::Bottom|Snap::FillX|Snap::InsideX, /* InsideX is implicit, same as above */
        nullptr, 0.0f, 0.0f,
        {0.0f, 0.0f}, {0.0f, 30.0f}, {100.0f, 30.0f}},
    {Snap::TopLeft|Snap::InsideX,
        nullptr, 0.0f, 0.0f,
        {0.0f, 170.0f}, {0.0f, -30.0f}, {20.0f, 30.0f}},
    {Snap::TopLeft|Snap::InsideY,
        nullptr, 0.0f, 0.0f,
        {80.0f, 0.0f}, {-20.0f, 0.0f}, {20.0f, 30.0f}},
    {Snap::TopRight|Snap::InsideX,
        nullptr, 0.0f, 0.0f,
        {80.0f, 170.0f}, {0.0f, -30.0f}, {20.0f, 30.0f}},
    {Snap::TopRight|Snap::InsideY,
        nullptr, 0.0f, 0.0f,
        {0.0f, 0.0f}, {20.0f, 0.0f}, {20.0f, 30.0f}},
    {Snap::BottomLeft|Snap::InsideX,
        nullptr, 0.0f, 0.0f,
        {0.0f, 0.0f}, {0.0f, 30.0f}, {20.0f, 30.0f}},
    {Snap::BottomLeft|Snap::InsideY,
        nullptr, 0.0f, 0.0f,
        {80.0f, 170.0f}, {-20.0f, 0.0f}, {20.0f, 30.0f}},
    {Snap::BottomRight|Snap::InsideX,
        nullptr, 0.0f, 0.0f,
        {80.0f, 0.0f}, {0.0f, 30.0f}, {20.0f, 30.0f}},
    {Snap::BottomRight|Snap::InsideY,
        nullptr, 0.0f, 0.0f,
        {0.0f, 170.0f}, {20.0f, 0.0f}, {20.0f, 30.0f}},
    {Snap::Right|Snap::FillY,
        "with padding", 5.0f, 0.0f,
        {5.0f, 5.0f}, {20.0f, 0.0f}, {20.0f, 190.0f}},
    {Snap::Right|Snap::FillY,
        "with margin", 0.0f, 5.0f,
        {5.0f, 5.0f}, {25.0f, 0.0f}, {20.0f, 190.0f}},
    {Snap::Right|Snap::FillY,
        "with padding and margin", 5.0f, 5.0f,
        {5.0f, 5.0f}, {25.0f, 0.0f}, {20.0f, 190.0f}},
    {Snap::Right|Snap::FillY|Snap::NoPadX,
        "with padding and margin", 5.0f, 5.0f,
        {0.0f, 5.0f}, {20.0f, 0.0f}, {20.0f, 190.0f}},
    {Snap::Right|Snap::FillY|Snap::NoPadY,
        "with padding and margin", 5.0f, 5.0f,
        {5.0f, 0.0f}, {25.0f, 0.0f}, {20.0f, 200.0f}},
    {Snap::Right|Snap::FillY|Snap::NoPad,
        "with padding and margin", 5.0f, 5.0f,
        {0.0f, 0.0f}, {20.0f, 0.0f}, {20.0f, 200.0f}},
};

const struct {
    const char* name;
    Vector2 outerSize;
    SnapLayoutFlags outerFlags;
    Vector2 innerSize;
    SnapLayoutFlags innerFlags;
    bool innerReverseDirection;
    Vector2 leftSize, explicit4Size, explicit4MinSize;
    bool explicit4IsSibling;
    Float centerHeight;
    SnapLayoutFlags centerFlags;
    bool explicitlySnappedNodes;
    Vector2 expectedOuterSize;
    Float expectedInnerOffsetX;
    Vector2 expectedInnerSize;
    Float expectedInnerCenterY;
    Float expectedCenterHeight, expectedCenterOffsetY, expectedCenterLeftEdge;
} LayoutPropagateChildSizesData[]{
    {"with no explicitly snapped nodes",
        {}, {},
        {0.0f, 40.0f}, {}, false,
        {15.0f, 15.0f}, {}, {}, false,
        0.0f, {}, false,
        {50.0f, 50.0f}, 0.0f,
        {50.0f, 40.0f}, 0.0f,
        25.0f, 5.0f, 5.0f},
    {"",
        {}, {},
        {0.0f, 40.0f}, {}, false,
        {15.0f, 15.0f}, {}, {}, false,
        0.0f, {}, true,
        {50.0f, 50.0f}, 0.0f,
        {50.0f, 40.0f}, 0.0f,
        25.0f, 5.0f, 5.0f},
    {"exactly outer size, ignored overflow",
        {50.0f, 50.0f}, SnapLayoutFlag::IgnoreOverflow,
        {0.0f, 40.0f}, {}, false,
        {15.0f, 15.0f}, {}, {}, false,
        0.0f, {}, true,
        /* Same as above */
        {50.0f, 50.0f}, 0.0f,
        {50.0f, 40.0f}, 0.0f,
        25.0f, 5.0f, 5.0f},
    {"small outer size, ignored overflow",
        {30.0f, 30.0f}, SnapLayoutFlag::IgnoreOverflow,
        {0.0f, 40.0f}, {}, false,
        {15.0f, 15.0f}, {}, {}, false,
        0.0f, {}, true,
        /* Negative inner X offset because it overflows */
        {30.0f, 30.0f}, -10.0f,
        {50.0f, 40.0f}, 0.0f,
        25.0f, 5.0f, 5.0f},
    {"small outer size, ignored X overflow",
        {30.0f, 30.0f}, SnapLayoutFlag::IgnoreOverflowX,
        {0.0f, 40.0f}, {}, false,
        {15.0f, 15.0f}, {}, {}, false,
        0.0f, {}, true,
        /* Negative inner X offset because it overflows, Y size is enlarged */
        {30.0f, 50.0f}, -10.0f,
        {50.0f, 40.0f}, 0.0f,
        25.0f, 5.0f, 5.0f},
    {"small outer size, ignored Y overflow",
        {30.0f, 30.0f}, SnapLayoutFlag::IgnoreOverflowY,
        {0.0f, 40.0f}, {}, false,
        {15.0f, 15.0f}, {}, {}, false,
        0.0f, {}, true,
        /* Only X size is enlarged */
        {50.0f, 30.0f}, 0.0f,
        {50.0f, 40.0f}, 0.0f,
        25.0f, 5.0f, 5.0f},
    {"large outer size",
        {100.0f, 100.0f}, {},
        {0.0f, 40.0f}, {}, false,
        {15.0f, 15.0f}, {}, {}, false,
        0.0f, {}, true,
        {100.0f, 100.0f}, 25.0f,
        {50.0f, 40.0f}, 0.0f,
        25.0f, 5.0f, 5.0f},
    {"large outer size, ignored overflow",
        {100.0f, 100.0f}, SnapLayoutFlag::IgnoreOverflow,
        {0.0f, 40.0f}, {}, false,
        {15.0f, 15.0f}, {}, {}, false,
        0.0f, {}, true,
        /* Same as above */
        {100.0f, 100.0f}, 25.0f,
        {50.0f, 40.0f}, 0.0f,
        25.0f, 5.0f, 5.0f},
    {"exact inner width, ignored X overflow",
        {}, {},
        {50.0f, 40.0f}, SnapLayoutFlag::IgnoreOverflowX, false,
        {15.0f, 15.0f}, {}, {}, false,
        0.0f, {}, true,
        {50.0f, 50.0f}, 0.0f,
        {50.0f, 40.0f}, 0.0f,
        25.0f, 5.0f, 5.0f},
    {"small inner width, ignored X overflow",
        {}, {},
        {30.0f, 40.0f}, SnapLayoutFlag::IgnoreOverflowX, false,
        {15.0f, 15.0f}, {}, {}, false,
        0.0f, {}, true,
        /* The outer also becomes smaller because there's nothing to tell it
           that it should get larger */
        {30.0f, 50.0f}, 0.0f,
        {30.0f, 40.0f}, 0.0f,
        25.0f, 5.0f, 5.0f},
    {"large inner width",
        {}, {},
        {100.0f, 40.0f}, {}, false,
        {15.0f, 15.0f}, {}, {}, false,
        0.0f, {}, true,
        /* Enlarges the outer node */
        {100.0f, 50.0f}, 0.0f,
        {100.0f, 40.0f}, 0.0f,
        25.0f, 5.0f, 5.0f},
    {"large inner width, ignored X overflow",
        {}, {},
        {100.0f, 40.0f}, SnapLayoutFlag::IgnoreOverflowX, false,
        {15.0f, 15.0f}, {}, {}, false,
        0.0f, {}, true,
        /* Enlarges the outer node in this case as well, the Ignore is only
           taken into account for children */
        {100.0f, 50.0f}, 0.0f,
        {100.0f, 40.0f}, 0.0f,
        25.0f, 5.0f, 5.0f},
    {"inner with ignored Y overflow",
        {}, {},
        {0.0f, 40.0f}, SnapLayoutFlag::IgnoreOverflowY, false,
        {15.0f, 15.0f}, {}, {}, false,
        0.0f, {}, true,
        {50.0f, 50.0f}, 0.0f,
        /* Inner contents are not aligned with the `right` padding taken into
           account, shifted down by 2.5 units */
        {50.0f, 40.0f}, 2.5f,
        25.0f, 5.0f, 5.0f},
    {"exact center height, ignored Y overflow",
        {}, {},
        {0.0f, 40.0f}, {}, false,
        {15.0f, 15.0f}, {}, {}, false,
        25.0f, SnapLayoutFlag::IgnoreOverflowY, true,
        {50.0f, 50.0f}, 0.0f,
        {50.0f, 40.0f}, 0.0f,
        25.0f, 5.0f, 5.0f},
    {"small center height, ignored Y overflow",
        {}, {},
        {0.0f, 40.0f}, {}, false,
        {15.0f, 15.0f}, {}, {}, false,
        15.0f, SnapLayoutFlag::IgnoreOverflowY, true,
        {50.0f, 50.0f}, 0.0f,
        {50.0f, 40.0f}, 0.0f,
        /* Center gets aligned according to its original size */
        15.0f, 10.0f, 5.0f},
    {"large center height, ignored Y overflow",
        {}, {},
        {0.0f, 40.0f}, {}, false,
        {15.0f, 15.0f}, {}, {}, false,
        35.0f, SnapLayoutFlag::IgnoreOverflowY, true,
        {50.0f, 50.0f}, 0.0f,
        {50.0f, 40.0f}, 0.0f,
        /* Center gets aligned according to its original size */
        35.0f, 0.0f, 5.0f},
    {"center with ignored X overflow",
        {}, {},
        {0.0f, 40.0f}, {}, false,
        {15.0f, 15.0f}, {}, {}, false,
        0.0f, SnapLayoutFlag::IgnoreOverflowX, true,
        {50.0f, 50.0f}, 0.0f,
        {50.0f, 40.0f}, 0.0f,
        /* Center contents are not aligned with the `bottom` padding taken into
           account, shifted left by 5 units */
        25.0f, 5.0f, 0.0f},

    {"inner with X margin propagated",
        {}, {},
        {0.0f, 40.0f}, SnapLayoutFlag::PropagateMarginX, false,
        {15.0f, 15.0f}, {}, {}, false,
        0.0f, {}, true,
        {50.0f, 50.0f}, 0.0f,
        /* The inner width is without the right-side margin, other than that
           nothing changes */
        {45.0f, 40.0f}, 0.0f,
        25.0f, 5.0f, 5.0f},
    {"inner with X margin propagated, reverse layout direction",
        {}, {},
        {0.0f, 40.0f}, SnapLayoutFlag::PropagateMarginX, true,
        {15.0f, 15.0f}, {}, {}, false,
        0.0f, {}, true,
        /* Same as above, the right node (which is snapped to the parent)
           should be placed without horizontal margin applied */
        {50.0f, 50.0f}, 0.0f,
        {45.0f, 40.0f}, 0.0f,
        25.0f, 5.0f, 5.0f},
    {"inner with zero Y size and Y margin propagated",
        {}, {},
        {}, SnapLayoutFlag::PropagateMarginY, false,
        {15.0f, 15.0f}, {}, {}, false,
        0.0f, {}, true,
        /* The outer Y padding cancels out with the margin propagated from the
           inner, so the height is smaller */
        {50.0f, 40.0f}, 0.0f,
        /* The inner height matches the center height */
        {50.0f, 25.0f}, -5.0f,
        25.0f, 5.0f, 5.0f},
    {"inner with zero Y size and Y margin propagated, reverse layout direction",
        {}, {},
        {}, SnapLayoutFlag::PropagateMarginY, true,
        {15.0f, 15.0f}, {}, {}, false,
        0.0f, {}, true,
        /* Same as above, the right node (which is snapped to the parent)
           should be placed without vertical margin applied */
        {50.0f, 40.0f}, 0.0f,
        {50.0f, 25.0f}, -5.0f,
        25.0f, 5.0f, 5.0f},
    {"inner with zero size and margin propagated",
        {}, {},
        {}, SnapLayoutFlag::PropagateMargin, false,
        {15.0f, 15.0f}, {}, {}, false,
        0.0f, {}, true,
        /* Combination of the above, basically */
        {50.0f, 40.0f}, 0.0f,
        {45.0f, 25.0f}, -5.0f,
        25.0f, 5.0f, 5.0f},
    {"inner with zero size and margin propagated, reverse layout direction",
        {}, {},
        {}, SnapLayoutFlag::PropagateMargin, true,
        {15.0f, 15.0f}, {}, {}, false,
        0.0f, {}, true,
        /* Same as above, the right node (which is snapped to the parent)
           should be placed without margin applied */
        {50.0f, 40.0f}, 0.0f,
        {45.0f, 25.0f}, -5.0f,
        25.0f, 5.0f, 5.0f},
    {"left width propagated from an explicitly snapped child",
        {}, {},
        {0.0f, 40.0f}, {}, false,
        {0.0f, 15.0f}, {5.0f, 0.0f}, {}, false,
        0.0f, {}, true,
        {50.0f, 50.0f}, 0.0f,
        {50.0f, 40.0f}, 0.0f,
        25.0f, 5.0f, 5.0f},
    {"left height propagated from an explicitly snapped child",
        {}, {},
        {0.0f, 40.0f}, {}, false,
        {15.0f, 0.0f}, {0.0f, 15.0f}, {}, false,
        0.0f, {}, true,
        {50.0f, 50.0f}, 0.0f,
        {50.0f, 40.0f}, 0.0f,
        25.0f, 5.0f, 5.0f},
    {"left size propagated from an explicitly snapped child",
        {}, {},
        {0.0f, 40.0f}, {}, false,
        {0.0f, 0.0f}, {5.0f, 15.0f}, {}, false,
        0.0f, {}, true,
        {50.0f, 50.0f}, 0.0f,
        {50.0f, 40.0f}, 0.0f,
        25.0f, 5.0f, 5.0f},
    {"left size propagated from an explicitly snapped child size + min X size",
        {}, {},
        {0.0f, 40.0f}, {}, false,
        {0.0f, 0.0f}, {0.0f, 15.0f}, {5.0f, 0.0f}, false,
        0.0f, {}, true,
        {50.0f, 50.0f}, 0.0f,
        {50.0f, 40.0f}, 0.0f,
        25.0f, 5.0f, 5.0f},
    {"left size propagated from an explicitly snapped child size + min Y size",
        {}, {},
        {0.0f, 40.0f}, {}, false,
        {0.0f, 0.0f}, {5.0f, 0.0f}, {0.0f, 15.0f}, false,
        0.0f, {}, true,
        {50.0f, 50.0f}, 0.0f,
        {50.0f, 40.0f}, 0.0f,
        25.0f, 5.0f, 5.0f},
    {"left size not affected by an explicitly snapped sibling",
        {}, {},
        {0.0f, 40.0f}, {}, false,
        {15.0f, 15.0f}, {500.0f, 500.0f}, {}, true,
        0.0f, {}, true,
        {50.0f, 50.0f}, 0.0f,
        {50.0f, 40.0f}, 0.0f,
        25.0f, 5.0f, 5.0f},
};

const struct {
    TestSuite::TestCaseDescriptionSourceLocation name;
    bool minSizeInsteadOfFixed, explicitFillChildrenSize;
    Snaps rootFillInsteadOfSize;
    SnapLayoutFlags mainFlags, rightFlags;
} LayoutExpandChildLayoutsData[]{
    {"",
        false, false, Snaps{}, {}, {}},
    {"root fill X instead of width",
        false, false, Snap::FillX, {}, {}},
    {"root fill Y instead of height",
        false, false, Snap::FillY, {}, {}},
    {"root fill instead of size",
        false, false, Snap::Fill, {}, {}},
    /* The explicit size, if smaller than what the fill ends up being,
       shouldn't affect anything in the output */
    {"explicit size for fill children",
        false, true, Snaps{}, {}, {}},

    {"layout min sizes instead of fixed node sizes",
        true, false, Snaps{}, {}, {}},
    {"layout min sizes instead of fixed node sizes, root fill X instead of width",
        true, false, Snap::FillX, {}, {}},
    {"layout min sizes instead of fixed node sizes, root fill Y instead of height",
        true, false, Snap::FillY, {}, {}},
    {"layout min sizes instead of fixed node sizes, root fill instead of size",
        true, false, Snap::Fill, {}, {}},

    /* The edge case being tested here is hit only if the node has an explicit
       size specified in direction of the propagation. In case of `main` it's
       always the case. */
    {"main propagating X margin",
        false, false, Snaps{}, SnapLayoutFlag::PropagateMarginX, {}},
    /* In case of `right` it's not, so test both with and without te size being
       explicit */
    {"right propagating Y margin",
        false, false, Snaps{}, {}, SnapLayoutFlag::PropagateMarginY},
    {"right propagating Y margin, explicit right height",
        false, true, Snaps{}, {}, SnapLayoutFlag::PropagateMarginY},
};

const struct {
    TestSuite::TestCaseDescriptionSourceLocation name;
    Float size;
    Snaps snapX, snapY;
    SnapLayoutFlags flagsX, flagsY;
    Float expectedSize, expectedRootSize;
} LayoutExpandChildLayoutsOverflowData[]{
    {"small size",
        5.0f, {}, {}, {}, {},
        5.0f, 100.0f},
    /* This should cause no change to the positioning */
    {"small size, ignore overflow",
        5.0f, {}, {},
        SnapLayoutFlag::IgnoreOverflow, SnapLayoutFlag::IgnoreOverflow,
        5.0f, 100.0f},

    {"small size, expand to available size",
        5.0f, Snap::FillX, Snap::FillY, {}, {},
        15.0f, 100.0f},
    /* The other direction should be ignored */
    {"small size, expand to available size in all directions",
        5.0f, Snap::Fill, Snap::Fill, {}, {},
        15.0f, 100.0f},
    /* This should again cause no change to the positioning */
    {"small size, expand to available size, ignore overflow",
        5.0f, Snap::FillX, Snap::FillY,
        SnapLayoutFlag::IgnoreOverflowX, SnapLayoutFlag::IgnoreOverflowY,
        15.0f, 100.0f},
    /* Also */
    {"small size, expand to available size, ignore overflow in opposite dimension",
        5.0f, Snap::FillX, Snap::FillY,
        SnapLayoutFlag::IgnoreOverflowY, SnapLayoutFlag::IgnoreOverflowX,
        15.0f, 100.0f},
    {"small size, expand to available size, ignore overflow in all dimensions",
        5.0f, Snap::FillX, Snap::FillY,
        SnapLayoutFlag::IgnoreOverflow, SnapLayoutFlag::IgnoreOverflow,
        15.0f, 100.0f},

    {"large size",
        50.0f, {}, {}, {}, {},
        50.0f, 170.0f},
    /* Should cause no change */
    {"large size, ignore overflow in opposite dimension",
        50.0f, {}, {},
        SnapLayoutFlag::IgnoreOverflowY, SnapLayoutFlag::IgnoreOverflowX,
        50.0f, 170.0f},
    /* Only the root node size is affected, the children stay the same */
    {"large size, ignore overflow",
        50.0f, {}, {},
        SnapLayoutFlag::IgnoreOverflowX, SnapLayoutFlag::IgnoreOverflowY,
        50.0f, 100.0f},
    {"large size, ignore overflow in all dimensions",
        50.0f, {}, {},
        SnapLayoutFlag::IgnoreOverflow, SnapLayoutFlag::IgnoreOverflow,
        50.0f, 100.0f},
    /* This has nowhere to expand so it doesn't cause any change */
    {"large size, expand to available size",
        50.0f, Snap::FillX, Snap::FillY, {}, {},
        50.0f, 170.0f},
    /* Similarly here, only the root node size is affected, the children stay
       the same as if Fill wasn't specified */
    {"large size, expand to available size, ignore overflow",
        50.0f, Snap::FillX, Snap::FillY,
        SnapLayoutFlag::IgnoreOverflowX, SnapLayoutFlag::IgnoreOverflowY,
        50.0f, 100.0f},
    /* Combinations of the above */
    {"large size, expand to available size, ignore overflow in opposite dimension",
        50.0f, Snap::FillX, Snap::FillY,
        SnapLayoutFlag::IgnoreOverflowY, SnapLayoutFlag::IgnoreOverflowX,
        50.0f, 170.0f},
    {"large size, expand to available size, ignore overflow in all dimensions",
        50.0f, Snap::FillX, Snap::FillY,
        SnapLayoutFlag::IgnoreOverflow, SnapLayoutFlag::IgnoreOverflow,
        50.0f, 100.0f},
};

SnapLayouterTest::SnapLayouterTest() {
    addTests({&SnapLayouterTest::debugSnap,
              &SnapLayouterTest::debugSnapPacked,
              &SnapLayouterTest::debugSnaps,
              &SnapLayouterTest::debugSnapsPacked,
              &SnapLayouterTest::debugSnapsSupersets,

              &SnapLayouterTest::debugFlag,
              &SnapLayouterTest::debugFlags,
              &SnapLayouterTest::debugFlagsSupersets});

    addInstancedTests({&SnapLayouterTest::snapInsideImplementation},
        Containers::arraySize(SnapInsideImplementationData));

    addInstancedTests({&SnapLayouterTest::snapImplementation},
        Containers::arraySize(SnapImplementationData));

    addInstancedTests({&SnapLayouterTest::layoutSizePaddingMargin},
        Containers::arraySize(LayoutSizePaddingMarginData));

    addInstancedTests({&SnapLayouterTest::layoutSizePaddingMarginPropagate},
        Containers::arraySize(LayoutSizePaddingMarginPropagateData));

    addInstancedTests({&SnapLayouterTest::childLayoutSizeSide},
        Containers::arraySize(ChildLayoutSizeSideData));

    addInstancedTests({&SnapLayouterTest::childLayoutSizeForward},
        Containers::arraySize(ChildLayoutSizeForwardData));

    addInstancedTests({&SnapLayouterTest::explicitlySnappedChildLayoutSize},
        Containers::arraySize(ExplicitlySnappedChildLayoutSizeData));

    addInstancedTests({&SnapLayouterTest::expandChildLayouts},
        Containers::arraySize(ExpandChildLayoutsData));

    addTests({&SnapLayouterTest::orderLayoutsBreadthFirstEmpty,
              &SnapLayouterTest::orderLayoutsBreadthFirst,

              &SnapLayouterTest::construct,
              &SnapLayouterTest::constructCopy,
              &SnapLayouterTest::constructMove});

    addInstancedTests({&SnapLayouterTest::addRemove,
                       &SnapLayouterTest::addRemoveExplicitSnap},
        Containers::arraySize(AddRemoveData));

    addTests({&SnapLayouterTest::addRemoveHandleRecycle,
              &SnapLayouterTest::addInvalid,
              &SnapLayouterTest::removeInvalid,

              &SnapLayouterTest::flags,
              &SnapLayouterTest::flagsInvalid,

              &SnapLayouterTest::snap,
              &SnapLayouterTest::snapInvalid,

              &SnapLayouterTest::childSnap,
              &SnapLayouterTest::childSnapInvalid,

              &SnapLayouterTest::setSize,

              &SnapLayouterTest::invalidHandle,
              &SnapLayouterTest::invalidExplicitSnap,

              &SnapLayouterTest::clean,
              &SnapLayouterTest::cleanInvalid,

              &SnapLayouterTest::layoutEmpty});

    addInstancedTests({&SnapLayouterTest::layoutDataOrder},
        Containers::arraySize(LayoutDataOrderData));

    addInstancedTests({&SnapLayouterTest::layoutLayoutProperties},
        Containers::arraySize(LayoutLayoutPropertiesData));

    addInstancedTests({&SnapLayouterTest::layoutChildSnap},
        Containers::arraySize(LayoutChildSnapData));

    addInstancedTests({&SnapLayouterTest::layoutPropagateChildSizes},
        Containers::arraySize(LayoutPropagateChildSizesData));

    addInstancedTests({&SnapLayouterTest::layoutExpandChildLayouts},
        Containers::arraySize(LayoutExpandChildLayoutsData));

    addInstancedTests({&SnapLayouterTest::layoutExpandChildLayoutsOverflow},
        Containers::arraySize(LayoutExpandChildLayoutsOverflowData));
}

void SnapLayouterTest::debugSnap() {
    Containers::String out;
    Debug{&out} << Snap::InsideX << Snap(0xbe);
    CORRADE_COMPARE(out, "Ui::Snap::InsideX Ui::Snap(0xbe)\n");
}

void SnapLayouterTest::debugSnapPacked() {
    Containers::String out;
    /* Last is not packed, ones before should not make any flags persistent */
    Debug{&out} << Debug::packed << Snap::InsideX << Debug::packed << Snap(0xbe) << Snap::Right;
    CORRADE_COMPARE(out, "InsideX 0xbe Ui::Snap::Right\n");
}

void SnapLayouterTest::debugSnaps() {
    Containers::String out;
    /* There isn't any bit free to test how the remains get printed */
    Debug{&out} << (Snap::Left|Snap::InsideX) << Snaps{};
    CORRADE_COMPARE(out, "Ui::Snap::Left|Ui::Snap::InsideX Ui::Snaps{}\n");
}

void SnapLayouterTest::debugSnapsPacked() {
    Containers::String out;
    /* There isn't any bit free to test how the remains get printed. Last is
       not packed, ones before should not make any flags persistent. */
    Debug{&out} << Debug::packed << (Snap::Left|Snap::NoPadY) << Debug::packed << Snaps{} << (Snap::InsideX|Snap::NoPadY);
    CORRADE_COMPARE(out, "Left|NoPadY {} Ui::Snap::InsideX|Ui::Snap::NoPadY\n");
}

void SnapLayouterTest::debugSnapsSupersets() {
    /* Fill is all FillX and FillY combined */
    {
        Containers::String out;
        Debug{&out} << (Snap::Fill|Snap::FillX|Snap::FillY);
        CORRADE_COMPARE(out, "Ui::Snap::Fill\n");

    /* FillX and FillY is edges combined */
    } {
        Containers::String out;
        Debug{&out}
            << (Snap::FillX|Snap::Left|Snap::Right)
            << (Snap::FillY|Snap::Top|Snap::Bottom);
        CORRADE_COMPARE(out, "Ui::Snap::FillX Ui::Snap::FillY\n");

    /* Corners are edges combined */
    } {
        Containers::String out;
        Debug{&out}
            << (Snap::TopLeft|Snap::Top|Snap::Left)
            << (Snap::BottomLeft|Snap::Bottom|Snap::Left)
            << (Snap::TopRight|Snap::Top|Snap::Right)
            << (Snap::BottomRight|Snap::Bottom|Snap::Right);
        CORRADE_COMPARE(out, "Ui::Snap::TopLeft Ui::Snap::BottomLeft Ui::Snap::TopRight Ui::Snap::BottomRight\n");

    /* Combining corners + edges picks up the fill first, not corners */
    } {
        Containers::String out;
        Debug{&out}
            /* Both in each pair do the same */
            << (Snap::TopLeft|Snap::Right) << (Snap::FillX|Snap::Top)
            << (Snap::BottomRight|Snap::Top) << (Snap::FillY|Snap::Right);
        CORRADE_COMPARE(out, "Ui::Snap::FillX|Ui::Snap::Top Ui::Snap::FillX|Ui::Snap::Top Ui::Snap::FillY|Ui::Snap::Right Ui::Snap::FillY|Ui::Snap::Right\n");

    /* Inside is InsideX and InsideY combined */
    } {
        Containers::String out;
        Debug{&out} << (Snap::InsideX|Snap::InsideY);
        CORRADE_COMPARE(out, "Ui::Snap::Inside\n");

    /* NoPad is NoPadX and NoPadY combined */
    } {
        Containers::String out;
        Debug{&out} << (Snap::NoPadX |Snap::NoPadY);
        CORRADE_COMPARE(out, "Ui::Snap::NoPad\n");
    }
}

void SnapLayouterTest::debugFlag() {
    Containers::String out;
    Debug{&out} << SnapLayoutFlag::IgnoreOverflowY << SnapLayoutFlag(0xbe);
    CORRADE_COMPARE(out, "Ui::SnapLayoutFlag::IgnoreOverflowY Ui::SnapLayoutFlag(0xbe)\n");
}

void SnapLayouterTest::debugFlags() {
    Containers::String out;
    Debug{&out} << (SnapLayoutFlag::IgnoreOverflowX|SnapLayoutFlag(0xe0)) << SnapLayoutFlags{};
    CORRADE_COMPARE(out, "Ui::SnapLayoutFlag::IgnoreOverflowX|Ui::SnapLayoutFlag(0xe0) Ui::SnapLayoutFlags{}\n");
}

void SnapLayouterTest::debugFlagsSupersets() {
    /* IgnoreOverflow is IgnoreOverflowX and IgnoreOverflowY combined */
    {
        Containers::String out;
        Debug{&out} << (SnapLayoutFlag::IgnoreOverflow|SnapLayoutFlag::IgnoreOverflowX|SnapLayoutFlag::IgnoreOverflowY);
        CORRADE_COMPARE(out, "Ui::SnapLayoutFlag::IgnoreOverflow\n");

    /* PropagateMargin is PropagateMarginX and PropagateMarginY combined */
    } {
        Containers::String out;
        Debug{&out} << (SnapLayoutFlag::PropagateMargin|SnapLayoutFlag::PropagateMarginX|SnapLayoutFlag::PropagateMarginY);
        CORRADE_COMPARE(out, "Ui::SnapLayoutFlag::PropagateMargin\n");
    }
}

void SnapLayouterTest::snapInsideImplementation() {
    auto&& data = SnapInsideImplementationData[testCaseInstanceId()];
    {
        Containers::String out;
        Debug{&out, Debug::Flag::NoNewlineAtTheEnd|Debug::Flag::Packed} << data.snap;
        setTestCaseDescription(out);
    }

    CORRADE_COMPARE(Implementation::snapInside(data.snap), data.expected);
}

void SnapLayouterTest::snapImplementation() {
    auto&& data = SnapImplementationData[testCaseInstanceId()];
    {
        Containers::String out;
        {
            Debug debug{&out, Debug::Flag::NoNewlineAtTheEnd|Debug::Flag::Packed};
            debug << data.snap;
            if(data.name)
                debug << Debug::nospace << "," << data.name;
        }
        setTestCaseDescription(out);
    }

    Containers::Pair<Vector2, Vector2> out = Implementation::snap(data.snap,
        {100.0f, 200.0f}, {400.0f, 300.0f},
        data.referencePadding,
        data.referenceMargin,
        data.margin,
        Size);

    CORRADE_COMPARE(out, Containers::pair(data.expectedOffset, data.expectedSize));
}

void SnapLayouterTest::layoutSizePaddingMargin() {
    auto&& data = LayoutSizePaddingMarginData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Behavior with PropagateMargin is tested in
       layoutSizePaddingMarginPropagate() below, verifying all rotations
       variants at the same time, here just verify that it
       always passes the margin through unchanged. The value of hasFillChildren
       should also have no effect on the result without PropagateMargin. Value
       of 0b11 is never produced by childLayoutSizeMargin() and thus the
       layoutSizePaddingMargin() disallows it. */
    for(BitVector2 hasFillChildren: {0, 1 /*0b01*/, 2 /* 0b10 */}) {
        CORRADE_ITERATION(hasFillChildren);

        Implementation::LayoutSizePaddingMargin out = Implementation::layoutSizePaddingMargin(
            data.flags,
            data.extraChildSnap,
            data.nodeSize,
            data.nodePadding,
            {19.0f, 23.0f, 13.0f, 29.0f},
            data.childLayoutSize,
            data.childLayoutMargin,
            hasFillChildren);
        CORRADE_COMPARE(out.paddedChildSize, data.expectedPaddedChildLayoutSize);
        CORRADE_COMPARE(out.size, data.expectedLayoutSize);
        CORRADE_COMPARE(out.padding, data.expectedLayoutPadding);
        CORRADE_COMPARE(out.margin, (Vector4{19.0f, 23.0f, 13.0f, 29.0f}));
    }
}

void SnapLayouterTest::layoutSizePaddingMarginPropagate() {
    auto&& data = LayoutSizePaddingMarginPropagateData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    Vector2 nodeSizes[4];
    Vector4 nodeMargins[4];
    Vector4 nodePaddings[4];
    Vector2 childLayoutSizes[4];
    Vector4 childLayoutMargins[4];
    BitVector2 hasFillChildren[4];
    Vector2 expectedPaddedChildLayoutSizes[4];
    Vector2 expectedLayoutSizes[4];
    Vector4 expectedLayoutPaddings[4];
    Vector4 expectedLayoutMargins[4];

    /* Data for the first (left-to-right) rotation. In the side direction the
       sizes, margins and paddings are always constant. The node padding, child
       layout size and child layout margin is a constant always. */
    nodeSizes[0] = data.nodeSize;
    nodePaddings[0] = {3.0f, 2.0f, 1.0f, 4.0f};
    nodeMargins[0] = data.nodeMargin;
    childLayoutSizes[0] = {10.0f, 16.0f};
    childLayoutMargins[0] = {7.0f, 5.0f, 3.0f, 9.0f};
    hasFillChildren[0] = data.hasFillChildren;
    expectedPaddedChildLayoutSizes[0] = data.expectedPaddedChildLayoutSize;
    expectedLayoutSizes[0] = data.expectedLayoutSize;
    expectedLayoutPaddings[0] = data.expectedLayoutPadding;
    expectedLayoutMargins[0] = data.expectedLayoutMargin;

    /* Gradually rotate the inputs to generate all four rotations */
    for(UnsignedInt i = 0; i != 3; ++i) {
        nodeSizes[i + 1] = Math::gather<'y', 'x'>(nodeSizes[i]);
        nodeMargins[i + 1] = Math::gather<'w', 'x', 'y', 'z'>(nodeMargins[i]);
        nodePaddings[i + 1] = Math::gather<'w', 'x', 'y', 'z'>(nodePaddings[i]);
        childLayoutSizes[i + 1] = Math::gather<'y', 'x'>(childLayoutSizes[i]);
        childLayoutMargins[i + 1] = Math::gather<'w', 'x', 'y', 'z'>(childLayoutMargins[i]);
        /** @todo have Math::gather for BitVector as well, this is cryptic and
            error-prone */
        hasFillChildren[i + 1] = hasFillChildren[i][1] + 2*hasFillChildren[i][0];
        expectedPaddedChildLayoutSizes[i + 1] = Math::gather<'y', 'x'>(expectedPaddedChildLayoutSizes[i]);
        expectedLayoutSizes[i + 1] = Math::gather<'y', 'x'>(expectedLayoutSizes[i]);
        expectedLayoutPaddings[i + 1] = Math::gather<'w', 'x', 'y', 'z'>(expectedLayoutPaddings[i]);
        expectedLayoutMargins[i + 1] = Math::gather<'w', 'x', 'y', 'z'>(expectedLayoutMargins[i]);
    }

    for(UnsignedInt rotation = 0; rotation != 4; ++rotation) {
        CORRADE_ITERATION("rotation" << rotation);
        CORRADE_ITERATION(data.snaps[rotation]);

        Implementation::LayoutSizePaddingMargin out = Implementation::layoutSizePaddingMargin(
            data.flags[rotation % 2],
            data.snaps[rotation],
            nodeSizes[rotation],
            nodePaddings[rotation],
            nodeMargins[rotation],
            childLayoutSizes[rotation],
            childLayoutMargins[rotation],
            hasFillChildren[rotation]);
        CORRADE_COMPARE(out.paddedChildSize, expectedPaddedChildLayoutSizes[rotation]);
        CORRADE_COMPARE(out.size, expectedLayoutSizes[rotation]);
        CORRADE_COMPARE(out.padding, expectedLayoutPaddings[rotation]);
        CORRADE_COMPARE(out.margin, expectedLayoutMargins[rotation]);
    }
}

void SnapLayouterTest::childLayoutSizeSide() {
    auto&& data = ChildLayoutSizeSideData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct Layout {
        NodeHandle node;
        LayouterDataHandle next;
    } layouts[]{
        {},
        {nodeHandle(3, 0xbeb), layouterDataHandle(5, 0x555)}, /* layout 1 */
        {nodeHandle(8, 0xaba), layouterDataHandle(1, 0x111)}, /* layout 2 */
        {}, {},
        {nodeHandle(6, 0xcec), layouterDataHandle(2, 0x222)}, /* layout 5 */
    };

    Vector4 margins[4][3];
    Vector2 sizes[4][3];
    Vector2 expectedSizes[4];
    Vector4 expectedMargins[4];
    Vector2 expectedChildOffsets[4][3];
    Vector2 expectedChildSizes[4][3];

    /* Data for the first (left-to-right) rotation. In the forward direction
       the margin is always 3 from both sides, and all three nodes have the
       same width of 30. */
    expectedSizes[0] = {90.0f + 6.0f, data.expectedSizeSide};
    expectedMargins[0] = {3.0f, data.expectedPaddingSide[0], 3.0f, data.expectedPaddingSide[1]};
    expectedChildOffsets[0][0][0] = 3.0f;
    expectedChildOffsets[0][1][0] = expectedChildOffsets[0][0].x() + 30.0f + 3.0f;
    expectedChildOffsets[0][2][0] = expectedChildOffsets[0][1].x() + 30.0f + 3.0f;
    expectedChildSizes[0][0][0] = 30.0f;
    expectedChildSizes[0][1][0] = 30.0f;
    expectedChildSizes[0][2][0] = 30.0f;
    for(UnsignedInt i = 0; i != 3; ++i) {
        margins[0][i] = {3.0f, data.marginsSide[i][0], 3.0f, data.marginsSide[i][1]};
        sizes[0][i] = {30.0f, data.sizesSide[i]};
        expectedChildOffsets[0][i][1] = data.expectedOffsetsSide[i];
        /* If the expected size is 0.0f, it's the same as the input size */
        expectedChildSizes[0][i][1] = data.expectedSizesSide[i] ?
            data.expectedSizesSide[i] : data.sizesSide[i];
    }

    /* Gradually rotate the inputs to generate all four rotations */
    for(UnsignedInt i = 0; i != 3; ++i) {
        expectedSizes[i + 1] = Math::gather<'y', 'x'>(expectedSizes[i]);
        expectedMargins[i + 1] = Math::gather<'w', 'x', 'y', 'z'>(expectedMargins[i]);

        for(UnsignedInt j = 0; j != 3; ++j) {
            margins[i + 1][j] = Math::gather<'w', 'x', 'y', 'z'>(margins[i][j]);
            sizes[i + 1][j] = Math::gather<'y', 'x'>(sizes[i][j]);
            expectedChildOffsets[i + 1][j] = Math::gather<'y', 'x'>(expectedChildOffsets[i][j]);
            expectedChildSizes[i + 1][j] = Math::gather<'y', 'x'>(expectedChildSizes[i][j]);
        }
    }

    /* Offsets in the right-to-left and bottom-to-top rotations are from the
       other edges, so have to subtract it from the size and padding on both
       sides */
    for(UnsignedInt j = 0; j != 3; ++j) {
        expectedChildOffsets[1][j].x() = expectedSizes[1].x() + expectedMargins[1][0] + expectedMargins[1][2] - expectedChildOffsets[1][j].x() - expectedChildSizes[1][j].x();
        expectedChildOffsets[2][j] = expectedSizes[2] + expectedMargins[2].xy() + Math::gather<'z', 'w'>(expectedMargins[2]) - expectedChildOffsets[2][j] - expectedChildSizes[2][j];
        expectedChildOffsets[3][j].y() = expectedSizes[3].y() + expectedMargins[3][1] + expectedMargins[3][3] - expectedChildOffsets[3][j].y() - expectedChildSizes[3][j].y();
    }

    /* Different snap adjustment for layout 2, which should result in just the
       BitVector2 return value differing, nothing else */
    struct SnapAdjustment {
        Snaps snapsEmpty[6]; /* should result in nothing */
        Snaps snaps[6]; /* should result in `expected` */
        Snaps snapsSingleDirection[6]; /* should result in `expected` */
        Snaps snapsOtherDirection[6]; /* should result in nothing */
        BitVector2 expected;
    } snapAdjustments[]{
        {{},
         {{}, {}, Snap::Fill, {}, {}, {}}, /* layout 2 */
         {{}, Snap::FillX, {}, {}, {}, {}}, /* layout 1 */
         {{}, {}, Snap::FillY, {}, {}, {}}, 1},
        {{},
         {{}, {}, Snap::Fill, {}, {}, {}},
         {{}, {}, {}, {}, {}, Snap::FillY}, /* layout 5 */
         {{}, {}, Snap::FillX, {}, {}, {}}, 2},
        {{},
         {{}, {}, Snap::Fill, {}, {}, {}},
         {{}, {}, Snap::FillX, {}, {}, {}}, /* layout 2 */
         {{}, {}, Snap::FillY, {}, {}, {}}, 1},
        {{},
         {{}, {}, {}, {}, {}, Snap::Fill}, /* layout 5 */
         {{}, Snap::FillY, {}, {}, {}, {}}, /* layout 1 */
         {{}, {}, Snap::FillX, {}, {}, {}}, 2},
    };

    /* The output should be the same for all rotations and node orderings */
    UnsignedInt nodeIds[][3]{
        {8, 3, 6},
        {8, 6, 3},
        {3, 6, 8},
        {3, 8, 6},
        {6, 8, 3},
        {6, 3, 8}
    };
    for(UnsignedInt rotation = 0; rotation != 4; ++rotation) {
        const Snaps snap = data.childSnaps[rotation];
        CORRADE_ITERATION("rotation" << rotation << snap);

        for(UnsignedInt shuffle = 0; shuffle != 6; ++shuffle) {
            CORRADE_ITERATION("shuffle" << shuffle);

            struct Node {
                Vector2 size;
                Vector4 margin;
            } nodes[9];

            for(UnsignedInt i = 0; i != 3; ++i) {
                nodes[nodeIds[shuffle][i]].size = sizes[rotation][i];
                nodes[nodeIds[shuffle][i]].margin = margins[rotation][i];
            }

            Containers::Triple<Vector2, Vector4, BitVector2> sizeMargin = Implementation::childLayoutSizeMargin(
                snap,
                Containers::stridedArrayView(nodes).slice(&Node::margin),
                Containers::stridedArrayView(nodes).slice(&Node::size),
                layouterDataHandle(2, 0x222),
                Containers::stridedArrayView(layouts).slice(&Layout::node),
                Containers::stridedArrayView(layouts).slice(&Layout::next),
                snapAdjustments[rotation].snapsEmpty);
            CORRADE_COMPARE(sizeMargin, Containers::triple(expectedSizes[rotation], expectedMargins[rotation], BitVector2{}));

            /* Different layout order, should work the same */
            sizeMargin = Implementation::childLayoutSizeMargin(
                snap,
                Containers::stridedArrayView(nodes).slice(&Node::margin),
                Containers::stridedArrayView(nodes).slice(&Node::size),
                layouterDataHandle(1, 0x111),
                Containers::stridedArrayView(layouts).slice(&Layout::node),
                Containers::stridedArrayView(layouts).slice(&Layout::next),
                snapAdjustments[rotation].snapsEmpty);
            CORRADE_COMPARE(sizeMargin, Containers::triple(expectedSizes[rotation], expectedMargins[rotation], BitVector2{}));

            /* Snap adjustments in one or both directions should propagate to
               the output accordingly ... */
            sizeMargin = Implementation::childLayoutSizeMargin(
                snap,
                Containers::stridedArrayView(nodes).slice(&Node::margin),
                Containers::stridedArrayView(nodes).slice(&Node::size),
                layouterDataHandle(1, 0x111),
                Containers::stridedArrayView(layouts).slice(&Layout::node),
                Containers::stridedArrayView(layouts).slice(&Layout::next),
                snapAdjustments[rotation].snaps);
            CORRADE_COMPARE(sizeMargin, Containers::triple(expectedSizes[rotation], expectedMargins[rotation], snapAdjustments[rotation].expected));

            sizeMargin = Implementation::childLayoutSizeMargin(
                snap,
                Containers::stridedArrayView(nodes).slice(&Node::margin),
                Containers::stridedArrayView(nodes).slice(&Node::size),
                layouterDataHandle(1, 0x111),
                Containers::stridedArrayView(layouts).slice(&Layout::node),
                Containers::stridedArrayView(layouts).slice(&Layout::next),
                snapAdjustments[rotation].snapsSingleDirection);
            CORRADE_COMPARE(sizeMargin, Containers::triple(expectedSizes[rotation], expectedMargins[rotation], snapAdjustments[rotation].expected));

            /* ... but not if in the other direction */
            sizeMargin = Implementation::childLayoutSizeMargin(
                snap,
                Containers::stridedArrayView(nodes).slice(&Node::margin),
                Containers::stridedArrayView(nodes).slice(&Node::size),
                layouterDataHandle(1, 0x111),
                Containers::stridedArrayView(layouts).slice(&Layout::node),
                Containers::stridedArrayView(layouts).slice(&Layout::next),
                snapAdjustments[rotation].snapsOtherDirection);
            CORRADE_COMPARE(sizeMargin, Containers::triple(expectedSizes[rotation], expectedMargins[rotation], BitVector2{}));

            /* Just to get the layout size and padding to pass to snap(), the
               padded child size and margin is unused (and thus also the
               hasFillChildren isn't passed). Tested thoroughly in
               layoutSizePaddingMargin() instead. */
            Implementation::LayoutSizePaddingMargin layoutSizePaddingMargin = Implementation::layoutSizePaddingMargin({}, snap, {}, {}, {}, sizeMargin.first(), sizeMargin.second(), BitVector2{});

            /* With the above size and padding, snapping the three nodes should
               give the expected offsets. The first child uses a different
               snap, and is affected by the reported size and padding */
            Containers::Pair<Vector2, Vector2> offsetSize0 = Implementation::snap(
                Implementation::firstChildSnap(snap),
                {}, layoutSizePaddingMargin.size,
                layoutSizePaddingMargin.padding, {},
                nodes[nodeIds[shuffle][0]].margin,
                nodes[nodeIds[shuffle][0]].size);
            CORRADE_COMPARE(offsetSize0, Containers::pair(expectedChildOffsets[rotation][0], expectedChildSizes[rotation][0]));

            Containers::Pair<Vector2, Vector2> offsetSize1 = Implementation::snap(
                snap,
                offsetSize0.first(), offsetSize0.second(),
                {}, nodes[nodeIds[shuffle][0]].margin,
                nodes[nodeIds[shuffle][1]].margin,
                nodes[nodeIds[shuffle][1]].size);
            CORRADE_COMPARE(offsetSize1, Containers::pair(expectedChildOffsets[rotation][1], expectedChildSizes[rotation][1]));

            Containers::Pair<Vector2, Vector2> offsetSize2 = Implementation::snap(
                snap,
                offsetSize1.first(), offsetSize1.second(),
                {}, nodes[nodeIds[shuffle][1]].margin,
                nodes[nodeIds[shuffle][2]].margin,
                nodes[nodeIds[shuffle][2]].size);
            CORRADE_COMPARE(offsetSize2, Containers::pair(expectedChildOffsets[rotation][2], expectedChildSizes[rotation][2]));
        }
    }
}

void SnapLayouterTest::childLayoutSizeForward() {
    auto&& data = ChildLayoutSizeForwardData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    UnsignedInt nodeIds[]{8, 3, 6};
    /* Every group of four snaps is rotated 90° clockwise compared to the
       previous */
    Snaps snaps[]{
        Snap::Right|data.extraChildSnaps[0],
        Snap::Right|Snap::FillY|data.extraChildSnaps[0],
        Snap::TopRight|Snap::InsideY|data.extraChildSnaps[0],
        Snap::BottomRight|Snap::InsideY|data.extraChildSnaps[0],

        Snap::Bottom|data.extraChildSnaps[1],
        Snap::Bottom|Snap::FillX|data.extraChildSnaps[1],
        Snap::BottomRight|Snap::InsideX|data.extraChildSnaps[1],
        Snap::BottomLeft|Snap::InsideX|data.extraChildSnaps[1],

        Snap::Left|data.extraChildSnaps[0],
        Snap::Left|Snap::FillY|data.extraChildSnaps[0],
        Snap::BottomLeft|Snap::InsideY|data.extraChildSnaps[0],
        Snap::TopLeft|Snap::InsideY|data.extraChildSnaps[0],

        Snap::Top|data.extraChildSnaps[1],
        Snap::Top|Snap::FillX|data.extraChildSnaps[1],
        Snap::TopLeft|Snap::InsideX|data.extraChildSnaps[1],
        Snap::TopRight|Snap::InsideX|data.extraChildSnaps[1],
    };

    /* Accounting for snap adjustment is tested in childLayoutSizeSide()
       already, pass an empty set here always */
    struct Layout {
        NodeHandle node;
        LayouterDataHandle next;
        Snaps snap;
    } layouts[]{
        {},
        {nodeHandle(3, 0xbeb), layouterDataHandle(5, 0x555), {}}, /* layout 1 */
        {nodeHandle(8, 0xaba), layouterDataHandle(1, 0x111), {}}, /* layout 2 */
        {}, {},
        {nodeHandle(6, 0xcec), layouterDataHandle(2, 0x222), {}}, /* layout 5 */
    };

    Vector4 margins[4][3];
    Vector2 sizes[4][3];
    Vector2 expectedSizes[4];
    Vector4 expectedMargins[4];
    Vector2 expectedChildOffsets[4][3];

    /* Data for the first (left-to-right) rotation. In the side direction the
       margin is always 3 from both sides, and all three nodes have the same
       height of 30. */
    expectedSizes[0] = {data.expectedSizeForward, 30.0f};
    expectedMargins[0] = {data.expectedPaddingForward[0], 3.0f, data.expectedPaddingForward[1], 3.0f};
    expectedChildOffsets[0][0][1] = 3.0f;
    expectedChildOffsets[0][1][1] = 3.0f;
    expectedChildOffsets[0][2][1] = 3.0f;
    for(UnsignedInt i = 0; i != 3; ++i) {
        margins[0][i] = {data.marginsForward[i][0], 3.0f, data.marginsForward[i][1], 3.0f};
        sizes[0][i] = {data.sizesForward[i], 30.0f};
        expectedChildOffsets[0][i][0] = data.expectedOffsetsForward[i];
    }

    /* Gradually rotate the inputs to generate all four rotations */
    for(UnsignedInt i = 0; i != 3; ++i) {
        expectedSizes[i + 1] = Math::gather<'y', 'x'>(expectedSizes[i]);
        expectedMargins[i + 1] = Math::gather<'w', 'x', 'y', 'z'>(expectedMargins[i]);

        for(UnsignedInt j = 0; j != 3; ++j) {
            margins[i + 1][j] = Math::gather<'w', 'x', 'y', 'z'>(margins[i][j]);
            sizes[i + 1][j] = Math::gather<'y', 'x'>(sizes[i][j]);
            expectedChildOffsets[i + 1][j] = Math::gather<'y', 'x'>(expectedChildOffsets[i][j]);
        }
    }

    /* Offsets in the right-to-left and bottom-to-top rotations are from the
       other edges, so have to subtract it from the size and padding on both
       sides */
    for(UnsignedInt j = 0; j != 3; ++j) {
        expectedChildOffsets[1][j].x() = expectedSizes[1].x() + expectedMargins[1][0] + expectedMargins[1][2] - expectedChildOffsets[1][j].x() - sizes[1][j].x();
        expectedChildOffsets[2][j] = expectedSizes[2] + expectedMargins[2].xy() + Math::gather<'z', 'w'>(expectedMargins[2]) - expectedChildOffsets[2][j] - sizes[2][j];
        expectedChildOffsets[3][j].y() = expectedSizes[3].y() + expectedMargins[3][1] + expectedMargins[3][3] - expectedChildOffsets[3][j].y() - sizes[3][j].y();
    }

    for(UnsignedInt rotation = 0; rotation != 4; ++rotation) {
        CORRADE_ITERATION("rotation" << rotation);

        for(UnsignedInt snapI = 0; snapI != 4; ++snapI) {
            const Snaps snap = snaps[rotation*4 + snapI];
            CORRADE_ITERATION(snap);

            struct Node {
                Vector2 size;
                Vector4 margin;
            } nodes[9];

            for(UnsignedInt i = 0; i != 3; ++i) {
                nodes[nodeIds[i]].size = sizes[rotation][i];
                nodes[nodeIds[i]].margin = margins[rotation][i];
            }

            Containers::Triple<Vector2, Vector4, BitVector2> sizeMargin = Implementation::childLayoutSizeMargin(
                snap,
                Containers::stridedArrayView(nodes).slice(&Node::margin),
                Containers::stridedArrayView(nodes).slice(&Node::size),
                layouterDataHandle(2, 0x222),
                Containers::stridedArrayView(layouts).slice(&Layout::node),
                Containers::stridedArrayView(layouts).slice(&Layout::next),
                Containers::stridedArrayView(layouts).slice(&Layout::snap));
            CORRADE_COMPARE(sizeMargin, Containers::triple(expectedSizes[rotation], expectedMargins[rotation], BitVector2{}));

            /* Just to get the layout size and padding to pass to snap(), the
               padded child size and margin is unused (and thus also the
               hasFillChildren isn't passed). Tested thoroughly in
               layoutSizePaddingMargin() instead. */
            Implementation::LayoutSizePaddingMargin layoutSizePaddingMargin = Implementation::layoutSizePaddingMargin({}, snap, {}, {}, {}, sizeMargin.first(), sizeMargin.second(), BitVector2{});

            /* With the above size and padding, snapping the three nodes should
               give the expected offsets. The first child uses a different
               snap, and is affected by the reported size and padding */
            Containers::Pair<Vector2, Vector2> offsetSize0 = Implementation::snap(
                Implementation::firstChildSnap(snap),
                {}, layoutSizePaddingMargin.size,
                layoutSizePaddingMargin.padding, {},
                nodes[nodeIds[0]].margin,
                nodes[nodeIds[0]].size);
            CORRADE_COMPARE(offsetSize0, Containers::pair(expectedChildOffsets[rotation][0], sizes[rotation][0]));

            Containers::Pair<Vector2, Vector2> offsetSize1 = Implementation::snap(
                snap,
                offsetSize0.first(), offsetSize0.second(),
                {}, nodes[nodeIds[0]].margin,
                nodes[nodeIds[1]].margin,
                nodes[nodeIds[1]].size);
            CORRADE_COMPARE(offsetSize1, Containers::pair(expectedChildOffsets[rotation][1], sizes[rotation][1]));

            Containers::Pair<Vector2, Vector2> offsetSize2 = Implementation::snap(
                snap,
                offsetSize1.first(), offsetSize1.second(),
                {}, nodes[nodeIds[1]].margin,
                nodes[nodeIds[2]].margin,
                nodes[nodeIds[2]].size);
            CORRADE_COMPARE(offsetSize2, Containers::pair(expectedChildOffsets[rotation][2], sizes[rotation][2]));
        }
    }
}

void SnapLayouterTest::explicitlySnappedChildLayoutSize() {
    auto&& data = ExplicitlySnappedChildLayoutSizeData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct Layout {
        NodeHandle node;
        LayouterDataHandle next;
        SnapLayoutFlags flags;
        Snaps explicitSnap;
    } layouts[]{
        {},
        {nodeHandle(3, 0xbeb),                  /* layout 1 */
         layouterDataHandle(4, 0x444),
         Implementation::SnapLayoutFlagExplicitSnapToParent,
         data.snaps[1]}, /* use the same data index as node 3 below */
        {nodeHandle(8, 0xaba),                  /* layout 2 */
         layouterDataHandle(1, 0x111),
         Implementation::SnapLayoutFlagExplicitSnapToParent,
         data.snaps[0]}, /* use the same data index as node 8 below */
        {},
        {nodeHandle(2, 0x111),                  /* layout 4, skipped */
         layouterDataHandle(5, 0x555),
         {}, /* not snapped to a parent */
         {}},
        {nodeHandle(6, 0xcec),                  /* layout 5 */
         layouterDataHandle(2, 0x222),
         Implementation::SnapLayoutFlagExplicitSnapToParent,
         data.snaps[2]}, /* use the same data index as node 6 below */
    };

    /* Unlike with childLayoutSizeMargin(), the algorithm is not
       order-dependent so we're not shuffling the node data in any way. */
    struct Node {
        Vector2 size;
        Vector4 margin;
    } nodes[9];
    nodes[8].size = data.nodeSizes[0];
    nodes[8].margin = data.nodeMargins[0];
    nodes[3].size = data.nodeSizes[1];
    nodes[3].margin = data.nodeMargins[1];
    nodes[6].size = data.nodeSizes[2];
    nodes[6].margin = data.nodeMargins[2];
    /* Node 2 is used by layout 4, which is skipped because it snaps to a
       sibling, so the size & margin shouldn't affect anything */
    nodes[2].size = Vector2{99999.0f};
    nodes[2].margin = Vector4{99999.0f};

    /* Node size passed directly */
    CORRADE_COMPARE(Implementation::explicitlySnappedChildLayoutSize(
        data.parentFlags,
        data.parentPadding,
        Containers::stridedArrayView(nodes).slice(&Node::margin),
        Containers::stridedArrayView(nodes).slice(&Node::size),
        layouterDataHandle(1, 0x111),
        Containers::stridedArrayView(layouts).slice(&Layout::node),
        Containers::stridedArrayView(layouts).slice(&Layout::flags),
        Containers::stridedArrayView(layouts).slice(&Layout::explicitSnap),
        Containers::stridedArrayView(layouts).slice(&Layout::next)
    ), data.expected);

    /* Different layout order, should work the same */
    CORRADE_COMPARE(Implementation::explicitlySnappedChildLayoutSize(
        data.parentFlags,
        data.parentPadding,
        Containers::stridedArrayView(nodes).slice(&Node::margin),
        Containers::stridedArrayView(nodes).slice(&Node::size),
        layouterDataHandle(4, 0x444),
        Containers::stridedArrayView(layouts).slice(&Layout::node),
        Containers::stridedArrayView(layouts).slice(&Layout::flags),
        Containers::stridedArrayView(layouts).slice(&Layout::explicitSnap),
        Containers::stridedArrayView(layouts).slice(&Layout::next)
    ), data.expected);
}

void SnapLayouterTest::expandChildLayouts() {
    auto&& data = ExpandChildLayoutsData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* The layout order is 5, 1, 7, 3, 9, 4, which becomes node order
       4, 8, 2, 5, 7, 1. Layouts 4 and 5 don't have FillX set, which means
       they're not expanding and thus node 1 and 4 size isn't used. */
    const struct Layout {
        NodeHandle node;
        LayouterDataHandle next;
        Snaps snapX, snapY;
    } layouts[]{
        {},
        {nodeHandle(8, 0xcec), layouterDataHandle(7, 0x777), /* 1 */
            Snap::FillX & data.snapMask, Snap::FillY & data.snapMask},
        {},
        {nodeHandle(5, 0xbab), layouterDataHandle(9, 0x999), /* 3 */
            Snap::FillX & data.snapMask, Snap::FillY & data.snapMask},
        {nodeHandle(1, 0xded), layouterDataHandle(5, 0x555), /* 4 */
            {}, {}},
        {nodeHandle(4, 0xf0f), layouterDataHandle(1, 0x111), /* 5 */
            Snap::FillY & data.snapMask, Snap::FillX & data.snapMask},
        {},
        {nodeHandle(2, 0xb0b), layouterDataHandle(3, 0x333), /* 7 */
            Snap::Fill & data.snapMask, Snap::Fill & data.snapMask},
        {},
        {nodeHandle(7, 0xbeb), layouterDataHandle(4, 0x444), /* 9 */
            Snap::FillX & data.snapMask, Snap::FillY & data.snapMask},
    };
    UnsignedInt nodeIds[4];

    /* Verify all possible horizontal variants */
    for(Snaps childSnap: {
        ~~Snap::Right,
        Snap::Right|Snap::InsideY,
        Snap::Right|Snap::FillY,
        Snap::Right|Snap::FillY|Snap::InsideY,
        Snap::TopRight|Snap::InsideY,
        Snap::BottomRight|Snap::InsideY,

        ~~Snap::Left,
        Snap::Left|Snap::InsideY,
        Snap::Left|Snap::FillY,
        Snap::Left|Snap::FillY|Snap::InsideY,
        Snap::BottomLeft|Snap::InsideY,
        Snap::TopLeft|Snap::InsideY,

        /* NoPad should be excluded altogether, verify just with one variant */
        Snap::Right|Snap::NoPad,
    }) {
        CORRADE_ITERATION(childSnap);

        Vector2 nodeSizes[]{
            {},
            {-9999.9f, 0.0f},       /* 1, no FillX, should stay untouched */
            {data.sizes[1], 0.0f},  /* 2 */
            {},
            {-9999.9f, 0.0f},       /* 4, no FillX, should stay untouched */
            {data.sizes[2], 0.0f},  /* 5 */
            {},
            {data.sizes[3], 0.0f},  /* 7 */
            {data.sizes[0], 0.0f},  /* 8 */
        };

        Implementation::expandChildLayouts(
            childSnap,
            /* The algorithm extracts the extraAvailableSize out of this again
               so the original number doesn't matter -- in practice it'll
               contain also various margins between the nodes, parent node
               padding and size of all non-expandable nodes */
            {987.0f + data.extraAvailableSize, 0.0f},
            {987.0f, 0.0f},
            nodeSizes,
            layouterDataHandle(5, 0x555),
            Containers::stridedArrayView(layouts).slice(&Layout::node),
            Containers::stridedArrayView(layouts).slice(&Layout::snapX),
            Containers::stridedArrayView(layouts).slice(&Layout::next),
            nodeIds);
        CORRADE_COMPARE_AS(Containers::arrayView(nodeSizes), Containers::arrayView<Vector2>({
            {},
            {-9999.9f, 0.0f},
            {data.expectedSizes[1], 0.0f},
            {},
            {-9999.9f, 0.0f},
            {data.expectedSizes[2], 0.0f},
            {},
            {data.expectedSizes[3], 0.0f},
            {data.expectedSizes[0], 0.0f},
        }), TestSuite::Compare::Container);
    }

    /* Verify all possible vertical variants. Same as above, just with X and Y
       swapped. */
    for(Snaps childSnap: {
        ~~Snap::Bottom,
        Snap::Bottom|Snap::InsideX,
        Snap::Bottom|Snap::FillX,
        Snap::Bottom|Snap::FillX|Snap::InsideX,
        Snap::BottomRight|Snap::InsideX,
        Snap::BottomLeft|Snap::InsideX,

        ~~Snap::Top,
        Snap::Top|Snap::InsideX,
        Snap::Top|Snap::FillX,
        Snap::Top|Snap::FillX|Snap::InsideX,
        Snap::TopLeft|Snap::InsideX,
        Snap::TopRight|Snap::InsideX,

        /* NoPad should be excluded altogether, verify just with one variant */
        Snap::Bottom|Snap::NoPad,
    }) {
        CORRADE_ITERATION(childSnap);

        Vector2 nodeSizes[]{
            {},
            {0.0f, -9999.9f},       /* 1, no FillY, should stay untouched */
            {0.0f, data.sizes[1]},  /* 2 */
            {},
            {0.0f, -9999.9f},       /* 4, no FillY, should stay untouched */
            {0.0f, data.sizes[2]},  /* 5 */
            {},
            {0.0f, data.sizes[3]},  /* 7 */
            {0.0f, data.sizes[0]},  /* 8 */
        };

        Implementation::expandChildLayouts(
            childSnap,
            /* Using a different number compared to above but it shouldn't
               matter, as again the algorithm extracts only the
               extraAvailableSize out of this again */
            {0.0f, 6789.0f + data.extraAvailableSize},
            {0.0f, 6789.0f},
            nodeSizes,
            layouterDataHandle(5, 0x555),
            Containers::stridedArrayView(layouts).slice(&Layout::node),
            Containers::stridedArrayView(layouts).slice(&Layout::snapY),
            Containers::stridedArrayView(layouts).slice(&Layout::next),
            nodeIds);
        CORRADE_COMPARE_AS(Containers::arrayView(nodeSizes), Containers::arrayView<Vector2>({
            {},
            {0.0f, -9999.9f},
            {0.0f, data.expectedSizes[1]},
            {},
            {0.0f, -9999.9f},
            {0.0f, data.expectedSizes[2]},
            {},
            {0.0f, data.expectedSizes[3]},
            {0.0f, data.expectedSizes[0]},
        }), TestSuite::Compare::Container);
    }
}

void SnapLayouterTest::orderLayoutsBreadthFirstEmpty() {
    /* Just to verify it doesn't blow up with some OOB access in this case */
    UnsignedInt childrenOffsets[2]{};
    Int layoutIds[1];
    Implementation::orderLayoutsBreadthFirstInto(
        {},
        {},
        {},
        {},
        childrenOffsets, {}, layoutIds);
    CORRADE_COMPARE_AS(Containers::arrayView(layoutIds), Containers::arrayView({
        -1
    }), TestSuite::Compare::Container);
}

void SnapLayouterTest::orderLayoutsBreadthFirst() {
    /* Expands AbstractUserInterfaceImplementationTest::orderNodesBreadthFirst()
       (which tests the base of the algorithm) with explicit ordering between
       children */

    /* The handle generations aren't used for anything here so can be
       arbitrary. But multiple uses should have matching generations so the
       cyclic linked lists work as expected. */
    const struct Layout {
        LayouterDataHandle parentOrTarget;
        LayouterDataHandle firstChild;
        LayouterDataHandle firstExplicitSnap;
        LayouterDataHandle next;
    } layouts[]{
        /* Forward parent and next reference, no dependent layouts */
        {layouterDataHandle(11, 0x323),     /* 0 */
         LayouterDataHandle::Null,
         LayouterDataHandle::Null,
         layouterDataHandle(3, 0xcec)},
        /* Root layout, has both children and explicitly snapped layouts */
        {LayouterDataHandle::Null,          /* 1 */
         layouterDataHandle(10, 0x111),
         layouterDataHandle(4, 0xaba),
         LayouterDataHandle::Null},
        /* Unused / freed layout. Has to have all four handles null to not
           contribute to anything. */
        {},                                 /* 2 */
        /* Sibling of layout 0 or possibly with the same target. Has just
           children, no explicitly snapped layouts. There's one more in the
           same list. */
        {layouterDataHandle(11, 0x323),     /* 3 */
         layouterDataHandle(7, 0x321),
         LayouterDataHandle::Null,
         layouterDataHandle(8, 0x666)},
        /* Explicitly snapping to layout 1, no children or dependent layouts */
        {layouterDataHandle(1, 0xabc),      /* 4 */
         LayouterDataHandle::Null,
         LayouterDataHandle::Null,
         layouterDataHandle(16, 0xede)},
        /* Child of layout 1, having one more sibling and an explicitly
           snapped node */
        {layouterDataHandle(1, 0xabc),      /* 5 */
         LayouterDataHandle::Null,
         layouterDataHandle(11, 0x323),
         layouterDataHandle(10, 0x111)},
        /* Unused / freed layout again */
        {},                                 /* 6 */
        /* The only child of layout 3 so next points to itself, no more nested
           layouts */
        {layouterDataHandle(3, 0xcec),      /* 7 */
         LayouterDataHandle::Null,
         LayouterDataHandle::Null,
         layouterDataHandle(7, 0x321)},
        /* Remaining sibling of layout 0 or possibly with the same target, the
           next one is layout 0 which cycles back */
        {layouterDataHandle(11, 0x323),     /* 8 */
         LayouterDataHandle::Null,
         LayouterDataHandle::Null,
         layouterDataHandle(0, 0x888)},
        /* Root layout with nothing else, treated the same as an unused / freed
           layout */
        {LayouterDataHandle::Null,          /* 9 */
         LayouterDataHandle::Null,
         LayouterDataHandle::Null,
         LayouterDataHandle::Null},
        /* Second, last child of layout 1 */
        {layouterDataHandle(1, 0xabc),      /* 10 */
         LayouterDataHandle::Null,
         LayouterDataHandle::Null,
         layouterDataHandle(5, 0x444)},
        /* Explicitly snapped to layout 5, having children but no other layouts
           snapped to the same target so next points to itself */
        {layouterDataHandle(5, 0x444),      /* 11 */
         layouterDataHandle(3, 0xcec),
         LayouterDataHandle::Null,
         layouterDataHandle(11, 0x323)},
        /* Another unused layout */
        {},                                 /* 12 */
        /* Third layout snapped to layout 1 */
        {layouterDataHandle(1, 0xabc),      /* 13 */
         LayouterDataHandle::Null,
         LayouterDataHandle::Null,
         layouterDataHandle(4, 0xaba)},
        /* Root with a single snapped layout that's right before */
        {layouterDataHandle(15, 0x566),     /* 14 */
         LayouterDataHandle::Null,
         LayouterDataHandle::Null,
         layouterDataHandle(14, 0x565)},
        {LayouterDataHandle::Null,          /* 15 */
         LayouterDataHandle::Null,
         layouterDataHandle(14, 0x565),
         LayouterDataHandle::Null},
        /* Second layout snapped to layout 1 */
        {layouterDataHandle(1, 0xabc),      /* 16 */
         LayouterDataHandle::Null,
         LayouterDataHandle::Null,
         layouterDataHandle(13, 0x444)},
    };

    /* Important: the childrenOffsets array has to be zero-initialized. Others
       don't need to be. */
    UnsignedInt childrenOffsets[Containers::arraySize(layouts) + 2]{};
    UnsignedInt children[Containers::arraySize(layouts)];
    Int layoutIds[Containers::arraySize(layouts) + 1];
    Implementation::orderLayoutsBreadthFirstInto(
        Containers::stridedArrayView(layouts).slice(&Layout::parentOrTarget),
        Containers::stridedArrayView(layouts).slice(&Layout::firstChild),
        Containers::stridedArrayView(layouts).slice(&Layout::firstExplicitSnap),
        Containers::stridedArrayView(layouts).slice(&Layout::next),
        childrenOffsets, children, layoutIds);
    CORRADE_COMPARE_AS(Containers::arrayView(layoutIds), Containers::arrayView({
        /* -1 is always first */
        -1,
        /* Root / unused layouts first, in order as found */
        1,
        2,
        6,
        9,
        12,
        15,
        /* Then children of layout 1, clustered together, in order following
           the linked list */
        10,
        5,
        /* Then layouts snapped to layout 1, again following the linked list */
        4,
        16,
        13,
        /* Layout 2, 6 and 12 have no children or explicit snaps */
        /* Layout 15 has a single snapped layout */
        14,
        /* Layout 10 has no children or explicit snaps */
        /* Layout 5 has no children, but a single explicit snap */
        11,
        /* Layout 4, 16 and 13 have no children or explicit snaps */
        /* Layout 11 has only children, no explicit snaps */
        3,
        8,
        0,
        /* Layout 3 has a single child */
        7,
        /* Layout 8 and 0 have no children or explicit snaps */
        /* Layout 7 has no children or explicit snaps */
    }), TestSuite::Compare::Container);
}

void SnapLayouterTest::construct() {
    SnapLayouter layouter{layouterHandle(0xab, 0x12)};
    CORRADE_COMPARE(layouter.features(), LayouterFeature::UniqueLayouts);
    CORRADE_COMPARE(layouter.handle(), layouterHandle(0xab, 0x12));
}

void SnapLayouterTest::constructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<SnapLayouter>{});
    CORRADE_VERIFY(!std::is_copy_assignable<SnapLayouter>{});
}

void SnapLayouterTest::constructMove() {
    SnapLayouter a{layouterHandle(0xab, 0x12)};

    SnapLayouter b{Utility::move(a)};
    CORRADE_COMPARE(b.handle(), layouterHandle(0xab, 0x12));

    SnapLayouter c{layouterHandle(3, 5)};
    c = Utility::move(b);
    CORRADE_COMPARE(c.handle(), layouterHandle(0xab, 0x12));

    CORRADE_VERIFY(std::is_nothrow_move_constructible<SnapLayouter>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<SnapLayouter>::value);
}

void SnapLayouterTest::addRemove() {
    auto&& data = AddRemoveData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui{{100, 100}};
    SnapLayouter& layouter = ui.setLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    NodeHandle root1 = ui.createNode({}, {});
    NodeHandle root1Child1 = ui.createNode(root1, {}, {});
    NodeHandle root1Child2 = ui.createNode(root1, {}, {});
    NodeHandle root1Child3 = ui.createNode(root1, {}, {});
    NodeHandle root1Child4 = ui.createNode(root1, {}, {});
    NodeHandle root1Child5 = ui.createNode(root1, {}, {});
    NodeHandle root1Child6 = ui.createNode(root1, {}, {});
    NodeHandle root1ExplicitSnap = ui.createNode(root1, {}, {});

    NodeHandle root2 = ui.createNode({}, {});

    /* Layout that does nothing on its own but can manage children in a
       specific order and snap */
    LayoutHandle root1Layout = layouter.add(root1);
    CORRADE_COMPARE(layouter.node(root1Layout), root1);
    CORRADE_VERIFY(!layouter.hasExplicitSnap(root1Layout));
    CORRADE_COMPARE(layouter.flags(root1Layout), SnapLayoutFlags{});
    CORRADE_COMPARE(layouter.snap(root1Layout), Snaps{});
    CORRADE_COMPARE(layouter.parent(root1Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.firstChild(root1Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.firstExplicitSnap(root1Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.previous(root1Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.next(root1Layout), LayoutHandle::Null);

    /* Layout that again does nothing on its own, with an explicit
       LayouterDataHandle::Null argument, and LayouterDataHandle overloads */
    LayoutHandle root2Layout = layouter.add(root2, LayouterDataHandle::Null, SnapLayoutFlag::IgnoreOverflowX);
    CORRADE_COMPARE(layouter.node(root2Layout), root2);
    CORRADE_VERIFY(!layouter.hasExplicitSnap(layoutHandleData(root2Layout)));
    CORRADE_COMPARE(layouter.flags(root2Layout), SnapLayoutFlag::IgnoreOverflowX);
    CORRADE_COMPARE(layouter.snap(root2Layout), Snaps{});
    CORRADE_COMPARE(layouter.parent(layoutHandleData(root2Layout)), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.firstChild(layoutHandleData(root2Layout)), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.firstExplicitSnap(layoutHandleData(root2Layout)), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.previous(layoutHandleData(root2Layout)), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.next(layoutHandleData(root2Layout)), LayoutHandle::Null);

    /* Layout that's the first implicit child inside, with flags */
    LayoutHandle root1Child1Layout = layouter.add(root1Child1, SnapLayoutFlag::IgnoreOverflowY);
    CORRADE_COMPARE(layouter.firstChild(root1Layout), root1Child1Layout);
    CORRADE_COMPARE(layouter.firstExplicitSnap(root1Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.node(root1Child1Layout), root1Child1);
    CORRADE_VERIFY(!layouter.hasExplicitSnap(root1Child1Layout));
    CORRADE_COMPARE(layouter.flags(root1Child1Layout), SnapLayoutFlag::IgnoreOverflowY);
    CORRADE_COMPARE(layouter.snap(root1Child1Layout), Snaps{});
    CORRADE_COMPARE(layouter.parent(root1Child1Layout), root1Layout);
    CORRADE_COMPARE(layouter.firstChild(root1Child1Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.firstExplicitSnap(root1Child1Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.previous(root1Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.next(root1Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.previous(root1Child1Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.next(root1Child1Layout), LayoutHandle::Null);

    /* Layout that's the second */
    LayoutHandle root1Child2Layout = layouter.add(root1Child2);
    CORRADE_COMPARE(layouter.firstChild(root1Layout), root1Child1Layout);
    CORRADE_COMPARE(layouter.firstExplicitSnap(root1Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.node(root1Child2Layout), root1Child2);
    CORRADE_VERIFY(!layouter.hasExplicitSnap(root1Child2Layout));
    CORRADE_COMPARE(layouter.flags(root1Child2Layout), SnapLayoutFlags{});
    CORRADE_COMPARE(layouter.snap(root1Child2Layout), Snaps{});
    CORRADE_COMPARE(layouter.parent(root1Child2Layout), root1Layout);
    CORRADE_COMPARE(layouter.firstChild(root1Child2Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.firstExplicitSnap(root1Child2Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.previous(root1Child1Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.next(root1Child1Layout), root1Child2Layout);
    CORRADE_COMPARE(layouter.previous(root1Child2Layout), root1Child1Layout);
    CORRADE_COMPARE(layouter.next(root1Child2Layout), LayoutHandle::Null);

    /* Layout inserted in the middle, with flags */
    LayoutHandle root1Child3Layout = layouter.add(root1Child3, root1Child2Layout, SnapLayoutFlags{0x20});
    CORRADE_COMPARE(layouter.firstChild(root1Layout), root1Child1Layout);
    CORRADE_COMPARE(layouter.firstExplicitSnap(root1Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.node(root1Child3Layout), root1Child3);
    CORRADE_VERIFY(!layouter.hasExplicitSnap(root1Child3Layout));
    CORRADE_COMPARE(layouter.flags(root1Child3Layout), SnapLayoutFlags{0x20});
    CORRADE_COMPARE(layouter.snap(root1Child3Layout), Snaps{});
    CORRADE_COMPARE(layouter.parent(root1Child3Layout), root1Layout);
    CORRADE_COMPARE(layouter.firstChild(root1Child3Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.firstExplicitSnap(root1Child3Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.previous(root1Child1Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.next(root1Child1Layout), root1Child3Layout);
    CORRADE_COMPARE(layouter.previous(root1Child3Layout), root1Child1Layout);
    CORRADE_COMPARE(layouter.next(root1Child3Layout), root1Child2Layout);
    CORRADE_COMPARE(layouter.previous(root1Child2Layout), root1Child3Layout);
    CORRADE_COMPARE(layouter.next(root1Child2Layout), LayoutHandle::Null);

    /* Layout that's explicitly snapped isn't added to the child list even
       though it's assigned to a node that's a sibling of the others. It's
       added to the snap list of the target however. */
    LayoutHandle root1ExplicitSnapLayout = layouter.addExplicit(root1ExplicitSnap, Snap::Left|Snap::NoPadY, root1Child2Layout);
    CORRADE_COMPARE(layouter.firstChild(root1Layout), root1Child1Layout);
    CORRADE_COMPARE(layouter.firstExplicitSnap(root1Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.firstExplicitSnap(root1Child2Layout), root1ExplicitSnapLayout);
    CORRADE_COMPARE(layouter.node(root1ExplicitSnapLayout), root1ExplicitSnap);
    CORRADE_VERIFY(layouter.hasExplicitSnap(root1ExplicitSnapLayout));
    CORRADE_COMPARE(layouter.flags(root1ExplicitSnapLayout), SnapLayoutFlags{});
    CORRADE_COMPARE(layouter.snap(root1ExplicitSnapLayout), Snap::Left|Snap::NoPadY);
    CORRADE_COMPARE(layouter.explicitSnapTarget(root1ExplicitSnapLayout), root1Child2Layout);
    CORRADE_COMPARE(layouter.firstChild(root1ExplicitSnapLayout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.firstChild(root1Layout), root1Child1Layout);
    CORRADE_COMPARE(layouter.previous(root1Child1Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.next(root1Child2Layout), LayoutHandle::Null);

    /* Layout inserted at the front, with LayouterDataHandle overload, and
       getter overloads as well */
    LayoutHandle root1Child4Layout = layouter.add(root1Child4, layoutHandleData(root1Child1Layout), SnapLayoutFlag::IgnoreOverflow);
    CORRADE_COMPARE(layouter.firstChild(layoutHandleData(root1Layout)), root1Child4Layout);
    CORRADE_COMPARE(layouter.firstExplicitSnap(layoutHandleData(root1Layout)), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.firstExplicitSnap(layoutHandleData(root1Child2Layout)), root1ExplicitSnapLayout);
    CORRADE_COMPARE(layouter.node(root1Child4Layout), root1Child4);
    CORRADE_VERIFY(!layouter.hasExplicitSnap(layoutHandleData(root1Child4Layout)));
    CORRADE_COMPARE(layouter.flags(layoutHandleData(root1Child4Layout)), SnapLayoutFlag::IgnoreOverflow);
    CORRADE_COMPARE(layouter.snap(layoutHandleData(root1Child4Layout)), Snaps{});
    CORRADE_COMPARE(layouter.parent(layoutHandleData(root1Child4Layout)), root1Layout);
    CORRADE_COMPARE(layouter.firstChild(layoutHandleData(root1Child4Layout)), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.firstExplicitSnap(layoutHandleData(root1Child4Layout)), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.previous(layoutHandleData(root1Child4Layout)), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.next(layoutHandleData(root1Child4Layout)), root1Child1Layout);
    CORRADE_COMPARE(layouter.previous(layoutHandleData(root1Child1Layout)), root1Child4Layout);
    CORRADE_COMPARE(layouter.next(layoutHandleData(root1Child1Layout)), root1Child3Layout);
    CORRADE_COMPARE(layouter.previous(layoutHandleData(root1Child3Layout)), root1Child1Layout);
    CORRADE_COMPARE(layouter.next(layoutHandleData(root1Child3Layout)), root1Child2Layout);
    CORRADE_COMPARE(layouter.previous(layoutHandleData(root1Child2Layout)), root1Child3Layout);
    CORRADE_COMPARE(layouter.next(layoutHandleData(root1Child2Layout)), LayoutHandle::Null);

    /* Fill layout, inserted at the front */
    LayoutHandle root1Child5Layout = layouter.add(root1Child5, Snap::FillY, root1Child4Layout, SnapLayoutFlags{0x20});
    CORRADE_COMPARE(layouter.firstChild(root1Layout), root1Child5Layout);
    CORRADE_COMPARE(layouter.firstExplicitSnap(root1Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.node(root1Child5Layout), root1Child5);
    CORRADE_VERIFY(!layouter.hasExplicitSnap(root1Child5Layout));
    CORRADE_COMPARE(layouter.flags(root1Child5Layout), SnapLayoutFlags{0x20});
    CORRADE_COMPARE(layouter.snap(root1Child5Layout), Snap::FillY);
    CORRADE_COMPARE(layouter.parent(root1Child5Layout), root1Layout);
    CORRADE_COMPARE(layouter.firstChild(root1Child5Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.firstExplicitSnap(root1Child5Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.previous(root1Child5Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.next(root1Child5Layout), root1Child4Layout);
    CORRADE_COMPARE(layouter.previous(root1Child4Layout), root1Child5Layout);
    CORRADE_COMPARE(layouter.next(root1Child4Layout), root1Child1Layout);
    CORRADE_COMPARE(layouter.previous(root1Child1Layout), root1Child4Layout);
    CORRADE_COMPARE(layouter.next(root1Child1Layout), root1Child3Layout);
    CORRADE_COMPARE(layouter.previous(root1Child3Layout), root1Child1Layout);
    CORRADE_COMPARE(layouter.next(root1Child3Layout), root1Child2Layout);
    CORRADE_COMPARE(layouter.previous(root1Child2Layout), root1Child3Layout);
    CORRADE_COMPARE(layouter.next(root1Child2Layout), LayoutHandle::Null);

    /* Fill layout, inserted at the front, LayouterDataHandle overload. Getter
       LayouterDataHandle overloads tested above already, no need to repeat
       that. */
    LayoutHandle root1Child6Layout = layouter.add(root1Child6, Snap::Fill, layoutHandleData(root1Child5Layout), SnapLayoutFlags{0x10});
    CORRADE_COMPARE(layouter.firstChild(root1Layout), root1Child6Layout);
    CORRADE_COMPARE(layouter.firstExplicitSnap(root1Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.node(root1Child6Layout), root1Child6);
    CORRADE_VERIFY(!layouter.hasExplicitSnap(root1Child6Layout));
    CORRADE_COMPARE(layouter.flags(root1Child6Layout), SnapLayoutFlags{0x10});
    CORRADE_COMPARE(layouter.snap(root1Child6Layout), Snap::Fill);
    CORRADE_COMPARE(layouter.parent(root1Child6Layout), root1Layout);
    CORRADE_COMPARE(layouter.firstChild(root1Child6Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.firstExplicitSnap(root1Child6Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.previous(root1Child6Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.next(root1Child6Layout), root1Child5Layout);
    CORRADE_COMPARE(layouter.previous(root1Child5Layout), root1Child6Layout);
    CORRADE_COMPARE(layouter.next(root1Child5Layout), root1Child4Layout);
    CORRADE_COMPARE(layouter.previous(root1Child4Layout), root1Child5Layout);
    CORRADE_COMPARE(layouter.next(root1Child4Layout), root1Child1Layout);
    CORRADE_COMPARE(layouter.previous(root1Child1Layout), root1Child4Layout);
    CORRADE_COMPARE(layouter.next(root1Child1Layout), root1Child3Layout);
    CORRADE_COMPARE(layouter.previous(root1Child3Layout), root1Child1Layout);
    CORRADE_COMPARE(layouter.next(root1Child3Layout), root1Child2Layout);
    CORRADE_COMPARE(layouter.previous(root1Child2Layout), root1Child3Layout);
    CORRADE_COMPARE(layouter.next(root1Child2Layout), LayoutHandle::Null);

    /* Passed to cleanNodes(). As no nodes were removed until now, the
       generations are all initially at 1. */
    UnsignedShort nodeHandleGenerations[]{
        1, 1, 1,
        1, 1, 1,
        1, 1, 1
    };

    /* Removing a layout from the middle of the sibling list */
    if(data.clean) {
        nodeHandleGenerations[nodeHandleId(root1Child1)] += 1;
        layouter.cleanNodes(nodeHandleGenerations);
    } else layouter.remove(root1Child1Layout);
    CORRADE_VERIFY(!layouter.isHandleValid(root1Child1Layout));
    CORRADE_COMPARE(layouter.firstChild(root1Layout), root1Child6Layout);
    CORRADE_COMPARE(layouter.firstExplicitSnap(root1Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.previous(root1Child6Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.next(root1Child6Layout), root1Child5Layout);
    CORRADE_COMPARE(layouter.previous(root1Child5Layout), root1Child6Layout);
    CORRADE_COMPARE(layouter.next(root1Child5Layout), root1Child4Layout);
    CORRADE_COMPARE(layouter.previous(root1Child4Layout), root1Child5Layout);
    CORRADE_COMPARE(layouter.next(root1Child4Layout), root1Child3Layout);
    CORRADE_COMPARE(layouter.previous(root1Child3Layout), root1Child4Layout);
    CORRADE_COMPARE(layouter.next(root1Child3Layout), root1Child2Layout);
    CORRADE_COMPARE(layouter.previous(root1Child2Layout), root1Child3Layout);
    CORRADE_COMPARE(layouter.next(root1Child2Layout), LayoutHandle::Null);

    /* Removing a layout that has no parent nor any children or explicitly
       snapped nodes doesn't cause any list to be updated */
    if(data.clean) {
        nodeHandleGenerations[nodeHandleId(root2)] += 1;
        layouter.cleanNodes(nodeHandleGenerations);
    } else layouter.remove(root2Layout);
    CORRADE_VERIFY(!layouter.isHandleValid(root2Layout));

    /* Removing a layout with an explicit snap doesn't affect the sibling list,
       even though the node it's assigned to is a sibling. It's removed from
       the snap list of the target though. */
    if(data.clean) {
        nodeHandleGenerations[nodeHandleId(root1ExplicitSnap)] += 1;
        layouter.cleanNodes(nodeHandleGenerations);
    } else layouter.remove(root1ExplicitSnapLayout);
    CORRADE_VERIFY(!layouter.isHandleValid(root1ExplicitSnapLayout));
    CORRADE_COMPARE(layouter.firstChild(root1Layout), root1Child6Layout);
    CORRADE_COMPARE(layouter.firstExplicitSnap(root1Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.firstExplicitSnap(root1Child2Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.previous(root1Child6Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.next(root1Child6Layout), root1Child5Layout);
    CORRADE_COMPARE(layouter.previous(root1Child5Layout), root1Child6Layout);
    CORRADE_COMPARE(layouter.next(root1Child5Layout), root1Child4Layout);
    CORRADE_COMPARE(layouter.previous(root1Child4Layout), root1Child5Layout);
    CORRADE_COMPARE(layouter.next(root1Child4Layout), root1Child3Layout);
    CORRADE_COMPARE(layouter.previous(root1Child3Layout), root1Child4Layout);
    CORRADE_COMPARE(layouter.next(root1Child3Layout), root1Child2Layout);
    CORRADE_COMPARE(layouter.previous(root1Child2Layout), root1Child3Layout);
    CORRADE_COMPARE(layouter.next(root1Child2Layout), LayoutHandle::Null);

    /* Removing a layout from the back of the sibling list, LayouterDataHandle
       overload */
    if(data.clean) {
        nodeHandleGenerations[nodeHandleId(root1Child2)] += 1;
        layouter.cleanNodes(nodeHandleGenerations);
    } else layouter.remove(layoutHandleData(root1Child2Layout));
    CORRADE_VERIFY(!layouter.isHandleValid(root1Child2Layout));
    CORRADE_COMPARE(layouter.firstChild(layoutHandleData(root1Layout)), root1Child6Layout);
    CORRADE_COMPARE(layouter.firstExplicitSnap(layoutHandleData(root1Layout)), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.previous(layoutHandleData(root1Child6Layout)), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.next(layoutHandleData(root1Child6Layout)), root1Child5Layout);
    CORRADE_COMPARE(layouter.previous(layoutHandleData(root1Child5Layout)), root1Child6Layout);
    CORRADE_COMPARE(layouter.next(layoutHandleData(root1Child5Layout)), root1Child4Layout);
    CORRADE_COMPARE(layouter.previous(layoutHandleData(root1Child4Layout)), root1Child5Layout);
    CORRADE_COMPARE(layouter.next(layoutHandleData(root1Child4Layout)), root1Child3Layout);
    CORRADE_COMPARE(layouter.previous(layoutHandleData(root1Child3Layout)), root1Child4Layout);
    CORRADE_COMPARE(layouter.next(layoutHandleData(root1Child3Layout)), LayoutHandle::Null);

    CORRADE_COMPARE(layouter.parent(root1Child4Layout), root1Layout);
    CORRADE_COMPARE(layouter.parent(root1Child5Layout), root1Layout);
    CORRADE_COMPARE(layouter.parent(root1Child6Layout), root1Layout);
    CORRADE_VERIFY(!layouter.hasExplicitSnap(root1Child4Layout));
    CORRADE_COMPARE(layouter.firstChild(root1Layout), root1Child6Layout);

    /* Removing layouts from the front of the sibling list results in just one
       item left */
    if(data.clean) {
        nodeHandleGenerations[nodeHandleId(root1Child4)] += 1;
        nodeHandleGenerations[nodeHandleId(root1Child5)] += 1;
        nodeHandleGenerations[nodeHandleId(root1Child6)] += 1;
        layouter.cleanNodes(nodeHandleGenerations);
    } else {
        layouter.remove(root1Child4Layout);
        layouter.remove(root1Child5Layout);
        layouter.remove(root1Child6Layout);
    }
    CORRADE_VERIFY(!layouter.isHandleValid(root1Child4Layout));
    CORRADE_VERIFY(!layouter.isHandleValid(root1Child5Layout));
    CORRADE_VERIFY(!layouter.isHandleValid(root1Child6Layout));
    CORRADE_COMPARE(layouter.firstChild(root1Layout), root1Child3Layout);
    CORRADE_COMPARE(layouter.firstExplicitSnap(root1Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.previous(root1Child3Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.next(root1Child3Layout), LayoutHandle::Null);

    /* Removing the remaining layout */
    if(data.clean) {
        nodeHandleGenerations[nodeHandleId(root1Child3)] += 1;
        layouter.cleanNodes(nodeHandleGenerations);
    } else layouter.remove(root1Child3Layout);
    CORRADE_VERIFY(!layouter.isHandleValid(root1Child3Layout));
    CORRADE_COMPARE(layouter.firstChild(root1Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.firstExplicitSnap(root1Layout), LayoutHandle::Null);
}

void SnapLayouterTest::addRemoveExplicitSnap() {
    auto&& data = AddRemoveData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Like addRemove(), but instead of most layouts being implicit children,
       most are with explicit swap, with just one layout being implicit, to
       verify the linked list is appropriately updated in both cases. */

    AbstractUserInterface ui{{100, 100}};
    SnapLayouter& layouter = ui.setLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    NodeHandle root1 = ui.createNode({}, {});
    NodeHandle root1Child1 = ui.createNode(root1, {}, {});
    NodeHandle root1Child2 = ui.createNode(root1, {}, {});
    NodeHandle root1ImplicitSnap = ui.createNode(root1, {}, {});
    NodeHandle root1Child3 = ui.createNode(root1, {}, {});

    NodeHandle root2 = ui.createNode({}, {});
    NodeHandle root3 = ui.createNode({}, {});

    /* Layout snapped to the whole UI, acting as a target to others */
    LayoutHandle root1Layout = layouter.addExplicit(root1, Snap::Top|Snap::FillX, LayoutHandle::Null);
    CORRADE_COMPARE(layouter.node(root1Layout), root1);
    CORRADE_VERIFY(layouter.hasExplicitSnap(root1Layout));
    CORRADE_COMPARE(layouter.flags(root1Layout), SnapLayoutFlags{});
    CORRADE_COMPARE(layouter.snap(root1Layout), Snap::Top|Snap::FillX);
    CORRADE_COMPARE(layouter.explicitSnapTarget(root1Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.firstChild(root1Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.firstExplicitSnap(root1Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.previous(root1Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.next(root1Layout), LayoutHandle::Null);

    /* Layout again snapped to the whole UI, LayouterDataHandle overloads */
    LayoutHandle root2Layout = layouter.addExplicit(root2, Snap::Right, LayouterDataHandle::Null);
    CORRADE_COMPARE(layouter.node(root2Layout), root2);
    CORRADE_VERIFY(layouter.hasExplicitSnap(layoutHandleData(root2Layout)));
    CORRADE_COMPARE(layouter.flags(layoutHandleData(root2Layout)), SnapLayoutFlags{});
    CORRADE_COMPARE(layouter.snap(layoutHandleData(root2Layout)), Snap::Right);
    CORRADE_COMPARE(layouter.explicitSnapTarget(layoutHandleData(root2Layout)), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.firstChild(layoutHandleData(root2Layout)), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.firstExplicitSnap(layoutHandleData(root2Layout)), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.previous(layoutHandleData(root2Layout)), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.next(layoutHandleData(root2Layout)), LayoutHandle::Null);

    /* Layout that's snapped to a parent, being implicitly treated as inside,
       with flags */
    LayoutHandle root1Child1Layout = layouter.addExplicit(root1Child1, Snap::BottomRight, root1Layout, SnapLayoutFlag::IgnoreOverflowY);
    CORRADE_COMPARE(layouter.firstChild(root1Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.firstExplicitSnap(root1Layout), root1Child1Layout);
    CORRADE_COMPARE(layouter.node(root1Child1Layout), root1Child1);
    CORRADE_VERIFY(layouter.hasExplicitSnap(root1Child1Layout));
    CORRADE_COMPARE(layouter.flags(root1Child1Layout), SnapLayoutFlag::IgnoreOverflowY);
    CORRADE_COMPARE(layouter.snap(root1Child1Layout), Snap::BottomRight);
    CORRADE_COMPARE(layouter.explicitSnapTarget(root1Child1Layout), root1Layout);
    CORRADE_COMPARE(layouter.firstChild(root1Child1Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.firstExplicitSnap(root1Child1Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.previous(root1Child1Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.next(root1Child1Layout), LayoutHandle::Null);

    /* Layout that's snapped to a parent, and explicitly snapped inside */
    LayoutHandle root1Child2Layout = layouter.addExplicit(root1Child2, Snap::TopRight|Snap::Inside, root1Layout);
    CORRADE_COMPARE(layouter.firstChild(root1Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.firstExplicitSnap(root1Layout), root1Child1Layout);
    CORRADE_COMPARE(layouter.node(root1Child2Layout), root1Child2);
    CORRADE_VERIFY(layouter.hasExplicitSnap(root1Child2Layout));
    CORRADE_COMPARE(layouter.flags(root1Child2Layout), SnapLayoutFlags{});
    CORRADE_COMPARE(layouter.snap(root1Child2Layout), Snap::TopRight|Snap::Inside);
    CORRADE_COMPARE(layouter.explicitSnapTarget(root1Child2Layout), root1Layout);
    CORRADE_COMPARE(layouter.firstChild(root1Child2Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.firstExplicitSnap(root1Child2Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.previous(root1Child1Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.next(root1Child1Layout), root1Child2Layout);
    CORRADE_COMPARE(layouter.previous(root1Child2Layout), root1Child1Layout);
    CORRADE_COMPARE(layouter.next(root1Child2Layout), LayoutHandle::Null);

    /* Layout that's an implicitly snapped child doesn't get inserted into the
       parent's list of explicitly snapped layouts */
    LayoutHandle root1ImplicitSnapLayout = layouter.add(root1ImplicitSnap);
    CORRADE_COMPARE(layouter.firstChild(root1Layout), root1ImplicitSnapLayout);
    CORRADE_COMPARE(layouter.firstExplicitSnap(root1Layout), root1Child1Layout);
    CORRADE_COMPARE(layouter.node(root1ImplicitSnapLayout), root1ImplicitSnap);
    CORRADE_VERIFY(!layouter.hasExplicitSnap(root1ImplicitSnapLayout));
    CORRADE_COMPARE(layouter.flags(root1ImplicitSnapLayout), SnapLayoutFlags{});
    CORRADE_COMPARE(layouter.snap(root1ImplicitSnapLayout), Snaps{});
    CORRADE_COMPARE(layouter.parent(root1ImplicitSnapLayout), root1Layout);
    CORRADE_COMPARE(layouter.firstChild(root1ImplicitSnapLayout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.firstExplicitSnap(root1ImplicitSnapLayout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.previous(root1ImplicitSnapLayout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.next(root1ImplicitSnapLayout), LayoutHandle::Null);

    /* Layout that's snapped to a sibling doesn't get inserted into the
       parent's list either. LayouterDataHandle overloads, with flags */
    LayoutHandle root1Child3Layout = layouter.addExplicit(root1Child3, Snap::Left|Snap::NoPadX, layoutHandleData(root1Child1Layout), SnapLayoutFlag::IgnoreOverflowX);
    CORRADE_COMPARE(layouter.firstChild(root1Layout), root1ImplicitSnapLayout);
    CORRADE_COMPARE(layouter.firstExplicitSnap(root1Layout), root1Child1Layout);
    CORRADE_COMPARE(layouter.node(root1Child3Layout), root1Child3);
    CORRADE_VERIFY(layouter.hasExplicitSnap(layoutHandleData(root1Child3Layout)));
    CORRADE_COMPARE(layouter.flags(layoutHandleData(root1Child3Layout)), SnapLayoutFlag::IgnoreOverflowX);
    CORRADE_COMPARE(layouter.snap(layoutHandleData(root1Child3Layout)), Snap::Left|Snap::NoPadX);
    CORRADE_COMPARE(layouter.explicitSnapTarget(layoutHandleData(root1Child3Layout)), root1Child1Layout);
    CORRADE_COMPARE(layouter.firstChild(layoutHandleData(root1Child3Layout)), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.firstExplicitSnap(layoutHandleData(root1Child3Layout)), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.previous(layoutHandleData(root1Child1Layout)), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.next(layoutHandleData(root1Child1Layout)), root1Child2Layout);
    CORRADE_COMPARE(layouter.previous(layoutHandleData(root1Child2Layout)), root1Child1Layout);
    CORRADE_COMPARE(layouter.next(layoutHandleData(root1Child2Layout)), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.previous(layoutHandleData(root1Child3Layout)), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.next(layoutHandleData(root1Child3Layout)), LayoutHandle::Null);

    /* Another layout, snapped as a sibling to the root layout, gets added to
       the list along the layouts assigned to node children */
    LayoutHandle root3Layout = layouter.addExplicit(root3, Snap::Top|Snap::NoPadY, root1Layout, SnapLayoutFlag::IgnoreOverflow);
    CORRADE_COMPARE(layouter.firstChild(root1Layout), root1ImplicitSnapLayout);
    CORRADE_COMPARE(layouter.firstExplicitSnap(root1Layout), root1Child1Layout);
    CORRADE_COMPARE(layouter.node(root3Layout), root3);
    CORRADE_VERIFY(layouter.hasExplicitSnap(root3Layout));
    CORRADE_COMPARE(layouter.flags(root3Layout), SnapLayoutFlag::IgnoreOverflow);
    CORRADE_COMPARE(layouter.snap(root3Layout), Snap::Top|Snap::NoPadY);
    CORRADE_COMPARE(layouter.explicitSnapTarget(root3Layout), root1Layout);
    CORRADE_COMPARE(layouter.firstChild(root3Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.firstExplicitSnap(root3Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.previous(root1Child1Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.next(root1Child1Layout), root1Child2Layout);
    CORRADE_COMPARE(layouter.previous(root1Child2Layout), root1Child1Layout);
    CORRADE_COMPARE(layouter.next(root1Child2Layout), root3Layout);
    CORRADE_COMPARE(layouter.previous(root3Layout), root1Child2Layout);
    CORRADE_COMPARE(layouter.next(root3Layout), LayoutHandle::Null);

    /* Passed to cleanNodes(). As no nodes were removed until now, the
       generations are all initially at 1. */
    UnsignedShort nodeHandleGenerations[]{
        1, 1, 1,
        1, 1, 1,
        1,
    };

    /* Removing an explicitly snapped layout that has no parent nor any
       children or explicitly snapped nodes doesn't cause any list to be
       updated */
    if(data.clean) {
        nodeHandleGenerations[nodeHandleId(root1Child3)] += 1;
        layouter.cleanNodes(nodeHandleGenerations);
    } else layouter.remove(root1Child3Layout);
    CORRADE_VERIFY(!layouter.isHandleValid(root1Child3Layout));

    /* Remove a layout from the front of the list updates the handle in the
       target to point to the next one */
    if(data.clean) {
        nodeHandleGenerations[nodeHandleId(root1Child1)] += 1;
        layouter.cleanNodes(nodeHandleGenerations);
    } else layouter.remove(root1Child1Layout);
    CORRADE_VERIFY(!layouter.isHandleValid(root1Child1Layout));
    CORRADE_COMPARE(layouter.firstChild(root1Layout), root1ImplicitSnapLayout);
    CORRADE_COMPARE(layouter.firstExplicitSnap(root1Layout), root1Child2Layout);
    CORRADE_COMPARE(layouter.previous(root1Child2Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.next(root1Child2Layout), root3Layout);
    CORRADE_COMPARE(layouter.previous(root3Layout), root1Child2Layout);
    CORRADE_COMPARE(layouter.next(root3Layout), LayoutHandle::Null);

    /* Removing a layout with an implicit snap doesn't affect the list, even
       though the node it's assigned to is a sibling of the others. It's
       removed from its parent tho. */
    if(data.clean) {
        nodeHandleGenerations[nodeHandleId(root1ImplicitSnap)] += 1;
        layouter.cleanNodes(nodeHandleGenerations);
    } else layouter.remove(root1ImplicitSnapLayout);
    CORRADE_VERIFY(!layouter.isHandleValid(root1ImplicitSnapLayout));
    CORRADE_COMPARE(layouter.firstChild(root1Layout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.firstExplicitSnap(root1Layout), root1Child2Layout);

    /* Removing a layout from the back of the list, LayouterDataHandle
       overload */
    if(data.clean) {
        nodeHandleGenerations[nodeHandleId(root3)] += 1;
        layouter.cleanNodes(nodeHandleGenerations);
    } else layouter.remove(layoutHandleData(root3Layout));
    CORRADE_VERIFY(!layouter.isHandleValid(root3Layout));
    CORRADE_COMPARE(layouter.firstChild(layoutHandleData(root1Layout)), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.firstExplicitSnap(layoutHandleData(root1Layout)), root1Child2Layout);
    CORRADE_COMPARE(layouter.previous(layoutHandleData(root1Child2Layout)), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.next(layoutHandleData(root1Child2Layout)), LayoutHandle::Null);

    /* Removing the last layout updates the node in the target also */
    if(data.clean) {
        nodeHandleGenerations[nodeHandleId(root1Child2)] += 1;
        layouter.cleanNodes(nodeHandleGenerations);
    } else layouter.remove(layoutHandleData(root1Child2Layout));
    CORRADE_VERIFY(!layouter.isHandleValid(root1Child2Layout));
    CORRADE_COMPARE(layouter.firstChild(layoutHandleData(root1Layout)), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.firstExplicitSnap(layoutHandleData(root1Layout)), LayoutHandle::Null);
}

void SnapLayouterTest::addRemoveHandleRecycle() {
    AbstractUserInterface ui{{100, 100}};

    SnapLayouter& layouter = ui.setLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    NodeHandle node = ui.createNode({}, {});
    NodeHandle child0 = ui.createNode(node, {}, {});
    NodeHandle child1 = ui.createNode(node, {}, {});
    NodeHandle child2 = ui.createNode(node, {}, {});
    NodeHandle child3 = ui.createNode(node, {}, {});
    NodeHandle child4 = ui.createNode(node, {}, {});

    LayoutHandle first = layouter.add(node);
    LayoutHandle second = layouter.add(child0);
    LayoutHandle implicit1 = layouter.add(child1, SnapLayoutFlags{0x08});
    LayoutHandle implicit2 = layouter.add(child2, SnapLayoutFlags{0x10});
    LayoutHandle explicit1 = layouter.addExplicit(child3, Snap::Left, first, SnapLayoutFlags{0x20});
    LayoutHandle explicit2 = layouter.addExplicit(child4, Snap::Right, first, SnapLayoutFlags{0x30});
    layouter.setChildSnap(implicit1, Snap::Top|Snap::FillX);
    layouter.setChildSnap(implicit2, Snap::Right|Snap::FillY);
    layouter.setChildSnap(explicit1, Snap::Top|Snap::NoPadX);
    layouter.setChildSnap(explicit2, Snap::BottomLeft|Snap::InsideX);
    CORRADE_VERIFY(!layouter.hasExplicitSnap(implicit1));
    CORRADE_VERIFY(!layouter.hasExplicitSnap(implicit2));
    CORRADE_COMPARE(layouter.parent(implicit1), first);
    CORRADE_COMPARE(layouter.parent(implicit2), first);
    CORRADE_COMPARE(layouter.previous(implicit1), second);
    CORRADE_COMPARE(layouter.next(implicit1), implicit2);
    CORRADE_COMPARE(layouter.previous(implicit2), implicit1);
    CORRADE_COMPARE(layouter.next(implicit2), LayoutHandle::Null);
    CORRADE_VERIFY(layouter.hasExplicitSnap(explicit1));
    CORRADE_VERIFY(layouter.hasExplicitSnap(explicit2));
    CORRADE_COMPARE(layouter.previous(explicit1), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.next(explicit1), explicit2);
    CORRADE_COMPARE(layouter.previous(explicit2), explicit1);
    CORRADE_COMPARE(layouter.next(explicit2), LayoutHandle::Null);

    /* Layouts that reuse a previous slot should have the properties cleared
       even if having them empty / null. First replacing with the same kind
       ... */
    layouter.remove(implicit1);
    LayoutHandle implicit1Replacement = layouter.add(child1);
    CORRADE_COMPARE(layoutHandleId(implicit1Replacement), layoutHandleId(implicit1));
    CORRADE_VERIFY(!layouter.hasExplicitSnap(implicit1Replacement));
    CORRADE_COMPARE(layouter.flags(implicit1Replacement), SnapLayoutFlags{});
    CORRADE_COMPARE(layouter.snap(implicit1Replacement), Snaps{});
    CORRADE_COMPARE(layouter.childSnap(implicit1Replacement), Snap::Bottom);
    CORRADE_COMPARE(layouter.parent(implicit1Replacement), first);
    CORRADE_COMPARE(layouter.previous(implicit1Replacement), implicit2);
    CORRADE_COMPARE(layouter.next(implicit1Replacement), LayoutHandle::Null);

    layouter.remove(explicit1);
    LayoutHandle explicit1Replacement = layouter.addExplicit(child3, Snap::Right|Snap::NoPad, second);
    CORRADE_COMPARE(layoutHandleId(explicit1Replacement), layoutHandleId(explicit1));
    CORRADE_VERIFY(layouter.hasExplicitSnap(explicit1Replacement));
    CORRADE_COMPARE(layouter.flags(explicit1Replacement), SnapLayoutFlags{});
    CORRADE_COMPARE(layouter.snap(explicit1Replacement), Snap::Right|Snap::NoPad);
    CORRADE_COMPARE(layouter.childSnap(explicit1Replacement), Snap::Bottom);
    CORRADE_COMPARE(layouter.explicitSnapTarget(explicit1Replacement), second);

    /* ... and then with the other kind */
    layouter.remove(implicit2);
    LayoutHandle implicit2ExplicitReplacement = layouter.addExplicit(child2, Snap::FillY|Snap::NoPadX, second);
    CORRADE_COMPARE(layoutHandleId(implicit2ExplicitReplacement), layoutHandleId(implicit2));
    CORRADE_VERIFY(layouter.hasExplicitSnap(implicit2ExplicitReplacement));
    CORRADE_COMPARE(layouter.flags(implicit2ExplicitReplacement), SnapLayoutFlags{});
    CORRADE_COMPARE(layouter.snap(implicit2ExplicitReplacement), Snap::FillY|Snap::NoPadX);
    CORRADE_COMPARE(layouter.childSnap(implicit2ExplicitReplacement), Snap::Bottom);
    CORRADE_COMPARE(layouter.explicitSnapTarget(implicit2ExplicitReplacement), second);

    layouter.remove(explicit2);
    LayoutHandle explicit2ImplicitReplacement = layouter.add(child4);
    CORRADE_COMPARE(layoutHandleId(explicit2ImplicitReplacement), layoutHandleId(explicit2));
    CORRADE_VERIFY(!layouter.hasExplicitSnap(explicit2ImplicitReplacement));
    CORRADE_COMPARE(layouter.flags(explicit2ImplicitReplacement), SnapLayoutFlags{});
    CORRADE_COMPARE(layouter.snap(explicit2ImplicitReplacement), Snaps{});
    CORRADE_COMPARE(layouter.childSnap(explicit2ImplicitReplacement), Snap::Bottom);
    CORRADE_COMPARE(layouter.parent(explicit2ImplicitReplacement), first);
    CORRADE_COMPARE(layouter.previous(explicit2ImplicitReplacement), implicit1Replacement);
    CORRADE_COMPARE(layouter.next(explicit2ImplicitReplacement), LayoutHandle::Null);
}

void SnapLayouterTest::addInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui{{100, 100}};
    SnapLayouter& layouter = ui.setLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));
    SnapLayouter& differentLayouter = ui.setLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    NodeHandle nodeWithNoParent = ui.createNode({}, {});
    NodeHandle nodeWithNoParent2 = ui.createNode({}, {});
    NodeHandle nodeWithNoLayout = ui.createNode({}, {});
    NodeHandle nodeWithNoParentLayout = ui.createNode(nodeWithNoLayout, {}, {});
    NodeHandle nodeWithNoParentLayout2 = ui.createNode(nodeWithNoLayout, {}, {});
    NodeHandle nodeWithNoParentLayoutSibling = ui.createNode(nodeWithNoLayout, {}, {});
    LayoutHandle nodeWithNoParentLayoutSiblingLayout = layouter.add(nodeWithNoParentLayoutSibling);

    NodeHandle nodeWithLayout = ui.createNode({}, {});
    LayoutHandle layout = layouter.add(nodeWithLayout);
    LayoutHandle differentLayouterLayout = differentLayouter.add(nodeWithLayout);

    NodeHandle node1 = ui.createNode(nodeWithLayout, {}, {});
    NodeHandle node2 = ui.createNode(nodeWithLayout, {}, {});
    NodeHandle node3 = ui.createNode(nodeWithLayout, {}, {});
    NodeHandle node4 = ui.createNode(nodeWithLayout, {}, {});
    NodeHandle node5 = ui.createNode(nodeWithLayout, {}, {});
    NodeHandle node6 = ui.createNode(nodeWithLayout, {}, {});
    NodeHandle notSiblingOrParentLayoutOfNode = ui.createNode(node1, {}, {});
    LayoutHandle notSiblingOrParentLayoutOfNodeLayout = layouter.add(notSiblingOrParentLayoutOfNode);

    Containers::String out;
    Error redirectError{&out};
    /* Invalid before or target handle */
    layouter.add(ui.createNode({}, {}), differentLayouterLayout);
    layouter.add(ui.createNode({}, {}), layouterDataHandle(0xabcde, 0x123));
    layouter.add(ui.createNode({}, {}), Snap::Fill, differentLayouterLayout);
    layouter.add(ui.createNode({}, {}), Snap::Fill, layouterDataHandle(0xabcde, 0x123));
    layouter.addExplicit(ui.createNode({}, {}), Snap::Left, differentLayouterLayout);
    layouter.addExplicit(ui.createNode({}, {}), Snap::Left, layouterDataHandle(0xabcde, 0x123));
    /* Before handle should be null but isn't */
    layouter.add(nodeWithNoParent, layout);
    layouter.add(nodeWithNoParent2, layoutHandleData(layout));
    layouter.add(nodeWithNoParentLayout, nodeWithNoParentLayoutSiblingLayout);
    layouter.add(nodeWithNoParentLayout2, layoutHandleData(nodeWithNoParentLayoutSiblingLayout));
    /* Layout not a sibling of node */
    layouter.add(node1, layout);
    layouter.add(node2, layoutHandleData(layout));
    /* Layout not a sibling or parent of node */
    layouter.addExplicit(node3, Snap::Top, notSiblingOrParentLayoutOfNodeLayout);
    layouter.addExplicit(node4, Snap::Top, layoutHandleData(notSiblingOrParentLayoutOfNodeLayout));
    /* Snapping a non-root node to the UI */
    layouter.addExplicit(node5, Snap::Fill, LayoutHandle::Null);
    layouter.addExplicit(node6, Snap::Fill, LayouterDataHandle::Null);
    /* Invalid SnapLayoutFlags are tested along with setters in flagsInvalid() */
    CORRADE_COMPARE_AS(out,
        "Ui::SnapLayouter::add(): invalid before handle Ui::LayoutHandle({0x1, 0x1}, {0x0, 0x1})\n"
        "Ui::SnapLayouter::add(): invalid before handle Ui::LayouterDataHandle(0xabcde, 0x123)\n"
        "Ui::SnapLayouter::add(): invalid before handle Ui::LayoutHandle({0x1, 0x1}, {0x0, 0x1})\n"
        "Ui::SnapLayouter::add(): invalid before handle Ui::LayouterDataHandle(0xabcde, 0x123)\n"
        "Ui::SnapLayouter::addExplicit(): invalid target handle Ui::LayoutHandle({0x1, 0x1}, {0x0, 0x1})\n"
        "Ui::SnapLayouter::addExplicit(): invalid target handle Ui::LayouterDataHandle(0xabcde, 0x123)\n"

        "Ui::SnapLayouter::add(): expected before to be null for Ui::NodeHandle(0x0, 0x1) without a parent layout but got Ui::LayouterDataHandle(0x1, 0x1)\n"
        "Ui::SnapLayouter::add(): expected before to be null for Ui::NodeHandle(0x1, 0x1) without a parent layout but got Ui::LayouterDataHandle(0x1, 0x1)\n"
        "Ui::SnapLayouter::add(): expected before to be null for Ui::NodeHandle(0x3, 0x1) without a parent layout but got Ui::LayouterDataHandle(0x0, 0x1)\n"
        "Ui::SnapLayouter::add(): expected before to be null for Ui::NodeHandle(0x4, 0x1) without a parent layout but got Ui::LayouterDataHandle(0x0, 0x1)\n"

        "Ui::SnapLayouter::add(): expected before to be a child of Ui::NodeHandle(0x6, 0x1) but Ui::LayouterDataHandle(0x1, 0x1) is a child of Ui::NodeHandle::Null\n"
        "Ui::SnapLayouter::add(): expected before to be a child of Ui::NodeHandle(0x6, 0x1) but Ui::LayouterDataHandle(0x1, 0x1) is a child of Ui::NodeHandle::Null\n"

        "Ui::SnapLayouter::addExplicit(): expected target to be assigned to either Ui::NodeHandle(0x6, 0x1) or a child of it but Ui::LayouterDataHandle(0x2, 0x1) is a child of Ui::NodeHandle(0x7, 0x1)\n"
        "Ui::SnapLayouter::addExplicit(): expected target to be assigned to either Ui::NodeHandle(0x6, 0x1) or a child of it but Ui::LayouterDataHandle(0x2, 0x1) is a child of Ui::NodeHandle(0x7, 0x1)\n"

        "Ui::SnapLayouter::addExplicit(): can't snap a non-root Ui::NodeHandle(0xb, 0x1) to the whole UI\n"
        "Ui::SnapLayouter::addExplicit(): can't snap a non-root Ui::NodeHandle(0xc, 0x1) to the whole UI\n",
        TestSuite::Compare::String);
}

void SnapLayouterTest::removeInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui{{100, 100}};
    SnapLayouter& layouter = ui.setLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    NodeHandle withChild = ui.createNode({}, {});
    NodeHandle child = ui.createNode(withChild, {}, {});
    LayoutHandle withChildLayout = layouter.add(withChild);
    layouter.add(child);

    NodeHandle withDependent = ui.createNode({}, {});
    NodeHandle dependent = ui.createNode({}, {});
    LayoutHandle withDependentLayout = layouter.add(withDependent);
    layouter.addExplicit(dependent, Snap::Bottom, withDependentLayout);

    NodeHandle withBoth = ui.createNode({}, {});
    NodeHandle child1 = ui.createNode(withBoth, {}, {});
    NodeHandle child2 = ui.createNode(withBoth, {}, {});
    NodeHandle child3 = ui.createNode(withBoth, {}, {});
    NodeHandle dependent1 = ui.createNode({}, {});
    NodeHandle dependent2 = ui.createNode({}, {});
    LayoutHandle withBothLayoutLayout = layouter.add(withBoth);
    layouter.add(child1);
    layouter.add(child2);
    layouter.addExplicit(child3, Snap::Left, withBothLayoutLayout);
    layouter.addExplicit(dependent1, Snap::Top, withBothLayoutLayout);
    layouter.addExplicit(dependent2, Snap::Top, withBothLayoutLayout);

    Containers::String out;
    Error redirectError{&out};
    /* The check is duplicated in both overloads to print the original handle,
       test both */
    layouter.remove(withChildLayout);
    layouter.remove(layoutHandleData(withChildLayout));
    layouter.remove(withDependentLayout);
    layouter.remove(layoutHandleData(withDependentLayout));
    layouter.remove(withBothLayoutLayout);
    layouter.remove(layoutHandleData(withBothLayoutLayout));
    CORRADE_COMPARE_AS(out,
        "Ui::SnapLayouter::remove(): cannot remove Ui::LayoutHandle({0x0, 0x1}, {0x0, 0x1}) with 1 children and 0 dependent layouts\n"
        "Ui::SnapLayouter::remove(): cannot remove Ui::LayouterDataHandle(0x0, 0x1) with 1 children and 0 dependent layouts\n"
        "Ui::SnapLayouter::remove(): cannot remove Ui::LayoutHandle({0x0, 0x1}, {0x2, 0x1}) with 0 children and 1 dependent layouts\n"
        "Ui::SnapLayouter::remove(): cannot remove Ui::LayouterDataHandle(0x2, 0x1) with 0 children and 1 dependent layouts\n"
        "Ui::SnapLayouter::remove(): cannot remove Ui::LayoutHandle({0x0, 0x1}, {0x4, 0x1}) with 2 children and 3 dependent layouts\n"
        "Ui::SnapLayouter::remove(): cannot remove Ui::LayouterDataHandle(0x4, 0x1) with 2 children and 3 dependent layouts\n",
        TestSuite::Compare::String);
}

void SnapLayouterTest::invalidHandle() {
    CORRADE_SKIP_IF_NO_ASSERT();

    SnapLayouter layouter{layouterHandle(0, 1)};

    Containers::String out;
    Error redirectError{&out};
    layouter.remove(LayoutHandle::Null);
    layouter.remove(LayouterDataHandle::Null);
    layouter.hasExplicitSnap(LayoutHandle::Null);
    layouter.hasExplicitSnap(LayouterDataHandle::Null);
    layouter.flags(LayoutHandle::Null);
    layouter.flags(LayouterDataHandle::Null);
    layouter.setFlags(LayoutHandle::Null, {});
    layouter.setFlags(LayouterDataHandle::Null, {});
    layouter.addFlags(LayoutHandle::Null, {});
    layouter.addFlags(LayouterDataHandle::Null, {});
    layouter.clearFlags(LayoutHandle::Null, {});
    layouter.clearFlags(LayouterDataHandle::Null, {});
    layouter.snap(LayoutHandle::Null);
    layouter.snap(LayouterDataHandle::Null);
    layouter.setSnap(LayoutHandle::Null, {});
    layouter.setSnap(LayouterDataHandle::Null, {});
    layouter.childSnap(LayoutHandle::Null);
    layouter.childSnap(LayouterDataHandle::Null);
    layouter.setChildSnap(LayoutHandle::Null, {});
    layouter.setChildSnap(LayouterDataHandle::Null, {});
    layouter.parent(LayoutHandle::Null);
    layouter.parent(LayouterDataHandle::Null);
    layouter.explicitSnapTarget(LayoutHandle::Null);
    layouter.explicitSnapTarget(LayouterDataHandle::Null);
    layouter.firstChild(LayoutHandle::Null);
    layouter.firstChild(LayouterDataHandle::Null);
    layouter.firstExplicitSnap(LayoutHandle::Null);
    layouter.firstExplicitSnap(LayouterDataHandle::Null);
    layouter.previous(LayoutHandle::Null);
    layouter.previous(LayouterDataHandle::Null);
    layouter.next(LayoutHandle::Null);
    layouter.next(LayouterDataHandle::Null);
    CORRADE_COMPARE_AS(out,
        "Ui::SnapLayouter::remove(): invalid handle Ui::LayoutHandle::Null\n"
        "Ui::SnapLayouter::remove(): invalid handle Ui::LayouterDataHandle::Null\n"
        "Ui::SnapLayouter::hasExplicitSnap(): invalid handle Ui::LayoutHandle::Null\n"
        "Ui::SnapLayouter::hasExplicitSnap(): invalid handle Ui::LayouterDataHandle::Null\n"
        "Ui::SnapLayouter::flags(): invalid handle Ui::LayoutHandle::Null\n"
        "Ui::SnapLayouter::flags(): invalid handle Ui::LayouterDataHandle::Null\n"
        "Ui::SnapLayouter::setFlags(): invalid handle Ui::LayoutHandle::Null\n"
        "Ui::SnapLayouter::setFlags(): invalid handle Ui::LayouterDataHandle::Null\n"
        "Ui::SnapLayouter::addFlags(): invalid handle Ui::LayoutHandle::Null\n"
        "Ui::SnapLayouter::addFlags(): invalid handle Ui::LayouterDataHandle::Null\n"
        "Ui::SnapLayouter::clearFlags(): invalid handle Ui::LayoutHandle::Null\n"
        "Ui::SnapLayouter::clearFlags(): invalid handle Ui::LayouterDataHandle::Null\n"
        "Ui::SnapLayouter::snap(): invalid handle Ui::LayoutHandle::Null\n"
        "Ui::SnapLayouter::snap(): invalid handle Ui::LayouterDataHandle::Null\n"
        "Ui::SnapLayouter::setSnap(): invalid handle Ui::LayoutHandle::Null\n"
        "Ui::SnapLayouter::setSnap(): invalid handle Ui::LayouterDataHandle::Null\n"
        "Ui::SnapLayouter::childSnap(): invalid handle Ui::LayoutHandle::Null\n"
        "Ui::SnapLayouter::childSnap(): invalid handle Ui::LayouterDataHandle::Null\n"
        "Ui::SnapLayouter::setChildSnap(): invalid handle Ui::LayoutHandle::Null\n"
        "Ui::SnapLayouter::setChildSnap(): invalid handle Ui::LayouterDataHandle::Null\n"
        "Ui::SnapLayouter::parent(): invalid handle Ui::LayoutHandle::Null\n"
        "Ui::SnapLayouter::parent(): invalid handle Ui::LayouterDataHandle::Null\n"
        "Ui::SnapLayouter::explicitSnapTarget(): invalid handle Ui::LayoutHandle::Null\n"
        "Ui::SnapLayouter::explicitSnapTarget(): invalid handle Ui::LayouterDataHandle::Null\n"
        "Ui::SnapLayouter::firstChild(): invalid handle Ui::LayoutHandle::Null\n"
        "Ui::SnapLayouter::firstChild(): invalid handle Ui::LayouterDataHandle::Null\n"
        "Ui::SnapLayouter::firstExplicitSnap(): invalid handle Ui::LayoutHandle::Null\n"
        "Ui::SnapLayouter::firstExplicitSnap(): invalid handle Ui::LayouterDataHandle::Null\n"
        "Ui::SnapLayouter::previous(): invalid handle Ui::LayoutHandle::Null\n"
        "Ui::SnapLayouter::previous(): invalid handle Ui::LayouterDataHandle::Null\n"
        "Ui::SnapLayouter::next(): invalid handle Ui::LayoutHandle::Null\n"
        "Ui::SnapLayouter::next(): invalid handle Ui::LayouterDataHandle::Null\n",
        TestSuite::Compare::String);
}

void SnapLayouterTest::invalidExplicitSnap() {
    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui{{100, 100}};
    SnapLayouter& layouter = ui.setLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    LayoutHandle implicitSnap = layouter.add(ui.createNode({}, {}));
    LayoutHandle explicitSnap = layouter.addExplicit(ui.createNode({}, {}), {}, LayoutHandle::Null);
    CORRADE_VERIFY(!layouter.hasExplicitSnap(implicitSnap));
    CORRADE_VERIFY(layouter.hasExplicitSnap(explicitSnap));

    Containers::String out;
    Error redirectError{&out};
    layouter.parent(explicitSnap);
    layouter.parent(layoutHandleData(explicitSnap));
    layouter.explicitSnapTarget(implicitSnap);
    layouter.explicitSnapTarget(layoutHandleData(implicitSnap));
    CORRADE_COMPARE_AS(out,
        "Ui::SnapLayouter::parent(): Ui::LayoutHandle({0x0, 0x1}, {0x1, 0x1}) has an explicit snap\n"
        "Ui::SnapLayouter::parent(): Ui::LayouterDataHandle(0x1, 0x1) has an explicit snap\n"
        "Ui::SnapLayouter::explicitSnapTarget(): Ui::LayoutHandle({0x0, 0x1}, {0x0, 0x1}) doesn't have an explicit snap\n"
        "Ui::SnapLayouter::explicitSnapTarget(): Ui::LayouterDataHandle(0x0, 0x1) doesn't have an explicit snap\n",
        TestSuite::Compare::String);
}

void SnapLayouterTest::flags() {
    AbstractUserInterface ui{{100, 100}};
    SnapLayouter& layouter = ui.setLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    NodeHandle node0 = ui.createNode({}, {});
    NodeHandle node1 = ui.createNode({}, {});
    NodeHandle node2 = ui.createNode({}, {});

    /* Add more than one layout to verify the correct one gets updated and not
       always the first. Also verify that it behaves correctly for both
       implicit and explicit snaps and doesn't affect the explicitness. */
    /*LayoutHandle layout0 =*/ layouter.add(node0);
    LayoutHandle layout1 = layouter.add(node1, SnapLayoutFlag::IgnoreOverflowX);
    LayoutHandle layout2 = layouter.addExplicit(node2, Snap::Left, LayoutHandle::Null, SnapLayoutFlag::IgnoreOverflowY);
    CORRADE_VERIFY(!layouter.hasExplicitSnap(layout1));
    CORRADE_VERIFY(layouter.hasExplicitSnap(layout2));
    CORRADE_COMPARE(layouter.flags(layout1), SnapLayoutFlag::IgnoreOverflowX);
    CORRADE_COMPARE(layouter.flags(layout2), SnapLayoutFlag::IgnoreOverflowY);
    CORRADE_COMPARE(layouter.state(), LayouterState::NeedsAssignmentUpdate);

    /* Reset state flags */
    ui.update();
    CORRADE_COMPARE(layouter.state(), LayouterStates{});

    layouter.setFlags(layout1, SnapLayoutFlags{0x30});
    layouter.setFlags(layout2, SnapLayoutFlags{0x20});
    CORRADE_VERIFY(!layouter.hasExplicitSnap(layout1));
    CORRADE_VERIFY(layouter.hasExplicitSnap(layout2));
    CORRADE_COMPARE(layouter.flags(layout1), SnapLayoutFlags{0x30});
    CORRADE_COMPARE(layouter.flags(layout2), SnapLayoutFlags{0x20});
    CORRADE_COMPARE(layouter.state(), LayouterState::NeedsUpdate);

    /* Reset state flags */
    ui.update();
    CORRADE_COMPARE(layouter.state(), LayouterStates{});

    layouter.addFlags(layout1, SnapLayoutFlag::IgnoreOverflowY);
    layouter.addFlags(layout2, SnapLayoutFlag::IgnoreOverflow);
    CORRADE_VERIFY(!layouter.hasExplicitSnap(layout1));
    CORRADE_VERIFY(layouter.hasExplicitSnap(layout2));
    CORRADE_COMPARE(layouter.flags(layout1), SnapLayoutFlag::IgnoreOverflowY|SnapLayoutFlags{0x30});
    CORRADE_COMPARE(layouter.flags(layout2), SnapLayoutFlag::IgnoreOverflow|SnapLayoutFlags{0x20});
    CORRADE_COMPARE(layouter.state(), LayouterState::NeedsUpdate);

    /* Reset state flags */
    ui.update();
    CORRADE_COMPARE(layouter.state(), LayouterStates{});

    layouter.clearFlags(layout1, SnapLayoutFlag::IgnoreOverflowX|SnapLayoutFlags{0x30});
    layouter.clearFlags(layout2, SnapLayoutFlag::PropagateMargin|SnapLayoutFlags{0x20});
    CORRADE_VERIFY(!layouter.hasExplicitSnap(layout1));
    CORRADE_VERIFY(layouter.hasExplicitSnap(layout2));
    CORRADE_COMPARE(layouter.flags(layout1), SnapLayoutFlag::IgnoreOverflowY);
    CORRADE_COMPARE(layouter.flags(layout2), SnapLayoutFlag::IgnoreOverflow);
    CORRADE_COMPARE(layouter.state(), LayouterState::NeedsUpdate);

    /* Reset state flags */
    ui.update();
    CORRADE_COMPARE(layouter.state(), LayouterStates{});

    /* Repeat of the above, just with LayouterDataHandle overloads */
    layouter.setFlags(layoutHandleData(layout1), SnapLayoutFlags{0x30});
    layouter.setFlags(layoutHandleData(layout2), SnapLayoutFlags{0x20});
    CORRADE_VERIFY(!layouter.hasExplicitSnap(layout1));
    CORRADE_VERIFY(layouter.hasExplicitSnap(layout2));
    CORRADE_COMPARE(layouter.flags(layoutHandleData(layout1)), SnapLayoutFlags{0x30});
    CORRADE_COMPARE(layouter.flags(layoutHandleData(layout2)), SnapLayoutFlags{0x20});
    CORRADE_COMPARE(layouter.state(), LayouterState::NeedsUpdate);

    /* Reset state flags */
    ui.update();
    CORRADE_COMPARE(layouter.state(), LayouterStates{});

    layouter.addFlags(layoutHandleData(layout1), SnapLayoutFlag::IgnoreOverflowY);
    layouter.addFlags(layoutHandleData(layout2), SnapLayoutFlag::IgnoreOverflow);
    CORRADE_VERIFY(!layouter.hasExplicitSnap(layout1));
    CORRADE_VERIFY(layouter.hasExplicitSnap(layout2));
    CORRADE_COMPARE(layouter.flags(layoutHandleData(layout1)), SnapLayoutFlag::IgnoreOverflowY|SnapLayoutFlags{0x30});
    CORRADE_COMPARE(layouter.flags(layoutHandleData(layout2)), SnapLayoutFlag::IgnoreOverflow|SnapLayoutFlags{0x20});
    CORRADE_COMPARE(layouter.state(), LayouterState::NeedsUpdate);

    /* Reset state flags */
    ui.update();
    CORRADE_COMPARE(layouter.state(), LayouterStates{});

    layouter.clearFlags(layoutHandleData(layout1), SnapLayoutFlag::IgnoreOverflowX|SnapLayoutFlags{0x30});
    layouter.clearFlags(layoutHandleData(layout2), SnapLayoutFlag::PropagateMargin|SnapLayoutFlags{0x20});
    CORRADE_VERIFY(!layouter.hasExplicitSnap(layout1));
    CORRADE_VERIFY(layouter.hasExplicitSnap(layout2));
    CORRADE_COMPARE(layouter.flags(layoutHandleData(layout1)), SnapLayoutFlag::IgnoreOverflowY);
    CORRADE_COMPARE(layouter.flags(layoutHandleData(layout2)), SnapLayoutFlag::IgnoreOverflow);
    CORRADE_COMPARE(layouter.state(), LayouterState::NeedsUpdate);
}

void SnapLayouterTest::flagsInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui{{100, 100}};
    SnapLayouter& layouter = ui.setLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    /* Combining IgnoreOverflow and PropagateMargin in different axes is fine */
    layouter.add(ui.createNode({}, {}), SnapLayoutFlag::IgnoreOverflowX|SnapLayoutFlag::PropagateMarginY);
    layouter.add(ui.createNode({}, {}), SnapLayoutFlag::IgnoreOverflowY|SnapLayoutFlag::PropagateMarginX);
    LayoutHandle layoutIsFine = layouter.add(ui.createNode({}, {}));
    layouter.setFlags(layoutIsFine, SnapLayoutFlag::IgnoreOverflowX|SnapLayoutFlag::PropagateMarginY);
    /* setFlags() shouldn't take previous flags into account, so this is fine
       even though above the conflicting combination was set */
    layouter.setFlags(layoutIsFine, SnapLayoutFlag::IgnoreOverflowY|SnapLayoutFlag::PropagateMarginX);
    /* And addFlags() *should* take previous flags into account */
    layouter.setFlags(layoutIsFine, SnapLayoutFlag::IgnoreOverflowX);
    layouter.addFlags(layoutIsFine, SnapLayoutFlag::PropagateMarginY);
    layouter.setFlags(layoutIsFine, SnapLayoutFlag::IgnoreOverflowY);
    layouter.addFlags(layoutIsFine, SnapLayoutFlag::PropagateMarginX);

    /* Calling clearFlags() with conflicting flags is fine also, as it won't
       result in them being set */
    layouter.clearFlags(layoutIsFine, SnapLayoutFlag::IgnoreOverflow|SnapLayoutFlag::PropagateMargin);

    NodeHandle node = ui.createNode({}, {});
    LayoutHandle layout = layouter.add(node);
    LayoutHandle layoutIgnoreOverflowX = layouter.add(ui.createNode({}, {}), SnapLayoutFlag::IgnoreOverflowX);
    LayoutHandle layoutIgnoreOverflowY = layouter.add(ui.createNode({}, {}), SnapLayoutFlag::IgnoreOverflowY);
    LayoutHandle layoutIgnoreOverflow = layouter.add(ui.createNode({}, {}), SnapLayoutFlag::IgnoreOverflow);
    LayoutHandle layoutPropagateMarginX = layouter.add(ui.createNode({}, {}), SnapLayoutFlag::PropagateMarginX);
    LayoutHandle layoutPropagateMarginY = layouter.add(ui.createNode({}, {}), SnapLayoutFlag::PropagateMarginY);
    LayoutHandle layoutPropagateMargin = layouter.add(ui.createNode({}, {}), SnapLayoutFlag::PropagateMargin);

    LayoutHandle layoutFill = layouter.add(ui.createNode(node, {}, {}), Snap::Fill);
    LayoutHandle layoutFillX = layouter.add(ui.createNode(node, {}, {}), Snap::FillX);
    LayoutHandle layoutFillY = layouter.add(ui.createNode(node, {}, {}), Snap::FillY);

    /* Propagating margin in a direction different from the fill is okay */
    layouter.setFlags(layoutFillX, SnapLayoutFlag::PropagateMarginY);
    layouter.setFlags(layoutFillY, SnapLayoutFlag::PropagateMarginX);

    Containers::String out;
    Error redirectError{&out};
    /* Using illegal bits */
    layouter.add(ui.createNode({}, {}), SnapLayoutFlags{0x40}|SnapLayoutFlag::IgnoreOverflow);
    layouter.add(ui.createNode({}, {}), SnapLayoutFlags{0x80}|SnapLayoutFlag::IgnoreOverflowX);
    layouter.addExplicit(ui.createNode({}, {}), {}, layout, SnapLayoutFlags{0x40}|SnapLayoutFlag::IgnoreOverflowY);
    layouter.addExplicit(ui.createNode({}, {}), {}, layout, SnapLayoutFlags{0x80}|SnapLayoutFlag::IgnoreOverflow);
    layouter.setFlags(layout, SnapLayoutFlags{0x80}|SnapLayoutFlag::IgnoreOverflowX);
    layouter.setFlags(layoutHandleData(layout), SnapLayoutFlags{0x80}|SnapLayoutFlag::IgnoreOverflowX);
    layouter.setFlags(layout, SnapLayoutFlags{0x40}|SnapLayoutFlag::IgnoreOverflowY);
    layouter.setFlags(layoutHandleData(layout), SnapLayoutFlags{0x40}|SnapLayoutFlag::IgnoreOverflowY);
    layouter.addFlags(layout, SnapLayoutFlags{0x40}|SnapLayoutFlag::IgnoreOverflow);
    layouter.addFlags(layoutHandleData(layout), SnapLayoutFlags{0x40}|SnapLayoutFlag::IgnoreOverflow);
    layouter.addFlags(layout, SnapLayoutFlags{0x80}|SnapLayoutFlag::IgnoreOverflowY);
    layouter.addFlags(layoutHandleData(layout), SnapLayoutFlags{0x80}|SnapLayoutFlag::IgnoreOverflowY);
    layouter.clearFlags(layout, SnapLayoutFlags{0x80}|SnapLayoutFlag::IgnoreOverflow);
    layouter.clearFlags(layoutHandleData(layout), SnapLayoutFlags{0x80}|SnapLayoutFlag::IgnoreOverflow);
    layouter.clearFlags(layout, SnapLayoutFlags{0x40}|SnapLayoutFlag::IgnoreOverflowX);
    layouter.clearFlags(layoutHandleData(layout), SnapLayoutFlags{0x40}|SnapLayoutFlag::IgnoreOverflowX);

    /* Combining IgnoreOverflow and PropagateMargin in same axes, all possible
       conflicting combinations */
    layouter.add(ui.createNode({}, {}), SnapLayoutFlag::IgnoreOverflowX|SnapLayoutFlag::PropagateMarginX);
    layouter.add(ui.createNode({}, {}), SnapLayoutFlag::IgnoreOverflowX|SnapLayoutFlag::PropagateMargin);
    layouter.add(ui.createNode({}, {}), SnapLayoutFlag::IgnoreOverflowY|SnapLayoutFlag::PropagateMarginY);
    layouter.add(ui.createNode({}, {}), SnapLayoutFlag::IgnoreOverflowY|SnapLayoutFlag::PropagateMargin);
    layouter.add(ui.createNode({}, {}), SnapLayoutFlag::IgnoreOverflow|SnapLayoutFlag::PropagateMarginX);
    layouter.add(ui.createNode({}, {}), SnapLayoutFlag::IgnoreOverflow|SnapLayoutFlag::PropagateMarginY);
    layouter.add(ui.createNode({}, {}), SnapLayoutFlag::IgnoreOverflow|SnapLayoutFlag::PropagateMargin);
    /* Explicir snap variant */
    layouter.addExplicit(ui.createNode({}, {}), {}, layout, SnapLayoutFlag::IgnoreOverflowX|SnapLayoutFlag::PropagateMarginX);
    layouter.addExplicit(ui.createNode({}, {}), {}, layout, SnapLayoutFlag::IgnoreOverflowX|SnapLayoutFlag::PropagateMargin);
    layouter.addExplicit(ui.createNode({}, {}), {}, layout, SnapLayoutFlag::IgnoreOverflowY|SnapLayoutFlag::PropagateMarginY);
    layouter.addExplicit(ui.createNode({}, {}), {}, layout, SnapLayoutFlag::IgnoreOverflowY|SnapLayoutFlag::PropagateMargin);
    layouter.addExplicit(ui.createNode({}, {}), {}, layout, SnapLayoutFlag::IgnoreOverflow|SnapLayoutFlag::PropagateMarginX);
    layouter.addExplicit(ui.createNode({}, {}), {}, layout, SnapLayoutFlag::IgnoreOverflow|SnapLayoutFlag::PropagateMarginY);
    layouter.addExplicit(ui.createNode({}, {}), {}, layout, SnapLayoutFlag::IgnoreOverflow|SnapLayoutFlag::PropagateMargin);

    layouter.setFlags(layout, SnapLayoutFlag::IgnoreOverflowX|SnapLayoutFlag::PropagateMarginX);
    layouter.setFlags(layout, SnapLayoutFlag::IgnoreOverflowX|SnapLayoutFlag::PropagateMargin);
    layouter.setFlags(layout, SnapLayoutFlag::IgnoreOverflowY|SnapLayoutFlag::PropagateMarginY);
    layouter.setFlags(layout, SnapLayoutFlag::IgnoreOverflowY|SnapLayoutFlag::PropagateMargin);
    layouter.setFlags(layout, SnapLayoutFlag::IgnoreOverflow|SnapLayoutFlag::PropagateMarginX);
    layouter.setFlags(layout, SnapLayoutFlag::IgnoreOverflow|SnapLayoutFlag::PropagateMarginY);
    layouter.setFlags(layout, SnapLayoutFlag::IgnoreOverflow|SnapLayoutFlag::PropagateMargin);
    /* All setFlags() should delegate to the same helper, so verify just one
       case */
    layouter.setFlags(layoutHandleData(layout), SnapLayoutFlag::IgnoreOverflow|SnapLayoutFlag::PropagateMargin);

    /* Both existing and new flags should be taken into account in addFlags() */
    layouter.addFlags(layoutIgnoreOverflowX, SnapLayoutFlag::PropagateMarginX);
    layouter.addFlags(layoutIgnoreOverflowX, SnapLayoutFlag::PropagateMargin);
    layouter.addFlags(layoutIgnoreOverflowY, SnapLayoutFlag::PropagateMarginY);
    layouter.addFlags(layoutIgnoreOverflowY, SnapLayoutFlag::PropagateMargin);
    layouter.addFlags(layoutIgnoreOverflow, SnapLayoutFlag::PropagateMarginX);
    layouter.addFlags(layoutIgnoreOverflow, SnapLayoutFlag::PropagateMarginY);
    layouter.addFlags(layoutIgnoreOverflow, SnapLayoutFlag::PropagateMargin);
    layouter.addFlags(layoutPropagateMarginX, SnapLayoutFlag::IgnoreOverflowX);
    layouter.addFlags(layoutPropagateMargin, SnapLayoutFlag::IgnoreOverflowX);
    layouter.addFlags(layoutPropagateMarginY, SnapLayoutFlag::IgnoreOverflowY);
    layouter.addFlags(layoutPropagateMargin, SnapLayoutFlag::IgnoreOverflowY);
    layouter.addFlags(layoutPropagateMarginX, SnapLayoutFlag::IgnoreOverflow);
    layouter.addFlags(layoutPropagateMarginY, SnapLayoutFlag::IgnoreOverflow);
    layouter.addFlags(layoutPropagateMargin, SnapLayoutFlag::IgnoreOverflow);
    /* But also the new flags alone should be checked */
    layouter.addFlags(layout, SnapLayoutFlag::IgnoreOverflowX|SnapLayoutFlag::PropagateMarginX);
    layouter.addFlags(layout, SnapLayoutFlag::IgnoreOverflowX|SnapLayoutFlag::PropagateMargin);
    layouter.addFlags(layout, SnapLayoutFlag::IgnoreOverflowY|SnapLayoutFlag::PropagateMarginY);
    layouter.addFlags(layout, SnapLayoutFlag::IgnoreOverflowY|SnapLayoutFlag::PropagateMargin);
    layouter.addFlags(layout, SnapLayoutFlag::IgnoreOverflow|SnapLayoutFlag::PropagateMarginX);
    layouter.addFlags(layout, SnapLayoutFlag::IgnoreOverflow|SnapLayoutFlag::PropagateMarginY);
    layouter.addFlags(layout, SnapLayoutFlag::IgnoreOverflow|SnapLayoutFlag::PropagateMargin);
    /* Propagating margin in a direction that fills, all possible
       combinations */
    layouter.setFlags(layoutFillX, SnapLayoutFlag::PropagateMargin);
    layouter.setFlags(layoutFillX, SnapLayoutFlag::PropagateMarginX);
    layouter.setFlags(layoutFillY, SnapLayoutFlag::PropagateMargin);
    layouter.setFlags(layoutFillY, SnapLayoutFlag::PropagateMarginY);
    layouter.setFlags(layoutFill, SnapLayoutFlag::PropagateMargin);
    layouter.setFlags(layoutFill, SnapLayoutFlag::PropagateMarginX);
    layouter.setFlags(layoutFill, SnapLayoutFlag::PropagateMarginY);
    /* Again all addFlags() should delegate to the same helper, so verify just
       one case */
    layouter.addFlags(layoutHandleData(layoutIgnoreOverflow), SnapLayoutFlag::PropagateMargin);
    CORRADE_COMPARE_AS(out,
        "Ui::SnapLayouter::add(): invalid flags Ui::SnapLayoutFlag::IgnoreOverflow|Ui::SnapLayoutFlag(0x40)\n"
        "Ui::SnapLayouter::add(): invalid flags Ui::SnapLayoutFlag::IgnoreOverflowX|Ui::SnapLayoutFlag(0x80)\n"
        "Ui::SnapLayouter::addExplicit(): invalid flags Ui::SnapLayoutFlag::IgnoreOverflowY|Ui::SnapLayoutFlag(0x40)\n"
        "Ui::SnapLayouter::addExplicit(): invalid flags Ui::SnapLayoutFlag::IgnoreOverflow|Ui::SnapLayoutFlag(0x80)\n"
        "Ui::SnapLayouter::setFlags(): invalid flags Ui::SnapLayoutFlag::IgnoreOverflowX|Ui::SnapLayoutFlag(0x80)\n"
        "Ui::SnapLayouter::setFlags(): invalid flags Ui::SnapLayoutFlag::IgnoreOverflowX|Ui::SnapLayoutFlag(0x80)\n"
        "Ui::SnapLayouter::setFlags(): invalid flags Ui::SnapLayoutFlag::IgnoreOverflowY|Ui::SnapLayoutFlag(0x40)\n"
        "Ui::SnapLayouter::setFlags(): invalid flags Ui::SnapLayoutFlag::IgnoreOverflowY|Ui::SnapLayoutFlag(0x40)\n"
        "Ui::SnapLayouter::addFlags(): invalid flags Ui::SnapLayoutFlag::IgnoreOverflow|Ui::SnapLayoutFlag(0x40)\n"
        "Ui::SnapLayouter::addFlags(): invalid flags Ui::SnapLayoutFlag::IgnoreOverflow|Ui::SnapLayoutFlag(0x40)\n"
        "Ui::SnapLayouter::addFlags(): invalid flags Ui::SnapLayoutFlag::IgnoreOverflowY|Ui::SnapLayoutFlag(0x80)\n"
        "Ui::SnapLayouter::addFlags(): invalid flags Ui::SnapLayoutFlag::IgnoreOverflowY|Ui::SnapLayoutFlag(0x80)\n"
        "Ui::SnapLayouter::clearFlags(): invalid flags Ui::SnapLayoutFlag::IgnoreOverflow|Ui::SnapLayoutFlag(0x80)\n"
        "Ui::SnapLayouter::clearFlags(): invalid flags Ui::SnapLayoutFlag::IgnoreOverflow|Ui::SnapLayoutFlag(0x80)\n"
        "Ui::SnapLayouter::clearFlags(): invalid flags Ui::SnapLayoutFlag::IgnoreOverflowX|Ui::SnapLayoutFlag(0x40)\n"
        "Ui::SnapLayouter::clearFlags(): invalid flags Ui::SnapLayoutFlag::IgnoreOverflowX|Ui::SnapLayoutFlag(0x40)\n"

        "Ui::SnapLayouter::add(): Ui::SnapLayoutFlag::IgnoreOverflowX and Ui::SnapLayoutFlag::PropagateMarginX are mutually exclusive\n"
        "Ui::SnapLayouter::add(): Ui::SnapLayoutFlag::IgnoreOverflowX and Ui::SnapLayoutFlag::PropagateMargin are mutually exclusive\n"
        "Ui::SnapLayouter::add(): Ui::SnapLayoutFlag::IgnoreOverflowY and Ui::SnapLayoutFlag::PropagateMarginY are mutually exclusive\n"
        "Ui::SnapLayouter::add(): Ui::SnapLayoutFlag::IgnoreOverflowY and Ui::SnapLayoutFlag::PropagateMargin are mutually exclusive\n"
        "Ui::SnapLayouter::add(): Ui::SnapLayoutFlag::IgnoreOverflow and Ui::SnapLayoutFlag::PropagateMarginX are mutually exclusive\n"
        "Ui::SnapLayouter::add(): Ui::SnapLayoutFlag::IgnoreOverflow and Ui::SnapLayoutFlag::PropagateMarginY are mutually exclusive\n"
        "Ui::SnapLayouter::add(): Ui::SnapLayoutFlag::IgnoreOverflow and Ui::SnapLayoutFlag::PropagateMargin are mutually exclusive\n"
        /* Explicit snap variants */
        "Ui::SnapLayouter::addExplicit(): Ui::SnapLayoutFlag::IgnoreOverflowX and Ui::SnapLayoutFlag::PropagateMarginX are mutually exclusive\n"
        "Ui::SnapLayouter::addExplicit(): Ui::SnapLayoutFlag::IgnoreOverflowX and Ui::SnapLayoutFlag::PropagateMargin are mutually exclusive\n"
        "Ui::SnapLayouter::addExplicit(): Ui::SnapLayoutFlag::IgnoreOverflowY and Ui::SnapLayoutFlag::PropagateMarginY are mutually exclusive\n"
        "Ui::SnapLayouter::addExplicit(): Ui::SnapLayoutFlag::IgnoreOverflowY and Ui::SnapLayoutFlag::PropagateMargin are mutually exclusive\n"
        "Ui::SnapLayouter::addExplicit(): Ui::SnapLayoutFlag::IgnoreOverflow and Ui::SnapLayoutFlag::PropagateMarginX are mutually exclusive\n"
        "Ui::SnapLayouter::addExplicit(): Ui::SnapLayoutFlag::IgnoreOverflow and Ui::SnapLayoutFlag::PropagateMarginY are mutually exclusive\n"
        "Ui::SnapLayouter::addExplicit(): Ui::SnapLayoutFlag::IgnoreOverflow and Ui::SnapLayoutFlag::PropagateMargin are mutually exclusive\n"

        "Ui::SnapLayouter::setFlags(): Ui::SnapLayoutFlag::IgnoreOverflowX and Ui::SnapLayoutFlag::PropagateMarginX are mutually exclusive\n"
        "Ui::SnapLayouter::setFlags(): Ui::SnapLayoutFlag::IgnoreOverflowX and Ui::SnapLayoutFlag::PropagateMargin are mutually exclusive\n"
        "Ui::SnapLayouter::setFlags(): Ui::SnapLayoutFlag::IgnoreOverflowY and Ui::SnapLayoutFlag::PropagateMarginY are mutually exclusive\n"
        "Ui::SnapLayouter::setFlags(): Ui::SnapLayoutFlag::IgnoreOverflowY and Ui::SnapLayoutFlag::PropagateMargin are mutually exclusive\n"
        "Ui::SnapLayouter::setFlags(): Ui::SnapLayoutFlag::IgnoreOverflow and Ui::SnapLayoutFlag::PropagateMarginX are mutually exclusive\n"
        "Ui::SnapLayouter::setFlags(): Ui::SnapLayoutFlag::IgnoreOverflow and Ui::SnapLayoutFlag::PropagateMarginY are mutually exclusive\n"
        "Ui::SnapLayouter::setFlags(): Ui::SnapLayoutFlag::IgnoreOverflow and Ui::SnapLayoutFlag::PropagateMargin are mutually exclusive\n"
        /* The LayouterDataHandle overload */
        "Ui::SnapLayouter::setFlags(): Ui::SnapLayoutFlag::IgnoreOverflow and Ui::SnapLayoutFlag::PropagateMargin are mutually exclusive\n"

        /* One conflicting flag present before already */
        "Ui::SnapLayouter::addFlags(): Ui::SnapLayoutFlag::IgnoreOverflowX and Ui::SnapLayoutFlag::PropagateMarginX are mutually exclusive\n"
        "Ui::SnapLayouter::addFlags(): Ui::SnapLayoutFlag::IgnoreOverflowX and Ui::SnapLayoutFlag::PropagateMargin are mutually exclusive\n"
        "Ui::SnapLayouter::addFlags(): Ui::SnapLayoutFlag::IgnoreOverflowY and Ui::SnapLayoutFlag::PropagateMarginY are mutually exclusive\n"
        "Ui::SnapLayouter::addFlags(): Ui::SnapLayoutFlag::IgnoreOverflowY and Ui::SnapLayoutFlag::PropagateMargin are mutually exclusive\n"
        "Ui::SnapLayouter::addFlags(): Ui::SnapLayoutFlag::IgnoreOverflow and Ui::SnapLayoutFlag::PropagateMarginX are mutually exclusive\n"
        "Ui::SnapLayouter::addFlags(): Ui::SnapLayoutFlag::IgnoreOverflow and Ui::SnapLayoutFlag::PropagateMarginY are mutually exclusive\n"
        "Ui::SnapLayouter::addFlags(): Ui::SnapLayoutFlag::IgnoreOverflow and Ui::SnapLayoutFlag::PropagateMargin are mutually exclusive\n"
        /* Same, just different order */
        "Ui::SnapLayouter::addFlags(): Ui::SnapLayoutFlag::IgnoreOverflowX and Ui::SnapLayoutFlag::PropagateMarginX are mutually exclusive\n"
        "Ui::SnapLayouter::addFlags(): Ui::SnapLayoutFlag::IgnoreOverflowX and Ui::SnapLayoutFlag::PropagateMargin are mutually exclusive\n"
        "Ui::SnapLayouter::addFlags(): Ui::SnapLayoutFlag::IgnoreOverflowY and Ui::SnapLayoutFlag::PropagateMarginY are mutually exclusive\n"
        "Ui::SnapLayouter::addFlags(): Ui::SnapLayoutFlag::IgnoreOverflowY and Ui::SnapLayoutFlag::PropagateMargin are mutually exclusive\n"
        "Ui::SnapLayouter::addFlags(): Ui::SnapLayoutFlag::IgnoreOverflow and Ui::SnapLayoutFlag::PropagateMarginX are mutually exclusive\n"
        "Ui::SnapLayouter::addFlags(): Ui::SnapLayoutFlag::IgnoreOverflow and Ui::SnapLayoutFlag::PropagateMarginY are mutually exclusive\n"
        "Ui::SnapLayouter::addFlags(): Ui::SnapLayoutFlag::IgnoreOverflow and Ui::SnapLayoutFlag::PropagateMargin are mutually exclusive\n"
        /* Both conflicting flags passed to add() */
        "Ui::SnapLayouter::addFlags(): Ui::SnapLayoutFlag::IgnoreOverflowX and Ui::SnapLayoutFlag::PropagateMarginX are mutually exclusive\n"
        "Ui::SnapLayouter::addFlags(): Ui::SnapLayoutFlag::IgnoreOverflowX and Ui::SnapLayoutFlag::PropagateMargin are mutually exclusive\n"
        "Ui::SnapLayouter::addFlags(): Ui::SnapLayoutFlag::IgnoreOverflowY and Ui::SnapLayoutFlag::PropagateMarginY are mutually exclusive\n"
        "Ui::SnapLayouter::addFlags(): Ui::SnapLayoutFlag::IgnoreOverflowY and Ui::SnapLayoutFlag::PropagateMargin are mutually exclusive\n"
        "Ui::SnapLayouter::addFlags(): Ui::SnapLayoutFlag::IgnoreOverflow and Ui::SnapLayoutFlag::PropagateMarginX are mutually exclusive\n"
        "Ui::SnapLayouter::addFlags(): Ui::SnapLayoutFlag::IgnoreOverflow and Ui::SnapLayoutFlag::PropagateMarginY are mutually exclusive\n"
        "Ui::SnapLayouter::addFlags(): Ui::SnapLayoutFlag::IgnoreOverflow and Ui::SnapLayoutFlag::PropagateMargin are mutually exclusive\n"

        "Ui::SnapLayouter::setFlags(): Ui::SnapLayoutFlag::PropagateMargin is mutually exclusive with Ui::Snap::FillX for an implicitly snapped layout\n"
        "Ui::SnapLayouter::setFlags(): Ui::SnapLayoutFlag::PropagateMarginX is mutually exclusive with Ui::Snap::FillX for an implicitly snapped layout\n"
        "Ui::SnapLayouter::setFlags(): Ui::SnapLayoutFlag::PropagateMargin is mutually exclusive with Ui::Snap::FillY for an implicitly snapped layout\n"
        "Ui::SnapLayouter::setFlags(): Ui::SnapLayoutFlag::PropagateMarginY is mutually exclusive with Ui::Snap::FillY for an implicitly snapped layout\n"
        "Ui::SnapLayouter::setFlags(): Ui::SnapLayoutFlag::PropagateMargin is mutually exclusive with Ui::Snap::Fill for an implicitly snapped layout\n"
        "Ui::SnapLayouter::setFlags(): Ui::SnapLayoutFlag::PropagateMarginX is mutually exclusive with Ui::Snap::Fill for an implicitly snapped layout\n"
        "Ui::SnapLayouter::setFlags(): Ui::SnapLayoutFlag::PropagateMarginY is mutually exclusive with Ui::Snap::Fill for an implicitly snapped layout\n"
        /* The LayouterDataHandle overload */
        "Ui::SnapLayouter::addFlags(): Ui::SnapLayoutFlag::IgnoreOverflow and Ui::SnapLayoutFlag::PropagateMargin are mutually exclusive\n",
        TestSuite::Compare::String);
}

void SnapLayouterTest::snap() {
    AbstractUserInterface ui{{100, 100}};
    SnapLayouter& layouter = ui.setLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    NodeHandle parent = ui.createNode({}, {});
    layouter.add(parent);

    LayoutHandle layout1 = layouter.addExplicit(ui.createNode({}, {}), Snap::TopLeft|Snap::NoPadX, LayoutHandle::Null);
    LayoutHandle layout2 = layouter.add(ui.createNode(parent, {}, {}), Snap::FillX);
    CORRADE_COMPARE(layouter.snap(layout1), Snap::TopLeft|Snap::NoPadX);
    CORRADE_COMPARE(layouter.snap(layout2), Snap::FillX);
    CORRADE_COMPARE(layouter.childSnap(layout1), Snap::Bottom);
    CORRADE_COMPARE(layouter.state(), LayouterState::NeedsAssignmentUpdate);

    /* Reset state flags */
    ui.update();
    CORRADE_COMPARE(layouter.state(), LayouterStates{});

    /* Changing the value sets a flag. The child snap is unaffected. */
    layouter.setSnap(layout1, Snap::Right|Snap::FillY);
    CORRADE_COMPARE(layouter.snap(layout1), Snap::Right|Snap::FillY);
    CORRADE_COMPARE(layouter.childSnap(layout1), Snap::Bottom);
    CORRADE_COMPARE(layouter.state(), LayouterState::NeedsUpdate);

    /* Reset state flags */
    ui.update();
    CORRADE_COMPARE(layouter.state(), LayouterStates{});

    /* Changing the value for an implicit snap */
    layouter.setSnap(layout2, Snap::FillY);
    CORRADE_COMPARE(layouter.snap(layout2), Snap::FillY);
    CORRADE_COMPARE(layouter.childSnap(layout2), Snap::Bottom);
    CORRADE_COMPARE(layouter.state(), LayouterState::NeedsUpdate);

    /* Reset state flags */
    ui.update();
    CORRADE_COMPARE(layouter.state(), LayouterStates{});

    /* LayouterDataHandle overload */
    layouter.setSnap(layoutHandleData(layout1), Snap::TopRight|Snap::InsideX);
    CORRADE_COMPARE(layouter.snap(layoutHandleData(layout1)), Snap::TopRight|Snap::InsideX);
    CORRADE_COMPARE(layouter.childSnap(layout1), Snap::Bottom);
    CORRADE_COMPARE(layouter.state(), LayouterState::NeedsUpdate);

    /* Reset state flags */
    ui.update();
    CORRADE_COMPARE(layouter.state(), LayouterStates{});

    /* Implicit snap LayouterDataHandle overload */
    layouter.setSnap(layoutHandleData(layout2), Snap::Fill);
    CORRADE_COMPARE(layouter.snap(layoutHandleData(layout2)), Snap::Fill);
    CORRADE_COMPARE(layouter.childSnap(layout2), Snap::Bottom);
    CORRADE_COMPARE(layouter.state(), LayouterState::NeedsUpdate);
}

void SnapLayouterTest::snapInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();
    AbstractUserInterface ui{{100, 100}};

    /* Create some extra layouters to have non-trivial handles */
    ui.createLayouter();
    ui.createLayouter();
    ui.removeLayouter(ui.createLayouter());
    SnapLayouter& layouter = ui.setLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    NodeHandle parent = ui.createNode({}, {});
    layouter.add(parent);
    LayoutHandle layout = layouter.add(ui.createNode(parent, {}, {}));
    LayoutHandle layoutPropagateMargin = layouter.add(ui.createNode(parent, {}, {}), SnapLayoutFlag::PropagateMargin);
    LayoutHandle layoutPropagateMarginX = layouter.add(ui.createNode(parent, {}, {}), SnapLayoutFlag::PropagateMarginX);
    LayoutHandle layoutPropagateMarginY = layouter.add(ui.createNode(parent, {}, {}), SnapLayoutFlag::PropagateMarginY);
    LayoutHandle noParentLayout = layouter.add(ui.createNode(ui.createNode({}, {}), {}, {}));

    LayoutHandle explicitSnap = layouter.addExplicit(ui.createNode({}, {}), {}, LayoutHandle::Null);
    LayoutHandle explicitSnapWithNoParentLayout = layouter.addExplicit(ui.createNode({}, {}), {}, LayoutHandle::Null);
    LayoutHandle explicitSnapPropagateMarginX = layouter.addExplicit(ui.createNode({}, {}), {}, LayoutHandle::Null, SnapLayoutFlag::PropagateMarginX);
    LayoutHandle explicitSnapPropagateMarginY = layouter.addExplicit(ui.createNode({}, {}), {}, LayoutHandle::Null, SnapLayoutFlag::PropagateMarginY);

    /* Everything that fails for implicit snap below is okay for explicit */
    layouter.setSnap(explicitSnapWithNoParentLayout, Snap::Fill);
    layouter.setSnap(explicitSnap, Snap::Left);
    layouter.setSnap(explicitSnapPropagateMarginX, Snap::FillX);
    layouter.setSnap(explicitSnapPropagateMarginY, Snap::FillY);

    /* Using fill in a direction different from margin propagation is okay */
    layouter.setSnap(layoutPropagateMarginX, Snap::FillY);
    layouter.setSnap(layoutPropagateMarginY, Snap::FillX);

    Containers::String out;
    Error redirectError{&out};
    /* Setting snap adjustment on a layout with no parent, three different
       variants */
    layouter.setSnap(noParentLayout, Snap::Fill);
    layouter.setSnap(layoutHandleData(noParentLayout), Snap::Fill);
    layouter.add(ui.createNode(ui.createNode({}, {}), {}, {}), Snap::Fill);
    /* Invalid Snap combinations. It should disallow also Fill combined with
       other things */
    layouter.setSnap(layout, Snap::Left);
    layouter.setSnap(layout, Snap::Fill|Snap::InsideX);
    layouter.setSnap(layout, Snap::FillX|Snap::NoPad);
    layouter.setSnap(layout, Snap::FillX|Snap::Top);
    /* Same as above, just triggered directly from add() */
    layouter.add(ui.createNode(parent, {}, {}), Snap::Left);
    /* Fill in a direction that propagates a margin, all possible
       combinations */
    layouter.setSnap(layoutPropagateMarginX, Snap::FillX);
    layouter.setSnap(layoutPropagateMarginX, Snap::Fill);
    layouter.setSnap(layoutPropagateMarginY, Snap::FillY);
    layouter.setSnap(layoutPropagateMarginY, Snap::Fill);
    layouter.setSnap(layoutPropagateMargin, Snap::Fill);
    layouter.setSnap(layoutPropagateMargin, Snap::FillX);
    layouter.setSnap(layoutPropagateMargin, Snap::FillY);
    /* Same, just triggered directly from add() */
    layouter.add(ui.createNode(parent, {}, {}), Snap::Fill, SnapLayoutFlag::PropagateMarginY);
    CORRADE_COMPARE_AS(out,
        "Ui::SnapLayouter::setSnap(): can't set snap adjustment for Ui::LayoutHandle({0x2, 0x2}, {0x5, 0x1}) without a parent layout\n"
        "Ui::SnapLayouter::setSnap(): can't set snap adjustment for Ui::LayouterDataHandle(0x5, 0x1) without a parent layout\n"
        "Ui::SnapLayouter::add(): can't set snap adjustment for Ui::NodeHandle(0xc, 0x1) without a parent layout\n"

        "Ui::SnapLayouter::setSnap(): expected a combination of Ui::Snap::FillX and Ui::Snap::FillY for an implicitly snapped layout but got Ui::Snap::Left\n"
        "Ui::SnapLayouter::setSnap(): expected a combination of Ui::Snap::FillX and Ui::Snap::FillY for an implicitly snapped layout but got Ui::Snap::Fill|Ui::Snap::InsideX\n"
        "Ui::SnapLayouter::setSnap(): expected a combination of Ui::Snap::FillX and Ui::Snap::FillY for an implicitly snapped layout but got Ui::Snap::FillX|Ui::Snap::NoPad\n"
        "Ui::SnapLayouter::setSnap(): expected a combination of Ui::Snap::FillX and Ui::Snap::FillY for an implicitly snapped layout but got Ui::Snap::FillX|Ui::Snap::Top\n"
        "Ui::SnapLayouter::add(): expected a combination of Ui::Snap::FillX and Ui::Snap::FillY for an implicitly snapped layout but got Ui::Snap::Left\n"

        "Ui::SnapLayouter::setSnap(): Ui::Snap::FillX is mutually exclusive with Ui::SnapLayoutFlag::PropagateMarginX for an implicitly snapped layout\n"
        "Ui::SnapLayouter::setSnap(): Ui::Snap::Fill is mutually exclusive with Ui::SnapLayoutFlag::PropagateMarginX for an implicitly snapped layout\n"
        "Ui::SnapLayouter::setSnap(): Ui::Snap::FillY is mutually exclusive with Ui::SnapLayoutFlag::PropagateMarginY for an implicitly snapped layout\n"
        "Ui::SnapLayouter::setSnap(): Ui::Snap::Fill is mutually exclusive with Ui::SnapLayoutFlag::PropagateMarginY for an implicitly snapped layout\n"
        "Ui::SnapLayouter::setSnap(): Ui::Snap::Fill is mutually exclusive with Ui::SnapLayoutFlag::PropagateMargin for an implicitly snapped layout\n"
        "Ui::SnapLayouter::setSnap(): Ui::Snap::FillX is mutually exclusive with Ui::SnapLayoutFlag::PropagateMargin for an implicitly snapped layout\n"
        "Ui::SnapLayouter::setSnap(): Ui::Snap::FillY is mutually exclusive with Ui::SnapLayoutFlag::PropagateMargin for an implicitly snapped layout\n"
        "Ui::SnapLayouter::add(): Ui::Snap::Fill is mutually exclusive with Ui::SnapLayoutFlag::PropagateMarginY for an implicitly snapped layout\n",
        TestSuite::Compare::String);
}

void SnapLayouterTest::childSnap() {
    AbstractUserInterface ui{{100, 100}};
    SnapLayouter& layouter = ui.setLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    NodeHandle node1 = ui.createNode({}, {});
    NodeHandle node2 = ui.createNode({}, {});

    /* By default, the child snap is Bottom for both implicitly and explicitly
       snapped layouts */
    LayoutHandle layout1 = layouter.add(node1);
    LayoutHandle layout2 = layouter.addExplicit(node2, Snap::Left, LayoutHandle::Null);
    CORRADE_COMPARE(layouter.childSnap(layout1), Snap::Bottom);
    CORRADE_COMPARE(layouter.childSnap(layout2), Snap::Bottom);
    CORRADE_COMPARE(layouter.state(), LayouterState::NeedsAssignmentUpdate);

    /* Reset state flags */
    ui.update();
    CORRADE_COMPARE(layouter.state(), LayouterStates{});

    /* Changing the value sets a flag */
    layouter.setChildSnap(layout1, Snap::Right|Snap::FillY);
    CORRADE_COMPARE(layouter.childSnap(layout1), Snap::Right|Snap::FillY);
    CORRADE_COMPARE(layouter.state(), LayouterState::NeedsUpdate);

    /* Reset state flags */
    ui.update();
    CORRADE_COMPARE(layouter.state(), LayouterStates{});

    /* LayouterDataHandle overloads */
    layouter.setChildSnap(layoutHandleData(layout2), Snap::TopRight|Snap::InsideX);
    CORRADE_COMPARE(layouter.childSnap(layoutHandleData(layout2)), Snap::TopRight|Snap::InsideX);
    CORRADE_COMPARE(layouter.state(), LayouterState::NeedsUpdate);
}

void SnapLayouterTest::childSnapInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui{{100, 100}};
    SnapLayouter& layouter = ui.setLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    LayoutHandle layout = layouter.add(ui.createNode({}, {}));

    /* These are fine, as the Inside is implicit */
    layouter.setChildSnap(layout, Snap::Left|Snap::InsideY);
    layouter.setChildSnap(layout, Snap::Right|Snap::InsideY);
    layouter.setChildSnap(layout, Snap::Top|Snap::InsideX);
    layouter.setChildSnap(layout, Snap::Bottom|Snap::InsideX);

    Containers::String out;
    Error redirectError{&out};
    /* Centering / filling in any direction would cause children to be stacked
       on top of each other, overlapping */
    layouter.setChildSnap(layout, {});
    layouter.setChildSnap(layout, Snap::Fill);
    layouter.setChildSnap(layout, Snap::FillX);
    layouter.setChildSnap(layout, Snap::FillY);
    /* NoPad or Inside doesn't turn the above into a valid case */
    layouter.setChildSnap(layout, Snap::NoPad);
    layouter.setChildSnap(layout, Snap::Fill|Snap::Inside);
    /* Snapping to an edge but inside leads to an overlap as well */
    layouter.setChildSnap(layout, Snap::Left|Snap::InsideX);
    layouter.setChildSnap(layout, Snap::Right|Snap::InsideX);
    layouter.setChildSnap(layout, Snap::Top|Snap::InsideY);
    layouter.setChildSnap(layout, Snap::Bottom|Snap::InsideY);
    /* Snapping to a corner wouldn't create a purely horizontal / vertical
       sequence */
    layouter.setChildSnap(layout, Snap::TopLeft);
    layouter.setChildSnap(layout, Snap::TopRight);
    layouter.setChildSnap(layout, Snap::BottomLeft);
    layouter.setChildSnap(layout, Snap::BottomRight);
    CORRADE_COMPARE_AS(out,
        "Ui::SnapLayouter::setChildSnap(): Ui::Snaps{} doesn't produce a non-overlapping purely horizontal or vertical order\n"
        "Ui::SnapLayouter::setChildSnap(): Ui::Snap::Fill doesn't produce a non-overlapping purely horizontal or vertical order\n"
        "Ui::SnapLayouter::setChildSnap(): Ui::Snap::FillX doesn't produce a non-overlapping purely horizontal or vertical order\n"
        "Ui::SnapLayouter::setChildSnap(): Ui::Snap::FillY doesn't produce a non-overlapping purely horizontal or vertical order\n"

        "Ui::SnapLayouter::setChildSnap(): Ui::Snap::NoPad doesn't produce a non-overlapping purely horizontal or vertical order\n"
        "Ui::SnapLayouter::setChildSnap(): Ui::Snap::Fill|Ui::Snap::Inside doesn't produce a non-overlapping purely horizontal or vertical order\n"

        "Ui::SnapLayouter::setChildSnap(): Ui::Snap::Left|Ui::Snap::InsideX doesn't produce a non-overlapping purely horizontal or vertical order\n"
        "Ui::SnapLayouter::setChildSnap(): Ui::Snap::Right|Ui::Snap::InsideX doesn't produce a non-overlapping purely horizontal or vertical order\n"
        "Ui::SnapLayouter::setChildSnap(): Ui::Snap::Top|Ui::Snap::InsideY doesn't produce a non-overlapping purely horizontal or vertical order\n"
        "Ui::SnapLayouter::setChildSnap(): Ui::Snap::Bottom|Ui::Snap::InsideY doesn't produce a non-overlapping purely horizontal or vertical order\n"

        "Ui::SnapLayouter::setChildSnap(): Ui::Snap::TopLeft doesn't produce a non-overlapping purely horizontal or vertical order\n"
        "Ui::SnapLayouter::setChildSnap(): Ui::Snap::TopRight doesn't produce a non-overlapping purely horizontal or vertical order\n"
        "Ui::SnapLayouter::setChildSnap(): Ui::Snap::BottomLeft doesn't produce a non-overlapping purely horizontal or vertical order\n"
        "Ui::SnapLayouter::setChildSnap(): Ui::Snap::BottomRight doesn't produce a non-overlapping purely horizontal or vertical order\n",
        TestSuite::Compare::String);
}

void SnapLayouterTest::setSize() {
    SnapLayouter layouter{layouterHandle(0, 1)};
    CORRADE_COMPARE(layouter.state(), LayouterStates{});

    /* Setting a size sets NeedsUpdate */
    layouter.setSize({153.7f, 0.7f});
    CORRADE_COMPARE(layouter.state(), LayouterState::NeedsUpdate);

    /* Setting it again to the same size triggers that again -- the
       AbstractUserInterface itself makes sure that setSize() is called only
       when the value is different, so it doesn't make sense to check it again
       here */
    layouter.setSize({153.7f, 0.7f});
    CORRADE_COMPARE(layouter.state(), LayouterState::NeedsUpdate);
}

void SnapLayouterTest::clean() {
    AbstractUserInterface ui{{100, 100}};
    SnapLayouter& layouter = ui.setLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    /* Individual removals in clean() are tested in addRemove() and
       addRemoveExplicitSnap(), this verifies that it correctly removes more
       than one */

    NodeHandle root = ui.createNode({}, {});
    NodeHandle nested = ui.createNode(root, {}, {});
    NodeHandle child1 = ui.createNode(nested, {}, {});
    NodeHandle child2 = ui.createNode(nested, {}, {});
    NodeHandle child3 = ui.createNode(nested, {}, {});
    NodeHandle sibling1 = ui.createNode(root, {}, {});
    NodeHandle sibling2 = ui.createNode(root, {}, {});
    NodeHandle rootSibling = ui.createNode({}, {});

    LayoutHandle rootLayout = layouter.add(root);
    LayoutHandle nestedLayout = layouter.add(nested);
    LayoutHandle child1Layout = layouter.add(child1);
    LayoutHandle child2Layout = layouter.add(child2);
    LayoutHandle child3Layout = layouter.addExplicit(child3, Snap::Top, child1Layout);
    LayoutHandle sibling1Layout = layouter.addExplicit(sibling1, Snap::Right, nestedLayout);
    LayoutHandle rootSiblingLayout = layouter.addExplicit(rootSibling, Snap::Right, rootLayout);
    LayoutHandle sibling2Layout = layouter.addExplicit(sibling2, Snap::Right, nestedLayout);
    CORRADE_COMPARE(layouter.capacity(), 8);
    CORRADE_COMPARE(layouter.firstChild(rootLayout), nestedLayout);
    CORRADE_COMPARE(layouter.firstExplicitSnap(rootLayout), rootSiblingLayout);

    ui.removeNode(nested);
    /* The sibling{1,2}Layout depends on nestedLayout and thus has to be
       removed as well to have clean() not fail */
    ui.removeNode(sibling1);
    ui.removeNode(sibling2);
    ui.clean();
    CORRADE_COMPARE(layouter.capacity(), 8);
    CORRADE_COMPARE(layouter.usedCount(), 2);
    CORRADE_VERIFY(layouter.isHandleValid(rootLayout));
    CORRADE_VERIFY(!layouter.isHandleValid(nestedLayout));
    CORRADE_VERIFY(!layouter.isHandleValid(child1Layout));
    CORRADE_VERIFY(!layouter.isHandleValid(child2Layout));
    CORRADE_VERIFY(!layouter.isHandleValid(child3Layout));
    CORRADE_VERIFY(!layouter.isHandleValid(sibling1Layout));
    CORRADE_VERIFY(!layouter.isHandleValid(sibling2Layout));
    CORRADE_VERIFY(layouter.isHandleValid(rootSiblingLayout));
    CORRADE_COMPARE(layouter.firstChild(rootLayout), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.firstExplicitSnap(rootLayout), rootSiblingLayout);
}

void SnapLayouterTest::cleanInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    /* Trimmed-down variant of clean() above, triggering the failure case. It
       should assert on siblingLayout, all others layouts should be either not
       affected or scheduled for removal as well. */

    AbstractUserInterface ui{{100, 100}};
    SnapLayouter& layouter = ui.setLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    NodeHandle root = ui.createNode({}, {});
    NodeHandle nested = ui.createNode(root, {}, {});
    NodeHandle child1 = ui.createNode(nested, {}, {});
    NodeHandle child2 = ui.createNode(nested, {}, {});
    NodeHandle rootSibling = ui.createNode({}, {});
    NodeHandle sibling1 = ui.createNode(root, {}, {});
    NodeHandle sibling2 = ui.createNode(root, {}, {});

    LayoutHandle rootLayout = layouter.add(root);
    /* Just to have a non-trivial handle generation */
    layouter.remove(layouter.add(nested));
    layouter.remove(layouter.add(nested));
    layouter.remove(layouter.add(nested));
    LayoutHandle nestedLayout = layouter.add(nested);
    LayoutHandle child1Layout = layouter.add(child1);
    layouter.addExplicit(child2, Snap::Top, child1Layout);
    layouter.addExplicit(rootSibling, Snap::Right, rootLayout);
    layouter.addExplicit(sibling1, Snap::Right, nestedLayout);
    layouter.addExplicit(sibling2, Snap::Right, nestedLayout);
    ui.removeNode(nested);

    /* This should match what's in the assert below */
    CORRADE_COMPARE(layoutHandleData(nestedLayout), layouterDataHandle(1, 4));

    Containers::String out;
    Error redirectError{&out};
    ui.clean();
    CORRADE_COMPARE_AS(out,
        "Ui::SnapLayouter::clean(): cannot remove Ui::LayouterDataHandle(0x1, 0x4) that still has 2 dependent layouts\n",
        TestSuite::Compare::String);
}

void SnapLayouterTest::layoutEmpty() {
    SnapLayouter layouter{layouterHandle(0, 1)};

    /* Required to be called before layout() (because AbstractUserInterface
       guarantees the same on a higher level), not needed for anything here */
    layouter.setSize({1, 1});

    /* It shouldn't crash or do anything weird */
    layouter.layout({}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_VERIFY(true);
}

void SnapLayouterTest::layoutDataOrder() {
    auto&& data = LayoutDataOrderData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui{{100, 100}};

    /* Just to supply node-specific paddings and margins. Tested to verify the
       properties get used at all, and from both the snapped and target node.
       Tests for complete padding/margin behavior is in snap(), tests for other
       layout properties are in updateLayoutProperties(). */
    struct LayoutLayer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Layout;
        }
        void doLayout(Containers::BitArrayView, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Float>&, const Containers::StridedArrayView1D<Vector4>& nodePaddings, const Containers::StridedArrayView1D<Vector4>& nodeMargins) override {
            /* nodeRoot is used as a target node for a child layout3, so max of
               its Y padding and layout3 Y margin is used, on X no padding is
               used */
            nodePaddings[0 /*nodeRoot*/] = {Nan, 4.0f, Nan, 0.5f};
            nodeMargins[0 /*nodeRoot*/] = Nan4;

            /* nodeChild is used as a target node for a neighbor layout, but
               without any padding, so the margin isn't used at all */
            nodePaddings[1 /*nodeChild*/] = Nan4;
            nodeMargins[1 /*nodeChild*/] = Nan4;

            /* layout1 has no children, no padding gets used; only bottom right
               margin (6, 5) is used */
            nodePaddings[2 /*layout1*/] = Nan4;
            nodeMargins[2 /*layout1*/] = {Nan, Nan, 6.0f, 5.0f};

            /* layout2 is snapped to nodeChild without any padding and has
               nothing else snapped to it, so neither of the two is used */
            nodePaddings[3 /*layout2*/] = Nan4;
            nodeMargins[3 /*layout2*/] = Nan4;

            /* layout3 is snapped to nodeRoot so max of its Y margin and
               nodeRoot Y padding is used (4 top, 5 bottom); it has implicitly
               snapped children to the bottom, and no pad on X so only bottom
               padding is used. The left margin is used for positioning
               layout4. */
            nodePaddings[4 /*layout3*/] = {Nan, Nan, Nan, 15.0f};
            nodeMargins[4 /*layout3*/] = {3.0f, 0.4f, Nan, 5.0f};

            /* layout3 children have no further children so paddings are
               unused, and only the bottom margin is used, but it's never
               larger than layout3 padding */
            nodePaddings[5 /*layout3Child1*/] = Nan4;
            nodePaddings[6 /*layout3Child2*/] = Nan4;
            nodePaddings[7 /*layout3Child3*/] = Nan4;
            nodeMargins[5 /*layout3Child1*/] = {Nan, Nan, Nan, 10.0f};
            nodeMargins[6 /*layout3Child2*/] = {Nan, Nan, Nan, 1.0f};
            nodeMargins[7 /*layout3Child3*/] = {Nan, Nan, Nan, 15.0f};

            /* Snapped to the left of layout3, a max of right margin and
               layout3 left margin is used (3). Padding is used for layout5
               placement. */
            nodePaddings[8 /*layout4*/] = {1.0f, 0.4f, 6.0f, 0.5f};
            nodeMargins[8 /*layout4*/] = {Nan, Nan, 0.3f, Nan};

            /* Placed inside layout4, with a max of the margin and layout4
               padding (1, 4, 6, 5) */
            nodePaddings[9 /*layout5*/] = Nan4;
            nodeMargins[9 /*layout5*/] = {0.1f, 4.0f, 0.6f, 5.0f};
            ++called;
        }

        Int called = 0;
    };
    LayoutLayer& layoutLayer = ui.setLayerInstance(Containers::pointer<LayoutLayer>(ui.createLayer()));

    SnapLayouter& layouter = ui.setLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    if(data.recycledLayouts) {
        NodeHandle node1 = ui.createNode({}, {});
        NodeHandle node2 = ui.createNode({}, {});
        NodeHandle node3 = ui.createNode({}, {});
        NodeHandle node3Child1 = ui.createNode({}, {});
        NodeHandle node3Child2 = ui.createNode({}, {});
        NodeHandle node3Child3 = ui.createNode({}, {});
        NodeHandle node4 = ui.createNode({}, {});
        NodeHandle node5 = ui.createNode({}, {});
        LayoutHandle layout1 = layouter.add(node1);
        LayoutHandle layout2 = layouter.add(node2);
        LayoutHandle layout3 = layouter.add(node3);
        LayoutHandle layout3Child1 = layouter.add(node3Child1);
        LayoutHandle layout3Child2 = layouter.add(node3Child2);
        LayoutHandle layout3Child3 = layouter.add(node3Child3);
        LayoutHandle layout4 = layouter.add(node4);
        LayoutHandle layout5 = layouter.add(node5);
        layouter.remove(layout4);
        layouter.remove(layout3Child3);
        layouter.remove(layout5);
        layouter.remove(layout3);
        layouter.remove(layout3Child1);
        layouter.remove(layout1);
        layouter.remove(layout3Child2);
        layouter.remove(layout2);
        ui.removeNode(node1);
        ui.removeNode(node2);
        ui.removeNode(node3);
        ui.removeNode(node3Child1);
        ui.removeNode(node3Child2);
        ui.removeNode(node3Child3);
        ui.removeNode(node4);
        ui.removeNode(node5);
    }

    /* A layout snapped to the whole UI, with Snap::Inside being implicit */
    NodeHandle nodeRoot = ui.createNode({10.0f, 40.0f}, {100.0f, 200.0f});
    NodeHandle nodeChild = ui.createNode(nodeRoot, {30.0f, 20.0f}, {50.0f, 150.0f});
    NodeHandle node1 = ui.createNode({}, {70.0f, 90.0f});
    /*LayoutHandle layout1 =*/ layouter.addExplicit(node1, Snap::Bottom|Snap::Right, LayoutHandle::Null);

    /* A layout snapped outside of a sibling (non-layouted) node, inheriting
       its offset in addition to having its own offset preserved */
    LayoutHandle nodeChildLayout = layouter.add(nodeChild);
    NodeHandle node2 = ui.createNode(nodeRoot, {0.3f, -0.2f}, {0.0f, 25.0f});
    /*LayoutHandle layout2 =*/ layouter.addExplicit(node2, Snap::Left|Snap::Right|Snap::Top|Snap::NoPadY, nodeChildLayout);

    /* A layout snapped inside of a parent (non-layouted) node, not inheriting
       its offset but having its own offset preserved. The Snap::Inside is
       implicit in this case. */
    LayoutHandle nodeRootLayout = layouter.add(nodeRoot);
    NodeHandle node3 = ui.createNode(nodeRoot, {0.9f, 0.6f}, {10.0f, 0.0f});
    LayoutHandle layout3 = layouter.addExplicit(node3, Snap::Top|Snap::Bottom|Snap::Right|Snap::NoPadX, nodeRootLayout);

    /* Three child nodes & layouts for node3, ordered at the bottom, right to
       left, with no X padding, but with X/Y shift due to offset. All variants
       of setChildSnap() tested in updateChildSnap() below. */
    layouter.setChildSnap(layout3, Snap::BottomLeft|Snap::InsideY|Snap::NoPadX);
    NodeHandle node3Child1 = ui.createNode(node3, {0.0f, -0.5f}, {2.0f, 10.0f});
    NodeHandle node3Child2 = ui.createNode(node3, {0.0f, +0.5f}, {2.0f, 10.0f});
    NodeHandle node3Child3 = ui.createNode(node3, {0.0f, -0.5f}, {2.0f, 10.0f});
    if(!data.shuffledChildLayouts) {
        /*LayoutHandle layout3Child1 =*/ layouter.add(node3Child1);
        /*LayoutHandle layout3Child2 =*/ layouter.add(node3Child2);
        /*LayoutHandle layout3Child3 =*/ layouter.add(node3Child3);
    } else {
        LayoutHandle layout3Child3 = layouter.add(node3Child3);
        /*LayoutHandle layout3Child1 =*/ layouter.add(node3Child1, layout3Child3);
        /*LayoutHandle layout3Child2 =*/ layouter.add(node3Child2, layout3Child3);
    }

    /* A layout relative to layouted node with an offset, should inerit that
       offset in addition to its own, and match its Y size */
    NodeHandle node4 = ui.createNode(nodeRoot, {0.2f, -0.5f}, {20.0f, 0.0f});
    LayoutHandle layout4 = layouter.addExplicit(node4, Snap::Top|Snap::Bottom|Snap::Left, layout3);

    /* A layout that's further dependent on previous, match its XY size & being
       inside */
    NodeHandle node5 = ui.createNode(node4, {0.02f, -0.05f}, {});
    /*LayoutHandle layout5 =*/ layouter.addExplicit(node5, Snap::Top|Snap::Bottom|Snap::Left|Snap::Right, layout4);

    /* Layouts that are root / children / dependent on the above but
       subsequently removed, to verify they're not taken into account when
       ordering them internally. Nodes present always to have it easier to
       verify node properties later. */
    NodeHandle node6 = ui.createNode({}, {});
    NodeHandle node7 = ui.createNode(node6, {}, {});
    NodeHandle node8 = ui.createNode({}, {});
    NodeHandle node9 = ui.createNode(nodeRoot, {}, {});
    NodeHandle node10 = ui.createNode({}, {});
    if(data.removedLayouts) {
        LayoutHandle layout6 = layouter.add(node6);
        layouter.add(node7);
        layouter.addExplicit(node8, Snap::Top, layout6);
        layouter.add(node9);
        layouter.addExplicit(node10, Snap::Top, nodeRootLayout);
    }

    /* Removing the nodes + clean() instead of removing the layouts one by one
       directly to verify that the "mark as unused" operation behaves correctly
       also when the layouts are removed in bulk. */
    ui.removeNode(node6);
    ui.removeNode(node7);
    ui.removeNode(node8);
    ui.removeNode(node9);
    ui.removeNode(node10);
    ui.clean();

    /* The size also */
    ui.setSize({500, 400});

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    /* Add a dummy second layouter because that's the easiest way to verify the
       calculated node offsets / sizes */
    struct DummyLayouter: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::add;

        LayouterFeatures doFeatures() const override { return {}; }
        void doLayout(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Float>&, const Containers::StridedArrayView1D<Vector4>&, const Containers::StridedArrayView1D<Vector4>&, const Containers::StridedArrayView1D<Vector2>& nodeOffsets, const Containers::StridedArrayView1D<Vector2>& nodeSizes) override {
            /* Skipping the last 5 removed nodes, as their sizes/offsets are
               left at random */
            CORRADE_COMPARE_AS(nodeOffsets.exceptSuffix(5), Containers::stridedArrayView<Vector2>({
                /* (6, 5) is right and bottom padding */
                {10.0f, 40.0f},                                 /* nodeRoot */
                {30.0f, 20.0f},                                 /* nodeChild */
                {500.0f - 70.0f - 6.0f, 400.0f - 90.0f - 5.0f}, /* layout1 */
                /* Snapped outside so no X margin / padding, Y spacing
                   disabled, (0.3, -0.2) is the node offset */
                {30.0f + 0.3f, 20.0f - 0.2f - 25.0f},           /* layout2 */
                /* 4 is top padding, X padding disabled, (0.9, 0.6) is the
                   node offset */
                {100.0f - 10.0f + 0.9f, 4.0f + 0.6f},           /* layout3 */
                /* Each child is {2.0f, 10.0f}, no padding on X, on Y there's
                   +-0.5 offset that accumulates, the layout3 height is
                   further shortened by 15 padding */
                {8.0f, 200.0f - 9.0f - 10.0f - 15.0f - 0.5f},   /* layout3Child1 */
                {6.0f, 200.0f - 9.0f - 10.0f - 15.0f + 0.0f},   /* layout3Child2 */
                {4.0f, 200.0f - 9.0f - 10.0f - 15.0f - 0.5f},   /* layout3Child3 */
                /* In addition to what's in layout3 3 is horizontal margin,
                   (0.2, -0.5) is node offset */
                {100.0f - 10.0f + 0.9f - 20.0f - 3.0f + 0.2f,
                 4.0f + 0.6f - 0.5f},                           /* layout4 */
                 /* (1, 4) is padding and (0.02, -0.05) is node offset,
                    relative to layout4 so layout4's offset isn't included */
                {1.0f + 0.02f, 4.0f - 0.05f},                   /* layout5 */
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(nodeSizes.exceptSuffix(5), Containers::stridedArrayView<Vector2>({
                {100.0f, 200.0f},                               /* nodeRoot */
                {50.0f, 150.0f},                                /* nodeChild */
                {70.0f, 90.0f},                                 /* layout1 */
                {50.0f, 25.0f},                                 /* layout2 */
                /* Y size matches nodeRoot height minus top/bottom padding */
                {10.0f, 200.0f - 4.0f - 5.0f},                  /* layout3 */
                {2.0f, 10.0f},                                  /* layout3Child1 */
                {2.0f, 10.0f},                                  /* layout3Child2 */
                {2.0f, 10.0f},                                  /* layout3Child3 */
                /* Y size matches layout3 height, is outside so no padding */
                {20.0f, 200.0f - 4.0f - 5.0f},                  /* layout4 */
                /* XY size matches layout4 size minus padding */
                {20.0f - 1.0f - 6.0f,
                 200.0f - 4.0f - 5.0f - 4.0f - 5.0f},           /* layout5 */
            }), TestSuite::Compare::Container);
            ++called;
        }

        Int called = 0;
    };
    DummyLayouter& dummyLayouter = ui.setLayouterInstance(Containers::pointer<DummyLayouter>(ui.createLayouter()));
    dummyLayouter.add(nodeRoot);
    ui.update();
    CORRADE_COMPARE(layoutLayer.called, 1);
    CORRADE_COMPARE(dummyLayouter.called, 1);
}

void SnapLayouterTest::layoutLayoutProperties() {
    auto&& data = LayoutLayoutPropertiesData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui{{500, 600}};

    struct LayoutLayer: AbstractLayer {
        explicit LayoutLayer(LayerHandle handle, const Vector2& minSize, Float paddingTarget, Float marginTarget, Float margin): AbstractLayer{handle}, _minSize{minSize}, _paddingTarget{paddingTarget}, _marginTarget{marginTarget}, _margin{margin} {}

        LayerFeatures doFeatures() const override {
            return LayerFeature::Layout;
        }
        void doLayout(Containers::BitArrayView, const Containers::StridedArrayView1D<Vector2>& nodeMinSizes, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Float>&, const Containers::StridedArrayView1D<Vector4>& nodePaddings, const Containers::StridedArrayView1D<Vector4>& nodeMargins) override {
            /* Only select properties of the two nodes should be used,
               initialize everything else to 0 */
            for(Vector2& i: nodeMinSizes)
                i = Vector2{Constants::nan()};
            for(Vector4& i: nodePaddings)
                i = Vector4{Constants::nan()};
            for(Vector4& i: nodeMargins)
                i = Vector4{Constants::nan()};

            nodeMinSizes[7 /*node*/] = _minSize;
            nodePaddings[3 /*targetNode*/] = Vector4{_paddingTarget};
            nodeMargins[3 /*targetNode*/] = Vector4{_marginTarget};
            nodeMargins[7 /*node*/] = Vector4{_margin};
            ++called;
        }

        Int called = 0;

        private:
            Vector2 _minSize;
            Float _paddingTarget, _marginTarget, _margin;
    };
    LayoutLayer& layoutLayer = ui.setLayerInstance(Containers::pointer<LayoutLayer>(ui.createLayer(), data.minSize, data.paddingTarget, data.marginTarget, data.margin));

    SnapLayouter& layouter = ui.setLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    ui.createNode({}, {});
    ui.createNode({}, {});
    ui.createNode({}, {});
    NodeHandle targetNode = ui.createNode({300.0f, 400.0f}, {100.0f, 200.0f});
    LayoutHandle targetNodeLayout = layouter.add(targetNode);
    ui.createNode({}, {});
    ui.createNode({}, {});
    ui.createNode({}, {});
    NodeHandle node = ui.createNode(data.child ? targetNode : NodeHandle::Null, {0.25f, 0.75f}, {50.0f, 100.0f});
    CORRADE_COMPARE(nodeHandleId(targetNode), 3);
    CORRADE_COMPARE(nodeHandleId(node), 7);
    if(data.explicitSnap)
        layouter.addExplicit(node, *data.explicitSnap, data.targetUi ? LayoutHandle::Null : targetNodeLayout);
    else
        layouter.add(node);

    /* Add a dummy second layouter because that's the easiest way to verify the
       calculated node offsets / sizes */
    struct DummyLayouter: AbstractLayouter {
        explicit DummyLayouter(LayouterHandle handle, const Vector2& expectedOffset, const Vector2& expectedSize, bool xfailMinSize): AbstractLayouter{handle}, _expectedOffset{expectedOffset}, _expectedSize{expectedSize}, _xfailMinSize{xfailMinSize} {}

        using AbstractLayouter::add;

        LayouterFeatures doFeatures() const override { return {}; }
        void doLayout(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<Vector2>& nodeMinSizes, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Float>&, const Containers::StridedArrayView1D<Vector4>&, const Containers::StridedArrayView1D<Vector4>&, const Containers::StridedArrayView1D<Vector2>& nodeOffsets, const Containers::StridedArrayView1D<Vector2>& nodeSizes) override {
            /* The offset should always include the node-specific offset,
               subtract it for comparison */
            CORRADE_COMPARE(nodeOffsets[7 /*node*/] - (Vector2{0.25f, 0.75f}), _expectedOffset);
            CORRADE_COMPARE(nodeSizes[7 /*node*/], _expectedSize);
            {
                CORRADE_EXPECT_FAIL_IF(_xfailMinSize, "Min size doesn't get correctly applied in this case.");
                CORRADE_COMPARE_AS(nodeSizes[7], nodeMinSizes[7],
                    TestSuite::Compare::GreaterOrEqual);
            }
            ++called;
        }

        Int called = 0;

        private:
            Vector2 _expectedOffset, _expectedSize;
            bool _xfailMinSize;
    };
    DummyLayouter& dummyLayouter = ui.setLayouterInstance(Containers::pointer<DummyLayouter>(ui.createLayouter(), data.expectedOffset, data.expectedSize, data.xfailMinSize));
    dummyLayouter.add(node);
    ui.update();
    CORRADE_COMPARE(layoutLayer.called, 1);
    CORRADE_COMPARE(dummyLayouter.called, 1);
}

void SnapLayouterTest::layoutChildSnap() {
    auto&& data = LayoutChildSnapData[testCaseInstanceId()];
    {
        Containers::String out;
        {
            Debug debug{&out, Debug::Flag::NoNewlineAtTheEnd|Debug::Flag::Packed};
            debug << data.snap;
            if(data.name)
                debug << Debug::nospace << "," << data.name;
        }
        setTestCaseDescription(out);
    }

    AbstractUserInterface ui{{500, 600}};

    struct LayoutLayer: AbstractLayer {
        explicit LayoutLayer(LayerHandle handle, Float padding, Float childMargin): AbstractLayer{handle}, _padding{padding}, _childMargin{childMargin} {}

        LayerFeatures doFeatures() const override {
            return LayerFeature::Layout;
        }
        void doLayout(Containers::BitArrayView, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Float>&, const Containers::StridedArrayView1D<Vector4>& nodePaddings, const Containers::StridedArrayView1D<Vector4>& nodeMargins) override {
            nodePaddings[0 /*root*/] = Vector4{_padding};
            nodeMargins[1 /*child1*/] = Vector4{_childMargin};
            nodeMargins[2 /*child2*/] = Vector4{_childMargin};
            nodeMargins[2 /*child3*/] = Vector4{_childMargin};
            ++called;
        }

        Int called = 0;

        private:
            Float _padding, _childMargin;
    };
    LayoutLayer& layoutLayer = ui.setLayerInstance(Containers::pointer<LayoutLayer>(ui.createLayer(), data.padding, data.childMargin));

    SnapLayouter& layouter = ui.setLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    NodeHandle root = ui.createNode({15.0f, 35.0f}, {100.0f, 200.0f});

    /* Empty Snaps and all Snaps bit set are abused for verifying the default
       behavior, i.e. what gets filled by one of the two add() variants */
    LayoutHandle rootLayout = data.snap == ~Snaps{} ?
        layouter.addExplicit(root, Snap::TopLeft, LayoutHandle::Null) :
        layouter.add(root);
    if(data.snap && data.snap != ~Snaps{})
        layouter.setChildSnap(rootLayout, data.snap);

    NodeHandle child1 = ui.createNode(root, {}, {20.0f, 30.0f});
    NodeHandle child2 = ui.createNode(root, {}, {20.0f, 30.0f});
    NodeHandle child3 = ui.createNode(root, {}, {20.0f, 30.0f});
    layouter.add(child1);
    layouter.add(child2);
    layouter.add(child3);

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    /* Add a dummy second layouter because that's the easiest way to verify the
       calculated node offsets / sizes */
    struct DummyLayouter: AbstractLayouter {
        explicit DummyLayouter(LayouterHandle handle, const Vector2& offset, const Vector2& advance, const Vector2& childSize): AbstractLayouter{handle}, _offset{offset}, _advance{advance}, _childSize{childSize} {}

        using AbstractLayouter::add;

        LayouterFeatures doFeatures() const override { return {}; }

        void doLayout(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Float>&, const Containers::StridedArrayView1D<Vector4>&, const Containers::StridedArrayView1D<Vector4>&, const Containers::StridedArrayView1D<Vector2>& nodeOffsets, const Containers::StridedArrayView1D<Vector2>& nodeSizes) override {
            CORRADE_COMPARE_AS(nodeOffsets, Containers::stridedArrayView<Vector2>({
                {15.0f, 35.0f},
                _offset,
                _offset + _advance,
                _offset + 2.0f*_advance,
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(nodeSizes, Containers::stridedArrayView<Vector2>({
                {100.0f, 200.0f},
                _childSize,
                _childSize,
                _childSize,
            }), TestSuite::Compare::Container);
            ++called;
        }

        Int called = 0;

        private:
            Vector2 _offset, _advance, _childSize;
    };
    DummyLayouter& dummyLayouter = ui.setLayouterInstance(Containers::pointer<DummyLayouter>(ui.createLayouter(), data.offset, data.advance, data.childSize));
    dummyLayouter.add(root);
    ui.update();
    CORRADE_COMPARE(layoutLayer.called, 1);
    CORRADE_COMPARE(dummyLayouter.called, 1);
}

void SnapLayouterTest::layoutPropagateChildSizes() {
    auto&& data = LayoutPropagateChildSizesData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* This doesn't deal with child layout expansion, as that'd make the test
       case even more complex than it is now. It's instead checked in
       layoutExpandChildLayouts() below. */

    AbstractUserInterface ui{{100, 100}};

    /* 5          20  25  30 35 40  45 50  55
    10 +------------------------------------+ outer, ??? width & height,
       |####################################|        padding top/bottom
    15 |+----------------------------------+| inner, fixed? height, ??? width,
       ||                           @@@@@  ||        optional propagated margin
    20 ||          +---+-----+---+  @@@@@  || top, fixed width and height
       ||          |   |     |   |  @@@@@  ||
    25 ||+--------+|   +-----+-----+@@@@@  ||
       |||       #||   @@@@  |x2 | |@@@@@  ||
    30 ||+---+   #||   +--+  +-----++---+@@|| middle, min width, fixed height,
       |||x3 |   #||   |  |      |@@|   |@@||         bottom & top margin
    35 |||@+----+#|+---+--+      |@@+---+@@||
       |||@| |x4|#||x1 |@@@      |  @@@@@  ||
    40 ||+@+----+-+|@@@|-----+   |  @@@@@  || bottom, fixed width, min height,
       ||          |@@@|     |   |  @@@@@  ||         left margin
    45 ||          +---+-----+---+  @@@@@  ||
       ||                           @@@@@  ||
    50 ||                           @@@@@  ||
       ||                           @@@@@  ||
    55 |+----------------------------------+|
       |####################################|
    60 +------------------------------------+
        left,       center,         right,
        ??? size    ??? height,     min width and height,
                    fixed width     margin on all sides

       The left margin of `bottom` node causes all three to shift to the right,
       the top/bottom margin of `right` causes the vertical center alignment to
       shift.

       The `explicit1` (x1) is snapped bottom left inside `center`, it ignores
       the extra left-side padding used by children. The `explicit2` (x2) is
       snapped to the bottom right of `top`, it doesn't affect the `center`
       node size and overflows. The `explicit3` (x3) is snapped to left of
       `inner`, it ignores the vertical shift of children. The `explicit4` is
       snapped inside `left`, either as a sibling or as a child, and its size
       optionally affects `left` size. */

    NodeHandle outer = ui.createNode({10.0f, 5.0f}, data.outerSize);
    NodeHandle inner = ui.createNode(outer, {}, data.innerSize);
    NodeHandle center = ui.createNode(inner, {}, {20.0f, data.centerHeight});
    NodeHandle right = ui.createNode(inner, {}, {});
    NodeHandle middle = ui.createNode(center, {}, {0.0f, 5.0f});
    NodeHandle top = ui.createNode(center, {}, {10.0f, 5.0f});
    NodeHandle left = ui.createNode(inner, {}, data.leftSize);
    NodeHandle bottom = ui.createNode(center, {}, {10.0f, 0.0f});
    NodeHandle explicit1 = ui.createNode(center, {}, {5.0f, 10.0f});
    NodeHandle explicit2 = ui.createNode(center, {}, {10.0f, 5.0f});
    NodeHandle explicit3 = ui.createNode(inner, {}, {5.0f, 10.0f});
    NodeHandle explicit4 = ui.createNode(data.explicit4IsSibling ? inner : left, {}, data.explicit4Size);

    struct LayoutLayer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Layout;
        }
        void doLayout(Containers::BitArrayView, const Containers::StridedArrayView1D<Vector2>& nodeMinSizes, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Float>&, const Containers::StridedArrayView1D<Vector4>& nodePaddings, const Containers::StridedArrayView1D<Vector4>& nodeMargins) override {
            nodePaddings[0 /*outer*/] = {0.0f, 5.0f, 0.0f, 5.0f};

            nodeMinSizes[3 /*right*/] = {5.0f, 5.0f};
            nodeMargins[3 /*right*/] = {5.0f, 15.0f, 5.0f, 20.0f};

            nodeMinSizes[4 /*middle*/].x() = 5.0f;
            nodeMargins[4 /*middle*/] = {0.0f, 5.0f, 0.0f, 5.0f};
            nodePaddings[6 /*left*/] = {0.0f, 0.0f, 5.0f, 0.0f};
            nodeMinSizes[7 /*bottom*/].y() = 5.0f;
            nodeMargins[7 /*bottom*/] = {5.0f, 0.0f, 0.0f, 0.0f};

            nodeMinSizes[11 /*explicit4*/] = explicit4MinSize;
            /* Top/bottom margin isn't used because of NoPadY */
            nodeMargins[11 /*explicit4*/] = {5.0f, 5000.0f, 0.0f, 5000.0f};

            ++called;
        }

        Vector2 explicit4MinSize;
        Int called = 0;
    };
    LayoutLayer& layoutLayer = ui.setLayerInstance(Containers::pointer<LayoutLayer>(ui.createLayer()));
    layoutLayer.explicit4MinSize = data.explicit4MinSize;

    SnapLayouter& layouter = ui.setLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));
    layouter.add(outer, data.outerFlags);

    LayoutHandle innerLayout = layouter.add(inner, data.innerFlags);
    layouter.setChildSnap(innerLayout, data.innerReverseDirection ?
        Snap::Left : Snap::Right);
    LayoutHandle leftLayout = layouter.add(left);

    LayoutHandle centerLayout = layouter.add(center, data.innerReverseDirection ?
        leftLayout : LayoutHandle::Null,
        data.centerFlags);
    layouter.setChildSnap(centerLayout, Snap::BottomLeft|Snap::InsideX);
    LayoutHandle topLayout = layouter.add(top);
    layouter.add(middle);
    layouter.add(bottom);

    layouter.add(right, data.innerReverseDirection ?
        centerLayout : LayoutHandle::Null);

    if(data.explicitlySnappedNodes) {
        /* These shouldn't affect the rest of the layout in any way (so yeah
           they'll overlap) */
        layouter.addExplicit(explicit1, Snap::BottomLeft, centerLayout); /* Inside implicit */
        layouter.addExplicit(explicit2, Snap::BottomRight, topLayout);
        layouter.addExplicit(explicit3, Snap::Left, innerLayout); /* Inside implicit */
        layouter.addExplicit(explicit4, Snap::BottomLeft|Snap::Inside|Snap::NoPadY, leftLayout);
    }

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    /* Add a dummy second layouter because that's the easiest way to verify the
       calculated node offsets / sizes */
    struct DummyLayouter: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::add;

        LayouterFeatures doFeatures() const override { return {}; }
        void doLayout(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Float>&, const Containers::StridedArrayView1D<Vector4>&, const Containers::StridedArrayView1D<Vector4>&, const Containers::StridedArrayView1D<Vector2>& nodeOffsets, const Containers::StridedArrayView1D<Vector2>& nodeSizes) override {
            CORRADE_COMPARE_AS(nodeOffsets, Containers::stridedArrayView<Vector2>({
                {10.0f, 5.0f},                  /*  0, outer */
                {expectedInnerOffsetX, 5.0f},   /*  1, inner */
                {15.0f,                         /*  2, center */
                    expectedInnerCenterY + expectedCenterOffsetY},
                {40.0f,                         /*  3, right */
                    expectedInnerCenterY + 15.0f},
                {expectedCenterLeftEdge, 10.0f},/*  4, middle */
                {expectedCenterLeftEdge, 0.0f}, /*  5, top */
                {0.0f,                          /*  6, left */
                    expectedInnerCenterY + 10.0f},
                {expectedCenterLeftEdge,        /*  7, bottom */
                    20.0f},
                explicitlySnappedNodes ?        /*  8, explicit1 */
                    Vector2{0.0f, expectedCenterHeight - 10.0f} : Vector2{},
                explicitlySnappedNodes ?        /*  9, explicit2 */
                    Vector2{expectedCenterLeftEdge + 10.0f, 5.0f} : Vector2{},
                explicitlySnappedNodes ?        /* 10, explicit3 */
                    Vector2{0.0f, (expectedInnerSize.y() - 10.0f)*0.5f} : Vector2{},
                explicitlySnappedNodes ?        /* 11, explicit4 */
                    expectedExplicit4Offset : Vector2{},
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(nodeSizes, Containers::stridedArrayView<Vector2>({
                expectedOuterSize,              /*  0, outer */
                expectedInnerSize,              /*  1, inner */
                {20.0f, expectedCenterHeight},  /*  2, center */
                {5.0f, 5.0f},                   /*  3, right */
                {5.0f, 5.0f},                   /*  4, middle */
                {10.0f, 5.0f},                  /*  5, top (unchanged) */
                {15.0f, 15.0f},                 /*  6, left */
                {10.0f, 5.0f},                  /*  7, bottom (X unchanged) */
                {5.0f, 10.0f},                  /*  8, explicit1 (unchanged) */
                {10.0f, 5.0f},                  /*  9, explicit2 (unchanged) */
                {5.0f, 10.0f},                  /* 10, explicit3 (unchanged) */
                expectedExplicit4Size,          /* 11, explicit4 */
            }), TestSuite::Compare::Container);
            ++called;
        }

        Vector2 expectedOuterSize;
        Float expectedInnerOffsetX;
        Vector2 expectedInnerSize;
        Float expectedInnerCenterY;
        Vector2 expectedExplicit4Offset, expectedExplicit4Size;
        Float expectedCenterHeight, expectedCenterOffsetY;
        Float expectedCenterLeftEdge;
        bool explicitlySnappedNodes;
        Int called = 0;
    };
    DummyLayouter& dummyLayouter = ui.setLayouterInstance(Containers::pointer<DummyLayouter>(ui.createLayouter()));
    dummyLayouter.expectedOuterSize = data.expectedOuterSize;
    dummyLayouter.expectedInnerOffsetX = data.expectedInnerOffsetX;
    dummyLayouter.expectedInnerSize = data.expectedInnerSize;
    dummyLayouter.expectedInnerCenterY = data.expectedInnerCenterY;
    dummyLayouter.expectedExplicit4Offset = {5.0f,
        (data.explicit4IsSibling ? 25.0f : 15.0f) - Math::max(data.explicit4Size.y(), data.explicit4MinSize.y())};
    dummyLayouter.expectedExplicit4Size = Math::max(data.explicit4Size, data.explicit4MinSize);
    dummyLayouter.expectedCenterHeight = data.expectedCenterHeight;
    dummyLayouter.expectedCenterOffsetY = data.expectedCenterOffsetY;
    dummyLayouter.expectedCenterLeftEdge = data.expectedCenterLeftEdge;
    dummyLayouter.explicitlySnappedNodes = data.explicitlySnappedNodes;
    dummyLayouter.add(outer);
    ui.update();
    CORRADE_COMPARE(layoutLayer.called, 1);
    CORRADE_COMPARE(dummyLayouter.called, 1);
}

void SnapLayouterTest::layoutExpandChildLayouts() {
    auto&& data = LayoutExpandChildLayoutsData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* While layoutPropagateChildSizes() tested bottom-up propagation of min
       sizes, this verifies top-down expansion of child layouts to available
       size.

        root column layout FillX, min / fixed height
           0 5  15 25 30           70 75     95 100
        10 +------------------------------------+ root
        15 +------------------------------------+
           | toolbar, fixed height              |
        25 +------------------------------------+
        30 +------------------------------------+ center
        35 |#+------+@+-------------+@+@@@@@@@+#|
        40 |#| left |@| main, min / |@|+-----+|#|
        45 |#|+--+-+|@| fixed width |@||rTop ||#|
        50 |#||l0|1||@@+-----------+@@|+-----+|#| center + nested row layout
        55 |#|+--+-+|@@|inner, fix |@@||rBot ||#|      FillY
        60 |#|      |@@|  ed height|@@|+-----+|#|
        65 |#+------+@++-----------++@+@@@@@@@+#|
        70 +------------------------------------+ /center
        75 +------------------------------------+
           | statusbar, min height              |
        85 +------------------------------------+
        90 +------------------------------------+ /root
          left, lO, l1               right FillX
             FillX               rTop, rBottom FillY

       The #s and @s denote padding and margin in the center area. It's chosen
       in a way that there's no redundancy, so omitting just one will break the
       result. In particular:
        - the `center` has left/right padding, `left` and `right` don't have
          any corresponding margin on these sides
        - `left` has no margin on its own
        - `main` has left/right margin if PropagateMarginX isn't enabled on it,
          otherwise it has no margin on its own and it's propagated from
          `inner` which then has left/right margin instead
        - `right` has top/bottom margin if PropagateMarginY isn't enabled on
          it, otherwise it has no margin on its own and it's propagated from
          `rTop`, which has top margin, and `rBottom` which has bottom */

    AbstractUserInterface ui{{100, 100}};

    NodeHandle root = ui.createNode({
        0.0f, data.rootFillInsteadOfSize ? 0.0f : 10.0f
    }, {
        data.rootFillInsteadOfSize >= Snap::FillX || data.minSizeInsteadOfFixed ? 0.0f : 100.0f,
        data.rootFillInsteadOfSize >= Snap::FillY || data.minSizeInsteadOfFixed ? 0.0f : 80.0f
    });
    NodeHandle toolbar = ui.createNode(root, {}, {0.0f, 10.0f});
    NodeHandle center = ui.createNode(root, {}, {data.explicitFillChildrenSize ?
        10.0f : 0.0f, 0.0f});
    NodeHandle left = ui.createNode(center, {}, {});
    NodeHandle l0 = ui.createNode(left, {}, {data.explicitFillChildrenSize ?
        1.0f : 0.0f, 10.0f});
    NodeHandle l1 = ui.createNode(left, {}, {data.explicitFillChildrenSize ?
        1.0f : 0.0f, 10.0f});
    NodeHandle main = ui.createNode(center, {}, {data.minSizeInsteadOfFixed ? 0.0f : 40.0f, 0.0f});
    NodeHandle inner = ui.createNode(main, {}, {0.0f, 15.0f});
    NodeHandle right = ui.createNode(center, {}, {0.0f,
        data.explicitFillChildrenSize ? 10.0f : 0.0f});
    NodeHandle rTop = ui.createNode(right, {}, {0.0f,
        data.explicitFillChildrenSize ? 4.0f : 0.0f});
    NodeHandle rBottom = ui.createNode(right, {}, {0.0f,
        data.explicitFillChildrenSize ? 7.0f : 0.0f});
    NodeHandle statusbar = ui.createNode(root, {}, {0.0f, 10.0f});

    struct LayoutLayer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Layout;
        }
        void doLayout(Containers::BitArrayView, const Containers::StridedArrayView1D<Vector2>& nodeMinSizes, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Float>&, const Containers::StridedArrayView1D<Vector4>& nodePaddings, const Containers::StridedArrayView1D<Vector4>& nodeMargins) override {
            nodeMinSizes[11 /*statusbar*/].y() = 10.0f;
            if(minSizeInsteadOfFixed) {
                nodeMinSizes[0 /*root*/] = {
                    rootFillInsteadOfSize >= Snap::FillX ? 0.0f : 100.0f,
                    rootFillInsteadOfSize >= Snap::FillY ? 0.0f : 80.0f
                };
                nodeMinSizes[6 /*main*/].x() = 40.0f;
            }

            /* Only left / right padding */
            nodePaddings[2 /*center*/] = {5.0f, 0.0f, 5.0f, 0.0f};

            /* Only top / bottom margins for these */
            nodeMargins[1 /*toolbar*/] = {0.0f, 5.0f, 0.0f, 5.0f};
            nodeMargins[11 /*statusbar*/] = {0.0f, 5.0f, 0.0f, 5.0f};

            /* Only left / right for `main`. If propagating margin from the
               children, move the horizontal padding from `main` to `inner`. */
            if(mainFlags >= SnapLayoutFlag::PropagateMarginX) {
                nodeMargins[7 /*inner*/] = Vector4{5.0f, 0.0f, 5.0f, 0.0f};
            } else {
                nodeMargins[6 /*main*/] = Vector4{5.0f, 0.0f, 5.0f, 0.0f};
            }

            /* Only top / bottom for `right`. If propagating margin from the
               children, move the vertical margin from `right` to them. It has
               to be larger to extend beyong `right` top / bottom padding
               also. */
            nodePaddings[8 /*right*/] = {0.0f, 5.0f, 0.0f, 5.0f};
            if(rightFlags >= SnapLayoutFlag::PropagateMarginY) {
                nodeMargins[9 /*rTop*/][1] = 10.0f;
                nodeMargins[10 /*rBottom*/][3] = 10.0f;
            } else {
                nodeMargins[8 /*right*/] = Vector4{0.0f, 5.0f, 0.0f, 5.0f};
            }

            /* Extra top/bottom margin for root so it stays at the same offset
               and size as if size was explicitly specified */
            if(rootFillInsteadOfSize >= Snap::FillY)
                nodeMargins[0 /*root*/] = {0.0f, 10.0f, 0.0f, 10.0f};

            ++called;
        }

        bool minSizeInsteadOfFixed;
        Snaps rootFillInsteadOfSize;
        SnapLayoutFlags mainFlags, rightFlags;
        Int called = 0;
    };
    LayoutLayer& layoutLayer = ui.setLayerInstance(Containers::pointer<LayoutLayer>(ui.createLayer()));
    layoutLayer.minSizeInsteadOfFixed = data.minSizeInsteadOfFixed;
    layoutLayer.rootFillInsteadOfSize = data.rootFillInsteadOfSize;
    layoutLayer.mainFlags = data.mainFlags;
    layoutLayer.rightFlags = data.rightFlags;

    SnapLayouter& layouter = ui.setLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    LayoutHandle rootLayout = data.rootFillInsteadOfSize ?
        layouter.addExplicit(root, data.rootFillInsteadOfSize, LayoutHandle::Null) :
        layouter.add(root);
    layouter.setChildSnap(rootLayout, Snap::Bottom|Snap::FillX);

    layouter.add(toolbar);

    LayoutHandle centerLayout = layouter.add(center, Snap::FillY);
    layouter.setChildSnap(centerLayout, Snap::Right|Snap::FillY);

    LayoutHandle leftLayout = layouter.add(left, Snap::FillX);
    layouter.setChildSnap(leftLayout, Snap::Right);
    layouter.add(l0, Snap::FillX);
    layouter.add(l1, Snap::FillX);

    LayoutHandle mainLayout = layouter.add(main, data.mainFlags);
    layouter.setChildSnap(mainLayout, Snap::BottomRight|Snap::InsideY);
    layouter.add(inner, Snap::FillX);

    /* The right layout is ordered bottom up just for extra fun */
    LayoutHandle rightLayout = layouter.add(right, Snap::FillX, data.rightFlags);
    layouter.setChildSnap(rightLayout, Snap::Top|Snap::FillX);
    layouter.add(rBottom, Snap::FillY);
    layouter.add(rTop, Snap::FillY);

    layouter.add(statusbar);

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    /* Add a dummy second layouter because that's the easiest way to verify the
       calculated node offsets / sizes */
    struct DummyLayouter: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::add;

        LayouterFeatures doFeatures() const override { return {}; }
        void doLayout(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Float>&, const Containers::StridedArrayView1D<Vector4>&, const Containers::StridedArrayView1D<Vector4>&, const Containers::StridedArrayView1D<Vector2>& nodeOffsets, const Containers::StridedArrayView1D<Vector2>& nodeSizes) override {
            CORRADE_COMPARE_AS(nodeOffsets, Containers::stridedArrayView<Vector2>({
                { 0.0f, 10.0f},                 /*  0, root */
                { 0.0f,  5.0f},                 /*  1, toolbar */
                { 0.0f, 20.0f},                 /*  2, center */
                { 5.0f,  5.0f},                 /*  3, left */
                { 0.0f, 10.0f},                 /*  4, l0 */
                {10.0f, 10.0f},                 /*  5, l1 */
                {30.0f,  5.0f},                 /*  6, main */
                { 0.0f, 15.0f},                 /*  7, inner */
                {75.0f,  5.0f},                 /*  8, right */
                { 0.0f,  5.0f},                 /*  9, rTop */
                { 0.0f, 15.0f},                 /* 10, rBottom */
                { 0.0f, 65.0f},                 /* 11, statusbar */
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(nodeSizes, Containers::stridedArrayView<Vector2>({
                {100.0f, 80.0f},                /*  0, root */
                {100.0f, 10.0f},                /*  1, toolbar */
                {100.0f, 40.0f},                /*  2, center */
                { 20.0f, 30.0f},                /*  3, left */
                { 10.0f, 10.0f},                /*  4, l0 */
                { 10.0f, 10.0f},                /*  5, l1 */
                { 40.0f, 30.0f},                /*  6, main */
                { 40.0f, 15.0f},                /*  7, inner */
                { 20.0f, 30.0f},                /*  8, right */
                { 20.0f, 10.0f},                /*  9, rTop */
                { 20.0f, 10.0f},                /* 10, rBottom */
                {100.0f, 10.0f},                /* 11, statusbar */
            }), TestSuite::Compare::Container);
            ++called;
        }

        Int called = 0;
    };
    DummyLayouter& dummyLayouter = ui.setLayouterInstance(Containers::pointer<DummyLayouter>(ui.createLayouter()));
    dummyLayouter.add(root);
    ui.update();
    CORRADE_COMPARE(layoutLayer.called, 1);
    CORRADE_COMPARE(dummyLayouter.called, 1);
}

void SnapLayouterTest::layoutExpandChildLayoutsOverflow() {
    auto&& data = LayoutExpandChildLayoutsOverflowData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Verifies interaction of expanding children and IgnoreOverflow. If the
       size including any padding or margin allows, the children should expand
       until (fixed) parent size, otherwise they should stay at their minimal
       size. */

    AbstractUserInterface ui{{100, 100}};

    struct LayoutLayer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Layout;
        }
        void doLayout(Containers::BitArrayView, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Float>&, const Containers::StridedArrayView1D<Vector4>& nodePaddings, const Containers::StridedArrayView1D<Vector4>& nodeMargins) override {
            nodePaddings[0 /*root*/] = Vector4{5.0f};
            nodeMargins[2 /*second*/] = Vector4{5.0f};
            ++called;
        }

        Int called = 0;
    };
    LayoutLayer& layoutLayer = ui.setLayerInstance(Containers::pointer<LayoutLayer>(ui.createLayer()));

    SnapLayouter& layouter = ui.setLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    struct DummyLayouter: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::add;

        LayouterFeatures doFeatures() const override { return {}; }
        void doLayout(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Float>&, const Containers::StridedArrayView1D<Vector4>&, const Containers::StridedArrayView1D<Vector4>&, const Containers::StridedArrayView1D<Vector2>& nodeOffsets, const Containers::StridedArrayView1D<Vector2>& nodeSizes) override {
            CORRADE_COMPARE_AS(nodeOffsets, Containers::stridedArrayView<Vector2>({
                { 5.0f, 5.0f},                              /* 0, root */
                { 5.0f, 5.0f},                              /* 1, first */
                {horizontal ? 10.0f + expectedSize : 5.0f,  /* 2, second */
                 horizontal ? 5.0f : 10.0f + expectedSize},
                {horizontal ? 65.0f + expectedSize : 5.0f,  /* 3, third */
                 horizontal ? 5.0f : 65.0f + expectedSize},
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(nodeSizes, Containers::stridedArrayView<Vector2>({
                {horizontal ? expectedRootSize : 60.0f,     /* 1, first */
                 horizontal ? 60.0f : expectedRootSize},
                {horizontal ? expectedSize : 50.0f,         /* 1, first */
                 horizontal ? 50.0f : expectedSize},
                {50.0f, 50.0f},                             /* 2, second */
                {horizontal ? expectedSize : 50.0f,         /* 3, third */
                 horizontal ? 50.0f : expectedSize},
            }), TestSuite::Compare::Container);
            ++called;
        }

        bool xfail;
        bool horizontal;
        Float expectedSize, expectedRootSize;
        Int called = 0;
    };
    DummyLayouter& dummyLayouter = ui.setLayouterInstance(Containers::pointer<DummyLayouter>(ui.createLayouter()));
    dummyLayouter.expectedSize = data.expectedSize;
    dummyLayouter.expectedRootSize = data.expectedRootSize;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    /* Horizontal case */
    {
        CORRADE_ITERATION(__FILE__ ":" CORRADE_LINE_STRING);

        NodeHandle root = ui.createNode({5.0f, 5.0f}, {100.0f, 60.0f});
        NodeHandle first = ui.createNode(root, {}, {data.size, 0.0f});
        NodeHandle second = ui.createNode(root, {}, {50.0f, 0.0f});
        NodeHandle third = ui.createNode(root, {}, {data.size, 0.0f});

        LayoutHandle rootLayout = layouter.add(root, data.flagsX);
        layouter.setChildSnap(rootLayout, Snap::Right|Snap::FillY);
        layouter.add(first, data.snapX);
        layouter.add(second);
        layouter.add(third, data.snapX);
        dummyLayouter.add(root);
        dummyLayouter.horizontal = true;

        ui.update();
        CORRADE_COMPARE(layoutLayer.called, 1);
        CORRADE_COMPARE(dummyLayouter.called, 1);

        /* Remove the existing nodes in the order they were created to make the
           other nodes below occupy the exact same IDs */
        ui.removeNode(root);
        ui.removeNode(first);
        ui.removeNode(second);
        ui.removeNode(third);

    /* Vertical case */
    } {
        CORRADE_ITERATION(__FILE__ ":" CORRADE_LINE_STRING);

        NodeHandle root = ui.createNode({5.0f, 5.0f}, {60.0f, 100.0f});
        NodeHandle first = ui.createNode(root, {}, {0.0f, data.size});
        NodeHandle second = ui.createNode(root, {}, {0.0f, 50.0f});
        NodeHandle third = ui.createNode(root, {}, {0.0f, data.size});
        CORRADE_COMPARE(nodeHandleId(root), 0);
        CORRADE_COMPARE(nodeHandleId(first), 1);
        CORRADE_COMPARE(nodeHandleId(second), 2);
        CORRADE_COMPARE(nodeHandleId(third), 3);

        LayoutHandle rootLayout = layouter.add(root, data.flagsY);
        layouter.setChildSnap(rootLayout, Snap::Bottom|Snap::FillX);
        layouter.add(first, data.snapY);
        layouter.add(second);
        layouter.add(third, data.snapY);
        dummyLayouter.add(root);
        dummyLayouter.horizontal = false;

        ui.update();
        CORRADE_COMPARE(layoutLayer.called, 2);
        CORRADE_COMPARE(dummyLayouter.called, 2);
    }
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::SnapLayouterTest)
