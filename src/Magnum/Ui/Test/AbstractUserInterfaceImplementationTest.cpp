/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023, 2024
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
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/Pair.h>
#include <Corrade/Containers/StridedBitArrayView.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Container.h>
#include <Corrade/TestSuite/Compare/Numeric.h>
#include <Magnum/Math/Vector2.h>

#include "Magnum/Ui/AbstractLayer.h" /* LayerFeatures */
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/Implementation/abstractUserInterface.h"
#include "Magnum/Ui/Implementation/orderNodesBreadthFirstInto.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct AbstractUserInterfaceImplementationTest: TestSuite::Tester {
    explicit AbstractUserInterfaceImplementationTest();

    void orderNodesBreadthFirst();

    void orderVisibleNodesDepthFirst();
    void orderVisibleNodesDepthFirstSingleBranch();
    void orderVisibleNodesDepthFirstNoTopLevelNodes();

    void visibleTopLevelNodeIndices();

    void propagateNodeFlagToChildren();

    void discoverTopLevelLayoutNodesSingleLayouterPerNode();
    void discoverTopLevelLayoutNodesMultipleLayoutersPerNode();
    void discoverTopLevelLayoutNodesNoLayouters();
    void discoverTopLevelLayoutNodesNoVisibleNodes();
    void discoverTopLevelLayoutNodesSingleNode();
    void discoverTopLevelLayoutNodesSingleNodeLayoutChain();

    void fillLayoutUpdateMasks();
    void fillLayoutUpdateMasksNoLayouters();

    void cullVisibleNodesClipRects();
    void cullVisibleNodesEdges();
    void cullVisibleNodes();
    void cullVisibleNodesNoTopLevelNodes();

    void orderVisibleNodeData();
    void orderVisibleNodeDataNoTopLevelNodes();

    void countOrderNodeDataForEventHandling();

    void compactDraws();

    void compositeRectsEdges();
    void compositingRects();

    void partitionedAnimatorsInsert();
    void partitionedAnimatorsInsertNoLayers();
    void partitionedAnimatorsRemove();
    void partitionedAnimatorsRemoveNoLayers();
    void partitionedAnimatorsGet();
    void partitionedAnimatorsGetNoLayers();
    void partitionedAnimatorsCreateLayer();
    void partitionedAnimatorsRemoveLayer();
};

const struct {
    const char* name;
    /* The 2D node layout list is defined in the function because it's less
       annoying that way */
    Containers::Array<UnsignedInt> topLevelLayoutOffsets;
    Containers::Array<UnsignedByte> topLevelLayoutLayouterIds;
    Containers::Array<UnsignedInt> topLevelLayoutIds;
} DiscoverTopLevelLayoutNodesMultipleLayoutersPerNodeData[]{
    /* node, layouter, calculated level
       1    AB      01
       2    ab DE   01 23
       3     bC      12
       4      c       2
       5    A       0
       6    a       0     */
    {"same layouter, independent run", {InPlaceInit, {
            0,    2,    3,    4,    5,    6
        }}, {InPlaceInit, {
            0xaa, 0xbb, 0xcc, 0xdd, 0xee,
        }}, {InPlaceInit, {
            0xaaa1, 0xaaa5, 0xbbb1, 0xccc3, 0xddd2, 0xeee2
        }}},
    /* 1    AB      01
       2    ab DE   01 23
       3     bC      12
       4      c       2
       5    A c     3 2
       6    a       3     */
    {"same layouter, dependent run", {InPlaceInit, {
            0,    1,    2,    3,    4,    5,    6
        }}, {InPlaceInit, {
            0xaa, 0xbb, 0xcc, 0xdd, 0xaa, 0xee,
        }}, {InPlaceInit, {
            0xaaa1, 0xbbb1, 0xccc3, 0xddd2, 0xaaa5, 0xeee2
        }}},
    /* 1    AB      01
       2    ab DE   01 23
       3     bCd     132
       4      c       3
       5    A       0
       6    a       0     */
    {"parent dependency", {InPlaceInit, {
            0,    2,    3,    4,    5,    6
        }}, {InPlaceInit, {
            0xaa, 0xbb, 0xdd, 0xcc, 0xee,
        }}, {InPlaceInit, {
            0xaaa1, 0xaaa5, 0xbbb1, 0xddd2, 0xccc3, 0xeee2
        }}},
    /** @todo this looks strangely suboptimal, fix to be the same as above */
    /* 1    AB      01
       2    ab DE   01 23
       3     bCd     132
       4      c       3
       5    A       4
       6    a       4     */
    {"same layouter, transitive parent dependency", {InPlaceInit, {
            0,    1,    2,    3,    4,    5,    6
        }}, {InPlaceInit, {
            0xaa, 0xbb, 0xdd, 0xcc, 0xee, 0xaa
        }}, {InPlaceInit, {
            0xaaa1, 0xbbb1, 0xddd2, 0xccc3, 0xeee2, 0xaaa5
        }}},
};

