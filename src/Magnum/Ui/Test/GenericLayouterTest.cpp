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
#include <Corrade/Containers/Function.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/String.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Container.h>
#include <Corrade/TestSuite/Compare/String.h>
#include <Magnum/Math/Vector4.h>
#include <Magnum/Math/Swizzle.h>

#include "Magnum/Ui/AbstractUserInterface.h"
#include "Magnum/Ui/GenericLayouter.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/LayoutLayer.h"
#include "Magnum/Ui/NodeFlags.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct GenericLayouterTest: TestSuite::Tester {
    explicit GenericLayouterTest();

    void construct();
    void constructCopy();
    void constructMove();

    void addRemove();
    void addRemoveHandleRecycle();
    void addInvalid();
    /* There's no assert to trigger in remove() other than what's checked by
       AbstractAnimator::remove() already */

    void nodePropertiesNotInLayout();

    void clean();

    void layoutEmpty();
    void layoutNodeProperties();
    void layoutNodePropertiesInvalid();
    void layoutDataOrder();
};

const struct {
    const char* name;
    bool recycledLayouts;
    Vector2 expectedNode4Size;
} LayoutDataOrderData[]{
    {"", false, {2*(50.0f + 0.4f), 2*(60.0f + 0.6f)}},
    {"recycled layouts", true, {2*50.0f + 0.4f, 2*60.0f + 0.6f}},
};

GenericLayouterTest::GenericLayouterTest() {
    addTests({&GenericLayouterTest::construct,
              &GenericLayouterTest::constructCopy,
              &GenericLayouterTest::constructMove,

              &GenericLayouterTest::addRemove,
              &GenericLayouterTest::addRemoveHandleRecycle,
              &GenericLayouterTest::addInvalid,

              &GenericLayouterTest::nodePropertiesNotInLayout,

              &GenericLayouterTest::clean,

              &GenericLayouterTest::layoutEmpty,
              &GenericLayouterTest::layoutNodeProperties,
              &GenericLayouterTest::layoutNodePropertiesInvalid});

    addInstancedTests({&GenericLayouterTest::layoutDataOrder},
        Containers::arraySize(LayoutDataOrderData));
}

void GenericLayouterTest::construct() {
    GenericLayouter layouter{layouterHandle(0xab, 0x12)};

    CORRADE_COMPARE(layouter.features(), LayouterFeatures{});
    CORRADE_COMPARE(layouter.handle(), layouterHandle(0xab, 0x12));
    CORRADE_COMPARE(layouter.usedAllocatedLayoutCount(), 0);
}

void GenericLayouterTest::constructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<GenericLayouter>{});
    CORRADE_VERIFY(!std::is_copy_assignable<GenericLayouter>{});
}

void GenericLayouterTest::constructMove() {
    GenericLayouter a{layouterHandle(0xab, 0x12)};

    GenericLayouter b{Utility::move(a)};
    CORRADE_COMPARE(b.handle(), layouterHandle(0xab, 0x12));

    GenericLayouter c{layouterHandle(3, 5)};
    c = Utility::move(b);
    CORRADE_COMPARE(c.handle(), layouterHandle(0xab, 0x12));

    CORRADE_VERIFY(std::is_nothrow_move_constructible<GenericLayouter>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<GenericLayouter>::value);
}

void GenericLayouterTest::addRemove() {
    Int destructedCount = 0;
    struct NonTrivial {
        explicit NonTrivial(int& output): destructedCount{&output} {}
        ~NonTrivial() {
            ++*destructedCount;
        }
        void operator()(const GenericLayouter&, Vector2&, Vector2&) const {
            CORRADE_FAIL("This should never be called.");
        }

        Int* destructedCount;
    };

    GenericLayouter layouter{layouterHandle(0, 1)};

    LayoutHandle trivial = layouter.add(nodeHandle(0x12345, 0xabc), [](const GenericLayouter&, Vector2&, Vector2&) {
        CORRADE_FAIL("This should never be called.");
    });
    CORRADE_COMPARE(layouter.usedCount(), 1);
    CORRADE_COMPARE(layouter.usedAllocatedLayoutCount(), 0);
    CORRADE_COMPARE(layouter.node(trivial), nodeHandle(0x12345, 0xabc));

    LayoutHandle nonTrivial = layouter.add(nodeHandle(0x67890, 0xdef), NonTrivial{destructedCount});
    CORRADE_COMPARE(layouter.usedCount(), 2);
    CORRADE_COMPARE(layouter.usedAllocatedLayoutCount(), 1);
    CORRADE_COMPARE(layouter.node(nonTrivial), nodeHandle(0x67890, 0xdef));

    layouter.remove(trivial);
    CORRADE_COMPARE(layouter.usedCount(), 1);
    CORRADE_COMPARE(layouter.usedAllocatedLayoutCount(), 1);
    CORRADE_COMPARE(destructedCount, 1);

    /* Verifying also the other handle overload. They should both delegate into
       the same internal implementation. */
    layouter.remove(layoutHandleData(nonTrivial));
    CORRADE_COMPARE(layouter.usedCount(), 0);
    CORRADE_COMPARE(layouter.usedAllocatedLayoutCount(), 0);
    CORRADE_COMPARE(destructedCount, 2);
}

