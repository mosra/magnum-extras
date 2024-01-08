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
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/StridedBitArrayView.h>
#include <Corrade/Containers/StringStl.h> /** @todo remove once Debug is stream-free */
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Container.h>
#include <Corrade/TestSuite/Compare/String.h>
#include <Corrade/Utility/DebugStl.h>
#include <Magnum/Math/Vector2.h>

#include "Magnum/Whee/AbstractLayer.h"
#include "Magnum/Whee/AbstractRenderer.h"
#include "Magnum/Whee/Event.h"
#include "Magnum/Whee/Handle.h"

namespace Magnum { namespace Whee { namespace Test { namespace {

struct AbstractLayerTest: TestSuite::Tester {
    explicit AbstractLayerTest();

    void debugFeature();
    void debugFeatures();
    void debugFeaturesSupersets();
    void debugState();
    void debugStates();
    void debugStatesSupersets();

    void construct();
    void constructInvalidHandle();
    void constructCopy();
    void constructMove();

    void createRemove();
    void createRemoveHandleRecycle();
    void createRemoveHandleDisable();
    void createNoHandlesLeft();
    void createAttached();
    void removeInvalid();
    void attach();
    void attachInvalid();

    void setSize();
    void setSizeZero();
    void setSizeNotSupported();
    void setSizeNotImplemented();

    void cleanNodes();
    void cleanNodesEmpty();
    void cleanNodesNotImplemented();

    void update();
    void updateEmpty();
    void updateNotImplemented();
    void updateInvalidSizes();

    void state();

    void composite();
    void compositeEmpty();
    void compositeNotSupported();
    void compositeNotImplemented();
    void compositeInvalidSizes();

    void draw();
    void drawEmpty();
    void drawNotSupported();
    void drawNotImplemented();
    void drawInvalidSizes();

    void pointerEvent();
    void pointerEventNotSupported();
    void pointerEventNotImplemented();
    void pointerEventOutOfRange();
    void pointerEventAlreadyAccepted();
};

AbstractLayerTest::AbstractLayerTest() {
    addTests({&AbstractLayerTest::debugFeature,
              &AbstractLayerTest::debugFeatures,
              &AbstractLayerTest::debugFeaturesSupersets,
              &AbstractLayerTest::debugState,
              &AbstractLayerTest::debugStates,
              &AbstractLayerTest::debugStatesSupersets,

              &AbstractLayerTest::construct,
              &AbstractLayerTest::constructInvalidHandle,
              &AbstractLayerTest::constructCopy,
              &AbstractLayerTest::constructMove,

              &AbstractLayerTest::createRemove,
              &AbstractLayerTest::createRemoveHandleRecycle,
              &AbstractLayerTest::createRemoveHandleDisable,
              &AbstractLayerTest::createNoHandlesLeft,
              &AbstractLayerTest::createAttached,
              &AbstractLayerTest::removeInvalid,
              &AbstractLayerTest::attach,
              &AbstractLayerTest::attachInvalid,

              &AbstractLayerTest::setSize,
              &AbstractLayerTest::setSizeZero,
              &AbstractLayerTest::setSizeNotSupported,
              &AbstractLayerTest::setSizeNotImplemented,

              &AbstractLayerTest::cleanNodes,
              &AbstractLayerTest::cleanNodesEmpty,
              &AbstractLayerTest::cleanNodesNotImplemented,

              &AbstractLayerTest::update,
              &AbstractLayerTest::updateEmpty,
              &AbstractLayerTest::updateNotImplemented,
              &AbstractLayerTest::updateInvalidSizes,

              &AbstractLayerTest::state,

              &AbstractLayerTest::composite,
              &AbstractLayerTest::compositeEmpty,
              &AbstractLayerTest::compositeNotSupported,
              &AbstractLayerTest::compositeNotImplemented,
              &AbstractLayerTest::compositeInvalidSizes,

              &AbstractLayerTest::draw,
              &AbstractLayerTest::drawEmpty,
              &AbstractLayerTest::drawNotSupported,
              &AbstractLayerTest::drawNotImplemented,
              &AbstractLayerTest::drawInvalidSizes,

              &AbstractLayerTest::pointerEvent,
              &AbstractLayerTest::pointerEventNotSupported,
              &AbstractLayerTest::pointerEventNotImplemented,
              &AbstractLayerTest::pointerEventOutOfRange,
              &AbstractLayerTest::pointerEventAlreadyAccepted});
}

void AbstractLayerTest::debugFeature() {
    std::ostringstream out;
    Debug{&out} << LayerFeature::Draw << LayerFeature(0xbe);
    CORRADE_COMPARE(out.str(), "Whee::LayerFeature::Draw Whee::LayerFeature(0xbe)\n");
}

void AbstractLayerTest::debugFeatures() {
    std::ostringstream out;
    Debug{&out} << (LayerFeature::Draw|LayerFeature(0xe0)) << LayerFeatures{};
    CORRADE_COMPARE(out.str(), "Whee::LayerFeature::Draw|Whee::LayerFeature(0xe0) Whee::LayerFeatures{}\n");
}

void AbstractLayerTest::debugFeaturesSupersets() {
    /* DrawUsesBlending and DrawUsesScissor are both a superset of Draw, so
       only one should be printed, but if there are both then both should be */
    {
        std::ostringstream out;
        Debug{&out} << (LayerFeature::DrawUsesBlending|LayerFeature::Draw);
        CORRADE_COMPARE(out.str(), "Whee::LayerFeature::DrawUsesBlending\n");
    } {
        std::ostringstream out;
        Debug{&out} << (LayerFeature::DrawUsesScissor|LayerFeature::Draw);
        CORRADE_COMPARE(out.str(), "Whee::LayerFeature::DrawUsesScissor\n");
    } {
        std::ostringstream out;
        Debug{&out} << (LayerFeature::DrawUsesBlending|LayerFeature::DrawUsesScissor);
        CORRADE_COMPARE(out.str(), "Whee::LayerFeature::DrawUsesBlending|Whee::LayerFeature::DrawUsesScissor\n");

    /* Composite is a superset of Draw, so only one should be printed */
    } {
        std::ostringstream out;
        Debug{&out} << (LayerFeature::Composite|LayerFeature::Draw);
        CORRADE_COMPARE(out.str(), "Whee::LayerFeature::Composite\n");
    }
}

void AbstractLayerTest::debugState() {
    std::ostringstream out;
    Debug{&out} << LayerState::NeedsAttachmentUpdate << LayerState(0xbe);
    CORRADE_COMPARE(out.str(), "Whee::LayerState::NeedsAttachmentUpdate Whee::LayerState(0xbe)\n");
}

void AbstractLayerTest::debugStates() {
    std::ostringstream out;
    Debug{&out} << (LayerState::NeedsUpdate|LayerState(0xe0)) << LayerStates{};
    CORRADE_COMPARE(out.str(), "Whee::LayerState::NeedsUpdate|Whee::LayerState(0xe0) Whee::LayerStates{}\n");
}

void AbstractLayerTest::debugStatesSupersets() {
    /* NeedsAttachmentUpdate is a superset of NeedsUpdate, so only one should
       be printed */
    {
        std::ostringstream out;
        Debug{&out} << (LayerState::NeedsUpdate|LayerState::NeedsAttachmentUpdate);
        CORRADE_COMPARE(out.str(), "Whee::LayerState::NeedsAttachmentUpdate\n");
    }
}

void AbstractLayerTest::construct() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return LayerFeatures(0xe0); }
    } layer{layerHandle(0xab, 0x12)};

