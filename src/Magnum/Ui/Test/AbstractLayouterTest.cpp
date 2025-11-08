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

#include <Corrade/Containers/StridedBitArrayView.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/String.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Container.h>
#include <Corrade/TestSuite/Compare/String.h>
#include <Magnum/Math/Vector2.h>

#include "Magnum/Ui/AbstractLayouter.h"
#include "Magnum/Ui/AbstractUserInterface.h" /* for UniqueLayouts */
#include "Magnum/Ui/Handle.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct AbstractLayouterTest: TestSuite::Tester {
    explicit AbstractLayouterTest();

    void debugFeature();
    void debugFeatures();
    void debugState();
    void debugStates();
    void debugStatesSupersets();

    void construct();
    void constructInvalidHandle();
    void constructCopy();
    void constructMove();

    void uiInvalid();

    void addRemove();
    void addRemoveUniqueLayouts();
    void addRemoveHandleRecycle();
    void addRemoveHandleDisable();
    void addInvalid();
    void addInvalidUniqueLayouts();
    void addNoHandlesLeft();
    void removeUniqueLayoutInvalidNode();
    void removeInvalid();
    void nodeInvalid();

    void setSize();
    void setSizeZero();
    void setSizeNotImplemented();

    void cleanNodes();
    void cleanNodesEmpty();
    void cleanNodesNotImplemented();

    void update();
    void updateEmpty();
    void updateInvalidSizes();
    void updateNoSizeSet();

    void state();
};

AbstractLayouterTest::AbstractLayouterTest() {
    addTests({&AbstractLayouterTest::debugFeature,
              &AbstractLayouterTest::debugFeatures,
              &AbstractLayouterTest::debugState,
              &AbstractLayouterTest::debugStates,
              &AbstractLayouterTest::debugStatesSupersets,

              &AbstractLayouterTest::construct,
              &AbstractLayouterTest::constructInvalidHandle,
              &AbstractLayouterTest::constructCopy,
              &AbstractLayouterTest::constructMove,

              &AbstractLayouterTest::uiInvalid,

              &AbstractLayouterTest::addRemove,
              &AbstractLayouterTest::addRemoveUniqueLayouts,
              &AbstractLayouterTest::addRemoveHandleRecycle,
              &AbstractLayouterTest::addRemoveHandleDisable,
              &AbstractLayouterTest::addInvalid,
              &AbstractLayouterTest::addInvalidUniqueLayouts,
              &AbstractLayouterTest::addNoHandlesLeft,
              &AbstractLayouterTest::removeUniqueLayoutInvalidNode,
              &AbstractLayouterTest::removeInvalid,
              &AbstractLayouterTest::nodeInvalid,

              &AbstractLayouterTest::setSize,
              &AbstractLayouterTest::setSizeZero,
              &AbstractLayouterTest::setSizeNotImplemented,

              &AbstractLayouterTest::cleanNodes,
              &AbstractLayouterTest::cleanNodesEmpty,
              &AbstractLayouterTest::cleanNodesNotImplemented,

              &AbstractLayouterTest::update,
              &AbstractLayouterTest::updateEmpty,
              &AbstractLayouterTest::updateInvalidSizes,
              &AbstractLayouterTest::updateNoSizeSet,

              &AbstractLayouterTest::state});
}

void AbstractLayouterTest::debugFeature() {
    Containers::String out;
    Debug{&out} << LayouterFeature::UniqueLayouts << LayouterFeature(0xbe);
    CORRADE_COMPARE(out, "Ui::LayouterFeature::UniqueLayouts Ui::LayouterFeature(0xbe)\n");
}

void AbstractLayouterTest::debugFeatures() {
    Containers::String out;
    Debug{&out} << (LayouterFeature::UniqueLayouts|LayouterFeature(0x80)) << LayouterFeatures{};
    CORRADE_COMPARE(out, "Ui::LayouterFeature::UniqueLayouts|Ui::LayouterFeature(0x80) Ui::LayouterFeatures{}\n");
}

void AbstractLayouterTest::debugState() {
    Containers::String out;
    Debug{&out} << LayouterState::NeedsUpdate << LayouterState(0xbe);
    CORRADE_COMPARE(out, "Ui::LayouterState::NeedsUpdate Ui::LayouterState(0xbe)\n");
}

void AbstractLayouterTest::debugStates() {
    Containers::String out;
    Debug{&out} << (LayouterState::NeedsUpdate|LayouterState(0xe0)) << LayouterStates{};
    CORRADE_COMPARE(out, "Ui::LayouterState::NeedsUpdate|Ui::LayouterState(0xe0) Ui::LayouterStates{}\n");
}

void AbstractLayouterTest::debugStatesSupersets() {
    /* NeedsAssignmentUpdate is a superset of NeedsUpdate, so only one should
       be printed */
    {
        Containers::String out;
        Debug{&out} << (LayouterState::NeedsUpdate|LayouterState::NeedsAssignmentUpdate);
        CORRADE_COMPARE(out, "Ui::LayouterState::NeedsAssignmentUpdate\n");
    }
}

void AbstractLayouterTest::construct() {
    struct: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::hasUi;

        LayouterFeatures doFeatures() const override { return LayouterFeatures{0xe0}; }
        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const  Containers::StridedArrayView1D<Vector2>&) override {}
    } layouter{layouterHandle(0xab, 0x12)};

    CORRADE_COMPARE(layouter.handle(), layouterHandle(0xab, 0x12));
    /* Tests the implicit LayouterHandle conversion */
    CORRADE_COMPARE(layouter, layouterHandle(0xab, 0x12));
    CORRADE_COMPARE(layouter.features(), LayouterFeatures(0xe0));
    CORRADE_COMPARE(layouter.state(), LayouterStates{});
    CORRADE_COMPARE(layouter.capacity(), 0);
    CORRADE_COMPARE(layouter.usedCount(), 0);
    CORRADE_VERIFY(!layouter.isHandleValid(LayouterDataHandle::Null));
    CORRADE_VERIFY(!layouter.isHandleValid(LayoutHandle::Null));
    /* Verify that out-of-bounds ID and zero generation is handled correctly
       even for an empty layouter */
    CORRADE_VERIFY(!layouter.isHandleValid(layouterDataHandle(0, 1)));
    CORRADE_VERIFY(!layouter.isHandleValid(layouterDataHandle(1, 0)));
    CORRADE_VERIFY(!layouter.isHandleValid(layoutHandle(layouter.handle(), 0, 1)));
    CORRADE_VERIFY(!layouter.isHandleValid(layoutHandle(layouter.handle(), 1, 0)));

    CORRADE_VERIFY(!layouter.hasUi());
    /* ui() and hasUi() tested thoroughly in
       AbstractUserInterfaceTest::layouterUserInterfaceReference(), invalid
       access in uiInvalid() below */
}

void AbstractLayouterTest::constructInvalidHandle() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Layouter: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;

        LayouterFeatures doFeatures() const override { return {}; }
        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const  Containers::StridedArrayView1D<Vector2>&) override {}
    };

    Containers::String out;
    Error redirectError{&out};
    Layouter{LayouterHandle::Null};
    Layouter{layouterHandle(0xab, 0)};
    CORRADE_COMPARE_AS(out,
        "Ui::AbstractLayouter: invalid handle Ui::LayouterHandle::Null\n"
        "Ui::AbstractLayouter: invalid handle Ui::LayouterHandle(0xab, 0x0)\n",
        TestSuite::Compare::String);
}

