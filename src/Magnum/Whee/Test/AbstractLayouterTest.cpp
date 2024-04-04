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
#include <Corrade/Containers/StridedBitArrayView.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/StringStl.h> /** @todo remove once Debug is stream-free */
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Container.h>
#include <Corrade/TestSuite/Compare/String.h>
#include <Corrade/Utility/DebugStl.h> /** @todo remove once Debug is stream-free */
#include <Magnum/Math/Vector2.h>

#include "Magnum/Whee/AbstractLayouter.h"
#include "Magnum/Whee/Handle.h"

namespace Magnum { namespace Whee { namespace Test { namespace {

struct AbstractLayouterTest: TestSuite::Tester {
    explicit AbstractLayouterTest();

    void debugState();
    void debugStates();
    void debugStatesSupersets();

    void construct();
    void constructInvalidHandle();
    void constructCopy();
    void constructMove();

    void addRemove();
    void addRemoveHandleRecycle();
    void addRemoveHandleDisable();
    void addNullNode();
    void addNoHandlesLeft();
    void removeInvalid();
    void nodeInvalid();

    void cleanNodes();
    void cleanNodesEmpty();
    void cleanNodesNotImplemented();

    void update();
    void updateEmpty();
    void updateInvalidSizes();

    void state();
};

AbstractLayouterTest::AbstractLayouterTest() {
    addTests({&AbstractLayouterTest::debugState,
              &AbstractLayouterTest::debugStates,
              &AbstractLayouterTest::debugStatesSupersets,

              &AbstractLayouterTest::construct,
              &AbstractLayouterTest::constructInvalidHandle,
              &AbstractLayouterTest::constructCopy,
              &AbstractLayouterTest::constructMove,

              &AbstractLayouterTest::addRemove,
              &AbstractLayouterTest::addRemoveHandleRecycle,
              &AbstractLayouterTest::addRemoveHandleDisable,
              &AbstractLayouterTest::addNullNode,
              &AbstractLayouterTest::addNoHandlesLeft,
              &AbstractLayouterTest::removeInvalid,
              &AbstractLayouterTest::nodeInvalid,

              &AbstractLayouterTest::cleanNodes,
              &AbstractLayouterTest::cleanNodesEmpty,
              &AbstractLayouterTest::cleanNodesNotImplemented,

              &AbstractLayouterTest::update,
              &AbstractLayouterTest::updateEmpty,
              &AbstractLayouterTest::updateInvalidSizes,

              &AbstractLayouterTest::state});
}

void AbstractLayouterTest::debugState() {
    std::ostringstream out;
    Debug{&out} << LayouterState::NeedsUpdate << LayouterState(0xbe);
    CORRADE_COMPARE(out.str(), "Whee::LayouterState::NeedsUpdate Whee::LayouterState(0xbe)\n");
}

void AbstractLayouterTest::debugStates() {
    std::ostringstream out;
    Debug{&out} << (LayouterState::NeedsUpdate|LayouterState(0xe0)) << LayouterStates{};
    CORRADE_COMPARE(out.str(), "Whee::LayouterState::NeedsUpdate|Whee::LayouterState(0xe0) Whee::LayouterStates{}\n");
}

void AbstractLayouterTest::debugStatesSupersets() {
    /* NeedsAssignmentUpdate is a superset of NeedsUpdate, so only one should
       be printed */
    {
        std::ostringstream out;
        Debug{&out} << (LayouterState::NeedsUpdate|LayouterState::NeedsAssignmentUpdate);
        CORRADE_COMPARE(out.str(), "Whee::LayouterState::NeedsAssignmentUpdate\n");
    }
}

void AbstractLayouterTest::construct() {
    struct: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;

        void doUpdate(const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<Vector2>&, const  Containers::StridedArrayView1D<Vector2>&) override {}
    } layouter{layouterHandle(0xab, 0x12)};