    CORRADE_COMPARE(layer.handle(), layerHandle(0xab, 0x12));
    CORRADE_COMPARE(layer.features(), LayerFeatures(0xe0));
    CORRADE_COMPARE(layer.state(), LayerStates{});
    CORRADE_COMPARE(layer.capacity(), 0);
    CORRADE_COMPARE(layer.usedCount(), 0);
    CORRADE_VERIFY(!layer.isHandleValid(LayerDataHandle::Null));
    CORRADE_VERIFY(!layer.isHandleValid(DataHandle::Null));
}

void AbstractLayerTest::constructInvalidHandle() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    };

    std::ostringstream out;
    Error redirectError{&out};
    Layer{LayerHandle::Null};
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractLayer: handle is null\n");
}

void AbstractLayerTest::constructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<AbstractLayer>{});
    CORRADE_VERIFY(!std::is_copy_assignable<AbstractLayer>{});
}

void AbstractLayerTest::constructMove() {
    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    };

    /* The class has an internal state struct containing everything, so it's
       not needed to test each and every property */
    Layer a{layerHandle(0xab, 0x12)};

    Layer b{Utility::move(a)};
    CORRADE_COMPARE(b.handle(), layerHandle(0xab, 0x12));

    Layer c{layerHandle(0xcd, 0x34)};
    c = Utility::move(b);
    CORRADE_COMPARE(c.handle(), layerHandle(0xab, 0x12));

    CORRADE_VERIFY(std::is_nothrow_move_constructible<Layer>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<Layer>::value);
}

void AbstractLayerTest::createRemove() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;
        using AbstractLayer::remove;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0xab, 0x12)};

    /* The node argument is tested in createAttached() below */

    DataHandle first = layer.create();
    CORRADE_COMPARE(first, dataHandle(layer.handle(), 0, 1));
    CORRADE_VERIFY(layer.isHandleValid(first));
    CORRADE_COMPARE(layer.state(), LayerStates{});
    CORRADE_COMPARE(layer.capacity(), 1);
    CORRADE_COMPARE(layer.usedCount(), 1);
    CORRADE_COMPARE(layer.node(first), NodeHandle::Null);

    DataHandle second = layer.create();
    CORRADE_COMPARE(second, dataHandle(layer.handle(), 1, 1));
    CORRADE_VERIFY(layer.isHandleValid(second));
    CORRADE_COMPARE(layer.state(), LayerStates{});
    CORRADE_COMPARE(layer.capacity(), 2);
    CORRADE_COMPARE(layer.usedCount(), 2);
    CORRADE_COMPARE(layer.node(second), NodeHandle::Null);

    layer.remove(first);
    CORRADE_VERIFY(!layer.isHandleValid(first));
    CORRADE_VERIFY(layer.isHandleValid(second));
    CORRADE_COMPARE(layer.state(), LayerStates{});
    CORRADE_COMPARE(layer.capacity(), 2);
    CORRADE_COMPARE(layer.usedCount(), 1);

    /* Using also the LayouterDataHandle overload */
    layer.remove(dataHandleData(second));
    CORRADE_VERIFY(!layer.isHandleValid(first));
    CORRADE_VERIFY(!layer.isHandleValid(second));
    CORRADE_COMPARE(layer.state(), LayerStates{});
    CORRADE_COMPARE(layer.capacity(), 2);
    CORRADE_COMPARE(layer.usedCount(), 0);
}

void AbstractLayerTest::createRemoveHandleRecycle() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;
        using AbstractLayer::remove;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0xab, 0x12)};
    DataHandle first = layer.create();
    DataHandle second = layer.create();
    DataHandle third = layer.create();
    DataHandle fourth = layer.create();
    CORRADE_COMPARE(first, dataHandle(layer.handle(), 0, 1));
    CORRADE_COMPARE(second, dataHandle(layer.handle(), 1, 1));
    CORRADE_COMPARE(third, dataHandle(layer.handle(), 2, 1));
    CORRADE_COMPARE(fourth, dataHandle(layer.handle(), 3, 1));
    CORRADE_VERIFY(layer.isHandleValid(first));
    CORRADE_VERIFY(layer.isHandleValid(second));
    CORRADE_VERIFY(layer.isHandleValid(third));
    CORRADE_VERIFY(layer.isHandleValid(fourth));
    CORRADE_COMPARE(layer.capacity(), 4);
    CORRADE_COMPARE(layer.usedCount(), 4);
    CORRADE_COMPARE(layer.node(first), NodeHandle::Null);
    CORRADE_COMPARE(layer.node(second), NodeHandle::Null);
    CORRADE_COMPARE(layer.node(third), NodeHandle::Null);
    CORRADE_COMPARE(layer.node(fourth), NodeHandle::Null);

    /* Attach some handles to an arbitrary node to populate their internals */
    layer.attach(first, NodeHandle(0xabc12345));
    layer.attach(third, NodeHandle(0x123abcde));
    CORRADE_COMPARE(layer.node(first), NodeHandle(0xabc12345));
    CORRADE_COMPARE(layer.node(third), NodeHandle(0x123abcde));
    CORRADE_COMPARE_AS(layer.nodes(), Containers::arrayView({
        NodeHandle(0xabc12345),
        NodeHandle::Null,
        NodeHandle(0x123abcde),
        NodeHandle::Null
    }), TestSuite::Compare::Container);

    /* Remove three out of the four in an arbitrary order */
    layer.remove(fourth);
    layer.remove(first);
    layer.remove(third);
    CORRADE_VERIFY(!layer.isHandleValid(first));
    CORRADE_VERIFY(layer.isHandleValid(second));
    CORRADE_VERIFY(!layer.isHandleValid(third));
    CORRADE_VERIFY(!layer.isHandleValid(fourth));
    CORRADE_COMPARE(layer.capacity(), 4);
    CORRADE_COMPARE(layer.usedCount(), 1);
    CORRADE_COMPARE(layer.node(second), NodeHandle::Null);

    /* Internally all attachments should be set to a null handle after
       deletion */
    CORRADE_COMPARE_AS(layer.nodes(), Containers::arrayView({
        NodeHandle::Null,
        NodeHandle::Null,
        NodeHandle::Null,
        NodeHandle::Null
    }), TestSuite::Compare::Container);

    /* Allocating new handles should recycle the handles in the order they were
       removed (oldest first). Their properties should be cleared. */
    DataHandle fourth2 = layer.create();
    DataHandle first2 = layer.create();
    DataHandle third2 = layer.create();
    CORRADE_COMPARE(first2, dataHandle(layer.handle(), 0, 2));
    CORRADE_COMPARE(third2, dataHandle(layer.handle(), 2, 2));
    CORRADE_COMPARE(fourth2, dataHandle(layer.handle(), 3, 2));
    CORRADE_COMPARE(layer.capacity(), 4);
    CORRADE_COMPARE(layer.usedCount(), 4);
    CORRADE_COMPARE(layer.node(first2), NodeHandle::Null);
    CORRADE_COMPARE(layer.node(second), NodeHandle::Null);
    CORRADE_COMPARE(layer.node(third2), NodeHandle::Null);
    CORRADE_COMPARE(layer.node(fourth2), NodeHandle::Null);

    /* Old handles shouldn't get valid again */
    CORRADE_VERIFY(!layer.isHandleValid(first));
    CORRADE_VERIFY(layer.isHandleValid(first2));
    CORRADE_VERIFY(!layer.isHandleValid(third));
    CORRADE_VERIFY(layer.isHandleValid(third2));
    CORRADE_VERIFY(!layer.isHandleValid(fourth));
    CORRADE_VERIFY(layer.isHandleValid(fourth2));

    /* Removing a single handle and creating a new one directly reuses it if
       there's just one in the free list */
    layer.remove(third2);
    DataHandle third3 = layer.create();
    CORRADE_COMPARE(third3, dataHandle(layer.handle(), 2, 3));
    CORRADE_VERIFY(!layer.isHandleValid(third));
    CORRADE_VERIFY(!layer.isHandleValid(third2));
    CORRADE_VERIFY(layer.isHandleValid(third3));
    CORRADE_COMPARE(layer.capacity(), 4);
    CORRADE_COMPARE(layer.usedCount(), 4);
    CORRADE_COMPARE(layer.node(third3), NodeHandle::Null);

    /* Allocating a new handle with the free list empty will grow it */
    DataHandle fifth = layer.create();
    CORRADE_COMPARE(fifth, dataHandle(layer.handle(), 4, 1));
    CORRADE_VERIFY(layer.isHandleValid(fifth));
    CORRADE_COMPARE(layer.capacity(), 5);
    CORRADE_COMPARE(layer.usedCount(), 5);
    CORRADE_COMPARE(layer.node(fifth), NodeHandle::Null);
}

