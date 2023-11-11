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

    void cullVisibleNodesEdges();
    void cullVisibleNodes();

    void orderVisibleNodeData();
    void orderVisibleNodeDataNoLayers();
    void orderVisibleNodeDataNoValidLayers();

    void orderNodeDataForEventHandling();
    void orderNodeDataForEventHandlingNoLayers();
    void orderNodeDataForEventHandlingNoVisibleNodes();
    void orderNodeDataForEventHandlingNoVisibleNodesNoLayers();
    void orderNodeDataForEventHandlingAllLayers();
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

AbstractUserInterfaceImplementationTest::AbstractUserInterfaceImplementationTest() {
    addTests({&AbstractUserInterfaceImplementationTest::orderNodesBreadthFirst,

              &AbstractUserInterfaceImplementationTest::orderVisibleNodesDepthFirst,
              &AbstractUserInterfaceImplementationTest::orderVisibleNodesDepthFirstSingleBranch,
              &AbstractUserInterfaceImplementationTest::orderVisibleNodesDepthFirstNoTopLevelNodes,

              &AbstractUserInterfaceImplementationTest::visibleTopLevelNodeIndices});

    addInstancedTests({&AbstractUserInterfaceImplementationTest::cullVisibleNodesEdges},
        Containers::arraySize(CullVisibleNodesEdgesData));

    addTests({&AbstractUserInterfaceImplementationTest::cullVisibleNodes,

              &AbstractUserInterfaceImplementationTest::orderVisibleNodeData,
              &AbstractUserInterfaceImplementationTest::orderVisibleNodeDataNoLayers,
              &AbstractUserInterfaceImplementationTest::orderVisibleNodeDataNoValidLayers,

              &AbstractUserInterfaceImplementationTest::orderNodeDataForEventHandling,
              &AbstractUserInterfaceImplementationTest::orderNodeDataForEventHandlingNoLayers,
              &AbstractUserInterfaceImplementationTest::orderNodeDataForEventHandlingNoVisibleNodes,
              &AbstractUserInterfaceImplementationTest::orderNodeDataForEventHandlingNoVisibleNodesNoLayers,
              &AbstractUserInterfaceImplementationTest::orderNodeDataForEventHandlingAllLayers});
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
    } nodeOffsetsSizes[]{
        {{ 0.0f,  0.0f}, {4.0f, 4.0f}}, /*  0, top left */
        {{ 3.0f, -1.0f}, {3.0f, 3.0f}}, /*  1, top */
        {{ 5.0f,  0.0f}, {4.0f, 4.0f}}, /*  2, top right */
        {{ 0.0f,  0.0f}, {9.0f, 4.0f}}, /*  3, top left + right */
        {{-1.0f,  3.0f}, {3.0f, 3.0f}}, /*  4, left */
        {{ 7.0f,  3.0f}, {3.0f, 3.0f}}, /*  5, right */
        {{ 0.0f,  0.0f}, {4.0f, 9.0f}}, /*  6, left top + bottom */
        {{ 5.0f,  0.0f}, {4.0f, 9.0f}}, /*  7, right top + bottom */
        {{ 0.0f,  5.0f}, {9.0f, 4.0f}}, /*  8, bottom left + right */
        {{ 3.0f,  7.0f}, {3.0f, 3.0f}}, /*  9, bottom */
        {{ 0.0f,  5.0f}, {4.0f, 4.0f}}, /* 10, bottom left */
        {{ 5.0f,  5.0f}, {4.0f, 4.0f}}, /* 11, bottom right */

        {{ 4.0f,  4.0f}, {2.0f, 2.0f}}, /* 12, in the center */
        {{ 0.0f,  0.0f}, {9.0f, 9.0f}}, /* 13, covering whole area */

        {{-2.0f, -2.0f}, {5.0f, 2.0f}}, /* 14, outside top extended left */
        {{ 6.0f, -2.0f}, {5.0f, 2.0f}}, /* 15, outside top extended right */
        {{-2.0f, -2.0f}, {2.0f, 2.0f}}, /* 16, outside top left */
        {{ 9.0f, -2.0f}, {2.0f, 2.0f}}, /* 17, outside top right */
        {{ 3.0f, -2.0f}, {3.0f, 2.0f}}, /* 18, outside top */
        {{-2.0f,  0.0f}, {2.0f, 3.0f}}, /* 19, outside left extended top */
        {{ 9.0f,  0.0f}, {2.0f, 3.0f}}, /* 20, outside right extended top */
        {{-2.0f,  6.0f}, {2.0f, 3.0f}}, /* 21, outside left extended bottom */
        {{ 9.0f,  0.0f}, {2.0f, 3.0f}}, /* 22, outside right extended bottom */
        {{-2.0f,  3.0f}, {2.0f, 3.0f}}, /* 23, outside left */
        {{ 9.0f,  3.0f}, {2.0f, 3.0f}}, /* 24, outside right */
        {{-2.0f,  9.0f}, {5.0f, 2.0f}}, /* 25, outside bottom extended left */
        {{ 6.0f,  9.0f}, {5.0f, 2.0f}}, /* 26, outside bottom extended right */
        {{-2.0f,  9.0f}, {2.0f, 2.0f}}, /* 27, outside bottom left */
        {{ 9.0f,  9.0f}, {2.0f, 2.0f}}, /* 28, outside bottom right */
        {{ 3.0f,  9.0f}, {3.0f, 2.0f}}, /* 29, outside bottom */
        {{ 0.0f, -3.0f}, {9.0f, 3.0f}}, /* 30, outside top left + right */
        {{-3.0f,  0.0f}, {3.0f, 9.0f}}, /* 31, outside left top + bottom */
        {{ 9.0f,  0.0f}, {3.0f, 9.0f}}, /* 32, outside right top + bottom */
        {{ 0.0f,  9.0f}, {9.0f, 3.0f}}, /* 33, outside bottom left + right */
        {data.offset, data.size},       /* 34, clip node */
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
    Containers::MutableBitArrayView visibleNodeMask{visibleNodeMaskStorage, 0, Containers::arraySize(nodeOffsetsSizes)};

    Containers::Triple<Vector2, Vector2, UnsignedInt> clipStack[Containers::arraySize(nodeOffsetsSizes)];
    Implementation::cullVisibleNodesInto(
        Containers::stridedArrayView(nodeOffsetsSizes).slice(&Node::offset),
        Containers::stridedArrayView(nodeOffsetsSizes).slice(&Node::size),
        clipStack,
        Containers::stridedArrayView(nodeIdsChildrenCount).slice(&Children::id),
        Containers::stridedArrayView(nodeIdsChildrenCount).slice(&Children::count),
        visibleNodeMask);
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
}

