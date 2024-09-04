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
#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/StringStl.h> /** @todo remove once Debug is stream-free */
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Container.h>
#include <Corrade/TestSuite/Compare/String.h>
#include <Corrade/Utility/DebugStl.h> /** @todo remove once Debug is stream-free */

#include "Magnum/Ui/Anchor.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/NodeFlags.h"
#include "Magnum/Ui/SnapLayouter.h"
#include "Magnum/Ui/UserInterface.h"
#include "Magnum/Ui/Implementation/orderNodesBreadthFirstInto.h"
#include "Magnum/Ui/Implementation/snapLayouter.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct SnapLayouterTest: TestSuite::Tester {
    explicit SnapLayouterTest();

    void debugSnap();
    void debugSnapPacked();
    void debugSnaps();
    void debugSnapsPacked();
    void debugSnapsSupersets();

    void snapInside();
    void snap();
    void orderLayoutsBreadthFirst();

    void construct();
    void constructCopy();
    void constructMove();

    void setPadding();
    void setMargin();

    template<class T> void layoutConstructInside();
    template<class T> void layoutConstructOutside();
    void layoutConstructDefaultLayouter();
    template<class T> void layoutConstructCopy();
    template<class T> void layoutConstructMove();

    template<class T> void addRemove();
    void addRemoveHandleRecycle();
    void addDefaultLayouter();
    void layoutInvalid();

    void setSize();

    void invalidHandle();

    void updateEmpty();
    void updateDataOrder();
};