void AbstractLayerTest::createRemoveHandleDisable() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;
        using AbstractLayer::remove;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0xab, 0x12)};

    DataHandle first = layer.create();
    CORRADE_COMPARE(first, dataHandle(layer.handle(), 0, 1));

    for(std::size_t i = 0; i != (1 << Implementation::LayerDataHandleGenerationBits) - 1; ++i) {
        DataHandle second = layer.create();
        CORRADE_COMPARE(second, dataHandle(layer.handle(), 1, 1 + i));
        layer.remove(second);
    }

    /* The generation for the second slot is exhausted so the handle is not
       recycled */
    CORRADE_COMPARE(layer.capacity(), 2);
    CORRADE_COMPARE(layer.usedCount(), 2);

    /* It shouldn't think a handle from the second slot with generation 0 is
       valid */
    CORRADE_VERIFY(!layer.isHandleValid(dataHandle(layer.handle(), 1, 0)));

    /* There's nowhere to create a new handle from so the capacity is grown */
    DataHandle third = layer.create();
    CORRADE_COMPARE(third, dataHandle(layer.handle(), 2, 1));
    CORRADE_COMPARE(layer.capacity(), 3);
    CORRADE_COMPARE(layer.usedCount(), 3);
}

void AbstractLayerTest::createNoHandlesLeft() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0, 1)};

    for(std::size_t i = 0; i != 1 << Implementation::LayerDataHandleIdBits; ++i)
        layer.create();

    CORRADE_COMPARE(layer.capacity(), 1 << Implementation::LayerDataHandleIdBits);
    CORRADE_COMPARE(layer.usedCount(), 1 << Implementation::LayerDataHandleIdBits);

    std::ostringstream out;
    Error redirectError{&out};
    layer.create();
    /* Number is hardcoded in the expected message but not elsewhere in order
       to give a heads-up when modifying the handle ID bit count */
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractLayer::create(): can only have at most 1048576 data\n");
}

void AbstractLayerTest::createAttached() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0xab, 0x12)};

    NodeHandle node = nodeHandle(9872, 0xbeb);

    /* Explicitly passing a null handle should work too, and cause no state
       change */
    DataHandle notAttached = layer.create(NodeHandle::Null);
    CORRADE_COMPARE(layer.node(notAttached), NodeHandle::Null);
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Passing a non-null handle causes a state change */
    DataHandle attached = layer.create(node);
    CORRADE_COMPARE(layer.node(attached), node);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsAttachmentUpdate);

    /* The attachment should be reflected in the view as well */
    CORRADE_COMPARE_AS(layer.nodes(), Containers::arrayView({
        NodeHandle::Null,
        node
    }), TestSuite::Compare::Container);
}

void AbstractLayerTest::removeInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;
        using AbstractLayer::remove;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0, 1)};

    DataHandle handle = layer.create();

    std::ostringstream out;
    Error redirectError{&out};
    layer.remove(DataHandle::Null);
    /* Valid layer, invalid data */
    layer.remove(dataHandle(layer.handle(), LayerDataHandle(0x123abcde)));
    /* Invalid layer, valid data */
    layer.remove(dataHandle(LayerHandle::Null, dataHandleData(handle)));
    /* LayerDataHandle directly */
    layer.remove(LayerDataHandle(0x123abcde));
    CORRADE_COMPARE_AS(out.str(),
        "Whee::AbstractLayer::remove(): invalid handle Whee::DataHandle::Null\n"
        "Whee::AbstractLayer::remove(): invalid handle Whee::DataHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Whee::AbstractLayer::remove(): invalid handle Whee::DataHandle(Null, {0x0, 0x1})\n"
        "Whee::AbstractLayer::remove(): invalid handle Whee::LayerDataHandle(0xabcde, 0x123)\n",
        TestSuite::Compare::String);
}

