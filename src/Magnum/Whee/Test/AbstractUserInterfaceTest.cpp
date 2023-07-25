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

    void construct();
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

    void eventCapture();
    void eventCaptureNotAccepted();
    void eventCaptureNotCaptured();
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

              &AbstractUserInterfaceTest::construct,
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

    addInstancedTests({&AbstractUserInterfaceTest::drawEmpty,
                       &AbstractUserInterfaceTest::draw,
                       &AbstractUserInterfaceTest::eventEmpty},
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

    addTests({&AbstractUserInterfaceTest::eventCapture,
              &AbstractUserInterfaceTest::eventCaptureNotAccepted,
              &AbstractUserInterfaceTest::eventCaptureNotCaptured});

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

    /* NeedsDataClean is a superset of NeedsDataAttachmentUpdate, so only one
       should be printed */
    } {
        std::ostringstream out;
        Debug{&out} << (UserInterfaceState::NeedsDataClean |UserInterfaceState::NeedsDataAttachmentUpdate);
        CORRADE_COMPARE(out.str(), "Whee::UserInterfaceState::NeedsDataClean\n");

    /* NeedsNodeUpdate is a superset of NeedsDataAttachmentUpdate, so only one
       should be printed */
    } {
        std::ostringstream out;
        Debug{&out} << (UserInterfaceState::NeedsNodeUpdate|UserInterfaceState::NeedsDataAttachmentUpdate);
        CORRADE_COMPARE(out.str(), "Whee::UserInterfaceState::NeedsNodeUpdate\n");

    /* NeedsNodeLayoutUpdate is a superset of NeedsDataUpdate, so only one
       should be printed */
    } {
        std::ostringstream out;
        Debug{&out} << (UserInterfaceState::NeedsNodeLayoutUpdate|UserInterfaceState::NeedsDataUpdate);
        CORRADE_COMPARE(out.str(), "Whee::UserInterfaceState::NeedsNodeLayoutUpdate\n");

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

    /* NeedsNodeUpdate is a superset of both NeedsDataAttachmentUpdate and
       NeedsNodeLayoutUpdate, so only one should be printed */
    } {
        std::ostringstream out;
        Debug{&out} << (UserInterfaceState::NeedsNodeUpdate|UserInterfaceState::NeedsNodeLayoutUpdate|UserInterfaceState::NeedsNodeUpdate);
        CORRADE_COMPARE(out.str(), "Whee::UserInterfaceState::NeedsNodeUpdate\n");

    /* NeedsNodeUpdate and NeedsDataClean are both supersets of
       NeedsDataAttachmentUpdate, so only the two should be printed */
    } {
        std::ostringstream out;
        Debug{&out} << (UserInterfaceState::NeedsNodeUpdate|UserInterfaceState::NeedsDataClean);
        CORRADE_COMPARE(out.str(), "Whee::UserInterfaceState::NeedsNodeUpdate|Whee::UserInterfaceState::NeedsDataClean\n");

    /* NeedsNodeClean is a superset of all others, so it should be printed
       alone */
    } {
        std::ostringstream out;
        Debug{&out} << (UserInterfaceState::NeedsNodeClean|UserInterfaceState::NeedsDataClean |UserInterfaceState::NeedsNodeUpdate|UserInterfaceState::NeedsDataUpdate|UserInterfaceState::NeedsDataAttachmentUpdate);
        CORRADE_COMPARE(out.str(), "Whee::UserInterfaceState::NeedsNodeClean\n");
    }
}

void AbstractUserInterfaceTest::construct() {
    AbstractUserInterface ui;

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
}

void AbstractUserInterfaceTest::constructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<AbstractUserInterface>{});
    CORRADE_VERIFY(!std::is_copy_assignable<AbstractUserInterface>{});
}

void AbstractUserInterfaceTest::constructMove() {
    AbstractUserInterface a;
    a.createLayer();

    /* The class has an internal state struct containing everything, so it's
       not needed to test each and every property */
    AbstractUserInterface b{Utility::move(a)};
    CORRADE_COMPARE(b.layerCapacity(), 1);
    CORRADE_COMPARE(b.layerUsedCount(), 1);
    CORRADE_COMPARE(b.nodeCapacity(), 0);
    CORRADE_COMPARE(b.nodeUsedCount(), 0);
    CORRADE_COMPARE(b.dataAttachmentCount(), 0);

    AbstractUserInterface c;
    c.createNode(NodeHandle::Null, {}, {}, {});
    c = Utility::move(b);
    CORRADE_COMPARE(c.layerCapacity(), 1);
    CORRADE_COMPARE(c.layerUsedCount(), 1);
    CORRADE_COMPARE(c.nodeCapacity(), 0);
    CORRADE_COMPARE(c.nodeUsedCount(), 0);
    CORRADE_COMPARE(c.dataAttachmentCount(), 0);

    CORRADE_VERIFY(std::is_nothrow_move_constructible<AbstractUserInterface>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<AbstractUserInterface>::value);
}

void AbstractUserInterfaceTest::layer() {
    AbstractUserInterface ui;
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
    AbstractUserInterface ui;
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
    AbstractUserInterface ui;

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
    AbstractUserInterface ui;
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
        AbstractUserInterface ui;
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
        ui.setLayerInstance(Utility::move(secondInstance));
        ui.setLayerInstance(Utility::move(firstInstance));
        CORRADE_COMPARE(ui.layerCapacity(), 3);
        CORRADE_COMPARE(ui.layerUsedCount(), 3);
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

    AbstractUserInterface ui;

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

    AbstractUserInterface ui;

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

    AbstractUserInterface ui;
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

    AbstractUserInterface ui;

    std::ostringstream out;
    Error redirectError{&out};
    ui.removeLayer(LayerHandle::Null);
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractUserInterface::removeLayer(): invalid handle Whee::LayerHandle::Null\n");
}