const struct {
    TestSuite::TestCaseDescriptionSourceLocation name;
    Vector2 uiOffset, uiSize;
    Containers::Array<Containers::Pair<UnsignedInt, UnsignedInt>> nodeIdsChildrenCount;
    Containers::Array<Containers::Triple<Vector2, Vector2, NodeFlags>> nodeOffsetsSizesFlags;
    Containers::Array<bool> expectedVisible;
    Containers::Array<Containers::Triple<Vector2, Vector2, UnsignedInt>> expectedClipRects;
} CullVisibleNodesClipRectsData[]{
    {"single non-clipping node", {}, {100.0f, 100.0f}, {InPlaceInit, {
            {0, 0},
        }}, {InPlaceInit, {
            {{3.0f, 4.0f}, {5.0f, 6.0f}, {}},
        }}, {InPlaceInit, {
            true
        }}, {InPlaceInit, {
            /* Verifies that no OOB access happens internally */
            {{}, {}, 1}
        }}},
    {"single clipping node", {}, {100.0f, 100.0f}, {InPlaceInit, {
            {0, 0},
        }}, {InPlaceInit, {
            {{3.0f, 4.0f}, {5.0f, 6.0f}, NodeFlag::Clip},
        }}, {InPlaceInit, {
            true
        }}, {InPlaceInit, {
            /* Verifies that no OOB access happens internally here as well */
            {{3.0f, 4.0f}, {5.0f, 6.0f}, 1}
        }}},
    {"single non-clipping node overlapping window edges", {4.0f, 5.0f}, {1.0f, 1.0f}, {InPlaceInit, {
            {0, 0},
        }}, {InPlaceInit, {
            {{3.0f, 4.0f}, {5.0f, 6.0f}, {}},
        }}, {InPlaceInit, {
            true
        }}, {InPlaceInit, {
            /* Should be empty, not the window offset/size */
            {{}, {}, 1}
        }}},
    {"single clipping node overlapping window edges", {4.0f, 5.0f}, {10.0f, 1.0f}, {InPlaceInit, {
            {0, 0},
        }}, {InPlaceInit, {
            {{3.0f, 4.0f}, {5.0f, 6.0f}, NodeFlag::Clip},
        }}, {InPlaceInit, {
            true
        }}, {InPlaceInit, {
            /* Gets joined with the window rect */
            {{4.0f, 5.0f}, {4.0f, 1.0f}, 1}
        }}},
    {"multiple non-clipping top-level nodes", {}, {100.0f, 100.0f}, {InPlaceInit, {
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
               calls as well. All should be empty, not the window
               offset/size. */
            {{}, {}, 1},
            {{}, {}, 1},
            {{0.0f, 1.0f}, {2.0f, 3.0f}, 1},
            {{}, {}, 1},
            {{}, {}, 1}
        }}},
    {"skip a fully culled clipping node including children", {}, {100.0f, 100.0f}, {InPlaceInit, {
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
    {"return to parent clip rect", {}, {100.0f, 100.0f}, {InPlaceInit, {
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
    {"return to parent clip rect, invisible node", {}, {100.0f, 100.0f}, {InPlaceInit, {
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
            {{}, {}, 1} /* should be empty, not the window offset/size */
        }}},
    {"return to parent clip rect, invisible node at the end", {}, {100.0f, 100.0f}, {InPlaceInit, {
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
    Vector2 uiOffset, uiSize;
    Vector2 clipNodeOffset, clipNodeSize;
    NodeFlags clipNodeFlags;
    Vector2 clipRectOffset, clipRectSize;
    bool allVisible;
} CullVisibleNodesEdgesData[]{
    {"",
        {-3.0f, -3.0f}, {16.0f, 16.0f},
        {1.0f, 1.0f}, {7.0f, 7.0f}, {},
        {}, {}, true},
    {"clipping node",
        {-3.0f, -3.0f}, {16.0f, 16.0f},
        {1.0f, 1.0f}, {7.0f, 7.0f}, NodeFlag::Clip,
        {1.0f, 1.0f}, {7.0f, 7.0f}, false},
    {"clipping node, touching edges",
        {-3.0f, -3.0f}, {16.0f, 16.0f},
        {0.0f, 0.0f}, {9.0f, 9.0f}, NodeFlag::Clip,
        {0.0f, 0.0f}, {9.0f, 9.0f}, false},
    {"clipping node, touching everything",
        {-3.0f, -3.0f}, {16.0f, 16.0f},
        {-0.01f, -0.01f}, {9.02f, 9.02f}, NodeFlag::Clip,
        {-0.01f, -0.01f}, {9.02f, 9.02f}, true},
    {"culled by window edges",
        {1.0f, 1.0f}, {7.0f, 7.0f},
        {1.0f, 1.0f}, {7.0f, 7.0f}, {},
        {}, {}, false},
    {"culled by window edges, touching edges",
        {0.0f, 0.0f}, {9.0f, 9.0f},
        {1.0f, 1.0f}, {7.0f, 7.0f}, {},
        {}, {}, false},
    {"culled by window edges, touching everything",
        {-0.01f, -0.01f}, {9.02f, 9.02f},
        {1.0f, 1.0f}, {7.0f, 7.0f}, {},
        {}, {}, true},
};

const struct {
    const char* name;
    Vector2 uiOffset, uiSize;
    NodeFlags flags[15];
    bool visible[15];
    Containers::Array<Containers::Triple<Vector2, Vector2, UnsignedInt>> clipRects;
} CullVisibleNodesData[]{
    {"all clipping", {}, {100.0f, 100.0f}, {
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
    {"no clipping", {}, {100.0f, 100.0f}, {
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
    {"no clipping, culled by window edges", {2.0f, 0.0f}, {100.0f, 6.0f}, {
        {},             {},             {},             {},             /* 0-3 */
        {},             {},             {},             {},             /* 4-7 */
        {},             {},             {},             {},             /* 8-11 */
        {},             {},             {},                             /* 12-14 */
    }, {
        false, /* 0, outside of the window area */
        true,  /* 1 */
        true,  /* 2 */
        false, /* 3, outside of the window area */
        true,  /* 4 */
        true,  /* 5 */
        true,  /* 6, child of a culled node but because it doesn't clip it's
                     still partially visible in the window area */
        true,  /* 7 */
        true,  /* 8 */
        true,  /* 9 */
        true,  /* 10 */
        true,  /* 11 */
        true,  /* 12 */
        true,  /* 13 */
        true,  /* 14 */
    }, {InPlaceInit, {
        {{}, {}, 1},  /* Top-level node 3 */
        {{}, {}, 11}, /* Top-level node 7 */
        {{}, {}, 3},  /* Top-level node 5 */
    }}},
    {"special cases", {}, {100.0f, 100.0f}, {
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

              &AbstractUserInterfaceImplementationTest::visibleTopLevelNodeIndices,

              &AbstractUserInterfaceImplementationTest::propagateNodeFlagToChildren,

              &AbstractUserInterfaceImplementationTest::discoverTopLevelLayoutNodesSingleLayouterPerNode});

    addInstancedTests({&AbstractUserInterfaceImplementationTest::discoverTopLevelLayoutNodesMultipleLayoutersPerNode},
        Containers::arraySize(DiscoverTopLevelLayoutNodesMultipleLayoutersPerNodeData));

    addTests({&AbstractUserInterfaceImplementationTest::discoverTopLevelLayoutNodesNoLayouters,
              &AbstractUserInterfaceImplementationTest::discoverTopLevelLayoutNodesNoVisibleNodes,
              &AbstractUserInterfaceImplementationTest::discoverTopLevelLayoutNodesSingleNode,
              &AbstractUserInterfaceImplementationTest::discoverTopLevelLayoutNodesSingleNodeLayoutChain,

              &AbstractUserInterfaceImplementationTest::fillLayoutUpdateMasks,
              &AbstractUserInterfaceImplementationTest::fillLayoutUpdateMasksNoLayouters});

    addInstancedTests({&AbstractUserInterfaceImplementationTest::cullVisibleNodesClipRects},
        Containers::arraySize(CullVisibleNodesClipRectsData));

    addInstancedTests({&AbstractUserInterfaceImplementationTest::cullVisibleNodesEdges},
        Containers::arraySize(CullVisibleNodesEdgesData));

    addInstancedTests({&AbstractUserInterfaceImplementationTest::cullVisibleNodes},
        Containers::arraySize(CullVisibleNodesData));

    addTests({&AbstractUserInterfaceImplementationTest::cullVisibleNodesNoTopLevelNodes,

              &AbstractUserInterfaceImplementationTest::orderVisibleNodeData,
              &AbstractUserInterfaceImplementationTest::orderVisibleNodeDataNoTopLevelNodes,

              &AbstractUserInterfaceImplementationTest::countOrderNodeDataForEventHandling,

              &AbstractUserInterfaceImplementationTest::compactDraws,

              &AbstractUserInterfaceImplementationTest::compositeRectsEdges,
              &AbstractUserInterfaceImplementationTest::compositingRects,

              &AbstractUserInterfaceImplementationTest::partitionedAnimatorsInsert,
              &AbstractUserInterfaceImplementationTest::partitionedAnimatorsInsertNoLayers,
              &AbstractUserInterfaceImplementationTest::partitionedAnimatorsRemove,
              &AbstractUserInterfaceImplementationTest::partitionedAnimatorsRemoveNoLayers,
              &AbstractUserInterfaceImplementationTest::partitionedAnimatorsGet,
              &AbstractUserInterfaceImplementationTest::partitionedAnimatorsGetNoLayers,
              &AbstractUserInterfaceImplementationTest::partitionedAnimatorsCreateLayer,
              &AbstractUserInterfaceImplementationTest::partitionedAnimatorsRemoveLayer});
}

void AbstractUserInterfaceImplementationTest::orderNodesBreadthFirst() {
    /* The handle generations aren't used for anything here so can be
       arbitrary */
    const struct Node {
        NodeHandle parent;
    } nodes[]{
        /* Forward parent reference */
        {nodeHandle(9, 0x123)},           /* 0 */
        /* Root elements */
        {NodeHandle::Null},               /* 1 */
        {NodeHandle::Null},               /* 2 */
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
        {NodeHandle::Null}                /* 9 */
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
        NodeHandle parent;
        UnsignedInt order;
        NodeFlags flags;
    } nodes[]{
        /* Forward parent reference */
        {nodeHandle(13, 0x123), ~UnsignedInt{}, {}},            /* 0 */
        /* Root elements, the middle one isn't included in the order and its ID
           can again be whatever for purposes of this algorithm */
        {NodeHandle::Null, 2, {}},                              /* 1 */
        {NodeHandle::Null, 0xfefe, {}},                         /* 2 */
        {NodeHandle::Null, 7, {}},                              /* 3 */
        /* Backward parent reference */
        {nodeHandle(1, 0xabc), ~UnsignedInt{}, {}},             /* 4 */
        /* Deep hierarchy */
        {nodeHandle(4, 0x1), ~UnsignedInt{}, {}},               /* 5 */
        {nodeHandle(5, 0xfff), ~UnsignedInt{}, {}},             /* 6 */
        /* Hidden nodes, the first is top-level */
        {NodeHandle::Null, 3, NodeFlag::Hidden},                /* 7 */
        {nodeHandle(1, 0xebe), ~UnsignedInt{}, NodeFlag::Hidden}, /* 8 */
        /* Multiple children */
        {nodeHandle(1, 0x1), ~UnsignedInt{}, {}},               /* 9 */
        {nodeHandle(11, 0x1), ~UnsignedInt{}, {}},              /* 10 */
        {nodeHandle(1, 0x1), ~UnsignedInt{}, {}},               /* 11 */
        /* Top-level nodes that aren't root nodes, the first isn't included in
           the order. For the purpose of this algorithm they behave the same as
           if the parent was null. */
        {nodeHandle(4, 0x1), 0xbaba, {}},                       /* 12 */
        {nodeHandle(5, 0xfff), 6, {}},                          /* 13 */
        /* A top-level node that's nested right under the hidden node shouldn't
           be considered */
        {nodeHandle(8, 0x1), 11, {}},                           /* 14 */
        /* A top-level node that's nested right under a visible node should
           be */
        {nodeHandle(13, 0x1), 4, {}},                           /* 15 */
        /* A subtree that's nested under the hidden node shouldn't be
           considered at all. The second node is top-level nested below another
           non-top level node, should be excluded as well, and its
           (non-top-level) child also. */
        {nodeHandle(8, 0x1), ~UnsignedInt{}, {}},               /* 16 */
        {nodeHandle(16, 0x1), 9, {}},                           /* 17 */
        {nodeHandle(17, 0x1), ~UnsignedInt{}, {}},              /* 18 */
        /* A hidden top-level node nested under node 3 (which isn't hidden)
           should be skipped too */
        {nodeHandle(3, 0x1), 1, NodeFlag::Hidden},              /* 19 */
    };

    /* The generation can be again arbitrary but it has to match with
       `firstNodeOrder` at least so the iteration of the cyclic list knows
       when to stop */
    const struct NodeOrder {
        NodeHandle next;
    } nodeOrder[]{
        {},                                 /* 0 */
        /* Next after node 18 is node 1, cycling back */
        {nodeHandle(1, 0x080)},             /* 1 */
        /* Next after node 1 (which references order 2) is node 13 */
        {nodeHandle(13, 0xfef)},            /* 2 */
        /* Next after node 7 (which is directly hidden) is node 3 */
        {nodeHandle(3, 0xbab)},             /* 3 */
        /* Next after node 15 (which is non-root top-level) is node 17 */
        {nodeHandle(17, 0xb0b)},            /* 4 */
        {},                                 /* 5 */
        /* Next after node 13 (which is non-root top-level) is node 14 */
        {nodeHandle(14, 0xebe)},            /* 6 */
        /* Next after node 3 is node 19 */
        {nodeHandle(19, 0xded)},            /* 7 */
        {},                                 /* 8 */
        /* Next after node 17 (which is also transitively hidden) is node 7 */
        {nodeHandle(7, 0xefe)},             /* 9 */
        {},
        /* Next after node 14 (which is transitively hidden) is node 15 */
        {nodeHandle(15, 0xaaa)},            /* 11 */
    };
    const NodeHandle firstNodeOrder = nodeHandle(1, 0x080);

    /* Important: the childrenOffsets array has to be zero-initialized. Others
       don't need to be. */
    char visibleNodes[3]{};
    UnsignedInt childrenOffsets[Containers::arraySize(nodes) + 1]{};
    UnsignedInt children[Containers::arraySize(nodes)];
    Containers::Triple<UnsignedInt, UnsignedInt, UnsignedInt> parentsToProcess[Containers::arraySize(nodes)];
    Containers::Pair<UnsignedInt, UnsignedInt> out[Containers::arraySize(nodes)];
    std::size_t count = Implementation::orderVisibleNodesDepthFirstInto(
        Containers::stridedArrayView(nodes).slice(&Node::parent),
        Containers::stridedArrayView(nodes).slice(&Node::order),
        Containers::stridedArrayView(nodes).slice(&Node::flags),
        Containers::stridedArrayView(nodeOrder).slice(&NodeOrder::next),
        firstNodeOrder,
        Containers::MutableBitArrayView{visibleNodes, 0, Containers::arraySize(nodes)},
        childrenOffsets, children, parentsToProcess,
        Containers::stridedArrayView(out).slice(&Containers::Pair<UnsignedInt, UnsignedInt>::first),
        Containers::stridedArrayView(out).slice(&Containers::Pair<UnsignedInt, UnsignedInt>::second));
    CORRADE_COMPARE_AS(count,
        Containers::arraySize(nodes),
        TestSuite::Compare::LessOrEqual);
    CORRADE_COMPARE_AS(Containers::arrayView(out).prefix(count), (Containers::arrayView<Containers::Pair<UnsignedInt, UnsignedInt>>({
        /* First is node 1 and its children */
        {1, 6},
            {4, 2},
                {5, 1},
                    {6, 0},
            /* Node 8 is hidden, not listed here, neither is node 16 which is
               its child */
            {9, 0},
            {11, 1},
                {10, 0},

        /* Next is top-level node 13 (which itself is a child of node 5), then
           its children. It has to be ordered after it in order to be treated
           as visible. */
        {13, 1},
            {0, 0},

        /* Top-level node 14 is a child of node 8, which is hidden, so not
           listed here */

        /* Top-level node 15 is a direct child of node 13 */
        {15, 0},

        /* Top-level node 17 is a child of node 16, which is a child of node 8,
           which is hidden, so not listed here, neither is its child 18 */

        /* Top-level node 7 is itself hidden, so not listed here either */

        /* Next is top-level node 3, it has no children */
        {3, 0},

        /* Top-level node 19 is a child of node 3, but is hidden */

        /* Node 2 and 12 not present as these aren't included in the order */
    })), TestSuite::Compare::Container);
}

void AbstractUserInterfaceImplementationTest::orderVisibleNodesDepthFirstSingleBranch() {
    /* Verifies behavior with just a single visible top-level node and single
       branch, i.e. so the internal arrays are correctly sized as this should
       lead to the longest stack in orderVisibleNodesDepthFirstInto(). */

    const struct Node {
        NodeHandle parent;
        UnsignedInt order;
        NodeFlags flags;
    } nodes[]{
        {NodeHandle::Null, 0, {}},                  /* 0 */
        {nodeHandle(0, 0xabc), ~UnsignedInt{}, {}}, /* 1 */
        {nodeHandle(3, 0xbca), ~UnsignedInt{}, {}}, /* 2 */
        {nodeHandle(1, 0xcab), ~UnsignedInt{}, {}}, /* 3 */
    };
    const struct NodeOrder {
        NodeHandle next;
    } nodeOrder[]{
        {nodeHandle(0, 0xacb)},                     /* 0 */
    };
    const NodeHandle firstNodeOrder = nodeHandle(0, 0xacb);

    char visibleNodes[1]{};
    UnsignedInt childrenOffsets[Containers::arraySize(nodes) + 1]{};
    UnsignedInt children[Containers::arraySize(nodes)];
    Containers::Triple<UnsignedInt, UnsignedInt, UnsignedInt> parentsToProcess[Containers::arraySize(nodes)];
    Containers::Pair<UnsignedInt, UnsignedInt> out[Containers::arraySize(nodes)];
    std::size_t count = Implementation::orderVisibleNodesDepthFirstInto(
        Containers::stridedArrayView(nodes).slice(&Node::parent),
        Containers::stridedArrayView(nodes).slice(&Node::order),
        Containers::stridedArrayView(nodes).slice(&Node::flags),
        Containers::stridedArrayView(nodeOrder).slice(&NodeOrder::next),
        firstNodeOrder,
        Containers::MutableBitArrayView{visibleNodes, 0, Containers::arraySize(nodes)},
        childrenOffsets, children, parentsToProcess,
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
        NodeHandle parent;
        UnsignedInt order;
        NodeFlags flags;
    } nodes[10]; /* {} makes GCC 4.8 crash */
    const struct NodeOrder {
        NodeHandle next;
    } nodeOrder[10]{};

    /* There's no first node order, so nothing is visible */
    char visibleNodes[2]{};
    UnsignedInt childrenOffsets[Containers::arraySize(nodes) + 1]{};
    UnsignedInt children[Containers::arraySize(nodes)];
    Containers::Triple<UnsignedInt, UnsignedInt, UnsignedInt> parentsToProcess[Containers::arraySize(nodes)];
    Containers::Pair<UnsignedInt, UnsignedInt> out[Containers::arraySize(nodes)];
    std::size_t count = Implementation::orderVisibleNodesDepthFirstInto(
        Containers::stridedArrayView(nodes).slice(&Node::parent),
        Containers::stridedArrayView(nodes).slice(&Node::order),
        Containers::stridedArrayView(nodes).slice(&Node::flags),
        Containers::stridedArrayView(nodeOrder).slice(&NodeOrder::next),
        NodeHandle::Null,
        Containers::MutableBitArrayView{visibleNodes, 0, Containers::arraySize(nodes)},
        childrenOffsets, children, parentsToProcess,
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

void AbstractUserInterfaceImplementationTest::propagateNodeFlagToChildren() {
    /* Mostly like the output in the orderVisibleNodesDepthFirst() case or
       input in visibleTopLevelNodeIndices() */
    Containers::Pair<UnsignedInt, UnsignedInt> visibleNodeIdsChildrenCountsFlags[]{
        {3, 0}, /* NoEvents */
        {13, 1},
            {0, 0},
        {1, 6}, /* NoEvents */
            {4, 2}, /* Disabled */
                {5, 1},
                    {6, 0},
            {9, 0},
            {11, 1}, /* Disabled */
                {10, 0},
        {17, 0} /* Disabled */
    };

    NodeFlags nodeFlags[]{
        {},                 /* 0 */
        NodeFlag::NoEvents, /* 1, affects also 4, 5, 6, 9, 11, 10 */
        NodeFlag::Disabled, /* 2, not visible */
        NodeFlag::NoEvents, /* 3 */
        NodeFlag::Disabled, /* 4, affects also 5, 6 */
        {},                 /* 5 */
        {},                 /* 6 */
        {},                 /* 7, not visible */
        NodeFlag::NoEvents, /* 8, not visible */
        {},                 /* 9 */
        {},                 /* 10 */
        NodeFlag::Disabled, /* 11, affects also 10 */
        {},                 /* 12, not visible */
        {},                 /* 13 */
        {},                 /* 14, not visible */
        {},                 /* 15, not visible */
        {},                 /* 16, not visible */
        NodeFlag::Disabled, /* 17 */
    };

    /* The NoEvents is implied by Disabled, so it should be reset for both */
    UnsignedByte nodesNoEventsData[3]{0xff, 0xff, 0xff};
    Containers::MutableBitArrayView nodesNoEvents{nodesNoEventsData, 0, 18};
    Implementation::propagateNodeFlagToChildrenInto<NodeFlag::NoEvents>(
        nodeFlags,
        Containers::stridedArrayView(visibleNodeIdsChildrenCountsFlags).slice(&Containers::Pair<UnsignedInt, UnsignedInt>::first),
        Containers::stridedArrayView(visibleNodeIdsChildrenCountsFlags).slice(&Containers::Pair<UnsignedInt, UnsignedInt>::second),
        nodesNoEvents);
    CORRADE_COMPARE_AS(nodesNoEvents, Containers::stridedArrayView({
     /* 0  1  2  3  4  5  6  7 */
        1, 0, 1, 0, 0, 0, 0, 1,
     /* 8  9 10 11 12 13 14 15 16 17 */
        1, 0, 0, 0, 1, 1, 1, 1, 1, 0
    }).sliceBit(0), TestSuite::Compare::Container);

    /* OTOH, Disabled shouldn't be set for nodes that are only NoEvents */
    UnsignedByte nodesDisabledData[3]{0xff, 0xff, 0xff};
    Containers::MutableBitArrayView nodesDisabled{nodesDisabledData, 0, 18};
    Implementation::propagateNodeFlagToChildrenInto<NodeFlag::Disabled>(
        nodeFlags,
        Containers::stridedArrayView(visibleNodeIdsChildrenCountsFlags).slice(&Containers::Pair<UnsignedInt, UnsignedInt>::first),
        Containers::stridedArrayView(visibleNodeIdsChildrenCountsFlags).slice(&Containers::Pair<UnsignedInt, UnsignedInt>::second),
        nodesDisabled);
    CORRADE_COMPARE_AS(nodesDisabled, Containers::stridedArrayView({
     /* 0  1  2  3  4  5  6  7 */
        1, 1, 1, 1, 0, 0, 0, 1,
     /* 8  9 10 11 12 13 14 15 16 17 */
        1, 1, 0, 0, 1, 1, 1, 1, 1, 0
    }).sliceBit(0), TestSuite::Compare::Container);

    /* It should never reset bits, only set them */
    UnsignedByte allZerosData[3]{};
    Containers::MutableBitArrayView allZeros{allZerosData, 0, 18};
    Implementation::propagateNodeFlagToChildrenInto<NodeFlag::Disabled>(
        nodeFlags,
        Containers::stridedArrayView(visibleNodeIdsChildrenCountsFlags).slice(&Containers::Pair<UnsignedInt, UnsignedInt>::first),
        Containers::stridedArrayView(visibleNodeIdsChildrenCountsFlags).slice(&Containers::Pair<UnsignedInt, UnsignedInt>::second),
        allZeros);
    CORRADE_COMPARE_AS(allZeros, Containers::stridedArrayView({
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    }).sliceBit(0), TestSuite::Compare::Container);
}

void AbstractUserInterfaceImplementationTest::discoverTopLevelLayoutNodesSingleLayouterPerNode() {
    /*  10-       9  12    15
        |\ \      |  | .
        7 6 5     8  13 14
        .   .        .
        4   0        11
        .   |\
        3   1 2

       (10, 7, 6, 5), (disconnected) 3 and 11 is one layouter, (disconnected)
       (0, 1, 2), (9, 8) and (11, 12) another. Node 4, 14, 15 has no layout,
       (9, 8) and 11 is not visible. Should result in 4 runs with top-level
       layout nodes 10, 3, 0 and 12. Shuffled to test for accidental ordering
       assumptions. */

    NodeHandle node0 = nodeHandle(0x0, 1);
    NodeHandle node1 = nodeHandle(0x1, 1);
    NodeHandle node2 = nodeHandle(0x2, 1);
    NodeHandle node3 = nodeHandle(0x3, 1);
    NodeHandle node4 = nodeHandle(0x4, 1);
    NodeHandle node5 = nodeHandle(0x5, 1);
    NodeHandle node6 = nodeHandle(0x6, 1);
    NodeHandle node7 = nodeHandle(0x7, 1);
    NodeHandle node8 = nodeHandle(0x8, 1);
    NodeHandle node9 = nodeHandle(0x9, 1);
    NodeHandle node10 = nodeHandle(0xa, 1);
    NodeHandle node11 = nodeHandle(0xb, 1);
    NodeHandle node12 = nodeHandle(0xc, 1);
    NodeHandle node13 = nodeHandle(0xd, 1);
    NodeHandle node14 = nodeHandle(0xe, 1);
    NodeHandle node15 = nodeHandle(0xf, 1);
    LayouterHandle layouterA = layouterHandle(0xaa, 1);
    LayouterHandle layouterB = layouterHandle(0xbb, 1);
    LayoutHandle b0 = layoutHandle(layouterB, 0xbbb0, 1);
    LayoutHandle b1 = layoutHandle(layouterB, 0xbbb1, 1);
    LayoutHandle b2 = layoutHandle(layouterB, 0xbbb2, 1);
    LayoutHandle a3 = layoutHandle(layouterA, 0xaaa3, 1);
    /* No layout for node 4 */
    LayoutHandle a5 = layoutHandle(layouterA, 0xaaa5, 1);
    LayoutHandle a6 = layoutHandle(layouterA, 0xaaa6, 1);
    LayoutHandle a7 = layoutHandle(layouterA, 0xaaa7, 1);
    LayoutHandle b8 = layoutHandle(layouterB, 0xbbb8, 1);
    LayoutHandle b9 = layoutHandle(layouterB, 0xbbb9, 1);
    LayoutHandle a10 = layoutHandle(layouterA, 0xaaa10, 1);
    LayoutHandle a11 = layoutHandle(layouterA, 0xbbb11, 1);
    LayoutHandle b12 = layoutHandle(layouterB, 0xbbb12, 1);
    LayoutHandle b13 = layoutHandle(layouterB, 0xbbb13, 1);
    /* No layout for node 14, 15 */

    NodeHandle nodeParents[16]{};
    nodeParents[nodeHandleId(node0)] = node5;
    nodeParents[nodeHandleId(node1)] = node0;
    nodeParents[nodeHandleId(node2)] = node0;
    nodeParents[nodeHandleId(node3)] = node4;
    nodeParents[nodeHandleId(node4)] = node7;
    nodeParents[nodeHandleId(node5)] = node10;
    nodeParents[nodeHandleId(node6)] = node10;
    nodeParents[nodeHandleId(node7)] = node10;
    nodeParents[nodeHandleId(node8)] = node9;
    nodeParents[nodeHandleId(node11)] = node13;
    nodeParents[nodeHandleId(node13)] = node12;
    nodeParents[nodeHandleId(node14)] = node12;

    /* Again shuffled to test for accidental ordering assumptions, though
       children *have to* be after parents in this case. */
    UnsignedInt visibleNodeIds[]{
        nodeHandleId(node12),
        nodeHandleId(node13),
        nodeHandleId(node14),
        nodeHandleId(node10),
        /* In the middle of the 10-765 tree, shouldn't cause it being split in
           two runs */
        nodeHandleId(node15),
        nodeHandleId(node5),
        nodeHandleId(node7),
        nodeHandleId(node6),
        /* Same here, is in the middle of the 5-012 tree but shouldn't cause it
           being split */
        nodeHandleId(node4),
        nodeHandleId(node3),
        nodeHandleId(node0),
        nodeHandleId(node2),
        nodeHandleId(node1),
    };

    LayoutHandle nodeLayouts[2*16]{
        LayoutHandle{}, b0,
        LayoutHandle{}, b1,
        LayoutHandle{}, b2,
        a3,             LayoutHandle{},
        LayoutHandle{}, LayoutHandle{},
        a5,             LayoutHandle{},
        a6,             LayoutHandle{},
        a7,             LayoutHandle{},
        LayoutHandle{}, b8,
        LayoutHandle{}, b9,
        a10,            LayoutHandle{},
        a11,            LayoutHandle{},
        LayoutHandle{}, b12,
        LayoutHandle{}, b13,
        LayoutHandle{}, LayoutHandle{},
        LayoutHandle{}, LayoutHandle{}
    };

    UnsignedInt nodeLayoutLevels[2*16]{};
    UnsignedInt layoutLevelOffsets[16 + 1]{};
    LayoutHandle topLevelLayouts[16];
    UnsignedInt topLevelLayoutLevels[16];
    LayoutHandle levelPartitionedTopLevelLayouts[16];
    UnsignedInt topLevelLayoutOffsets[16 + 1];
    UnsignedByte topLevelLayoutLayouterIds[16];
    UnsignedInt topLevelLayoutIds[16];
    std::size_t count = Implementation::discoverTopLevelLayoutNodesInto(
        nodeParents,
        visibleNodeIds,
        0xef,
        Containers::StridedArrayView2D<const LayoutHandle>{nodeLayouts, {16, 2}},
        Containers::StridedArrayView2D<UnsignedInt>{nodeLayoutLevels, {16, 2}},
        layoutLevelOffsets,
        topLevelLayouts,
        topLevelLayoutLevels,
        levelPartitionedTopLevelLayouts,
        topLevelLayoutOffsets,
        topLevelLayoutLayouterIds,
        topLevelLayoutIds).second();
    CORRADE_COMPARE_AS(count,
        0,
        TestSuite::Compare::Greater);
    CORRADE_COMPARE_AS(count,
        Containers::arraySize(topLevelLayoutOffsets),
        TestSuite::Compare::LessOrEqual);
    CORRADE_COMPARE_AS(Containers::arrayView(topLevelLayoutOffsets).prefix(count),
        Containers::arrayView({0u, 2u, 4u}),
        TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(topLevelLayoutLayouterIds).prefix(count - 1),
        Containers::arrayView<UnsignedByte>({0xaa, 0xbb}),
        TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(topLevelLayoutIds).prefix(topLevelLayoutOffsets[count - 1]),
        Containers::arrayView({layoutHandleId(a10), layoutHandleId(a3), layoutHandleId(b12), layoutHandleId(b0)}),
        TestSuite::Compare::Container);
}

void AbstractUserInterfaceImplementationTest::discoverTopLevelLayoutNodesMultipleLayoutersPerNode() {
    auto&& data = DiscoverTopLevelLayoutNodesMultipleLayoutersPerNodeData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    NodeHandle node1 = nodeHandle(0x1, 1);
    NodeHandle node2 = nodeHandle(0x2, 1);
    NodeHandle node3 = nodeHandle(0x3, 1);
    NodeHandle node4 = nodeHandle(0x4, 1);
    NodeHandle node5 = nodeHandle(0x5, 1);
    NodeHandle node6 = nodeHandle(0x6, 1);
    LayouterHandle layouterA = layouterHandle(0xaa, 1);
    LayouterHandle layouterB = layouterHandle(0xbb, 1);
    LayouterHandle layouterC = layouterHandle(0xcc, 1);
    LayouterHandle layouterD = layouterHandle(0xdd, 1);
    LayouterHandle layouterE = layouterHandle(0xee, 1);
    LayoutHandle a1 = layoutHandle(layouterA, 0xaaa1, 1);
    LayoutHandle a2 = layoutHandle(layouterA, 0xaaa2, 1);
    LayoutHandle a5 = layoutHandle(layouterA, 0xaaa5, 1);
    LayoutHandle a6 = layoutHandle(layouterA, 0xaaa6, 1);
    LayoutHandle b1 = layoutHandle(layouterB, 0xbbb1, 1);
    LayoutHandle b2 = layoutHandle(layouterB, 0xbbb2, 1);
    LayoutHandle b3 = layoutHandle(layouterB, 0xbbb3, 1);
    LayoutHandle c3 = layoutHandle(layouterC, 0xccc3, 1);
    LayoutHandle c4 = layoutHandle(layouterC, 0xccc4, 1);
    LayoutHandle c5 = layoutHandle(layouterC, 0xccc5, 1);
    LayoutHandle d2 = layoutHandle(layouterD, 0xddd2, 1);
    LayoutHandle d3 = layoutHandle(layouterD, 0xddd3, 1);
    LayoutHandle e2 = layoutHandle(layouterE, 0xeee2, 1);

    NodeHandle nodeParents[0x7]{};
    nodeParents[nodeHandleId(node2)] = node1;
    nodeParents[nodeHandleId(node3)] = node2;
    nodeParents[nodeHandleId(node4)] = node3;
    nodeParents[nodeHandleId(node5)] = node4;
    nodeParents[nodeHandleId(node6)] = node5;

    UnsignedInt visibleNodeIds[]{
        nodeHandleId(node1),
        nodeHandleId(node2),
        nodeHandleId(node3),
        nodeHandleId(node4),
        nodeHandleId(node5),
        nodeHandleId(node6),
    };

    /* This list is here instead of in
       DiscoverTopLevelLayoutNodesMultipleLayoutersPerNodeData as it's
       significantly less annoying that way.

       In case there would be more than one layout assigned to the same node,
       the code in UserInterface::update() would arbitrarily use just one of
       them. Such condition can't be tested here but is checked in UserInterfaceTest::state().

        0 (node 0 unused to test that it's not indexing with wrong IDs)
        1  AB    01          AB    01        AB    01        AB    01
        2  ab DE 01 23       ab DE 01 23     ab DE 01 23     ab DE 01 23
        3   bC    12          bC    12        bCd   132       bCd   132
        4    c     2           c     2         c     3         c     3
        5  A     0           A c   3 2       A     0         A c   4 3
        6  a     0           a     3         a     0         a     4     */
    LayoutHandle nodeLayouts[Containers::arraySize(DiscoverTopLevelLayoutNodesMultipleLayoutersPerNodeData)][7*5]{
        /** @todo cleanup once GCC 4.8, which fails with "error: braces around
            scalar initializer" when encountering {}, is no longer supported */
        #define __ LayoutHandle{}
        {
            __, __, __, __, __,
            a1, b1, __, __, __,
            a2, b2, __, d2, e2,
            __, b3, c3, __, __,
            __, __, c4, __, __,
            a5, __, __, __, __,
            a6, __, __, __, __
        }, {
            __, __, __, __, __,
            a1, b1, __, __, __,
            a2, b2, __, d2, e2,
            __, b3, c3, __, __,
            __, __, c4, __, __,
            a5, __, c5, __, __,
            a6, __, __, __, __
        }, {
            __, __, __, __, __,
            a1, b1, __, __, __,
            a2, b2, __, d2, e2,
            __, b3, c3, d3, __,
            __, __, c4, __, __,
            a5, __, __, __, __,
            a6, __, __, __, __
        }, {
            __, __, __, __, __,
            a1, b1, __, __, __,
            a2, b2, __, d2, e2,
            __, b3, c3, d3, __,
            __, __, c4, __, __,
            a5, __, c5, __, __,
            a6, __, __, __, __
        }
        #undef __
    };

    UnsignedInt nodeLayoutLevels[7*5]{};
    UnsignedInt layoutLevelOffsets[11 + 1]{};
    LayoutHandle topLevelLayouts[11];
    UnsignedInt topLevelLayoutLevels[11];
    LayoutHandle levelPartitionedTopLevelLayouts[11];
    UnsignedInt topLevelLayoutOffsets[11 + 1];
    UnsignedByte topLevelLayoutLayouterIds[11];
    UnsignedInt topLevelLayoutIds[11];
    std::size_t count = Implementation::discoverTopLevelLayoutNodesInto(
        nodeParents,
        visibleNodeIds,
        0xef,
        Containers::StridedArrayView2D<const LayoutHandle>{nodeLayouts[testCaseInstanceId()], {7, 5}},
        Containers::StridedArrayView2D<UnsignedInt>{nodeLayoutLevels, {7, 5}},
        layoutLevelOffsets,
        topLevelLayouts,
        topLevelLayoutLevels,
        levelPartitionedTopLevelLayouts,
        topLevelLayoutOffsets,
        topLevelLayoutLayouterIds,
        topLevelLayoutIds).second();
    CORRADE_COMPARE_AS(count,
        0,
        TestSuite::Compare::Greater);
    CORRADE_COMPARE_AS(count,
        Containers::arraySize(topLevelLayoutOffsets),
        TestSuite::Compare::LessOrEqual);
    CORRADE_COMPARE_AS(Containers::arrayView(topLevelLayoutOffsets).prefix(count),
        arrayView(data.topLevelLayoutOffsets),
        TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(topLevelLayoutLayouterIds).prefix(count - 1),
        arrayView(data.topLevelLayoutLayouterIds),
        TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(topLevelLayoutIds).prefix(topLevelLayoutOffsets[count - 1]),
        arrayView(data.topLevelLayoutIds),
        TestSuite::Compare::Container);
}

void AbstractUserInterfaceImplementationTest::discoverTopLevelLayoutNodesNoLayouters() {
    NodeHandle node1 = nodeHandle(0x1, 1);
    NodeHandle node2 = nodeHandle(0x2, 1);

    NodeHandle nodeParents[3]{};
    nodeParents[nodeHandleId(node2)] = node1;

    UnsignedInt visibleNodeIds[]{
        nodeHandleId(node1),
        nodeHandleId(node2),
    };

    /* Shouldn't blow up in any way */
    UnsignedInt layoutLevelOffsets[1]{};
    UnsignedInt topLevelLayoutOffsets[1];
    std::size_t count = Implementation::discoverTopLevelLayoutNodesInto(
        nodeParents,
        visibleNodeIds,
        0xef,
        Containers::StridedArrayView2D<const LayoutHandle>{nullptr, {3, 0}},
        Containers::StridedArrayView2D<UnsignedInt>{nullptr, {3, 0}},
        layoutLevelOffsets,
        {},
        {},
        {},
        topLevelLayoutOffsets,
        {},
        {}).second();
    CORRADE_COMPARE(count, 1);
    CORRADE_COMPARE_AS(Containers::arrayView(topLevelLayoutOffsets),
        Containers::arrayView({0u}),
        TestSuite::Compare::Container);
}

void AbstractUserInterfaceImplementationTest::discoverTopLevelLayoutNodesNoVisibleNodes() {
    NodeHandle node1 = nodeHandle(0x1, 1);
    NodeHandle node2 = nodeHandle(0x2, 1);
    LayouterHandle layouterA = layouterHandle(0xaa, 1);
    LayouterHandle layouterB = layouterHandle(0xbb, 1);
    LayoutHandle a1 = layoutHandle(layouterA, 0xaaa1, 1);
    LayoutHandle b2 = layoutHandle(layouterB, 0xbbb2, 1);

    NodeHandle nodeParents[3]{};
    nodeParents[nodeHandleId(node2)] = node1;

    LayoutHandle nodeLayouts[3*2]{
        LayoutHandle{}, LayoutHandle{},
        a1,             LayoutHandle{},
        LayoutHandle{}, b2
    };
    UnsignedInt nodeLayoutLevels[3*2]{};

    /* Shouldn't blow up in any way */
    UnsignedInt layoutLevelOffsets[1]{};
    UnsignedInt topLevelLayoutOffsets[1];
    std::size_t count = Implementation::discoverTopLevelLayoutNodesInto(
        nodeParents,
        {},
        0xef,
        Containers::StridedArrayView2D<const LayoutHandle>{nodeLayouts, {3, 2}},
        Containers::StridedArrayView2D<UnsignedInt>{nodeLayoutLevels, {3, 2}},
        layoutLevelOffsets,
        {},
        {},
        {},
        topLevelLayoutOffsets,
        {},
        {}).second();
    CORRADE_COMPARE(count, 1);
    CORRADE_COMPARE_AS(Containers::arrayView(topLevelLayoutOffsets),
        Containers::arrayView({0u}),
        TestSuite::Compare::Container);
}

void AbstractUserInterfaceImplementationTest::discoverTopLevelLayoutNodesSingleNode() {
    NodeHandle node = nodeHandle(0x0, 1);
    LayouterHandle layouter = layouterHandle(0xaa, 1);
    LayoutHandle a = layoutHandle(layouter, 0xaaa1, 1);

    NodeHandle nodeParents[1]{};

    UnsignedInt visibleNodeIds[]{
        nodeHandleId(node),
    };

    LayoutHandle nodeLayouts[]{
        a
    };

    UnsignedInt nodeLayoutLevels[1]{};
    UnsignedInt layoutLevelOffsets[1 + 1]{};
    LayoutHandle topLevelLayouts[1];
    UnsignedInt topLevelLayoutLevels[1];
    LayoutHandle levelPartitionedTopLevelLayouts[1];
    UnsignedInt topLevelLayoutOffsets[1 + 1];
    UnsignedByte topLevelLayoutLayouterIds[1];
    UnsignedInt topLevelLayoutIds[1];
    std::size_t count = Implementation::discoverTopLevelLayoutNodesInto(
        nodeParents,
        visibleNodeIds,
        0xef,
        Containers::StridedArrayView2D<const LayoutHandle>{nodeLayouts, {1, 1}},
        Containers::StridedArrayView2D<UnsignedInt>{nodeLayoutLevels, {1, 1}},
        layoutLevelOffsets,
        topLevelLayouts,
        topLevelLayoutLevels,
        levelPartitionedTopLevelLayouts,
        topLevelLayoutOffsets,
        topLevelLayoutLayouterIds,
        topLevelLayoutIds).second();
    CORRADE_COMPARE_AS(count,
        0,
        TestSuite::Compare::Greater);
    CORRADE_COMPARE_AS(count,
        Containers::arraySize(topLevelLayoutOffsets),
        TestSuite::Compare::LessOrEqual);
    CORRADE_COMPARE_AS(Containers::arrayView(topLevelLayoutOffsets).prefix(count),
        Containers::arrayView({0u, 1u}),
        TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(topLevelLayoutLayouterIds).prefix(count - 1),
        Containers::arrayView<UnsignedByte>({0xaa}),
        TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(topLevelLayoutIds).prefix(topLevelLayoutOffsets[count - 1]),
        Containers::arrayView({layoutHandleId(a)}),
        TestSuite::Compare::Container);
}

void AbstractUserInterfaceImplementationTest::discoverTopLevelLayoutNodesSingleNodeLayoutChain() {
    NodeHandle node = nodeHandle(0x0, 1);
    LayouterHandle layouterA = layouterHandle(0xaa, 1);
    LayouterHandle layouterB = layouterHandle(0xbb, 1);
    LayouterHandle layouterC = layouterHandle(0xcc, 1);
    LayoutHandle a = layoutHandle(layouterA, 0xaaa1, 1);
    LayoutHandle b = layoutHandle(layouterB, 0xbbb1, 1);
    LayoutHandle c = layoutHandle(layouterC, 0xccc1, 1);

    NodeHandle nodeParents[1]{};

    UnsignedInt visibleNodeIds[]{
        nodeHandleId(node),
    };

    LayoutHandle nodeLayouts[]{
        a, b, c
    };

    UnsignedInt nodeLayoutLevels[3]{};
    UnsignedInt layoutLevelOffsets[3 + 1]{};
    LayoutHandle topLevelLayouts[3];
    UnsignedInt topLevelLayoutLevels[3];
    LayoutHandle levelPartitionedTopLevelLayouts[3];
    UnsignedInt topLevelLayoutOffsets[3 + 1];
    UnsignedByte topLevelLayoutLayouterIds[3];
    UnsignedInt topLevelLayoutIds[3];
    std::size_t count = Implementation::discoverTopLevelLayoutNodesInto(
        nodeParents,
        visibleNodeIds,
        0xef,
        Containers::StridedArrayView2D<const LayoutHandle>{nodeLayouts, {1, 3}},
        Containers::StridedArrayView2D<UnsignedInt>{nodeLayoutLevels, {1, 3}},
        layoutLevelOffsets,
        topLevelLayouts,
        topLevelLayoutLevels,
        levelPartitionedTopLevelLayouts,
        topLevelLayoutOffsets,
        topLevelLayoutLayouterIds,
        topLevelLayoutIds).second();
    CORRADE_COMPARE_AS(count,
        0,
        TestSuite::Compare::Greater);
    CORRADE_COMPARE_AS(count,
        Containers::arraySize(topLevelLayoutOffsets),
        TestSuite::Compare::LessOrEqual);
    CORRADE_COMPARE_AS(Containers::arrayView(topLevelLayoutOffsets).prefix(count),
        Containers::arrayView({0u, 1u, 2u, 3u}),
        TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(topLevelLayoutLayouterIds).prefix(count - 1),
        Containers::arrayView<UnsignedByte>({0xaa, 0xbb, 0xcc}),
        TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(topLevelLayoutIds).prefix(topLevelLayoutOffsets[count - 1]),
        Containers::arrayView({layoutHandleId(a), layoutHandleId(b), layoutHandleId(c)}),
        TestSuite::Compare::Container);
}

void AbstractUserInterfaceImplementationTest::fillLayoutUpdateMasks() {
    LayouterHandle layouterA = layouterHandle(0xa, 1);
    LayouterHandle layouterB = layouterHandle(0xb, 1);
    LayouterHandle layouterC = layouterHandle(0xc, 1);
    LayoutHandle a0 = layoutHandle(layouterA, 0, 1);
    LayoutHandle a1 = layoutHandle(layouterA, 1, 1);
    LayoutHandle a2 = layoutHandle(layouterA, 2, 1);
    LayoutHandle a5 = layoutHandle(layouterA, 5, 1);
    LayoutHandle a6 = layoutHandle(layouterA, 6, 1);
    LayoutHandle b0 = layoutHandle(layouterB, 0, 1);
    LayoutHandle b1 = layoutHandle(layouterB, 1, 1);
    LayoutHandle b3 = layoutHandle(layouterB, 3, 1);
    LayoutHandle b4 = layoutHandle(layouterB, 4, 1);
    LayoutHandle b5 = layoutHandle(layouterB, 5, 1);
    LayoutHandle b7 = layoutHandle(layouterB, 7, 1);
    LayoutHandle b9 = layoutHandle(layouterB, 9, 1);
    LayoutHandle c0 = layoutHandle(layouterC, 0, 1);
    LayoutHandle c1 = layoutHandle(layouterC, 1, 1);
    LayoutHandle c2 = layoutHandle(layouterC, 2, 1);
    LayoutHandle c3 = layoutHandle(layouterC, 3, 1);

    Containers::Pair<LayoutHandle, UnsignedInt> nodeLayoutsLevels[]{
        /* Level + 1, with 0 (for b9) indicating the node has a layouter but it
           isn't visible so it shouldn't be in the mask either */
        {},      {b4, 3}, {},
        {a1, 1}, {b1, 2}, {c0, 4},
        {a2, 5}, {b7, 3}, {},
        {},      {b3, 3}, {c3, 4},
        {},      {},      {},
        {a5, 3}, {b9, 0}, {},
        {a6, 5}, {},      {},
        {a0, 1}, {b5, 2}, {c1, 4},
        {},      {b0, 3}, {c2, 4},
    };

    Containers::Pair<UnsignedInt, UnsignedByte> topLevelLayoutOffsetsLayouterIds[]{
        /* Not using layouterHandleId(layouterA) etc because the cast to
           UnsignedByte is then extremely annoying */
        {0, 0xa}, /* level 0, a1 + a0 */
        {1, 0xb}, /* level 1, b1 and b5 separately */
        {3, 0xb}, /* level 2, b4 + b0 and then b7 + b3 */
        {5, 0xa}, /* level 2, a5 */
        {6, 0xc}, /* level 3, c0 to c2 and then c3 */
        {8, 0xa}, /* level 4, a2 + a6 */
        {9, 0xff},
    };

    UnsignedInt layoutLevelOffsets[]{
        0, /* level 0 is 1 item */
        1, /* level 1 is 2 items */
        3, /* level 2 is 3 items */
        6, /* level 3 is 2 items */
        8, /* level 4 is 1 item */
        9
    };

    UnsignedInt layouterCapacities[0xd];
    layouterCapacities[layouterHandleId(layouterA)] = 7;  /* 2 places unused */
    layouterCapacities[layouterHandleId(layouterB)] = 10; /* 4 places unused */
    layouterCapacities[layouterHandleId(layouterC)] = 4;  /* all places used */

    std::size_t layouterLevelMaskOffsets[0xd*5];
    UnsignedByte masksData[6]{};
    Containers::MutableBitArrayView masks{masksData, 0, 7 + 2*10 + 7 + 4 + 7};
    Implementation::fillLayoutUpdateMasksInto(
        stridedArrayView(nodeLayoutsLevels).slice(&Containers::Pair<LayoutHandle, UnsignedInt>::first).expanded<0, 2>({9, 3}),
        stridedArrayView(nodeLayoutsLevels).slice(&Containers::Pair<LayoutHandle, UnsignedInt>::second).expanded<0, 2>({9, 3}),
        layoutLevelOffsets,
        stridedArrayView(topLevelLayoutOffsetsLayouterIds).slice(&Containers::Pair<UnsignedInt, UnsignedByte>::first),
        stridedArrayView(topLevelLayoutOffsetsLayouterIds).slice(&Containers::Pair<UnsignedInt, UnsignedByte>::second).exceptSuffix(1),
        layouterCapacities,
        Containers::stridedArrayView(layouterLevelMaskOffsets).expanded<0, 2>({5, 0xd}),
        masks);
    /* The bits should be mutually disjoint for each layouter */
    CORRADE_COMPARE_AS(masks, Containers::stridedArrayView({
     /* 0  1  2  3  4  5  6  7  8  9 */
        /* level 0; a1, a0 */
        1, 1, 0, 0, 0, 0, 0,
        /* level 1; b1, b5 */
        0, 1, 0, 0, 0, 1, 0, 0, 0, 0,
        /* level 2; b4, b0, b7, b3 */
        1, 0, 0, 1, 1, 0, 0, 1, 0, 0,
        /* level 2; a5 */
        0, 0, 0, 0, 0, 1, 0,
        /* level 3; c0, c1, c2, c3 */
        1, 1, 1, 1,
        /* level 4; a2, a6 */
        0, 0, 1, 0, 0, 0, 1
    }).sliceBit(0), TestSuite::Compare::Container);
}

void AbstractUserInterfaceImplementationTest::fillLayoutUpdateMasksNoLayouters() {
    /* Shouldn't blow up in any way */
    UnsignedInt topLevelLayoutOffsets[1];
    UnsignedInt layoutLevelOffsets[1]{};
    Implementation::fillLayoutUpdateMasksInto(
        Containers::StridedArrayView2D<const LayoutHandle>{nullptr, {9, 0}},
        Containers::StridedArrayView2D<UnsignedInt>{nullptr, {9, 0}},
        layoutLevelOffsets,
        topLevelLayoutOffsets,
        {},
        {},
        {},
        {});
    CORRADE_VERIFY(true);
}

void AbstractUserInterfaceImplementationTest::cullVisibleNodesClipRects() {
    auto&& data = CullVisibleNodesClipRectsData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    UnsignedByte visibleNodeMaskStorage[1];
    Containers::MutableBitArrayView visibleNodeMask{visibleNodeMaskStorage, 0, data.nodeIdsChildrenCount.size()};

    /* One more item for the stack root, which is the whole UI offset + size */
    Containers::Triple<Vector2, Vector2, UnsignedInt> clipStack[8 + 1];
    Containers::Triple<Vector2, Vector2, UnsignedInt> clipRects[8];
    UnsignedInt count = Implementation::cullVisibleNodesInto(
        data.uiOffset, data.uiSize,
        stridedArrayView(data.nodeOffsetsSizesFlags).slice(&Containers::Triple<Vector2, Vector2, NodeFlags>::first),
        stridedArrayView(data.nodeOffsetsSizesFlags).slice(&Containers::Triple<Vector2, Vector2, NodeFlags>::second),
        stridedArrayView(data.nodeOffsetsSizesFlags).slice(&Containers::Triple<Vector2, Vector2, NodeFlags>::third),
        Containers::arrayView(clipStack).prefix(data.nodeIdsChildrenCount.size() + 1),
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
        {data.clipNodeOffset,
         data.clipNodeSize,
         data.clipNodeFlags},               /* 34, clip node */
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

    /* One more item for the stack root, which is the whole UI offset + size */
    Containers::Triple<Vector2, Vector2, UnsignedInt> clipStack[Containers::arraySize(nodeOffsetsSizesFlags) + 1];
    Containers::Triple<Vector2, Vector2, UnsignedInt> clipRects[Containers::arraySize(nodeOffsetsSizesFlags)];
    UnsignedInt count = Implementation::cullVisibleNodesInto(
        data.uiOffset, data.uiSize,
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
        {data.clipRectOffset, data.clipRectSize, 35u}
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
            {11, 1}, /* Zero size, so gets skipped and its child also (if
                        it clips) */
                {14, 0},
            {13, 0}, /* Zero width, skipped if clips */
            {12, 0}, /* Zero height, skipped if clips */
            {2, 5},
                {0, 1}, /* Visible in 2 but not in 7, skipped if 7 clips */
                    /* Extends back to 7 but still gets skipped without testing
                       because it's fully clipped by 0 (if it clips) */
                    {6, 0},
                {10, 0},
                {1, 1}, /* Visible in the top-level rect but not the parent,
                           skipped if clips */
                    {4, 0}, /* If parent clips, gets skipped without testing */

        /* Two children are outside of the node rect, get skipped if the node
           clips */
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

    /* One more item for the stack root, which is the whole UI offset + size */
    Containers::Triple<Vector2, Vector2, UnsignedInt> clipStack[Containers::arraySize(nodeOffsetsSizes) + 1];
    Containers::Triple<Vector2, Vector2, UnsignedInt> clipRects[Containers::arraySize(nodeOffsetsSizes)];
    UnsignedInt count = Implementation::cullVisibleNodesInto(
        data.uiOffset, data.uiSize,
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
    Containers::Triple<Vector2, Vector2, UnsignedInt> clipStack[1];
    Containers::MutableBitArrayView visibleNodeMask{visibleNodeMaskData, 0, 3};
    UnsignedInt count = Implementation::cullVisibleNodesInto(
        /* A non-zero UI size so it doesn't just clip all for no reason */
        {}, {100.0f, 100.0f},
        absoluteNodeOffsets,
        nodeSizes,
        nodeFlags,
        clipStack,
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
    UnsignedInt visibleNodeDataIds[3];
    Containers::Pair<UnsignedInt, UnsignedInt> count = Implementation::orderVisibleNodeDataInto(
        nullptr,
        nullptr,
        dataNodes,
        {},
        visibleNodeMask,
        nullptr,
        visibleNodeDataOffsets,
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

void AbstractUserInterfaceImplementationTest::countOrderNodeDataForEventHandling() {
    /* Subset of data node attachments from orderVisibleNodeData() above for
       layers that have Event set. */
    const NodeHandle layer2NodeAttachments[]{
        NodeHandle{},           /* 0 */
        NodeHandle{},           /* 1 */
        nodeHandle(4, 0xbab),   /* 2 */
        nodeHandle(3, 0xfef),   /* 3 */
        nodeHandle(12, 0xccc),  /* 4 */
        NodeHandle{},           /* 5 */
        nodeHandle(2, 0xddd)    /* 6 */
    };
    const NodeHandle layer3NodeAttachments[]{
        nodeHandle(2, 0xefe),   /* 0 */
        NodeHandle{},           /* 1 */
        nodeHandle(7, 0xf0f)    /* 2 */
    };
    const NodeHandle layer5NodeAttachments[]{
        NodeHandle{},           /* 0 */
        nodeHandle(3, 0xc0c),   /* 1 */
        nodeHandle(3, 0xc0c),   /* 2 */
        NodeHandle{},           /* 3 */
        /* Node 8 isn't in the visible hierarchy so the assignment gets
           ignored */
        nodeHandle(8, 0xbbb),   /* 4 */
    };

    /* Compared to orderVisibleNodeData(), only node 8 is left among the
       assignments, all others can stay visible even if they aren't as it
       shouldn't matter for them */
    UnsignedShort visibleEventNodeMaskData[]{0xffff & ~(1 << 8)};
    Containers::BitArrayView visibleEventNodeMask{visibleEventNodeMaskData, 0, 14};

    LayerHandle layer2 = layerHandle(2, 0x88);
    LayerHandle layer3 = layerHandle(3, 0x22);
    LayerHandle layer5 = layerHandle(5, 0x44);
    Containers::Pair<Containers::StridedArrayView1D<const NodeHandle>, LayerHandle> layers[]{
        {layer5NodeAttachments, layer5},
        {layer3NodeAttachments, layer3},
        {layer2NodeAttachments, layer2},
    };

    /* First count the event data for all layers */
    UnsignedInt visibleNodeEventDataOffsets[15]{};
    for(const auto& layer: layers) {
        CORRADE_ITERATION(layer.second());
        Implementation::countNodeDataForEventHandlingInto(layer.first(), visibleNodeEventDataOffsets, visibleEventNodeMask);
    }
    CORRADE_COMPARE_AS(Containers::arrayView(visibleNodeEventDataOffsets), Containers::arrayView<UnsignedInt>({
        0,
        0,  /* Node 0, not part of the top-level hierarchy */
        0,  /* Node 1 */
        2,  /* Node 2, layers 2 and 3 */
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

    /* Turn the counts into running offsets */
    {
        UnsignedInt visibleNodeEventDataCount = 0;
        for(UnsignedInt& i: visibleNodeEventDataOffsets) {
            const UnsignedInt nextOffset = visibleNodeEventDataCount + i;
            i = visibleNodeEventDataCount;
            visibleNodeEventDataCount = nextOffset;
        }
    }
    CORRADE_COMPARE_AS(Containers::arrayView(visibleNodeEventDataOffsets), Containers::arrayView<UnsignedInt>({
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
    }), TestSuite::Compare::Container);

    /* Then order the data for all layers */
    DataHandle visibleNodeEventData[9];
    for(const auto& layer: layers) {
        CORRADE_ITERATION(layer.second());
        Implementation::orderNodeDataForEventHandlingInto(
            layer.second(),
            layer.first(),
            visibleNodeEventDataOffsets,
            visibleEventNodeMask,
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
        dataHandle(layer3, 0, 0xfff),
        dataHandle(layer2, 6, 0xfff),
        /* Node 3. Order of items from the same layer matches inverse data ID
           order, not the order in which they were created or attached. */
        dataHandle(layer5, 2, 0xfff),
        dataHandle(layer5, 1, 0xfff),
        dataHandle(layer2, 3, 0xfff),
        /* Node 4 */
        dataHandle(layer2, 2, 0xfff),
        /* Node 7 */
        dataHandle(layer3, 2, 0xfff),
        /* Node 8 isn't visible */
        /* Node 12 */
        dataHandle(layer2, 4, 0xfff)
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

void AbstractUserInterfaceImplementationTest::compositeRectsEdges() {
    /* Offsets + sizes like in cullVisibleNodesEdges(), without the outside.
       The double-line rectangle is one side of the culling, the 0 to 13
       rectangles are the other. There's enough combinations so that it's
       sufficient to test just one rectangle always. Combination of more
       nodes and clip rectangles is tested in drawBounds() below.

        -1 0 12 3 4   5 6 78 9 10
      -1        +-------+
       0  +-----|-+...+-|-----+
          |0    | | 1 | |    2|
       1  |  +==|=|===|=|==+  |
       2  |  |  +-------+  |  |
       3 +----+   | 3 |   +----+
       4 |+---|---+...+---|---+|
         | 4 || 6 .   . 7 || 5 |
       5 |+---|---+...+---|---+|
       6 +----+   | 8 |   +----+
       7  |  |  +-------+  |  |
       8  |  +==|=|===|=|==+  |
          |10   | | 9 | |   11|
       9  +-----|-+   +-|-----+
      10        +-------+      13 */
    const Containers::Pair<Vector2, Vector2> offsetsSizes[]{
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
    };
    const Containers::Pair<Vector2, Vector2> expectedOffsetsSizes[]{
        {{1.0f, 1.0f}, {3.0f, 3.0f}}, /*  0 */
        {{3.0f, 1.0f}, {3.0f, 1.0f}}, /*  1 */
        {{5.0f, 1.0f}, {3.0f, 3.0f}}, /*  2 */
        {{1.0f, 1.0f}, {7.0f, 3.0f}}, /*  3 */
        {{1.0f, 3.0f}, {1.0f, 3.0f}}, /*  4 */
        {{7.0f, 3.0f}, {1.0f, 3.0f}}, /*  5 */
        {{1.0f, 1.0f}, {3.0f, 7.0f}}, /*  6 */
        {{5.0f, 1.0f}, {3.0f, 7.0f}}, /*  7 */
        {{1.0f, 5.0f}, {7.0f, 3.0f}}, /*  8 */
        {{3.0f, 7.0f}, {3.0f, 1.0f}}, /*  9 */
        {{1.0f, 5.0f}, {3.0f, 3.0f}}, /* 10 */
        {{5.0f, 5.0f}, {3.0f, 3.0f}}, /* 11 */
        {{4.0f, 4.0f}, {2.0f, 2.0f}}, /* 12 */
        {{1.0f, 1.0f}, {7.0f, 7.0f}}, /* 13 */
    };
    CORRADE_COMPARE(Containers::arraySize(expectedOffsetsSizes), Containers::arraySize(offsetsSizes));

    for(UnsignedInt i = 0; i != Containers::arraySize(offsetsSizes); ++i) {
        CORRADE_ITERATION(i);

        Containers::Pair<Vector2, Vector2> compositeRectOffsetsSizes[1];

        /* No clip rect and a large enough UI rect should result in no
           clipping */
        Implementation::compositeRectsInto(
            Vector2{-1.0f}, Vector2{11.0f},
            Containers::arrayView({0u}),
            Containers::arrayView({0u}),
            Containers::arrayView({1u}),
            Containers::arrayView({nodeHandle(i, 0xece)}),
            stridedArrayView(offsetsSizes).slice(&Containers::Pair<Vector2, Vector2>::first),
            stridedArrayView(offsetsSizes).slice(&Containers::Pair<Vector2, Vector2>::second),
            Containers::arrayView({Vector2{}}),
            Containers::arrayView({Vector2{}}),
            stridedArrayView(compositeRectOffsetsSizes).slice(&Containers::Pair<Vector2, Vector2>::first),
            stridedArrayView(compositeRectOffsetsSizes).slice(&Containers::Pair<Vector2, Vector2>::second));
        CORRADE_COMPARE(compositeRectOffsetsSizes[0], offsetsSizes[i]);

        /* Large enough UI size with a clip rect should clip */
        Implementation::compositeRectsInto(
            Vector2{-1.0f}, Vector2{11.0f},
            Containers::arrayView({0u}),
            Containers::arrayView({0u}),
            Containers::arrayView({1u}),
            Containers::arrayView({nodeHandle(i, 0xece)}),
            stridedArrayView(offsetsSizes).slice(&Containers::Pair<Vector2, Vector2>::first),
            stridedArrayView(offsetsSizes).slice(&Containers::Pair<Vector2, Vector2>::second),
            Containers::arrayView({Vector2{1.0f}}),
            Containers::arrayView({Vector2{7.0f}}),
            stridedArrayView(compositeRectOffsetsSizes).slice(&Containers::Pair<Vector2, Vector2>::first),
            stridedArrayView(compositeRectOffsetsSizes).slice(&Containers::Pair<Vector2, Vector2>::second));
        CORRADE_COMPARE(compositeRectOffsetsSizes[0], expectedOffsetsSizes[i]);

        /* Small UI size and no clip rect should clip the same way */
        Implementation::compositeRectsInto(
            Vector2{1.0f}, Vector2{7.0f},
            Containers::arrayView({0u}),
            Containers::arrayView({0u}),
            Containers::arrayView({1u}),
            Containers::arrayView({nodeHandle(i, 0xece)}),
            stridedArrayView(offsetsSizes).slice(&Containers::Pair<Vector2, Vector2>::first),
            stridedArrayView(offsetsSizes).slice(&Containers::Pair<Vector2, Vector2>::second),
            Containers::arrayView({Vector2{}}),
            Containers::arrayView({Vector2{}}),
            stridedArrayView(compositeRectOffsetsSizes).slice(&Containers::Pair<Vector2, Vector2>::first),
            stridedArrayView(compositeRectOffsetsSizes).slice(&Containers::Pair<Vector2, Vector2>::second));
        CORRADE_COMPARE(compositeRectOffsetsSizes[0], expectedOffsetsSizes[i]);

        /* Swapping the node size and the clip rect should give the same
           result */
        Implementation::compositeRectsInto(
            Vector2{-1.0f}, Vector2{11.0f},
            Containers::arrayView({0u}),
            Containers::arrayView({0u}),
            Containers::arrayView({1u}),
            Containers::arrayView({nodeHandle(0, 0xece)}),
            Containers::arrayView({Vector2{1.0f}}),
            Containers::arrayView({Vector2{7.0f}}),
            Containers::arrayView({offsetsSizes[i].first()}),
            Containers::arrayView({offsetsSizes[i].second()}),
            stridedArrayView(compositeRectOffsetsSizes).slice(&Containers::Pair<Vector2, Vector2>::first),
            stridedArrayView(compositeRectOffsetsSizes).slice(&Containers::Pair<Vector2, Vector2>::second));
        CORRADE_COMPARE(compositeRectOffsetsSizes[0], expectedOffsetsSizes[i]);

        /* Swapping the node size and the UI size with an no clip rect should
           give the same result */
        Implementation::compositeRectsInto(
            offsetsSizes[i].first(), offsetsSizes[i].second(),
            Containers::arrayView({0u}),
            Containers::arrayView({0u}),
            Containers::arrayView({1u}),
            Containers::arrayView({nodeHandle(0, 0xece)}),
            Containers::arrayView({Vector2{1.0f}}),
            Containers::arrayView({Vector2{7.0f}}),
            Containers::arrayView({Vector2{}}),
            Containers::arrayView({Vector2{}}),
            stridedArrayView(compositeRectOffsetsSizes).slice(&Containers::Pair<Vector2, Vector2>::first),
            stridedArrayView(compositeRectOffsetsSizes).slice(&Containers::Pair<Vector2, Vector2>::second));
        CORRADE_COMPARE(compositeRectOffsetsSizes[0], expectedOffsetsSizes[i]);
    }
}

void AbstractUserInterfaceImplementationTest::compositingRects() {
    /* Verifying just that the clip rects get used for the right nodes. In
       particular, the data -> node mapping is nontrivial and each node is
       only visible in the clip rect it should belong to. The intersection
       calculation is tested in drawBoundsEdges() above, this verifies that the
       union is calculated correctly as well.

           0 1 2 3   4 5 6 7   8 9  10 11
         0
         1     +-----------+
         2     |     +---+ |
         3   +---+   | 3 | |
         4   | 2 |   +---+ |   +-----+
         5   +---+     +-------|  6  |-+
         6     |       |       +-----+ |
         7     +-------|   +---+       |
         8   rect 0    |   | 7 | +---+ |
         9             +---+---+-| 4 |-+
        10             rect 2    +---+    */

    UnsignedInt dataIds[]{
        1, 8, 0, 10, /* first clip rect, two data using the same node */
        5, 9         /* second clip rect */
    };
    NodeHandle dataNodes[]{
        nodeHandle(6, 0xcec), /* 0 */
        nodeHandle(7, 0xcec), /* 1 */
        NodeHandle::Null,     /* 2, unused */
        NodeHandle::Null,     /* 3, unused */
        NodeHandle::Null,     /* 4, unused */
        nodeHandle(3, 0xcec), /* 5 */
        NodeHandle::Null,     /* 6, unused */
        NodeHandle::Null,     /* 7, unused */
        nodeHandle(4, 0xcec), /* 8 */
        nodeHandle(2, 0xcec), /* 9 */
        nodeHandle(6, 0xcec), /* 10 */
    };
    Containers::Pair<Vector2, Vector2> nodeOffsetsSizes[]{
        {},                           /* 0, unused */
        {},                           /* 1, unused */
        {{1.0f, 3.0f}, {2.0f, 2.0f}}, /* 2 */
        {{4.0f, 2.0f}, {2.0f, 2.0f}}, /* 3 */
        {{9.0f, 8.0f}, {1.0f, 2.0f}}, /* 4 */
        {},                           /* 5, unused */
        {{8.0f, 4.0f}, {2.0f, 2.0f}}, /* 6 */
        {{7.0f, 7.0f}, {1.0f, 2.0f}}, /* 7 */
    };
    Containers::Pair<Vector2, Vector2> clipRectOffsetsSizes[]{
        {{2.0f, 1.0f}, {5.0f, 6.0f}}, /* 0 */
        {},                           /* 1, unused */
        {{5.0f, 5.0f}, {6.0f, 4.0f}}, /* 2 */
    };

    /* With a sufficiently large UI size it should clip just on the left and
       bottom */
    {
        Containers::Pair<Vector2, Vector2> compositeRectOffsetsSizes[Containers::arraySize(dataIds)];

        Implementation::compositeRectsInto(
            {0.0f, 0.0f}, {100.0f, 100.0f},
            dataIds,
            Containers::arrayView({2u, 0u}),
            Containers::arrayView({4u, 2u}),
            dataNodes,
            stridedArrayView(nodeOffsetsSizes).slice(&Containers::Pair<Vector2, Vector2>::first),
            stridedArrayView(nodeOffsetsSizes).slice(&Containers::Pair<Vector2, Vector2>::second),
            stridedArrayView(clipRectOffsetsSizes).slice(&Containers::Pair<Vector2, Vector2>::first),
            stridedArrayView(clipRectOffsetsSizes).slice(&Containers::Pair<Vector2, Vector2>::second),
            stridedArrayView(compositeRectOffsetsSizes).slice(&Containers::Pair<Vector2, Vector2>::first),
            stridedArrayView(compositeRectOffsetsSizes).slice(&Containers::Pair<Vector2, Vector2>::second));
        CORRADE_COMPARE_AS(Containers::arrayView(compositeRectOffsetsSizes), (Containers::arrayView<Containers::Pair<Vector2, Vector2>>({
            {{7.0f, 7.0f}, {1.0f, 2.0f}}, /* 1, node 7 */
            {{9.0f, 8.0f}, {1.0f, 1.0f}}, /* 8, node 4, clipped on the bottom */
            {{8.0f, 5.0f}, {2.0f, 1.0f}}, /* 0, node 6, clipped on the top */
            {{8.0f, 5.0f}, {2.0f, 1.0f}}, /* 10, node 6 again, cliped again */
            {{4.0f, 2.0f}, {2.0f, 2.0f}}, /* 5, node 3 */
            {{2.0f, 3.0f}, {1.0f, 2.0f}}  /* 9, node 2, clipped on the left */
        })), TestSuite::Compare::Container);
    }

    /* With a smaller UI size it clips also on the top and right. The clip
       rects are expected to be clipped against the UI rect already. */
    {
        Containers::Pair<Vector2, Vector2> compositeRectOffsetsSizes[Containers::arraySize(dataIds)];

        Containers::Pair<Vector2, Vector2> clipRectOffsetsSizesUiClipped[]{
            {{2.0f, 3.0f}, {5.0f, 3.0f}},
            {},
            {{5.0f, 5.0f}, {4.5f, 4.0f}},
        };
        Implementation::compositeRectsInto(
            {0.0f, 3.0f}, {9.5f, 100.0f},
            dataIds,
            Containers::arrayView({2u, 0u}),
            Containers::arrayView({4u, 2u}),
            dataNodes,
            stridedArrayView(nodeOffsetsSizes).slice(&Containers::Pair<Vector2, Vector2>::first),
            stridedArrayView(nodeOffsetsSizes).slice(&Containers::Pair<Vector2, Vector2>::second),
            stridedArrayView(clipRectOffsetsSizesUiClipped).slice(&Containers::Pair<Vector2, Vector2>::first),
            stridedArrayView(clipRectOffsetsSizesUiClipped).slice(&Containers::Pair<Vector2, Vector2>::second),
            stridedArrayView(compositeRectOffsetsSizes).slice(&Containers::Pair<Vector2, Vector2>::first),
            stridedArrayView(compositeRectOffsetsSizes).slice(&Containers::Pair<Vector2, Vector2>::second));
        CORRADE_COMPARE_AS(Containers::arrayView(compositeRectOffsetsSizes), (Containers::arrayView<Containers::Pair<Vector2, Vector2>>({
            {{7.0f, 7.0f}, {1.0f, 2.0f}}, /* 1, node 7 */
            {{9.0f, 8.0f}, {0.5f, 1.0f}}, /* 8, node 4, bottom & right */
            {{8.0f, 5.0f}, {1.5f, 1.0f}}, /* 0, node 6, top & right */
            {{8.0f, 5.0f}, {1.5f, 1.0f}}, /* 10, node 6 again, cliped again */
            {{4.0f, 3.0f}, {2.0f, 1.0f}}, /* 5, node 3 top */
            {{2.0f, 3.0f}, {1.0f, 2.0f}}  /* 9, node 2, left */
        })), TestSuite::Compare::Container);
    }
}

void AbstractUserInterfaceImplementationTest::partitionedAnimatorsInsert() {
    AbstractAnimator& animator1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef01});
    AbstractAnimator& animator2 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef02});
    AbstractAnimator& animatorNodeAttachment1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef03});
    AbstractAnimator& animatorNodeAttachment2 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef04});
    AbstractAnimator& animatorNodeAttachment3 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef05});
    AbstractAnimator& animatorNode1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef06});
    AbstractAnimator& animatorNode2 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef07});
    AbstractAnimator& animatorLayer0DataAttachment1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef08});
    AbstractAnimator& animatorLayer0DataAttachment2 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef09});
    AbstractAnimator& animatorLayer2DataAttachment1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef0a});
    AbstractAnimator& animatorLayer3DataAttachment1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef0b});
    AbstractAnimator& animatorLayer3DataAttachment2 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef0c});
    AbstractAnimator& animatorLayer3Data1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef0d});
    AbstractAnimator& animatorLayer3Data2 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef0e});
    AbstractAnimator& animatorLayer3Data3 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef0f});
    AbstractAnimator& animatorLayer3Style1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef10});
    AbstractAnimator& animatorLayer3Style2 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef11});
    AbstractAnimator& animatorLayer4DataAttachment1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef12});
    AbstractAnimator& animatorLayer4DataAttachment2 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef13});
    AbstractAnimator& animatorLayer4Data1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef14});
    AbstractAnimator& animatorLayer4Style1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef15});

    Containers::Array<Containers::Reference<AbstractAnimator>> instances{InPlaceInit, {
        animator1,                      /*  0 */
        animatorNodeAttachment2,        /*  1 */
        animatorNodeAttachment1,        /*  2 */
        animatorNode1,                  /*  3 */
        animatorLayer0DataAttachment2,  /*  4 */
        animatorLayer0DataAttachment1,  /*  5 */
        animatorLayer3DataAttachment1,  /*  6 */
        animatorLayer3Data2,            /*  7 */
        animatorLayer3Data1,            /*  8 */
        animatorLayer3Style1,           /*  9 */
        animatorLayer4DataAttachment1   /* 10 */
    }};
    UnsignedInt nodeAttachmentAnimatorOffset = 1;
    UnsignedInt nodeAnimatorOffset = 3;
    UnsignedShort dataAttachmentAnimatorOffsets[]{
        4,  /* Layer 0 has two attachments */
        6,  /* Layer 1 has none */
        6,  /* Layer 2 has none */
        6,  /* Layer 3 has four */
        10, /* Layer 4 has one (i.e., until the end of the instance list) */
    };
    UnsignedShort dataAnimatorOffsets[]{
        6,  /* Layer 0 has no data animators */
        6,  /* Layer 1 has none */
        6,  /* Layer 2 has none */
        7,  /* Layer 3 has two */
        11, /* Layer 4 has none (i.e., until the end of the instance list) */
    };
    UnsignedShort styleAnimatorOffsets[]{
        6,  /* Layer 0 has no style animators */
        6,  /* Layer 1 has none */
        6,  /* Layer 2 has none */
        9,  /* Layer 3 has one */
        11, /* Layer 4 has none (i.e., until the end of the instance list) */
    };

    /* Insert a non-*Attachment animator. Containers::Reference has the same
       layout as a pointer, abuse that for easy comparison. */
    Implementation::partitionedAnimatorsInsert(instances, animator2, Implementation::AnimatorType::Generic, AnimatorFeatures{}, LayerHandle::Null, nodeAttachmentAnimatorOffset, nodeAnimatorOffset, dataAttachmentAnimatorOffsets, dataAnimatorOffsets, styleAnimatorOffsets);
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator*>(instances), Containers::arrayView<AbstractAnimator*>({
        &animator1,                     /*  0 */
        &animator2,                     /*  1 */
        &animatorNodeAttachment2,       /*  2 */
        &animatorNodeAttachment1,       /*  3 */
        &animatorNode1,                 /*  4 */
        &animatorLayer0DataAttachment2, /*  5 */
        &animatorLayer0DataAttachment1, /*  6 */
        &animatorLayer3DataAttachment1, /*  7 */
        &animatorLayer3Data2,           /*  8 */
        &animatorLayer3Data1,           /*  9 */
        &animatorLayer3Style1,          /* 10 */
        &animatorLayer4DataAttachment1  /* 11 */
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(nodeAttachmentAnimatorOffset, 2);
    CORRADE_COMPARE(nodeAnimatorOffset, 4);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAttachmentAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        5, 7, 7, 7, 11
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        7, 7, 7, 8, 12
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(styleAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        7, 7, 7, 10, 12
    }), TestSuite::Compare::Container);

    /* Insert a NodeAttachment animator */
    Implementation::partitionedAnimatorsInsert(instances, animatorNodeAttachment3, Implementation::AnimatorType::Generic, AnimatorFeature::NodeAttachment, LayerHandle::Null, nodeAttachmentAnimatorOffset, nodeAnimatorOffset, dataAttachmentAnimatorOffsets, dataAnimatorOffsets, styleAnimatorOffsets);
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator*>(instances), Containers::arrayView<AbstractAnimator*>({
        &animator1,                     /*  0 */
        &animator2,                     /*  1 */
        &animatorNodeAttachment2,       /*  2 */
        &animatorNodeAttachment1,       /*  3 */
        &animatorNodeAttachment3,       /*  4 */
        &animatorNode1,                 /*  5 */
        &animatorLayer0DataAttachment2, /*  6 */
        &animatorLayer0DataAttachment1, /*  7 */
        &animatorLayer3DataAttachment1, /*  8 */
        &animatorLayer3Data2,           /*  9 */
        &animatorLayer3Data1,           /* 10 */
        &animatorLayer3Style1,          /* 11 */
        &animatorLayer4DataAttachment1  /* 12 */
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(nodeAttachmentAnimatorOffset, 2);
    CORRADE_COMPARE(nodeAnimatorOffset, 5);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAttachmentAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        6, 8, 8, 8, 12
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        8, 8, 8, 9, 13
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(styleAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        8, 8, 8, 11, 13
    }), TestSuite::Compare::Container);

    /* Insert a DataAttachment animator into a layer that's empty so far */
    Implementation::partitionedAnimatorsInsert(instances, animatorLayer2DataAttachment1, Implementation::AnimatorType::Generic, AnimatorFeature::DataAttachment, layerHandle(2, 0xbc), nodeAttachmentAnimatorOffset, nodeAnimatorOffset, dataAttachmentAnimatorOffsets, dataAnimatorOffsets, styleAnimatorOffsets);
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator*>(instances), Containers::arrayView<AbstractAnimator*>({
        &animator1,                     /*  0 */
        &animator2,                     /*  1 */
        &animatorNodeAttachment2,       /*  2 */
        &animatorNodeAttachment1,       /*  3 */
        &animatorNodeAttachment3,       /*  4 */
        &animatorNode1,                 /*  5 */
        &animatorLayer0DataAttachment2, /*  6 */
        &animatorLayer0DataAttachment1, /*  7 */
        &animatorLayer2DataAttachment1, /*  8 */
        &animatorLayer3DataAttachment1, /*  9 */
        &animatorLayer3Data2,           /* 10 */
        &animatorLayer3Data1,           /* 11 */
        &animatorLayer3Style1,          /* 12 */
        &animatorLayer4DataAttachment1  /* 13 */
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(nodeAttachmentAnimatorOffset, 2);
    CORRADE_COMPARE(nodeAnimatorOffset, 5);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAttachmentAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        6, 8, 8, 9, 13
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        8, 8, 9, 10, 14
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(styleAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        8, 8, 9, 12, 14
    }), TestSuite::Compare::Container);

    /* Insert a DataAttachment animator into a layer that already has
       AbstractDataAnimator instances */
    Implementation::partitionedAnimatorsInsert(instances, animatorLayer3DataAttachment2, Implementation::AnimatorType::Generic, AnimatorFeature::DataAttachment, layerHandle(3, 0xee), nodeAttachmentAnimatorOffset, nodeAnimatorOffset, dataAttachmentAnimatorOffsets, dataAnimatorOffsets, styleAnimatorOffsets);
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator*>(instances), Containers::arrayView<AbstractAnimator*>({
        &animator1,                     /*  0 */
        &animator2,                     /*  1 */
        &animatorNodeAttachment2,       /*  2 */
        &animatorNodeAttachment1,       /*  3 */
        &animatorNodeAttachment3,       /*  4 */
        &animatorNode1,                 /*  5 */
        &animatorLayer0DataAttachment2, /*  6 */
        &animatorLayer0DataAttachment1, /*  7 */
        &animatorLayer2DataAttachment1, /*  8 */
        &animatorLayer3DataAttachment1, /*  9 */
        &animatorLayer3DataAttachment2, /* 10 */
        &animatorLayer3Data2,           /* 11 */
        &animatorLayer3Data1,           /* 12 */
        &animatorLayer3Style1,          /* 13 */
        &animatorLayer4DataAttachment1  /* 14 */
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(nodeAttachmentAnimatorOffset, 2);
    CORRADE_COMPARE(nodeAnimatorOffset, 5);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAttachmentAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        6, 8, 8, 9, 14
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        8, 8, 9, 11, 15
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(styleAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        8, 8, 9, 13, 15
    }), TestSuite::Compare::Container);

    /* Insert a DataAttachment animator into the last layer */
    Implementation::partitionedAnimatorsInsert(instances, animatorLayer4DataAttachment2, Implementation::AnimatorType::Generic, AnimatorFeature::DataAttachment, layerHandle(4, 0x66), nodeAttachmentAnimatorOffset, nodeAnimatorOffset, dataAttachmentAnimatorOffsets, dataAnimatorOffsets, styleAnimatorOffsets);
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator*>(instances), Containers::arrayView<AbstractAnimator*>({
        &animator1,                     /*  0 */
        &animator2,                     /*  1 */
        &animatorNodeAttachment2,       /*  2 */
        &animatorNodeAttachment1,       /*  3 */
        &animatorNodeAttachment3,       /*  4 */
        &animatorNode1,                 /*  5 */
        &animatorLayer0DataAttachment2, /*  6 */
        &animatorLayer0DataAttachment1, /*  7 */
        &animatorLayer2DataAttachment1, /*  8 */
        &animatorLayer3DataAttachment1, /*  9 */
        &animatorLayer3DataAttachment2, /* 10 */
        &animatorLayer3Data2,           /* 11 */
        &animatorLayer3Data1,           /* 12 */
        &animatorLayer3Style1,          /* 13 */
        &animatorLayer4DataAttachment1, /* 14 */
        &animatorLayer4DataAttachment2  /* 15 */
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(nodeAttachmentAnimatorOffset, 2);
    CORRADE_COMPARE(nodeAnimatorOffset, 5);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAttachmentAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        6, 8, 8, 9, 14
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        8, 8, 9, 11, 16
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(styleAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        8, 8, 9, 13, 16
    }), TestSuite::Compare::Container);

    /* Insert an AbstractNodeAnimator */
    Implementation::partitionedAnimatorsInsert(instances, animatorNode2, Implementation::AnimatorType::Node, AnimatorFeature::NodeAttachment, LayerHandle::Null, nodeAttachmentAnimatorOffset, nodeAnimatorOffset, dataAttachmentAnimatorOffsets, dataAnimatorOffsets, styleAnimatorOffsets);
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator*>(instances), Containers::arrayView<AbstractAnimator*>({
        &animator1,                     /*  0 */
        &animator2,                     /*  1 */
        &animatorNodeAttachment2,       /*  2 */
        &animatorNodeAttachment1,       /*  3 */
        &animatorNodeAttachment3,       /*  4 */
        &animatorNode1,                 /*  5 */
        &animatorNode2,                 /*  6 */
        &animatorLayer0DataAttachment2, /*  7 */
        &animatorLayer0DataAttachment1, /*  8 */
        &animatorLayer2DataAttachment1, /*  9 */
        &animatorLayer3DataAttachment1, /* 10 */
        &animatorLayer3DataAttachment2, /* 11 */
        &animatorLayer3Data2,           /* 12 */
        &animatorLayer3Data1,           /* 13 */
        &animatorLayer3Style1,          /* 14 */
        &animatorLayer4DataAttachment1, /* 15 */
        &animatorLayer4DataAttachment2  /* 16 */
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(nodeAttachmentAnimatorOffset, 2);
    CORRADE_COMPARE(nodeAnimatorOffset, 5);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAttachmentAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        7, 9, 9, 10, 15
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        9, 9, 10, 12, 17
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(styleAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        9, 9, 10, 14, 17
    }), TestSuite::Compare::Container);

    /* Insert an AbstractDataAnimator to a layer that already has some */
    Implementation::partitionedAnimatorsInsert(instances, animatorLayer3Data3, Implementation::AnimatorType::Data, AnimatorFeature::DataAttachment, layerHandle(3, 0x33), nodeAttachmentAnimatorOffset, nodeAnimatorOffset, dataAttachmentAnimatorOffsets, dataAnimatorOffsets, styleAnimatorOffsets);
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator*>(instances), Containers::arrayView<AbstractAnimator*>({
        &animator1,                     /*  0 */
        &animator2,                     /*  1 */
        &animatorNodeAttachment2,       /*  2 */
        &animatorNodeAttachment1,       /*  3 */
        &animatorNodeAttachment3,       /*  4 */
        &animatorNode1,                 /*  5 */
        &animatorNode2,                 /*  6 */
        &animatorLayer0DataAttachment2, /*  7 */
        &animatorLayer0DataAttachment1, /*  8 */
        &animatorLayer2DataAttachment1, /*  9 */
        &animatorLayer3DataAttachment1, /* 10 */
        &animatorLayer3DataAttachment2, /* 11 */
        &animatorLayer3Data2,           /* 12 */
        &animatorLayer3Data1,           /* 13 */
        &animatorLayer3Data3,           /* 14 */
        &animatorLayer3Style1,          /* 15 */
        &animatorLayer4DataAttachment1, /* 16 */
        &animatorLayer4DataAttachment2  /* 17 */
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(nodeAttachmentAnimatorOffset, 2);
    CORRADE_COMPARE(nodeAnimatorOffset, 5);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAttachmentAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        7, 9, 9, 10, 16
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        9, 9, 10, 12, 18
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(styleAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        9, 9, 10, 15, 18
    }), TestSuite::Compare::Container);

    /* Insert an AbstractDataAnimator to the last layer that has none so far */
    Implementation::partitionedAnimatorsInsert(instances, animatorLayer4Data1, Implementation::AnimatorType::Data, AnimatorFeature::DataAttachment, layerHandle(4, 0x22), nodeAttachmentAnimatorOffset, nodeAnimatorOffset, dataAttachmentAnimatorOffsets, dataAnimatorOffsets, styleAnimatorOffsets);
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator*>(instances), Containers::arrayView<AbstractAnimator*>({
        &animator1,                     /*  0 */
        &animator2,                     /*  1 */
        &animatorNodeAttachment2,       /*  2 */
        &animatorNodeAttachment1,       /*  3 */
        &animatorNodeAttachment3,       /*  4 */
        &animatorNode1,                 /*  5 */
        &animatorNode2,                 /*  6 */
        &animatorLayer0DataAttachment2, /*  7 */
        &animatorLayer0DataAttachment1, /*  8 */
        &animatorLayer2DataAttachment1, /*  9 */
        &animatorLayer3DataAttachment1, /* 10 */
        &animatorLayer3DataAttachment2, /* 11 */
        &animatorLayer3Data2,           /* 12 */
        &animatorLayer3Data1,           /* 13 */
        &animatorLayer3Data3,           /* 14 */
        &animatorLayer3Style1,          /* 15 */
        &animatorLayer4DataAttachment1, /* 16 */
        &animatorLayer4DataAttachment2, /* 17 */
        &animatorLayer4Data1            /* 18 */
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(nodeAttachmentAnimatorOffset, 2);
    CORRADE_COMPARE(nodeAnimatorOffset, 5);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAttachmentAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        7, 9, 9, 10, 16
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        9, 9, 10, 12, 18
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(styleAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        9, 9, 10, 15, 19
    }), TestSuite::Compare::Container);

    /* Insert an AbstractDataAnimator to a layer that already has some */
    Implementation::partitionedAnimatorsInsert(instances, animatorLayer3Style2, Implementation::AnimatorType::Style, AnimatorFeature::DataAttachment, layerHandle(3, 0x11), nodeAttachmentAnimatorOffset, nodeAnimatorOffset, dataAttachmentAnimatorOffsets, dataAnimatorOffsets, styleAnimatorOffsets);
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator*>(instances), Containers::arrayView<AbstractAnimator*>({
        &animator1,                     /*  0 */
        &animator2,                     /*  1 */
        &animatorNodeAttachment2,       /*  2 */
        &animatorNodeAttachment1,       /*  3 */
        &animatorNodeAttachment3,       /*  4 */
        &animatorNode1,                 /*  5 */
        &animatorNode2,                 /*  6 */
        &animatorLayer0DataAttachment2, /*  7 */
        &animatorLayer0DataAttachment1, /*  8 */
        &animatorLayer2DataAttachment1, /*  9 */
        &animatorLayer3DataAttachment1, /* 10 */
        &animatorLayer3DataAttachment2, /* 11 */
        &animatorLayer3Data2,           /* 12 */
        &animatorLayer3Data1,           /* 13 */
        &animatorLayer3Data3,           /* 14 */
        &animatorLayer3Style1,          /* 15 */
        &animatorLayer3Style2,          /* 16 */
        &animatorLayer4DataAttachment1, /* 17 */
        &animatorLayer4DataAttachment2, /* 18 */
        &animatorLayer4Data1            /* 19 */
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(nodeAttachmentAnimatorOffset, 2);
    CORRADE_COMPARE(nodeAnimatorOffset, 5);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAttachmentAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        7, 9, 9, 10, 17
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        9, 9, 10, 12, 19
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(styleAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        9, 9, 10, 15, 20
    }), TestSuite::Compare::Container);

    /* Insert an AbstractDataAnimator to the last layer that has none so far */
    Implementation::partitionedAnimatorsInsert(instances, animatorLayer4Style1, Implementation::AnimatorType::Style, AnimatorFeature::DataAttachment, layerHandle(4, 0x77), nodeAttachmentAnimatorOffset, nodeAnimatorOffset, dataAttachmentAnimatorOffsets, dataAnimatorOffsets, styleAnimatorOffsets);
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator*>(instances), Containers::arrayView<AbstractAnimator*>({
        &animator1,                     /*  0 */
        &animator2,                     /*  1 */
        &animatorNodeAttachment2,       /*  2 */
        &animatorNodeAttachment1,       /*  3 */
        &animatorNodeAttachment3,       /*  4 */
        &animatorNode1,                 /*  5 */
        &animatorNode2,                 /*  6 */
        &animatorLayer0DataAttachment2, /*  7 */
        &animatorLayer0DataAttachment1, /*  8 */
        &animatorLayer2DataAttachment1, /*  9 */
        &animatorLayer3DataAttachment1, /* 10 */
        &animatorLayer3DataAttachment2, /* 11 */
        &animatorLayer3Data2,           /* 12 */
        &animatorLayer3Data1,           /* 13 */
        &animatorLayer3Data3,           /* 14 */
        &animatorLayer3Style1,          /* 15 */
        &animatorLayer3Style2,          /* 16 */
        &animatorLayer4DataAttachment1, /* 17 */
        &animatorLayer4DataAttachment2, /* 18 */
        &animatorLayer4Data1,           /* 19 */
        &animatorLayer4Style1           /* 20 */
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(nodeAttachmentAnimatorOffset, 2);
    CORRADE_COMPARE(nodeAnimatorOffset, 5);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAttachmentAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        7, 9, 9, 10, 17
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        9, 9, 10, 12, 19
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(styleAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        9, 9, 10, 15, 20
    }), TestSuite::Compare::Container);
}