void AbstractLayerTest::attach() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0xab, 0x12)};

    DataHandle first = layer.create();
    DataHandle second = layer.create();
    CORRADE_COMPARE(layer.node(first), NodeHandle::Null);
    CORRADE_COMPARE(layer.node(second), NodeHandle::Null);

    NodeHandle nodeFirst = nodeHandle(2865, 0xcec);
    NodeHandle nodeSecond = nodeHandle(9872, 0xbeb);
    NodeHandle nodeThird = nodeHandle(12, 0x888);

    layer.attach(first, nodeSecond);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsAttachmentUpdate);
    CORRADE_COMPARE(layer.node(first), nodeSecond);

    /* The attachment should be reflected in the view as well */
    CORRADE_COMPARE_AS(layer.nodes(), Containers::arrayView({
        nodeSecond,
        NodeHandle::Null
    }), TestSuite::Compare::Container);

    /* Calling with the layer-specific handles should work too */
    layer.attach(dataHandleData(second), nodeFirst);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsAttachmentUpdate);
    CORRADE_COMPARE(layer.node(dataHandleData(second)), nodeFirst);

    /* Attaching to a new node should overwrite the previous */
    layer.attach(first, nodeThird);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsAttachmentUpdate);
    CORRADE_COMPARE(layer.node(first), nodeThird);

    /* Attaching two data to the same node should work too */
    layer.attach(second, nodeThird);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsAttachmentUpdate);
    CORRADE_COMPARE(layer.node(first), nodeThird);
    CORRADE_COMPARE(layer.node(second), nodeThird);

    /* Detaching as well */
    layer.attach(first, NodeHandle::Null);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsAttachmentUpdate);
    CORRADE_COMPARE(layer.node(first), NodeHandle::Null);
    CORRADE_COMPARE(layer.node(second), nodeThird);

    /* The cleared attachment should be reflected in the view as well */
    CORRADE_COMPARE_AS(layer.nodes(), Containers::arrayView({
        NodeHandle::Null,
        nodeThird
    }), TestSuite::Compare::Container);
}

void AbstractLayerTest::attachInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0xab, 0x12)};

    DataHandle handle = layer.create();

    std::ostringstream out;
    Error redirectError{&out};
    layer.attach(DataHandle::Null, nodeHandle(2865, 0xcec));
    layer.node(DataHandle::Null);
    /* Valid layer, invalid data */
    layer.attach(dataHandle(layer.handle(), LayerDataHandle(0x123abcde)), nodeHandle(2865, 0xcec));
    layer.node(dataHandle(layer.handle(), LayerDataHandle(0x123abcde)));
    /* Invalid layer, valid data */
    layer.attach(dataHandle(LayerHandle::Null, dataHandleData(handle)), nodeHandle(2865, 0xcec));
    layer.node(dataHandle(LayerHandle::Null, dataHandleData(handle)));
    /* LayerDataHandle directly */
    layer.attach(LayerDataHandle(0x123abcde), nodeHandle(2865, 0xcec));
    layer.node(LayerDataHandle(0x123abcde));
    CORRADE_COMPARE_AS(out.str(),
        "Whee::AbstractLayer::attach(): invalid handle Whee::DataHandle::Null\n"
        "Whee::AbstractLayer::node(): invalid handle Whee::DataHandle::Null\n"
        "Whee::AbstractLayer::attach(): invalid handle Whee::DataHandle({0xab, 0x12}, {0xabcde, 0x123})\n"
        "Whee::AbstractLayer::node(): invalid handle Whee::DataHandle({0xab, 0x12}, {0xabcde, 0x123})\n"
        "Whee::AbstractLayer::attach(): invalid handle Whee::DataHandle(Null, {0x0, 0x1})\n"
        "Whee::AbstractLayer::node(): invalid handle Whee::DataHandle(Null, {0x0, 0x1})\n"
        "Whee::AbstractLayer::attach(): invalid handle Whee::LayerDataHandle(0xabcde, 0x123)\n"
        "Whee::AbstractLayer::node(): invalid handle Whee::LayerDataHandle(0xabcde, 0x123)\n",
        TestSuite::Compare::String);
}

void AbstractLayerTest::setSize() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Draw;
        }

        void doSetSize(const Vector2& size, const Vector2i& framebufferSize) override {
            ++called;
            CORRADE_COMPARE(size, (Vector2{1.0f, 2.0f}));
            CORRADE_COMPARE(framebufferSize, (Vector2i{3, 4}));
        }

        Int called = 0;
    } layer{layerHandle(0, 1)};

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    layer.setSize({1.0f, 2.0f}, {3, 4});
    CORRADE_COMPARE(layer.called, 1);
}

void AbstractLayerTest::setSizeZero() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Draw;
        }
    } layer{layerHandle(0, 1)};

    std::ostringstream out;
    Error redirectError{&out};
    layer.setSize({0.0f, 1.0f}, {2, 3});
    layer.setSize({1.0f, 0.0f}, {2, 3});
    layer.setSize({1.0f, 2.0f}, {0, 3});
    layer.setSize({1.0f, 2.0f}, {3, 0});
    CORRADE_COMPARE_AS(out.str(),
        "Whee::AbstractLayer::setSize(): expected non-zero sizes, got Vector(0, 1) and Vector(2, 3)\n"
        "Whee::AbstractLayer::setSize(): expected non-zero sizes, got Vector(1, 0) and Vector(2, 3)\n"
        "Whee::AbstractLayer::setSize(): expected non-zero sizes, got Vector(1, 2) and Vector(0, 3)\n"
        "Whee::AbstractLayer::setSize(): expected non-zero sizes, got Vector(1, 2) and Vector(3, 0)\n",
        TestSuite::Compare::String);
}

void AbstractLayerTest::setSizeNotSupported() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0, 1)};

    std::ostringstream out;
    Error redirectError{&out};
    layer.setSize({}, {});
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractLayer::setSize(): Whee::LayerFeature::Draw not supported\n");
}

void AbstractLayerTest::setSizeNotImplemented() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Draw;
        }
    } layer{layerHandle(0, 1)};

    layer.setSize({1.0f, 2.0f}, {3, 4});

    /* Shouldn't crash or anything */
    CORRADE_VERIFY(true);
}

