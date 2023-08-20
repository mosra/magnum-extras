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
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/Pair.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/StridedBitArrayView.h>
#include <Corrade/Containers/StringStl.h> /** @todo remove once Debug is stream-free */
#include <Corrade/Containers/Triple.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Container.h>
#include <Corrade/TestSuite/Compare/String.h>
#include <Corrade/Utility/DebugStl.h> /** @todo remove once Debug is stream-free */
#include <Corrade/Utility/Format.h>
#include <Magnum/Math/Vector4.h>

#include "Magnum/Whee/AbstractLayer.h"
#include "Magnum/Whee/AbstractUserInterface.h"
#include "Magnum/Whee/Event.h"
#include "Magnum/Whee/Handle.h"

namespace Magnum { namespace Whee { namespace Test { namespace {

struct AbstractUserInterfaceTest: TestSuite::Tester {
    explicit AbstractUserInterfaceTest();

    void debugNodeFlag();
    void debugNodeFlags();
    void debugState();
    void debugStates();
    void debugStatesSupersets();

    void constructNoCreate();
    void construct();
    void constructSingleSize();
    void constructCopy();
    void constructMove();

    void layer();
    void layerHandleRecycle();
    void layerHandleDisable();
    void layerHandleLastFree();
    void layerSetInstance();
    void layerCreateInvalid();
    void layerSetInstanceInvalid();
    void layerGetInvalid();
    void layerRemoveInvalid();
    void layerNoHandlesLeft();

    void node();
    void nodeHandleRecycle();
    void nodeHandleDisable();
    void nodeFlags();
    void nodeGetSetInvalid();
    void nodeCreateInvalid();
    void nodeRemoveInvalid();
    void nodeNoHandlesLeft();

    void nodeOrder();
    void nodeOrderGetSetInvalid();

    void data();
    void dataAttach();
    void dataAttachInvalid();

    void setSize();
    void setSizeZero();
    void setSizeNotCalledBeforeUpdate();

    void cleanEmpty();
    void cleanNoOp();
    void cleanRemoveInvalidData();
    void cleanRemoveDataInvalidLayer();
    void cleanRemoveAttachedData();
    void cleanRemoveNestedNodes();
    void cleanRemoveNestedNodesAlreadyRemoved();
    void cleanRemoveNestedNodesAlreadyRemovedDangling();
    void cleanRemoveNestedNodesRecycledHandle();
    void cleanRemoveNestedNodesRecycledHandleOrphanedCycle();
    void cleanRemoveAll();

    void state();
    void statePropagateFromLayers();

    void drawEmpty();
    void draw();

    void eventEmpty();
    void eventAlreadyAccepted();
    void eventNodePropagation();
    void eventEdges();

    void eventPointerPress();
    void eventPointerRelease();
    void eventPointerMove();
    void eventPointerMoveRelativePositionWithPressRelease();
    void eventPointerMoveNotAccepted();
    void eventPointerMoveNodePositionUpdated();
    void eventPointerMoveNodeBecomesHidden();
    void eventPointerMoveNodeRemoved();
    void eventPointerMoveDataRemoved();

    void eventCapture();
    void eventCaptureEdges();
    void eventCaptureNotAccepted();
    void eventCaptureNotCaptured();
    void eventCaptureChangeCaptureInNotAcceptedEvent();
    void eventCaptureNodePositionUpdated();
    void eventCaptureNodeBecomesHidden();
    void eventCaptureNodeRemoved();
    void eventCaptureDataRemoved();
};

const struct {
    const char* name;
    bool clean;
    bool noOp;
} StateData[]{
    {"", true, false},
    {"with no-op calls", true, true},
    {"with implicit clean", false, false},
    {"with implicit clean and no-op calls", false, true},
};

const struct {
    const char* name;
    bool clean;
    bool update;
    bool reorderLayers;
} DrawData[]{
    {"clean + update before", true, true, false},
    {"clean before", true, false, false},
    {"update before", false, true, false},
    {"", false, false, false},
    {"non-implicit layer order", false, false, true},
};

const struct {
    const char* name;
    bool clean;
    bool update;
} CleanUpdateData[]{
    {"clean + update before", true, true},
    {"clean before", true, false},
    {"update before", false, true},
    {"", false, false},
};

const struct {
    const char* name;
    bool update;
} UpdateData[]{
    {"update before", true},
    {"", false},
};

const struct {
    const char* name;
    bool accept;
} EventPointerMoveRelativePositionWithPressReleaseData[]{
    {"events accepted", true},
    {"events not accepted", false}
};

const struct {
    const char* name;
    NodeFlags flags;
    bool clearOrder;
    bool update;
} EventPointerNodeBecomesHiddenData[]{
    {"removed from top level order, update before", {}, true, true},
    {"removed from top level order", {}, true, false},
    {"hidden, update before", NodeFlag::Hidden, false, true},
    {"hidden", NodeFlag::Hidden, false, false},
};

const struct {
    const char* name;
    bool update;
    bool removeParent;
} EventNodeRemovedData[]{
    {"update before", true, false},
    {"update before, remove parent node", true, true},
    {"", false, false},
    {"remove parent node", false, true}
};

const struct {
    const char* name;
    bool release;
    bool move;
    bool update;
} EventCaptureUpdateData[]{
    {"release, update before", true, false, true},
    {"release", true, false, false},
    {"move, update before", false, true, true},
    {"move", false, true, false},
};

const struct {
    const char* name;
    NodeFlags flags;
    bool clearOrder;
    bool release;
    bool move;
    bool update;
} EventCaptureNodeBecomesHiddenData[]{
    {"removed from top level order, release, update before", {}, true,
        true, false, true},
    {"removed from top level order, release", {}, true,
        true, false, false},
    {"removed from top level order, move, update before", {}, true,
        false, true, true},
    {"removed from top level order, move", {}, true,
        false, true, false},
    {"hidden, release, update before", NodeFlag::Hidden, false,
        true, false, true},
    {"hidden, release", NodeFlag::Hidden, false,
        true, false, false},
    {"hidden, move, update before", NodeFlag::Hidden, false,
        false, true, true},
    {"hidden, move", NodeFlag::Hidden, false,
        false, true, false},
};

const struct {
    const char* name;
    bool release;
    bool move;
    bool update;
    bool removeParent;
} EventCaptureNodeRemovedData[]{
    {"release, update before", true, false, true, false},
    {"release, update before, remove parent node", true, false, true, true},
    {"release", true, false, false, false},
    {"release, remove parent node", true, false, false, true},
    {"move", false, true, false, false},
};

const struct {
    const char* name;
    bool release;
    bool move;
    bool clean;
    bool update;
} EventCaptureCleanUpdateData[]{
    {"release, clean + update before", true, false, true, true},
    {"release, clean before", true, false, true, false},
    {"release, update before", true, false, false, true},
    {"release", true, false, false, false},
    {"move", false, true, false, false},
};

AbstractUserInterfaceTest::AbstractUserInterfaceTest() {
    addTests({&AbstractUserInterfaceTest::debugNodeFlag,
              &AbstractUserInterfaceTest::debugNodeFlags,
              &AbstractUserInterfaceTest::debugState,
              &AbstractUserInterfaceTest::debugStates,
              &AbstractUserInterfaceTest::debugStatesSupersets,

              &AbstractUserInterfaceTest::constructNoCreate,
              &AbstractUserInterfaceTest::construct,
              &AbstractUserInterfaceTest::constructSingleSize,
              &AbstractUserInterfaceTest::constructCopy,
              &AbstractUserInterfaceTest::constructMove,

              &AbstractUserInterfaceTest::layer,
              &AbstractUserInterfaceTest::layerHandleRecycle,
              &AbstractUserInterfaceTest::layerHandleDisable,
              &AbstractUserInterfaceTest::layerHandleLastFree,
              &AbstractUserInterfaceTest::layerSetInstance,
              &AbstractUserInterfaceTest::layerCreateInvalid,
              &AbstractUserInterfaceTest::layerSetInstanceInvalid,
              &AbstractUserInterfaceTest::layerGetInvalid,
              &AbstractUserInterfaceTest::layerRemoveInvalid,
              &AbstractUserInterfaceTest::layerNoHandlesLeft,

              &AbstractUserInterfaceTest::node,
              &AbstractUserInterfaceTest::nodeHandleRecycle,
              &AbstractUserInterfaceTest::nodeHandleDisable,
              &AbstractUserInterfaceTest::nodeFlags,
              &AbstractUserInterfaceTest::nodeCreateInvalid,
              &AbstractUserInterfaceTest::nodeGetSetInvalid,
              &AbstractUserInterfaceTest::nodeRemoveInvalid,
              &AbstractUserInterfaceTest::nodeNoHandlesLeft,

              &AbstractUserInterfaceTest::nodeOrder,
              &AbstractUserInterfaceTest::nodeOrderGetSetInvalid,

              &AbstractUserInterfaceTest::data,
              &AbstractUserInterfaceTest::dataAttach,
              &AbstractUserInterfaceTest::dataAttachInvalid,

              &AbstractUserInterfaceTest::setSize,
              &AbstractUserInterfaceTest::setSizeZero,
              &AbstractUserInterfaceTest::setSizeNotCalledBeforeUpdate,

              &AbstractUserInterfaceTest::cleanEmpty,
              &AbstractUserInterfaceTest::cleanNoOp,
              &AbstractUserInterfaceTest::cleanRemoveInvalidData,
              &AbstractUserInterfaceTest::cleanRemoveDataInvalidLayer,
              &AbstractUserInterfaceTest::cleanRemoveAttachedData,
              &AbstractUserInterfaceTest::cleanRemoveNestedNodes,
              &AbstractUserInterfaceTest::cleanRemoveNestedNodesAlreadyRemoved,
              &AbstractUserInterfaceTest::cleanRemoveNestedNodesAlreadyRemovedDangling,
              &AbstractUserInterfaceTest::cleanRemoveNestedNodesRecycledHandle,
              &AbstractUserInterfaceTest::cleanRemoveNestedNodesRecycledHandleOrphanedCycle,
              &AbstractUserInterfaceTest::cleanRemoveAll});

    addInstancedTests({&AbstractUserInterfaceTest::state},
        Containers::arraySize(StateData));

    addTests({&AbstractUserInterfaceTest::statePropagateFromLayers});

    addInstancedTests({&AbstractUserInterfaceTest::drawEmpty},
        Containers::arraySize(CleanUpdateData));

    addInstancedTests({&AbstractUserInterfaceTest::draw},
        Containers::arraySize(DrawData));

    addInstancedTests({&AbstractUserInterfaceTest::eventEmpty},
        Containers::arraySize(CleanUpdateData));

    addTests({&AbstractUserInterfaceTest::eventAlreadyAccepted});

    addInstancedTests({&AbstractUserInterfaceTest::eventNodePropagation},
        Containers::arraySize(CleanUpdateData));

    addTests({&AbstractUserInterfaceTest::eventEdges});

    addInstancedTests({&AbstractUserInterfaceTest::eventPointerPress,
                       &AbstractUserInterfaceTest::eventPointerRelease,
                       &AbstractUserInterfaceTest::eventPointerMove},
        Containers::arraySize(UpdateData));

    addInstancedTests({&AbstractUserInterfaceTest::eventPointerMoveRelativePositionWithPressRelease},
        Containers::arraySize(EventPointerMoveRelativePositionWithPressReleaseData));

    addTests({&AbstractUserInterfaceTest::eventPointerMoveNotAccepted});

    addInstancedTests({&AbstractUserInterfaceTest::eventPointerMoveNodePositionUpdated},
        Containers::arraySize(UpdateData));

    addInstancedTests({&AbstractUserInterfaceTest::eventPointerMoveNodeBecomesHidden},
        Containers::arraySize(EventPointerNodeBecomesHiddenData));

    addInstancedTests({&AbstractUserInterfaceTest::eventPointerMoveNodeRemoved},
        Containers::arraySize(EventNodeRemovedData));

    addInstancedTests({&AbstractUserInterfaceTest::eventPointerMoveDataRemoved},
        Containers::arraySize(CleanUpdateData));

    addTests({&AbstractUserInterfaceTest::eventCapture,
              &AbstractUserInterfaceTest::eventCaptureEdges,
              &AbstractUserInterfaceTest::eventCaptureNotAccepted,
              &AbstractUserInterfaceTest::eventCaptureNotCaptured,
              &AbstractUserInterfaceTest::eventCaptureChangeCaptureInNotAcceptedEvent});

    addInstancedTests({&AbstractUserInterfaceTest::eventCaptureNodePositionUpdated},
        Containers::arraySize(EventCaptureUpdateData));

    addInstancedTests({&AbstractUserInterfaceTest::eventCaptureNodeBecomesHidden},
        Containers::arraySize(EventCaptureNodeBecomesHiddenData));

    addInstancedTests({&AbstractUserInterfaceTest::eventCaptureNodeRemoved},
        Containers::arraySize(EventCaptureNodeRemovedData));

    addInstancedTests({&AbstractUserInterfaceTest::eventCaptureDataRemoved},
        Containers::arraySize(EventCaptureCleanUpdateData));
}

void AbstractUserInterfaceTest::debugNodeFlag() {
    std::ostringstream out;
    Debug{&out} << NodeFlag::Hidden << NodeFlag(0xbe);
    CORRADE_COMPARE(out.str(), "Whee::NodeFlag::Hidden Whee::NodeFlag(0xbe)\n");
}

void AbstractUserInterfaceTest::debugNodeFlags() {
    std::ostringstream out;
    Debug{&out} << (NodeFlag::Hidden|NodeFlag(0xe0)) << NodeFlags{};
    CORRADE_COMPARE(out.str(), "Whee::NodeFlag::Hidden|Whee::NodeFlag(0xe0) Whee::NodeFlags{}\n");
}

void AbstractUserInterfaceTest::debugState() {
    std::ostringstream out;
    Debug{&out} << UserInterfaceState::NeedsNodeClean << UserInterfaceState(0xbe);
    CORRADE_COMPARE(out.str(), "Whee::UserInterfaceState::NeedsNodeClean Whee::UserInterfaceState(0xbe)\n");
}

void AbstractUserInterfaceTest::debugStates() {
    std::ostringstream out;
    Debug{&out} << (UserInterfaceState::NeedsNodeClean|UserInterfaceState(0x80)) << UserInterfaceStates{};
    CORRADE_COMPARE(out.str(), "Whee::UserInterfaceState::NeedsNodeClean|Whee::UserInterfaceState(0x80) Whee::UserInterfaceStates{}\n");
}

void AbstractUserInterfaceTest::debugStatesSupersets() {
    /* NeedsDataAttachmentUpdate is a superset of NeedsDataUpdate, so only one
       should be printed */
    {
        std::ostringstream out;
        Debug{&out} << (UserInterfaceState::NeedsDataUpdate|UserInterfaceState::NeedsDataAttachmentUpdate);
        CORRADE_COMPARE(out.str(), "Whee::UserInterfaceState::NeedsDataAttachmentUpdate\n");

    /* NeedsNodeClipUpdate is a superset of NeedsDataAttachmentUpdate, so only
       one should be printed */
    } {
        std::ostringstream out;
        Debug{&out} << (UserInterfaceState::NeedsNodeClipUpdate|UserInterfaceState::NeedsDataAttachmentUpdate);
        CORRADE_COMPARE(out.str(), "Whee::UserInterfaceState::NeedsNodeClipUpdate\n");

    /* NeedsNodeLayoutUpdate is a superset of NeedsNodeClipUpdate, so only one
       should be printed */
    } {
        std::ostringstream out;
        Debug{&out} << (UserInterfaceState::NeedsNodeLayoutUpdate|UserInterfaceState::NeedsNodeClipUpdate);
        CORRADE_COMPARE(out.str(), "Whee::UserInterfaceState::NeedsNodeLayoutUpdate\n");

    /* NeedsNodeUpdate is a superset of NeedsNodeLayoutUpdate, so only one
       should be printed */
    } {
        std::ostringstream out;
        Debug{&out} << (UserInterfaceState::NeedsNodeUpdate|UserInterfaceState::NeedsNodeLayoutUpdate);
        CORRADE_COMPARE(out.str(), "Whee::UserInterfaceState::NeedsNodeUpdate\n");

    /* NeedsDataClean is a superset of NeedsDataAttachmentUpdate, so only one
       should be printed */
    } {
        std::ostringstream out;
        Debug{&out} << (UserInterfaceState::NeedsDataClean |UserInterfaceState::NeedsDataAttachmentUpdate);
        CORRADE_COMPARE(out.str(), "Whee::UserInterfaceState::NeedsDataClean\n");

    /* NeedsNodeClean is a superset of NeedsNodeUpdate, so only one should be
       printed */
    } {
        std::ostringstream out;
        Debug{&out} << (UserInterfaceState::NeedsNodeClean|UserInterfaceState::NeedsNodeUpdate);
        CORRADE_COMPARE(out.str(), "Whee::UserInterfaceState::NeedsNodeClean\n");

    /* NeedsNodeClean is a superset of NeedsDataClean, so only one should be
       printed */
    } {
        std::ostringstream out;
        Debug{&out} << (UserInterfaceState::NeedsNodeClean|UserInterfaceState::NeedsDataClean);
        CORRADE_COMPARE(out.str(), "Whee::UserInterfaceState::NeedsNodeClean\n");

    /* NeedsNodeClipUpdate and NeedsDataClean are both supersets of
       NeedsDataAttachmentUpdate, so only the two should be printed */
    } {
        std::ostringstream out;
        Debug{&out} << (UserInterfaceState::NeedsNodeClipUpdate|UserInterfaceState::NeedsDataClean);
        CORRADE_COMPARE(out.str(), "Whee::UserInterfaceState::NeedsNodeClipUpdate|Whee::UserInterfaceState::NeedsDataClean\n");

    /* NeedsNodeClean is a superset of all others, so it should be printed
       alone */
    } {
        std::ostringstream out;
        Debug{&out} << (UserInterfaceState::NeedsNodeClean|UserInterfaceState::NeedsDataClean |UserInterfaceState::NeedsNodeUpdate|UserInterfaceState::NeedsDataUpdate|UserInterfaceState::NeedsDataAttachmentUpdate);
        CORRADE_COMPARE(out.str(), "Whee::UserInterfaceState::NeedsNodeClean\n");
    }
}

void AbstractUserInterfaceTest::constructNoCreate() {
    /* Currently, the only difference to the regular constructor is that the
       size vectors are zero */
    AbstractUserInterface ui{NoCreate};

    CORRADE_COMPARE(ui.size(), Vector2{});
    CORRADE_COMPARE(ui.windowSize(), Vector2{});
    CORRADE_COMPARE(ui.framebufferSize(), Vector2i{});

    CORRADE_COMPARE(ui.layerCapacity(), 0);
    CORRADE_COMPARE(ui.layerUsedCount(), 0);
    CORRADE_COMPARE(ui.layerFirst(), LayerHandle::Null);
    CORRADE_VERIFY(!ui.isHandleValid(LayerHandle::Null));

    CORRADE_COMPARE(ui.nodeCapacity(), 0);
    CORRADE_COMPARE(ui.nodeUsedCount(), 0);
    CORRADE_VERIFY(!ui.isHandleValid(NodeHandle::Null));

    CORRADE_COMPARE(ui.nodeOrderFirst(), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLast(), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 0);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 0);

    CORRADE_COMPARE(ui.dataAttachmentCount(), 0);
    CORRADE_VERIFY(!ui.isHandleValid(DataHandle::Null));
    CORRADE_VERIFY(!ui.isHandleValid(dataHandle(LayerHandle(0xffff), LayerDataHandle::Null)));
    CORRADE_VERIFY(!ui.isHandleValid(dataHandle(LayerHandle::Null, LayerDataHandle(0xffffffff))));
    CORRADE_VERIFY(!ui.isHandleValid(dataHandle(LayerHandle(0xffff), LayerDataHandle(0xffffffff))));

    CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
    CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);
}

void AbstractUserInterfaceTest::construct() {
    AbstractUserInterface ui{{100.0f, 150.0f}, {50.0f, 75.0f}, {200, 300}};

    CORRADE_COMPARE(ui.size(), (Vector2{100.0f, 150.0f}));
    CORRADE_COMPARE(ui.windowSize(), (Vector2{50.0f, 75.0f}));
    CORRADE_COMPARE(ui.framebufferSize(), (Vector2i{200, 300}));

    /* The constructor delegates to NoCreate, which is tested above */
}

void AbstractUserInterfaceTest::constructSingleSize() {
    AbstractUserInterface ui{{200, 300}};

    CORRADE_COMPARE(ui.size(), (Vector2{200.0f, 300.0f}));
    CORRADE_COMPARE(ui.windowSize(), (Vector2{200.0f, 300.0f}));
    CORRADE_COMPARE(ui.framebufferSize(), (Vector2i{200, 300}));

    /* The constructor delegates to NoCreate, which is tested above */
}

void AbstractUserInterfaceTest::constructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<AbstractUserInterface>{});
    CORRADE_VERIFY(!std::is_copy_assignable<AbstractUserInterface>{});
}

void AbstractUserInterfaceTest::constructMove() {
    AbstractUserInterface a{{100.0f, 150.0f}, {50.0f, 75.0f}, {200, 300}};
    a.createLayer();

    /* The class has an internal state struct containing everything, so it's
       not needed to test each and every property, yet this test is doing it
       for some unexplainable reason */
    AbstractUserInterface b{Utility::move(a)};
    CORRADE_COMPARE(b.size(), (Vector2{100.0f, 150.0f}));
    CORRADE_COMPARE(b.windowSize(), (Vector2{50.0f, 75.0f}));
    CORRADE_COMPARE(b.framebufferSize(), (Vector2i{200, 300}));
    CORRADE_COMPARE(b.layerCapacity(), 1);
    CORRADE_COMPARE(b.layerUsedCount(), 1);
    CORRADE_COMPARE(b.nodeCapacity(), 0);
    CORRADE_COMPARE(b.nodeUsedCount(), 0);
    CORRADE_COMPARE(b.dataAttachmentCount(), 0);

    AbstractUserInterface c{{10, 10}};
    c.createNode(NodeHandle::Null, {}, {}, {});
    c = Utility::move(b);
    CORRADE_COMPARE(c.size(), (Vector2{100.0f, 150.0f}));
    CORRADE_COMPARE(c.windowSize(), (Vector2{50.0f, 75.0f}));
    CORRADE_COMPARE(c.framebufferSize(), (Vector2i{200, 300}));
    CORRADE_COMPARE(c.layerCapacity(), 1);
    CORRADE_COMPARE(c.layerUsedCount(), 1);
    CORRADE_COMPARE(c.nodeCapacity(), 0);
    CORRADE_COMPARE(c.nodeUsedCount(), 0);
    CORRADE_COMPARE(c.dataAttachmentCount(), 0);

    CORRADE_VERIFY(std::is_nothrow_move_constructible<AbstractUserInterface>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<AbstractUserInterface>::value);
}

void AbstractUserInterfaceTest::layer() {
    AbstractUserInterface ui{{100, 100}};
    CORRADE_COMPARE(ui.layerCapacity(), 0);
    CORRADE_COMPARE(ui.layerUsedCount(), 0);
    CORRADE_COMPARE(ui.layerFirst(), LayerHandle::Null);
    CORRADE_COMPARE(ui.layerLast(), LayerHandle::Null);

    /* First layer ever */
    LayerHandle first = ui.createLayer();
    CORRADE_COMPARE(first, layerHandle(0, 1));
    CORRADE_VERIFY(ui.isHandleValid(first));
    CORRADE_COMPARE(ui.layerFirst(), first);
    CORRADE_COMPARE(ui.layerLast(), first);
    CORRADE_COMPARE(ui.layerPrevious(first), LayerHandle::Null);
    CORRADE_COMPARE(ui.layerNext(first), LayerHandle::Null);
    CORRADE_COMPARE(ui.layerCapacity(), 1);
    CORRADE_COMPARE(ui.layerUsedCount(), 1);

    /* Adding a layer at the end */
    LayerHandle second = ui.createLayer();
    CORRADE_COMPARE(second, layerHandle(1, 1));
    CORRADE_VERIFY(ui.isHandleValid(second));
    CORRADE_COMPARE(ui.layerFirst(), first);
    CORRADE_COMPARE(ui.layerLast(), second);
    CORRADE_COMPARE(ui.layerPrevious(first), LayerHandle::Null);
    CORRADE_COMPARE(ui.layerNext(first), second);
    CORRADE_COMPARE(ui.layerPrevious(second), first);
    CORRADE_COMPARE(ui.layerNext(second), LayerHandle::Null);
    CORRADE_COMPARE(ui.layerCapacity(), 2);
    CORRADE_COMPARE(ui.layerUsedCount(), 2);

    /* Adding a layer at the front */
    LayerHandle third = ui.createLayer(first);
    CORRADE_COMPARE(third, layerHandle(2, 1));
    CORRADE_VERIFY(ui.isHandleValid(third));
    CORRADE_COMPARE(ui.layerFirst(), third);
    CORRADE_COMPARE(ui.layerLast(), second);
    CORRADE_COMPARE(ui.layerPrevious(third), LayerHandle::Null);
    CORRADE_COMPARE(ui.layerNext(third), first);
    CORRADE_COMPARE(ui.layerPrevious(first), third);
    CORRADE_COMPARE(ui.layerNext(first), second);
    CORRADE_COMPARE(ui.layerPrevious(second), first);
    CORRADE_COMPARE(ui.layerNext(second), LayerHandle::Null);
    CORRADE_COMPARE(ui.layerCapacity(), 3);
    CORRADE_COMPARE(ui.layerUsedCount(), 3);

    /* Adding a layer in the middle */
    LayerHandle fourth = ui.createLayer(first);
    CORRADE_COMPARE(fourth, layerHandle(3, 1));
    CORRADE_VERIFY(ui.isHandleValid(fourth));
    CORRADE_COMPARE(ui.layerFirst(), third);
    CORRADE_COMPARE(ui.layerLast(), second);
    CORRADE_COMPARE(ui.layerPrevious(third), LayerHandle::Null);
    CORRADE_COMPARE(ui.layerNext(third), fourth);
    CORRADE_COMPARE(ui.layerPrevious(fourth), third);
    CORRADE_COMPARE(ui.layerNext(fourth), first);
    CORRADE_COMPARE(ui.layerPrevious(first), fourth);
    CORRADE_COMPARE(ui.layerNext(first), second);
    CORRADE_COMPARE(ui.layerPrevious(second), first);
    CORRADE_COMPARE(ui.layerNext(second), LayerHandle::Null);
    CORRADE_COMPARE(ui.layerCapacity(), 4);
    CORRADE_COMPARE(ui.layerUsedCount(), 4);

    /* Removing from the middle of the list */
    ui.removeLayer(first);
    CORRADE_COMPARE(ui.layerCapacity(), 4);
    CORRADE_COMPARE(ui.layerUsedCount(), 3);
    CORRADE_VERIFY(!ui.isHandleValid(first));
    CORRADE_COMPARE(ui.layerFirst(), third);
    CORRADE_COMPARE(ui.layerLast(), second);
    CORRADE_COMPARE(ui.layerPrevious(third), LayerHandle::Null);
    CORRADE_COMPARE(ui.layerNext(third), fourth);
    CORRADE_COMPARE(ui.layerPrevious(fourth), third);
    CORRADE_COMPARE(ui.layerNext(fourth), second);
    CORRADE_COMPARE(ui.layerPrevious(second), fourth);
    CORRADE_COMPARE(ui.layerNext(second), LayerHandle::Null);

    /* Removing from the back of the list */
    ui.removeLayer(second);
    CORRADE_COMPARE(ui.layerCapacity(), 4);
    CORRADE_COMPARE(ui.layerUsedCount(), 2);
    CORRADE_VERIFY(!ui.isHandleValid(second));
    CORRADE_COMPARE(ui.layerFirst(), third);
    CORRADE_COMPARE(ui.layerLast(), fourth);
    CORRADE_COMPARE(ui.layerPrevious(third), LayerHandle::Null);
    CORRADE_COMPARE(ui.layerNext(third), fourth);
    CORRADE_COMPARE(ui.layerPrevious(fourth), third);
    CORRADE_COMPARE(ui.layerNext(fourth), LayerHandle::Null);

    /* Removing from the front of the list */
    ui.removeLayer(third);
    CORRADE_COMPARE(ui.layerCapacity(), 4);
    CORRADE_COMPARE(ui.layerUsedCount(), 1);
    CORRADE_VERIFY(!ui.isHandleValid(third));
    CORRADE_COMPARE(ui.layerFirst(), fourth);
    CORRADE_COMPARE(ui.layerLast(), fourth);
    CORRADE_COMPARE(ui.layerPrevious(fourth), LayerHandle::Null);
    CORRADE_COMPARE(ui.layerNext(fourth), LayerHandle::Null);

    /* Removing the last layer */
    ui.removeLayer(fourth);
    CORRADE_COMPARE(ui.layerCapacity(), 4);
    CORRADE_COMPARE(ui.layerUsedCount(), 0);
    CORRADE_VERIFY(!ui.isHandleValid(fourth));
    CORRADE_COMPARE(ui.layerFirst(), LayerHandle::Null);
    CORRADE_COMPARE(ui.layerLast(), LayerHandle::Null);
}