void AbstractUserInterfaceImplementationTest::partitionedAnimatorsInsertNoLayers() {
    AbstractAnimator& animator1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef01});
    AbstractAnimator& animator2 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef02});
    AbstractAnimator& animatorNodeAttachment1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef03});
    AbstractAnimator& animatorNodeAttachment2 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef04});
    AbstractAnimator& animatorNodeAttachment3 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef05});
    AbstractAnimator& animatorNode1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef06});

    Containers::Array<Containers::Reference<AbstractAnimator>> instances{InPlaceInit, {
        animator1,
        animatorNodeAttachment2,
        animatorNodeAttachment1,
    }};
    UnsignedInt nodeAttachmentAnimatorOffset = 1;
    UnsignedInt nodeAnimatorOffset = 3;

    /* Insert a non-NodeAttachment animator. Containers::Reference has the same
       layout as a pointer, abuse that for easy comparison. */
    Implementation::partitionedAnimatorsInsert(instances, animator2, Implementation::AnimatorType::Generic, AnimatorFeatures{}, LayerHandle::Null, nodeAttachmentAnimatorOffset, nodeAnimatorOffset, {}, {}, {});
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator*>(instances), Containers::arrayView<AbstractAnimator*>({
        &animator1,
        &animator2,
        &animatorNodeAttachment2,
        &animatorNodeAttachment1
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(nodeAttachmentAnimatorOffset, 2);
    CORRADE_COMPARE(nodeAnimatorOffset, 4);

    /* Insert a NodeAttachment animator */
    Implementation::partitionedAnimatorsInsert(instances, animatorNodeAttachment3, Implementation::AnimatorType::Generic, AnimatorFeature::NodeAttachment, LayerHandle::Null, nodeAttachmentAnimatorOffset, nodeAnimatorOffset, {}, {}, {});
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator*>(instances), Containers::arrayView<AbstractAnimator*>({
        &animator1,
        &animator2,
        &animatorNodeAttachment2,
        &animatorNodeAttachment1,
        &animatorNodeAttachment3
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(nodeAttachmentAnimatorOffset, 2);
    CORRADE_COMPARE(nodeAnimatorOffset, 5);

    /* Insert an AbstractNodeAnimator */
    Implementation::partitionedAnimatorsInsert(instances, animatorNode1, Implementation::AnimatorType::Node, AnimatorFeature::NodeAttachment, LayerHandle::Null, nodeAttachmentAnimatorOffset, nodeAnimatorOffset, {}, {}, {});
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator*>(instances), Containers::arrayView<AbstractAnimator*>({
        &animator1,
        &animator2,
        &animatorNodeAttachment2,
        &animatorNodeAttachment1,
        &animatorNodeAttachment3,
        &animatorNode1
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(nodeAttachmentAnimatorOffset, 2);
    CORRADE_COMPARE(nodeAnimatorOffset, 5);
}

void AbstractUserInterfaceImplementationTest::partitionedAnimatorsRemove() {
    AbstractAnimator& animator1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef01});
    AbstractAnimator& animator2 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef02});
    AbstractAnimator& animator3 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef03});
    AbstractAnimator& animatorNodeAttachment1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef04});
    AbstractAnimator& animatorNode1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef05});
    AbstractAnimator& animatorNode2 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef06});
    AbstractAnimator& animatorLayer0DataAttachment1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef07});
    AbstractAnimator& animatorLayer2DataAttachment1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef08});
    AbstractAnimator& animatorLayer2DataAttachment2 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef09});
    AbstractAnimator& animatorLayer2Data1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef0a});
    AbstractAnimator& animatorLayer2Data2 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef0b});
    AbstractAnimator& animatorLayer2Style1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef0c});
    AbstractAnimator& animatorLayer2Style2 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef0d});
    AbstractAnimator& animatorLayer3DataAttachment1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef0e});
    AbstractAnimator& animatorLayer3Data1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef0f});
    AbstractAnimator& animatorLayer3Style1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef10});

    Containers::Array<Containers::Reference<AbstractAnimator>> instances{InPlaceInit, {
        animator2,                      /*  0 */
        animator3,                      /*  1 */
        animator1,                      /*  2 */
        animatorNodeAttachment1,        /*  3 */
        animatorNode2,                  /*  4 */
        animatorNode1,                  /*  5 */
        animatorLayer0DataAttachment1,  /*  6 */
        animatorLayer2DataAttachment2,  /*  7 */
        animatorLayer2DataAttachment1,  /*  8 */
        animatorLayer2Data2,            /*  9 */
        animatorLayer2Data1,            /* 10 */
        animatorLayer2Style1,           /* 11 */
        animatorLayer2Style2,           /* 12 */
        animatorLayer3DataAttachment1,  /* 13 */
        animatorLayer3Data1,            /* 14 */
        animatorLayer3Style1            /* 15 */
    }};
    UnsignedInt nodeAttachmentAnimatorOffset = 3;
    UnsignedInt nodeAnimatorOffset = 4;
    UnsignedShort dataAttachmentAnimatorOffsets[]{
        6,  /* Layer 0 has one attachment */
        7,  /* Layer 1 has none */
        7,  /* Layer 2 has six */
        13, /* Layer 3 has three (i.e., until the end of the instance list) */
    };
    UnsignedShort dataAnimatorOffsets[]{
        7,  /* Layer 0 has no data animators */
        7,  /* Layer 1 has none */
        9,  /* Layer 2 has two */
        14, /* Layer 3 has one (i.e., until the end of the instance list) */
    };
    UnsignedShort styleAnimatorOffsets[]{
        7,  /* Layer 0 has no style animators */
        7,  /* Layer 1 has none */
        11, /* Layer 2 has two */
        15, /* Layer 3 has one (i.e., until the end of the instance list) */
    };

    /* Remove from the middle of the non-NodeAttachment partition.
       Containers::Reference has the same layout as a pointer, abuse that for
       easy comparison. */
    Implementation::partitionedAnimatorsRemove(instances, animator3, AnimatorFeatures{}, LayerHandle::Null, nodeAttachmentAnimatorOffset, nodeAnimatorOffset, dataAttachmentAnimatorOffsets, dataAnimatorOffsets, styleAnimatorOffsets);
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator*>(instances), Containers::arrayView<AbstractAnimator*>({
        &animator2,                     /*  0 */
        &animator1,                     /*  1 */
        &animatorNodeAttachment1,       /*  2 */
        &animatorNode2,                 /*  3 */
        &animatorNode1,                 /*  4 */
        &animatorLayer0DataAttachment1, /*  5 */
        &animatorLayer2DataAttachment2, /*  6 */
        &animatorLayer2DataAttachment1, /*  7 */
        &animatorLayer2Data2,           /*  8 */
        &animatorLayer2Data1,           /*  9 */
        &animatorLayer2Style1,          /* 10 */
        &animatorLayer2Style2,          /* 11 */
        &animatorLayer3DataAttachment1, /* 12 */
        &animatorLayer3Data1,           /* 13 */
        &animatorLayer3Style1,          /* 14 */
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(nodeAttachmentAnimatorOffset, 2);
    CORRADE_COMPARE(nodeAnimatorOffset, 3);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAttachmentAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        5, 6, 6, 12
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        6, 6, 8, 13
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(styleAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        6, 6, 10, 14
    }), TestSuite::Compare::Container);

    /* Remove from the end of the non-NodeAttachment partition */
    Implementation::partitionedAnimatorsRemove(instances, animator1, AnimatorFeatures{}, LayerHandle::Null, nodeAttachmentAnimatorOffset, nodeAnimatorOffset, dataAttachmentAnimatorOffsets, dataAnimatorOffsets, styleAnimatorOffsets);
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator*>(instances), Containers::arrayView<AbstractAnimator*>({
        &animator2,                     /*  0 */
        &animatorNodeAttachment1,       /*  1 */
        &animatorNode2,                 /*  2 */
        &animatorNode1,                 /*  3 */
        &animatorLayer0DataAttachment1, /*  4 */
        &animatorLayer2DataAttachment2, /*  5 */
        &animatorLayer2DataAttachment1, /*  6 */
        &animatorLayer2Data2,           /*  7 */
        &animatorLayer2Data1,           /*  8 */
        &animatorLayer2Style1,          /*  9 */
        &animatorLayer2Style2,          /* 10 */
        &animatorLayer3DataAttachment1, /* 11 */
        &animatorLayer3Data1,           /* 12 */
        &animatorLayer3Style1,          /* 13 */
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(nodeAttachmentAnimatorOffset, 1);
    CORRADE_COMPARE(nodeAnimatorOffset, 2);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAttachmentAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        4, 5, 5, 11
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        5, 5, 7, 12
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(styleAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        5, 5, 9, 13
    }), TestSuite::Compare::Container);

    /* Remove an AbstractNodeAnimator */
    Implementation::partitionedAnimatorsRemove(instances, animatorNode2, AnimatorFeature::NodeAttachment, LayerHandle::Null, nodeAttachmentAnimatorOffset, nodeAnimatorOffset, dataAttachmentAnimatorOffsets, dataAnimatorOffsets, styleAnimatorOffsets);
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator*>(instances), Containers::arrayView<AbstractAnimator*>({
        &animator2,                     /*  0 */
        &animatorNodeAttachment1,       /*  1 */
        &animatorNode1,                 /*  2 */
        &animatorLayer0DataAttachment1, /*  3 */
        &animatorLayer2DataAttachment2, /*  4 */
        &animatorLayer2DataAttachment1, /*  5 */
        &animatorLayer2Data2,           /*  6 */
        &animatorLayer2Data1,           /*  7 */
        &animatorLayer2Style1,          /*  8 */
        &animatorLayer2Style2,          /*  9 */
        &animatorLayer3DataAttachment1, /* 10 */
        &animatorLayer3Data1,           /* 11 */
        &animatorLayer3Style1,          /* 12 */
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(nodeAttachmentAnimatorOffset, 1);
    CORRADE_COMPARE(nodeAnimatorOffset, 2);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAttachmentAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        3, 4, 4, 10
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        4, 4, 6, 11
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(styleAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        4, 4, 8, 12
    }), TestSuite::Compare::Container);

    /* Remove a NodeAttachment animator */
    Implementation::partitionedAnimatorsRemove(instances, animatorNodeAttachment1, AnimatorFeature::NodeAttachment, LayerHandle::Null, nodeAttachmentAnimatorOffset, nodeAnimatorOffset, dataAttachmentAnimatorOffsets, dataAnimatorOffsets, styleAnimatorOffsets);
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator*>(instances), Containers::arrayView<AbstractAnimator*>({
        &animator2,                     /*  0 */
        &animatorNode1,                 /*  1 */
        &animatorLayer0DataAttachment1, /*  2 */
        &animatorLayer2DataAttachment2, /*  3 */
        &animatorLayer2DataAttachment1, /*  4 */
        &animatorLayer2Data2,           /*  5 */
        &animatorLayer2Data1,           /*  6 */
        &animatorLayer2Style1,          /*  7 */
        &animatorLayer2Style2,          /*  8 */
        &animatorLayer3DataAttachment1, /*  9 */
        &animatorLayer3Data1,           /* 10 */
        &animatorLayer3Style1,          /* 11 */
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(nodeAttachmentAnimatorOffset, 1);
    CORRADE_COMPARE(nodeAnimatorOffset, 1);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAttachmentAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        2, 3, 3, 9
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        3, 3, 5, 10
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(styleAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        3, 3, 7, 11
    }), TestSuite::Compare::Container);

    /* Remove a DataAttachment animator */
    Implementation::partitionedAnimatorsRemove(instances, animatorLayer2DataAttachment1, AnimatorFeature::DataAttachment, layerHandle(2, 0xac), nodeAttachmentAnimatorOffset, nodeAnimatorOffset, dataAttachmentAnimatorOffsets, dataAnimatorOffsets, styleAnimatorOffsets);
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator*>(instances), Containers::arrayView<AbstractAnimator*>({
        &animator2,                     /*  0 */
        &animatorNode1,                 /*  1 */
        &animatorLayer0DataAttachment1, /*  2 */
        &animatorLayer2DataAttachment2, /*  3 */
        &animatorLayer2Data2,           /*  4 */
        &animatorLayer2Data1,           /*  5 */
        &animatorLayer2Style1,          /*  6 */
        &animatorLayer2Style2,          /*  7 */
        &animatorLayer3DataAttachment1, /*  8 */
        &animatorLayer3Data1,           /*  9 */
        &animatorLayer3Style1,          /* 10 */
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(nodeAttachmentAnimatorOffset, 1);
    CORRADE_COMPARE(nodeAnimatorOffset, 1);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAttachmentAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        2, 3, 3, 8
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        3, 3, 4, 9
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(styleAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        3, 3, 6, 10
    }), TestSuite::Compare::Container);

    /* Remove from the beginning of the non-NodeAttachment partition */
    Implementation::partitionedAnimatorsRemove(instances, animator2, AnimatorFeatures{}, LayerHandle::Null, nodeAttachmentAnimatorOffset, nodeAnimatorOffset, dataAttachmentAnimatorOffsets, dataAnimatorOffsets, styleAnimatorOffsets);
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator*>(instances), Containers::arrayView<AbstractAnimator*>({
        &animatorNode1,                 /*  0 */
        &animatorLayer0DataAttachment1, /*  1 */
        &animatorLayer2DataAttachment2, /*  2 */
        &animatorLayer2Data2,           /*  3 */
        &animatorLayer2Data1,           /*  4 */
        &animatorLayer2Style1,          /*  5 */
        &animatorLayer2Style2,          /*  6 */
        &animatorLayer3DataAttachment1, /*  7 */
        &animatorLayer3Data1,           /*  8 */
        &animatorLayer3Style1,          /*  9 */
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(nodeAttachmentAnimatorOffset, 0);
    CORRADE_COMPARE(nodeAnimatorOffset, 0);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAttachmentAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        1, 2, 2, 7
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        2, 2, 3, 8
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(styleAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        2, 2, 5, 9
    }), TestSuite::Compare::Container);

    /* Remove a DataAttachment animator from the last layer */
    Implementation::partitionedAnimatorsRemove(instances, animatorLayer3DataAttachment1, AnimatorFeature::DataAttachment, layerHandle(3, 0xac), nodeAttachmentAnimatorOffset, nodeAnimatorOffset, dataAttachmentAnimatorOffsets, dataAnimatorOffsets, styleAnimatorOffsets);
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator*>(instances), Containers::arrayView<AbstractAnimator*>({
        &animatorNode1,                 /*  0 */
        &animatorLayer0DataAttachment1, /*  1 */
        &animatorLayer2DataAttachment2, /*  2 */
        &animatorLayer2Data2,           /*  3 */
        &animatorLayer2Data1,           /*  4 */
        &animatorLayer2Style1,          /*  5 */
        &animatorLayer2Style2,          /*  6 */
        &animatorLayer3Data1,           /*  7 */
        &animatorLayer3Style1,          /*  8 */
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(nodeAttachmentAnimatorOffset, 0);
    CORRADE_COMPARE(nodeAnimatorOffset, 0);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAttachmentAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        1, 2, 2, 7
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        2, 2, 3, 7
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(styleAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        2, 2, 5, 8
    }), TestSuite::Compare::Container);

    /* Remove an AbstractStyleAnimator from the last layer */
    Implementation::partitionedAnimatorsRemove(instances, animatorLayer3Style1, AnimatorFeature::DataAttachment, layerHandle(3, 0xac), nodeAttachmentAnimatorOffset, nodeAnimatorOffset, dataAttachmentAnimatorOffsets, dataAnimatorOffsets, styleAnimatorOffsets);
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator*>(instances), Containers::arrayView<AbstractAnimator*>({
        &animatorNode1,                 /*  0 */
        &animatorLayer0DataAttachment1, /*  1 */
        &animatorLayer2DataAttachment2, /*  2 */
        &animatorLayer2Data2,           /*  3 */
        &animatorLayer2Data1,           /*  4 */
        &animatorLayer2Style1,          /*  5 */
        &animatorLayer2Style2,          /*  6 */
        &animatorLayer3Data1,           /*  7 */
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(nodeAttachmentAnimatorOffset, 0);
    CORRADE_COMPARE(nodeAnimatorOffset, 0);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAttachmentAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        1, 2, 2, 7
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        2, 2, 3, 7
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(styleAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        2, 2, 5, 8
    }), TestSuite::Compare::Container);

    /* Remove the first AbstractDataAnimator from a layer */
    Implementation::partitionedAnimatorsRemove(instances, animatorLayer2Data2, AnimatorFeature::DataAttachment, layerHandle(2, 0xcc), nodeAttachmentAnimatorOffset, nodeAnimatorOffset, dataAttachmentAnimatorOffsets, dataAnimatorOffsets, styleAnimatorOffsets);
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator*>(instances), Containers::arrayView<AbstractAnimator*>({
        &animatorNode1,                 /*  0 */
        &animatorLayer0DataAttachment1, /*  1 */
        &animatorLayer2DataAttachment2, /*  2 */
        &animatorLayer2Data1,           /*  3 */
        &animatorLayer2Style1,          /*  4 */
        &animatorLayer2Style2,          /*  5 */
        &animatorLayer3Data1            /*  6 */
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(nodeAttachmentAnimatorOffset, 0);
    CORRADE_COMPARE(nodeAnimatorOffset, 0);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAttachmentAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        1, 2, 2, 6
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        2, 2, 3, 6
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(styleAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        2, 2, 4, 7
    }), TestSuite::Compare::Container);

    /* Remove the second AbstractStyleAnimator from a layer */
    Implementation::partitionedAnimatorsRemove(instances, animatorLayer2Style2, AnimatorFeature::DataAttachment, layerHandle(2, 0xcc), nodeAttachmentAnimatorOffset, nodeAnimatorOffset, dataAttachmentAnimatorOffsets, dataAnimatorOffsets, styleAnimatorOffsets);
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator*>(instances), Containers::arrayView<AbstractAnimator*>({
        &animatorNode1,                 /*  0 */
        &animatorLayer0DataAttachment1, /*  1 */
        &animatorLayer2DataAttachment2, /*  2 */
        &animatorLayer2Data1,           /*  3 */
        &animatorLayer2Style1,          /*  4 */
        &animatorLayer3Data1            /*  5 */
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(nodeAttachmentAnimatorOffset, 0);
    CORRADE_COMPARE(nodeAnimatorOffset, 0);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAttachmentAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        1, 2, 2, 5
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        2, 2, 3, 5
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(styleAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        2, 2, 4, 6
    }), TestSuite::Compare::Container);

    /* Remove an AbstractDataAnimator from the last layer */
    Implementation::partitionedAnimatorsRemove(instances, animatorLayer3Data1, AnimatorFeature::DataAttachment, layerHandle(3, 0xec), nodeAttachmentAnimatorOffset, nodeAnimatorOffset, dataAttachmentAnimatorOffsets, dataAnimatorOffsets, styleAnimatorOffsets);
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator*>(instances), Containers::arrayView<AbstractAnimator*>({
        &animatorNode1,                 /*  0 */
        &animatorLayer0DataAttachment1, /*  1 */
        &animatorLayer2DataAttachment2, /*  2 */
        &animatorLayer2Data1,           /*  3 */
        &animatorLayer2Style1,          /*  4 */
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(nodeAttachmentAnimatorOffset, 0);
    CORRADE_COMPARE(nodeAnimatorOffset, 0);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAttachmentAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        1, 2, 2, 5
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        2, 2, 3, 5
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(styleAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        2, 2, 4, 5
    }), TestSuite::Compare::Container);
}