void AbstractLayerTest::cleanNodes() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;
        using AbstractLayer::remove;

        LayerFeatures doFeatures() const override { return {}; }

        void doClean(Containers::BitArrayView dataIdsToRemove) override {
            ++called;
            CORRADE_COMPARE_AS(dataIdsToRemove, Containers::stridedArrayView({
                true, false, false, true, false, true, false
            }).sliceBit(0), TestSuite::Compare::Container);
        }

        Int called = 0;
    } layer{layerHandle(0, 1)};

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    NodeHandle nodeFirst = nodeHandle(0, 0xcec);
    NodeHandle nodeSecond = nodeHandle(1, 0xded);
    NodeHandle nodeFourth = nodeHandle(3, 0xaba);
    NodeHandle nodeEighth = nodeHandle(7, 0xfef);

    /* Create seven data to match the seven bits. Attach them to random
       handles, leave one unassigned, attach two data to one node. */
    DataHandle first = layer.create(nodeEighth);
    DataHandle second = layer.create();
    DataHandle third = layer.create(nodeSecond);
    DataHandle fourth = layer.create(nodeFirst);
    DataHandle fifth = layer.create(nodeFourth);
    DataHandle sixth = layer.create(nodeFirst);
    DataHandle seventh = layer.create(nodeFourth);

    /* Remove two of them */
    layer.remove(third);
    layer.remove(seventh);

    /* Call cleanNodes() with updated generation counters */
    layer.cleanNodes(Containers::arrayView({
        /* First node generation gets different, affecting fourth and sixth
           data */
        UnsignedShort(nodeHandleGeneration(nodeFirst) + 1),
        /* Second node generation gets different but since the third data is
           already removed it doesn't affect anything */
        UnsignedShort(nodeHandleGeneration(nodeSecond) - 1),
        /* Third node has no attachments so it can be arbitrary */
        UnsignedShort{0xbeb},
        /* Fourth node stays the same generation so the fifth data stay.
           Seventh data are already removed so they aren't set for deletion
           either. */
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
    CORRADE_COMPARE(layer.called, 1);

    /* Only the second and fifth data should stay afterwards */
    CORRADE_VERIFY(!layer.isHandleValid(first));
    CORRADE_VERIFY(layer.isHandleValid(second));
    CORRADE_VERIFY(!layer.isHandleValid(third));
    CORRADE_VERIFY(!layer.isHandleValid(fourth));
    CORRADE_VERIFY(layer.isHandleValid(fifth));
    CORRADE_VERIFY(!layer.isHandleValid(sixth));
    CORRADE_VERIFY(!layer.isHandleValid(seventh));
}

void AbstractLayerTest::cleanNodesEmpty() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }

        void doClean(Containers::BitArrayView) override {
            ++called;
        }

        Int called = 0;
    } layer{layerHandle(0, 1)};

    /* It should call the implementation even with empty contents */
    layer.cleanNodes({});
    CORRADE_COMPARE(layer.called, 1);
}

void AbstractLayerTest::cleanNodesNotImplemented() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0, 1)};

    layer.cleanNodes({});

    /* Shouldn't crash or anything */
    CORRADE_VERIFY(true);
}

