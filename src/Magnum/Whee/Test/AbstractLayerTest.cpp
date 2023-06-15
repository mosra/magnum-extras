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
#include <Corrade/Containers/StridedBitArrayView.h>
#include <Corrade/Containers/StringStl.h> /** @todo remove once Debug is stream-free */
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Container.h>
#include <Corrade/TestSuite/Compare/String.h>
#include <Corrade/Utility/DebugStl.h>
#include <Magnum/Math/Vector2.h>

#include "Magnum/Whee/AbstractLayer.h"
#include "Magnum/Whee/Event.h"
#include "Magnum/Whee/Handle.h"

namespace Magnum { namespace Whee { namespace Test { namespace {

struct AbstractLayerTest: TestSuite::Tester {
    explicit AbstractLayerTest();

    void debugFeature();
    void debugFeatures();
    void debugState();
    void debugStates();

    void construct();
    void constructInvalidHandle();
    void constructCopy();
    void constructMove();

    void createRemove();
    void createRemoveHandleRecycle();
    void createRemoveHandleDisable();
    void createNoHandlesLeft();
    void removeInvalid();

    void clean();
    void cleanEmpty();
    void cleanNotImplemented();
    void cleanInvalidSize();

    void update();
    void updateEmpty();
    void updateNotImplemented();
    void updateInvalidSizes();

    void state();

    void draw();
    void drawEmpty();
    void drawNotSupported();
    void drawNotImplemented();
    void drawInvalidSizes();

    void pointerEvent();
    void pointerEventNotSupported();
    void pointerEventNotImplemented();
    void pointerEventOutOfRange();
};

AbstractLayerTest::AbstractLayerTest() {
    addTests({&AbstractLayerTest::debugFeature,
              &AbstractLayerTest::debugFeatures,
              &AbstractLayerTest::debugState,
              &AbstractLayerTest::debugStates,

              &AbstractLayerTest::construct,
              &AbstractLayerTest::constructInvalidHandle,
              &AbstractLayerTest::constructCopy,
              &AbstractLayerTest::constructMove,

              &AbstractLayerTest::createRemove,
              &AbstractLayerTest::createRemoveHandleRecycle,
              &AbstractLayerTest::createRemoveHandleDisable,
              &AbstractLayerTest::createNoHandlesLeft,
              &AbstractLayerTest::removeInvalid,

              &AbstractLayerTest::clean,
              &AbstractLayerTest::cleanEmpty,
              &AbstractLayerTest::cleanNotImplemented,
              &AbstractLayerTest::cleanInvalidSize,

              &AbstractLayerTest::update,
              &AbstractLayerTest::updateEmpty,
              &AbstractLayerTest::updateNotImplemented,
              &AbstractLayerTest::updateInvalidSizes,

              &AbstractLayerTest::state,

              &AbstractLayerTest::draw,
              &AbstractLayerTest::drawEmpty,
              &AbstractLayerTest::drawNotSupported,
              &AbstractLayerTest::drawNotImplemented,
              &AbstractLayerTest::drawInvalidSizes,

              &AbstractLayerTest::pointerEvent,
              &AbstractLayerTest::pointerEventNotSupported,
              &AbstractLayerTest::pointerEventNotImplemented,
              &AbstractLayerTest::pointerEventOutOfRange});
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

void AbstractLayerTest::debugState() {
    std::ostringstream out;
    Debug{&out} << LayerState::NeedsClean << LayerState(0xbe);
    CORRADE_COMPARE(out.str(), "Whee::LayerState::NeedsClean Whee::LayerState(0xbe)\n");
}

void AbstractLayerTest::debugStates() {
    std::ostringstream out;
    Debug{&out} << (LayerState::NeedsClean|LayerState(0xe0)) << LayerStates{};
    CORRADE_COMPARE(out.str(), "Whee::LayerState::NeedsClean|Whee::LayerState(0xe0) Whee::LayerStates{}\n");
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

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0xab, 0x12)};

    DataHandle first = layer.create();
    CORRADE_COMPARE(first, dataHandle(layer.handle(), 0, 1));
    CORRADE_VERIFY(layer.isHandleValid(first));
    CORRADE_COMPARE(layer.state(), LayerStates{});
    CORRADE_COMPARE(layer.capacity(), 1);
    CORRADE_COMPARE(layer.usedCount(), 1);