void AbstractLayouterTest::constructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<AbstractLayouter>{});
    CORRADE_VERIFY(!std::is_copy_assignable<AbstractLayouter>{});
}

void AbstractLayouterTest::constructMove() {
    struct Layouter: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;

        LayouterFeatures doFeatures() const override { return {}; }
        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const  Containers::StridedArrayView1D<Vector2>&) override {}
    };

    /* The class has an internal state struct containing everything, so it's
       not needed to test each and every property */
    Layouter a{layouterHandle(0xab, 0x12)};

    Layouter b{Utility::move(a)};
    CORRADE_COMPARE(b.handle(), layouterHandle(0xab, 0x12));

    Layouter c{layouterHandle(0xcd, 0x34)};
    c = Utility::move(b);
    CORRADE_COMPARE(c.handle(), layouterHandle(0xab, 0x12));

    CORRADE_VERIFY(std::is_nothrow_move_constructible<Layouter>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<Layouter>::value);
}

void AbstractLayouterTest::uiInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Layouter: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::hasUi;
        using AbstractLayouter::ui;

        LayouterFeatures doFeatures() const override { return {}; }
        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const  Containers::StridedArrayView1D<Vector2>&) override {}
    } layouter{layouterHandle(0, 1)};

    CORRADE_VERIFY(!layouter.hasUi());

    Containers::String out;
    Error redirectError{&out};
    layouter.ui();
    CORRADE_COMPARE(out, "Ui::AbstractLayouter::ui(): layouter not part of a user interface\n");
}

void AbstractLayouterTest::addRemove() {
    struct: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::add;
        using AbstractLayouter::remove;

        LayouterFeatures doFeatures() const override { return {}; }
        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const  Containers::StridedArrayView1D<Vector2>&) override {}
    } layouter{layouterHandle(0xab, 0x12)};

    LayoutHandle first = layouter.add(nodeHandle(0x12345, 0xabc));
    CORRADE_COMPARE(first, layoutHandle(layouter.handle(), 0, 1));
    CORRADE_VERIFY(layouter.isHandleValid(first));
    CORRADE_COMPARE(layouter.state(), LayouterState::NeedsAssignmentUpdate);
    CORRADE_COMPARE(layouter.capacity(), 1);
    CORRADE_COMPARE(layouter.usedCount(), 1);
    CORRADE_COMPARE(layouter.node(first), nodeHandle(0x12345, 0xabc));

    LayoutHandle second = layouter.add(nodeHandle(0xabcde, 0x123));
    CORRADE_COMPARE(second, layoutHandle(layouter.handle(), 1, 1));
    CORRADE_VERIFY(layouter.isHandleValid(second));
    CORRADE_COMPARE(layouter.state(), LayouterState::NeedsAssignmentUpdate);
    CORRADE_COMPARE(layouter.capacity(), 2);
    CORRADE_COMPARE(layouter.usedCount(), 2);
    /* Using also the LayouterDataHandle overload */
    CORRADE_COMPARE(layouter.node(layoutHandleData(second)), nodeHandle(0xabcde, 0x123));

    layouter.remove(first);
    CORRADE_VERIFY(!layouter.isHandleValid(first));
    CORRADE_VERIFY(layouter.isHandleValid(second));
    CORRADE_COMPARE(layouter.state(), LayouterState::NeedsAssignmentUpdate);
    CORRADE_COMPARE(layouter.capacity(), 2);
    CORRADE_COMPARE(layouter.usedCount(), 1);

    /* Using also the LayouterDataHandle overload */
    layouter.remove(layoutHandleData(second));
    CORRADE_VERIFY(!layouter.isHandleValid(first));
    CORRADE_VERIFY(!layouter.isHandleValid(second));
    CORRADE_COMPARE(layouter.state(), LayouterState::NeedsAssignmentUpdate);
    CORRADE_COMPARE(layouter.capacity(), 2);
    CORRADE_COMPARE(layouter.usedCount(), 0);
}