void AbstractLayerTest::update() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }

        void doUpdate(const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes) override {
            ++called;
            CORRADE_COMPARE_AS(dataIds, Containers::arrayView({
                0xabcdeu,
                0x45678u
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(clipRectIds, Containers::arrayView({
                /* These should be small enough to index into clipRectOffsets
                   and clipRectSizes but nobody cares here */
                3u,
                16u,
                27u,
                2u
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(clipRectDataCounts, Containers::arrayView({
                /* The sum should be equal to dataIds.size(), yes, nobody cares
                   here */
                265u,
                1u,
                13u,
                7u
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(nodeOffsets, Containers::arrayView<Vector2>({
                {1.0f, 2.0f},
                {3.0f, 4.0f},
                {5.0f, 6.0f}
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(nodeSizes, Containers::arrayView<Vector2>({
                {0.1f, 0.2f},
                {0.3f, 0.4f},
                {0.5f, 0.6f}
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(nodesEnabled, Containers::stridedArrayView({
                true,
                false,
                true
            }).sliceBit(0), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(clipRectOffsets, Containers::arrayView<Vector2>({
                {6.5f, 7.5f},
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(clipRectSizes, Containers::arrayView<Vector2>({
                {8.5f, 9.5f},
            }), TestSuite::Compare::Container);
        }

        Int called = 0;
    } layer{layerHandle(0, 1)};

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    UnsignedByte nodesEnabled[1]{0x5};

    layer.update(
        Containers::arrayView({
            0xabcdeu,
            0x45678u,
        }),
        Containers::arrayView({
            3u,
            16u,
            27u,
            2u
        }),
        Containers::arrayView({
            265u,
            1u,
            13u,
            7u
        }),
        Containers::arrayView<Vector2>({
            {1.0f, 2.0f},
            {3.0f, 4.0f},
            {5.0f, 6.0f}
        }),
        Containers::arrayView<Vector2>({
            {0.1f, 0.2f},
            {0.3f, 0.4f},
            {0.5f, 0.6f}
        }),
        Containers::BitArrayView{nodesEnabled, 0, 3},
        Containers::arrayView<Vector2>({
            {6.5f, 7.5f},
        }),
        Containers::arrayView<Vector2>({
            {8.5f, 9.5f},
        })
    );
    CORRADE_COMPARE(layer.called, 1);
}

void AbstractLayerTest::updateEmpty() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }

        void doUpdate(const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, Containers::BitArrayView, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&) override {
            ++called;
        }

        Int called = 0;
    } layer{layerHandle(0, 1)};

    /* It should call the implementation even with empty contents */
    layer.update({}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.called, 1);
}

void AbstractLayerTest::updateNotImplemented() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0, 1)};

    UnsignedByte nodesEnabled[1]{};

    layer.update(
        Containers::arrayView({
            0u,
            0u
        }),
        Containers::arrayView({
            0u,
            0u,
            0u,
            0u
        }),
        Containers::arrayView({
            0u,
            0u,
            0u,
            0u
        }),
        Containers::arrayView<Vector2>({
            {},
            {},
            {}
        }),
        Containers::arrayView<Vector2>({
            {},
            {},
            {}
        }),
        Containers::BitArrayView{nodesEnabled, 0, 3},
        Containers::arrayView<Vector2>({
            {}
        }),
        Containers::arrayView<Vector2>({
            {}
        })
    );

    /* Shouldn't crash or anything */
    CORRADE_VERIFY(true);
}

void AbstractLayerTest::updateInvalidSizes() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0, 1)};

    UnsignedByte nodesEnabled[1]{};

    std::ostringstream out;
    Error redirectError{&out};
    layer.update(
        {},
        Containers::arrayView({
            0u,
            0u,
            0u
        }),
        Containers::arrayView({
            0u,
            0u
        }),
        {}, {}, {},
        {}, {}
    );
    layer.update(
        {}, {}, {},
        Containers::arrayView<Vector2>({
            {},
            {}
        }),
        Containers::arrayView<Vector2>({
            {},
            {},
            {}
        }),
        Containers::BitArrayView{nodesEnabled, 0, 2},
        {}, {}
    );
    layer.update(
        {}, {}, {},
        Containers::arrayView<Vector2>({
            {},
            {}
        }),
        Containers::arrayView<Vector2>({
            {},
            {}
        }),
        Containers::BitArrayView{nodesEnabled, 0, 3},
        {}, {}
    );
    layer.update(
        {}, {}, {},
        {}, {}, {},
        Containers::arrayView<Vector2>({
            {},
            {},
            {}
        }),
        Containers::arrayView<Vector2>({
            {},
            {}
        })
    );
    CORRADE_COMPARE_AS(out.str(),
        "Whee::AbstractLayer::update(): expected clip rect ID and data count views to have the same size but got 3 and 2\n"
        "Whee::AbstractLayer::update(): expected node offset, size and enabled views to have the same size but got 2, 3 and 2\n"
        "Whee::AbstractLayer::update(): expected node offset, size and enabled views to have the same size but got 2, 2 and 3\n"
        "Whee::AbstractLayer::update(): expected clip rect offset and size views to have the same size but got 3 and 2\n",
        TestSuite::Compare::String);
}

void AbstractLayerTest::state() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;
        using AbstractLayer::remove;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0xab, 0x12)};

    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Creating a data adds no state flag as the data don't show up anywhere
       implicitly */
    DataHandle data1 = layer.create();
    DataHandle data2 = layer.create();
    DataHandle data3 = layer.create();
    DataHandle data4 = layer.create();
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* No other way to trigger this flag */
    layer.setNeedsUpdate();
    CORRADE_COMPARE(layer.state(), LayerState::NeedsUpdate);

    /* update() then resets it */
    layer.update({}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Attaching to a node sets a state flag */
    layer.attach(data2, nodeHandle(0, 0x123));
    layer.attach(data3, nodeHandle(0, 0x123));
    layer.attach(data4, nodeHandle(0, 0x123));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsAttachmentUpdate);

    /* update() then resets it */
    layer.update({}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Detaching sets a state flag as well (even if the data originally weren't
       attached either). Also testing the other overload here. */
    layer.attach(dataHandleData(data1), NodeHandle::Null);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsAttachmentUpdate);

    /* update() then resets it */
    layer.update({}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* remove() adds nothing on its own */
    layer.remove(data1);
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* remove() adds NeedsAttachmentUpdate if the data were attached */
    layer.remove(data2);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsAttachmentUpdate);

    /* update() then resets one */
    layer.update({}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Testing the other overload */
    layer.remove(dataHandleData(data3));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsAttachmentUpdate);

    /* cleanNodes() (no-op in this case) doesn't remove any flags on its own */
    CORRADE_COMPARE(layer.usedCount(), 1);
    layer.cleanNodes(Containers::arrayView({UnsignedShort{0x123}}));
    CORRADE_COMPARE(layer.usedCount(), 1);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsAttachmentUpdate);

    /* Only update() does */
    layer.update({}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* cleanNodes() that removes a data doesn't set any flags either */
    CORRADE_VERIFY(layer.isHandleValid(data4));
    layer.cleanNodes(Containers::arrayView({UnsignedShort{0xfef}}));
    CORRADE_COMPARE(layer.state(), LayerStates{});
    CORRADE_VERIFY(!layer.isHandleValid(data4));
}

void AbstractLayerTest::composite() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Composite;
        }

        void doComposite(AbstractRenderer& renderer, const Containers::StridedArrayView1D<const Vector2>& rectOffsets, const Containers::StridedArrayView1D<const Vector2>& rectSizes) override {
            ++called;
            CORRADE_COMPARE(renderer.framebufferSize(), (Vector2i{12, 34}));
            CORRADE_COMPARE_AS(rectOffsets, Containers::arrayView<Vector2>({
                {1.0f, 2.0f},
                {3.0f, 4.0f}
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(rectSizes, Containers::arrayView<Vector2>({
                {0.1f, 0.2f},
                {0.3f, 0.4f}
            }), TestSuite::Compare::Container);
        }

        Int called = 0;
    } layer{layerHandle(0, 1)};

    struct: AbstractRenderer {
        RendererFeatures doFeatures() const override { return {}; }
        void doSetupFramebuffers(const Vector2i&) override {}
        void doTransition(RendererTargetState, RendererTargetState, RendererDrawStates, RendererDrawStates) override {}
    } renderer;
    renderer.setupFramebuffers({12, 34});

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    layer.composite(renderer,
        Containers::arrayView<Vector2>({
            {1.0f, 2.0f},
            {3.0f, 4.0f}
        }),
        Containers::arrayView<Vector2>({
                {0.1f, 0.2f},
                {0.3f, 0.4f}
        })
    );
    CORRADE_COMPARE(layer.called, 1);
}

void AbstractLayerTest::compositeEmpty() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Composite;
        }

        void doComposite(AbstractRenderer&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&) override {
            ++called;
        }

        Int called = 0;
    } layer{layerHandle(0, 1)};

    struct: AbstractRenderer {
        RendererFeatures doFeatures() const override { return {}; }
        void doSetupFramebuffers(const Vector2i&) override {}
        void doTransition(RendererTargetState, RendererTargetState, RendererDrawStates, RendererDrawStates) override {}
    } renderer;

    /* It should call the implementation even with empty contents */
    layer.composite(renderer, {}, {});
    CORRADE_COMPARE(layer.called, 1);
}

void AbstractLayerTest::compositeNotSupported() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0, 1)};

    struct: AbstractRenderer {
        RendererFeatures doFeatures() const override { return {}; }
        void doSetupFramebuffers(const Vector2i&) override {}
        void doTransition(RendererTargetState, RendererTargetState, RendererDrawStates, RendererDrawStates) override {}
    } renderer;

    std::ostringstream out;
    Error redirectError{&out};
    layer.composite(renderer, {}, {});
    CORRADE_COMPARE(out.str(), "Whee::AbstractLayer::composite(): feature not supported\n");
}

void AbstractLayerTest::compositeNotImplemented() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Composite;
        }
    } layer{layerHandle(0, 1)};

    struct: AbstractRenderer {
        RendererFeatures doFeatures() const override { return {}; }
        void doSetupFramebuffers(const Vector2i&) override {}
        void doTransition(RendererTargetState, RendererTargetState, RendererDrawStates, RendererDrawStates) override {}
    } renderer;

    std::ostringstream out;
    Error redirectError{&out};
    layer.composite(renderer, {}, {});
    CORRADE_COMPARE(out.str(), "Whee::AbstractLayer::composite(): feature advertised but not implemented\n");
}

void AbstractLayerTest::compositeInvalidSizes() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Composite;
        }
    } layer{layerHandle(0, 1)};

    struct: AbstractRenderer {
        RendererFeatures doFeatures() const override { return {}; }
        void doSetupFramebuffers(const Vector2i&) override {}
        void doTransition(RendererTargetState, RendererTargetState, RendererDrawStates, RendererDrawStates) override {}
    } renderer;

    std::ostringstream out;
    Error redirectError{&out};
    layer.composite(renderer,
        Containers::arrayView<Vector2>({
            {},
            {}
        }),
        Containers::arrayView<Vector2>({
            {},
            {},
            {}
        })
    );
    CORRADE_COMPARE(out.str(), "Whee::AbstractLayer::composite(): expected rect offset and size views to have the same size but got 2 and 3\n");
}