void AbstractUserInterfaceTest::layerHandleRecycle() {
    AbstractUserInterface ui{{100, 100}};
    LayerHandle first = ui.createLayer();
    LayerHandle second = ui.createLayer();
    LayerHandle third = ui.createLayer();
    LayerHandle fourth = ui.createLayer();
    CORRADE_COMPARE(first, layerHandle(0, 1));
    CORRADE_COMPARE(second, layerHandle(1, 1));
    CORRADE_COMPARE(third, layerHandle(2, 1));
    CORRADE_COMPARE(fourth, layerHandle(3, 1));
    CORRADE_VERIFY(ui.isHandleValid(first));
    CORRADE_VERIFY(ui.isHandleValid(second));
    CORRADE_VERIFY(ui.isHandleValid(third));
    CORRADE_VERIFY(ui.isHandleValid(fourth));
    CORRADE_COMPARE(ui.layerCapacity(), 4);
    CORRADE_COMPARE(ui.layerUsedCount(), 4);

    /* Remove three out of the four in an arbitrary order */
    ui.removeLayer(second);
    ui.removeLayer(fourth);
    ui.removeLayer(first);
    CORRADE_VERIFY(!ui.isHandleValid(first));
    CORRADE_VERIFY(!ui.isHandleValid(second));
    CORRADE_VERIFY(ui.isHandleValid(third));
    CORRADE_VERIFY(!ui.isHandleValid(fourth));
    CORRADE_COMPARE(ui.layerCapacity(), 4);
    CORRADE_COMPARE(ui.layerUsedCount(), 1);

    /* Allocating new handles should recycle the handles in the order they were
       removed (oldest first) */
    LayerHandle second2 = ui.createLayer();
    LayerHandle fourth2 = ui.createLayer();
    LayerHandle first2 = ui.createLayer();
    CORRADE_COMPARE(first2, layerHandle(0, 2));
    CORRADE_COMPARE(second2, layerHandle(1, 2));
    CORRADE_COMPARE(fourth2, layerHandle(3, 2));
    CORRADE_COMPARE(ui.layerCapacity(), 4);
    CORRADE_COMPARE(ui.layerUsedCount(), 4);

    /* Old handles shouldn't get valid again */
    CORRADE_VERIFY(!ui.isHandleValid(first));
    CORRADE_VERIFY(ui.isHandleValid(first2));
    CORRADE_VERIFY(!ui.isHandleValid(second));
    CORRADE_VERIFY(ui.isHandleValid(second2));
    CORRADE_VERIFY(!ui.isHandleValid(fourth));
    CORRADE_VERIFY(ui.isHandleValid(fourth2));

    /* Removing a single handle and creating a new one directly reuses it if
       there's just one in the free list */
    ui.removeLayer(second2);
    LayerHandle second3 = ui.createLayer();
    CORRADE_COMPARE(second3, layerHandle(1, 3));
    CORRADE_VERIFY(!ui.isHandleValid(second));
    CORRADE_VERIFY(!ui.isHandleValid(second2));
    CORRADE_VERIFY(ui.isHandleValid(second3));
    CORRADE_COMPARE(ui.layerCapacity(), 4);
    CORRADE_COMPARE(ui.layerUsedCount(), 4);

    /* Allocating a new handle with the free list empty will grow it */
    LayerHandle fifth = ui.createLayer();
    CORRADE_COMPARE(fifth, layerHandle(4, 1));
    CORRADE_VERIFY(ui.isHandleValid(fifth));
    CORRADE_COMPARE(ui.layerCapacity(), 5);
    CORRADE_COMPARE(ui.layerUsedCount(), 5);
}

void AbstractUserInterfaceTest::layerHandleDisable() {
    AbstractUserInterface ui{{100, 100}};

    LayerHandle first = ui.createLayer();
    CORRADE_COMPARE(first, layerHandle(0, 1));

    for(std::size_t i = 0; i != (1 << Implementation::LayerHandleGenerationBits) - 1; ++i) {
        LayerHandle second = ui.createLayer();
        CORRADE_COMPARE(second, layerHandle(1, 1 + i));
        ui.removeLayer(second);
    }

    /* The generation for the second slot is exhausted so the handle is not
       recycled */
    CORRADE_COMPARE(ui.layerCapacity(), 2);
    CORRADE_COMPARE(ui.layerUsedCount(), 2);

    /* It shouldn't think a handle from the second slot with generation 0 is
       valid */
    CORRADE_VERIFY(!ui.isHandleValid(layerHandle(1, 0)));

    /* There's nowhere to create a new handle from so the capacity is grown */
    LayerHandle third = ui.createLayer();
    CORRADE_COMPARE(third, layerHandle(2, 1));
    CORRADE_COMPARE(ui.layerCapacity(), 3);
    CORRADE_COMPARE(ui.layerUsedCount(), 3);
}

void AbstractUserInterfaceTest::layerHandleLastFree() {
    AbstractUserInterface ui{{100, 100}};
    LayerHandle first = ui.createLayer();
    LayerHandle second = ui.createLayer();
    for(std::size_t i = 0; i != (1 << Implementation::LayerHandleIdBits) - 3; ++i)
        ui.createLayer();
    LayerHandle last = ui.createLayer();
    CORRADE_COMPARE(first, layerHandle(0, 1));
    CORRADE_COMPARE(second, layerHandle(1, 1));
    CORRADE_COMPARE(last, layerHandle(255, 1));
    CORRADE_COMPARE(ui.layerCapacity(), 256);
    CORRADE_COMPARE(ui.layerUsedCount(), 256);

    /* Removing the last layer should lead to one being marked as free, not 0
       due to 255 treated as "no more free layers" */
    ui.removeLayer(last);
    CORRADE_COMPARE(ui.layerCapacity(), 256);
    CORRADE_COMPARE(ui.layerUsedCount(), 255);

    /* Create a layer with ID 255 again */
    last = ui.createLayer();
    CORRADE_COMPARE(last, layerHandle(255, 2));

    /* Removing the three layers (with the one with ID 255 being in the middle)
       should mark all three as free, not just 2 due to 255 being treated as
       "no more free layers" */
    ui.removeLayer(first);
    ui.removeLayer(last);
    ui.removeLayer(second);
    CORRADE_COMPARE(ui.layerCapacity(), 256);
    CORRADE_COMPARE(ui.layerUsedCount(), 253);
}

void AbstractUserInterfaceTest::layerSetInstance() {
    int firstDestructed = 0;
    int secondDestructed = 0;

    {
        /* Size propagation to layers is tested thoroughly in setSize() */
        AbstractUserInterface ui{{100, 100}};
        LayerHandle first = ui.createLayer();
        LayerHandle second = ui.createLayer();
        LayerHandle third = ui.createLayer();

        struct Layer: AbstractLayer {
            explicit Layer(LayerHandle handle, int& destructed): AbstractLayer{handle}, destructed(destructed) {}

            ~Layer() {
                ++destructed;
            }

            LayerFeatures doFeatures() const override { return {}; }

            int& destructed;
        };

        Containers::Pointer<Layer> firstInstance{InPlaceInit, first, firstDestructed};
        Containers::Pointer<Layer> secondInstance{InPlaceInit, second, secondDestructed};
        /* Third deliberately doesn't have an instance set */
        Layer* firstInstancePointer = firstInstance.get();
        Layer* secondInstancePointer = secondInstance.get();
        /* Add them in different order, shouldn't matter */
        Layer& secondInstanceReference = ui.setLayerInstance(Utility::move(secondInstance));
        Layer& firstInstanceReference = ui.setLayerInstance(Utility::move(firstInstance));
        CORRADE_COMPARE(ui.layerCapacity(), 3);
        CORRADE_COMPARE(ui.layerUsedCount(), 3);
        CORRADE_COMPARE(&firstInstanceReference, firstInstancePointer);
        CORRADE_COMPARE(&secondInstanceReference, secondInstancePointer);
        CORRADE_COMPARE(&ui.layer(first), firstInstancePointer);
        CORRADE_COMPARE(&ui.layer(second), secondInstancePointer);
        CORRADE_COMPARE(&ui.layer<Layer>(first), firstInstancePointer);
        CORRADE_COMPARE(&ui.layer<Layer>(second), secondInstancePointer);
        CORRADE_COMPARE(firstDestructed, 0);
        CORRADE_COMPARE(secondDestructed, 0);

        /* Const overloads */
        const AbstractUserInterface& cui = ui;
        CORRADE_COMPARE(&cui.layer(first), firstInstancePointer);
        CORRADE_COMPARE(&cui.layer(second), secondInstancePointer);
        CORRADE_COMPARE(&cui.layer<Layer>(first), firstInstancePointer);
        CORRADE_COMPARE(&cui.layer<Layer>(second), secondInstancePointer);

        ui.removeLayer(first);
        CORRADE_COMPARE(firstDestructed, 1);
        CORRADE_COMPARE(secondDestructed, 0);

        /* Removing a layer that doesn't have any instance set shouldn't affect
           the others in any way */
        ui.removeLayer(third);
        CORRADE_COMPARE(firstDestructed, 1);
        CORRADE_COMPARE(secondDestructed, 0);
    }

    /* The remaining layer should be deleted at destruction */
    CORRADE_COMPARE(firstDestructed, 1);
    CORRADE_COMPARE(secondDestructed, 1);
}

void AbstractUserInterfaceTest::layerCreateInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui{{100, 100}};

    std::ostringstream out;
    Error redirectError{&out};
    ui.createLayer(LayerHandle(0xabcd));
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractUserInterface::createLayer(): invalid before handle Whee::LayerHandle(0xcd, 0xab)\n");
}

void AbstractUserInterfaceTest::layerSetInstanceInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    };

    AbstractUserInterface ui{{100, 100}};

    LayerHandle handle = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<Layer>(handle));

    std::ostringstream out;
    Error redirectError{&out};
    ui.setLayerInstance(nullptr);
    ui.setLayerInstance(Containers::pointer<Layer>(LayerHandle(0xabcd)));
    ui.setLayerInstance(Containers::pointer<Layer>(handle));
    CORRADE_COMPARE_AS(out.str(),
        "Whee::AbstractUserInterface::setLayerInstance(): instance is null\n"
        "Whee::AbstractUserInterface::setLayerInstance(): invalid handle Whee::LayerHandle(0xcd, 0xab)\n"
        "Whee::AbstractUserInterface::setLayerInstance(): instance for Whee::LayerHandle(0x0, 0x1) already set\n",
        TestSuite::Compare::String);
}

void AbstractUserInterfaceTest::layerGetInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    };

    AbstractUserInterface ui{{100, 100}};
    /* Need at least one layer to be present so layer() asserts can return
       something */
    ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer()));

    const AbstractUserInterface& cui = ui;

    LayerHandle handle = ui.createLayer();

    std::ostringstream out;
    Error redirectError{&out};
    ui.layerPrevious(LayerHandle(0x12ab));
    ui.layerPrevious(LayerHandle::Null);
    ui.layerNext(LayerHandle(0x12ab));
    ui.layerNext(LayerHandle::Null);
    ui.layer(handle);
    ui.layer(LayerHandle::Null);
    /* Const overloads */
    cui.layer(handle);
    cui.layer(LayerHandle::Null);
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractUserInterface::layerPrevious(): invalid handle Whee::LayerHandle(0xab, 0x12)\n"
        "Whee::AbstractUserInterface::layerPrevious(): invalid handle Whee::LayerHandle::Null\n"
        "Whee::AbstractUserInterface::layerNext(): invalid handle Whee::LayerHandle(0xab, 0x12)\n"
        "Whee::AbstractUserInterface::layerNext(): invalid handle Whee::LayerHandle::Null\n"
        "Whee::AbstractUserInterface::layer(): Whee::LayerHandle(0x1, 0x1) has no instance set\n"
        "Whee::AbstractUserInterface::layer(): invalid handle Whee::LayerHandle::Null\n"
        "Whee::AbstractUserInterface::layer(): Whee::LayerHandle(0x1, 0x1) has no instance set\n"
        "Whee::AbstractUserInterface::layer(): invalid handle Whee::LayerHandle::Null\n");
}

void AbstractUserInterfaceTest::layerRemoveInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui{{100, 100}};

    std::ostringstream out;
    Error redirectError{&out};
    ui.removeLayer(LayerHandle::Null);
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractUserInterface::removeLayer(): invalid handle Whee::LayerHandle::Null\n");
}

void AbstractUserInterfaceTest::layerNoHandlesLeft() {
    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui{{100, 100}};

    LayerHandle handle;
    for(std::size_t i = 0; i != 1 << Implementation::LayerHandleIdBits; ++i)
        handle = ui.createLayer();
    CORRADE_COMPARE(handle, layerHandle((1 << Implementation::LayerHandleIdBits) - 1, 1));

    CORRADE_COMPARE(ui.layerCapacity(), 1 << Implementation::LayerHandleIdBits);
    CORRADE_COMPARE(ui.layerUsedCount(), 1 << Implementation::LayerHandleIdBits);

    std::ostringstream out;
    Error redirectError{&out};
    ui.createLayer();
    /* Number is hardcoded in the expected message but not elsewhere in order
       to give a heads-up when modifying the handle ID bit count */
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractUserInterface::createLayer(): can only have at most 256 layers\n");
}

void AbstractUserInterfaceTest::node() {
    AbstractUserInterface ui{{100, 100}};
    CORRADE_COMPARE(ui.nodeCapacity(), 0);
    CORRADE_COMPARE(ui.nodeUsedCount(), 0);

    NodeHandle first = ui.createNode({1.0f, 2.0f}, {3.0f, 4.0f});
    CORRADE_COMPARE(first, nodeHandle(0, 1));
    CORRADE_VERIFY(ui.isHandleValid(first));
    CORRADE_COMPARE(ui.nodeParent(first), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOffset(first), (Vector2{1.0f, 2.0f}));
    CORRADE_COMPARE(ui.nodeSize(first), (Vector2{3.0f, 4.0f}));
    CORRADE_COMPARE(ui.nodeFlags(first), NodeFlags{});
    CORRADE_COMPARE(ui.nodeCapacity(), 1);
    CORRADE_COMPARE(ui.nodeUsedCount(), 1);

    NodeHandle second = ui.createNode({5.0f, 6.0f}, {7.0f, 8.0f}, NodeFlag::Hidden);
    CORRADE_COMPARE(second, nodeHandle(1, 1));
    CORRADE_VERIFY(ui.isHandleValid(second));
    CORRADE_COMPARE(ui.nodeParent(second), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOffset(second), (Vector2{5.0f, 6.0f}));
    CORRADE_COMPARE(ui.nodeSize(second), (Vector2{7.0f, 8.0f}));
    CORRADE_COMPARE(ui.nodeFlags(second), NodeFlag::Hidden);
    CORRADE_COMPARE(ui.nodeCapacity(), 2);
    CORRADE_COMPARE(ui.nodeUsedCount(), 2);

    NodeHandle third = ui.createNode(first, {9.0f, 0.0f}, {-1.0f, -2.0f}, NodeFlags{0xe0});
    CORRADE_COMPARE(third, nodeHandle(2, 1));
    CORRADE_VERIFY(ui.isHandleValid(third));
    CORRADE_COMPARE(ui.nodeParent(third), first);
    CORRADE_COMPARE(ui.nodeOffset(third), (Vector2{9.0f, 0.0f}));
    CORRADE_COMPARE(ui.nodeSize(third), (Vector2{-1.0f, -2.0f}));
    CORRADE_COMPARE(ui.nodeFlags(third), NodeFlags{0xe0});
    CORRADE_COMPARE(ui.nodeCapacity(), 3);
    CORRADE_COMPARE(ui.nodeUsedCount(), 3);

    ui.removeNode(first);
    CORRADE_COMPARE(ui.nodeCapacity(), 3);
    CORRADE_COMPARE(ui.nodeUsedCount(), 2);
    CORRADE_VERIFY(!ui.isHandleValid(first));
    CORRADE_VERIFY(ui.isHandleValid(second));
    /* The nested node isn't removed immediately, only during next clean() --
       tested in cleanRemoveNestedNodes() below -- which also implies its
       parent handle is invalid now */
    CORRADE_VERIFY(ui.isHandleValid(third));
    CORRADE_COMPARE(ui.nodeParent(third), first);
}

void AbstractUserInterfaceTest::nodeHandleRecycle() {
    AbstractUserInterface ui{{100, 100}};
    NodeHandle first = ui.createNode({}, {});
    NodeHandle second = ui.createNode({}, {});
    NodeHandle third = ui.createNode({}, {});
    NodeHandle fourth = ui.createNode({}, {});
    CORRADE_COMPARE(first, nodeHandle(0, 1));
    CORRADE_COMPARE(second, nodeHandle(1, 1));
    CORRADE_COMPARE(third, nodeHandle(2, 1));
    CORRADE_COMPARE(fourth, nodeHandle(3, 1));
    CORRADE_VERIFY(ui.isHandleValid(first));
    CORRADE_VERIFY(ui.isHandleValid(second));
    CORRADE_VERIFY(ui.isHandleValid(third));
    CORRADE_VERIFY(ui.isHandleValid(fourth));
    CORRADE_COMPARE(ui.nodeCapacity(), 4);
    CORRADE_COMPARE(ui.nodeUsedCount(), 4);

    /* Remove three out of the four in an arbitrary order */
    ui.removeNode(fourth);
    ui.removeNode(first);
    ui.removeNode(third);
    CORRADE_VERIFY(!ui.isHandleValid(first));
    CORRADE_VERIFY(ui.isHandleValid(second));
    CORRADE_VERIFY(!ui.isHandleValid(third));
    CORRADE_VERIFY(!ui.isHandleValid(fourth));
    CORRADE_COMPARE(ui.nodeCapacity(), 4);
    CORRADE_COMPARE(ui.nodeUsedCount(), 1);

    /* Allocating new handles should recycle the handles in the order they were
       removed (oldest first) */
    NodeHandle fourth2 = ui.createNode({}, {});
    NodeHandle first2 = ui.createNode({}, {});
    NodeHandle third2 = ui.createNode({}, {});
    CORRADE_COMPARE(first2, nodeHandle(0, 2));
    CORRADE_COMPARE(third2, nodeHandle(2, 2));
    CORRADE_COMPARE(fourth2, nodeHandle(3, 2));
    CORRADE_COMPARE(ui.nodeCapacity(), 4);
    CORRADE_COMPARE(ui.nodeUsedCount(), 4);

    /* Old handles shouldn't get valid again */
    CORRADE_VERIFY(!ui.isHandleValid(first));
    CORRADE_VERIFY(ui.isHandleValid(first2));
    CORRADE_VERIFY(!ui.isHandleValid(third));
    CORRADE_VERIFY(ui.isHandleValid(third2));
    CORRADE_VERIFY(!ui.isHandleValid(fourth));
    CORRADE_VERIFY(ui.isHandleValid(fourth2));

    /* Removing a single handle and creating a new one directly reuses it if
       there's just one in the free list */
    ui.removeNode(third2);
    NodeHandle third3 = ui.createNode({}, {});
    CORRADE_COMPARE(third3, nodeHandle(2, 3));
    CORRADE_VERIFY(!ui.isHandleValid(third));
    CORRADE_VERIFY(!ui.isHandleValid(third2));
    CORRADE_VERIFY(ui.isHandleValid(third3));
    CORRADE_COMPARE(ui.nodeCapacity(), 4);
    CORRADE_COMPARE(ui.nodeUsedCount(), 4);

    /* Allocating a new handle with the free list empty will grow it */
    NodeHandle fifth = ui.createNode({}, {});
    CORRADE_COMPARE(fifth, nodeHandle(4, 1));
    CORRADE_VERIFY(ui.isHandleValid(fifth));
    CORRADE_COMPARE(ui.nodeCapacity(), 5);
    CORRADE_COMPARE(ui.nodeUsedCount(), 5);
}

void AbstractUserInterfaceTest::nodeHandleDisable() {
    AbstractUserInterface ui{{100, 100}};

    NodeHandle first = ui.createNode({}, {});
    CORRADE_COMPARE(first, nodeHandle(0, 1));

    for(std::size_t i = 0; i != (1 << Implementation::NodeHandleGenerationBits) - 1; ++i) {
        NodeHandle second = ui.createNode({}, {});
        CORRADE_COMPARE(second, nodeHandle(1, 1 + i));
        ui.removeNode(second);
    }

    /* The generation for the second slot is exhausted so the handle is not
       recycled */
    CORRADE_COMPARE(ui.nodeCapacity(), 2);
    CORRADE_COMPARE(ui.nodeUsedCount(), 2);

    /* It shouldn't think a handle from the second slot with generation 0 is
       valid */
    CORRADE_VERIFY(!ui.isHandleValid(nodeHandle(1, 0)));

    /* There's nowhere to create a new handle from so the capacity is grown */
    NodeHandle third = ui.createNode({}, {});
    CORRADE_COMPARE(third, nodeHandle(2, 1));
    CORRADE_COMPARE(ui.nodeCapacity(), 3);
    CORRADE_COMPARE(ui.nodeUsedCount(), 3);
}

void AbstractUserInterfaceTest::nodeFlags() {
    AbstractUserInterface ui{{100, 100}};

    /* Add more than one handle to verify the correct one gets updated and not
       always the first */
    NodeHandle another = ui.createNode({}, {});
    CORRADE_COMPARE(ui.nodeFlags(another), NodeFlags{});

    NodeHandle node = ui.createNode({}, {}, NodeFlag::Hidden);
    CORRADE_COMPARE(ui.nodeFlags(node), NodeFlag::Hidden);

    ui.setNodeFlags(node, NodeFlags{0xe0});
    CORRADE_COMPARE(ui.nodeFlags(node), NodeFlags{0xe0});

    ui.addNodeFlags(node, NodeFlag::Hidden);
    CORRADE_COMPARE(ui.nodeFlags(node), NodeFlags{0xe0}|NodeFlag::Hidden);

    ui.clearNodeFlags(node, NodeFlags{0xe0});
    CORRADE_COMPARE(ui.nodeFlags(node), NodeFlag::Hidden);

    CORRADE_COMPARE(ui.nodeFlags(another), NodeFlags{});
}

void AbstractUserInterfaceTest::nodeCreateInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui{{100, 100}};

    std::ostringstream out;
    Error redirectError{&out};
    ui.createNode(NodeHandle(0x123abcde), {}, {});
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractUserInterface::createNode(): invalid parent handle Whee::NodeHandle(0xabcde, 0x123)\n");
}

void AbstractUserInterfaceTest::nodeGetSetInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui{{100, 100}};

    std::ostringstream out;
    Error redirectError{&out};
    ui.nodeParent(NodeHandle(0x123abcde));
    ui.nodeOffset(NodeHandle(0x123abcde));
    ui.nodeSize(NodeHandle(0x123abcde));
    ui.nodeFlags(NodeHandle(0x123abcde));
    ui.setNodeOffset(NodeHandle(0x123abcde), {});
    ui.setNodeSize(NodeHandle(0x123abcde), {});
    ui.setNodeFlags(NodeHandle(0x123abcde), {});
    ui.addNodeFlags(NodeHandle(0x123abcde), {});
    ui.clearNodeFlags(NodeHandle(0x123abcde), {});
    CORRADE_COMPARE_AS(out.str(),
        "Whee::AbstractUserInterface::nodeParent(): invalid handle Whee::NodeHandle(0xabcde, 0x123)\n"
        "Whee::AbstractUserInterface::nodeOffset(): invalid handle Whee::NodeHandle(0xabcde, 0x123)\n"
        "Whee::AbstractUserInterface::nodeSize(): invalid handle Whee::NodeHandle(0xabcde, 0x123)\n"
        "Whee::AbstractUserInterface::nodeFlags(): invalid handle Whee::NodeHandle(0xabcde, 0x123)\n"
        "Whee::AbstractUserInterface::setNodeOffset(): invalid handle Whee::NodeHandle(0xabcde, 0x123)\n"
        "Whee::AbstractUserInterface::setNodeSize(): invalid handle Whee::NodeHandle(0xabcde, 0x123)\n"
        "Whee::AbstractUserInterface::setNodeFlags(): invalid handle Whee::NodeHandle(0xabcde, 0x123)\n"
        "Whee::AbstractUserInterface::addNodeFlags(): invalid handle Whee::NodeHandle(0xabcde, 0x123)\n"
        "Whee::AbstractUserInterface::clearNodeFlags(): invalid handle Whee::NodeHandle(0xabcde, 0x123)\n",
        TestSuite::Compare::String);
}

void AbstractUserInterfaceTest::nodeRemoveInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui{{100, 100}};

    std::ostringstream out;
    Error redirectError{&out};
    ui.removeNode(NodeHandle::Null);
    ui.removeNode(NodeHandle(0x123abcde));
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractUserInterface::removeNode(): invalid handle Whee::NodeHandle::Null\n"
        "Whee::AbstractUserInterface::removeNode(): invalid handle Whee::NodeHandle(0xabcde, 0x123)\n");
}

void AbstractUserInterfaceTest::nodeNoHandlesLeft() {
    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui{{100, 100}};

    NodeHandle handle;
    for(std::size_t i = 0; i != 1 << Implementation::NodeHandleIdBits; ++i)
        handle = ui.createNode(NodeHandle::Null, {}, {}, {});
    CORRADE_COMPARE(handle, nodeHandle((1 << Implementation::NodeHandleIdBits) - 1, 1));

    CORRADE_COMPARE(ui.nodeCapacity(), 1 << Implementation::NodeHandleIdBits);
    CORRADE_COMPARE(ui.nodeUsedCount(), 1 << Implementation::NodeHandleIdBits);

    std::ostringstream out;
    Error redirectError{&out};
    ui.createNode(NodeHandle::Null, {}, {}, {});
    /* Number is hardcoded in the expected message but not elsewhere in order
       to give a heads-up when modifying the handle ID bit count */
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractUserInterface::createNode(): can only have at most 1048576 nodes\n");
}