void AbstractLayouterTest::addRemoveUniqueLayouts() {
    /* By default the node unique layout storage is empty */
    AbstractUserInterface ui{{100, 100}};
    CORRADE_COMPARE(ui.nodeUniqueLayoutCapacity(), 0);
    CORRADE_COMPARE(ui.nodeUniqueLayoutUsedCount(), 0);

    /* Adding unique layouters doesn't change anything for the unique layout
       storage. Removing some of the layouters to have non-trivial handles. */
    struct Layouter: AbstractLayouter {
        explicit Layouter(LayouterHandle handle, LayouterFeatures features): AbstractLayouter{handle}, _features{features} {}

        using AbstractLayouter::add;
        using AbstractLayouter::remove;

        LayouterFeatures doFeatures() const override { return _features; }
        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&) override {}

        private:
            LayouterFeatures _features;
    };
    ui.createLayouter();
    ui.removeLayouter(ui.createLayouter());
    ui.removeLayouter(ui.createLayouter());
    Layouter& layouter1 = ui.setLayouterInstance(Containers::pointer<Layouter>(ui.createLayouter(), LayouterFeature::UniqueLayouts));
    Layouter& layouterNonUnique = ui.setLayouterInstance(Containers::pointer<Layouter>(ui.createLayouter(), LayouterFeatures{}));
    ui.removeLayouter(ui.createLayouter());
    ui.createLayouter();
    Layouter& layouter2 = ui.setLayouterInstance(Containers::pointer<Layouter>(ui.createLayouter(), LayouterFeature::UniqueLayouts));
    CORRADE_COMPARE(ui.nodeUniqueLayoutCapacity(), 0);
    CORRADE_COMPARE(ui.nodeUniqueLayoutUsedCount(), 0);

    /* Neither does adding nodes, and those initially report no unique
       layouts. Removing some of the nodes to have non-trivial handles. */
    ui.createNode({}, {});
    ui.createNode({}, {});
    ui.removeNode(ui.createNode({}, {}));
    ui.removeNode(ui.createNode({}, {}));
    NodeHandle node1 = ui.createNode({}, {});
    NodeHandle node2Parent = ui.createNode({}, {});
    NodeHandle node2 = ui.createNode(node2Parent, {}, {});
    ui.removeNode(ui.createNode({}, {}));
    NodeHandle node3 = ui.createNode({}, {});
    CORRADE_COMPARE(ui.nodeUniqueLayoutCapacity(), 0);
    CORRADE_COMPARE(ui.nodeUniqueLayoutUsedCount(), 0);
    CORRADE_COMPARE(ui.nodeUniqueLayout(node1, layouter1), LayouterDataHandle::Null);
    CORRADE_COMPARE(ui.nodeUniqueLayout(node1, layouter2), LayouterDataHandle::Null);
    CORRADE_COMPARE(ui.nodeUniqueLayout(node2, layouter1), LayouterDataHandle::Null);
    CORRADE_COMPARE(ui.nodeUniqueLayout(node2, layouter2), LayouterDataHandle::Null);
    CORRADE_COMPARE(ui.nodeUniqueLayout(node3, layouter1), LayouterDataHandle::Null);
    CORRADE_COMPARE(ui.nodeUniqueLayout(node3, layouter2), LayouterDataHandle::Null);

    /* Creating the first few unique layouts enlarges the capacity. So far
       at most one layout per node. */
    LayoutHandle node1Layout1 = layouter1.add(node1);
    LayoutHandle node3Layout1 = layouter1.add(node3);
    LayoutHandle node2Layout2 = layouter2.add(node2);
    CORRADE_COMPARE(ui.nodeUniqueLayoutCapacity(), 3);
    CORRADE_COMPARE(ui.nodeUniqueLayoutUsedCount(), 3);
    CORRADE_COMPARE(ui.nodeUniqueLayout(node1, layouter1), layoutHandleData(node1Layout1));
    CORRADE_COMPARE(ui.nodeUniqueLayout(node1, layouter2), LayouterDataHandle::Null);
    CORRADE_COMPARE(ui.nodeUniqueLayout(node2, layouter1), LayouterDataHandle::Null);
    CORRADE_COMPARE(ui.nodeUniqueLayout(node2, layouter2), layoutHandleData(node2Layout2));
    CORRADE_COMPARE(ui.nodeUniqueLayout(node3, layouter1), layoutHandleData(node3Layout1));
    CORRADE_COMPARE(ui.nodeUniqueLayout(node3, layouter2), LayouterDataHandle::Null);

    /* Adding a non-unique layout doesn't change anything in these */
    /*LayoutHandle node3LayoutNonUnique =*/ layouterNonUnique.add(node3);
    CORRADE_COMPARE(ui.nodeUniqueLayoutCapacity(), 3);
    CORRADE_COMPARE(ui.nodeUniqueLayoutUsedCount(), 3);
    CORRADE_COMPARE(ui.nodeUniqueLayout(node2, layouter1), LayouterDataHandle::Null);
    CORRADE_COMPARE(ui.nodeUniqueLayout(node2, layouter2), layoutHandleData(node2Layout2));

    /* Add a layout to a node that already has a unique layout from another
       layouter should work too */
    LayoutHandle node1Layout2 = layouter2.add(node1);
    LayoutHandle node2Layout1 = layouter1.add(node2);
    CORRADE_COMPARE(ui.nodeUniqueLayoutCapacity(), 5);
    CORRADE_COMPARE(ui.nodeUniqueLayoutUsedCount(), 5);
    CORRADE_COMPARE(ui.nodeUniqueLayout(node1, layouter1), layoutHandleData(node1Layout1));
    CORRADE_COMPARE(ui.nodeUniqueLayout(node1, layouter2), layoutHandleData(node1Layout2));
    CORRADE_COMPARE(ui.nodeUniqueLayout(node2, layouter1), layoutHandleData(node2Layout1));
    CORRADE_COMPARE(ui.nodeUniqueLayout(node2, layouter2), layoutHandleData(node2Layout2));

    /* Adding yet another layouter and a third layout to a node isn't any
       different from adding a second */
    Layouter& layouter3 = ui.setLayouterInstance(Containers::pointer<Layouter>(ui.createLayouter(), LayouterFeature::UniqueLayouts));
    LayoutHandle node1Layout3 = layouter3.add(node1);
    CORRADE_COMPARE(ui.nodeUniqueLayoutCapacity(), 6);
    CORRADE_COMPARE(ui.nodeUniqueLayoutUsedCount(), 6);
    CORRADE_COMPARE(ui.nodeUniqueLayout(node1, layouter1), layoutHandleData(node1Layout1));
    CORRADE_COMPARE(ui.nodeUniqueLayout(node1, layouter2), layoutHandleData(node1Layout2));
    CORRADE_COMPARE(ui.nodeUniqueLayout(node1, layouter3), layoutHandleData(node1Layout3));

    /* Remove one layout out of the three assigned to node1. The two remaining
       should still be circularly connected to each other. */
    layouter1.remove(node1Layout1);
    CORRADE_COMPARE(ui.nodeUniqueLayoutCapacity(), 6);
    CORRADE_COMPARE(ui.nodeUniqueLayoutUsedCount(), 5);
    CORRADE_COMPARE(ui.nodeUniqueLayout(node1, layouter1), LayouterDataHandle::Null);
    CORRADE_COMPARE(ui.nodeUniqueLayout(node1, layouter2), layoutHandleData(node1Layout2));
    CORRADE_COMPARE(ui.nodeUniqueLayout(node1, layouter3), layoutHandleData(node1Layout3));

    /* Remove another layout, now it's just one left connected to itself */
    layouter2.remove(node1Layout2);
    CORRADE_COMPARE(ui.nodeUniqueLayoutCapacity(), 6);
    CORRADE_COMPARE(ui.nodeUniqueLayoutUsedCount(), 4);
    CORRADE_COMPARE(ui.nodeUniqueLayout(node1, layouter1), LayouterDataHandle::Null);
    CORRADE_COMPARE(ui.nodeUniqueLayout(node1, layouter2), LayouterDataHandle::Null);
    CORRADE_COMPARE(ui.nodeUniqueLayout(node1, layouter3), layoutHandleData(node1Layout3));

    /* Remove the last layout assigned to node1, now the node is the same as in
       the initial state, just the free list is now non-empty */
    layouter3.remove(node1Layout3);
    CORRADE_COMPARE(ui.nodeUniqueLayoutCapacity(), 6);
    CORRADE_COMPARE(ui.nodeUniqueLayoutUsedCount(), 3);
    CORRADE_COMPARE(ui.nodeUniqueLayout(node1, layouter1), LayouterDataHandle::Null);
    CORRADE_COMPARE(ui.nodeUniqueLayout(node1, layouter2), LayouterDataHandle::Null);
    CORRADE_COMPARE(ui.nodeUniqueLayout(node1, layouter3), LayouterDataHandle::Null);

    /* Adding new layouts should pick items from the free list */
    LayoutHandle node1Layout3Replacement = layouter3.add(node1);
    LayoutHandle node1Layout2Replacement = layouter2.add(node1);
    LayoutHandle node1Layout1Replacement = layouter1.add(node1);
    CORRADE_COMPARE(ui.nodeUniqueLayoutCapacity(), 6);
    CORRADE_COMPARE(ui.nodeUniqueLayoutUsedCount(), 6);
    CORRADE_COMPARE(ui.nodeUniqueLayout(node1, layouter1), layoutHandleData(node1Layout1Replacement));
    CORRADE_COMPARE(ui.nodeUniqueLayout(node1, layouter2), layoutHandleData(node1Layout2Replacement));
    CORRADE_COMPARE(ui.nodeUniqueLayout(node1, layouter3), layoutHandleData(node1Layout3Replacement));

    /* Adding one more layout grows the storage capacity again */
    LayoutHandle node3Layout3 = layouter3.add(node3);
    CORRADE_COMPARE(ui.nodeUniqueLayoutCapacity(), 7);
    CORRADE_COMPARE(ui.nodeUniqueLayoutUsedCount(), 7);
    CORRADE_COMPARE(ui.nodeUniqueLayout(node3, layouter1), layoutHandleData(node3Layout1));
    CORRADE_COMPARE(ui.nodeUniqueLayout(node3, layouter2), LayouterDataHandle::Null);
    CORRADE_COMPARE(ui.nodeUniqueLayout(node3, layouter3), layoutHandleData(node3Layout3));

    /* Removing a node removes its unique layout assignments from the storage.
       The layouts are not touched, only their internal node unique layout
       references are cleared. */
    ui.removeNode(node1);
    CORRADE_COMPARE(ui.nodeUniqueLayoutCapacity(), 7);
    CORRADE_COMPARE(ui.nodeUniqueLayoutUsedCount(), 4);
    CORRADE_VERIFY(ui.isHandleValid(node1Layout1Replacement));
    CORRADE_VERIFY(ui.isHandleValid(node1Layout2Replacement));
    CORRADE_VERIFY(ui.isHandleValid(node1Layout3Replacement));
    CORRADE_COMPARE(layouter1.node(node1Layout1Replacement), node1);
    CORRADE_COMPARE(layouter2.node(node1Layout2Replacement), node1);
    CORRADE_COMPARE(layouter3.node(node1Layout3Replacement), node1);

    /* Only after update() they get removed as well. Nothing else changes for
       the node unique layout storage. */
    ui.update();
    CORRADE_COMPARE(ui.nodeUniqueLayoutCapacity(), 7);
    CORRADE_COMPARE(ui.nodeUniqueLayoutUsedCount(), 4);
    CORRADE_VERIFY(!ui.isHandleValid(node1Layout1Replacement));
    CORRADE_VERIFY(!ui.isHandleValid(node1Layout2Replacement));
    CORRADE_VERIFY(!ui.isHandleValid(node1Layout3Replacement));

    /* Removing a parent of a node containing two unique layouts does nothing */
    ui.removeNode(node2Parent);
    CORRADE_COMPARE(ui.nodeUniqueLayoutCapacity(), 7);
    CORRADE_COMPARE(ui.nodeUniqueLayoutUsedCount(), 4);
    CORRADE_VERIFY(ui.isHandleValid(node2Layout1));
    CORRADE_VERIFY(ui.isHandleValid(node2Layout2));

    /* But after update() it gets cleaned up as well */
    ui.update();
    CORRADE_COMPARE(ui.nodeUniqueLayoutCapacity(), 7);
    CORRADE_COMPARE(ui.nodeUniqueLayoutUsedCount(), 2);
    CORRADE_VERIFY(!ui.isHandleValid(node2Layout1));
    CORRADE_VERIFY(!ui.isHandleValid(node2Layout2));

    /* Removing a layouter that has no layouts anymore does nothing to the node
       unique layout storage */
    CORRADE_COMPARE(layouter2.usedCount(), 0);
    ui.removeLayouter(layouter2);
    CORRADE_COMPARE(ui.nodeUniqueLayoutCapacity(), 7);
    CORRADE_COMPARE(ui.nodeUniqueLayoutUsedCount(), 2);

    /* Add back a bunch of nodes & layouts so we don't have just one node with
       two layouts */
    NodeHandle node4 = ui.createNode({}, {});
    NodeHandle node5 = ui.createNode({}, {});
    LayoutHandle node4Layout1 = layouter1.add(node4);
    LayoutHandle node4Layout3 = layouter3.add(node4);
    LayoutHandle node5Layout1 = layouter1.add(node5);
    CORRADE_COMPARE(ui.nodeUniqueLayoutCapacity(), 7);
    CORRADE_COMPARE(ui.nodeUniqueLayoutUsedCount(), 5);
    CORRADE_COMPARE(ui.nodeUniqueLayout(node3, layouter1), layoutHandleData(node3Layout1));
    CORRADE_COMPARE(ui.nodeUniqueLayout(node3, layouter3), layoutHandleData(node3Layout3));
    CORRADE_COMPARE(ui.nodeUniqueLayout(node4, layouter1), layoutHandleData(node4Layout1));
    CORRADE_COMPARE(ui.nodeUniqueLayout(node4, layouter3), layoutHandleData(node4Layout3));
    CORRADE_COMPARE(ui.nodeUniqueLayout(node5, layouter1), layoutHandleData(node5Layout1));

    /* Removing a layouter removes all unique layout assignments from it */
    ui.removeLayouter(layouter1);
    CORRADE_COMPARE(ui.nodeUniqueLayoutCapacity(), 7);
    CORRADE_COMPARE(ui.nodeUniqueLayoutUsedCount(), 2);
    CORRADE_COMPARE(ui.nodeUniqueLayout(node3, layouter3), layoutHandleData(node3Layout3));
    CORRADE_COMPARE(ui.nodeUniqueLayout(node4, layouter3), layoutHandleData(node4Layout3));

    /* Removing even the last layouter makes the whole storage unused */
    ui.removeLayouter(layouter3);
    CORRADE_COMPARE(ui.nodeUniqueLayoutCapacity(), 7);
    CORRADE_COMPARE(ui.nodeUniqueLayoutUsedCount(), 0);
}