    DataHandle second = layer.create();
    CORRADE_COMPARE(second, dataHandle(layer.handle(), 1, 1));
    CORRADE_VERIFY(layer.isHandleValid(second));
    CORRADE_COMPARE(layer.state(), LayerStates{});
    CORRADE_COMPARE(layer.capacity(), 2);
    CORRADE_COMPARE(layer.usedCount(), 2);

    layer.remove(first);
    CORRADE_VERIFY(!layer.isHandleValid(first));
    CORRADE_VERIFY(layer.isHandleValid(second));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsClean);
    CORRADE_COMPARE(layer.capacity(), 2);
    CORRADE_COMPARE(layer.usedCount(), 1);

    /* Using also the LayouterDataHandle overload */
    layer.remove(dataHandleData(second));
    CORRADE_VERIFY(!layer.isHandleValid(first));
    CORRADE_VERIFY(!layer.isHandleValid(second));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsClean);
    CORRADE_COMPARE(layer.capacity(), 2);
    CORRADE_COMPARE(layer.usedCount(), 0);
}

void AbstractLayerTest::createRemoveHandleRecycle() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

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
    CORRADE_COMPARE(layer.state(), LayerStates{});
    CORRADE_COMPARE(layer.capacity(), 4);
    CORRADE_COMPARE(layer.usedCount(), 4);

    /* Remove three out of the four in an arbitrary order */
    layer.remove(fourth);
    layer.remove(first);
    layer.remove(third);
    CORRADE_VERIFY(!layer.isHandleValid(first));
    CORRADE_VERIFY(layer.isHandleValid(second));
    CORRADE_VERIFY(!layer.isHandleValid(third));
    CORRADE_VERIFY(!layer.isHandleValid(fourth));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsClean);
    CORRADE_COMPARE(layer.capacity(), 4);
    CORRADE_COMPARE(layer.usedCount(), 1);

    /* Allocating new handles should recycle the handles in the order they were
       removed (oldest first) */
    DataHandle fourth2 = layer.create();
    DataHandle first2 = layer.create();
    DataHandle third2 = layer.create();
    CORRADE_COMPARE(first2, dataHandle(layer.handle(), 0, 2));
    CORRADE_COMPARE(third2, dataHandle(layer.handle(), 2, 2));
    CORRADE_COMPARE(fourth2, dataHandle(layer.handle(), 3, 2));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsClean);
    CORRADE_COMPARE(layer.capacity(), 4);
    CORRADE_COMPARE(layer.usedCount(), 4);

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
    CORRADE_COMPARE(layer.state(), LayerState::NeedsClean);
    CORRADE_COMPARE(layer.capacity(), 4);
    CORRADE_COMPARE(layer.usedCount(), 4);

    /* Allocating a new handle with the free list empty will grow it */
    DataHandle fifth = layer.create();
    CORRADE_COMPARE(fifth, dataHandle(layer.handle(), 4, 1));
    CORRADE_VERIFY(layer.isHandleValid(fifth));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsClean);
    CORRADE_COMPARE(layer.capacity(), 5);
    CORRADE_COMPARE(layer.usedCount(), 5);
}

void AbstractLayerTest::createRemoveHandleDisable() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

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

void AbstractLayerTest::removeInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

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

void AbstractLayerTest::clean() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }

        void doClean(Containers::BitArrayView dataIdsToRemove) override {
            ++called;
            CORRADE_COMPARE_AS(dataIdsToRemove, Containers::stridedArrayView({
                true, false, false, true
            }).sliceBit(0), TestSuite::Compare::Container);
        }

        Int called = 0;
    } layer{layerHandle(0, 1)};

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    /* Create four data to match the four bits, delete one of them (which still
       keeps the capacity at 4) */
    DataHandle first = layer.create();
    DataHandle second = layer.create();
    DataHandle third = layer.create();
    DataHandle fourth = layer.create();
    layer.remove(third);

    UnsignedByte dataIdsToRemove[]{0x9};
    layer.clean(Containers::BitArrayView{dataIdsToRemove, 0, 4});
    CORRADE_COMPARE(layer.called, 1);

    /* Only the second data should stay afterwards */
    CORRADE_VERIFY(!layer.isHandleValid(first));
    CORRADE_VERIFY(layer.isHandleValid(second));
    CORRADE_VERIFY(!layer.isHandleValid(third));
    CORRADE_VERIFY(!layer.isHandleValid(fourth));
}

void AbstractLayerTest::cleanEmpty() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }

        void doClean(Containers::BitArrayView) override {
            ++called;
        }

        Int called = 0;
    } layer{layerHandle(0, 1)};

    /* It should call the implementation even with empty contents */
    layer.clean({});
    CORRADE_COMPARE(layer.called, 1);
}