void AbstractUserInterfaceTest::nodeOrder() {
    AbstractUserInterface ui{{100, 100}};
    CORRADE_COMPARE(ui.nodeOrderFirst(), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLast(), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 0);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 0);

    NodeHandle first = ui.createNode({}, {});
    CORRADE_COMPARE(ui.nodeParent(first), NodeHandle::Null);
    CORRADE_VERIFY(ui.isNodeOrdered(first));
    CORRADE_COMPARE(ui.nodeOrderPrevious(first), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(first), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderFirst(), first);
    CORRADE_COMPARE(ui.nodeOrderLast(), first);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 1);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 1);

    /* Hidden doesn't have any effect on being included in the order, it's an
       orthogonal feature */
    NodeHandle second = ui.createNode({5.0f, 6.0f}, {7.0f, 8.0f}, NodeFlag::Hidden);
    CORRADE_COMPARE(ui.nodeParent(second), NodeHandle::Null);
    CORRADE_VERIFY(ui.isNodeOrdered(first));
    CORRADE_COMPARE(ui.nodeOrderPrevious(first), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(first), second);
    CORRADE_VERIFY(ui.isNodeOrdered(second));
    CORRADE_COMPARE(ui.nodeOrderPrevious(second), first);
    CORRADE_COMPARE(ui.nodeOrderNext(second), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderFirst(), first);
    CORRADE_COMPARE(ui.nodeOrderLast(), second);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 2);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 2);

    NodeHandle third = ui.createNode(first, {}, {});
    CORRADE_COMPARE(ui.nodeParent(third), first);
    /* Not a root node, so not added to the order. The original order
       stays. */
    CORRADE_VERIFY(ui.isNodeOrdered(first));
    CORRADE_COMPARE(ui.nodeOrderPrevious(first), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(first), second);
    CORRADE_VERIFY(ui.isNodeOrdered(second));
    CORRADE_COMPARE(ui.nodeOrderPrevious(second), first);
    CORRADE_COMPARE(ui.nodeOrderNext(second), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderFirst(), first);
    CORRADE_COMPARE(ui.nodeOrderLast(), second);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 2);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 2);

    NodeHandle fourth = ui.createNode({}, {});
    CORRADE_COMPARE(ui.nodeParent(fourth), NodeHandle::Null);
    CORRADE_VERIFY(ui.isNodeOrdered(first));
    CORRADE_COMPARE(ui.nodeOrderPrevious(first), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(first), second);
    CORRADE_VERIFY(ui.isNodeOrdered(second));
    CORRADE_COMPARE(ui.nodeOrderPrevious(second), first);
    CORRADE_COMPARE(ui.nodeOrderNext(second), fourth);
    CORRADE_VERIFY(ui.isNodeOrdered(fourth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fourth), second);
    CORRADE_COMPARE(ui.nodeOrderNext(fourth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderFirst(), first);
    CORRADE_COMPARE(ui.nodeOrderLast(), fourth);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 3);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 3);

    NodeHandle fifth = ui.createNode({}, {});
    CORRADE_COMPARE(ui.nodeParent(fifth), NodeHandle::Null);
    CORRADE_VERIFY(ui.isNodeOrdered(first));
    CORRADE_COMPARE(ui.nodeOrderPrevious(first), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(first), second);
    CORRADE_VERIFY(ui.isNodeOrdered(second));
    CORRADE_COMPARE(ui.nodeOrderPrevious(second), first);
    CORRADE_COMPARE(ui.nodeOrderNext(second), fourth);
    CORRADE_VERIFY(ui.isNodeOrdered(fourth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fourth), second);
    CORRADE_COMPARE(ui.nodeOrderNext(fourth), fifth);
    CORRADE_VERIFY(ui.isNodeOrdered(fifth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fifth), fourth);
    CORRADE_COMPARE(ui.nodeOrderNext(fifth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderFirst(), first);
    CORRADE_COMPARE(ui.nodeOrderLast(), fifth);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 4);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 4);

    /* Clearing from a middle of the list */
    ui.clearNodeOrder(second);
    CORRADE_VERIFY(!ui.isNodeOrdered(second));
    CORRADE_COMPARE(ui.nodeOrderPrevious(second), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(second), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 4);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 3);
    /* THe rest stays connected */
    CORRADE_VERIFY(ui.isNodeOrdered(first));
    CORRADE_COMPARE(ui.nodeOrderPrevious(first), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(first), fourth);
    CORRADE_VERIFY(ui.isNodeOrdered(fourth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fourth), first);
    CORRADE_COMPARE(ui.nodeOrderNext(fourth), fifth);
    CORRADE_VERIFY(ui.isNodeOrdered(fifth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fifth), fourth);
    CORRADE_COMPARE(ui.nodeOrderNext(fifth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderFirst(), first);
    CORRADE_COMPARE(ui.nodeOrderLast(), fifth);

    /* Clearing from the back of the list */
    ui.clearNodeOrder(first);
    CORRADE_VERIFY(!ui.isNodeOrdered(first));
    CORRADE_COMPARE(ui.nodeOrderPrevious(first), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(first), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 4);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 2);
    /* THe rest stays connected */
    CORRADE_VERIFY(ui.isNodeOrdered(fourth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fourth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(fourth), fifth);
    CORRADE_VERIFY(ui.isNodeOrdered(fifth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fifth), fourth);
    CORRADE_COMPARE(ui.nodeOrderNext(fifth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderFirst(), fourth);
    CORRADE_COMPARE(ui.nodeOrderLast(), fifth);

    /* Clearing from the front of the list */
    ui.clearNodeOrder(fifth);
    CORRADE_VERIFY(!ui.isNodeOrdered(fifth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fifth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(fifth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 4);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 1);
    /* THe remaining node stays */
    CORRADE_VERIFY(ui.isNodeOrdered(fourth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fourth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(fourth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderFirst(), fourth);
    CORRADE_COMPARE(ui.nodeOrderLast(), fourth);

    /* Clearing the last node */
    ui.clearNodeOrder(fourth);
    CORRADE_VERIFY(!ui.isNodeOrdered(fourth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fourth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(fourth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderFirst(), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLast(), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 4);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 0);

    /* Clearing a node that isn't connected is a no-op */
    ui.clearNodeOrder(second);
    CORRADE_VERIFY(!ui.isNodeOrdered(second));
    CORRADE_COMPARE(ui.nodeOrderPrevious(second), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(second), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 4);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 0);

    /* Setting node order into a pre-allocated capacity. There's no other node
       in the order right now so it's both first and last */
    ui.setNodeOrder(fifth, NodeHandle::Null);
    CORRADE_VERIFY(ui.isNodeOrdered(fifth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fifth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(fifth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderFirst(), fifth);
    CORRADE_COMPARE(ui.nodeOrderLast(), fifth);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 4);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 1);

    /* Setting node order as last again, this time it expands a single-item
       list */
    ui.setNodeOrder(second, NodeHandle::Null);
    CORRADE_VERIFY(ui.isNodeOrdered(fifth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fifth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(fifth), second);
    CORRADE_VERIFY(ui.isNodeOrdered(second));
    CORRADE_COMPARE(ui.nodeOrderPrevious(second), fifth);
    CORRADE_COMPARE(ui.nodeOrderNext(second), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderFirst(), fifth);
    CORRADE_COMPARE(ui.nodeOrderLast(), second);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 4);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 2);

    /* Setting node order in the middle, just different order than before */
    ui.setNodeOrder(first, second);
    CORRADE_VERIFY(ui.isNodeOrdered(fifth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fifth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(fifth), first);
    CORRADE_VERIFY(ui.isNodeOrdered(first));
    CORRADE_COMPARE(ui.nodeOrderPrevious(first), fifth);
    CORRADE_COMPARE(ui.nodeOrderNext(first), second);
    CORRADE_VERIFY(ui.isNodeOrdered(second));
    CORRADE_COMPARE(ui.nodeOrderPrevious(second), first);
    CORRADE_COMPARE(ui.nodeOrderNext(second), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderFirst(), fifth);
    CORRADE_COMPARE(ui.nodeOrderLast(), second);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 4);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 3);

    /* Setting node order first. This is what was already tested several times
       with the initial node addition, this time it's just with pre-allocated
       capacity, so the next setting would have to grow the capacity again. */
    ui.setNodeOrder(fourth, fifth);
    CORRADE_VERIFY(ui.isNodeOrdered(fourth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fourth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(fourth), fifth);
    CORRADE_VERIFY(ui.isNodeOrdered(fifth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fifth), fourth);
    CORRADE_COMPARE(ui.nodeOrderNext(fifth), first);
    CORRADE_VERIFY(ui.isNodeOrdered(first));
    CORRADE_COMPARE(ui.nodeOrderPrevious(first), fifth);
    CORRADE_COMPARE(ui.nodeOrderNext(first), second);
    CORRADE_VERIFY(ui.isNodeOrdered(second));
    CORRADE_COMPARE(ui.nodeOrderPrevious(second), first);
    CORRADE_COMPARE(ui.nodeOrderNext(second), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderFirst(), fourth);
    CORRADE_COMPARE(ui.nodeOrderLast(), second);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 4);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 4);

    /* Swapping two node next to each other in the middle. Internally it should
       be a clear & set operation, thus what was tested above already. */
    ui.setNodeOrder(first, fifth);
    CORRADE_VERIFY(ui.isNodeOrdered(fourth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fourth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(fourth), first);
    CORRADE_VERIFY(ui.isNodeOrdered(first));
    CORRADE_COMPARE(ui.nodeOrderPrevious(first), fourth);
    CORRADE_COMPARE(ui.nodeOrderNext(first), fifth);
    CORRADE_VERIFY(ui.isNodeOrdered(fifth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fifth), first);
    CORRADE_COMPARE(ui.nodeOrderNext(fifth), second);
    CORRADE_VERIFY(ui.isNodeOrdered(second));
    CORRADE_COMPARE(ui.nodeOrderPrevious(second), fifth);
    CORRADE_COMPARE(ui.nodeOrderNext(second), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderFirst(), fourth);
    CORRADE_COMPARE(ui.nodeOrderLast(), second);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 4);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 4);

    /* Resetting a node from the back to the front ... */
    ui.setNodeOrder(second, fourth);
    CORRADE_VERIFY(ui.isNodeOrdered(second));
    CORRADE_COMPARE(ui.nodeOrderPrevious(second), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(second), fourth);
    CORRADE_VERIFY(ui.isNodeOrdered(fourth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fourth), second);
    CORRADE_COMPARE(ui.nodeOrderNext(fourth), first);
    CORRADE_VERIFY(ui.isNodeOrdered(first));
    CORRADE_COMPARE(ui.nodeOrderPrevious(first), fourth);
    CORRADE_COMPARE(ui.nodeOrderNext(first), fifth);
    CORRADE_VERIFY(ui.isNodeOrdered(fifth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fifth), first);
    CORRADE_COMPARE(ui.nodeOrderNext(fifth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderFirst(), second);
    CORRADE_COMPARE(ui.nodeOrderLast(), fifth);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 4);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 4);

    /* ... and back, results in the same order as before */
    ui.setNodeOrder(second, NodeHandle::Null);
    CORRADE_VERIFY(ui.isNodeOrdered(fourth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fourth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(fourth), first);
    CORRADE_VERIFY(ui.isNodeOrdered(first));
    CORRADE_COMPARE(ui.nodeOrderPrevious(first), fourth);
    CORRADE_COMPARE(ui.nodeOrderNext(first), fifth);
    CORRADE_VERIFY(ui.isNodeOrdered(fifth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fifth), first);
    CORRADE_COMPARE(ui.nodeOrderNext(fifth), second);
    CORRADE_VERIFY(ui.isNodeOrdered(second));
    CORRADE_COMPARE(ui.nodeOrderPrevious(second), fifth);
    CORRADE_COMPARE(ui.nodeOrderNext(second), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderFirst(), fourth);
    CORRADE_COMPARE(ui.nodeOrderLast(), second);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 4);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 4);

    /* Adding a new node grows the capacity again */
    NodeHandle sixth = ui.createNode({}, {});
    CORRADE_VERIFY(ui.isNodeOrdered(fourth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fourth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(fourth), first);
    CORRADE_VERIFY(ui.isNodeOrdered(first));
    CORRADE_COMPARE(ui.nodeOrderPrevious(first), fourth);
    CORRADE_COMPARE(ui.nodeOrderNext(first), fifth);
    CORRADE_VERIFY(ui.isNodeOrdered(fifth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fifth), first);
    CORRADE_COMPARE(ui.nodeOrderNext(fifth), second);
    CORRADE_VERIFY(ui.isNodeOrdered(second));
    CORRADE_COMPARE(ui.nodeOrderPrevious(second), fifth);
    CORRADE_COMPARE(ui.nodeOrderNext(second), sixth);
    CORRADE_VERIFY(ui.isNodeOrdered(sixth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(sixth), second);
    CORRADE_COMPARE(ui.nodeOrderNext(sixth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderFirst(), fourth);
    CORRADE_COMPARE(ui.nodeOrderLast(), sixth);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 5);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 5);

    /* Removing a node implicitly calls clearNodeOrder() */
    ui.removeNode(first);
    CORRADE_VERIFY(ui.isNodeOrdered(fourth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fourth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(fourth), fifth);
    CORRADE_VERIFY(ui.isNodeOrdered(fifth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fifth), fourth);
    CORRADE_COMPARE(ui.nodeOrderNext(fifth), second);
    CORRADE_VERIFY(ui.isNodeOrdered(second));
    CORRADE_COMPARE(ui.nodeOrderPrevious(second), fifth);
    CORRADE_COMPARE(ui.nodeOrderNext(second), sixth);
    CORRADE_VERIFY(ui.isNodeOrdered(sixth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(sixth), second);
    CORRADE_COMPARE(ui.nodeOrderNext(sixth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderFirst(), fourth);
    CORRADE_COMPARE(ui.nodeOrderLast(), sixth);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 5);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 4);
}

void AbstractUserInterfaceTest::nodeOrderGetSetInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui{{100, 100}};
    NodeHandle inOrder = ui.createNode({}, {});
    CORRADE_VERIFY(ui.isNodeOrdered(inOrder));

    NodeHandle child = ui.createNode(inOrder, {}, {});
    CORRADE_COMPARE(ui.nodeParent(child), inOrder);

    NodeHandle notInOrder = ui.createNode({}, {});
    ui.clearNodeOrder(notInOrder);
    CORRADE_VERIFY(!ui.isNodeOrdered(notInOrder));

    std::ostringstream out;
    Error redirectError{&out};
    ui.isNodeOrdered(NodeHandle::Null);
    ui.isNodeOrdered(NodeHandle(0x123abcde));
    ui.isNodeOrdered(child);
    ui.nodeOrderPrevious(NodeHandle::Null);
    ui.nodeOrderPrevious(NodeHandle(0x123abcde));
    ui.nodeOrderPrevious(child);
    ui.nodeOrderNext(NodeHandle::Null);
    ui.nodeOrderNext(NodeHandle(0x123abcde));
    ui.nodeOrderNext(child);
    ui.setNodeOrder(NodeHandle::Null, NodeHandle::Null);
    ui.setNodeOrder(NodeHandle(0x123abcde), NodeHandle::Null);
    ui.setNodeOrder(inOrder, NodeHandle(0x123abcde));
    ui.setNodeOrder(child, NodeHandle::Null);
    ui.setNodeOrder(inOrder, notInOrder);
    ui.setNodeOrder(inOrder, inOrder);
    ui.clearNodeOrder(NodeHandle(0x123abcde));
    ui.clearNodeOrder(NodeHandle::Null);
    ui.clearNodeOrder(child);
    CORRADE_COMPARE_AS(out.str(),
        "Whee::AbstractUserInterface::isNodeOrdered(): invalid handle Whee::NodeHandle::Null\n"
        "Whee::AbstractUserInterface::isNodeOrdered(): invalid handle Whee::NodeHandle(0xabcde, 0x123)\n"
        "Whee::AbstractUserInterface::isNodeOrdered(): Whee::NodeHandle(0x1, 0x1) is not a root node\n"
        "Whee::AbstractUserInterface::nodeOrderPrevious(): invalid handle Whee::NodeHandle::Null\n"
        "Whee::AbstractUserInterface::nodeOrderPrevious(): invalid handle Whee::NodeHandle(0xabcde, 0x123)\n"
        "Whee::AbstractUserInterface::nodeOrderPrevious(): Whee::NodeHandle(0x1, 0x1) is not a root node\n"
        "Whee::AbstractUserInterface::nodeOrderNext(): invalid handle Whee::NodeHandle::Null\n"
        "Whee::AbstractUserInterface::nodeOrderNext(): invalid handle Whee::NodeHandle(0xabcde, 0x123)\n"
        "Whee::AbstractUserInterface::nodeOrderNext(): Whee::NodeHandle(0x1, 0x1) is not a root node\n"
        "Whee::AbstractUserInterface::setNodeOrder(): invalid handle Whee::NodeHandle::Null\n"
        "Whee::AbstractUserInterface::setNodeOrder(): invalid handle Whee::NodeHandle(0xabcde, 0x123)\n"
        "Whee::AbstractUserInterface::setNodeOrder(): invalid before handle Whee::NodeHandle(0xabcde, 0x123)\n"
        "Whee::AbstractUserInterface::setNodeOrder(): Whee::NodeHandle(0x1, 0x1) is not a root node\n"
        "Whee::AbstractUserInterface::setNodeOrder(): Whee::NodeHandle(0x2, 0x1) is not ordered\n"
        "Whee::AbstractUserInterface::setNodeOrder(): can't order Whee::NodeHandle(0x0, 0x1) before itself\n"
        "Whee::AbstractUserInterface::clearNodeOrder(): invalid handle Whee::NodeHandle(0xabcde, 0x123)\n"
        "Whee::AbstractUserInterface::clearNodeOrder(): invalid handle Whee::NodeHandle::Null\n"
        "Whee::AbstractUserInterface::clearNodeOrder(): Whee::NodeHandle(0x1, 0x1) is not a root node\n",
        TestSuite::Compare::String);
}

void AbstractUserInterfaceTest::data() {
    /* Event/framebuffer scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};
    CORRADE_COMPARE(ui.dataAttachmentCount(), 0);

    LayerHandle layerHandle = ui.createLayer();

    /* Data handles tested thoroughly in AbstractLayerTest already */
    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    };
    Containers::Pointer<Layer> layer{InPlaceInit, layerHandle};
    DataHandle dataHandle1 = layer->create();
    DataHandle dataHandle2 = layer->create();

    /* Not valid if the layer instance isn't set yet */
    CORRADE_VERIFY(!ui.isHandleValid(dataHandle1));
    CORRADE_VERIFY(!ui.isHandleValid(dataHandle2));

    /* Valid when is */
    ui.setLayerInstance(Utility::move(layer));
    CORRADE_VERIFY(ui.isHandleValid(dataHandle1));
    CORRADE_VERIFY(ui.isHandleValid(dataHandle2));

    /* Not valid when removed again */
    ui.layer(layerHandle).remove(dataHandle1);
    CORRADE_VERIFY(!ui.isHandleValid(dataHandle1));
    CORRADE_VERIFY(ui.isHandleValid(dataHandle2));

    /* Not valid anymore when the layer itself is removed */
    ui.removeLayer(layerHandle);
    CORRADE_VERIFY(!ui.isHandleValid(dataHandle1));
    CORRADE_VERIFY(!ui.isHandleValid(dataHandle2));
}

void AbstractUserInterfaceTest::dataAttach() {
    /* Event/framebuffer scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};
    LayerHandle layerHandle = ui.createLayer();
    NodeHandle node = ui.createNode({}, {});

    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    };
    ui.setLayerInstance(Containers::pointer<Layer>(layerHandle));

    DataHandle handle = ui.layer(layerHandle).create();
    CORRADE_COMPARE(ui.dataAttachmentCount(), 0);

    ui.attachData(node, handle);
    CORRADE_COMPARE(ui.dataAttachmentCount(), 1);

    /* The data attachments aren't removed immediately, only during next
       clean() -- tested in cleanRemoveData() below */
    ui.removeNode(node);
    CORRADE_COMPARE(ui.dataAttachmentCount(), 1);
}

void AbstractUserInterfaceTest::dataAttachInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui{{100, 100}};
    NodeHandle node = ui.createNode({}, {});

    std::ostringstream out;
    Error redirectError{&out};
    ui.attachData(NodeHandle::Null, DataHandle::Null);
    ui.attachData(NodeHandle(0x123abcde), DataHandle::Null);
    ui.attachData(node, DataHandle::Null);
    ui.attachData(node, DataHandle(0x12abcde34567));
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractUserInterface::attachData(): invalid handle Whee::NodeHandle::Null\n"
        "Whee::AbstractUserInterface::attachData(): invalid handle Whee::NodeHandle(0xabcde, 0x123)\n"
        "Whee::AbstractUserInterface::attachData(): invalid handle Whee::DataHandle::Null\n"
        "Whee::AbstractUserInterface::attachData(): invalid handle Whee::DataHandle({0xab, 0x12}, {0x34567, 0xcde})\n");
}

void AbstractUserInterfaceTest::setSize() {
    AbstractUserInterface ui{NoCreate};

    struct Layer: AbstractLayer {
        explicit Layer(LayerHandle handle, LayerFeatures features, Containers::Array<Containers::Triple<LayerHandle, Vector2, Vector2i>>& calls): AbstractLayer{handle}, features{features}, calls(calls) {}

        LayerFeatures doFeatures() const override { return features; }

        void doSetSize(const Vector2& size, const Vector2i& framebufferSize) override {
            arrayAppend(calls, InPlaceInit, handle(), size, framebufferSize);
        }

        LayerFeatures features;
        Containers::Array<Containers::Triple<LayerHandle, Vector2, Vector2i>>& calls;
    };

    Containers::Array<Containers::Triple<LayerHandle, Vector2, Vector2i>> calls;

    /* Layer instances set before the size is set shouldn't have doSetSize()
       called */
    /*LayerHandle layerWithNoInstance =*/ ui.createLayer();
    LayerHandle layerWithNoDrawFeature = ui.createLayer();
    LayerHandle layerSetBeforeFirstSize = ui.createLayer();
    LayerHandle layerThatIsRemoved = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<Layer>(layerWithNoDrawFeature, LayerFeature::Event, calls));
    ui.setLayerInstance(Containers::pointer<Layer>(layerSetBeforeFirstSize, LayerFeature::Draw|LayerFeature::Event, calls));
    ui.removeLayer(layerThatIsRemoved);
    CORRADE_COMPARE_AS(calls, (Containers::arrayView<Containers::Triple<LayerHandle, Vector2, Vector2i>>({
        /* Nothing yet */
    })), TestSuite::Compare::Container);

    /* Setting the size should set it for all layers that have instances and
       support Draw */
    ui.setSize({300.0f, 200.0f}, {3000.0f, 2000.0f}, {30, 20});
    CORRADE_COMPARE(ui.size(), (Vector2{300.0f, 200.0f}));
    CORRADE_COMPARE(ui.windowSize(), (Vector2{3000.0f, 2000.0f}));
    CORRADE_COMPARE(ui.framebufferSize(), (Vector2i{30, 20}));
    CORRADE_COMPARE_AS(calls, (Containers::arrayView<Containers::Triple<LayerHandle, Vector2, Vector2i>>({
        {layerSetBeforeFirstSize, {300.0f, 200.0f}, {30, 20}},
    })), TestSuite::Compare::Container);

    /* Setting a layer instance after setSize() was called should call
       doSetSize() directly, but again only if it supports Draw */
    calls = {};
    LayerHandle layerSetAfterFirstSizeWithNoDrawFeature = ui.createLayer();
    LayerHandle layerSetAfterFirstSize = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<Layer>(layerSetAfterFirstSizeWithNoDrawFeature, LayerFeatures{}, calls));
    ui.setLayerInstance(Containers::pointer<Layer>(layerSetAfterFirstSize, LayerFeature::Draw, calls));
    CORRADE_COMPARE_AS(calls, (Containers::arrayView<Containers::Triple<LayerHandle, Vector2, Vector2i>>({
        {layerSetAfterFirstSize, {300.0f, 200.0f}, {30, 20}},
    })), TestSuite::Compare::Container);

    /* Calling setSize() again with the same size and framebufferSize should do
       nothing even if window size is different, as window size never reaches
       the layers */
    calls = {};
    ui.setSize({300.0f, 200.0f}, {3.0f, 2.0f}, {30, 20});
    CORRADE_COMPARE(ui.size(), (Vector2{300.0f, 200.0f}));
    CORRADE_COMPARE(ui.windowSize(), (Vector2{3.0f, 2.0f}));
    CORRADE_COMPARE(ui.framebufferSize(), (Vector2i{30, 20}));
    CORRADE_COMPARE_AS(calls, (Containers::arrayView<Containers::Triple<LayerHandle, Vector2, Vector2i>>({
        /* Nothing */
    })), TestSuite::Compare::Container);

    /* Calling setSize() again with different size should call doSetSize() on
       all layers that have an instance and support Draw even if
       framebufferSize and windowSize stays the same */
    calls = {};
    ui.setSize({3000.0f, 2000.0f}, {3.0f, 2.0f}, {30, 20});
    CORRADE_COMPARE(ui.size(), (Vector2{3000.0f, 2000.0f}));
    CORRADE_COMPARE(ui.windowSize(), (Vector2{3.0f, 2.0f}));
    CORRADE_COMPARE(ui.framebufferSize(), (Vector2i{30, 20}));
    CORRADE_COMPARE_AS(calls, (Containers::arrayView<Containers::Triple<LayerHandle, Vector2, Vector2i>>({
        {layerSetBeforeFirstSize, {3000.0f, 2000.0f}, {30, 20}},
        {layerSetAfterFirstSize, {3000.0f, 2000.0f}, {30, 20}},
    })), TestSuite::Compare::Container);

    /* Calling setSize() again with different framebufferSize should call
       doSetSize() on all layers that have an instance and support Draw even if
       size and windowSize stays the same */
    calls = {};
    ui.setSize({3000.0f, 2000.0f}, {3.0f, 2.0f}, {300, 200});
    CORRADE_COMPARE(ui.size(), (Vector2{3000.0f, 2000.0f}));
    CORRADE_COMPARE(ui.windowSize(), (Vector2{3.0f, 2.0f}));
    CORRADE_COMPARE(ui.framebufferSize(), (Vector2i{300, 200}));
    CORRADE_COMPARE_AS(calls, (Containers::arrayView<Containers::Triple<LayerHandle, Vector2, Vector2i>>({
        {layerSetBeforeFirstSize, {3000.0f, 2000.0f}, {300, 200}},
        {layerSetAfterFirstSize, {3000.0f, 2000.0f}, {300, 200}},
    })), TestSuite::Compare::Container);

    /* Finally, verify that the unscaled size overload works as well */
    calls = {};
    ui.setSize({300, 200});
    CORRADE_COMPARE(ui.size(), (Vector2{300.0f, 200.0f}));
    CORRADE_COMPARE(ui.windowSize(), (Vector2{300.0f, 200.0f}));
    CORRADE_COMPARE(ui.framebufferSize(), (Vector2i{300, 200}));
    CORRADE_COMPARE_AS(calls, (Containers::arrayView<Containers::Triple<LayerHandle, Vector2, Vector2i>>({
        {layerSetBeforeFirstSize, {300.0f, 200.0f}, {300, 200}},
        {layerSetAfterFirstSize, {300.0f, 200.0f}, {300, 200}},
    })), TestSuite::Compare::Container);
}

void AbstractUserInterfaceTest::setSizeZero() {
    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui{NoCreate};

    std::ostringstream out;
    Error redirectError{&out};
    ui.setSize({0.0f, 1.0f}, {2.0f, 3.0f}, {4, 5});
    ui.setSize({1.0f, 0.0f}, {2.0f, 3.0f}, {4, 5});
    ui.setSize({1.0f, 2.0f}, {0.0f, 3.0f}, {4, 5});
    ui.setSize({1.0f, 2.0f}, {3.0f, 0.0f}, {4, 5});
    ui.setSize({1.0f, 2.0f}, {3.0f, 4.0f}, {0, 5});
    ui.setSize({1.0f, 2.0f}, {3.0f, 4.0f}, {5, 0});
    CORRADE_COMPARE_AS(out.str(),
        "Whee::AbstractUserInterface::setSize(): expected non-zero sizes, got Vector(0, 1), Vector(2, 3) and Vector(4, 5)\n"
        "Whee::AbstractUserInterface::setSize(): expected non-zero sizes, got Vector(1, 0), Vector(2, 3) and Vector(4, 5)\n"
        "Whee::AbstractUserInterface::setSize(): expected non-zero sizes, got Vector(1, 2), Vector(0, 3) and Vector(4, 5)\n"
        "Whee::AbstractUserInterface::setSize(): expected non-zero sizes, got Vector(1, 2), Vector(3, 0) and Vector(4, 5)\n"
        "Whee::AbstractUserInterface::setSize(): expected non-zero sizes, got Vector(1, 2), Vector(3, 4) and Vector(0, 5)\n"
        "Whee::AbstractUserInterface::setSize(): expected non-zero sizes, got Vector(1, 2), Vector(3, 4) and Vector(5, 0)\n",
        TestSuite::Compare::String);
}

void AbstractUserInterfaceTest::setSizeNotCalledBeforeUpdate() {
    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui{NoCreate};

    /* With an empty UI this shouldn't assert as it doesn't have any
       UserInterfaceState set */
    ui.update();

    /* Causes update() to not be a no-op */
    ui.createNode({}, {});
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);

    std::ostringstream out;
    Error redirectError{&out};
    ui.update();
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractUserInterface::update(): user interface size wasn't set\n");
}

void AbstractUserInterfaceTest::cleanEmpty() {
    AbstractUserInterface ui{{100, 100}};
    CORRADE_COMPARE(ui.nodeUsedCount(), 0);
    CORRADE_COMPARE(ui.dataAttachmentCount(), 0);

    ui.clean();
    CORRADE_COMPARE(ui.nodeUsedCount(), 0);
    CORRADE_COMPARE(ui.dataAttachmentCount(), 0);
}

void AbstractUserInterfaceTest::cleanNoOp() {
    /* Event/framebuffer scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};
    LayerHandle layerHandle = ui.createLayer();

    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    };
    ui.setLayerInstance(Containers::pointer<Layer>(layerHandle));

    /* Root and a nested node */
    NodeHandle root = ui.createNode({}, {});
    NodeHandle nested = ui.createNode(root, {}, {});

    /* Data attached to the root node */
    DataHandle data = ui.layer(layerHandle).create();
    ui.attachData(root, data);

    /* Remove the nested node to create some "dirtiness" */
    ui.removeNode(nested);
    CORRADE_COMPARE(ui.nodeUsedCount(), 1);
    CORRADE_COMPARE(ui.dataAttachmentCount(), 1);

    /* Clean should make no change as there's nothing dangling to remove */
    ui.clean();
    CORRADE_VERIFY(ui.isHandleValid(root));
    CORRADE_VERIFY(ui.isHandleValid(data));
    CORRADE_COMPARE(ui.nodeUsedCount(), 1);
    CORRADE_COMPARE(ui.dataAttachmentCount(), 1);
}

void AbstractUserInterfaceTest::cleanRemoveInvalidData() {
    /* Event/framebuffer scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};
    LayerHandle layerHandle1 = ui.createLayer();
    LayerHandle layerHandle2 = ui.createLayer();

    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    };
    ui.setLayerInstance(Containers::pointer<Layer>(layerHandle1));
    ui.setLayerInstance(Containers::pointer<Layer>(layerHandle2));

    /* Root and a nested node */
    NodeHandle root = ui.createNode({}, {});
    NodeHandle nested = ui.createNode(root, {}, {});

    /* Data attached to both, from both layers, in random order */
    DataHandle data1 = ui.layer(layerHandle1).create();
    DataHandle data2 = ui.layer(layerHandle2).create();
    DataHandle data3 = ui.layer(layerHandle1).create();
    DataHandle data4 = ui.layer(layerHandle2).create();
    ui.attachData(root, data2);
    ui.attachData(nested, data1);
    ui.attachData(nested, data4);
    ui.attachData(root, data3);

    /* Remove some data. They're now invalid but still attached. */
    ui.layer(layerHandle1).remove(data1);
    ui.layer(layerHandle2).remove(data4);
    CORRADE_VERIFY(!ui.isHandleValid(data1));
    CORRADE_VERIFY(ui.isHandleValid(data2));
    CORRADE_VERIFY(ui.isHandleValid(data3));
    CORRADE_VERIFY(!ui.isHandleValid(data4));
    CORRADE_COMPARE(ui.nodeUsedCount(), 2);
    CORRADE_COMPARE(ui.dataAttachmentCount(), 4);

    /* Clean should remove the data attachments */
    ui.clean();
    CORRADE_COMPARE(ui.nodeUsedCount(), 2);
    CORRADE_COMPARE(ui.dataAttachmentCount(), 2);
}