void AbstractLayouterTest::addRemoveHandleRecycle() {
    struct: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::add;
        using AbstractLayouter::remove;

        LayouterFeatures doFeatures() const override { return {}; }
        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const  Containers::StridedArrayView1D<Vector2>&) override {}
    } layouter{layouterHandle(0xab, 0x12)};

    LayoutHandle first = layouter.add(nodeHandle(0x1, 0xabc));
    LayoutHandle second = layouter.add(nodeHandle(0x2, 0xdef));
    LayoutHandle third = layouter.add(nodeHandle(0x3, 0xcfa));
    LayoutHandle fourth = layouter.add(nodeHandle(0x4, 0xeca));
    CORRADE_COMPARE(first, layoutHandle(layouter.handle(), 0, 1));
    CORRADE_COMPARE(second, layoutHandle(layouter.handle(), 1, 1));
    CORRADE_COMPARE(third, layoutHandle(layouter.handle(), 2, 1));
    CORRADE_COMPARE(fourth, layoutHandle(layouter.handle(), 3, 1));
    CORRADE_VERIFY(layouter.isHandleValid(first));
    CORRADE_VERIFY(layouter.isHandleValid(second));
    CORRADE_VERIFY(layouter.isHandleValid(third));
    CORRADE_VERIFY(layouter.isHandleValid(fourth));
    CORRADE_COMPARE(layouter.capacity(), 4);
    CORRADE_COMPARE(layouter.usedCount(), 4);
    CORRADE_COMPARE(layouter.node(first), nodeHandle(0x1, 0xabc));
    CORRADE_COMPARE(layouter.node(second), nodeHandle(0x2, 0xdef));
    CORRADE_COMPARE(layouter.node(third), nodeHandle(0x3, 0xcfa));
    CORRADE_COMPARE(layouter.node(fourth), nodeHandle(0x4, 0xeca));
    CORRADE_COMPARE_AS(layouter.nodes(), Containers::arrayView({
        nodeHandle(0x1, 0xabc),
        nodeHandle(0x2, 0xdef),
        nodeHandle(0x3, 0xcfa),
        nodeHandle(0x4, 0xeca)
    }), TestSuite::Compare::Container);

    /* Remove three out of the four in an arbitrary order */
    layouter.remove(fourth);
    layouter.remove(first);
    layouter.remove(third);
    CORRADE_VERIFY(!layouter.isHandleValid(first));
    CORRADE_VERIFY(layouter.isHandleValid(second));
    CORRADE_VERIFY(!layouter.isHandleValid(third));
    CORRADE_VERIFY(!layouter.isHandleValid(fourth));
    CORRADE_COMPARE(layouter.capacity(), 4);
    CORRADE_COMPARE(layouter.usedCount(), 1);
    CORRADE_COMPARE(layouter.node(second), nodeHandle(0x2, 0xdef));

    /* Internally all attachments should be set to a null handle after
       deletion */
    CORRADE_COMPARE_AS(layouter.nodes(), Containers::arrayView({
        NodeHandle::Null,
        nodeHandle(0x2, 0xdef),
        NodeHandle::Null,
        NodeHandle::Null
    }), TestSuite::Compare::Container);

    /* Handles crafted with a manually incremented generation (i.e., the
       generation that will be used next) shouldn't be reported as valid */
    LayoutHandle firstNext = layoutHandle(layouter.handle(), layoutHandleId(first), layoutHandleGeneration(first) + 1);
    LayoutHandle thirdNext = layoutHandle(layouter.handle(), layoutHandleId(third), layoutHandleGeneration(third) + 1);
    LayoutHandle fourthNext = layoutHandle(layouter.handle(), layoutHandleId(fourth), layoutHandleGeneration(fourth) + 1);
    CORRADE_VERIFY(!layouter.isHandleValid(firstNext));
    CORRADE_VERIFY(!layouter.isHandleValid(thirdNext));
    CORRADE_VERIFY(!layouter.isHandleValid(fourthNext));

    /* Allocating new handles should recycle the handles in the order they were
       removed (oldest first). They should be the same as the handles crafted
       above which should report as valid now. Their properties should be
       updated. */
    LayoutHandle fourth2 = layouter.add(nodeHandle(0x4, 0xecb));
    LayoutHandle first2 = layouter.add(nodeHandle(0x1, 0xabd));
    LayoutHandle third2 = layouter.add(nodeHandle(0x3, 0xcfb));
    CORRADE_COMPARE(first2, layoutHandle(layouter.handle(), 0, 2));
    CORRADE_COMPARE(third2, layoutHandle(layouter.handle(), 2, 2));
    CORRADE_COMPARE(fourth2, layoutHandle(layouter.handle(), 3, 2));
    CORRADE_COMPARE(first2, firstNext);
    CORRADE_COMPARE(third2, thirdNext);
    CORRADE_COMPARE(fourth2, fourthNext);
    CORRADE_VERIFY(layouter.isHandleValid(firstNext));
    CORRADE_VERIFY(layouter.isHandleValid(thirdNext));
    CORRADE_VERIFY(layouter.isHandleValid(fourthNext));
    CORRADE_COMPARE(layouter.capacity(), 4);
    CORRADE_COMPARE(layouter.usedCount(), 4);
    CORRADE_COMPARE(layouter.node(first2), nodeHandle(0x1, 0xabd));
    CORRADE_COMPARE(layouter.node(second), nodeHandle(0x2, 0xdef));
    CORRADE_COMPARE(layouter.node(third2), nodeHandle(0x3, 0xcfb));
    CORRADE_COMPARE(layouter.node(fourth2), nodeHandle(0x4, 0xecb));

    /* Old handles shouldn't get valid again */
    CORRADE_VERIFY(!layouter.isHandleValid(first));
    CORRADE_VERIFY(layouter.isHandleValid(first2));
    CORRADE_VERIFY(!layouter.isHandleValid(third));
    CORRADE_VERIFY(layouter.isHandleValid(third2));
    CORRADE_VERIFY(!layouter.isHandleValid(fourth));
    CORRADE_VERIFY(layouter.isHandleValid(fourth2));

    /* Removing a single handle and creating a new one directly reuses it if
       there's just one in the free list */
    layouter.remove(third2);
    LayoutHandle third3 = layouter.add(nodeHandle(0x3, 0xcfc));
    CORRADE_COMPARE(third3, layoutHandle(layouter.handle(), 2, 3));
    CORRADE_VERIFY(!layouter.isHandleValid(third));
    CORRADE_VERIFY(!layouter.isHandleValid(third2));
    CORRADE_VERIFY(layouter.isHandleValid(third3));
    CORRADE_COMPARE(layouter.capacity(), 4);
    CORRADE_COMPARE(layouter.usedCount(), 4);
    CORRADE_COMPARE(layouter.node(third3), nodeHandle(0x3, 0xcfc));

    /* Allocating a new handle with the free list empty will grow it */
    LayoutHandle fifth = layouter.add(nodeHandle(0x5, 0xded));
    CORRADE_COMPARE(fifth, layoutHandle(layouter.handle(), 4, 1));
    CORRADE_VERIFY(layouter.isHandleValid(fifth));
    CORRADE_COMPARE(layouter.capacity(), 5);
    CORRADE_COMPARE(layouter.usedCount(), 5);
    CORRADE_COMPARE(layouter.node(fifth), nodeHandle(0x5, 0xded));

    /* The generation counter view should reflect the number of how much was
       given ID recycled */
    CORRADE_COMPARE_AS(layouter.generations(), Containers::arrayView<UnsignedShort>({
        2,
        1,
        3,
        2,
        1
    }), TestSuite::Compare::Container);
}