    CORRADE_COMPARE(layouter.handle(), layouterHandle(0xab, 0x12));
    CORRADE_COMPARE(layouter.state(), LayouterStates{});
    CORRADE_COMPARE(layouter.capacity(), 0);
    CORRADE_COMPARE(layouter.usedCount(), 0);
    CORRADE_VERIFY(!layouter.isHandleValid(LayouterDataHandle::Null));
    CORRADE_VERIFY(!layouter.isHandleValid(LayoutHandle::Null));
}

void AbstractLayouterTest::constructInvalidHandle() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Layouter: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;

        void doUpdate(const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<Vector2>&, const  Containers::StridedArrayView1D<Vector2>&) override {}
    };

    std::ostringstream out;
    Error redirectError{&out};
    Layouter{LayouterHandle::Null};
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractLayouter: handle is null\n");
}

void AbstractLayouterTest::constructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<AbstractLayouter>{});
    CORRADE_VERIFY(!std::is_copy_assignable<AbstractLayouter>{});
}

void AbstractLayouterTest::constructMove() {
    struct Layouter: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;

        void doUpdate(const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<Vector2>&, const  Containers::StridedArrayView1D<Vector2>&) override {}
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

void AbstractLayouterTest::addRemove() {
    struct: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::add;
        using AbstractLayouter::remove;

        void doUpdate(const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<Vector2>&, const  Containers::StridedArrayView1D<Vector2>&) override {}
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

void AbstractLayouterTest::addRemoveHandleRecycle() {
    struct: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::add;
        using AbstractLayouter::remove;

        void doUpdate(const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<Vector2>&, const  Containers::StridedArrayView1D<Vector2>&) override {}
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

    /* Allocating new handles should recycle the handles in the order they were
       removed (oldest first). Their properties should be updated. */
    LayoutHandle fourth2 = layouter.add(nodeHandle(0x4, 0xecb));
    LayoutHandle first2 = layouter.add(nodeHandle(0x1, 0xabd));
    LayoutHandle third2 = layouter.add(nodeHandle(0x3, 0xcfb));
    CORRADE_COMPARE(first2, layoutHandle(layouter.handle(), 0, 2));
    CORRADE_COMPARE(third2, layoutHandle(layouter.handle(), 2, 2));
    CORRADE_COMPARE(fourth2, layoutHandle(layouter.handle(), 3, 2));
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
}

void AbstractLayouterTest::addRemoveHandleDisable() {
    struct: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::add;
        using AbstractLayouter::remove;

        void doUpdate(const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<Vector2>&, const  Containers::StridedArrayView1D<Vector2>&) override {}
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
}

void AbstractLayouterTest::addNullNode() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::add;

        void doUpdate(const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<Vector2>&, const  Containers::StridedArrayView1D<Vector2>&) override {}
    } layouter{layouterHandle(0, 1)};

    std::ostringstream out;
    Error redirectError{&out};
    layouter.add(NodeHandle::Null);
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractLayouter::add(): invalid handle Whee::NodeHandle::Null\n");
}

void AbstractLayouterTest::addNoHandlesLeft() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::add;

        void doUpdate(const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<Vector2>&, const  Containers::StridedArrayView1D<Vector2>&) override {}
    } layouter{layouterHandle(0, 1)};

    for(std::size_t i = 0; i != 1 << Implementation::LayouterDataHandleIdBits; ++i)
        layouter.add(nodeHandle(0x1, 0x2));

    CORRADE_COMPARE(layouter.capacity(), 1 << Implementation::LayouterDataHandleIdBits);
    CORRADE_COMPARE(layouter.usedCount(), 1 << Implementation::LayouterDataHandleIdBits);

    std::ostringstream out;
    Error redirectError{&out};
    layouter.add(nodeHandle(0x1, 0x2));
    /* Number is hardcoded in the expected message but not elsewhere in order
       to give a heads-up when modifying the handle ID bit count */
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractLayouter::add(): can only have at most 1048576 layouts\n");
}