void AbstractUserInterfaceTest::layerNoHandlesLeft() {
    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui;

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
    AbstractUserInterface ui;
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
    AbstractUserInterface ui;
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
    AbstractUserInterface ui;

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
    AbstractUserInterface ui;

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

    AbstractUserInterface ui;

    std::ostringstream out;
    Error redirectError{&out};
    ui.createNode(NodeHandle(0x123abcde), {}, {});
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractUserInterface::createNode(): invalid parent handle Whee::NodeHandle(0xabcde, 0x123)\n");
}

void AbstractUserInterfaceTest::nodeGetSetInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui;

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

    AbstractUserInterface ui;

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

    AbstractUserInterface ui;

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
    AbstractUserInterface ui;
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

    AbstractUserInterface ui;
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
    AbstractUserInterface ui;
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
    AbstractUserInterface ui;
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

    AbstractUserInterface ui;
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

void AbstractUserInterfaceTest::cleanEmpty() {
    AbstractUserInterface ui;
    CORRADE_COMPARE(ui.nodeUsedCount(), 0);
    CORRADE_COMPARE(ui.dataAttachmentCount(), 0);

    ui.clean();
    CORRADE_COMPARE(ui.nodeUsedCount(), 0);
    CORRADE_COMPARE(ui.dataAttachmentCount(), 0);
}