void AbstractLayouterTest::addRemoveHandleDisable() {
    struct: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::add;
        using AbstractLayouter::remove;

        LayouterFeatures doFeatures() const override { return {}; }
        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const  Containers::StridedArrayView1D<Vector2>&) override {}
    } layouter{layouterHandle(0xab, 0x12)};

    LayoutHandle first = layouter.add(nodeHandle(0x1, 0x2));
    CORRADE_COMPARE(first, layoutHandle(layouter.handle(), 0, 1));

    for(std::size_t i = 0; i != (1 << Implementation::LayouterDataHandleGenerationBits) - 1; ++i) {
        LayoutHandle second = layouter.add(nodeHandle(0x1, 0x2));
        CORRADE_COMPARE(second, layoutHandle(layouter.handle(), 1, 1 + i));
        layouter.remove(second);
    }

    /* The generation for the second slot is exhausted so the handle is not
       recycled */
    CORRADE_COMPARE(layouter.capacity(), 2);
    CORRADE_COMPARE(layouter.usedCount(), 2);

    /* It shouldn't think a handle from the second slot with generation 0 is
       valid */
    CORRADE_VERIFY(!layouter.isHandleValid(layoutHandle(layouter.handle(), 1, 0)));

    /* There's nowhere to create a new handle from so the capacity is grown */
    LayoutHandle third = layouter.add(nodeHandle(0x1, 0x2));
    CORRADE_COMPARE(third, layoutHandle(layouter.handle(), 2, 1));
    CORRADE_COMPARE(layouter.capacity(), 3);
    CORRADE_COMPARE(layouter.usedCount(), 3);

    /* The generation counter view should have 0 for the disabled slot */
    CORRADE_COMPARE_AS(layouter.generations(), Containers::arrayView<UnsignedShort>({
        1,
        0,
        1
    }), TestSuite::Compare::Container);
}

void AbstractLayouterTest::addInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::add;

        LayouterFeatures doFeatures() const override { return {}; }
        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const  Containers::StridedArrayView1D<Vector2>&) override {}
    } layouter{layouterHandle(0, 1)};

    Containers::String out;
    Error redirectError{&out};
    layouter.add(NodeHandle::Null);
    layouter.add(nodeHandle(0xabcde, 0));
    CORRADE_COMPARE_AS(out,
        "Ui::AbstractLayouter::add(): invalid handle Ui::NodeHandle::Null\n"
        "Ui::AbstractLayouter::add(): invalid handle Ui::NodeHandle(0xabcde, 0x0)\n",
        TestSuite::Compare::String);
}