void GenericLayouterTest::addRemoveHandleRecycle() {
    Int destructedCount1 = 0,
        destructedCount2 = 0;
    struct NonTrivial {
        explicit NonTrivial(int& output): destructedCount{&output} {}
        ~NonTrivial() {
            ++*destructedCount;
        }
        void operator()(const GenericLayouter&, Vector2&, Vector2&) const {
            CORRADE_FAIL("This should never be called.");
        }

        Int* destructedCount;
    };

    GenericLayouter layouter{layouterHandle(0, 1)};
    layouter.add(nodeHandle(0, 1), [](const GenericLayouter&, Vector2&, Vector2&) {});

    /* The temporary gets destructed right away */
    LayoutHandle second = layouter.add(nodeHandle(0, 1), NonTrivial{destructedCount1});
    CORRADE_COMPARE(destructedCount1, 1);

    layouter.remove(second);
    CORRADE_COMPARE(destructedCount1, 2);

    /* Layout that reuses a previous slot should not call the destructor on the
       previous function again or some such crazy stuff */
    LayoutHandle second2 = layouter.add(nodeHandle(0, 1), NonTrivial{destructedCount2});
    CORRADE_COMPARE(layoutHandleId(second2), layoutHandleId(second));
    CORRADE_COMPARE(destructedCount1, 2);
    CORRADE_COMPARE(destructedCount2, 1);
}

void GenericLayouterTest::addInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    GenericLayouter layouter{layouterHandle(0, 1)};

    Containers::String out;
    Error redirectError{&out};
    layouter.add(nodeHandle(0, 1), {});
    CORRADE_COMPARE(out, "Ui::GenericLayouter::add(): layout is null\n");
}

void GenericLayouterTest::nodePropertiesNotInLayout() {
    CORRADE_SKIP_IF_NO_ASSERT();

    GenericLayouter layouter{layouterHandle(0, 1)};

    /* Verify the check gets reset properly after update() again */
    Int i = 0;
    for(; i != 2; ++i) {
        CORRADE_ITERATION(i);

        /* Scoped redirect so any potential assertions in update() below aren't
           silently caught as well */
        Containers::String out;
        {
            Error redirectError{&out};
            layouter.nodeOffset({});
            layouter.nodeSize({});
            layouter.nodeMinSize({});
            layouter.nodeMaxSize({});
            layouter.nodeAspectRatio({});
            layouter.nodePadding({});
            layouter.nodeMargin({});
        }
        CORRADE_COMPARE_AS(out,
            "Ui::GenericLayouter::nodeOffset(): can only be called from within layout functions\n"
            "Ui::GenericLayouter::nodeSize(): can only be called from within layout functions\n"
            "Ui::GenericLayouter::nodeMinSize(): can only be called from within layout functions\n"
            "Ui::GenericLayouter::nodeMaxSize(): can only be called from within layout functions\n"
            "Ui::GenericLayouter::nodeAspectRatio(): can only be called from within layout functions\n"
            "Ui::GenericLayouter::nodePadding(): can only be called from within layout functions\n"
            "Ui::GenericLayouter::nodeMargin(): can only be called from within layout functions\n",
            TestSuite::Compare::String);

        /* Required to be called before update() (because AbstractUserInterface
           guarantees the same on a higher level), not needed for anything
           here */
        layouter.setSize({1, 1});

        Vector2 nodeOffsetsSizes[1];
        Float nodeAspectRatios[1];
        Vector4 nodePaddingsMargins[1];
        char layoutsIdsToUpdate[1]{};
        layouter.layout(
            Containers::BitArrayView{layoutsIdsToUpdate, 0, 0},
            {},
            nodeOffsetsSizes,
            nodeOffsetsSizes,
            nodeAspectRatios,
            nodePaddingsMargins,
            nodePaddingsMargins,
            nodeOffsetsSizes,
            nodeOffsetsSizes);
    }

    /* To ensure update() got actually called and the errors tested again
       after */
    CORRADE_COMPARE(i, 2);
}