const struct {
    Snaps snap;
    BitVector2 expected;
} SnapInsideData[]{
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

const struct {
    Snaps snap;
    Vector2 expectedOffset, expectedSize;
} SnapData[]{
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

const struct {
    const char* name;
    bool setMarginPaddingLater;
    bool recycledLayouts;
} UpdateDataOrderData[]{
    {"", false, false},
    {"margin & padding set later", true, false},
    {"layouts recycled in shuffled order", false, true},
};

SnapLayouterTest::SnapLayouterTest() {
    addTests({&SnapLayouterTest::debugSnap,
              &SnapLayouterTest::debugSnapPacked,
              &SnapLayouterTest::debugSnaps,
              &SnapLayouterTest::debugSnapsPacked,
              &SnapLayouterTest::debugSnapsSupersets});

    addInstancedTests({&SnapLayouterTest::snapInside},
        Containers::arraySize(SnapInsideData));

    addInstancedTests({&SnapLayouterTest::snap},
        Containers::arraySize(SnapData));

    addTests({&SnapLayouterTest::orderLayoutsBreadthFirst,

              &SnapLayouterTest::construct,
              &SnapLayouterTest::constructCopy,
              &SnapLayouterTest::constructMove,

              &SnapLayouterTest::setPadding,
              &SnapLayouterTest::setMargin,

              &SnapLayouterTest::layoutConstructInside<AbstractSnapLayout>,
              &SnapLayouterTest::layoutConstructInside<SnapLayout>,
              &SnapLayouterTest::layoutConstructDefaultLayouter,
              &SnapLayouterTest::layoutConstructOutside<AbstractSnapLayout>,
              &SnapLayouterTest::layoutConstructOutside<SnapLayout>,
              &SnapLayouterTest::layoutConstructCopy<AbstractSnapLayout>,
              &SnapLayouterTest::layoutConstructCopy<SnapLayout>,
              &SnapLayouterTest::layoutConstructMove<AbstractSnapLayout>,
              &SnapLayouterTest::layoutConstructMove<SnapLayout>,

              &SnapLayouterTest::addRemove<AbstractSnapLayout>,
              &SnapLayouterTest::addRemove<SnapLayout>,
              &SnapLayouterTest::addRemoveHandleRecycle,
              &SnapLayouterTest::addDefaultLayouter,
              &SnapLayouterTest::layoutInvalid,

              &SnapLayouterTest::setSize,

              &SnapLayouterTest::invalidHandle,

              &SnapLayouterTest::updateEmpty});

    addInstancedTests({&SnapLayouterTest::updateDataOrder},
        Containers::arraySize(UpdateDataOrderData));
}

void SnapLayouterTest::debugSnap() {
    std::ostringstream out;
    Debug{&out} << Snap::InsideX << Snap(0xbe);
    CORRADE_COMPARE(out.str(), "Ui::Snap::InsideX Ui::Snap(0xbe)\n");
}

void SnapLayouterTest::debugSnapPacked() {
    std::ostringstream out;
    /* Last is not packed, ones before should not make any flags persistent */
    Debug{&out} << Debug::packed << Snap::InsideX << Debug::packed << Snap(0xbe) << Snap::Right;
    CORRADE_COMPARE(out.str(), "InsideX 0xbe Ui::Snap::Right\n");
}

void SnapLayouterTest::debugSnaps() {
    std::ostringstream out;
    /* There isn't any bit free to test how the remains get printed */
    Debug{&out} << (Snap::Left|Snap::InsideX) << Snaps{};
    CORRADE_COMPARE(out.str(), "Ui::Snap::Left|Ui::Snap::InsideX Ui::Snaps{}\n");
}

void SnapLayouterTest::debugSnapsPacked() {
    std::ostringstream out;
    /* There isn't any bit free to test how the remains get printed. Last is
       not packed, ones before should not make any flags persistent. */
    Debug{&out} << Debug::packed << (Snap::Left|Snap::NoSpaceY) << Debug::packed << Snaps{} << (Snap::InsideX|Snap::NoSpaceY);
    CORRADE_COMPARE(out.str(), "Left|NoSpaceY {} Ui::Snap::InsideX|Ui::Snap::NoSpaceY\n");
}

void SnapLayouterTest::debugSnapsSupersets() {
    /* Fill is all FillX and FillY combined */
    {
        std::ostringstream out;
        Debug{&out} << (Snap::Fill|Snap::FillX|Snap::FillY);
        CORRADE_COMPARE(out.str(), "Ui::Snap::Fill\n");

    /* FillX and FillY is edges combined */
    } {
        std::ostringstream out;
        Debug{&out}
            << (Snap::FillX|Snap::Left|Snap::Right)
            << (Snap::FillY|Snap::Top|Snap::Bottom);
        CORRADE_COMPARE(out.str(), "Ui::Snap::FillX Ui::Snap::FillY\n");

    /* Corners are edges combined */
    } {
        std::ostringstream out;
        Debug{&out}
            << (Snap::TopLeft|Snap::Top|Snap::Left)
            << (Snap::BottomLeft|Snap::Bottom|Snap::Left)
            << (Snap::TopRight|Snap::Top|Snap::Right)
            << (Snap::BottomRight|Snap::Bottom|Snap::Right);
        CORRADE_COMPARE(out.str(), "Ui::Snap::TopLeft Ui::Snap::BottomLeft Ui::Snap::TopRight Ui::Snap::BottomRight\n");

    /* Combining corners + edges picks up the fill first, not corners */
    } {
        std::ostringstream out;
        Debug{&out}
            /* Both in each pair do the same */
            << (Snap::TopLeft|Snap::Right) << (Snap::FillX|Snap::Top)
            << (Snap::BottomRight|Snap::Top) << (Snap::FillY|Snap::Right);
        CORRADE_COMPARE(out.str(), "Ui::Snap::FillX|Ui::Snap::Top Ui::Snap::FillX|Ui::Snap::Top Ui::Snap::FillY|Ui::Snap::Right Ui::Snap::FillY|Ui::Snap::Right\n");

    /* Inside is InsideX and InsideY combined */
    } {
        std::ostringstream out;
        Debug{&out} << (Snap::InsideX|Snap::InsideY);
        CORRADE_COMPARE(out.str(), "Ui::Snap::Inside\n");

    /* NoSpace is NoSpaceX and NoSpaceY combined */
    } {
        std::ostringstream out;
        Debug{&out} << (Snap::NoSpaceX|Snap::NoSpaceY);
        CORRADE_COMPARE(out.str(), "Ui::Snap::NoSpace\n");
    }
}

void SnapLayouterTest::snapInside() {
    auto&& data = SnapInsideData[testCaseInstanceId()];
    {
        std::ostringstream out;
        Debug{&out, Debug::Flag::NoNewlineAtTheEnd|Debug::Flag::Packed} << data.snap;
        setTestCaseDescription(out.str());
    }

    CORRADE_COMPARE(Implementation::snapInside(data.snap), data.expected);
}

void SnapLayouterTest::snap() {
    auto&& data = SnapData[testCaseInstanceId()];
    {
        std::ostringstream out;
        Debug{&out, Debug::Flag::NoNewlineAtTheEnd|Debug::Flag::Packed} << data.snap;
        setTestCaseDescription(out.str());
    }

    Containers::Pair<Vector2, Vector2> out = Implementation::snap(data.snap,
        {100.0f, 200.0f}, {400.0f, 300.0f},
        /* Left, top, right, bottom */
        {10.0f, 5.0f, 15.0f, 25.0f},
        {7.0f, 3.0f},
        Size);

    CORRADE_COMPARE(out, Containers::pair(data.expectedOffset, data.expectedSize));
}

void SnapLayouterTest::orderLayoutsBreadthFirst() {
    /* Expands AbstractUserInterfaceImplementationTest::orderNodesBreadthFirst()
       with a subsequent mapping to layout IDs */

    /* The handle generations aren't used for anything here so can be
       arbitrary */
    const NodeHandle nodeParents[]{
        /* Forward parent reference */
        nodeHandle(11, 0x123),          /*  0 */
        /* Root elements */
        NodeHandle::Null,               /*  1 */
        NodeHandle::Null,               /*  2 */
        /* Backward parent reference */
        nodeHandle(1, 0xabc),           /*  3 */
        /* Deep hierarchy */
        nodeHandle(3, 0x1),             /*  4 */
        nodeHandle(3, 0x2),             /*  5, not referenced */
        nodeHandle(4, 0xfff),           /*  6 */
        /* Multiple children */
        nodeHandle(1, 0x1),             /*  7 */
        nodeHandle(10, 0x1),            /*  8 */
        nodeHandle(1, 0xeee),           /*  9, not referenced */
        nodeHandle(1, 0x1),             /* 10 */
        /* More root elements */
        NodeHandle::Null,               /* 11 */
        NodeHandle::Null,               /* 12, not referenced */
    };

    /* Important: the childrenOffsets array has to be zero-initialized. Others
       don't need to be. */
    UnsignedInt childrenOffsets[Containers::arraySize(nodeParents) + 2]{};
    UnsignedInt children[Containers::arraySize(nodeParents)];
    Int nodeIds[Containers::arraySize(nodeParents) + 1];
    Implementation::orderNodesBreadthFirstInto(
        nodeParents,
        childrenOffsets, children, nodeIds);
    CORRADE_COMPARE_AS(Containers::arrayView(nodeIds), Containers::arrayView({
        /* -1 is always first */
        -1,
        /* Root nodes first, in order as found */
        1,
        2,
        11,
        12,
        /* Then children of node 1, clustered together, in order as found */
        3,
        7,
        9,
        10,
        /* Then children of node 11 */
        0,
        /* Children of node 3 */
        4,
        5,
        /* Children of node 10 */
        8,
        /* Children of node 4 */
        6,
    }), TestSuite::Compare::Container);

    /* Now use that to order the masked layout IDs as well */
    UnsignedInt layoutIdsToUpdateData[]{0xffffffff};
    Containers::MutableBitArrayView layoutIdsToUpdate{layoutIdsToUpdateData, 0, 21};
    layoutIdsToUpdate.reset(2);
    layoutIdsToUpdate.reset(5);
    layoutIdsToUpdate.reset(6);
    layoutIdsToUpdate.reset(11);

    /* Again the handle generations aren't used for anything here so can be
       arbitrary */
    const NodeHandle layoutTargets[]{
        nodeHandle(8, 0xcec),           /*  0 */
        nodeHandle(3, 0xcec),           /*  1 */
        nodeHandle(0xfffff, 0xcec),     /*  2, skipped */
        nodeHandle(10, 0xcec),          /*  3 */
        nodeHandle(2, 0xcec),           /*  4 */
        nodeHandle(0xfffff, 0xcec),     /*  5, skipped */
        nodeHandle(0xfffff, 0xcec),     /*  6, skipped */
        nodeHandle(1, 0xcec),           /*  7 */
        nodeHandle(7, 0xcec),           /*  8 */
        NodeHandle::Null,               /*  9 */
        nodeHandle(2, 0xcec),           /* 10, same target as 4 */
        nodeHandle(0xfffff, 0xcec),     /* 11, skipped */
        nodeHandle(11, 0xcec),          /* 12 */
        NodeHandle::Null,               /* 13 */
        nodeHandle(0, 0xcec),           /* 14 */
        nodeHandle(6, 0xcec),           /* 15 */
        nodeHandle(3, 0xcec),           /* 16, same target as 1 */
        nodeHandle(3, 0xcec),           /* 17, same target as 1 */
        nodeHandle(8, 0xcec),           /* 18, same target as 0 */
        nodeHandle(4, 0xcec),           /* 19 */
        nodeHandle(6, 0xcec),           /* 20, same target as 15 */
    };

    /* Similarly here, the layoutOffsets array has to be zero-initialized */
    UnsignedInt layoutOffsets[Containers::arraySize(nodeParents) + 2]{};
    UnsignedInt layouts[Containers::arraySize(layoutTargets)];
    UnsignedInt layoutIds[Containers::arraySize(layoutTargets)];
    std::size_t count = Implementation::orderLayoutsBreadthFirstInto(
        layoutIdsToUpdate,
        layoutTargets,
        nodeIds,
        layoutOffsets,
        layouts,
        layoutIds);
    CORRADE_COMPARE_AS(Containers::arrayView(layoutIds).prefix(count), Containers::arrayView<UnsignedInt>({
        /* Layouts targeting the whole UI first, in order as found */
        9, 13,
        /* Layouts assigned to root nodes second, in order as found */
        7,          /* node 1 */
        4, 10,      /* node 2 */
        12,         /* node 11 */
        /* Then children of node 1, clustered together, in order as found */
        1, 16, 17,  /* node 3 */
        8,          /* node 7 */
        3,          /* node 10 */
        /* Then children of node 11 */
        14,         /* node 0 */
        /* Children of node 3 */
        19,         /* node 4 */
        /* Children of node 10 */
        0, 18,      /* node 8 */
        /* Children of node 4 */
        15, 20,     /* node 6 */
    }), TestSuite::Compare::Container);
}

void SnapLayouterTest::construct() {
    SnapLayouter layouter{layouterHandle(0xab, 0x12)};
    CORRADE_COMPARE(layouter.handle(), layouterHandle(0xab, 0x12));
    CORRADE_COMPARE(layouter.padding(), Vector4{});
    CORRADE_COMPARE(layouter.margin(), Vector2{});
}

void SnapLayouterTest::constructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<SnapLayouter>{});
    CORRADE_VERIFY(!std::is_copy_assignable<SnapLayouter>{});
}

