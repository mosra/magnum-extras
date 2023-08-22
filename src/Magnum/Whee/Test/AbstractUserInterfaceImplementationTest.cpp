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

#include <Corrade/Containers/BitArray.h>
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/Pair.h>
#include <Corrade/Containers/StridedBitArrayView.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Container.h>
#include <Corrade/TestSuite/Compare/Numeric.h>
#include <Magnum/Math/Vector2.h>

#include "Magnum/Whee/AbstractLayer.h" /* LayerFeatures */
#include "Magnum/Whee/Handle.h"
#include "Magnum/Whee/Implementation/abstractUserInterface.h"

namespace Magnum { namespace Whee { namespace Test { namespace {

struct AbstractUserInterfaceImplementationTest: TestSuite::Tester {
    explicit AbstractUserInterfaceImplementationTest();

    void orderNodesBreadthFirst();

    void orderVisibleNodesDepthFirst();
    void orderVisibleNodesDepthFirstSingleBranch();
    void orderVisibleNodesDepthFirstNoTopLevelNodes();

    void visibleTopLevelNodeIndices();

    void cullVisibleNodesClipRects();
    void cullVisibleNodesEdges();
    void cullVisibleNodes();
    void cullVisibleNodesNoTopLevelNodes();

    void orderVisibleNodeData();
    void orderVisibleNodeDataNoTopLevelNodes();

    void orderNodeDataForEventHandling();