void AbstractUserInterfaceTest::cleanNoOp() {
    AbstractUserInterface ui;
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
    AbstractUserInterface ui;
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
    AbstractUserInterface ui;
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
    AbstractUserInterface ui;
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
    AbstractUserInterface ui;
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
    AbstractUserInterface ui;
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
    AbstractUserInterface ui;
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
    AbstractUserInterface ui;
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

    AbstractUserInterface ui;
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
    AbstractUserInterface ui;
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

    AbstractUserInterface ui;
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

    /* Creating nodes sets a state flag */
    NodeHandle node = ui.createNode({1.0f, 2.0f}, {3.0f, 4.0f});
    NodeHandle another = ui.createNode({2.0f, 1.0f}, {4.0f, 3.0f});
    NodeHandle nested = ui.createNode(node, {0.5f, 1.5f}, {2.5f, 3.5f});
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
            CORRADE_ITERATION(__LINE__);
            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 0);
    }

    /* Calling update() should be a no-op, not calling anything in the layer */
    if(data.noOp) {
        {
            CORRADE_ITERATION(__LINE__);
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
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 0);

    /* Calling clean() should be a no-op */
    if(data.clean && data.noOp) {
        {
            CORRADE_ITERATION(__LINE__);
            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 0);
    }

    /* Calling update() should be a no-op too */
    if(data.noOp) {
        {
            CORRADE_ITERATION(__LINE__);
            ui.update();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 0);
    }

    /* Attaching the data sets flags. Shuffled order to have non-trivial
       results. */
    ui.attachData(node, data2);
    ui.attachData(nested, data1);
    ui.attachData(another, data3);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate);

    /* Calling clean() should be a no-op */
    if(data.clean && data.noOp) {
        {
            CORRADE_ITERATION(__LINE__);
            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate);
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 0);
    }

    /* Calling update() rebuilds internal state, calls doUpdate() on the layer,
       and resets the flag. */
    {
        CORRADE_ITERATION(__LINE__);
        Containers::Pair<UnsignedInt, UnsignedInt> expectedData[]{
            {dataHandleId(data2), nodeHandleId(node)},
            {dataHandleId(data1), nodeHandleId(nested)},
            {dataHandleId(data3), nodeHandleId(another)}
        };
        Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
            {{1.0f, 2.0f}, {3.0f, 4.0f}}, /* node */
            {{2.0f, 1.0f}, {4.0f, 3.0f}}, /* another */
            {{1.5f, 3.5f}, {2.5f, 3.5f}}, /* nested */
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
            CORRADE_ITERATION(__LINE__);
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
        CORRADE_ITERATION(__LINE__);
        Containers::Pair<UnsignedInt, UnsignedInt> expectedData[]{
            {dataHandleId(data2), nodeHandleId(node)},
            {dataHandleId(data1), nodeHandleId(nested)},
            {dataHandleId(data3), nodeHandleId(another)}
        };
        Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
            {{1.0f, 2.0f}, {3.0f, 4.0f}}, /* node */
            {{2.0f, 1.0f}, {4.0f, 3.0f}}, /* another */
            {{1.5f, 3.5f}, {2.5f, 3.5f}}, /* nested */
        };
        ui.layer<Layer>(layer).expectedData = expectedData;
        ui.layer<Layer>(layer).expectedNodeOffsetsSizes = expectedNodeOffsetsSizes;
        ui.update();
    }
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
    CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 2);

    /* Changing a node size sets a state flag to update the data. Individual
       node resize doesn't currently affect the layout in any way, so no
       NeedsNodeLayoutUpdate. */
    ui.setNodeSize(node, {3.5f, 4.5f});
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataUpdate);

    /* Calling clean() should be a no-op */
    if(data.clean && data.noOp) {
        {
            CORRADE_ITERATION(__LINE__);
            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataUpdate);
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 2);
    }

    /* Calling update() reuploads the data with a single size changed and
       resets the flag, but internally shouldn't do any other state rebuild */
    {
        CORRADE_ITERATION(__LINE__);
        Containers::Pair<UnsignedInt, UnsignedInt> expectedData[]{
            {dataHandleId(data2), nodeHandleId(node)},
            {dataHandleId(data1), nodeHandleId(nested)},
            {dataHandleId(data3), nodeHandleId(another)}
        };
        Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
            {{1.0f, 2.0f}, {3.5f, 4.5f}}, /* node */
            {{2.0f, 1.0f}, {4.0f, 3.0f}}, /* another */
            {{1.5f, 3.5f}, {2.5f, 3.5f}}, /* nested */
        };
        ui.layer<Layer>(layer).expectedData = expectedData;
        ui.layer<Layer>(layer).expectedNodeOffsetsSizes = expectedNodeOffsetsSizes;
        ui.update();
    }
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
    CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 3);

    /* Changing a node offset sets a state flag to recalculate also nested
       node offsets. */
    ui.setNodeOffset(node, {1.5f, 2.5f});
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeLayoutUpdate);

    /* Calling clean() should be a no-op */
    if(data.clean && data.noOp) {
        {
            CORRADE_ITERATION(__LINE__);
            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeLayoutUpdate);
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 3);
    }

    /* Calling update() recalculates absoute offsets, uploads the new data and
       resets the flag */
    {
        CORRADE_ITERATION(__LINE__);
        Containers::Pair<UnsignedInt, UnsignedInt> expectedData[]{
            {dataHandleId(data2), nodeHandleId(node)},
            {dataHandleId(data1), nodeHandleId(nested)},
            {dataHandleId(data3), nodeHandleId(another)}
        };
        Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
            {{1.5f, 2.5f}, {3.5f, 4.5f}}, /* node */
            {{2.0f, 1.0f}, {4.0f, 3.0f}}, /* another */
            {{2.0f, 4.0f}, {2.5f, 3.5f}}, /* nested */
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
            CORRADE_ITERATION(__LINE__);
            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 4);
    }

    /* Calling update() rebuilds internal state without the hidden hierarchy */
    {
        CORRADE_ITERATION(__LINE__);
        Containers::Pair<UnsignedInt, UnsignedInt> expectedData[]{
            {dataHandleId(data3), nodeHandleId(another)}
        };
        Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
            {},
            {{2.0f, 1.0f}, {4.0f, 3.0f}}, /* another */
            {},
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
            CORRADE_ITERATION(__LINE__);
            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 5);
    }

    /* Calling update() reuploads the original data again and resets the
       flag */
    {
        CORRADE_ITERATION(__LINE__);
        Containers::Pair<UnsignedInt, UnsignedInt> expectedData[]{
            {dataHandleId(data2), nodeHandleId(node)},
            {dataHandleId(data1), nodeHandleId(nested)},
            {dataHandleId(data3), nodeHandleId(another)}
        };
        Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
            {{1.5f, 2.5f}, {3.5f, 4.5f}}, /* node */
            {{2.0f, 1.0f}, {4.0f, 3.0f}}, /* another */
            {{2.0f, 4.0f}, {2.5f, 3.5f}}, /* nested */
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

    /* Calling clearNodeOrder() sets a state flag */
    ui.clearNodeOrder(another);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);

    /* Calling clean() should be a no-op */
    if(data.clean && data.noOp) {
        {
            CORRADE_ITERATION(__LINE__);
            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 6);
    }

    /* Calling update() uploads data in new order and resets the flag */
    {
        CORRADE_ITERATION(__LINE__);
        Containers::Pair<UnsignedInt, UnsignedInt> expectedData[]{
            {dataHandleId(data2), nodeHandleId(node)},
            {dataHandleId(data1), nodeHandleId(nested)}
        };
        Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
            {{1.5f, 2.5f}, {3.5f, 4.5f}}, /* node */
            {},
            {{2.0f, 4.0f}, {2.5f, 3.5f}}, /* nested */
        };
        ui.layer<Layer>(layer).expectedData = expectedData;
        ui.layer<Layer>(layer).expectedNodeOffsetsSizes = expectedNodeOffsetsSizes;
        ui.update();
    }
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
    CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 7);

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
            CORRADE_ITERATION(__LINE__);
            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 7);
    }

    /* Calling update() uploads data in new order and resets the flag */
    {
        CORRADE_ITERATION(__LINE__);
        Containers::Pair<UnsignedInt, UnsignedInt> expectedData[]{
            {dataHandleId(data3), nodeHandleId(another)},
            {dataHandleId(data2), nodeHandleId(node)},
            {dataHandleId(data1), nodeHandleId(nested)},
        };
        Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
            {{1.5f, 2.5f}, {3.5f, 4.5f}}, /* node */
            {{2.0f, 1.0f}, {4.0f, 3.0f}}, /* another */
            {{2.0f, 4.0f}, {2.5f, 3.5f}}, /* nested */
        };
        ui.layer<Layer>(layer).expectedData = expectedData;
        ui.layer<Layer>(layer).expectedNodeOffsetsSizes = expectedNodeOffsetsSizes;
        ui.update();
    }
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
    CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 8);

    /* Removing data marks the layer with NeedsClean, which is then propagated
       to the UI-wide state */
    ui.layer(layer).remove(data2);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataClean);
    CORRADE_COMPARE(ui.dataAttachmentCount(), 3);

    /* Calling clean() removes the now-invalid attachment and resets the states
       to not require clean() anymore */
    if(data.clean) {
        {
            CORRADE_ITERATION(__LINE__);
            bool expectedDataIdsToRemove[]{
                false, false, false /* data2 already removed, so not set */
            };
            ui.layer<Layer>(layer).expectedDataIdsToRemove = expectedDataIdsToRemove;
            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate);
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 1);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 8);
        CORRADE_COMPARE(ui.dataAttachmentCount(), 2);
    }

    /* Calling update() then uploads remaining data and resets the remaining
       state flag; also calls clean() if wasn't done above already */
    {
        CORRADE_ITERATION(__LINE__);
        bool expectedDataIdsToRemove[]{
            false, false, false /* data2 already removed, so not set */
        };
        Containers::Pair<UnsignedInt, UnsignedInt> expectedData[]{
            {dataHandleId(data3), nodeHandleId(another)},
            {dataHandleId(data1), nodeHandleId(nested)},
        };
        Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
            {{1.5f, 2.5f}, {3.5f, 4.5f}}, /* node */
            {{2.0f, 1.0f}, {4.0f, 3.0f}}, /* another */
            {{2.0f, 4.0f}, {2.5f, 3.5f}}, /* nested */
        };
        ui.layer<Layer>(layer).expectedDataIdsToRemove = expectedDataIdsToRemove;
        ui.layer<Layer>(layer).expectedData = expectedData;
        ui.layer<Layer>(layer).expectedNodeOffsetsSizes = expectedNodeOffsetsSizes;
        ui.update();
    }
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.dataAttachmentCount(), 2);
    /* doClean() should only be called either in the branch above or from
       update(), never both */
    CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 1);
    CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 9);

    /* Removing a node sets a state flag */
    ui.removeNode(node);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeClean);
    CORRADE_COMPARE(ui.nodeUsedCount(), 2);
    CORRADE_COMPARE(ui.dataAttachmentCount(), 2);

    /* Calling clean() removes the child nodes, the now-invalid attachment and
       resets the state to not require clean() anymore */
    if(data.clean) {
        {
            CORRADE_ITERATION(__LINE__);
            bool expectedDataIdsToRemove[]{
                /* data1 was attached to `nested`, which got orphaned after
                   removing its parent, `node` */
                true, false, false
            };
            ui.layer<Layer>(layer).expectedDataIdsToRemove = expectedDataIdsToRemove;
            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate|UserInterfaceState::NeedsDataAttachmentUpdate);
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 2);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 9);
        CORRADE_COMPARE(ui.nodeUsedCount(), 1);
        CORRADE_COMPARE(ui.dataAttachmentCount(), 1);
    }

    /* Calling update() then uploads remaining data and resets the remaining
       state flag */
    {
        CORRADE_ITERATION(__LINE__);
        bool expectedDataIdsToRemove[]{
            /* data1 was attached to `nested`, which got orphaned after
                removing its parent, `node` */
            true, false, false
        };
        Containers::Pair<UnsignedInt, UnsignedInt> expectedData[]{
            {dataHandleId(data3), nodeHandleId(another)},
        };
        Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
            {},
            {{2.0f, 1.0f}, {4.0f, 3.0f}}, /* another */
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
    CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 10);

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
            CORRADE_ITERATION(__LINE__);
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
        CORRADE_ITERATION(__LINE__);
        Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
            {},
            {{2.0f, 1.0f}, {4.0f, 3.0f}}, /* another */
            {},
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

    AbstractUserInterface ui;

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

    AbstractUserInterface ui;
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
    auto&& data = CleanUpdateData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui;

    struct Layer: AbstractLayer {
        explicit Layer(LayerHandle handle, LayerFeatures features): AbstractLayer{handle}, features{features} {}

        LayerFeatures doFeatures() const override { return features; }

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
        Containers::Array<Containers::Triple<LayerHandle, std::size_t, std::size_t>>* drawCalls;

        Containers::StridedArrayView1D<const UnsignedInt> actualDataIds;
        Containers::StridedArrayView1D<const UnsignedInt> actualDataNodeIds;
        Containers::StridedArrayView1D<const Vector2> actualNodeOffsets;
        Containers::StridedArrayView1D<const Vector2> actualNodeSizes;
    };

    NodeHandle topLevel = ui.createNode({10.0f, 20.0f}, {200.0f, 100.0f});
    NodeHandle left = ui.createNode(topLevel, {30.0f, 40.0f}, {20.0f, 10.0f});
    NodeHandle right = ui.createNode(topLevel, {60.0f, 40.0f}, {15.0f, 25.0f});
    NodeHandle anotherTopLevel = ui.createNode({100.0f, 200.0f}, {5.0f, 10.0f});
    NodeHandle topLevelNotInOrder = ui.createNode({}, {});
    NodeHandle removed = ui.createNode(right, {}, {});
    NodeHandle nested = ui.createNode(left, {20.0f, 30.0f}, {5.0f, 7.0f});

    /* These follow the node handle IDs, nodes that are not part of the
       visible hierarchy have the data undefined */
    Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
        {{10.0f, 20.0f}, {200.0f, 100.0f}},
        {{40.0f, 60.0f}, {20.0f, 10.0f}},
        {{70.0f, 60.0f}, {15.0f, 25.0f}},
        {{100.0f, 200.0f}, {5.0f, 10.0f}},
        {}, /* not in order */
        {}, /* removed */
        {{60.0f, 90.0f}, {5.0f, 7.0f}}
    };

    /* Layer without an instance, to verify those get skipped during updates */
    /*LayerHandle layerWithoutInstance =*/ ui.createLayer();

    LayerHandle layer1 = ui.createLayer();
    Containers::Pointer<Layer> layer1Instance{InPlaceInit, layer1, LayerFeature::Draw};

    LayerHandle layer2 = ui.createLayer();
    Containers::Pointer<Layer> layer2Instance{InPlaceInit, layer2, LayerFeature::Draw|LayerFeature::Event};

    LayerHandle layer3 = ui.createLayer();
    Containers::Pointer<Layer> layer3Instance{InPlaceInit, layer3, LayerFeature::Event};

    DataHandle leftData2 = layer1Instance->create();
    DataHandle leftData1 = layer2Instance->create();
    DataHandle leftData3 = layer1Instance->create();
    DataHandle anotherTopLevelData1 = layer1Instance->create();
    DataHandle anotherTopLevelData2 = layer2Instance->create();
    DataHandle anotherTopLevelData3 = layer3Instance->create();
    DataHandle anotherTopLevelData4 = layer2Instance->create();
    DataHandle topLevelData = layer3Instance->create();
    DataHandle nestedData = layer2Instance->create();
    DataHandle topLevelNotInOrderData = layer2Instance->create();
    DataHandle removedData = layer1Instance->create();
    DataHandle rightData1 = layer3Instance->create();
    DataHandle rightData2 = layer2Instance->create();

    /* These follow the node nesting order and then the order in which the
       data get attached below */
    Containers::Pair<UnsignedInt, UnsignedInt> expectedLayer1Data[]{
        /* anotherTopLevel is reordered as first */
        {dataHandleId(anotherTopLevelData1), nodeHandleId(anotherTopLevel)},
        /* Data belonging to topLevel are after it */
        {dataHandleId(leftData2), nodeHandleId(left)},
        {dataHandleId(leftData3), nodeHandleId(left)},
        /* removedData not here as the containing node is removed */
    };
    Containers::Pair<UnsignedInt, UnsignedInt> expectedLayer2Data[]{
        /* anotherTopLevel */
        {dataHandleId(anotherTopLevelData2), nodeHandleId(anotherTopLevel)},
        {dataHandleId(anotherTopLevelData4), nodeHandleId(anotherTopLevel)},
        /* topLevel */
        {dataHandleId(leftData1), nodeHandleId(left)},
        {dataHandleId(nestedData), nodeHandleId(nested)},
        {dataHandleId(rightData2), nodeHandleId(right)},
        /* Nothing for topLevelNotInOrderData as it's not visible */
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
    ui.attachData(right, rightData2);

    ui.setNodeOrder(anotherTopLevel, topLevel);
    ui.clearNodeOrder(topLevelNotInOrder);
    ui.removeNode(removed);
    CORRADE_COMPARE(ui.dataAttachmentCount(), 13);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeClean);
    CORRADE_COMPARE(layer1UpdateCallCount, 0);
    CORRADE_COMPARE(layer2UpdateCallCount, 0);
    CORRADE_COMPARE(layer3UpdateCallCount, 0);

    if(data.clean) {
        ui.clean();
        CORRADE_COMPARE(ui.dataAttachmentCount(), 12);
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);
        CORRADE_COMPARE(layer1UpdateCallCount, 0);
        CORRADE_COMPARE(layer2UpdateCallCount, 0);
        CORRADE_COMPARE(layer3UpdateCallCount, 0);
    }

    /* update() should call clean() only if needed */
    if(data.update) {
        ui.update();
        CORRADE_COMPARE(ui.dataAttachmentCount(), 12);
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
        CORRADE_COMPARE(layer1UpdateCallCount, 1);
        CORRADE_COMPARE(layer2UpdateCallCount, 1);
        CORRADE_COMPARE(layer3UpdateCallCount, 1);
    }

    /* draw() should call update() and clean() only if needed */
    ui.draw();
    CORRADE_COMPARE(ui.dataAttachmentCount(), 12);
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
            {layer2, 2, 3}
        /* layer 3 doesn't have LayerFeature::Draw, so draw() shouldn't be
           called with anything for it */
    })), TestSuite::Compare::Container);
}