void SnapLayouterTest::constructMove() {
    SnapLayouter a{layouterHandle(0xab, 0x12)};
    a.setPadding(1.0f);
    a.setMargin(3.0f);

    SnapLayouter b{Utility::move(a)};
    CORRADE_COMPARE(b.handle(), layouterHandle(0xab, 0x12));
    CORRADE_COMPARE(b.padding(), Vector4{1.0f});
    CORRADE_COMPARE(b.margin(), Vector2{3.0f});

    SnapLayouter c{layouterHandle(3, 5)};
    c = Utility::move(b);
    CORRADE_COMPARE(c.handle(), layouterHandle(0xab, 0x12));
    CORRADE_COMPARE(c.padding(), Vector4{1.0f});
    CORRADE_COMPARE(c.margin(), Vector2{3.0f});

    CORRADE_VERIFY(std::is_nothrow_move_constructible<SnapLayouter>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<SnapLayouter>::value);
}

void SnapLayouterTest::setPadding() {
    SnapLayouter layouter{layouterHandle(0, 1)};
    CORRADE_COMPARE(layouter.padding(), Vector4{});
    CORRADE_COMPARE(layouter.state(), LayouterStates{});

    /* Required to be called before update() (because AbstractUserInterface
       guarantees the same on a higher level), not needed for anything here */
    layouter.setSize({1, 1});

    /* Each side separately */
    layouter.setPadding({1.0f, 3.0f, 2.0f, 4.0f});
    CORRADE_COMPARE(layouter.padding(), (Vector4{1.0f, 3.0f, 2.0f, 4.0f}));
    CORRADE_COMPARE(layouter.state(), LayouterState::NeedsUpdate);

    /* Clear the state flags */
    layouter.update({}, {}, {}, {}, {});
    CORRADE_COMPARE(layouter.state(), LayouterStates{});

    /* Horizontal and vertical */
    layouter.setPadding({1.0f, 3.0f});
    CORRADE_COMPARE(layouter.padding(), (Vector4{1.0f, 3.0f, 1.0f, 3.0f}));
    CORRADE_COMPARE(layouter.state(), LayouterState::NeedsUpdate);

    /* Clear the state flags */
    layouter.update({}, {}, {}, {}, {});
    CORRADE_COMPARE(layouter.state(), LayouterStates{});

    /* All sides the same */
    layouter.setPadding(1.0f);
    CORRADE_COMPARE(layouter.padding(), (Vector4{1.0f}));
    CORRADE_COMPARE(layouter.state(), LayouterState::NeedsUpdate);
}

void SnapLayouterTest::setMargin() {
    SnapLayouter layouter{layouterHandle(0, 1)};
    CORRADE_COMPARE(layouter.margin(), Vector2{});
    CORRADE_COMPARE(layouter.state(), LayouterStates{});

    /* Required to be called before update() (because AbstractUserInterface
       guarantees the same on a higher level), not needed for anything here */
    layouter.setSize({1, 1});

    /* Horizontal and vertical separately */
    layouter.setMargin({2.0f, 4.0f});
    CORRADE_COMPARE(layouter.margin(), (Vector2{2.0f, 4.0f}));
    CORRADE_COMPARE(layouter.state(), LayouterState::NeedsUpdate);

    /* Clear the state flags */
    layouter.update({}, {}, {}, {}, {});
    CORRADE_COMPARE(layouter.state(), LayouterStates{});

    /* Both directions the same */
    layouter.setMargin(2.0f);
    CORRADE_COMPARE(layouter.margin(), (Vector2{2.0f}));
}

template<class> struct SnapLayoutTraits;
template<> struct SnapLayoutTraits<AbstractSnapLayout> {
    typedef AbstractUserInterface UserInterfaceType;
    typedef AbstractAnchor AnchorType;
    static const char* name() { return "AbstractSnapLayout"; }
};
template<> struct SnapLayoutTraits<SnapLayout> {
    typedef UserInterface UserInterfaceType;
    typedef Anchor AnchorType;
    static const char* name() { return "SnapLayout"; }
};

template<class T> void SnapLayouterTest::layoutConstructInside() {
    setTestCaseTemplateName(SnapLayoutTraits<T>::name());

    struct Interface: SnapLayoutTraits<T>::UserInterfaceType {
        explicit Interface(NoCreateT): SnapLayoutTraits<T>::UserInterfaceType{NoCreate} {}
    } ui{NoCreate};
    NodeHandle node = ui.createNode({}, {});

    /* The target is also a parent in this case */
    SnapLayouter layouter{layouterHandle(0, 1)};
    T layout{ui, layouter,
        Snap::Bottom|Snap::InsideY, node,
        Snap::Top|Snap::Left|Snap::Right};
    CORRADE_COMPARE(&layout.ui(), &ui);
    CORRADE_COMPARE(&layout.layouter(), &layouter);
    CORRADE_COMPARE(layout.parent(), node);
    CORRADE_COMPARE(layout.snapFirst(), Snap::Bottom|Snap::InsideY);
    CORRADE_COMPARE(layout.targetFirst(), node);
    CORRADE_COMPARE(layout.snapNext(), Snap::Top|Snap::Left|Snap::Right);
    CORRADE_COMPARE(layout.targetNext(), NodeHandle::Null);
}