void AbstractLayerTest::draw() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Draw;
        }

        void doDraw(const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, std::size_t offset, std::size_t count, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, std::size_t clipRectOffset, std::size_t clipRectCount, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes) override {
            ++called;
            CORRADE_COMPARE_AS(dataIds, Containers::arrayView({
                0xabcdeu,
                0u,
                0x45678u,
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE(offset, 1);
            CORRADE_COMPARE(count, 2);
            CORRADE_COMPARE_AS(clipRectIds, Containers::arrayView({
                /* These should be small enough to index into clipRectOffsets
                   and clipRectSizes but nobody cares here */
                3u,
                16u,
                0u,
                27u,
                2u
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(clipRectDataCounts, Containers::arrayView({
                /* The sum should be equal to dataIds.size(), yes, nobody cares
                   here */
                265u,
                1u,
                0u,
                13u,
                7u
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE(clipRectOffset, 2);
            CORRADE_COMPARE(clipRectCount, 3);
            CORRADE_COMPARE_AS(nodeOffsets, Containers::arrayView<Vector2>({
                {1.0f, 2.0f},
                {3.0f, 4.0f}
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(nodeSizes, Containers::arrayView<Vector2>({
                {0.1f, 0.2f},
                {0.3f, 0.4f}
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(nodesEnabled, Containers::stridedArrayView({
                false,
                true
            }).sliceBit(0), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(clipRectOffsets, Containers::arrayView<Vector2>({
                {6.5f, 7.5f},
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(clipRectSizes, Containers::arrayView<Vector2>({
                {8.5f, 9.5f},
            }), TestSuite::Compare::Container);
        }

        Int called = 0;
    } layer{layerHandle(0, 1)};

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    UnsignedByte nodesEnabled[1]{0x2};

    layer.draw(
        Containers::arrayView({
            0xabcdeu,
            0u,
            0x45678u
        }),
        1, 2,
        Containers::arrayView({
            3u,
            16u,
            0u,
            27u,
            2u
        }),
        Containers::arrayView({
            265u,
            1u,
            0u,
            13u,
            7u
        }),
        2, 3,
        Containers::arrayView<Vector2>({
            {1.0f, 2.0f},
            {3.0f, 4.0f}
        }),
        Containers::arrayView<Vector2>({
            {0.1f, 0.2f},
            {0.3f, 0.4f}
        }),
        Containers::BitArrayView{nodesEnabled, 0, 2},
        Containers::arrayView<Vector2>({
            {6.5f, 7.5f},
        }),
        Containers::arrayView<Vector2>({
            {8.5f, 9.5f},
        })
    );
    CORRADE_COMPARE(layer.called, 1);
}

void AbstractLayerTest::drawEmpty() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Draw;
        }

        void doDraw(const Containers::StridedArrayView1D<const UnsignedInt>&, std::size_t, std::size_t, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, std::size_t, std::size_t, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, Containers::BitArrayView, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&) override {
            ++called;
        }

        Int called = 0;
    } layer{layerHandle(0, 1)};

    /* It should call the implementation even with empty contents */
    layer.draw({}, 0, 0, {}, {}, 0, 0, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.called, 1);
}

void AbstractLayerTest::drawNotSupported() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0, 1)};

    std::ostringstream out;
    Error redirectError{&out};
    layer.draw({}, 0, 0, {}, {}, 0, 0, {}, {}, {}, {}, {});
    CORRADE_COMPARE(out.str(), "Whee::AbstractLayer::draw(): feature not supported\n");
}

void AbstractLayerTest::drawNotImplemented() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Draw;
        }
    } layer{layerHandle(0, 1)};

    std::ostringstream out;
    Error redirectError{&out};
    layer.draw({}, 0, 0, {}, {}, 0, 0, {}, {}, {}, {}, {});
    CORRADE_COMPARE(out.str(), "Whee::AbstractLayer::draw(): feature advertised but not implemented\n");
}

void AbstractLayerTest::drawInvalidSizes() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Draw;
        }
    } layer{layerHandle(0, 1)};

    UnsignedByte nodesEnabled[1]{};

    std::ostringstream out;
    Error redirectError{&out};
    layer.draw(
        {},
        0, 0,
        Containers::arrayView({
            0u,
            0u,
            0u
        }),
        Containers::arrayView({
            0u,
            0u
        }),
        0, 0,
        {}, {}, {},
        {}, {}
    );
    layer.draw(
        {},
        0, 0,
        {}, {},
        0, 0,
        Containers::arrayView<Vector2>({
            {},
            {}
        }),
        Containers::arrayView<Vector2>({
            {},
            {},
            {}
        }),
        Containers::BitArrayView{nodesEnabled, 0, 2},
        {}, {}
    );
    layer.draw(
        {},
        0, 0,
        {}, {},
        0, 0,
        Containers::arrayView<Vector2>({
            {},
            {}
        }),
        Containers::arrayView<Vector2>({
            {},
            {}
        }),
        Containers::BitArrayView{nodesEnabled, 0, 3},
        {}, {}
    );
    layer.draw(
        {},
        0, 0,
        {}, {},
        0, 0,
        {}, {}, {},
        Containers::arrayView<Vector2>({
            {},
            {},
            {}
        }),
        Containers::arrayView<Vector2>({
            {},
            {}
        })
    );
    layer.draw(
        Containers::arrayView({
            0u,
            0u
        }),
        3, 0,
        {}, {},
        0, 0,
        {}, {}, {},
        {}, {}
    );
    layer.draw(
        Containers::arrayView({
            0u,
            0u
        }),
        2, 1,
        {}, {},
        0, 0,
        {}, {}, {},
        {}, {}
    );
    layer.draw(
        {},
        0, 0,
        Containers::arrayView({
            0u,
            0u,
            0u
        }),
        Containers::arrayView({
            0u,
            0u,
            0u
        }),
        4, 0,
        {}, {}, {},
        {}, {}
    );
    layer.draw(
        {},
        0, 0,
        Containers::arrayView({
            0u,
            0u,
            0u
        }),
        Containers::arrayView({
            0u,
            0u,
            0u
        }),
        1, 3,
        {}, {}, {},
        {}, {}
    );
    CORRADE_COMPARE_AS(out.str(),
        "Whee::AbstractLayer::draw(): expected clip rect ID and data count views to have the same size but got 3 and 2\n"
        "Whee::AbstractLayer::draw(): expected node offset, size and enabled views to have the same size but got 2, 3 and 2\n"
        "Whee::AbstractLayer::draw(): expected node offset, size and enabled views to have the same size but got 2, 2 and 3\n"
        "Whee::AbstractLayer::draw(): expected clip rect offset and size views to have the same size but got 3 and 2\n"
        "Whee::AbstractLayer::draw(): offset 3 and count 0 out of range for 2 items\n"
        "Whee::AbstractLayer::draw(): offset 2 and count 1 out of range for 2 items\n"
        "Whee::AbstractLayer::draw(): clip rect offset 4 and count 0 out of range for 3 items\n"
        "Whee::AbstractLayer::draw(): clip rect offset 1 and count 3 out of range for 3 items\n",
        TestSuite::Compare::String);
}

void AbstractLayerTest::pointerEvent() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Event;
        }

        void doPointerPressEvent(UnsignedInt dataId, PointerEvent& event) override {
            CORRADE_COMPARE(dataId, 1);
            CORRADE_COMPARE(event.type(), Pointer::MouseLeft);
            called *= 2;
        }
        void doPointerReleaseEvent(UnsignedInt dataId, PointerEvent& event) override {
            CORRADE_COMPARE(dataId, 2);
            CORRADE_COMPARE(event.type(), Pointer::MouseRight);
            called *= 3;
        }
        void doPointerTapOrClickEvent(UnsignedInt dataId, PointerEvent& event) override {
            CORRADE_COMPARE(dataId, 3);
            CORRADE_COMPARE(event.type(), Pointer::Pen);
            called *= 5;
        }
        void doPointerMoveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            CORRADE_COMPARE(dataId, 4);
            CORRADE_COMPARE(event.type(), Pointer::Finger);
            called *= 7;
        }
        void doPointerEnterEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            CORRADE_COMPARE(dataId, 5);
            CORRADE_COMPARE(event.type(), Pointer::Finger);
            called *= 11;
        }
        void doPointerLeaveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            CORRADE_COMPARE(dataId, 6);
            CORRADE_COMPARE(event.type(), Pointer::Finger);
            called *= 13;
        }

        int called = 1;
    } layer{layerHandle(0, 1)};

    /* Capture correct test case name */
    CORRADE_VERIFY(true);

    layer.create();
    layer.create();
    layer.create();
    layer.create();
    layer.create();
    layer.create();
    layer.create();
    {
        PointerEvent event{Pointer::MouseLeft};
        layer.pointerPressEvent(1, event);
    } {
        PointerEvent event{Pointer::MouseRight};
        layer.pointerReleaseEvent(2, event);
    } {
        PointerEvent event{Pointer::Pen};
        layer.pointerTapOrClickEvent(3, event);
    } {
        PointerMoveEvent event{Pointer::Finger, {}};
        layer.pointerMoveEvent(4, event);
    } {
        PointerMoveEvent event{Pointer::Finger, {}};
        layer.pointerEnterEvent(5, event);
    } {
        PointerMoveEvent event{Pointer::Finger, {}};
        layer.pointerLeaveEvent(6, event);
    }
    CORRADE_COMPARE(layer.called, 2*3*5*7*11*13);
}