void AbstractUserInterfaceTest::eventEmpty() {
    auto&& data = CleanUpdateData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui;
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
    CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
}

void AbstractUserInterfaceTest::eventAlreadyAccepted() {
    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui;
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

    AbstractUserInterface ui;

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
        CORRADE_VERIFY(!ui.pointerPressEvent({300.0f, 300.0f}, event));
        CORRADE_COMPARE_AS(eventCalls, (Containers::arrayView<Containers::Triple<DataHandle, Vector2, bool>>({
        })), TestSuite::Compare::Container);

    /* On the notInOrder node that's not visible, no hit */
    } {
        layer1Accept = layer2Accept = layer3Accept = true;
        eventCalls = {};
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(!ui.pointerPressEvent({195.0f, 195.0f}, event));
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
        CORRADE_COMPARE_AS(eventCalls, (Containers::arrayView<Containers::Triple<DataHandle, Vector2, bool>>({
        })), TestSuite::Compare::Container);

    /* On the top-level node with no other node covering it */
    } {
        layer1Accept = layer2Accept = layer3Accept = true;
        eventCalls = {};
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({100.0f, 60.0f}, event));
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
        CORRADE_VERIFY(ui.pointerPressEvent({115.0f, 60.0f}, event));
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
        CORRADE_VERIFY(!ui.pointerPressEvent({115.0f, 60.0f}, event));
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
        CORRADE_VERIFY(ui.pointerPressEvent({100.0f, 60.0f}, event));
        CORRADE_COMPARE_AS(eventCalls, (Containers::arrayView<Containers::Triple<DataHandle, Vector2, bool>>({
            {topData, {85.0f, 35.0f}, false},
            {bottomData1, {90.0f, 40.0f}, true}
        })), TestSuite::Compare::Container);

    /* On a nested node, last added data get picked first */
    } {
        layer1Accept = layer2Accept = layer3Accept = true;
        eventCalls = {};
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({40.0f, 60.0f}, event));
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
        CORRADE_VERIFY(ui.pointerPressEvent({40.0f, 60.0f}, event));
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
        CORRADE_VERIFY(!ui.pointerPressEvent({40.0f, 60.0f}, event));
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
        CORRADE_VERIFY(ui.pointerPressEvent({43.0f, 63.0f}, event));
        CORRADE_COMPARE_AS(eventCalls, (Containers::arrayView<Containers::Triple<DataHandle, Vector2, bool>>({
            {topNestedOutsideData, {0.5f, 0.5f}, true},
        })), TestSuite::Compare::Container);
    } {
        layer1Accept = layer2Accept = layer3Accept = true;
        eventCalls = {};
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({46.0f, 66.0f}, event));
        CORRADE_COMPARE_AS(eventCalls, (Containers::arrayView<Containers::Triple<DataHandle, Vector2, bool>>({
            /* It's at {3.5f, 3.5f} for topNestedOutside, but that's outside
               of topNested so it isn't considered */
            {topData, {31.0f, 41.0f}, true},
        })), TestSuite::Compare::Container);
    }

    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
}