void AbstractLayouterTest::removeInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::add;
        using AbstractLayouter::remove;

        void doUpdate(const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<Vector2>&, const  Containers::StridedArrayView1D<Vector2>&) override {}
    } layouter{layouterHandle(0, 1)};

    LayoutHandle handle = layouter.add(nodeHandle(0x1, 0x2));

    std::ostringstream out;
    Error redirectError{&out};
    layouter.remove(LayoutHandle::Null);
    /* Valid layouter, invalid data */
    layouter.remove(layoutHandle(layouter.handle(), LayouterDataHandle(0x123abcde)));
    /* Invalid layouter, valid data */
    layouter.remove(layoutHandle(LayouterHandle::Null, layoutHandleData(handle)));
    /* LayouterDataHandle directly */
    layouter.remove(LayouterDataHandle(0x123abcde));
    CORRADE_COMPARE_AS(out.str(),
        "Whee::AbstractLayouter::remove(): invalid handle Whee::LayoutHandle::Null\n"
        "Whee::AbstractLayouter::remove(): invalid handle Whee::LayoutHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Whee::AbstractLayouter::remove(): invalid handle Whee::LayoutHandle(Null, {0x0, 0x1})\n"
        "Whee::AbstractLayouter::remove(): invalid handle Whee::LayouterDataHandle(0xabcde, 0x123)\n",
        TestSuite::Compare::String);
}

void AbstractLayouterTest::nodeInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::add;

        void doUpdate(const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<Vector2>&, const  Containers::StridedArrayView1D<Vector2>&) override {}
    } layouter{layouterHandle(0xab, 0x12)};

    LayoutHandle handle = layouter.add(nodeHandle(0x1, 0x2));

    std::ostringstream out;
    Error redirectError{&out};
    layouter.node(LayoutHandle::Null);
    /* Valid layer, invalid data */
    layouter.node(layoutHandle(layouter.handle(), LayouterDataHandle(0x123abcde)));
    /* Invalid layer, valid data */
    layouter.node(layoutHandle(LayouterHandle::Null, layoutHandleData(handle)));
    /* LayerDataHandle directly */
    layouter.node(LayouterDataHandle(0x123abcde));
    CORRADE_COMPARE_AS(out.str(),
        "Whee::AbstractLayouter::node(): invalid handle Whee::LayoutHandle::Null\n"
        "Whee::AbstractLayouter::node(): invalid handle Whee::LayoutHandle({0xab, 0x12}, {0xabcde, 0x123})\n"
        "Whee::AbstractLayouter::node(): invalid handle Whee::LayoutHandle(Null, {0x0, 0x1})\n"
        "Whee::AbstractLayouter::node(): invalid handle Whee::LayouterDataHandle(0xabcde, 0x123)\n",
        TestSuite::Compare::String);
}

void AbstractLayouterTest::cleanNodes() {
    struct: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::add;
        using AbstractLayouter::remove;

        void doClean(Containers::BitArrayView dataIdsToRemove) override {
            ++called;
            CORRADE_COMPARE_AS(dataIdsToRemove, Containers::stridedArrayView({
                true, false, true, false, true, false
            }).sliceBit(0), TestSuite::Compare::Container);
        }
        void doUpdate(const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<Vector2>&, const  Containers::StridedArrayView1D<Vector2>&) override {}

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

        void doClean(Containers::BitArrayView) override {
            ++called;
        }
        void doUpdate(const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<Vector2>&, const  Containers::StridedArrayView1D<Vector2>&) override {}

        Int called = 0;
    } layouter{layouterHandle(0, 1)};

    /* It should call the implementation even with empty contents */
    layouter.cleanNodes({});
    CORRADE_COMPARE(layouter.called, 1);
}

void AbstractLayouterTest::cleanNodesNotImplemented() {
    struct: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;

        void doUpdate(const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<Vector2>&, const  Containers::StridedArrayView1D<Vector2>&) override {}
    } layouter{layouterHandle(0, 1)};

    layouter.cleanNodes({});

    /* Shouldn't crash or anything */
    CORRADE_VERIFY(true);
}