void AbstractLayerTest::cleanNotImplemented() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0, 1)};

    /* Create three data to match the three bits */
    layer.create();
    layer.create();
    layer.create();

    UnsignedByte bits[]{0x5};
    layer.clean(Containers::BitArrayView{bits, 0, 3});

    /* Shouldn't crash or anything */
    CORRADE_VERIFY(true);
}

void AbstractLayerTest::cleanInvalidSize() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0, 1)};

    /* Create three data to (not) match the two bits */
    layer.create();
    layer.create();
    layer.create();

    std::ostringstream out;
    Error redirectError{&out};
    UnsignedByte bits[1]{};
    layer.clean(Containers::BitArrayView{bits, 0, 2});
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractLayer::clean(): expected 3 bits but got 2\n");
}

void AbstractLayerTest::update() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }

        void doUpdate(const Containers::StridedArrayView1D<const UnsignedInt>& data, const Containers::StridedArrayView1D<const UnsignedInt>& dataNodeIds, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes) override {
            ++called;
            CORRADE_COMPARE_AS(data, Containers::arrayView({
                0xabcdeu,
                0x45678u
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(dataNodeIds, Containers::arrayView<UnsignedInt>({
                3, 15
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
        }

        Int called = 0;
    } layer{layerHandle(0, 1)};

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    layer.update(
        Containers::arrayView({
            0xabcdeu,
            0x45678u,
        }),
        Containers::arrayView<UnsignedInt>({
            3, 15
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
        })
    );
    CORRADE_COMPARE(layer.called, 1);
}

void AbstractLayerTest::updateEmpty() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }

        void doUpdate(const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&) override {
            ++called;
        }

        Int called = 0;
    } layer{layerHandle(0, 1)};

    /* It should call the implementation even with empty contents */
    layer.update({}, {}, {}, {});
    CORRADE_COMPARE(layer.called, 1);
}

void AbstractLayerTest::updateNotImplemented() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0, 1)};

    layer.update(
        Containers::arrayView({
            0u,
            0u
        }),
        Containers::arrayView({
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

    std::ostringstream out;
    Error redirectError{&out};
    layer.update(
        Containers::arrayView({
            0u,
            0u
        }),
        Containers::arrayView({
            0u
        }),
        {}, {}
    );
    layer.update(
        {}, {},
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
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractLayer::update(): expected data and node ID views to have the same size but got 2 and 1\n"
        "Whee::AbstractLayer::update(): expected node offset and size views to have the same size but got 2 and 3\n");
}

void AbstractLayerTest::state() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0xab, 0x12)};

    CORRADE_COMPARE(layer.state(), LayerStates{});

    layer.setNeedsUpdate();
    CORRADE_COMPARE(layer.state(), LayerState::NeedsUpdate);

    /* remove() adds NeedsClean */
    layer.remove(layer.create());
    CORRADE_COMPARE(layer.state(), LayerState::NeedsUpdate|LayerState::NeedsClean);

    /* NeedsClean is cleaned by this, NeedsUpdate stays. There's one (removed)
       data so have to pass one bit. */
    UnsignedByte dataIdsToRemove[]{0x00};
    layer.clean(Containers::BitArrayView{dataIdsToRemove, 0, 1});
    CORRADE_COMPARE(layer.state(), LayerState::NeedsUpdate);

    /* Add both flags again; this time with the other overload to verify both
       do the same */
    layer.remove(dataHandleData(layer.create()));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsUpdate|LayerState::NeedsClean);

    /* update() then resets NeedsUpdate */
    layer.update({}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerState::NeedsClean);
}