void AbstractLayerTest::pointerEventNotSupported() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0, 1)};

    std::ostringstream out;
    Error redirectError{&out};
    PointerEvent event{Pointer::MouseMiddle};
    PointerMoveEvent moveEvent{{}, {}};
    layer.pointerPressEvent(0, event);
    layer.pointerReleaseEvent(0, event);
    layer.pointerTapOrClickEvent(0, event);
    layer.pointerMoveEvent(0, moveEvent);
    layer.pointerEnterEvent(0, moveEvent);
    layer.pointerLeaveEvent(0, moveEvent);
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractLayer::pointerPressEvent(): feature not supported\n"
        "Whee::AbstractLayer::pointerReleaseEvent(): feature not supported\n"
        "Whee::AbstractLayer::pointerTapOrClickEvent(): feature not supported\n"
        "Whee::AbstractLayer::pointerMoveEvent(): feature not supported\n"
        "Whee::AbstractLayer::pointerEnterEvent(): feature not supported\n"
        "Whee::AbstractLayer::pointerLeaveEvent(): feature not supported\n");
}

void AbstractLayerTest::pointerEventNotImplemented() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Event;
        }
    } layer{layerHandle(0, 1)};

    layer.create();

    PointerEvent event{Pointer::MouseMiddle};
    PointerMoveEvent moveEvent{{}, {}};
    layer.pointerPressEvent(0, event);
    layer.pointerReleaseEvent(0, event);
    layer.pointerTapOrClickEvent(0, event);
    layer.pointerMoveEvent(0, moveEvent);
    layer.pointerEnterEvent(0, moveEvent);
    layer.pointerLeaveEvent(0, moveEvent);

    /* Shouldn't crash or anything */
    CORRADE_VERIFY(true);
}

void AbstractLayerTest::pointerEventOutOfRange() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Event;
        }
    } layer{layerHandle(0, 1)};

    layer.create();
    layer.create();

    std::ostringstream out;
    Error redirectError{&out};
    PointerEvent event{Pointer::MouseMiddle};
    PointerMoveEvent moveEvent{{}, {}};
    layer.pointerPressEvent(2, event);
    layer.pointerReleaseEvent(2, event);
    layer.pointerTapOrClickEvent(2, event);
    layer.pointerMoveEvent(2, moveEvent);
    layer.pointerEnterEvent(2, moveEvent);
    layer.pointerLeaveEvent(2, moveEvent);
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractLayer::pointerPressEvent(): index 2 out of range for 2 data\n"
        "Whee::AbstractLayer::pointerReleaseEvent(): index 2 out of range for 2 data\n"
        "Whee::AbstractLayer::pointerTapOrClickEvent(): index 2 out of range for 2 data\n"
        "Whee::AbstractLayer::pointerMoveEvent(): index 2 out of range for 2 data\n"
        "Whee::AbstractLayer::pointerEnterEvent(): index 2 out of range for 2 data\n"
        "Whee::AbstractLayer::pointerLeaveEvent(): index 2 out of range for 2 data\n");
}

void AbstractLayerTest::pointerEventAlreadyAccepted() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Event;
        }
    } layer{layerHandle(0, 1)};

    layer.create();

    std::ostringstream out;
    Error redirectError{&out};
    PointerEvent event{Pointer::MouseMiddle};
    event.setAccepted();
    PointerMoveEvent moveEvent{{}, {}};
    moveEvent.setAccepted();
    layer.pointerPressEvent(0, event);
    layer.pointerReleaseEvent(0, event);
    layer.pointerTapOrClickEvent(0, event);
    layer.pointerMoveEvent(0, moveEvent);
    layer.pointerEnterEvent(0, moveEvent);
    layer.pointerLeaveEvent(0, moveEvent);
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractLayer::pointerPressEvent(): event already accepted\n"
        "Whee::AbstractLayer::pointerReleaseEvent(): event already accepted\n"
        "Whee::AbstractLayer::pointerTapOrClickEvent(): event already accepted\n"
        "Whee::AbstractLayer::pointerMoveEvent(): event already accepted\n"
        "Whee::AbstractLayer::pointerEnterEvent(): event already accepted\n"
        "Whee::AbstractLayer::pointerLeaveEvent(): event already accepted\n");
}

}}}}

CORRADE_TEST_MAIN(Magnum::Whee::Test::AbstractLayerTest)