    void compactDraws();
};

const struct {
    TestSuite::TestCaseDescriptionSourceLocation name;
    Containers::Array<Containers::Pair<UnsignedInt, UnsignedInt>> nodeIdsChildrenCount;
    Containers::Array<Containers::Triple<Vector2, Vector2, NodeFlags>> nodeOffsetsSizesFlags;
    Containers::Array<bool> expectedVisible;
    Containers::Array<Containers::Triple<Vector2, Vector2, UnsignedInt>> expectedClipRects;
} CullVisibleNodesClipRectsData[]{
    {"single non-clipping node", {InPlaceInit, {
            {0, 0},
        }}, {InPlaceInit, {
            {{3.0f, 4.0f}, {5.0f, 6.0f}, {}},
        }}, {InPlaceInit, {
            true
        }}, {InPlaceInit, {
            /* Verifies that no OOB access happens internally */
            {{}, {}, 1}
        }}},
    {"single clipping node", {InPlaceInit, {
            {0, 0},
        }}, {InPlaceInit, {
            {{3.0f, 4.0f}, {5.0f, 6.0f}, NodeFlag::Clip},
        }}, {InPlaceInit, {
            true
        }}, {InPlaceInit, {
            /* Verifies that no OOB access happens internally here as well */
            {{3.0f, 4.0f}, {5.0f, 6.0f}, 1}
        }}},
    {"multiple non-clipping top-level nodes", {InPlaceInit, {
            {0, 0},
            {2, 0},
            {3, 0}, /* clips */
            {1, 0},
            {4, 0},
        }}, {InPlaceInit, {
            {{0.0f, 1.0f}, {2.0f, 3.0f}, {}},
            {{3.0f, 4.0f}, {5.0f, 6.0f}, {}},
            {{6.0f, 7.0f}, {8.0f, 9.0f}, {}},
            {{0.0f, 1.0f}, {2.0f, 3.0f}, NodeFlag::Clip},
            {{3.0f, 4.0f}, {5.0f, 6.0f}, {}},
        }}, {InPlaceInit, {
            true, true, true, true, true
        }}, {InPlaceInit, {
            /* These shouldn't get merged together as they are separate draw
               calls as well */
            {{}, {}, 1},
            {{}, {}, 1},
            {{0.0f, 1.0f}, {2.0f, 3.0f}, 1},
            {{}, {}, 1},
            {{}, {}, 1}
        }}},
    {"skip a fully culled clipping node including children", {InPlaceInit, {
            {2, 3},         /* clips */
                {3, 2},     /* culled, clips */
                    {0, 0}, /* culled */
                    {1, 0}, /* culled */
        }}, {InPlaceInit, {
            /*  1   2 3 4   5 6   7 8
              1       +-------------+
              2 +---+ | +---+ +---+ |
                | 2 | | | 0 | | 1 | |
              3 +---+ | +---+ +---+ |
              4       +-------------+ */
            {{4.0f, 2.0f}, {1.0f, 1.0f}, {}},             /* 0 */
            {{6.0f, 2.0f}, {1.0f, 1.0f}, {}},             /* 1 */
            {{1.0f, 2.0f}, {1.0f, 1.0f}, NodeFlag::Clip}, /* 2 */
            {{3.0f, 1.0f}, {5.0f, 3.0f}, NodeFlag::Clip}, /* 3 */
        }}, {InPlaceInit, {
            false, false, true, false
        }}, {InPlaceInit, {
            {{1.0f, 2.0f}, {1.0f, 1.0f}, 4},
        }}},
    {"return to parent clip rect", {InPlaceInit, {
            {2, 3},     /* clips */
                {3, 0},
                {0, 0}, /* clips */
                {1, 0},
        }}, {InPlaceInit, {
            /*  1 2   3 4   5 6   7 8
              1 +-------------------+
              2 | +---+ +---+ +---+ |
                | | 3 | | 0 | | 1 | |
              3 | +---+ +---+ +---+ |
              4 +-------------------+ */
            {{4.0f, 2.0f}, {1.0f, 1.0f}, NodeFlag::Clip}, /* 0 */
            {{6.0f, 2.0f}, {1.0f, 1.0f}, {}},             /* 1 */
            {{1.0f, 1.0f}, {7.0f, 3.0f}, NodeFlag::Clip}, /* 2 */
            {{2.0f, 2.0f}, {1.0f, 1.0f}, {}},             /* 3 */
        }}, {InPlaceInit, {
            true, true, true, true
        }}, {InPlaceInit, {
            {{1.0f, 1.0f}, {7.0f, 3.0f}, 2},
            {{4.0f, 2.0f}, {1.0f, 1.0f}, 1},
            /* Same as the first clip rect */
            {{1.0f, 1.0f}, {7.0f, 3.0f}, 1},
        }}},
    {"return to parent clip rect, invisible node", {InPlaceInit, {
            {2, 3},     /* clips */
                {3, 0},
                {0, 0}, /* clips */
                {1, 0}, /* culled */
            {4, 0}
        }}, {InPlaceInit, {
            /*  1 2   3 4   5 6   7 8   9
              1 +------------+
              2 | +---+ +---+|+---+ +---+
                | | 3 | | 0 ||| 1 | | 4 |
              3 | +---+ +---+|+---+ +---+
              4 +------------+            */
            {{4.0f, 2.0f}, {1.0f, 1.0f}, NodeFlag::Clip}, /* 0 */
            {{6.0f, 2.0f}, {1.0f, 1.0f}, {}},             /* 1 */
            {{1.0f, 1.0f}, {4.5f, 3.0f}, NodeFlag::Clip}, /* 2 */
            {{2.0f, 2.0f}, {1.0f, 1.0f}, {}},             /* 3 */
            {{8.0f, 2.0f}, {1.0f, 1.0f}, {}},             /* 4 */
        }}, {InPlaceInit, {
            true, false, true, true, true
        }}, {InPlaceInit, {
            {{1.0f, 1.0f}, {4.5f, 3.0f}, 2},
            {{4.0f, 2.0f}, {1.0f, 1.0f}, 1},
            {{1.0f, 1.0f}, {4.5f, 3.0f}, 1}, /* node 1 is invisible */
            {{}, {}, 1}
        }}},
    {"return to parent clip rect, invisible node at the end", {InPlaceInit, {
            {2, 3},     /* clips */
                {3, 0},
                {0, 0}, /* clips */
                {1, 0}, /* culled */
        }}, {InPlaceInit, {
            /*  1 2   3 4   5 6   7
              1 +------------+
              2 | +---+ +---+|+---+
                | | 3 | | 0 ||| 1 |
              3 | +---+ +---+|+---+
              4 +------------+      */
            {{4.0f, 2.0f}, {1.0f, 1.0f}, NodeFlag::Clip}, /* 0 */
            {{6.0f, 2.0f}, {1.0f, 1.0f}, {}},             /* 1 */
            {{1.0f, 1.0f}, {4.5f, 3.0f}, NodeFlag::Clip}, /* 2 */
            {{2.0f, 2.0f}, {1.0f, 1.0f}, {}},             /* 3 */
        }}, {InPlaceInit, {
            true, false, true, true
        }}, {InPlaceInit, {
            {{1.0f, 1.0f}, {4.5f, 3.0f}, 2},
            {{4.0f, 2.0f}, {1.0f, 1.0f}, 1},
            {{1.0f, 1.0f}, {4.5f, 3.0f}, 1}, /* node 1 is invisible */
        }}},
};

const struct {
    const char* name;
    Vector2 offset;
    Vector2 size;
    bool allVisible;
} CullVisibleNodesEdgesData[]{
    {"", { 1.0f,  1.0f}, {7.0f, 7.0f}, false},
    {"touching edges", {0.0f, 0.0f}, {9.0f, 9.0f}, false},
    {"touching everything", {-0.01f, -0.01f}, {9.02f, 9.02f}, true}
};

const struct {
    const char* name;
    NodeFlags flags[15];
    bool visible[15];
    Containers::Array<Containers::Triple<Vector2, Vector2, UnsignedInt>> clipRects;
} CullVisibleNodesData[]{
    {"all clipping", {
        NodeFlag::Clip, NodeFlag::Clip, NodeFlag::Clip, NodeFlag::Clip, /* 0-3 */
        NodeFlag::Clip, NodeFlag::Clip, NodeFlag::Clip, NodeFlag::Clip, /* 4-7 */
        NodeFlag::Clip, NodeFlag::Clip, NodeFlag::Clip, NodeFlag::Clip, /* 8-11 */
        NodeFlag::Clip, NodeFlag::Clip, NodeFlag::Clip                  /* 12-14 */
    }, {
        false, /* 0 */
        false, /* 1, hidden because it's clipped by 2 */
        true,  /* 2 */
        true,  /* 3 */
        false, /* 4, hidden because it's clipped by 2 */
        true,  /* 5 */
        false, /* 6, hidden because it's clipped by 0 */
        true,  /* 7 */
        false, /* 8 */
        false, /* 9 */
        true,  /* 10 */
        false, /* 11, hidden because it has zero size */
        false, /* 12, hidden because it has zero height */
        false, /* 13, hidden because it has zero width */
        false, /* 14, hidden because it's a child of a zero-size rect */
    }, {InPlaceInit, {
        {{ 0.0f, 0.0f}, { 1.0f, 2.0f}, 1}, /* Node 3 (top-level) */
        {{ 2.0f, 0.0f}, {11.0f, 5.0f}, 5}, /* Node 7 (top-level), including
                                              hidden 11, 14, 13, 12 */
        {{ 3.0f, 2.0f}, { 5.0f, 3.0f}, 3}, /* Node 2 intersecting 7, including
                                              hidden 0, 6 */
        {{ 5.0f, 2.0f}, { 2.0f, 2.0f}, 1}, /* Node 10 intersecing 2 + 7 */
        {{ 3.0f, 2.0f}, { 5.0f, 3.0f}, 2}, /* Node 2 intersecting 7 remaining,
                                              hidden children 1, 4 */
        {{14.0f, 1.0f}, { 1.0f, 3.0f}, 3}, /* Node 5 (top-level), including
                                              hidden 9, 8 */
    }}},
    {"no clipping", {
        {},             {},             {},             {},             /* 0-3 */
        {},             {},             {},             {},             /* 4-7 */
        {},             {},             {},             {},             /* 8-11 */
        {},             {},             {},                             /* 12-14 */
    }, {
        true, true, true, true, true, true, true, true, true, true, true, true,
        true, true, true,
    }, {InPlaceInit, {
        {{}, {}, 1},  /* Top-level node 3 */
        {{}, {}, 11}, /* Top-level node 7 */
        {{}, {}, 3},  /* Top-level node 5 */
    }}},
    {"special cases", {
        {},             {},             {},             {},             /* 0-3 */
        {},             {},             {},             NodeFlag::Clip, /* 4-7 */
        NodeFlag::Clip, {},             {},             {},             /* 8-11 */
        NodeFlag::Clip, NodeFlag::Clip, NodeFlag::Clip,                 /* 12-14 */
    }, {
        false, /* 0, clipped by 7 */
        true,  /* 1, outside of 2 but that one is not clipping */
        true,  /* 2, partially visible in 7 */
        true,  /* 3 */
        true,  /* 4, inside 1 which is visible */
        true,  /* 5 */
        true,  /* 6, partially visible in 7 even though it's a child of 0
                     that's fully clipped */
        true,  /* 7 */
        true,  /* 8, clips but only its children, not itself against the
                     parent */
        true,  /* 9, outside of 5 but 5 doesn't clip */
        true,  /* 10, fully visible in 7 */
        true,  /* 11, shown even though it has zero size as it doesn't clip */
        false, /* 12, hidden because it clips and has zero height */
        false, /* 13, hidden because it clips and has zero width */
        true,  /* 14, shown even though it's a child of a zero-size rect, it
                      clips its children but not itself against the parent */
    }, {InPlaceInit, {
        {{}, {}, 1},                       /* Node 3, not clipping */
        {{ 2.0f, 0.0f}, {11.0f, 5.0f}, 2}, /* Node 7 plus 11 */
        {{12.0f, 2.0f}, { 1.0f, 1.0f}, 1}, /* Node 14 intersecting 7 */
        {{ 2.0f, 0.0f}, {11.0f, 5.0f}, 8}, /* Node 7 remaining, hidden 13, 12,
                                              clipped 2, hidden 0, clipped 6,
                                              10, 1, 4 */
        {{}, {}, 2},                       /* Node 5 plus 9, not clipping */
        {{16.0f, 3.0f}, { 1.0f, 2.0f}, 1}, /* Node 8 */
    }}},
};

AbstractUserInterfaceImplementationTest::AbstractUserInterfaceImplementationTest() {
    addTests({&AbstractUserInterfaceImplementationTest::orderNodesBreadthFirst,

              &AbstractUserInterfaceImplementationTest::orderVisibleNodesDepthFirst,
              &AbstractUserInterfaceImplementationTest::orderVisibleNodesDepthFirstSingleBranch,
              &AbstractUserInterfaceImplementationTest::orderVisibleNodesDepthFirstNoTopLevelNodes,

              &AbstractUserInterfaceImplementationTest::visibleTopLevelNodeIndices});

    addInstancedTests({&AbstractUserInterfaceImplementationTest::cullVisibleNodesClipRects},
        Containers::arraySize(CullVisibleNodesClipRectsData));

    addInstancedTests({&AbstractUserInterfaceImplementationTest::cullVisibleNodesEdges},
        Containers::arraySize(CullVisibleNodesEdgesData));

    addInstancedTests({&AbstractUserInterfaceImplementationTest::cullVisibleNodes},
        Containers::arraySize(CullVisibleNodesData));

    addTests({&AbstractUserInterfaceImplementationTest::cullVisibleNodesNoTopLevelNodes,

              &AbstractUserInterfaceImplementationTest::orderVisibleNodeData,
              &AbstractUserInterfaceImplementationTest::orderVisibleNodeDataNoTopLevelNodes,

              &AbstractUserInterfaceImplementationTest::orderNodeDataForEventHandling,

              &AbstractUserInterfaceImplementationTest::compactDraws});
}

void AbstractUserInterfaceImplementationTest::orderNodesBreadthFirst() {
    /* The handle generations aren't used for anything here so can be
       arbitrary */
    const struct Node {
        NodeHandle parent;
    } nodes[]{
        /* Forward parent reference */
        {nodeHandle(9, 0x123)},           /* 0 */
        /* Root elements. The IDs aren't used for anything so they can be
           arbitrary. */
        {nodeHandle(0xdead, 0)},          /* 1 */
        {nodeHandle(0xfefe, 0)},          /* 2 */
        /* Backward parent reference */
        {nodeHandle(1, 0xabc)},           /* 3 */
        /* Deep hierarchy */
        {nodeHandle(3, 0x1)},             /* 4 */
        {nodeHandle(4, 0xfff)},           /* 5 */
        /* Multiple children */
        {nodeHandle(1, 0x1)},             /* 6 */
        {nodeHandle(8, 0x1)},             /* 7 */
        {nodeHandle(1, 0x1)},             /* 8 */
        /* More root elements */
        {nodeHandle(0xcafe, 0)}           /* 9 */
    };

    /* Important: the childrenOffsets array has to be zero-initialized. Others
       don't need to be. */
    UnsignedInt childrenOffsets[Containers::arraySize(nodes) + 2]{};
    UnsignedInt children[Containers::arraySize(nodes)];
    Int out[Containers::arraySize(nodes) + 1];
    Implementation::orderNodesBreadthFirstInto(
        Containers::stridedArrayView(nodes).slice(&Node::parent),
        childrenOffsets, children, out);
    CORRADE_COMPARE_AS(Containers::arrayView(out), Containers::arrayView({
        /* -1 is always first */
        -1,
        /* Root nodes first, in order as found */
        1,
        2,
        9,
        /* Then children of node 1, clustered together, in order as found */
        3,
        6,
        8,
        /* Then children of node 9 */
        0,
        /* Children of node 3 */
        4,
        /* Children of node 8 */
        7,
        /* Children of node 4 */
        5,
    }), TestSuite::Compare::Container);
}

void AbstractUserInterfaceImplementationTest::orderVisibleNodesDepthFirst() {
    /* Non-zero handle generations aren't used for anything here so can be
       arbitrary */
    const struct Node {
        NodeHandle parentOrOrder;
        NodeFlags flags;
    } nodes[]{
        /* Forward parent reference */
        {nodeHandle(13, 0x123), {}},                /* 0 */
        /* Root elements, the middle one isn't included in the order and its ID
           can again be whatever for purposes of this algorithm */
        {nodeHandle(2, 0), {}},                     /* 1 */
        {nodeHandle(0xfefe, 0), {}},                /* 2 */
        {nodeHandle(7, 0), {}},                     /* 3 */
        /* Backward parent reference */
        {nodeHandle(1, 0xabc), {}},                 /* 4 */
        /* Deep hierarchy */
        {nodeHandle(4, 0x1), {}},                   /* 5 */
        {nodeHandle(5, 0xfff), {}},                 /* 6 */
        /* Hidden nodes, the first is top-level */
        {nodeHandle(3, 0), NodeFlag::Hidden},       /* 7 */
        {nodeHandle(1, 0xebe), NodeFlag::Hidden},   /* 8 */
        /* Multiple children */
        {nodeHandle(1, 0x1), {}},                   /* 9 */
        {nodeHandle(11, 0x1), {}},                  /* 10 */
        {nodeHandle(1, 0x1), {}},                   /* 11 */
        /* More root elements, the first isn't included in the order */
        {nodeHandle(0xbaba, 0), {}},                /* 12 */
        {nodeHandle(6, 0), {}}                      /* 13 */
    };

    /* The generation can be again arbitrary but it has to match with
       `firstNodeOrder` at least so the iteration of the cyclic list knows
       when to stop */
    const struct NodeOrder {
        NodeHandle next;
    } nodeOrder[]{
        {},                                 /* 0 */
        {},                                 /* 1 */
        /* Next after node 1 (which references order 2) is node 3 */
        {nodeHandle(3, 0xfef)},             /* 2 */
        /* Next after node 7 is node 1 */
        {nodeHandle(1, 0xbab)},             /* 3 */
        {},                                 /* 4 */
        {},                                 /* 5 */
        /* Next after node 13 is node 7 */
        {nodeHandle(7, 0xebe)},             /* 6 */
        /* Next after node 3 is node 13 */
        {nodeHandle(13, 0x080)}             /* 7 */
    };
    const NodeHandle firstNodeOrder = nodeHandle(3, 0xfef);

    /* Important: the childrenOffsets array has to be zero-initialized. Others
       don't need to be. */
    UnsignedInt childrenOffsets[Containers::arraySize(nodes) + 1]{};
    UnsignedInt children[Containers::arraySize(nodes)];
    Containers::Triple<UnsignedInt, UnsignedInt, UnsignedInt> parentsToProcess[Containers::arraySize(nodes)];
    Containers::Pair<UnsignedInt, UnsignedInt> out[Containers::arraySize(nodes)];
    std::size_t count = Implementation::orderVisibleNodesDepthFirstInto(
        Containers::stridedArrayView(nodes).slice(&Node::parentOrOrder),
        Containers::stridedArrayView(nodes).slice(&Node::flags),
        Containers::stridedArrayView(nodeOrder).slice(&NodeOrder::next),
        firstNodeOrder, childrenOffsets, children, parentsToProcess,
        Containers::stridedArrayView(out).slice(&Containers::Pair<UnsignedInt, UnsignedInt>::first),
        Containers::stridedArrayView(out).slice(&Containers::Pair<UnsignedInt, UnsignedInt>::second));
    CORRADE_COMPARE_AS(count,
        Containers::arraySize(nodes),
        TestSuite::Compare::LessOrEqual);
    CORRADE_COMPARE_AS(Containers::arrayView(out).prefix(count), (Containers::arrayView<Containers::Pair<UnsignedInt, UnsignedInt>>({
        /* First is node 3, it has no children */
        {3, 0},

        /* Next is node 13, then its children */
        {13, 1},
            {0, 0},

        /* Top-level node 7 is hidden, not listed here */

        /* Next is node 1 and its children */
        {1, 6},
            {4, 2},
                {5, 1},
                    {6, 0},
            /* Node 8 is hidden, not listed here */
            {9, 0},
            {11, 1},
                {10, 0},

        /* Node 2 and 12 not present as these aren't included in the order */
    })), TestSuite::Compare::Container);
}

void AbstractUserInterfaceImplementationTest::orderVisibleNodesDepthFirstSingleBranch() {
    /* Verifies behavior with just a single visible top-level node and single
       branch, i.e. so the internal arrays are correctly sized as this should
       lead to the longest stack in orderVisibleNodesDepthFirstInto(). */

    const struct Node {
        NodeHandle parentOrOrder;
        NodeFlags flags;
    } nodes[]{
        {nodeHandle(0, 0), {}},             /* 0 */
        {nodeHandle(0, 0xabc), {}},         /* 1 */
        {nodeHandle(3, 0xbca), {}},         /* 2 */
        {nodeHandle(1, 0xcab), {}},         /* 3 */
    };
    const struct NodeOrder {
        NodeHandle next;
    } nodeOrder[]{
        {nodeHandle(0, 0xacb)},             /* 0 */
    };
    const NodeHandle firstNodeOrder = nodeHandle(0, 0xacb);

    UnsignedInt childrenOffsets[Containers::arraySize(nodes) + 1]{};
    UnsignedInt children[Containers::arraySize(nodes)];
    Containers::Triple<UnsignedInt, UnsignedInt, UnsignedInt> parentsToProcess[Containers::arraySize(nodes)];
    Containers::Pair<UnsignedInt, UnsignedInt> out[Containers::arraySize(nodes)];
    std::size_t count = Implementation::orderVisibleNodesDepthFirstInto(
        Containers::stridedArrayView(nodes).slice(&Node::parentOrOrder),
        Containers::stridedArrayView(nodes).slice(&Node::flags),
        Containers::stridedArrayView(nodeOrder).slice(&NodeOrder::next),
        firstNodeOrder, childrenOffsets, children, parentsToProcess,
        Containers::stridedArrayView(out).slice(&Containers::Pair<UnsignedInt, UnsignedInt>::first),
        Containers::stridedArrayView(out).slice(&Containers::Pair<UnsignedInt, UnsignedInt>::second));
    CORRADE_COMPARE_AS(count,
        Containers::arraySize(nodes),
        TestSuite::Compare::LessOrEqual);
    CORRADE_COMPARE_AS(Containers::arrayView(out).prefix(count), (Containers::arrayView<Containers::Pair<UnsignedInt, UnsignedInt>>({
        {0, 3},
            {1, 2},
                {3, 1},
                    {2, 0}
    })), TestSuite::Compare::Container);
}

void AbstractUserInterfaceImplementationTest::orderVisibleNodesDepthFirstNoTopLevelNodes() {
    struct Node {
        NodeHandle parentOrOrder;
        NodeFlags flags;
    } nodes[10]; /* {} makes GCC 4.8 crash */
    const struct NodeOrder {
        NodeHandle next;
    } nodeOrder[10]{};

    /* There's no first node order, so nothing is visible */
    UnsignedInt childrenOffsets[Containers::arraySize(nodes) + 1]{};
    UnsignedInt children[Containers::arraySize(nodes)];
    Containers::Triple<UnsignedInt, UnsignedInt, UnsignedInt> parentsToProcess[Containers::arraySize(nodes)];
    Containers::Pair<UnsignedInt, UnsignedInt> out[Containers::arraySize(nodes)];
    std::size_t count = Implementation::orderVisibleNodesDepthFirstInto(
        Containers::stridedArrayView(nodes).slice(&Node::parentOrOrder),
        Containers::stridedArrayView(nodes).slice(&Node::flags),
        Containers::stridedArrayView(nodeOrder).slice(&NodeOrder::next),
        NodeHandle::Null, childrenOffsets, children, parentsToProcess,
        Containers::stridedArrayView(out).slice(&Containers::Pair<UnsignedInt, UnsignedInt>::first),
        Containers::stridedArrayView(out).slice(&Containers::Pair<UnsignedInt, UnsignedInt>::second));
    CORRADE_COMPARE(count, 0);
}

void AbstractUserInterfaceImplementationTest::visibleTopLevelNodeIndices() {
    /* Mostly like the output in the orderVisibleNodesDepthFirst() case */
    UnsignedInt visibleNodeChildrenCounts[]{
        /* First node has no children */
        0,

        /* Next has one child */
        1,
            0,

        /* Next has 6 children */
        6,
            2,
                1,
                    0,
            0,
            1,
                0,

        /* Next has none again */
        0,
    };

    UnsignedInt visibleTopLevelNodeIndices[5];
    std::size_t count = Implementation::visibleTopLevelNodeIndicesInto(visibleNodeChildrenCounts, visibleTopLevelNodeIndices);
    CORRADE_COMPARE(count, 4);
    CORRADE_COMPARE_AS(Containers::arrayView(visibleTopLevelNodeIndices).prefix(count), Containers::arrayView<UnsignedInt>({
        0, 1, 3, 10
    }), TestSuite::Compare::Container);
}

void AbstractUserInterfaceImplementationTest::cullVisibleNodesClipRects() {
    auto&& data = CullVisibleNodesClipRectsData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    UnsignedByte visibleNodeMaskStorage[1];
    Containers::MutableBitArrayView visibleNodeMask{visibleNodeMaskStorage, 0, data.nodeIdsChildrenCount.size()};

    Containers::Triple<Vector2, Vector2, UnsignedInt> clipStack[8];
    Containers::Triple<Vector2, Vector2, UnsignedInt> clipRects[8];
    UnsignedInt count = Implementation::cullVisibleNodesInto(
        stridedArrayView(data.nodeOffsetsSizesFlags).slice(&Containers::Triple<Vector2, Vector2, NodeFlags>::first),
        stridedArrayView(data.nodeOffsetsSizesFlags).slice(&Containers::Triple<Vector2, Vector2, NodeFlags>::second),
        stridedArrayView(data.nodeOffsetsSizesFlags).slice(&Containers::Triple<Vector2, Vector2, NodeFlags>::third),
        Containers::arrayView(clipStack).prefix(data.nodeIdsChildrenCount.size()),
        stridedArrayView(data.nodeIdsChildrenCount).slice(&Containers::Pair<UnsignedInt, UnsignedInt>::first),
        stridedArrayView(data.nodeIdsChildrenCount).slice(&Containers::Pair<UnsignedInt, UnsignedInt>::second),
        visibleNodeMask,
        Containers::stridedArrayView(clipRects)
            .prefix(data.nodeIdsChildrenCount.size())
            .slice(&Containers::Triple<Vector2, Vector2, UnsignedInt>::first),
        Containers::stridedArrayView(clipRects)
            .prefix(data.nodeIdsChildrenCount.size())
            .slice(&Containers::Triple<Vector2, Vector2, UnsignedInt>::second),
        Containers::stridedArrayView(clipRects)
            .prefix(data.nodeIdsChildrenCount.size())
            .slice(&Containers::Triple<Vector2, Vector2, UnsignedInt>::third));
    CORRADE_COMPARE_AS(visibleNodeMask,
        stridedArrayView(data.expectedVisible).sliceBit(0),
        TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(count,
        data.nodeIdsChildrenCount.size(),
        TestSuite::Compare::LessOrEqual);
    CORRADE_COMPARE_AS(Containers::arrayView(clipRects).prefix(count),
        arrayView(data.expectedClipRects),
        TestSuite::Compare::Container);

    /* The total count of all clip rects should be equal to the total node
       count, including hidden nodes */
    UnsignedInt clipRectCount = 0;
    for(const auto& i: Containers::arrayView(clipRects).prefix(count))
        clipRectCount += i.third();
    CORRADE_COMPARE(clipRectCount, data.nodeOffsetsSizesFlags.size());
}

void AbstractUserInterfaceImplementationTest::cullVisibleNodesEdges() {
    auto&& data = CullVisibleNodesEdgesData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /*                                -3 -2   0 1   34   56   8 9  11 12
                                    -3        +-----------------+
                                              |       30        |
        -1 0 12 3 4   5 6 78 9 10   -2    +---+.....+.....+.....+---+
      -1        +-------+                 |16 <  14 |  18 | 15  > 17|
       0  +-----|-+...+-|-----+      0 +--+---+-----+-----+---------+--+
          |0    | | 1 | |    2|        |  .   |                 |   .  |
       1  |  +==|=|===|=|==+  |      1 |  .19 | +=============+ | 20.  |
       2  |  |  +-------+  |  |        |  .   | |             | |   .  |
       3 +----+   | 3 |   +----+     3 |  +---+ |             | +---+  |
       4 |+---|---+...+---|---+|     4 |  .   | |    +---+    | |   .  |
         | 4 || 6 .   . 7 || 5 |       |31. 23| |    | 12|    | |24 .32|
       5 |+---|---+...+---|---+|     5 |  .   | |    +---+    | |   .  |
       6 +----+   | 8 |   +----+     6 |  +---+ |             | +---+  |
       7  |  |  +-------+  |  |        |  .   | |             | |   .  |
       8  |  +==|=|===|=|==+  |      8 |  .21 | +=============+ | 22.  |
          |10   | | 9 | |   11|        |  .   |13               |   .  |
       9  +-----|-+   +-|-----+      9 +--+---+-----+-----+-----+---+--+
      10        +-------+                 |27 <  25 | 29  | 26  > 28|
                                    11    +---+.....+.....+.....+---+
                                              |        33       |
                                    12        +-----------------+        */
    const struct Node {
        Vector2 offset;
        Vector2 size;
        NodeFlags flags;
    } nodeOffsetsSizesFlags[]{
        {{ 0.0f,  0.0f}, {4.0f, 4.0f}, {}}, /*  0, top left */
        {{ 3.0f, -1.0f}, {3.0f, 3.0f}, {}}, /*  1, top */
        {{ 5.0f,  0.0f}, {4.0f, 4.0f}, {}}, /*  2, top right */
        {{ 0.0f,  0.0f}, {9.0f, 4.0f}, {}}, /*  3, top left + right */
        {{-1.0f,  3.0f}, {3.0f, 3.0f}, {}}, /*  4, left */
        {{ 7.0f,  3.0f}, {3.0f, 3.0f}, {}}, /*  5, right */
        {{ 0.0f,  0.0f}, {4.0f, 9.0f}, {}}, /*  6, left top + bottom */
        {{ 5.0f,  0.0f}, {4.0f, 9.0f}, {}}, /*  7, right top + bottom */
        {{ 0.0f,  5.0f}, {9.0f, 4.0f}, {}}, /*  8, bottom left + right */
        {{ 3.0f,  7.0f}, {3.0f, 3.0f}, {}}, /*  9, bottom */
        {{ 0.0f,  5.0f}, {4.0f, 4.0f}, {}}, /* 10, bottom left */
        {{ 5.0f,  5.0f}, {4.0f, 4.0f}, {}}, /* 11, bottom right */

        {{ 4.0f,  4.0f}, {2.0f, 2.0f}, {}}, /* 12, in the center */
        {{ 0.0f,  0.0f}, {9.0f, 9.0f}, {}}, /* 13, covering whole area */

        {{-2.0f, -2.0f}, {5.0f, 2.0f}, {}}, /* 14, outside top extended left */
        {{ 6.0f, -2.0f}, {5.0f, 2.0f}, {}}, /* 15, outside top extended right */
        {{-2.0f, -2.0f}, {2.0f, 2.0f}, {}}, /* 16, outside top left */
        {{ 9.0f, -2.0f}, {2.0f, 2.0f}, {}}, /* 17, outside top right */
        {{ 3.0f, -2.0f}, {3.0f, 2.0f}, {}}, /* 18, outside top */
        {{-2.0f,  0.0f}, {2.0f, 3.0f}, {}}, /* 19, outside left extended top */
        {{ 9.0f,  0.0f}, {2.0f, 3.0f}, {}}, /* 20, outside right extended top */
        {{-2.0f,  6.0f}, {2.0f, 3.0f}, {}}, /* 21, outside left extended bottom */
        {{ 9.0f,  0.0f}, {2.0f, 3.0f}, {}}, /* 22, outside right extended bottom */
        {{-2.0f,  3.0f}, {2.0f, 3.0f}, {}}, /* 23, outside left */
        {{ 9.0f,  3.0f}, {2.0f, 3.0f}, {}}, /* 24, outside right */
        {{-2.0f,  9.0f}, {5.0f, 2.0f}, {}}, /* 25, outside bottom extended left */
        {{ 6.0f,  9.0f}, {5.0f, 2.0f}, {}}, /* 26, outside bottom extended right */
        {{-2.0f,  9.0f}, {2.0f, 2.0f}, {}}, /* 27, outside bottom left */
        {{ 9.0f,  9.0f}, {2.0f, 2.0f}, {}}, /* 28, outside bottom right */
        {{ 3.0f,  9.0f}, {3.0f, 2.0f}, {}}, /* 29, outside bottom */
        {{ 0.0f, -3.0f}, {9.0f, 3.0f}, {}}, /* 30, outside top left + right */
        {{-3.0f,  0.0f}, {3.0f, 9.0f}, {}}, /* 31, outside left top + bottom */
        {{ 9.0f,  0.0f}, {3.0f, 9.0f}, {}}, /* 32, outside right top + bottom */
        {{ 0.0f,  9.0f}, {9.0f, 3.0f}, {}}, /* 33, outside bottom left + right */
        {data.offset, data.size, NodeFlag::Clip}, /* 34, clip node */
    };

    /* Children after the parent */
    const struct Children {
        UnsignedInt id;
        UnsignedInt count;
    } nodeIdsChildrenCount[]{
        {34, 34},
             {0, 0},  {1, 0},  {2, 0},  {3, 0},  {4, 0},  {5, 0},  {6, 0},
             {7, 0},  {8, 0},  {9, 0}, {10, 0}, {11, 0}, {12, 0}, {13, 0},
            {14, 0}, {15, 0}, {16, 0}, {17, 0}, {18, 0}, {19, 0}, {20, 0},
            {21, 0}, {22, 0}, {23, 0}, {24, 0}, {25, 0}, {26, 0}, {27, 0},
            {28, 0}, {29, 0}, {30, 0}, {31, 0}, {32, 0}, {33, 0}
    };

    UnsignedInt visibleNodeMaskStorage[2];
    Containers::MutableBitArrayView visibleNodeMask{visibleNodeMaskStorage, 0, Containers::arraySize(nodeOffsetsSizesFlags)};

    Containers::Triple<Vector2, Vector2, UnsignedInt> clipStack[Containers::arraySize(nodeOffsetsSizesFlags)];
    Containers::Triple<Vector2, Vector2, UnsignedInt> clipRects[Containers::arraySize(nodeOffsetsSizesFlags)];
    UnsignedInt count = Implementation::cullVisibleNodesInto(
        Containers::stridedArrayView(nodeOffsetsSizesFlags).slice(&Node::offset),
        Containers::stridedArrayView(nodeOffsetsSizesFlags).slice(&Node::size),
        Containers::stridedArrayView(nodeOffsetsSizesFlags).slice(&Node::flags),
        clipStack,
        Containers::stridedArrayView(nodeIdsChildrenCount).slice(&Children::id),
        Containers::stridedArrayView(nodeIdsChildrenCount).slice(&Children::count),
        visibleNodeMask,
        Containers::stridedArrayView(clipRects).slice(&Containers::Triple<Vector2, Vector2, UnsignedInt>::first),
        Containers::stridedArrayView(clipRects).slice(&Containers::Triple<Vector2, Vector2, UnsignedInt>::second),
        Containers::stridedArrayView(clipRects).slice(&Containers::Triple<Vector2, Vector2, UnsignedInt>::third));

    if(data.allVisible) CORRADE_COMPARE_AS(visibleNodeMask,
        Containers::stridedArrayView({
            /* All 35 is visible */
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        }).sliceBit(0), TestSuite::Compare::Container);
    else CORRADE_COMPARE_AS(visibleNodeMask,
        Containers::stridedArrayView({
            /* First 14 should be all visible */
            1, 1, 1, 1,
            1, 1, 1, 1,
            1, 1, 1, 1,
            1, 1,
            /* The next 20 shouldn't */
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            /* The last one should be visible as it's the root one */
            1,
        }).sliceBit(0), TestSuite::Compare::Container);

    /* There's just one clip rect covering all. The count is always the same as
       it includes hidden nodes as well. */
    CORRADE_COMPARE_AS(count, Containers::arraySize(clipRects),
        TestSuite::Compare::LessOrEqual);
    CORRADE_COMPARE_AS(Containers::arrayView(clipRects).prefix(count), (Containers::arrayView<Containers::Triple<Vector2, Vector2, UnsignedInt>>({
        {data.offset, data.size, 35u}
    })), TestSuite::Compare::Container);
}

void AbstractUserInterfaceImplementationTest::cullVisibleNodes() {
    auto&& data = CullVisibleNodesData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    const struct Children {
        UnsignedInt id;
        UnsignedInt count;
    } nodeIdsChildrenCount[]{
        /* No children */
        {3, 0},

        /* Several nested children */
        {7, 10},
            {11, 1}, /* Zero size, so gets skipped and its child also */
                {14, 0},
            {13, 0}, /* Zero width */
            {12, 0}, /* Zero height */
            {2, 5},
                {0, 1}, /* Visible in 2 but not in 7 */
                    /* Extends back to 7 but still gets skipped without testing
                       because it's fully clipped by 0 */
                    {6, 0},
                {10, 0},
                {1, 1}, /* Visible in the top-level rect but not the parent */
                    {4, 0}, /* Gets skipped without testing */

        /* Two invisible children */
        {5, 2},
            {9, 0},
            {8, 0},
    };

    /*   0   1   234  5 678 9 10 11  12  13  14   15 16 17
       0 +---+   +-----------------------+           +---+
       1 | 3 |   | 7  +--+  +-----+ 11 12|   +-----+ | 9 |
       2 +---+   |+---|--|+ |  +--+   +--+   |     | +---+
                 ||   |10|| |1 |4 | 13|14|   |  5  |
       3         || 2 |  || |  +--+   +--+   |     | +---+
       4         ||   +--+| +-----+      |   +-----+ | 8 |
       5         +|---|--||--------------+           +---+
       6          |+--|--||
                  ||  |6 ||
       7          ||0 +--+|
       8          |+-----+|
       9          +-------+
                 234  5 678                           */
    const struct Node {
        Vector2 offset;
        Vector2 size;
    } nodeOffsetsSizes[]{
        {{ 4.0f, 6.0f}, { 3.0f, 2.0f}}, /* 0 */
        {{ 9.0f, 1.0f}, { 2.0f, 5.0f}}, /* 1 */
        {{ 3.0f, 2.0f}, { 5.0f, 7.0f}}, /* 2 */
        {{ 0.0f, 0.0f}, { 1.0f, 2.0f}}, /* 3 */
        {{10.0f, 2.0f}, { 1.0f, 1.0f}}, /* 4 */
        {{14.0f, 1.0f}, { 1.0f, 3.0f}}, /* 5 */
        {{ 5.0f, 4.0f}, { 2.0f, 3.0f}}, /* 6 */
        {{ 2.0f, 0.0f}, {11.0f, 5.0f}}, /* 7 */
        {{16.0f, 3.0f}, { 1.0f, 2.0f}}, /* 8 */
        {{16.0f, 0.0f}, { 1.0f, 2.0f}}, /* 9 */
        {{ 5.0f, 1.0f}, { 2.0f, 3.0f}}, /* 10 */
        {{12.0f, 2.0f}, { 0.0f, 0.0f}}, /* 11 */
        {{12.0f, 2.0f}, { 1.0f, 0.0f}}, /* 12 */
        {{12.0f, 2.0f}, { 0.0f, 1.0f}}, /* 13 */
        {{12.0f, 2.0f}, { 1.0f, 1.0f}}, /* 14 */
    };

    UnsignedShort visibleNodeMaskStorage[1];
    Containers::MutableBitArrayView visibleNodeMask{visibleNodeMaskStorage, 0, Containers::arraySize(nodeOffsetsSizes)};

    Containers::Triple<Vector2, Vector2, UnsignedInt> clipStack[Containers::arraySize(nodeOffsetsSizes)];
    Containers::Triple<Vector2, Vector2, UnsignedInt> clipRects[Containers::arraySize(nodeOffsetsSizes)];
    UnsignedInt count = Implementation::cullVisibleNodesInto(
        Containers::stridedArrayView(nodeOffsetsSizes).slice(&Node::offset),
        Containers::stridedArrayView(nodeOffsetsSizes).slice(&Node::size),
        Containers::arrayView(data.flags),
        clipStack,
        Containers::stridedArrayView(nodeIdsChildrenCount).slice(&Children::id),
        Containers::stridedArrayView(nodeIdsChildrenCount).slice(&Children::count),
        visibleNodeMask,
        Containers::stridedArrayView(clipRects).slice(&Containers::Triple<Vector2, Vector2, UnsignedInt>::first),
        Containers::stridedArrayView(clipRects).slice(&Containers::Triple<Vector2, Vector2, UnsignedInt>::second),
        Containers::stridedArrayView(clipRects).slice(&Containers::Triple<Vector2, Vector2, UnsignedInt>::third));
    CORRADE_COMPARE_AS(visibleNodeMask,
        Containers::stridedArrayView(data.visible).sliceBit(0),
        TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(count, Containers::arraySize(clipRects),
        TestSuite::Compare::LessOrEqual);
    CORRADE_COMPARE_AS(Containers::arrayView(clipRects).prefix(count),
        Containers::arrayView(data.clipRects),
        TestSuite::Compare::Container);

    /* The total count of all clip rects should be equal to the total node
       count, including hidden nodes */
    UnsignedInt clipRectCount = 0;
    for(const auto& i: Containers::arrayView(clipRects).prefix(count))
        clipRectCount += i.third();
    CORRADE_COMPARE(clipRectCount, Containers::arraySize(nodeOffsetsSizes));
}

void AbstractUserInterfaceImplementationTest::cullVisibleNodesNoTopLevelNodes() {
    Vector2 absoluteNodeOffsets[3];
    Vector2 nodeSizes[3];
    NodeFlags nodeFlags[3];
    UnsignedByte visibleNodeMaskData[1]{0xff};
    Containers::MutableBitArrayView visibleNodeMask{visibleNodeMaskData, 0, 3};
    UnsignedInt count = Implementation::cullVisibleNodesInto(
        absoluteNodeOffsets,
        nodeSizes,
        nodeFlags,
        nullptr,
        nullptr,
        nullptr,
        visibleNodeMask,
        nullptr,
        nullptr,
        nullptr);

    /* To not crash on OOB it should return early but should still clear the
       visibility bits for all visible nodes */
    CORRADE_COMPARE(count, 0);
    CORRADE_COMPARE_AS(visibleNodeMask, Containers::stridedArrayView({
        false, false, false
    }).sliceBit(0), TestSuite::Compare::Container);
}

void AbstractUserInterfaceImplementationTest::orderVisibleNodeData() {
    /* Ordered visible node hierarchy */
    const Containers::Pair<UnsignedInt, UnsignedInt> visibleNodeIdsChildrenCount[]{
        /* No children, three data attachments from layers 2 and 5 */
        {3, 0},

        /* Several nested children */
        {13, 7},
            {9, 3},
                {1, 1},
                    {4, 0}, /* One data attached from layer 2 */
                {2, 0}, /* One data attached from layer 1, two from layer 2,
                           one from layer 3 not for drawing */
            {6, 1}, /* Marked as invisible, one data attached from layer 2 */
                {5, 0}, /* Marked as invisible, one data from layer 3 */
            {7, 0}, /* One data attached from layer 1, one from layer 3 not for
                       drawing */

        /* One child, no data attachment, should get skipped */
        {11, 1},
            {10, 0},

        /* No children, one data attachment from layer 2 */
        {12, 0},
    };

    /* Node data assignments. Node generations don't matter in any way, the
       same node ID can even have different generations. */
    const NodeHandle layer1NodeAttachments[]{
        nodeHandle(7, 0xeee),  /* data handle ID 0 */
        nodeHandle(2, 0xaba),  /* data handle ID 1 */
    };
    const NodeHandle layer2NodeAttachments[]{
        nodeHandle(6, 0xece),  /* 0, but node 6 is not visible so ignored */
        NodeHandle{},          /* 1 */
        nodeHandle(4, 0xbab),  /* 2 */
        nodeHandle(3, 0xfef),  /* 3 */
        nodeHandle(12, 0xccc), /* 4 */
        NodeHandle{},          /* 5 */
        nodeHandle(2, 0xddd),  /* 6 */
        nodeHandle(2, 0x000),  /* 7 */
    };
    const NodeHandle layer3NodeAttachments[]{
        nodeHandle(2, 0xefe),  /* 0 */
        nodeHandle(5, 0xcec),  /* 1, but node 5 is not visible so ignored */
        nodeHandle(7, 0xf0f),  /* 2 */
    };
    const NodeHandle layer4NodeAttachments[]{
        NodeHandle{}, NodeHandle{}, NodeHandle{}, NodeHandle{}, NodeHandle{},
        NodeHandle{}, NodeHandle{}, NodeHandle{}, NodeHandle{}, NodeHandle{},
        NodeHandle{}, NodeHandle{}, NodeHandle{}, NodeHandle{}, NodeHandle{},
        NodeHandle{}, NodeHandle{}, /* 0 - 16 */
        /* Node 0 isn't in the visible hierarchy so the assignment gets
           ignored */
        nodeHandle(0, 0xefe),  /* 17 */
    };
    const NodeHandle layer5NodeAttachments[]{
        NodeHandle{},          /* 0 */
        nodeHandle(3, 0xc0c),  /* 1 */
        nodeHandle(3, 0xc0c),  /* 2 */
        NodeHandle{},          /* 3 */
        /* Node 8 isn't in the visible hierarchy so the assignment gets
           ignored */
        nodeHandle(8, 0xbbb),  /* 4 */
    };
    /* Nodes 5, 6 aren't present anywhere */

    /* Everything except nodes 0 and 8 (which are not part of the top-level
       order) and nodes 5 and 6 (which are culled) is visible */
    UnsignedShort visibleNodeMask[]{0xffff & ~(1 << 0) & ~(1 << 8) & ~(1 << 5) & ~(1 << 6)};

    /* Node counts for each clip rect used. A sum of these should be the total
       amount of visible nodes, i.e. arraySize(visibleNodeIdsChildrenCount). */
    UnsignedInt clipRectNodeCounts[]{
        /* Top level node 3 has one clip rect */
        1,
        /* Top-level node 13 has one clip rect for itself and node 9 */
        2,
            /* Then node 1 and 4 have another */
            2,
            /* Then node 2, invisible 6 and 5, and 7 fall back to the previous
               again */
            4,
        /* Top-level node 11 a clip rect for itself and node 10 */
        2,
        /* Top-level node 12 has one clip rect */
        1,
    };

    /* The layers are in order 4, 2, 3, 1, 5. Layer 0 doesn't have any data
       referenced, layer 3 doesn't have a Draw feature, layer 4 is referenced
       only by a node that isn't in the visible hierarchy. */
    Containers::Pair<Containers::StridedArrayView1D<const NodeHandle>, LayerFeatures> layers[]{
        {Containers::arrayView(layer4NodeAttachments), LayerFeature::Draw},
        {Containers::arrayView(layer2NodeAttachments), LayerFeature::Event|LayerFeature::Draw},
        {Containers::arrayView(layer3NodeAttachments), LayerFeature::Event},
        {Containers::arrayView(layer1NodeAttachments), LayerFeature::Draw},
        {Containers::arrayView(layer5NodeAttachments), LayerFeature::Draw|LayerFeature::Event}
    };

    UnsignedInt visibleNodeDataOffsets[15]{};
    UnsignedInt visibleNodeEventDataCounts[14]{};
    UnsignedInt visibleNodeDataIds[18];
    UnsignedInt dataToUpdateIds[18];
    Containers::Pair<UnsignedInt, UnsignedInt> dataToUpdateClipRectIdsDataCounts[Containers::arraySize(layers)*Containers::arraySize(clipRectNodeCounts)];
    Containers::Pair<UnsignedInt, UnsignedInt> dataOffsetsSizesToDraw[Containers::arraySize(layers)*4];
    Containers::Pair<UnsignedInt, UnsignedInt> dataClipRectOffsetsSizesToDraw[Containers::arraySize(layers)*4];

    /* This is similar to the process done by UserInterface::update(), except
       that here the layers aren't in a circular linked list */
    Containers::Array<Containers::Pair<UnsignedInt, UnsignedInt>> dataToUpdateLayerOffsets{InPlaceInit, {{0, 0}}};
    UnsignedInt offset = 0;
    UnsignedInt clipRectOffset = 0;
    for(const auto& layer: layers) {
        CORRADE_ITERATION(dataToUpdateLayerOffsets.size() - 1);
        auto out = Implementation::orderVisibleNodeDataInto(
            Containers::stridedArrayView(visibleNodeIdsChildrenCount).slice(&Containers::Pair<UnsignedInt, UnsignedInt>::first),
            Containers::stridedArrayView(visibleNodeIdsChildrenCount).slice(&Containers::Pair<UnsignedInt, UnsignedInt>::second),
            layer.first(),
            layer.second(),
            Containers::BitArrayView{visibleNodeMask, 0, 14},
            clipRectNodeCounts,
            visibleNodeDataOffsets,
            visibleNodeEventDataCounts,
            Containers::arrayView(visibleNodeDataIds).prefix(layer.first().size()),
            dataToUpdateIds,
            Containers::stridedArrayView(dataToUpdateClipRectIdsDataCounts)
                .slice(&Containers::Pair<UnsignedInt, UnsignedInt>::first),
            Containers::stridedArrayView(dataToUpdateClipRectIdsDataCounts)
                .slice(&Containers::Pair<UnsignedInt, UnsignedInt>::second),
            offset,
            clipRectOffset,
            Containers::stridedArrayView(dataOffsetsSizesToDraw)
                .slice(&Containers::Pair<UnsignedInt, UnsignedInt>::first)
                .exceptPrefix(dataToUpdateLayerOffsets.size() - 1)
                .every(Containers::arraySize(layers)),
            Containers::stridedArrayView(dataOffsetsSizesToDraw)
                .slice(&Containers::Pair<UnsignedInt, UnsignedInt>::second)
                .exceptPrefix(dataToUpdateLayerOffsets.size() - 1)
                .every(Containers::arraySize(layers)),
            Containers::stridedArrayView(dataClipRectOffsetsSizesToDraw)
                .slice(&Containers::Pair<UnsignedInt, UnsignedInt>::first)
                .exceptPrefix(dataToUpdateLayerOffsets.size() - 1)
                .every(Containers::arraySize(layers)),
            Containers::stridedArrayView(dataClipRectOffsetsSizesToDraw)
                .slice(&Containers::Pair<UnsignedInt, UnsignedInt>::second)
                .exceptPrefix(dataToUpdateLayerOffsets.size() - 1)
                .every(Containers::arraySize(layers)));
        offset = out.first();
        clipRectOffset = out.second();
        arrayAppend(dataToUpdateLayerOffsets, InPlaceInit, offset, clipRectOffset);
    }

    CORRADE_COMPARE_AS(Containers::arrayView(visibleNodeEventDataCounts), Containers::arrayView<UnsignedInt>({
        0,  /* Node 0, not part of the top-level hierarchy */
        0,  /* Node 1 */
        3,  /* Node 2, layers 2 and 3 */
        3,  /* Node 3, layer 2 and 5 */
        1,  /* Node 4, layer 2 */
        0,  /* Node 5, layer 3, but marked as invisible */
        0,  /* Node 6, layer 2, but marked as invisible */
        1,  /* Node 7, layer 3 */
        0,  /* Node 8, layer 5, but not part of the top-level hierarchy */
        0,  /* Node 9 */
        0,  /* Node 10 */
        0,  /* Node 11 */
        1,  /* Node 12, layer 2 */
        0,  /* Node 13 */
    }), TestSuite::Compare::Container);

    /* This is the offset filled in by the test itself above, in the order in
       which layers are processed */
    CORRADE_COMPARE_AS(dataToUpdateLayerOffsets, (Containers::arrayView<Containers::Pair<UnsignedInt, UnsignedInt>>({
        {0, 0},
        {0, 0}, /* Layer 4 has one item that isn't in the hierarchy, so
                   nothing */
        {5, 4}, /* Layer 2 has 5 items, and 4 clip rects */
        {7, 5}, /* Layer 3 has two items and one rect but doesn't have a Draw
                   feature, so these are then excluded from the draw call
                   list */
        {9, 6}, /* Layer 1 has 2 items and 1 clip rect */
        {11, 7} /* Layer 5 has 2 items and 1 clip rects plus one that isn't in
                   the hierarchy, so nothing */
    })), TestSuite::Compare::Container);

    /* Order inside layers is matching visible node order */
    CORRADE_COMPARE_AS(
        Containers::arrayView(dataToUpdateIds)
            /* The last element is the total filled size of the output array */
            .prefix(Containers::arrayView(dataToUpdateLayerOffsets).back().first()),
        Containers::arrayView<UnsignedInt>({
            /* Layer 4 has nothing */
            /* Layer 2 */
            3, 2, 6, 7, 4,
            /* Layer 3, but those aren't included in the draws below */
            0, 2,
            /* Layer 1 */
            1, 0,
            /* Layer 5, same node. Order matches the data ID order, not the
               order in which they were created or attached. */
            1, 2,
        }),
        TestSuite::Compare::Container);

    /* Each layer has a contiguous subsequence here, with the sum of it being
       the total count of data drawn there */
    CORRADE_COMPARE_AS(Containers::arrayView(dataToUpdateClipRectIdsDataCounts).prefix(clipRectOffset), (Containers::arrayView<Containers::Pair<UnsignedInt, UnsignedInt>>({
        /* Layer 4 has nothing */
        /* Layer 2 */
        {0, 1}, /* Node 3 */
        {2, 1}, /* Node 1, 4 */
        {3, 2}, /* Node 2, 7 */
        {5, 1}, /* Node 12 */
        /* Layer 3 but those aren't included in the draws below */
        {3, 2}, /* Node 2, 7 */
        /* Layer 1 */
        {3, 2}, /* Node 2, 7 */
        /* Layer 5 */
        {0, 2}, /* Node 3 */
        /* Nodes (13, 9) and (11, 10) have nothing attached so the
           corrresponding clip rects 1 and 4 are unused */
    })), TestSuite::Compare::Container);

    /* The draws are filled in for the whole layer across all top-level
       widgets, thus to be correctly ordered they have to be interleaved. If
       any of the layers doesn't have anything to draw for given top level
       node, the particular draw call count is zero. */
    CORRADE_COMPARE_AS(Containers::arrayView(dataOffsetsSizesToDraw),
        (Containers::arrayView<Containers::Pair<UnsignedInt, UnsignedInt>>({
            /* For top-level node 3 offset 0 from layer 2 (data 3) and offset
               8, 9 from layer 5 (data 1, 2) is drawn */
            {},     /* 4 */
            {0, 1}, /* 2 */
            {},     /* 3 */
            {},     /* 1 */
            {9, 2}, /* 5 */
            /* For top-level node 13 offset 1, 2, 3 from layer 2 (data 2, 6, 7)
               and offset 6, 7 from layer 1 (data 1, 0) is drawn */
            {},     /* 4 */
            {1, 3}, /* 2 */
            {},     /* 3 */
            {7, 2}, /* 1 */
            {},     /* 5 */
            /* For top-level node 11 nothing is drawn */
            {},     /* 4 */
            {},     /* 2 */
            {},     /* 3 */
            {},     /* 1 */
            {},     /* 5 */
            /* Top-level node 12 draws offset 5 from layer 2 (data 4) */
            {},     /* 4 */
            {4, 1}, /* 2 */
            {},     /* 3 */
            {},     /* 1 */
            {}      /* 5 */
    })), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(dataClipRectOffsetsSizesToDraw),
        (Containers::arrayView<Containers::Pair<UnsignedInt, UnsignedInt>>({
            /* For top-level node 3 offset 0 from layer 2 (rect 0) and offset
               6 from layer 5 (rect 0) is drawn */
            {},     /* 4 */
            {0, 1}, /* 2 */
            {},     /* 3 */
            {},     /* 1 */
            {6, 1}, /* 5 */
            /* For top-level node 13 offset 1 from layer 2 (rect 2) and offset
               5 from layer 1 (rect 3) is drawn */
            {},     /* 4 */
            {1, 2}, /* 2 */
            {},     /* 3 */
            {5, 1}, /* 1 */
            {},     /* 5 */
            /* For top-level node 11 nothing is drawn */
            {},     /* 4 */
            {},     /* 2 */
            {},     /* 3 */
            {},     /* 1 */
            {},     /* 5 */
            /* Top-level node 12 has offset 3 from layer 2 (rect 5) drawn */
            {},     /* 4 */
            {3, 1}, /* 2 */
            {},     /* 3 */
            {},     /* 1 */
            {}      /* 5 */
    })), TestSuite::Compare::Container);

    /* Each index in the draw data should appear exactly once. Rects can
       appaear multiple times. */
    Containers::BitArray dataDrawn{DirectInit, Containers::arrayView(dataToUpdateLayerOffsets).back().first(), false};
    for(const Containers::Pair<UnsignedInt, UnsignedInt>& i: dataOffsetsSizesToDraw) {
        CORRADE_ITERATION(i);
        for(UnsignedInt j = 0; j != i.second(); ++j) {
            CORRADE_ITERATION(j);
            CORRADE_VERIFY(!dataDrawn[i.first() + j]);
            dataDrawn.set(i.first() + j);
        }
    }

    /* Two items from layer 3 that doesn't have LayerFeature::Draw should not
       be present */
    CORRADE_COMPARE(dataDrawn.count(), Containers::arrayView(dataToUpdateLayerOffsets).back().first() - 2);
}

void AbstractUserInterfaceImplementationTest::orderVisibleNodeDataNoTopLevelNodes() {
    NodeHandle dataNodes[3];
    UnsignedByte visibleNodeMaskData[1];
    Containers::BitArrayView visibleNodeMask{visibleNodeMaskData, 0, 3};
    UnsignedInt visibleNodeDataOffsets[4];
    UnsignedInt visibleNodeEventDataCounts[3];
    UnsignedInt visibleNodeDataIds[3];
    Containers::Pair<UnsignedInt, UnsignedInt> count = Implementation::orderVisibleNodeDataInto(
        nullptr,
        nullptr,
        dataNodes,
        {},
        visibleNodeMask,
        nullptr,
        visibleNodeDataOffsets,
        visibleNodeEventDataCounts,
        visibleNodeDataIds,
        nullptr,
        nullptr,
        nullptr,
        0, 0,
        nullptr,
        nullptr,
        nullptr,
        nullptr);

    /* To avoid an OOB access it should return early */
    CORRADE_COMPARE(count.first(), 0);
    CORRADE_COMPARE(count.second(), 0);
}

void AbstractUserInterfaceImplementationTest::orderNodeDataForEventHandling() {
    /* Subset of data node attachments from orderVisibleNodeData() above for
       layers that have Event set */
    const Containers::Pair<NodeHandle, UnsignedShort> layer2NodeAttachments[]{
        {},                             /* 0 */
        {},                             /* 1 */
        {nodeHandle(4, 0xbab), 0x111},  /* 2 */
        {nodeHandle(3, 0xfef), 0x333},  /* 3 */
        {nodeHandle(12, 0xccc), 0x222}, /* 4 */
        {},                             /* 5 */
        {nodeHandle(2, 0xddd), 0x777}   /* 6 */
    };
    const Containers::Pair<NodeHandle, UnsignedShort> layer3NodeAttachments[]{
        {nodeHandle(2, 0xefe), 0x555},  /* 0 */
        {},                             /* 1 */
        {nodeHandle(7, 0xf0f), 0x888}   /* 2 */
    };
    const Containers::Pair<NodeHandle, UnsignedShort> layer5NodeAttachments[]{
        {},                             /* 0 */
        {nodeHandle(3, 0xc0c), 0x444},  /* 1 */
        {nodeHandle(3, 0xc0c), 0x666},  /* 2 */
        {},                             /* 3 */
        /* Node 8 isn't in the visible hierarchy so the assignment gets
           ignored */
        {nodeHandle(8, 0xbbb), 0x999},  /* 4 */
    };

    /* Compared to orderVisibleNodeData(), only node 8 is left among the
       assignments, all others can stay visible even if they aren't as it
       shouldn't matter for them */
    UnsignedShort visibleNodeMask[]{0xffff & ~(1 << 8)};

    /* Output from orderVisibleNodeData() above, turned into an offset array
       with an extra 0 at the front */
    UnsignedInt visibleNodeEventDataOffsets[]{
        0,
        0,  /* Node 0 */
        0,  /* Node 1 */
        0,  /* Node 2, 2 items from layers 2 and 3 */
        2,  /* Node 3, 3 items from layer 2 and 5 */
        5,  /* Node 4, 1 item from layer 2 */
        6,  /* Node 5 */
        6,  /* Node 6 */
        6,  /* Node 7, 1 item from layer 3 */
        7,  /* Node 8, 1 item from layer 5 which isn't visible */
        7,  /* Node 9 */
        7,  /* Node 10 */
        7,  /* Node 11 */
        7,  /* Node 12, 1 item from layer 2 */
        8,  /* Node 13 */
    };

    LayerHandle layer2 = layerHandle(2, 0x88);
    LayerHandle layer3 = layerHandle(3, 0x22);
    LayerHandle layer5 = layerHandle(5, 0x44);
    Containers::Pair<Containers::StridedArrayView1D<const Containers::Pair<NodeHandle, UnsignedShort>>, LayerHandle> layers[]{
        {layer5NodeAttachments, layer5},
        {layer3NodeAttachments, layer3},
        {layer2NodeAttachments, layer2},
    };

    DataHandle visibleNodeEventData[9];
    for(const auto& layer: layers) {
        CORRADE_ITERATION(layer.second());
        Implementation::orderNodeDataForEventHandlingInto(
            layer.second(),
            layer.first().slice(&Containers::Pair<NodeHandle, UnsignedShort>::second),
            layer.first().slice(&Containers::Pair<NodeHandle, UnsignedShort>::first),
            visibleNodeEventDataOffsets,
            Containers::BitArrayView{visibleNodeMask, 0, 14},
            visibleNodeEventData);
    }

    CORRADE_COMPARE_AS(Containers::arrayView(visibleNodeEventDataOffsets), Containers::arrayView<UnsignedInt>({
        0,  /* Node 0 */
        0,  /* Node 1 */
        0,  /* Node 2, 2 items from layers 2 and 3 */
        2,  /* Node 3, 3 items from layer 2 and 5 */
        5,  /* Node 4, 1 item from layer 2 */
        6,  /* Node 5 */
        6,  /* Node 6 */
        6,  /* Node 7, 1 item from layer 3 */
        7,  /* Node 8, 1 item from layer 5 which isn't visible */
        7,  /* Node 9 */
        7,  /* Node 10 */
        7,  /* Node 11 */
        7,  /* Node 12, 1 item from layer 2 */
        8,  /* Node 13 */
        8
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(visibleNodeEventData).prefix(Containers::arrayView(visibleNodeEventDataOffsets).back()), Containers::arrayView({
        /* Node 2 */
        dataHandle(layer3, 0, 0x555),
        dataHandle(layer2, 6, 0x777),
        /* Node 3. Order of items from the same layer matches inverse data ID
           order, not the order in which they were created or attached. */
        dataHandle(layer5, 2, 0x666),
        dataHandle(layer5, 1, 0x444),
        dataHandle(layer2, 3, 0x333),
        /* Node 4 */
        dataHandle(layer2, 2, 0x111),
        /* Node 7 */
        dataHandle(layer3, 2, 0x888),
        /* Node 8 isn't visible */
        /* Node 12 */
        dataHandle(layer2, 4, 0x222)
    }), TestSuite::Compare::Container);
}

void AbstractUserInterfaceImplementationTest::compactDraws() {
    Containers::Triple<UnsignedByte, Containers::Pair<UnsignedInt, UnsignedInt>, Containers::Pair<UnsignedInt, UnsignedInt>> draws[]{
        {8, {15, 3}, {1, 2}},
        {3, {226, 762}, {27, 46}},
        {4, {0, 0}, {2657, 0}},
        {7, {287628, 0}, {12, 0}},
        {8, {18, 2}, {1, 33}},
        {3, {0, 226}, {26, 78}},
        {4, {0, 6777}, {1, 233}},
        {4, {0, 0}, {0, 0}},
        {4, {6777, 2}, {233, 16}}
    };

    UnsignedInt count = Implementation::compactDrawsInPlace(
        Containers::stridedArrayView(draws)
            .slice(&Containers::Triple<UnsignedByte, Containers::Pair<UnsignedInt, UnsignedInt>, Containers::Pair<UnsignedInt, UnsignedInt>>::first),
        Containers::stridedArrayView(draws)
            .slice(&Containers::Triple<UnsignedByte, Containers::Pair<UnsignedInt, UnsignedInt>, Containers::Pair<UnsignedInt, UnsignedInt>>::second)
            .slice(&Containers::Pair<UnsignedInt, UnsignedInt>::first),
        Containers::stridedArrayView(draws)
            .slice(&Containers::Triple<UnsignedByte, Containers::Pair<UnsignedInt, UnsignedInt>, Containers::Pair<UnsignedInt, UnsignedInt>>::second)
            .slice(&Containers::Pair<UnsignedInt, UnsignedInt>::second),
        Containers::stridedArrayView(draws)
            .slice(&Containers::Triple<UnsignedByte, Containers::Pair<UnsignedInt, UnsignedInt>, Containers::Pair<UnsignedInt, UnsignedInt>>::third)
            .slice(&Containers::Pair<UnsignedInt, UnsignedInt>::first),
        Containers::stridedArrayView(draws)
            .slice(&Containers::Triple<UnsignedByte, Containers::Pair<UnsignedInt, UnsignedInt>, Containers::Pair<UnsignedInt, UnsignedInt>>::third)
            .slice(&Containers::Pair<UnsignedInt, UnsignedInt>::second));
    CORRADE_COMPARE_AS(count, Containers::arraySize(draws),
        TestSuite::Compare::LessOrEqual);
    CORRADE_COMPARE_AS(Containers::arrayView(draws).prefix(count), (Containers::arrayView<Containers::Triple<UnsignedByte, Containers::Pair<UnsignedInt, UnsignedInt>, Containers::Pair<UnsignedInt, UnsignedInt>>>({
        {8, {15, 3}, {1, 2}},
        {3, {226, 762}, {27, 46}},
        {8, {18, 2}, {1, 33}},
        {3, {0, 226}, {26, 78}},
        /* These two *could* get merged together eventually. So far aren't. */
        {4, {0, 6777}, {1, 233}},
        {4, {6777, 2}, {233, 16}}
    })), TestSuite::Compare::Container);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Whee::Test::AbstractUserInterfaceImplementationTest)