template<class T> void SnapLayouterTest::layoutConstructOutside() {
    setTestCaseTemplateName(SnapLayoutTraits<T>::name());

    struct Interface: SnapLayoutTraits<T>::UserInterfaceType {
        explicit Interface(NoCreateT): SnapLayoutTraits<T>::UserInterfaceType{NoCreate} {}
    } ui{NoCreate};
    NodeHandle node = ui.createNode({}, {});
    NodeHandle sub = ui.createNode(node, {}, {});

    /* The target is a sibling in this case */
    SnapLayouter layouter{layouterHandle(0, 1)};
    T layout{ui, layouter,
        Snap::Bottom|Snap::NoSpaceY, sub,
        Snap::Top|Snap::Left|Snap::Right};
    CORRADE_COMPARE(&layout.ui(), &ui);
    CORRADE_COMPARE(&layout.layouter(), &layouter);
    CORRADE_COMPARE(layout.parent(), node);
    CORRADE_COMPARE(layout.snapFirst(), Snap::Bottom|Snap::NoSpaceY);
    CORRADE_COMPARE(layout.targetFirst(), sub);
    CORRADE_COMPARE(layout.snapNext(), Snap::Top|Snap::Left|Snap::Right);
    CORRADE_COMPARE(layout.targetNext(), NodeHandle::Null);
}

void SnapLayouterTest::layoutConstructDefaultLayouter() {
    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setSnapLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));
    NodeHandle node = ui.createNode({}, {});

    /* The target is also a parent in this case */
    SnapLayout layout{ui,
        Snap::Bottom|Snap::InsideY, node,
        Snap::Top|Snap::Left|Snap::Right};
    CORRADE_COMPARE(&layout.ui(), &ui);
    CORRADE_COMPARE(&layout.layouter(), &ui.snapLayouter());
    CORRADE_COMPARE(layout.parent(), node);
    CORRADE_COMPARE(layout.snapFirst(), Snap::Bottom|Snap::InsideY);
    CORRADE_COMPARE(layout.targetFirst(), node);
    CORRADE_COMPARE(layout.snapNext(), Snap::Top|Snap::Left|Snap::Right);
    CORRADE_COMPARE(layout.targetNext(), NodeHandle::Null);
}

template<class T> void SnapLayouterTest::layoutConstructCopy() {
    setTestCaseTemplateName(SnapLayoutTraits<T>::name());

    CORRADE_VERIFY(!std::is_copy_constructible<T>{});
    CORRADE_VERIFY(!std::is_copy_assignable<T>{});
}

template<class T> void SnapLayouterTest::layoutConstructMove() {
    setTestCaseTemplateName(SnapLayoutTraits<T>::name());

    struct Interface: SnapLayoutTraits<T>::UserInterfaceType {
        explicit Interface(NoCreateT): SnapLayoutTraits<T>::UserInterfaceType{NoCreate} {}
    } ui1{NoCreate}, ui2{NoCreate};
    NodeHandle node1 = ui1.createNode({}, {});
    NodeHandle node2 = ui2.createNode({}, {});

    SnapLayouter layouter1{layouterHandle(0, 1)};
    SnapLayouter layouter2{layouterHandle(3, 4)};

    T a{ui1, layouter1, Snap::Bottom|Snap::Inside, node1, Snap::Top};

    T b{Utility::move(a)};
    CORRADE_COMPARE(&b.ui(), &ui1);
    CORRADE_COMPARE(&b.layouter(), &layouter1);
    CORRADE_COMPARE(b.snapFirst(), Snap::Bottom|Snap::Inside);
    CORRADE_COMPARE(b.targetFirst(), node1);
    CORRADE_COMPARE(b.snapNext(), Snap::Top);

    T c{ui2, layouter2, {}, node2, {}};
    c = Utility::move(b);
    CORRADE_COMPARE(&c.ui(), &ui1);
    CORRADE_COMPARE(&c.layouter(), &layouter1);
    CORRADE_COMPARE(c.snapFirst(), Snap::Bottom|Snap::Inside);
    CORRADE_COMPARE(c.targetFirst(), node1);
    CORRADE_COMPARE(c.snapNext(), Snap::Top);

    CORRADE_VERIFY(std::is_nothrow_move_constructible<SnapLayout>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<SnapLayout>::value);
}