void AbstractUserInterfaceTest::cleanRemoveDataInvalidLayer() {
    /* Event/framebuffer scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};
    LayerHandle layerHandle1 = ui.createLayer();
    LayerHandle layerHandle2 = ui.createLayer();

    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    };
    ui.setLayerInstance(Containers::pointer<Layer>(layerHandle1));
    ui.setLayerInstance(Containers::pointer<Layer>(layerHandle2));

    /* Root and a nested node */
    NodeHandle root = ui.createNode({}, {});
    NodeHandle nested = ui.createNode(root, {}, {});

    /* Data attached to both, from both layers, in random order */
    DataHandle data1 = ui.layer(layerHandle1).create();
    DataHandle data2 = ui.layer(layerHandle2).create();
    DataHandle data3 = ui.layer(layerHandle1).create();
    DataHandle data4 = ui.layer(layerHandle2).create();
    ui.attachData(root, data2);
    ui.attachData(nested, data1);
    ui.attachData(nested, data4);
    ui.attachData(root, data3);

    /* Remove the whole layer. The data from it are now invalid but still
       attached. */
    ui.removeLayer(layerHandle2);
    CORRADE_VERIFY(ui.isHandleValid(data1));
    CORRADE_VERIFY(!ui.isHandleValid(data2));
    CORRADE_VERIFY(ui.isHandleValid(data3));
    CORRADE_VERIFY(!ui.isHandleValid(data4));
    CORRADE_COMPARE(ui.nodeUsedCount(), 2);
    CORRADE_COMPARE(ui.dataAttachmentCount(), 4);

    /* Clean should remove the data attachments that belong to the now-invalid
       layer */
    ui.clean();
    CORRADE_COMPARE(ui.nodeUsedCount(), 2);
    CORRADE_COMPARE(ui.dataAttachmentCount(), 2);
}

void AbstractUserInterfaceTest::cleanRemoveAttachedData() {
    /* Event/framebuffer scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};
    LayerHandle layerHandle1 = ui.createLayer();
    LayerHandle layerHandle2 = ui.createLayer();

    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    };
    ui.setLayerInstance(Containers::pointer<Layer>(layerHandle1));
    ui.setLayerInstance(Containers::pointer<Layer>(layerHandle2));

    /* Root and a nested node */
    NodeHandle root = ui.createNode({}, {});
    NodeHandle nested = ui.createNode(root, {}, {});

    /* Data attached to both, from both layers, in random order */
    DataHandle data1 = ui.layer(layerHandle1).create();
    DataHandle data2 = ui.layer(layerHandle2).create();
    DataHandle data3 = ui.layer(layerHandle1).create();
    DataHandle data4 = ui.layer(layerHandle2).create();
    ui.attachData(nested, data1);
    ui.attachData(root, data2);
    ui.attachData(root, data3);
    ui.attachData(nested, data4);

    /* Remove the nested node */
    ui.removeNode(nested);
    CORRADE_COMPARE(ui.nodeUsedCount(), 1);
    CORRADE_COMPARE(ui.dataAttachmentCount(), 4);
    CORRADE_COMPARE(ui.layer(layerHandle1).usedCount(), 2);
    CORRADE_COMPARE(ui.layer(layerHandle2).usedCount(), 2);

    /* Clean removes the nested node data attachments and removes them from
       layers as well */
    ui.clean();
    CORRADE_COMPARE(ui.nodeUsedCount(), 1);
    CORRADE_COMPARE(ui.dataAttachmentCount(), 2);
    CORRADE_COMPARE(ui.layer(layerHandle1).usedCount(), 1);
    CORRADE_COMPARE(ui.layer(layerHandle2).usedCount(), 1);
    CORRADE_VERIFY(ui.isHandleValid(root));
    CORRADE_VERIFY(!ui.isHandleValid(data1));
    CORRADE_VERIFY(ui.isHandleValid(data2));
    CORRADE_VERIFY(ui.isHandleValid(data3));
    CORRADE_VERIFY(!ui.isHandleValid(data4));
}

void AbstractUserInterfaceTest::cleanRemoveNestedNodes() {
    /* Event/framebuffer scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};
    LayerHandle layerHandle = ui.createLayer();

    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    };
    ui.setLayerInstance(Containers::pointer<Layer>(layerHandle));

    /* A nested node tree */
    NodeHandle root = ui.createNode({}, {});
    NodeHandle first1 = ui.createNode(root, {}, {});
    NodeHandle second1 = ui.createNode(first1, {}, {});
    NodeHandle first2 = ui.createNode(root, {}, {});
    NodeHandle second2 = ui.createNode(first1, {}, {});

    /* Data attached to the leaf nodes */
    DataHandle data1 = ui.layer(layerHandle).create();
    DataHandle data2 = ui.layer(layerHandle).create();
    DataHandle data3 = ui.layer(layerHandle).create();
    ui.attachData(second1, data1);
    ui.attachData(first2, data2);
    ui.attachData(second2, data3);

    /* Remove the subtree */
    ui.removeNode(first1);
    CORRADE_COMPARE(ui.nodeUsedCount(), 4);
    CORRADE_COMPARE(ui.dataAttachmentCount(), 3);

    /* Clean removes the nested nodes and subsequently the data attached to
       them */
    ui.clean();
    CORRADE_COMPARE(ui.nodeUsedCount(), 2);
    CORRADE_COMPARE(ui.dataAttachmentCount(), 1);
    CORRADE_VERIFY(ui.isHandleValid(root));
    CORRADE_VERIFY(!ui.isHandleValid(first1));
    CORRADE_VERIFY(ui.isHandleValid(first2));
    CORRADE_VERIFY(!ui.isHandleValid(second1));
    CORRADE_VERIFY(!ui.isHandleValid(second2));
    CORRADE_VERIFY(!ui.isHandleValid(data1));
    CORRADE_VERIFY(ui.isHandleValid(data2));
    CORRADE_VERIFY(!ui.isHandleValid(data3));
}

void AbstractUserInterfaceTest::cleanRemoveNestedNodesAlreadyRemoved() {
    /* Event/framebuffer scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};
    NodeHandle node = ui.createNode({}, {});
    NodeHandle nested = ui.createNode(node, {}, {});

    ui.removeNode(nested);
    CORRADE_COMPARE(ui.nodeUsedCount(), 1);

    /* There's no dangling children, so this has nothing to do */
    ui.clean();
    CORRADE_COMPARE(ui.nodeUsedCount(), 1);

    ui.removeNode(node);
    CORRADE_COMPARE(ui.nodeUsedCount(), 0);

    /* Shouldn't attempt to remove the already-removed nested node again */
    ui.clean();
    CORRADE_COMPARE(ui.nodeUsedCount(), 0);
}

void AbstractUserInterfaceTest::cleanRemoveNestedNodesAlreadyRemovedDangling() {
    /* Event/framebuffer scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};
    NodeHandle node = ui.createNode({}, {});
    NodeHandle nested = ui.createNode(node, {}, {});

    /* Nested is now dangling */
    ui.removeNode(node);
    CORRADE_COMPARE(ui.nodeUsedCount(), 1);

    /* Removing it should cause no cycles or other internal state corruption */
    ui.removeNode(nested);
    CORRADE_COMPARE(ui.nodeUsedCount(), 0);

    /* Shouldn't attempt to remove the already-removed nested node again */
    ui.clean();
    CORRADE_COMPARE(ui.nodeUsedCount(), 0);
}

void AbstractUserInterfaceTest::cleanRemoveNestedNodesRecycledHandle() {
    /* Event/framebuffer scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};
    LayerHandle layerHandle = ui.createLayer();

    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    };
    ui.setLayerInstance(Containers::pointer<Layer>(layerHandle));

    /* A nested node branch */
    NodeHandle root = ui.createNode({}, {});
    NodeHandle first = ui.createNode(root, {}, {});
    NodeHandle second = ui.createNode(first, {}, {});

    /* Data attached to the leaf node */
    DataHandle data = ui.layer(layerHandle).create();
    ui.attachData(second, data);

    /* Remove a subtree but then create a new node which recycles the same
       handle */
    ui.removeNode(first);
    NodeHandle first2 = ui.createNode(root, {}, {});
    CORRADE_COMPARE(nodeHandleId(first2), nodeHandleId(first));
    CORRADE_COMPARE(ui.nodeUsedCount(), 3);
    CORRADE_COMPARE(ui.dataAttachmentCount(), 1);

    /* Clean should still remove the subtree attached to the first handle, even
       though there's a new valid node in the same slot */
    ui.clean();
    CORRADE_COMPARE(ui.nodeUsedCount(), 2);
    CORRADE_COMPARE(ui.dataAttachmentCount(), 0);
    CORRADE_VERIFY(ui.isHandleValid(root));
    CORRADE_VERIFY(!ui.isHandleValid(first));
    CORRADE_VERIFY(ui.isHandleValid(first2));
    CORRADE_VERIFY(!ui.isHandleValid(data));
}

void AbstractUserInterfaceTest::cleanRemoveNestedNodesRecycledHandleOrphanedCycle() {
    CORRADE_SKIP("Ugh, this asserts.");

    /* Event/framebuffer scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};
    LayerHandle layerHandle = ui.createLayer();

    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    };
    ui.setLayerInstance(Containers::pointer<Layer>(layerHandle));

    /* A nested node branch */
    NodeHandle root = ui.createNode({}, {});
    NodeHandle first = ui.createNode(root, {}, {});
    NodeHandle second = ui.createNode(first, {}, {});
    NodeHandle third = ui.createNode(second, {}, {});

    /* Data attached to the leaf node */
    DataHandle data = ui.layer(layerHandle).create();
    ui.attachData(third, data);

    /* Remove a subtree but then create a new node which recycles the same
       handle, and parent it to one of the (now dangling) nodes */
    ui.removeNode(first);
    NodeHandle first2 = ui.createNode(second, {}, {});
    CORRADE_COMPARE(nodeHandleId(first2), nodeHandleId(first));
    CORRADE_COMPARE(ui.nodeUsedCount(), 4);
    CORRADE_COMPARE(ui.dataAttachmentCount(), 1);

    ui.clean();
    CORRADE_COMPARE(ui.nodeUsedCount(), 2);
    CORRADE_COMPARE(ui.dataAttachmentCount(), 0);
    CORRADE_VERIFY(ui.isHandleValid(root));
    CORRADE_VERIFY(!ui.isHandleValid(first));
    CORRADE_VERIFY(ui.isHandleValid(first2));
    CORRADE_VERIFY(!ui.isHandleValid(second));
    CORRADE_VERIFY(!ui.isHandleValid(third));
    CORRADE_VERIFY(!ui.isHandleValid(data));
}

void AbstractUserInterfaceTest::cleanRemoveAll() {
    /* Event/framebuffer scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};
    LayerHandle layerHandle = ui.createLayer();

    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    };
    ui.setLayerInstance(Containers::pointer<Layer>(layerHandle));

    /* A nested node branch */
    NodeHandle root = ui.createNode({}, {});
    NodeHandle first = ui.createNode(root, {}, {});
    NodeHandle second = ui.createNode(first, {}, {});

    /* Data attached to the nested nodes */
    DataHandle data1 = ui.layer(layerHandle).create();
    DataHandle data2 = ui.layer(layerHandle).create();
    ui.attachData(second, data1);
    ui.attachData(first, data2);

    /* Removing the top-level node */
    ui.removeNode(root);
    CORRADE_COMPARE(ui.nodeUsedCount(), 2);
    CORRADE_COMPARE(ui.dataAttachmentCount(), 2);

    /* Clean should remove everything */
    ui.clean();
    CORRADE_COMPARE(ui.nodeUsedCount(), 0);
    CORRADE_COMPARE(ui.dataAttachmentCount(), 0);
}

void AbstractUserInterfaceTest::state() {
    auto&& data = StateData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Event/framebuffer scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

    /*   2        3         4         5         6
       0                              +---------+
       1 +----------------------------| another |
       2 |       node                 +---------+
       3 |                  +---------+
       4 |        +---------| nested2 |
       5 |        | nested1 +---------+
       6 +--------+---------+---------+           */
    NodeHandle node = ui.createNode({2.0f, 1.0f}, {3.0f, 5.0f}, NodeFlag::Clip);
    NodeHandle another = ui.createNode({5.0f, 0.0f}, {1.0f, 2.0f});
    NodeHandle nested1 = ui.createNode(node, {1.0f, 3.0f}, {1.0f, 2.0f});
    NodeHandle nested2 = ui.createNode(node, {2.0f, 2.0f}, {1.0f, 2.0f});

    /* Creating nodes sets a state flag */
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);

    /* Calling clean() doesn't do anything. Until data are added, there's
       nothing observable to test that it did the right thing. */
    if(data.clean && data.noOp) {
        ui.clean();
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);
    }

    /* Calling update() rebuilds internal state and resets the flag. Until data
       are added, there's nothing observable to test that it did the right
       thing. */
    ui.update();
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }

        /* doSetSize() not implemented here as it isn't called from
           ui.update(), tested thoroughly in setSize() instead */

        void doClean(Containers::BitArrayView dataIdsToRemove) override {
            CORRADE_COMPARE_AS(dataIdsToRemove,
                expectedDataIdsToRemove.sliceBit(0),
                TestSuite::Compare::Container);
            ++cleanCallCount;
        }

        void doUpdate(const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& dataNodeIds, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes) override {
            CORRADE_COMPARE_AS(dataIds,
                expectedData.slice(&Containers::Pair<UnsignedInt, UnsignedInt>::first),
                TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(dataNodeIds,
                expectedData.slice(&Containers::Pair<UnsignedInt, UnsignedInt>::second),
                TestSuite::Compare::Container);
            CORRADE_COMPARE(nodeOffsets.size(), expectedNodeOffsetsSizes.size());
            for(std::size_t i = 0; i != nodeOffsets.size(); ++i) {
                CORRADE_ITERATION(i);
                /* For nodes that aren't in the visible hierarchy or are
                   removed the value can be just anything, skip */
                if(expectedNodeOffsetsSizes[i].second().isZero())
                    continue;
                CORRADE_COMPARE(Containers::pair(nodeOffsets[i], nodeSizes[i]), expectedNodeOffsetsSizes[i]);
            }
            ++updateCallCount;
        }

        Containers::StridedArrayView1D<const bool> expectedDataIdsToRemove;
        Containers::StridedArrayView1D<const Containers::Pair<UnsignedInt, UnsignedInt>> expectedData;
        Containers::StridedArrayView1D<const Containers::Pair<Vector2, Vector2>> expectedNodeOffsetsSizes;
        Int cleanCallCount = 0;
        Int updateCallCount = 0;
    };

    /* Creating a layer sets no state flags */
    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<Layer>(layer));
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

    /* Calling clean() should be a no-op, not calling anything in the layer */
    if(data.clean && data.noOp) {
        {
            CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 0);
    }

    /* Calling update() should be a no-op, not calling anything in the layer */
    if(data.noOp) {
        {
            CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
            ui.update();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 0);
    }

    /* Creating a data in a layer sets no state flags */
    DataHandle data1 = ui.layer(layer).create();
    DataHandle data2 = ui.layer(layer).create();
    DataHandle data3 = ui.layer(layer).create();
    DataHandle data4 = ui.layer(layer).create();
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 0);

    /* Calling clean() should be a no-op */
    if(data.clean && data.noOp) {
        {
            CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 0);
    }

    /* Calling update() should be a no-op too */
    if(data.noOp) {
        {
            CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
            ui.update();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 0);
    }

    /* Attaching the data sets flags. Shuffled order to have non-trivial
       results. */
    ui.attachData(node, data2);
    ui.attachData(nested1, data4);
    ui.attachData(nested2, data1);
    ui.attachData(another, data3);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate);

    /* Calling clean() should be a no-op */
    if(data.clean && data.noOp) {
        {
            CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate);
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 0);
    }

    /* Calling update() rebuilds internal state, calls doUpdate() on the layer,
       and resets the flag. */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
        Containers::Pair<UnsignedInt, UnsignedInt> expectedData[]{
            {dataHandleId(data2), nodeHandleId(node)},
            {dataHandleId(data4), nodeHandleId(nested1)},
            {dataHandleId(data1), nodeHandleId(nested2)},
            {dataHandleId(data3), nodeHandleId(another)}
        };
        Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
            {{2.0f, 1.0f}, {3.0f, 5.0f}}, /* node */
            {{5.0f, 0.0f}, {1.0f, 2.0f}}, /* another */
            {{3.0f, 4.0f}, {1.0f, 2.0f}}, /* nested1 */
            {{4.0f, 3.0f}, {1.0f, 2.0f}}, /* nested2 */
        };
        ui.layer<Layer>(layer).expectedData = expectedData;
        ui.layer<Layer>(layer).expectedNodeOffsetsSizes = expectedNodeOffsetsSizes;
        ui.update();
    }
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
    CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 1);

    /* Marking the layer with NeedsUpdate propagates to the UI-wide state */
    ui.layer(layer).setNeedsUpdate();
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataUpdate);

    /* Calling clean() should be a no-op */
    if(data.clean && data.noOp) {
        {
            CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataUpdate);
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 1);
    }

    /* Calling update() reuploads the exact same data and resets the flag, but
       internally shouldn't do any other state rebuild. Nothing observable to
       verify that with, tho. */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
        Containers::Pair<UnsignedInt, UnsignedInt> expectedData[]{
            {dataHandleId(data2), nodeHandleId(node)},
            {dataHandleId(data4), nodeHandleId(nested1)},
            {dataHandleId(data1), nodeHandleId(nested2)},
            {dataHandleId(data3), nodeHandleId(another)}
        };
        Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
            {{2.0f, 1.0f}, {3.0f, 5.0f}}, /* node */
            {{5.0f, 0.0f}, {1.0f, 2.0f}}, /* another */
            {{3.0f, 4.0f}, {1.0f, 2.0f}}, /* nested1 */
            {{4.0f, 3.0f}, {1.0f, 2.0f}}, /* nested2 */
        };
        ui.layer<Layer>(layer).expectedData = expectedData;
        ui.layer<Layer>(layer).expectedNodeOffsetsSizes = expectedNodeOffsetsSizes;
        ui.update();
    }
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
    CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 2);

    /* Changing a node size sets a state flag to update clipping. In this case
       it causes the nested2 node to get culled:

         2        3         4         5         6
       0                              +---------+
       1 +------------------+         | another |
       2 |       node       |         +---------+
       3 |                  +---------+
       4 |        +---------| nested2 |
       5 +--------| nested1 +---------+
       6          +---------+                     */
    ui.setNodeSize(node, {2.0f, 4.0f});
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeClipUpdate);

    /* Calling clean() should be a no-op */
    if(data.clean && data.noOp) {
        {
            CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeClipUpdate);
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 2);
    }

    /* Calling update() reuploads the data except for the culled node, with
       a single size changed and resets the flag, but internally shouldn't do
       any other state rebuild */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
        Containers::Pair<UnsignedInt, UnsignedInt> expectedData[]{
            {dataHandleId(data2), nodeHandleId(node)},
            {dataHandleId(data4), nodeHandleId(nested1)},
            {dataHandleId(data3), nodeHandleId(another)}
        };
        Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
            {{2.0f, 1.0f}, {2.0f, 4.0f}}, /* node */
            {{5.0f, 0.0f}, {1.0f, 2.0f}}, /* another */
            {{3.0f, 4.0f}, {1.0f, 2.0f}}, /* nested1 */
            {},
        };
        ui.layer<Layer>(layer).expectedData = expectedData;
        ui.layer<Layer>(layer).expectedNodeOffsetsSizes = expectedNodeOffsetsSizes;
        ui.update();
    }
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
    CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 3);

    /* Changing a node offset sets a state flag to recalculate also nested
       node offsets, except for nested2 that's still culled.

         2        3         4         5         6
       0                              +---------+
       1          +-------------------| another |
       2          |       node        +---------+
       3          |                   +---------+
       4          |         +---------| nested2 |
       5          +---------| nested1 +---------+
       6                    +---------+           */
    ui.setNodeOffset(node, {3.0f, 1.0f});
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeLayoutUpdate);

    /* Calling clean() should be a no-op */
    if(data.clean && data.noOp) {
        {
            CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeLayoutUpdate);
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 3);
    }

    /* Calling update() recalculates absoute offsets, uploads the new data and
       resets the flag */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
        Containers::Pair<UnsignedInt, UnsignedInt> expectedData[]{
            {dataHandleId(data2), nodeHandleId(node)},
            {dataHandleId(data4), nodeHandleId(nested1)},
            {dataHandleId(data3), nodeHandleId(another)}
        };
        Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
            {{3.0f, 1.0f}, {2.0f, 4.0f}}, /* node */
            {{5.0f, 0.0f}, {1.0f, 2.0f}}, /* another */
            {{4.0f, 4.0f}, {1.0f, 2.0f}}, /* nested1 */
            {},
        };
        ui.layer<Layer>(layer).expectedData = expectedData;
        ui.layer<Layer>(layer).expectedNodeOffsetsSizes = expectedNodeOffsetsSizes;
        ui.update();
    }
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
    CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 4);

    /* Setting a Hidden flag sets a state flag */
    ui.addNodeFlags(node, NodeFlag::Hidden);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);

    /* Calling clean() should be a no-op */
    if(data.clean && data.noOp) {
        {
            CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 4);
    }

    /* Calling update() rebuilds internal state without the hidden hierarchy */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
        Containers::Pair<UnsignedInt, UnsignedInt> expectedData[]{
            {dataHandleId(data3), nodeHandleId(another)}
        };
        Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
            {},
            {{5.0f, 0.0f}, {1.0f, 2.0f}}, /* another */
            {},
            {}
        };
        ui.layer<Layer>(layer).expectedData = expectedData;
        ui.layer<Layer>(layer).expectedNodeOffsetsSizes = expectedNodeOffsetsSizes;
        ui.update();
    }
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
    CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 5);

    /* Setting a Hidden flag that's already set should be a no-op,
       independently of what other flags get added */
    ui.addNodeFlags(node, NodeFlag(0xe0)|NodeFlag::Hidden);
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

    /* Resetting a Hidden flag sets a state flag again */
    ui.clearNodeFlags(node, NodeFlag::Hidden);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);

    /* Calling clean() should be a no-op */
    if(data.clean && data.noOp) {
        {
            CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 5);
    }

    /* Calling update() reuploads the previous data again and resets the state
       flag */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
        Containers::Pair<UnsignedInt, UnsignedInt> expectedData[]{
            {dataHandleId(data2), nodeHandleId(node)},
            {dataHandleId(data4), nodeHandleId(nested1)},
            {dataHandleId(data3), nodeHandleId(another)}
        };
        Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
            {{3.0f, 1.0f}, {2.0f, 4.0f}}, /* node */
            {{5.0f, 0.0f}, {1.0f, 2.0f}}, /* another */
            {{4.0f, 4.0f}, {1.0f, 2.0f}}, /* nested1 */
            {},
        };
        ui.layer<Layer>(layer).expectedData = expectedData;
        ui.layer<Layer>(layer).expectedNodeOffsetsSizes = expectedNodeOffsetsSizes;
        ui.update();
    }
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
    CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 6);

    /* Resetting a Hidden flag that's not there should be a no-op,
       independently of what other flags get cleared */
    ui.clearNodeFlags(node, NodeFlag(0x70)|NodeFlag::Hidden);
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

    /* Setting a Clip flag that's already there should be a no-op,
       independently of what other flags get added */
    ui.addNodeFlags(node, NodeFlag(0x10)|NodeFlag::Clip);
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

    /* Resetting a Clip flag sets a state flag */
    ui.clearNodeFlags(node, NodeFlag::Clip);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeClipUpdate);

    /* Calling clean() should be a no-op */
    if(data.clean && data.noOp) {
        {
            CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeClipUpdate);
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 6);
    }

    /* Calling update() uploads the full data including the no-longer-clipped
       nodes */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
        Containers::Pair<UnsignedInt, UnsignedInt> expectedData[]{
            {dataHandleId(data2), nodeHandleId(node)},
            {dataHandleId(data4), nodeHandleId(nested1)},
            {dataHandleId(data1), nodeHandleId(nested2)},
            {dataHandleId(data3), nodeHandleId(another)}
        };
        Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
            {{3.0f, 1.0f}, {2.0f, 4.0f}}, /* node */
            {{5.0f, 0.0f}, {1.0f, 2.0f}}, /* another */
            {{4.0f, 4.0f}, {1.0f, 2.0f}}, /* nested1 */
            {{5.0f, 3.0f}, {1.0f, 2.0f}}, /* nested2 */
        };
        ui.layer<Layer>(layer).expectedData = expectedData;
        ui.layer<Layer>(layer).expectedNodeOffsetsSizes = expectedNodeOffsetsSizes;
        ui.update();
    }
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
    CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 7);

    /* Resetting a Clip flag that's not there should be a no-op, independently
       of what other flags get cleared */
    ui.clearNodeFlags(node, NodeFlag(0x30)|NodeFlag::Clip);
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

    /* Setting a Clip flag sets a state flag again */
    ui.addNodeFlags(node, NodeFlag::Clip);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeClipUpdate);

    /* Calling clean() should be a no-op */
    if(data.clean && data.noOp) {
        {
            CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeClipUpdate);
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 7);
    }

    /* Calling update() reuploads the previous data again and resets the state
       flag */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
        Containers::Pair<UnsignedInt, UnsignedInt> expectedData[]{
            {dataHandleId(data2), nodeHandleId(node)},
            {dataHandleId(data4), nodeHandleId(nested1)},
            {dataHandleId(data3), nodeHandleId(another)}
        };
        Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
            {{3.0f, 1.0f}, {2.0f, 4.0f}}, /* node */
            {{5.0f, 0.0f}, {1.0f, 2.0f}}, /* another */
            {{4.0f, 4.0f}, {1.0f, 2.0f}}, /* nested1 */
            {},
        };
        ui.layer<Layer>(layer).expectedData = expectedData;
        ui.layer<Layer>(layer).expectedNodeOffsetsSizes = expectedNodeOffsetsSizes;
        ui.update();
    }
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
    CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 8);

    /* Calling clearNodeOrder() sets a state flag */
    ui.clearNodeOrder(another);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);

    /* Calling clean() should be a no-op */
    if(data.clean && data.noOp) {
        {
            CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 8);
    }

    /* Calling update() uploads data in new order and resets the flag */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
        Containers::Pair<UnsignedInt, UnsignedInt> expectedData[]{
            {dataHandleId(data2), nodeHandleId(node)},
            {dataHandleId(data4), nodeHandleId(nested1)},
        };
        Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
            {{3.0f, 1.0f}, {2.0f, 4.0f}}, /* node */
            {},
            {{4.0f, 4.0f}, {1.0f, 2.0f}}, /* nested1 */
            {}
        };
        ui.layer<Layer>(layer).expectedData = expectedData;
        ui.layer<Layer>(layer).expectedNodeOffsetsSizes = expectedNodeOffsetsSizes;
        ui.update();
    }
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
    CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 9);

    /* Calling clearNodeOrder() on a node that isn't in the order is a no-op */
    ui.clearNodeOrder(another);
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

    /* Calling setNodeOrder() sets a state flag again */
    ui.setNodeOrder(another, node);
    /** @todo make this a no-op if the order is already that way (and test) */
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);

    /* Calling clean() should be a no-op */
    if(data.clean && data.noOp) {
        {
            CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 9);
    }

    /* Calling update() uploads data in new order and resets the flag */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
        Containers::Pair<UnsignedInt, UnsignedInt> expectedData[]{
            {dataHandleId(data3), nodeHandleId(another)},
            {dataHandleId(data2), nodeHandleId(node)},
            {dataHandleId(data4), nodeHandleId(nested1)}
        };
        Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
            {{3.0f, 1.0f}, {2.0f, 4.0f}}, /* node */
            {{5.0f, 0.0f}, {1.0f, 2.0f}}, /* another */
            {{4.0f, 4.0f}, {1.0f, 2.0f}}, /* nested1 */
            {},
        };
        ui.layer<Layer>(layer).expectedData = expectedData;
        ui.layer<Layer>(layer).expectedNodeOffsetsSizes = expectedNodeOffsetsSizes;
        ui.update();
    }
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
    CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 10);

    /* Removing data marks the layer with NeedsClean, which is then propagated
       to the UI-wide state */
    ui.layer(layer).remove(data2);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataClean);
    CORRADE_COMPARE(ui.dataAttachmentCount(), 4);

    /* Calling clean() removes the now-invalid attachment and resets the states
       to not require clean() anymore */
    if(data.clean) {
        {
            CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
            bool expectedDataIdsToRemove[]{
                false, false, false, false /* data2 already removed, so not set */
            };
            ui.layer<Layer>(layer).expectedDataIdsToRemove = expectedDataIdsToRemove;
            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate);
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 1);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 10);
        CORRADE_COMPARE(ui.dataAttachmentCount(), 3);
    }

    /* Calling update() then uploads remaining data and resets the remaining
       state flag; also calls clean() if wasn't done above already */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
        bool expectedDataIdsToRemove[]{
            false, false, false, false /* data2 already removed, so not set */
        };
        Containers::Pair<UnsignedInt, UnsignedInt> expectedData[]{
            {dataHandleId(data3), nodeHandleId(another)},
            {dataHandleId(data4), nodeHandleId(nested1)}
        };
        Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
            {{3.0f, 1.0f}, {2.0f, 4.0f}}, /* node */
            {{5.0f, 0.0f}, {1.0f, 2.0f}}, /* another */
            {{4.0f, 4.0f}, {1.0f, 2.0f}}, /* nested1 */
            {},
        };
        ui.layer<Layer>(layer).expectedDataIdsToRemove = expectedDataIdsToRemove;
        ui.layer<Layer>(layer).expectedData = expectedData;
        ui.layer<Layer>(layer).expectedNodeOffsetsSizes = expectedNodeOffsetsSizes;
        ui.update();
    }
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.dataAttachmentCount(), 3);
    /* doClean() should only be called either in the branch above or from
       update(), never both */
    CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 1);
    CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 11);

    /* Removing a node sets a state flag */
    ui.removeNode(node);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeClean);
    CORRADE_COMPARE(ui.nodeUsedCount(), 3);
    CORRADE_COMPARE(ui.dataAttachmentCount(), 3);

    /* Calling clean() removes the child nodes, the now-invalid attachment and
       resets the state to not require clean() anymore */
    if(data.clean) {
        {
            CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
            bool expectedDataIdsToRemove[]{
                /* data1 and data4 was attached to nested2 and nested1, which
                   got orphaned after removing its parent, `node` */
                true, false, false, true
            };
            ui.layer<Layer>(layer).expectedDataIdsToRemove = expectedDataIdsToRemove;
            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate|UserInterfaceState::NeedsDataAttachmentUpdate);
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 2);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 11);
        CORRADE_COMPARE(ui.nodeUsedCount(), 1);
        CORRADE_COMPARE(ui.dataAttachmentCount(), 1);
    }

    /* Calling update() then uploads remaining data and resets the remaining
       state flag */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
        bool expectedDataIdsToRemove[]{
            /* data1 and data4 was attached to nested2 and nested1, which got
               orphaned after removing its parent, `node` */
            true, false, false, true
        };
        Containers::Pair<UnsignedInt, UnsignedInt> expectedData[]{
            {dataHandleId(data3), nodeHandleId(another)},
        };
        Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
            {},
            {{5.0f, 0.0f}, {1.0f, 2.0f}}, /* another */
            {},
            {},
        };
        ui.layer<Layer>(layer).expectedDataIdsToRemove = expectedDataIdsToRemove;
        ui.layer<Layer>(layer).expectedData = expectedData;
        ui.layer<Layer>(layer).expectedNodeOffsetsSizes = expectedNodeOffsetsSizes;
        ui.update();
    }
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.nodeUsedCount(), 1);
    CORRADE_COMPARE(ui.dataAttachmentCount(), 1);
    CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 2);
    CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 12);

    /* Add one more layer to check layer removal behavior, should set no state
       flags again */
    LayerHandle anotherLayer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<Layer>(anotherLayer));
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

    /* Removing a layer sets a state flag */
    ui.removeLayer(layer);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataClean);

    /* Calling clean() removes the remaining attachment and resets the state */
    if(data.clean) {
        {
            CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
            /* The `layer` is no more, so nothing to check there. The
               `anotherLayer` gets called with an empty view because it has no
               data. */
            ui.layer<Layer>(anotherLayer).expectedDataIdsToRemove = {};
            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate);
        CORRADE_COMPARE(ui.dataAttachmentCount(), 0);
        CORRADE_COMPARE(ui.layer<Layer>(anotherLayer).cleanCallCount, 1);
        CORRADE_COMPARE(ui.layer<Layer>(anotherLayer).updateCallCount, 0);
    }

    /* Calling update() then resets the remaining state flag, There's no data
       anymore, but it's still called to let the layer refresh its internal
       state. */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
        Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
            {},
            {{5.0f, 0.0f}, {1.0f, 2.0f}}, /* another */
            {},
            {}
        };
        ui.layer<Layer>(anotherLayer).expectedDataIdsToRemove = {};
        ui.layer<Layer>(anotherLayer).expectedData = {};
        ui.layer<Layer>(anotherLayer).expectedNodeOffsetsSizes = expectedNodeOffsetsSizes;
        ui.update();
    }
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.dataAttachmentCount(), 0);
    CORRADE_COMPARE(ui.layer<Layer>(anotherLayer).cleanCallCount, 1);
    CORRADE_COMPARE(ui.layer<Layer>(anotherLayer).updateCallCount, 1);
}