void AbstractUserInterfaceImplementationTest::partitionedAnimatorsRemoveNoLayers() {
    AbstractAnimator& animator1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef01});
    AbstractAnimator& animator2 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef02});
    AbstractAnimator& animator3 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef03});
    AbstractAnimator& animatorNodeAttachment1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef04});
    AbstractAnimator& animatorNode1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef05});

    Containers::Array<Containers::Reference<AbstractAnimator>> instances{InPlaceInit, {
        animator2,
        animator3,
        animator1,
        animatorNodeAttachment1,
        animatorNode1
    }};
    UnsignedInt nodeAttachmentAnimatorOffset = 3;
    UnsignedInt nodeAnimatorOffset = 4;

    /* Remove from the middle of the non-NodeAttachment partition.
       Containers::Reference has the same layout as a pointer, abuse that for
       easy comparison. */
    Implementation::partitionedAnimatorsRemove(instances, animator3, AnimatorFeatures{}, LayerHandle::Null, nodeAttachmentAnimatorOffset, nodeAnimatorOffset, {}, {}, {});
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator*>(instances), Containers::arrayView<AbstractAnimator*>({
        &animator2,
        &animator1,
        &animatorNodeAttachment1,
        &animatorNode1
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(nodeAttachmentAnimatorOffset, 2);
    CORRADE_COMPARE(nodeAnimatorOffset, 3);

    /* Remove from the end of the non-*Attachment partition */
    Implementation::partitionedAnimatorsRemove(instances, animator1, AnimatorFeatures{}, LayerHandle::Null, nodeAttachmentAnimatorOffset, nodeAnimatorOffset, {}, {}, {});
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator*>(instances), Containers::arrayView<AbstractAnimator*>({
        &animator2,
        &animatorNodeAttachment1,
        &animatorNode1
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(nodeAttachmentAnimatorOffset, 1);
    CORRADE_COMPARE(nodeAnimatorOffset, 2);

    /* Remove a NodeAttachment animator */
    Implementation::partitionedAnimatorsRemove(instances, animatorNodeAttachment1, AnimatorFeature::NodeAttachment, LayerHandle::Null, nodeAttachmentAnimatorOffset, nodeAnimatorOffset, {}, {}, {});
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator*>(instances), Containers::arrayView<AbstractAnimator*>({
        &animator2,
        &animatorNode1
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(nodeAttachmentAnimatorOffset, 1);
    CORRADE_COMPARE(nodeAnimatorOffset, 1);

    /* Remove an AbstractNodeAnimator */
    Implementation::partitionedAnimatorsRemove(instances, animatorNode1, AnimatorFeature::NodeAttachment, LayerHandle::Null, nodeAttachmentAnimatorOffset, nodeAnimatorOffset, {}, {}, {});
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator*>(instances), Containers::arrayView<AbstractAnimator*>({
        &animator2
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(nodeAttachmentAnimatorOffset, 1);
    CORRADE_COMPARE(nodeAnimatorOffset, 1);

    /* Remove from the beginning of the non-*Attachment partition */
    Implementation::partitionedAnimatorsRemove(instances, animator2, AnimatorFeatures{}, LayerHandle::Null, nodeAttachmentAnimatorOffset, nodeAnimatorOffset, {}, {}, {});
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator*>(instances), Containers::arrayView<AbstractAnimator*>({
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(nodeAttachmentAnimatorOffset, 0);
    CORRADE_COMPARE(nodeAnimatorOffset, 0);
}

void AbstractUserInterfaceImplementationTest::partitionedAnimatorsGet() {
    AbstractAnimator& animator1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef01});
    AbstractAnimator& animatorNodeAttachment1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef02});
    AbstractAnimator& animatorNodeAttachment2 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef03});
    AbstractAnimator& animatorNode1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef04});
    AbstractAnimator& animatorNode2 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef05});
    AbstractAnimator& animatorLayer0DataAttachment1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef06});
    AbstractAnimator& animatorLayer2DataAttachment1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef07});
    AbstractAnimator& animatorLayer2DataAttachment2 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef08});
    AbstractAnimator& animatorLayer2Data1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef09});
    AbstractAnimator& animatorLayer2Data2 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef0a});
    AbstractAnimator& animatorLayer2Style1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef0b});
    AbstractAnimator& animatorLayer2Style2 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef0c});
    AbstractAnimator& animatorLayer3DataAttachment1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef0d});
    AbstractAnimator& animatorLayer3Data1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef0e});
    AbstractAnimator& animatorLayer3Style1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef0f});

    Containers::Reference<AbstractAnimator> instances[]{
        animator1,                      /*  0 */
        animatorNodeAttachment2,        /*  1 */
        animatorNodeAttachment1,        /*  2 */
        animatorNode2,                  /*  3 */
        animatorNode1,                  /*  4 */
        animatorLayer0DataAttachment1,  /*  5 */
        animatorLayer2DataAttachment2,  /*  6 */
        animatorLayer2DataAttachment1,  /*  7 */
        animatorLayer2Data2,            /*  8 */
        animatorLayer2Data1,            /*  9 */
        animatorLayer2Style1,           /* 10 */
        animatorLayer2Style2,           /* 11 */
        animatorLayer3DataAttachment1,  /* 12 */
        animatorLayer3Data1,            /* 13 */
        animatorLayer3Style1            /* 14 */
    };
    UnsignedInt nodeAttachmentAnimatorOffset = 1;
    UnsignedInt nodeAnimatorOffset = 3;
    UnsignedShort dataAttachmentAnimatorOffsets[]{
        5,  /* Layer 0 has one attachment */
        6,  /* Layer 1 has none */
        6,  /* Layer 2 has six */
        12, /* Layer 3 has three (i.e., until the end of the instance list) */
    };
    UnsignedShort dataAnimatorOffsets[]{
        6,  /* Layer 0 has no data animators */
        6,  /* Layer 1 has none */
        8,  /* Layer 2 has two */
        13, /* Layer 3 has one (i.e., until the end of the instance list) */
    };
    UnsignedShort styleAnimatorOffsets[]{
        6,  /* Layer 0 has no style animators */
        6,  /* Layer 1 has none */
        10,  /* Layer 2 has two */
        14, /* Layer 3 has one (i.e., until the end of the instance list) */
    };

    /* Containers::Reference has the same layout as a pointer, abuse that for
       easy comparison */
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator* const>(Implementation::partitionedAnimatorsNone(instances, nodeAttachmentAnimatorOffset)), Containers::arrayView<AbstractAnimator*>({
        &animator1
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator* const>(Implementation::partitionedAnimatorsAnyNodeAttachment(instances, nodeAttachmentAnimatorOffset, dataAttachmentAnimatorOffsets)), Containers::arrayView<AbstractAnimator*>({
        &animatorNodeAttachment2,
        &animatorNodeAttachment1,
        &animatorNode2,
        &animatorNode1,
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator* const>(Implementation::partitionedAnimatorsGenericNodeAttachment(instances, nodeAttachmentAnimatorOffset, nodeAnimatorOffset, dataAttachmentAnimatorOffsets)), Containers::arrayView<AbstractAnimator*>({
        &animatorNodeAttachment2,
        &animatorNodeAttachment1
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator* const>(Implementation::partitionedAnimatorsNodeNodeAttachment(instances, nodeAttachmentAnimatorOffset, nodeAnimatorOffset, dataAttachmentAnimatorOffsets)), Containers::arrayView<AbstractAnimator*>({
        &animatorNode2,
        &animatorNode1
    }), TestSuite::Compare::Container);

    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator* const>(Implementation::partitionedAnimatorsAnyDataAttachment(instances, dataAttachmentAnimatorOffsets, layerHandle(0, 0xac))), Containers::arrayView<AbstractAnimator*>({
        &animatorLayer0DataAttachment1
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator* const>(Implementation::partitionedAnimatorsGenericDataAttachment(instances, dataAttachmentAnimatorOffsets, dataAnimatorOffsets, styleAnimatorOffsets, layerHandle(0, 0xac))), Containers::arrayView<AbstractAnimator*>({
        &animatorLayer0DataAttachment1
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator* const>(Implementation::partitionedAnimatorsDataDataAttachment(instances, dataAttachmentAnimatorOffsets, dataAnimatorOffsets, styleAnimatorOffsets, layerHandle(0, 0xac))), Containers::arrayView<AbstractAnimator*>({
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator* const>(Implementation::partitionedAnimatorsStyleDataAttachment(instances, dataAttachmentAnimatorOffsets, dataAnimatorOffsets, styleAnimatorOffsets, layerHandle(0, 0xac))), Containers::arrayView<AbstractAnimator*>({
    }), TestSuite::Compare::Container);

    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator* const>(Implementation::partitionedAnimatorsAnyDataAttachment(instances, dataAttachmentAnimatorOffsets, layerHandle(1, 0xac))), Containers::arrayView<AbstractAnimator*>({
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator* const>(Implementation::partitionedAnimatorsGenericDataAttachment(instances, dataAttachmentAnimatorOffsets, dataAnimatorOffsets, styleAnimatorOffsets, layerHandle(1, 0xac))), Containers::arrayView<AbstractAnimator*>({
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator* const>(Implementation::partitionedAnimatorsDataDataAttachment(instances, dataAttachmentAnimatorOffsets, dataAnimatorOffsets, styleAnimatorOffsets, layerHandle(1, 0xac))), Containers::arrayView<AbstractAnimator*>({
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator* const>(Implementation::partitionedAnimatorsStyleDataAttachment(instances, dataAttachmentAnimatorOffsets, dataAnimatorOffsets, styleAnimatorOffsets, layerHandle(1, 0xac))), Containers::arrayView<AbstractAnimator*>({
    }), TestSuite::Compare::Container);

    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator* const>(Implementation::partitionedAnimatorsAnyDataAttachment(instances, dataAttachmentAnimatorOffsets, layerHandle(2, 0xac))), Containers::arrayView<AbstractAnimator*>({
        &animatorLayer2DataAttachment2,
        &animatorLayer2DataAttachment1,
        &animatorLayer2Data2,
        &animatorLayer2Data1,
        &animatorLayer2Style1,
        &animatorLayer2Style2
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator* const>(Implementation::partitionedAnimatorsGenericDataAttachment(instances, dataAttachmentAnimatorOffsets, dataAnimatorOffsets, styleAnimatorOffsets, layerHandle(2, 0xac))), Containers::arrayView<AbstractAnimator*>({
        &animatorLayer2DataAttachment2,
        &animatorLayer2DataAttachment1,
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator* const>(Implementation::partitionedAnimatorsDataDataAttachment(instances, dataAttachmentAnimatorOffsets, dataAnimatorOffsets, styleAnimatorOffsets, layerHandle(2, 0xac))), Containers::arrayView<AbstractAnimator*>({
        &animatorLayer2Data2,
        &animatorLayer2Data1
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator* const>(Implementation::partitionedAnimatorsStyleDataAttachment(instances, dataAttachmentAnimatorOffsets, dataAnimatorOffsets, styleAnimatorOffsets, layerHandle(2, 0xac))), Containers::arrayView<AbstractAnimator*>({
        &animatorLayer2Style1,
        &animatorLayer2Style2
    }), TestSuite::Compare::Container);

    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator* const>(Implementation::partitionedAnimatorsAnyDataAttachment(instances, dataAttachmentAnimatorOffsets, layerHandle(3, 0xac))), Containers::arrayView<AbstractAnimator*>({
        &animatorLayer3DataAttachment1,
        &animatorLayer3Data1,
        &animatorLayer3Style1
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator* const>(Implementation::partitionedAnimatorsGenericDataAttachment(instances, dataAttachmentAnimatorOffsets, dataAnimatorOffsets, styleAnimatorOffsets, layerHandle(3, 0xac))), Containers::arrayView<AbstractAnimator*>({
        &animatorLayer3DataAttachment1
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator* const>(Implementation::partitionedAnimatorsDataDataAttachment(instances, dataAttachmentAnimatorOffsets, dataAnimatorOffsets, styleAnimatorOffsets, layerHandle(3, 0xac))), Containers::arrayView<AbstractAnimator*>({
        &animatorLayer3Data1
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator* const>(Implementation::partitionedAnimatorsStyleDataAttachment(instances, dataAttachmentAnimatorOffsets, dataAnimatorOffsets, styleAnimatorOffsets, layerHandle(3, 0xac))), Containers::arrayView<AbstractAnimator*>({
        &animatorLayer3Style1
    }), TestSuite::Compare::Container);
}

void AbstractUserInterfaceImplementationTest::partitionedAnimatorsGetNoLayers() {
    AbstractAnimator& animator1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef01});
    AbstractAnimator& animatorNodeAttachment1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef02});
    AbstractAnimator& animatorNodeAttachment2 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef03});
    AbstractAnimator& animatorNode1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef04});
    AbstractAnimator& animatorNode2 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef05});

    Containers::Reference<AbstractAnimator> instances[]{
        animator1,
        animatorNodeAttachment2,
        animatorNodeAttachment1,
        animatorNode2,
        animatorNode1
    };
    UnsignedInt nodeAttachmentAnimatorOffset = 1;
    UnsignedInt nodeAnimatorOffset = 3;

    /* Containers::Reference has the same layout as a pointer, abuse that for
       easy comparison */
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator* const>(Implementation::partitionedAnimatorsNone(instances, nodeAttachmentAnimatorOffset)), Containers::arrayView<AbstractAnimator*>({
        &animator1
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator* const>(Implementation::partitionedAnimatorsAnyNodeAttachment(instances, nodeAttachmentAnimatorOffset, {})), Containers::arrayView<AbstractAnimator*>({
        &animatorNodeAttachment2,
        &animatorNodeAttachment1,
        &animatorNode2,
        &animatorNode1,
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator* const>(Implementation::partitionedAnimatorsGenericNodeAttachment(instances, nodeAttachmentAnimatorOffset, nodeAnimatorOffset, {})), Containers::arrayView<AbstractAnimator*>({
        &animatorNodeAttachment2,
        &animatorNodeAttachment1
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator* const>(Implementation::partitionedAnimatorsNodeNodeAttachment(instances, nodeAttachmentAnimatorOffset, nodeAnimatorOffset, {})), Containers::arrayView<AbstractAnimator*>({
        &animatorNode2,
        &animatorNode1
    }), TestSuite::Compare::Container);
}

void AbstractUserInterfaceImplementationTest::partitionedAnimatorsCreateLayer() {
    AbstractAnimator& animator1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef01});
    AbstractAnimator& animator2 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef02});
    AbstractAnimator& animatorLayer0DataAttachment1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef03});
    AbstractAnimator& animatorLayer2DataAttachment1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef04});
    AbstractAnimator& animatorLayer2DataAttachment2 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef05});
    AbstractAnimator& animatorLayer2Data1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef06});
    AbstractAnimator& animatorLayer2Data2 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef07});
    AbstractAnimator& animatorLayer2Style1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef08});
    AbstractAnimator& animatorLayer2Style2 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef09});
    AbstractAnimator& animatorLayer3DataAttachment1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef0a});
    AbstractAnimator& animatorLayer3Data1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef0b});
    AbstractAnimator& animatorLayer3Style1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef0c});

    Containers::Array<Containers::Reference<AbstractAnimator>> instances{InPlaceInit, {
        animator2,                      /*  0 */
        animator1,                      /*  1 */
        animatorLayer0DataAttachment1,  /*  2 */
        animatorLayer2DataAttachment1,  /*  3 */
        animatorLayer2DataAttachment2,  /*  4 */
        animatorLayer2Data1,            /*  5 */
        animatorLayer2Data2,            /*  6 */
        animatorLayer2Style1,           /*  7 */
        animatorLayer2Style2,           /*  8 */
        animatorLayer3DataAttachment1,  /*  9 */
        animatorLayer3Data1,            /* 10 */
        animatorLayer3Style1            /* 11 */
    }};
    UnsignedShort dataAttachmentAnimatorOffsets[]{
        2,  /* Layer 0 has one attachment */
        3,  /* Layer 1 doesn't exist and thus has none */
        3,  /* Layer 2 has six */
        9,  /* Layer 3 has three (i.e., until the end of the instance list) */
        0,  /* To be used by a new layer */
    };
    UnsignedShort dataAnimatorOffsets[]{
        3,  /* Layer 0 has no data animators */
        3,  /* Layer 1 doesn't exist and thus has none */
        5,  /* Layer 2 has two */
        10, /* Layer 3 has one (i.e., until the end of the instance list) */
        0,  /* To be used by a new layer */
    };
    UnsignedShort styleAnimatorOffsets[]{
        3,  /* Layer 0 has no style animators */
        3,  /* Layer 1 doesn't exist and thus has none */
        5,  /* Layer 2 has two */
        11, /* Layer 3 has one (i.e., until the end of the instance list) */
        0,  /* To be used by a new layer */
    };

    /* Inserting into the middle is a no-op, the offsets should already have
       everything correct */
    Implementation::partitionedAnimatorsCreateLayer(instances,
        Containers::arrayView(dataAttachmentAnimatorOffsets).exceptSuffix(1),
        Containers::arrayView(dataAnimatorOffsets).exceptSuffix(1),
        Containers::arrayView(styleAnimatorOffsets).exceptSuffix(1),
        layerHandle(1, 0xec));
    CORRADE_COMPARE_AS(Containers::arrayView(dataAttachmentAnimatorOffsets).exceptSuffix(1), Containers::arrayView<UnsignedShort>({
        2, 3, 3, 9
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAnimatorOffsets).exceptSuffix(1), Containers::arrayView<UnsignedShort>({
        3, 3, 5, 10
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(styleAnimatorOffsets).exceptSuffix(1), Containers::arrayView<UnsignedShort>({
        3, 3, 5, 11
    }), TestSuite::Compare::Container);

    /* Inserting at the end modifies the last element */
    Implementation::partitionedAnimatorsCreateLayer(instances,
        dataAttachmentAnimatorOffsets,
        dataAnimatorOffsets,
        styleAnimatorOffsets,
        layerHandle(4, 0xec));
    CORRADE_COMPARE_AS(Containers::arrayView(dataAttachmentAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        2, 3, 3, 9, 12
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        3, 3, 5, 10, 12
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(styleAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        3, 3, 5, 11, 12
    }), TestSuite::Compare::Container);
}

void AbstractUserInterfaceImplementationTest::partitionedAnimatorsRemoveLayer() {
    AbstractAnimator& animator1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef01});
    AbstractAnimator& animator2 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef02});
    AbstractAnimator& animatorLayer0DataAttachment1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef03});
    AbstractAnimator& animatorLayer2DataAttachment1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef04});
    AbstractAnimator& animatorLayer2DataAttachment2 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef05});
    AbstractAnimator& animatorLayer2Data1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef06});
    AbstractAnimator& animatorLayer2Data2 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef07});
    AbstractAnimator& animatorLayer2Style1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef08});
    AbstractAnimator& animatorLayer2Style2 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef09});
    AbstractAnimator& animatorLayer3DataAttachment1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef0a});
    AbstractAnimator& animatorLayer3Data1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef0b});
    AbstractAnimator& animatorLayer3Style1 = *reinterpret_cast<AbstractAnimator*>(std::size_t{0xabcdef0c});

    Containers::Array<Containers::Reference<AbstractAnimator>> instances{InPlaceInit, {
        animator2,                      /*  0 */
        animator1,                      /*  1 */
        animatorLayer0DataAttachment1,  /*  2 */
        animatorLayer2DataAttachment1,  /*  3 */
        animatorLayer2DataAttachment2,  /*  4 */
        animatorLayer2Data1,            /*  5 */
        animatorLayer2Data2,            /*  6 */
        animatorLayer2Style1,           /*  7 */
        animatorLayer2Style2,           /*  8 */
        animatorLayer3DataAttachment1,  /*  9 */
        animatorLayer3Data1,            /* 10 */
        animatorLayer3Style1            /* 11 */
    }};
    UnsignedShort dataAttachmentAnimatorOffsets[]{
        2,  /* Layer 0 has one attachment */
        3,  /* Layer 1 doesn't exist and thus has none */
        3,  /* Layer 2 has six */
        9,  /* Layer 3 has two (i.e., until the end of the instance list) */
    };
    UnsignedShort dataAnimatorOffsets[]{
        3,  /* Layer 0 has no data animators */
        3,  /* Layer 1 doesn't exist and thus has none */
        5,  /* Layer 2 has two */
        10, /* Layer 3 has one (i.e., until the end of the instance list) */
    };
    UnsignedShort styleAnimatorOffsets[]{
        3,  /* Layer 0 has no style animators */
        3,  /* Layer 1 doesn't exist and thus has none */
        5,  /* Layer 2 has two */
        11, /* Layer 3 has one (i.e., until the end of the instance list) */
    };

    /* Removing from the middle */
    Implementation::partitionedAnimatorsRemoveLayer(instances, dataAttachmentAnimatorOffsets, dataAnimatorOffsets, styleAnimatorOffsets, layerHandle(2, 0xaa));
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator*>(instances), Containers::arrayView<AbstractAnimator*>({
        &animator2,                     /*  0 */
        &animator1,                     /*  1 */
        &animatorLayer0DataAttachment1, /*  2 */
        &animatorLayer3DataAttachment1, /*  3 */
        &animatorLayer3Data1,           /*  4 */
        &animatorLayer3Style1           /*  5 */
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAttachmentAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        2, 3, 3, 3
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        3, 3, 3, 4
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(styleAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        3, 3, 3, 5
    }), TestSuite::Compare::Container);

    /* Removing an already-empty layer is practically a no-op */
    Implementation::partitionedAnimatorsRemoveLayer(instances, dataAttachmentAnimatorOffsets, dataAnimatorOffsets, styleAnimatorOffsets, layerHandle(1, 0x33));
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator*>(instances), Containers::arrayView<AbstractAnimator*>({
        &animator2,                     /*  0 */
        &animator1,                     /*  1 */
        &animatorLayer0DataAttachment1, /*  2 */
        &animatorLayer3DataAttachment1, /*  3 */
        &animatorLayer3Data1,           /*  4 */
        &animatorLayer3Style1           /*  5 */
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAttachmentAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        2, 3, 3, 3
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        3, 3, 3, 4
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(styleAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        3, 3, 3, 5
    }), TestSuite::Compare::Container);

    /* Removing from the end */
    Implementation::partitionedAnimatorsRemoveLayer(instances, dataAttachmentAnimatorOffsets, dataAnimatorOffsets, styleAnimatorOffsets, layerHandle(3, 0x11));
    CORRADE_COMPARE_AS(Containers::arrayCast<AbstractAnimator*>(instances), Containers::arrayView<AbstractAnimator*>({
        &animator2,                     /*  0 */
        &animator1,                     /*  1 */
        &animatorLayer0DataAttachment1  /*  2 */
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAttachmentAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        2, 3, 3, 3
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(dataAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        3, 3, 3, 3
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(styleAnimatorOffsets), Containers::arrayView<UnsignedShort>({
        3, 3, 3, 3
    }), TestSuite::Compare::Container);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::AbstractUserInterfaceImplementationTest)