void AbstractUserInterfaceTest::eventEdges() {
    AbstractUserInterface ui;

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
        CORRADE_VERIFY(ui.pointerPressEvent({10.0f, 20.0f}, event));
        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Pair<DataHandle, Vector2>>({
            {topData, {0.0f, 0.0f}},
        })), TestSuite::Compare::Container);

    /* Top edge should go to the top node */
    } {
        ui.layer<Layer>(layer).eventCalls = {};
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({57.0f, 20.0f}, event));
        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Pair<DataHandle, Vector2>>({
            {topData, {47.0f, 0.0f}},
        })), TestSuite::Compare::Container);

    /* Left edge should go to the top node */
    } {
        ui.layer<Layer>(layer).eventCalls = {};
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({10.0f, 34.0f}, event));
        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Pair<DataHandle, Vector2>>({
            {topData, {0.0f, 14.0f}},
        })), TestSuite::Compare::Container);

    /* Bottom right corner should go to the bottom node */
    } {
        ui.layer<Layer>(layer).eventCalls = {};
        PointerEvent event1{Pointer::MouseLeft};
        PointerEvent event2{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({90.0f, 80.0f}, event1));
        CORRADE_VERIFY(ui.pointerPressEvent({89.9f, 79.9f}, event2));
        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Pair<DataHandle, Vector2>>({
            {bottomData, {90.0f, 80.0f}},
            {topData, {79.9f, 59.9f}},
        })), TestSuite::Compare::Container);

    /* Bottom edge should go to the top node */
    } {
        ui.layer<Layer>(layer).eventCalls = {};
        PointerEvent event1{Pointer::MouseLeft};
        PointerEvent event2{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({90.0f, 34.0f}, event1));
        CORRADE_VERIFY(ui.pointerPressEvent({89.9f, 34.0f}, event2));
        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Pair<DataHandle, Vector2>>({
            {bottomData, {90.0f, 34.0f}},
            {topData, {79.9f, 14.0f}},
        })), TestSuite::Compare::Container);

    /* Right edge should go to the bottom node */
    } {
        ui.layer<Layer>(layer).eventCalls = {};
        PointerEvent event1{Pointer::MouseLeft};
        PointerEvent event2{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({57.0f, 80.0f}, event1));
        CORRADE_VERIFY(ui.pointerPressEvent({57.0f, 79.9f}, event2));
        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Pair<DataHandle, Vector2>>({
            {bottomData, {57.0f, 80.0f}},
            {topData, {47.0f, 59.9f}},
        })), TestSuite::Compare::Container);
    }
}