void AbstractUserInterfaceTest::statePropagateFromLayers() {
    /* Tests more complex behavior of state propagation that isn't checked in
       the state() case above */

    /* Event/framebuffer scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};

    /*LayerHandle layerWithoutInstance =*/ ui.createLayer();
    LayerHandle layerRemoved = ui.createLayer();
    LayerHandle layer1 = ui.createLayer();
    LayerHandle layer2 = ui.createLayer();

    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    };
    ui.setLayerInstance(Containers::pointer<Layer>(layerRemoved));
    ui.setLayerInstance(Containers::pointer<Layer>(layer1));
    ui.setLayerInstance(Containers::pointer<Layer>(layer2));
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

    /* Create a node for using later and make the state empty again */
    NodeHandle node = ui.createNode({}, {});
    ui.update();
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

    /* LayerState::NeedsUpdate on a removed layer isn't considered, and the
       layer without an instance is skipped */
    ui.layer(layerRemoved).setNeedsUpdate();
    ui.removeLayer(layerRemoved);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataClean);

    ui.update();
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

    /* It also shouldn't stop at those, states after those get checked as well */
    ui.layer(layer1).setNeedsUpdate();
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataUpdate);

    /* And updating should reset all of them again */
    ui.update();
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

    /* Creating a data doesn't result in any NeedsUpdate on the layer, but
       attaching results in NeedsDataAttachmentUpdate being set on the UI directly */
    DataHandle data = ui.layer(layer2).create();
    ui.attachData(node, data);
    CORRADE_COMPARE(ui.layer(layer2).state(), LayerStates{});
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate);

    /* Having the UI marked with NeedsDataUpdate shouldn't prevent the
       NeedsClean from a later layer from being propagated to the UI-wide
       state */
    ui.layer(layer2).remove(data);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataClean);
}

void AbstractUserInterfaceTest::drawEmpty() {
    auto&& data = CleanUpdateData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui{{100, 100}};
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

    /* Just verify that this doesn't crash or assert, there's nothing visibly
       changing after these calls */
    if(data.clean) {
        ui.clean();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }
    if(data.update) {
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }
    ui.draw();
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
}

void AbstractUserInterfaceTest::draw() {
    auto&& data = DrawData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* windowSize isn't used for anything here */
    AbstractUserInterface ui{{200.0f, 300.0f}, {20.0f, 30.0f}, {400, 500}};

    struct Layer: AbstractLayer {
        explicit Layer(LayerHandle handle, LayerFeatures features): AbstractLayer{handle}, features{features} {}

        LayerFeatures doFeatures() const override { return features; }

        void doSetSize(const Vector2& size, const Vector2i& framebufferSize) override {
            CORRADE_ITERATION(handle());
            ++setSizeCallCount;
            CORRADE_COMPARE(size, (Vector2{200.0f, 300.0f}));
            CORRADE_COMPARE(framebufferSize, (Vector2i{400, 500}));
        }

        void doUpdate(const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& dataNodeIds, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes) override {
            CORRADE_ITERATION(handle());
            /* doSetSize() should have been called exactly once at this point
               if this layer draws, and not at all if it doesn't */
            CORRADE_COMPARE(setSizeCallCount, features & LayerFeature::Draw ? 1 : 0);
            CORRADE_COMPARE_AS(dataIds,
                expectedData.slice(&Containers::Pair<UnsignedInt, UnsignedInt>::first),
                TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(dataNodeIds,
                expectedData.slice(&Containers::Pair<UnsignedInt, UnsignedInt>::second),
                TestSuite::Compare::Container);
            CORRADE_COMPARE(nodeOffsets.size(), expectedNodeOffsetsSizes.size());
            for(std::size_t i = 0; i != nodeOffsets.size(); ++i) {
                CORRADE_ITERATION(i);
                /* For nodes that aren't in the visible hierarchy the value
                   can be just anything, skip */
                if(expectedNodeOffsetsSizes[i].second().isZero())
                    continue;
                CORRADE_COMPARE(Containers::pair(nodeOffsets[i], nodeSizes[i]), expectedNodeOffsetsSizes[i]);
            }
            actualDataIds = dataIds;
            actualDataNodeIds = dataNodeIds;
            actualNodeOffsets = nodeOffsets;
            actualNodeSizes = nodeSizes;
            ++*updateCallCount;
        }

        void doDraw(const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& dataNodeIds, std::size_t offset, std::size_t count, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes) override {
            CORRADE_ITERATION(handle());
            /* doSetSize() should have been called exactly once at this point */
            CORRADE_COMPARE(setSizeCallCount, 1);
            /* The passed views should be exactly the same */
            CORRADE_COMPARE(dataIds.data(), actualDataIds.data());
            CORRADE_COMPARE(dataIds.size(), actualDataIds.size());
            CORRADE_COMPARE(dataIds.stride(), actualDataIds.stride());
            CORRADE_COMPARE(dataNodeIds.data(), actualDataNodeIds.data());
            CORRADE_COMPARE(dataNodeIds.size(), actualDataNodeIds.size());
            CORRADE_COMPARE(dataNodeIds.stride(), actualDataNodeIds.stride());
            CORRADE_COMPARE(nodeOffsets.data(), actualNodeOffsets.data());
            CORRADE_COMPARE(nodeOffsets.size(), actualNodeOffsets.size());
            CORRADE_COMPARE(nodeOffsets.stride(), actualNodeOffsets.stride());
            CORRADE_COMPARE(nodeSizes.data(), actualNodeSizes.data());
            CORRADE_COMPARE(nodeSizes.size(), actualNodeSizes.size());
            CORRADE_COMPARE(nodeSizes.stride(), actualNodeSizes.stride());
            arrayAppend(*drawCalls, InPlaceInit, handle(), offset, count);
        }

        LayerFeatures features;
        Containers::StridedArrayView1D<const Containers::Pair<UnsignedInt, UnsignedInt>> expectedData;
        Containers::StridedArrayView1D<const Containers::Pair<Vector2, Vector2>> expectedNodeOffsetsSizes;
        Int* updateCallCount;
        Int setSizeCallCount = 0;
        Containers::Array<Containers::Triple<LayerHandle, std::size_t, std::size_t>>* drawCalls;

        Containers::StridedArrayView1D<const UnsignedInt> actualDataIds;
        Containers::StridedArrayView1D<const UnsignedInt> actualDataNodeIds;
        Containers::StridedArrayView1D<const Vector2> actualNodeOffsets;
        Containers::StridedArrayView1D<const Vector2> actualNodeSizes;
    };

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    /*   1 2      3 4 5       6 7 8      9 10     11
       3 +------------------------+
         |         top level      |
       4 |          +-----------+ |      +--------+
       5 | +------+ |  right  +----------| not in |
       6 | | left | | +-------|          | order  |
       7 | +------+ | | nested|  another +--------+
       8 |          +-+-------| top level  |
       9 +------------+-------|          +--------+
      10   +--------+ |layer1 +----------| layer2 |
           | culled | |  only   |        |  only  |
      11   +--------+ +---------+        +--------+ */
    NodeHandle topLevel = ui.createNode({1.0f, 3.0f}, {7.0f, 6.0f});
    NodeHandle left = ui.createNode(topLevel, {1.0f, 2.0f}, {1.0f, 2.0f}, NodeFlag::Clip);
    NodeHandle right = ui.createNode(topLevel, {3.0f, 1.0f}, {3.0f, 4.0f});
    NodeHandle layer1Only = ui.createNode({5.0f, 9.0f}, {2.0f, 2.0f});
    NodeHandle anotherTopLevel = ui.createNode({6.0f, 5.0f}, {4.0f, 5.0f});
    NodeHandle layer2Only = ui.createNode({9.0f, 9.0f}, {2.0f, 2.0f});
    NodeHandle topLevelNotInOrder = ui.createNode({9.0f, 4.0f}, {2.0f, 3.0f});
    NodeHandle removed = ui.createNode(right, {}, {});
    NodeHandle culled = ui.createNode(left, {0.0f, 5.0f}, {2.0f, 1.0f});
    NodeHandle nested = ui.createNode(right, {1.0f, 2.0f}, {2.0f, 2.0f});

    /* These follow the node handle IDs, nodes that are not part of the
       visible hierarchy have the data undefined */
    Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
        {{1.0f, 3.0f}, {7.0f, 6.0f}}, /* topLevel */
        {{2.0f, 5.0f}, {1.0f, 2.0f}}, /* left */
        {{4.0f, 4.0f}, {3.0f, 4.0f}}, /* right */
        {{5.0f, 9.0f}, {2.0f, 2.0f}}, /* layer1Only */
        {{6.0f, 5.0f}, {4.0f, 5.0f}}, /* anotherTopLevel */
        {{9.0f, 9.0f}, {2.0f, 2.0f}}, /* layer2Only */
        {},                           /* removed */
        {},                           /* not in order */
        {},                           /* culled */
        {{5.0f, 6.0f}, {2.0f, 2.0f}}, /* nested */
    };

    /* Layer without an instance, to verify those get skipped during updates */
    /*LayerHandle layerWithoutInstance =*/ ui.createLayer();

    LayerHandle layer1, layer2, layerRemoved, layer3;
    if(!data.reorderLayers) {
        layer1 = ui.createLayer();
        layer2 = ui.createLayer();
        layerRemoved = ui.createLayer();
        layer3 = ui.createLayer();
    } else {
        layer3 = ui.createLayer();
        layer2 = ui.createLayer(layer3);
        layerRemoved = ui.createLayer();
        layer1 = ui.createLayer(layer2);
    }

    /* Layer that's subsequently removed, to verify it also gets skipped during
       updates */
    ui.removeLayer(layerRemoved);

    Containers::Pointer<Layer> layer1Instance{InPlaceInit, layer1, LayerFeature::Draw};

    Containers::Pointer<Layer> layer2Instance{InPlaceInit, layer2, LayerFeature::Draw|LayerFeature::Event};

    Containers::Pointer<Layer> layer3Instance{InPlaceInit, layer3, LayerFeature::Event};

    DataHandle leftData2 = layer1Instance->create();
    DataHandle leftData1 = layer2Instance->create();
    DataHandle leftData3 = layer1Instance->create();
    DataHandle anotherTopLevelData1 = layer1Instance->create();
    DataHandle anotherTopLevelData2 = layer2Instance->create();
    DataHandle anotherTopLevelData3 = layer3Instance->create();
    DataHandle anotherTopLevelData4 = layer2Instance->create();
    DataHandle topLevelData = layer3Instance->create();
    DataHandle culledData = layer2Instance->create();
    DataHandle nestedData = layer2Instance->create();
    DataHandle topLevelNotInOrderData = layer2Instance->create();
    DataHandle removedData = layer1Instance->create();
    DataHandle rightData1 = layer3Instance->create();
    DataHandle rightData2 = layer2Instance->create();
    DataHandle layer1OnlyData = layer1Instance->create();
    DataHandle layer2OnlyData = layer2Instance->create();

    /* These follow the node nesting order and then the order in which the
       data get attached below */
    Containers::Pair<UnsignedInt, UnsignedInt> expectedLayer1Data[]{
        /* anotherTopLevel is reordered as first */
        {dataHandleId(anotherTopLevelData1), nodeHandleId(anotherTopLevel)},
        /* Data belonging to topLevel are after it */
        {dataHandleId(leftData2), nodeHandleId(left)},
        {dataHandleId(leftData3), nodeHandleId(left)},
        /* removedData not here as the containing node is removed */
        {dataHandleId(layer1OnlyData), nodeHandleId(layer1Only)},
    };
    Containers::Pair<UnsignedInt, UnsignedInt> expectedLayer2Data[]{
        /* anotherTopLevel */
        {dataHandleId(anotherTopLevelData2), nodeHandleId(anotherTopLevel)},
        {dataHandleId(anotherTopLevelData4), nodeHandleId(anotherTopLevel)},
        /* topLevel */
        {dataHandleId(leftData1), nodeHandleId(left)},
        {dataHandleId(rightData2), nodeHandleId(right)},
        {dataHandleId(nestedData), nodeHandleId(nested)},
        /* Nothing for topLevelNotInOrderData and culledData as they're not
           visible */
        {dataHandleId(layer2OnlyData), nodeHandleId(layer2Only)},
    };
    Containers::Pair<UnsignedInt, UnsignedInt> expectedLayer3Data[]{
        /* anotherTopLevel */
        {dataHandleId(anotherTopLevelData3), nodeHandleId(anotherTopLevel)},
        /* topLevel */
        {dataHandleId(topLevelData), nodeHandleId(topLevel)},
        {dataHandleId(rightData1), nodeHandleId(right)},
    };

    layer1Instance->expectedData = expectedLayer1Data;
    layer2Instance->expectedData = expectedLayer2Data;
    layer3Instance->expectedData = expectedLayer3Data;
    layer1Instance->expectedNodeOffsetsSizes = expectedNodeOffsetsSizes;
    layer2Instance->expectedNodeOffsetsSizes = expectedNodeOffsetsSizes;
    layer3Instance->expectedNodeOffsetsSizes = expectedNodeOffsetsSizes;
    Int layer1UpdateCallCount = 0;
    Int layer2UpdateCallCount = 0;
    Int layer3UpdateCallCount = 0;
    layer1Instance->updateCallCount = &layer1UpdateCallCount;
    layer2Instance->updateCallCount = &layer2UpdateCallCount;
    layer3Instance->updateCallCount = &layer3UpdateCallCount;
    Containers::Array<Containers::Triple<LayerHandle, std::size_t, std::size_t>> drawCalls;
    layer1Instance->drawCalls = &drawCalls;
    layer2Instance->drawCalls = &drawCalls;
    layer3Instance->drawCalls = &drawCalls;
    ui.setLayerInstance(Utility::move(layer1Instance));
    ui.setLayerInstance(Utility::move(layer2Instance));
    ui.setLayerInstance(Utility::move(layer3Instance));

    ui.attachData(nested, nestedData);
    ui.attachData(left, leftData1);
    ui.attachData(anotherTopLevel, anotherTopLevelData1);
    ui.attachData(anotherTopLevel, anotherTopLevelData2);
    ui.attachData(anotherTopLevel, anotherTopLevelData3);
    ui.attachData(anotherTopLevel, anotherTopLevelData4);
    ui.attachData(left, leftData2);
    ui.attachData(topLevelNotInOrder, topLevelNotInOrderData);
    ui.attachData(removed, removedData);
    ui.attachData(topLevel, topLevelData);
    ui.attachData(right, rightData1);
    ui.attachData(left, leftData3);
    ui.attachData(culled, culledData);
    ui.attachData(right, rightData2);
    ui.attachData(layer1Only, layer1OnlyData);
    ui.attachData(layer2Only, layer2OnlyData);

    ui.setNodeOrder(anotherTopLevel, topLevel);
    ui.clearNodeOrder(topLevelNotInOrder);
    ui.removeNode(removed);
    CORRADE_COMPARE(ui.dataAttachmentCount(), 16);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeClean);
    CORRADE_COMPARE(layer1UpdateCallCount, 0);
    CORRADE_COMPARE(layer2UpdateCallCount, 0);
    CORRADE_COMPARE(layer3UpdateCallCount, 0);

    if(data.clean) {
        ui.clean();
        CORRADE_COMPARE(ui.dataAttachmentCount(), 15);
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);
        CORRADE_COMPARE(layer1UpdateCallCount, 0);
        CORRADE_COMPARE(layer2UpdateCallCount, 0);
        CORRADE_COMPARE(layer3UpdateCallCount, 0);
    }

    /* update() should call clean() only if needed */
    if(data.update) {
        ui.update();
        CORRADE_COMPARE(ui.dataAttachmentCount(), 15);
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
        CORRADE_COMPARE(layer1UpdateCallCount, 1);
        CORRADE_COMPARE(layer2UpdateCallCount, 1);
        CORRADE_COMPARE(layer3UpdateCallCount, 1);
    }

    /* draw() should call update() and clean() only if needed */
    ui.draw();
    CORRADE_COMPARE(ui.dataAttachmentCount(), 15);
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(layer1UpdateCallCount, 1);
    CORRADE_COMPARE(layer2UpdateCallCount, 1);
    CORRADE_COMPARE(layer3UpdateCallCount, 1);
    CORRADE_COMPARE_AS(drawCalls, (Containers::arrayView<Containers::Triple<LayerHandle, std::size_t, std::size_t>>({
        /* anotherTopLevel rendered first */
            /* first data from expectedLayer1Data */
            {layer1, 0, 1},
            /* first two data from expectedLayer2Data */
            {layer2, 0, 2},
        /* then topLevel */
            /* remaining data from expectedLayer1Data */
            {layer1, 1, 2},
            /* and then remaining data from expectedLayer2Data */
            {layer2, 2, 3},
        /* then layer1Only, with only data from layer 1 */
            {layer1, 3, 1},
        /* then layer2Only, with only data from layer 2 */
            {layer2, 5, 1},
        /* layer 3 doesn't have LayerFeature::Draw, so draw() shouldn't be
           called with anything for it */
    })), TestSuite::Compare::Container);
}

void AbstractUserInterfaceTest::eventEmpty() {
    auto&& data = CleanUpdateData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui{{100, 100}};
    CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);
    CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

    /* Just verify that this doesn't crash or assert, there's nothing visibly
       changing after these calls; the events stay unaccepted */
    if(data.clean) {
        ui.clean();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }
    if(data.update) {
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }
    PointerEvent pointerEvent{Pointer::MouseRight};
    PointerMoveEvent pointerMoveEvent{{}, {}};
    CORRADE_VERIFY(!ui.pointerPressEvent({}, pointerEvent));
    CORRADE_VERIFY(!pointerEvent.isAccepted());
    CORRADE_VERIFY(!ui.pointerReleaseEvent({}, pointerEvent));
    CORRADE_VERIFY(!pointerEvent.isAccepted());
    CORRADE_VERIFY(!ui.pointerMoveEvent({}, pointerMoveEvent));
    CORRADE_VERIFY(!pointerMoveEvent.isAccepted());
    CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);
    CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
}

void AbstractUserInterfaceTest::eventAlreadyAccepted() {
    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui{{100, 100}};
    PointerEvent pointerEvent{Pointer::MouseRight};
    pointerEvent.setAccepted();
    PointerMoveEvent pointerMoveEvent{{}, {}};
    pointerMoveEvent.setAccepted();

    std::ostringstream out;
    Error redirectError{&out};
    ui.pointerPressEvent({}, pointerEvent);
    ui.pointerReleaseEvent({}, pointerEvent);
    ui.pointerMoveEvent({}, pointerMoveEvent);
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractUserInterface::pointerPressEvent(): event already accepted\n"
        "Whee::AbstractUserInterface::pointerReleaseEvent(): event already accepted\n"
        "Whee::AbstractUserInterface::pointerMoveEvent(): event already accepted\n");
}

void AbstractUserInterfaceTest::eventNodePropagation() {
    auto&& data = CleanUpdateData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* framebufferSize isn't used for anything here; events should get scaled
       to (0.1, 0.01) */
    AbstractUserInterface ui{{300.0f, 200.0f}, {3000.0f, 20000.0f}, {30, 20}};

    struct Layer: AbstractLayer {
        explicit Layer(LayerHandle handle, LayerFeatures features): AbstractLayer{handle}, features{features} {}

        LayerFeatures doFeatures() const override { return features; }

        /* doClean() / doUpdate() tested thoroughly enough in draw() above */

        void doPointerPressEvent(UnsignedInt dataId, PointerEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(*eventCalls, InPlaceInit, dataHandle(handle(), dataId, 1), event.position(), *accept);
            if(*accept)
                event.setAccepted();
        }
        void doPointerReleaseEvent(UnsignedInt, PointerEvent&) override {
            CORRADE_FAIL("This function shouldn't be called");
        }
        void doPointerMoveEvent(UnsignedInt, PointerMoveEvent&) override {
            CORRADE_FAIL("This function shouldn't be called");
        }
        void doPointerEnterEvent(UnsignedInt, PointerMoveEvent&) override {
            CORRADE_FAIL("This function shouldn't be called");
        }
        void doPointerLeaveEvent(UnsignedInt, PointerMoveEvent&) override {
            CORRADE_FAIL("This function shouldn't be called");
        }

        LayerFeatures features;
        bool* accept;
        Containers::Array<Containers::Triple<DataHandle, Vector2, bool>>* eventCalls;
    };

    NodeHandle bottom = ui.createNode({10.0f, 20.0f}, {110.0f, 50.0f});
    NodeHandle top = ui.createNode({15.0f, 25.0f}, {90.0f, 45.0f});
    NodeHandle topNested = ui.createNode(top, {20.0f, 30.0f}, {10.0f, 10.0f});
    NodeHandle removed = ui.createNode(topNested, {}, {10.0f, 10.0f});
    NodeHandle notInOrder = ui.createNode({}, {200.0f, 200.0f});
    NodeHandle hidden = ui.createNode({}, {200.0f, 200.0f}, NodeFlag::Hidden);
    NodeHandle topNestedOutside = ui.createNode(topNested, {7.5f, 7.5f}, {10.0f, 10.0f});

    LayerHandle layer1 = ui.createLayer();
    Containers::Pointer<Layer> layer1Instance{InPlaceInit, layer1, LayerFeature::Event};

    LayerHandle layer2 = ui.createLayer();
    Containers::Pointer<Layer> layer2Instance{InPlaceInit, layer2, LayerFeature::Draw};

    LayerHandle layer3 = ui.createLayer();
    Containers::Pointer<Layer> layer3Instance{InPlaceInit, layer3, LayerFeature::Draw|LayerFeature::Event};

    DataHandle bottomData1 = layer1Instance->create();
    DataHandle bottomData2 = layer2Instance->create();
    DataHandle topNestedData1 = layer3Instance->create();
    DataHandle topNestedData2 = layer1Instance->create();
    DataHandle topNestedData3 = layer3Instance->create();
    DataHandle topNestedOutsideData = layer3Instance->create();
    DataHandle notInOrderData = layer1Instance->create();
    DataHandle hiddenData = layer2Instance->create();
    DataHandle removedData = layer3Instance->create();
    DataHandle topData = layer3Instance->create();

    bool layer1Accept = true;
    bool layer2Accept = true;
    bool layer3Accept = true;
    layer1Instance->accept = &layer1Accept;
    layer2Instance->accept = &layer2Accept;
    layer3Instance->accept = &layer3Accept;
    Containers::Array<Containers::Triple<DataHandle, Vector2, bool>> eventCalls;
    layer1Instance->eventCalls = &eventCalls;
    layer2Instance->eventCalls = &eventCalls;
    layer3Instance->eventCalls = &eventCalls;
    ui.setLayerInstance(Utility::move(layer1Instance));
    ui.setLayerInstance(Utility::move(layer2Instance));
    ui.setLayerInstance(Utility::move(layer3Instance));

    ui.attachData(bottom, bottomData1);
    ui.attachData(bottom, bottomData2);
    ui.attachData(top, topData);
    ui.attachData(topNested, topNestedData2);
    ui.attachData(topNested, topNestedData1);
    ui.attachData(topNested, topNestedData3);
    ui.attachData(topNestedOutside, topNestedOutsideData);
    ui.attachData(notInOrder, notInOrderData);
    ui.attachData(hidden, hiddenData);
    ui.attachData(removed, removedData);

    ui.clearNodeOrder(notInOrder);
    ui.removeNode(removed);
    CORRADE_COMPARE(ui.dataAttachmentCount(), 10);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeClean);

    if(data.clean) {
        ui.clean();
        CORRADE_COMPARE(ui.dataAttachmentCount(), 9);
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);
    }

    /* update() should call clean() only if needed */
    if(data.update) {
        ui.update();
        CORRADE_COMPARE(ui.dataAttachmentCount(), 9);
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    /* Completely outside, no hit */
    {
        layer1Accept = layer2Accept = layer3Accept = true;
        eventCalls = {};
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(!ui.pointerPressEvent({3000.0f, 30000.0f}, event));
        CORRADE_COMPARE_AS(eventCalls, (Containers::arrayView<Containers::Triple<DataHandle, Vector2, bool>>({
        })), TestSuite::Compare::Container);

    /* On the notInOrder node that's not visible, no hit */
    } {
        layer1Accept = layer2Accept = layer3Accept = true;
        eventCalls = {};
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(!ui.pointerPressEvent({1950.0f, 19500.0f}, event));
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
        CORRADE_COMPARE_AS(eventCalls, (Containers::arrayView<Containers::Triple<DataHandle, Vector2, bool>>({
        })), TestSuite::Compare::Container);

    /* On the top-level node with no other node covering it */
    } {
        layer1Accept = layer2Accept = layer3Accept = true;
        eventCalls = {};
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({1000.0f, 6000.0f}, event));
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
        CORRADE_COMPARE_AS(eventCalls, (Containers::arrayView<Containers::Triple<DataHandle, Vector2, bool>>({
            {topData, {85.0f, 35.0f}, true},
        })), TestSuite::Compare::Container);

    /* On the bottom node with no other node covering it (which is only the
       bottom side) */
    } {
        layer1Accept = layer2Accept = layer3Accept = true;
        eventCalls = {};
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({1150.0f, 6000.0f}, event));
        CORRADE_COMPARE_AS(eventCalls, (Containers::arrayView<Containers::Triple<DataHandle, Vector2, bool>>({
            /* The bottomData2 don't get selected as it's from layer2 that
               doesn't have LayerFeature::Event */
            {bottomData1, {105.0f, 40.0f}, true},
        })), TestSuite::Compare::Container);

    /* On the bottom node with no other node covering it (which is only the
       bottom side), if the data doesn't accept the event, falls back to
       nothing */
    } {
        layer1Accept = layer2Accept = false;
        layer3Accept = true;
        eventCalls = {};
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(!ui.pointerPressEvent({1150.0f, 6000.0f}, event));
        CORRADE_COMPARE_AS(eventCalls, (Containers::arrayView<Containers::Triple<DataHandle, Vector2, bool>>({
            {bottomData1, {105.0f, 40.0f}, false},
            /* The bottomData2 don't get selected as it's from layer2 that
               doesn't have LayerFeature::Event */
        })), TestSuite::Compare::Container);

    /* On the top-level node, falls back to the bottom node */
    } {
        layer1Accept = layer2Accept = true;
        layer3Accept = false;
        eventCalls = {};
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({1000.0f, 6000.0f}, event));
        CORRADE_COMPARE_AS(eventCalls, (Containers::arrayView<Containers::Triple<DataHandle, Vector2, bool>>({
            {topData, {85.0f, 35.0f}, false},
            {bottomData1, {90.0f, 40.0f}, true}
        })), TestSuite::Compare::Container);

    /* On a nested node, last added data get picked first */
    } {
        layer1Accept = layer2Accept = layer3Accept = true;
        eventCalls = {};
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({400.0f, 6000.0f}, event));
        CORRADE_COMPARE_AS(eventCalls, (Containers::arrayView<Containers::Triple<DataHandle, Vector2, bool>>({
            /* There's removedData covering this same position but weren't
               considered as they belong to a removed node */
            {topNestedData3, {5.0f, 5.0f}, true},
        })), TestSuite::Compare::Container);

    /* On a nested node, if the first doesn't accept the event, falls back to
       the next added data, and then to the next layer in order */
    } {
        layer1Accept = layer2Accept = true;
        layer3Accept = false;
        eventCalls = {};
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({400.0f, 6000.0f}, event));
        CORRADE_COMPARE_AS(eventCalls, (Containers::arrayView<Containers::Triple<DataHandle, Vector2, bool>>({
            {topNestedData3, {5.0f, 5.0f}, false},
            {topNestedData1, {5.0f, 5.0f}, false},
            {topNestedData2, {5.0f, 5.0f}, true},
        })), TestSuite::Compare::Container);

    /* Fall through everything */
    } {
        layer1Accept = layer2Accept = layer3Accept = false;
        eventCalls = {};
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(!ui.pointerPressEvent({400.0f, 6000.0f}, event));
        CORRADE_COMPARE_AS(eventCalls, (Containers::arrayView<Containers::Triple<DataHandle, Vector2, bool>>({
            {topNestedData3, {5.0f, 5.0f}, false},
            {topNestedData1, {5.0f, 5.0f}, false},
            {topNestedData2, {5.0f, 5.0f}, false},
            {topData, {25.0f, 35.0f}, false},
            {bottomData1, {30.0f, 40.0f}, false}
        })), TestSuite::Compare::Container);

    /* Only the area of a nested node that is inside of its parent is
       considered for a hit */
    } {
        layer1Accept = layer2Accept = layer3Accept = true;
        eventCalls = {};
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({430.0f, 6300.0f}, event));
        CORRADE_COMPARE_AS(eventCalls, (Containers::arrayView<Containers::Triple<DataHandle, Vector2, bool>>({
            {topNestedOutsideData, {0.5f, 0.5f}, true},
        })), TestSuite::Compare::Container);
    } {
        layer1Accept = layer2Accept = layer3Accept = true;
        eventCalls = {};
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({460.0f, 6600.0f}, event));
        CORRADE_COMPARE_AS(eventCalls, (Containers::arrayView<Containers::Triple<DataHandle, Vector2, bool>>({
            /* It's at {3.5f, 3.5f} for topNestedOutside, but that's outside
               of topNested so it isn't considered */
            {topData, {31.0f, 41.0f}, true},
        })), TestSuite::Compare::Container);
    }

    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
}