void AbstractLayouterTest::addInvalidUniqueLayouts() {
    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui{{100, 100}};

    /* Just to have a non-trivial node handle */
    NodeHandle node1 = ui.createNode({}, {});
    NodeHandle node2 = ui.createNode({}, {});
    ui.removeNode(ui.createNode({}, {}));
    NodeHandle node3 = ui.createNode({}, {});

    struct Layouter: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::add;
        using AbstractLayouter::remove;

        LayouterFeatures doFeatures() const override {
            return LayouterFeature::UniqueLayouts;
        }
        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const  Containers::StridedArrayView1D<Vector2>&) override {}
    } layouterNoUi{layouterHandle(0, 1)};

    /* ... and a non-trivial layouter handle */
    ui.createLayouter();
    ui.createLayouter();
    ui.createLayouter();
    Layouter& layouter = ui.setLayouterInstance(Containers::pointer<Layouter>(ui.createLayouter()));

    /* ... and a non-trivial layout handle */
    layouter.add(node1);
    layouter.remove(layouter.add(node2));
    LayoutHandle layout = layouter.add(node3);
    CORRADE_COMPARE(ui.nodeUniqueLayout(node3, layouter), layoutHandleData(layout));

    Containers::String out;
    Error redirectError{&out};
    layouterNoUi.add(node3);
    layouter.add(NodeHandle::Null);
    layouter.add(nodeHandle(0x12345, 0xabc));
    layouter.add(node3);
    CORRADE_COMPARE_AS(out,
        "Ui::AbstractLayouter::add(): layouter not part of a user interface\n"
        "Ui::AbstractLayouter::add(): invalid handle Ui::NodeHandle::Null\n"
        "Ui::AbstractLayouter::add(): invalid handle Ui::NodeHandle(0x12345, 0xabc)\n"
        "Ui::AbstractLayouter::add(): Ui::NodeHandle(0x2, 0x2) already has Ui::LayoutHandle({0x3, 0x1}, {0x1, 0x2}) from this layouter\n",
        TestSuite::Compare::String);
}

void AbstractLayouterTest::addNoHandlesLeft() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::add;

        LayouterFeatures doFeatures() const override { return {}; }
        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const  Containers::StridedArrayView1D<Vector2>&) override {}
    } layouter{layouterHandle(0, 1)};

    for(std::size_t i = 0; i != 1 << Implementation::LayouterDataHandleIdBits; ++i)
        layouter.add(nodeHandle(0x1, 0x2));

    CORRADE_COMPARE(layouter.capacity(), 1 << Implementation::LayouterDataHandleIdBits);
    CORRADE_COMPARE(layouter.usedCount(), 1 << Implementation::LayouterDataHandleIdBits);

    Containers::String out;
    Error redirectError{&out};
    layouter.add(nodeHandle(0x1, 0x2));
    /* Number is hardcoded in the expected message but not elsewhere in order
       to give a heads-up when modifying the handle ID bit count */
    CORRADE_COMPARE(out,
        "Ui::AbstractLayouter::add(): can only have at most 1048576 layouts\n");
}

void AbstractLayouterTest::removeUniqueLayoutInvalidNode() {
    AbstractUserInterface ui{{100, 100}};

    struct Layouter: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::add;
        using AbstractLayouter::remove;

        LayouterFeatures doFeatures() const override {
            return LayouterFeature::UniqueLayouts;
        }
        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&) override {}
    };
    Layouter& layouter = ui.setLayouterInstance(Containers::pointer<Layouter>(ui.createLayouter()));

    NodeHandle node = ui.createNode({}, {});
    LayoutHandle layout = layouter.add(node);
    CORRADE_COMPARE(ui.nodeUniqueLayoutCapacity(), 1);
    CORRADE_COMPARE(ui.nodeUniqueLayoutUsedCount(), 1);
    CORRADE_COMPARE(ui.nodeUniqueLayout(node, layouter), layoutHandleData(layout));

    /* Removing the node removes the node unique layout assignment already, but
       the layout itself still is assigned to it */
    ui.removeNode(node);
    CORRADE_COMPARE(ui.nodeUniqueLayoutCapacity(), 1);
    CORRADE_COMPARE(ui.nodeUniqueLayoutUsedCount(), 0);
    CORRADE_COMPARE(layouter.node(layout), node);

    /* Now removing the layout should not attempt to remove the node unique
       layout anymore, as it's gone already */
    layouter.remove(layout);
    CORRADE_COMPARE(ui.nodeUniqueLayoutCapacity(), 1);
    CORRADE_COMPARE(ui.nodeUniqueLayoutUsedCount(), 0);
}

void AbstractLayouterTest::removeInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::add;
        using AbstractLayouter::remove;

        LayouterFeatures doFeatures() const override { return {}; }
        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const  Containers::StridedArrayView1D<Vector2>&) override {}
    } layouter{layouterHandle(0, 1)};

    LayoutHandle handle = layouter.add(nodeHandle(0x1, 0x2));

    Containers::String out;
    Error redirectError{&out};
    layouter.remove(LayoutHandle::Null);
    /* Valid layouter, invalid data */
    layouter.remove(layoutHandle(layouter.handle(), LayouterDataHandle(0x123abcde)));
    /* Invalid layouter, valid data */
    layouter.remove(layoutHandle(LayouterHandle::Null, layoutHandleData(handle)));
    /* LayouterDataHandle directly */
    layouter.remove(LayouterDataHandle(0x123abcde));
    CORRADE_COMPARE_AS(out,
        "Ui::AbstractLayouter::remove(): invalid handle Ui::LayoutHandle::Null\n"
        "Ui::AbstractLayouter::remove(): invalid handle Ui::LayoutHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Ui::AbstractLayouter::remove(): invalid handle Ui::LayoutHandle(Null, {0x0, 0x1})\n"
        "Ui::AbstractLayouter::remove(): invalid handle Ui::LayouterDataHandle(0xabcde, 0x123)\n",
        TestSuite::Compare::String);
}

void AbstractLayouterTest::nodeInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::add;

        LayouterFeatures doFeatures() const override { return {}; }
        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const  Containers::StridedArrayView1D<Vector2>&) override {}
    } layouter{layouterHandle(0xab, 0x12)};

    LayoutHandle handle = layouter.add(nodeHandle(0x1, 0x2));

    Containers::String out;
    Error redirectError{&out};
    layouter.node(LayoutHandle::Null);
    /* Valid layer, invalid data */
    layouter.node(layoutHandle(layouter.handle(), LayouterDataHandle(0x123abcde)));
    /* Invalid layer, valid data */
    layouter.node(layoutHandle(LayouterHandle::Null, layoutHandleData(handle)));
    /* LayerDataHandle directly */
    layouter.node(LayouterDataHandle(0x123abcde));
    CORRADE_COMPARE_AS(out,
        "Ui::AbstractLayouter::node(): invalid handle Ui::LayoutHandle::Null\n"
        "Ui::AbstractLayouter::node(): invalid handle Ui::LayoutHandle({0xab, 0x12}, {0xabcde, 0x123})\n"
        "Ui::AbstractLayouter::node(): invalid handle Ui::LayoutHandle(Null, {0x0, 0x1})\n"
        "Ui::AbstractLayouter::node(): invalid handle Ui::LayouterDataHandle(0xabcde, 0x123)\n",
        TestSuite::Compare::String);
}