void AbstractUserInterfaceTest::eventPointerPress() {
    auto&& data = UpdateData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui;

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
        CORRADE_VERIFY(!ui.pointerPressEvent({100.0f, 100.0f}, event));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.layer<Layer>(layer).acceptedCount, 0);

    /* Inside, hit */
    } {
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({20.0f, 25.0f}, event));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), node);
        CORRADE_COMPARE(ui.layer<Layer>(layer).acceptedCount, 1);
    }

    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
}

void AbstractUserInterfaceTest::eventPointerRelease() {
    auto&& data = UpdateData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui;

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
        CORRADE_VERIFY(!ui.pointerReleaseEvent({100.0f, 100.0f}, event));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.layer<Layer>(layer).acceptedCount, 0);

    /* Inside, hit */
    } {
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({20.0f, 25.0f}, event));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.layer<Layer>(layer).acceptedCount, 1);
    }

    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
}

void AbstractUserInterfaceTest::eventPointerMove() {
    auto&& data = UpdateData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui;

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
            arrayAppend(eventCalls, InPlaceInit, dataHandle(handle(), dataId, 1), Vector4{event.position().x(), event.position().y(), event.relativePosition().x(), event.relativePosition().y()});
            event.setAccepted();
        }

        Containers::Array<Containers::Pair<DataHandle, Vector4>> eventCalls;
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
        CORRADE_VERIFY(!ui.pointerMoveEvent({10.0f, 10.0f}, event));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Pair<DataHandle, Vector4>>({
        })), TestSuite::Compare::Container);

    /* Inside and then to another item. Relative to previous move event even
       though it didn't hit anything. */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerMoveEvent event1{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({30.0f, 11.0f}, event1));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);

        PointerMoveEvent event2{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({50.0f, 10.0f}, event2));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Pair<DataHandle, Vector4>>({
            {leftData, {10.0f, 11.0f, 20.0f, 1.0f}},
            {rightData, {10.0f, 10.0f, 20.0f, -1.0f}},
        })), TestSuite::Compare::Container);

    /* Out of the item, again relative to what happened last */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerMoveEvent event{{}, {}};
        CORRADE_VERIFY(!ui.pointerMoveEvent({10.0f, 11.0f}, event));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Pair<DataHandle, Vector4>>({
            /* There's nothing to receive a Move event afterwards */
        })), TestSuite::Compare::Container);
    }

    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
}

void AbstractUserInterfaceTest::eventPointerMoveRelativePositionWithPressRelease() {
    auto&& data = EventPointerMoveRelativePositionWithPressReleaseData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Compared to eventPointerMove(), verifies that combining with press and
       release events also updates the relative position appropriately, and
       does it even if the events aren't accepted */

    AbstractUserInterface ui;

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
        CORRADE_COMPARE(ui.pointerPressEvent({30.0f, 10.0f}, pressEvent), data.accept);

        PointerMoveEvent moveEvent1{{}, {}};
        CORRADE_COMPARE(ui.pointerMoveEvent({35.0f, 15.0f}, moveEvent1), data.accept);

        PointerEvent releaseEvent{Pointer::MouseMiddle};
        CORRADE_COMPARE(ui.pointerReleaseEvent({25.0f, 5.0f}, releaseEvent), data.accept);

        PointerMoveEvent moveEvent2{{}, {}};
        CORRADE_COMPARE(ui.pointerMoveEvent({30.0f, 10.0f}, moveEvent2), data.accept);

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
        CORRADE_COMPARE(ui.pointerMoveEvent({30.0f, 10.0f}, moveEvent), data.accept);

        PointerEvent pressEvent{Pointer::MouseMiddle};
        CORRADE_VERIFY(!ui.pointerPressEvent({10.0f, 10.0f}, pressEvent));

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Pair<Int, Vector4>>({
            {Move, {10.0f, 10.0f, 0.0f, 0.0f}},
            /* There's nothing to receive a Press event afterwards */
        })), TestSuite::Compare::Container);
    }
}