void AbstractLayouterTest::update() {
    struct: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;

        void doUpdate(const Containers::StridedArrayView1D<const UnsignedInt>& topLevelLayoutIds, const Containers::StridedArrayView1D<Vector2>& nodeOffsets, const  Containers::StridedArrayView1D<Vector2>& nodeSizes) override {
            ++called;
            CORRADE_COMPARE_AS(topLevelLayoutIds, Containers::arrayView({
                0xabcdeu,
                0x45678u
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

    /* Capture correct function name */
    CORRADE_VERIFY(true);

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
    layouter.update(
        Containers::arrayView({
            0xabcdeu,
            0x45678u,
        }),
        nodeOffsets,
        nodeSizes);
    CORRADE_COMPARE(layouter.called, 1);
}

void AbstractLayouterTest::updateEmpty() {
    struct: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;

        void doUpdate(const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<Vector2>&, const  Containers::StridedArrayView1D<Vector2>&) override {
            ++called;
        }

        Int called = 0;
    } layouter{layouterHandle(0, 1)};

    /* It should call the implementation even with empty contents */
    layouter.update({}, {}, {});
    CORRADE_COMPARE(layouter.called, 1);
}

void AbstractLayouterTest::updateInvalidSizes() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;

        void doUpdate(const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<Vector2>&, const  Containers::StridedArrayView1D<Vector2>&) override {}
    } layouter{layouterHandle(0, 1)};

    std::ostringstream out;
    Error redirectError{&out};
    Vector2 offsets[2];
    Vector2 sizes[3];
    layouter.update({}, offsets, sizes);
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractLayouter::update(): expected node offset and size views to have the same size but got 2 and 3\n");
}

void AbstractLayouterTest::state() {
    struct: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::add;
        using AbstractLayouter::remove;

        void doUpdate(const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<Vector2>&, const  Containers::StridedArrayView1D<Vector2>&) override {}
    } layouter{layouterHandle(0, 1)};

    CORRADE_COMPARE(layouter.state(), LayouterStates{});

    /* Creating a data adds a state flag */
    LayoutHandle layout1 = layouter.add(nodeHandle(0, 0x123));
    LayoutHandle layout2 = layouter.add(nodeHandle(1, 0x231));
    LayoutHandle layout3 = layouter.add(nodeHandle(2, 0x321));
    CORRADE_COMPARE(layouter.state(), LayouterState::NeedsAssignmentUpdate);

    /* update() then resets it */
    layouter.update({}, {}, {});
    CORRADE_COMPARE(layouter.state(), LayouterStates{});

    /* No other way to trigger this flag */
    layouter.setNeedsUpdate();
    CORRADE_COMPARE(layouter.state(), LayouterState::NeedsUpdate);

    /* update() then resets it */
    layouter.update({}, {}, {});
    CORRADE_COMPARE(layouter.state(), LayouterStates{});

    /* remove() adds NeedsAttachmentUpdate */
    layouter.remove(layout2);
    CORRADE_COMPARE(layouter.state(), LayouterState::NeedsAssignmentUpdate);

    /* update() then resets one */
    layouter.update({}, {}, {});
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
    layouter.update({}, {}, {});
    CORRADE_COMPARE(layouter.state(), LayouterStates{});

    /* cleanNodes() that removes a data doesn't set any flags either */
    CORRADE_VERIFY(layouter.isHandleValid(layout1));
    layouter.cleanNodes(Containers::arrayView({UnsignedShort{0xfef}}));
    CORRADE_COMPARE(layouter.state(), LayouterStates{});
    CORRADE_VERIFY(!layouter.isHandleValid(layout1));
}

}}}}

CORRADE_TEST_MAIN(Magnum::Whee::Test::AbstractLayouterTest)