void AbstractUserInterfaceImplementationTest::cullVisibleNodes() {
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
    Implementation::cullVisibleNodesInto(
        Containers::stridedArrayView(nodeOffsetsSizes).slice(&Node::offset),
        Containers::stridedArrayView(nodeOffsetsSizes).slice(&Node::size),
        clipStack,
        Containers::stridedArrayView(nodeIdsChildrenCount).slice(&Children::id),
        Containers::stridedArrayView(nodeIdsChildrenCount).slice(&Children::count),
        visibleNodeMask);
    CORRADE_COMPARE_AS(visibleNodeMask,
        Containers::stridedArrayView({
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
        }).sliceBit(0), TestSuite::Compare::Container);
}

void AbstractUserInterfaceImplementationTest::orderVisibleNodeData() {
    /* Ordered visible node hierarchy */
    const Containers::Pair<UnsignedInt, UnsignedInt> visibleNodeIdsChildrenCount[]{
        /* No children, three data attachments from layers 2 and 5 */
        {3, 0},

        /* Several nested children */
        {13, 5},
            {9, 3},
                {1, 1},
                    {4, 0}, /* One data attached from layer 2 */
                {2, 0}, /* One data attached from layer 1, one from layer 2,
                           one from layer 3 not for drawing */
            {7, 0}, /* One data attached from layer 1, one from layer 3 not for
                       drawing */

        /* One child, no data attachment, should get skipped */
        {11, 1},
            {10, 0},

        /* No children, one data attachment from layer 2 */
        {12, 0},
    };

    /* Node data assignments. Node and data generations don't matter in any
       way, the same node ID can even have different generations. */
    LayerHandle layer1 = layerHandle(1, 0xef);
    LayerHandle layer2 = layerHandle(2, 0xfe);
    /* Layer 0 doesn't have any data referenced, layer 3 doesn't have a Draw
       feature, layer 4 is referenced only by a node that isn't in the visible
       hierarchy */
    LayerHandle layer3 = layerHandle(3, 0xfe);
    LayerHandle layer4 = layerHandle(4, 0xde);
    LayerHandle layer5 = layerHandle(5, 0xad);
    const Containers::Pair<NodeHandle, DataHandle> data[]{
        {nodeHandle(3, 0xfef),  dataHandle(layer2, 376, 0xded)},
        {nodeHandle(4, 0xbab),  dataHandle(layer2, 22, 0x0b0)},
        {nodeHandle(3, 0xc0c),  dataHandle(layer5, 1777, 0xfff)},
        /* Nodes 0 and 8 aren't in the visible hierarchy so the assignments
           get ignored */
        {nodeHandle(0, 0xefe),  dataHandle(layer4, 2020, 0x777)},
        {nodeHandle(8, 0xbbb),  dataHandle(layer5, 191836, 0xccc)},
        {nodeHandle(7, 0xf0f),  dataHandle(layer3, 226622, 0xbbb)},
        {nodeHandle(3, 0xc0c),  dataHandle(layer5, 1776, 0xfff)},
        {nodeHandle(7, 0xeee),  dataHandle(layer1, 22777, 0x000)},
        {nodeHandle(12, 0xccc), dataHandle(layer2, 79, 0xcec)},
        {nodeHandle(2, 0xddd),  dataHandle(layer2, 87878, 0x999)},
        {nodeHandle(2, 0xaba),  dataHandle(layer1, 333, 0xaaa)},
        /* Nodes 5, 6 aren't present anywhere */
        {nodeHandle(2, 0xefe),  dataHandle(layer3, 3675, 0xaba)},
    };

    /* The layers are in order 4, 2, 3, 1, 5. Handle generation isn't used or
       checked for anything except for comparing to the first handle again. */
    LayerHandle layerFirst = layerHandle(4, 0x00);
    LayerHandle layersNext[]{
        LayerHandle{},
        layerHandle(5, 0x11),
        layerHandle(3, 0x44),
        layerHandle(1, 0x22),
        layerHandle(2, 0x33),
        layerHandle(4, 0x00)
    };
    LayerFeatures layerFeatures[]{
        {},
        LayerFeature::Draw,
        LayerFeature::Event|LayerFeature::Draw,
        LayerFeature::Event,
        LayerFeature::Draw,
        LayerFeature::Draw,
    };

    /* Everything except nodes 0 and 8 is visible */
    UnsignedShort visibleNodeMask[]{0xffff & ~(1 << 0) & ~(1 << 8)};

    UnsignedInt visibleNodeDataOffsets[15]{};
    DataHandle visibleNodeData[Containers::arraySize(data)];
    UnsignedInt dataToUpdateLayerOffsets[6 + 1]{};
    UnsignedInt previousDataToUpdateLayerOffsets[6 + 1];
    Containers::Pair<UnsignedInt, UnsignedInt> dataIdsNodeIdsToUpdate[Containers::arraySize(data)];
    Containers::Triple<UnsignedByte, UnsignedInt, UnsignedInt> dataLayerIdsDataOffsetsSizesToDraw[Containers::arraySize(data)];

    UnsignedInt drawCount = Implementation::orderVisibleNodeDataInto(
        Containers::stridedArrayView(visibleNodeIdsChildrenCount).slice(&Containers::Pair<UnsignedInt, UnsignedInt>::first),
        Containers::stridedArrayView(visibleNodeIdsChildrenCount).slice(&Containers::Pair<UnsignedInt, UnsignedInt>::second),
        Containers::stridedArrayView(data).slice(&Containers::Pair<NodeHandle, DataHandle>::first),
        Containers::stridedArrayView(data).slice(&Containers::Pair<NodeHandle, DataHandle>::second),
        layersNext,
        layerFirst,
        layerFeatures,
        Containers::BitArrayView{visibleNodeMask, 0, 14},
        visibleNodeDataOffsets,
        visibleNodeData,
        dataToUpdateLayerOffsets,
        previousDataToUpdateLayerOffsets,
        Containers::stridedArrayView(dataIdsNodeIdsToUpdate).slice(&Containers::Pair<UnsignedInt, UnsignedInt>::first),
        Containers::stridedArrayView(dataIdsNodeIdsToUpdate).slice(&Containers::Pair<UnsignedInt, UnsignedInt>::second),
        Containers::stridedArrayView(dataLayerIdsDataOffsetsSizesToDraw).slice(&Containers::Triple<UnsignedByte, UnsignedInt, UnsignedInt>::first),
        Containers::stridedArrayView(dataLayerIdsDataOffsetsSizesToDraw).slice(&Containers::Triple<UnsignedByte, UnsignedInt, UnsignedInt>::second),
        Containers::stridedArrayView(dataLayerIdsDataOffsetsSizesToDraw).slice(&Containers::Triple<UnsignedByte, UnsignedInt, UnsignedInt>::third));

    CORRADE_COMPARE_AS(Containers::arrayView(visibleNodeDataOffsets), Containers::arrayView<UnsignedInt>({
        0,  /* Node 0 */
        0,  /* Node 1 */
        0,  /* Node 2, three data */
        3,  /* Node 3, three data */
        6,  /* Node 4, one data */
        7,  /* Node 5 */
        7,  /* Node 6 */
        7,  /* Node 7, two data */
        9,  /* Node 8 */
        9,  /* Node 9 */
        9,  /* Node 10 */
        9,  /* Node 11 */
        9,  /* Node 12, one data */
        10, /* Node 13 */
        10
    }), TestSuite::Compare::Container);

    CORRADE_COMPARE_AS(Containers::arrayView(visibleNodeData).prefix(visibleNodeDataOffsets[Containers::arraySize(visibleNodeDataOffsets) - 1]), Containers::arrayView<DataHandle>({
        /* Node 2 */
        dataHandle(layer2, 87878, 0x999),
        dataHandle(layer1, 333, 0xaaa),
        dataHandle(layer3, 3675, 0xaba),
        /* Node 3 */
        dataHandle(layer2, 376, 0xded),
        dataHandle(layer5, 1777, 0xfff),
        dataHandle(layer5, 1776, 0xfff),
        /* Node 4 */
        dataHandle(layer2, 22, 0x0b0),
        /* Node 7 */
        dataHandle(layer3, 226622, 0xbbb),
        dataHandle(layer1, 22777, 0x000),
        /* Node 12 */
        dataHandle(layer2, 79, 0xcec)
    }), TestSuite::Compare::Container);

    CORRADE_COMPARE_AS(Containers::arrayView(dataToUpdateLayerOffsets), Containers::arrayView<UnsignedInt>({
        0,
        0,  /* Layer 0 has nothing */
        2,  /* Layer 1 has 2 items */
        6,  /* Layer 2 has 4 items */
        8,  /* Layer 3 has two items but doesn't have a Draw feature, so
               these are then excluded from the draw call list */
        8,  /* Layer 4 has one item that isn't in the hierarchy, so nothing */
        10, /* Layer 5 has 2 items plus one that isn't in the hierarchy, so
               nothing */
    }), TestSuite::Compare::Container);

    /* Order inside layers is matching visible node order */
    CORRADE_COMPARE_AS(
        Containers::arrayView(dataIdsNodeIdsToUpdate)
            /* The last element is the total filled size of the output array */
            .prefix(Containers::arrayView(dataToUpdateLayerOffsets).back()),
        (Containers::arrayView<Containers::Pair<UnsignedInt, UnsignedInt>>({
            /* Layer 1 */
            {333, 2},
            {22777, 7},
            /* Layer 2 */
            {376, 3},
            {22, 4},
            {87878, 2},
            {79, 12},
            /* Layer 3, but those aren't included in the draws below */
            {3675, 2},
            {226622, 7},
            /* Layer 4 has nothing */
            /* Layer 5, same node, order in which the assignments were found */
            {1777, 3},
            {1776, 3},
        })),
        TestSuite::Compare::Container);

    CORRADE_COMPARE_AS(drawCount,
        Containers::arraySize(dataLayerIdsDataOffsetsSizesToDraw),
        TestSuite::Compare::LessOrEqual);
    CORRADE_COMPARE_AS(
        Containers::arrayView(dataLayerIdsDataOffsetsSizesToDraw)
            .prefix(drawCount),
        (Containers::arrayView<Containers::Triple<UnsignedByte, UnsignedInt, UnsignedInt>>({
            /* For top-level node 3 offset 2 from layer 2 and offset 8, 9
               from layer 5 is drawn */
            {2, 2, 1},
            {5, 8, 2},
            /* For top-level node 13 draws offset 3, 4 from layer 2 and
               offset 0, 1 from layer 1 is drawn */
            {2, 3, 2},
            {1, 0, 2},
            /* For top-level node 11 nothing is drawn */
            /* Top-level node 12 draws offset 5 from layer 2 */
            {2, 5, 1}
    })), TestSuite::Compare::Container);

    /* Each index in the draw data should appear exactly once */
    Containers::BitArray dataDrawn{DirectInit, Containers::arrayView(dataToUpdateLayerOffsets).back(), false};
    for(const Containers::Triple<UnsignedByte, UnsignedInt, UnsignedInt>& i: dataLayerIdsDataOffsetsSizesToDraw) {
        CORRADE_ITERATION(i);
        for(UnsignedInt j = 0; j != i.third(); ++j) {
            CORRADE_ITERATION(j);
            CORRADE_VERIFY(!dataDrawn[i.second() + j]);
            dataDrawn.set(i.second() + j);
        }
    }

    /* Two items from layer 3 that doesn't have LayerFeature::Draw should not
       be present */
    CORRADE_COMPARE(dataDrawn.count(), Containers::arrayView(dataToUpdateLayerOffsets).back() - 2);
}