template<class T> void SnapLayouterTest::addRemove() {
    setTestCaseTemplateName(SnapLayoutTraits<T>::name());

    struct Interface: SnapLayoutTraits<T>::UserInterfaceType {
        explicit Interface(NoCreateT): SnapLayoutTraits<T>::UserInterfaceType{NoCreate} {}
    } ui{NoCreate};
    SnapLayouter& layouter = ui.setLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    NodeHandle node = ui.createNode({}, {});
    NodeHandle child = ui.createNode(node, {}, {});

    /* Snapping inside the node, thus it is also a parent */
    T snap1{ui, layouter, Snap::Left|Snap::InsideX, child, Snap::Right|Snap::NoSpaceX};
    CORRADE_COMPARE(snap1.parent(), child);
    CORRADE_COMPARE(snap1.targetFirst(), child);
    CORRADE_COMPARE(snap1.targetNext(), NodeHandle::Null);

    /* First gets snapped to the parent */
    typename SnapLayoutTraits<T>::AnchorType anchor1 = snap1({1.0f, 2.0f}, {3.0f, 4.0f}, NodeFlag::Disabled|NodeFlag::Focusable);
    CORRADE_COMPARE(&anchor1.ui(), &ui);
    CORRADE_COMPARE(anchor1, nodeHandle(2, 1));
    CORRADE_COMPARE(ui.nodeParent(anchor1), child);
    CORRADE_COMPARE(ui.nodeOffset(anchor1), (Vector2{1.0f, 2.0f}));
    CORRADE_COMPARE(ui.nodeSize(anchor1), (Vector2{3.0f, 4.0f}));
    CORRADE_COMPARE(ui.nodeFlags(anchor1), NodeFlag::Disabled|NodeFlag::Focusable);
    CORRADE_COMPARE(anchor1, layoutHandle(layouter.handle(), 0, 1));
    CORRADE_COMPARE(layouter.snap(anchor1), Snap::Left|Snap::InsideX);
    CORRADE_COMPARE(layouter.target(anchor1), child);
    CORRADE_COMPARE(snap1.targetNext(), anchor1);

    /* Second to the first. Testing the overload with implicit offset. */
    typename SnapLayoutTraits<T>::AnchorType anchor2 = snap1({5.0f, 6.0f}, NodeFlag::NoEvents);
    CORRADE_COMPARE(&anchor2.ui(), &ui);
    CORRADE_COMPARE(anchor2, nodeHandle(3, 1));
    CORRADE_COMPARE(ui.nodeParent(anchor2), child);
    CORRADE_COMPARE(ui.nodeOffset(anchor2), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(anchor2), (Vector2{5.0f, 6.0f}));
    CORRADE_COMPARE(ui.nodeFlags(anchor2), NodeFlag::NoEvents);
    CORRADE_COMPARE(anchor2, layoutHandle(layouter.handle(), 1, 1));
    CORRADE_COMPARE(layouter.snap(anchor2), Snap::Right|Snap::NoSpaceX);
    CORRADE_COMPARE(layouter.target(anchor2), anchor1);
    CORRADE_COMPARE(snap1.targetNext(), anchor2);

    /* Third to the second. Omitting the flags. */
    typename SnapLayoutTraits<T>::AnchorType anchor3 = snap1({7.0f, 8.0f}, {9.0f, 10.0f});
    CORRADE_COMPARE(&anchor3.ui(), &ui);
    CORRADE_COMPARE(anchor3, nodeHandle(4, 1));
    CORRADE_COMPARE(ui.nodeParent(anchor3), child);
    CORRADE_COMPARE(ui.nodeOffset(anchor3), (Vector2{7.0f, 8.0f}));
    CORRADE_COMPARE(ui.nodeSize(anchor3), (Vector2{9.0f, 10.0f}));
    CORRADE_COMPARE(ui.nodeFlags(anchor3), NodeFlags{});
    CORRADE_COMPARE(anchor3, layoutHandle(layouter.handle(), 2, 1));
    CORRADE_COMPARE(layouter.snap(anchor3), Snap::Right|Snap::NoSpaceX);
    CORRADE_COMPARE(layouter.target(anchor3), anchor2);
    CORRADE_COMPARE(snap1.targetNext(), anchor3);

    /* Snapping outside of the node, thus it's a sibling */
    T snap2{ui, layouter, Snap::Left|Snap::Top, child, Snap::Bottom};
    CORRADE_COMPARE(snap2.parent(), node);
    CORRADE_COMPARE(snap2.targetFirst(), child);
    CORRADE_COMPARE(snap2.targetNext(), NodeHandle::Null);

    /* First gets snapped to the target. Querying with the LayouterDataHandle
       overloads. */
    typename SnapLayoutTraits<T>::AnchorType anchor4 = snap2({2.0f, 1.0f}, {4.0f, 3.0f}, NodeFlag::Focusable);
    CORRADE_COMPARE(&anchor4.ui(), &ui);
    CORRADE_COMPARE(anchor4, nodeHandle(5, 1));
    CORRADE_COMPARE(ui.nodeParent(anchor4), node);
    CORRADE_COMPARE(ui.nodeOffset(anchor4), (Vector2{2.0f, 1.0f}));
    CORRADE_COMPARE(ui.nodeSize(anchor4), (Vector2{4.0f, 3.0f}));
    CORRADE_COMPARE(ui.nodeFlags(anchor4), NodeFlag::Focusable);
    CORRADE_COMPARE(anchor4, layoutHandle(layouter.handle(), 3, 1));
    CORRADE_COMPARE(layouter.snap(layoutHandleData(anchor4)), Snap::Left|Snap::Top);
    CORRADE_COMPARE(layouter.target(layoutHandleData(anchor4)), child);
    CORRADE_COMPARE(snap2.targetNext(), anchor4);

    /* Second gets snapped to the first. Omitting both offset and flags. */
    typename SnapLayoutTraits<T>::AnchorType anchor5 = snap2({11.0f, 12.0f});
    CORRADE_COMPARE(&anchor5.ui(), &ui);
    CORRADE_COMPARE(anchor5, nodeHandle(6, 1));
    CORRADE_COMPARE(ui.nodeParent(anchor5), node);
    CORRADE_COMPARE(ui.nodeOffset(anchor5), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(anchor5), (Vector2{11.0f, 12.0f}));
    CORRADE_COMPARE(ui.nodeFlags(anchor5), NodeFlags{});
    CORRADE_COMPARE(anchor5, layoutHandle(layouter.handle(), 4, 1));
    CORRADE_COMPARE(layouter.snap(anchor5), Snap::Bottom);
    CORRADE_COMPARE(layouter.target(anchor5), anchor4);
    CORRADE_COMPARE(snap2.targetNext(), anchor5);

    /* Snapping a single layout inside of the node, thus a child */
    typename SnapLayoutTraits<T>::AnchorType anchor6 = Ui::snap(ui, layouter, Snap::Bottom|Snap::InsideY, child, {13.0f, 14.0f}, {15.0f, 16.0f}, NodeFlag::NoEvents|NodeFlag::Clip);
    CORRADE_COMPARE(&anchor6.ui(), &ui);
    CORRADE_COMPARE(anchor6, nodeHandle(7, 1));
    CORRADE_COMPARE(ui.nodeParent(anchor6), child);
    CORRADE_COMPARE(ui.nodeOffset(anchor6), (Vector2{13.0f, 14.0f}));
    CORRADE_COMPARE(ui.nodeSize(anchor6), (Vector2{15.0f, 16.0f}));
    CORRADE_COMPARE(ui.nodeFlags(anchor6), NodeFlag::NoEvents|NodeFlag::Clip);
    CORRADE_COMPARE(anchor6, layoutHandle(layouter.handle(), 5, 1));
    CORRADE_COMPARE(layouter.snap(anchor6), Snap::Bottom|Snap::InsideY);
    CORRADE_COMPARE(layouter.target(anchor6), child);

    /* Snapping a single layout outside of the node, thus a sibling. Omitting
       the offset. */
    typename SnapLayoutTraits<T>::AnchorType anchor7 = Ui::snap(ui, layouter, Snap::Right|Snap::InsideY, child, {17.0f, 18.0f}, NodeFlag::Hidden);
    CORRADE_COMPARE(&anchor7.ui(), &ui);
    CORRADE_COMPARE(anchor7, nodeHandle(8, 1));
    CORRADE_COMPARE(ui.nodeParent(anchor7), node);
    CORRADE_COMPARE(ui.nodeOffset(anchor7), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(anchor7), (Vector2{17.0f, 18.0f}));
    CORRADE_COMPARE(ui.nodeFlags(anchor7), NodeFlag::Hidden);
    CORRADE_COMPARE(anchor7, layoutHandle(layouter.handle(), 6, 1));
    CORRADE_COMPARE(layouter.snap(anchor7), Snap::Right|Snap::InsideY);
    CORRADE_COMPARE(layouter.target(anchor7), child);

    /* Snapping a single layout outside of a root node, thus also a root node.
       Omitting the flags. */
    typename SnapLayoutTraits<T>::AnchorType anchor8 = Ui::snap(ui, layouter, Snap::Top|Snap::InsideX, node, {19.0f, 20.0f}, {21.0f, 22.0f});
    CORRADE_COMPARE(&anchor8.ui(), &ui);
    CORRADE_COMPARE(anchor8, nodeHandle(9, 1));
    CORRADE_COMPARE(ui.nodeParent(anchor8), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOffset(anchor8), (Vector2{19.0f, 20.0f}));
    CORRADE_COMPARE(ui.nodeSize(anchor8), (Vector2{21.0f, 22.0f}));
    CORRADE_COMPARE(ui.nodeFlags(anchor8), NodeFlags{});
    CORRADE_COMPARE(anchor8, layoutHandle(layouter.handle(), 7, 1));
    CORRADE_COMPARE(layouter.snap(anchor8), Snap::Top|Snap::InsideX);
    CORRADE_COMPARE(layouter.target(anchor8), node);

    /* Snapping a single layout to the UI itself, thus being implicitly
       inside. Variant with the null parent explicit and implicit. */
    typename SnapLayoutTraits<T>::AnchorType anchor9a = Ui::snap(ui, layouter, Snap::Left|Snap::Bottom, NodeHandle::Null, {23.0f, 24.0f}, {25.0f, 26.0f}, NodeFlag::Clip);
    typename SnapLayoutTraits<T>::AnchorType anchor9b = Ui::snap(ui, layouter, Snap::Left|Snap::Bottom, {23.0f, 24.0f}, {25.0f, 26.0f}, NodeFlag::Clip);
    CORRADE_COMPARE(&anchor9a.ui(), &ui);
    CORRADE_COMPARE(&anchor9b.ui(), &ui);
    CORRADE_COMPARE(anchor9a, nodeHandle(10, 1));
    CORRADE_COMPARE(anchor9b, nodeHandle(11, 1));
    CORRADE_COMPARE(ui.nodeParent(anchor9a), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeParent(anchor9b), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOffset(anchor9a), (Vector2{23.0f, 24.0f}));
    CORRADE_COMPARE(ui.nodeOffset(anchor9b), (Vector2{23.0f, 24.0f}));
    CORRADE_COMPARE(ui.nodeSize(anchor9a), (Vector2{25.0f, 26.0f}));
    CORRADE_COMPARE(ui.nodeSize(anchor9b), (Vector2{25.0f, 26.0f}));
    CORRADE_COMPARE(ui.nodeFlags(anchor9a), NodeFlag::Clip);
    CORRADE_COMPARE(ui.nodeFlags(anchor9b), NodeFlag::Clip);
    CORRADE_COMPARE(anchor9a, layoutHandle(layouter.handle(), 8, 1));
    CORRADE_COMPARE(anchor9b, layoutHandle(layouter.handle(), 9, 1));
    CORRADE_COMPARE(layouter.snap(anchor9a), Snap::Left|Snap::Bottom);
    CORRADE_COMPARE(layouter.snap(anchor9b), Snap::Left|Snap::Bottom);
    CORRADE_COMPARE(layouter.target(anchor9a), NodeHandle::Null);
    CORRADE_COMPARE(layouter.target(anchor9b), NodeHandle::Null);

    /* Snapping a single layout to the UI itself with offset omitted, again a
       variant with the null parent explicit and implicit */
    typename SnapLayoutTraits<T>::AnchorType anchor10a = Ui::snap(ui, layouter, Snap::Bottom|Snap::NoSpace, NodeHandle::Null, {27.0f, 28.0f}, NodeFlag::Focusable);
    typename SnapLayoutTraits<T>::AnchorType anchor10b = Ui::snap(ui, layouter, Snap::Bottom|Snap::NoSpace, {27.0f, 28.0f}, NodeFlag::Focusable);
    CORRADE_COMPARE(&anchor10a.ui(), &ui);
    CORRADE_COMPARE(&anchor10b.ui(), &ui);
    CORRADE_COMPARE(anchor10a, nodeHandle(12, 1));
    CORRADE_COMPARE(anchor10b, nodeHandle(13, 1));
    CORRADE_COMPARE(ui.nodeParent(anchor10a), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeParent(anchor10b), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOffset(anchor10a), Vector2{});
    CORRADE_COMPARE(ui.nodeOffset(anchor10b), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(anchor10a), (Vector2{27.0f, 28.0f}));
    CORRADE_COMPARE(ui.nodeSize(anchor10b), (Vector2{27.0f, 28.0f}));
    CORRADE_COMPARE(ui.nodeFlags(anchor10a), NodeFlag::Focusable);
    CORRADE_COMPARE(ui.nodeFlags(anchor10b), NodeFlag::Focusable);
    CORRADE_COMPARE(anchor10a, layoutHandle(layouter.handle(), 10, 1));
    CORRADE_COMPARE(anchor10b, layoutHandle(layouter.handle(), 11, 1));
    CORRADE_COMPARE(layouter.snap(anchor10a), Snap::Bottom|Snap::NoSpace);
    CORRADE_COMPARE(layouter.snap(anchor10b), Snap::Bottom|Snap::NoSpace);
    CORRADE_COMPARE(layouter.target(anchor10a), NodeHandle::Null);
    CORRADE_COMPARE(layouter.target(anchor10b), NodeHandle::Null);

    /* Removing a layout just delegates to the base implementation, nothing
       else needs to be cleaned up */
    layouter.remove(anchor6);
    layouter.remove(layoutHandleData(anchor9b));
    CORRADE_VERIFY(!layouter.isHandleValid(anchor6));
    CORRADE_VERIFY(!layouter.isHandleValid(anchor9b));
}