void AbstractUserInterfaceTest::eventEdges() {
    /* framebufferSize isn't used for anything here; events should get scaled
       to (0.1, 0.01) */
    AbstractUserInterface ui{{300.0f, 200.0f}, {3000.0f, 20000.0f}, {30, 20}};

    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doPointerPressEvent(UnsignedInt dataId, PointerEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, dataHandle(handle(), dataId, 1), event.position());
            event.setAccepted();
        }

        Containers::Array<Containers::Pair<DataHandle, Vector2>> eventCalls;
    };

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<Layer>(layer));

    NodeHandle bottom = ui.createNode({0.0f, 0.0f}, {100.0f, 100.0f});
    NodeHandle top = ui.createNode({10.0f, 20.0f}, {80.0f, 60.0f});
    DataHandle bottomData = ui.layer<Layer>(layer).create();
    DataHandle topData = ui.layer<Layer>(layer).create();
    ui.attachData(bottom, bottomData);
    ui.attachData(top, topData);

    /* Top left corner should go to the top node */
    {
        ui.layer<Layer>(layer).eventCalls = {};
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({100.0f, 2000.0f}, event));
        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Pair<DataHandle, Vector2>>({
            {topData, {0.0f, 0.0f}},
        })), TestSuite::Compare::Container);

    /* Top edge should go to the top node */
    } {
        ui.layer<Layer>(layer).eventCalls = {};
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({570.0f, 2000.0f}, event));
        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Pair<DataHandle, Vector2>>({
            {topData, {47.0f, 0.0f}},
        })), TestSuite::Compare::Container);

    /* Left edge should go to the top node */
    } {
        ui.layer<Layer>(layer).eventCalls = {};
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({100.0f, 3400.0f}, event));
        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Pair<DataHandle, Vector2>>({
            {topData, {0.0f, 14.0f}},
        })), TestSuite::Compare::Container);

    /* Bottom right corner should go to the bottom node */
    } {
        ui.layer<Layer>(layer).eventCalls = {};
        PointerEvent event1{Pointer::MouseLeft};
        PointerEvent event2{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({900.0f, 8000.0f}, event1));
        CORRADE_VERIFY(ui.pointerPressEvent({899.0f, 7990.0f}, event2));
        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Pair<DataHandle, Vector2>>({
            {bottomData, {90.0f, 80.0f}},
            {topData, {79.9f, 59.9f}},
        })), TestSuite::Compare::Container);

    /* Bottom edge should go to the top node */
    } {
        ui.layer<Layer>(layer).eventCalls = {};
        PointerEvent event1{Pointer::MouseLeft};
        PointerEvent event2{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({900.0f, 3400.0f}, event1));
        CORRADE_VERIFY(ui.pointerPressEvent({899.0f, 3400.0f}, event2));
        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Pair<DataHandle, Vector2>>({
            {bottomData, {90.0f, 34.0f}},
            {topData, {79.9f, 14.0f}},
        })), TestSuite::Compare::Container);

    /* Right edge should go to the bottom node */
    } {
        ui.layer<Layer>(layer).eventCalls = {};
        PointerEvent event1{Pointer::MouseLeft};
        PointerEvent event2{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({570.0f, 8000.0f}, event1));
        CORRADE_VERIFY(ui.pointerPressEvent({570.0f, 7990.0f}, event2));
        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Pair<DataHandle, Vector2>>({
            {bottomData, {57.0f, 80.0f}},
            {topData, {47.0f, 59.9f}},
        })), TestSuite::Compare::Container);
    }
}

void AbstractUserInterfaceTest::eventPointerPress() {
    auto&& data = UpdateData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* framebufferSize isn't used for anything here; events should get scaled
       to (0.1, 0.01) */
    AbstractUserInterface ui{{300.0f, 200.0f}, {3000.0f, 20000.0f}, {30, 20}};

    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doPointerPressEvent(UnsignedInt dataId, PointerEvent& event) override {
            CORRADE_COMPARE(dataId, 1);
            CORRADE_COMPARE(event.position(), (Vector2{10.0f, 5.0f}));
            ++acceptedCount;
            event.setAccepted();
        }
        void doPointerReleaseEvent(UnsignedInt, PointerEvent&) override {
            CORRADE_FAIL("This function shouldn't be called.");
        }
        void doPointerMoveEvent(UnsignedInt, PointerMoveEvent&) override {
            CORRADE_FAIL("This function shouldn't be called.");
        }
        void doPointerEnterEvent(UnsignedInt, PointerMoveEvent&) override {
            CORRADE_FAIL("This function shouldn't be called.");
        }
        void doPointerLeaveEvent(UnsignedInt, PointerMoveEvent&) override {
            CORRADE_FAIL("This function shouldn't be called.");
        }

        Int acceptedCount = 0;
    };

    NodeHandle node = ui.createNode({10.0f, 20.0f}, {20.0f, 20.0f});

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<Layer>(layer));
    /*DataHandle data1 =*/ ui.layer<Layer>(layer).create();
    DataHandle data2 = ui.layer<Layer>(layer).create();
    ui.attachData(node, data2);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    /* Outside, no hit */
    {
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(!ui.pointerPressEvent({1000.0f, 10000.0f}, event));
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.layer<Layer>(layer).acceptedCount, 0);

    /* Inside, hit */
    } {
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({200.0f, 2500.0f}, event));
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), node);
        CORRADE_COMPARE(ui.layer<Layer>(layer).acceptedCount, 1);
    }

    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
}

void AbstractUserInterfaceTest::eventPointerRelease() {
    auto&& data = UpdateData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* framebufferSize isn't used for anything here; events should get scaled
       to (0.1, 0.01) */
    AbstractUserInterface ui{{300.0f, 200.0f}, {3000.0f, 20000.0f}, {30, 20}};

    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doPointerPressEvent(UnsignedInt, PointerEvent&) override {
            CORRADE_FAIL("This function shouldn't be called.");
        }
        void doPointerReleaseEvent(UnsignedInt dataId, PointerEvent& event) override {
            CORRADE_COMPARE(dataId, 1);
            CORRADE_COMPARE(event.position(), (Vector2{10.0f, 5.0f}));
            ++acceptedCount;
            event.setAccepted();
        }
        void doPointerMoveEvent(UnsignedInt, PointerMoveEvent&) override {
            CORRADE_FAIL("This function shouldn't be called.");
        }
        void doPointerEnterEvent(UnsignedInt, PointerMoveEvent&) override {
            CORRADE_FAIL("This function shouldn't be called.");
        }
        void doPointerLeaveEvent(UnsignedInt, PointerMoveEvent&) override {
            CORRADE_FAIL("This function shouldn't be called.");
        }

        Int acceptedCount = 0;
    };

    NodeHandle node = ui.createNode({10.0f, 20.0f}, {20.0f, 20.0f});

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<Layer>(layer));
    /*DataHandle data1 =*/ ui.layer<Layer>(layer).create();
    DataHandle data2 = ui.layer<Layer>(layer).create();
    ui.attachData(node, data2);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    /* Outside, no hit */
    {
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(!ui.pointerReleaseEvent({1000.0f, 10000.0f}, event));
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.layer<Layer>(layer).acceptedCount, 0);

    /* Inside, hit */
    } {
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({200.0f, 2500.0f}, event));
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.layer<Layer>(layer).acceptedCount, 1);
    }

    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
}

void AbstractUserInterfaceTest::eventPointerMove() {
    auto&& data = UpdateData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* framebufferSize isn't used for anything here; events should get scaled
       to (0.1, 0.01) */
    AbstractUserInterface ui{{300.0f, 200.0f}, {3000.0f, 20000.0f}, {30, 20}};

    enum Event {
        Move = 2,
        Enter = 4,
        Leave = 6
    };
    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doPointerPressEvent(UnsignedInt, PointerEvent&) override {
            CORRADE_FAIL("This function shouldn't be called.");
        }
        void doPointerReleaseEvent(UnsignedInt, PointerEvent&) override {
            CORRADE_FAIL("This function shouldn't be called.");
        }
        void doPointerMoveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Move, dataHandle(handle(), dataId, 1), Vector4{event.position().x(), event.position().y(), event.relativePosition().x(), event.relativePosition().y()});
            event.setAccepted();
        }
        void doPointerEnterEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Enter, dataHandle(handle(), dataId, 1), Vector4{event.position().x(), event.position().y(), event.relativePosition().x(), event.relativePosition().y()});
        }
        void doPointerLeaveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Leave, dataHandle(handle(), dataId, 1), Vector4{event.position().x(), event.position().y(), event.relativePosition().x(), event.relativePosition().y()});
        }

        Containers::Array<Containers::Triple<Int, DataHandle, Vector4>> eventCalls;
    };

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<Layer>(layer));

    /* Two nodes next to each other */
    NodeHandle left = ui.createNode({20.0f, 0.0f}, {20.0f, 20.0f});
    NodeHandle right = ui.createNode({40.0f, 0.0f}, {20.0f, 20.0f});
    DataHandle leftData = ui.layer<Layer>(layer).create();
    DataHandle rightData = ui.layer<Layer>(layer).create();
    ui.attachData(left, leftData);
    ui.attachData(right, rightData);

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    /* Outside, no hit */
    {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerMoveEvent event{{}, {}};
        CORRADE_VERIFY(!ui.pointerMoveEvent({100.0f, 1000.0f}, event));
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector4>>({
        })), TestSuite::Compare::Container);

    /* Inside a node. Relative to previous move event even though it didn't hit
       anything, the hovered node gets set to given node. */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerMoveEvent event1{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({300.0f, 1100.0f}, event1));
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), left);
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);

        PointerMoveEvent event2{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({350.0f, 1000.0f}, event2));
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), left);
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector4>>({
            /* There's nothing to receive a Leave event */
            {Move, leftData, {10.0f, 11.0f, 20.0f, 1.0f}},
            /* It has to first execute the Move to discover a node that accepts
               the event, thus Enter can't be before the Move */
            {Enter, leftData, {10.0f, 11.0f, 0.0f, 0.0f}},
            {Move, leftData, {15.0f, 10.0f, 5.0f, -1.0f}},
            /* It stays on the same node, so no further Enter or Leave */
        })), TestSuite::Compare::Container);

    /* Inside and then to another node. Relative to previous move event even
       though it didn't hit anything, the hovered node changes based on what's
       under the pointer at the moment. */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerMoveEvent event1{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({300.0f, 1100.0f}, event1));
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), left);
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);

        PointerMoveEvent event2{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({550.0f, 1000.0f}, event2));
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), right);
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector4>>({
            /* It stays on the same node, so no further Enter or Leave */
            {Move, leftData, {10.0f, 11.0f, -5.0f, 1.0f}},
            {Move, rightData, {15.0f, 10.0f, 25.0f, -1.0f}},
            /* It has to first execute the Move to discover the next node that
               accepts the event, thus Leave can't be before the Move either
               because it could end up at the same node. Tested thoroughly in
               eventPointerMoveNotAccepted() below. */
            {Leave, leftData, {35.0f, 10.0f, 0.0f, 0.0f}},
            {Enter, rightData, {15.0f, 10.0f, 0.0f, 0.0f}},
        })), TestSuite::Compare::Container);

    /* Out of the item, again relative to what happened last */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerMoveEvent event{{}, {}};
        CORRADE_VERIFY(!ui.pointerMoveEvent({100.0f, 1100.0f}, event));
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector4>>({
            {Leave, rightData, {-30.0f, 11.0f, 0.0f, 0.0f}},
            /* There's nothing to receive a Move event afterwards */
        })), TestSuite::Compare::Container);

    /* After changing the UI size, the relative position should be still in the
       already scaled units */
    } {
        /* Events should get scaled to (0.01, 0.1), i.e. the scale is flipped
           now */
        ui.setSize({300.0f, 200.0f}, {30000.0f, 2000.0f}, {30, 20});
        ui.layer<Layer>(layer).eventCalls = {};

        /* Back hovering on the right node */
        PointerMoveEvent event2{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({5500.0f, 100.0f}, event2));
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), right);
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector4>>({
            /* Is relative to the {-30, 11} that was above, without considering
               the 10x / 100x scale in any way */
            {Move, rightData, {15.0f, 10.0f, 45.0f, -1.0f}},
            {Enter, rightData, {15.0f, 10.0f, 0.0f, 0.0f}},
        })), TestSuite::Compare::Container);
    }

    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
}

void AbstractUserInterfaceTest::eventPointerMoveRelativePositionWithPressRelease() {
    auto&& data = EventPointerMoveRelativePositionWithPressReleaseData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Compared to eventPointerMove(), verifies that combining with press and
       release events also updates the relative position appropriately, and
       does it even if the events aren't accepted. The Enter and Leave events
       are enforced to have the relative position a zero vector so they
       aren't tested here. */

    /* framebufferSize isn't used for anything here; events should get scaled
       to (0.1, 0.01) */
    AbstractUserInterface ui{{300.0f, 200.0f}, {3000.0f, 20000.0f}, {30, 20}};

    enum Event {
        Press = 0,
        Release = 1,
        Move = 2
    };
    struct Layer: AbstractLayer {
        explicit Layer(LayerHandle handle, bool accept): AbstractLayer{handle}, accept{accept} {}

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doPointerPressEvent(UnsignedInt dataId, PointerEvent& event) override {
            CORRADE_COMPARE(dataId, 1);
            arrayAppend(eventCalls, InPlaceInit, Press, Vector4{event.position().x(), event.position().y(), 0.0f, 0.0f});
            if(accept)
                event.setAccepted();
        }
        void doPointerReleaseEvent(UnsignedInt dataId, PointerEvent& event) override {
            CORRADE_COMPARE(dataId, 1);
            arrayAppend(eventCalls, InPlaceInit, Release, Vector4{event.position().x(), event.position().y(), 0.0f, 0.0f});
            if(accept)
                event.setAccepted();
        }
        void doPointerMoveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            CORRADE_COMPARE(dataId, 1);
            arrayAppend(eventCalls, InPlaceInit, Move, Vector4{event.position().x(), event.position().y(), event.relativePosition().x(), event.relativePosition().y()});
            if(accept)
                event.setAccepted();
        }

        Containers::Array<Containers::Pair<Int, Vector4>> eventCalls;
        bool accept;
    };

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<Layer>(layer, data.accept));

    NodeHandle node = ui.createNode({20.0f, 0.0f}, {20.0f, 20.0f});
    /*DataHandle nodeData1 =*/ ui.layer<Layer>(layer).create();
    DataHandle nodeData2 = ui.layer<Layer>(layer).create();
    ui.attachData(node, nodeData2);

    /* Press, move, release, move on the same node */
    {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent pressEvent{Pointer::MouseRight};
        CORRADE_COMPARE(ui.pointerPressEvent({300.0f, 1000.0f}, pressEvent), data.accept);

        PointerMoveEvent moveEvent1{{}, {}};
        CORRADE_COMPARE(ui.pointerMoveEvent({350.0f, 1500.0f}, moveEvent1), data.accept);

        PointerEvent releaseEvent{Pointer::MouseMiddle};
        CORRADE_COMPARE(ui.pointerReleaseEvent({250.0f, 500.0f}, releaseEvent), data.accept);

        PointerMoveEvent moveEvent2{{}, {}};
        CORRADE_COMPARE(ui.pointerMoveEvent({300.0f, 1000.0f}, moveEvent2), data.accept);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Pair<Int, Vector4>>({
            {Press, {10.0f, 10.0f, 0.0f, 0.0f}},
            {Move, {15.0f, 15.0f, 5.0f, 5.0f}},
            /* Ideally a move event would be called with the position closer
               to when the release happens, to not lose that much of the
               relative position */
            {Release, {5.0f, 5.0f, 0.0f, 0.0f}},
            {Move, {10.0f, 10.0f, 5.0f, 5.0f}},
        })), TestSuite::Compare::Container);

    /* Move on a node, press outside */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerMoveEvent moveEvent{{}, {}};
        CORRADE_COMPARE(ui.pointerMoveEvent({300.0f, 1000.0f}, moveEvent), data.accept);

        PointerEvent pressEvent{Pointer::MouseMiddle};
        CORRADE_VERIFY(!ui.pointerPressEvent({100.0f, 1000.0f}, pressEvent));

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Pair<Int, Vector4>>({
            {Move, {10.0f, 10.0f, 0.0f, 0.0f}},
            /* There's nothing to receive a Press event afterwards. The Press
               event also doesn't synthesize a Leave event for the original
               node at the moment. */
        })), TestSuite::Compare::Container);
    }
}

void AbstractUserInterfaceTest::eventPointerMoveNotAccepted() {
    /* Event scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};

    enum Event {
        Accepted = 1,
        Move = 2,
        Enter = 4,
        Leave = 6
    };
    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doPointerMoveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            if(dataId == 0 || (dataId == 2 && accept2) || (dataId == 1 && accept1))
                event.setAccepted();
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Move|(event.isAccepted() ? Accepted : 0), dataHandle(handle(), dataId, 1), event.position());
        }
        void doPointerEnterEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Enter, dataHandle(handle(), dataId, 1), event.position());
        }
        void doPointerLeaveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Leave, dataHandle(handle(), dataId, 1), event.position());
        }

        Containers::Array<Containers::Triple<Int, DataHandle, Vector2>> eventCalls;
        bool accept1 = true,
            accept2 = true;
    };

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<Layer>(layer));

    /* 10 --------     accepts maybe
       01     -------- accepts maybe
       00     -------- accepts always */
    NodeHandle node0 = ui.createNode({20.0f, 0.0f}, {20.0f, 20.0f});
    NodeHandle node1 = ui.createNode({10.0f, 0.0f}, {20.0f, 20.0f});
    DataHandle data00 = ui.layer<Layer>(layer).create();
    DataHandle data01 = ui.layer<Layer>(layer).create();
    DataHandle data10 = ui.layer<Layer>(layer).create();
    ui.attachData(node0, data00);
    ui.attachData(node0, data01);
    ui.attachData(node1, data10);

    /* Move on node 1, but the second move doesn't get accepted and falls back
       to node 0, generating Leave and Enter events as appropriate */
    {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerMoveEvent eventMove1{{}, {}};
        ui.layer<Layer>(layer).accept2 = true;
        CORRADE_VERIFY(ui.pointerMoveEvent({15.0f, 10.0f}, eventMove1));
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), node1);

        PointerMoveEvent eventMove2{{}, {}};
        ui.layer<Layer>(layer).accept2 = false;
        CORRADE_VERIFY(ui.pointerMoveEvent({25.0f, 15.0f}, eventMove2));
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), node0);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Move|Accepted, data10, {5.0f, 10.0f}},
            {Enter, data10, {5.0f, 10.0f}},
            {Move, data10, {15.0f, 15.0f}}, /* not accepted */
            {Move|Accepted, data01, {5.0f, 15.0f}},
            {Leave, data10, {15.0f, 15.0f}},
            {Enter, data01, {5.0f, 15.0f}}
        })), TestSuite::Compare::Container);

    /* Move from node 0 to node 1, but the second move doesn't get accepted and
       falls back to node 0, not generating any Enter/Leave event */
    } {
        /* Just to reset everything */
        /** @todo have a pointerCancelEvent() for this */
        PointerMoveEvent eventMoveReset{{}, {}};
        ui.pointerMoveEvent({1000.0f, 1000.0f}, eventMoveReset);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        ui.layer<Layer>(layer).eventCalls = {};

        PointerMoveEvent eventMove1{{}, {}};
        ui.layer<Layer>(layer).accept1 = true;
        CORRADE_VERIFY(ui.pointerMoveEvent({35.0f, 10.0f}, eventMove1));
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), node0);

        PointerMoveEvent eventMove2{{}, {}};
        ui.layer<Layer>(layer).accept2 = false;
        CORRADE_VERIFY(ui.pointerMoveEvent({25.0f, 15.0f}, eventMove2));
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), node0);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Move|Accepted, data01, {15.0f, 10.0f}},
            {Enter, data01, {15.0f, 10.0f}},
            {Move, data10, {15.0f, 15.0f}}, /* not accepted */
            {Move|Accepted, data01, {5.0f, 15.0f}},
            /* No Enter/Leave as we ended up staying on the same node */
        })), TestSuite::Compare::Container);

    /* Move on node 0, but the second move on data 01 doesn't get accepted and
       falls back to data00, generating Leave and Enter events as appropriate
       even though on the same node */
    } {
        /* Just to reset everything */
        /** @todo have a pointerCancelEvent() for this */
        PointerMoveEvent eventMoveReset{{}, {}};
        ui.pointerMoveEvent({1000.0f, 1000.0f}, eventMoveReset);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        ui.layer<Layer>(layer).eventCalls = {};

        PointerMoveEvent eventMove1{{}, {}};
        ui.layer<Layer>(layer).accept1 = true;
        CORRADE_VERIFY(ui.pointerMoveEvent({35.0f, 10.0f}, eventMove1));
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), node0);

        PointerMoveEvent eventMove2{{}, {}};
        ui.layer<Layer>(layer).accept1 = false;
        CORRADE_VERIFY(ui.pointerMoveEvent({33.0f, 15.0f}, eventMove2));
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), node0);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Move|Accepted, data01, {15.0f, 10.0f}},
            {Enter, data01, {15.0f, 10.0f}},
            {Move, data01, {13.0f, 15.0f}}, /* not accepted */
            {Move|Accepted, data00, {13.0f, 15.0f}},
            {Leave, data01, {13.0f, 15.0f}},
            {Enter, data00, {13.0f, 15.0f}}
        })), TestSuite::Compare::Container);
    }
}

void AbstractUserInterfaceTest::eventPointerMoveNodePositionUpdated() {
    auto&& data = UpdateData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* framebufferSize isn't used for anything here; events should get scaled
       to (0.1, 0.01) */
    AbstractUserInterface ui{{300.0f, 200.0f}, {3000.0f, 20000.0f}, {30, 20}};

    enum Event {
        Move = 0,
        Enter = 1,
        Leave = 2
    };
    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doPointerMoveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Move, dataHandle(handle(), dataId, 1), event.position());
            event.setAccepted();
        }
        void doPointerEnterEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Enter, dataHandle(handle(), dataId, 1), event.position());
        }
        void doPointerLeaveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Leave, dataHandle(handle(), dataId, 1), event.position());
        }

        Containers::Array<Containers::Triple<Int, DataHandle, Vector2>> eventCalls;
    };

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<Layer>(layer));

    /* Nested node in order to verify that the hidden flag gets propagated
       through the hierarchy */
    NodeHandle node = ui.createNode({20.0f, 0.0f}, {20.0f, 20.0f});
    NodeHandle nested = ui.createNode(node, {}, {20.0f, 20.0f});
    DataHandle nestedData = ui.layer<Layer>(layer).create();
    ui.attachData(nested, nestedData);

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    PointerMoveEvent eventMove1{{}, {}};
    CORRADE_VERIFY(ui.pointerMoveEvent({300.0f, 1000.0f}, eventMove1));
    CORRADE_COMPARE(ui.pointerEventHoveredNode(), nested);

    ui.setNodeOffset(node, {30.0f, 20.0f});
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeLayoutUpdate);

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    PointerMoveEvent eventMove2{{}, {}};
    CORRADE_VERIFY(ui.pointerMoveEvent({350.0f, 2500.0f}, eventMove2));
    CORRADE_COMPARE(ui.pointerEventHoveredNode(), nested);

    CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
        {Move, nestedData, {10.0f, 10.0f}},
        {Enter, nestedData, {10.0f, 10.0f}},
        /** @todo what if no mouse event happens here? then it gets nothing?
            synthesize another move event in case stuff gets moved around,
            potentially also Enter and Leave, to prevent those usual UI bugs
            when stuff changes while mouse doesn't move? */
        /* Should receive up-to-date position, not something relative to a
           position cached at the last move; also properly considering the
           event scale */
        {Move, nestedData, {15.0f - 10.0f, 25.0f - 20.0f}},
    })), TestSuite::Compare::Container);

    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
}

void AbstractUserInterfaceTest::eventPointerMoveNodeBecomesHidden() {
    auto&& data = EventPointerNodeBecomesHiddenData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Event scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};

    enum Event {
        Move = 0,
        Enter = 1,
        Leave = 2
    };
    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doPointerMoveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Move, dataHandle(handle(), dataId, 1), event.position());
            event.setAccepted();
        }
        void doPointerEnterEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Enter, dataHandle(handle(), dataId, 1), event.position());
        }
        void doPointerLeaveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Leave, dataHandle(handle(), dataId, 1), event.position());
        }

        Containers::Array<Containers::Triple<Int, DataHandle, Vector2>> eventCalls;
    };

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<Layer>(layer));

    /* Nested node in order to verify that the hidden flag gets propagated
       through the hierarchy */
    NodeHandle node = ui.createNode({20.0f, 0.0f}, {20.0f, 20.0f});
    NodeHandle nested = ui.createNode(node, {}, {20.0f, 20.0f});
    DataHandle nestedData = ui.layer<Layer>(layer).create();
    ui.attachData(nested, nestedData);

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    PointerMoveEvent eventMove1{{}, {}};
    CORRADE_VERIFY(ui.pointerMoveEvent({30.0f, 10.0f}, eventMove1));
    CORRADE_COMPARE(ui.pointerEventHoveredNode(), nested);

    if(data.flags)
        ui.addNodeFlags(node, data.flags);
    else if(data.clearOrder)
        ui.clearNodeOrder(node);
    else CORRADE_INTERNAL_ASSERT_UNREACHABLE();
    /* The current hovered node stays after setting the flags, is only updated
       after update() -- there it also handles if any parent gets the flag as
       well */
    CORRADE_COMPARE(ui.pointerEventHoveredNode(), nested);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);

    if(data.update) {
        ui.update();

        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    PointerMoveEvent eventMove2{{}, {}};
    /* There's no node to execute the move on */
    CORRADE_VERIFY(!ui.pointerMoveEvent({35.0f, 15.0f}, eventMove2));
    CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

    CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
        {Move, nestedData, {10.0f, 10.0f}},
        {Enter, nestedData, {10.0f, 10.0f}},
        /* There's no node to execute the Move on, neither a Leave can be
           emitted as the node isn't part of the visible hierarchy and thus its
           absolute offset is unknown */
    })), TestSuite::Compare::Container);

    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
}