void AbstractUserInterfaceImplementationTest::orderVisibleNodeDataNoLayers() {
    UnsignedInt visibleNodeDataOffsets[1];
    UnsignedInt dataToUpdateLayerOffsets[1]{0xdeadbeef};
    UnsignedInt previousDataToUpdateLayerOffsets[1];
    UnsignedInt drawCount = Implementation::orderVisibleNodeDataInto(
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        LayerHandle::Null,
        nullptr,
        nullptr,
        visibleNodeDataOffsets,
        nullptr,
        dataToUpdateLayerOffsets,
        previousDataToUpdateLayerOffsets,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr);
    CORRADE_COMPARE(drawCount, 0);
    CORRADE_COMPARE(dataToUpdateLayerOffsets[0], 0);
}

void AbstractUserInterfaceImplementationTest::orderVisibleNodeDataNoValidLayers() {
    /* Compared to orderVisibleNodeDataNoLayers() above, there's a non-empty
       layer list, but all of them are removed and thus not part of the
       order */
    LayerHandle layersNext[3];
    UnsignedInt visibleNodeDataOffsets[1];
    UnsignedInt dataToUpdateLayerOffsets[4]{
        0xdeadbeef, 0xdeadbeef, 0xdeadbeef, 0xdeadbeef
    };
    UnsignedInt previousDataToUpdateLayerOffsets[4];
    UnsignedInt drawCount = Implementation::orderVisibleNodeDataInto(
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        layersNext,
        LayerHandle::Null,
        nullptr,
        nullptr,
        visibleNodeDataOffsets,
        nullptr,
        dataToUpdateLayerOffsets,
        previousDataToUpdateLayerOffsets,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr);
    CORRADE_COMPARE(drawCount, 0);
    CORRADE_COMPARE_AS(Containers::arrayView(dataToUpdateLayerOffsets), Containers::arrayView<UnsignedInt>({
        0, 0, 0, 0
    }), TestSuite::Compare::Container);
}