void SnapLayouterTest::addDefaultLayouter() {
    /* Subset of addRemove() testing just the snap() overloads that take the
       implicit layouter instance */

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setSnapLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    NodeHandle node = ui.createNode({}, {});
    NodeHandle child = ui.createNode(node, {}, {});

    /* Full signature */
    Anchor anchor1 = Ui::snap(ui, Snap::Bottom|Snap::InsideY, child, {13.0f, 14.0f}, {15.0f, 16.0f}, NodeFlag::NoEvents|NodeFlag::Clip);
    CORRADE_COMPARE(&anchor1.ui(), &ui);
    CORRADE_COMPARE(anchor1, nodeHandle(2, 1));
    CORRADE_COMPARE(ui.nodeParent(anchor1), child);
    CORRADE_COMPARE(ui.nodeOffset(anchor1), (Vector2{13.0f, 14.0f}));
    CORRADE_COMPARE(ui.nodeSize(anchor1), (Vector2{15.0f, 16.0f}));
    CORRADE_COMPARE(ui.nodeFlags(anchor1), NodeFlag::NoEvents|NodeFlag::Clip);
    CORRADE_COMPARE(anchor1, layoutHandle(ui.snapLayouter().handle(), 0, 1));
    CORRADE_COMPARE(ui.snapLayouter().snap(anchor1), Snap::Bottom|Snap::InsideY);
    CORRADE_COMPARE(ui.snapLayouter().target(anchor1), child);

    /* With offset omitted */
    Anchor anchor2 = Ui::snap(ui, Snap::Right|Snap::InsideY, child, {17.0f, 18.0f}, NodeFlag::Hidden);
    CORRADE_COMPARE(&anchor2.ui(), &ui);
    CORRADE_COMPARE(anchor2, nodeHandle(3, 1));
    CORRADE_COMPARE(ui.nodeParent(anchor2), node);
    CORRADE_COMPARE(ui.nodeOffset(anchor2), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(anchor2), (Vector2{17.0f, 18.0f}));
    CORRADE_COMPARE(ui.nodeFlags(anchor2), NodeFlag::Hidden);
    CORRADE_COMPARE(anchor2, layoutHandle(ui.snapLayouter().handle(), 1, 1));
    CORRADE_COMPARE(ui.snapLayouter().snap(anchor2), Snap::Right|Snap::InsideY);
    CORRADE_COMPARE(ui.snapLayouter().target(anchor2), child);

    /* With implicit null parent */
    Anchor anchor3 = Ui::snap(ui, Snap::Left|Snap::Bottom, {23.0f, 24.0f}, {25.0f, 26.0f}, NodeFlag::Clip);
    CORRADE_COMPARE(&anchor3.ui(), &ui);
    CORRADE_COMPARE(anchor3, nodeHandle(4, 1));
    CORRADE_COMPARE(ui.nodeParent(anchor3), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOffset(anchor3), (Vector2{23.0f, 24.0f}));
    CORRADE_COMPARE(ui.nodeSize(anchor3), (Vector2{25.0f, 26.0f}));
    CORRADE_COMPARE(ui.nodeFlags(anchor3), NodeFlag::Clip);
    CORRADE_COMPARE(anchor3, layoutHandle(ui.snapLayouter().handle(), 2, 1));
    CORRADE_COMPARE(ui.snapLayouter().snap(anchor3), Snap::Left|Snap::Bottom);
    CORRADE_COMPARE(ui.snapLayouter().target(anchor3), NodeHandle::Null);

    /* With implicit null parent and offset omitted */
    Anchor anchor4 = Ui::snap(ui, Snap::Bottom|Snap::NoSpace, {27.0f, 28.0f}, NodeFlag::Focusable);
    CORRADE_COMPARE(&anchor4.ui(), &ui);
    CORRADE_COMPARE(anchor4, nodeHandle(5, 1));
    CORRADE_COMPARE(ui.nodeParent(anchor4), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOffset(anchor4), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(anchor4), (Vector2{27.0f, 28.0f}));
    CORRADE_COMPARE(ui.nodeFlags(anchor4), NodeFlag::Focusable);
    CORRADE_COMPARE(anchor4, layoutHandle(ui.snapLayouter().handle(), 3, 1));
    CORRADE_COMPARE(ui.snapLayouter().snap(anchor4), Snap::Bottom|Snap::NoSpace);
    CORRADE_COMPARE(ui.snapLayouter().target(anchor4), NodeHandle::Null);
}