void AbstractUserInterfaceTest::eventCapture() {
    AbstractUserInterface ui;

    enum Event {
        Captured = 1,
        Press = 2,
        Release = 4,
        Move = 6
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
        CORRADE_VERIFY(ui.pointerPressEvent({30.0f, 10.0f}, eventPress));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), left);

        PointerEvent eventRelease{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({32.0f, 10.0f}, eventRelease));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, leftData, {10.0f, 10.0f}},
            {Release|Captured, leftData, {12.0f, 10.0f}},
        })), TestSuite::Compare::Container);

    /* Capture on the left node, release on the right one */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent eventPress{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({30.0f, 10.0f}, eventPress));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), left);

        PointerEvent eventRelease{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({50.0f, 10.0f}, eventRelease));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, leftData, {10.0f, 10.0f}},
            {Release|Captured, leftData, {30.0f, 10.0f}}, /* actually on rightData */
        })), TestSuite::Compare::Container);

    /* Capture on the right node, release on the left one */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent eventPress{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({50.0f, 10.0f}, eventPress));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), right);

        PointerEvent eventRelease{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({30.0f, 10.0f}, eventRelease));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, rightData, {10.0f, 10.0f}},
            {Release|Captured, rightData, {-10.0f, 10.0f}}, /* actually on leftData */
        })), TestSuite::Compare::Container);

    /* Moves are implicitly captured only if they happen between a press &
       release */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerMoveEvent eventMove1{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({50.0f, 15.0f}, eventMove1));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);

        PointerEvent eventPress{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({30.0f, 10.0f}, eventPress));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), left);

        PointerMoveEvent eventMove2{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({35.0f, 15.0f}, eventMove2));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), left);

        PointerEvent eventRelease{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({50.0f, 10.0f}, eventRelease));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);

        PointerMoveEvent eventMove3{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({55.0f, 15.0f}, eventMove3));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            /* A move that happens before a press isn't captured */
            {Move, rightData, {10.0f, 15.0f}},
            {Press|Captured, leftData, {10.0f, 10.0f}},
            /* A move that happens during a press is captured */
            {Move|Captured, leftData, {15.0f, 15.0f}},
            {Release|Captured, leftData, {30.0f, 10.0f}}, /* actually on rightData */
            /* A move that happens after a press isn't captured again */
            {Move, rightData, {15.0f, 15.0f}}
        })), TestSuite::Compare::Container);

    /* Capture on the right node, then capture again on the left one. In
       practice this can only happen if a release event is missed for some
       reason. */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent eventPress1{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({50.0f, 10.0f}, eventPress1));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), right);

        PointerEvent eventPress2{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({30.0f, 10.0f}, eventPress2));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), left);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, rightData, {10.0f, 10.0f}},
            {Press|Captured, leftData, {10.0f, 10.0f}},
        })), TestSuite::Compare::Container);
    }
}

void AbstractUserInterfaceTest::eventCaptureNotAccepted() {
    AbstractUserInterface ui;

    enum Event {
        Captured = 1,
        Press = 2,
        Release = 4,
        Move = 6
    };
    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doPointerPressEvent(UnsignedInt dataId, PointerEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Press|(event.isCaptured() ? Captured : 0), dataHandle(handle(), dataId, 1), event.position());
            if(accept)
                event.setAccepted();
        }
        void doPointerReleaseEvent(UnsignedInt dataId, PointerEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Release|(event.isCaptured() ? Captured : 0), dataHandle(handle(), dataId, 1), event.position());
            if(accept)
                event.setAccepted();
        }
        void doPointerMoveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Move|(event.isCaptured() ? Captured : 0), dataHandle(handle(), dataId, 1), event.position());
            if(accept)
                event.setAccepted();
        }

        bool accept = true;
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

    /* If the press event isn't accepted, no capture should happen, so the
       release happens on the actual node that is under */
    {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent eventPress{Pointer::MouseLeft};
        ui.layer<Layer>(layer).accept = false;
        CORRADE_VERIFY(!ui.pointerPressEvent({30.0f, 10.0f}, eventPress));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);

        PointerEvent eventRelease{Pointer::MouseLeft};
        ui.layer<Layer>(layer).accept = true;
        CORRADE_VERIFY(ui.pointerReleaseEvent({50.0f, 10.0f}, eventRelease));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, leftData, {10.0f, 10.0f}},
            /* The release event isn't happening on a captured node, so
               isCaptured() is false for it */
            {Release, rightData, {10.0f, 10.0f}},
        })), TestSuite::Compare::Container);

    /* Same, but move instead of release */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent eventPress{Pointer::MouseLeft};
        ui.layer<Layer>(layer).accept = false;
        CORRADE_VERIFY(!ui.pointerPressEvent({30.0f, 10.0f}, eventPress));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);

        PointerMoveEvent eventMove{{}, {}};
        ui.layer<Layer>(layer).accept = true;
        CORRADE_VERIFY(ui.pointerMoveEvent({50.0f, 10.0f}, eventMove));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, leftData, {10.0f, 10.0f}},
            /* The move event isn't happening on a captured node, so
               isCaptured() is false for it */
            {Move, rightData, {10.0f, 10.0f}},
        })), TestSuite::Compare::Container);

    /* If the release event isn't accepted, the capture should still get
       reset nevertheless */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent eventPress{Pointer::MouseLeft};
        ui.layer<Layer>(layer).accept = true;
        CORRADE_VERIFY(ui.pointerPressEvent({30.0f, 10.0f}, eventPress));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), left);

        PointerEvent eventRelease{Pointer::MouseLeft};
        ui.layer<Layer>(layer).accept = false;
        CORRADE_VERIFY(!ui.pointerReleaseEvent({50.0f, 10.0f}, eventRelease));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, leftData, {10.0f, 10.0f}},
            {Release|Captured, leftData, {30.0f, 10.0f}}, /* actually on rightData */
        })), TestSuite::Compare::Container);

    /* With move however, it should stay, even if it isn't accepted */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent eventPress{Pointer::MouseLeft};
        ui.layer<Layer>(layer).accept = true;
        CORRADE_VERIFY(ui.pointerPressEvent({30.0f, 10.0f}, eventPress));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), left);

        PointerMoveEvent eventMove{{}, {}};
        ui.layer<Layer>(layer).accept = false;
        CORRADE_VERIFY(!ui.pointerMoveEvent({50.0f, 10.0f}, eventMove));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), left);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, leftData, {10.0f, 10.0f}},
            {Move|Captured, leftData, {30.0f, 10.0f}}, /* actually on rightData */
        })), TestSuite::Compare::Container);
    }
}