void AbstractLayerTest::draw() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Draw;
        }

        void doDraw(const Containers::StridedArrayView1D<const UnsignedInt>& data, const Containers::StridedArrayView1D<const UnsignedInt>& dataNodeIds, std::size_t offset, std::size_t count, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes) override {
            ++called;
            CORRADE_COMPARE_AS(data, Containers::arrayView({
                0xabcdeu,
                0u,
                0x45678u,
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(dataNodeIds, Containers::arrayView<UnsignedInt>({
                3, 2, 15
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE(offset, 1);
            CORRADE_COMPARE(count, 2);
            CORRADE_COMPARE_AS(nodeOffsets, Containers::arrayView<Vector2>({
                {1.0f, 2.0f},
                {3.0f, 4.0f}
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(nodeSizes, Containers::arrayView<Vector2>({
                {0.1f, 0.2f},
                {0.3f, 0.4f}
            }), TestSuite::Compare::Container);
        }

        Int called = 0;
    } layer{layerHandle(0, 1)};

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    layer.draw(
        Containers::arrayView({
            0xabcdeu,
            0u,
            0x45678u
        }),
        Containers::arrayView<UnsignedInt>({
            3, 2, 15
        }),
        1, 2,
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

void AbstractLayerTest::drawEmpty() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Draw;
        }

        void doDraw(const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, std::size_t, std::size_t, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&) override {
            ++called;
        }

        Int called = 0;
    } layer{layerHandle(0, 1)};

    /* It should call the implementation even with empty contents */
    layer.draw({}, {}, 0, 0, {}, {});
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
    layer.draw({}, {}, 0, 0, {}, {});
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
    layer.draw({}, {}, 0, 0, {}, {});
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

    std::ostringstream out;
    Error redirectError{&out};
    layer.draw(
        Containers::arrayView({
            0u,
            0u
        }),
        Containers::arrayView({
            0u
        }),
        0, 0,
        {}, {}
    );
    layer.draw(
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
        })
    );
    layer.draw(
        Containers::arrayView({
            0u,
            0u
        }),
        Containers::arrayView({
            0u,
            0u
        }),
        3, 0,
        {}, {}
    );
    layer.draw(
        Containers::arrayView({
            0u,
            0u
        }),
        Containers::arrayView({
            0u,
            0u
        }),
        2, 1,
        {}, {}
    );
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractLayer::draw(): expected data and node ID views to have the same size but got 2 and 1\n"
        "Whee::AbstractLayer::draw(): expected node offset and size views to have the same size but got 2 and 3\n"
        "Whee::AbstractLayer::draw(): offset 3 and count 0 out of range for 2 items\n"
        "Whee::AbstractLayer::draw(): offset 2 and count 1 out of range for 2 items\n");
}

void AbstractLayerTest::pointerEvent() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Event;
        }

        void doPointerPressEvent(UnsignedInt dataId, PointerEvent& event) override {
            CORRADE_COMPARE(dataId, 1);
            CORRADE_COMPARE(event.position(), (Vector2{1.0f, 2.0f}));
            called *= 2;
        }
        void doPointerReleaseEvent(UnsignedInt dataId, PointerEvent& event) override {
            CORRADE_COMPARE(dataId, 2);
            CORRADE_COMPARE(event.position(), (Vector2{3.0f, 4.0f}));
            called *= 3;
        }

        int called = 1;
    } layer{layerHandle(0, 1)};

    /* Capture correct test case name */
    CORRADE_VERIFY(true);

    layer.create();
    layer.create();
    layer.create();
    {
        PointerEvent event{Pointer::MouseLeft};
        event.setPosition({1.0f, 2.0f});
        layer.pointerPressEvent(1, event);
    } {
        PointerEvent event{Pointer::MouseLeft};
        event.setPosition({3.0f, 4.0f});
        layer.pointerReleaseEvent(2, event);
    }
    CORRADE_COMPARE(layer.called, 2*3);
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
    layer.pointerPressEvent(0, event);
    layer.pointerReleaseEvent(0, event);
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractLayer::pointerPressEvent(): feature not supported\n"
        "Whee::AbstractLayer::pointerReleaseEvent(): feature not supported\n");
}

void AbstractLayerTest::pointerEventNotImplemented() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Event;
        }
    } layer{layerHandle(0, 1)};

    layer.create();

    PointerEvent event{Pointer::MouseMiddle};
    layer.pointerPressEvent(0, event);
    layer.pointerReleaseEvent(0, event);

    /* Shouldn't crash or anything */
    CORRADE_VERIFY(true);
}

void AbstractLayerTest::pointerEventOutOfRange() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Event;
        }
    } layer{layerHandle(0, 1)};

    layer.create();
    layer.create();

    std::ostringstream out;
    Error redirectError{&out};
    PointerEvent event{Pointer::MouseMiddle};
    layer.pointerPressEvent(2, event);
    layer.pointerReleaseEvent(2, event);
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractLayer::pointerPressEvent(): index 2 out of range for 2 data\n"
        "Whee::AbstractLayer::pointerReleaseEvent(): index 2 out of range for 2 data\n");
}

}}}}

CORRADE_TEST_MAIN(Magnum::Whee::Test::AbstractLayerTest)