void AbstractUserInterfaceTest::eventPointerMoveNodeRemoved() {
    auto&& data = EventNodeRemovedData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Event scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};

    enum Event {
        Move = 0,
        Enter = 1,
        Leave = 2
    };
    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doPointerMoveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Move, dataHandle(handle(), dataId, 1), event.position());
            event.setAccepted();
        }
        void doPointerEnterEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Enter, dataHandle(handle(), dataId, 1), event.position());
        }
        void doPointerLeaveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Leave, dataHandle(handle(), dataId, 1), event.position());
        }

        Containers::Array<Containers::Triple<Int, DataHandle, Vector2>> eventCalls;
    };

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<Layer>(layer));

    /* Nested node in order to verify that the removal gets propagated through
       the hierarchy */
    NodeHandle node = ui.createNode({20.0f, 0.0f}, {20.0f, 20.0f});
    NodeHandle nested = ui.createNode(node, {}, {20.0f, 20.0f});
    DataHandle nestedData = ui.layer<Layer>(layer).create();
    ui.attachData(nested, nestedData);

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    PointerMoveEvent eventMove1{{}, {}};
    CORRADE_VERIFY(ui.pointerMoveEvent({30.0f, 10.0f}, eventMove1));
    CORRADE_COMPARE(ui.pointerEventHoveredNode(), nested);

    ui.removeNode(data.removeParent ? node : nested);
    /* The current hovered node stays after removal, is only updated after
       update() -- there it also handles if any parent is removed */
    CORRADE_COMPARE(ui.pointerEventHoveredNode(), nested);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeClean);

    /* Add a new node in a different place, to verify the generation is
       correctly checked as well */
    if(!data.removeParent) {
        NodeHandle nestedReplacement = ui.createNode(node, {}, {40.0f, 20.0f});
        CORRADE_COMPARE(nodeHandleId(nestedReplacement), nodeHandleId(nested));
    }

    if(data.update) {
        ui.update();

        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    PointerMoveEvent eventMove2{{}, {}};
    CORRADE_VERIFY(!ui.pointerMoveEvent({35.0f, 15.0f}, eventMove2));
    CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

    CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
        {Move, nestedData, {10.0f, 10.0f}},
        {Enter, nestedData, {10.0f, 10.0f}},
        /* There's no node to execute the Move on, nor a Leave */
    })), TestSuite::Compare::Container);

    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
}

void AbstractUserInterfaceTest::eventPointerMoveDataRemoved() {
    auto&& data = CleanUpdateData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Event scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};

    enum Event {
        Move = 0,
        Enter = 1,
        Leave = 2
    };
    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doPointerMoveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Move, dataHandle(handle(), dataId, 1), event.position());
            event.setAccepted();
        }
        void doPointerEnterEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Enter, dataHandle(handle(), dataId, 1), event.position());
        }
        void doPointerLeaveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Leave, dataHandle(handle(), dataId, 1), event.position());
        }

        Containers::Array<Containers::Triple<Int, DataHandle, Vector2>> eventCalls;
    };

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<Layer>(layer));

    NodeHandle node = ui.createNode({20.0f, 0.0f}, {20.0f, 20.0f});
    DataHandle nodeData = ui.layer<Layer>(layer).create();
    ui.attachData(node, nodeData);

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    PointerMoveEvent eventMove1{{}, {}};
    CORRADE_VERIFY(ui.pointerMoveEvent({30.0f, 10.0f}, eventMove1));
    CORRADE_COMPARE(ui.pointerEventHoveredNode(), node);

    ui.layer<Layer>(layer).remove(nodeData);
    CORRADE_COMPARE(ui.pointerEventHoveredNode(), node);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataClean);

    if(data.clean) {
        ui.clean();

        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate);
    }

    if(data.update) {
        ui.update();

        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    PointerMoveEvent eventMove2{{}, {}};
    /* There's no data to execute the move on */
    CORRADE_VERIFY(!ui.pointerMoveEvent({35.0f, 15.0f}, eventMove2));
    CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

    CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
        {Move, nodeData, {10.0f, 10.0f}},
        {Enter, nodeData, {10.0f, 10.0f}},
        /* There's no data to execute the Move on, nor a Leave */
    })), TestSuite::Compare::Container);

    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
}

void AbstractUserInterfaceTest::eventCapture() {
    /* framebufferSize isn't used for anything here; events should get scaled
       to (0.1, 0.01) */
    AbstractUserInterface ui{{300.0f, 200.0f}, {3000.0f, 20000.0f}, {30, 20}};

    enum Event {
        Captured = 1,
        Press = 2,
        Release = 4,
        Move = 6,
        Enter = 8,
        Leave = 10
    };
    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doPointerPressEvent(UnsignedInt dataId, PointerEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Press|(event.isCaptured() ? Captured : 0), dataHandle(handle(), dataId, 1), event.position());
            event.setAccepted();
        }
        void doPointerReleaseEvent(UnsignedInt dataId, PointerEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Release|(event.isCaptured() ? Captured : 0), dataHandle(handle(), dataId, 1), event.position());
            event.setAccepted();
        }
        void doPointerMoveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Move|(event.isCaptured() ? Captured : 0), dataHandle(handle(), dataId, 1), event.position());
            event.setAccepted();
        }
        void doPointerEnterEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Enter|(event.isCaptured() ? Captured : 0), dataHandle(handle(), dataId, 1), event.position());
        }
        void doPointerLeaveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Leave|(event.isCaptured() ? Captured : 0), dataHandle(handle(), dataId, 1), event.position());
        }

        Containers::Array<Containers::Triple<Int, DataHandle, Vector2>> eventCalls;
    };

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<Layer>(layer));

    /* Two nodes next to each other */
    NodeHandle left = ui.createNode({20.0f, 0.0f}, {20.0f, 20.0f});
    NodeHandle right = ui.createNode({40.0f, 0.0f}, {20.0f, 20.0f});
    DataHandle leftData = ui.layer<Layer>(layer).create();
    DataHandle rightData = ui.layer<Layer>(layer).create();
    ui.attachData(left, leftData);
    ui.attachData(right, rightData);

    /* Nothing captured initially */
    CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);

    /* Capture on the left node, release on it again */
    {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent eventPress{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({300.0f, 1000.0f}, eventPress));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), left);
        /* No Enter/Leave events synthesized from Press at the moment, so no
           hovered node either */
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        PointerEvent eventRelease{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({320.0f, 1000.0f}, eventRelease));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        /* No Enter/Leave events synthesized from Release at the moment, so no
           hovered node either */
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, leftData, {10.0f, 10.0f}},
            /* No Enter/Leave events synthesized from Press and Release at the
               moment */
            {Release|Captured, leftData, {12.0f, 10.0f}},
        })), TestSuite::Compare::Container);

    /* Capture on the left node, release on the right one */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent eventPress{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({300.0f, 1000.0f}, eventPress));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), left);
        /* No Enter/Leave events synthesized from Press at the moment, so no
           hovered node either */
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        PointerEvent eventRelease{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({500.0f, 1000.0f}, eventRelease));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        /* No Enter/Leave events synthesized from Release at the moment, so no
           hovered node either */
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, leftData, {10.0f, 10.0f}},
            /* No Enter/Leave events synthesized from Press and Release at the
               moment */
            {Release|Captured, leftData, {30.0f, 10.0f}}, /* actually on rightData */
        })), TestSuite::Compare::Container);

    /* Capture on the right node, release on the left one */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent eventPress{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({500.0f, 1000.0f}, eventPress));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), right);
        /* No Enter/Leave events synthesized from Press at the moment, so no
           hovered node either */
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        PointerEvent eventRelease{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({300.0f, 1000.0f}, eventRelease));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        /* No Enter/Leave events synthesized from Release at the moment, so no
           hovered node either */
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, rightData, {10.0f, 10.0f}},
            /* No Enter/Leave events synthesized from Press and Release at the
               moment */
            {Release|Captured, rightData, {-10.0f, 10.0f}}, /* actually on leftData */
        })), TestSuite::Compare::Container);

    /* Moves are implicitly captured only if they happen between a press &
       release */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerMoveEvent eventMove1{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({500.0f, 1500.0f}, eventMove1));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), right);

        PointerEvent eventPress{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({300.0f, 1000.0f}, eventPress));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), left);
        /* No Enter/Leave events synthesized from Press at the moment, so the
           hovered node doesn't get updated until the next move */
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), right);

        PointerMoveEvent eventMove2{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({350.0f, 1500.0f}, eventMove2));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), left);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), left);

        PointerEvent eventRelease{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({500.0f, 1000.0f}, eventRelease));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        /* No Enter/Leave events synthesized from Release at the moment, so the
           hovered node doesn't get updated until the next move */
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), left);

        PointerMoveEvent eventMove3{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({550.0f, 1500.0f}, eventMove3));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), right);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            /* A move that happens before a press isn't captured */
            {Move, rightData, {10.0f, 15.0f}},
            /* Neither is the Enter synthesized from it */
            {Enter, rightData, {10.0f, 15.0f}},
            {Press|Captured, leftData, {10.0f, 10.0f}},
            /* No Enter/Leave event synthesized from Press at the moment. If
               they would, neither would be captured. */
            /* A move that happens during a press is captured. Since no
               Enter/Leave events were synthesized from the Press, they get emitted here. The Leave event isn't captured, the Enter is, in
               order to allow a capture reset in its handler. */
            {Move|Captured, leftData, {15.0f, 15.0f}},
            {Leave, rightData, {-5.0f, 15.0f}},
            {Enter|Captured, leftData, {15.0f, 15.0f}},
            {Release|Captured, leftData, {30.0f, 10.0f}}, /* actually on rightData */
            /* Again, no Leave event for leftData synthesized from Release at
               the moment. If it would, it *would* be captured. */
            /* A move that happens after a release isn't captured again,
               together with a matching Enter/Leave as we're on a different
               node again. The Enter/Leave is not captured either as it didn't
               happen during a capture, but after it was released. */
            {Move, rightData, {15.0f, 15.0f}},
            {Leave, leftData, {35.0f, 15.0f}},
            {Enter, rightData, {15.0f, 15.0f}}
        })), TestSuite::Compare::Container);

    /* Captured moves synthesize a Leave when leaving and Enter when returning
       to the captured node area, but no corresponding Enter / Leave get
       synthesized for the other nodes that may be underneath */
    } {
        /* Just to reset everything */
        /** @todo have a pointerCancelEvent() for this */
        PointerEvent eventReleaseReset{Pointer::MouseLeft};
        ui.pointerReleaseEvent({10000.0f, 10000.0f}, eventReleaseReset);
        PointerMoveEvent eventMoveReset{{}, {}};
        ui.pointerMoveEvent({10000.0f, 10000.0f}, eventMoveReset);
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent eventPress{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({500.0f, 1000.0f}, eventPress));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), right);
        /* No Enter/Leave events synthesized from Press at the moment, so the
           hovered node doesn't get updated until the next move */
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        PointerMoveEvent eventMove1{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({550.0f, 1500.0f}, eventMove1));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), right);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), right);

        PointerMoveEvent eventMove2{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({350.0f, 1000.0f}, eventMove2));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), right);
        /* We're on the `left` node, but since the pointer is captured on the
           `right` node, there's no hover */
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        PointerMoveEvent eventMove3{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({450.0f, 500.0f}, eventMove3));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), right);
        /* Moving back on the `right` node makes it hovered again */
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), right);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, rightData, {10.0f, 10.0f}},
            /* A captured move on the same node */
            {Move|Captured, rightData, {15.0f, 15.0f}},
            /* Which synthesizes an Enter event, as before this node wasn't
               hovered. It's captured in order to allow a capture reset in its
               handler. */
            {Enter|Captured, rightData, {15.0f, 15.0f}},
            /* A captured move outside of the node */
            {Move|Captured, rightData, {-5.0f, 10.0f}},
            /* Which synthesizes a (captured) Leave event, but no matching
               Enter event for any other node underneath */
            {Leave|Captured, rightData, {-5.0f, 10.0f}},
            /* A capture move back again */
            {Move|Captured, rightData, {5.0f, 5.0f}},
            /* Which synthesizes a (captured) Enter event again, but no
               matcing Leave for any other node underneath */
            {Enter|Captured, rightData, {5.0f, 5.0f}},
        })), TestSuite::Compare::Container);

    /* Capture on the right node, then capture again on the left one. In
       practice this can only happen if a release event is missed for some
       reason. */
    } {
        /* Just to reset everything */
        /** @todo have a pointerCancelEvent() for this */
        PointerEvent eventReleaseReset{Pointer::MouseLeft};
        ui.pointerReleaseEvent({10000.0f, 10000.0f}, eventReleaseReset);
        PointerMoveEvent eventMoveReset{{}, {}};
        ui.pointerMoveEvent({10000.0f, 10000.0f}, eventMoveReset);
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent eventPress1{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({500.0f, 1000.0f}, eventPress1));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), right);
        /* No Enter/Leave events synthesized from Press at the moment, so the
           hovered node doesn't get updated until the next move */
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        PointerEvent eventPress2{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({300.0f, 1000.0f}, eventPress2));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), left);
        /* No Enter/Leave events synthesized from Press at the moment, so the
           hovered node doesn't get updated until the next move */
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, rightData, {10.0f, 10.0f}},
            /* No Enter/Leave events synthesized from Press and Release at the
               moment */
            {Press|Captured, leftData, {10.0f, 10.0f}},
        })), TestSuite::Compare::Container);
    }
}

void AbstractUserInterfaceTest::eventCaptureEdges() {
    /* framebufferSize isn't used for anything here; events should get scaled
       to (0.1, 0.01) */
    AbstractUserInterface ui{{300.0f, 200.0f}, {3000.0f, 20000.0f}, {30, 20}};

    /* Like eventEdges(), but testing the special case with event capture where
       it's used to fire Enter and Leave events */

    enum Event {
        Press = 0,
        Move = 1,
        Enter = 2,
        Leave = 3
    };
    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doPointerPressEvent(UnsignedInt dataId, PointerEvent& event) override {
            CORRADE_COMPARE(dataId, 1);
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Press, event.position());
            event.setAccepted();
        }
        void doPointerMoveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            CORRADE_COMPARE(dataId, 1);
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Move, event.position());
            event.setAccepted();
        }
        void doPointerEnterEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            CORRADE_COMPARE(dataId, 1);
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Enter, event.position());
        }
        void doPointerLeaveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            CORRADE_COMPARE(dataId, 1);
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Leave, event.position());
        }

        Containers::Array<Containers::Pair<Int, Vector2>> eventCalls;
    };

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<Layer>(layer));

    NodeHandle node = ui.createNode({20.0f, 10.0f}, {20.0f, 20.0f});
    /*DataHandle nodeData1 =*/ ui.layer<Layer>(layer).create();
    DataHandle nodeData2 = ui.layer<Layer>(layer).create();
    ui.attachData(node, nodeData2);

    /* Set the node as initially hovered */
    PointerMoveEvent eventMove0{{}, {}};
    CORRADE_VERIFY(ui.pointerMoveEvent({300.0f, 2500.0f}, eventMove0));
    CORRADE_COMPARE(ui.pointerEventHoveredNode(), node);

    /* Top left corner */
    {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent eventPress{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({300.0f, 2500.0f}, eventPress));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), node);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), node);

        PointerMoveEvent eventMove1{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({199.0f, 990.0f}, eventMove1));
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        PointerMoveEvent eventMove2{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({200.0f, 1000.0f}, eventMove2));
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), node);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Pair<Int, Vector2>>({
            {Press, {10.0f, 15.0f}},
            {Move, {-0.1f, -0.1f}},
            {Leave, {-0.1f, -0.1f}},
            {Move, {0.0f, 0.0f}},
            {Enter, {0.0f, 0.0f}}
        })), TestSuite::Compare::Container);

    /* Top edge */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent eventPress{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({300.0f, 2500.0f}, eventPress));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), node);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), node);

        PointerMoveEvent eventMove1{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({300.0f, 990.0f}, eventMove1));
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        PointerMoveEvent eventMove2{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({300.0f, 1000.0f}, eventMove2));
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), node);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Pair<Int, Vector2>>({
            {Press, {10.0f, 15.0f}},
            {Move, {10.0f, -0.1f}},
            {Leave, {10.0f, -0.1f}},
            {Move, {10.0f, 0.0f}},
            {Enter, {10.0f, 0.0f}}
        })), TestSuite::Compare::Container);

    /* Left edge */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent eventPress{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({300.0f, 2500.0f}, eventPress));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), node);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), node);

        PointerMoveEvent eventMove1{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({199.0f, 2500.0f}, eventMove1));
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        PointerMoveEvent eventMove2{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({200.0f, 2500.0f}, eventMove2));
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), node);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Pair<Int, Vector2>>({
            {Press, {10.0f, 15.0f}},
            {Move, {-0.1f, 15.0f}},
            {Leave, {-0.1f, 15.0f}},
            {Move, {0.0f, 15.0f}},
            {Enter, {0.0f, 15.0f}}
        })), TestSuite::Compare::Container);

    /* Bottom right corner */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent eventPress{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({300.0f, 2500.0f}, eventPress));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), node);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), node);

        PointerMoveEvent eventMove1{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({400.0f, 3000.0f}, eventMove1));
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        PointerMoveEvent eventMove2{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({399.0f, 2990.0f}, eventMove2));
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), node);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Pair<Int, Vector2>>({
            {Press, {10.0f, 15.0f}},
            {Move, {20.0f, 20.0f}},
            {Leave, {20.0f, 20.0f}},
            {Move, {19.9f, 19.9f}},
            {Enter, {19.9f, 19.9f}}
        })), TestSuite::Compare::Container);

    /* Bottom edge */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent eventPress{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({300.0f, 2500.0f}, eventPress));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), node);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), node);

        PointerMoveEvent eventMove1{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({300.0f, 3000.0f}, eventMove1));
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        PointerMoveEvent eventMove2{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({300.0f, 2990.0f}, eventMove2));
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), node);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Pair<Int, Vector2>>({
            {Press, {10.0f, 15.0f}},
            {Move, {10.0f, 20.0f}},
            {Leave, {10.0f, 20.0f}},
            {Move, {10.0f, 19.9f}},
            {Enter, {10.0f, 19.9f}}
        })), TestSuite::Compare::Container);

    /* Right edge */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent eventPress{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({300.0f, 2500.0f}, eventPress));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), node);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), node);

        PointerMoveEvent eventMove1{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({400.0f, 2500.0f}, eventMove1));
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        PointerMoveEvent eventMove2{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({399.0f, 2500.0f}, eventMove2));
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), node);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Pair<Int, Vector2>>({
            {Press, {10.0f, 15.0f}},
            {Move, {20.0f, 15.0f}},
            {Leave, {20.0f, 15.0f}},
            {Move, {19.9f, 15.0f}},
            {Enter, {19.9f, 15.0f}}
        })), TestSuite::Compare::Container);
    }
}

void AbstractUserInterfaceTest::eventCaptureNotAccepted() {
    /* Event scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};

    enum Event {
        Captured = 1,
        Press = 2,
        Release = 4,
        Move = 6,
        Enter = 8,
        Leave = 10
    };
    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doPointerPressEvent(UnsignedInt dataId, PointerEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Press|(event.isCaptured() ? Captured : 0), dataHandle(handle(), dataId, 1), event.position());
            if((accept1 && dataId <= 1) || (accept2 && dataId == 2))
                event.setAccepted();
        }
        void doPointerReleaseEvent(UnsignedInt dataId, PointerEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Release|(event.isCaptured() ? Captured : 0), dataHandle(handle(), dataId, 1), event.position());
            if((accept1 && dataId <= 1) || (accept2 && dataId == 2))
                event.setAccepted();
        }
        void doPointerMoveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Move|(event.isCaptured() ? Captured : 0), dataHandle(handle(), dataId, 1), event.position());
            if((accept1 && dataId <= 1) || (accept2 && dataId == 2))
                event.setAccepted();
        }
        void doPointerEnterEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Enter|(event.isCaptured() ? Captured : 0), dataHandle(handle(), dataId, 1), event.position());
        }
        void doPointerLeaveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Leave|(event.isCaptured() ? Captured : 0), dataHandle(handle(), dataId, 1), event.position());
        }

        bool accept1 = true,
            accept2 = true;
        Containers::Array<Containers::Triple<Int, DataHandle, Vector2>> eventCalls;
    };

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<Layer>(layer));

    /* Two nodes next to each other */
    NodeHandle left = ui.createNode({20.0f, 0.0f}, {20.0f, 20.0f});
    NodeHandle right = ui.createNode({40.0f, 0.0f}, {20.0f, 20.0f});
    DataHandle leftData1 = ui.layer<Layer>(layer).create();
    DataHandle rightData1 = ui.layer<Layer>(layer).create();
    DataHandle rightData2 = ui.layer<Layer>(layer).create();
    ui.attachData(left, leftData1);
    ui.attachData(right, rightData2);
    ui.attachData(right, rightData1);

    /* If the press event isn't accepted, no capture should happen, so the
       release happens on the actual node that is under */
    {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent eventPress{Pointer::MouseLeft};
        ui.layer<Layer>(layer).accept1 = false;
        ui.layer<Layer>(layer).accept2 = false;
        CORRADE_VERIFY(!ui.pointerPressEvent({30.0f, 10.0f}, eventPress));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        PointerEvent eventRelease{Pointer::MouseLeft};
        ui.layer<Layer>(layer).accept1 = true;
        ui.layer<Layer>(layer).accept2 = false;
        CORRADE_VERIFY(ui.pointerReleaseEvent({50.0f, 10.0f}, eventRelease));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        /* No Enter/Leave events synthesized from Release at the moment, so the
           hovered node doesn't get updated until the next move */
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, leftData1, {10.0f, 10.0f}},
            /* The release event isn't happening on a captured node, so
               isCaptured() is false for it */
            {Release, rightData1, {10.0f, 10.0f}},
        })), TestSuite::Compare::Container);

    /* Same, but move instead of release */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent eventPress{Pointer::MouseLeft};
        ui.layer<Layer>(layer).accept1 = false;
        ui.layer<Layer>(layer).accept2 = false;
        CORRADE_VERIFY(!ui.pointerPressEvent({30.0f, 10.0f}, eventPress));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        PointerMoveEvent eventMove{{}, {}};
        ui.layer<Layer>(layer).accept1 = true;
        ui.layer<Layer>(layer).accept2 = false;
        CORRADE_VERIFY(ui.pointerMoveEvent({50.0f, 10.0f}, eventMove));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), right);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, leftData1, {10.0f, 10.0f}},
            /* The move event isn't happening on a captured node, so
               isCaptured() is false for it */
            {Move, rightData1, {10.0f, 10.0f}},
            {Enter, rightData1, {10.0f, 10.0f}}
        })), TestSuite::Compare::Container);

    /* If the release event isn't accepted, the capture should still get
       reset nevertheless */
    } {
        /* Just to reset everything */
        /** @todo have a pointerCancelEvent() for this */
        PointerEvent eventReleaseReset{Pointer::MouseLeft};
        ui.pointerReleaseEvent({1000.0f, 1000.0f}, eventReleaseReset);
        PointerMoveEvent eventMoveReset{{}, {}};
        ui.pointerMoveEvent({1000.0f, 1000.0f}, eventMoveReset);
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent eventPress{Pointer::MouseLeft};
        ui.layer<Layer>(layer).accept1 = true;
        ui.layer<Layer>(layer).accept2 = false;
        CORRADE_VERIFY(ui.pointerPressEvent({30.0f, 10.0f}, eventPress));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), left);
        /* No Enter/Leave events synthesized from Press at the moment, so the
           hovered node doesn't get updated until the next move */
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        PointerEvent eventRelease{Pointer::MouseLeft};
        ui.layer<Layer>(layer).accept1 = false;
        ui.layer<Layer>(layer).accept2 = false;
        CORRADE_VERIFY(!ui.pointerReleaseEvent({50.0f, 10.0f}, eventRelease));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, leftData1, {10.0f, 10.0f}},
            {Release|Captured, leftData1, {30.0f, 10.0f}}, /* actually on rightData */
        })), TestSuite::Compare::Container);

    /* With move however, it should stay, even if it isn't accepted */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent eventPress{Pointer::MouseLeft};
        ui.layer<Layer>(layer).accept1 = true;
        ui.layer<Layer>(layer).accept2 = false;
        CORRADE_VERIFY(ui.pointerPressEvent({30.0f, 10.0f}, eventPress));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), left);
        /* No Enter/Leave events synthesized from Press at the moment, so the
           hovered node doesn't get updated until the next move */
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        PointerMoveEvent eventMove{{}, {}};
        ui.layer<Layer>(layer).accept1 = false;
        ui.layer<Layer>(layer).accept2 = false;
        CORRADE_VERIFY(!ui.pointerMoveEvent({50.0f, 10.0f}, eventMove));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), left);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, leftData1, {10.0f, 10.0f}},
            {Move|Captured, leftData1, {30.0f, 10.0f}}, /* actually on rightData */
            /* No node was hovered before, so no Leave is emitted */
        })), TestSuite::Compare::Container);

    /* Moving on the same node but not accepting the move causes Enter / Leave
       to be generated */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent eventPress{Pointer::MouseLeft};
        ui.layer<Layer>(layer).accept1 = true;
        ui.layer<Layer>(layer).accept2 = false;
        CORRADE_VERIFY(ui.pointerPressEvent({30.0f, 10.0f}, eventPress));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), left);
        /* No Enter/Leave events synthesized from Press at the moment, so the
           hovered node doesn't get updated until the next move */
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        PointerMoveEvent eventMove1{{}, {}};
        ui.layer<Layer>(layer).accept1 = false;
        ui.layer<Layer>(layer).accept2 = false;
        CORRADE_VERIFY(!ui.pointerMoveEvent({35.0f, 15.0f}, eventMove1));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), left);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        PointerMoveEvent eventMove2{{}, {}};
        ui.layer<Layer>(layer).accept1 = true;
        ui.layer<Layer>(layer).accept2 = false;
        CORRADE_VERIFY(ui.pointerMoveEvent({30.0f, 10.0f}, eventMove2));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), left);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), left);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, leftData1, {10.0f, 10.0f}},
            {Move|Captured, leftData1, {15.0f, 15.0f}},
            /* No node was hovered before, so no Leave is emitted */
            {Move|Captured, leftData1, {10.0f, 10.0f}},
            {Enter|Captured, leftData1, {10.0f, 10.0f}},
        })), TestSuite::Compare::Container);

    /* Moving on the same node but capturing on a different data should cause
       Enter / Leave to be generated as well */
    } {
        /* Just to reset everything */
        /** @todo have a pointerCancelEvent() for this */
        PointerEvent eventReleaseReset{Pointer::MouseLeft};
        ui.pointerReleaseEvent({1000.0f, 1000.0f}, eventReleaseReset);
        PointerMoveEvent eventMoveReset{{}, {}};
        ui.pointerMoveEvent({1000.0f, 1000.0f}, eventMoveReset);
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        ui.layer<Layer>(layer).eventCalls = {};

        PointerMoveEvent eventMove1{{}, {}};
        ui.layer<Layer>(layer).accept1 = false;
        ui.layer<Layer>(layer).accept2 = true;
        CORRADE_VERIFY(ui.pointerMoveEvent({50.0f, 10.0f}, eventMove1));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), right);

        PointerEvent eventPress{Pointer::MouseLeft};
        ui.layer<Layer>(layer).accept1 = true;
        ui.layer<Layer>(layer).accept2 = false;
        CORRADE_VERIFY(ui.pointerPressEvent({50.0f, 10.0f}, eventPress));
        /* Capture is rightData1 */
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), right);
        /* Hover stays from the previous move, rightData2 */
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), right);

        PointerMoveEvent eventMove2{{}, {}};
        ui.layer<Layer>(layer).accept1 = true;
        ui.layer<Layer>(layer).accept2 = false;
        CORRADE_VERIFY(ui.pointerMoveEvent({55.0f, 15.0f}, eventMove2));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), right);
        /* The hovered node should now be rightData1 */
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), right);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Move, rightData1, {10.0f, 10.0f}}, /* not accepted */
            {Move, rightData2, {10.0f, 10.0f}},
            {Enter, rightData2, {10.0f, 10.0f}},
            {Press|Captured, rightData1, {10.0f, 10.0f}},
            {Move|Captured, rightData1, {15.0f, 15.0f}},
            {Leave, rightData2, {15.0f, 15.0f}},
            {Enter|Captured, rightData1, {15.0f, 15.0f}},
        })), TestSuite::Compare::Container);

    /* Capturing on a hovered node but with different data hovered should cause
       Leave to be generated for the original data, not the captured */
    } {
        /* Just to reset everything */
        /** @todo have a pointerCancelEvent() for this */
        PointerEvent eventReleaseReset{Pointer::MouseLeft};
        ui.pointerReleaseEvent({1000.0f, 1000.0f}, eventReleaseReset);
        PointerMoveEvent eventMoveReset{{}, {}};
        ui.pointerMoveEvent({1000.0f, 1000.0f}, eventMoveReset);
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        ui.layer<Layer>(layer).eventCalls = {};

        PointerMoveEvent eventMove1{{}, {}};
        ui.layer<Layer>(layer).accept1 = false;
        ui.layer<Layer>(layer).accept2 = true;
        CORRADE_VERIFY(ui.pointerMoveEvent({50.0f, 10.0f}, eventMove1));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), right);

        PointerEvent eventPress{Pointer::MouseLeft};
        ui.layer<Layer>(layer).accept1 = true;
        ui.layer<Layer>(layer).accept2 = false;
        CORRADE_VERIFY(ui.pointerPressEvent({50.0f, 10.0f}, eventPress));
        /* Capture is rightData1 */
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), right);
        /* Hover stays from the previous move, rightData2 */
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), right);

        PointerMoveEvent eventMove2{{}, {}};
        ui.layer<Layer>(layer).accept1 = true;
        ui.layer<Layer>(layer).accept2 = false;
        CORRADE_VERIFY(ui.pointerMoveEvent({100.0f, 100.0f}, eventMove2));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), right);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Move, rightData1, {10.0f, 10.0f}}, /* not accepted */
            {Move, rightData2, {10.0f, 10.0f}},
            {Enter, rightData2, {10.0f, 10.0f}},
            {Press|Captured, rightData1, {10.0f, 10.0f}},
            {Move|Captured, rightData1, {60.0f, 100.0f}},
            {Leave, rightData2, {60.0f, 100.0f}},
        })), TestSuite::Compare::Container);
    }
}