void AbstractLayouterTest::setSize() {
    struct: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;

        LayouterFeatures doFeatures() const override { return {}; }
        void doSetSize(const Vector2& size) override {
            ++called;
            CORRADE_COMPARE(size, (Vector2{1.0f, 2.0f}));
        }
        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const  Containers::StridedArrayView1D<Vector2>&) override {}

        Int called = 0;
    } layouter{layouterHandle(0, 1)};

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    layouter.setSize({1.0f, 2.0f});
    CORRADE_COMPARE(layouter.called, 1);
}

void AbstractLayouterTest::setSizeZero() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;

        LayouterFeatures doFeatures() const override { return {}; }
        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const  Containers::StridedArrayView1D<Vector2>&) override {}
    } layouter{layouterHandle(0, 1)};

    Containers::String out;
    Error redirectError{&out};
    layouter.setSize({0.0f, 1.0f});
    layouter.setSize({1.0f, 0.0f});
    CORRADE_COMPARE_AS(out,
        "Ui::AbstractLayouter::setSize(): expected a non-zero size, got Vector(0, 1)\n"
        "Ui::AbstractLayouter::setSize(): expected a non-zero size, got Vector(1, 0)\n",
        TestSuite::Compare::String);
}

void AbstractLayouterTest::setSizeNotImplemented() {
    struct: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;

        LayouterFeatures doFeatures() const override { return {}; }
        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const  Containers::StridedArrayView1D<Vector2>&) override {}
    } layouter{layouterHandle(0, 1)};

    layouter.setSize({1.0f, 2.0f});

    /* Shouldn't crash or anything */
    CORRADE_VERIFY(true);
}

void AbstractLayouterTest::cleanNodes() {
    struct: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::add;
        using AbstractLayouter::remove;

        LayouterFeatures doFeatures() const override { return {}; }
        void doClean(Containers::BitArrayView dataIdsToRemove) override {
            ++called;
            CORRADE_COMPARE_AS(dataIdsToRemove, Containers::stridedArrayView({
                true, false, true, false, true, false
            }).sliceBit(0), TestSuite::Compare::Container);
        }
        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const  Containers::StridedArrayView1D<Vector2>&) override {}

        Int called = 0;
    } layouter{layouterHandle(0, 1)};

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    NodeHandle nodeFirst = nodeHandle(0, 0xcec);
    NodeHandle nodeSecond = nodeHandle(1, 0xded);
    NodeHandle nodeFourth = nodeHandle(3, 0xaba);
    NodeHandle nodeEighth = nodeHandle(7, 0xfef);

    /* Create six data to match the six bits. Attach them to random handles,
       leave one unassigned, attach two data to one node. */
    LayoutHandle first = layouter.add(nodeEighth);
    LayoutHandle second = layouter.add(nodeSecond);
    LayoutHandle third = layouter.add(nodeFirst);
    LayoutHandle fourth = layouter.add(nodeFourth);
    LayoutHandle fifth = layouter.add(nodeFirst);
    LayoutHandle sixth = layouter.add(nodeFourth);

    /* Remove two of them */
    layouter.remove(second);
    layouter.remove(sixth);

    /* Call cleanNodes() with updated generation counters */
    layouter.cleanNodes(Containers::arrayView({
        /* First node generation gets different, affecting third and fifth
           data */
        UnsignedShort(nodeHandleGeneration(nodeFirst) + 1),
        /* Second node generation gets different but since the second data is
           already removed it doesn't affect anything */
        UnsignedShort(nodeHandleGeneration(nodeSecond) - 1),
        /* Third node has no attachments so it can be arbitrary */
        UnsignedShort{0xbeb},
        /* Fourth node stays the same generation so the fourth data stay. Sixth
           data are already removed so they aren't set for deletion either. */
        UnsignedShort(nodeHandleGeneration(nodeFourth)),
        /* Fifth, sixth, seventh nodes have no attachments so they can be
           arbitrary again */
        UnsignedShort{0xaca},
        UnsignedShort{0x808},
        UnsignedShort{0xefe},
        /* Eighth node is now a zero generation, i.e. disabled, which should
           trigger removal of first data */
        UnsignedShort{},
    }));
    CORRADE_COMPARE(layouter.called, 1);

    /* Only the fourth data should stay afterwards */
    CORRADE_VERIFY(!layouter.isHandleValid(first));
    CORRADE_VERIFY(!layouter.isHandleValid(second));
    CORRADE_VERIFY(!layouter.isHandleValid(third));
    CORRADE_VERIFY(layouter.isHandleValid(fourth));
    CORRADE_VERIFY(!layouter.isHandleValid(fifth));
    CORRADE_VERIFY(!layouter.isHandleValid(sixth));
}

void AbstractLayouterTest::cleanNodesEmpty() {
    struct: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;

        LayouterFeatures doFeatures() const override { return {}; }
        void doClean(Containers::BitArrayView) override {
            ++called;
        }
        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const  Containers::StridedArrayView1D<Vector2>&) override {}

        Int called = 0;
    } layouter{layouterHandle(0, 1)};

    /* It should call the implementation even with empty contents */
    layouter.cleanNodes({});
    CORRADE_COMPARE(layouter.called, 1);
}

void AbstractLayouterTest::cleanNodesNotImplemented() {
    struct: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;

        LayouterFeatures doFeatures() const override { return {}; }
        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const  Containers::StridedArrayView1D<Vector2>&) override {}
    } layouter{layouterHandle(0, 1)};

    layouter.cleanNodes({});

    /* Shouldn't crash or anything */
    CORRADE_VERIFY(true);
}