void SnapLayouterTest::addRemoveHandleRecycle() {
    struct Interface: AbstractUserInterface {
        explicit Interface(NoCreateT): AbstractUserInterface{NoCreate} {}
    } ui{NoCreate};
    SnapLayouter& layouter = ui.setLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    NodeHandle node = ui.createNode({}, {});
    NodeHandle child = ui.createNode(node, {}, {});
    /*LayoutHandle first =*/ Ui::snap(ui, layouter, Snap::Bottom|Snap::Inside, node, {0.0f, 1.0f});
    LayoutHandle second = Ui::snap(ui, layouter, Snap::Right|Snap::NoSpaceX, child, {2.0f, 3.0f});

    /* Layout that reuses a previous slot should have the snap and target
       cleared even if having them empty / null */
    layouter.remove(second);
    LayoutHandle second2 = Ui::snap(ui, layouter, {}, {2.0f, 3.0f});
    CORRADE_COMPARE(layoutHandleId(second2), layoutHandleId(second));
    CORRADE_COMPARE(layouter.target(second2), NodeHandle::Null);
    CORRADE_COMPARE(layouter.snap(second2), Snaps{});
}

void SnapLayouterTest::layoutInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    SnapLayouter layouter{layouterHandle(0, 1)};

    struct Interface: AbstractUserInterface {
        explicit Interface(NoCreateT): AbstractUserInterface{NoCreate} {}
    } ui{NoCreate};

    NodeHandle node = ui.createNode({}, {});

    std::ostringstream out;
    Error redirectError{&out};
    AbstractSnapLayout{ui, layouter, {}, NodeHandle::Null, {}};
    AbstractSnapLayout{ui, layouter, {}, nodeHandle(0x12345, 0xabc), {}};
    AbstractSnapLayout{ui, layouter, Snap::Right|Snap::Bottom|Snap::InsideY, node, {}};
    Ui::snap(ui, layouter, {}, nodeHandle(0x12345, 0xabc), {});
    CORRADE_COMPARE_AS(out.str(),
        "Ui::AbstractSnapLayout: invalid target handle Ui::NodeHandle::Null\n"
        "Ui::AbstractSnapLayout: invalid target handle Ui::NodeHandle(0x12345, 0xabc)\n"
        "Ui::AbstractSnapLayout: target cannot be a root node for Ui::Snap::BottomRight|Ui::Snap::InsideY\n"
        "Ui::snap(): invalid target handle Ui::NodeHandle(0x12345, 0xabc)\n",
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

void SnapLayouterTest::invalidHandle() {
    CORRADE_SKIP_IF_NO_ASSERT();

    SnapLayouter layouter{layouterHandle(0, 1)};

    std::ostringstream out;
    Error redirectError{&out};
    layouter.snap(LayoutHandle::Null);
    layouter.snap(LayouterDataHandle::Null);
    layouter.target(LayoutHandle::Null);
    layouter.target(LayouterDataHandle::Null);
    CORRADE_COMPARE_AS(out.str(),
        "Ui::SnapLayouter::snap(): invalid handle Ui::LayoutHandle::Null\n"
        "Ui::SnapLayouter::snap(): invalid handle Ui::LayouterDataHandle::Null\n"
        "Ui::SnapLayouter::target(): invalid handle Ui::LayoutHandle::Null\n"
        "Ui::SnapLayouter::target(): invalid handle Ui::LayouterDataHandle::Null\n",
        TestSuite::Compare::String);
}

void SnapLayouterTest::updateEmpty() {
    SnapLayouter layouter{layouterHandle(0, 1)};

    /* Required to be called before update() (because AbstractUserInterface
       guarantees the same on a higher level), not needed for anything here */
    layouter.setSize({1, 1});

    /* It shouldn't crash or do anything weird */
    layouter.update({}, {}, {}, {}, {});
    CORRADE_VERIFY(true);
}

void SnapLayouterTest::updateDataOrder() {
    auto&& data = UpdateDataOrderData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct Interface: AbstractUserInterface {
        explicit Interface(NoCreateT): AbstractUserInterface{NoCreate} {}
    } ui{NoCreate};
    SnapLayouter& layouter = ui.setLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    if(data.recycledLayouts) {
        AbstractAnchor layout1 = Ui::snap(ui, layouter, {}, {});
        AbstractAnchor layout2 = Ui::snap(ui, layouter, {}, {});
        AbstractAnchor layout3 = Ui::snap(ui, layouter, {}, {});
        AbstractAnchor layout4 = Ui::snap(ui, layouter, {}, {});
        AbstractAnchor layout5 = Ui::snap(ui, layouter, {}, {});
        layouter.remove(layout4);
        layouter.remove(layout5);
        layouter.remove(layout3);
        layouter.remove(layout1);
        layouter.remove(layout2);
        /* Recycle the nodes in the order they were created so they retain the
           ID order when created again below, in order to not need to shuffle
           them in the checks in doUpdate() */
        ui.removeNode(layout1);
        ui.removeNode(layout2);
        ui.removeNode(layout3);
        ui.removeNode(layout4);
        ui.removeNode(layout5);
    }

    if(!data.setMarginPaddingLater) {
        layouter.setMargin({3.0f, 2.0f});
        layouter.setPadding({1.0f, 4.0f, 6.0f, 5.0f});
    }

    /* A layout snapped to the whole UI, with Snap::Inside being implicit */
    AbstractAnchor layout1 = Ui::snap(ui, layouter, Snap::Bottom|Snap::Right, {70.0f, 90.0f});
    CORRADE_COMPARE(ui.nodeParent(layout1), NodeHandle::Null);

    /* A layout snapped outside of a (non-layouted) node, inheriting its offset
       in addition to having its own offset preserved */
    NodeHandle nodeRoot = ui.createNode({10.0f, 40.0f}, {100.0f, 200.0f});
    NodeHandle nodeChild = ui.createNode(nodeRoot, {30.0f, 20.0f}, {50.0f, 150.0f});
    AbstractAnchor layout2 = Ui::snap(ui, layouter, Snap::Left|Snap::Right|Snap::Top|Snap::NoSpaceY, nodeChild, {0.3f, -0.2f}, {0.0f, 25.0f});
    CORRADE_COMPARE(ui.nodeParent(layout2), nodeRoot);

    /* A layout snapped inside of a (non-layouted) node, not inheriting its
       offset but having its own offset preserved */
    AbstractAnchor layout3 = Ui::snap(ui, layouter, Snap::Top|Snap::Bottom|Snap::Right|Snap::Inside|Snap::NoSpaceX, nodeRoot, {0.9f, 0.6f}, {10.0f, 0.0f});
    CORRADE_COMPARE(ui.nodeParent(layout3), nodeRoot);

    /* A layout relative to layouted node with an offset, should inerit that
       offset in addition to its own, and match its Y size */
    AbstractAnchor layout4 = Ui::snap(ui, layouter, Snap::Top|Snap::Bottom|Snap::Left, layout3, {0.2f, -0.5f}, {20.0f, 0.0f});
    CORRADE_COMPARE(ui.nodeParent(layout4), nodeRoot);

    /* A layout that's further dependent on previous, match its XY size */
    AbstractAnchor layout5 = Ui::snap(ui, layouter, Snap::Top|Snap::Bottom|Snap::Left|Snap::Right, layout4, {0.02f, -0.05f}, Vector2{});
    CORRADE_COMPARE(ui.nodeParent(layout5), layout4);

    /* The padding should be taken into account even if set later */
    if(data.setMarginPaddingLater) {
        layouter.setMargin({3.0f, 2.0f});
        /* left, top, right, bottom */
        layouter.setPadding({1.0f, 4.0f, 6.0f, 5.0f});
    }

    /* The size also */
    ui.setSize({500, 400});

    /* Add a dummy second layouter because that's the easiest way verify the
       calculated node offsets / sizes */
    struct DummyLayouter: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::add;

        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>& nodeOffsets, const Containers::StridedArrayView1D<Vector2>& nodeSizes) override {
            CORRADE_COMPARE_AS(nodeOffsets, Containers::stridedArrayView<Vector2>({
                /* (6, 5) is right and bottom padding */
                {500.0f - 70.0f - 6.0f, 400.0f - 90.0f - 5.0f}, /* layout1 */
                {10.0f, 40.0f},                                 /* nodeRoot */
                {30.0f, 20.0f},                                 /* nodeChild */
                /* Snapped outside so no X margin / padding, Y spacing
                   disabled, (0.3, -0.2) is the node offset */
                {30.0f + 0.3f, 20.0f - 0.2f - 25.0f},           /* layout2 */
                /* 4 is top padding, X padding disabled, (0.9, 0.6) is the
                   node offset */
                {100.0f - 10.0f + 0.9f, 4.0f + 0.6f},           /* layout3 */
                /* In addition to what's in layout3 3 is horizontal margin,
                   (0.2, -0.5) is node offset */
                {100.0f - 10.0f + 0.9f - 20.0f - 3.0f + 0.2f,
                 4.0f + 0.6f - 0.5f},                           /* layout4 */
                 /* (1, 4) is padding and (0.02, -0.05) is node offset,
                    relative to layout4 so layout4's offset isn't included */
                {1.0f + 0.02f, 4.0f - 0.05f},                   /* layout5 */
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(nodeSizes, Containers::stridedArrayView<Vector2>({
                {70.0f, 90.0f},                                 /* layout1 */
                {100.0f, 200.0f},                               /* nodeRoot */
                {50.0f, 150.0f},                                /* nodeChild */
                {50.0f, 25.0f},                                 /* layout2 */
                /* Y size matches nodeRoot height minus top/bottom padding */
                {10.0f, 200.0f - 4.0f - 5.0f},                  /* layout3 */
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
    dummyLayouter.add(layout5);
    ui.update();
    CORRADE_COMPARE(dummyLayouter.called, 1);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::SnapLayouterTest)