void AbstractUserInterfaceTest::eventCaptureNotCaptured() {
    AbstractUserInterface ui;

    enum Event {
        Captured = 1,
        Press = 2,
        Release = 4,
        Move = 6
    };
    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doPointerPressEvent(UnsignedInt dataId, PointerEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Press|(event.isCaptured() ? Captured : 0), dataHandle(handle(), dataId, 1), event.position());
            if(!capture)
                event.setCaptured(false);
            event.setAccepted();
        }
        void doPointerReleaseEvent(UnsignedInt dataId, PointerEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Release|(event.isCaptured() ? Captured : 0), dataHandle(handle(), dataId, 1), event.position());
            if(!capture)
                event.setCaptured(false);
            event.setAccepted();
        }
        void doPointerMoveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Move|(event.isCaptured() ? Captured : 0), dataHandle(handle(), dataId, 1), event.position());
            if(capture != event.isCaptured())
                event.setCaptured(capture);
            event.setAccepted();
        }

        bool capture = true;
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
        CORRADE_VERIFY(ui.pointerPressEvent({30.0f, 10.0f}, eventPress));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);

        PointerEvent eventRelease{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({50.0f, 10.0f}, eventRelease));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);

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
        CORRADE_VERIFY(ui.pointerPressEvent({30.0f, 10.0f}, eventPress));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);

        PointerMoveEvent eventMove{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({50.0f, 10.0f}, eventMove));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, leftData, {10.0f, 10.0f}},
            /* The move event isn't happening on a captured node, so
               isCaptured() is false for it */
            {Move, rightData, {10.0f, 10.0f}},
        })), TestSuite::Compare::Container);

    /* If capture is disabled on release, it doesn't affect anything */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent eventPress{Pointer::MouseLeft};
        ui.layer<Layer>(layer).capture = true;
        CORRADE_VERIFY(ui.pointerPressEvent({30.0f, 10.0f}, eventPress));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), left);

        PointerEvent eventRelease{Pointer::MouseLeft};
        ui.layer<Layer>(layer).capture = false;
        CORRADE_VERIFY(ui.pointerReleaseEvent({50.0f, 10.0f}, eventRelease));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);

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
        CORRADE_VERIFY(ui.pointerPressEvent({30.0f, 10.0f}, eventPress));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), left);

        PointerMoveEvent eventMove1{{}, {}};
        ui.layer<Layer>(layer).capture = false;
        CORRADE_VERIFY(ui.pointerMoveEvent({50.0f, 10.0f}, eventMove1));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);

        PointerMoveEvent eventMove2{{}, {}};
        ui.layer<Layer>(layer).capture = true;
        CORRADE_VERIFY(ui.pointerMoveEvent({55.0f, 15.0f}, eventMove2));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), right);

        PointerMoveEvent eventMove3{{}, {}};
        ui.layer<Layer>(layer).capture = false;
        CORRADE_VERIFY(ui.pointerMoveEvent({30.0f, 10.0f}, eventMove3));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, leftData, {10.0f, 10.0f}},
            {Move|Captured, leftData, {30.0f, 10.0f}}, /* actually on rightData */
            {Move, rightData, {15.0f, 15.0f}},
            {Move|Captured, rightData, {-10.0f, 10.0f}} /* actually on leftData */
        })), TestSuite::Compare::Container);
    }
}

void AbstractUserInterfaceTest::eventCaptureNodePositionUpdated() {
    auto&& data = EventCaptureUpdateData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui;

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
    CORRADE_VERIFY(ui.pointerPressEvent({30.0f, 10.0f}, eventPress));
    CORRADE_COMPARE(ui.pointerEventCapturedNode(), nested);

    ui.setNodeOffset(node, {30.0f, 20.0f});
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeLayoutUpdate);

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    if(data.release) {
        PointerEvent eventRelease{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({32.0f, 10.0f}, eventRelease));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
    } else if(data.move) {
        PointerMoveEvent eventMove{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({32.0f, 10.0f}, eventMove));
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), nested);
    } else CORRADE_INTERNAL_ASSERT_UNREACHABLE();

    CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Pair<DataHandle, Vector2>>({
        {nestedData, {10.0f, 10.0f}},
        /* Should receive up-to-date position, not something relative to a
           position cached at the press */
        {nestedData, {12.0f - 10.0f, 10.0f - 20.0f}},
    })), TestSuite::Compare::Container);

    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
}

void AbstractUserInterfaceTest::eventCaptureNodeBecomesHidden() {
    auto&& data = EventCaptureNodeBecomesHiddenData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui;

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

    AbstractUserInterface ui;

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

    AbstractUserInterface ui;

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