void AbstractLayouterTest::update() {
    struct: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::add;

        LayouterFeatures doFeatures() const override { return {}; }
        void doUpdate(Containers::BitArrayView layoutIdsToUpdate, const Containers::StridedArrayView1D<const UnsignedInt>& topLevelLayoutIds, const Containers::StridedArrayView1D<const NodeHandle>& nodeParents, const Containers::StridedArrayView1D<Vector2>& nodeOffsets, const  Containers::StridedArrayView1D<Vector2>& nodeSizes) override {
            ++called;
            CORRADE_COMPARE_AS(layoutIdsToUpdate, Containers::stridedArrayView({
                false, true, false, false, true
            }).sliceBit(0), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(topLevelLayoutIds, Containers::arrayView({
                0xabcdeu,
                0x45678u
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(nodeParents, Containers::arrayView({
                NodeHandle::Null,
                nodeHandle(7, 1),
                nodeHandle(1, 7)
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(nodeOffsets, Containers::stridedArrayView<Vector2>({
                {1.0f, 2.0f},
                {3.0f, 4.0f},
                {5.0f, 6.0f}
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(nodeSizes, Containers::stridedArrayView<Vector2>({
                {0.1f, 0.2f},
                {0.3f, 0.4f},
                {0.5f, 0.6f}
            }), TestSuite::Compare::Container);
        }

        Int called = 0;
    } layouter{layouterHandle(0, 1)};

    layouter.add(nodeHandle(0, 1));
    layouter.add(nodeHandle(1, 1));
    layouter.add(nodeHandle(2, 1));
    layouter.add(nodeHandle(3, 1));
    layouter.add(nodeHandle(4, 1));

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    /* Required to be called before update() (because AbstractUserInterface
       guarantees the same on a higher level), not needed for anything here */
    layouter.setSize({1, 1});

    Vector2 nodeOffsets[]{
        {1.0f, 2.0f},
        {3.0f, 4.0f},
        {5.0f, 6.0f}
    };
    Vector2 nodeSizes[]{
        {0.1f, 0.2f},
        {0.3f, 0.4f},
        {0.5f, 0.6f}
    };
    UnsignedByte layoutIdsToUpdate[]{
        0x12
    };
    layouter.update(
        Containers::BitArrayView{layoutIdsToUpdate, 0, 5},
        Containers::arrayView({
            0xabcdeu,
            0x45678u,
        }),
        Containers::arrayView({
            NodeHandle::Null,
            nodeHandle(7, 1),
            nodeHandle(1, 7)
        }),
        nodeOffsets,
        nodeSizes);
    CORRADE_COMPARE(layouter.called, 1);
}

void AbstractLayouterTest::updateEmpty() {
    struct: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;

        LayouterFeatures doFeatures() const override { return {}; }
        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const  Containers::StridedArrayView1D<Vector2>&) override {
            ++called;
        }

        Int called = 0;
    } layouter{layouterHandle(0, 1)};

    /* Required to be called before update() (because AbstractUserInterface
       guarantees the same on a higher level), not needed for anything here */
    layouter.setSize({1, 1});

    /* It should call the implementation even with empty contents */
    layouter.update({}, {}, {}, {}, {});
    CORRADE_COMPARE(layouter.called, 1);
}

void AbstractLayouterTest::updateInvalidSizes() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::add;

        LayouterFeatures doFeatures() const override { return {}; }
        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const  Containers::StridedArrayView1D<Vector2>&) override {}
    } layouter{layouterHandle(0, 1)};

    layouter.add(nodeHandle(0, 1));
    layouter.add(nodeHandle(1, 1));
    layouter.add(nodeHandle(2, 1));
    layouter.add(nodeHandle(3, 1));
    layouter.add(nodeHandle(4, 1));

    Containers::String out;
    Error redirectError{&out};
    UnsignedByte layoutIdsToUpdate[1]{};
    NodeHandle parents[2];
    NodeHandle parentsInvalid[3];
    Vector2 offsets[2];
    Vector2 offsetsInvalid[3];
    Vector2 sizes[2];
    Vector2 sizesInvalid[3];
    layouter.update(Containers::BitArrayView{layoutIdsToUpdate, 0, 6}, {}, parents, offsets, sizes);
    layouter.update(Containers::BitArrayView{layoutIdsToUpdate, 0, 5}, {}, parentsInvalid, offsets, sizes);
    layouter.update(Containers::BitArrayView{layoutIdsToUpdate, 0, 5}, {}, parents, offsetsInvalid, sizes);
    layouter.update(Containers::BitArrayView{layoutIdsToUpdate, 0, 5}, {}, parents, offsets, sizesInvalid);
    CORRADE_COMPARE(out,
        "Ui::AbstractLayouter::update(): expected layoutIdsToUpdate to have 5 bits but got 6\n"
        "Ui::AbstractLayouter::update(): expected node parent, offset and size views to have the same size but got 3, 2 and 2\n"
        "Ui::AbstractLayouter::update(): expected node parent, offset and size views to have the same size but got 2, 3 and 2\n"
        "Ui::AbstractLayouter::update(): expected node parent, offset and size views to have the same size but got 2, 2 and 3\n");
}

void AbstractLayouterTest::updateNoSizeSet() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;

        LayouterFeatures doFeatures() const override { return {}; }
        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const  Containers::StridedArrayView1D<Vector2>&) override {
            ++called;
        }

        Int called = 0;
    } layouter{layouterHandle(0, 1)};

    Containers::String out;
    Error redirectError{&out};
    layouter.update({}, {}, {}, {}, {});
    CORRADE_COMPARE(out, "Ui::AbstractLayouter::update(): user interface size wasn't set\n");
}

void AbstractLayouterTest::state() {
    struct: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::add;
        using AbstractLayouter::remove;

        LayouterFeatures doFeatures() const override { return {}; }
        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const  Containers::StridedArrayView1D<Vector2>&) override {}
    } layouter{layouterHandle(0, 1)};

    /* Required to be called before update() (because AbstractUserInterface
       guarantees the same on a higher level), not needed for anything here */
    layouter.setSize({1, 1});

    CORRADE_COMPARE(layouter.state(), LayouterStates{});

    /* Creating a data adds a state flag */
    LayoutHandle layout1 = layouter.add(nodeHandle(0, 0x123));
    LayoutHandle layout2 = layouter.add(nodeHandle(1, 0x231));
    LayoutHandle layout3 = layouter.add(nodeHandle(2, 0x321));
    CORRADE_COMPARE(layouter.state(), LayouterState::NeedsAssignmentUpdate);

    UnsignedByte layoutIdsToUpdateData[1]{};
    Containers::BitArrayView layoutIdsToUpdate{layoutIdsToUpdateData, 0, 3};

    /* update() then resets it */
    layouter.update(layoutIdsToUpdate, {}, {}, {}, {});
    CORRADE_COMPARE(layouter.state(), LayouterStates{});

    /* No other way to trigger this flag */
    layouter.setNeedsUpdate();
    CORRADE_COMPARE(layouter.state(), LayouterState::NeedsUpdate);

    /* update() then resets it */
    layouter.update(layoutIdsToUpdate, {}, {}, {}, {});
    CORRADE_COMPARE(layouter.state(), LayouterStates{});

    /* remove() adds NeedsAttachmentUpdate */
    layouter.remove(layout2);
    CORRADE_COMPARE(layouter.state(), LayouterState::NeedsAssignmentUpdate);

    /* update() then resets one */
    layouter.update(layoutIdsToUpdate, {}, {}, {}, {});
    CORRADE_COMPARE(layouter.state(), LayouterStates{});

    /* Testing the other overload */
    layouter.remove(layoutHandleData(layout3));
    CORRADE_COMPARE(layouter.state(), LayouterState::NeedsAssignmentUpdate);

    /* cleanNodes() (no-op in this case) doesn't remove any flags on its own */
    CORRADE_COMPARE(layouter.usedCount(), 1);
    layouter.cleanNodes(Containers::arrayView({
        UnsignedShort{0x123},
        UnsignedShort{0x231},
        UnsignedShort{0x321},
    }));
    CORRADE_COMPARE(layouter.usedCount(), 1);
    CORRADE_COMPARE(layouter.state(), LayouterState::NeedsAssignmentUpdate);

    /* Only update() does */
    layouter.update(layoutIdsToUpdate, {}, {}, {}, {});
    CORRADE_COMPARE(layouter.state(), LayouterStates{});

    /* cleanNodes() that removes a data doesn't set any flags either */
    CORRADE_VERIFY(layouter.isHandleValid(layout1));
    layouter.cleanNodes(Containers::arrayView({UnsignedShort{0xfef}}));
    CORRADE_COMPARE(layouter.state(), LayouterStates{});
    CORRADE_VERIFY(!layouter.isHandleValid(layout1));
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::AbstractLayouterTest)