void GenericLayouterTest::clean() {
    Int destructedCount = 0,
        anotherDestructedCount = 0;
    struct NonTrivial {
        explicit NonTrivial(int& output): destructedCount{&output} {}
        ~NonTrivial() {
            ++*destructedCount;
        }
        void operator()(const GenericLayouter&, Vector2&, Vector2&) const {
            CORRADE_FAIL("This should never be called.");
        }

        Int* destructedCount;
    };

    GenericLayouter layouter{layouterHandle(0, 1)};

    LayoutHandle trivial = layouter.add(nodeHandle(0, 7), [](const GenericLayouter&, Vector2&, Vector2&) {});
    CORRADE_COMPARE(layouter.usedCount(), 1);
    CORRADE_COMPARE(layouter.usedAllocatedLayoutCount(), 0);

    /* The temporary gets destructed right away */
    LayoutHandle nonTrivial = layouter.add(nodeHandle(1, 11), NonTrivial{destructedCount});
    CORRADE_COMPARE(layouter.usedCount(), 2);
    CORRADE_COMPARE(layouter.usedAllocatedLayoutCount(), 1);
    CORRADE_COMPARE(destructedCount, 1);

    LayoutHandle another = layouter.add(nodeHandle(2, 23), [](const GenericLayouter&, Vector2&, Vector2&) {});
    CORRADE_COMPARE(layouter.usedCount(), 3);
    CORRADE_COMPARE(layouter.usedAllocatedLayoutCount(), 1);
    CORRADE_COMPARE(destructedCount, 1);

    /* The temporary gets destructed right away */
    LayoutHandle anotherNonTrivial = layouter.add(nodeHandle(3, 17), NonTrivial{anotherDestructedCount});
    CORRADE_COMPARE(layouter.usedCount(), 4);
    CORRADE_COMPARE(layouter.usedAllocatedLayoutCount(), 2);
    CORRADE_COMPARE(anotherDestructedCount, 1);

    /* It should remove two but call just one destructor */
    UnsignedShort nodeHandleGenerations[]{
        7 + 1,
        11,
        23,
        17 + 1
    };
    layouter.cleanNodes(nodeHandleGenerations);
    CORRADE_COMPARE(layouter.usedCount(), 2);
    CORRADE_COMPARE(layouter.usedAllocatedLayoutCount(), 1);
    CORRADE_COMPARE(destructedCount, 1);
    CORRADE_COMPARE(anotherDestructedCount, 2);
    CORRADE_VERIFY(!layouter.isHandleValid(trivial));
    CORRADE_VERIFY(layouter.isHandleValid(nonTrivial));
    CORRADE_VERIFY(layouter.isHandleValid(another));
    CORRADE_VERIFY(!layouter.isHandleValid(anotherNonTrivial));
}