void AbstractUserInterfaceImplementationTest::orderNodeDataForEventHandling() {
    UnsignedInt visibleNodeDataOffsets[]{
        0,
        4,
        4, /* Second node has no data */
        8,
    };
    /* Handle generations aren't used for anything here */
    DataHandle visibleNodeData[]{
        /* First node has one data from layer 3 that doesn't have the Event
           feature */
        dataHandle(layerHandle(2, 0xef), 1776, 0xded),
        dataHandle(layerHandle(5, 0xfe), 62326, 0xf0f),
        dataHandle(layerHandle(3, 0xef), 282912, 0xdfd),
        dataHandle(layerHandle(1, 0xfe), 275286, 0xe0e),
        /* Second node has nothing */
        /* Third node has has three data from layer 1 */
        dataHandle(layerHandle(1, 0xbe), 78, 0xdad),
        dataHandle(layerHandle(4, 0xeb), 2267, 0x000),
        dataHandle(layerHandle(1, 0xee), 79, 0xbeb),
        dataHandle(layerHandle(1, 0xbb), 80, 0xcec),
    };
    /* The layers are in order 4, 2, 3, 1, 5, same as in orderVisibleNodeData()
       above, only that this points the other direction. Handle generation
       isn't used or checked for anything except for comparing to the first
       handle again. */
    LayerHandle layerLast = layerHandle(5, 0x11);
    LayerHandle layersPrevious[]{
        LayerHandle{},
        layerHandle(3, 0x44),
        layerHandle(4, 0x00),
        layerHandle(2, 0x33),
        layerHandle(5, 0x11),
        layerHandle(1, 0x22)
    };
    LayerFeatures layerFeatures[]{
        {},
        LayerFeature::Event|LayerFeature::Draw,
        LayerFeature::Event,
        LayerFeature::Draw,
        LayerFeature::Event,
        LayerFeature::Event|LayerFeature::Draw,
    };

    UnsignedInt visibleNodeEventDataOffsets[4]{};
    DataHandle visibleNodeEventData[7];
    Implementation::orderNodeDataForEventHandling(
        visibleNodeDataOffsets,
        visibleNodeData,
        layersPrevious,
        layerLast,
        layerFeatures,
        visibleNodeEventDataOffsets,
        visibleNodeEventData);
    CORRADE_COMPARE_AS(Containers::arrayView(visibleNodeEventDataOffsets), Containers::arrayView<UnsignedInt>({
        0,
        3,
        3,
        7
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(visibleNodeEventData), Containers::arrayView({
        /* First node */
        dataHandle(layerHandle(5, 0xfe), 62326, 0xf0f),
        dataHandle(layerHandle(1, 0xfe), 275286, 0xe0e),
        /* Data from layer 3 that doesn't have LayerFeature::Event omitted */
        dataHandle(layerHandle(2, 0xef), 1776, 0xded),
        /* Second node */
        /* Third node. Items from the same layer are in the order they were
           added, but reversed. */
        dataHandle(layerHandle(1, 0xbb), 80, 0xcec),
        dataHandle(layerHandle(1, 0xee), 79, 0xbeb),
        dataHandle(layerHandle(1, 0xbe), 78, 0xdad),
        dataHandle(layerHandle(4, 0xeb), 2267, 0x000),
    }), TestSuite::Compare::Container);
}

void AbstractUserInterfaceImplementationTest::orderNodeDataForEventHandlingNoLayers() {
    UnsignedInt visibleNodeDataOffsets[]{
        0,
        0,
        0
    };
    UnsignedInt visibleNodeEventDataOffsets[]{
        0xdeadbeef,
        0xdeadbeef,
        0xdeadbeef
    };
    Implementation::orderNodeDataForEventHandling(
        visibleNodeDataOffsets,
        nullptr,
        nullptr,
        LayerHandle::Null,
        nullptr,
        visibleNodeEventDataOffsets,
        nullptr);
    CORRADE_COMPARE_AS(Containers::arrayView(visibleNodeEventDataOffsets), Containers::arrayView<UnsignedInt>({
        0, 0, 0
    }), TestSuite::Compare::Container);
}

void AbstractUserInterfaceImplementationTest::orderNodeDataForEventHandlingNoVisibleNodes() {
    UnsignedInt visibleNodeDataOffsets[]{
        0
    };
    LayerHandle layerLast = layerHandle(5, 0x11);
    LayerHandle layersPrevious[]{
        LayerHandle{},
        layerHandle(2, 0x33),
        layerHandle(4, 0x00),
        LayerHandle{},
        layerHandle(5, 0x11),
        layerHandle(1, 0x22)
    };
    LayerFeatures layerFeatures[]{
        {},
        LayerFeature::Event,
        LayerFeature::Event,
        {},
        LayerFeature::Event,
        LayerFeature::Event,
    };

    /* It shouldn't crash or loop indefinitely */
    UnsignedInt visibleNodeEventDataOffsets[1]{};
    Implementation::orderNodeDataForEventHandling(
        visibleNodeDataOffsets,
        nullptr,
        layersPrevious,
        layerLast,
        layerFeatures,
        visibleNodeEventDataOffsets,
        nullptr);
    CORRADE_VERIFY(true);
}

void AbstractUserInterfaceImplementationTest::orderNodeDataForEventHandlingNoVisibleNodesNoLayers() {
    UnsignedInt visibleNodeDataOffsets[1];
    UnsignedInt visibleNodeEventDataOffsets[1]{0xdeadbeef};
    Implementation::orderNodeDataForEventHandling(
        visibleNodeDataOffsets,
        nullptr,
        nullptr,
        LayerHandle::Null,
        nullptr,
        visibleNodeEventDataOffsets,
        nullptr);
    CORRADE_COMPARE(visibleNodeEventDataOffsets[0], 0);
}

void AbstractUserInterfaceImplementationTest::orderNodeDataForEventHandlingAllLayers() {
    /* All layers used */
    LayerHandle layerLast = layerHandle(0, 0x1);
    LayerHandle layersPrevious[1 << Implementation::LayerHandleIdBits];
    for(std::size_t i = 0; i != Containers::arraySize(layersPrevious) - 1; ++i)
        layersPrevious[i] = layerHandle(i + 1, 0x1);
    layersPrevious[Containers::arraySize(layersPrevious) - 1] = layerHandle(0, 0x1);
    LayerFeatures layerFeatures[1 << Implementation::LayerHandleIdBits];
    for(LayerFeatures& i: layerFeatures)
        i = LayerFeature::Event;

    /* Data in the last two layers */
    UnsignedInt visibleNodeDataOffsets[]{
        0,
        2
    };
    /* Handle generations aren't used for anything here */
    DataHandle visibleNodeData[]{
        dataHandle(layerHandle((1 << Implementation::LayerHandleIdBits) - 1, 0x1), 27542, 0xb0b),
        dataHandle(layerHandle((1 << Implementation::LayerHandleIdBits) - 2, 0x1), 24567, 0xded),
    };

    /* Should not cause any memory corruption due to the internal statically
       sized arrays being too tiny or something */
    UnsignedInt visibleNodeEventDataOffsets[2]{};
    DataHandle visibleNodeEventData[2];
    Implementation::orderNodeDataForEventHandling(
        visibleNodeDataOffsets,
        visibleNodeData,
        layersPrevious,
        layerLast,
        layerFeatures,
        visibleNodeEventDataOffsets,
        visibleNodeEventData);
    CORRADE_COMPARE_AS(Containers::arrayView(visibleNodeDataOffsets), Containers::arrayView<UnsignedInt>({
        0,
        2
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(visibleNodeEventData), Containers::arrayView({
        dataHandle(layerHandle(254, 0x1), 24567, 0xded),
        dataHandle(layerHandle(255, 0x1), 27542, 0xb0b),
    }), TestSuite::Compare::Container);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Whee::Test::AbstractUserInterfaceImplementationTest)