void AbstractUserInterfaceTest::eventCaptureNotCaptured() {
    /* Event scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};

    enum Event {
        Captured = 1,
        Press = 2,
        Release = 4,
        Move = 6,
        Enter = 8,
        Leave = 10
    };
    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doPointerPressEvent(UnsignedInt dataId, PointerEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Press|(event.isCaptured() ? Captured : 0), dataHandle(handle(), dataId, 1), event.position());
            if(capture)
                event.setCaptured(*capture);
            event.setAccepted();
        }
        void doPointerReleaseEvent(UnsignedInt dataId, PointerEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Release|(event.isCaptured() ? Captured : 0), dataHandle(handle(), dataId, 1), event.position());
            if(capture)
                event.setCaptured(*capture);
            event.setAccepted();
        }
        void doPointerMoveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Move|(event.isCaptured() ? Captured : 0), dataHandle(handle(), dataId, 1), event.position());
            if(capture)
                event.setCaptured(*capture);
            event.setAccepted();
        }
        void doPointerEnterEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Enter|(event.isCaptured() ? Captured : 0), dataHandle(handle(), dataId, 1), event.position());
            if(captureEnter)
                event.setCaptured(*captureEnter);
        }
        void doPointerLeaveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Leave|(event.isCaptured() ? Captured : 0), dataHandle(handle(), dataId, 1), event.position());
            if(captureLeave)
                event.setCaptured(*captureLeave);
        }

        Containers::Optional<bool> capture;
        Containers::Optional<bool> captureEnter;
        Containers::Optional<bool> captureLeave;
        Containers::Array<Containers::Triple<Int, DataHandle, Vector2>> eventCalls;
    };

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<Layer>(layer));

    /* Two nodes next to each other */
    NodeHandle left = ui.createNode({20.0f, 0.0f}, {20.0f, 20.0f});
    NodeHandle right = ui.createNode({40.0f, 0.0f}, {20.0f, 20.0f});
    DataHandle leftData = ui.layer<Layer>(layer).create();
    DataHandle rightData = ui.layer<Layer>(layer).create();
    ui.attachData(left, leftData);
    ui.attachData(right, rightData);

    /* If capture is disabled on press, the release happens on the actual node
       that is under */
    {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent eventPress{Pointer::MouseLeft};
        ui.layer<Layer>(layer).capture = false;
        ui.layer<Layer>(layer).captureEnter = {};
        ui.layer<Layer>(layer).captureLeave = {};
        CORRADE_VERIFY(ui.pointerPressEvent({30.0f, 10.0f}, eventPress));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        /* No Enter/Leave events synthesized from Press at the moment, so the
           hovered node doesn't get updated until the next move */
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        PointerEvent eventRelease{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({50.0f, 10.0f}, eventRelease));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        /* No Enter/Leave events synthesized from Release at the moment, so the
           hovered node doesn't get updated until the next move */
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, leftData, {10.0f, 10.0f}},
            /* The release event isn't happening on a captured node, so
               isCaptured() is false for it */
            {Release, rightData, {10.0f, 10.0f}},
        })), TestSuite::Compare::Container);

    /* Same for move */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent eventPress{Pointer::MouseLeft};
        ui.layer<Layer>(layer).capture = false;
        ui.layer<Layer>(layer).captureEnter = {};
        ui.layer<Layer>(layer).captureLeave = {};
        CORRADE_VERIFY(ui.pointerPressEvent({30.0f, 10.0f}, eventPress));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        /* No Enter/Leave events synthesized from Press at the moment, so the
           hovered node doesn't get updated until the next move */
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        PointerMoveEvent eventMove{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({50.0f, 10.0f}, eventMove));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), right);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, leftData, {10.0f, 10.0f}},
            /* The move event isn't happening on a captured node, so
               isCaptured() is false for it */
            {Move, rightData, {10.0f, 10.0f}},
            {Enter, rightData, {10.0f, 10.0f}}
        })), TestSuite::Compare::Container);

    /* If capture is disabled on release, it doesn't affect anything */
    } {
        /* Just to reset everything */
        /** @todo have a pointerCancelEvent() for this */
        PointerEvent eventReleaseReset{Pointer::MouseLeft};
        ui.pointerReleaseEvent({1000.0f, 1000.0f}, eventReleaseReset);
        PointerMoveEvent eventMoveReset{{}, {}};
        ui.pointerMoveEvent({1000.0f, 1000.0f}, eventMoveReset);
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent eventPress{Pointer::MouseLeft};
        ui.layer<Layer>(layer).capture = true;
        ui.layer<Layer>(layer).captureEnter = {};
        ui.layer<Layer>(layer).captureLeave = {};
        CORRADE_VERIFY(ui.pointerPressEvent({30.0f, 10.0f}, eventPress));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), left);
        /* No Enter/Leave events synthesized from Press at the moment, so the
           hovered node doesn't get updated until the next move */
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        PointerEvent eventRelease{Pointer::MouseLeft};
        ui.layer<Layer>(layer).capture = false;
        ui.layer<Layer>(layer).captureEnter = {};
        ui.layer<Layer>(layer).captureLeave = {};
        CORRADE_VERIFY(ui.pointerReleaseEvent({50.0f, 10.0f}, eventRelease));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        /* No Enter/Leave events synthesized from Release at the moment, so the
           hovered node doesn't get updated until the next move */
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, leftData, {10.0f, 10.0f}},
            {Release|Captured, leftData, {30.0f, 10.0f}}, /* actually on rightData */
        })), TestSuite::Compare::Container);

    /* For a move the capture can be disabled and re-enabled again. The next
       (move/release) event then happens either on the captured node or the
       actual node that's under. */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent eventPress{Pointer::MouseLeft};
        ui.layer<Layer>(layer).capture = true;
        ui.layer<Layer>(layer).captureEnter = {};
        ui.layer<Layer>(layer).captureLeave = {};
        CORRADE_VERIFY(ui.pointerPressEvent({30.0f, 10.0f}, eventPress));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), left);
        /* No Enter/Leave events synthesized from Press at the moment, so the
           hovered node doesn't get updated until the next move */
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        PointerMoveEvent eventMove1{{}, {}};
        ui.layer<Layer>(layer).capture = false;
        ui.layer<Layer>(layer).captureEnter = {};
        ui.layer<Layer>(layer).captureLeave = {};
        CORRADE_VERIFY(ui.pointerMoveEvent({50.0f, 10.0f}, eventMove1));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        /* The event removed the capture, however it's not looking for a
           now-hovered node as that would mean doing the whole bubbling
           again */
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        PointerMoveEvent eventMove2{{}, {}};
        ui.layer<Layer>(layer).capture = true;
        ui.layer<Layer>(layer).captureEnter = {};
        ui.layer<Layer>(layer).captureLeave = {};
        CORRADE_VERIFY(ui.pointerMoveEvent({55.0f, 15.0f}, eventMove2));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), right);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), right);

        PointerMoveEvent eventMove3{{}, {}};
        ui.layer<Layer>(layer).capture = false;
        ui.layer<Layer>(layer).captureEnter = {};
        ui.layer<Layer>(layer).captureLeave = {};
        CORRADE_VERIFY(ui.pointerMoveEvent({30.0f, 10.0f}, eventMove3));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        /* Again, as the event removed the capture there's no node to be
           hovered anymore */
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, leftData, {10.0f, 10.0f}},
            {Move|Captured, leftData, {30.0f, 10.0f}}, /* actually on rightData */
            /* No Enter/Leave as the hovered node stays null */
            {Move, rightData, {15.0f, 15.0f}},
            /* Enter is captured as the move captured it */
            {Enter|Captured, rightData, {15.0f, 15.0f}},
            {Move|Captured, rightData, {-10.0f, 10.0f}}, /* actually on leftData */
            /* Leave not captured anymore as the move released it */
            {Leave, rightData, {-10.0f, 10.0f}}
        })), TestSuite::Compare::Container);

    /* Capturing should also be possible on an uncaptured Enter event */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerMoveEvent eventMove1{{}, {}};
        ui.layer<Layer>(layer).capture = {};
        ui.layer<Layer>(layer).captureEnter = {};
        ui.layer<Layer>(layer).captureLeave = {};
        CORRADE_VERIFY(ui.pointerMoveEvent({50.0f, 10.0f}, eventMove1));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), right);

        PointerMoveEvent eventMove2{{}, {}};
        ui.layer<Layer>(layer).capture = {};
        ui.layer<Layer>(layer).captureEnter = true;
        ui.layer<Layer>(layer).captureLeave = {};
        CORRADE_VERIFY(ui.pointerMoveEvent({30.0f, 15.0f}, eventMove2));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), left);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), left);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Move, rightData, {10.0f, 10.0f}},
            {Enter, rightData, {10.0f, 10.0f}},
            {Move, leftData, {10.0f, 15.0f}},
            {Leave, rightData, {-10.0f, 15.0f}},
            {Enter, leftData, {10.0f, 15.0f}}
        })), TestSuite::Compare::Container);

    /* Releasing the capture should also be possible on a captured Leave
       event */
    } {
        /* Just to reset everything */
        /** @todo have a pointerCancelEvent() for this */
        PointerEvent eventReleaseReset{Pointer::MouseLeft};
        ui.pointerReleaseEvent({1000.0f, 1000.0f}, eventReleaseReset);
        PointerMoveEvent eventMoveReset{{}, {}};
        ui.pointerMoveEvent({1000.0f, 1000.0f}, eventMoveReset);
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        ui.layer<Layer>(layer).eventCalls = {};

        PointerMoveEvent eventMove1{{}, {}};
        ui.layer<Layer>(layer).capture = true;
        ui.layer<Layer>(layer).captureEnter = {};
        ui.layer<Layer>(layer).captureLeave = {};
        CORRADE_VERIFY(ui.pointerMoveEvent({30.0f, 10.0f}, eventMove1));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), left);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), left);

        PointerMoveEvent eventMove2{{}, {}};
        ui.layer<Layer>(layer).capture = {};
        ui.layer<Layer>(layer).captureEnter = {};
        ui.layer<Layer>(layer).captureLeave = false;
        CORRADE_VERIFY(ui.pointerMoveEvent({50.0f, 15.0f}, eventMove2));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        /* The event removed the capture, however it's not looking for a
           now-hovered node as that would mean doing the whole bubbling
           again */
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Move, leftData, {10.0f, 10.0f}},
            {Enter|Captured, leftData, {10.0f, 10.0f}},
            {Move|Captured, leftData, {30.0f, 15.0f}}, /* actually on rightData */
            {Leave|Captured, leftData, {30.0f, 15.0f}},
        })), TestSuite::Compare::Container);

    /* Capturing on an uncaptured Leave event does nothing however */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerMoveEvent eventMove1{{}, {}};
        ui.layer<Layer>(layer).capture = {};
        ui.layer<Layer>(layer).captureEnter = {};
        ui.layer<Layer>(layer).captureLeave = {};
        CORRADE_VERIFY(ui.pointerMoveEvent({30.0f, 10.0f}, eventMove1));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), left);

        PointerMoveEvent eventMove2{{}, {}};
        ui.layer<Layer>(layer).capture = {};
        ui.layer<Layer>(layer).captureEnter = {};
        ui.layer<Layer>(layer).captureLeave = true;
        CORRADE_VERIFY(ui.pointerMoveEvent({50.0f, 15.0f}, eventMove2));
        /* The capture isn't changed even though the Leave requested it */
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), right);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Move, leftData, {10.0f, 10.0f}},
            {Enter, leftData, {10.0f, 10.0f}},
            {Move, rightData, {10.0f, 15.0f}},
            {Leave, leftData, {30.0f, 15.0f}},
            {Enter, rightData, {10.0f, 15.0f}}
        })), TestSuite::Compare::Container);

    /* Enabling capture on an uncaptured Move and then disabling it again on an
       Enter should keep it disabled */
    } {
        /* Just to reset everything */
        /** @todo have a pointerCancelEvent() for this */
        PointerMoveEvent eventMoveReset{{}, {}};
        ui.pointerMoveEvent({1000.0f, 1000.0f}, eventMoveReset);
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        ui.layer<Layer>(layer).eventCalls = {};

        PointerMoveEvent eventMove1{{}, {}};
        ui.layer<Layer>(layer).capture = {};
        ui.layer<Layer>(layer).captureEnter = {};
        ui.layer<Layer>(layer).captureLeave = {};
        CORRADE_VERIFY(ui.pointerMoveEvent({30.0f, 10.0f}, eventMove1));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), left);

        PointerMoveEvent eventMove2{{}, {}};
        ui.layer<Layer>(layer).capture = true;
        ui.layer<Layer>(layer).captureEnter = false;
        ui.layer<Layer>(layer).captureLeave = {};
        CORRADE_VERIFY(ui.pointerMoveEvent({50.0f, 15.0f}, eventMove2));
        /* No capture as Enter reset it again */
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), right);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Move, leftData, {10.0f, 10.0f}},
            {Enter, leftData, {10.0f, 10.0f}},
            {Move, rightData, {10.0f, 15.0f}},
            /* Leave is only captured if it happens on a captured node, here it
               happens on some other */
            {Leave, leftData, {30.0f, 15.0f}},
            {Enter|Captured, rightData, {10.0f, 15.0f}}
        })), TestSuite::Compare::Container);

    /* Disabling capture on a captured Move and then enabling it again in Leave
       should keep it enabled */
    } {
        /* Just to reset everything */
        /** @todo have a pointerCancelEvent() for this */
        PointerMoveEvent eventMoveReset{{}, {}};
        ui.pointerMoveEvent({1000.0f, 1000.0f}, eventMoveReset);
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        ui.layer<Layer>(layer).eventCalls = {};

        PointerMoveEvent eventMove1{{}, {}};
        ui.layer<Layer>(layer).capture = true;
        ui.layer<Layer>(layer).captureEnter = {};
        ui.layer<Layer>(layer).captureLeave = {};
        CORRADE_VERIFY(ui.pointerMoveEvent({30.0f, 10.0f}, eventMove1));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), left);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), left);

        PointerMoveEvent eventMove2{{}, {}};
        ui.layer<Layer>(layer).capture = false;
        ui.layer<Layer>(layer).captureEnter = {};
        ui.layer<Layer>(layer).captureLeave = true;
        CORRADE_VERIFY(ui.pointerMoveEvent({50.0f, 15.0f}, eventMove2));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), left);
        /* On the right node, but captured on the left, so no hover */
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Move, leftData, {10.0f, 10.0f}},
            {Enter|Captured, leftData, {10.0f, 10.0f}},
            {Move|Captured, leftData, {30.0f, 15.0f}},
            /* Leave is only captured if it happens on a captured node, here it
               happens on some other */
            {Leave, leftData, {30.0f, 15.0f}}
        })), TestSuite::Compare::Container);
    }
}

void AbstractUserInterfaceTest::eventCaptureChangeCaptureInNotAcceptedEvent() {
    /* Event scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};

    enum Event {
        Captured = 1,
        Press = 2,
        Move = 4,
        Enter = 6,
        Leave = 8
    };
    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doPointerPressEvent(UnsignedInt dataId, PointerEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Press|(event.isCaptured() ? Captured : 0), dataHandle(handle(), dataId, 1), event.position());
            event.setAccepted();
        }
        void doPointerMoveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Move|(event.isCaptured() ? Captured : 0), dataHandle(handle(), dataId, 1), event.position());
            if((accept0 && dataId == 0) || (accept1 && dataId == 1))
                event.setAccepted();
            if(capture0 && dataId == 0)
                event.setCaptured(*capture0);
            if(capture1 && dataId == 1)
                event.setCaptured(*capture1);
        }
        void doPointerEnterEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Enter|(event.isCaptured() ? Captured : 0), dataHandle(handle(), dataId, 1), event.position());
        }
        void doPointerLeaveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Leave|(event.isCaptured() ? Captured : 0), dataHandle(handle(), dataId, 1), event.position());
        }

        bool accept0 = false,
            accept1 = false;
        Containers::Optional<bool> capture0, capture1;
        Containers::Array<Containers::Triple<Int, DataHandle, Vector2>> eventCalls;
    };

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<Layer>(layer));

    /* Two nodes on top of each other, node 1 above */
    NodeHandle node0 = ui.createNode({20.0f, 0.0f}, {20.0f, 20.0f});
    NodeHandle node1 = ui.createNode({20.0f, 0.0f}, {20.0f, 20.0f});
    DataHandle data0 = ui.layer<Layer>(layer).create();
    DataHandle data1 = ui.layer<Layer>(layer).create();
    ui.attachData(node0, data0);
    ui.attachData(node1, data1);

    /* Setting capture in events that don't get accepted should do nothing to
       subsequent events and nothing to the end result also */
    {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerMoveEvent eventMove{{}, {}};
        ui.layer<Layer>(layer).accept0 = true;
        ui.layer<Layer>(layer).accept1 = false;
        ui.layer<Layer>(layer).capture0 = {};
        ui.layer<Layer>(layer).capture1 = true;
        CORRADE_VERIFY(ui.pointerMoveEvent({30.0f, 10.0f}, eventMove));
        /* Node 1 captures in a non-accepted event, which should be ignored */
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), node0);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Move, data1, {10.0f, 10.0f}}, /* capturing but not accepted */
            {Move, data0, {10.0f, 10.0f}}, /* shouldn't capture */
            {Enter, data0, {10.0f, 10.0f}}, /* neither this */
        })), TestSuite::Compare::Container);

    /* Cancelling capture in a non-accepted captured move event (i.e., outside
       of bounds) should still work */
    } {
        /* Just to reset everything */
        /** @todo have a pointerCancelEvent() for this */
        PointerEvent eventReleaseReset{Pointer::MouseLeft};
        ui.pointerReleaseEvent({1000.0f, 1000.0f}, eventReleaseReset);
        PointerMoveEvent eventMoveReset{{}, {}};
        ui.pointerMoveEvent({1000.0f, 1000.0f}, eventMoveReset);
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        ui.layer<Layer>(layer).eventCalls = {};

        /* The press event accepts and captures unconditionally */
        PointerEvent eventPress{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({30.0f, 10.0f}, eventPress));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), node1);
        /* No Enter/Leave events synthesized from Press at the moment, so the
           hovered node doesn't get updated until the next move */
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        PointerMoveEvent eventMove{{}, {}};
        ui.layer<Layer>(layer).accept0 = false;
        ui.layer<Layer>(layer).accept1 = false;
        ui.layer<Layer>(layer).capture0 = {};
        ui.layer<Layer>(layer).capture1 = false;
        CORRADE_VERIFY(!ui.pointerMoveEvent({100.0f, 100.0f}, eventMove));
        /* The capture should be reset even though the move wasn't accepted */
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, data1, {10.0f, 10.0f}},
            {Move|Captured, data1, {80.0f, 100.0f}}, /* cancels the capture */
            /* There should be nothing else after */
        })), TestSuite::Compare::Container);
    }
}

void AbstractUserInterfaceTest::eventCaptureNodePositionUpdated() {
    auto&& data = EventCaptureUpdateData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* framebufferSize isn't used for anything here; events should get scaled
       to (0.1, 0.01) */
    AbstractUserInterface ui{{300.0f, 200.0f}, {3000.0f, 20000.0f}, {30, 20}};

    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doPointerPressEvent(UnsignedInt dataId, PointerEvent& event) override {
            CORRADE_VERIFY(event.isCaptured());
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, dataHandle(handle(), dataId, 1), event.position());
            event.setAccepted();
        }
        void doPointerReleaseEvent(UnsignedInt dataId, PointerEvent& event) override {
            CORRADE_VERIFY(event.isCaptured());
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, dataHandle(handle(), dataId, 1), event.position());
            event.setAccepted();
        }
        void doPointerMoveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            CORRADE_VERIFY(event.isCaptured());
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, dataHandle(handle(), dataId, 1), event.position());
            event.setAccepted();
        }
        /* No enter/leave events, those are tested in
           eventPointerMoveNodePositionUpdated() already */

        Containers::Array<Containers::Pair<DataHandle, Vector2>> eventCalls;
    };

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<Layer>(layer));

    /* A nested node to verify the event receives up-to-date position after its
       parent gets moved */
    NodeHandle node = ui.createNode({20.0f, 0.0f}, {20.0f, 20.0f});
    NodeHandle nested = ui.createNode(node, {}, {20.0f, 20.0f});
    DataHandle nestedData = ui.layer<Layer>(layer).create();
    ui.attachData(nested, nestedData);

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    PointerEvent eventPress{Pointer::MouseLeft};
    CORRADE_VERIFY(ui.pointerPressEvent({300.0f, 1000.0f}, eventPress));
    CORRADE_COMPARE(ui.pointerEventCapturedNode(), nested);

    ui.setNodeOffset(node, {30.0f, 20.0f});
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeLayoutUpdate);

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    if(data.release) {
        PointerEvent eventRelease{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({320.0f, 1000.0f}, eventRelease));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
    } else if(data.move) {
        PointerMoveEvent eventMove{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({320.0f, 1000.0f}, eventMove));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), nested);
    } else CORRADE_INTERNAL_ASSERT_UNREACHABLE();

    CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Pair<DataHandle, Vector2>>({
        {nestedData, {10.0f, 10.0f}},
        /* Should receive up-to-date position, not something relative to a
           position cached at the press; also properly considering the event
           scale */
        {nestedData, {12.0f - 10.0f, 10.0f - 20.0f}},
    })), TestSuite::Compare::Container);

    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
}

void AbstractUserInterfaceTest::eventCaptureNodeBecomesHidden() {
    auto&& data = EventCaptureNodeBecomesHiddenData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Event scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};

    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doPointerPressEvent(UnsignedInt dataId, PointerEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, dataHandle(handle(), dataId, 1), event.position(), event.isCaptured());
            event.setAccepted();
        }
        void doPointerReleaseEvent(UnsignedInt dataId, PointerEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, dataHandle(handle(), dataId, 1), event.position(), event.isCaptured());
            event.setAccepted();
        }
        void doPointerMoveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, dataHandle(handle(), dataId, 1), event.position(), event.isCaptured());
            event.setAccepted();
        }
        /* No enter/leave events, those are tested in
           eventPointerMoveNodeHidden() already */

        Containers::Array<Containers::Triple<DataHandle, Vector2, bool>> eventCalls;
    };

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<Layer>(layer));

    /* Two nodes next to each other, nested in order to verify that the hidden
       flag gets propagated through the hierarchy */
    NodeHandle left = ui.createNode({20.0f, 0.0f}, {20.0f, 20.0f});
    NodeHandle leftNested = ui.createNode(left, {}, {20.0f, 20.0f});
    NodeHandle right = ui.createNode({40.0f, 0.0f}, {20.0f, 20.0f});
    DataHandle leftData = ui.layer<Layer>(layer).create();
    DataHandle rightData = ui.layer<Layer>(layer).create();
    ui.attachData(leftNested, leftData);
    ui.attachData(right, rightData);

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    PointerEvent eventPress{Pointer::MouseLeft};
    CORRADE_VERIFY(ui.pointerPressEvent({30.0f, 10.0f}, eventPress));
    CORRADE_COMPARE(ui.pointerEventCapturedNode(), leftNested);

    if(data.flags)
        ui.addNodeFlags(left, data.flags);
    else if(data.clearOrder)
        ui.clearNodeOrder(left);
    else CORRADE_INTERNAL_ASSERT_UNREACHABLE();
    /* The current captured node stays after setting the flags, is only updated
       after update() -- there it also handles if any parent gets the flag as
       well */
    CORRADE_COMPARE(ui.pointerEventCapturedNode(), leftNested);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);

    if(data.update) {
        ui.update();

        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    if(data.release) {
        PointerEvent eventRelease{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({50.0f, 10.0f}, eventRelease));
    } else if(data.move) {
        PointerMoveEvent eventMove{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({50.0f, 10.0f}, eventMove));
    } else CORRADE_INTERNAL_ASSERT_UNREACHABLE();

    CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);

    CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<DataHandle, Vector2, bool>>({
        {leftData, {10.0f, 10.0f}, true},
        /* The release / move event isn't happening on a captured node, so
           isCaptured() is false for it */
        {rightData, {10.0f, 10.0f}, false},
    })), TestSuite::Compare::Container);

    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
}

void AbstractUserInterfaceTest::eventCaptureNodeRemoved() {
    auto&& data = EventCaptureNodeRemovedData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Event scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};

    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doPointerPressEvent(UnsignedInt dataId, PointerEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, dataHandle(handle(), dataId, 1), event.position(), event.isCaptured());
            event.setAccepted();
        }
        void doPointerReleaseEvent(UnsignedInt dataId, PointerEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, dataHandle(handle(), dataId, 1), event.position(), event.isCaptured());
            event.setAccepted();
        }
        void doPointerMoveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, dataHandle(handle(), dataId, 1), event.position(), event.isCaptured());
            event.setAccepted();
        }
        /* No enter/leave events, those are tested in
           eventPointerMoveNodeRemoved() already */

        Containers::Array<Containers::Triple<DataHandle, Vector2, bool>> eventCalls;
    };

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<Layer>(layer));

    /* Two nodes next to each other */
    NodeHandle left = ui.createNode({20.0f, 0.0f}, {20.0f, 20.0f});
    NodeHandle leftNested = ui.createNode(left, {}, {20.0f, 20.0f});
    NodeHandle right = ui.createNode({40.0f, 0.0f}, {20.0f, 20.0f});
    DataHandle leftData = ui.layer<Layer>(layer).create();
    DataHandle rightData = ui.layer<Layer>(layer).create();
    ui.attachData(leftNested, leftData);
    ui.attachData(right, rightData);

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    PointerEvent eventPress{Pointer::MouseLeft};
    CORRADE_VERIFY(ui.pointerPressEvent({30.0f, 10.0f}, eventPress));
    CORRADE_COMPARE(ui.pointerEventCapturedNode(), leftNested);

    ui.removeNode(data.removeParent ? left : leftNested);
    /* The current hovered node stays after removal, is only updated after
       update() -- there it also handles if any parent is removed */
    CORRADE_COMPARE(ui.pointerEventCapturedNode(), leftNested);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeClean);

    /* Add a visible node right in place of the removed one, to verify the
       generation is correctly checked as well */
    if(!data.removeParent) {
        NodeHandle leftNestedReplacement = ui.createNode(left, {}, {20.0f, 20.0f});
        CORRADE_COMPARE(nodeHandleId(leftNestedReplacement), nodeHandleId(leftNested));
    }

    if(data.update) {
        ui.update();

        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    if(data.release) {
        PointerEvent eventRelease{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({50.0f, 10.0f}, eventRelease));
    } else if(data.move) {
        PointerMoveEvent eventMove{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({50.0f, 10.0f}, eventMove));
    } else CORRADE_INTERNAL_ASSERT_UNREACHABLE();

    CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);

    CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<DataHandle, Vector2, bool>>({
        {leftData, {10.0f, 10.0f}, true},
        /* The release / move event isn't happening on a captured node, so
           isCaptured() is false for it */
        {rightData, {10.0f, 10.0f}, false},
    })), TestSuite::Compare::Container);

    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
}

void AbstractUserInterfaceTest::eventCaptureDataRemoved() {
    auto&& data = EventCaptureCleanUpdateData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Event scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};

    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doPointerPressEvent(UnsignedInt dataId, PointerEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, dataHandle(handle(), dataId, 1), event.position(), event.isCaptured());
            event.setAccepted();
        }
        void doPointerReleaseEvent(UnsignedInt dataId, PointerEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, dataHandle(handle(), dataId, 1), event.position(), event.isCaptured());
            event.setAccepted();
        }
        void doPointerMoveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, dataHandle(handle(), dataId, 1), event.position(), event.isCaptured());
            event.setAccepted();
        }
        /* No enter/leave events, those are tested in
           eventPointerMoveDataRemoved() already */

        Containers::Array<Containers::Triple<DataHandle, Vector2, bool>> eventCalls;
    };

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<Layer>(layer));

    /* Two nodes next to each other */
    NodeHandle left = ui.createNode({20.0f, 0.0f}, {20.0f, 20.0f});
    NodeHandle right = ui.createNode({40.0f, 0.0f}, {20.0f, 20.0f});
    DataHandle leftData = ui.layer<Layer>(layer).create();
    DataHandle rightData = ui.layer<Layer>(layer).create();
    ui.attachData(left, leftData);
    ui.attachData(right, rightData);

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    PointerEvent eventPress{Pointer::MouseLeft};
    CORRADE_VERIFY(ui.pointerPressEvent({30.0f, 10.0f}, eventPress));
    CORRADE_COMPARE(ui.pointerEventCapturedNode(), left);

    ui.layer<Layer>(layer).remove(leftData);
    CORRADE_COMPARE(ui.pointerEventCapturedNode(), left);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataClean);

    if(data.clean) {
        ui.clean();

        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate);
    }

    if(data.update) {
        ui.update();

        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    if(data.release) {
        PointerEvent eventRelease{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({50.0f, 10.0f}, eventRelease));
    } else if(data.move) {
        PointerMoveEvent eventMove{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({50.0f, 10.0f}, eventMove));
    } else CORRADE_INTERNAL_ASSERT_UNREACHABLE();

    CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);

    CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<DataHandle, Vector2, bool>>({
        {leftData, {10.0f, 10.0f}, true},
        /* The release / move event isn't happening on a captured node, so
            isCaptured() is false for it */
        /** @todo eventually this might then try to call different data on the
            same node (with isCaptured() set) if that ends up being a desirable
            behavior */
        {rightData, {10.0f, 10.0f}, false},
    })), TestSuite::Compare::Container);

    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
}

}}}}

CORRADE_TEST_MAIN(Magnum::Whee::Test::AbstractUserInterfaceTest)