void GenericLayouterTest::layoutEmpty() {
    GenericLayouter layouter{layouterHandle(0, 1)};

    /* Required to be called before update() (because AbstractUserInterface
       guarantees the same on a higher level), not needed for anything here */
    layouter.setSize({1, 1});

    /* It shouldn't crash or do anything weird */
    layouter.layout({}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_VERIFY(true);
}

void GenericLayouterTest::layoutNodeProperties() {
    AbstractUserInterface ui{{100, 100}};

    /* As size is known already, setSize() is called on the layouter right
       here */
    GenericLayouter& layouter = ui.setLayouterInstance(Containers::pointer<GenericLayouter>(ui.createLayouter()));

    /* Add nodes with specific handles to verify their validity gets checked */
    ui.createNode({}, {});
    ui.removeNode(ui.createNode({}, {}));
    ui.removeNode(ui.createNode({}, {}));
    ui.removeNode(ui.createNode({}, {}));
    NodeHandle first = ui.createNode({}, {});
    ui.createNode({}, {});
    NodeHandle second = ui.createNode({}, {});
    ui.createNode({}, {});
    ui.removeNode(ui.createNode({}, {}));
    NodeHandle third = ui.createNode({}, {});
    CORRADE_COMPARE(first, nodeHandle(1, 4));
    CORRADE_COMPARE(second, nodeHandle(3, 1));
    CORRADE_COMPARE(third, nodeHandle(5, 2));

    Int called = 0;
    LayoutHandle layout = layouter.add(second, [first, second, third, &called](const GenericLayouter& layouter, Vector2& nodeOffset, Vector2& nodeSize) {
        CORRADE_COMPARE(layouter.nodeOffset(first), (Vector2{1.0f, 2.0f}));
        CORRADE_COMPARE(layouter.nodeSize(third), (Vector2{2.0f, 3.0f}));
        CORRADE_COMPARE(layouter.nodeMinSize(first), (Vector2{3.0f, 4.0f}));
        CORRADE_COMPARE(layouter.nodeMaxSize(third), (Vector2{4.0f, 5.0f}));
        CORRADE_COMPARE(layouter.nodeAspectRatio(first), 5.6f);
        CORRADE_COMPARE(layouter.nodePadding(first), (Vector4{6.0f, 7.0f, 8.0f, 9.0f}));
        CORRADE_COMPARE(layouter.nodeMargin(third), (Vector4{7.0f, 8.0f, 9.0f, 0.0f}));

        /* The arguments are just for mutable access, otherwise accessible
           through the same APIs as other properties */
        CORRADE_COMPARE(nodeOffset, (Vector2{2.5f, 5.2f}));
        CORRADE_COMPARE(layouter.nodeOffset(second), (Vector2{2.5f, 5.2f}));
        CORRADE_COMPARE(nodeSize, (Vector2{4.3f, 3.4f}));
        CORRADE_COMPARE(layouter.nodeSize(second), (Vector2{4.3f, 3.4f}));

        /* Updating the offset and size should be immediately reflected in the
           getters */
        nodeOffset *= 2.0f;
        nodeSize += Vector2{1.0f};
        CORRADE_COMPARE(layouter.nodeOffset(second), (Vector2{5.0f, 10.4f}));
        CORRADE_COMPARE(layouter.nodeSize(second), (Vector2{5.3f, 4.4f}));

        ++called;
    });

    /* These views should be (temporarily) exposed via getters */
    Vector2 nodeOffsets[]{
        {},
        {1.0f, 2.0f},
        {},
        {2.5f, 5.2f},
        {},
        {}
    };
    Vector2 nodeSizes[]{
        {},
        {},
        {},
        {4.3f, 3.4f},
        {},
        {2.0f, 3.0f}
    };
    Vector2 nodeMinSizes[]{
        {},
        {3.0f, 4.0f},
        {},
        {},
        {},
        {}
    };
    Vector2 nodeMaxSizes[]{
        {},
        {},
        {},
        {},
        {},
        {4.0f, 5.0f}
    };
    Float nodeAspectRatios[]{
        0.0f,
        5.6f,
        0.0f,
        0.0f,
        0.0f,
        0.0f
    };
    Vector4 nodePaddings[]{
        {},
        {6.0f, 7.0f, 8.0f, 9.0f},
        {},
        {},
        {},
        {}
    };
    Vector4 nodeMargins[]{
        {},
        {},
        {},
        {},
        {},
        {7.0f, 8.0f, 9.0f, 0.0f}
    };
    char layoutsIdsToUpdate[]{
        char(1 << layoutHandleId(layout))
    };

    layouter.layout(
        Containers::BitArrayView{layoutsIdsToUpdate, 0, 1},
        {},
        nodeMinSizes,
        nodeMaxSizes,
        nodeAspectRatios,
        nodePaddings,
        nodeMargins,
        nodeOffsets,
        nodeSizes);
    CORRADE_COMPARE(called, 1);

    /* The updated offset and size should be reflected back to the original
       views (which are passed from the UI itself internally), not somewhere
       else */
    CORRADE_COMPARE_AS(Containers::arrayView(nodeOffsets), Containers::arrayView<Vector2>({
        {},
        {1.0f, 2.0f},
        {},
        {5.0f, 10.4f},
        {},
        {}
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(nodeSizes), Containers::arrayView<Vector2>({
        {},
        {},
        {},
        {5.3f, 4.4f},
        {},
        {2.0f, 3.0f}
    }), TestSuite::Compare::Container);
}

void GenericLayouterTest::layoutNodePropertiesInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui{{100, 100}};

    GenericLayouter& layouter = ui.setLayouterInstance(Containers::pointer<GenericLayouter>(ui.createLayouter()));

    /* Add nodes with specific handles to verify their validity gets checked */
    ui.createNode({}, {});
    ui.createNode({}, {});
    ui.createNode({}, {});
    ui.removeNode(ui.createNode({}, {}));
    ui.removeNode(ui.createNode({}, {}));
    ui.removeNode(ui.createNode({}, {}));
    NodeHandle node = ui.createNode({}, {});
    CORRADE_COMPARE(ui.nodeCapacity(), 4);
    CORRADE_COMPARE(node, nodeHandle(3, 4));

    Int called = 0;
    LayoutHandle layout = layouter.add(node, [node, &called](const GenericLayouter& layouter, Vector2&, Vector2&) {
        ++called;

        /* Correct node handle shouldn't fire anything (for example because we
           passed wrongly-sized views to update()) */
        layouter.nodeOffset(node);
        layouter.nodeSize(node);
        layouter.nodeMinSize(node);
        layouter.nodeMaxSize(node);
        layouter.nodeAspectRatio(node);
        layouter.nodePadding(node);
        layouter.nodeMargin(node);

        Containers::String out;
        Error redirectError{&out};
        /* Null handle */
        layouter.nodeOffset(NodeHandle::Null);
        layouter.nodeSize(NodeHandle::Null);
        layouter.nodeMinSize(NodeHandle::Null);
        layouter.nodeMaxSize(NodeHandle::Null);
        layouter.nodeAspectRatio(NodeHandle::Null);
        layouter.nodePadding(NodeHandle::Null);
        layouter.nodeMargin(NodeHandle::Null);
        /* ID correct, generation wrong */
        layouter.nodeOffset(nodeHandle(3, 5));
        layouter.nodeSize(nodeHandle(3, 5));
        layouter.nodeMinSize(nodeHandle(3, 5));
        layouter.nodeMaxSize(nodeHandle(3, 5));
        layouter.nodeAspectRatio(nodeHandle(3, 5));
        layouter.nodePadding(nodeHandle(3, 5));
        layouter.nodeMargin(nodeHandle(3, 5));
        /* ID out of bounds */
        layouter.nodeOffset(nodeHandle(4, 1));
        layouter.nodeSize(nodeHandle(4, 1));
        layouter.nodeMinSize(nodeHandle(4, 1));
        layouter.nodeMaxSize(nodeHandle(4, 1));
        layouter.nodeAspectRatio(nodeHandle(4, 1));
        layouter.nodePadding(nodeHandle(4, 1));
        layouter.nodeMargin(nodeHandle(4, 1));
        CORRADE_COMPARE_AS(out,
            "Ui::GenericLayouter::nodeOffset(): invalid handle Ui::NodeHandle::Null\n"
            "Ui::GenericLayouter::nodeSize(): invalid handle Ui::NodeHandle::Null\n"
            "Ui::GenericLayouter::nodeMinSize(): invalid handle Ui::NodeHandle::Null\n"
            "Ui::GenericLayouter::nodeMaxSize(): invalid handle Ui::NodeHandle::Null\n"
            "Ui::GenericLayouter::nodeAspectRatio(): invalid handle Ui::NodeHandle::Null\n"
            "Ui::GenericLayouter::nodePadding(): invalid handle Ui::NodeHandle::Null\n"
            "Ui::GenericLayouter::nodeMargin(): invalid handle Ui::NodeHandle::Null\n"

            "Ui::GenericLayouter::nodeOffset(): invalid handle Ui::NodeHandle(0x3, 0x5)\n"
            "Ui::GenericLayouter::nodeSize(): invalid handle Ui::NodeHandle(0x3, 0x5)\n"
            "Ui::GenericLayouter::nodeMinSize(): invalid handle Ui::NodeHandle(0x3, 0x5)\n"
            "Ui::GenericLayouter::nodeMaxSize(): invalid handle Ui::NodeHandle(0x3, 0x5)\n"
            "Ui::GenericLayouter::nodeAspectRatio(): invalid handle Ui::NodeHandle(0x3, 0x5)\n"
            "Ui::GenericLayouter::nodePadding(): invalid handle Ui::NodeHandle(0x3, 0x5)\n"
            "Ui::GenericLayouter::nodeMargin(): invalid handle Ui::NodeHandle(0x3, 0x5)\n"

            "Ui::GenericLayouter::nodeOffset(): invalid handle Ui::NodeHandle(0x4, 0x1)\n"
            "Ui::GenericLayouter::nodeSize(): invalid handle Ui::NodeHandle(0x4, 0x1)\n"
            "Ui::GenericLayouter::nodeMinSize(): invalid handle Ui::NodeHandle(0x4, 0x1)\n"
            "Ui::GenericLayouter::nodeMaxSize(): invalid handle Ui::NodeHandle(0x4, 0x1)\n"
            "Ui::GenericLayouter::nodeAspectRatio(): invalid handle Ui::NodeHandle(0x4, 0x1)\n"
            "Ui::GenericLayouter::nodePadding(): invalid handle Ui::NodeHandle(0x4, 0x1)\n"
            "Ui::GenericLayouter::nodeMargin(): invalid handle Ui::NodeHandle(0x4, 0x1)\n",
            TestSuite::Compare::String);
    });

    Vector2 nodeOffsetsSizes[4];
    Float nodeAspectRatios[4];
    Vector4 nodePaddingsMargins[4];
    char layoutsIdsToUpdate[1]{
        char(1 << layoutHandleId(layout))
    };
    layouter.layout(
        Containers::BitArrayView{layoutsIdsToUpdate, 0, 1},
        {},
        nodeOffsetsSizes,
        nodeOffsetsSizes,
        nodeAspectRatios,
        nodePaddingsMargins,
        nodePaddingsMargins,
        nodeOffsetsSizes,
        nodeOffsetsSizes);
    CORRADE_COMPARE(called, 1);
}

void GenericLayouterTest::layoutDataOrder() {
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
            nodePaddings[1 /*node2*/] = {0.1f, 0.2f, 0.3f, 0.4f};
            nodeMargins[3 /*node4*/] = {1.0f, 2.0f, 3.0f, 4.0f};
            ++called;
        }

        Int called = 0;
    };
    LayoutLayer& layoutLayer = ui.setLayerInstance(Containers::pointer<LayoutLayer>(ui.createLayer()));

    GenericLayouter& layouter = ui.setLayouterInstance(Containers::pointer<GenericLayouter>(ui.createLayouter()));

    NodeHandle node1 = ui.createNode({}, {});
    NodeHandle node2 = ui.createNode({10.0f, 20.0f}, {});
    NodeHandle node3 = ui.createNode({}, {});
    NodeHandle node4 = ui.createNode({30.0f, 40.0f}, {50.0f, 60.0f});
    NodeHandle node5 = ui.createNode({}, {});

    if(data.recycledLayouts) {
        LayoutHandle layout1 = layouter.add(node1, [](const GenericLayouter&, Vector2&, Vector2&) {});
        LayoutHandle layout2 = layouter.add(node2, [](const GenericLayouter&, Vector2&, Vector2&) {});
        LayoutHandle layout3 = layouter.add(node3, [](const GenericLayouter&, Vector2&, Vector2&) {});
        LayoutHandle layout4 = layouter.add(node4, [](const GenericLayouter&, Vector2&, Vector2&) {});
        LayoutHandle layout5 = layouter.add(node1, [](const GenericLayouter&, Vector2&, Vector2&) {});
        LayoutHandle layout6 = layouter.add(node4, [](const GenericLayouter&, Vector2&, Vector2&) {});
        LayoutHandle layout7 = layouter.add(node5, [](const GenericLayouter&, Vector2&, Vector2&) {});
        layouter.remove(layout4);
        layouter.remove(layout1);
        layouter.remove(layout5);
        layouter.remove(layout7);
        layouter.remove(layout3);
        layouter.remove(layout2);
        layouter.remove(layout6);
    }

    /* Layout affecting the offset using another node margin */
    layouter.add(node2, [node4](const GenericLayouter& layouter, Vector2& nodeOffset, Vector2&) {
        nodeOffset +=
            Math::gather<0, 1>(layouter.nodeMargin(node4)) +
            Math::gather<2, 3>(layouter.nodeMargin(node4));
    });

    /* Layout that's removed */
    layouter.remove(layouter.add(node4, [](const GenericLayouter&, Vector2&, Vector2&) {
        CORRADE_FAIL("This shouldn't be called.");
    }));

    /* Layout affecting the size using another node padding */
    layouter.add(node4, [node2](const GenericLayouter& layouter, Vector2&, Vector2& nodeSize) {
        nodeSize +=
            Math::gather<0, 1>(layouter.nodePadding(node2)) +
            Math::gather<2, 3>(layouter.nodePadding(node2));
    });

    /* Layout attached to a node that isn't in top level order that shouldn't
       be called */
    ui.clearNodeOrder(node1);
    layouter.add(node1, [](const GenericLayouter&, Vector2&, Vector2&) {
        CORRADE_FAIL("This shouldn't be called.");
    });

    /* Layout affecting size and offset of a node that already has a layout */
    layouter.add(node4, [](const GenericLayouter&, Vector2& nodeOffset, Vector2& nodeSize) {
        nodeOffset *= 2.0f;
        nodeSize *= 2.0f;
    });

    /* Layout attached to a hidden node that shouldn't be called */
    ui.addNodeFlags(node5, NodeFlag::Hidden);
    layouter.add(node5, [](const GenericLayouter&, Vector2&, Vector2&) {
        CORRADE_FAIL("This shouldn't be called.");
    });

    /* Layout doing nothing that should still be called */
    Int called = 0;
    layouter.add(node3, [&called](const GenericLayouter&, Vector2&, Vector2&) {
        ++called;
    });

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    /* Add a dummy second layouter because that's the easiest way to verify the
       calculated node offsets / sizes */
    struct DummyLayouter: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::add;

        LayouterFeatures doFeatures() const override { return {}; }
        void doLayout(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Float>&, const Containers::StridedArrayView1D<Vector4>&, const Containers::StridedArrayView1D<Vector4>&, const Containers::StridedArrayView1D<Vector2>& nodeOffsets, const Containers::StridedArrayView1D<Vector2>& nodeSizes) override {
            {
                CORRADE_EXPECT_FAIL_IF(expectRecycled, "AbstractUserInterface currently discards extra layouts assigned to the same node, only one is executed.");
                CORRADE_COMPARE(nodeOffsets[3], (Vector2{60.0f, 80.0f}));
            } {
                CORRADE_EXPECT_FAIL("AbstractUserInterface currently discards extra layouts assigned to the same node, only one is executed.");
                CORRADE_COMPARE(nodeSizes[3], (expectRecycled ?
                    Vector2{2*50.0f + 0.4f, 2*60.0f + 0.6f} :
                    Vector2{2*(50.0f + 0.4f), 2*(60.0f + 0.6f)}));
            }

            CORRADE_COMPARE_AS(nodeOffsets, Containers::stridedArrayView<Vector2>({
                {},                 /* node1 */
                {14.0f, 26.0f},     /* node2 */
                {},                 /* node3 */
                /** @todo this is wrong, clean up once the above XFAIL passes */
                expectRecycled ?    /* node4 */
                    Vector2{30.0f, 40.0f} :
                    Vector2{60.0f, 80.0f},
                {},                 /* node5 */
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(nodeSizes, Containers::stridedArrayView<Vector2>({
                {},                 /* node1 */
                {},                 /* node2 */
                {},                 /* node3 */
                /** @todo this is wrong, clean up once the above XFAIL passes */
                expectRecycled ?    /* node4 */
                    Vector2{50.4f, 60.6f} :
                    Vector2{100.0f, 120.0f},
                {},                 /* node5 */
            }), TestSuite::Compare::Container);
            ++called;
        }

        bool expectRecycled;
        Int called = 0;
    };
    DummyLayouter& dummyLayouter = ui.setLayouterInstance(Containers::pointer<DummyLayouter>(ui.createLayouter()));
    dummyLayouter.expectRecycled = data.recycledLayouts;
    dummyLayouter.add(node1);
    ui.update();
    CORRADE_COMPARE(layoutLayer.called, 1);
    CORRADE_COMPARE(dummyLayouter.called, 1);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::GenericLayouterTest)
