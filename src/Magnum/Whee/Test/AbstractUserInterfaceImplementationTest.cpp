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
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Container.h>
#include <Corrade/TestSuite/Compare/Numeric.h>

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

    void orderVisibleNodeData();
    void orderVisibleNodeDataNoLayers();
    void orderVisibleNodeDataNoValidLayers();

    void orderNodeDataForEventHandling();
    void orderNodeDataForEventHandlingNoLayers();
    void orderNodeDataForEventHandlingNoVisibleNodes();
    void orderNodeDataForEventHandlingNoVisibleNodesNoLayers();
    void orderNodeDataForEventHandlingAllLayers();
};

AbstractUserInterfaceImplementationTest::AbstractUserInterfaceImplementationTest() {
    addTests({&AbstractUserInterfaceImplementationTest::orderNodesBreadthFirst,

              &AbstractUserInterfaceImplementationTest::orderVisibleNodesDepthFirst,
              &AbstractUserInterfaceImplementationTest::orderVisibleNodesDepthFirstSingleBranch,
              &AbstractUserInterfaceImplementationTest::orderVisibleNodesDepthFirstNoTopLevelNodes,

              &AbstractUserInterfaceImplementationTest::visibleTopLevelNodeIndices,

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

    UnsignedByte visibleNodeMask[2]{};
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
        Containers::MutableBitArrayView{visibleNodeMask, 0, 14},
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
