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
#include <Corrade/Containers/Iterable.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/Pair.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/StridedBitArrayView.h>
#include <Corrade/Containers/StringStl.h> /** @todo remove once Debug is stream-free */
#include <Corrade/Containers/Triple.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Container.h>
#include <Corrade/TestSuite/Compare/Numeric.h>
#include <Corrade/TestSuite/Compare/String.h>
#include <Corrade/Utility/DebugStl.h> /** @todo remove once Debug is stream-free */
#include <Corrade/Utility/Format.h>
#include <Magnum/Math/Time.h>
#include <Magnum/Math/Vector4.h>

#include "Magnum/Whee/AbstractAnimator.h"
#include "Magnum/Whee/AbstractLayer.h"
#include "Magnum/Whee/AbstractLayouter.h"
#include "Magnum/Whee/AbstractRenderer.h"
#include "Magnum/Whee/AbstractUserInterface.h"
#include "Magnum/Whee/Event.h"
#include "Magnum/Whee/Handle.h"
#include "Magnum/Whee/NodeFlags.h"

namespace Magnum { namespace Whee { namespace Test { namespace {

struct AbstractUserInterfaceTest: TestSuite::Tester {
    explicit AbstractUserInterfaceTest();

    void debugState();
    void debugStates();
    void debugStatesSupersets();

    void constructNoCreate();
    void construct();
    void constructSingleSize();
    void constructCopy();
    void constructMove();

    void renderer();
    void rendererSetInstanceInvalid();
    void rendererSetInstanceCompositeNotSupported();
    void rendererNotSet();

    void layer();
    void layerHandleRecycle();
    void layerHandleDisable();
    void layerHandleLastFree();
    void layerSetInstance();
    void layerCreateInvalid();
    void layerSetInstanceInvalid();
    void layerSetInstanceCompositeNotSupported();
    void layerGetInvalid();
    void layerRemoveInvalid();
    void layerNoHandlesLeft();

    void layouter();
    void layouterHandleRecycle();
    void layouterHandleDisable();
    void layouterHandleLastFree();
    void layouterSetInstance();
    void layouterCreateInvalid();
    void layouterSetInstanceInvalid();
    void layouterGetInvalid();
    void layouterRemoveInvalid();
    void layouterNoHandlesLeft();

    void animator();
    void animatorHandleRecycle();
    void animatorHandleDisable();
    void animatorHandleLastFree();
    void animatorSetInstance();
    void animatorSetInstanceInvalid();
    void animatorGetInvalid();
    void animatorRemoveInvalid();
    void animatorNoHandlesLeft();

    void node();
    void nodeHandleRecycle();
    void nodeHandleDisable();
    void nodeFlags();
    void nodeGetSetInvalid();
    void nodeCreateInvalid();
    void nodeRemoveInvalid();
    void nodeNoHandlesLeft();

    void nodeOrderRoot();
    void nodeOrderNested();
    void nodeOrderGetSetInvalid();

    void data();
    void dataAttach();
    void dataAttachInvalid();

    void layout();

    void animation();
    void animationAttachNode();
    void animationAttachNodeInvalid();
    void animationAttachNodeInvalidFeatures();
    void animationAttachData();
    void animationAttachDataInvalid();
    void animationAttachDataInvalidFeatures();

    void setSizeToLayers();
    void setSizeToLayouters();
    void setSizeToRenderer();
    void setSizeZero();
    void setSizeNotCalledBeforeUpdate();

    void cleanEmpty();
    void cleanNoOp();
    void cleanRemoveAttachedData();
    void cleanRemoveNestedNodes();
    void cleanRemoveNestedNodesAlreadyRemoved();
    void cleanRemoveNestedNodesAlreadyRemovedDangling();
    void cleanRemoveNestedNodesRecycledHandle();
    void cleanRemoveNestedNodesRecycledHandleOrphanedCycle();
    void cleanRemoveAll();

    void cleanRecycledLayerWithAnimators();

    void advanceAnimationsEmpty();
    void advanceAnimationsNoOp();
    void advanceAnimations();
    void advanceAnimationsGeneric();
    void advanceAnimationsNode();
    void advanceAnimationsData();
    void advanceAnimationsStyle();
    void advanceAnimationsInvalidTime();

    void updateOrder();
    void updateRecycledLayerWithoutInstance();

    /* Tests update() and clean() calls on AbstractLayer, AbstractLayouter and
       AbstractAnimator, and that the UserInterfaceState flags cause the right
       subset of these to be called. Does *not* verify the state update
       behavior consistency for events, that's done in event*() cases below. */
    void state();
    /* Tests update() and clean() calls triggered by advanceAnimations() */
    void stateAnimations();

    void statePropagateFromLayers();
    void statePropagateFromLayouters();
    void statePropagateFromAnimators();

    void draw();
    void drawComposite();
    void drawRendererTransitions();
    void drawEmpty();
    void drawNoRendererSet();

    void eventEmpty();
    void eventAlreadyAccepted();
    void eventNodePropagation();
    void eventEdges();

    void eventPointerPress();
    void eventPointerPressNotAccepted();

    void eventPointerRelease();

    void eventPointerMove();
    void eventPointerMoveRelativePositionWithPressRelease();
    void eventPointerMoveNotAccepted();
    void eventPointerMoveNodePositionUpdated();
    void eventPointerMoveNodeBecomesHiddenDisabledNoEvents();
    void eventPointerMoveNodeRemoved();
    void eventPointerMoveAllDataRemoved();

    void eventCapture();
    void eventCaptureEdges();
    void eventCaptureNotAccepted();
    void eventCaptureNotCaptured();
    void eventCaptureChangeCaptureInNotAcceptedEvent();
    void eventCaptureNodePositionUpdated();
    void eventCaptureNodeBecomesHiddenDisabledNoEvents();
    void eventCaptureNodeRemoved();
    void eventCaptureAllDataRemoved();

    void eventTapOrClick();
    void eventTapOrClickNodeBecomesHiddenDisabledNoEvents();
    void eventTapOrClickNodeRemoved();
    void eventTapOrClickAllDataRemoved();

    void eventFocus();
    void eventFocusNotAccepted();
    void eventFocusNodeHiddenDisabledNoEvents();
    void eventFocusNodeBecomesHiddenDisabledNoEventsNotFocusable();
    void eventFocusNodeRemoved();
    void eventFocusAllDataRemoved();
    void eventFocusInvalid();

    void eventFocusBlurByPointerPress();
    void eventFocusBlurByPointerPressNotAccepted();
    void eventFocusBlurByPointerPressNodeDisabledNoEvents();

    void eventKeyPressRelease();
    void eventTextInput();

    void eventConvertExternal();
};

using namespace Math::Literals;

const struct {
    const char* name;
    bool setSizeFirst;
} RendererData[]{
    {"size not set yet", false},
    {"size already set", true},
};

const struct {
    const char* name;
    bool layers, layouters, nodeAttachmentAnimators, dataAttachmentAnimators;
} CleanData[]{
    {"", false, false, false, false},
    {"layers", true, false, false, false},
    {"layouters", false, true, false, false},
    {"node attachment animators", false, false, true, false},
    /* Layers need to be enabled as well to have something to attach to */
    {"data attachment animators", true, false, false, true},
    {"all", true, true, true, true},
};

const struct {
    const char* name;
    AnimatorFeatures features;
} AdvanceAnimationsGenericData[]{
    {"", {}},
    {"node attachment", AnimatorFeature::NodeAttachment},
    {"data attachment", AnimatorFeature::DataAttachment},
};

const struct {
    const char* name;
    bool shuffle;
} UpdateOrderData[]{
    {"created in layer order", false},
    {"created in shuffled order", true}
};

const struct {
    const char* name;
    bool compositingLayer;
    bool layouters;
    bool nodeAttachmentAnimators;
    bool dataAttachmentAnimators;
    bool clean;
    bool noOp;
} StateData[]{
    {"",
        false, false, false, false, true, false},
    {"with no-op calls",
        false, false, false, false, true, true},
    {"with implicit clean",
        false, false, false, false, false, false},
    {"with implicit clean and no-op calls",
        false, false, false, false, false, true},
    {"with layouters",
        false, true, false, false, true, false},
    {"with layouters, with no-op calls",
        false, true, false, false, true, true},
    {"with layouters, with implicit clean",
        false, true, false, false, false, false},
    {"with layouters, with implicit clean and no-op calls",
        false, true, false, false, false, true},
    {"with node attachment animators",
        false, false, true, false, true, false},
    {"with node attachment animators, with no-op calls",
        false, false, true, false, true, true},
    {"with node attachment animators, with implicit clean",
        false, false, true, false, false, false},
    {"with node attachment animators, with implicit clean and no-op calls",
        false, false, true, false, false, true},
    {"with data attachment animators",
        false, false, false, true, true, false},
    {"with data attachment animators, with no-op calls",
        false, false, false, true, true, true},
    {"with data attachment animators, with implicit clean",
        false, false, false, true, false, false},
    {"with data attachment animators, with implicit clean and no-op calls",
        false, false, false, true, false, true},
    {"compositing layer",
        true, false, false, false, true, false},
    {"compositing layer, with layouters",
        true, true, false, false, true, false},
};

const struct {
    TestSuite::TestCaseDescriptionSourceLocation name;
    bool clean;
    bool noOp;
    bool runningAnimation;
    UserInterfaceStates expectedInitialState, expectedAdvanceState;
    Containers::Optional<NodeAnimations> nodeAnimations1, nodeAnimations2;
    bool dataAnimations, styleAnimations;
    Vector2 expectedNodeOffsetsAnimator2[2], expectedNodeOffsetsLayer[2];
    NodeFlags expectedNodeFlagsAnimator2[2];
    bool expectedNodesRemoveAnimator2[2], expectedNodesEnabledLayer[2];
    Containers::Array<Vector2> expectedClipRectOffsetsLayer;
    bool expectedNode1Valid, expectedNode2Valid;
    bool expectedCleanAfterAnimation;
    LayerStates expectedLayerUpdateState;
} StateAnimationsData[]{
    {"",
        true, false, false, {}, {},
        {}, {}, false, false,
        {{1.0f, 2.0f}, {3.0f, 4.0f}},
        {{1.0f, 2.0f}, {3.0f, 4.0f}},
        {NodeFlag::Clip, NodeFlag::Disabled},
        {false, false}, {true, false},
        {InPlaceInit, {{1.0f, 2.0f}, {0.0f, 0.0f}}},
        true, true, false, {}},
    {"with no-op calls",
        true, false, false, {}, {},
        {}, {}, false, false,
        {{1.0f, 2.0f}, {3.0f, 4.0f}},
        {{1.0f, 2.0f}, {3.0f, 4.0f}},
        {NodeFlag::Clip, NodeFlag::Disabled},
        {false, false}, {true, false},
        {InPlaceInit, {{1.0f, 2.0f}, {0.0f, 0.0f}}},
        true, true, false, {}},
    {"with implicit clean",
        false, false, false, {}, {},
        {}, {}, false, false,
        {{1.0f, 2.0f}, {3.0f, 4.0f}},
        {{1.0f, 2.0f}, {3.0f, 4.0f}},
        {NodeFlag::Clip, NodeFlag::Disabled},
        {false, false}, {true, false},
        {InPlaceInit, {{1.0f, 2.0f}, {0.0f, 0.0f}}},
        true, true, false, {}},
    {"with implicit clean and no-op calls",
        false, true, false, {}, {},
        {}, {}, false, false,
        {{1.0f, 2.0f}, {3.0f, 4.0f}},
        {{1.0f, 2.0f}, {3.0f, 4.0f}},
        {NodeFlag::Clip, NodeFlag::Disabled},
        {false, false}, {true, false},
        {InPlaceInit, {{1.0f, 2.0f}, {0.0f, 0.0f}}},
        true, true, false, {}},
    {"running animation",
        true, false, true, UserInterfaceState::NeedsAnimationAdvance, UserInterfaceState::NeedsAnimationAdvance,
        {}, {}, false, false,
        {{1.0f, 2.0f}, {3.0f, 4.0f}},
        {{1.0f, 2.0f}, {3.0f, 4.0f}},
        {NodeFlag::Clip, NodeFlag::Disabled},
        {false, false}, {true, false},
        {InPlaceInit, {{1.0f, 2.0f}, {0.0f, 0.0f}}},
        true, true, false, {}},
    {"running animation, with no-op calls",
        true, false, true, UserInterfaceState::NeedsAnimationAdvance, UserInterfaceState::NeedsAnimationAdvance,
        {}, {}, false, false,
        {{1.0f, 2.0f}, {3.0f, 4.0f}},
        {{1.0f, 2.0f}, {3.0f, 4.0f}},
        {NodeFlag::Clip, NodeFlag::Disabled},
        {false, false}, {true, false},
        {InPlaceInit, {{1.0f, 2.0f}, {0.0f, 0.0f}}},
        true, true, false, {}},
    {"running animation, with implicit clean",
        false, false, true, UserInterfaceState::NeedsAnimationAdvance, UserInterfaceState::NeedsAnimationAdvance,
        {}, {}, false, false,
        {{1.0f, 2.0f}, {3.0f, 4.0f}},
        {{1.0f, 2.0f}, {3.0f, 4.0f}},
        {NodeFlag::Clip, NodeFlag::Disabled},
        {false, false}, {true, false},
        {InPlaceInit, {{1.0f, 2.0f}, {0.0f, 0.0f}}},
        true, true, false, {}},
    {"running animation, with implicit clean and no-op calls",
        false, true, true, UserInterfaceState::NeedsAnimationAdvance, UserInterfaceState::NeedsAnimationAdvance,
        {}, {}, false, false,
        {{1.0f, 2.0f}, {3.0f, 4.0f}},
        {{1.0f, 2.0f}, {3.0f, 4.0f}},
        {NodeFlag::Clip, NodeFlag::Disabled},
        {false, false}, {true, false},
        {InPlaceInit, {{1.0f, 2.0f}, {0.0f, 0.0f}}},
        true, true, false, {}},
    {"two node animators doing nothing",
        true, false, false, {}, {},
        NodeAnimations{}, NodeAnimations{}, false, false,
        {{1.0f, 2.0f}, {3.0f, 4.0f}},
        {{1.0f, 2.0f}, {3.0f, 4.0f}},
        {NodeFlag::Clip, NodeFlag::Disabled},
        {false, false}, {true, false},
        {InPlaceInit, {{1.0f, 2.0f}, {0.0f, 0.0f}}},
        true, true, false, {}},
    {"first node animator moving nodes",
        true, false, false, {}, UserInterfaceState::NeedsLayoutUpdate,
        NodeAnimations{NodeAnimation::OffsetSize}, NodeAnimations{}, false, false,
        {{1.0f, 2.0f}, {2.0f, 3.0f}},
        {{1.0f, 2.0f}, {2.0f, 3.0f}},
        {NodeFlag::Clip, NodeFlag::Disabled},
        {false, false}, {true, false},
        {InPlaceInit, {{1.0f, 2.0f}, {0.0f, 0.0f}}},
        true, true, false,
        LayerState::NeedsNodeOrderUpdate|LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsNodeOffsetSizeUpdate},
    {"second node animator moving nodes",
        true, false, false, {}, UserInterfaceState::NeedsLayoutUpdate,
        NodeAnimations{}, NodeAnimations{NodeAnimation::OffsetSize}, false, false,
        {{1.0f, 2.0f}, {3.0f, 4.0f}},
        {{0.0f, 1.0f}, {3.0f, 4.0f}},
        {NodeFlag::Clip, NodeFlag::Disabled},
        {false, false}, {true, false},
        {InPlaceInit, {{0.0f, 1.0f}, {0.0f, 0.0f}}},
        true, true, false,
        LayerState::NeedsNodeOrderUpdate|LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsNodeOffsetSizeUpdate},
    {"first node animator changing node enablement, second doing nothing",
        true, false, false, {}, UserInterfaceState::NeedsNodeEnabledUpdate,
        NodeAnimations{NodeAnimation::Enabled}, NodeAnimations{}, false, false,
        {{1.0f, 2.0f}, {3.0f, 4.0f}},
        {{1.0f, 2.0f}, {3.0f, 4.0f}},
        {NodeFlag::Clip, {}},
        {false, false}, {true, true},
        {InPlaceInit, {{1.0f, 2.0f}, {0.0f, 0.0f}}},
        true, true, false,
        /* The draw order needs an update as well due to the coarseness of
           UserInterfaceState bits */
        LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsNodeOrderUpdate},
    {"node animator changing node clip state",
        true, false, false, {}, UserInterfaceState::NeedsNodeClipUpdate,
        NodeAnimations{NodeAnimation::Clip}, {}, false, false,
        {{1.0f, 2.0f}, {3.0f, 4.0f}},
        {{1.0f, 2.0f}, {3.0f, 4.0f}},
        {NodeFlag::Clip, NodeFlag::Disabled},
        {false, false}, {true, false},
        {InPlaceInit, {{1.0f, 2.0f}, {3.0f, 4.0f}}},
        true, true, false,
        LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsNodeOrderUpdate},
    {"node animator removing a node",
        true, false, false, {}, UserInterfaceState::NeedsNodeClean,
        NodeAnimations{NodeAnimation::Removal}, {}, false, false,
        {{1.0f, 2.0f}, {3.0f, 4.0f}},
        {{1.0f, 2.0f}, {3.0f, 4.0f}},
        {NodeFlag::Clip, NodeFlag::Disabled},
        {false, true}, {true, false},
        {InPlaceInit, {{1.0f, 2.0f}}},
        true, false, true,
        LayerState::NeedsNodeOrderUpdate|LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsNodeOffsetSizeUpdate},
    {"node animator removing a node, with implicit clean",
        false, false, false, {}, UserInterfaceState::NeedsNodeClean,
        NodeAnimations{NodeAnimation::Removal}, {}, false, false,
        {{1.0f, 2.0f}, {3.0f, 4.0f}},
        {{1.0f, 2.0f}, {3.0f, 4.0f}},
        {NodeFlag::Clip, NodeFlag::Disabled},
        {false, true}, {true, false},
        {InPlaceInit, {{1.0f, 2.0f}}},
        true, false, true,
        LayerState::NeedsNodeOrderUpdate|LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsNodeOffsetSizeUpdate},
    {"two node animators doing parts of everything",
        true, false, false, {}, UserInterfaceState::NeedsNodeClean,
        NodeAnimation::OffsetSize|NodeAnimation::Enabled|NodeAnimation::Clip, NodeAnimations{NodeAnimation::Removal}, false, false,
        {{1.0f, 2.0f}, {2.0f, 3.0f}},
        {{1.0f, 2.0f}, {2.0f, 3.0f}},
        {NodeFlag::Clip, NodeFlag::Clip},
        {false, false}, {false, true},
        {InPlaceInit, {{2.0f, 3.0f}}},
        false, true, true,
        LayerState::NeedsNodeOrderUpdate|LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsNodeOffsetSizeUpdate},
    {"two node animators doing parts of everything, the other way around",
        true, false, false, {}, UserInterfaceState::NeedsNodeClean,
        NodeAnimations{NodeAnimation::Removal},
        NodeAnimation::OffsetSize|NodeAnimation::Enabled|NodeAnimation::Clip, false, false,
        {{1.0f, 2.0f}, {3.0f, 4.0f}},
        {{0.0f, 1.0f}, {3.0f, 4.0f}},
        {NodeFlag::Clip, NodeFlag::Disabled},
        {false, true}, {false, false},
        {InPlaceInit, {{0.0f, 0.0f}}},
        true, false, true,
        LayerState::NeedsNodeOrderUpdate|LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsNodeOffsetSizeUpdate},
    {"data animator",
        true, false, false, {}, UserInterfaceState::NeedsDataUpdate,
        {}, {}, true, false,
        {{1.0f, 2.0f}, {3.0f, 4.0f}},
        {{1.0f, 2.0f}, {3.0f, 4.0f}},
        {NodeFlag::Clip, NodeFlag::Disabled},
        {false, false}, {true, false},
        {InPlaceInit, {{1.0f, 2.0f}, {0.0f, 0.0f}}},
        true, true, false,
        LayerState::NeedsDataUpdate},
    {"data animator, with implicit clean",
        false, false, false, {}, UserInterfaceState::NeedsDataUpdate,
        {}, {}, true, false,
        {{1.0f, 2.0f}, {3.0f, 4.0f}},
        {{1.0f, 2.0f}, {3.0f, 4.0f}},
        {NodeFlag::Clip, NodeFlag::Disabled},
        {false, false}, {true, false},
        {InPlaceInit, {{1.0f, 2.0f}, {0.0f, 0.0f}}},
        true, true, false,
        LayerState::NeedsDataUpdate},
    {"data animator, with implicit clean and no-op calls",
        false, true, false, {}, UserInterfaceState::NeedsDataUpdate,
        {}, {}, true, false,
        {{1.0f, 2.0f}, {3.0f, 4.0f}},
        {{1.0f, 2.0f}, {3.0f, 4.0f}},
        {NodeFlag::Clip, NodeFlag::Disabled},
        {false, false}, {true, false},
        {InPlaceInit, {{1.0f, 2.0f}, {0.0f, 0.0f}}},
        true, true, false,
        LayerState::NeedsDataUpdate},
    {"style animator",
        true, false, false, {}, UserInterfaceState::NeedsDataUpdate,
        {}, {}, false, true,
        {{1.0f, 2.0f}, {3.0f, 4.0f}},
        {{1.0f, 2.0f}, {3.0f, 4.0f}},
        {NodeFlag::Clip, NodeFlag::Disabled},
        {false, false}, {true, false},
        {InPlaceInit, {{1.0f, 2.0f}, {0.0f, 0.0f}}},
        true, true, false,
        LayerState::NeedsCommonDataUpdate},
    {"style animator, with implicit clean",
        false, false, false, {}, UserInterfaceState::NeedsDataUpdate,
        {}, {}, false, true,
        {{1.0f, 2.0f}, {3.0f, 4.0f}},
        {{1.0f, 2.0f}, {3.0f, 4.0f}},
        {NodeFlag::Clip, NodeFlag::Disabled},
        {false, false}, {true, false},
        {InPlaceInit, {{1.0f, 2.0f}, {0.0f, 0.0f}}},
        true, true, false,
        LayerState::NeedsCommonDataUpdate},
    {"style animator, with implicit clean and no-op calls",
        false, true, false, {}, UserInterfaceState::NeedsDataUpdate,
        {}, {}, false, true,
        {{1.0f, 2.0f}, {3.0f, 4.0f}},
        {{1.0f, 2.0f}, {3.0f, 4.0f}},
        {NodeFlag::Clip, NodeFlag::Disabled},
        {false, false}, {true, false},
        {InPlaceInit, {{1.0f, 2.0f}, {0.0f, 0.0f}}},
        true, true, false,
        LayerState::NeedsCommonDataUpdate},
};

const struct {
    const char* name;
    LayerStates state;
} StatePropagateFromLayersData[]{
    {"needs data update", LayerState::NeedsDataUpdate},
    {"needs common data update", LayerState::NeedsCommonDataUpdate},
    {"needs shared data update", LayerState::NeedsSharedDataUpdate},
    {"all", LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate|LayerState::NeedsSharedDataUpdate},
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
    bool layer1, layer2, node;
    bool hide, attach, clean, update;
} DrawEmptyData[]{
    {"nothing, clean + update before",
        false, false, false,
        false, false, true, true},
    {"nothing, clean before",
        false, false, false,
        false, false, true, false},
    {"nothing, update before",
        false, false, false,
        false, false, false, true},
    {"nothing",
        false, false, false,
        false, false, false, false},

    {"no node, one layer with update needed",
        true, false, false,
        false, false, false, false},
    {"no node, two layers with update needed",
        true, true, false,
        false, false, false, false},
    {"no node, two layers with update needed, clean + update before",
        true, true, false,
        false, false, true, true},
    {"no node, two layers with update needed, clean before",
        true, true, false,
        false, false, true, false},
    {"no node, two layers with update needed, update before",
        true, true, false,
        false, false, false, true},

    {"node but no data attachment, one layer with update needed",
        true, false, true,
        false, false, false, false},
    {"node but no data attachment, two layers with update needed",
        true, true, true,
        false, false, false, false},
    {"node but no data attachment, two layers with update needed, clean + update before",
        true, true, true,
        false, false, true, true},
    {"node but no data attachment, two layers with update needed, clean before",
        true, true, true,
        false, false, true, false},
    {"node but no data attachment, two layers with update needed, update before",
        true, true, true,
        false, false, false, true},

    {"node not in top-level order, no layers",
        false, false, true,
        false, true, false, false},
    {"node not in top-level order, one layer",
        true, false, true,
        false, true, false, false},
    {"node not in top-level order, two layers",
        true, true, true,
        false, true, false, false},
    {"node not in top-level order, two layers, clean + update before",
        true, true, true,
        false, true, true, true},
    {"node not in top-level order, two layers, clean before",
        true, true, true,
        false, true, true, false},
    {"node not in top-level order, two layers, update before",
        true, true, true,
        false, true, false, true},

    {"node hidden, no layers",
        false, false, true,
        true, true, false, false},
    {"node hidden, one layer",
        true, false, true,
        true, true, false, false},
    {"node hidden, two layers",
        true, true, true,
        true, true, false, false},
    {"node hidden, two layers, clean + update before",
        true, true, true,
        true, true, true, true},
    {"node hidden, two layers, clean before",
        true, true, true,
        true, true, true, false},
    {"node hidden, two layers, update before",
        true, true, true,
        true, true, false, false},
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
    bool clean;
    bool update;
    bool layouter;
} EventNodePropagationData[]{
    {"clean + update before", true, true, false},
    {"clean before", true, false, false},
    {"update before", false, true, false},
    {"", false, false, false},
    {"layouter, clean + update before", true, true, true},
    {"layouter, clean before", true, false, true},
    {"layouter, update before", false, true, true},
    {"layouter", false, false, true},
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
    UserInterfaceState expectedState;
} EventPointerNodeBecomesHiddenDisabledNoEventsData[]{
    {"removed from top level order, update before", {}, true,
        true, UserInterfaceState::NeedsNodeUpdate},
    {"removed from top level order", {}, true,
        false, UserInterfaceState::NeedsNodeUpdate},
    {"hidden, update before", NodeFlag::Hidden, false,
        true, UserInterfaceState::NeedsNodeUpdate},
    {"hidden", NodeFlag::Hidden, false,
        false, UserInterfaceState::NeedsNodeUpdate},
    {"no events, update before", NodeFlag::NoEvents, false,
        true, UserInterfaceState::NeedsNodeEnabledUpdate},
    {"no events", NodeFlag::NoEvents, false,
        false, UserInterfaceState::NeedsNodeEnabledUpdate},
    {"disabled, update before", NodeFlag::Disabled, false,
        true, UserInterfaceState::NeedsNodeEnabledUpdate},
    {"disabled", NodeFlag::Disabled, false,
        false, UserInterfaceState::NeedsNodeEnabledUpdate},
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
    bool layouter;
} EventLayouterData[]{
    {"", false},
    {"layouter", true},
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

const struct {
    const char* name;
    NodeFlags flags;
    bool clearOrder;
    bool release;
    bool move;
    bool update;
    UserInterfaceState expectedState;
} EventCaptureNodeBecomesHiddenDisabledNoEventsData[]{
    {"removed from top level order, release, update before", {}, true,
        true, false, true, UserInterfaceState::NeedsNodeUpdate},
    {"removed from top level order, release", {}, true,
        true, false, false, UserInterfaceState::NeedsNodeUpdate},
    {"removed from top level order, move, update before", {}, true,
        false, true, true, UserInterfaceState::NeedsNodeUpdate},
    {"removed from top level order, move", {}, true,
        false, true, false, UserInterfaceState::NeedsNodeUpdate},
    {"hidden, release, update before", NodeFlag::Hidden, false,
        true, false, true, UserInterfaceState::NeedsNodeUpdate},
    {"hidden, release", NodeFlag::Hidden, false,
        true, false, false, UserInterfaceState::NeedsNodeUpdate},
    {"hidden, move, update before", NodeFlag::Hidden, false,
        false, true, true, UserInterfaceState::NeedsNodeUpdate},
    {"hidden, move", NodeFlag::Hidden, false,
        false, true, false, UserInterfaceState::NeedsNodeUpdate},
    {"no events, release, update before", NodeFlag::NoEvents, false,
        true, false, true, UserInterfaceState::NeedsNodeEnabledUpdate},
    {"no events, release", NodeFlag::NoEvents, false,
        true, false, false, UserInterfaceState::NeedsNodeEnabledUpdate},
    {"no events, move, update before", NodeFlag::NoEvents, false,
        false, true, true, UserInterfaceState::NeedsNodeEnabledUpdate},
    {"no events, move", NodeFlag::NoEvents, false,
        false, true, false, UserInterfaceState::NeedsNodeEnabledUpdate},
    {"disabled, release, update before", NodeFlag::Disabled, false,
        true, false, true, UserInterfaceState::NeedsNodeEnabledUpdate},
    {"disabled, release", NodeFlag::Disabled, false,
        true, false, false, UserInterfaceState::NeedsNodeEnabledUpdate},
    {"disabled, move, update before", NodeFlag::Disabled, false,
        false, true, true, UserInterfaceState::NeedsNodeEnabledUpdate},
    {"disabled, move", NodeFlag::Disabled, false,
        false, true, false, UserInterfaceState::NeedsNodeEnabledUpdate},
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
    bool update;
    bool layouter;
} EventLayouterUpdateData[]{
    {"update before", true, false},
    {"", false, false},
    {"layouter, update before", true, true},
    {"layouter", false, true},
};

const struct {
    const char* name;
    NodeFlags flags;
    bool clearOrder;
    bool update, previousFocused;
    UserInterfaceState expectedState;
} EventFocusNodeHiddenDisabledNoEventsData[]{
    {"removed from top level order, update before", {}, true,
        true, false, UserInterfaceState::NeedsNodeUpdate},
    {"removed from top level order,", {}, true,
        false, false, UserInterfaceState::NeedsNodeUpdate},
    {"removed from top level order, previous focused node", {}, true,
        false, true, UserInterfaceState::NeedsNodeUpdate},
    {"hidden, update before", NodeFlag::Hidden, false,
        true, false, UserInterfaceState::NeedsNodeUpdate},
    {"hidden", NodeFlag::Hidden, false,
        false, false, UserInterfaceState::NeedsNodeUpdate},
    {"hidden, previous focused node", NodeFlag::Hidden, false,
        false, true, UserInterfaceState::NeedsNodeUpdate},
    {"no events, update before", NodeFlag::NoEvents, false,
        true, false, UserInterfaceState::NeedsNodeEnabledUpdate},
    {"no events", NodeFlag::NoEvents, false,
        false, false, UserInterfaceState::NeedsNodeEnabledUpdate},
    {"no events, previous focused node", NodeFlag::NoEvents, false,
        false, true, UserInterfaceState::NeedsNodeEnabledUpdate},
    {"disabled, update before", NodeFlag::Disabled, false,
        true, false, UserInterfaceState::NeedsNodeEnabledUpdate},
    {"disabled", NodeFlag::Disabled, false,
        false, false, UserInterfaceState::NeedsNodeEnabledUpdate},
    {"disabled, previous focused node", NodeFlag::Disabled, false,
        false, true, UserInterfaceState::NeedsNodeEnabledUpdate},
};

const struct {
    const char* name;
    NodeFlags addFlags;
    NodeFlags clearFlags;
    bool clearOrder;
    bool update;
    bool pressed, hovered;
    UserInterfaceState expectedState;
    bool expectPressedHoveringBlur;
} EventFocusNodeBecomesHiddenDisabledNoEventsNotFocusableData[]{
    {"removed from top level order, update before", {}, {}, true,
        true, true, true, UserInterfaceState::NeedsNodeUpdate, false},
    {"removed from top level order", {}, {}, true,
        false, true, true, UserInterfaceState::NeedsNodeUpdate, false},
    {"hidden, update before", NodeFlag::Hidden, {}, false,
        true, true, true, UserInterfaceState::NeedsNodeUpdate, false},
    {"hidden", NodeFlag::Hidden, {}, false,
        false, true, true, UserInterfaceState::NeedsNodeUpdate, false},
    {"no events, update before", NodeFlag::NoEvents, {}, false,
        true, true, true, UserInterfaceState::NeedsNodeEnabledUpdate, false},
    {"no events", NodeFlag::NoEvents, {}, false,
        false, true, true, UserInterfaceState::NeedsNodeEnabledUpdate, false},
    {"disabled, update before", NodeFlag::Disabled, {}, false,
        true, true, true, UserInterfaceState::NeedsNodeEnabledUpdate, false},
    {"disabled", NodeFlag::Disabled, {}, false,
        false, true, true, UserInterfaceState::NeedsNodeEnabledUpdate, false},
    {"not focusable, pressed, update before", {}, NodeFlag::Focusable, false,
        true, true, false, UserInterfaceState::NeedsNodeEnabledUpdate, true},
    {"not focusable, pressed", {}, NodeFlag::Focusable, false,
        false, true, false, UserInterfaceState::NeedsNodeEnabledUpdate, true},
    {"not focusable, hovered, update before", {}, NodeFlag::Focusable, false,
        true, false, true, UserInterfaceState::NeedsNodeEnabledUpdate, true},
    {"not focusable, hovered", {}, NodeFlag::Focusable, false,
        false, false, true, UserInterfaceState::NeedsNodeEnabledUpdate, true},
    {"not focusable, pressed & hovered, update before", {}, NodeFlag::Focusable, false,
        true, true, true, UserInterfaceState::NeedsNodeEnabledUpdate, true},
    {"not focusable, pressed & hovered", {}, NodeFlag::Focusable, false,
        false, true, true, UserInterfaceState::NeedsNodeEnabledUpdate, true},
};

const struct {
    const char* name;
    NodeFlag flag;
    bool previousFocused;
    UserInterfaceState expectedState;
} EventFocusBlurByPointerPressNodeDisabledNoEventsData[]{
    {"no events", NodeFlag::NoEvents, false,
        UserInterfaceState::NeedsNodeEnabledUpdate},
    {"no events, previous focused node", NodeFlag::NoEvents, true,
        UserInterfaceState::NeedsNodeEnabledUpdate},
    {"disabled", NodeFlag::Disabled, false,
        UserInterfaceState::NeedsNodeEnabledUpdate},
    {"disabled, previous focused node", NodeFlag::Disabled, false,
        UserInterfaceState::NeedsNodeEnabledUpdate},
};

AbstractUserInterfaceTest::AbstractUserInterfaceTest() {
    addTests({&AbstractUserInterfaceTest::debugState,
              &AbstractUserInterfaceTest::debugStates,
              &AbstractUserInterfaceTest::debugStatesSupersets,

              &AbstractUserInterfaceTest::constructNoCreate,
              &AbstractUserInterfaceTest::construct,
              &AbstractUserInterfaceTest::constructSingleSize,
              &AbstractUserInterfaceTest::constructCopy,
              &AbstractUserInterfaceTest::constructMove});

    addInstancedTests({&AbstractUserInterfaceTest::renderer},
        Containers::arraySize(RendererData));

    addTests({&AbstractUserInterfaceTest::rendererSetInstanceInvalid,
              &AbstractUserInterfaceTest::rendererSetInstanceCompositeNotSupported,
              &AbstractUserInterfaceTest::rendererNotSet,

              &AbstractUserInterfaceTest::layer,
              &AbstractUserInterfaceTest::layerHandleRecycle,
              &AbstractUserInterfaceTest::layerHandleDisable,
              &AbstractUserInterfaceTest::layerHandleLastFree,
              &AbstractUserInterfaceTest::layerSetInstance,
              &AbstractUserInterfaceTest::layerCreateInvalid,
              &AbstractUserInterfaceTest::layerSetInstanceInvalid,
              &AbstractUserInterfaceTest::layerSetInstanceCompositeNotSupported,
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

              &AbstractUserInterfaceTest::layouter,
              &AbstractUserInterfaceTest::layouterHandleRecycle,
              &AbstractUserInterfaceTest::layouterHandleDisable,
              &AbstractUserInterfaceTest::layouterHandleLastFree,
              &AbstractUserInterfaceTest::layouterSetInstance,
              &AbstractUserInterfaceTest::layouterCreateInvalid,
              &AbstractUserInterfaceTest::layouterSetInstanceInvalid,
              &AbstractUserInterfaceTest::layouterGetInvalid,
              &AbstractUserInterfaceTest::layouterRemoveInvalid,
              &AbstractUserInterfaceTest::layouterNoHandlesLeft,

              &AbstractUserInterfaceTest::animator,
              &AbstractUserInterfaceTest::animatorHandleRecycle,
              &AbstractUserInterfaceTest::animatorHandleDisable,
              &AbstractUserInterfaceTest::animatorHandleLastFree,
              &AbstractUserInterfaceTest::animatorSetInstance,
              &AbstractUserInterfaceTest::animatorSetInstanceInvalid,
              &AbstractUserInterfaceTest::animatorGetInvalid,
              &AbstractUserInterfaceTest::animatorRemoveInvalid,
              &AbstractUserInterfaceTest::animatorNoHandlesLeft,

              &AbstractUserInterfaceTest::nodeOrderRoot,
              &AbstractUserInterfaceTest::nodeOrderNested,
              &AbstractUserInterfaceTest::nodeOrderGetSetInvalid,

              &AbstractUserInterfaceTest::data,
              &AbstractUserInterfaceTest::dataAttach,
              &AbstractUserInterfaceTest::dataAttachInvalid,

              &AbstractUserInterfaceTest::layout,

              &AbstractUserInterfaceTest::animation,
              &AbstractUserInterfaceTest::animationAttachNode,
              &AbstractUserInterfaceTest::animationAttachNodeInvalid,
              &AbstractUserInterfaceTest::animationAttachNodeInvalidFeatures,
              &AbstractUserInterfaceTest::animationAttachData,
              &AbstractUserInterfaceTest::animationAttachDataInvalid,
              &AbstractUserInterfaceTest::animationAttachDataInvalidFeatures,

              &AbstractUserInterfaceTest::setSizeToLayers,
              &AbstractUserInterfaceTest::setSizeToLayouters,
              &AbstractUserInterfaceTest::setSizeToRenderer,
              &AbstractUserInterfaceTest::setSizeZero,
              &AbstractUserInterfaceTest::setSizeNotCalledBeforeUpdate,

              &AbstractUserInterfaceTest::cleanEmpty});

    addInstancedTests({&AbstractUserInterfaceTest::cleanNoOp,
                       &AbstractUserInterfaceTest::cleanRemoveAttachedData,
                       &AbstractUserInterfaceTest::cleanRemoveNestedNodes},
        Containers::arraySize(CleanData));

    addTests({&AbstractUserInterfaceTest::cleanRemoveNestedNodesAlreadyRemoved,
              &AbstractUserInterfaceTest::cleanRemoveNestedNodesAlreadyRemovedDangling});

    addInstancedTests({&AbstractUserInterfaceTest::cleanRemoveNestedNodesRecycledHandle,
                       &AbstractUserInterfaceTest::cleanRemoveNestedNodesRecycledHandleOrphanedCycle,
                       &AbstractUserInterfaceTest::cleanRemoveAll},
        Containers::arraySize(CleanData));

    addTests({&AbstractUserInterfaceTest::cleanRecycledLayerWithAnimators,

              &AbstractUserInterfaceTest::advanceAnimationsEmpty,
              &AbstractUserInterfaceTest::advanceAnimationsNoOp,
              &AbstractUserInterfaceTest::advanceAnimations});

    addInstancedTests({&AbstractUserInterfaceTest::advanceAnimationsGeneric},
        Containers::arraySize(AdvanceAnimationsGenericData));

    addTests({&AbstractUserInterfaceTest::advanceAnimationsNode,
              &AbstractUserInterfaceTest::advanceAnimationsData,
              &AbstractUserInterfaceTest::advanceAnimationsStyle,
              &AbstractUserInterfaceTest::advanceAnimationsInvalidTime});

    addInstancedTests({&AbstractUserInterfaceTest::updateOrder},
        Containers::arraySize(UpdateOrderData));

    addTests({&AbstractUserInterfaceTest::updateRecycledLayerWithoutInstance});

    addInstancedTests({&AbstractUserInterfaceTest::state},
        Containers::arraySize(StateData));

    addInstancedTests({&AbstractUserInterfaceTest::stateAnimations},
        Containers::arraySize(StateAnimationsData));

    addInstancedTests({&AbstractUserInterfaceTest::statePropagateFromLayers},
        Containers::arraySize(StatePropagateFromLayersData));

    addTests({&AbstractUserInterfaceTest::statePropagateFromLayouters,
              &AbstractUserInterfaceTest::statePropagateFromAnimators});

    addInstancedTests({&AbstractUserInterfaceTest::draw},
        Containers::arraySize(DrawData));

    addTests({&AbstractUserInterfaceTest::drawComposite,
              &AbstractUserInterfaceTest::drawRendererTransitions});

    addInstancedTests({&AbstractUserInterfaceTest::drawEmpty},
        Containers::arraySize(DrawEmptyData));

    addTests({&AbstractUserInterfaceTest::drawNoRendererSet});

    addInstancedTests({&AbstractUserInterfaceTest::eventEmpty},
        Containers::arraySize(CleanUpdateData));

    addTests({&AbstractUserInterfaceTest::eventAlreadyAccepted});

    addInstancedTests({&AbstractUserInterfaceTest::eventNodePropagation},
        Containers::arraySize(EventNodePropagationData));

    addTests({&AbstractUserInterfaceTest::eventEdges});

    addInstancedTests({&AbstractUserInterfaceTest::eventPointerPress},
        Containers::arraySize(UpdateData));

    addTests({&AbstractUserInterfaceTest::eventPointerPressNotAccepted});

    addInstancedTests({&AbstractUserInterfaceTest::eventPointerRelease,

                       &AbstractUserInterfaceTest::eventPointerMove},
        Containers::arraySize(UpdateData));

    addInstancedTests({&AbstractUserInterfaceTest::eventPointerMoveRelativePositionWithPressRelease},
        Containers::arraySize(EventPointerMoveRelativePositionWithPressReleaseData));

    addTests({&AbstractUserInterfaceTest::eventPointerMoveNotAccepted});

    addInstancedTests({&AbstractUserInterfaceTest::eventPointerMoveNodePositionUpdated},
        Containers::arraySize(UpdateData));

    addInstancedTests({&AbstractUserInterfaceTest::eventPointerMoveNodeBecomesHiddenDisabledNoEvents},
        Containers::arraySize(EventPointerNodeBecomesHiddenDisabledNoEventsData));

    addInstancedTests({&AbstractUserInterfaceTest::eventPointerMoveNodeRemoved},
        Containers::arraySize(EventNodeRemovedData));

    addInstancedTests({&AbstractUserInterfaceTest::eventPointerMoveAllDataRemoved},
        Containers::arraySize(CleanUpdateData));

    addInstancedTests({&AbstractUserInterfaceTest::eventCapture},
        Containers::arraySize(EventLayouterData));

    addTests({&AbstractUserInterfaceTest::eventCaptureEdges,
              &AbstractUserInterfaceTest::eventCaptureNotAccepted,
              &AbstractUserInterfaceTest::eventCaptureNotCaptured,
              &AbstractUserInterfaceTest::eventCaptureChangeCaptureInNotAcceptedEvent});

    addInstancedTests({&AbstractUserInterfaceTest::eventCaptureNodePositionUpdated},
        Containers::arraySize(EventCaptureUpdateData));

    addInstancedTests({&AbstractUserInterfaceTest::eventCaptureNodeBecomesHiddenDisabledNoEvents},
        Containers::arraySize(EventCaptureNodeBecomesHiddenDisabledNoEventsData));

    addInstancedTests({&AbstractUserInterfaceTest::eventCaptureNodeRemoved},
        Containers::arraySize(EventCaptureNodeRemovedData));

    addInstancedTests({&AbstractUserInterfaceTest::eventCaptureAllDataRemoved},
        Containers::arraySize(EventCaptureCleanUpdateData));

    addInstancedTests({&AbstractUserInterfaceTest::eventTapOrClick},
        Containers::arraySize(EventLayouterData));

    addInstancedTests({&AbstractUserInterfaceTest::eventTapOrClickNodeBecomesHiddenDisabledNoEvents},
        Containers::arraySize(EventPointerNodeBecomesHiddenDisabledNoEventsData));

    addInstancedTests({&AbstractUserInterfaceTest::eventTapOrClickNodeRemoved},
        Containers::arraySize(EventNodeRemovedData));

    addInstancedTests({&AbstractUserInterfaceTest::eventTapOrClickAllDataRemoved},
        Containers::arraySize(CleanUpdateData));

    addTests({&AbstractUserInterfaceTest::eventFocus,
              &AbstractUserInterfaceTest::eventFocusNotAccepted});

    addInstancedTests({&AbstractUserInterfaceTest::eventFocusNodeHiddenDisabledNoEvents},
        Containers::arraySize(EventFocusNodeHiddenDisabledNoEventsData));

    addInstancedTests({&AbstractUserInterfaceTest::eventFocusNodeBecomesHiddenDisabledNoEventsNotFocusable},
        Containers::arraySize(EventFocusNodeBecomesHiddenDisabledNoEventsNotFocusableData));

    addInstancedTests({&AbstractUserInterfaceTest::eventFocusNodeRemoved},
        Containers::arraySize(EventNodeRemovedData));

    addInstancedTests({&AbstractUserInterfaceTest::eventFocusAllDataRemoved},
        Containers::arraySize(CleanUpdateData));

    addTests({&AbstractUserInterfaceTest::eventFocusInvalid});

    addTests({&AbstractUserInterfaceTest::eventFocusBlurByPointerPress,
              &AbstractUserInterfaceTest::eventFocusBlurByPointerPressNotAccepted});

    addInstancedTests({&AbstractUserInterfaceTest::eventFocusBlurByPointerPressNodeDisabledNoEvents},
        Containers::arraySize(EventFocusBlurByPointerPressNodeDisabledNoEventsData));

    addInstancedTests({&AbstractUserInterfaceTest::eventKeyPressRelease},
        Containers::arraySize(EventLayouterUpdateData));

    addInstancedTests({&AbstractUserInterfaceTest::eventTextInput},
        Containers::arraySize(UpdateData));

    addTests({&AbstractUserInterfaceTest::eventConvertExternal});
}

void AbstractUserInterfaceTest::debugState() {
    std::ostringstream out;
    Debug{&out} << UserInterfaceState::NeedsNodeClean << UserInterfaceState(0xbebe);
    CORRADE_COMPARE(out.str(), "Whee::UserInterfaceState::NeedsNodeClean Whee::UserInterfaceState(0xbebe)\n");
}

void AbstractUserInterfaceTest::debugStates() {
    std::ostringstream out;
    Debug{&out} << (UserInterfaceState::NeedsNodeClean|UserInterfaceState(0x8000)) << UserInterfaceStates{};
    CORRADE_COMPARE(out.str(), "Whee::UserInterfaceState::NeedsNodeClean|Whee::UserInterfaceState(0x8000) Whee::UserInterfaceStates{}\n");
}

void AbstractUserInterfaceTest::debugStatesSupersets() {
    /* NeedsDataAttachmentUpdate is a superset of NeedsDataUpdate, so only one
       should be printed */
    {
        std::ostringstream out;
        Debug{&out} << (UserInterfaceState::NeedsDataUpdate|UserInterfaceState::NeedsDataAttachmentUpdate);
        CORRADE_COMPARE(out.str(), "Whee::UserInterfaceState::NeedsDataAttachmentUpdate\n");

    /* NeedsNodeEnabledUpdate is a superset of NeedsDataAttachmentUpdate, so
       only one should be printed */
    } {
        std::ostringstream out;
        Debug{&out} << (UserInterfaceState::NeedsNodeEnabledUpdate|UserInterfaceState::NeedsDataAttachmentUpdate);
        CORRADE_COMPARE(out.str(), "Whee::UserInterfaceState::NeedsNodeEnabledUpdate\n");

    /* NeedsNodeClipUpdate is a superset of NeedsNodeEnabledUpdate, so only one
       should be printed */
    } {
        std::ostringstream out;
        Debug{&out} << (UserInterfaceState::NeedsNodeClipUpdate|UserInterfaceState::NeedsNodeEnabledUpdate);
        CORRADE_COMPARE(out.str(), "Whee::UserInterfaceState::NeedsNodeClipUpdate\n");

    /* NeedsLayoutUpdate is a superset of NeedsNodeClipUpdate, so only one
       should be printed */
    } {
        std::ostringstream out;
        Debug{&out} << (UserInterfaceState::NeedsLayoutUpdate|UserInterfaceState::NeedsNodeClipUpdate);
        CORRADE_COMPARE(out.str(), "Whee::UserInterfaceState::NeedsLayoutUpdate\n");

    /* NeedsLayoutAssignmentUpdate is a superset of NeedsLayoutUpdate, so only
       one should be printed */
    } {
        std::ostringstream out;
        Debug{&out} << (UserInterfaceState::NeedsLayoutAssignmentUpdate|UserInterfaceState::NeedsLayoutUpdate);
        CORRADE_COMPARE(out.str(), "Whee::UserInterfaceState::NeedsLayoutAssignmentUpdate\n");

    /* NeedsNodeUpdate is a superset of NeedsNodeLayoutAssignmentUpdate, so
       only one should be printed */
    } {
        std::ostringstream out;
        Debug{&out} << (UserInterfaceState::NeedsNodeUpdate|UserInterfaceState::NeedsLayoutAssignmentUpdate);
        CORRADE_COMPARE(out.str(), "Whee::UserInterfaceState::NeedsNodeUpdate\n");

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

    /* NeedsNodeClean is a superset of all others, so it should be printed
       alone */
    } {
        std::ostringstream out;
        Debug{&out} << (UserInterfaceState::NeedsNodeClean|UserInterfaceState::NeedsDataClean|UserInterfaceState::NeedsNodeUpdate|UserInterfaceState::NeedsDataUpdate|UserInterfaceState::NeedsDataAttachmentUpdate);
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

    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.animationTime(), 0_nsec);

    CORRADE_VERIFY(!ui.hasRenderer());

    CORRADE_COMPARE(ui.layerCapacity(), 0);
    CORRADE_COMPARE(ui.layerUsedCount(), 0);
    CORRADE_COMPARE(ui.layerFirst(), LayerHandle::Null);
    CORRADE_VERIFY(!ui.isHandleValid(LayerHandle::Null));

    CORRADE_COMPARE(ui.layouterCapacity(), 0);
    CORRADE_COMPARE(ui.layouterUsedCount(), 0);
    CORRADE_COMPARE(ui.layouterFirst(), LayouterHandle::Null);
    CORRADE_VERIFY(!ui.isHandleValid(LayouterHandle::Null));

    CORRADE_COMPARE(ui.nodeCapacity(), 0);
    CORRADE_COMPARE(ui.nodeUsedCount(), 0);
    CORRADE_VERIFY(!ui.isHandleValid(NodeHandle::Null));

    CORRADE_COMPARE(ui.nodeOrderFirst(), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLast(), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 0);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 0);

    CORRADE_VERIFY(!ui.isHandleValid(LayoutHandle::Null));
    CORRADE_VERIFY(!ui.isHandleValid(layoutHandle(LayouterHandle(0xffff), LayouterDataHandle::Null)));
    CORRADE_VERIFY(!ui.isHandleValid(layoutHandle(LayouterHandle::Null, LayouterDataHandle(0xffffffff))));
    CORRADE_VERIFY(!ui.isHandleValid(layoutHandle(LayouterHandle(0xffff), LayouterDataHandle(0xffffffff))));

    CORRADE_VERIFY(!ui.isHandleValid(DataHandle::Null));
    CORRADE_VERIFY(!ui.isHandleValid(dataHandle(LayerHandle(0xffff), LayerDataHandle::Null)));
    CORRADE_VERIFY(!ui.isHandleValid(dataHandle(LayerHandle::Null, LayerDataHandle(0xffffffff))));
    CORRADE_VERIFY(!ui.isHandleValid(dataHandle(LayerHandle(0xffff), LayerDataHandle(0xffffffff))));

    CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
    CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
    CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
    CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
    CORRADE_COMPARE(ui.currentGlobalPointerPosition(), Containers::NullOpt);
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

    CORRADE_VERIFY(std::is_nothrow_move_constructible<AbstractUserInterface>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<AbstractUserInterface>::value);
}

void AbstractUserInterfaceTest::renderer() {
    auto&& data = RendererData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* State flags from setSize() affecting node clip state are tested
       thoroughly in state(). Here there are no nodes, which makes them not
       being set at all. */

    int destructed = 0;
    int setupCalled = 0;

    {
        AbstractUserInterface ui{NoCreate};
        if(data.setSizeFirst)
            ui.setSize({165.0f, 156.0f}, {376.0f, 234.0f}, {17, 35});

        struct Renderer: AbstractRenderer {
            explicit Renderer(int& destructed, int& setupCalled): destructed(destructed), setupCalled(setupCalled) {}

            ~Renderer() {
                ++destructed;
            }

            RendererFeatures doFeatures() const override { return {}; }
            void doSetupFramebuffers(const Vector2i& size) override {
                CORRADE_COMPARE(size, (Vector2i{17, 35}));
                ++setupCalled;
            }
            void doTransition(RendererTargetState, RendererTargetState, RendererDrawStates, RendererDrawStates) override {}

            int& destructed;
            int& setupCalled;
        };

        CORRADE_VERIFY(!ui.hasRenderer());
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

        /* If UI size was already set, setting a renderer instance causes
           setupFramebuffers() to be directly called */
        Containers::Pointer<Renderer> instance{InPlaceInit, destructed, setupCalled};
        Renderer* instancePointer = instance.get();
        Renderer& instanceReference = ui.setRendererInstance(Utility::move(instance));
        CORRADE_VERIFY(ui.hasRenderer());
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
        CORRADE_COMPARE(&instanceReference, instancePointer);
        CORRADE_COMPARE(&ui.renderer(), instancePointer);
        CORRADE_COMPARE(&ui.renderer<Renderer>(), instancePointer);
        CORRADE_COMPARE(setupCalled, data.setSizeFirst ? 1 : 0);
        CORRADE_COMPARE(destructed, 0);

        /* Const overloads */
        const AbstractUserInterface& cui = ui;
        CORRADE_COMPARE(&cui.renderer(), instancePointer);
        CORRADE_COMPARE(&cui.renderer<Renderer>(), instancePointer);

        /* Setting the UI size only after the renderer will call the setup
           now */
        if(!data.setSizeFirst)
            ui.setSize({165.0f, 156.0f}, {376.0f, 234.0f}, {17, 35});
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
        CORRADE_COMPARE(setupCalled, 1);
    }

    /* The renderer should be deleted at destruction */
    CORRADE_COMPARE(setupCalled, 1);
    CORRADE_COMPARE(destructed, 1);
}

void AbstractUserInterfaceTest::rendererSetInstanceInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui{{100, 100}};

    struct Renderer: AbstractRenderer {
        RendererFeatures doFeatures() const override { return {}; }
        void doSetupFramebuffers(const Vector2i&) override {}
        void doTransition(RendererTargetState, RendererTargetState, RendererDrawStates, RendererDrawStates) override {}
    };

    ui.setRendererInstance(Containers::pointer<Renderer>());
    CORRADE_VERIFY(ui.hasRenderer());

    std::ostringstream out;
    Error redirectError{&out};
    ui.setRendererInstance(nullptr);
    ui.setRendererInstance(Containers::pointer<Renderer>());
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractUserInterface::setRendererInstance(): instance is null\n"
        "Whee::AbstractUserInterface::setRendererInstance(): instance already set\n");
}

void AbstractUserInterfaceTest::rendererSetInstanceCompositeNotSupported() {
    /* See layerSetInstanceCompositeNotSupported() for checking the same in
       reverse order */

    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui{{100, 100}};

    /* Add a compositing layer */
    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Composite;
        }
    };
    ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer()));

    struct Renderer: AbstractRenderer {
        RendererFeatures doFeatures() const override { return {}; }
        void doSetupFramebuffers(const Vector2i&) override {}
        void doTransition(RendererTargetState, RendererTargetState, RendererDrawStates, RendererDrawStates) override {}
    };

    /* Setting a renderer instance that doesn't support compositing will then
       fail */
    std::ostringstream out;
    Error redirectError{&out};
    ui.setRendererInstance(Containers::pointer<Renderer>());
    CORRADE_COMPARE(out.str(), "Whee::AbstractUserInterface::setRendererInstance(): renderer without Whee::RendererFeature::Composite not usable with a layer that has Whee::LayerFeature::Composite\n");
}

void AbstractUserInterfaceTest::rendererNotSet() {
    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui{{100, 100}};
    const AbstractUserInterface& cui = ui;
    CORRADE_VERIFY(!ui.hasRenderer());

    std::ostringstream out;
    Error redirectError{&out};
    ui.renderer();
    cui.renderer();
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractUserInterface::renderer(): no renderer instance set\n"
        "Whee::AbstractUserInterface::renderer(): no renderer instance set\n");
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

void AbstractUserInterfaceTest::layerSetInstanceCompositeNotSupported() {
    /* See rendererSetInstanceCompositeNotSupported() for checking the same in
       reverse order */

    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui{{100, 100}};

    /* Set a renderer instance that doesn't support compositing */
    struct Renderer: AbstractRenderer {
        RendererFeatures doFeatures() const override { return {}; }
        void doSetupFramebuffers(const Vector2i&) override {}
        void doTransition(RendererTargetState, RendererTargetState, RendererDrawStates, RendererDrawStates) override {}
    };
    ui.setRendererInstance(Containers::pointer<Renderer>());

    /* Adding a compositing layer will then fail */
    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Composite;
        }
    };

    std::ostringstream out;
    Error redirectError{&out};
    ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer()));
    CORRADE_COMPARE(out.str(), "Whee::AbstractUserInterface::setLayerInstance(): layer with Whee::LayerFeature::Composite not usable with a renderer that has Whee::RendererFeatures{}\n");
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

void AbstractUserInterfaceTest::layouter() {
    AbstractUserInterface ui{{100, 100}};
    CORRADE_COMPARE(ui.layouterCapacity(), 0);
    CORRADE_COMPARE(ui.layouterUsedCount(), 0);
    CORRADE_COMPARE(ui.layouterFirst(), LayouterHandle::Null);
    CORRADE_COMPARE(ui.layouterLast(), LayouterHandle::Null);

    /* First layouter ever */
    LayouterHandle first = ui.createLayouter();
    CORRADE_COMPARE(first, layouterHandle(0, 1));
    CORRADE_VERIFY(ui.isHandleValid(first));
    CORRADE_COMPARE(ui.layouterFirst(), first);
    CORRADE_COMPARE(ui.layouterLast(), first);
    CORRADE_COMPARE(ui.layouterPrevious(first), LayouterHandle::Null);
    CORRADE_COMPARE(ui.layouterNext(first), LayouterHandle::Null);
    CORRADE_COMPARE(ui.layouterCapacity(), 1);
    CORRADE_COMPARE(ui.layouterUsedCount(), 1);

    /* Adding a layouter at the end */
    LayouterHandle second = ui.createLayouter();
    CORRADE_COMPARE(second, layouterHandle(1, 1));
    CORRADE_VERIFY(ui.isHandleValid(second));
    CORRADE_COMPARE(ui.layouterFirst(), first);
    CORRADE_COMPARE(ui.layouterLast(), second);
    CORRADE_COMPARE(ui.layouterPrevious(first), LayouterHandle::Null);
    CORRADE_COMPARE(ui.layouterNext(first), second);
    CORRADE_COMPARE(ui.layouterPrevious(second), first);
    CORRADE_COMPARE(ui.layouterNext(second), LayouterHandle::Null);
    CORRADE_COMPARE(ui.layouterCapacity(), 2);
    CORRADE_COMPARE(ui.layouterUsedCount(), 2);

    /* Adding a layouter at the front */
    LayouterHandle third = ui.createLayouter(first);
    CORRADE_COMPARE(third, layouterHandle(2, 1));
    CORRADE_VERIFY(ui.isHandleValid(third));
    CORRADE_COMPARE(ui.layouterFirst(), third);
    CORRADE_COMPARE(ui.layouterLast(), second);
    CORRADE_COMPARE(ui.layouterPrevious(third), LayouterHandle::Null);
    CORRADE_COMPARE(ui.layouterNext(third), first);
    CORRADE_COMPARE(ui.layouterPrevious(first), third);
    CORRADE_COMPARE(ui.layouterNext(first), second);
    CORRADE_COMPARE(ui.layouterPrevious(second), first);
    CORRADE_COMPARE(ui.layouterNext(second), LayouterHandle::Null);
    CORRADE_COMPARE(ui.layouterCapacity(), 3);
    CORRADE_COMPARE(ui.layouterUsedCount(), 3);

    /* Adding a layouter in the middle */
    LayouterHandle fourth = ui.createLayouter(first);
    CORRADE_COMPARE(fourth, layouterHandle(3, 1));
    CORRADE_VERIFY(ui.isHandleValid(fourth));
    CORRADE_COMPARE(ui.layouterFirst(), third);
    CORRADE_COMPARE(ui.layouterLast(), second);
    CORRADE_COMPARE(ui.layouterPrevious(third), LayouterHandle::Null);
    CORRADE_COMPARE(ui.layouterNext(third), fourth);
    CORRADE_COMPARE(ui.layouterPrevious(fourth), third);
    CORRADE_COMPARE(ui.layouterNext(fourth), first);
    CORRADE_COMPARE(ui.layouterPrevious(first), fourth);
    CORRADE_COMPARE(ui.layouterNext(first), second);
    CORRADE_COMPARE(ui.layouterPrevious(second), first);
    CORRADE_COMPARE(ui.layouterNext(second), LayouterHandle::Null);
    CORRADE_COMPARE(ui.layouterCapacity(), 4);
    CORRADE_COMPARE(ui.layouterUsedCount(), 4);

    /* Removing from the middle of the list */
    ui.removeLayouter(first);
    CORRADE_COMPARE(ui.layouterCapacity(), 4);
    CORRADE_COMPARE(ui.layouterUsedCount(), 3);
    CORRADE_VERIFY(!ui.isHandleValid(first));
    CORRADE_COMPARE(ui.layouterFirst(), third);
    CORRADE_COMPARE(ui.layouterLast(), second);
    CORRADE_COMPARE(ui.layouterPrevious(third), LayouterHandle::Null);
    CORRADE_COMPARE(ui.layouterNext(third), fourth);
    CORRADE_COMPARE(ui.layouterPrevious(fourth), third);
    CORRADE_COMPARE(ui.layouterNext(fourth), second);
    CORRADE_COMPARE(ui.layouterPrevious(second), fourth);
    CORRADE_COMPARE(ui.layouterNext(second), LayouterHandle::Null);

    /* Removing from the back of the list */
    ui.removeLayouter(second);
    CORRADE_COMPARE(ui.layouterCapacity(), 4);
    CORRADE_COMPARE(ui.layouterUsedCount(), 2);
    CORRADE_VERIFY(!ui.isHandleValid(second));
    CORRADE_COMPARE(ui.layouterFirst(), third);
    CORRADE_COMPARE(ui.layouterLast(), fourth);
    CORRADE_COMPARE(ui.layouterPrevious(third), LayouterHandle::Null);
    CORRADE_COMPARE(ui.layouterNext(third), fourth);
    CORRADE_COMPARE(ui.layouterPrevious(fourth), third);
    CORRADE_COMPARE(ui.layouterNext(fourth), LayouterHandle::Null);

    /* Removing from the front of the list */
    ui.removeLayouter(third);
    CORRADE_COMPARE(ui.layouterCapacity(), 4);
    CORRADE_COMPARE(ui.layouterUsedCount(), 1);
    CORRADE_VERIFY(!ui.isHandleValid(third));
    CORRADE_COMPARE(ui.layouterFirst(), fourth);
    CORRADE_COMPARE(ui.layouterLast(), fourth);
    CORRADE_COMPARE(ui.layouterPrevious(fourth), LayouterHandle::Null);
    CORRADE_COMPARE(ui.layouterNext(fourth), LayouterHandle::Null);

    /* Removing the last layouter */
    ui.removeLayouter(fourth);
    CORRADE_COMPARE(ui.layouterCapacity(), 4);
    CORRADE_COMPARE(ui.layouterUsedCount(), 0);
    CORRADE_VERIFY(!ui.isHandleValid(fourth));
    CORRADE_COMPARE(ui.layouterFirst(), LayouterHandle::Null);
    CORRADE_COMPARE(ui.layouterLast(), LayouterHandle::Null);
}

void AbstractUserInterfaceTest::layouterHandleRecycle() {
    AbstractUserInterface ui{{100, 100}};
    LayouterHandle first = ui.createLayouter();
    LayouterHandle second = ui.createLayouter();
    LayouterHandle third = ui.createLayouter();
    LayouterHandle fourth = ui.createLayouter();
    CORRADE_COMPARE(first, layouterHandle(0, 1));
    CORRADE_COMPARE(second, layouterHandle(1, 1));
    CORRADE_COMPARE(third, layouterHandle(2, 1));
    CORRADE_COMPARE(fourth, layouterHandle(3, 1));
    CORRADE_VERIFY(ui.isHandleValid(first));
    CORRADE_VERIFY(ui.isHandleValid(second));
    CORRADE_VERIFY(ui.isHandleValid(third));
    CORRADE_VERIFY(ui.isHandleValid(fourth));
    CORRADE_COMPARE(ui.layouterCapacity(), 4);
    CORRADE_COMPARE(ui.layouterUsedCount(), 4);

    /* Remove three out of the four in an arbitrary order */
    ui.removeLayouter(second);
    ui.removeLayouter(fourth);
    ui.removeLayouter(first);
    CORRADE_VERIFY(!ui.isHandleValid(first));
    CORRADE_VERIFY(!ui.isHandleValid(second));
    CORRADE_VERIFY(ui.isHandleValid(third));
    CORRADE_VERIFY(!ui.isHandleValid(fourth));
    CORRADE_COMPARE(ui.layouterCapacity(), 4);
    CORRADE_COMPARE(ui.layouterUsedCount(), 1);

    /* Allocating new handles should recycle the handles in the order they were
       removed (oldest first) */
    LayouterHandle second2 = ui.createLayouter();
    LayouterHandle fourth2 = ui.createLayouter();
    LayouterHandle first2 = ui.createLayouter();
    CORRADE_COMPARE(first2, layouterHandle(0, 2));
    CORRADE_COMPARE(second2, layouterHandle(1, 2));
    CORRADE_COMPARE(fourth2, layouterHandle(3, 2));
    CORRADE_COMPARE(ui.layouterCapacity(), 4);
    CORRADE_COMPARE(ui.layouterUsedCount(), 4);

    /* Old handles shouldn't get valid again */
    CORRADE_VERIFY(!ui.isHandleValid(first));
    CORRADE_VERIFY(ui.isHandleValid(first2));
    CORRADE_VERIFY(!ui.isHandleValid(second));
    CORRADE_VERIFY(ui.isHandleValid(second2));
    CORRADE_VERIFY(!ui.isHandleValid(fourth));
    CORRADE_VERIFY(ui.isHandleValid(fourth2));

    /* Removing a single handle and creating a new one directly reuses it if
       there's just one in the free list */
    ui.removeLayouter(second2);
    LayouterHandle second3 = ui.createLayouter();
    CORRADE_COMPARE(second3, layouterHandle(1, 3));
    CORRADE_VERIFY(!ui.isHandleValid(second));
    CORRADE_VERIFY(!ui.isHandleValid(second2));
    CORRADE_VERIFY(ui.isHandleValid(second3));
    CORRADE_COMPARE(ui.layouterCapacity(), 4);
    CORRADE_COMPARE(ui.layouterUsedCount(), 4);

    /* Allocating a new handle with the free list empty will grow it */
    LayouterHandle fifth = ui.createLayouter();
    CORRADE_COMPARE(fifth, layouterHandle(4, 1));
    CORRADE_VERIFY(ui.isHandleValid(fifth));
    CORRADE_COMPARE(ui.layouterCapacity(), 5);
    CORRADE_COMPARE(ui.layouterUsedCount(), 5);
}

void AbstractUserInterfaceTest::layouterHandleDisable() {
    AbstractUserInterface ui{{100, 100}};

    LayouterHandle first = ui.createLayouter();
    CORRADE_COMPARE(first, layouterHandle(0, 1));

    for(std::size_t i = 0; i != (1 << Implementation::LayouterHandleGenerationBits) - 1; ++i) {
        LayouterHandle second = ui.createLayouter();
        CORRADE_COMPARE(second, layouterHandle(1, 1 + i));
        ui.removeLayouter(second);
    }

    /* The generation for the second slot is exhausted so the handle is not
       recycled */
    CORRADE_COMPARE(ui.layouterCapacity(), 2);
    CORRADE_COMPARE(ui.layouterUsedCount(), 2);

    /* It shouldn't think a handle from the second slot with generation 0 is
       valid */
    CORRADE_VERIFY(!ui.isHandleValid(layouterHandle(1, 0)));

    /* There's nowhere to create a new handle from so the capacity is grown */
    LayouterHandle third = ui.createLayouter();
    CORRADE_COMPARE(third, layouterHandle(2, 1));
    CORRADE_COMPARE(ui.layouterCapacity(), 3);
    CORRADE_COMPARE(ui.layouterUsedCount(), 3);
}

void AbstractUserInterfaceTest::layouterHandleLastFree() {
    AbstractUserInterface ui{{100, 100}};
    LayouterHandle first = ui.createLayouter();
    LayouterHandle second = ui.createLayouter();
    for(std::size_t i = 0; i != (1 << Implementation::LayouterHandleIdBits) - 3; ++i)
        ui.createLayouter();
    LayouterHandle last = ui.createLayouter();
    CORRADE_COMPARE(first, layouterHandle(0, 1));
    CORRADE_COMPARE(second, layouterHandle(1, 1));
    CORRADE_COMPARE(last, layouterHandle(255, 1));
    CORRADE_COMPARE(ui.layouterCapacity(), 256);
    CORRADE_COMPARE(ui.layouterUsedCount(), 256);

    /* Removing the last layouter should lead to one being marked as free, not
       0 due to 255 treated as "no more free layouters" */
    ui.removeLayouter(last);
    CORRADE_COMPARE(ui.layouterCapacity(), 256);
    CORRADE_COMPARE(ui.layouterUsedCount(), 255);

    /* Create a layouter with ID 255 again */
    last = ui.createLayouter();
    CORRADE_COMPARE(last, layouterHandle(255, 2));

    /* Removing the three layouters (with the one with ID 255 being in the
       middle) should mark all three as free, not just 2 due to 255 being
       treated as "no more free layouters" */
    ui.removeLayouter(first);
    ui.removeLayouter(last);
    ui.removeLayouter(second);
    CORRADE_COMPARE(ui.layouterCapacity(), 256);
    CORRADE_COMPARE(ui.layouterUsedCount(), 253);
}

void AbstractUserInterfaceTest::layouterSetInstance() {
    int firstDestructed = 0;
    int secondDestructed = 0;

    {
        AbstractUserInterface ui{{100, 100}};
        LayouterHandle first = ui.createLayouter();
        LayouterHandle second = ui.createLayouter();
        LayouterHandle third = ui.createLayouter();

        struct Layouter: AbstractLayouter {
            explicit Layouter(LayouterHandle handle, int& destructed): AbstractLayouter{handle}, destructed(destructed) {}

            ~Layouter() {
                ++destructed;
            }

            void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const  Containers::StridedArrayView1D<Vector2>&) override {}

            int& destructed;
        };

        Containers::Pointer<Layouter> firstInstance{InPlaceInit, first, firstDestructed};
        Containers::Pointer<Layouter> secondInstance{InPlaceInit, second, secondDestructed};
        /* Third deliberately doesn't have an instance set */
        Layouter* firstInstancePointer = firstInstance.get();
        Layouter* secondInstancePointer = secondInstance.get();
        /* Set them in different order, shouldn't matter */
        Layouter& secondInstanceReference = ui.setLayouterInstance(Utility::move(secondInstance));
        Layouter& firstInstanceReference = ui.setLayouterInstance(Utility::move(firstInstance));
        CORRADE_COMPARE(ui.layouterCapacity(), 3);
        CORRADE_COMPARE(ui.layouterUsedCount(), 3);
        CORRADE_COMPARE(&firstInstanceReference, firstInstancePointer);
        CORRADE_COMPARE(&secondInstanceReference, secondInstancePointer);
        CORRADE_COMPARE(&ui.layouter(first), firstInstancePointer);
        CORRADE_COMPARE(&ui.layouter(second), secondInstancePointer);
        CORRADE_COMPARE(&ui.layouter<Layouter>(first), firstInstancePointer);
        CORRADE_COMPARE(&ui.layouter<Layouter>(second), secondInstancePointer);
        CORRADE_COMPARE(firstDestructed, 0);
        CORRADE_COMPARE(secondDestructed, 0);

        /* Const overloads */
        const AbstractUserInterface& cui = ui;
        CORRADE_COMPARE(&cui.layouter(first), firstInstancePointer);
        CORRADE_COMPARE(&cui.layouter(second), secondInstancePointer);
        CORRADE_COMPARE(&cui.layouter<Layouter>(first), firstInstancePointer);
        CORRADE_COMPARE(&cui.layouter<Layouter>(second), secondInstancePointer);

        ui.removeLayouter(first);
        CORRADE_COMPARE(firstDestructed, 1);
        CORRADE_COMPARE(secondDestructed, 0);

        /* Removing a layouter that doesn't have any instance set shouldn't
           affect the others in any way */
        ui.removeLayouter(third);
        CORRADE_COMPARE(firstDestructed, 1);
        CORRADE_COMPARE(secondDestructed, 0);
    }

    /* The remaining layouter should be deleted at destruction */
    CORRADE_COMPARE(firstDestructed, 1);
    CORRADE_COMPARE(secondDestructed, 1);
}

void AbstractUserInterfaceTest::layouterCreateInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui{{100, 100}};

    std::ostringstream out;
    Error redirectError{&out};
    ui.createLayouter(LayouterHandle(0xabcd));
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractUserInterface::createLayouter(): invalid before handle Whee::LayouterHandle(0xcd, 0xab)\n");
}

void AbstractUserInterfaceTest::layouterSetInstanceInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Layouter: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;

        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const  Containers::StridedArrayView1D<Vector2>&) override {}
    };

    AbstractUserInterface ui{{100, 100}};

    LayouterHandle handle = ui.createLayouter();
    ui.setLayouterInstance(Containers::pointer<Layouter>(handle));

    std::ostringstream out;
    Error redirectError{&out};
    ui.setLayouterInstance(nullptr);
    ui.setLayouterInstance(Containers::pointer<Layouter>(LayouterHandle(0xabcd)));
    ui.setLayouterInstance(Containers::pointer<Layouter>(handle));
    CORRADE_COMPARE_AS(out.str(),
        "Whee::AbstractUserInterface::setLayouterInstance(): instance is null\n"
        "Whee::AbstractUserInterface::setLayouterInstance(): invalid handle Whee::LayouterHandle(0xcd, 0xab)\n"
        "Whee::AbstractUserInterface::setLayouterInstance(): instance for Whee::LayouterHandle(0x0, 0x1) already set\n",
        TestSuite::Compare::String);
}

void AbstractUserInterfaceTest::layouterGetInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Layouter: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;

        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const  Containers::StridedArrayView1D<Vector2>&) override {}
    };

    AbstractUserInterface ui{{100, 100}};
    /* Need at least one layouter to be present so layouter() asserts can
       return something */
    ui.setLayouterInstance(Containers::pointer<Layouter>(ui.createLayouter()));

    const AbstractUserInterface& cui = ui;

    LayouterHandle handle = ui.createLayouter();

    std::ostringstream out;
    Error redirectError{&out};
    ui.layouterPrevious(LayouterHandle(0x12ab));
    ui.layouterPrevious(LayouterHandle::Null);
    ui.layouterNext(LayouterHandle(0x12ab));
    ui.layouterNext(LayouterHandle::Null);
    ui.layouter(handle);
    ui.layouter(LayouterHandle::Null);
    /* Const overloads */
    cui.layouter(handle);
    cui.layouter(LayouterHandle::Null);
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractUserInterface::layouterPrevious(): invalid handle Whee::LayouterHandle(0xab, 0x12)\n"
        "Whee::AbstractUserInterface::layouterPrevious(): invalid handle Whee::LayouterHandle::Null\n"
        "Whee::AbstractUserInterface::layouterNext(): invalid handle Whee::LayouterHandle(0xab, 0x12)\n"
        "Whee::AbstractUserInterface::layouterNext(): invalid handle Whee::LayouterHandle::Null\n"
        "Whee::AbstractUserInterface::layouter(): Whee::LayouterHandle(0x1, 0x1) has no instance set\n"
        "Whee::AbstractUserInterface::layouter(): invalid handle Whee::LayouterHandle::Null\n"
        "Whee::AbstractUserInterface::layouter(): Whee::LayouterHandle(0x1, 0x1) has no instance set\n"
        "Whee::AbstractUserInterface::layouter(): invalid handle Whee::LayouterHandle::Null\n");
}

void AbstractUserInterfaceTest::layouterRemoveInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui{{100, 100}};

    std::ostringstream out;
    Error redirectError{&out};
    ui.removeLayouter(LayouterHandle::Null);
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractUserInterface::removeLayouter(): invalid handle Whee::LayouterHandle::Null\n");
}

void AbstractUserInterfaceTest::layouterNoHandlesLeft() {
    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui{{100, 100}};

    LayouterHandle handle;
    for(std::size_t i = 0; i != 1 << Implementation::LayouterHandleIdBits; ++i)
        handle = ui.createLayouter();
    CORRADE_COMPARE(handle, layouterHandle((1 << Implementation::LayouterHandleIdBits) - 1, 1));

    CORRADE_COMPARE(ui.layouterCapacity(), 1 << Implementation::LayouterHandleIdBits);
    CORRADE_COMPARE(ui.layouterUsedCount(), 1 << Implementation::LayouterHandleIdBits);

    std::ostringstream out;
    Error redirectError{&out};
    ui.createLayouter();
    /* Number is hardcoded in the expected message but not elsewhere in order
       to give a heads-up when modifying the handle ID bit count */
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractUserInterface::createLayouter(): can only have at most 256 layouters\n");
}

void AbstractUserInterfaceTest::animator() {
    AbstractUserInterface ui{{100, 100}};
    CORRADE_COMPARE(ui.animatorCapacity(), 0);
    CORRADE_COMPARE(ui.animatorUsedCount(), 0);

    AnimatorHandle first = ui.createAnimator();
    CORRADE_COMPARE(first, animatorHandle(0, 1));
    CORRADE_VERIFY(ui.isHandleValid(first));
    CORRADE_COMPARE(ui.animatorCapacity(), 1);
    CORRADE_COMPARE(ui.animatorUsedCount(), 1);

    AnimatorHandle second = ui.createAnimator();
    CORRADE_COMPARE(second, animatorHandle(1, 1));
    CORRADE_VERIFY(ui.isHandleValid(second));
    CORRADE_COMPARE(ui.animatorCapacity(), 2);
    CORRADE_COMPARE(ui.animatorUsedCount(), 2);

    AnimatorHandle third = ui.createAnimator();
    CORRADE_COMPARE(third, animatorHandle(2, 1));
    CORRADE_VERIFY(ui.isHandleValid(third));
    CORRADE_COMPARE(ui.animatorCapacity(), 3);
    CORRADE_COMPARE(ui.animatorUsedCount(), 3);

    AnimatorHandle fourth = ui.createAnimator();
    CORRADE_COMPARE(fourth, animatorHandle(3, 1));
    CORRADE_VERIFY(ui.isHandleValid(fourth));
    CORRADE_COMPARE(ui.animatorCapacity(), 4);
    CORRADE_COMPARE(ui.animatorUsedCount(), 4);

    /* Removing from the middle of the list */
    ui.removeAnimator(third);
    CORRADE_COMPARE(ui.animatorCapacity(), 4);
    CORRADE_COMPARE(ui.animatorUsedCount(), 3);
    CORRADE_VERIFY(!ui.isHandleValid(third));

    /* Removing from the back of the list */
    ui.removeAnimator(fourth);
    CORRADE_COMPARE(ui.animatorCapacity(), 4);
    CORRADE_COMPARE(ui.animatorUsedCount(), 2);
    CORRADE_VERIFY(!ui.isHandleValid(fourth));

    /* Removing from the front of the list */
    ui.removeAnimator(first);
    CORRADE_COMPARE(ui.animatorCapacity(), 4);
    CORRADE_COMPARE(ui.animatorUsedCount(), 1);
    CORRADE_VERIFY(!ui.isHandleValid(third));

    /* Removing the last animator */
    ui.removeAnimator(second);
    CORRADE_COMPARE(ui.animatorCapacity(), 4);
    CORRADE_COMPARE(ui.animatorUsedCount(), 0);
    CORRADE_VERIFY(!ui.isHandleValid(second));
}

void AbstractUserInterfaceTest::animatorHandleRecycle() {
    AbstractUserInterface ui{{100, 100}};
    AnimatorHandle first = ui.createAnimator();
    AnimatorHandle second = ui.createAnimator();
    AnimatorHandle third = ui.createAnimator();
    AnimatorHandle fourth = ui.createAnimator();
    CORRADE_COMPARE(first, animatorHandle(0, 1));
    CORRADE_COMPARE(second, animatorHandle(1, 1));
    CORRADE_COMPARE(third, animatorHandle(2, 1));
    CORRADE_COMPARE(fourth, animatorHandle(3, 1));
    CORRADE_VERIFY(ui.isHandleValid(first));
    CORRADE_VERIFY(ui.isHandleValid(second));
    CORRADE_VERIFY(ui.isHandleValid(third));
    CORRADE_VERIFY(ui.isHandleValid(fourth));
    CORRADE_COMPARE(ui.animatorCapacity(), 4);
    CORRADE_COMPARE(ui.animatorUsedCount(), 4);

    /* Remove three out of the four in an arbitrary order */
    ui.removeAnimator(second);
    ui.removeAnimator(fourth);
    ui.removeAnimator(first);
    CORRADE_VERIFY(!ui.isHandleValid(first));
    CORRADE_VERIFY(!ui.isHandleValid(second));
    CORRADE_VERIFY(ui.isHandleValid(third));
    CORRADE_VERIFY(!ui.isHandleValid(fourth));
    CORRADE_COMPARE(ui.animatorCapacity(), 4);
    CORRADE_COMPARE(ui.animatorUsedCount(), 1);

    /* Allocating new handles should recycle the handles in the order they were
       removed (oldest first) */
    AnimatorHandle second2 = ui.createAnimator();
    AnimatorHandle fourth2 = ui.createAnimator();
    AnimatorHandle first2 = ui.createAnimator();
    CORRADE_COMPARE(first2, animatorHandle(0, 2));
    CORRADE_COMPARE(second2, animatorHandle(1, 2));
    CORRADE_COMPARE(fourth2, animatorHandle(3, 2));
    CORRADE_COMPARE(ui.animatorCapacity(), 4);
    CORRADE_COMPARE(ui.animatorUsedCount(), 4);

    /* Old handles shouldn't get valid again */
    CORRADE_VERIFY(!ui.isHandleValid(first));
    CORRADE_VERIFY(ui.isHandleValid(first2));
    CORRADE_VERIFY(!ui.isHandleValid(second));
    CORRADE_VERIFY(ui.isHandleValid(second2));
    CORRADE_VERIFY(!ui.isHandleValid(fourth));
    CORRADE_VERIFY(ui.isHandleValid(fourth2));

    /* Removing a single handle and creating a new one directly reuses it if
       there's just one in the free list */
    ui.removeAnimator(second2);
    AnimatorHandle second3 = ui.createAnimator();
    CORRADE_COMPARE(second3, animatorHandle(1, 3));
    CORRADE_VERIFY(!ui.isHandleValid(second));
    CORRADE_VERIFY(!ui.isHandleValid(second2));
    CORRADE_VERIFY(ui.isHandleValid(second3));
    CORRADE_COMPARE(ui.animatorCapacity(), 4);
    CORRADE_COMPARE(ui.animatorUsedCount(), 4);

    /* Allocating a new handle with the free list empty will grow it */
    AnimatorHandle fifth = ui.createAnimator();
    CORRADE_COMPARE(fifth, animatorHandle(4, 1));
    CORRADE_VERIFY(ui.isHandleValid(fifth));
    CORRADE_COMPARE(ui.animatorCapacity(), 5);
    CORRADE_COMPARE(ui.animatorUsedCount(), 5);
}

void AbstractUserInterfaceTest::animatorHandleDisable() {
    AbstractUserInterface ui{{100, 100}};

    AnimatorHandle first = ui.createAnimator();
    CORRADE_COMPARE(first, animatorHandle(0, 1));

    for(std::size_t i = 0; i != (1 << Implementation::AnimatorHandleGenerationBits) - 1; ++i) {
        AnimatorHandle second = ui.createAnimator();
        CORRADE_COMPARE(second, animatorHandle(1, 1 + i));
        ui.removeAnimator(second);
    }

    /* The generation for the second slot is exhausted so the handle is not
       recycled */
    CORRADE_COMPARE(ui.animatorCapacity(), 2);
    CORRADE_COMPARE(ui.animatorUsedCount(), 2);

    /* It shouldn't think a handle from the second slot with generation 0 is
       valid */
    CORRADE_VERIFY(!ui.isHandleValid(animatorHandle(1, 0)));

    /* There's nowhere to create a new handle from so the capacity is grown */
    AnimatorHandle third = ui.createAnimator();
    CORRADE_COMPARE(third, animatorHandle(2, 1));
    CORRADE_COMPARE(ui.animatorCapacity(), 3);
    CORRADE_COMPARE(ui.animatorUsedCount(), 3);
}

void AbstractUserInterfaceTest::animatorHandleLastFree() {
    AbstractUserInterface ui{{100, 100}};
    AnimatorHandle first = ui.createAnimator();
    AnimatorHandle second = ui.createAnimator();
    for(std::size_t i = 0; i != (1 << Implementation::AnimatorHandleIdBits) - 3; ++i)
        ui.createAnimator();
    AnimatorHandle last = ui.createAnimator();
    CORRADE_COMPARE(first, animatorHandle(0, 1));
    CORRADE_COMPARE(second, animatorHandle(1, 1));
    CORRADE_COMPARE(last, animatorHandle(255, 1));
    CORRADE_COMPARE(ui.animatorCapacity(), 256);
    CORRADE_COMPARE(ui.animatorUsedCount(), 256);

    /* Removing the last animator should lead to one being marked as free, not
       0 due to 255 treated as "no more free animators" */
    ui.removeAnimator(last);
    CORRADE_COMPARE(ui.animatorCapacity(), 256);
    CORRADE_COMPARE(ui.animatorUsedCount(), 255);

    /* Create a animator with ID 255 again */
    last = ui.createAnimator();
    CORRADE_COMPARE(last, animatorHandle(255, 2));

    /* Removing the three animators (with the one with ID 255 being in the
       middle) should mark all three as free, not just 2 due to 255 being
       treated as "no more free animators" */
    ui.removeAnimator(first);
    ui.removeAnimator(last);
    ui.removeAnimator(second);
    CORRADE_COMPARE(ui.animatorCapacity(), 256);
    CORRADE_COMPARE(ui.animatorUsedCount(), 253);
}

void AbstractUserInterfaceTest::animatorSetInstance() {
    int firstDestructed = 0;
    int secondDestructed = 0;
    int fourthDestructed = 0;
    int fifthDestructed = 0;
    int sixthDestructed = 0;

    {
        AbstractUserInterface ui{{100, 100}};

        struct Layer: AbstractLayer {
            using AbstractLayer::AbstractLayer;
            using AbstractLayer::setAnimator;

            LayerFeatures doFeatures() const override {
                return LayerFeature::AnimateData|LayerFeature::AnimateStyles;
            }
        };
        Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer()));

        AnimatorHandle first = ui.createAnimator();
        AnimatorHandle second = ui.createAnimator();
        AnimatorHandle third = ui.createAnimator();
        AnimatorHandle fourth = ui.createAnimator();
        AnimatorHandle fifth = ui.createAnimator();
        AnimatorHandle sixth = ui.createAnimator();

        struct GenericAnimator: AbstractGenericAnimator {
            explicit GenericAnimator(AnimatorHandle handle, int& destructed): AbstractGenericAnimator{handle}, destructed(destructed) {}
            ~GenericAnimator() {
                ++destructed;
            }
            AnimatorFeatures doFeatures() const override { return {}; }
            void doAdvance(Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}

            int& destructed;
        };
        struct NodeAnimator: AbstractNodeAnimator {
            explicit NodeAnimator(AnimatorHandle handle, int& destructed): AbstractNodeAnimator{handle}, destructed(destructed) {}
            ~NodeAnimator() {
                ++destructed;
            }
            AnimatorFeatures doFeatures() const override {
                return AnimatorFeature::NodeAttachment;
            }
            NodeAnimations doAdvance(Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<NodeFlags>&, Containers::MutableBitArrayView) override { return {}; }

            int& destructed;
        };
        struct DataAnimator: AbstractDataAnimator {
            explicit DataAnimator(AnimatorHandle handle, int& destructed): AbstractDataAnimator{handle}, destructed(destructed) {}
            ~DataAnimator() {
                ++destructed;
            }

            AnimatorFeatures doFeatures() const override {
                return AnimatorFeature::DataAttachment;
            }

            int& destructed;
        };
        struct StyleAnimator: AbstractStyleAnimator {
            explicit StyleAnimator(AnimatorHandle handle, int& destructed): AbstractStyleAnimator{handle}, destructed(destructed) {}
            ~StyleAnimator() {
                ++destructed;
            }

            AnimatorFeatures doFeatures() const override {
                return AnimatorFeature::DataAttachment;
            }

            int& destructed;
        };

        Containers::Pointer<GenericAnimator> firstInstance{InPlaceInit, first, firstDestructed};
        Containers::Pointer<NodeAnimator> secondInstance{InPlaceInit, second, secondDestructed};
        /* Third deliberately doesn't have an instance set */
        Containers::Pointer<GenericAnimator> fourthInstance{InPlaceInit, fourth, fourthDestructed};
        Containers::Pointer<DataAnimator> fifthInstance{InPlaceInit, fifth, fifthDestructed};
        Containers::Pointer<StyleAnimator> sixthInstance{InPlaceInit, sixth, sixthDestructed};
        layer.setAnimator(*fifthInstance);
        layer.setAnimator(*sixthInstance);
        GenericAnimator* firstInstancePointer = firstInstance.get();
        NodeAnimator* secondInstancePointer = secondInstance.get();
        GenericAnimator* fourthInstancePointer = fourthInstance.get();
        DataAnimator* fifthInstancePointer = fifthInstance.get();
        StyleAnimator* sixthInstancePointer = sixthInstance.get();
        /* Set them in different order, shouldn't matter */
        GenericAnimator& fourthInstanceReference = ui.setGenericAnimatorInstance(Utility::move(fourthInstance));
        StyleAnimator& sixthInstanceReference = ui.setStyleAnimatorInstance(Utility::move(sixthInstance));
        NodeAnimator& secondInstanceReference = ui.setNodeAnimatorInstance(Utility::move(secondInstance));
        DataAnimator& fifthInstanceReference = ui.setDataAnimatorInstance(Utility::move(fifthInstance));
        GenericAnimator& firstInstanceReference = ui.setGenericAnimatorInstance(Utility::move(firstInstance));
        CORRADE_COMPARE(ui.animatorCapacity(), 6);
        CORRADE_COMPARE(ui.animatorUsedCount(), 6);
        CORRADE_COMPARE(&firstInstanceReference, firstInstancePointer);
        CORRADE_COMPARE(&secondInstanceReference, secondInstancePointer);
        CORRADE_COMPARE(&fourthInstanceReference, fourthInstancePointer);
        CORRADE_COMPARE(&fifthInstanceReference, fifthInstancePointer);
        CORRADE_COMPARE(&sixthInstanceReference, sixthInstancePointer);
        CORRADE_COMPARE(&ui.animator(first), firstInstancePointer);
        CORRADE_COMPARE(&ui.animator(second), secondInstancePointer);
        CORRADE_COMPARE(&ui.animator(fourth), fourthInstancePointer);
        CORRADE_COMPARE(&ui.animator(fifth), fifthInstancePointer);
        CORRADE_COMPARE(&ui.animator(sixth), sixthInstancePointer);
        CORRADE_COMPARE(&ui.animator<GenericAnimator>(first), firstInstancePointer);
        CORRADE_COMPARE(&ui.animator<NodeAnimator>(second), secondInstancePointer);
        CORRADE_COMPARE(&ui.animator<GenericAnimator>(fourth), fourthInstancePointer);
        CORRADE_COMPARE(&ui.animator<DataAnimator>(fifth), fifthInstancePointer);
        CORRADE_COMPARE(&ui.animator<StyleAnimator>(sixth), sixthInstancePointer);
        CORRADE_COMPARE(firstDestructed, 0);
        CORRADE_COMPARE(secondDestructed, 0);
        CORRADE_COMPARE(fourthDestructed, 0);
        CORRADE_COMPARE(fifthDestructed, 0);
        CORRADE_COMPARE(sixthDestructed, 0);

        /* Const overloads */
        const AbstractUserInterface& cui = ui;
        CORRADE_COMPARE(&cui.animator(first), firstInstancePointer);
        CORRADE_COMPARE(&cui.animator(second), secondInstancePointer);
        CORRADE_COMPARE(&cui.animator(fourth), fourthInstancePointer);
        CORRADE_COMPARE(&cui.animator(fifth), fifthInstancePointer);
        CORRADE_COMPARE(&cui.animator(sixth), sixthInstancePointer);
        CORRADE_COMPARE(&cui.animator<GenericAnimator>(first), firstInstancePointer);
        CORRADE_COMPARE(&cui.animator<NodeAnimator>(second), secondInstancePointer);
        CORRADE_COMPARE(&cui.animator<GenericAnimator>(fourth), fourthInstancePointer);
        CORRADE_COMPARE(&cui.animator<DataAnimator>(fifth), fifthInstancePointer);
        CORRADE_COMPARE(&cui.animator<StyleAnimator>(sixth), sixthInstancePointer);

        ui.removeAnimator(first);
        CORRADE_COMPARE(firstDestructed, 1);
        CORRADE_COMPARE(secondDestructed, 0);
        CORRADE_COMPARE(fourthDestructed, 0);
        CORRADE_COMPARE(fifthDestructed, 0);
        CORRADE_COMPARE(sixthDestructed, 0);

        /* Removing an animator that doesn't have any instance set shouldn't
           affect the others in any way */
        ui.removeAnimator(third);
        CORRADE_COMPARE(firstDestructed, 1);
        CORRADE_COMPARE(secondDestructed, 0);
        CORRADE_COMPARE(fourthDestructed, 0);
        CORRADE_COMPARE(fifthDestructed, 0);
        CORRADE_COMPARE(sixthDestructed, 0);
    }

    /* The remaining animators should be deleted at destruction */
    CORRADE_COMPARE(firstDestructed, 1);
    CORRADE_COMPARE(secondDestructed, 1);
    CORRADE_COMPARE(fourthDestructed, 1);
    CORRADE_COMPARE(fifthDestructed, 1);
    CORRADE_COMPARE(sixthDestructed, 1);
}

void AbstractUserInterfaceTest::animatorSetInstanceInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct GenericAnimator: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;

        AnimatorFeatures doFeatures() const override { return {}; }
        void doAdvance(Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
    };

    struct GenericDataAnimator: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::DataAttachment;
        }
        void doAdvance(Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
    };

    struct NodeAnimatorWithoutFeature: AbstractNodeAnimator {
        using AbstractNodeAnimator::AbstractNodeAnimator;

        AnimatorFeatures doFeatures() const override { return {}; }
        NodeAnimations doAdvance(Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<NodeFlags>&, Containers::MutableBitArrayView) override {
            return {};
        }
    };

    struct DataAnimator: AbstractDataAnimator {
        using AbstractDataAnimator::AbstractDataAnimator;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::DataAttachment;
        }
    };

    struct DataAnimatorWithoutFeature: AbstractDataAnimator {
        using AbstractDataAnimator::AbstractDataAnimator;

        AnimatorFeatures doFeatures() const override { return {}; }
    };

    struct StyleAnimator: AbstractStyleAnimator {
        using AbstractStyleAnimator::AbstractStyleAnimator;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::DataAttachment;
        }
    };

    struct StyleAnimatorWithoutFeature: AbstractStyleAnimator {
        using AbstractStyleAnimator::AbstractStyleAnimator;

        AnimatorFeatures doFeatures() const override { return {}; }
    };

    AbstractUserInterface ui{{100, 100}};

    AnimatorHandle handle = ui.createAnimator();
    ui.setGenericAnimatorInstance(Containers::pointer<GenericAnimator>(handle));

    std::ostringstream out;
    Error redirectError{&out};
    ui.setGenericAnimatorInstance(nullptr);
    ui.setNodeAnimatorInstance(nullptr);
    ui.setDataAnimatorInstance(nullptr);
    ui.setStyleAnimatorInstance(nullptr);
    ui.setGenericAnimatorInstance(Containers::pointer<GenericAnimator>(AnimatorHandle(0xabcd)));
    ui.setGenericAnimatorInstance(Containers::pointer<GenericAnimator>(handle));
    ui.setGenericAnimatorInstance(Containers::pointer<GenericDataAnimator>(ui.createAnimator()));
    ui.setDataAnimatorInstance(Containers::pointer<DataAnimator>(ui.createAnimator()));
    ui.setStyleAnimatorInstance(Containers::pointer<StyleAnimator>(ui.createAnimator()));
    ui.setNodeAnimatorInstance(Containers::pointer<NodeAnimatorWithoutFeature>(ui.createAnimator()));
    ui.setDataAnimatorInstance(Containers::pointer<DataAnimatorWithoutFeature>(ui.createAnimator()));
    ui.setStyleAnimatorInstance(Containers::pointer<StyleAnimatorWithoutFeature>(ui.createAnimator()));
    CORRADE_COMPARE_AS(out.str(),
        "Whee::AbstractUserInterface::setGenericAnimatorInstance(): instance is null\n"
        "Whee::AbstractUserInterface::setNodeAnimatorInstance(): instance is null\n"
        "Whee::AbstractUserInterface::setDataAnimatorInstance(): instance is null\n"
        "Whee::AbstractUserInterface::setStyleAnimatorInstance(): instance is null\n"
        "Whee::AbstractUserInterface::setGenericAnimatorInstance(): invalid handle Whee::AnimatorHandle(0xcd, 0xab)\n"
        "Whee::AbstractUserInterface::setGenericAnimatorInstance(): instance for Whee::AnimatorHandle(0x0, 0x1) already set\n"
        "Whee::AbstractUserInterface::setGenericAnimatorInstance(): no layer set for a data attachment animator\n"
        "Whee::AbstractUserInterface::setDataAnimatorInstance(): no layer set for a data attachment animator\n"
        "Whee::AbstractUserInterface::setStyleAnimatorInstance(): no layer set for a data attachment animator\n"
        "Whee::AbstractUserInterface::setNodeAnimatorInstance(): Whee::AnimatorFeature::NodeAttachment not advertised for a node animator\n"
        "Whee::AbstractUserInterface::setDataAnimatorInstance(): Whee::AnimatorFeature::DataAttachment not advertised for a data animator\n"
        "Whee::AbstractUserInterface::setStyleAnimatorInstance(): Whee::AnimatorFeature::DataAttachment not advertised for a style animator\n",
        TestSuite::Compare::String);
}

void AbstractUserInterfaceTest::animatorGetInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Animator: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;

        AnimatorFeatures doFeatures() const override { return {}; }
        void doAdvance(Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
    };

    AbstractUserInterface ui{{100, 100}};
    /* Need at least one animator to be present so animator() asserts can
       return something */
    ui.setGenericAnimatorInstance(Containers::pointer<Animator>(ui.createAnimator()));

    const AbstractUserInterface& cui = ui;

    AnimatorHandle handle = ui.createAnimator();

    std::ostringstream out;
    Error redirectError{&out};
    ui.animator(handle);
    ui.animator(AnimatorHandle::Null);
    /* Const overloads */
    cui.animator(handle);
    cui.animator(AnimatorHandle::Null);
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractUserInterface::animator(): Whee::AnimatorHandle(0x1, 0x1) has no instance set\n"
        "Whee::AbstractUserInterface::animator(): invalid handle Whee::AnimatorHandle::Null\n"
        "Whee::AbstractUserInterface::animator(): Whee::AnimatorHandle(0x1, 0x1) has no instance set\n"
        "Whee::AbstractUserInterface::animator(): invalid handle Whee::AnimatorHandle::Null\n");
}

void AbstractUserInterfaceTest::animatorRemoveInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui{{100, 100}};

    std::ostringstream out;
    Error redirectError{&out};
    ui.removeAnimator(AnimatorHandle::Null);
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractUserInterface::removeAnimator(): invalid handle Whee::AnimatorHandle::Null\n");
}

void AbstractUserInterfaceTest::animatorNoHandlesLeft() {
    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui{{100, 100}};

    AnimatorHandle handle;
    for(std::size_t i = 0; i != 1 << Implementation::AnimatorHandleIdBits; ++i)
        handle = ui.createAnimator();
    CORRADE_COMPARE(handle, animatorHandle((1 << Implementation::AnimatorHandleIdBits) - 1, 1));

    CORRADE_COMPARE(ui.animatorCapacity(), 1 << Implementation::AnimatorHandleIdBits);
    CORRADE_COMPARE(ui.animatorUsedCount(), 1 << Implementation::AnimatorHandleIdBits);

    std::ostringstream out;
    Error redirectError{&out};
    ui.createAnimator();
    /* Number is hardcoded in the expected message but not elsewhere in order
       to give a heads-up when modifying the handle ID bit count */
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractUserInterface::createAnimator(): can only have at most 256 animators\n");
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

void AbstractUserInterfaceTest::nodeOrderRoot() {
    /* Tests just ordering of root nodes. See nodeOrderNested() for ordering of
       child nodes as well. */

    AbstractUserInterface ui{{100, 100}};
    CORRADE_COMPARE(ui.nodeOrderFirst(), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLast(), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 0);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 0);

    NodeHandle first = ui.createNode({}, {});
    CORRADE_COMPARE(ui.nodeParent(first), NodeHandle::Null);
    CORRADE_VERIFY(ui.isNodeTopLevel(first));
    CORRADE_VERIFY(ui.isNodeOrdered(first));
    CORRADE_COMPARE(ui.nodeOrderPrevious(first), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(first), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(first), first);
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
    CORRADE_COMPARE(ui.nodeOrderLastNested(first), first);
    CORRADE_VERIFY(ui.isNodeTopLevel(second));
    CORRADE_VERIFY(ui.isNodeOrdered(second));
    CORRADE_COMPARE(ui.nodeOrderPrevious(second), first);
    CORRADE_COMPARE(ui.nodeOrderNext(second), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(second), second);
    CORRADE_COMPARE(ui.nodeOrderFirst(), first);
    CORRADE_COMPARE(ui.nodeOrderLast(), second);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 2);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 2);

    NodeHandle third = ui.createNode(first, {}, {});
    CORRADE_COMPARE(ui.nodeParent(third), first);
    /* Not a root node, so not made top-level nor added to the order by
       default. The original order stays. */
    CORRADE_VERIFY(!ui.isNodeTopLevel(third));
    CORRADE_VERIFY(!ui.isNodeOrdered(third));
    CORRADE_COMPARE(ui.nodeOrderPrevious(third), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(third), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(third), third);
    CORRADE_VERIFY(ui.isNodeOrdered(first));
    CORRADE_COMPARE(ui.nodeOrderPrevious(first), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(first), second);
    CORRADE_COMPARE(ui.nodeOrderLastNested(first), first);
    CORRADE_VERIFY(ui.isNodeOrdered(second));
    CORRADE_COMPARE(ui.nodeOrderPrevious(second), first);
    CORRADE_COMPARE(ui.nodeOrderNext(second), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(second), second);
    CORRADE_COMPARE(ui.nodeOrderFirst(), first);
    CORRADE_COMPARE(ui.nodeOrderLast(), second);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 2);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 2);

    NodeHandle fourth = ui.createNode({}, {});
    CORRADE_COMPARE(ui.nodeParent(fourth), NodeHandle::Null);
    CORRADE_VERIFY(ui.isNodeOrdered(first));
    CORRADE_COMPARE(ui.nodeOrderPrevious(first), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(first), second);
    CORRADE_COMPARE(ui.nodeOrderLastNested(first), first);
    CORRADE_VERIFY(ui.isNodeOrdered(second));
    CORRADE_COMPARE(ui.nodeOrderPrevious(second), first);
    CORRADE_COMPARE(ui.nodeOrderNext(second), fourth);
    CORRADE_COMPARE(ui.nodeOrderLastNested(second), second);
    CORRADE_VERIFY(ui.isNodeTopLevel(fourth));
    CORRADE_VERIFY(ui.isNodeOrdered(fourth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fourth), second);
    CORRADE_COMPARE(ui.nodeOrderNext(fourth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(fourth), fourth);
    CORRADE_COMPARE(ui.nodeOrderFirst(), first);
    CORRADE_COMPARE(ui.nodeOrderLast(), fourth);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 3);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 3);

    NodeHandle fifth = ui.createNode({}, {});
    CORRADE_COMPARE(ui.nodeParent(fifth), NodeHandle::Null);
    CORRADE_VERIFY(ui.isNodeOrdered(first));
    CORRADE_COMPARE(ui.nodeOrderPrevious(first), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(first), second);
    CORRADE_COMPARE(ui.nodeOrderLastNested(first), first);
    CORRADE_VERIFY(ui.isNodeOrdered(second));
    CORRADE_COMPARE(ui.nodeOrderPrevious(second), first);
    CORRADE_COMPARE(ui.nodeOrderNext(second), fourth);
    CORRADE_COMPARE(ui.nodeOrderLastNested(second), second);
    CORRADE_VERIFY(ui.isNodeOrdered(fourth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fourth), second);
    CORRADE_COMPARE(ui.nodeOrderNext(fourth), fifth);
    CORRADE_COMPARE(ui.nodeOrderLastNested(fourth), fourth);
    CORRADE_VERIFY(ui.isNodeTopLevel(fifth));
    CORRADE_VERIFY(ui.isNodeOrdered(fifth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fifth), fourth);
    CORRADE_COMPARE(ui.nodeOrderNext(fifth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(fifth), fifth);
    CORRADE_COMPARE(ui.nodeOrderFirst(), first);
    CORRADE_COMPARE(ui.nodeOrderLast(), fifth);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 4);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 4);

    /* Clearing from a middle of the list. The node stays top-level. */
    ui.clearNodeOrder(second);
    CORRADE_VERIFY(ui.isNodeTopLevel(second));
    CORRADE_VERIFY(!ui.isNodeOrdered(second));
    CORRADE_COMPARE(ui.nodeOrderPrevious(second), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(second), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(second), second);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 4);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 4);
    /* THe rest stays connected */
    CORRADE_VERIFY(ui.isNodeOrdered(first));
    CORRADE_COMPARE(ui.nodeOrderPrevious(first), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(first), fourth);
    CORRADE_COMPARE(ui.nodeOrderLastNested(first), first);
    CORRADE_VERIFY(ui.isNodeOrdered(fourth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fourth), first);
    CORRADE_COMPARE(ui.nodeOrderNext(fourth), fifth);
    CORRADE_COMPARE(ui.nodeOrderLastNested(fourth), fourth);
    CORRADE_VERIFY(ui.isNodeOrdered(fifth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fifth), fourth);
    CORRADE_COMPARE(ui.nodeOrderNext(fifth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(fifth), fifth);
    CORRADE_COMPARE(ui.nodeOrderFirst(), first);
    CORRADE_COMPARE(ui.nodeOrderLast(), fifth);

    /* Clearing from the back of the list. The node stays top-level. */
    ui.clearNodeOrder(first);
    CORRADE_VERIFY(ui.isNodeTopLevel(first));
    CORRADE_VERIFY(!ui.isNodeOrdered(first));
    CORRADE_COMPARE(ui.nodeOrderPrevious(first), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(first), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(first), first);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 4);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 4);
    /* THe rest stays connected */
    CORRADE_VERIFY(ui.isNodeOrdered(fourth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fourth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(fourth), fifth);
    CORRADE_COMPARE(ui.nodeOrderLastNested(fourth), fourth);
    CORRADE_VERIFY(ui.isNodeOrdered(fifth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fifth), fourth);
    CORRADE_COMPARE(ui.nodeOrderNext(fifth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(fifth), fifth);
    CORRADE_COMPARE(ui.nodeOrderFirst(), fourth);
    CORRADE_COMPARE(ui.nodeOrderLast(), fifth);

    /* Clearing from the front of the list. The node stays top-level. */
    ui.clearNodeOrder(fifth);
    CORRADE_VERIFY(ui.isNodeTopLevel(fifth));
    CORRADE_VERIFY(!ui.isNodeOrdered(fifth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fifth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(fifth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(fifth), fifth);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 4);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 4);
    /* THe remaining node stays */
    CORRADE_VERIFY(ui.isNodeOrdered(fourth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fourth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(fourth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(fourth), fourth);
    CORRADE_COMPARE(ui.nodeOrderFirst(), fourth);
    CORRADE_COMPARE(ui.nodeOrderLast(), fourth);

    /* Clearing the last node. The node stays top-level. */
    ui.clearNodeOrder(fourth);
    CORRADE_VERIFY(ui.isNodeTopLevel(fourth));
    CORRADE_VERIFY(!ui.isNodeOrdered(fourth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fourth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(fourth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(fourth), fourth);
    CORRADE_COMPARE(ui.nodeOrderFirst(), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLast(), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 4);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 4);

    /* Clearing a node that isn't connected is a no-op */
    ui.clearNodeOrder(second);
    CORRADE_VERIFY(ui.isNodeTopLevel(second));
    CORRADE_VERIFY(!ui.isNodeOrdered(second));
    CORRADE_COMPARE(ui.nodeOrderPrevious(second), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(second), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(second), second);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 4);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 4);

    /* Setting node order back. There's no other node in the order right now so
       it's both first and last. */
    ui.setNodeOrder(fifth, NodeHandle::Null);
    CORRADE_VERIFY(ui.isNodeTopLevel(fifth));
    CORRADE_VERIFY(ui.isNodeOrdered(fifth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fifth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(fifth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(fifth), fifth);
    CORRADE_COMPARE(ui.nodeOrderFirst(), fifth);
    CORRADE_COMPARE(ui.nodeOrderLast(), fifth);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 4);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 4);

    /* Setting node order as last again, this time it expands a single-item
       list */
    ui.setNodeOrder(second, NodeHandle::Null);
    CORRADE_VERIFY(ui.isNodeOrdered(fifth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fifth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(fifth), second);
    CORRADE_COMPARE(ui.nodeOrderLastNested(fifth), fifth);
    CORRADE_VERIFY(ui.isNodeTopLevel(second));
    CORRADE_VERIFY(ui.isNodeOrdered(second));
    CORRADE_COMPARE(ui.nodeOrderPrevious(second), fifth);
    CORRADE_COMPARE(ui.nodeOrderNext(second), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(second), second);
    CORRADE_COMPARE(ui.nodeOrderFirst(), fifth);
    CORRADE_COMPARE(ui.nodeOrderLast(), second);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 4);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 4);

    /* Setting node order in the middle, just different order than before */
    ui.setNodeOrder(first, second);
    CORRADE_VERIFY(ui.isNodeTopLevel(fifth));
    CORRADE_VERIFY(ui.isNodeOrdered(fifth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fifth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(fifth), first);
    CORRADE_COMPARE(ui.nodeOrderLastNested(fifth), fifth);
    CORRADE_VERIFY(ui.isNodeOrdered(first));
    CORRADE_COMPARE(ui.nodeOrderPrevious(first), fifth);
    CORRADE_COMPARE(ui.nodeOrderNext(first), second);
    CORRADE_COMPARE(ui.nodeOrderLastNested(first), first);
    CORRADE_VERIFY(ui.isNodeOrdered(second));
    CORRADE_COMPARE(ui.nodeOrderPrevious(second), first);
    CORRADE_COMPARE(ui.nodeOrderNext(second), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(second), second);
    CORRADE_COMPARE(ui.nodeOrderFirst(), fifth);
    CORRADE_COMPARE(ui.nodeOrderLast(), second);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 4);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 4);

    /* Setting node order first. This is what was already tested several times
       with the initial node addition, this time it's just with pre-allocated
       capacity, so the next setting would have to grow the capacity again. */
    ui.setNodeOrder(fourth, fifth);
    CORRADE_VERIFY(ui.isNodeTopLevel(fourth));
    CORRADE_VERIFY(ui.isNodeOrdered(fourth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fourth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(fourth), fifth);
    CORRADE_VERIFY(ui.isNodeOrdered(fifth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fifth), fourth);
    CORRADE_COMPARE(ui.nodeOrderNext(fifth), first);
    CORRADE_COMPARE(ui.nodeOrderLastNested(fifth), fifth);
    CORRADE_VERIFY(ui.isNodeOrdered(first));
    CORRADE_COMPARE(ui.nodeOrderPrevious(first), fifth);
    CORRADE_COMPARE(ui.nodeOrderNext(first), second);
    CORRADE_COMPARE(ui.nodeOrderLastNested(first), first);
    CORRADE_VERIFY(ui.isNodeOrdered(second));
    CORRADE_COMPARE(ui.nodeOrderPrevious(second), first);
    CORRADE_COMPARE(ui.nodeOrderNext(second), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(second), second);
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
    CORRADE_COMPARE(ui.nodeOrderLastNested(fourth), fourth);
    CORRADE_VERIFY(ui.isNodeTopLevel(first));
    CORRADE_VERIFY(ui.isNodeOrdered(first));
    CORRADE_COMPARE(ui.nodeOrderPrevious(first), fourth);
    CORRADE_COMPARE(ui.nodeOrderNext(first), fifth);
    CORRADE_COMPARE(ui.nodeOrderLastNested(first), first);
    CORRADE_VERIFY(ui.isNodeOrdered(fifth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fifth), first);
    CORRADE_COMPARE(ui.nodeOrderNext(fifth), second);
    CORRADE_COMPARE(ui.nodeOrderLastNested(fifth), fifth);
    CORRADE_VERIFY(ui.isNodeOrdered(second));
    CORRADE_COMPARE(ui.nodeOrderPrevious(second), fifth);
    CORRADE_COMPARE(ui.nodeOrderNext(second), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(second), second);
    CORRADE_COMPARE(ui.nodeOrderFirst(), fourth);
    CORRADE_COMPARE(ui.nodeOrderLast(), second);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 4);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 4);

    /* Resetting a node from the back to the front ... */
    ui.setNodeOrder(second, fourth);
    CORRADE_VERIFY(ui.isNodeTopLevel(second));
    CORRADE_VERIFY(ui.isNodeOrdered(second));
    CORRADE_COMPARE(ui.nodeOrderPrevious(second), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(second), fourth);
    CORRADE_COMPARE(ui.nodeOrderLastNested(second), second);
    CORRADE_VERIFY(ui.isNodeOrdered(fourth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fourth), second);
    CORRADE_COMPARE(ui.nodeOrderNext(fourth), first);
    CORRADE_COMPARE(ui.nodeOrderLastNested(fourth), fourth);
    CORRADE_VERIFY(ui.isNodeOrdered(first));
    CORRADE_COMPARE(ui.nodeOrderPrevious(first), fourth);
    CORRADE_COMPARE(ui.nodeOrderNext(first), fifth);
    CORRADE_COMPARE(ui.nodeOrderLastNested(first), first);
    CORRADE_VERIFY(ui.isNodeOrdered(fifth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fifth), first);
    CORRADE_COMPARE(ui.nodeOrderNext(fifth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(fifth), fifth);
    CORRADE_COMPARE(ui.nodeOrderFirst(), second);
    CORRADE_COMPARE(ui.nodeOrderLast(), fifth);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 4);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 4);

    /* ... and back, results in the same order as before */
    ui.setNodeOrder(second, NodeHandle::Null);
    CORRADE_VERIFY(ui.isNodeOrdered(fourth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fourth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(fourth), first);
    CORRADE_COMPARE(ui.nodeOrderLastNested(fourth), fourth);
    CORRADE_VERIFY(ui.isNodeOrdered(first));
    CORRADE_COMPARE(ui.nodeOrderPrevious(first), fourth);
    CORRADE_COMPARE(ui.nodeOrderNext(first), fifth);
    CORRADE_COMPARE(ui.nodeOrderLastNested(first), first);
    CORRADE_VERIFY(ui.isNodeOrdered(fifth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fifth), first);
    CORRADE_COMPARE(ui.nodeOrderNext(fifth), second);
    CORRADE_COMPARE(ui.nodeOrderLastNested(fifth), fifth);
    CORRADE_VERIFY(ui.isNodeTopLevel(second));
    CORRADE_VERIFY(ui.isNodeOrdered(second));
    CORRADE_COMPARE(ui.nodeOrderPrevious(second), fifth);
    CORRADE_COMPARE(ui.nodeOrderNext(second), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(second), second);
    CORRADE_COMPARE(ui.nodeOrderFirst(), fourth);
    CORRADE_COMPARE(ui.nodeOrderLast(), second);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 4);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 4);

    /* Adding a new node grows the capacity again */
    NodeHandle sixth = ui.createNode({}, {});
    CORRADE_VERIFY(ui.isNodeOrdered(fourth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fourth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(fourth), first);
    CORRADE_COMPARE(ui.nodeOrderLastNested(fourth), fourth);
    CORRADE_VERIFY(ui.isNodeOrdered(first));
    CORRADE_COMPARE(ui.nodeOrderPrevious(first), fourth);
    CORRADE_COMPARE(ui.nodeOrderNext(first), fifth);
    CORRADE_COMPARE(ui.nodeOrderLastNested(first), first);
    CORRADE_VERIFY(ui.isNodeOrdered(fifth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fifth), first);
    CORRADE_COMPARE(ui.nodeOrderNext(fifth), second);
    CORRADE_COMPARE(ui.nodeOrderLastNested(fifth), fifth);
    CORRADE_VERIFY(ui.isNodeOrdered(second));
    CORRADE_COMPARE(ui.nodeOrderPrevious(second), fifth);
    CORRADE_COMPARE(ui.nodeOrderNext(second), sixth);
    CORRADE_COMPARE(ui.nodeOrderLastNested(second), second);
    CORRADE_VERIFY(ui.isNodeTopLevel(sixth));
    CORRADE_VERIFY(ui.isNodeOrdered(sixth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(sixth), second);
    CORRADE_COMPARE(ui.nodeOrderNext(sixth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(sixth), sixth);
    CORRADE_COMPARE(ui.nodeOrderFirst(), fourth);
    CORRADE_COMPARE(ui.nodeOrderLast(), sixth);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 5);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 5);

    /* Removing a node implicitly calls clearNodeOrder(), and also frees the
       order slot */
    ui.removeNode(first);
    CORRADE_VERIFY(ui.isNodeOrdered(fourth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fourth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(fourth), fifth);
    CORRADE_COMPARE(ui.nodeOrderLastNested(fourth), fourth);
    CORRADE_VERIFY(ui.isNodeOrdered(fifth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fifth), fourth);
    CORRADE_COMPARE(ui.nodeOrderNext(fifth), second);
    CORRADE_COMPARE(ui.nodeOrderLastNested(fifth), fifth);
    CORRADE_VERIFY(ui.isNodeOrdered(second));
    CORRADE_COMPARE(ui.nodeOrderPrevious(second), fifth);
    CORRADE_COMPARE(ui.nodeOrderNext(second), sixth);
    CORRADE_COMPARE(ui.nodeOrderLastNested(second), second);
    CORRADE_VERIFY(ui.isNodeOrdered(sixth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(sixth), second);
    CORRADE_COMPARE(ui.nodeOrderNext(sixth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(sixth), sixth);
    CORRADE_COMPARE(ui.nodeOrderFirst(), fourth);
    CORRADE_COMPARE(ui.nodeOrderLast(), sixth);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 5);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 4);

    /* Verify also removing a node that had the order explicitly cleared
       before */
    ui.clearNodeOrder(fifth);
    ui.removeNode(fifth);
    CORRADE_VERIFY(ui.isNodeOrdered(fourth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fourth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(fourth), second);
    CORRADE_COMPARE(ui.nodeOrderLastNested(fourth), fourth);
    CORRADE_VERIFY(ui.isNodeOrdered(second));
    CORRADE_COMPARE(ui.nodeOrderPrevious(second), fourth);
    CORRADE_COMPARE(ui.nodeOrderNext(second), sixth);
    CORRADE_COMPARE(ui.nodeOrderLastNested(second), second);
    CORRADE_VERIFY(ui.isNodeOrdered(sixth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(sixth), second);
    CORRADE_COMPARE(ui.nodeOrderNext(sixth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(sixth), sixth);
    CORRADE_COMPARE(ui.nodeOrderFirst(), fourth);
    CORRADE_COMPARE(ui.nodeOrderLast(), sixth);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 5);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 3);

    /* Adding a new node reuses the freed capacity */
    NodeHandle seventh = ui.createNode({}, {});
    CORRADE_VERIFY(ui.isNodeOrdered(fourth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(fourth), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(fourth), second);
    CORRADE_COMPARE(ui.nodeOrderLastNested(fourth), fourth);
    CORRADE_VERIFY(ui.isNodeOrdered(second));
    CORRADE_COMPARE(ui.nodeOrderPrevious(second), fourth);
    CORRADE_COMPARE(ui.nodeOrderNext(second), sixth);
    CORRADE_COMPARE(ui.nodeOrderLastNested(second), second);
    CORRADE_VERIFY(ui.isNodeOrdered(sixth));
    CORRADE_COMPARE(ui.nodeOrderPrevious(sixth), second);
    CORRADE_COMPARE(ui.nodeOrderNext(sixth), seventh);
    CORRADE_COMPARE(ui.nodeOrderLastNested(sixth), sixth);
    CORRADE_VERIFY(ui.isNodeTopLevel(seventh));
    CORRADE_VERIFY(ui.isNodeOrdered(seventh));
    CORRADE_COMPARE(ui.nodeOrderPrevious(seventh), sixth);
    CORRADE_COMPARE(ui.nodeOrderNext(seventh), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(seventh), seventh);
    CORRADE_COMPARE(ui.nodeOrderFirst(), fourth);
    CORRADE_COMPARE(ui.nodeOrderLast(), seventh);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 5);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 4);
}

void AbstractUserInterfaceTest::nodeOrderNested() {
    /* Compared to nodeOrderRoot() tests especially handling of non-root
       top-level nodes */

    AbstractUserInterface ui{{100, 100}};

    /* Three root nodes, two being in order */
    NodeHandle rootA = ui.createNode({}, {});
    NodeHandle rootB = ui.createNode({}, {});
    NodeHandle rootCNotInOrder = ui.createNode({}, {});
    ui.clearNodeOrder(rootCNotInOrder);
    CORRADE_VERIFY(ui.isNodeTopLevel(rootA));
    CORRADE_VERIFY(ui.isNodeOrdered(rootA));
    CORRADE_COMPARE(ui.nodeOrderPrevious(rootA), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(rootA), rootB);
    CORRADE_COMPARE(ui.nodeOrderLastNested(rootA), rootA);
    CORRADE_VERIFY(ui.isNodeTopLevel(rootB));
    CORRADE_VERIFY(ui.isNodeOrdered(rootB));
    CORRADE_COMPARE(ui.nodeOrderPrevious(rootB), rootA);
    CORRADE_COMPARE(ui.nodeOrderNext(rootB), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(rootB), rootB);
    CORRADE_VERIFY(ui.isNodeTopLevel(rootCNotInOrder));
    CORRADE_VERIFY(!ui.isNodeOrdered(rootCNotInOrder));
    CORRADE_COMPARE(ui.nodeOrderPrevious(rootCNotInOrder), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(rootCNotInOrder), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(rootCNotInOrder), rootCNotInOrder);
    CORRADE_COMPARE(ui.nodeOrderFirst(), rootA);
    CORRADE_COMPARE(ui.nodeOrderLast(), rootB);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 3);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 3);

    /* A nested tree with a grandchild being a top-level node */
    NodeHandle childA = ui.createNode(rootA, {}, {});
    NodeHandle child2A1 = ui.createNode(childA, {}, {});
    NodeHandle child3A1 = ui.createNode(child2A1, {}, {});
    NodeHandle child4A1TopLevel = ui.createNode(child3A1, {}, {});
    ui.setNodeOrder(child4A1TopLevel, NodeHandle::Null);
    CORRADE_VERIFY(ui.isNodeOrdered(rootA));
    CORRADE_COMPARE(ui.nodeOrderPrevious(rootA), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(rootA), child4A1TopLevel);
    /* This now includes the child */
    CORRADE_COMPARE(ui.nodeOrderLastNested(rootA), child4A1TopLevel);
    CORRADE_VERIFY(!ui.isNodeTopLevel(childA));
    CORRADE_VERIFY(!ui.isNodeTopLevel(child2A1));
    CORRADE_VERIFY(!ui.isNodeTopLevel(child3A1));
    CORRADE_VERIFY(ui.isNodeTopLevel(child4A1TopLevel));
    CORRADE_VERIFY(ui.isNodeOrdered(child4A1TopLevel));
    CORRADE_COMPARE(ui.nodeOrderPrevious(child4A1TopLevel), rootA);
    CORRADE_COMPARE(ui.nodeOrderNext(child4A1TopLevel), rootB);
    CORRADE_COMPARE(ui.nodeOrderLastNested(child4A1TopLevel), child4A1TopLevel);
    CORRADE_VERIFY(ui.isNodeOrdered(rootB));
    CORRADE_COMPARE(ui.nodeOrderPrevious(rootB), child4A1TopLevel);
    CORRADE_COMPARE(ui.nodeOrderNext(rootB), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(rootB), rootB);
    CORRADE_COMPARE(ui.nodeOrderFirst(), rootA);
    CORRADE_COMPARE(ui.nodeOrderLast(), rootB);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 4);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 4);

    /* A nested tree with a child in a root node that isn't top-level doesn't
       affect the actual visible order but is still reported as ordered within
       its parent */
    NodeHandle childC = ui.createNode(rootCNotInOrder, {}, {});
    NodeHandle child2CTopLevel = ui.createNode(childC, {}, {});
    ui.setNodeOrder(child2CTopLevel, NodeHandle::Null);
    CORRADE_VERIFY(ui.isNodeOrdered(rootA));
    CORRADE_COMPARE(ui.nodeOrderPrevious(rootA), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(rootA), child4A1TopLevel);
    CORRADE_COMPARE(ui.nodeOrderLastNested(rootA), child4A1TopLevel);
    CORRADE_VERIFY(ui.isNodeOrdered(child4A1TopLevel));
    CORRADE_COMPARE(ui.nodeOrderPrevious(child4A1TopLevel), rootA);
    CORRADE_COMPARE(ui.nodeOrderNext(child4A1TopLevel), rootB);
    CORRADE_COMPARE(ui.nodeOrderLastNested(child4A1TopLevel), child4A1TopLevel);
    CORRADE_VERIFY(ui.isNodeOrdered(rootB));
    CORRADE_COMPARE(ui.nodeOrderPrevious(rootB), child4A1TopLevel);
    CORRADE_COMPARE(ui.nodeOrderNext(rootB), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(rootB), rootB);
    CORRADE_COMPARE(ui.nodeOrderFirst(), rootA);
    CORRADE_COMPARE(ui.nodeOrderLast(), rootB);
    CORRADE_VERIFY(!ui.isNodeOrdered(rootCNotInOrder));
    CORRADE_COMPARE(ui.nodeOrderPrevious(rootCNotInOrder), NodeHandle::Null);
    /* This now includes the child */
    CORRADE_COMPARE(ui.nodeOrderNext(rootCNotInOrder), child2CTopLevel);
    CORRADE_COMPARE(ui.nodeOrderLastNested(rootCNotInOrder), child2CTopLevel);
    CORRADE_VERIFY(!ui.isNodeTopLevel(childC));
    CORRADE_VERIFY(ui.isNodeTopLevel(child2CTopLevel));
    /* Reported as ordered because it is, relative to the parent, but is not
       actually visible */
    CORRADE_VERIFY(ui.isNodeOrdered(child2CTopLevel));
    CORRADE_COMPARE(ui.nodeOrderPrevious(child2CTopLevel), rootCNotInOrder);
    CORRADE_COMPARE(ui.nodeOrderNext(child2CTopLevel), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(child2CTopLevel), child2CTopLevel);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 5);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 5);

    /* Clearing the order of a node that is only connected to its parent but no
       (lastNested's) next node should work. The other case (connected only to
       (lastNested's) next but not previous) cannot happen. */
    ui.clearNodeOrder(child2CTopLevel);
    CORRADE_COMPARE(ui.nodeOrderFirst(), rootA);
    CORRADE_COMPARE(ui.nodeOrderLast(), rootB);
    CORRADE_VERIFY(!ui.isNodeOrdered(rootCNotInOrder));
    CORRADE_COMPARE(ui.nodeOrderPrevious(rootCNotInOrder), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(rootCNotInOrder), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(rootCNotInOrder), rootCNotInOrder);
    /* Stays top-level even though not root */
    CORRADE_VERIFY(ui.isNodeTopLevel(child2CTopLevel));
    CORRADE_VERIFY(!ui.isNodeOrdered(child2CTopLevel));
    CORRADE_COMPARE(ui.nodeOrderPrevious(child2CTopLevel), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(child2CTopLevel), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(child2CTopLevel), child2CTopLevel);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 5);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 5);

    /* Putting it back results in the same state as above, again the total
       order isn't affected because the root code isn't connected */
    ui.setNodeOrder(child2CTopLevel, NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderFirst(), rootA);
    CORRADE_COMPARE(ui.nodeOrderLast(), rootB);
    CORRADE_VERIFY(!ui.isNodeOrdered(rootCNotInOrder));
    CORRADE_COMPARE(ui.nodeOrderPrevious(rootCNotInOrder), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(rootCNotInOrder), child2CTopLevel);
    CORRADE_COMPARE(ui.nodeOrderLastNested(rootCNotInOrder), child2CTopLevel);
    CORRADE_VERIFY(ui.isNodeTopLevel(child2CTopLevel));
    CORRADE_VERIFY(ui.isNodeOrdered(child2CTopLevel));
    CORRADE_COMPARE(ui.nodeOrderPrevious(child2CTopLevel), rootCNotInOrder);
    CORRADE_COMPARE(ui.nodeOrderNext(child2CTopLevel), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(child2CTopLevel), child2CTopLevel);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 5);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 5);

    /* Another grandchild being a top-level node, inserted before the other. It
       has a child which isn't a top-level node. */
    NodeHandle child2A2 = ui.createNode(childA, {}, {});
    NodeHandle child3A2TopLevel = ui.createNode(child2A2, {}, {});
    NodeHandle child4A2 = ui.createNode(child3A2TopLevel, {}, {});
    ui.setNodeOrder(child3A2TopLevel, child4A1TopLevel);
    CORRADE_VERIFY(ui.isNodeOrdered(rootA));
    CORRADE_COMPARE(ui.nodeOrderPrevious(rootA), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(rootA), child3A2TopLevel);
    /* This now includes both children, the new subChildA2TopLevel is first */
    CORRADE_COMPARE(ui.nodeOrderLastNested(rootA), child4A1TopLevel);
    CORRADE_VERIFY(!ui.isNodeTopLevel(child2A2));
    CORRADE_VERIFY(ui.isNodeTopLevel(child3A2TopLevel));
    CORRADE_VERIFY(ui.isNodeOrdered(child3A2TopLevel));
    CORRADE_COMPARE(ui.nodeOrderPrevious(child3A2TopLevel), rootA);
    CORRADE_COMPARE(ui.nodeOrderNext(child3A2TopLevel), child4A1TopLevel);
    CORRADE_COMPARE(ui.nodeOrderLastNested(child3A2TopLevel), child3A2TopLevel);
    CORRADE_VERIFY(!ui.isNodeTopLevel(child4A2));
    CORRADE_VERIFY(!ui.isNodeOrdered(child4A2));
    CORRADE_COMPARE(ui.nodeOrderPrevious(child4A2), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(child4A2), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(child4A2), child4A2);
    CORRADE_VERIFY(ui.isNodeOrdered(child4A1TopLevel));
    CORRADE_COMPARE(ui.nodeOrderPrevious(child4A1TopLevel), child3A2TopLevel);
    CORRADE_COMPARE(ui.nodeOrderNext(child4A1TopLevel), rootB);
    CORRADE_COMPARE(ui.nodeOrderLastNested(child4A1TopLevel), child4A1TopLevel);
    CORRADE_VERIFY(ui.isNodeOrdered(rootB));
    CORRADE_COMPARE(ui.nodeOrderPrevious(rootB), child4A1TopLevel);
    CORRADE_COMPARE(ui.nodeOrderNext(rootB), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(rootB), rootB);
    CORRADE_COMPARE(ui.nodeOrderFirst(), rootA);
    CORRADE_COMPARE(ui.nodeOrderLast(), rootB);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 6);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 6);

    /* A further-nested top-level node, this time immediately under the other
       top-level node */
    NodeHandle child5A1TopLevel = ui.createNode(child4A1TopLevel, {}, {});
    ui.setNodeOrder(child5A1TopLevel, NodeHandle::Null);
    CORRADE_VERIFY(ui.isNodeOrdered(rootA));
    CORRADE_COMPARE(ui.nodeOrderPrevious(rootA), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(rootA), child3A2TopLevel);
    /* This now (also) includes the nested top-level node */
    CORRADE_COMPARE(ui.nodeOrderLastNested(rootA), child5A1TopLevel);
    CORRADE_VERIFY(ui.isNodeOrdered(child3A2TopLevel));
    CORRADE_COMPARE(ui.nodeOrderPrevious(child3A2TopLevel), rootA);
    CORRADE_COMPARE(ui.nodeOrderNext(child3A2TopLevel), child4A1TopLevel);
    CORRADE_COMPARE(ui.nodeOrderLastNested(child3A2TopLevel), child3A2TopLevel);
    CORRADE_VERIFY(ui.isNodeOrdered(child4A1TopLevel));
    CORRADE_COMPARE(ui.nodeOrderPrevious(child4A1TopLevel), child3A2TopLevel);
    CORRADE_COMPARE(ui.nodeOrderNext(child4A1TopLevel), child5A1TopLevel);
    /* This now includes the nested top-level node */
    CORRADE_COMPARE(ui.nodeOrderLastNested(child4A1TopLevel), child5A1TopLevel);
    CORRADE_VERIFY(ui.isNodeTopLevel(child5A1TopLevel));
    CORRADE_VERIFY(ui.isNodeOrdered(child5A1TopLevel));
    CORRADE_COMPARE(ui.nodeOrderPrevious(child5A1TopLevel), child4A1TopLevel);
    CORRADE_COMPARE(ui.nodeOrderNext(child5A1TopLevel), rootB);
    CORRADE_COMPARE(ui.nodeOrderLastNested(child5A1TopLevel), child5A1TopLevel);
    CORRADE_VERIFY(ui.isNodeOrdered(rootB));
    CORRADE_COMPARE(ui.nodeOrderPrevious(rootB), child5A1TopLevel);
    CORRADE_COMPARE(ui.nodeOrderNext(rootB), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(rootB), rootB);
    CORRADE_COMPARE(ui.nodeOrderFirst(), rootA);
    CORRADE_COMPARE(ui.nodeOrderLast(), rootB);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 7);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 7);

    /* Clearing the order disconnects the children as well but keeps them
       alongside the node */
    ui.clearNodeOrder(child4A1TopLevel);
    CORRADE_VERIFY(ui.isNodeOrdered(rootA));
    CORRADE_COMPARE(ui.nodeOrderPrevious(rootA), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(rootA), child3A2TopLevel);
    /* subChildA1TopLevel and subSubChildA1TopLevel are no longer under this
       node */
    CORRADE_COMPARE(ui.nodeOrderLastNested(rootA), child3A2TopLevel);
    CORRADE_VERIFY(ui.isNodeOrdered(child3A2TopLevel));
    CORRADE_COMPARE(ui.nodeOrderPrevious(child3A2TopLevel), rootA);
    /* Neither after subChildA2TopLevel */
    CORRADE_COMPARE(ui.nodeOrderNext(child3A2TopLevel), rootB);
    CORRADE_COMPARE(ui.nodeOrderLastNested(child3A2TopLevel), child3A2TopLevel);
    /* The children are remembered, but the hierarchy as a whole is
       disconnected */
    CORRADE_VERIFY(ui.isNodeTopLevel(child4A1TopLevel));
    CORRADE_VERIFY(!ui.isNodeOrdered(child4A1TopLevel));
    CORRADE_COMPARE(ui.nodeOrderPrevious(child4A1TopLevel), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(child4A1TopLevel), child5A1TopLevel);
    CORRADE_COMPARE(ui.nodeOrderLastNested(child4A1TopLevel), child5A1TopLevel);
    CORRADE_VERIFY(ui.isNodeOrdered(child5A1TopLevel));
    CORRADE_COMPARE(ui.nodeOrderPrevious(child5A1TopLevel), child4A1TopLevel);
    CORRADE_COMPARE(ui.nodeOrderNext(child5A1TopLevel), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(child5A1TopLevel), child5A1TopLevel);
    CORRADE_VERIFY(ui.isNodeOrdered(rootB));
    CORRADE_COMPARE(ui.nodeOrderPrevious(rootB), child3A2TopLevel);
    CORRADE_COMPARE(ui.nodeOrderNext(rootB), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(rootB), rootB);
    CORRADE_COMPARE(ui.nodeOrderFirst(), rootA);
    CORRADE_COMPARE(ui.nodeOrderLast(), rootB);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 7);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 7);

    /* Clearing the order again is a no-op, resulting in the same state, same
       with a node that's not top-level. Flattening an order of a non-top-level
       node is also no-op. */
    ui.clearNodeOrder(child4A1TopLevel);
    ui.clearNodeOrder(child2A1);
    ui.flattenNodeOrder(child2A1);
    CORRADE_VERIFY(ui.isNodeOrdered(rootA));
    CORRADE_COMPARE(ui.nodeOrderPrevious(rootA), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(rootA), child3A2TopLevel);
    CORRADE_COMPARE(ui.nodeOrderLastNested(rootA), child3A2TopLevel);
    CORRADE_VERIFY(!ui.isNodeTopLevel(child2A1));
    CORRADE_VERIFY(ui.isNodeOrdered(child3A2TopLevel));
    CORRADE_COMPARE(ui.nodeOrderPrevious(child3A2TopLevel), rootA);
    CORRADE_COMPARE(ui.nodeOrderNext(child3A2TopLevel), rootB);
    CORRADE_COMPARE(ui.nodeOrderLastNested(child3A2TopLevel), child3A2TopLevel);
    CORRADE_VERIFY(ui.isNodeTopLevel(child4A1TopLevel));
    CORRADE_VERIFY(!ui.isNodeOrdered(child4A1TopLevel));
    CORRADE_COMPARE(ui.nodeOrderPrevious(child4A1TopLevel), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(child4A1TopLevel), child5A1TopLevel);
    CORRADE_COMPARE(ui.nodeOrderLastNested(child4A1TopLevel), child5A1TopLevel);
    CORRADE_VERIFY(ui.isNodeOrdered(child5A1TopLevel));
    CORRADE_COMPARE(ui.nodeOrderPrevious(child5A1TopLevel), child4A1TopLevel);
    CORRADE_COMPARE(ui.nodeOrderNext(child5A1TopLevel), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(child5A1TopLevel), child5A1TopLevel);
    CORRADE_VERIFY(ui.isNodeOrdered(rootB));
    CORRADE_COMPARE(ui.nodeOrderPrevious(rootB), child3A2TopLevel);
    CORRADE_COMPARE(ui.nodeOrderNext(rootB), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(rootB), rootB);
    CORRADE_COMPARE(ui.nodeOrderFirst(), rootA);
    CORRADE_COMPARE(ui.nodeOrderLast(), rootB);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 7);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 7);

    /* Setting the order back to the same place should result in the same state
       as before */
    ui.setNodeOrder(child4A1TopLevel, NodeHandle::Null);
    CORRADE_VERIFY(ui.isNodeOrdered(rootA));
    CORRADE_COMPARE(ui.nodeOrderPrevious(rootA), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(rootA), child3A2TopLevel);
    CORRADE_COMPARE(ui.nodeOrderLastNested(rootA), child5A1TopLevel);
    CORRADE_VERIFY(ui.isNodeOrdered(child3A2TopLevel));
    CORRADE_COMPARE(ui.nodeOrderPrevious(child3A2TopLevel), rootA);
    CORRADE_COMPARE(ui.nodeOrderNext(child3A2TopLevel), child4A1TopLevel);
    CORRADE_COMPARE(ui.nodeOrderLastNested(child3A2TopLevel), child3A2TopLevel);
    CORRADE_VERIFY(ui.isNodeTopLevel(child4A1TopLevel));
    CORRADE_VERIFY(ui.isNodeOrdered(child4A1TopLevel));
    CORRADE_COMPARE(ui.nodeOrderPrevious(child4A1TopLevel), child3A2TopLevel);
    CORRADE_COMPARE(ui.nodeOrderNext(child4A1TopLevel), child5A1TopLevel);
    CORRADE_COMPARE(ui.nodeOrderLastNested(child4A1TopLevel), child5A1TopLevel);
    CORRADE_VERIFY(ui.isNodeOrdered(child5A1TopLevel));
    CORRADE_COMPARE(ui.nodeOrderPrevious(child5A1TopLevel), child4A1TopLevel);
    CORRADE_COMPARE(ui.nodeOrderNext(child5A1TopLevel), rootB);
    CORRADE_COMPARE(ui.nodeOrderLastNested(child5A1TopLevel), child5A1TopLevel);
    CORRADE_VERIFY(ui.isNodeOrdered(rootB));
    CORRADE_COMPARE(ui.nodeOrderPrevious(rootB), child5A1TopLevel);
    CORRADE_COMPARE(ui.nodeOrderNext(rootB), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(rootB), rootB);
    CORRADE_COMPARE(ui.nodeOrderFirst(), rootA);
    CORRADE_COMPARE(ui.nodeOrderLast(), rootB);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 7);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 7);

    /* Clearing a top-level order includes all children as well */
    ui.clearNodeOrder(rootA);
    CORRADE_VERIFY(ui.isNodeTopLevel(rootA));
    CORRADE_VERIFY(!ui.isNodeOrdered(rootA));
    CORRADE_COMPARE(ui.nodeOrderPrevious(rootA), NodeHandle::Null);
    /* The children are remembered, but the hierarchy as a whole is
       disconnected */
    CORRADE_COMPARE(ui.nodeOrderNext(rootA), child3A2TopLevel);
    CORRADE_COMPARE(ui.nodeOrderLastNested(rootA), child5A1TopLevel);
    /* The children themselves *are* still ordered underneath */
    CORRADE_VERIFY(ui.isNodeOrdered(child3A2TopLevel));
    CORRADE_COMPARE(ui.nodeOrderPrevious(child3A2TopLevel), rootA);
    CORRADE_COMPARE(ui.nodeOrderNext(child3A2TopLevel), child4A1TopLevel);
    CORRADE_COMPARE(ui.nodeOrderLastNested(child3A2TopLevel), child3A2TopLevel);
    CORRADE_VERIFY(ui.isNodeOrdered(child4A1TopLevel));
    CORRADE_COMPARE(ui.nodeOrderPrevious(child4A1TopLevel), child3A2TopLevel);
    CORRADE_COMPARE(ui.nodeOrderNext(child4A1TopLevel), child5A1TopLevel);
    CORRADE_COMPARE(ui.nodeOrderLastNested(child4A1TopLevel), child5A1TopLevel);
    CORRADE_VERIFY(ui.isNodeOrdered(child5A1TopLevel));
    CORRADE_COMPARE(ui.nodeOrderPrevious(child5A1TopLevel), child4A1TopLevel);
    CORRADE_COMPARE(ui.nodeOrderNext(child5A1TopLevel), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(child5A1TopLevel), child5A1TopLevel);
    CORRADE_VERIFY(ui.isNodeOrdered(rootB));
    CORRADE_COMPARE(ui.nodeOrderPrevious(rootB), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(rootB), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(rootB), rootB);
    CORRADE_COMPARE(ui.nodeOrderFirst(), rootB);
    CORRADE_COMPARE(ui.nodeOrderLast(), rootB);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 7);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 7);

    /* Setting the order back to the same place should result in the same state
       as before */
    ui.setNodeOrder(rootA, rootB);
    CORRADE_VERIFY(ui.isNodeTopLevel(rootA));
    CORRADE_VERIFY(ui.isNodeOrdered(rootA));
    CORRADE_COMPARE(ui.nodeOrderPrevious(rootA), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(rootA), child3A2TopLevel);
    CORRADE_COMPARE(ui.nodeOrderLastNested(rootA), child5A1TopLevel);
    CORRADE_VERIFY(ui.isNodeOrdered(child3A2TopLevel));
    CORRADE_COMPARE(ui.nodeOrderPrevious(child3A2TopLevel), rootA);
    CORRADE_COMPARE(ui.nodeOrderNext(child3A2TopLevel), child4A1TopLevel);
    CORRADE_COMPARE(ui.nodeOrderLastNested(child3A2TopLevel), child3A2TopLevel);
    CORRADE_VERIFY(ui.isNodeOrdered(child4A1TopLevel));
    CORRADE_COMPARE(ui.nodeOrderPrevious(child4A1TopLevel), child3A2TopLevel);
    CORRADE_COMPARE(ui.nodeOrderNext(child4A1TopLevel), child5A1TopLevel);
    CORRADE_COMPARE(ui.nodeOrderLastNested(child4A1TopLevel), child5A1TopLevel);
    CORRADE_VERIFY(ui.isNodeOrdered(child5A1TopLevel));
    CORRADE_COMPARE(ui.nodeOrderPrevious(child5A1TopLevel), child4A1TopLevel);
    CORRADE_COMPARE(ui.nodeOrderNext(child5A1TopLevel), rootB);
    CORRADE_COMPARE(ui.nodeOrderLastNested(child5A1TopLevel), child5A1TopLevel);
    CORRADE_VERIFY(ui.isNodeOrdered(rootB));
    CORRADE_COMPARE(ui.nodeOrderPrevious(rootB), child5A1TopLevel);
    CORRADE_COMPARE(ui.nodeOrderNext(rootB), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(rootB), rootB);
    CORRADE_COMPARE(ui.nodeOrderFirst(), rootA);
    CORRADE_COMPARE(ui.nodeOrderLast(), rootB);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 7);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 7);

    /* Changing a top-level order moves all its child top-level nodes as
       well */
    ui.setNodeOrder(child4A1TopLevel, child3A2TopLevel);
    CORRADE_VERIFY(ui.isNodeOrdered(rootA));
    CORRADE_COMPARE(ui.nodeOrderPrevious(rootA), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(rootA), child4A1TopLevel);
    CORRADE_COMPARE(ui.nodeOrderLastNested(rootA), child3A2TopLevel);
    CORRADE_VERIFY(ui.isNodeTopLevel(child4A1TopLevel));
    CORRADE_VERIFY(ui.isNodeOrdered(child4A1TopLevel));
    CORRADE_COMPARE(ui.nodeOrderPrevious(child4A1TopLevel), rootA);
    CORRADE_COMPARE(ui.nodeOrderNext(child4A1TopLevel), child5A1TopLevel);
    CORRADE_COMPARE(ui.nodeOrderLastNested(child4A1TopLevel), child5A1TopLevel);
    CORRADE_VERIFY(ui.isNodeOrdered(child5A1TopLevel));
    CORRADE_COMPARE(ui.nodeOrderPrevious(child5A1TopLevel), child4A1TopLevel);
    CORRADE_COMPARE(ui.nodeOrderNext(child5A1TopLevel), child3A2TopLevel);
    CORRADE_COMPARE(ui.nodeOrderLastNested(child5A1TopLevel), child5A1TopLevel);
    CORRADE_VERIFY(ui.isNodeOrdered(child3A2TopLevel));
    CORRADE_COMPARE(ui.nodeOrderPrevious(child3A2TopLevel), child5A1TopLevel);
    CORRADE_COMPARE(ui.nodeOrderNext(child3A2TopLevel), rootB);
    CORRADE_COMPARE(ui.nodeOrderLastNested(child3A2TopLevel), child3A2TopLevel);
    CORRADE_VERIFY(ui.isNodeOrdered(rootB));
    CORRADE_COMPARE(ui.nodeOrderPrevious(rootB), child3A2TopLevel);
    CORRADE_COMPARE(ui.nodeOrderNext(rootB), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(rootB), rootB);
    CORRADE_COMPARE(ui.nodeOrderFirst(), rootA);
    CORRADE_COMPARE(ui.nodeOrderLast(), rootB);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 7);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 7);

    /* It's possible to create a new top-level node with nested top-level nodes
       if their order is cleared */
    ui.clearNodeOrder(child4A1TopLevel);
    ui.setNodeOrder(child3A1, child3A2TopLevel);
    CORRADE_VERIFY(ui.isNodeOrdered(rootA));
    CORRADE_COMPARE(ui.nodeOrderPrevious(rootA), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(rootA), child3A1);
    CORRADE_COMPARE(ui.nodeOrderLastNested(rootA), child3A2TopLevel);
    CORRADE_VERIFY(ui.isNodeOrdered(child3A1));
    CORRADE_COMPARE(ui.nodeOrderPrevious(child3A1), rootA);
    CORRADE_COMPARE(ui.nodeOrderNext(child3A1), child3A2TopLevel);
    /* This doesn't include the now-cleared nested orders */
    CORRADE_COMPARE(ui.nodeOrderLastNested(child3A1), child3A1);
    CORRADE_VERIFY(ui.isNodeOrdered(child3A2TopLevel));
    CORRADE_COMPARE(ui.nodeOrderPrevious(child3A2TopLevel), child3A1);
    CORRADE_COMPARE(ui.nodeOrderNext(child3A2TopLevel), rootB);
    CORRADE_COMPARE(ui.nodeOrderLastNested(child3A2TopLevel), child3A2TopLevel);
    CORRADE_VERIFY(ui.isNodeTopLevel(child4A1TopLevel));
    CORRADE_VERIFY(!ui.isNodeOrdered(child4A1TopLevel));
    CORRADE_VERIFY(ui.isNodeOrdered(rootB));
    CORRADE_COMPARE(ui.nodeOrderPrevious(rootB), child3A2TopLevel);
    CORRADE_COMPARE(ui.nodeOrderNext(rootB), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(rootB), rootB);
    CORRADE_COMPARE(ui.nodeOrderFirst(), rootA);
    CORRADE_COMPARE(ui.nodeOrderLast(), rootB);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 8);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 8);

    /* Setting the nested order back will make it appear under the new node */
    ui.setNodeOrder(child4A1TopLevel, NodeHandle::Null);
    CORRADE_VERIFY(ui.isNodeOrdered(rootA));
    CORRADE_COMPARE(ui.nodeOrderPrevious(rootA), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(rootA), child3A1);
    CORRADE_COMPARE(ui.nodeOrderLastNested(rootA), child3A2TopLevel);
    CORRADE_VERIFY(ui.isNodeOrdered(child3A1));
    CORRADE_COMPARE(ui.nodeOrderPrevious(child3A1), rootA);
    CORRADE_COMPARE(ui.nodeOrderNext(child3A1), child4A1TopLevel);
    /* This now includes the two nested */
    CORRADE_COMPARE(ui.nodeOrderLastNested(child3A1), child5A1TopLevel);
    CORRADE_VERIFY(ui.isNodeTopLevel(child4A1TopLevel));
    CORRADE_VERIFY(ui.isNodeOrdered(child4A1TopLevel));
    CORRADE_COMPARE(ui.nodeOrderPrevious(child4A1TopLevel), child3A1);
    CORRADE_COMPARE(ui.nodeOrderNext(child4A1TopLevel), child5A1TopLevel);
    CORRADE_COMPARE(ui.nodeOrderLastNested(child4A1TopLevel), child5A1TopLevel);
    CORRADE_VERIFY(ui.isNodeOrdered(child5A1TopLevel));
    CORRADE_COMPARE(ui.nodeOrderPrevious(child5A1TopLevel), child4A1TopLevel);
    CORRADE_COMPARE(ui.nodeOrderNext(child5A1TopLevel), child3A2TopLevel);
    CORRADE_COMPARE(ui.nodeOrderLastNested(child5A1TopLevel), child5A1TopLevel);
    CORRADE_VERIFY(ui.isNodeOrdered(child3A2TopLevel));
    CORRADE_COMPARE(ui.nodeOrderPrevious(child3A2TopLevel), child5A1TopLevel);
    CORRADE_COMPARE(ui.nodeOrderNext(child3A2TopLevel), rootB);
    CORRADE_COMPARE(ui.nodeOrderLastNested(child3A2TopLevel), child3A2TopLevel);
    CORRADE_VERIFY(ui.isNodeOrdered(rootB));
    CORRADE_COMPARE(ui.nodeOrderPrevious(rootB), child3A2TopLevel);
    CORRADE_COMPARE(ui.nodeOrderNext(rootB), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(rootB), rootB);
    CORRADE_COMPARE(ui.nodeOrderFirst(), rootA);
    CORRADE_COMPARE(ui.nodeOrderLast(), rootB);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 8);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 8);

    /* Flattening it returns to the previous state, child top-level nodes kept */
    ui.flattenNodeOrder(child3A1);
    CORRADE_VERIFY(ui.isNodeOrdered(rootA));
    CORRADE_COMPARE(ui.nodeOrderPrevious(rootA), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(rootA), child4A1TopLevel);
    CORRADE_COMPARE(ui.nodeOrderLastNested(rootA), child3A2TopLevel);
    /* Not in top-level order anymore */
    CORRADE_VERIFY(!ui.isNodeTopLevel(child3A1));
    CORRADE_VERIFY(!ui.isNodeOrdered(child3A1));
    CORRADE_COMPARE(ui.nodeOrderPrevious(child3A1), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(child3A1), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(child3A1), child3A1);
    CORRADE_VERIFY(ui.isNodeOrdered(child4A1TopLevel));
    CORRADE_COMPARE(ui.nodeOrderPrevious(child4A1TopLevel), rootA);
    CORRADE_COMPARE(ui.nodeOrderNext(child4A1TopLevel), child5A1TopLevel);
    CORRADE_COMPARE(ui.nodeOrderLastNested(child4A1TopLevel), child5A1TopLevel);
    CORRADE_VERIFY(ui.isNodeOrdered(child5A1TopLevel));
    CORRADE_COMPARE(ui.nodeOrderPrevious(child5A1TopLevel), child4A1TopLevel);
    CORRADE_COMPARE(ui.nodeOrderNext(child5A1TopLevel), child3A2TopLevel);
    CORRADE_COMPARE(ui.nodeOrderLastNested(child5A1TopLevel), child5A1TopLevel);
    CORRADE_VERIFY(ui.isNodeOrdered(child3A2TopLevel));
    CORRADE_COMPARE(ui.nodeOrderPrevious(child3A2TopLevel), child5A1TopLevel);
    CORRADE_COMPARE(ui.nodeOrderNext(child3A2TopLevel), rootB);
    CORRADE_COMPARE(ui.nodeOrderLastNested(child3A2TopLevel), child3A2TopLevel);
    CORRADE_VERIFY(ui.isNodeOrdered(rootB));
    CORRADE_COMPARE(ui.nodeOrderPrevious(rootB), child3A2TopLevel);
    CORRADE_COMPARE(ui.nodeOrderNext(rootB), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(rootB), rootB);
    CORRADE_COMPARE(ui.nodeOrderFirst(), rootA);
    CORRADE_COMPARE(ui.nodeOrderLast(), rootB);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 8);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 7);

    /* Flattening a child of a disconnected non-root top-level node */
    ui.clearNodeOrder(child4A1TopLevel);
    ui.flattenNodeOrder(child5A1TopLevel);
    CORRADE_VERIFY(ui.isNodeOrdered(rootA));
    CORRADE_COMPARE(ui.nodeOrderPrevious(rootA), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(rootA), child3A2TopLevel);
    CORRADE_COMPARE(ui.nodeOrderLastNested(rootA), child3A2TopLevel);
    CORRADE_VERIFY(ui.isNodeTopLevel(child4A1TopLevel));
    CORRADE_VERIFY(!ui.isNodeOrdered(child4A1TopLevel));
    CORRADE_COMPARE(ui.nodeOrderPrevious(child4A1TopLevel), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(child4A1TopLevel), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(child4A1TopLevel), child4A1TopLevel);
    /* The child is now disconnected and not top-level */
    CORRADE_VERIFY(!ui.isNodeTopLevel(child5A1TopLevel));
    CORRADE_VERIFY(!ui.isNodeOrdered(child5A1TopLevel));
    CORRADE_COMPARE(ui.nodeOrderPrevious(child5A1TopLevel), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(child5A1TopLevel), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(child5A1TopLevel), child5A1TopLevel);
    CORRADE_VERIFY(ui.isNodeOrdered(child3A2TopLevel));
    CORRADE_COMPARE(ui.nodeOrderPrevious(child3A2TopLevel), rootA);
    CORRADE_COMPARE(ui.nodeOrderNext(child3A2TopLevel), rootB);
    CORRADE_COMPARE(ui.nodeOrderLastNested(child3A2TopLevel), child3A2TopLevel);
    CORRADE_VERIFY(ui.isNodeOrdered(rootB));
    CORRADE_COMPARE(ui.nodeOrderPrevious(rootB), child3A2TopLevel);
    CORRADE_COMPARE(ui.nodeOrderNext(rootB), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(rootB), rootB);
    CORRADE_COMPARE(ui.nodeOrderFirst(), rootA);
    CORRADE_COMPARE(ui.nodeOrderLast(), rootB);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 8);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 6);

    /* Flattening a disconnected non-root top-level node */
    ui.flattenNodeOrder(child4A1TopLevel);
    CORRADE_VERIFY(ui.isNodeOrdered(rootA));
    CORRADE_COMPARE(ui.nodeOrderPrevious(rootA), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(rootA), child3A2TopLevel);
    CORRADE_COMPARE(ui.nodeOrderLastNested(rootA), child3A2TopLevel);
    CORRADE_VERIFY(!ui.isNodeTopLevel(child4A1TopLevel));
    CORRADE_VERIFY(!ui.isNodeOrdered(child4A1TopLevel));
    CORRADE_COMPARE(ui.nodeOrderPrevious(child4A1TopLevel), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(child4A1TopLevel), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(child4A1TopLevel), child4A1TopLevel);
    CORRADE_VERIFY(ui.isNodeOrdered(child3A2TopLevel));
    CORRADE_COMPARE(ui.nodeOrderPrevious(child3A2TopLevel), rootA);
    CORRADE_COMPARE(ui.nodeOrderNext(child3A2TopLevel), rootB);
    CORRADE_COMPARE(ui.nodeOrderLastNested(child3A2TopLevel), child3A2TopLevel);
    CORRADE_VERIFY(ui.isNodeOrdered(rootB));
    CORRADE_COMPARE(ui.nodeOrderPrevious(rootB), child3A2TopLevel);
    CORRADE_COMPARE(ui.nodeOrderNext(rootB), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(rootB), rootB);
    CORRADE_COMPARE(ui.nodeOrderFirst(), rootA);
    CORRADE_COMPARE(ui.nodeOrderLast(), rootB);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 8);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 5);

    /* Removing a top-level node directly frees all orders used by its range
       (doesn't defer that to clean()) */
    ui.removeNode(rootA);
    CORRADE_VERIFY(ui.isHandleValid(child3A2TopLevel));
    CORRADE_VERIFY(!ui.isNodeTopLevel(child3A2TopLevel));
    CORRADE_VERIFY(!ui.isNodeOrdered(child3A2TopLevel));
    CORRADE_COMPARE(ui.nodeOrderPrevious(child3A2TopLevel), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(child3A2TopLevel), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(child3A2TopLevel), child3A2TopLevel);
    CORRADE_VERIFY(ui.isNodeOrdered(rootB));
    CORRADE_COMPARE(ui.nodeOrderPrevious(rootB), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(rootB), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(rootB), rootB);
    CORRADE_COMPARE(ui.nodeOrderFirst(), rootB);
    CORRADE_COMPARE(ui.nodeOrderLast(), rootB);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 8);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 3); /* 2 less */
    CORRADE_COMPARE(ui.nodeUsedCount(), 12);

    /* Update doesn't affect the order in any way, just removes child nodes */
    ui.update();
    CORRADE_VERIFY(!ui.isHandleValid(child3A2TopLevel));
    CORRADE_VERIFY(ui.isNodeOrdered(rootB));
    CORRADE_COMPARE(ui.nodeOrderPrevious(rootB), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(rootB), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(rootB), rootB);
    CORRADE_COMPARE(ui.nodeOrderFirst(), rootB);
    CORRADE_COMPARE(ui.nodeOrderLast(), rootB);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 8);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 3);
    CORRADE_COMPARE(ui.nodeUsedCount(), 4);

    /* Removing a non-top-level node doesn't free anything even if there are
       child nodes below, that's deferred to clean() */
    ui.removeNode(childC);
    CORRADE_VERIFY(ui.isHandleValid(child2CTopLevel));
    CORRADE_VERIFY(ui.isNodeTopLevel(child2CTopLevel));
    CORRADE_VERIFY(ui.isNodeOrdered(child2CTopLevel));
    CORRADE_COMPARE(ui.nodeOrderPrevious(child2CTopLevel), rootCNotInOrder);
    CORRADE_COMPARE(ui.nodeOrderNext(child2CTopLevel), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(child2CTopLevel), child2CTopLevel);
    CORRADE_VERIFY(ui.isNodeOrdered(rootB));
    CORRADE_COMPARE(ui.nodeOrderPrevious(rootB), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(rootB), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(rootB), rootB);
    CORRADE_COMPARE(ui.nodeOrderFirst(), rootB);
    CORRADE_COMPARE(ui.nodeOrderLast(), rootB);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 8);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 3); /* no change */
    CORRADE_COMPARE(ui.nodeUsedCount(), 3);

    ui.update();
    CORRADE_VERIFY(!ui.isHandleValid(child2CTopLevel));
    CORRADE_VERIFY(ui.isNodeOrdered(rootB));
    CORRADE_COMPARE(ui.nodeOrderPrevious(rootB), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(rootB), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(rootB), rootB);
    CORRADE_COMPARE(ui.nodeOrderFirst(), rootB);
    CORRADE_COMPARE(ui.nodeOrderLast(), rootB);
    CORRADE_COMPARE(ui.nodeOrderCapacity(), 8);
    CORRADE_COMPARE(ui.nodeOrderUsedCount(), 2); /* just rootB and rootC now */
    CORRADE_COMPARE(ui.nodeUsedCount(), 2);

    /* Setting a nested top-level node order with no nodes in top-level order
       should not put it there */
    ui.clearNodeOrder(rootB);
    NodeHandle childBTopLevel = ui.createNode(rootB, {}, {});
    ui.setNodeOrder(childBTopLevel, NodeHandle::Null);
    CORRADE_VERIFY(ui.isNodeTopLevel(rootB));
    CORRADE_VERIFY(!ui.isNodeOrdered(rootB));
    CORRADE_COMPARE(ui.nodeOrderPrevious(rootB), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderNext(rootB), childBTopLevel);
    CORRADE_COMPARE(ui.nodeOrderLastNested(rootB), childBTopLevel);
    CORRADE_VERIFY(ui.isNodeOrdered(childBTopLevel));
    CORRADE_COMPARE(ui.nodeOrderPrevious(childBTopLevel), rootB);
    CORRADE_COMPARE(ui.nodeOrderNext(childBTopLevel), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLastNested(childBTopLevel), childBTopLevel);
    CORRADE_COMPARE(ui.nodeOrderFirst(), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOrderLast(), NodeHandle::Null);
}

void AbstractUserInterfaceTest::nodeOrderGetSetInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui{{100, 100}};
    NodeHandle inOrder = ui.createNode({}, {});
    CORRADE_VERIFY(ui.isNodeOrdered(inOrder));

    NodeHandle childA = ui.createNode(inOrder, {}, {});
    NodeHandle childB = ui.createNode(inOrder, {}, {});
    ui.setNodeOrder(childA, NodeHandle::Null);
    ui.setNodeOrder(childB, NodeHandle::Null);

    NodeHandle child2A = ui.createNode(childA, {}, {});
    ui.setNodeOrder(child2A, NodeHandle::Null);

    NodeHandle child2ANotInOrder = ui.createNode(childA, {}, {});
    CORRADE_VERIFY(!ui.isNodeOrdered(child2ANotInOrder));

    /* Duplicated because childC1NotInOrder gets into a broken state in the
       process */
    NodeHandle childC1NotInOrder = ui.createNode(inOrder, {}, {});
    NodeHandle childC2NotInOrder = ui.createNode(inOrder, {}, {});
    NodeHandle child2C1 = ui.createNode(childC1NotInOrder, {}, {});
    NodeHandle child2C2 = ui.createNode(childC2NotInOrder, {}, {});
    ui.setNodeOrder(child2C1, NodeHandle::Null);
    ui.setNodeOrder(child2C2, NodeHandle::Null);
    NodeHandle childC21 = ui.createNode(inOrder, {}, {});
    NodeHandle childC22 = ui.createNode(inOrder, {}, {});
    ui.setNodeOrder(childC21, child2C1);
    ui.setNodeOrder(childC22, child2C2);

    NodeHandle notInOrder = ui.createNode({}, {});
    ui.clearNodeOrder(notInOrder);
    CORRADE_VERIFY(!ui.isNodeOrdered(notInOrder));

    std::ostringstream out;
    Error redirectError{&out};
    ui.isNodeTopLevel(NodeHandle::Null);
    ui.isNodeTopLevel(NodeHandle(0x123abcde));
    ui.isNodeOrdered(NodeHandle::Null);
    ui.isNodeOrdered(NodeHandle(0x123abcde));
    ui.nodeOrderPrevious(NodeHandle::Null);
    ui.nodeOrderPrevious(NodeHandle(0x123abcde));
    ui.nodeOrderNext(NodeHandle::Null);
    ui.nodeOrderNext(NodeHandle(0x123abcde));
    ui.setNodeOrder(NodeHandle::Null, NodeHandle::Null);
    ui.setNodeOrder(NodeHandle(0x123abcde), NodeHandle::Null);
    ui.clearNodeOrder(NodeHandle::Null);
    ui.clearNodeOrder(NodeHandle(0x123abcde));
    ui.flattenNodeOrder(NodeHandle::Null);
    ui.flattenNodeOrder(NodeHandle(0x123abcde));
    ui.setNodeOrder(inOrder, NodeHandle(0x123abcde));

    /* Ordering before a node that isn't ordered */
    ui.setNodeOrder(inOrder, notInOrder);
    ui.setNodeOrder(childC21, childC1NotInOrder);

    /* Node ordered before itself */
    ui.setNodeOrder(inOrder, inOrder);
    /* Ordering a root node before non-root and vice versa */
    ui.setNodeOrder(inOrder, childA);
    ui.setNodeOrder(childA, inOrder);

    /* These three fire later in the algorithm, with the order on the node
       already cleared, so they have to be in this particular order to not use
       an already-broken state */
    /* Node ordered before its own child, not in order yet */
    ui.setNodeOrder(child2ANotInOrder, childA);
    /* Node ordered before its own child */
    ui.setNodeOrder(childA, child2A);
    /* Node ordered before a node with unrelated parent */
    ui.setNodeOrder(child2A, childB);

    /* Node already containing nested top-level nodes, not implemented yet */
    ui.setNodeOrder(childC1NotInOrder, NodeHandle::Null);
    /* Same, but childC2 is what contains childC1NotInOrder so if the check
       stops before `before`, it won't see it */
    ui.setNodeOrder(childC2NotInOrder, childC22);

    /* Flattening root nodes is not allowed, whether they're in order or not */
    ui.flattenNodeOrder(inOrder);
    ui.flattenNodeOrder(notInOrder);
    CORRADE_COMPARE_AS(out.str(),
        "Whee::AbstractUserInterface::isNodeTopLevel(): invalid handle Whee::NodeHandle::Null\n"
        "Whee::AbstractUserInterface::isNodeTopLevel(): invalid handle Whee::NodeHandle(0xabcde, 0x123)\n"
        "Whee::AbstractUserInterface::isNodeOrdered(): invalid handle Whee::NodeHandle::Null\n"
        "Whee::AbstractUserInterface::isNodeOrdered(): invalid handle Whee::NodeHandle(0xabcde, 0x123)\n"
        "Whee::AbstractUserInterface::nodeOrderPrevious(): invalid handle Whee::NodeHandle::Null\n"
        "Whee::AbstractUserInterface::nodeOrderPrevious(): invalid handle Whee::NodeHandle(0xabcde, 0x123)\n"
        "Whee::AbstractUserInterface::nodeOrderNext(): invalid handle Whee::NodeHandle::Null\n"
        "Whee::AbstractUserInterface::nodeOrderNext(): invalid handle Whee::NodeHandle(0xabcde, 0x123)\n"
        "Whee::AbstractUserInterface::setNodeOrder(): invalid handle Whee::NodeHandle::Null\n"
        "Whee::AbstractUserInterface::setNodeOrder(): invalid handle Whee::NodeHandle(0xabcde, 0x123)\n"
        "Whee::AbstractUserInterface::clearNodeOrder(): invalid handle Whee::NodeHandle::Null\n"
        "Whee::AbstractUserInterface::clearNodeOrder(): invalid handle Whee::NodeHandle(0xabcde, 0x123)\n"
        "Whee::AbstractUserInterface::flattenNodeOrder(): invalid handle Whee::NodeHandle::Null\n"
        "Whee::AbstractUserInterface::flattenNodeOrder(): invalid handle Whee::NodeHandle(0xabcde, 0x123)\n"
        "Whee::AbstractUserInterface::setNodeOrder(): invalid before handle Whee::NodeHandle(0xabcde, 0x123)\n"

        "Whee::AbstractUserInterface::setNodeOrder(): Whee::NodeHandle(0xb, 0x1) is not ordered\n"
        "Whee::AbstractUserInterface::setNodeOrder(): Whee::NodeHandle(0x5, 0x1) is not ordered\n"

        "Whee::AbstractUserInterface::setNodeOrder(): can't order Whee::NodeHandle(0x0, 0x1) before itself\n"
        "Whee::AbstractUserInterface::setNodeOrder(): Whee::NodeHandle(0x0, 0x1) is a root node but Whee::NodeHandle(0x1, 0x1) is not\n"
        "Whee::AbstractUserInterface::setNodeOrder(): Whee::NodeHandle(0x1, 0x1) is not a root node but Whee::NodeHandle(0x0, 0x1) is\n"

        "Whee::AbstractUserInterface::setNodeOrder(): Whee::NodeHandle(0x1, 0x1) doesn't share the nearest top-level parent with Whee::NodeHandle(0x4, 0x1)\n"
        "Whee::AbstractUserInterface::setNodeOrder(): Whee::NodeHandle(0x3, 0x1) doesn't share the nearest top-level parent with Whee::NodeHandle(0x1, 0x1)\n"
        "Whee::AbstractUserInterface::setNodeOrder(): Whee::NodeHandle(0x2, 0x1) doesn't share the nearest top-level parent with Whee::NodeHandle(0x3, 0x1)\n"

        "Whee::AbstractUserInterface::setNodeOrder(): creating a new top-level node with existing nested top-level nodes isn't implemented yet, sorry; clear the order or flatten it first\n"
        "Whee::AbstractUserInterface::setNodeOrder(): creating a new top-level node with existing nested top-level nodes isn't implemented yet, sorry; clear the order or flatten it first\n"

        "Whee::AbstractUserInterface::flattenNodeOrder(): Whee::NodeHandle(0x0, 0x1) is a root node\n"
        "Whee::AbstractUserInterface::flattenNodeOrder(): Whee::NodeHandle(0xb, 0x1) is a root node\n",
        TestSuite::Compare::String);
}

void AbstractUserInterfaceTest::data() {
    /* Event/framebuffer scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};

    LayerHandle layerHandle = ui.createLayer();

    /* Data handles tested thoroughly in AbstractLayerTest already */
    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;
        using AbstractLayer::remove;

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
    ui.layer<Layer>(layerHandle).remove(dataHandle1);
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
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override { return {}; }
    };
    ui.setLayerInstance(Containers::pointer<Layer>(layerHandle));

    DataHandle handle = ui.layer<Layer>(layerHandle).create();
    CORRADE_COMPARE(ui.layer(layerHandle).node(handle), NodeHandle::Null);

    ui.attachData(node, handle);
    CORRADE_COMPARE(ui.layer(layerHandle).node(handle), node);

    /* The data attachments aren't removed immediately, only during next
       clean() -- tested in cleanRemoveAttachedData() below */
    ui.removeNode(node);
    CORRADE_COMPARE(ui.layer(layerHandle).node(handle), node);

    /* Attaching to a null node should work also, it resets the attachment */
    ui.attachData(NodeHandle::Null, handle);
    CORRADE_COMPARE(ui.layer(layerHandle).node(handle), NodeHandle::Null);
}

void AbstractUserInterfaceTest::dataAttachInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui{{100, 100}};
    NodeHandle node = ui.createNode({}, {});

    std::ostringstream out;
    Error redirectError{&out};
    ui.attachData(NodeHandle(0x123abcde), DataHandle::Null);
    ui.attachData(node, DataHandle::Null);
    ui.attachData(node, DataHandle(0x12abcde34567));
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractUserInterface::attachData(): invalid handle Whee::NodeHandle(0xabcde, 0x123)\n"
        "Whee::AbstractUserInterface::attachData(): invalid handle Whee::DataHandle::Null\n"
        "Whee::AbstractUserInterface::attachData(): invalid handle Whee::DataHandle({0xab, 0x12}, {0x34567, 0xcde})\n");
}

void AbstractUserInterfaceTest::layout() {
    /* Event/framebuffer scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};

    LayouterHandle layouterHandle = ui.createLayouter();

    /* Layout handles tested thoroughly in AbstractLayouterTest already */
    struct Layouter: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::add;
        using AbstractLayouter::remove;

        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const  Containers::StridedArrayView1D<Vector2>&) override {}
    };
    Containers::Pointer<Layouter> layouter{InPlaceInit, layouterHandle};
    LayoutHandle layoutHandle1 = layouter->add(nodeHandle(0x12345, 0xabc));
    LayoutHandle layoutHandle2 = layouter->add(nodeHandle(0xabcde, 0x123));

    /* Not valid if the layouter instance isn't set yet */
    CORRADE_VERIFY(!ui.isHandleValid(layoutHandle1));
    CORRADE_VERIFY(!ui.isHandleValid(layoutHandle2));

    /* Valid when is */
    ui.setLayouterInstance(Utility::move(layouter));
    CORRADE_VERIFY(ui.isHandleValid(layoutHandle1));
    CORRADE_VERIFY(ui.isHandleValid(layoutHandle2));

    /* Not valid when removed again */
    ui.layouter<Layouter>(layouterHandle).remove(layoutHandle1);
    CORRADE_VERIFY(!ui.isHandleValid(layoutHandle1));
    CORRADE_VERIFY(ui.isHandleValid(layoutHandle2));

    /* Not valid anymore when the layouter itself is removed */
    ui.removeLayouter(layouterHandle);
    CORRADE_VERIFY(!ui.isHandleValid(layoutHandle1));
    CORRADE_VERIFY(!ui.isHandleValid(layoutHandle2));
}

void AbstractUserInterfaceTest::animation() {
    /* Event/framebuffer scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};

    AnimatorHandle animatorHandle = ui.createAnimator();

    /* Animation handles tested thoroughly in AbstractAnimatorTest already */
    struct Animator: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;
        using AbstractGenericAnimator::create;
        using AbstractGenericAnimator::remove;

        AnimatorFeatures doFeatures() const override { return {}; }
        void doAdvance(Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
    };
    Containers::Pointer<Animator> animator{InPlaceInit, animatorHandle};
    AnimationHandle animationHandle1 = animator->create(0_nsec, 1_nsec);
    AnimationHandle animationHandle2 = animator->create(0_nsec, 1_nsec);

    /* Not valid if the animator instance isn't set yet */
    CORRADE_VERIFY(!ui.isHandleValid(animationHandle1));
    CORRADE_VERIFY(!ui.isHandleValid(animationHandle1));

    /* Valid when is */
    ui.setGenericAnimatorInstance(Utility::move(animator));
    CORRADE_VERIFY(ui.isHandleValid(animationHandle1));
    CORRADE_VERIFY(ui.isHandleValid(animationHandle1));

    /* Not valid when removed again */
    ui.animator<Animator>(animatorHandle).remove(animationHandle1);
    CORRADE_VERIFY(!ui.isHandleValid(animationHandle1));
    CORRADE_VERIFY(ui.isHandleValid(animationHandle2));

    /* Not valid anymore when the animator itself is removed */
    ui.removeAnimator(animatorHandle);
    CORRADE_VERIFY(!ui.isHandleValid(animationHandle1));
    CORRADE_VERIFY(!ui.isHandleValid(animationHandle2));
}

void AbstractUserInterfaceTest::animationAttachNode() {
    /* Like dataAttach(), just with an animator instead */

    /* Event/framebuffer scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};
    NodeHandle node = ui.createNode({}, {});

    struct Animator: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;
        using AbstractGenericAnimator::create;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::NodeAttachment;
        }
        void doAdvance(Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
    };
    Animator& animator = ui.setGenericAnimatorInstance(Containers::pointer<Animator>(ui.createAnimator()));

    AnimationHandle handle = animator.create(0_nsec, 1_nsec);
    CORRADE_COMPARE(animator.node(handle), NodeHandle::Null);

    ui.attachAnimation(node, handle);
    CORRADE_COMPARE(animator.node(handle), node);

    /* The animation attachments aren't removed immediately, only during next
       clean() -- tested in cleanRemoveAttachedData() below */
    ui.removeNode(node);
    CORRADE_COMPARE(animator.node(handle), node);

    /* Attaching to a null node should work also, it resets the attachment */
    ui.attachAnimation(NodeHandle::Null, handle);
    CORRADE_COMPARE(animator.node(handle), NodeHandle::Null);
}

void AbstractUserInterfaceTest::animationAttachNodeInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui{{100, 100}};
    NodeHandle node = ui.createNode({}, {});

    struct Animator: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;
        using AbstractGenericAnimator::create;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::NodeAttachment;
        }
        void doAdvance(Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
    };
    Animator& animator = ui.setGenericAnimatorInstance(Containers::pointer<Animator>(ui.createAnimator()));

    AnimationHandle animation = animator.create(0_nsec, 1_nsec);

    std::ostringstream out;
    Error redirectError{&out};
    ui.attachAnimation(NodeHandle(0x123abcde), animation);
    ui.attachAnimation(node, AnimationHandle::Null);
    ui.attachAnimation(node, AnimationHandle(0x12abcde34567));
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractUserInterface::attachAnimation(): invalid handle Whee::NodeHandle(0xabcde, 0x123)\n"
        "Whee::AbstractUserInterface::attachAnimation(): invalid handle Whee::AnimationHandle::Null\n"
        "Whee::AbstractUserInterface::attachAnimation(): invalid handle Whee::AnimationHandle({0xab, 0x12}, {0x34567, 0xcde})\n");
}

void AbstractUserInterfaceTest::animationAttachNodeInvalidFeatures() {
    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui{{100, 100}};
    AnimatorHandle animatorHandle = ui.createAnimator();
    NodeHandle node = ui.createNode({}, {});

    struct Animator: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;
        using AbstractGenericAnimator::create;

        AnimatorFeatures doFeatures() const override {
            /* Not using DataAttachment as it would need also a layer to be
               set up, etc. Not worth the pain. */
            return {};
        }
        void doAdvance(Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
    };
    ui.setGenericAnimatorInstance(Containers::pointer<Animator>(animatorHandle));

    AnimationHandle handle = ui.animator<Animator>(animatorHandle).create(0_nsec, 1_nsec);

    std::ostringstream out;
    Error redirectError{&out};
    ui.attachAnimation(node, handle);
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractUserInterface::attachAnimation(): node attachment not supported by this animator\n");
}

void AbstractUserInterfaceTest::animationAttachData() {
    /* Event/framebuffer scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};

    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;
        using AbstractLayer::remove;

        LayerFeatures doFeatures() const override { return {}; }
    };
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer()));

    DataHandle data = layer.create();

    struct Animator: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;
        using AbstractGenericAnimator::setLayer;
        using AbstractGenericAnimator::create;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::DataAttachment;
        }
        void doAdvance(Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
    };
    Containers::Pointer<Animator> instance{InPlaceInit, ui.createAnimator()};
    instance->setLayer(layer);
    Animator& animator = ui.setGenericAnimatorInstance(Utility::move(instance));

    AnimationHandle handle = animator.create(0_nsec, 1_nsec);
    CORRADE_COMPARE(animator.data(handle), DataHandle::Null);

    ui.attachAnimation(data, handle);
    CORRADE_COMPARE(animator.data(handle), data);

    /* The animation attachments aren't removed immediately, only during next
       clean() -- tested in cleanRemoveAttachedData() below */
    layer.remove(data);
    CORRADE_COMPARE(animator.data(handle), data);

    /* Attaching to a null data should work also, it resets the attachment */
    ui.attachAnimation(DataHandle::Null, handle);
    CORRADE_COMPARE(animator.data(handle), DataHandle::Null);

    /* There's no LayerDataHandle overload as that would not add anything
       useful (i.e., no extra checks) on top of calling
       AbstractAnimator::attach() directly */
}

void AbstractUserInterfaceTest::animationAttachDataInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui{{100, 100}};

    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override { return {}; }
    };
    Layer& layer1 = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer()));
    Layer& layer2 = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer()));

    struct Animator: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;
        using AbstractGenericAnimator::setLayer;
        using AbstractGenericAnimator::create;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::DataAttachment;
        }
        void doAdvance(Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
    };
    Containers::Pointer<Animator> instance{InPlaceInit, ui.createAnimator()};
    instance->setLayer(layer1);
    Animator& animator = ui.setGenericAnimatorInstance(Utility::move(instance));

    AnimationHandle animation = animator.create(0_nsec, 1_nsec);

    DataHandle dataLayer1 = layer1.create();
    DataHandle dataLayer2 = layer2.create();

    std::ostringstream out;
    Error redirectError{&out};
    ui.attachAnimation(dataHandle(layer1.handle(), 0xabcde, 0x123), animation);
    ui.attachAnimation(dataLayer1, AnimationHandle::Null);
    ui.attachAnimation(dataLayer1, AnimationHandle(0x12abcde34567));
    ui.attachAnimation(dataLayer2, animation);
    CORRADE_COMPARE_AS(out.str(),
        "Whee::AbstractUserInterface::attachAnimation(): invalid handle Whee::DataHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Whee::AbstractUserInterface::attachAnimation(): invalid handle Whee::AnimationHandle::Null\n"
        "Whee::AbstractUserInterface::attachAnimation(): invalid handle Whee::AnimationHandle({0xab, 0x12}, {0x34567, 0xcde})\n"
        "Whee::AbstractUserInterface::attachAnimation(): expected a data handle with Whee::LayerHandle(0x0, 0x1) but got Whee::DataHandle({0x1, 0x1}, {0x0, 0x1})\n",
        TestSuite::Compare::String);
}

void AbstractUserInterfaceTest::animationAttachDataInvalidFeatures() {
    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui{{100, 100}};

    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override { return {}; }
    };
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer()));

    struct Animator: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;
        using AbstractGenericAnimator::create;

        AnimatorFeatures doFeatures() const override {
            /* Not DataAttachment */
            return AnimatorFeature::NodeAttachment;
        }
        void doAdvance(Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
    };
    Animator& animator = ui.setGenericAnimatorInstance(Containers::pointer<Animator>(ui.createAnimator()));

    DataHandle data = layer.create();
    AnimationHandle handle = animator.create(0_nsec, 1_nsec);

    std::ostringstream out;
    Error redirectError{&out};
    ui.attachAnimation(data, handle);
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractUserInterface::attachAnimation(): data attachment not supported by this animator\n");
}

void AbstractUserInterfaceTest::setSizeToLayers() {
    AbstractUserInterface ui{NoCreate};

    /* State flags from setSize() affecting node clip state are tested
       thoroughly in state(). Here there are no nodes, which makes them not
       being set at all. */

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
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate);
    CORRADE_COMPARE_AS(calls, (Containers::arrayView<Containers::Triple<LayerHandle, Vector2, Vector2i>>({
        /* Nothing yet */
    })), TestSuite::Compare::Container);

    /* Setting the size should set it for all layers that have instances and
       support Draw. UserInterfaceState::NeedsRendererSizeSetup shouldn't be
       set because there's no renderer for which it would be deferred. */
    ui.setSize({300.0f, 200.0f}, {3000.0f, 2000.0f}, {30, 20});
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate);
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
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate);
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
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate);
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
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate);
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
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate);
    CORRADE_COMPARE(ui.size(), (Vector2{300.0f, 200.0f}));
    CORRADE_COMPARE(ui.windowSize(), (Vector2{300.0f, 200.0f}));
    CORRADE_COMPARE(ui.framebufferSize(), (Vector2i{300, 200}));
    CORRADE_COMPARE_AS(calls, (Containers::arrayView<Containers::Triple<LayerHandle, Vector2, Vector2i>>({
        {layerSetBeforeFirstSize, {300.0f, 200.0f}, {300, 200}},
        {layerSetAfterFirstSize, {300.0f, 200.0f}, {300, 200}},
    })), TestSuite::Compare::Container);
}

void AbstractUserInterfaceTest::setSizeToLayouters() {
    AbstractUserInterface ui{NoCreate};

    /* Like setSizeToLayers(), but affectings layouters. Again state flags from
       setSize() affecting layout state are tested thoroughly in state(). Here
       there are no nodes, which makes them not being set at all. */

    struct Layouter: AbstractLayouter {
        explicit Layouter(LayouterHandle handle, Containers::Array<Containers::Pair<LayouterHandle, Vector2>>& calls): AbstractLayouter{handle}, calls(calls) {}

        void doSetSize(const Vector2& size) override {
            arrayAppend(calls, InPlaceInit, handle(), size);
        }
        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&) override {}

        Containers::Array<Containers::Pair<LayouterHandle, Vector2>>& calls;
    };

    Containers::Array<Containers::Pair<LayouterHandle, Vector2>> calls;

    /* Layouter instances set before the size is set shouldn't have doSetSize()
       called */
    /*LayouterHandle layerWithNoInstance =*/ ui.createLayouter();
    LayouterHandle layouterSetBeforeFirstSize = ui.createLayouter();
    LayouterHandle layouterThatIsRemoved = ui.createLayouter();
    ui.setLayouterInstance(Containers::pointer<Layouter>(layouterSetBeforeFirstSize, calls));
    ui.removeLayouter(layouterThatIsRemoved);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsLayoutAssignmentUpdate);
    CORRADE_COMPARE_AS(calls, (Containers::arrayView<Containers::Pair<LayouterHandle, Vector2>>({
        /* Nothing yet */
    })), TestSuite::Compare::Container);

    /* Setting the size should set it for all layers that have instances and
       support Draw. UserInterfaceState::NeedsRendererSizeSetup shouldn't be
       set because there's no renderer for which it would be deferred. */
    ui.setSize({300.0f, 200.0f}, {3000.0f, 2000.0f}, {30, 20});
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsLayoutAssignmentUpdate);
    CORRADE_COMPARE(ui.size(), (Vector2{300.0f, 200.0f}));
    CORRADE_COMPARE(ui.windowSize(), (Vector2{3000.0f, 2000.0f}));
    CORRADE_COMPARE(ui.framebufferSize(), (Vector2i{30, 20}));
    CORRADE_COMPARE_AS(calls, (Containers::arrayView<Containers::Pair<LayouterHandle, Vector2>>({
        {layouterSetBeforeFirstSize, {300.0f, 200.0f}},
    })), TestSuite::Compare::Container);

    /* Setting a layer instance after setSize() was called should call
       doSetSize() directly, but again only if it supports Draw */
    calls = {};
    LayouterHandle layouterSetAfterFirstSize = ui.createLayouter();
    ui.setLayouterInstance(Containers::pointer<Layouter>(layouterSetAfterFirstSize, calls));
    CORRADE_COMPARE_AS(calls, (Containers::arrayView<Containers::Pair<LayouterHandle, Vector2>>({
        {layouterSetAfterFirstSize, {300.0f, 200.0f}},
    })), TestSuite::Compare::Container);

    /* Calling setSize() again with the same size should do nothing even if
       framebuffer and window size is different, as framebuffer or window size
       never reaches the layouters */
    calls = {};
    ui.setSize({300.0f, 200.0f}, {3.0f, 2.0f}, {30000, 20000});
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsLayoutAssignmentUpdate);
    CORRADE_COMPARE(ui.size(), (Vector2{300.0f, 200.0f}));
    CORRADE_COMPARE(ui.windowSize(), (Vector2{3.0f, 2.0f}));
    CORRADE_COMPARE(ui.framebufferSize(), (Vector2i{30000, 20000}));
    CORRADE_COMPARE_AS(calls, (Containers::arrayView<Containers::Pair<LayouterHandle, Vector2>>({
        /* Nothing */
    })), TestSuite::Compare::Container);

    /* Calling setSize() again with different size should call doSetSize() on
       all layers that have an instance and support Draw even if
       framebufferSize and windowSize stays the same */
    calls = {};
    ui.setSize({3000.0f, 2000.0f}, {3.0f, 2.0f}, {30000, 20000});
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsLayoutAssignmentUpdate);
    CORRADE_COMPARE(ui.size(), (Vector2{3000.0f, 2000.0f}));
    CORRADE_COMPARE(ui.windowSize(), (Vector2{3.0f, 2.0f}));
    CORRADE_COMPARE(ui.framebufferSize(), (Vector2i{30000, 20000}));
    CORRADE_COMPARE_AS(calls, (Containers::arrayView<Containers::Pair<LayouterHandle, Vector2>>({
        {layouterSetBeforeFirstSize, {3000.0f, 2000.0f}},
        {layouterSetAfterFirstSize, {3000.0f, 2000.0f}},
    })), TestSuite::Compare::Container);

    /* Finally, verify that the unscaled size overload works as well */
    calls = {};
    ui.setSize({300, 200});
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsLayoutAssignmentUpdate);
    CORRADE_COMPARE(ui.size(), (Vector2{300.0f, 200.0f}));
    CORRADE_COMPARE(ui.windowSize(), (Vector2{300.0f, 200.0f}));
    CORRADE_COMPARE(ui.framebufferSize(), (Vector2i{300, 200}));
    CORRADE_COMPARE_AS(calls, (Containers::arrayView<Containers::Pair<LayouterHandle, Vector2>>({
        {layouterSetBeforeFirstSize, {300.0f, 200.0f}},
        {layouterSetAfterFirstSize, {300.0f, 200.0f}},
    })), TestSuite::Compare::Container);
}

void AbstractUserInterfaceTest::setSizeToRenderer() {
    AbstractUserInterface ui{NoCreate};

    struct Renderer: AbstractRenderer {
        Renderer(Int& called): called(called) {}

        RendererFeatures doFeatures() const override { return {}; }
        void doSetupFramebuffers(const Vector2i& size) override {
            if(!called)
                CORRADE_COMPARE(size, (Vector2i{10, 100}));
            else
                CORRADE_COMPARE(size, (Vector2i{17, 35}));
            ++called;
        }
        void doTransition(RendererTargetState, RendererTargetState, RendererDrawStates, RendererDrawStates) override {}

        Int& called;
    };

    /* Neither UserInterfaceState::NeedsRendererSizeSetup should be set nor
       setupFramebuffers() should be called because there's no renderer yet */
    ui.setSize({10, 100});
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

    /* Assigning a renderer instance will cause setupFramebuffers() to be
       called as the size is already set and the first call is immediate.
       The inverse (setRendererInstance() instance called first, setSize()
       second) is tested in renderer() above. */
    Int called = 0;
    ui.setRendererInstance(Containers::pointer<Renderer>(called));
    CORRADE_COMPARE(called, 1);
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.renderer<Renderer>().framebufferSize(), (Vector2i{10, 100}));

    /* Calling setSize() again will propagate it directly again. Deferring it
       to a later time would only cause pain on application side, as it'd have
       to explicitly call update() before clearing a compositing framebuffer,
       etc. etc.  */
    ui.setSize({165.0f, 156.0f}, {376.0f, 234.0f}, {17, 35});
    CORRADE_COMPARE(called, 2);
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.renderer<Renderer>().framebufferSize(), (Vector2i{17, 35}));
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

    ui.clean();
    CORRADE_COMPARE(ui.nodeUsedCount(), 0);
}

void AbstractUserInterfaceTest::cleanNoOp() {
    auto&& data = CleanData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Event/framebuffer scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};

    /* Root and a nested node */
    NodeHandle root = ui.createNode({}, {});
    NodeHandle nested = ui.createNode(root, {}, {});

    /* GCC in Release mode complains that some of these may be used
       uninitialized, MSVC as well. They aren't. */
    LayerHandle layerHandle{};
    LayouterHandle layouterHandle{};
    AnimatorHandle animatorHandle1{};
    AnimatorHandle animatorHandle2{};
    DataHandle layerData{};
    LayoutHandle layouterData{};
    AnimationHandle animatorData1{};
    AnimationHandle animatorData2{};
    if(data.layers) {
        layerHandle = ui.createLayer();

        struct Layer: AbstractLayer {
            using AbstractLayer::AbstractLayer;
            using AbstractLayer::create;

            LayerFeatures doFeatures() const override { return {}; }
        };
        ui.setLayerInstance(Containers::pointer<Layer>(layerHandle));

        /* Data attached to the root node */
        layerData = ui.layer<Layer>(layerHandle).create(root);
    }
    if(data.layouters) {
        layouterHandle = ui.createLayouter();

        struct Layouter: AbstractLayouter {
            using AbstractLayouter::AbstractLayouter;
            using AbstractLayouter::add;

            void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&) override {}
        };
        ui.setLayouterInstance(Containers::pointer<Layouter>(layouterHandle));

        /* Layout assigned to the root node */
        layouterData = ui.layouter<Layouter>(layouterHandle).add(root);
    }
    if(data.nodeAttachmentAnimators) {
        animatorHandle1 = ui.createAnimator();

        struct Animator: AbstractGenericAnimator {
            using AbstractGenericAnimator::AbstractGenericAnimator;
            using AbstractGenericAnimator::create;

            AnimatorFeatures doFeatures() const override {
                return AnimatorFeature::NodeAttachment;
            }
            void doAdvance(Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
        };
        ui.setGenericAnimatorInstance(Containers::pointer<Animator>(animatorHandle1));

        /* Animation attached to the root node */
        animatorData1 = ui.animator<Animator>(animatorHandle1).create(0_nsec, 1_nsec, root);
    }
    if(data.dataAttachmentAnimators) {
        animatorHandle2 = ui.createAnimator();

        struct Animator: AbstractGenericAnimator {
            using AbstractGenericAnimator::AbstractGenericAnimator;
            using AbstractGenericAnimator::setLayer;
            using AbstractGenericAnimator::create;

            AnimatorFeatures doFeatures() const override {
                return AnimatorFeature::DataAttachment;
            }
            void doAdvance(Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
        };
        Containers::Pointer<Animator> instance{InPlaceInit, animatorHandle2};
        instance->setLayer(ui.layer(layerHandle));
        Animator& animator = ui.setGenericAnimatorInstance(Utility::move(instance));

        /* Animation attached to data attached to the root node */
        animatorData2 = animator.create(0_nsec, 1_nsec, layerData);
    }

    /* Remove the nested node to create some "dirtiness" */
    ui.removeNode(nested);
    CORRADE_COMPARE(ui.nodeUsedCount(), 1);
    if(data.layers)
        CORRADE_COMPARE(ui.layer(layerHandle).node(layerData), root);
    if(data.layouters)
        CORRADE_COMPARE(ui.layouter(layouterHandle).node(layouterData), root);
    if(data.nodeAttachmentAnimators)
        CORRADE_COMPARE(ui.animator(animatorHandle1).node(animatorData1), root);
    if(data.dataAttachmentAnimators)
        CORRADE_COMPARE(ui.animator(animatorHandle2).data(animatorData2), layerData);

    /* Clean should make no change as there's nothing dangling to remove */
    ui.clean();
    CORRADE_VERIFY(ui.isHandleValid(root));
    CORRADE_COMPARE(ui.nodeUsedCount(), 1);
    if(data.layers) {
        CORRADE_VERIFY(ui.isHandleValid(layerData));
        CORRADE_COMPARE(ui.layer(layerHandle).node(layerData), root);
    }
    if(data.layouters) {
        CORRADE_VERIFY(ui.isHandleValid(layouterData));
        CORRADE_COMPARE(ui.layouter(layouterHandle).node(layouterData), root);
    }
    if(data.nodeAttachmentAnimators) {
        CORRADE_VERIFY(ui.isHandleValid(animatorData1));
        CORRADE_COMPARE(ui.animator(animatorHandle1).node(animatorData1), root);
    }
    if(data.dataAttachmentAnimators) {
        CORRADE_VERIFY(ui.isHandleValid(animatorData2));
        CORRADE_COMPARE(ui.animator(animatorHandle2).data(animatorData2), layerData);
    }
}

void AbstractUserInterfaceTest::cleanRemoveAttachedData() {
    auto&& data = CleanData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Event/framebuffer scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};

    /* Root and a nested node */
    NodeHandle root = ui.createNode({}, {});
    NodeHandle nested = ui.createNode(root, {}, {});

    /* GCC in Release mode complains that some of these may be used
       uninitialized, MSVC as well. They aren't. */
    LayerHandle layerHandle1{};
    LayerHandle layerHandle2{};
    LayouterHandle layouterHandle1{};
    LayouterHandle layouterHandle2{};
    AnimatorHandle animatorHandle1{};
    AnimatorHandle animatorHandle2{};
    AnimatorHandle animatorHandle3{};
    AnimatorHandle animatorHandle4{};
    AnimatorHandle animatorHandle5{};
    DataHandle layerData1{};
    DataHandle layerData2{};
    DataHandle layerData3{};
    DataHandle layerData4{};
    LayoutHandle layouterData1{};
    LayoutHandle layouterData2{};
    LayoutHandle layouterData3{};
    LayoutHandle layouterData4{};
    AnimationHandle animatorData1{};
    AnimationHandle animatorData2{};
    AnimationHandle animatorData3{};
    AnimationHandle animatorData4{};
    AnimationHandle animatorData5{};
    AnimationHandle animatorData6{};
    AnimationHandle animatorData7{};
    AnimationHandle animatorData8{};
    AnimationHandle animatorData9{};
    if(data.layers) {
        layerHandle1 = ui.createLayer();
        layerHandle2 = ui.createLayer();

        struct Layer: AbstractLayer {
            using AbstractLayer::AbstractLayer;
            using AbstractLayer::create;

            LayerFeatures doFeatures() const override { return {}; }
        };
        ui.setLayerInstance(Containers::pointer<Layer>(layerHandle1));
        ui.setLayerInstance(Containers::pointer<Layer>(layerHandle2));

        /* Data attached to both, from both layers, in random order */
        layerData1 = ui.layer<Layer>(layerHandle1).create(nested);
        layerData2 = ui.layer<Layer>(layerHandle2).create(root);
        layerData3 = ui.layer<Layer>(layerHandle1).create(root);
        layerData4 = ui.layer<Layer>(layerHandle2).create(nested);
    }
    if(data.layouters) {
        layouterHandle1 = ui.createLayouter();
        layouterHandle2 = ui.createLayouter();

        struct Layouter: AbstractLayouter {
            using AbstractLayouter::AbstractLayouter;
            using AbstractLayouter::add;

            void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&) override {}
        };
        ui.setLayouterInstance(Containers::pointer<Layouter>(layouterHandle1));
        ui.setLayouterInstance(Containers::pointer<Layouter>(layouterHandle2));

        /* Layouts assigned to both, from both layouters, in random order */
        layouterData1 = ui.layouter<Layouter>(layouterHandle1).add(nested);
        layouterData2 = ui.layouter<Layouter>(layouterHandle2).add(root);
        layouterData3 = ui.layouter<Layouter>(layouterHandle1).add(root);
        layouterData4 = ui.layouter<Layouter>(layouterHandle2).add(nested);
    }
    if(data.nodeAttachmentAnimators || data.dataAttachmentAnimators) {
        animatorHandle2 = ui.createAnimator();
        if(data.nodeAttachmentAnimators) {
            animatorHandle1 = ui.createAnimator();
            animatorHandle3 = ui.createAnimator();
        }
        if(data.dataAttachmentAnimators) {
            animatorHandle4 = ui.createAnimator();
            animatorHandle5 = ui.createAnimator();
        }

        struct Animator: AbstractGenericAnimator {
            explicit Animator(AnimatorHandle handle, AnimatorFeatures features): AbstractGenericAnimator{handle}, _features{features} {}

            using AbstractGenericAnimator::setLayer;
            using AbstractGenericAnimator::create;

            AnimatorFeatures doFeatures() const override { return _features; }
            void doAdvance(Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
            private:
                AnimatorFeatures _features;
        };
        if(data.nodeAttachmentAnimators) {
            ui.setGenericAnimatorInstance(Containers::pointer<Animator>(animatorHandle1, AnimatorFeature::NodeAttachment));
            ui.setGenericAnimatorInstance(Containers::pointer<Animator>(animatorHandle3, AnimatorFeature::NodeAttachment));

            /* Animations attached to both nodes, from both animators, in
               random order */
            animatorData1 = ui.animator<Animator>(animatorHandle1).create(0_nsec, 1_nsec, nested);
            animatorData2 = ui.animator<Animator>(animatorHandle3).create(0_nsec, 1_nsec, root);
            animatorData4 = ui.animator<Animator>(animatorHandle1).create(0_nsec, 1_nsec, root);
            animatorData5 = ui.animator<Animator>(animatorHandle3).create(0_nsec, 1_nsec, nested);
        }

        /* This one has no node attachment, should be skipped in clean() */
        ui.setGenericAnimatorInstance(Containers::pointer<Animator>(animatorHandle2, AnimatorFeatures{}));
        animatorData3 = ui.animator<Animator>(animatorHandle2).create(0_nsec, 1_nsec);

        if(data.dataAttachmentAnimators) {
            Containers::Pointer<Animator> instanceLayer1{InPlaceInit, animatorHandle4, AnimatorFeature::DataAttachment};
            Containers::Pointer<Animator> instanceLayer2{InPlaceInit, animatorHandle5, AnimatorFeature::DataAttachment};
            instanceLayer1->setLayer(ui.layer(layerHandle1));
            instanceLayer2->setLayer(ui.layer(layerHandle2));
            Animator& animatorLayer1 = ui.setGenericAnimatorInstance(Utility::move(instanceLayer1));
            Animator& animatorLayer2 = ui.setGenericAnimatorInstance(Utility::move(instanceLayer2));

            /* Animation attached to data from both animators, in random order,
               two animations to one data in one case */
            animatorData6 = animatorLayer1.create(0_nsec, 1_nsec, layerData3);
            animatorData7 = animatorLayer1.create(0_nsec, 1_nsec, layerData1);
            animatorData8 = animatorLayer2.create(0_nsec, 1_nsec, layerData4);
            animatorData9 = animatorLayer2.create(0_nsec, 1_nsec, layerData4);
        }
    }

    /* Remove the nested node */
    ui.removeNode(nested);
    CORRADE_COMPARE(ui.nodeUsedCount(), 1);
    if(data.layers) {
        CORRADE_COMPARE(ui.layer(layerHandle1).usedCount(), 2);
        CORRADE_COMPARE(ui.layer(layerHandle2).usedCount(), 2);
    }
    if(data.layouters) {
        CORRADE_COMPARE(ui.layouter(layouterHandle1).usedCount(), 2);
        CORRADE_COMPARE(ui.layouter(layouterHandle2).usedCount(), 2);
    }
    if(data.nodeAttachmentAnimators) {
        CORRADE_COMPARE(ui.animator(animatorHandle1).usedCount(), 2);
        CORRADE_COMPARE(ui.animator(animatorHandle2).usedCount(), 1);
        CORRADE_COMPARE(ui.animator(animatorHandle3).usedCount(), 2);
    }

    /* Clean removes the nested node data & animations */
    ui.clean();
    CORRADE_COMPARE(ui.nodeUsedCount(), 1);
    CORRADE_VERIFY(ui.isHandleValid(root));
    if(data.layers) {
        CORRADE_COMPARE(ui.layer(layerHandle1).usedCount(), 1);
        CORRADE_COMPARE(ui.layer(layerHandle2).usedCount(), 1);
        CORRADE_VERIFY(!ui.isHandleValid(layerData1));
        CORRADE_VERIFY(ui.isHandleValid(layerData2));
        CORRADE_VERIFY(ui.isHandleValid(layerData3));
        CORRADE_VERIFY(!ui.isHandleValid(layerData4));
    }
    if(data.layouters) {
        CORRADE_COMPARE(ui.layouter(layouterHandle1).usedCount(), 1);
        CORRADE_COMPARE(ui.layouter(layouterHandle2).usedCount(), 1);
        CORRADE_VERIFY(!ui.isHandleValid(layouterData1));
        CORRADE_VERIFY(ui.isHandleValid(layouterData2));
        CORRADE_VERIFY(ui.isHandleValid(layouterData3));
        CORRADE_VERIFY(!ui.isHandleValid(layouterData4));
    }
    if(data.nodeAttachmentAnimators || data.dataAttachmentAnimators) {
        CORRADE_COMPARE(ui.animator(animatorHandle2).usedCount(), 1);
        CORRADE_VERIFY(ui.isHandleValid(animatorData3));
        if(data.nodeAttachmentAnimators) {
            CORRADE_COMPARE(ui.animator(animatorHandle1).usedCount(), 1);
            CORRADE_COMPARE(ui.animator(animatorHandle3).usedCount(), 1);
            CORRADE_VERIFY(!ui.isHandleValid(animatorData1));
            CORRADE_VERIFY(ui.isHandleValid(animatorData2));
            CORRADE_VERIFY(ui.isHandleValid(animatorData4));
            CORRADE_VERIFY(!ui.isHandleValid(animatorData5));
        }
        if(data.dataAttachmentAnimators) {
            CORRADE_COMPARE(ui.animator(animatorHandle4).usedCount(), 1);
            CORRADE_COMPARE(ui.animator(animatorHandle5).usedCount(), 0);
            CORRADE_VERIFY(ui.isHandleValid(animatorData6));
            CORRADE_VERIFY(!ui.isHandleValid(animatorData7));
            CORRADE_VERIFY(!ui.isHandleValid(animatorData8));
            CORRADE_VERIFY(!ui.isHandleValid(animatorData9));
        }
    }
}

void AbstractUserInterfaceTest::cleanRemoveNestedNodes() {
    auto&& data = CleanData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Event/framebuffer scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};

    /* A nested node tree */
    NodeHandle root = ui.createNode({}, {});
    NodeHandle first1 = ui.createNode(root, {}, {});
    NodeHandle second1 = ui.createNode(first1, {}, {});
    NodeHandle first2 = ui.createNode(root, {}, {});
    NodeHandle second2 = ui.createNode(first1, {}, {});

    /* GCC in Release mode complains that some of these may be used
       uninitialized, MSVC as well. They aren't. */
    LayerHandle layerHandle{};
    LayouterHandle layouterHandle;
    AnimatorHandle animatorHandle1{};
    AnimatorHandle animatorHandle2;
    DataHandle layerData1{};
    DataHandle layerData2{};
    DataHandle layerData3{};
    LayoutHandle layouterData1{};
    LayoutHandle layouterData2{};
    LayoutHandle layouterData3{};
    AnimationHandle animatorData1{};
    AnimationHandle animatorData2{};
    AnimationHandle animatorData3{};
    AnimationHandle animatorData4{};
    AnimationHandle animatorData5{};
    AnimationHandle animatorData6{};
    if(data.layers) {
        layerHandle = ui.createLayer();

        struct Layer: AbstractLayer {
            using AbstractLayer::AbstractLayer;
            using AbstractLayer::create;

            LayerFeatures doFeatures() const override { return {}; }
        };
        ui.setLayerInstance(Containers::pointer<Layer>(layerHandle));

        /* Data attached to the leaf nodes */
        layerData1 = ui.layer<Layer>(layerHandle).create(second1);
        layerData2 = ui.layer<Layer>(layerHandle).create(first2);
        layerData3 = ui.layer<Layer>(layerHandle).create(second2);
    }
    if(data.layouters) {
        layouterHandle = ui.createLayouter();

        struct Layouter: AbstractLayouter {
            using AbstractLayouter::AbstractLayouter;
            using AbstractLayouter::add;

            void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&) override {}
        };
        ui.setLayouterInstance(Containers::pointer<Layouter>(layouterHandle));

        /* Layouts assigned to the leaf nodes */
        layouterData1 = ui.layouter<Layouter>(layouterHandle).add(second1);
        layouterData2 = ui.layouter<Layouter>(layouterHandle).add(first2);
        layouterData3 = ui.layouter<Layouter>(layouterHandle).add(second2);
    }
    if(data.nodeAttachmentAnimators) {
        animatorHandle1 = ui.createAnimator();

        struct Animator: AbstractGenericAnimator {
            using AbstractGenericAnimator::AbstractGenericAnimator;
            using AbstractGenericAnimator::create;

            AnimatorFeatures doFeatures() const override {
                return AnimatorFeature::NodeAttachment;
            }
            void doAdvance(Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
        };
        ui.setGenericAnimatorInstance(Containers::pointer<Animator>(animatorHandle1));

        /* Animations attached to the leaf nodes */
        animatorData1 = ui.animator<Animator>(animatorHandle1).create(0_nsec, 1_nsec, second1);
        animatorData2 = ui.animator<Animator>(animatorHandle1).create(0_nsec, 1_nsec, first2);
        animatorData3 = ui.animator<Animator>(animatorHandle1).create(0_nsec, 1_nsec, second2);
    }
    if(data.dataAttachmentAnimators) {
        animatorHandle2 = ui.createAnimator();

        struct Animator: AbstractGenericAnimator {
            using AbstractGenericAnimator::AbstractGenericAnimator;
            using AbstractGenericAnimator::setLayer;
            using AbstractGenericAnimator::create;

            AnimatorFeatures doFeatures() const override {
                return AnimatorFeature::DataAttachment;
            }
            void doAdvance(Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
        };
        Containers::Pointer<Animator> instance{InPlaceInit, animatorHandle2};
        instance->setLayer(ui.layer(layerHandle));
        Animator& animator = ui.setGenericAnimatorInstance(Utility::move(instance));

        /* Animations attached to data attached to the leaf nodes */
        animatorData4 = animator.create(0_nsec, 1_nsec, layerData1);
        animatorData5 = animator.create(0_nsec, 1_nsec, layerData2);
        animatorData6 = animator.create(0_nsec, 1_nsec, layerData3);
    }

    /* Remove the subtree */
    ui.removeNode(first1);
    CORRADE_COMPARE(ui.nodeUsedCount(), 4);

    /* Clean removes the nested nodes and subsequently the data & animations
       attached to them */
    ui.clean();
    CORRADE_COMPARE(ui.nodeUsedCount(), 2);
    CORRADE_VERIFY(ui.isHandleValid(root));
    CORRADE_VERIFY(!ui.isHandleValid(first1));
    CORRADE_VERIFY(ui.isHandleValid(first2));
    CORRADE_VERIFY(!ui.isHandleValid(second1));
    CORRADE_VERIFY(!ui.isHandleValid(second2));
    if(data.layers) {
        CORRADE_VERIFY(!ui.isHandleValid(layerData1));
        CORRADE_VERIFY(ui.isHandleValid(layerData2));
        CORRADE_VERIFY(!ui.isHandleValid(layerData3));
    }
    if(data.layouters) {
        CORRADE_VERIFY(!ui.isHandleValid(layouterData1));
        CORRADE_VERIFY(ui.isHandleValid(layouterData2));
        CORRADE_VERIFY(!ui.isHandleValid(layouterData3));
    }
    if(data.nodeAttachmentAnimators) {
        CORRADE_VERIFY(!ui.isHandleValid(animatorData1));
        CORRADE_VERIFY(ui.isHandleValid(animatorData2));
        CORRADE_VERIFY(!ui.isHandleValid(animatorData3));
    }
    if(data.dataAttachmentAnimators) {
        CORRADE_VERIFY(!ui.isHandleValid(animatorData4));
        CORRADE_VERIFY(ui.isHandleValid(animatorData5));
        CORRADE_VERIFY(!ui.isHandleValid(animatorData6));
    }
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
    auto&& data = CleanData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Event/framebuffer scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};

    /* A nested node branch */
    NodeHandle root = ui.createNode({}, {});
    NodeHandle first = ui.createNode(root, {}, {});
    NodeHandle second = ui.createNode(first, {}, {});

    /* GCC in Release mode complains that some of these may be used
       uninitialized, MSVC as well. They aren't. */
    LayerHandle layerHandle{};
    LayouterHandle layouterHandle;
    AnimatorHandle animatorHandle1{};
    AnimatorHandle animatorHandle2;
    DataHandle layerData{};
    LayoutHandle layouterData{};
    AnimationHandle animatorData1{};
    AnimationHandle animatorData2{};
    if(data.layers) {
        layerHandle = ui.createLayer();

        struct Layer: AbstractLayer {
            using AbstractLayer::AbstractLayer;
            using AbstractLayer::create;

            LayerFeatures doFeatures() const override { return {}; }
        };
        ui.setLayerInstance(Containers::pointer<Layer>(layerHandle));

        /* Data attached to the leaf node */
        layerData = ui.layer<Layer>(layerHandle).create(second);
    }
    if(data.layouters) {
        layouterHandle = ui.createLayouter();

        struct Layouter: AbstractLayouter {
            using AbstractLayouter::AbstractLayouter;
            using AbstractLayouter::add;

            void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&) override {}
        };
        ui.setLayouterInstance(Containers::pointer<Layouter>(layouterHandle));

        /* Layout assigned to the leaf node */
        layouterData = ui.layouter<Layouter>(layouterHandle).add(second);
    }
    if(data.nodeAttachmentAnimators) {
        animatorHandle1 = ui.createAnimator();

        struct Animator: AbstractGenericAnimator {
            using AbstractGenericAnimator::AbstractGenericAnimator;
            using AbstractGenericAnimator::create;

            AnimatorFeatures doFeatures() const override {
                return AnimatorFeature::NodeAttachment;
            }
            void doAdvance(Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
        };
        ui.setGenericAnimatorInstance(Containers::pointer<Animator>(animatorHandle1));

        /* Animation attached to the leaf node */
        animatorData1 = ui.animator<Animator>(animatorHandle1).create(0_nsec, 1_nsec, second);
    }
    if(data.dataAttachmentAnimators) {
        animatorHandle2 = ui.createAnimator();

        struct Animator: AbstractGenericAnimator {
            using AbstractGenericAnimator::AbstractGenericAnimator;
            using AbstractGenericAnimator::setLayer;
            using AbstractGenericAnimator::create;

            AnimatorFeatures doFeatures() const override {
                return AnimatorFeature::DataAttachment;
            }
            void doAdvance(Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
        };
        Containers::Pointer<Animator> instance{InPlaceInit, animatorHandle2};
        instance->setLayer(ui.layer(layerHandle));
        Animator& animator = ui.setGenericAnimatorInstance(Utility::move(instance));

        /* Animation attached to the data attached to the leaf node */
        animatorData2 = animator.create(0_nsec, 1_nsec, layerData);
    }

    /* Remove a subtree but then create a new node which recycles the same
       handle */
    ui.removeNode(first);
    NodeHandle first2 = ui.createNode(root, {}, {});
    CORRADE_COMPARE(nodeHandleId(first2), nodeHandleId(first));
    CORRADE_COMPARE(ui.nodeUsedCount(), 3);

    /* Clean should still remove the subtree attached to the first handle, even
       though there's a new valid node in the same slot */
    ui.clean();
    CORRADE_COMPARE(ui.nodeUsedCount(), 2);
    CORRADE_VERIFY(ui.isHandleValid(root));
    CORRADE_VERIFY(!ui.isHandleValid(first));
    CORRADE_VERIFY(ui.isHandleValid(first2));
    if(data.layers)
        CORRADE_VERIFY(!ui.isHandleValid(layerData));
    if(data.layouters)
        CORRADE_VERIFY(!ui.isHandleValid(layouterData));
    if(data.nodeAttachmentAnimators)
        CORRADE_VERIFY(!ui.isHandleValid(animatorData1));
    if(data.dataAttachmentAnimators)
        CORRADE_VERIFY(!ui.isHandleValid(animatorData2));
}

void AbstractUserInterfaceTest::cleanRemoveNestedNodesRecycledHandleOrphanedCycle() {
    auto&& data = CleanData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    CORRADE_SKIP("Ugh, this asserts.");

    /* Event/framebuffer scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};

    /* A nested node branch */
    NodeHandle root = ui.createNode({}, {});
    NodeHandle first = ui.createNode(root, {}, {});
    NodeHandle second = ui.createNode(first, {}, {});
    NodeHandle third = ui.createNode(second, {}, {});

    /* GCC in Release mode complains that some of these may be used
       uninitialized, MSVC as well. They aren't. */
    LayerHandle layerHandle;
    LayouterHandle layouterHandle;
    AnimatorHandle animatorHandle1;
    AnimatorHandle animatorHandle2;
    DataHandle layerData{};
    LayoutHandle layouterData;
    AnimationHandle animatorData1;
    AnimationHandle animatorData2;
    if(data.layers) {
        layerHandle = ui.createLayer();

        struct Layer: AbstractLayer {
            using AbstractLayer::AbstractLayer;
            using AbstractLayer::create;

            LayerFeatures doFeatures() const override { return {}; }
        };
        ui.setLayerInstance(Containers::pointer<Layer>(layerHandle));

        /* Data attached to the leaf node */
        layerData = ui.layer<Layer>(layerHandle).create(third);
    }
    if(data.layouters) {
        layouterHandle = ui.createLayouter();

        struct Layouter: AbstractLayouter {
            using AbstractLayouter::AbstractLayouter;
            using AbstractLayouter::add;

            void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&) override {}
        };
        ui.setLayouterInstance(Containers::pointer<Layouter>(layouterHandle));

        /* Layout assigned to the leaf node */
        layouterData = ui.layouter<Layouter>(layouterHandle).add(third);
    }
    if(data.nodeAttachmentAnimators) {
        animatorHandle1 = ui.createAnimator();

        struct Animator: AbstractGenericAnimator {
            using AbstractGenericAnimator::AbstractGenericAnimator;
            using AbstractGenericAnimator::create;

            AnimatorFeatures doFeatures() const override {
                return AnimatorFeature::NodeAttachment;
            }
            void doAdvance(Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
        };
        ui.setGenericAnimatorInstance(Containers::pointer<Animator>(animatorHandle1));

        /* Animation attached to the leaf node */
        animatorData1 = ui.animator<Animator>(animatorHandle1).create(0_nsec, 1_nsec, third);
    }
    if(data.dataAttachmentAnimators) {
        animatorHandle2 = ui.createAnimator();

        struct Animator: AbstractGenericAnimator {
            using AbstractGenericAnimator::AbstractGenericAnimator;
            using AbstractGenericAnimator::setLayer;
            using AbstractGenericAnimator::create;

            AnimatorFeatures doFeatures() const override {
                return AnimatorFeature::DataAttachment;
            }
            void doAdvance(Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
        };
        Containers::Pointer<Animator> instance{InPlaceInit, animatorHandle2};
        instance->setLayer(ui.layer(layerHandle));
        Animator& animator = ui.setGenericAnimatorInstance(Utility::move(instance));

        /* Animation attached to the data attached to the leaf node */
        animatorData2 = animator.create(0_nsec, 1_nsec, layerData);
    }

    /* Remove a subtree but then create a new node which recycles the same
       handle, and parent it to one of the (now dangling) nodes */
    ui.removeNode(first);
    NodeHandle first2 = ui.createNode(second, {}, {});
    CORRADE_COMPARE(nodeHandleId(first2), nodeHandleId(first));
    CORRADE_COMPARE(ui.nodeUsedCount(), 4);

    ui.clean();
    CORRADE_COMPARE(ui.nodeUsedCount(), 2);
    CORRADE_VERIFY(ui.isHandleValid(root));
    CORRADE_VERIFY(!ui.isHandleValid(first));
    CORRADE_VERIFY(ui.isHandleValid(first2));
    CORRADE_VERIFY(!ui.isHandleValid(second));
    CORRADE_VERIFY(!ui.isHandleValid(third));
    if(data.layers)
        CORRADE_VERIFY(!ui.isHandleValid(layerData));
    if(data.layouters)
        CORRADE_VERIFY(!ui.isHandleValid(layouterData));
    if(data.nodeAttachmentAnimators)
        CORRADE_VERIFY(!ui.isHandleValid(animatorData1));
    if(data.dataAttachmentAnimators)
        CORRADE_VERIFY(!ui.isHandleValid(animatorData2));
}

void AbstractUserInterfaceTest::cleanRemoveAll() {
    auto&& data = CleanData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Event/framebuffer scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};

    /* A nested node branch */
    NodeHandle root = ui.createNode({}, {});
    NodeHandle first = ui.createNode(root, {}, {});
    NodeHandle second = ui.createNode(first, {}, {});

    /* GCC in Release mode complains that some of these may be used
       uninitialized, MSVC as well. They aren't. */
    LayerHandle layerHandle{};
    DataHandle layerData1{};
    DataHandle layerData2{};
    LayouterHandle layouterHandle{};
    AnimatorHandle animatorHandle1{};
    AnimatorHandle animatorHandle2{};
    if(data.layers) {
        layerHandle = ui.createLayer();

        struct Layer: AbstractLayer {
            using AbstractLayer::AbstractLayer;
            using AbstractLayer::create;

            LayerFeatures doFeatures() const override { return {}; }
        };
        ui.setLayerInstance(Containers::pointer<Layer>(layerHandle));

        /* Data attached to the nested nodes */
        layerData1 = ui.layer<Layer>(layerHandle).create(second);
        layerData2 = ui.layer<Layer>(layerHandle).create(first);
    }
    if(data.layouters) {
        layouterHandle = ui.createLayouter();

        struct Layouter: AbstractLayouter {
            using AbstractLayouter::AbstractLayouter;
            using AbstractLayouter::add;

            void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&) override {}
        };
        ui.setLayouterInstance(Containers::pointer<Layouter>(layouterHandle));

        /* Data attached to the nested nodes */
        ui.layouter<Layouter>(layouterHandle).add(second);
        ui.layouter<Layouter>(layouterHandle).add(first);
    }
    if(data.nodeAttachmentAnimators) {
        animatorHandle1 = ui.createAnimator();

        struct Animator: AbstractGenericAnimator {
            using AbstractGenericAnimator::AbstractGenericAnimator;
            using AbstractGenericAnimator::create;

            AnimatorFeatures doFeatures() const override {
                return AnimatorFeature::NodeAttachment;
            }
            void doAdvance(Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
        };
        ui.setGenericAnimatorInstance(Containers::pointer<Animator>(animatorHandle1));

        /* Animations attached to the nested nodes */
        ui.animator<Animator>(animatorHandle1).create(0_nsec, 1_nsec, second);
        ui.animator<Animator>(animatorHandle1).create(0_nsec, 1_nsec, first);
    }
    if(data.dataAttachmentAnimators) {
        animatorHandle2 = ui.createAnimator();

        struct Animator: AbstractGenericAnimator {
            using AbstractGenericAnimator::AbstractGenericAnimator;
            using AbstractGenericAnimator::setLayer;
            using AbstractGenericAnimator::create;

            AnimatorFeatures doFeatures() const override {
                return AnimatorFeature::DataAttachment;
            }
            void doAdvance(Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
        };
        Containers::Pointer<Animator> instance{InPlaceInit, animatorHandle2};
        instance->setLayer(ui.layer(layerHandle));
        Animator& animator = ui.setGenericAnimatorInstance(Utility::move(instance));

        /* Data attached to the data attached to the nested nodes */
        animator.create(0_nsec, 1_nsec, layerData1);
        animator.create(0_nsec, 1_nsec, layerData2);
    }

    /* Removing the top-level node */
    ui.removeNode(root);
    CORRADE_COMPARE(ui.nodeUsedCount(), 2);
    if(data.layers)
        CORRADE_COMPARE(ui.layer(layerHandle).usedCount(), 2);
    if(data.layouters)
        CORRADE_COMPARE(ui.layouter(layouterHandle).usedCount(), 2);
    if(data.nodeAttachmentAnimators)
        CORRADE_COMPARE(ui.animator(animatorHandle1).usedCount(), 2);
    if(data.dataAttachmentAnimators)
        CORRADE_COMPARE(ui.animator(animatorHandle2).usedCount(), 2);

    /* Clean should remove everything */
    ui.clean();
    CORRADE_COMPARE(ui.nodeUsedCount(), 0);
    if(data.layers)
        CORRADE_COMPARE(ui.layer(layerHandle).usedCount(), 0);
    if(data.layouters)
        CORRADE_COMPARE(ui.layouter(layouterHandle).usedCount(), 0);
    if(data.nodeAttachmentAnimators)
        CORRADE_COMPARE(ui.animator(animatorHandle1).usedCount(), 0);
    if(data.dataAttachmentAnimators)
        CORRADE_COMPARE(ui.animator(animatorHandle2).usedCount(), 0);
}

void AbstractUserInterfaceTest::cleanRecycledLayerWithAnimators() {
    /* Removing a layer with associated animators should remove those instances
       from the internal list to prevent weird behavior during next clean()
       etc. */

    AbstractUserInterface ui{{100, 100}};

    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    };
    struct LayerAnimatingDataStyles: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::setAnimator;

        LayerFeatures doFeatures() const override {
            return LayerFeature::AnimateData|LayerFeature::AnimateStyles;
        }
    };

    Layer& layer1 = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer()));
    LayerAnimatingDataStyles& layer2 = ui.setLayerInstance(Containers::pointer<LayerAnimatingDataStyles>(ui.createLayer()));

    struct GenericAnimator: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;
        using AbstractGenericAnimator::setLayer;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::DataAttachment;
        }
        void doClean(Containers::BitArrayView) override {
            ++cleanCallCount;
        }
        void doAdvance(Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}

        Int cleanCallCount = 0;
    };
    struct DataAnimator: AbstractDataAnimator {
        using AbstractDataAnimator::AbstractDataAnimator;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::DataAttachment;
        }
        void doClean(Containers::BitArrayView) override {
            ++cleanCallCount;
        }

        Int cleanCallCount = 0;
    };
    struct StyleAnimator: AbstractStyleAnimator {
        using AbstractStyleAnimator::AbstractStyleAnimator;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::DataAttachment;
        }
        void doClean(Containers::BitArrayView) override {
            ++cleanCallCount;
        }

        Int cleanCallCount = 0;
    };
    Containers::Pointer<GenericAnimator> instance11{InPlaceInit, ui.createAnimator()};
    Containers::Pointer<GenericAnimator> instance12{InPlaceInit, ui.createAnimator()};
    Containers::Pointer<DataAnimator> instance21{InPlaceInit, ui.createAnimator()};
    Containers::Pointer<GenericAnimator> instance22{InPlaceInit, ui.createAnimator()};
    Containers::Pointer<StyleAnimator> instance23{InPlaceInit, ui.createAnimator()};
    instance11->setLayer(layer1);
    instance12->setLayer(layer1);
    layer2.setAnimator(*instance21);
    instance22->setLayer(layer2);
    layer2.setAnimator(*instance23);
    GenericAnimator& animator11 = ui.setGenericAnimatorInstance(Utility::move(instance11));
    GenericAnimator& animator12 = ui.setGenericAnimatorInstance(Utility::move(instance12));
    DataAnimator& animator21 = ui.setDataAnimatorInstance(Utility::move(instance21));
    GenericAnimator& animator22 = ui.setGenericAnimatorInstance(Utility::move(instance22));
    StyleAnimator& animator23 = ui.setStyleAnimatorInstance(Utility::move(instance23));

    /* Create and immediately remove a node to trigger NeedsNodeClean, which
       implies NeedsDataClean on all animators */
    ui.removeNode(ui.createNode({}, {}));
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeClean);

    /* clean() then calls doClean() on all three */
    ui.clean();
    CORRADE_COMPARE(animator11.cleanCallCount, 1);
    CORRADE_COMPARE(animator12.cleanCallCount, 1);
    CORRADE_COMPARE(animator21.cleanCallCount, 1);
    CORRADE_COMPARE(animator22.cleanCallCount, 1);
    CORRADE_COMPARE(animator23.cleanCallCount, 1);

    /* Remove the first layer and recycle its slot for a new one */
    LayerHandle layer1Handle = layer1.handle();
    ui.removeLayer(layer1Handle);
    Layer& layer1Again = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer()));
    CORRADE_COMPARE(layerHandleId(layer1Again.handle()), layerHandleId(layer1Handle));

    /* Triggering NeedsNodeClean should then cause doClean() to be only called
       on the animators associated with the remaining layer */
    ui.removeNode(ui.createNode({}, {}));
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeClean);

    ui.clean();
    CORRADE_COMPARE(animator11.cleanCallCount, 1);
    CORRADE_COMPARE(animator12.cleanCallCount, 1);
    CORRADE_COMPARE(animator21.cleanCallCount, 2);
    CORRADE_COMPARE(animator22.cleanCallCount, 2);
    CORRADE_COMPARE(animator23.cleanCallCount, 2);

    /* Remove the last layer and recycle its slot for a new one */
    LayerHandle layer2Handle = layer2.handle();
    ui.removeLayer(layer2Handle);
    Layer& layer2Again = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer()));
    CORRADE_COMPARE(layerHandleId(layer2Again.handle()), layerHandleId(layer2Handle));

    /* This should cause doClean() to not be called on any animator anymore */
    ui.removeNode(ui.createNode({}, {}));
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeClean);

    ui.clean();
    CORRADE_COMPARE(animator11.cleanCallCount, 1);
    CORRADE_COMPARE(animator12.cleanCallCount, 1);
    CORRADE_COMPARE(animator21.cleanCallCount, 2);
    CORRADE_COMPARE(animator22.cleanCallCount, 2);
    CORRADE_COMPARE(animator23.cleanCallCount, 2);
}

void AbstractUserInterfaceTest::advanceAnimationsEmpty() {
    AbstractUserInterface ui{{100, 100}};
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.animationTime(), 0_nsec);

    /* THere are no animations that actually need to be advanced but the
       time should get updated nevertheless to catch accidental backward time
       travel later */
    ui.advanceAnimations(23_nsec);
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.animationTime(), 23_nsec);
}

void AbstractUserInterfaceTest::advanceAnimationsNoOp() {
    AbstractUserInterface ui{{100, 100}};

    struct GenericAnimator: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;
        using AbstractGenericAnimator::create;

        AnimatorFeatures doFeatures() const override { return {}; }
        void doAdvance(Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doClean(Containers::BitArrayView) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
    };

    GenericAnimator& animator = ui.setGenericAnimatorInstance(Containers::pointer<GenericAnimator>(ui.createAnimator()));

    /* An animation is present, but it's stopped and not scheduled for removal
       so the animator doesn't need its advance() called */
    AnimationHandle animation = animator.create(-30_nsec, 10_nsec, AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.state(animation), AnimationState::Stopped);
    CORRADE_COMPARE(animator.state(), AnimatorStates{});
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.animationTime(), 0_nsec);

    /* This doesn't call the animator, but updates the time */
    ui.advanceAnimations(23_nsec);
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.animationTime(), 23_nsec);
}

void AbstractUserInterfaceTest::advanceAnimations() {
    /* Verifies that all possible kinds of animators get advanced when they
       should, not when they shouldn't, and that each animator kind gets
       advanced through the correct interface.

       Correctness of the arguments passed to them is verified with singular
       animator instances in advanceAnimationsGeneric(),
       advanceAnimationsNode(), advanceAnimationsData() and
       advanceAnimationsStyle(). Proper handling of things affected by
       animators (such as node removal) and corresponding state flags being set
       is tested in stateAnimations(). */

    AbstractUserInterface ui{{100, 100}};

    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    };
    /*Layer& layer1 =*/ ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer()));
    Layer& layer2 = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer()));

    enum Call {
        Advance,
        Clean
    };
    struct GenericAnimator: AbstractGenericAnimator {
        explicit GenericAnimator(AnimatorHandle handle, AnimatorFeatures features, Containers::Array<Containers::Pair<AnimatorHandle, Call>>& calls): AbstractGenericAnimator{handle}, calls(calls), _features{features} {}

        using AbstractGenericAnimator::create;
        using AbstractGenericAnimator::setLayer;

        AnimatorFeatures doFeatures() const override { return _features; }
        void doClean(Containers::BitArrayView) override {
            arrayAppend(calls, InPlaceInit, handle(), Clean);
        }
        void doAdvance(Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {
            arrayAppend(calls, InPlaceInit, handle(), Advance);
        }

        Containers::Array<Containers::Pair<AnimatorHandle, Call>>& calls;

        private:
            AnimatorFeatures _features;
    };
    struct NodeAnimator: AbstractNodeAnimator {
        explicit NodeAnimator(AnimatorHandle handle, Containers::Array<Containers::Pair<AnimatorHandle, Call>>& calls): AbstractNodeAnimator{handle}, calls(calls) {}

        using AbstractNodeAnimator::create;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::NodeAttachment;
        }
        void doClean(Containers::BitArrayView) override {
            arrayAppend(calls, InPlaceInit, handle(), Clean);
        }
        NodeAnimations doAdvance(Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<NodeFlags>&, Containers::MutableBitArrayView) override {
            arrayAppend(calls, InPlaceInit, handle(), Advance);
            return {};
        }

        Containers::Array<Containers::Pair<AnimatorHandle, Call>>& calls;
    };

    struct DataAnimator: AbstractDataAnimator {
        explicit DataAnimator(AnimatorHandle handle, Containers::Array<Containers::Pair<AnimatorHandle, Call>>& calls): AbstractDataAnimator{handle}, calls(calls) {}

        using AbstractDataAnimator::create;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::DataAttachment;
        }
        void doClean(Containers::BitArrayView) override {
            arrayAppend(calls, InPlaceInit, handle(), Clean);
        }
        /* Called by the layer. Not strictly necessary in order to test things
           but shows how a real implementation would look like. */
        void advance() {
            arrayAppend(calls, InPlaceInit, handle(), Advance);
        }

        Containers::Array<Containers::Pair<AnimatorHandle, Call>>& calls;
    };
    struct StyleAnimator: AbstractStyleAnimator {
        explicit StyleAnimator(AnimatorHandle handle, Containers::Array<Containers::Pair<AnimatorHandle, Call>>& calls): AbstractStyleAnimator{handle}, calls(calls) {}

        using AbstractStyleAnimator::create;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::DataAttachment;
        }
        void doClean(Containers::BitArrayView) override {
            arrayAppend(calls, InPlaceInit, handle(), Clean);
        }
        /* Called by the layer. Not strictly necessary in order to test things
           but shows how a real implementation would look like. */
        void advance() {
            arrayAppend(calls, InPlaceInit, handle(), Advance);
        }

        Containers::Array<Containers::Pair<AnimatorHandle, Call>>& calls;
    };
    struct LayerAnimatingDataStyles: AbstractLayer {
        explicit LayerAnimatingDataStyles(LayerHandle handle, LayerFeatures features): AbstractLayer{handle}, _features{features} {}

        void setAnimator(DataAnimator& animator) const {
            AbstractLayer::setAnimator(animator);
        }
        void setAnimator(StyleAnimator& animator) const {
            AbstractLayer::setAnimator(animator);
        }

        LayerFeatures doFeatures() const override {
            return _features;
        }
        void doAdvanceAnimations(Nanoseconds time, Containers::MutableBitArrayView activeStorage, const Containers::StridedArrayView1D<Float>& factorStorage, Containers::MutableBitArrayView removeStorage, const Containers::Iterable<AbstractDataAnimator>& animators) override {
            for(AbstractDataAnimator& animator: animators) {
                if(!(animator.state() >= AnimatorState::NeedsAdvance))
                    continue;

                /* Not strictly necessary in order to test things but shows how
                   a real implementation would look like */
                CORRADE_COMPARE(animator.capacity(), 1);
                CORRADE_COMPARE(activeStorage.size(), 1);
                CORRADE_COMPARE(factorStorage.size(), 1);
                CORRADE_COMPARE(removeStorage.size(), 1);
                Containers::Pair<bool, bool> advanceCleanNeeded = animator.update(time, activeStorage, factorStorage, removeStorage);

                if(advanceCleanNeeded.first())
                    static_cast<DataAnimator&>(animator).advance();
                if(advanceCleanNeeded.second())
                    animator.clean(removeStorage);
            }
        }
        void doAdvanceAnimations(Nanoseconds time, Containers::MutableBitArrayView activeStorage, const Containers::StridedArrayView1D<Float>& factorStorage, Containers::MutableBitArrayView removeStorage, const Containers::Iterable<AbstractStyleAnimator>& animators) override {
            for(AbstractStyleAnimator& animator: animators) {
                if(!(animator.state() >= AnimatorState::NeedsAdvance))
                    continue;

                /* Not strictly necessary in order to test things but shows how
                   a real implementation would look like */
                CORRADE_COMPARE(animator.capacity(), 1);
                CORRADE_COMPARE(activeStorage.size(), 1);
                CORRADE_COMPARE(factorStorage.size(), 1);
                CORRADE_COMPARE(removeStorage.size(), 1);
                Containers::Pair<bool, bool> advanceCleanNeeded = animator.update(time, activeStorage, factorStorage, removeStorage);

                if(advanceCleanNeeded.first())
                    static_cast<StyleAnimator&>(animator).advance();
                if(advanceCleanNeeded.second())
                    animator.clean(removeStorage);
            }
        }

        private:
            LayerFeatures _features;
    };
    LayerAnimatingDataStyles& layer3 = ui.setLayerInstance(Containers::pointer<LayerAnimatingDataStyles>(ui.createLayer(), LayerFeature::AnimateData));
    LayerAnimatingDataStyles& layer4 = ui.setLayerInstance(Containers::pointer<LayerAnimatingDataStyles>(ui.createLayer(), LayerFeature::AnimateStyles));

    Containers::Array<Containers::Pair<AnimatorHandle, Call>> calls;

    /*AnimatorHandle animatorWithoutInstance =*/ ui.createAnimator();
    GenericAnimator& animator1 = ui.setGenericAnimatorInstance(Containers::pointer<GenericAnimator>(ui.createAnimator(), AnimatorFeatures{}, calls));
    GenericAnimator& animatorRemoved = ui.setGenericAnimatorInstance(Containers::pointer<GenericAnimator>(ui.createAnimator(), AnimatorFeatures{}, calls));
    GenericAnimator& animatorNoAdvanceNeeded = ui.setGenericAnimatorInstance(Containers::pointer<GenericAnimator>(ui.createAnimator(), AnimatorFeatures{}, calls));
    GenericAnimator& animator2 = ui.setGenericAnimatorInstance(Containers::pointer<GenericAnimator>(ui.createAnimator(), AnimatorFeatures{}, calls));
    GenericAnimator& animatorNodeAttachmentNoAdvanceNeeded = ui.setGenericAnimatorInstance(Containers::pointer<GenericAnimator>(ui.createAnimator(), AnimatorFeature::NodeAttachment, calls));
    GenericAnimator& animatorNodeAttachment = ui.setGenericAnimatorInstance(Containers::pointer<GenericAnimator>(ui.createAnimator(), AnimatorFeature::NodeAttachment, calls));
    NodeAnimator& animatorNodeNoAdvanceNeeded = ui.setNodeAnimatorInstance(Containers::pointer<NodeAnimator>(ui.createAnimator(), calls));
    NodeAnimator& animatorNode = ui.setNodeAnimatorInstance(Containers::pointer<NodeAnimator>(ui.createAnimator(), calls));

    Containers::Pointer<GenericAnimator> animatorLayer2DataAttachmentNoAdvanceNeededInstance{InPlaceInit, ui.createAnimator(), AnimatorFeature::DataAttachment, calls};
    animatorLayer2DataAttachmentNoAdvanceNeededInstance->setLayer(layer2);
    GenericAnimator& animatorLayer2DataAttachmentNoAdvanceNeeded = ui.setGenericAnimatorInstance(Utility::move(animatorLayer2DataAttachmentNoAdvanceNeededInstance));

    /* Layer 3 has generic and data animators associated */
    Containers::Pointer<GenericAnimator> animatorLayer3DataAttachmentInstance{InPlaceInit, ui.createAnimator(), AnimatorFeature::DataAttachment, calls};
    Containers::Pointer<DataAnimator> animatorLayer3DataNoAdvanceNeededInstance{InPlaceInit, ui.createAnimator(), calls};
    Containers::Pointer<DataAnimator> animatorLayer3DataInstance{InPlaceInit, ui.createAnimator(), calls};
    animatorLayer3DataAttachmentInstance->setLayer(layer3);
    layer3.setAnimator(*animatorLayer3DataNoAdvanceNeededInstance);
    layer3.setAnimator(*animatorLayer3DataInstance);
    GenericAnimator& animatorLayer3DataAttachment = ui.setGenericAnimatorInstance(Utility::move(animatorLayer3DataAttachmentInstance));
    DataAnimator& animatorLayer3DataNoAdvanceNeeded = ui.setDataAnimatorInstance(Utility::move(animatorLayer3DataNoAdvanceNeededInstance));
    DataAnimator& animatorLayer3Data = ui.setDataAnimatorInstance(Utility::move(animatorLayer3DataInstance));

    /* Layer 4 has style animators associated */
    Containers::Pointer<StyleAnimator> animatorLayer4StyleNoAdvanceNeededInstance{InPlaceInit, ui.createAnimator(), calls};
    Containers::Pointer<StyleAnimator> animatorLayer4StyleInstance{InPlaceInit, ui.createAnimator(), calls};
    layer4.setAnimator(*animatorLayer4StyleNoAdvanceNeededInstance);
    layer4.setAnimator(*animatorLayer4StyleInstance);
    StyleAnimator& animatorLayer4StyleNoAdvanceNeeded = ui.setStyleAnimatorInstance(Utility::move(animatorLayer4StyleNoAdvanceNeededInstance));
    StyleAnimator& animatorLayer4Style = ui.setStyleAnimatorInstance(Utility::move(animatorLayer4StyleInstance));

    /* It's important to remove an animator that has an instance already --
       animators without an instance aren't even added to the list of animators
       to process in advanceAnimations() */
    ui.removeAnimator(animatorRemoved.handle());

    /* One scheduled, one stopped, one playing with empty AnimatorFeatures;
       then one stopped and one playing with NodeAttachment, a NodeAnimator and
       DataAttachment */
    animator1.create(5_nsec, 10_nsec);
    animatorNoAdvanceNeeded.create(-50_nsec, 10_nsec, AnimationFlag::KeepOncePlayed);
    animator2.create(0_nsec, 10_nsec);
    /* Not attaching these to any node, should work even then */
    animatorNodeAttachmentNoAdvanceNeeded.create(-50_nsec, 10_nsec, AnimationFlag::KeepOncePlayed);
    animatorNodeAttachment.create(5_nsec, 10_nsec);
    animatorNodeNoAdvanceNeeded.create(-50_nsec, 10_nsec, NodeHandle::Null, AnimationFlag::KeepOncePlayed);
    animatorNode.create(5_nsec, 10_nsec, NodeHandle::Null);
    /* Not attaching these to any data, should work even then */
    animatorLayer2DataAttachmentNoAdvanceNeeded.create(-50_nsec, 10_nsec, AnimationFlag::KeepOncePlayed);
    animatorLayer3DataAttachment.create(5_nsec, 10_nsec);
    animatorLayer3DataNoAdvanceNeeded.create(-50_nsec, 10_nsec, DataHandle::Null, AnimationFlag::KeepOncePlayed);
    animatorLayer3Data.create(5_nsec, 10_nsec, DataHandle::Null);
    animatorLayer4StyleNoAdvanceNeeded.create(-50_nsec, 10_nsec, DataHandle::Null, AnimationFlag::KeepOncePlayed);
    animatorLayer4Style.create(5_nsec, 10_nsec, DataHandle::Null);
    CORRADE_COMPARE(animator1.state(), AnimatorState::NeedsAdvance);
    CORRADE_COMPARE(animatorNoAdvanceNeeded.state(), AnimatorStates{});
    CORRADE_COMPARE(animator2.state(), AnimatorState::NeedsAdvance);
    CORRADE_COMPARE(animatorNodeAttachmentNoAdvanceNeeded.state(), AnimatorStates{});
    CORRADE_COMPARE(animatorNodeAttachment.state(), AnimatorState::NeedsAdvance);
    CORRADE_COMPARE(animatorNodeNoAdvanceNeeded.state(), AnimatorStates{});
    CORRADE_COMPARE(animatorNode.state(), AnimatorState::NeedsAdvance);
    CORRADE_COMPARE(animatorLayer2DataAttachmentNoAdvanceNeeded.state(), AnimatorStates{});
    CORRADE_COMPARE(animatorLayer3DataAttachment.state(), AnimatorState::NeedsAdvance);
    CORRADE_COMPARE(animatorLayer3DataNoAdvanceNeeded.state(), AnimatorStates{});
    CORRADE_COMPARE(animatorLayer3Data.state(), AnimatorState::NeedsAdvance);
    CORRADE_COMPARE(animatorLayer4StyleNoAdvanceNeeded.state(), AnimatorStates{});
    CORRADE_COMPARE(animatorLayer4Style.state(), AnimatorState::NeedsAdvance);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsAnimationAdvance);

    /* Initially all animators are at 0 time. This changes if advance() is
       called on them. */
    CORRADE_COMPARE(animator1.time(), 0_nsec);
    CORRADE_COMPARE(animatorNoAdvanceNeeded.time(), 0_nsec);
    CORRADE_COMPARE(animator2.time(), 0_nsec);
    CORRADE_COMPARE(animatorNodeAttachmentNoAdvanceNeeded.time(), 0_nsec);
    CORRADE_COMPARE(animatorNodeAttachment.time(), 0_nsec);
    CORRADE_COMPARE(animatorNodeNoAdvanceNeeded.time(), 0_nsec);
    CORRADE_COMPARE(animatorNode.time(), 0_nsec);
    CORRADE_COMPARE(animatorLayer2DataAttachmentNoAdvanceNeeded.time(), 0_nsec);
    CORRADE_COMPARE(animatorLayer3DataAttachment.time(), 0_nsec);
    CORRADE_COMPARE(animatorLayer3DataNoAdvanceNeeded.time(), 0_nsec);
    CORRADE_COMPARE(animatorLayer3Data.time(), 0_nsec);
    CORRADE_COMPARE(animatorLayer4StyleNoAdvanceNeeded.time(), 0_nsec);
    CORRADE_COMPARE(animatorLayer4Style.time(), 0_nsec);

    /* First advance. The scheduled animations aren't advanced yet (and there's
       nothing else to call doAdvance() for), the playing is. */
    ui.advanceAnimations(2_nsec);
    CORRADE_COMPARE_AS(calls, (Containers::arrayView<Containers::Pair<AnimatorHandle, Call>>({
        {animator2.handle(), Advance},
    })), TestSuite::Compare::Container);
    CORRADE_COMPARE(animator1.state(), AnimatorState::NeedsAdvance);
    CORRADE_COMPARE(animatorNoAdvanceNeeded.state(), AnimatorStates{});
    CORRADE_COMPARE(animator2.state(), AnimatorState::NeedsAdvance);
    CORRADE_COMPARE(animatorNodeAttachmentNoAdvanceNeeded.state(), AnimatorStates{});
    CORRADE_COMPARE(animatorNodeAttachment.state(), AnimatorState::NeedsAdvance);
    CORRADE_COMPARE(animatorNodeNoAdvanceNeeded.state(), AnimatorStates{});
    CORRADE_COMPARE(animatorNode.state(), AnimatorState::NeedsAdvance);
    CORRADE_COMPARE(animatorLayer2DataAttachmentNoAdvanceNeeded.state(), AnimatorStates{});
    CORRADE_COMPARE(animatorLayer3DataAttachment.state(), AnimatorState::NeedsAdvance);
    CORRADE_COMPARE(animatorLayer3DataNoAdvanceNeeded.state(), AnimatorStates{});
    CORRADE_COMPARE(animatorLayer3Data.state(), AnimatorState::NeedsAdvance);
    CORRADE_COMPARE(animatorLayer4StyleNoAdvanceNeeded.state(), AnimatorStates{});
    CORRADE_COMPARE(animatorLayer4Style.state(), AnimatorState::NeedsAdvance);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsAnimationAdvance);

    /* advance() wasn't even called on the second, fourth, sixth, eighth,
       tenth and twelfth one */
    CORRADE_COMPARE(animator1.time(), 2_nsec);
    CORRADE_COMPARE(animatorNoAdvanceNeeded.time(), 0_nsec);
    CORRADE_COMPARE(animator2.time(), 2_nsec);
    CORRADE_COMPARE(animatorNodeAttachmentNoAdvanceNeeded.time(), 0_nsec);
    CORRADE_COMPARE(animatorNodeAttachment.time(), 2_nsec);
    CORRADE_COMPARE(animatorNodeNoAdvanceNeeded.time(), 0_nsec);
    CORRADE_COMPARE(animatorNode.time(), 2_nsec);
    CORRADE_COMPARE(animatorLayer2DataAttachmentNoAdvanceNeeded.time(), 0_nsec);
    CORRADE_COMPARE(animatorLayer3DataAttachment.time(), 2_nsec);
    CORRADE_COMPARE(animatorLayer3DataNoAdvanceNeeded.time(), 0_nsec);
    CORRADE_COMPARE(animatorLayer3Data.time(), 2_nsec);
    CORRADE_COMPARE(animatorLayer4StyleNoAdvanceNeeded.time(), 0_nsec);
    CORRADE_COMPARE(animatorLayer4Style.time(), 2_nsec);

    /* Second advance, the seven get further advanced, the second gets also
       cleaned */
    calls = {};
    ui.advanceAnimations(10_nsec);
    CORRADE_COMPARE_AS(calls, (Containers::arrayView<Containers::Pair<AnimatorHandle, Call>>({
        {animator1.handle(), Advance},
        {animator2.handle(), Advance},
        {animator2.handle(), Clean},
        {animatorNodeAttachment.handle(), Advance},
        {animatorLayer3DataAttachment.handle(), Advance},
        /* All generic animators get executed first, then node animators */
        {animatorNode.handle(), Advance},
        /* Then data animators */
        {animatorLayer3Data.handle(), Advance},
        /* And then (another layer) style animators */
        {animatorLayer4Style.handle(), Advance},
    })), TestSuite::Compare::Container);
    CORRADE_COMPARE(animator1.state(), AnimatorState::NeedsAdvance);
    CORRADE_COMPARE(animatorNoAdvanceNeeded.state(), AnimatorStates{});
    CORRADE_COMPARE(animator2.state(), AnimatorStates{});
    CORRADE_COMPARE(animatorNodeAttachmentNoAdvanceNeeded.state(), AnimatorStates{});
    CORRADE_COMPARE(animatorNodeAttachment.state(), AnimatorState::NeedsAdvance);
    CORRADE_COMPARE(animatorNodeNoAdvanceNeeded.state(), AnimatorStates{});
    CORRADE_COMPARE(animatorNode.state(), AnimatorState::NeedsAdvance);
    CORRADE_COMPARE(animatorLayer2DataAttachmentNoAdvanceNeeded.state(), AnimatorStates{});
    CORRADE_COMPARE(animatorLayer3DataAttachment.state(), AnimatorState::NeedsAdvance);
    CORRADE_COMPARE(animatorLayer3DataNoAdvanceNeeded.state(), AnimatorStates{});
    CORRADE_COMPARE(animatorLayer3Data.state(), AnimatorState::NeedsAdvance);
    CORRADE_COMPARE(animatorLayer4StyleNoAdvanceNeeded.state(), AnimatorStates{});
    CORRADE_COMPARE(animatorLayer4Style.state(), AnimatorState::NeedsAdvance);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsAnimationAdvance);

    /* advance() wasn't even called on the second, fourth, sixth, eighth, tenth
       and twelfth one now either */
    CORRADE_COMPARE(animator1.time(), 10_nsec);
    CORRADE_COMPARE(animatorNoAdvanceNeeded.time(), 0_nsec);
    CORRADE_COMPARE(animator2.time(), 10_nsec);
    CORRADE_COMPARE(animatorNodeAttachmentNoAdvanceNeeded.time(), 0_nsec);
    CORRADE_COMPARE(animatorNodeAttachment.time(), 10_nsec);
    CORRADE_COMPARE(animatorNodeNoAdvanceNeeded.time(), 0_nsec);
    CORRADE_COMPARE(animatorNode.time(), 10_nsec);
    CORRADE_COMPARE(animatorLayer2DataAttachmentNoAdvanceNeeded.time(), 0_nsec);
    CORRADE_COMPARE(animatorLayer3DataAttachment.time(), 10_nsec);
    CORRADE_COMPARE(animatorLayer3DataNoAdvanceNeeded.time(), 0_nsec);
    CORRADE_COMPARE(animatorLayer3Data.time(), 10_nsec);
    CORRADE_COMPARE(animatorLayer4StyleNoAdvanceNeeded.time(), 0_nsec);
    CORRADE_COMPARE(animatorLayer4Style.time(), 10_nsec);

    /* Third advance, only the first, fifth, seventh, ninth, eleventh and
       thirteenth is advanced & cleaned */
    calls = {};
    ui.advanceAnimations(15_nsec);
    CORRADE_COMPARE_AS(calls, (Containers::arrayView<Containers::Pair<AnimatorHandle, Call>>({
        {animator1.handle(), Advance},
        {animator1.handle(), Clean},
        {animatorNodeAttachment.handle(), Advance},
        {animatorNodeAttachment.handle(), Clean},
        {animatorLayer3DataAttachment.handle(), Advance},
        {animatorLayer3DataAttachment.handle(), Clean},
        /* All generic animators get executed first, then node animators */
        {animatorNode.handle(), Advance},
        {animatorNode.handle(), Clean},
        /* Then data animators */
        {animatorLayer3Data.handle(), Advance},
        {animatorLayer3Data.handle(), Clean},
        /* And then (another layer) style animators */
        {animatorLayer4Style.handle(), Advance},
        {animatorLayer4Style.handle(), Clean},
    })), TestSuite::Compare::Container);
    CORRADE_COMPARE(animator1.state(), AnimatorStates{});
    CORRADE_COMPARE(animatorNoAdvanceNeeded.state(), AnimatorStates{});
    CORRADE_COMPARE(animator2.state(), AnimatorStates{});
    CORRADE_COMPARE(animatorNodeAttachmentNoAdvanceNeeded.state(), AnimatorStates{});
    CORRADE_COMPARE(animatorNodeAttachment.state(), AnimatorStates{});
    CORRADE_COMPARE(animatorNodeNoAdvanceNeeded.state(), AnimatorStates{});
    CORRADE_COMPARE(animatorNode.state(), AnimatorStates{});
    CORRADE_COMPARE(animatorLayer2DataAttachmentNoAdvanceNeeded.state(), AnimatorStates{});
    CORRADE_COMPARE(animatorLayer3DataAttachment.state(), AnimatorStates{});
    CORRADE_COMPARE(animatorLayer3DataNoAdvanceNeeded.state(), AnimatorStates{});
    CORRADE_COMPARE(animatorLayer3Data.state(), AnimatorState{});
    CORRADE_COMPARE(animatorLayer4StyleNoAdvanceNeeded.state(), AnimatorStates{});
    CORRADE_COMPARE(animatorLayer4Style.state(), AnimatorState{});
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

    /* advance() wasn't even called on the second, third, fourth, sixth,
       eighth, tenth and twelfth */
    CORRADE_COMPARE(animator1.time(), 15_nsec);
    CORRADE_COMPARE(animatorNoAdvanceNeeded.time(), 0_nsec);
    CORRADE_COMPARE(animator2.time(), 10_nsec);
    CORRADE_COMPARE(animatorNodeAttachmentNoAdvanceNeeded.time(), 0_nsec);
    CORRADE_COMPARE(animatorNodeAttachment.time(), 15_nsec);
    CORRADE_COMPARE(animatorNodeNoAdvanceNeeded.time(), 0_nsec);
    CORRADE_COMPARE(animatorNode.time(), 15_nsec);
    CORRADE_COMPARE(animatorLayer2DataAttachmentNoAdvanceNeeded.time(), 0_nsec);
    CORRADE_COMPARE(animatorLayer3DataAttachment.time(), 15_nsec);
    CORRADE_COMPARE(animatorLayer3DataNoAdvanceNeeded.time(), 0_nsec);
    CORRADE_COMPARE(animatorLayer3DataAttachment.time(), 15_nsec);
    CORRADE_COMPARE(animatorLayer3DataNoAdvanceNeeded.time(), 0_nsec);
    CORRADE_COMPARE(animatorLayer3Data.time(), 15_nsec);
    CORRADE_COMPARE(animatorLayer4StyleNoAdvanceNeeded.time(), 0_nsec);
    CORRADE_COMPARE(animatorLayer4Style.time(), 15_nsec);

    /* Fourth advance, nothing left to be done */
    calls = {};
    ui.advanceAnimations(20_nsec);
    CORRADE_COMPARE_AS(calls, (Containers::arrayView<Containers::Pair<AnimatorHandle, Call>>({
    })), TestSuite::Compare::Container);
    CORRADE_COMPARE(animator1.state(), AnimatorStates{});
    CORRADE_COMPARE(animatorNoAdvanceNeeded.state(), AnimatorStates{});
    CORRADE_COMPARE(animator2.state(), AnimatorStates{});
    CORRADE_COMPARE(animatorNodeAttachmentNoAdvanceNeeded.state(), AnimatorStates{});
    CORRADE_COMPARE(animatorNodeAttachment.state(), AnimatorStates{});
    CORRADE_COMPARE(animatorNodeNoAdvanceNeeded.state(), AnimatorStates{});
    CORRADE_COMPARE(animatorNode.state(), AnimatorStates{});
    CORRADE_COMPARE(animatorLayer2DataAttachmentNoAdvanceNeeded.state(), AnimatorStates{});
    CORRADE_COMPARE(animatorLayer3DataAttachment.state(), AnimatorStates{});
    CORRADE_COMPARE(animatorLayer3DataNoAdvanceNeeded.state(), AnimatorStates{});
    CORRADE_COMPARE(animatorLayer3Data.state(), AnimatorState{});
    CORRADE_COMPARE(animatorLayer4StyleNoAdvanceNeeded.state(), AnimatorStates{});
    CORRADE_COMPARE(animatorLayer4Style.state(), AnimatorState{});
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

    /* advance() wasn't called on any */
    CORRADE_COMPARE(animator1.time(), 15_nsec);
    CORRADE_COMPARE(animatorNoAdvanceNeeded.time(), 0_nsec);
    CORRADE_COMPARE(animator2.time(), 10_nsec);
    CORRADE_COMPARE(animatorNodeAttachmentNoAdvanceNeeded.time(), 0_nsec);
    CORRADE_COMPARE(animatorNodeAttachment.time(), 15_nsec);
    CORRADE_COMPARE(animatorNodeNoAdvanceNeeded.time(), 0_nsec);
    CORRADE_COMPARE(animatorNode.time(), 15_nsec);
    CORRADE_COMPARE(animatorLayer2DataAttachmentNoAdvanceNeeded.time(), 0_nsec);
    CORRADE_COMPARE(animatorLayer3DataAttachment.time(), 15_nsec);
    CORRADE_COMPARE(animatorLayer3DataNoAdvanceNeeded.time(), 0_nsec);
    CORRADE_COMPARE(animatorLayer3Data.time(), 15_nsec);
    CORRADE_COMPARE(animatorLayer4StyleNoAdvanceNeeded.time(), 0_nsec);
    CORRADE_COMPARE(animatorLayer4Style.time(), 15_nsec);
}

void AbstractUserInterfaceTest::advanceAnimationsGeneric() {
    auto&& data = AdvanceAnimationsGenericData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Verifies that generic animators have doAdvance() and doClean() called
       with proper data. Handling multiple heterogenous animators is tested in
       advanceAnimations(), effect of the animators on state flags etc is
       tested in stateAnimations(). */

    AbstractUserInterface ui{{100, 100}};

    struct Animator: AbstractGenericAnimator {
        explicit Animator(AnimatorHandle handle, AnimatorFeatures features): AbstractGenericAnimator{handle}, _features{features} {}

        using AbstractGenericAnimator::setLayer;
        using AbstractGenericAnimator::create;

        AnimatorFeatures doFeatures() const override { return _features; }
        void doAdvance(Containers::BitArrayView active, const Containers::StridedArrayView1D<const Float>& factors) override {
            CORRADE_COMPARE_AS(active,
                expectedActive,
                TestSuite::Compare::Container);
            for(std::size_t i = 0; i != active.size(); ++i) if(active[i]) {
                CORRADE_ITERATION(i);
                CORRADE_COMPARE(factors[i], expectedFactors[i]);
            }
            ++advanceCallCount;
        }
        void doClean(Containers::BitArrayView animationIdsToRemove) override {
            CORRADE_COMPARE_AS(animationIdsToRemove,
                expectedAnimationIdsToRemove,
                TestSuite::Compare::Container);
            ++cleanCallCount;
        }

        Containers::StridedBitArrayView1D expectedActive;
        Containers::StridedBitArrayView1D expectedAnimationIdsToRemove;
        Containers::StridedArrayView1D<const Float> expectedFactors;
        Int advanceCallCount = 0;
        Int cleanCallCount = 0;

        private:
            AnimatorFeatures _features;
    };

    Containers::Pointer<Animator> animatorInstance{InPlaceInit, ui.createAnimator(), data.features};
    if(data.features >= AnimatorFeature::DataAttachment) {
        struct Layer: AbstractLayer {
            using AbstractLayer::AbstractLayer;

            LayerFeatures doFeatures() const override { return {}; }
        };
        Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer()));
        animatorInstance->setLayer(layer);
    }
    Animator& animator = ui.setGenericAnimatorInstance(Utility::move(animatorInstance));

    /* Call to advance(5) advances the first, nothing to clean */
    animator.create(0_nsec, 10_nsec);
    animator.create(-20_nsec, 10_nsec, AnimationFlag::KeepOncePlayed);
    animator.create(6_nsec, 4_nsec);
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
        bool active[]{
            true,
            false,
            false
        };
        Float factors[]{
            0.5f,
            0.0f, /* unused */
            0.0f, /* unused */
        };
        animator.expectedActive = Containers::stridedArrayView(active).sliceBit(0);
        animator.expectedFactors = factors;
        animator.expectedAnimationIdsToRemove = {};
        ui.advanceAnimations(5_nsec);
    }
    CORRADE_COMPARE(animator.advanceCallCount, 1);
    CORRADE_COMPARE(animator.cleanCallCount, 0);

    /* Call to advance(10) advances the first and last to end, both get
       cleaned afterwards */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
        bool active[]{
            true,
            false,
            true
        };
        Float factors[]{
            1.0f,
            0.0f, /* unused */
            1.0f,
        };
        bool animationIdsToRemove[]{
            true,
            false,
            true
        };
        animator.expectedActive = Containers::stridedArrayView(active).sliceBit(0);
        animator.expectedFactors = factors;
        animator.expectedAnimationIdsToRemove = Containers::stridedArrayView(animationIdsToRemove).sliceBit(0);
        ui.advanceAnimations(10_nsec);
    }
    CORRADE_COMPARE(animator.advanceCallCount, 2);
    CORRADE_COMPARE(animator.cleanCallCount, 1);

    /* Call to advance(20) does nothing */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
        animator.expectedActive = {};
        animator.expectedFactors = {};
        animator.expectedAnimationIdsToRemove = {};
        ui.advanceAnimations(20_nsec);
    }
    CORRADE_COMPARE(animator.advanceCallCount, 2);
    CORRADE_COMPARE(animator.cleanCallCount, 1);
}

void AbstractUserInterfaceTest::advanceAnimationsNode() {
    /* Verifies that node animators have doAdvance() and doClean() called
       with proper data. Handling multiple heterogenous animators is tested in
       advanceAnimations(), effect of the animators on state flags etc is
       tested in stateAnimations(). */

    AbstractUserInterface ui{{100, 100}};

    struct Animator: AbstractNodeAnimator {
        using AbstractNodeAnimator::AbstractNodeAnimator;
        using AbstractNodeAnimator::create;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::NodeAttachment;
        }
        NodeAnimations doAdvance(Containers::BitArrayView active, const Containers::StridedArrayView1D<const Float>& factors, const Containers::StridedArrayView1D<Vector2>& nodeOffsets, const Containers::StridedArrayView1D<Vector2>& nodeSizes, const Containers::StridedArrayView1D<NodeFlags>& nodeFlags, Containers::MutableBitArrayView nodesRemove) override {
            CORRADE_COMPARE_AS(active,
                expectedActive,
                TestSuite::Compare::Container);
            for(std::size_t i = 0; i != active.size(); ++i) if(active[i]) {
                CORRADE_ITERATION(i);
                CORRADE_COMPARE(factors[i], expectedFactors[i]);
            }
            /* We're not touching the output in any way, just verify it was
               passed through correctly */
            CORRADE_COMPARE_AS(nodeOffsets, Containers::stridedArrayView<Vector2>({
                {1.0f, 2.0f},
                {3.0f, 4.0f},
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(nodeSizes, Containers::stridedArrayView<Vector2>({
                {5.0f, 6.0f},
                {8.0f, 9.0f},
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(nodeFlags, Containers::stridedArrayView({
                NodeFlags{},
                NodeFlag::Clip|NodeFlag::Disabled,
            }), TestSuite::Compare::Container);
            /* The nodesRemove view is all zeros initially */
            CORRADE_COMPARE_AS(nodesRemove, Containers::stridedArrayView({
                false,
                false
            }).sliceBit(0), TestSuite::Compare::Container);
            ++advanceCallCount;

            return {};
        }
        void doClean(Containers::BitArrayView animationIdsToRemove) override {
            CORRADE_COMPARE_AS(animationIdsToRemove,
                expectedAnimationIdsToRemove,
                TestSuite::Compare::Container);
            ++cleanCallCount;
        }

        Containers::StridedBitArrayView1D expectedActive;
        Containers::StridedBitArrayView1D expectedAnimationIdsToRemove;
        Containers::StridedArrayView1D<const Float> expectedFactors;
        Int advanceCallCount = 0;
        Int cleanCallCount = 0;
    };
    Animator& animator = ui.setNodeAnimatorInstance(Containers::pointer<Animator>(ui.createAnimator()));

    ui.createNode({1.0f, 2.0f}, {5.0f, 6.0f});
    ui.createNode({3.0f, 4.0f}, {8.0f, 9.0f}, NodeFlag::Clip|NodeFlag::Disabled);

    /* Call to advance(5) advances the first, nothing to clean */
    animator.create(0_nsec, 10_nsec);
    animator.create(-20_nsec, 10_nsec, AnimationFlag::KeepOncePlayed);
    animator.create(6_nsec, 4_nsec);
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
        bool active[]{
            true,
            false,
            false
        };
        Float factors[]{
            0.5f,
            0.0f, /* unused */
            0.0f, /* unused */
        };
        animator.expectedActive = Containers::stridedArrayView(active).sliceBit(0);
        animator.expectedFactors = factors;
        animator.expectedAnimationIdsToRemove = {};
        ui.advanceAnimations(5_nsec);
    }
    CORRADE_COMPARE(animator.advanceCallCount, 1);
    CORRADE_COMPARE(animator.cleanCallCount, 0);

    /* Call to advance(10) advances the first and last to end, both get
       cleaned afterwards */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
        bool active[]{
            true,
            false,
            true
        };
        Float factors[]{
            1.0f,
            0.0f, /* unused */
            1.0f,
        };
        bool animationIdsToRemove[]{
            true,
            false,
            true
        };
        animator.expectedActive = Containers::stridedArrayView(active).sliceBit(0);
        animator.expectedFactors = factors;
        animator.expectedAnimationIdsToRemove = Containers::stridedArrayView(animationIdsToRemove).sliceBit(0);
        ui.advanceAnimations(10_nsec);
    }
    CORRADE_COMPARE(animator.advanceCallCount, 2);
    CORRADE_COMPARE(animator.cleanCallCount, 1);

    /* Call to advance(20) does nothing */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
        animator.expectedActive = {};
        animator.expectedFactors = {};
        animator.expectedAnimationIdsToRemove = {};
        ui.advanceAnimations(20_nsec);
    }
    CORRADE_COMPARE(animator.advanceCallCount, 2);
    CORRADE_COMPARE(animator.cleanCallCount, 1);
}

void AbstractUserInterfaceTest::advanceAnimationsData() {
    /* Verifies that data animators have doAdvanceAnimations() and doClean()
       called with proper data. Handling multiple heterogenous animators is
       tested in advanceAnimations(), effect of the animators on state flags
       etc is tested in stateAnimations(). */

    AbstractUserInterface ui{{100, 100}};

    struct Animator: AbstractDataAnimator {
        using AbstractDataAnimator::AbstractDataAnimator;
        using AbstractDataAnimator::create;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::DataAttachment;
        }
        void advance(Containers::BitArrayView active, const Containers::StridedArrayView1D<const Float>& factors) {
            CORRADE_COMPARE_AS(active,
                expectedActive,
                TestSuite::Compare::Container);
            for(std::size_t i = 0; i != active.size(); ++i) if(active[i]) {
                CORRADE_ITERATION(i);
                CORRADE_COMPARE(factors[i], expectedFactors[i]);
            }
            ++advanceCallCount;
        }
        void doClean(Containers::BitArrayView animationIdsToRemove) override {
            CORRADE_COMPARE_AS(animationIdsToRemove,
                expectedAnimationIdsToRemove,
                TestSuite::Compare::Container);
            ++cleanCallCount;
        }

        Containers::StridedBitArrayView1D expectedActive;
        Containers::StridedBitArrayView1D expectedAnimationIdsToRemove;
        Containers::StridedArrayView1D<const Float> expectedFactors;
        Int advanceCallCount = 0;
        Int cleanCallCount = 0;

        private:
            AnimatorFeatures _features;
    };

    Containers::Pointer<Animator> animatorInstance{InPlaceInit, ui.createAnimator()};

    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override {
            return LayerFeature::AnimateData;
        }
        void setAnimator(Animator& animator) const {
            AbstractLayer::setAnimator(animator);
        }

        void doAdvanceAnimations(Nanoseconds time, Containers::MutableBitArrayView activeStorage, const Containers::StridedArrayView1D<Float>& factorStorage, Containers::MutableBitArrayView removeStorage, const Containers::Iterable<AbstractDataAnimator>& animators) override {
            for(AbstractDataAnimator& animator: animators) {
                if(!(animator.state() >= AnimatorState::NeedsAdvance))
                    continue;

                /* Not strictly necessary in order to test things but shows how
                   a real implementation would look like. */
                Containers::Pair<bool, bool> advanceCleanNeeded = animator.update(time, activeStorage, factorStorage, removeStorage);
                if(advanceCleanNeeded.first())
                    static_cast<Animator&>(animator).advance(activeStorage, factorStorage);
                if(advanceCleanNeeded.second())
                    animator.clean(removeStorage);
            }
        }
    };
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer()));
    layer.setAnimator(*animatorInstance);
    Animator& animator = ui.setDataAnimatorInstance(Utility::move(animatorInstance));

    /* Call to advance(5) advances the first, nothing to clean */
    animator.create(0_nsec, 10_nsec);
    animator.create(-20_nsec, 10_nsec, AnimationFlag::KeepOncePlayed);
    animator.create(6_nsec, 4_nsec);
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
        bool active[]{
            true,
            false,
            false
        };
        Float factors[]{
            0.5f,
            0.0f, /* unused */
            0.0f, /* unused */
        };
        animator.expectedActive = Containers::stridedArrayView(active).sliceBit(0);
        animator.expectedFactors = factors;
        animator.expectedAnimationIdsToRemove = {};
        ui.advanceAnimations(5_nsec);
    }
    CORRADE_COMPARE(animator.advanceCallCount, 1);
    CORRADE_COMPARE(animator.cleanCallCount, 0);

    /* Call to advance(10) advances the first and last to end, both get
       cleaned afterwards */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
        bool active[]{
            true,
            false,
            true
        };
        Float factors[]{
            1.0f,
            0.0f, /* unused */
            1.0f,
        };
        bool animationIdsToRemove[]{
            true,
            false,
            true
        };
        animator.expectedActive = Containers::stridedArrayView(active).sliceBit(0);
        animator.expectedFactors = factors;
        animator.expectedAnimationIdsToRemove = Containers::stridedArrayView(animationIdsToRemove).sliceBit(0);
        ui.advanceAnimations(10_nsec);
    }
    CORRADE_COMPARE(animator.advanceCallCount, 2);
    CORRADE_COMPARE(animator.cleanCallCount, 1);

    /* Call to advance(20) does nothing */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
        animator.expectedActive = {};
        animator.expectedFactors = {};
        animator.expectedAnimationIdsToRemove = {};
        ui.advanceAnimations(20_nsec);
    }
    CORRADE_COMPARE(animator.advanceCallCount, 2);
    CORRADE_COMPARE(animator.cleanCallCount, 1);
}

void AbstractUserInterfaceTest::advanceAnimationsStyle() {
    /* Verifies that style animators have doAdvanceAnimations() and doClean()
       called with proper data. Handling multiple heterogenous animators is
       tested in advanceAnimations(), effect of the animators on state flags
       etc is tested in stateAnimations(). */

    AbstractUserInterface ui{{100, 100}};

    struct Animator: AbstractStyleAnimator {
        using AbstractStyleAnimator::AbstractStyleAnimator;
        using AbstractStyleAnimator::create;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::DataAttachment;
        }
        void advance(Containers::BitArrayView active, const Containers::StridedArrayView1D<const Float>& factors) {
            CORRADE_COMPARE_AS(active,
                expectedActive,
                TestSuite::Compare::Container);
            for(std::size_t i = 0; i != active.size(); ++i) if(active[i]) {
                CORRADE_ITERATION(i);
                CORRADE_COMPARE(factors[i], expectedFactors[i]);
            }
            ++advanceCallCount;
        }
        void doClean(Containers::BitArrayView animationIdsToRemove) override {
            CORRADE_COMPARE_AS(animationIdsToRemove,
                expectedAnimationIdsToRemove,
                TestSuite::Compare::Container);
            ++cleanCallCount;
        }

        Containers::StridedBitArrayView1D expectedActive;
        Containers::StridedBitArrayView1D expectedAnimationIdsToRemove;
        Containers::StridedArrayView1D<const Float> expectedFactors;
        Int advanceCallCount = 0;
        Int cleanCallCount = 0;

        private:
            AnimatorFeatures _features;
    };

    Containers::Pointer<Animator> animatorInstance{InPlaceInit, ui.createAnimator()};

    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override {
            return LayerFeature::AnimateStyles;
        }
        void setAnimator(Animator& animator) const {
            AbstractLayer::setAnimator(animator);
        }

        void doAdvanceAnimations(Nanoseconds time, Containers::MutableBitArrayView activeStorage, const Containers::StridedArrayView1D<Float>& factorStorage, Containers::MutableBitArrayView removeStorage, const Containers::Iterable<AbstractStyleAnimator>& animators) override {
            for(AbstractStyleAnimator& animator: animators) {
                if(!(animator.state() >= AnimatorState::NeedsAdvance))
                    continue;

                /* Not strictly necessary in order to test things but shows how
                   a real implementation would look like. */
                Containers::Pair<bool, bool> advanceCleanNeeded = animator.update(time, activeStorage, factorStorage, removeStorage);
                if(advanceCleanNeeded.first())
                    static_cast<Animator&>(animator).advance(activeStorage, factorStorage);
                if(advanceCleanNeeded.second())
                    animator.clean(removeStorage);
            }
        }
    };
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer()));
    layer.setAnimator(*animatorInstance);
    Animator& animator = ui.setStyleAnimatorInstance(Utility::move(animatorInstance));

    /* Call to advance(5) advances the first, nothing to clean */
    animator.create(0_nsec, 10_nsec);
    animator.create(-20_nsec, 10_nsec, AnimationFlag::KeepOncePlayed);
    animator.create(6_nsec, 4_nsec);
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
        bool active[]{
            true,
            false,
            false
        };
        Float factors[]{
            0.5f,
            0.0f, /* unused */
            0.0f, /* unused */
        };
        animator.expectedActive = Containers::stridedArrayView(active).sliceBit(0);
        animator.expectedFactors = factors;
        animator.expectedAnimationIdsToRemove = {};
        ui.advanceAnimations(5_nsec);
    }
    CORRADE_COMPARE(animator.advanceCallCount, 1);
    CORRADE_COMPARE(animator.cleanCallCount, 0);

    /* Call to advance(10) advances the first and last to end, both get
       cleaned afterwards */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
        bool active[]{
            true,
            false,
            true
        };
        Float factors[]{
            1.0f,
            0.0f, /* unused */
            1.0f,
        };
        bool animationIdsToRemove[]{
            true,
            false,
            true
        };
        animator.expectedActive = Containers::stridedArrayView(active).sliceBit(0);
        animator.expectedFactors = factors;
        animator.expectedAnimationIdsToRemove = Containers::stridedArrayView(animationIdsToRemove).sliceBit(0);
        ui.advanceAnimations(10_nsec);
    }
    CORRADE_COMPARE(animator.advanceCallCount, 2);
    CORRADE_COMPARE(animator.cleanCallCount, 1);

    /* Call to advance(20) does nothing */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
        animator.expectedActive = {};
        animator.expectedFactors = {};
        animator.expectedAnimationIdsToRemove = {};
        ui.advanceAnimations(20_nsec);
    }
    CORRADE_COMPARE(animator.advanceCallCount, 2);
    CORRADE_COMPARE(animator.cleanCallCount, 1);
}

void AbstractUserInterfaceTest::advanceAnimationsInvalidTime() {
    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui{{100, 100}};

    /* Same time should be okay */
    ui.advanceAnimations(56_nsec);
    ui.advanceAnimations(56_nsec);
    CORRADE_COMPARE(ui.animationTime(), 56_nsec);

    std::ostringstream out;
    Error redirectError{&out};
    ui.advanceAnimations(55_nsec);
    CORRADE_COMPARE(out.str(), "Whee::AbstractUserInterface::advanceAnimations(): expected a time at least Nanoseconds(56) but got Nanoseconds(55)\n");
}

void AbstractUserInterfaceTest::updateOrder() {
    auto&& data = UpdateOrderData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui{{100, 100}};

    struct Layer: AbstractLayer {
        explicit Layer(LayerHandle handle, Containers::Array<LayerHandle>& order): AbstractLayer{handle}, _order(order) {}

        LayerFeatures doFeatures() const override { return {}; }
        void doUpdate(LayerStates, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, Containers::BitArrayView, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&) override {
            arrayAppend(_order, handle());
        }

        private:
            Containers::Array<LayerHandle>& _order;
    };

    LayerHandle layer1, layer2, layer3NoInstance, layer4, layer5Removed, layer6;
    if(!data.shuffle) {
        layer1 = ui.createLayer();
        layer2 = ui.createLayer();
        layer3NoInstance = ui.createLayer();
        layer4 = ui.createLayer();
        layer5Removed = ui.createLayer();
        layer6 = ui.createLayer();
    } else {
        layer3NoInstance = ui.createLayer();
        layer1 = ui.createLayer(layer3NoInstance);
        layer2 = ui.createLayer(layer3NoInstance);
        layer6 = ui.createLayer();
        layer4 = ui.createLayer(layer6);
        layer5Removed = ui.createLayer(layer6);
    }

    CORRADE_COMPARE(ui.layerFirst(), layer1);
    CORRADE_COMPARE(ui.layerNext(layer1), layer2);
    CORRADE_COMPARE(ui.layerNext(layer2), layer3NoInstance);
    CORRADE_COMPARE(ui.layerNext(layer3NoInstance), layer4);
    CORRADE_COMPARE(ui.layerNext(layer4), layer5Removed);
    CORRADE_COMPARE(ui.layerNext(layer5Removed), layer6);
    CORRADE_COMPARE(ui.layerNext(layer6), LayerHandle::Null);

    Containers::Array<LayerHandle> order;
    ui.setLayerInstance(Containers::pointer<Layer>(layer1, order));
    ui.setLayerInstance(Containers::pointer<Layer>(layer2, order));
    /* No instance for layer 3 */
    ui.setLayerInstance(Containers::pointer<Layer>(layer4, order));
    ui.removeLayer(layer5Removed);
    ui.setLayerInstance(Containers::pointer<Layer>(layer6, order));

    /* Initially the update goes through everything due to the layer being
       removed */
    ui.update();
    CORRADE_COMPARE_AS(order, Containers::arrayView({
        layer1,
        layer2,
        layer4,
        layer6,
    }), TestSuite::Compare::Container);

    /* Next time it should go only through layers that actually have any
       state set, but still in order */
    order = {};
    ui.layer(layer6).setNeedsUpdate(LayerState::NeedsDataUpdate);
    ui.layer(layer2).setNeedsUpdate(LayerState::NeedsCommonDataUpdate);
    ui.update();
    CORRADE_COMPARE_AS(order, Containers::arrayView({
        layer2,
        layer6,
    }), TestSuite::Compare::Container);
}

void AbstractUserInterfaceTest::updateRecycledLayerWithoutInstance() {
    /* Verifies that the cached layer feature set is properly reset for freed
       and recycled layers as otherwise it'd cause all sorts of issues in
       internal state update */

    AbstractUserInterface ui{{100, 100}};

    struct LayerCanDoAnything: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return ~LayerFeatures{}; }
    };

    /* Add two layers with all possible features and remove them */
    LayerHandle removedLayer1 = ui.createLayer();
    LayerHandle removedLayer2 = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<LayerCanDoAnything>(removedLayer1));
    ui.setLayerInstance(Containers::pointer<LayerCanDoAnything>(removedLayer2));
    ui.removeLayer(removedLayer1);
    ui.removeLayer(removedLayer2);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate);

    /* A new layer should recycle the same memory location */
    LayerHandle recycledLayer = ui.createLayer();
    CORRADE_COMPARE(layerHandleId(recycledLayer), layerHandleId(removedLayer1));

    /* Calling update() should not rely on the cached feature set from the
       now-removed layer nor from the recycled-but-no-instance-set-yet layer */
    ui.update();
}

void AbstractUserInterfaceTest::state() {
    auto&& data = StateData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Event/framebuffer scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

    /*   2        3         4         5          6
       0                              +----------+
       1 +----------------------------| another1 |
       2 |       node                 +----------+
       3 |                  +---------+ another2 |
       4 |        +---------| nested2 |----------+
       5 |        | nested1 +---------+
       6 +--------+---------+---------+           */
    NodeHandle node, another1, another2, nested2;
    /* node, another1, another2 and nested2 are assigned a layout that moves by
       -2 on Y and scales by 2 on Y; invisible and another1 is assigned a
       layout that moves by 2 on X and scales by 2 on X, so the initial offsets
       are adjusted to make the end results match between the two */
    if(!data.layouters) {
        node = ui.createNode({2.0f, 1.0f}, {3.0f, 5.0f}, NodeFlag::Clip);
        another1 = ui.createNode({5.0f, 0.0f}, {1.0f, 2.0f});
        another2 = ui.createNode(another1, {0.0f, 2.0f}, {1.0f, 2.0f});
        ui.setNodeOrder(another2, NodeHandle::Null);
    } else {
        node = ui.createNode({2.0f, 3.0f}, {3.0f, 2.5f}, NodeFlag::Clip);
        another1 = ui.createNode({3.0f, 2.0f}, {0.5f, 1.0f});
        another2 = ui.createNode(another1, {0.0f, 4.0f}, {1.0f, 1.0f});
        ui.setNodeOrder(another2, NodeHandle::Null);
    }
    /* This node isn't assigned any layout */
    NodeHandle nested1 = ui.createNode(node, {1.0f, 3.0f}, {1.0f, 2.0f});
    if(!data.layouters) {
        nested2 = ui.createNode(node, {2.0f, 2.0f}, {1.0f, 2.0f});
    } else {
        nested2 = ui.createNode(node, {2.0f, 4.0f}, {1.0f, 1.0f});
    }
    /* This node is assigned a layout but isn't visible so it won't appear
       anywhere. It also shouldn't be modified by the layouter in any way. */
    NodeHandle invisible = ui.createNode({9.0f, 9.0f}, {9.0f, 9.0f}, NodeFlag::Hidden);
    /* This node is a child of the invisible top-level node, so it shouldn't
       appear anywhere even though it's top-level */
    NodeHandle topLevelChildOfInvisible = ui.createNode(invisible, {0.0f, 0.0f}, {9.0f, 9.0f});
    ui.setNodeOrder(topLevelChildOfInvisible, NodeHandle::Null);
    /* This node is not part of the top-level order so it won't appear anywhere
       either. It also shouldn't be modified by the layouter in any way. */
    NodeHandle notInOrder = ui.createNode({8.0f, 8.0f}, {8.0f, 8.0f});
    ui.clearNodeOrder(notInOrder);

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

    struct Layouter: AbstractLayouter {
        explicit Layouter(LayouterHandle handle, Containers::Array<UnsignedInt>& updateCalls): AbstractLayouter{handle}, updateCalls(updateCalls) {}

        using AbstractLayouter::add;
        using AbstractLayouter::remove;

        /* doSetSize() not implemented here as it isn't called from
           ui.update(), tested thoroughly in setSizeToLayouters() instead */

        void doClean(Containers::BitArrayView layoutIdsToRemove) override {
            CORRADE_COMPARE_AS(layoutIdsToRemove,
                expectedLayoutIdsToRemove.sliceBit(0),
                TestSuite::Compare::Container);
            ++cleanCallCount;
        }

        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&) override {
            arrayAppend(updateCalls, layouterHandleId(handle()));
        }

        Containers::StridedArrayView1D<const bool> expectedLayoutIdsToRemove;
        Int cleanCallCount = 0;
        Containers::Array<UnsignedInt>& updateCalls;
    };
    struct Layouter1: Layouter {
        using Layouter::Layouter;

        void doUpdate(Containers::BitArrayView layoutIdsToUpdate, const Containers::StridedArrayView1D<const UnsignedInt>& topLevelLayoutIds, const Containers::StridedArrayView1D<const NodeHandle>& nodeParents, const Containers::StridedArrayView1D<Vector2>& nodeOffsets, const Containers::StridedArrayView1D<Vector2>& nodeSizes) override {
            CORRADE_ITERATION("Layouter1");
            Layouter::doUpdate(layoutIdsToUpdate, topLevelLayoutIds, nodeParents, nodeOffsets, nodeSizes);
            CORRADE_COMPARE_AS(layoutIdsToUpdate,
                expectedLayoutIdsToUpdate.sliceBit(0),
                TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(topLevelLayoutIds,
                expectedTopLevelLayoutIds,
                TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(nodeParents,
                expectedNodeParents,
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
            for(std::size_t id = 0; id != layoutIdsToUpdate.size(); ++id) {
                /** @todo some way to iterate set bits */
                if(!layoutIdsToUpdate[id])
                    continue;
                nodeOffsets[nodeHandleId(nodes()[id])].x() += 2.0f;
                nodeSizes[nodeHandleId(nodes()[id])].x() *= 2.0f;
            }
        }

        Containers::StridedArrayView1D<const bool> expectedLayoutIdsToUpdate;
        Containers::StridedArrayView1D<const UnsignedInt> expectedTopLevelLayoutIds;
        Containers::StridedArrayView1D<const Containers::Pair<Vector2, Vector2>> expectedNodeOffsetsSizes;
        Containers::StridedArrayView1D<const NodeHandle> expectedNodeParents;
    };
    struct Layouter2: Layouter {
        using Layouter::Layouter;

        void doUpdate(Containers::BitArrayView layoutIdsToUpdate, const Containers::StridedArrayView1D<const UnsignedInt>& topLevelLayoutIds, const Containers::StridedArrayView1D<const NodeHandle>& nodeParents, const Containers::StridedArrayView1D<Vector2>& nodeOffsets, const Containers::StridedArrayView1D<Vector2>& nodeSizes) override {
            CORRADE_ITERATION("Layouter2 call" << updateCallId);
            Layouter::doUpdate(layoutIdsToUpdate, topLevelLayoutIds, nodeParents, nodeOffsets, nodeSizes);
            CORRADE_COMPARE_AS(updateCallId,
                2,
                TestSuite::Compare::Less);
            CORRADE_COMPARE_AS(layoutIdsToUpdate,
                expectedLayoutIdsToUpdate[updateCallId].sliceBit(0),
                TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(topLevelLayoutIds,
                expectedTopLevelLayoutIds[updateCallId],
                TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(nodeParents,
                expectedNodeParents,
                TestSuite::Compare::Container);
            CORRADE_COMPARE(nodeOffsets.size(), expectedNodeOffsetsSizes[updateCallId].size());
            for(std::size_t i = 0; i != nodeOffsets.size(); ++i) {
                CORRADE_ITERATION(i);
                /* For nodes that aren't in the visible hierarchy or are
                   removed the value can be just anything, skip */
                if(expectedNodeOffsetsSizes[updateCallId][i].second().isZero())
                    continue;
                CORRADE_COMPARE(Containers::pair(nodeOffsets[i], nodeSizes[i]), expectedNodeOffsetsSizes[updateCallId][i]);
            }
            for(std::size_t id = 0; id != layoutIdsToUpdate.size(); ++id) {
                /** @todo some way to iterate set bits */
                if(!layoutIdsToUpdate[id])
                    continue;
                nodeOffsets[nodeHandleId(nodes()[id])].y() += -2.0f;
                nodeSizes[nodeHandleId(nodes()[id])].y() *= 2.0f;
            }
            ++updateCallId;
        }

        Containers::StridedArrayView1D<const bool> expectedLayoutIdsToUpdate[2];
        Containers::StridedArrayView1D<const UnsignedInt> expectedTopLevelLayoutIds[2];
        Containers::StridedArrayView1D<const Containers::Pair<Vector2, Vector2>> expectedNodeOffsetsSizes[2];
        Containers::StridedArrayView1D<const NodeHandle> expectedNodeParents;
        Int updateCallId = 0;
    };

    Containers::Array<UnsignedInt> layouterUpdateCalls;

    /* Creating layouters sets no state flags */
    LayouterHandle layouter1{}, layouter2{};
    /* These are set to all 1s to not trigger a null assertion when
       unconditionally querying layoutHandleId() on them when layouters are not
       used */
    LayoutHandle layout2Node = LayoutHandle(~UnsignedLong{});
    LayoutHandle layout2Another1 = LayoutHandle(~UnsignedLong{});
    LayoutHandle layout1Another1 = LayoutHandle(~UnsignedLong{});
    NodeHandle expectedNodeParents[]{
        NodeHandle::Null,
        NodeHandle::Null,
        another1,
        node,
        node,
        NodeHandle::Null,
        invisible,
        NodeHandle::Null,
    };
    if(data.layouters) {
        /* Layouter 2 is ordered first even though it has a higher ID */
        layouter2 = ui.createLayouter();
        layouter1 = ui.createLayouter(layouter2);
        ui.setLayouterInstance(Containers::pointer<Layouter2>(layouter2, layouterUpdateCalls));
        ui.setLayouterInstance(Containers::pointer<Layouter1>(layouter1, layouterUpdateCalls));
        /* The node parents change only after node removal so they're set just
           once and then the original array gets updated */
        ui.layouter<Layouter1>(layouter1).expectedNodeParents = expectedNodeParents;
        ui.layouter<Layouter2>(layouter2).expectedNodeParents = expectedNodeParents;
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

        /* Calling clean() should be a no-op, not calling anything in the
           layouters */
        if(data.clean && data.noOp) {
            {
                CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
                ui.clean();
            }
            CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 0);
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 0);
            CORRADE_COMPARE_AS(layouterUpdateCalls,
                Containers::arrayView<UnsignedInt>({}),
                TestSuite::Compare::Container);
        }

        /* Calling update() should be a no-op, not calling anything in the
           layouters */
        if(data.noOp) {
            {
                CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
                ui.update();
            }
            CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 0);
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 0);
            CORRADE_COMPARE_AS(layouterUpdateCalls,
                Containers::arrayView<UnsignedInt>({}),
                TestSuite::Compare::Container);
        }

        /* Creating a layout in a layouter sets state flags */
        /* This one gets ignored, as there's the same node added again after */
        /*layout2Nested2Duplicate =*/ ui.layouter<Layouter2>(layouter2).add(nested2);
        layout2Node = ui.layouter<Layouter2>(layouter2).add(node);
        /*layout2Nested2 =*/ ui.layouter<Layouter2>(layouter2).add(nested2);
        layout2Another1 = ui.layouter<Layouter2>(layouter2).add(another1);
        /* This one is unused because the node is invisible */
        /*layout1Invisible =*/ ui.layouter<Layouter1>(layouter1).add(invisible);
        /* This one is not referenced by the test as another2 is a non-root
           top-level node so it never appears among top-level layout nodes */
        /*layout2Another2 =*/ ui.layouter<Layouter2>(layouter2).add(another2);
        layout1Another1 = ui.layouter<Layouter1>(layouter1).add(another1);
        /* This one is again unused because the node is not in the top-level
           order */
        /*layout1NotInOrder =*/ ui.layouter<Layouter1>(layouter1).add(notInOrder);
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsLayoutAssignmentUpdate);

        /* Calling clean() should be a no-op */
        if(data.clean && data.noOp) {
            {
                CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
                ui.clean();
            }
            CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsLayoutAssignmentUpdate);
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 0);
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 0);
            CORRADE_COMPARE_AS(layouterUpdateCalls,
                Containers::arrayView<UnsignedInt>({}),
                TestSuite::Compare::Container);
        }

        /* Calling update() rebuilds internal state, calls doUpdate() on the
           layouters, and resets the flag. */
        {
            CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
            bool expectedLayoutIdsToUpdate1[]{
                /* layout2Node and layout2Nested2 */
                false, true, true, false, false
            };
            bool expectedLayoutIdsToUpdate2[]{
                /* layout1Another1 */
                false, true, false
            };
            bool expectedLayoutIdsToUpdate3[]{
                /* layout2Another1 and layout2Another2 */
                false, false, false, true, true
            };
            UnsignedInt expectedTopLevelLayoutIds1[]{
                layoutHandleId(layout2Node),
            };
            UnsignedInt expectedTopLevelLayoutIds2[]{
                layoutHandleId(layout1Another1),
            };
            UnsignedInt expectedTopLevelLayoutIds3[]{
                layoutHandleId(layout2Another1),
                /* layout2Another2 is top-level but not root, so not included
                   here */
            };
            Containers::Pair<Vector2, Vector2> expectedLayoutNodeOffsetsSizes1[]{
                /* This gets the original sizes and *relative* offsets,
                   subsequent layouters then modified values and the layers
                   then what the layouter produces and absolute */
                {{2.0f, 3.0f}, {3.0f, 2.5f}}, /* node */
                {{3.0f, 2.0f}, {0.5f, 1.0f}}, /* another1 */
                {{0.0f, 4.0f}, {1.0f, 1.0f}}, /* another2 */
                {{1.0f, 3.0f}, {1.0f, 2.0f}}, /* nested1 */
                {{2.0f, 4.0f}, {1.0f, 1.0f}}, /* nested2 */
                {{9.0f, 9.0f}, {9.0f, 9.0f}}, /* invisible */
                {{0.0f, 0.0f}, {9.0f, 9.0f}}, /* topLevelChildOfInvisible */
                {{8.0f, 8.0f}, {8.0f, 8.0f}}, /* notInOrder */
            };
            Containers::Pair<Vector2, Vector2> expectedLayoutNodeOffsetsSizes2[]{
                {{2.0f, 1.0f}, {3.0f, 5.0f}}, /* changed by layouter 2 */
                {{3.0f, 2.0f}, {0.5f, 1.0f}},
                {{0.0f, 4.0f}, {1.0f, 1.0f}},
                {{1.0f, 3.0f}, {1.0f, 2.0f}},
                {{2.0f, 2.0f}, {1.0f, 2.0f}},
                {{9.0f, 9.0f}, {9.0f, 9.0f}},
                {{0.0f, 0.0f}, {9.0f, 9.0f}},
                {{8.0f, 8.0f}, {8.0f, 8.0f}}
            };
            Containers::Pair<Vector2, Vector2> expectedLayoutNodeOffsetsSizes3[]{
                {{2.0f, 1.0f}, {3.0f, 5.0f}},
                {{5.0f, 2.0f}, {1.0f, 1.0f}}, /* changed by layouter 1 */
                {{0.0f, 4.0f}, {1.0f, 1.0f}},
                {{1.0f, 3.0f}, {1.0f, 2.0f}},
                {{2.0f, 2.0f}, {1.0f, 2.0f}},
                {{9.0f, 9.0f}, {9.0f, 9.0f}},
                {{0.0f, 0.0f}, {9.0f, 9.0f}},
                {{8.0f, 8.0f}, {8.0f, 8.0f}}
            };
            ui.layouter<Layouter2>(layouter2).expectedLayoutIdsToUpdate[0] = expectedLayoutIdsToUpdate1;
            ui.layouter<Layouter1>(layouter1).expectedLayoutIdsToUpdate = expectedLayoutIdsToUpdate2;
            ui.layouter<Layouter2>(layouter2).expectedLayoutIdsToUpdate[1] = expectedLayoutIdsToUpdate3;
            ui.layouter<Layouter2>(layouter2).expectedTopLevelLayoutIds[0] = expectedTopLevelLayoutIds1;
            ui.layouter<Layouter1>(layouter1).expectedTopLevelLayoutIds = expectedTopLevelLayoutIds2;
            ui.layouter<Layouter2>(layouter2).expectedTopLevelLayoutIds[1] = expectedTopLevelLayoutIds3;
            ui.layouter<Layouter2>(layouter2).expectedNodeOffsetsSizes[0] = expectedLayoutNodeOffsetsSizes1;
            ui.layouter<Layouter1>(layouter1).expectedNodeOffsetsSizes = expectedLayoutNodeOffsetsSizes2;
            ui.layouter<Layouter2>(layouter2).expectedNodeOffsetsSizes[1] = expectedLayoutNodeOffsetsSizes3;
            ui.layouter<Layouter2>(layouter2).updateCallId = 0;
            ui.update();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
        CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 0);
        CORRADE_COMPARE_AS(layouterUpdateCalls, Containers::arrayView({
            /* update() gets called twice on layouter 2 */
            layouterHandleId(layouter2), layouterHandleId(layouter1), layouterHandleId(layouter2)
        }), TestSuite::Compare::Container);
    }

    struct Layer: AbstractLayer {
        explicit Layer(LayerHandle handle, LayerFeatures features): AbstractLayer{handle}, _features{features} {}

        using AbstractLayer::create;
        using AbstractLayer::remove;

        LayerFeatures doFeatures() const override { return _features; }

        /* doSetSize() not implemented here as it isn't called from
           ui.update(), tested thoroughly in setSizeToLayers() instead */

        void doClean(Containers::BitArrayView dataIdsToRemove) override {
            CORRADE_COMPARE_AS(dataIdsToRemove,
                expectedDataIdsToRemove.sliceBit(0),
                TestSuite::Compare::Container);
            ++cleanCallCount;
        }

        void doUpdate(const LayerStates state, const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes, const Containers::StridedArrayView1D<const Vector2>& compositeRectOffsets, const Containers::StridedArrayView1D<const Vector2>& compositeRectSizes) override {
            /* The doUpdate() should never get the NeedsAttachmentUpdate, only
               the NeedsNodeOrderUpdate that's a subset of it */
            CORRADE_VERIFY(!(state >= LayerState::NeedsAttachmentUpdate));
            CORRADE_COMPARE(state, expectedState);
            CORRADE_COMPARE_AS(dataIds,
                expectedDataIds,
                TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(clipRectIds,
                expectedClipRectIdsDataCounts.slice(&Containers::Pair<UnsignedInt, UnsignedInt>::first),
                TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(clipRectDataCounts,
                expectedClipRectIdsDataCounts.slice(&Containers::Pair<UnsignedInt, UnsignedInt>::second),
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
            CORRADE_COMPARE_AS(nodesEnabled,
                expectedNodesEnabled.sliceBit(0),
                TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(clipRectOffsets,
                expectedClipRectOffsetsSizes.slice(&Containers::Pair<Vector2, Vector2>::first),
                TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(clipRectSizes,
                expectedClipRectOffsetsSizes.slice(&Containers::Pair<Vector2, Vector2>::second),
                TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(compositeRectOffsets,
                expectedCompositeRectOffsetsSizes.slice(&Containers::Pair<Vector2, Vector2>::first),
                TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(compositeRectSizes,
                expectedCompositeRectOffsetsSizes.slice(&Containers::Pair<Vector2, Vector2>::second),
                TestSuite::Compare::Container);
            ++updateCallCount;
        }

        LayerStates expectedState;
        Containers::StridedArrayView1D<const bool> expectedDataIdsToRemove;
        Containers::StridedArrayView1D<const UnsignedInt> expectedDataIds;
        Containers::StridedArrayView1D<const Containers::Pair<UnsignedInt, UnsignedInt>> expectedClipRectIdsDataCounts;
        Containers::StridedArrayView1D<const Containers::Pair<Vector2, Vector2>> expectedNodeOffsetsSizes;
        Containers::StridedArrayView1D<const bool> expectedNodesEnabled;
        Containers::StridedArrayView1D<const Containers::Pair<Vector2, Vector2>> expectedClipRectOffsetsSizes;
        Containers::StridedArrayView1D<const Containers::Pair<Vector2, Vector2>> expectedCompositeRectOffsetsSizes;
        Int cleanCallCount = 0;
        Int updateCallCount = 0;

        private:
            LayerFeatures _features;
    };

    /* Creating a layer sets no state flags */
    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<Layer>(layer, data.compositingLayer ? LayerFeature::Composite : LayerFeatures{}));
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

    /* Calling clean() should be a no-op, not calling anything in the layouters
       or layers */
    if(data.clean && data.noOp) {
        {
            CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
        if(data.layouters) {
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 0);
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 0);
        }
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 0);
    }

    /* Calling update() should be a no-op, not calling anything in the
       layouters or layers */
    if(data.noOp) {
        {
            CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
            ui.update();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
        if(data.layouters) {
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 0);
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 0);
            CORRADE_COMPARE_AS(layouterUpdateCalls, Containers::arrayView({
                layouterHandleId(layouter2), layouterHandleId(layouter1), layouterHandleId(layouter2)
            }), TestSuite::Compare::Container);
        }
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 0);
    }

    /* Creating a data in a layer sets state flags to populate their contents */
    DataHandle dataNested2 = ui.layer<Layer>(layer).create();
    DataHandle dataNode = ui.layer<Layer>(layer).create();
    DataHandle dataAnother1 = ui.layer<Layer>(layer).create();
    DataHandle dataNotAttached = ui.layer<Layer>(layer).create();
    DataHandle dataNested1 = ui.layer<Layer>(layer).create();
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataUpdate);
    CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 0);

    /* Calling clean() should be a no-op */
    if(data.clean && data.noOp) {
        {
            CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataUpdate);
        if(data.layouters) {
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 0);
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 0);
        }
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 0);
    }

    /* Calling update() rebuilds internal state, calls doUpdate() on the layer,
       and resets the flag. Since nothing is attached, there are no IDs to
       draw and no rects to composite. It doesn't call the layouters either. */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
        Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
            {{2.0f, 1.0f}, {3.0f, 5.0f}}, /* node */
            {{5.0f, 0.0f}, {1.0f, 2.0f}}, /* another1 */
            {{5.0f, 2.0f}, {1.0f, 2.0f}}, /* another2 */
            {{3.0f, 4.0f}, {1.0f, 2.0f}}, /* nested1 */
            {{4.0f, 3.0f}, {1.0f, 2.0f}}, /* nested2 */
            {},                           /* invisible */
            {},                           /* topLevelChildOfInvisible */
            {},                           /* notInOrder */
        };
        bool expectedNodesEnabled[]{
            /* All enabled except invisible, topLevelChildOfInvisible and
               notInOrder */
            true, true, true, true, true, false, false, false
        };
        Containers::Pair<Vector2, Vector2> expectedClipRectOffsetsSizes[]{
            {{2.0f, 1.0f}, {3.0f, 5.0f}},
            {{}, {}},
            {{}, {}}
        };
        /* Nothing is attached to any nodes, so it's just the data alone being
           updated */
        ui.layer<Layer>(layer).expectedState = LayerState::NeedsDataUpdate;
        ui.layer<Layer>(layer).expectedDataIds = {};
        ui.layer<Layer>(layer).expectedNodeOffsetsSizes = expectedNodeOffsetsSizes;
        ui.layer<Layer>(layer).expectedNodesEnabled = expectedNodesEnabled;
        ui.layer<Layer>(layer).expectedClipRectIdsDataCounts = {};
        ui.layer<Layer>(layer).expectedClipRectOffsetsSizes = expectedClipRectOffsetsSizes;
        if(data.compositingLayer)
            ui.layer<Layer>(layer).expectedCompositeRectOffsetsSizes = {};
        ui.update();
    }
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    if(data.layouters) {
        CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 0);
        CORRADE_COMPARE_AS(layouterUpdateCalls, Containers::arrayView({
            layouterHandleId(layouter2), layouterHandleId(layouter1), layouterHandleId(layouter2)
        }), TestSuite::Compare::Container);
    }
    CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
    CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 1);

    struct Animator: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;
        using AbstractGenericAnimator::create;

        AnimatorFeatures doFeatures() const override { return {}; }
        void doClean(Containers::BitArrayView) override {
            CORRADE_FAIL("This shouldn't be called");
        }
        void doAdvance(Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {
            CORRADE_FAIL("This shouldn't be called");
        }
    };
    struct AttachmentAnimator: AbstractGenericAnimator {
        explicit AttachmentAnimator(AnimatorHandle handle, AnimatorFeatures features): AbstractGenericAnimator{handle}, _features{features} {}

        using AbstractGenericAnimator::AbstractGenericAnimator;
        using AbstractGenericAnimator::setLayer;
        using AbstractGenericAnimator::create;

        AnimatorFeatures doFeatures() const override { return _features; }
        void doClean(Containers::BitArrayView animationIdsToRemove) override {
            CORRADE_COMPARE_AS(animationIdsToRemove,
                expectedAnimationIdsToRemove.sliceBit(0),
                TestSuite::Compare::Container);
            ++cleanCallCount;
        }
        void doAdvance(Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {
            CORRADE_FAIL("This shouldn't be called");
        }

        Containers::StridedArrayView1D<const bool> expectedAnimationIdsToRemove;
        Int cleanCallCount = 0;

        private:
            AnimatorFeatures _features;
    };

    /* Creating animators sets no state flags */
    AnimatorHandle animator{}, nodeAttachmentAnimator{}, dataAttachmentAnimator{};
    if(data.nodeAttachmentAnimators || data.dataAttachmentAnimators) {
        animator = ui.createAnimator();
        ui.setGenericAnimatorInstance(Containers::pointer<Animator>(animator));
        if(data.nodeAttachmentAnimators) {
            nodeAttachmentAnimator = ui.createAnimator();
            ui.setGenericAnimatorInstance(Containers::pointer<AttachmentAnimator>(nodeAttachmentAnimator, AnimatorFeature::NodeAttachment));
        }
        if(data.dataAttachmentAnimators) {
            dataAttachmentAnimator = ui.createAnimator();
            Containers::Pointer<AttachmentAnimator> instance{InPlaceInit, dataAttachmentAnimator, AnimatorFeature::DataAttachment};
            instance->setLayer(ui.layer(layer));
            ui.setGenericAnimatorInstance(Utility::move(instance));
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

        /* Creating (stopped) animations sets no state flags, independently of
           whether they're attacheable at all, attached to nodes / data or not
           attached */
        /*AnimationHandle animation =*/ ui.animator<Animator>(animator).create(-10_nsec, 1_nsec, AnimationFlag::KeepOncePlayed);
        /* One is attached to a node, one to nothing, one to a nested node.
           Both first and third get removed when `node` is removed. */
        if(data.nodeAttachmentAnimators) {
            /*AnimationHandle animation1 =*/ ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).create(-10_nsec, 1_nsec, node, AnimationFlag::KeepOncePlayed);
            /*AnimationHandle animation2 =*/ ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).create(-10_nsec, 1_nsec, NodeHandle::Null, AnimationFlag::KeepOncePlayed);
            /*AnimationHandle animation3 =*/ ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).create(-10_nsec, 1_nsec, nested1, AnimationFlag::KeepOncePlayed);
        }
        if(data.dataAttachmentAnimators) {
            /* One is attached to a data not attached to any node, one to
               nothing, one to a nested node. The first gets removed when
               removing the data directly, the third when removing `node`. */
            /*AnimationHandle animation1 =*/ ui.animator<AttachmentAnimator>(dataAttachmentAnimator).create(-10_nsec, 1_nsec, dataNotAttached, AnimationFlag::KeepOncePlayed);
            ui.animator<AttachmentAnimator>(dataAttachmentAnimator).create(-10_nsec, 1_nsec, DataHandle::Null, AnimationFlag::KeepOncePlayed);
            ui.animator<AttachmentAnimator>(dataAttachmentAnimator).create(-10_nsec, 1_nsec, dataNested2, AnimationFlag::KeepOncePlayed);
        }
    }

    /* Calling clean() should be a no-op, not calling anything in the
       layouters, layers or animators */
    if(data.clean && data.noOp) {
        {
            CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
        if(data.layouters) {
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 0);
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 0);
        }
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 1);
        if(data.nodeAttachmentAnimators)
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).cleanCallCount, 0);
        if(data.dataAttachmentAnimators)
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(dataAttachmentAnimator).cleanCallCount, 0);
    }

    /* Calling update() should be a no-op, not calling anything in the
       layouters, layers or animators */
    if(data.noOp) {
        {
            CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
            ui.update();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
        if(data.layouters) {
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 0);
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 0);
            CORRADE_COMPARE_AS(layouterUpdateCalls, Containers::arrayView({
                layouterHandleId(layouter2), layouterHandleId(layouter1), layouterHandleId(layouter2)
            }), TestSuite::Compare::Container);
        }
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 1);
        if(data.nodeAttachmentAnimators)
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).cleanCallCount, 0);
        if(data.dataAttachmentAnimators)
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(dataAttachmentAnimator).cleanCallCount, 0);
    }

    /* Attaching the data sets flags. Order doesn't matter, as internally it's
       always ordered by the data ID. */
    ui.attachData(node, dataNode);
    ui.attachData(nested1, dataNested1);
    ui.attachData(nested2, dataNested2);
    ui.attachData(another1, dataAnother1);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate);

    /* Calling clean() should be a no-op */
    if(data.clean && data.noOp) {
        {
            CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate);
        if(data.layouters) {
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 0);
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 0);
        }
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 1);
        if(data.nodeAttachmentAnimators)
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).cleanCallCount, 0);
        if(data.dataAttachmentAnimators)
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(dataAttachmentAnimator).cleanCallCount, 0);
    }

    /* Calling update() rebuilds internal state, calls doUpdate() on the layer,
       and resets the flag. It doesn't call the layouters. */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
        UnsignedInt expectedDataIds[]{
            dataHandleId(dataNode),
            dataHandleId(dataNested1),
            dataHandleId(dataNested2),
            dataHandleId(dataAnother1),
        };
        Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
            {{2.0f, 1.0f}, {3.0f, 5.0f}}, /* node */
            {{5.0f, 0.0f}, {1.0f, 2.0f}}, /* another1 */
            {{5.0f, 2.0f}, {1.0f, 2.0f}}, /* another2 */
            {{3.0f, 4.0f}, {1.0f, 2.0f}}, /* nested1 */
            {{4.0f, 3.0f}, {1.0f, 2.0f}}, /* nested2 */
            {},                           /* invisible */
            {},                           /* topLevelChildOfInvisible */
            {},                           /* notInOrder */
        };
        bool expectedNodesEnabled[]{
            /* All enabled except invisible, topLevelChildOfInvisible and
               notInOrder */
            true, true, true, true, true, false, false, false
        };
        Containers::Pair<UnsignedInt, UnsignedInt> expectedClipRectIdsDataCounts[]{
            {0, 3}, /* node and all children */
            {1, 1}  /* another1, unclipped */
            /* another2 has no data */
        };
        Containers::Pair<Vector2, Vector2> expectedClipRectOffsetsSizes[]{
            {{2.0f, 1.0f}, {3.0f, 5.0f}},
            {{}, {}},
            {{}, {}}
        };
        Containers::Pair<Vector2, Vector2> expectedCompositeRectOffsetsSizes[]{
            {{2.0f, 1.0f}, {3.0f, 5.0f}}, /* matching node */
            {{3.0f, 4.0f}, {1.0f, 2.0f}}, /* matching nested1 */
            {{4.0f, 3.0f}, {1.0f, 2.0f}}, /* matching nested2 */
            {{5.0f, 0.0f}, {1.0f, 2.0f}}, /* matching another1 */
        };
        /* Data were updated in the previous call, so it's now just the
           node-related state */
        ui.layer<Layer>(layer).expectedState = LayerState::NeedsNodeOffsetSizeUpdate|LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsNodeOrderUpdate|(data.compositingLayer ? LayerState::NeedsCompositeOffsetSizeUpdate : LayerStates{});
        ui.layer<Layer>(layer).expectedDataIds = expectedDataIds;
        ui.layer<Layer>(layer).expectedNodeOffsetsSizes = expectedNodeOffsetsSizes;
        ui.layer<Layer>(layer).expectedNodesEnabled = expectedNodesEnabled;
        ui.layer<Layer>(layer).expectedClipRectIdsDataCounts = expectedClipRectIdsDataCounts;
        ui.layer<Layer>(layer).expectedClipRectOffsetsSizes = expectedClipRectOffsetsSizes;
        if(data.compositingLayer)
            ui.layer<Layer>(layer).expectedCompositeRectOffsetsSizes = expectedCompositeRectOffsetsSizes;
        ui.update();
    }
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    if(data.layouters) {
        CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 0);
        CORRADE_COMPARE_AS(layouterUpdateCalls, Containers::arrayView({
            layouterHandleId(layouter2), layouterHandleId(layouter1), layouterHandleId(layouter2)
        }), TestSuite::Compare::Container);
    }
    CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
    CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 2);
    if(data.nodeAttachmentAnimators)
        CORRADE_COMPARE(ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).cleanCallCount, 0);
    if(data.dataAttachmentAnimators)
        CORRADE_COMPARE(ui.animator<AttachmentAnimator>(dataAttachmentAnimator).cleanCallCount, 0);

    /* Setting a renderer instance propagates the size to it without setting
       any state flag */
    struct Renderer: AbstractRenderer {
        explicit Renderer(RendererFeatures features): _features{features} {}

        RendererFeatures doFeatures() const override { return _features; }
        void doSetupFramebuffers(const Vector2i& size) override {
            if(!setupFramebufferCallCount)
                CORRADE_COMPARE(size, Vector2i{100});
            else
                CORRADE_COMPARE(size, (Vector2i{17, 35}));
            ++setupFramebufferCallCount;
        }
        void doTransition(RendererTargetState, RendererTargetState, RendererDrawStates, RendererDrawStates) override {}

        Int setupFramebufferCallCount = 0;

        private:
            RendererFeatures _features;
    };
    ui.setRendererInstance(Containers::pointer<Renderer>(data.compositingLayer ? RendererFeature::Composite : RendererFeatures{}));
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.renderer<Renderer>().setupFramebufferCallCount, 1);
    CORRADE_COMPARE(ui.renderer().framebufferSize(), Vector2i{100});

    /* Calling setSize() with the same size is a no-op */
    ui.setSize({100, 100});
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.renderer<Renderer>().setupFramebufferCallCount, 1);
    CORRADE_COMPARE(ui.renderer().framebufferSize(), Vector2i{100});

    /* Calling setSize() with all values different setups the renderer and sets
       state flags to update the clip state. Other interactions between setRendererInstance(), setSize() and layer / layouter instances are
       tested thoroughly in renderer(), setSizeToRenderer(), setSizeToLayers()
       and setSizeToLayouters() instead. */
    ui.setSize({4.0f, 5.0f}, {376.0f, 234.0f}, {17, 35});
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeClipUpdate);
    CORRADE_COMPARE(ui.renderer<Renderer>().setupFramebufferCallCount, 2);
    CORRADE_COMPARE(ui.renderer().framebufferSize(), (Vector2i{17, 35}));

    /* Calling clean() should be a no-op */
    if(data.clean && data.noOp) {
        {
            CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeClipUpdate);
        if(data.layouters) {
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 0);
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 0);
        }
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 2);
        if(data.nodeAttachmentAnimators)
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).cleanCallCount, 0);
        if(data.dataAttachmentAnimators)
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(dataAttachmentAnimator).cleanCallCount, 0);
    }

    /* Calling update() should refresh the cull state in all layers. It doesn't
       call anything in the layouters. */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
        UnsignedInt expectedDataIds[]{
            dataHandleId(dataNode),
            dataHandleId(dataNested1)
        };
        Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
            {{2.0f, 1.0f}, {3.0f, 5.0f}}, /* node */
            {},                           /* another1 */
            {},                           /* another2 */
            {{3.0f, 4.0f}, {1.0f, 2.0f}}, /* nested1 */
            {},                           /* nested2 */
            {},                           /* invisible */
            {},                           /* topLevelChildOfInvisible */
            {},                           /* notInOrder */
        };
        bool expectedNodesEnabled[]{
            /* Only node and nested1 is visible (and enabled) now */
            true, false, false, true, false, false, false, false
        };
        Containers::Pair<UnsignedInt, UnsignedInt> expectedClipRectIdsDataCounts[]{
            {0, 2}, /* node and nested1 */
            /* the rest is invisible */
        };
        Containers::Pair<Vector2, Vector2> expectedClipRectOffsetsSizes[]{
            {{2.0f, 1.0f}, {2.0f, 4.0f}}, /* This is clipped to the UI size */
            {{}, {}},
            {{}, {}}
        };
        Containers::Pair<Vector2, Vector2> expectedCompositeRectOffsetsSizes[]{
            {{2.0f, 1.0f}, {2.0f, 4.0f}}, /* matching node, clipped */
            {{3.0f, 4.0f}, {1.0f, 1.0f}}, /* matching nested1, clipped */
        };
        /* Updating the UI size caused some nodes to be invisible now, meaning
           an update of node order is triggered. It's however also node enabled
           update because some of the differently visible nodes can now be
           differently enabled. */
        ui.layer<Layer>(layer).expectedState = LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsNodeOrderUpdate;
        ui.layer<Layer>(layer).expectedDataIds = expectedDataIds;
        ui.layer<Layer>(layer).expectedNodeOffsetsSizes = expectedNodeOffsetsSizes;
        ui.layer<Layer>(layer).expectedNodesEnabled = expectedNodesEnabled;
        ui.layer<Layer>(layer).expectedClipRectIdsDataCounts = expectedClipRectIdsDataCounts;
        ui.layer<Layer>(layer).expectedClipRectOffsetsSizes = expectedClipRectOffsetsSizes;
        if(data.compositingLayer)
            ui.layer<Layer>(layer).expectedCompositeRectOffsetsSizes = expectedCompositeRectOffsetsSizes;
        ui.update();
    }
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.renderer<Renderer>().setupFramebufferCallCount, 2);
    CORRADE_COMPARE(ui.renderer().framebufferSize(), (Vector2i{17, 35}));
    if(data.layouters) {
        CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 0);
        CORRADE_COMPARE_AS(layouterUpdateCalls, Containers::arrayView({
            layouterHandleId(layouter2), layouterHandleId(layouter1), layouterHandleId(layouter2)
        }), TestSuite::Compare::Container);
    }
    CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
    CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 3);
    if(data.nodeAttachmentAnimators)
        CORRADE_COMPARE(ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).cleanCallCount, 0);
    if(data.dataAttachmentAnimators)
        CORRADE_COMPARE(ui.animator<AttachmentAnimator>(dataAttachmentAnimator).cleanCallCount, 0);

    /* Calling setSize() with just the UI size being different but window and
       framebuffer size being the same causes just the cull state update to be
       set */
    ui.setSize({165.0f, 156.0f}, {376.0f, 234.0f}, {17, 35});
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeClipUpdate);
    CORRADE_COMPARE(ui.renderer<Renderer>().setupFramebufferCallCount, 2);
    CORRADE_COMPARE(ui.renderer().framebufferSize(), (Vector2i{17, 35}));

    /* Calling clean() should be a no-op */
    if(data.clean && data.noOp) {
        {
            CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeClipUpdate);
        if(data.layouters) {
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 0);
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 0);
        }
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 3);
        if(data.nodeAttachmentAnimators)
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).cleanCallCount, 0);
        if(data.dataAttachmentAnimators)
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(dataAttachmentAnimator).cleanCallCount, 0);
    }

    /* Calling update() resets back to the "everything visible" state like
       before. It doesn't call the layouters. */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
        UnsignedInt expectedDataIds[]{
            dataHandleId(dataNode),
            dataHandleId(dataNested1),
            dataHandleId(dataNested2),
            dataHandleId(dataAnother1),
        };
        Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
            {{2.0f, 1.0f}, {3.0f, 5.0f}}, /* node */
            {{5.0f, 0.0f}, {1.0f, 2.0f}}, /* another1 */
            {{5.0f, 2.0f}, {1.0f, 2.0f}}, /* another2 */
            {{3.0f, 4.0f}, {1.0f, 2.0f}}, /* nested1 */
            {{4.0f, 3.0f}, {1.0f, 2.0f}}, /* nested2 */
            {},                           /* invisible */
            {},                           /* topLevelChildOfInvisible */
            {},                           /* notInOrder */
        };
        bool expectedNodesEnabled[]{
            /* All enabled except invisible, topLevelChildOfInvisible and
               notInOrder */
            true, true, true, true, true, false, false, false
        };
        Containers::Pair<UnsignedInt, UnsignedInt> expectedClipRectIdsDataCounts[]{
            {0, 3}, /* node and all children */
            {1, 1}  /* another1, unclipped */
            /* another2 has no data */
        };
        Containers::Pair<Vector2, Vector2> expectedClipRectOffsetsSizes[]{
            {{2.0f, 1.0f}, {3.0f, 5.0f}},
            {{}, {}},
            {{}, {}}
        };
        Containers::Pair<Vector2, Vector2> expectedCompositeRectOffsetsSizes[]{
            {{2.0f, 1.0f}, {3.0f, 5.0f}}, /* matching node */
            {{3.0f, 4.0f}, {1.0f, 2.0f}}, /* matching nested1 */
            {{4.0f, 3.0f}, {1.0f, 2.0f}}, /* matching nested2 */
            {{5.0f, 0.0f}, {1.0f, 2.0f}}, /* matching another1 */
        };
        /* Updating the UI size again caused some previously-invisible nodes to
           be visible again, meaning an update of node order and enabled state
           is triggered again */
        ui.layer<Layer>(layer).expectedState = LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsNodeOrderUpdate;
        ui.layer<Layer>(layer).expectedDataIds = expectedDataIds;
        ui.layer<Layer>(layer).expectedNodeOffsetsSizes = expectedNodeOffsetsSizes;
        ui.layer<Layer>(layer).expectedNodesEnabled = expectedNodesEnabled;
        ui.layer<Layer>(layer).expectedClipRectIdsDataCounts = expectedClipRectIdsDataCounts;
        ui.layer<Layer>(layer).expectedClipRectOffsetsSizes = expectedClipRectOffsetsSizes;
        if(data.compositingLayer)
            ui.layer<Layer>(layer).expectedCompositeRectOffsetsSizes = expectedCompositeRectOffsetsSizes;
        ui.update();
    }
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.renderer<Renderer>().setupFramebufferCallCount, 2);
    CORRADE_COMPARE(ui.renderer().framebufferSize(), (Vector2i{17, 35}));
    if(data.layouters) {
        CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 0);
        CORRADE_COMPARE_AS(layouterUpdateCalls, Containers::arrayView({
            layouterHandleId(layouter2), layouterHandleId(layouter1), layouterHandleId(layouter2)
        }), TestSuite::Compare::Container);
    }
    CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
    CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 4);
    if(data.nodeAttachmentAnimators)
        CORRADE_COMPARE(ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).cleanCallCount, 0);
    if(data.dataAttachmentAnimators)
        CORRADE_COMPARE(ui.animator<AttachmentAnimator>(dataAttachmentAnimator).cleanCallCount, 0);

    /* Marking the layer with Needs*DataUpdate propagates to the UI-wide
       state */
    ui.layer(layer).setNeedsUpdate(LayerState::NeedsCommonDataUpdate);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataUpdate);

    /* Calling clean() should be a no-op */
    if(data.clean && data.noOp) {
        {
            CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataUpdate);
        CORRADE_COMPARE(ui.renderer<Renderer>().setupFramebufferCallCount, 2);
        if(data.layouters) {
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 0);
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 0);
        }
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 4);
        if(data.nodeAttachmentAnimators)
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).cleanCallCount, 0);
        if(data.dataAttachmentAnimators)
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(dataAttachmentAnimator).cleanCallCount, 0);
    }

    /* Calling update() reuploads the exact same data and resets the flag, but
       internally shouldn't do any other state rebuild. Nothing observable to
       verify that with, tho. It doesn't call layouters. */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
        UnsignedInt expectedDataIds[]{
            dataHandleId(dataNode),
            dataHandleId(dataNested1),
            dataHandleId(dataNested2),
            dataHandleId(dataAnother1),
        };
        Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
            {{2.0f, 1.0f}, {3.0f, 5.0f}}, /* node */
            {{5.0f, 0.0f}, {1.0f, 2.0f}}, /* another1 */
            {{5.0f, 2.0f}, {1.0f, 2.0f}}, /* another2 */
            {{3.0f, 4.0f}, {1.0f, 2.0f}}, /* nested1 */
            {{4.0f, 3.0f}, {1.0f, 2.0f}}, /* nested2 */
            {},                           /* invisible */
            {},                           /* topLevelChildOfInvisible */
            {},                           /* notInOrder */
        };
        bool expectedNodesEnabled[]{
            /* All enabled except invisible, topLevelChildOfInvisible and
               notInOrder */
            true, true, true, true, true, false, false, false
        };
        Containers::Pair<UnsignedInt, UnsignedInt> expectedClipRectIdsDataCounts[]{
            {0, 3}, /* node and all children */
            {1, 1}  /* another1, unclipped */
            /* another2 has no data */
        };
        Containers::Pair<Vector2, Vector2> expectedClipRectOffsetsSizes[]{
            {{2.0f, 1.0f}, {3.0f, 5.0f}},
            {{}, {}},
            {{}, {}}
        };
        Containers::Pair<Vector2, Vector2> expectedCompositeRectOffsetsSizes[]{
            {{2.0f, 1.0f}, {3.0f, 5.0f}}, /* matching node */
            {{3.0f, 4.0f}, {1.0f, 2.0f}}, /* matching nested1 */
            {{4.0f, 3.0f}, {1.0f, 2.0f}}, /* matching nested2 */
            {{5.0f, 0.0f}, {1.0f, 2.0f}}, /* matching another1 */
        };
        /* Just a common data update was requested, which is done now */
        ui.layer<Layer>(layer).expectedState = LayerState::NeedsCommonDataUpdate;
        ui.layer<Layer>(layer).expectedDataIds = expectedDataIds;
        ui.layer<Layer>(layer).expectedNodeOffsetsSizes = expectedNodeOffsetsSizes;
        ui.layer<Layer>(layer).expectedNodesEnabled = expectedNodesEnabled;
        ui.layer<Layer>(layer).expectedClipRectIdsDataCounts = expectedClipRectIdsDataCounts;
        ui.layer<Layer>(layer).expectedClipRectOffsetsSizes = expectedClipRectOffsetsSizes;
        if(data.compositingLayer)
            ui.layer<Layer>(layer).expectedCompositeRectOffsetsSizes = expectedCompositeRectOffsetsSizes;
        ui.update();
    }
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.renderer<Renderer>().setupFramebufferCallCount, 2);
    if(data.layouters) {
        CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 0);
        CORRADE_COMPARE_AS(layouterUpdateCalls, Containers::arrayView({
            layouterHandleId(layouter2), layouterHandleId(layouter1), layouterHandleId(layouter2)
        }), TestSuite::Compare::Container);
    }
    CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
    CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 5);
    if(data.nodeAttachmentAnimators)
        CORRADE_COMPARE(ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).cleanCallCount, 0);
    if(data.dataAttachmentAnimators)
        CORRADE_COMPARE(ui.animator<AttachmentAnimator>(dataAttachmentAnimator).cleanCallCount, 0);

    /* Changing a node size sets a state flag to update layout. In this case it
       causes the nested2 node to get culled:

         2        3         4         5          6
       0                              +----------+
       1 +------------------+         | another1 |
       2 |       node       |         +----------+
       3 |                  +---------+ another2 |
       4 |        +---------| nested2 |----------+
       5 +--------| nested1 +---------+
       6          +---------+                     */
    if(!data.layouters)
        ui.setNodeSize(node, {2.0f, 4.0f});
    else
        ui.setNodeSize(node, {2.0f, 2.0f});
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsLayoutUpdate);

    /* Calling clean() should be a no-op */
    if(data.clean && data.noOp) {
        {
            CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsLayoutUpdate);
        CORRADE_COMPARE(ui.renderer<Renderer>().setupFramebufferCallCount, 2);
        if(data.layouters) {
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 0);
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 0);
        }
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 5);
        if(data.nodeAttachmentAnimators)
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).cleanCallCount, 0);
        if(data.dataAttachmentAnimators)
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(dataAttachmentAnimator).cleanCallCount, 0);
    }

    /* Calling update() calls layouters and reuploads the data except for the
       culled node, with a single size changed and resets the flag, but
       internally shouldn't do any other state rebuild */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));

        bool expectedLayoutIdsToUpdate1[]{
            /* layout2Node and layout2Nested2. Before calling layouters it's
               not yet clear that layout2Nested2 is going to be culled, so this
               includes it. */
            false, true, true, false, false
        };
        bool expectedLayoutIdsToUpdate2[]{
            /* layout1Another1 */
            false, true, false
        };
        bool expectedLayoutIdsToUpdate3[]{
            /* layout2Another1 and layout2Another2 */
            false, false, false, true, true
        };
        UnsignedInt expectedTopLevelLayoutIds1[]{
            layoutHandleId(layout2Node),
        };
        UnsignedInt expectedTopLevelLayoutIds2[]{
            layoutHandleId(layout1Another1),
        };
        UnsignedInt expectedTopLevelLayoutIds3[]{
            layoutHandleId(layout2Another1),
            /* layout2Another2 is top-level but not root, so not included
               here */
        };
        Containers::Pair<Vector2, Vector2> expectedLayoutNodeOffsetsSizes1[]{
            {{2.0f, 3.0f}, {2.0f, 2.0f}}, /* node, size changed */
            {{3.0f, 2.0f}, {0.5f, 1.0f}}, /* another1 */
            {{0.0f, 4.0f}, {1.0f, 1.0f}}, /* another2 */
            {{1.0f, 3.0f}, {1.0f, 2.0f}}, /* nested1 */
            {{2.0f, 4.0f}, {1.0f, 1.0f}}, /* nested2 */
            {{9.0f, 9.0f}, {9.0f, 9.0f}}, /* invisible */
            {{0.0f, 0.0f}, {9.0f, 9.0f}}, /* topLevelChildOfInvisible */
            {{8.0f, 8.0f}, {8.0f, 8.0f}}, /* notInOrder */
        };
        Containers::Pair<Vector2, Vector2> expectedLayoutNodeOffsetsSizes2[]{
            {{2.0f, 1.0f}, {2.0f, 4.0f}}, /* changed by layouter 2 */
            {{3.0f, 2.0f}, {0.5f, 1.0f}},
            {{0.0f, 4.0f}, {1.0f, 1.0f}},
            {{1.0f, 3.0f}, {1.0f, 2.0f}},
            {{2.0f, 2.0f}, {1.0f, 2.0f}},
            {{9.0f, 9.0f}, {9.0f, 9.0f}},
            {{0.0f, 0.0f}, {9.0f, 9.0f}},
            {{8.0f, 8.0f}, {8.0f, 8.0f}}
        };
        Containers::Pair<Vector2, Vector2> expectedLayoutNodeOffsetsSizes3[]{
            {{2.0f, 1.0f}, {2.0f, 4.0f}},
            {{5.0f, 2.0f}, {1.0f, 1.0f}}, /* changed by layouter 1 */
            {{0.0f, 4.0f}, {1.0f, 1.0f}},
            {{1.0f, 3.0f}, {1.0f, 2.0f}},
            {{2.0f, 2.0f}, {1.0f, 2.0f}},
            {{9.0f, 9.0f}, {9.0f, 9.0f}},
            {{0.0f, 0.0f}, {9.0f, 9.0f}},
            {{8.0f, 8.0f}, {8.0f, 8.0f}}
        };
        if(data.layouters) {
            layouterUpdateCalls = {};
            ui.layouter<Layouter2>(layouter2).expectedLayoutIdsToUpdate[0] = expectedLayoutIdsToUpdate1;
            ui.layouter<Layouter1>(layouter1).expectedLayoutIdsToUpdate = expectedLayoutIdsToUpdate2;
            ui.layouter<Layouter2>(layouter2).expectedLayoutIdsToUpdate[1] = expectedLayoutIdsToUpdate3;
            ui.layouter<Layouter2>(layouter2).expectedTopLevelLayoutIds[0] = expectedTopLevelLayoutIds1;
            ui.layouter<Layouter1>(layouter1).expectedTopLevelLayoutIds = expectedTopLevelLayoutIds2;
            ui.layouter<Layouter2>(layouter2).expectedTopLevelLayoutIds[1] = expectedTopLevelLayoutIds3;
            ui.layouter<Layouter2>(layouter2).expectedNodeOffsetsSizes[0] = expectedLayoutNodeOffsetsSizes1;
            ui.layouter<Layouter1>(layouter1).expectedNodeOffsetsSizes = expectedLayoutNodeOffsetsSizes2;
            ui.layouter<Layouter2>(layouter2).expectedNodeOffsetsSizes[1] = expectedLayoutNodeOffsetsSizes3;
            ui.layouter<Layouter2>(layouter2).updateCallId = 0;
        }

        UnsignedInt expectedDataIds[]{
            dataHandleId(dataNode),
            dataHandleId(dataNested1),
            dataHandleId(dataAnother1),
        };
        Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
            {{2.0f, 1.0f}, {2.0f, 4.0f}}, /* node */
            {{5.0f, 0.0f}, {1.0f, 2.0f}}, /* another1 */
            {{5.0f, 2.0f}, {1.0f, 2.0f}}, /* another2 */
            {{3.0f, 4.0f}, {1.0f, 2.0f}}, /* nested1 */
            {},
            {},                           /* invisible */
            {},                           /* topLevelChildOfInvisible */
            {},                           /* notInOrder */
        };
        bool expectedNodesEnabled[]{
            /* All enabled except invisible, topLevelChildOfInvisible,
               notInOrder and now nested2 */
            true, true, true, true, false, false, false, false
        };
        Containers::Pair<UnsignedInt, UnsignedInt> expectedClipRectIdsDataCounts[]{
            {0, 2}, /* node and remaining child */
            {1, 1}  /* another1, unclipped */
            /* another2 has no data */
        };
        Containers::Pair<Vector2, Vector2> expectedClipRectOffsetsSizes[]{
            {{2.0f, 1.0f}, {2.0f, 4.0f}},
            {{}, {}},
            {{}, {}}
        };
        Containers::Pair<Vector2, Vector2> expectedCompositeRectOffsetsSizes[]{
            {{2.0f, 1.0f}, {2.0f, 4.0f}}, /* matching node */
            {{3.0f, 4.0f}, {1.0f, 1.0f}}, /* matching nested1, clipped */
            {{5.0f, 0.0f}, {1.0f, 2.0f}}, /* matching another1 */
        };
        /* Updating node size means offsets/sizes have to be changed, and the
           order + enabled state as well, as the set of visible nodes may be
           different now */
        ui.layer<Layer>(layer).expectedState = LayerState::NeedsNodeOffsetSizeUpdate|LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsNodeOrderUpdate|(data.compositingLayer ? LayerState::NeedsCompositeOffsetSizeUpdate : LayerStates{});
        ui.layer<Layer>(layer).expectedDataIds = expectedDataIds;
        ui.layer<Layer>(layer).expectedNodeOffsetsSizes = expectedNodeOffsetsSizes;
        ui.layer<Layer>(layer).expectedNodesEnabled = expectedNodesEnabled;
        ui.layer<Layer>(layer).expectedClipRectIdsDataCounts = expectedClipRectIdsDataCounts;
        ui.layer<Layer>(layer).expectedClipRectOffsetsSizes = expectedClipRectOffsetsSizes;
        if(data.compositingLayer)
            ui.layer<Layer>(layer).expectedCompositeRectOffsetsSizes = expectedCompositeRectOffsetsSizes;

        ui.update();
    }
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.renderer<Renderer>().setupFramebufferCallCount, 2);
    if(data.layouters) {
        CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 0);
        CORRADE_COMPARE_AS(layouterUpdateCalls, Containers::arrayView({
            /* update() again gets called twice on layouter 2 */
            layouterHandleId(layouter2), layouterHandleId(layouter1), layouterHandleId(layouter2)
        }), TestSuite::Compare::Container);
    }
    CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
    CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 6);
    if(data.nodeAttachmentAnimators)
        CORRADE_COMPARE(ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).cleanCallCount, 0);
    if(data.dataAttachmentAnimators)
        CORRADE_COMPARE(ui.animator<AttachmentAnimator>(dataAttachmentAnimator).cleanCallCount, 0);

    /* Changing a node offset sets a state flag to update layout and
       recalculate nested node offsets, except for nested2 that's still culled.

         2        3         4         5         6
       0                              +----------+
       1          +-------------------| another1 |
       2          |       node        +----------+
       3          |                   +----------+
       4          |         +---------|  nested2 |
       5          +---------| nested1 +----------+
       6                    +---------+           */
    if(!data.layouters)
        ui.setNodeOffset(node, {3.0f, 1.0f});
    else
        ui.setNodeOffset(node, {3.0f, 3.0f});
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsLayoutUpdate);

    /* Calling clean() should be a no-op */
    if(data.clean && data.noOp) {
        {
            CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsLayoutUpdate);
        CORRADE_COMPARE(ui.renderer<Renderer>().setupFramebufferCallCount, 2);
        if(data.layouters) {
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 0);
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 0);
        }
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 6);
        if(data.nodeAttachmentAnimators)
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).cleanCallCount, 0);
        if(data.dataAttachmentAnimators)
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(dataAttachmentAnimator).cleanCallCount, 0);
    }

    /* Calling update() calls layouters, recalculates absoute offsets, uploads
       the new data and resets the flag */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));

        bool expectedLayoutIdsToUpdate1[]{
            /* layout2Node and layout2Nested2. Again, before calling layouters
               it's not yet clear that layout2Nested2 is going to be culled
               even though it was culled before, so this includes it. */
            false, true, true, false, false
        };
        bool expectedLayoutIdsToUpdate2[]{
            /* layout1Another1 */
            false, true, false
        };
        bool expectedLayoutIdsToUpdate3[]{
            /* layout2Another1 and layout2Another2 */
            false, false, false, true, true
        };
        UnsignedInt expectedTopLevelLayoutIds1[]{
            layoutHandleId(layout2Node),
        };
        UnsignedInt expectedTopLevelLayoutIds2[]{
            layoutHandleId(layout1Another1),
        };
        UnsignedInt expectedTopLevelLayoutIds3[]{
            layoutHandleId(layout2Another1)
            /* layout2Another2 is top-level but not root, so not included
               here */
        };
        Containers::Pair<Vector2, Vector2> expectedLayoutNodeOffsetsSizes1[]{
            {{3.0f, 3.0f}, {2.0f, 2.0f}}, /* node, offset changed */
            {{3.0f, 2.0f}, {0.5f, 1.0f}}, /* another1 */
            {{0.0f, 4.0f}, {1.0f, 1.0f}}, /* another2 */
            {{1.0f, 3.0f}, {1.0f, 2.0f}}, /* nested1 */
            {{2.0f, 4.0f}, {1.0f, 1.0f}}, /* nested2 */
            {{9.0f, 9.0f}, {9.0f, 9.0f}}, /* invisible */
            {{0.0f, 0.0f}, {9.0f, 9.0f}}, /* topLevelChildOfInvisible */
            {{8.0f, 8.0f}, {8.0f, 8.0f}}, /* notInOrder */
        };
        Containers::Pair<Vector2, Vector2> expectedLayoutNodeOffsetsSizes2[]{
            {{3.0f, 1.0f}, {2.0f, 4.0f}}, /* changed by layouter 2 */
            {{3.0f, 2.0f}, {0.5f, 1.0f}},
            {{0.0f, 4.0f}, {1.0f, 1.0f}}, /* changed by layouter 2 */
            {{1.0f, 3.0f}, {1.0f, 2.0f}},
            {{2.0f, 2.0f}, {1.0f, 2.0f}},
            {{9.0f, 9.0f}, {9.0f, 9.0f}},
            {{0.0f, 0.0f}, {9.0f, 9.0f}},
            {{8.0f, 8.0f}, {8.0f, 8.0f}}
        };
        Containers::Pair<Vector2, Vector2> expectedLayoutNodeOffsetsSizes3[]{
            {{3.0f, 1.0f}, {2.0f, 4.0f}},
            {{5.0f, 2.0f}, {1.0f, 1.0f}}, /* changed by layouter 1 */
            {{0.0f, 4.0f}, {1.0f, 1.0f}},
            {{1.0f, 3.0f}, {1.0f, 2.0f}},
            {{2.0f, 2.0f}, {1.0f, 2.0f}},
            {{9.0f, 9.0f}, {9.0f, 9.0f}},
            {{0.0f, 0.0f}, {9.0f, 9.0f}},
            {{8.0f, 8.0f}, {8.0f, 8.0f}}
        };
        if(data.layouters) {
            layouterUpdateCalls = {};
            ui.layouter<Layouter2>(layouter2).expectedLayoutIdsToUpdate[0] = expectedLayoutIdsToUpdate1;
            ui.layouter<Layouter1>(layouter1).expectedLayoutIdsToUpdate = expectedLayoutIdsToUpdate2;
            ui.layouter<Layouter2>(layouter2).expectedLayoutIdsToUpdate[1] = expectedLayoutIdsToUpdate3;
            ui.layouter<Layouter2>(layouter2).expectedTopLevelLayoutIds[0] = expectedTopLevelLayoutIds1;
            ui.layouter<Layouter1>(layouter1).expectedTopLevelLayoutIds = expectedTopLevelLayoutIds2;
            ui.layouter<Layouter2>(layouter2).expectedTopLevelLayoutIds[1] = expectedTopLevelLayoutIds3;
            ui.layouter<Layouter2>(layouter2).expectedNodeOffsetsSizes[0] = expectedLayoutNodeOffsetsSizes1;
            ui.layouter<Layouter1>(layouter1).expectedNodeOffsetsSizes = expectedLayoutNodeOffsetsSizes2;
            ui.layouter<Layouter2>(layouter2).expectedNodeOffsetsSizes[1] = expectedLayoutNodeOffsetsSizes3;
            ui.layouter<Layouter2>(layouter2).updateCallId = 0;
        }

        UnsignedInt expectedDataIds[]{
            dataHandleId(dataNode),
            dataHandleId(dataNested1),
            dataHandleId(dataAnother1),
        };
        Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
            {{3.0f, 1.0f}, {2.0f, 4.0f}}, /* node */
            {{5.0f, 0.0f}, {1.0f, 2.0f}}, /* another1 */
            {{5.0f, 2.0f}, {1.0f, 2.0f}}, /* another2 */
            {{4.0f, 4.0f}, {1.0f, 2.0f}}, /* nested1 */
            {},
            {},                           /* invisible */
            {},                           /* topLevelChildOfInvisible */
            {},                           /* notInOrder */
        };
        bool expectedNodesEnabled[]{
            /* All enabled except invisible, topLevelChildOfInvisible,
               notInOrder and again nested2 */
            true, true, true, true, false, false, false, false
        };
        Containers::Pair<UnsignedInt, UnsignedInt> expectedClipRectIdsDataCounts[]{
            {0, 2}, /* node and remaining child */
            {1, 1}  /* another1, unclipped */
            /* another2 has no data */
        };
        Containers::Pair<Vector2, Vector2> expectedClipRectOffsetsSizes[]{
            {{3.0f, 1.0f}, {2.0f, 4.0f}},
            {{}, {}},
            {{}, {}}
        };
        Containers::Pair<Vector2, Vector2> expectedCompositeRectOffsetsSizes[]{
            {{3.0f, 1.0f}, {2.0f, 4.0f}}, /* matching node */
            {{4.0f, 4.0f}, {1.0f, 1.0f}}, /* matching nested1, clipped */
            {{5.0f, 0.0f}, {1.0f, 2.0f}}, /* matching another1 */
        };
        /* Updating node size agains means offsets/sizes + order + enabled
           has to be updated now */
        ui.layer<Layer>(layer).expectedState = LayerState::NeedsNodeOffsetSizeUpdate|LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsNodeOrderUpdate|(data.compositingLayer ? LayerState::NeedsCompositeOffsetSizeUpdate : LayerStates{});
        ui.layer<Layer>(layer).expectedDataIds = expectedDataIds;
        ui.layer<Layer>(layer).expectedNodeOffsetsSizes = expectedNodeOffsetsSizes;
        ui.layer<Layer>(layer).expectedNodesEnabled = expectedNodesEnabled;
        ui.layer<Layer>(layer).expectedClipRectIdsDataCounts = expectedClipRectIdsDataCounts;
        ui.layer<Layer>(layer).expectedClipRectOffsetsSizes = expectedClipRectOffsetsSizes;
        if(data.compositingLayer)
            ui.layer<Layer>(layer).expectedCompositeRectOffsetsSizes = expectedCompositeRectOffsetsSizes;

        ui.update();
    }
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.renderer<Renderer>().setupFramebufferCallCount, 2);
    if(data.layouters) {
        CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 0);
        CORRADE_COMPARE_AS(layouterUpdateCalls, Containers::arrayView({
            /* update() again gets called twice on layouter 2 */
            layouterHandleId(layouter2), layouterHandleId(layouter1), layouterHandleId(layouter2)
        }), TestSuite::Compare::Container);
    }
    CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
    CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 7);
    if(data.nodeAttachmentAnimators)
        CORRADE_COMPARE(ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).cleanCallCount, 0);
    if(data.dataAttachmentAnimators)
        CORRADE_COMPARE(ui.animator<AttachmentAnimator>(dataAttachmentAnimator).cleanCallCount, 0);

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
        CORRADE_COMPARE(ui.renderer<Renderer>().setupFramebufferCallCount, 2);
        if(data.layouters) {
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 0);
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 0);
        }
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 7);
        if(data.nodeAttachmentAnimators)
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).cleanCallCount, 0);
        if(data.dataAttachmentAnimators)
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(dataAttachmentAnimator).cleanCallCount, 0);
    }

    /* Calling update() rebuilds internal state without the hidden hierarchy,
       calls layouters and uploads the data */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));

        bool expectedLayoutIdsToUpdate1[]{
            /* layout1Another1 */
            false, true, false
        };
        bool expectedLayoutIdsToUpdate2[]{
            /* layout2Another1 and layout2Another2 */
            false, false, false, true, true
        };
        UnsignedInt expectedTopLevelLayoutIds1[]{
            layoutHandleId(layout1Another1),
        };
        UnsignedInt expectedTopLevelLayoutIds2[]{
            layoutHandleId(layout2Another1),
            /* layout2Another2 is top-level but not root, so not included
               here */
        };
        Containers::Pair<Vector2, Vector2> expectedLayoutNodeOffsetsSizes1[]{
            {},
            {{3.0f, 2.0f}, {0.5f, 1.0f}}, /* another1 */
            {{0.0f, 4.0f}, {1.0f, 1.0f}}, /* another2 */
            {},
            {},
            {{9.0f, 9.0f}, {9.0f, 9.0f}}, /* invisible */
            {{0.0f, 0.0f}, {9.0f, 9.0f}}, /* topLevelChildOfInvisible */
            {{8.0f, 8.0f}, {8.0f, 8.0f}}  /* notInOrder */
        };
        Containers::Pair<Vector2, Vector2> expectedLayoutNodeOffsetsSizes2[]{
            {},
            {{5.0f, 2.0f}, {1.0f, 1.0f}}, /* changed by layouter 1 */
            {{0.0f, 4.0f}, {1.0f, 1.0f}},
            {},
            {},
            {{9.0f, 9.0f}, {9.0f, 9.0f}},
            {{0.0f, 0.0f}, {9.0f, 9.0f}},
            {{8.0f, 8.0f}, {8.0f, 8.0f}},
        };
        if(data.layouters) {
            layouterUpdateCalls = {};
            ui.layouter<Layouter1>(layouter1).expectedLayoutIdsToUpdate = expectedLayoutIdsToUpdate1;
            ui.layouter<Layouter2>(layouter2).expectedLayoutIdsToUpdate[0] = expectedLayoutIdsToUpdate2;
            ui.layouter<Layouter2>(layouter2).expectedLayoutIdsToUpdate[1] = {};
            ui.layouter<Layouter1>(layouter1).expectedTopLevelLayoutIds = expectedTopLevelLayoutIds1;
            ui.layouter<Layouter2>(layouter2).expectedTopLevelLayoutIds[0] = expectedTopLevelLayoutIds2;
            ui.layouter<Layouter2>(layouter2).expectedTopLevelLayoutIds[1] = {};
            ui.layouter<Layouter1>(layouter1).expectedNodeOffsetsSizes = expectedLayoutNodeOffsetsSizes1;
            ui.layouter<Layouter2>(layouter2).expectedNodeOffsetsSizes[0] = expectedLayoutNodeOffsetsSizes2;
            ui.layouter<Layouter2>(layouter2).expectedNodeOffsetsSizes[1] = {};
            ui.layouter<Layouter2>(layouter2).updateCallId = 0;
        }

        UnsignedInt expectedDataIds[]{
            dataHandleId(dataAnother1),
        };
        Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
            {},
            {{5.0f, 0.0f}, {1.0f, 2.0f}}, /* another1 */
            {{5.0f, 2.0f}, {1.0f, 2.0f}}, /* another2 */
            {},
            {},
            {},                           /* invisible */
            {},                           /* topLevelChildOfInvisible */
            {},                           /* notInOrder */
        };
        bool expectedNodesEnabled[]{
            /* Only another1 and another2 are left visible (and enabled) now */
            false, true, true, false, false, false, false, false
        };
        Containers::Pair<UnsignedInt, UnsignedInt> expectedClipRectIdsDataCounts[]{
            {0, 1}  /* another1, unclipped */
            /* another2 has no data */
        };
        Containers::Pair<Vector2, Vector2> expectedClipRectOffsetsSizes[]{
            {{}, {}},
            {{}, {}}
        };
        Containers::Pair<Vector2, Vector2> expectedCompositeRectOffsetsSizes[]{
            {{5.0f, 0.0f}, {1.0f, 2.0f}}, /* matching another1 */
        };
        /* Updating node hidden flags means order has to be changed due to
           potentially different set of nodes being visible (and also enabled),
           which is however smashed together with offset/size changes due to
           the coarseness of the UserInterfaceState bits. */
        /** @todo separate those */
        ui.layer<Layer>(layer).expectedState = LayerState::NeedsNodeOrderUpdate|LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsNodeOffsetSizeUpdate|(data.compositingLayer ? LayerState::NeedsCompositeOffsetSizeUpdate : LayerStates{});
        ui.layer<Layer>(layer).expectedDataIds = expectedDataIds;
        ui.layer<Layer>(layer).expectedNodeOffsetsSizes = expectedNodeOffsetsSizes;
        ui.layer<Layer>(layer).expectedNodesEnabled = expectedNodesEnabled;
        ui.layer<Layer>(layer).expectedClipRectIdsDataCounts = expectedClipRectIdsDataCounts;
        ui.layer<Layer>(layer).expectedClipRectOffsetsSizes = expectedClipRectOffsetsSizes;
        if(data.compositingLayer)
            ui.layer<Layer>(layer).expectedCompositeRectOffsetsSizes = expectedCompositeRectOffsetsSizes;

        ui.update();
    }
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.renderer<Renderer>().setupFramebufferCallCount, 2);
    if(data.layouters) {
        CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 0);
        CORRADE_COMPARE_AS(layouterUpdateCalls, Containers::arrayView({
            /* layouter 2 gets called just once this time, i.e. the usual first
               call to layouter2 is omitted now */
            layouterHandleId(layouter1), layouterHandleId(layouter2)
        }), TestSuite::Compare::Container);
    }
    CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
    CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 8);
    if(data.nodeAttachmentAnimators)
        CORRADE_COMPARE(ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).cleanCallCount, 0);
    if(data.dataAttachmentAnimators)
        CORRADE_COMPARE(ui.animator<AttachmentAnimator>(dataAttachmentAnimator).cleanCallCount, 0);

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
        CORRADE_COMPARE(ui.renderer<Renderer>().setupFramebufferCallCount, 2);
        if(data.layouters) {
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 0);
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 0);
        }
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 8);
        if(data.nodeAttachmentAnimators)
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).cleanCallCount, 0);
        if(data.dataAttachmentAnimators)
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(dataAttachmentAnimator).cleanCallCount, 0);
    }

    /* Calling update() calls layouters with previous data, reuploads the
       previous data again and resets the state flag */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));

        bool expectedLayoutIdsToUpdate1[]{
            /* layout2Node and layout2Nested2 */
            false, true, true, false, false
        };
        bool expectedLayoutIdsToUpdate2[]{
            /* layout1Another1 */
            false, true, false
        };
        bool expectedLayoutIdsToUpdate3[]{
            /* layout2Another1 and layout2Another2 */
            false, false, false, true, true
        };
        UnsignedInt expectedTopLevelLayoutIds1[]{
            layoutHandleId(layout2Node),
        };
        UnsignedInt expectedTopLevelLayoutIds2[]{
            layoutHandleId(layout1Another1),
        };
        UnsignedInt expectedTopLevelLayoutIds3[]{
            layoutHandleId(layout2Another1),
            /* layout2Another2 is top-level but not root, so not included
               here */
        };
        Containers::Pair<Vector2, Vector2> expectedLayoutNodeOffsetsSizes1[]{
            {{3.0f, 3.0f}, {2.0f, 2.0f}}, /* node, visible again */
            {{3.0f, 2.0f}, {0.5f, 1.0f}}, /* another1 */
            {{0.0f, 4.0f}, {1.0f, 1.0f}}, /* another2 */
            {{1.0f, 3.0f}, {1.0f, 2.0f}}, /* nested1 */
            {{2.0f, 4.0f}, {1.0f, 1.0f}}, /* nested2 */
            {{9.0f, 9.0f}, {9.0f, 9.0f}}, /* invisible */
            {{0.0f, 0.0f}, {9.0f, 9.0f}}, /* topLevelChildOfInvisible */
            {{8.0f, 8.0f}, {8.0f, 8.0f}}, /* notInOrder */
        };
        Containers::Pair<Vector2, Vector2> expectedLayoutNodeOffsetsSizes2[]{
            {{3.0f, 1.0f}, {2.0f, 4.0f}}, /* changed by layouter 2 */
            {{3.0f, 2.0f}, {0.5f, 1.0f}},
            {{0.0f, 4.0f}, {1.0f, 1.0f}},
            {{1.0f, 3.0f}, {1.0f, 2.0f}},
            {{2.0f, 2.0f}, {1.0f, 2.0f}},
            {{9.0f, 9.0f}, {9.0f, 9.0f}},
            {{0.0f, 0.0f}, {9.0f, 9.0f}},
            {{8.0f, 8.0f}, {8.0f, 8.0f}}
        };
        Containers::Pair<Vector2, Vector2> expectedLayoutNodeOffsetsSizes3[]{
            {{3.0f, 1.0f}, {2.0f, 4.0f}},
            {{5.0f, 2.0f}, {1.0f, 1.0f}}, /* changed by layouter 1 */
            {{0.0f, 4.0f}, {1.0f, 1.0f}},
            {{1.0f, 3.0f}, {1.0f, 2.0f}},
            {{2.0f, 2.0f}, {1.0f, 2.0f}},
            {{9.0f, 9.0f}, {9.0f, 9.0f}},
            {{0.0f, 0.0f}, {9.0f, 9.0f}},
            {{8.0f, 8.0f}, {8.0f, 8.0f}}
        };
        if(data.layouters) {
            layouterUpdateCalls = {};
            ui.layouter<Layouter2>(layouter2).expectedLayoutIdsToUpdate[0] = expectedLayoutIdsToUpdate1;
            ui.layouter<Layouter1>(layouter1).expectedLayoutIdsToUpdate = expectedLayoutIdsToUpdate2;
            ui.layouter<Layouter2>(layouter2).expectedLayoutIdsToUpdate[1] = expectedLayoutIdsToUpdate3;
            ui.layouter<Layouter2>(layouter2).expectedTopLevelLayoutIds[0] = expectedTopLevelLayoutIds1;
            ui.layouter<Layouter1>(layouter1).expectedTopLevelLayoutIds = expectedTopLevelLayoutIds2;
            ui.layouter<Layouter2>(layouter2).expectedTopLevelLayoutIds[1] = expectedTopLevelLayoutIds3;
            ui.layouter<Layouter2>(layouter2).expectedNodeOffsetsSizes[0] = expectedLayoutNodeOffsetsSizes1;
            ui.layouter<Layouter1>(layouter1).expectedNodeOffsetsSizes = expectedLayoutNodeOffsetsSizes2;
            ui.layouter<Layouter2>(layouter2).expectedNodeOffsetsSizes[1] = expectedLayoutNodeOffsetsSizes3;
            ui.layouter<Layouter2>(layouter2).updateCallId = 0;
        }

        UnsignedInt expectedDataIds[]{
            dataHandleId(dataNode),
            dataHandleId(dataNested1),
            dataHandleId(dataAnother1),
        };
        Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
            {{3.0f, 1.0f}, {2.0f, 4.0f}}, /* node */
            {{5.0f, 0.0f}, {1.0f, 2.0f}}, /* another1 */
            {{5.0f, 2.0f}, {1.0f, 2.0f}}, /* another2 */
            {{4.0f, 4.0f}, {1.0f, 2.0f}}, /* nested1 */
            {},
            {},                           /* invisible */
            {},                           /* topLevelChildOfInvisible */
            {},                           /* notInOrder */
        };
        bool expectedNodesEnabled[]{
            /* All enabled except invisible, topLevelChildOfInvisible,
               notInOrder and nested2 again */
            true, true, true, true, false, false, false, false
        };
        Containers::Pair<UnsignedInt, UnsignedInt> expectedClipRectIdsDataCounts[]{
            {0, 2}, /* node and remaining child */
            {1, 1}  /* another1, unclipped */
            /* another2 has no data */
        };
        Containers::Pair<Vector2, Vector2> expectedClipRectOffsetsSizes[]{
            {{3.0f, 1.0f}, {2.0f, 4.0f}},
            {{}, {}},
            {{}, {}}
        };
        Containers::Pair<Vector2, Vector2> expectedCompositeRectOffsetsSizes[]{
            {{3.0f, 1.0f}, {2.0f, 4.0f}}, /* matching node */
            {{4.0f, 4.0f}, {1.0f, 1.0f}}, /* matching nested1, clipped */
            {{5.0f, 0.0f}, {1.0f, 2.0f}}, /* matching another1 */
        };
        /* Updating node hidden flags again means order has to be changed due
           to potentially different set of nodes being visible (and enabled),
           which is again smashed together with offset/size changes due to
           the coarseness of the UserInterfaceState bits. */
        /** @todo separate those */
        ui.layer<Layer>(layer).expectedState = LayerState::NeedsNodeOrderUpdate|LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsNodeOffsetSizeUpdate|(data.compositingLayer ? LayerState::NeedsCompositeOffsetSizeUpdate : LayerStates{});
        ui.layer<Layer>(layer).expectedDataIds = expectedDataIds;
        ui.layer<Layer>(layer).expectedNodeOffsetsSizes = expectedNodeOffsetsSizes;
        ui.layer<Layer>(layer).expectedNodesEnabled = expectedNodesEnabled;
        ui.layer<Layer>(layer).expectedClipRectIdsDataCounts = expectedClipRectIdsDataCounts;
        ui.layer<Layer>(layer).expectedClipRectOffsetsSizes = expectedClipRectOffsetsSizes;
        if(data.compositingLayer)
            ui.layer<Layer>(layer).expectedCompositeRectOffsetsSizes = expectedCompositeRectOffsetsSizes;

        ui.update();
    }
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.renderer<Renderer>().setupFramebufferCallCount, 2);
    if(data.layouters) {
        CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 0);
        CORRADE_COMPARE_AS(layouterUpdateCalls, Containers::arrayView({
            /* update() again gets called twice on layouter 2 */
            layouterHandleId(layouter2), layouterHandleId(layouter1), layouterHandleId(layouter2)
        }), TestSuite::Compare::Container);
    }
    CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
    CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 9);
    if(data.nodeAttachmentAnimators)
        CORRADE_COMPARE(ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).cleanCallCount, 0);
    if(data.dataAttachmentAnimators)
        CORRADE_COMPARE(ui.animator<AttachmentAnimator>(dataAttachmentAnimator).cleanCallCount, 0);

    /* Resetting a Hidden flag that's not there should be a no-op,
       independently of what other flags get cleared */
    ui.clearNodeFlags(node, NodeFlag(0x70)|NodeFlag::Hidden);
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

    /* Setting a Disabled flag sets a state flag */
    ui.addNodeFlags(node, NodeFlag::Disabled);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeEnabledUpdate);

    /* Calling clean() should be a no-op */
    if(data.clean && data.noOp) {
        {
            CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeEnabledUpdate);
        CORRADE_COMPARE(ui.renderer<Renderer>().setupFramebufferCallCount, 2);
        if(data.layouters) {
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 0);
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 0);
            CORRADE_COMPARE_AS(layouterUpdateCalls, Containers::arrayView({
                layouterHandleId(layouter2), layouterHandleId(layouter1), layouterHandleId(layouter2)
            }), TestSuite::Compare::Container);
        }
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 9);
    }

    /* Calling update() rebuilds internal masks of enabled nodes. It doesn't
       call layouters. */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
        UnsignedInt expectedDataIds[]{
            dataHandleId(dataNode),
            dataHandleId(dataNested1),
            dataHandleId(dataAnother1),
        };
        Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
            {{3.0f, 1.0f}, {2.0f, 4.0f}}, /* node */
            {{5.0f, 0.0f}, {1.0f, 2.0f}}, /* another1 */
            {{5.0f, 2.0f}, {1.0f, 2.0f}}, /* another2 */
            {{4.0f, 4.0f}, {1.0f, 2.0f}}, /* nested1 */
            {},
            {},                           /* invisible */
            {},                           /* topLevelChildOfInvisible */
            {},                           /* notInOrder */
        };
        bool expectedNodesEnabled[]{
            /* Only another1 and another2 are left enabled now */
            false, true, true, false, false, false, false, false
        };
        Containers::Pair<UnsignedInt, UnsignedInt> expectedClipRectIdsDataCounts[]{
            {0, 2}, /* node and remaining child */
            {1, 1}  /* another1, unclipped */
            /* another2 has no data */
        };
        Containers::Pair<Vector2, Vector2> expectedClipRectOffsetsSizes[]{
            {{3.0f, 1.0f}, {2.0f, 4.0f}},
            {{}, {}},
            {{}, {}}
        };
        Containers::Pair<Vector2, Vector2> expectedCompositeRectOffsetsSizes[]{
            {{3.0f, 1.0f}, {2.0f, 4.0f}}, /* matching node */
            {{4.0f, 4.0f}, {1.0f, 1.0f}}, /* matching nested1, clipped */
            {{5.0f, 0.0f}, {1.0f, 2.0f}}, /* matching another1 */
        };
        /* Toggling a node Disabled flag means enabled state has to be updated,
           however due to the coarseness of UserInterfaceState bits it's
           together with draw order as well */
        /** @todo separate those */
        ui.layer<Layer>(layer).expectedState = LayerState::NeedsNodeOrderUpdate|LayerState::NeedsNodeEnabledUpdate;
        ui.layer<Layer>(layer).expectedDataIds = expectedDataIds;
        ui.layer<Layer>(layer).expectedNodeOffsetsSizes = expectedNodeOffsetsSizes;
        ui.layer<Layer>(layer).expectedNodesEnabled = expectedNodesEnabled;
        ui.layer<Layer>(layer).expectedClipRectIdsDataCounts = expectedClipRectIdsDataCounts;
        ui.layer<Layer>(layer).expectedClipRectOffsetsSizes = expectedClipRectOffsetsSizes;
        if(data.compositingLayer)
            ui.layer<Layer>(layer).expectedCompositeRectOffsetsSizes = expectedCompositeRectOffsetsSizes;

        ui.update();
    }
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.renderer<Renderer>().setupFramebufferCallCount, 2);
    if(data.layouters) {
        CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 0);
        CORRADE_COMPARE_AS(layouterUpdateCalls, Containers::arrayView({
            /* update() again gets called twice on layouter 2 */
            layouterHandleId(layouter2), layouterHandleId(layouter1), layouterHandleId(layouter2)
        }), TestSuite::Compare::Container);
    }
    CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
    CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 10);
    if(data.nodeAttachmentAnimators)
        CORRADE_COMPARE(ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).cleanCallCount, 0);
    if(data.dataAttachmentAnimators)
        CORRADE_COMPARE(ui.animator<AttachmentAnimator>(dataAttachmentAnimator).cleanCallCount, 0);

    /* Setting a Disabled flag that's already there should be a no-op */
    ui.addNodeFlags(node, NodeFlag::Disabled);
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

    /* Setting an NoEvents flag that's already implied by Disabled should be a
       no-op */
    ui.addNodeFlags(node, NodeFlag::NoEvents);
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

    /* Setting just the NoEvents flag (i.e, removing what Disabled adds on top
       of it) sets a state flag. It'll no longer mark the nodes as disabled for
       drawing, but still updates internal state related to events (which is
       tested directly in the event*NodeHiddenDisabledNoEvents() cases). */
    ui.clearNodeFlags(node, NodeFlag::Disabled & ~NodeFlag::NoEvents);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeEnabledUpdate);

    /* Calling clean() should be a no-op */
    if(data.clean && data.noOp) {
        {
            CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeEnabledUpdate);
        CORRADE_COMPARE(ui.renderer<Renderer>().setupFramebufferCallCount, 2);
        if(data.layouters) {
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 0);
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 0);
        }
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 10);
        if(data.nodeAttachmentAnimators)
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).cleanCallCount, 0);
        if(data.dataAttachmentAnimators)
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(dataAttachmentAnimator).cleanCallCount, 0);
    }

    /* Calling update() rebuilds internal masks of enabled nodes. It doesn't
       call layouters. */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
        UnsignedInt expectedDataIds[]{
            dataHandleId(dataNode),
            dataHandleId(dataNested1),
            dataHandleId(dataAnother1),
        };
        Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
            {{3.0f, 1.0f}, {2.0f, 4.0f}}, /* node */
            {{5.0f, 0.0f}, {1.0f, 2.0f}}, /* another1 */
            {{5.0f, 2.0f}, {1.0f, 2.0f}}, /* another2 */
            {{4.0f, 4.0f}, {1.0f, 2.0f}}, /* nested1 */
            {},
            {},                           /* invisible */
            {},                           /* topLevelChildOfInvisible */
            {},                           /* notInOrder */
        };
        bool expectedNodesEnabled[]{
            /* Again all enabled except invisible, topLevelChildOfInvisible,
               notInOrder and nested2 */
            true, true, true, true, false, false, false, false
        };
        Containers::Pair<UnsignedInt, UnsignedInt> expectedClipRectIdsDataCounts[]{
            {0, 2}, /* node and remaining child */
            {1, 1}  /* another1, unclipped */
            /* another2 has no data */
        };
        Containers::Pair<Vector2, Vector2> expectedClipRectOffsetsSizes[]{
            {{3.0f, 1.0f}, {2.0f, 4.0f}},
            {{}, {}},
            {{}, {}}
        };
        Containers::Pair<Vector2, Vector2> expectedCompositeRectOffsetsSizes[]{
            {{3.0f, 1.0f}, {2.0f, 4.0f}}, /* matching node */
            {{4.0f, 4.0f}, {1.0f, 1.0f}}, /* matching nested1, clipped */
            {{5.0f, 0.0f}, {1.0f, 2.0f}}, /* matching another1 */
        };
        /* Toggling a node NoEvents flag causes the same as Disabled due to the
           coarseness of UserInterfaceState bits */
        /** @todo separate those from disabled, separate from node order */
        ui.layer<Layer>(layer).expectedState = LayerState::NeedsNodeOrderUpdate|LayerState::NeedsNodeEnabledUpdate;
        ui.layer<Layer>(layer).expectedDataIds = expectedDataIds;
        ui.layer<Layer>(layer).expectedNodeOffsetsSizes = expectedNodeOffsetsSizes;
        ui.layer<Layer>(layer).expectedNodesEnabled = expectedNodesEnabled;
        ui.layer<Layer>(layer).expectedClipRectIdsDataCounts = expectedClipRectIdsDataCounts;
        ui.layer<Layer>(layer).expectedClipRectOffsetsSizes = expectedClipRectOffsetsSizes;
        if(data.compositingLayer)
            ui.layer<Layer>(layer).expectedCompositeRectOffsetsSizes = expectedCompositeRectOffsetsSizes;

        ui.update();
    }
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.renderer<Renderer>().setupFramebufferCallCount, 2);
    if(data.layouters) {
        CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 0);
        CORRADE_COMPARE_AS(layouterUpdateCalls, Containers::arrayView({
            /* update() again gets called twice on layouter 2 */
            layouterHandleId(layouter2), layouterHandleId(layouter1), layouterHandleId(layouter2)
        }), TestSuite::Compare::Container);
    }
    CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
    CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 11);
    if(data.nodeAttachmentAnimators)
        CORRADE_COMPARE(ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).cleanCallCount, 0);
    if(data.dataAttachmentAnimators)
        CORRADE_COMPARE(ui.animator<AttachmentAnimator>(dataAttachmentAnimator).cleanCallCount, 0);

    /* Clearing also the NoEvents flag sets a state flag to update also the
       event-related masks. */
    ui.clearNodeFlags(node, NodeFlag::NoEvents);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeEnabledUpdate);

    /* Calling clean() should be a no-op */
    if(data.clean && data.noOp) {
        {
            CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeEnabledUpdate);
        CORRADE_COMPARE(ui.renderer<Renderer>().setupFramebufferCallCount, 2);
        if(data.layouters) {
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 0);
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 0);
        }
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 11);
        if(data.nodeAttachmentAnimators)
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).cleanCallCount, 0);
        if(data.dataAttachmentAnimators)
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(dataAttachmentAnimator).cleanCallCount, 0);
    }

    /* Calling update() rebuilds internal masks of nodes enabled for events, so
       the state passed to doUpdate() is the same as above. It doesn't call
       layouters. */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
        UnsignedInt expectedDataIds[]{
            dataHandleId(dataNode),
            dataHandleId(dataNested1),
            dataHandleId(dataAnother1),
        };
        Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
            {{3.0f, 1.0f}, {2.0f, 4.0f}}, /* node */
            {{5.0f, 0.0f}, {1.0f, 2.0f}}, /* another1 */
            {{5.0f, 2.0f}, {1.0f, 2.0f}}, /* another2 */
            {{4.0f, 4.0f}, {1.0f, 2.0f}}, /* nested1 */
            {},
            {},                           /* invisible */
            {},                           /* topLevelChildOfInvisible */
            {},                           /* notInOrder */
        };
        bool expectedNodesEnabled[]{
            /* Again all enabled except invisible, topLevelChildOfInvisible,
               notInOrder and nested2 */
            true, true, true, true, false, false, false, false
        };
        Containers::Pair<UnsignedInt, UnsignedInt> expectedClipRectIdsDataCounts[]{
            {0, 2}, /* node and remaining child */
            {1, 1}  /* another1, unclipped */
            /* another2 has no data */
        };
        Containers::Pair<Vector2, Vector2> expectedClipRectOffsetsSizes[]{
            {{3.0f, 1.0f}, {2.0f, 4.0f}},
            {{}, {}},
            {{}, {}}
        };
        Containers::Pair<Vector2, Vector2> expectedCompositeRectOffsetsSizes[]{
            {{3.0f, 1.0f}, {2.0f, 4.0f}}, /* matching node */
            {{4.0f, 4.0f}, {1.0f, 1.0f}}, /* matching nested1, clipped */
            {{5.0f, 0.0f}, {1.0f, 2.0f}}, /* matching another1 */
        };
        /* Toggling a node NoEvents flag back causes the same as Disabled due
           to the coarseness of UserInterfaceState bits */
        /** @todo separate those from disabled, separate from node order */
        ui.layer<Layer>(layer).expectedState = LayerState::NeedsNodeOrderUpdate|LayerState::NeedsNodeEnabledUpdate;
        ui.layer<Layer>(layer).expectedDataIds = expectedDataIds;
        ui.layer<Layer>(layer).expectedNodeOffsetsSizes = expectedNodeOffsetsSizes;
        ui.layer<Layer>(layer).expectedNodesEnabled = expectedNodesEnabled;
        ui.layer<Layer>(layer).expectedClipRectIdsDataCounts = expectedClipRectIdsDataCounts;
        ui.layer<Layer>(layer).expectedClipRectOffsetsSizes = expectedClipRectOffsetsSizes;
        if(data.compositingLayer)
            ui.layer<Layer>(layer).expectedCompositeRectOffsetsSizes = expectedCompositeRectOffsetsSizes;

        ui.update();
    }
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.renderer<Renderer>().setupFramebufferCallCount, 2);
    if(data.layouters) {
        CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 0);
        CORRADE_COMPARE_AS(layouterUpdateCalls, Containers::arrayView({
            /* update() again gets called twice on layouter 2 */
            layouterHandleId(layouter2), layouterHandleId(layouter1), layouterHandleId(layouter2)
        }), TestSuite::Compare::Container);
    }
    CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
    CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 12);
    if(data.nodeAttachmentAnimators)
        CORRADE_COMPARE(ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).cleanCallCount, 0);
    if(data.dataAttachmentAnimators)
        CORRADE_COMPARE(ui.animator<AttachmentAnimator>(dataAttachmentAnimator).cleanCallCount, 0);

    /* Resetting a NoEvents flag that's not there should be a no-op */
    ui.clearNodeFlags(node, NodeFlag::NoEvents);
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

    /* Not testing NodeFlag::Focusable because it sets NeedsNodeEnabledUpdate
       but doesn't actually cause anything to change in the internal state,
       apart from clearing the currentFocusedNode(). And that's tested well
       enough elsewhere. */
    /** @todo test once it causes some focusable node trees or some such to be
        rebuilt */

    /* Setting a Clip flag that's already there should be a no-op,
       independently of what other flags get added */
    ui.addNodeFlags(node, NodeFlag(0x80)|NodeFlag::Clip);
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
        CORRADE_COMPARE(ui.renderer<Renderer>().setupFramebufferCallCount, 2);
        if(data.layouters) {
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 0);
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 0);
        }
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 12);
        if(data.nodeAttachmentAnimators)
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).cleanCallCount, 0);
        if(data.dataAttachmentAnimators)
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(dataAttachmentAnimator).cleanCallCount, 0);
    }

    /* Calling update() uploads the full data including the no-longer-clipped
       nodes. It doesn't call layouters. */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
        UnsignedInt expectedDataIds[]{
            dataHandleId(dataNode),
            dataHandleId(dataNested1),
            dataHandleId(dataNested2),
            dataHandleId(dataAnother1),
        };
        Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
            {{3.0f, 1.0f}, {2.0f, 4.0f}}, /* node */
            {{5.0f, 0.0f}, {1.0f, 2.0f}}, /* another1 */
            {{5.0f, 2.0f}, {1.0f, 2.0f}}, /* another2 */
            {{4.0f, 4.0f}, {1.0f, 2.0f}}, /* nested1 */
            {{5.0f, 3.0f}, {1.0f, 2.0f}}, /* nested2 */
            {},                           /* invisible */
            {},                           /* topLevelChildOfInvisible */
            {},                           /* notInOrder */
        };
        bool expectedNodesEnabled[]{
            /* All enabled except invisible, topLevelChildOfInvisible and
               notInOrder */
            true, true, true, true, true, false, false, false
        };
        Containers::Pair<UnsignedInt, UnsignedInt> expectedClipRectIdsDataCounts[]{
            {0, 3}, /* node and all children */
            {1, 1}  /* another1, unclipped */
            /* another2 has no data */
        };
        Containers::Pair<Vector2, Vector2> expectedClipRectOffsetsSizes[]{
            {{}, {}},
            {{}, {}},
            {{}, {}}
        };
        Containers::Pair<Vector2, Vector2> expectedCompositeRectOffsetsSizes[]{
            {{3.0f, 1.0f}, {2.0f, 4.0f}}, /* matching node */
            {{4.0f, 4.0f}, {1.0f, 2.0f}}, /* matching nested1 */
            {{5.0f, 3.0f}, {1.0f, 2.0f}}, /* matching nested2 */
            {{5.0f, 0.0f}, {1.0f, 2.0f}}, /* matching another1 */
        };
        /* Toggling a node Clip flag causes the set of visible nodes to be
           changed, which also affects the set of enabled nodes */
        ui.layer<Layer>(layer).expectedState = LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsNodeOrderUpdate;
        ui.layer<Layer>(layer).expectedDataIds = expectedDataIds;
        ui.layer<Layer>(layer).expectedNodeOffsetsSizes = expectedNodeOffsetsSizes;
        ui.layer<Layer>(layer).expectedNodesEnabled = expectedNodesEnabled;
        ui.layer<Layer>(layer).expectedClipRectIdsDataCounts = expectedClipRectIdsDataCounts;
        ui.layer<Layer>(layer).expectedClipRectOffsetsSizes = expectedClipRectOffsetsSizes;
        if(data.compositingLayer)
            ui.layer<Layer>(layer).expectedCompositeRectOffsetsSizes = expectedCompositeRectOffsetsSizes;
        ui.update();
    }
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.renderer<Renderer>().setupFramebufferCallCount, 2);
    if(data.layouters) {
        CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 0);
        CORRADE_COMPARE_AS(layouterUpdateCalls, Containers::arrayView({
            layouterHandleId(layouter2), layouterHandleId(layouter1), layouterHandleId(layouter2)
        }), TestSuite::Compare::Container);
    }
    CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
    CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 13);
    if(data.nodeAttachmentAnimators)
        CORRADE_COMPARE(ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).cleanCallCount, 0);
    if(data.dataAttachmentAnimators)
        CORRADE_COMPARE(ui.animator<AttachmentAnimator>(dataAttachmentAnimator).cleanCallCount, 0);

    /* Resetting a Clip flag that's not there should be a no-op, independently
       of what other flags get cleared */
    ui.clearNodeFlags(node, NodeFlag(0xc0)|NodeFlag::Clip);
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
        CORRADE_COMPARE(ui.renderer<Renderer>().setupFramebufferCallCount, 2);
        if(data.layouters) {
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 0);
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 0);
        }
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 13);
        if(data.nodeAttachmentAnimators)
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).cleanCallCount, 0);
        if(data.dataAttachmentAnimators)
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(dataAttachmentAnimator).cleanCallCount, 0);
    }

    /* Calling update() reuploads the previous data again and resets the state
       flag. Doesn't call layouters. */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
        UnsignedInt expectedDataIds[]{
            dataHandleId(dataNode),
            dataHandleId(dataNested1),
            dataHandleId(dataAnother1),
        };
        Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
            {{3.0f, 1.0f}, {2.0f, 4.0f}}, /* node */
            {{5.0f, 0.0f}, {1.0f, 2.0f}}, /* another1 */
            {{5.0f, 2.0f}, {1.0f, 2.0f}}, /* another2 */
            {{4.0f, 4.0f}, {1.0f, 2.0f}}, /* nested1 */
            {},
            {},                           /* invisible */
            {},                           /* topLevelChildOfInvisible */
            {},                           /* notInOrder */
        };
        bool expectedNodesEnabled[]{
            /* All enabled except invisible, topLevelChildOfInvisible,
               notInOrder & nested2 now again */
            true, true, true, true, false, false, false, false
        };
        Containers::Pair<UnsignedInt, UnsignedInt> expectedClipRectIdsDataCounts[]{
            {0, 2}, /* node and remaining child */
            {1, 1}  /* another1, unclipped */
            /* another2 has no data */
        };
        Containers::Pair<Vector2, Vector2> expectedClipRectOffsetsSizes[]{
            {{3.0f, 1.0f}, {2.0f, 4.0f}},
            {{}, {}},
            {{}, {}}
        };
        Containers::Pair<Vector2, Vector2> expectedCompositeRectOffsetsSizes[]{
            {{3.0f, 1.0f}, {2.0f, 4.0f}}, /* matching node */
            {{4.0f, 4.0f}, {1.0f, 1.0f}}, /* matching nested1, clipped */
            {{5.0f, 0.0f}, {1.0f, 2.0f}}, /* matching another1 */
        };
        /* Toggling a node Clip flag again causes the set of visible nodes to
           be changed, which also affects the set of enabled nodes */
        ui.layer<Layer>(layer).expectedState = LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsNodeOrderUpdate;
        ui.layer<Layer>(layer).expectedDataIds = expectedDataIds;
        ui.layer<Layer>(layer).expectedNodeOffsetsSizes = expectedNodeOffsetsSizes;
        ui.layer<Layer>(layer).expectedNodesEnabled = expectedNodesEnabled;
        ui.layer<Layer>(layer).expectedClipRectIdsDataCounts = expectedClipRectIdsDataCounts;
        ui.layer<Layer>(layer).expectedClipRectOffsetsSizes = expectedClipRectOffsetsSizes;
        if(data.compositingLayer)
            ui.layer<Layer>(layer).expectedCompositeRectOffsetsSizes = expectedCompositeRectOffsetsSizes;
        ui.update();
    }
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.renderer<Renderer>().setupFramebufferCallCount, 2);
    if(data.layouters) {
        CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 0);
        CORRADE_COMPARE_AS(layouterUpdateCalls, Containers::arrayView({
            layouterHandleId(layouter2), layouterHandleId(layouter1), layouterHandleId(layouter2)
        }), TestSuite::Compare::Container);
    }
    CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
    CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 14);
    if(data.nodeAttachmentAnimators)
        CORRADE_COMPARE(ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).cleanCallCount, 0);
    if(data.dataAttachmentAnimators)
        CORRADE_COMPARE(ui.animator<AttachmentAnimator>(dataAttachmentAnimator).cleanCallCount, 0);

    /* Calling clearNodeOrder() sets a state flag */
    ui.clearNodeOrder(another1);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);

    /* Calling clean() should be a no-op */
    if(data.clean && data.noOp) {
        {
            CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);
        CORRADE_COMPARE(ui.renderer<Renderer>().setupFramebufferCallCount, 2);
        if(data.layouters) {
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 0);
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 0);
        }
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 14);
        if(data.nodeAttachmentAnimators)
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).cleanCallCount, 0);
        if(data.dataAttachmentAnimators)
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(dataAttachmentAnimator).cleanCallCount, 0);
    }

    /* Calling update() calls the one remaining layouter, uploads data in
       new order and resets the flag */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));

        bool expectedLayoutIdsToUpdate1[]{
            /* layout2Node and layout2Nested2 */
            false, true, true, false, false
        };
        UnsignedInt expectedTopLevelLayoutIds1[]{
            layoutHandleId(layout2Node),
        };
        Containers::Pair<Vector2, Vector2> expectedLayoutNodeOffsetsSizes1[]{
            {{3.0f, 3.0f}, {2.0f, 2.0f}}, /* node, visible again */
            {},
            {},
            {{1.0f, 3.0f}, {1.0f, 2.0f}}, /* nested1 */
            {{2.0f, 4.0f}, {1.0f, 1.0f}}, /* nested2 */
            {{9.0f, 9.0f}, {9.0f, 9.0f}}, /* invisible */
            {{0.0f, 0.0f}, {9.0f, 9.0f}}, /* topLevelChildOfInvisible */
            {{8.0f, 8.0f}, {8.0f, 8.0f}}, /* notInOrder */
        };
        if(data.layouters) {
            layouterUpdateCalls = {};
            ui.layouter<Layouter2>(layouter2).expectedLayoutIdsToUpdate[0] = expectedLayoutIdsToUpdate1;
            ui.layouter<Layouter1>(layouter1).expectedLayoutIdsToUpdate = {};
            ui.layouter<Layouter2>(layouter2).expectedLayoutIdsToUpdate[1] = {};
            ui.layouter<Layouter2>(layouter2).expectedTopLevelLayoutIds[0] = expectedTopLevelLayoutIds1;
            ui.layouter<Layouter1>(layouter1).expectedTopLevelLayoutIds = {};
            ui.layouter<Layouter2>(layouter2).expectedTopLevelLayoutIds[1] = {};
            ui.layouter<Layouter2>(layouter2).expectedNodeOffsetsSizes[0] = expectedLayoutNodeOffsetsSizes1;
            ui.layouter<Layouter1>(layouter1).expectedNodeOffsetsSizes = {};
            ui.layouter<Layouter2>(layouter2).expectedNodeOffsetsSizes[1] = {};
            ui.layouter<Layouter2>(layouter2).updateCallId = 0;
        }

        UnsignedInt expectedDataIds[]{
            dataHandleId(dataNode),
            dataHandleId(dataNested1),
        };
        Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
            {{3.0f, 1.0f}, {2.0f, 4.0f}}, /* node */
            {},
            {},
            {{4.0f, 4.0f}, {1.0f, 2.0f}}, /* nested1 */
            {},
            {},                           /* invisible */
            {},                           /* topLevelChildOfInvisible */
            {},                           /* notInOrder */
        };
        bool expectedNodesEnabled[]{
            /* Only node and nested1 left visible & enabled */
            true, false, false, true, false, false, false, false
        };
        Containers::Pair<UnsignedInt, UnsignedInt> expectedClipRectIdsDataCounts[]{
            {0, 2}, /* node and remaining child */
        };
        Containers::Pair<Vector2, Vector2> expectedClipRectOffsetsSizes[]{
            {{3.0f, 1.0f}, {2.0f, 4.0f}},
        };
        Containers::Pair<Vector2, Vector2> expectedCompositeRectOffsetsSizes[]{
            {{3.0f, 1.0f}, {2.0f, 4.0f}}, /* matching node */
            {{4.0f, 4.0f}, {1.0f, 1.0f}}, /* matching nested1, clipped */
        };
        /* Removing a node from the top-level order means just the set of drawn
           nodes (and thus also enabled state) being updated, but offset/size
           update is also triggered due to the coarseness of the
           UserInterfaceState bits */
        /** @todo separate those */
        ui.layer<Layer>(layer).expectedState = LayerState::NeedsNodeOrderUpdate|LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsNodeOffsetSizeUpdate|(data.compositingLayer ? LayerState::NeedsCompositeOffsetSizeUpdate : LayerStates{});
        ui.layer<Layer>(layer).expectedDataIds = expectedDataIds;
        ui.layer<Layer>(layer).expectedNodeOffsetsSizes = expectedNodeOffsetsSizes;
        ui.layer<Layer>(layer).expectedNodesEnabled = expectedNodesEnabled;
        ui.layer<Layer>(layer).expectedClipRectIdsDataCounts = expectedClipRectIdsDataCounts;
        ui.layer<Layer>(layer).expectedClipRectOffsetsSizes = expectedClipRectOffsetsSizes;
        if(data.compositingLayer)
            ui.layer<Layer>(layer).expectedCompositeRectOffsetsSizes = expectedCompositeRectOffsetsSizes;

        ui.update();
    }
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.renderer<Renderer>().setupFramebufferCallCount, 2);
    if(data.layouters) {
        CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 0);
        CORRADE_COMPARE_AS(layouterUpdateCalls, Containers::arrayView({
            /* Now only layouter2 gets called, once */
            layouterHandleId(layouter2)
        }), TestSuite::Compare::Container);
    }
    CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
    CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 15);
    if(data.nodeAttachmentAnimators)
        CORRADE_COMPARE(ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).cleanCallCount, 0);
    if(data.dataAttachmentAnimators)
        CORRADE_COMPARE(ui.animator<AttachmentAnimator>(dataAttachmentAnimator).cleanCallCount, 0);

    /* Calling clearNodeOrder() on a node that isn't in the order is a no-op */
    ui.clearNodeOrder(another1);
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

    /* Calling setNodeOrder() sets a state flag again */
    ui.setNodeOrder(another1, node);
    /** @todo make this a no-op if the order is already that way (and test) */
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);

    /* Calling clean() should be a no-op */
    if(data.clean && data.noOp) {
        {
            CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);
        CORRADE_COMPARE(ui.renderer<Renderer>().setupFramebufferCallCount, 2);
        if(data.layouters) {
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 0);
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 0);
        }
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 15);
        if(data.nodeAttachmentAnimators)
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).cleanCallCount, 0);
        if(data.dataAttachmentAnimators)
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(dataAttachmentAnimator).cleanCallCount, 0);
    }

    /* Calling update() calls the layouters the same way as before the another1
       node was removed from the order, uploads data in new order and resets
       the flag */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));

        bool expectedLayoutIdsToUpdate1[]{
            /* layout2Node and layout2Nested2 */
            false, true, true, false, false
        };
        bool expectedLayoutIdsToUpdate2[]{
            /* layout1Another1 */
            false, true, false
        };
        bool expectedLayoutIdsToUpdate3[]{
            /* layout2Another1 and layout2Another2 */
            false, false, false, true, true
        };
        UnsignedInt expectedTopLevelLayoutIds1[]{
            layoutHandleId(layout2Node),
        };
        UnsignedInt expectedTopLevelLayoutIds2[]{
            layoutHandleId(layout1Another1),
        };
        UnsignedInt expectedTopLevelLayoutIds3[]{
            layoutHandleId(layout2Another1),
            /* layout2Another2 is top-level but not root, so not included
               here */
        };
        Containers::Pair<Vector2, Vector2> expectedLayoutNodeOffsetsSizes1[]{
            {{3.0f, 3.0f}, {2.0f, 2.0f}}, /* node, visible again */
            {{3.0f, 2.0f}, {0.5f, 1.0f}}, /* another1 */
            {{0.0f, 4.0f}, {1.0f, 1.0f}}, /* another2 */
            {{1.0f, 3.0f}, {1.0f, 2.0f}}, /* nested1 */
            {{2.0f, 4.0f}, {1.0f, 1.0f}}, /* nested2 */
            {{9.0f, 9.0f}, {9.0f, 9.0f}}, /* invisible */
            {{0.0f, 0.0f}, {9.0f, 9.0f}}, /* topLevelChildOfInvisible */
            {{8.0f, 8.0f}, {8.0f, 8.0f}}, /* notInOrder */
        };
        Containers::Pair<Vector2, Vector2> expectedLayoutNodeOffsetsSizes2[]{
            {{3.0f, 1.0f}, {2.0f, 4.0f}}, /* changed by layouter 2 */
            {{3.0f, 2.0f}, {0.5f, 1.0f}},
            {{0.0f, 4.0f}, {1.0f, 1.0f}},
            {{1.0f, 3.0f}, {1.0f, 2.0f}},
            {{2.0f, 2.0f}, {1.0f, 2.0f}},
            {{9.0f, 9.0f}, {9.0f, 9.0f}},
            {{0.0f, 0.0f}, {9.0f, 9.0f}},
            {{8.0f, 8.0f}, {8.0f, 8.0f}}
        };
        Containers::Pair<Vector2, Vector2> expectedLayoutNodeOffsetsSizes3[]{
            {{3.0f, 1.0f}, {2.0f, 4.0f}},
            {{5.0f, 2.0f}, {1.0f, 1.0f}}, /* changed by layouter 1 */
            {{0.0f, 4.0f}, {1.0f, 1.0f}},
            {{1.0f, 3.0f}, {1.0f, 2.0f}},
            {{2.0f, 2.0f}, {1.0f, 2.0f}},
            {{9.0f, 9.0f}, {9.0f, 9.0f}},
            {{0.0f, 0.0f}, {9.0f, 9.0f}},
            {{8.0f, 8.0f}, {8.0f, 8.0f}}
        };
        if(data.layouters) {
            layouterUpdateCalls = {};
            ui.layouter<Layouter2>(layouter2).expectedLayoutIdsToUpdate[0] = expectedLayoutIdsToUpdate1;
            ui.layouter<Layouter1>(layouter1).expectedLayoutIdsToUpdate = expectedLayoutIdsToUpdate2;
            ui.layouter<Layouter2>(layouter2).expectedLayoutIdsToUpdate[1] = expectedLayoutIdsToUpdate3;
            ui.layouter<Layouter2>(layouter2).expectedTopLevelLayoutIds[0] = expectedTopLevelLayoutIds1;
            ui.layouter<Layouter1>(layouter1).expectedTopLevelLayoutIds = expectedTopLevelLayoutIds2;
            ui.layouter<Layouter2>(layouter2).expectedTopLevelLayoutIds[1] = expectedTopLevelLayoutIds3;
            ui.layouter<Layouter2>(layouter2).expectedNodeOffsetsSizes[0] = expectedLayoutNodeOffsetsSizes1;
            ui.layouter<Layouter1>(layouter1).expectedNodeOffsetsSizes = expectedLayoutNodeOffsetsSizes2;
            ui.layouter<Layouter2>(layouter2).expectedNodeOffsetsSizes[1] = expectedLayoutNodeOffsetsSizes3;
            ui.layouter<Layouter2>(layouter2).updateCallId = 0;
        }

        UnsignedInt expectedDataIds[]{
            dataHandleId(dataAnother1),
            dataHandleId(dataNode),
            dataHandleId(dataNested1),
        };
        Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
            {{3.0f, 1.0f}, {2.0f, 4.0f}}, /* node */
            {{5.0f, 0.0f}, {1.0f, 2.0f}}, /* another1 */
            {{5.0f, 2.0f}, {1.0f, 2.0f}}, /* another2 */
            {{4.0f, 4.0f}, {1.0f, 2.0f}}, /* nested1 */
            {},
            {},                           /* invisible */
            {},                           /* topLevelChildOfInvisible */
            {},                           /* notInOrder */
        };
        bool expectedNodesEnabled[]{
            /* Again all enabled except invisible, topLevelChildOfInvisible,
               notInOrder and nested2 */
            true, true, true, true, false, false, false, false
        };
        Containers::Pair<UnsignedInt, UnsignedInt> expectedClipRectIdsDataCounts[]{
            {0, 1}, /* another1, unclipped */
            /* another2 has no data */
            {2, 2}  /* node and remaining child */
        };
        Containers::Pair<Vector2, Vector2> expectedClipRectOffsetsSizes[]{
            {{}, {}},
            {{}, {}},
            {{3.0f, 1.0f}, {2.0f, 4.0f}},
        };
        Containers::Pair<Vector2, Vector2> expectedCompositeRectOffsetsSizes[]{
            {{5.0f, 0.0f}, {1.0f, 2.0f}}, /* matching another1 */
            {{3.0f, 1.0f}, {2.0f, 4.0f}}, /* matching node */
            {{4.0f, 4.0f}, {1.0f, 1.0f}}, /* matching nested1, clipped */
        };
        /* Putting a node back to the top-level order triggers offset/size
           update in addition to just the order + enabled */
        ui.layer<Layer>(layer).expectedState = LayerState::NeedsNodeOrderUpdate|LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsNodeOffsetSizeUpdate|(data.compositingLayer ? LayerState::NeedsCompositeOffsetSizeUpdate : LayerStates{});
        ui.layer<Layer>(layer).expectedDataIds = expectedDataIds;
        ui.layer<Layer>(layer).expectedNodeOffsetsSizes = expectedNodeOffsetsSizes;
        ui.layer<Layer>(layer).expectedNodesEnabled = expectedNodesEnabled;
        ui.layer<Layer>(layer).expectedClipRectIdsDataCounts = expectedClipRectIdsDataCounts;
        ui.layer<Layer>(layer).expectedClipRectOffsetsSizes = expectedClipRectOffsetsSizes;
        if(data.compositingLayer)
            ui.layer<Layer>(layer).expectedCompositeRectOffsetsSizes = expectedCompositeRectOffsetsSizes;

        ui.update();
    }
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.renderer<Renderer>().setupFramebufferCallCount, 2);
    if(data.layouters) {
        CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 0);
        CORRADE_COMPARE_AS(layouterUpdateCalls, Containers::arrayView({
            /* layouter2 getting called twice again */
            layouterHandleId(layouter2), layouterHandleId(layouter1), layouterHandleId(layouter2),
        }), TestSuite::Compare::Container);
    }
    CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
    CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 16);
    if(data.nodeAttachmentAnimators)
        CORRADE_COMPARE(ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).cleanCallCount, 0);
    if(data.dataAttachmentAnimators)
        CORRADE_COMPARE(ui.animator<AttachmentAnimator>(dataAttachmentAnimator).cleanCallCount, 0);

    /* Calling flattenNodeOrder() sets a state flag */
    ui.flattenNodeOrder(another2);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);

    /* Calling clean() should be a no-op */
    if(data.clean && data.noOp) {
        {
            CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);
        CORRADE_COMPARE(ui.renderer<Renderer>().setupFramebufferCallCount, 2);
        if(data.layouters) {
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 0);
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 0);
        }
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 16);
        if(data.nodeAttachmentAnimators)
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).cleanCallCount, 0);
        if(data.dataAttachmentAnimators)
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(dataAttachmentAnimator).cleanCallCount, 0);
    }

    /* Calling update() calls the layouters the same way (flattening the order
       has only an effect on the draw order, not on the layouters or on actual
       node offsets and sizes) */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));

        bool expectedLayoutIdsToUpdate1[]{
            /* layout2Node and layout2Nested2 */
            false, true, true, false, false
        };
        bool expectedLayoutIdsToUpdate2[]{
            /* layout1Another1 */
            false, true, false
        };
        bool expectedLayoutIdsToUpdate3[]{
            /* layout2Another1 and layout2Another2 */
            false, false, false, true, true
        };
        UnsignedInt expectedTopLevelLayoutIds1[]{
            layoutHandleId(layout2Node),
        };
        UnsignedInt expectedTopLevelLayoutIds2[]{
            layoutHandleId(layout1Another1),
        };
        UnsignedInt expectedTopLevelLayoutIds3[]{
            layoutHandleId(layout2Another1),
            /* layout2Another2 is top-level but not root, so not included
               here */
        };
        Containers::Pair<Vector2, Vector2> expectedLayoutNodeOffsetsSizes1[]{
            {{3.0f, 3.0f}, {2.0f, 2.0f}}, /* node, visible again */
            {{3.0f, 2.0f}, {0.5f, 1.0f}}, /* another1 */
            {{0.0f, 4.0f}, {1.0f, 1.0f}}, /* another2 */
            {{1.0f, 3.0f}, {1.0f, 2.0f}}, /* nested1 */
            {{2.0f, 4.0f}, {1.0f, 1.0f}}, /* nested2 */
            {{9.0f, 9.0f}, {9.0f, 9.0f}}, /* invisible */
            {{0.0f, 0.0f}, {9.0f, 9.0f}}, /* topLevelChildOfInvisible */
            {{8.0f, 8.0f}, {8.0f, 8.0f}}, /* notInOrder */
        };
        Containers::Pair<Vector2, Vector2> expectedLayoutNodeOffsetsSizes2[]{
            {{3.0f, 1.0f}, {2.0f, 4.0f}}, /* changed by layouter 2 */
            {{3.0f, 2.0f}, {0.5f, 1.0f}},
            {{0.0f, 4.0f}, {1.0f, 1.0f}},
            {{1.0f, 3.0f}, {1.0f, 2.0f}},
            {{2.0f, 2.0f}, {1.0f, 2.0f}},
            {{9.0f, 9.0f}, {9.0f, 9.0f}},
            {{0.0f, 0.0f}, {9.0f, 9.0f}},
            {{8.0f, 8.0f}, {8.0f, 8.0f}}
        };
        Containers::Pair<Vector2, Vector2> expectedLayoutNodeOffsetsSizes3[]{
            {{3.0f, 1.0f}, {2.0f, 4.0f}},
            {{5.0f, 2.0f}, {1.0f, 1.0f}}, /* changed by layouter 1 */
            {{0.0f, 4.0f}, {1.0f, 1.0f}},
            {{1.0f, 3.0f}, {1.0f, 2.0f}},
            {{2.0f, 2.0f}, {1.0f, 2.0f}},
            {{9.0f, 9.0f}, {9.0f, 9.0f}},
            {{0.0f, 0.0f}, {9.0f, 9.0f}},
            {{8.0f, 8.0f}, {8.0f, 8.0f}}
        };
        if(data.layouters) {
            layouterUpdateCalls = {};
            ui.layouter<Layouter2>(layouter2).expectedLayoutIdsToUpdate[0] = expectedLayoutIdsToUpdate1;
            ui.layouter<Layouter1>(layouter1).expectedLayoutIdsToUpdate = expectedLayoutIdsToUpdate2;
            ui.layouter<Layouter2>(layouter2).expectedLayoutIdsToUpdate[1] = expectedLayoutIdsToUpdate3;
            ui.layouter<Layouter2>(layouter2).expectedTopLevelLayoutIds[0] = expectedTopLevelLayoutIds1;
            ui.layouter<Layouter1>(layouter1).expectedTopLevelLayoutIds = expectedTopLevelLayoutIds2;
            ui.layouter<Layouter2>(layouter2).expectedTopLevelLayoutIds[1] = expectedTopLevelLayoutIds3;
            ui.layouter<Layouter2>(layouter2).expectedNodeOffsetsSizes[0] = expectedLayoutNodeOffsetsSizes1;
            ui.layouter<Layouter1>(layouter1).expectedNodeOffsetsSizes = expectedLayoutNodeOffsetsSizes2;
            ui.layouter<Layouter2>(layouter2).expectedNodeOffsetsSizes[1] = expectedLayoutNodeOffsetsSizes3;
            ui.layouter<Layouter2>(layouter2).updateCallId = 0;
        }

        UnsignedInt expectedDataIds[]{
            dataHandleId(dataAnother1),
            dataHandleId(dataNode),
            dataHandleId(dataNested1),
        };
        Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
            {{3.0f, 1.0f}, {2.0f, 4.0f}}, /* node */
            {{5.0f, 0.0f}, {1.0f, 2.0f}}, /* another1 */
            {{5.0f, 2.0f}, {1.0f, 2.0f}}, /* another2 */
            {{4.0f, 4.0f}, {1.0f, 2.0f}}, /* nested1 */
            {},
            {},                           /* invisible */
            {},                           /* topLevelChildOfInvisible */
            {},                           /* notInOrder */
        };
        bool expectedNodesEnabled[]{
            /* Again all enabled except invisible, topLevelChildOfInvisible,
               notInOrder and nested2 */
            true, true, true, true, false, false, false, false
        };
        Containers::Pair<UnsignedInt, UnsignedInt> expectedClipRectIdsDataCounts[]{
            {0, 1}, /* another1, unclipped */
            {1, 2}, /* node and remaining child */
        };
        Containers::Pair<Vector2, Vector2> expectedClipRectOffsetsSizes[]{
            {{}, {}},
            {{3.0f, 1.0f}, {2.0f, 4.0f}},
        };
        Containers::Pair<Vector2, Vector2> expectedCompositeRectOffsetsSizes[]{
            {{5.0f, 0.0f}, {1.0f, 2.0f}}, /* matching another1 */
            {{3.0f, 1.0f}, {2.0f, 4.0f}}, /* matching node */
            {{4.0f, 4.0f}, {1.0f, 1.0f}}, /* matching nested1, clipped */
        };
        /* Removing a node from the top-level order means just the set of drawn
           nodes (and thus also enabled state) being updated, but offset/size
           update is also triggered due to the coarseness of the
           UserInterfaceState bits */
        /** @todo separate those */
        ui.layer<Layer>(layer).expectedState = LayerState::NeedsNodeOrderUpdate|LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsNodeOffsetSizeUpdate|(data.compositingLayer ? LayerState::NeedsCompositeOffsetSizeUpdate : LayerStates{});
        ui.layer<Layer>(layer).expectedDataIds = expectedDataIds;
        ui.layer<Layer>(layer).expectedNodeOffsetsSizes = expectedNodeOffsetsSizes;
        ui.layer<Layer>(layer).expectedNodesEnabled = expectedNodesEnabled;
        ui.layer<Layer>(layer).expectedClipRectIdsDataCounts = expectedClipRectIdsDataCounts;
        ui.layer<Layer>(layer).expectedClipRectOffsetsSizes = expectedClipRectOffsetsSizes;
        if(data.compositingLayer)
            ui.layer<Layer>(layer).expectedCompositeRectOffsetsSizes = expectedCompositeRectOffsetsSizes;

        ui.update();
    }
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.renderer<Renderer>().setupFramebufferCallCount, 2);
    if(data.layouters) {
        CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 0);
        CORRADE_COMPARE_AS(layouterUpdateCalls, Containers::arrayView({
            /* layouter2 getting called twice again */
            layouterHandleId(layouter2), layouterHandleId(layouter1), layouterHandleId(layouter2),
        }), TestSuite::Compare::Container);
    }
    CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
    CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 17);
    if(data.nodeAttachmentAnimators)
        CORRADE_COMPARE(ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).cleanCallCount, 0);
    if(data.dataAttachmentAnimators)
        CORRADE_COMPARE(ui.animator<AttachmentAnimator>(dataAttachmentAnimator).cleanCallCount, 0);

    /* Calling flattenNodeOrder() on a node that isn't in top-level (anymore)
       is a no-op */
    ui.flattenNodeOrder(another2);
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

    if(data.layouters) {
        /* Removing a layout marks the layouter with NeedsAssignmentUpdate,
           which is then propagated to the UI-wide state */
        ui.layouter<Layouter>(layouter1).remove(layout1Another1);
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsLayoutAssignmentUpdate);
        CORRADE_COMPARE(ui.layouter(layouter1).usedCount(), 2);
        CORRADE_COMPARE(ui.layouter(layouter2).usedCount(), 5);

        /* Calling update() then calls the remaining layouter, uploads
           remaining data and resets the remaining state flag */
        {
            CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));

            bool expectedLayoutIdsToUpdate1[]{
                /* layout2Node, layout2Nested2, layout2Another1 and
                   layout2Another2 */
                false, true, true, true, true
            };
            /* Layouter1 is called with nothing just to reset the
               NeedsLayoutAssignmentUpdate flag. */
            /** @todo which is rather ugly, better idea? */
            bool expectedLayoutIdsToUpdate2[3]{};
            UnsignedInt expectedTopLevelLayoutIds1[]{
                layoutHandleId(layout2Another1),
                layoutHandleId(layout2Node),
                /* layout2Another2 is top-level but not root, so not included
                   here */
            };
            Containers::Pair<Vector2, Vector2> expectedLayoutNodeOffsetsSizes1[]{
                {{3.0f, 3.0f}, {2.0f, 2.0f}}, /* node, visible again */
                {{3.0f, 2.0f}, {0.5f, 1.0f}}, /* another1 */
                {{0.0f, 4.0f}, {1.0f, 1.0f}}, /* another2 */
                {{1.0f, 3.0f}, {1.0f, 2.0f}}, /* nested1 */
                {{2.0f, 4.0f}, {1.0f, 1.0f}}, /* nested2 */
                {{9.0f, 9.0f}, {9.0f, 9.0f}}, /* invisible */
                {{0.0f, 0.0f}, {9.0f, 9.0f}}, /* topLevelChildOfInvisible */
                {{8.0f, 8.0f}, {8.0f, 8.0f}}, /* notInOrder */
            };
            Containers::Pair<Vector2, Vector2> expectedLayoutNodeOffsetsSizes2[]{
                {{3.0f, 1.0f}, {2.0f, 4.0f}}, /* changed by layouter 2 */
                {{3.0f, 0.0f}, {0.5f, 2.0f}}, /* changed by layouter 2 */
                {{0.0f, 2.0f}, {1.0f, 2.0f}}, /* changed by layouter 2 */
                {{1.0f, 3.0f}, {1.0f, 2.0f}},
                {{2.0f, 2.0f}, {1.0f, 2.0f}},
                {{9.0f, 9.0f}, {9.0f, 9.0f}},
                {{0.0f, 0.0f}, {9.0f, 9.0f}},
                {{8.0f, 8.0f}, {8.0f, 8.0f}}
            };
            layouterUpdateCalls = {};
            ui.layouter<Layouter2>(layouter2).expectedLayoutIdsToUpdate[0] = expectedLayoutIdsToUpdate1;
            ui.layouter<Layouter1>(layouter1).expectedLayoutIdsToUpdate = expectedLayoutIdsToUpdate2;
            ui.layouter<Layouter2>(layouter2).expectedLayoutIdsToUpdate[1] = {};
            ui.layouter<Layouter2>(layouter2).expectedTopLevelLayoutIds[0] = expectedTopLevelLayoutIds1;
            ui.layouter<Layouter1>(layouter1).expectedTopLevelLayoutIds = {};
            ui.layouter<Layouter2>(layouter2).expectedTopLevelLayoutIds[1] = {};
            ui.layouter<Layouter2>(layouter2).expectedNodeOffsetsSizes[0] = expectedLayoutNodeOffsetsSizes1;
            ui.layouter<Layouter1>(layouter1).expectedNodeOffsetsSizes = expectedLayoutNodeOffsetsSizes2;
            ui.layouter<Layouter2>(layouter2).expectedNodeOffsetsSizes[1] = {};
            ui.layouter<Layouter2>(layouter2).updateCallId = 0;

            UnsignedInt expectedDataIds[]{
                dataHandleId(dataAnother1),
                dataHandleId(dataNode),
                dataHandleId(dataNested1),
            };
            Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
                {{3.0f, 1.0f}, {2.0f, 4.0f}}, /* node */
                /* With the layouter for another1 gone, the size is not updated
                   anymore */
                {{3.0f, 0.0f}, {0.5f, 2.0f}},
                /* Which also affects another2 that's a child */
                {{3.0f, 2.0f}, {1.0f, 2.0f}},
                {{4.0f, 4.0f}, {1.0f, 2.0f}}, /* nested1 */
                {},
                {},                           /* invisible */
                {},                           /* topLevelChildOfInvisible */
                {}                            /* notInOrder */
            };
            bool expectedNodesEnabled[]{
                /* All enabled except invisible, topLevelChildOfInvisible,
                   notInOrder and nested2 */
                true, true, true, true, false, false, false, false
            };
            Containers::Pair<UnsignedInt, UnsignedInt> expectedClipRectIdsDataCounts[]{
                {0, 1}, /* another1, unclipped */
                {1, 2}  /* node and remaining child */
            };
            Containers::Pair<Vector2, Vector2> expectedClipRectOffsetsSizes[]{
                {{}, {}},
                {{3.0f, 1.0f}, {2.0f, 4.0f}},
            };
            Containers::Pair<Vector2, Vector2> expectedCompositeRectOffsetsSizes[]{
                {{3.0f, 0.0f}, {0.5f, 2.0f}}, /* matching another1 */
                {{3.0f, 1.0f}, {2.0f, 4.0f}}, /* matching node */
                {{4.0f, 4.0f}, {1.0f, 1.0f}}, /* matching nested1, clipped */
            };
            /* Layout change means offset/size update, which may affect the set
               of visible nodes and thus also the enabled state */
            ui.layer<Layer>(layer).expectedState = LayerState::NeedsNodeOrderUpdate|LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsNodeOffsetSizeUpdate|(data.compositingLayer ? LayerState::NeedsCompositeOffsetSizeUpdate : LayerStates{});
            ui.layer<Layer>(layer).expectedDataIds = expectedDataIds;
            ui.layer<Layer>(layer).expectedNodeOffsetsSizes = expectedNodeOffsetsSizes;
            ui.layer<Layer>(layer).expectedNodesEnabled = expectedNodesEnabled;
            ui.layer<Layer>(layer).expectedClipRectIdsDataCounts = expectedClipRectIdsDataCounts;
            ui.layer<Layer>(layer).expectedClipRectOffsetsSizes = expectedClipRectOffsetsSizes;
            if(data.compositingLayer)
                ui.layer<Layer>(layer).expectedCompositeRectOffsetsSizes = expectedCompositeRectOffsetsSizes;

            ui.update();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
        CORRADE_COMPARE(ui.layouter(layouter1).usedCount(), 2);
        CORRADE_COMPARE(ui.layouter(layouter2).usedCount(), 5);
        CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 0);
        CORRADE_COMPARE_AS(layouterUpdateCalls, Containers::arrayView({
            /* There's nothing visible left in layouter1, so layouter2 is
               called with everything it has, and then layouter1 with nothing
               just to reset the NeedsLayoutAssignmentUpdate flag. */
            /** @todo which is rather ugly, better idea? */
            layouterHandleId(layouter2), layouterHandleId(layouter1),
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 18);
        if(data.nodeAttachmentAnimators)
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).cleanCallCount, 0);
        if(data.dataAttachmentAnimators)
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(dataAttachmentAnimator).cleanCallCount, 0);
    }

    /* Add one more layer with an attached animator to check data & layer
       removal behavior, should set no state flags again. Unlike the first one
       it doesn't enable compositing ever. */
    LayerHandle anotherLayer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<Layer>(anotherLayer, LayerFeatures{}));
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    /* GCC in Release mode complains that some of these may be used
       uninitialized, MSVC as well. They aren't. */
    AnimatorHandle anotherDataAttachmentAnimator{};
    if(data.dataAttachmentAnimators) {
        anotherDataAttachmentAnimator = ui.createAnimator();
        Containers::Pointer<AttachmentAnimator> instance{InPlaceInit, anotherDataAttachmentAnimator, AnimatorFeature::DataAttachment};
        instance->setLayer(ui.layer(anotherLayer));
        ui.setGenericAnimatorInstance(Utility::move(instance));
    }

    /* Removing data that's not attached to any node marks the layer with
       NeedsDataClean, which is then propagated to the UI-wide state */
    ui.layer<Layer>(layer).remove(dataNotAttached);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataClean);
    CORRADE_COMPARE(ui.layer(layer).usedCount(), 4);

    /* Calling clean() removes animations attached to the removed data and
       resets the states to not require clean() anymore */
    if(data.clean) {
        {
            CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));

            bool expectedAnimationIdsToRemove[]{
                /* animation1 was attached to `dataNotAttached` */
                true, false, false
            };
            if(data.dataAttachmentAnimators)
                ui.animator<AttachmentAnimator>(dataAttachmentAnimator).expectedAnimationIdsToRemove = expectedAnimationIdsToRemove;

            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
        CORRADE_COMPARE(ui.layer(layer).usedCount(), 4);
        if(data.layouters) {
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 0);
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 0);
        }
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 17 + (data.layouters ? 1 : 0));
        CORRADE_COMPARE(ui.layer<Layer>(anotherLayer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(anotherLayer).updateCallCount, 0);
        if(data.nodeAttachmentAnimators)
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).cleanCallCount, 0);
        if(data.dataAttachmentAnimators) {
            CORRADE_COMPARE(ui.animator(dataAttachmentAnimator).usedCount(), 2);
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(dataAttachmentAnimator).cleanCallCount, 1);
            /* The layer this animator is associated with doesn't have
               NeedsDataClean set, so clean() shouldn't be called for it */
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(anotherDataAttachmentAnimator).cleanCallCount, 0);
        }
    }

    /* Calling update() calls clean() if wasn't done above already but
       otherwise it's a no-op */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));

        bool expectedAnimationIdsToRemove[]{
            /* animation1 was attached to `dataNotAttached` */
            true, false, false
        };
        if(data.dataAttachmentAnimators)
            ui.animator<AttachmentAnimator>(dataAttachmentAnimator).expectedAnimationIdsToRemove = expectedAnimationIdsToRemove;

        ui.update();
    }
    CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
    CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 17 + (data.layouters ? 1 : 0));
    CORRADE_COMPARE(ui.layer<Layer>(anotherLayer).cleanCallCount, 0);
    CORRADE_COMPARE(ui.layer<Layer>(anotherLayer).updateCallCount, 0);
    if(data.nodeAttachmentAnimators)
        CORRADE_COMPARE(ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).cleanCallCount, 0);
    if(data.dataAttachmentAnimators) {
        CORRADE_COMPARE(ui.animator(dataAttachmentAnimator).usedCount(), 2);
        CORRADE_COMPARE(ui.animator<AttachmentAnimator>(dataAttachmentAnimator).cleanCallCount, 1);
        /* The layer this animator is associated with doesn't have
           NeedsDataClean set, so clean() shouldn't be called for it */
        CORRADE_COMPARE(ui.animator<AttachmentAnimator>(anotherDataAttachmentAnimator).cleanCallCount, 0);
    }

    /* Removing attached data marks the layer with both NeedsDataClean and
       NeedsAttachmentUpdate, which is then propagated to the UI-wide state */
    ui.layer<Layer>(layer).remove(dataNode);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataClean|UserInterfaceState::NeedsDataAttachmentUpdate);
    CORRADE_COMPARE(ui.layer(layer).usedCount(), 3);

    /* Calling clean() calls animator clean (with an empty mask because no
       animations were attached to this data) and resets the states to not
       require clean() anymore */
    if(data.clean) {
        {
            CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));

            bool expectedAnimationIdsToRemove[]{
                false, false, false
            };
            if(data.dataAttachmentAnimators)
                ui.animator<AttachmentAnimator>(dataAttachmentAnimator).expectedAnimationIdsToRemove = expectedAnimationIdsToRemove;

            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate);
        CORRADE_COMPARE(ui.layer(layer).usedCount(), 3);
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 17 + (data.layouters ? 1 : 0));
        CORRADE_COMPARE(ui.layer<Layer>(anotherLayer).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layer<Layer>(anotherLayer).updateCallCount, 0);
        if(data.nodeAttachmentAnimators)
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).cleanCallCount, 0);
        if(data.dataAttachmentAnimators) {
            CORRADE_COMPARE(ui.animator(dataAttachmentAnimator).usedCount(), 2);
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(dataAttachmentAnimator).cleanCallCount, 2);
            /* The layer this animator is associated with doesn't have
               NeedsDataClean set, so clean() shouldn't be called for it */
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(anotherDataAttachmentAnimator).cleanCallCount, 0);
        }
    }

    /* Calling update() then uploads remaining data and resets the remaining
       state flag; also calls clean() if wasn't done above already. Layouts
       don't get updated. */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
        UnsignedInt expectedDataIds[]{
            dataHandleId(dataAnother1),
            dataHandleId(dataNested1)
        };
        Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
            {{3.0f, 1.0f}, {2.0f, 4.0f}}, /* node */
            /* With the layouter for another1 gone, the size is not updated
               anymore */
            data.layouters ?
                Containers::Pair<Vector2, Vector2>{{3.0f, 0.0f}, {0.5f, 2.0f}} :
                Containers::Pair<Vector2, Vector2>{{5.0f, 0.0f}, {1.0f, 2.0f}},
            /* Which also affects another2 that's a child */
            data.layouters ?
                Containers::Pair<Vector2, Vector2>{{3.0f, 2.0f}, {1.0f, 2.0f}} :
                Containers::Pair<Vector2, Vector2>{{5.0f, 2.0f}, {1.0f, 2.0f}},
            {{4.0f, 4.0f}, {1.0f, 2.0f}}, /* nested1 */
            {},
            {},                           /* invisible */
            {},                           /* topLevelChildOfInvisible */
            {},                           /* notInOrder */
        };
        Containers::Pair<UnsignedInt, UnsignedInt> expectedClipRectIdsDataCounts[]{
            {0, 1}, /* another1, unclipped */
            {1, 1}  /* remaining node child */
        };
        Containers::Pair<Vector2, Vector2> expectedClipRectOffsetsSizes[]{
            {{}, {}},
            {{3.0f, 1.0f}, {2.0f, 4.0f}},
        };
        Containers::Pair<Vector2, Vector2> expectedCompositeRectOffsetsSizes[]{
            data.layouters ?              /* matching another1 */
                Containers::Pair<Vector2, Vector2>{{3.0f, 0.0f}, {0.5f, 2.0f}} :
                Containers::Pair<Vector2, Vector2>{{5.0f, 0.0f}, {1.0f, 2.0f}},
            {{4.0f, 4.0f}, {1.0f, 1.0f}}, /* matching nested1, clipped */
        };
        bool expectedNodesEnabled[]{
            /* Again all enabled except invisible, topLevelChildOfInvisible,
               notInOrder and nested2 */
            true, true, true, true, false, false, false, false
        };
        ui.layer<Layer>(layer).expectedDataIds = expectedDataIds;
        ui.layer<Layer>(layer).expectedClipRectIdsDataCounts = expectedClipRectIdsDataCounts;
        if(data.compositingLayer)
            ui.layer<Layer>(layer).expectedCompositeRectOffsetsSizes = expectedCompositeRectOffsetsSizes;
        for(LayerHandle i: {layer, anotherLayer}) {
            CORRADE_ITERATION(i);
            /* Removing attached data means the draw order needs an update.
               Currently, affecting any node attachments triggers a need to
               update draw order on all layers, even those that didn't have the
               attachments affected. */
            /** @todo separate those? or with the addition of draw merging this
                won't be possible anymore? */
            ui.layer<Layer>(i).expectedState = LayerState::NeedsNodeOrderUpdate;
            ui.layer<Layer>(i).expectedNodeOffsetsSizes = expectedNodeOffsetsSizes;
            ui.layer<Layer>(i).expectedNodesEnabled = expectedNodesEnabled;
            ui.layer<Layer>(i).expectedClipRectOffsetsSizes = expectedClipRectOffsetsSizes;
        }

        bool expectedAnimationIdsToRemove[]{
            false, false, false
        };
        if(data.dataAttachmentAnimators)
            ui.animator<AttachmentAnimator>(dataAttachmentAnimator).expectedAnimationIdsToRemove = expectedAnimationIdsToRemove;

        ui.update();
    }
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.renderer<Renderer>().setupFramebufferCallCount, 2);
    if(data.layouters) {
        CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 0);
        CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 0);
        CORRADE_COMPARE_AS(layouterUpdateCalls, Containers::arrayView({
            layouterHandleId(layouter2), layouterHandleId(layouter1),
        }), TestSuite::Compare::Container);
    }
    CORRADE_COMPARE(ui.layer(layer).usedCount(), 3);
    CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 0);
    CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 18 + (data.layouters ? 1 : 0));
    CORRADE_COMPARE(ui.layer<Layer>(anotherLayer).cleanCallCount, 0);
    CORRADE_COMPARE(ui.layer<Layer>(anotherLayer).updateCallCount, 1);
    if(data.nodeAttachmentAnimators)
        CORRADE_COMPARE(ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).cleanCallCount, 0);
    if(data.dataAttachmentAnimators) {
        CORRADE_COMPARE(ui.animator(dataAttachmentAnimator).usedCount(), 2);
        CORRADE_COMPARE(ui.animator<AttachmentAnimator>(dataAttachmentAnimator).cleanCallCount, 2);
        /* The layer this animator is associated with doesn't have
           NeedsDataClean set, so clean() shouldn't be called for it */
        CORRADE_COMPARE(ui.animator<AttachmentAnimator>(anotherDataAttachmentAnimator).cleanCallCount, 0);
    }

    /* Removing a node sets a state flag */
    ui.removeNode(node);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeClean);
    CORRADE_COMPARE(ui.nodeUsedCount(), 7);
    if(data.layouters) {
        CORRADE_COMPARE(ui.layouter(layouter1).usedCount(), 2);
        CORRADE_COMPARE(ui.layouter(layouter2).usedCount(), 5);
        /* Node parents are reset to Null in clean() for freed nodes, Update
           the expected array to match that */
        expectedNodeParents[nodeHandleId(nested1)] = NodeHandle::Null;
        expectedNodeParents[nodeHandleId(nested2)] = NodeHandle::Null;
    }
    CORRADE_COMPARE(ui.layer(layer).usedCount(), 3);

    /* Calling clean() removes the child nodes, the now-invalid layout
       assignments, data and animation attachments and resets the state to not
       require clean() anymore */
    if(data.clean) {
        {
            CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));

            bool expectedLayoutIdsToRemove1[]{
                /* Nothing changes in layouter1 */
                false, false, false
            };
            bool expectedLayoutIdsToRemove2[]{
                /* layout2Node, layout2Nested2Duplicate and layout2Nested2 were
                   attached to node and nested2, which got all orphaned when
                   removing `node`. The duplicate layout gets removed also. */
                true, true, true, false, false
            };
            if(data.layouters) {
                ui.layouter<Layouter>(layouter1).expectedLayoutIdsToRemove = expectedLayoutIdsToRemove1;
                ui.layouter<Layouter>(layouter2).expectedLayoutIdsToRemove = expectedLayoutIdsToRemove2;
            }

            bool expectedDataIdsToRemove[]{
                /* data1 and data5 was attached to nested2 and nested1, which
                   got orphaned after removing its parent, `node` */
                true, false, false, false, true
            };
            ui.layer<Layer>(layer).expectedDataIdsToRemove = expectedDataIdsToRemove;
            /* The second layer doesn't have anything */
            ui.layer<Layer>(anotherLayer).expectedDataIdsToRemove = {};

            bool expectedNodeAttachmentAnimationIdsToRemove[]{
                /* animation1 and animation3 were attached to `node` and
                   `nested1`, which got removed */
                true, false, true
            };
            bool expectedDataAttachmentAnimationIdsToRemove[]{
                /* animation3 was attached to `dataNested2` which was attached
                   to `nested2`, which got removed */
                false, false, true
            };
            if(data.nodeAttachmentAnimators)
                ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).expectedAnimationIdsToRemove = expectedNodeAttachmentAnimationIdsToRemove;
            if(data.dataAttachmentAnimators)
                ui.animator<AttachmentAnimator>(dataAttachmentAnimator).expectedAnimationIdsToRemove = expectedDataAttachmentAnimationIdsToRemove;

            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate|UserInterfaceState::NeedsDataAttachmentUpdate);
        CORRADE_COMPARE(ui.renderer<Renderer>().setupFramebufferCallCount, 2);
        CORRADE_COMPARE(ui.nodeUsedCount(), 5);
        if(data.layouters) {
            CORRADE_COMPARE(ui.layouter(layouter1).usedCount(), 2);
            CORRADE_COMPARE(ui.layouter(layouter2).usedCount(), 2);
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 1);
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 1);
        }
        CORRADE_COMPARE(ui.layer(layer).usedCount(), 1);
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 1);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 18 + (data.layouters ? 1 : 0));
        CORRADE_COMPARE(ui.layer<Layer>(anotherLayer).cleanCallCount, 1);
        CORRADE_COMPARE(ui.layer<Layer>(anotherLayer).updateCallCount, 1);
        if(data.nodeAttachmentAnimators) {
            CORRADE_COMPARE(ui.animator(nodeAttachmentAnimator).usedCount(), 1);
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).cleanCallCount, 1);
        }
        if(data.dataAttachmentAnimators) {
            CORRADE_COMPARE(ui.animator(dataAttachmentAnimator).usedCount(), 1);
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(dataAttachmentAnimator).cleanCallCount, 3);
            /* In this case NeedsDataClean is implied by NeedsNodeClean
               globally and thus clean() *does* get called for this one as
               well */
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(anotherDataAttachmentAnimator).cleanCallCount, 1);
        }
    }

    /* Calling update() then calls the layouters, uploads remaining data and
       resets the remaining state flag */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));

        bool expectedLayoutIdsToRemove1[]{
            /* Nothing changes in layouter1 */
            false, false, false
        };
        bool expectedLayoutIdsToRemove2[]{
            /* layout2Node, layout2Nested2Duplicate and layout2Nested2 were
               attached to node and nested2, which got all orphaned when
               removing `node`. The duplicate layout gets removed also. */
            true, true, true, false, false
        };
        bool expectedLayoutIdsToUpdate1[]{
            /* layout2Another1 and layout2Another2 */
            false, false, false, true, true
        };
        UnsignedInt expectedTopLevelLayoutIds1[]{
            layoutHandleId(layout2Another1),
            /* layout2Another2 is top-level but not root, so not included
               here */
        };
        Containers::Pair<Vector2, Vector2> expectedLayoutNodeOffsetsSizes1[]{
            {},
            {{3.0f, 2.0f}, {0.5f, 1.0f}}, /* another1 */
            {{0.0f, 4.0f}, {1.0f, 1.0f}}, /* another2 */
            {},
            {},
            {{9.0f, 9.0f}, {9.0f, 9.0f}}, /* invisible */
            {{0.0f, 0.0f}, {9.0f, 9.0f}}, /* topLevelChildOfInvisible */
            {{8.0f, 8.0f}, {8.0f, 8.0f}}, /* notInOrder */
        };
        if(data.layouters) {
            layouterUpdateCalls = {};
            ui.layouter<Layouter>(layouter1).expectedLayoutIdsToRemove = expectedLayoutIdsToRemove1;
            ui.layouter<Layouter>(layouter2).expectedLayoutIdsToRemove = expectedLayoutIdsToRemove2;
            ui.layouter<Layouter2>(layouter2).expectedLayoutIdsToUpdate[0] = expectedLayoutIdsToUpdate1;
            ui.layouter<Layouter1>(layouter1).expectedLayoutIdsToUpdate = {};
            ui.layouter<Layouter2>(layouter2).expectedLayoutIdsToUpdate[1] = {};
            ui.layouter<Layouter2>(layouter2).expectedTopLevelLayoutIds[0] = expectedTopLevelLayoutIds1;
            ui.layouter<Layouter1>(layouter1).expectedTopLevelLayoutIds = {};
            ui.layouter<Layouter2>(layouter2).expectedTopLevelLayoutIds[1] = {};
            ui.layouter<Layouter2>(layouter2).expectedNodeOffsetsSizes[0] = expectedLayoutNodeOffsetsSizes1;
            ui.layouter<Layouter1>(layouter1).expectedNodeOffsetsSizes = {};
            ui.layouter<Layouter2>(layouter2).expectedNodeOffsetsSizes[1] = {};
            ui.layouter<Layouter2>(layouter2).updateCallId = 0;
        }

        bool expectedDataIdsToRemove[]{
            /* data1 and data5 was attached to nested2 and nested1, which got
               orphaned after removing its parent, `node` */
            true, false, false, false, true
        };
        UnsignedInt expectedDataIds[]{
            dataHandleId(dataAnother1)
        };
        Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
            {},
            /* With the layouter for another1 gone, the size is not updated
               anymore */
            data.layouters ?
                Containers::Pair<Vector2, Vector2>{{3.0f, 0.0f}, {0.5f, 2.0f}} :
                Containers::Pair<Vector2, Vector2>{{5.0f, 0.0f}, {1.0f, 2.0f}},
            /* Which also affects another2 that's a child */
            data.layouters ?
                Containers::Pair<Vector2, Vector2>{{3.0f, 2.0f}, {1.0f, 2.0f}} :
                Containers::Pair<Vector2, Vector2>{{5.0f, 2.0f}, {1.0f, 2.0f}},
            {},
            {},
            {},                           /* invisible */
            {},                           /* topLevelChildOfInvisible */
            {},                           /* notInOrder */
        };
        bool expectedNodesEnabled[]{
            /* Only another1 and another2 left now */
            false, true, true, false, false, false, false, false
        };
        Containers::Pair<UnsignedInt, UnsignedInt> expectedClipRectIdsDataCounts[]{
            {0, 1}, /* another1, unclipped */
        };
        Containers::Pair<Vector2, Vector2> expectedClipRectOffsetsSizes[]{
            {{}, {}}
        };
        Containers::Pair<Vector2, Vector2> expectedCompositeRectOffsetsSizes[]{
            data.layouters ?              /* matching another1 */
                Containers::Pair<Vector2, Vector2>{{3.0f, 0.0f}, {0.5f, 2.0f}} :
                Containers::Pair<Vector2, Vector2>{{5.0f, 0.0f}, {1.0f, 2.0f}},
        };
        /* Removing a node causes just node order and enabled state to get
           updated, however again with the coarseness of UserInterfaceState
           bits it's together with offset/size as well */
        /** @todo separate */
        ui.layer<Layer>(layer).expectedState = LayerState::NeedsNodeOrderUpdate|LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsNodeOffsetSizeUpdate|(data.compositingLayer ? LayerState::NeedsCompositeOffsetSizeUpdate : LayerStates{});
        /* The other layer doesn't have compositing enabled ever */
        ui.layer<Layer>(anotherLayer).expectedState = LayerState::NeedsNodeOrderUpdate|LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsNodeOffsetSizeUpdate;
        ui.layer<Layer>(layer).expectedDataIdsToRemove = expectedDataIdsToRemove;
        ui.layer<Layer>(layer).expectedDataIds = expectedDataIds;
        ui.layer<Layer>(layer).expectedClipRectIdsDataCounts = expectedClipRectIdsDataCounts;
        if(data.compositingLayer)
            ui.layer<Layer>(layer).expectedCompositeRectOffsetsSizes = expectedCompositeRectOffsetsSizes;
        for(LayerHandle i: {layer, anotherLayer}) {
            CORRADE_ITERATION(i);
            ui.layer<Layer>(i).expectedNodeOffsetsSizes = expectedNodeOffsetsSizes;
            ui.layer<Layer>(i).expectedNodesEnabled = expectedNodesEnabled;
            ui.layer<Layer>(i).expectedClipRectOffsetsSizes = expectedClipRectOffsetsSizes;
        }

        bool expectedNodeAttachmentAnimationIdsToRemove[]{
            /* animation1 and animation3 were attached to `node` and
                `nested1`, which got removed */
            true, false, true
        };
        bool expectedDataAttachmentAnimationIdsToRemove[]{
            /* animation3 was attached to `dataNested2` which was attached
                to `nested2`, which got removed */
            false, false, true
        };
        if(data.nodeAttachmentAnimators)
            ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).expectedAnimationIdsToRemove = expectedNodeAttachmentAnimationIdsToRemove;
        if(data.dataAttachmentAnimators)
            ui.animator<AttachmentAnimator>(dataAttachmentAnimator).expectedAnimationIdsToRemove = expectedDataAttachmentAnimationIdsToRemove;

        ui.update();
    }
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.renderer<Renderer>().setupFramebufferCallCount, 2);
    CORRADE_COMPARE(ui.nodeUsedCount(), 5);
    if(data.layouters) {
        CORRADE_COMPARE(ui.layouter(layouter1).usedCount(), 2);
        CORRADE_COMPARE(ui.layouter(layouter2).usedCount(), 2);
        CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 1);
        CORRADE_COMPARE(ui.layouter<Layouter>(layouter2).cleanCallCount, 1);
        CORRADE_COMPARE_AS(layouterUpdateCalls, Containers::arrayView({
            /* There's just layouter2 called now */
            layouterHandleId(layouter2),
        }), TestSuite::Compare::Container);
    }
    CORRADE_COMPARE(ui.layer(layer).usedCount(), 1);
    CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 1);
    CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 19 + (data.layouters ? 1 : 0));
    CORRADE_COMPARE(ui.layer<Layer>(anotherLayer).cleanCallCount, 1);
    CORRADE_COMPARE(ui.layer<Layer>(anotherLayer).updateCallCount, 2);
    if(data.nodeAttachmentAnimators) {
        CORRADE_COMPARE(ui.animator(nodeAttachmentAnimator).usedCount(), 1);
        CORRADE_COMPARE(ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).cleanCallCount, 1);
    }
    if(data.dataAttachmentAnimators) {
        CORRADE_COMPARE(ui.animator(dataAttachmentAnimator).usedCount(), 1);
        CORRADE_COMPARE(ui.animator<AttachmentAnimator>(dataAttachmentAnimator).cleanCallCount, 3);
        /* In this case NeedsDataClean is implied by NeedsNodeClean globally
           and thus clean() *does* get called for this one as well */
        CORRADE_COMPARE(ui.animator<AttachmentAnimator>(anotherDataAttachmentAnimator).cleanCallCount, 1);
    }

    /* Removing a layouter sets a state flag */
    if(data.layouters) {
        ui.removeLayouter(layouter2);
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsLayoutAssignmentUpdate);

        /* Calling clean() should be a no-op */
        if(data.clean && data.noOp) {
            {
                CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
                ui.clean();
            }
            CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsLayoutAssignmentUpdate);
            CORRADE_COMPARE(ui.layouter(layouter1).usedCount(), 2);
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 1);
            CORRADE_COMPARE_AS(layouterUpdateCalls, Containers::arrayView({
                layouterHandleId(layouter2),
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 1);
            CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 20);
            CORRADE_COMPARE(ui.layer<Layer>(anotherLayer).cleanCallCount, 1);
            CORRADE_COMPARE(ui.layer<Layer>(anotherLayer).updateCallCount, 2);
        }

        /* Calling update() then resets the remaining state flag. There's no
           visible layouts to update anymore, so no update() is called on
           them, only on the layer. */
        {
            CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));

            layouterUpdateCalls = {};
            ui.layouter<Layouter1>(layouter1).expectedLayoutIdsToUpdate = {};
            ui.layouter<Layouter1>(layouter1).expectedTopLevelLayoutIds = {};
            ui.layouter<Layouter1>(layouter1).expectedNodeOffsetsSizes = {};

            UnsignedInt expectedDataIds[]{
                dataHandleId(dataAnother1)
            };
            Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
                {},
                /* With all layouters for another1 and another2 gone, the sizes
                   are not updated anymore */
                {{3.0f, 2.0f}, {0.5f, 1.0f}},
                {{3.0f, 6.0f}, {1.0f, 1.0f}},
                {},
                {},
                {},                           /* invisible */
                {},                           /* topLevelChildOfInvisible */
                {}                            /* notInOrder */
            };
            bool expectedNodesEnabled[]{
                /* Only another1 and another2 left */
                false, true, true, false, false, false, false, false
            };
            Containers::Pair<UnsignedInt, UnsignedInt> expectedClipRectIdsDataCounts[]{
                {0, 1}, /* another1, unclipped */
            };
            Containers::Pair<Vector2, Vector2> expectedClipRectOffsetsSizes[]{
                {{}, {}}
            };
            Containers::Pair<Vector2, Vector2> expectedCompositeRectOffsetsSizes[]{
                {{3.0f, 2.0f}, {0.5f, 1.0f}}, /* matching another1 */
            };
            /* Removing a layouter means node offsets/sizes can change but also
               the set of visible nodes and enabled state */
            ui.layer<Layer>(layer).expectedState = LayerState::NeedsNodeOrderUpdate|LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsNodeOffsetSizeUpdate|(data.compositingLayer ? LayerState::NeedsCompositeOffsetSizeUpdate : LayerStates{});
            /* The other layer doesn't have compositing enabled ever */
            ui.layer<Layer>(anotherLayer).expectedState = LayerState::NeedsNodeOrderUpdate|LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsNodeOffsetSizeUpdate;
            ui.layer<Layer>(layer).expectedDataIds = expectedDataIds;
            ui.layer<Layer>(layer).expectedClipRectIdsDataCounts = expectedClipRectIdsDataCounts;
            if(data.compositingLayer)
                ui.layer<Layer>(layer).expectedCompositeRectOffsetsSizes = expectedCompositeRectOffsetsSizes;
            for(LayerHandle i: {layer, anotherLayer}) {
                CORRADE_ITERATION(i);
                ui.layer<Layer>(i).expectedNodeOffsetsSizes = expectedNodeOffsetsSizes;
                ui.layer<Layer>(i).expectedNodesEnabled = expectedNodesEnabled;
                ui.layer<Layer>(i).expectedClipRectOffsetsSizes = expectedClipRectOffsetsSizes;
            }

            ui.update();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
        CORRADE_COMPARE(ui.layouter(layouter1).usedCount(), 2);
        CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 1);
        CORRADE_COMPARE_AS(layouterUpdateCalls, Containers::arrayView<UnsignedInt>({
            /* No update() is called on layouter2 anymore */
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE(ui.layer(layer).usedCount(), 1);
        CORRADE_COMPARE(ui.layer<Layer>(layer).cleanCallCount, 1);
        CORRADE_COMPARE(ui.layer<Layer>(layer).updateCallCount, 21);
        CORRADE_COMPARE(ui.layer<Layer>(anotherLayer).cleanCallCount, 1);
        CORRADE_COMPARE(ui.layer<Layer>(anotherLayer).updateCallCount, 3);
        if(data.nodeAttachmentAnimators) {
            CORRADE_COMPARE(ui.animator(nodeAttachmentAnimator).usedCount(), 1);
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).cleanCallCount, 1);
        }
        if(data.dataAttachmentAnimators) {
            CORRADE_COMPARE(ui.animator(dataAttachmentAnimator).usedCount(), 1);
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(dataAttachmentAnimator).cleanCallCount, 3);
        }
    }

    /* Removing a layer sets a state flag */
    ui.removeLayer(layer);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate);

    /* Calling clean() should be a no-op */
    if(data.clean && data.noOp) {
        {
            CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate);
        CORRADE_COMPARE(ui.renderer<Renderer>().setupFramebufferCallCount, 2);
        if(data.layouters) {
            CORRADE_COMPARE(ui.layouter(layouter1).usedCount(), 2);
            CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 1);
        }
        CORRADE_COMPARE(ui.layer<Layer>(anotherLayer).cleanCallCount, 1);
        CORRADE_COMPARE(ui.layer<Layer>(anotherLayer).updateCallCount, 2 + (data.layouters ? 1 : 0));
        if(data.nodeAttachmentAnimators) {
            CORRADE_COMPARE(ui.animator(nodeAttachmentAnimator).usedCount(), 1);
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).cleanCallCount, 1);
        }
        if(data.dataAttachmentAnimators) {
            CORRADE_COMPARE(ui.animator(dataAttachmentAnimator).usedCount(), 1);
            CORRADE_COMPARE(ui.animator<AttachmentAnimator>(dataAttachmentAnimator).cleanCallCount, 3);
        }
    }

    /* Calling update() then resets the remaining state flag. There's no
       visible layouts or data anymore, only the remaining another and nested3
       nodes for which offset/size and a corresponding clip rect offset/size is
       passed. */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
        Containers::Pair<Vector2, Vector2> expectedNodeOffsetsSizes[]{
            {},
            /* With all layouters for another1 and another2 gone, the sizes are
               not updated anymore */
            data.layouters ?
                Containers::Pair<Vector2, Vector2>{{3.0f, 2.0f}, {0.5f, 1.0f}} :
                Containers::Pair<Vector2, Vector2>{{5.0f, 0.0f}, {1.0f, 2.0f}},
            data.layouters ?
                Containers::Pair<Vector2, Vector2>{{3.0f, 6.0f}, {1.0f, 1.0f}} :
                Containers::Pair<Vector2, Vector2>{{5.0f, 2.0f}, {1.0f, 2.0f}},
            {},
            {},
            {},                           /* invisible */
            {},                           /* topLevelChildOfInvisible */
            {},                           /* notInOrder */
        };
        bool expectedNodesEnabled[]{
            /* Only another1 and another2 left */
            false, true, true, false, false, false, false, false
        };
        Containers::Pair<Vector2, Vector2> expectedClipRectOffsetsSizes[]{
            {{}, {}}
        };
        ui.layer<Layer>(anotherLayer).expectedDataIdsToRemove = {};
        /* Currently, affecting any node attachments triggers a need to update
           draw order on all layers, even those that didn't have the
           attachments affected */
        /** @todo separate those? or with the addition of draw merging this
            won't be possible anymore? */
        ui.layer<Layer>(anotherLayer).expectedState = LayerState::NeedsNodeOrderUpdate;
        ui.layer<Layer>(anotherLayer).expectedDataIds = {};
        ui.layer<Layer>(anotherLayer).expectedNodeOffsetsSizes = expectedNodeOffsetsSizes;
        ui.layer<Layer>(anotherLayer).expectedNodesEnabled = expectedNodesEnabled;
        ui.layer<Layer>(anotherLayer).expectedClipRectIdsDataCounts = {};
        ui.layer<Layer>(anotherLayer).expectedClipRectOffsetsSizes = expectedClipRectOffsetsSizes;
        ui.update();
    }
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.renderer<Renderer>().setupFramebufferCallCount, 2);
    CORRADE_COMPARE(ui.layer(anotherLayer).usedCount(), 0);
    if(data.layouters) {
        CORRADE_COMPARE(ui.layouter(layouter1).usedCount(), 2);
        CORRADE_COMPARE(ui.layouter<Layouter>(layouter1).cleanCallCount, 1);
        CORRADE_COMPARE_AS(layouterUpdateCalls, Containers::arrayView<UnsignedInt>({
        }), TestSuite::Compare::Container);
    }
    CORRADE_COMPARE(ui.layer<Layer>(anotherLayer).cleanCallCount, 1);
    CORRADE_COMPARE(ui.layer<Layer>(anotherLayer).updateCallCount, 3 + (data.layouters ? 1 : 0));
    if(data.nodeAttachmentAnimators) {
        CORRADE_COMPARE(ui.animator(nodeAttachmentAnimator).usedCount(), 1);
        CORRADE_COMPARE(ui.animator<AttachmentAnimator>(nodeAttachmentAnimator).cleanCallCount, 1);
    }
    if(data.dataAttachmentAnimators) {
        CORRADE_COMPARE(ui.animator(dataAttachmentAnimator).usedCount(), 1);
        /* The layer the animator was associated with is gone but the animator
           stays. It's just not used for anything anymore. */
        CORRADE_COMPARE(ui.animator<AttachmentAnimator>(dataAttachmentAnimator).cleanCallCount, 3);
        /* The other animator isn't cleaned or affected in any other way */
        CORRADE_COMPARE(ui.animator<AttachmentAnimator>(anotherDataAttachmentAnimator).cleanCallCount, 1);
    }
}

void AbstractUserInterfaceTest::stateAnimations() {
    auto&& data = StateAnimationsData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Event/framebuffer scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

    /* Creating a layer sets no state flags */
    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::setAnimator;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override {
            return LayerFeature::AnimateData|LayerFeature::AnimateStyles;
        }

        void doClean(Containers::BitArrayView) override {
            ++cleanCallCount;
        }
        void doUpdate(LayerStates state, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&) override {
            /* The doUpdate() should never get the NeedsAttachmentUpdate, only
               the NeedsNodeOrderUpdate that's a subset of it */
            CORRADE_VERIFY(!(state >= LayerState::NeedsAttachmentUpdate));
            CORRADE_COMPARE(state, expectedState);

            Vector2 expectedNodeSizes[]{
                {0.5f, 0.25f},
                {0.75f, 1.0f}
            };
            for(std::size_t i = 0; i != nodeOffsets.size(); ++i) {
                if(!expectedNodesValid[i]) continue;

                CORRADE_ITERATION(i);
                CORRADE_COMPARE(nodeOffsets[i], expectedNodeOffsets[i]);
                CORRADE_COMPARE(nodeSizes[i], expectedNodeSizes[i]);
            }
            CORRADE_COMPARE_AS(nodesEnabled,
                expectedNodesEnabled.sliceBit(0),
                TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(clipRectOffsets,
                expectedClipRectOffsets,
                TestSuite::Compare::Container);
            ++updateCallCount;
        }

        void doAdvanceAnimations(Nanoseconds time, Containers::MutableBitArrayView, const Containers::StridedArrayView1D<Float>&, Containers::MutableBitArrayView, const Containers::Iterable<AbstractDataAnimator>& animators) override {
            for(AbstractDataAnimator& animator: animators)
                if(animator.state() & AnimatorState::NeedsAdvance) {
                    CORRADE_COMPARE(time, 5_nsec);
                    ++advanceDataAnimationsCallCount;
                    setNeedsUpdate(LayerState::NeedsDataUpdate);
                }
        }
        void doAdvanceAnimations(Nanoseconds time, Containers::MutableBitArrayView, const Containers::StridedArrayView1D<Float>&, Containers::MutableBitArrayView, const Containers::Iterable<AbstractStyleAnimator>& animators) override {
            for(AbstractStyleAnimator& animator: animators)
                if(animator.state() & AnimatorState::NeedsAdvance) {
                    CORRADE_COMPARE(time, 5_nsec);
                    ++advanceStyleAnimationsCallCount;
                    setNeedsUpdate(LayerState::NeedsCommonDataUpdate);
                }
        }

        LayerStates expectedState;
        Containers::StridedArrayView1D<const Vector2> expectedNodeOffsets;
        Containers::StridedArrayView1D<const bool> expectedNodesEnabled;
        Containers::StridedArrayView1D<const Vector2> expectedClipRectOffsets;
        Containers::ArrayView<const bool> expectedNodesValid;
        Int cleanCallCount = 0;
        Int updateCallCount = 0;
        Int advanceDataAnimationsCallCount = 0;
        Int advanceStyleAnimationsCallCount = 0;
    };
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer()));
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

    /* Creating animators sets no state flags */
    struct Animator: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;
        using AbstractGenericAnimator::create;

        AnimatorFeatures doFeatures() const override { return {}; }
        void doClean(Containers::BitArrayView) override {
            ++cleanCallCount;
        }
        void doAdvance(Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {
            ++advanceCallCount;
        }

        Int cleanCallCount = 0;
        Int advanceCallCount = 0;
    };
    struct NodeAnimator: AbstractNodeAnimator {
        explicit NodeAnimator(AnimatorHandle handle, NodeAnimations animations): AbstractNodeAnimator{handle}, _animations{animations} {}

        using AbstractNodeAnimator::create;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::NodeAttachment;
        }
        void doClean(Containers::BitArrayView) override {
            ++cleanCallCount;
        }
        NodeAnimations doAdvance(Containers::BitArrayView active, const Containers::StridedArrayView1D<const Float>& factors, const Containers::StridedArrayView1D<Vector2>& nodeOffsets, const Containers::StridedArrayView1D<Vector2>& nodeSizes, const Containers::StridedArrayView1D<NodeFlags>& nodeFlags, Containers::MutableBitArrayView nodesRemove) override {
            CORRADE_ITERATION(handle());
            CORRADE_COMPARE_AS(nodeOffsets,
                expectedNodeOffsets,
                TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(nodeSizes, Containers::stridedArrayView<Vector2>({
                {0.5f, 0.25f},
                {0.75f, 1.0f}
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(nodeFlags,
                expectedNodeFlags,
                TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(nodesRemove,
                expectedNodesRemove.sliceBit(0),
                TestSuite::Compare::Container);

            /* Do the thing the obvious way */
            for(std::size_t i = 0; i != active.size(); ++i) {
                CORRADE_ITERATION(i);
                CORRADE_VERIFY(active[i]);
                CORRADE_COMPARE(factors[i], 0.5f);

                if(_animations >= NodeAnimation::OffsetSize)
                    nodeOffsets[nodeHandleId(nodes()[i])] -= Vector2{1.0f};
                if(_animations >= NodeAnimation::Enabled) {
                    NodeFlags& flags = nodeFlags[nodeHandleId(nodes()[i])];
                    if(flags & NodeFlag::Disabled)
                        flags &= ~NodeFlag::Disabled;
                    else
                        flags |= NodeFlag::Disabled;
                }
                if(_animations >= NodeAnimation::Clip) {
                    NodeFlags& flags = nodeFlags[nodeHandleId(nodes()[i])];
                    if(flags & NodeFlag::Clip)
                        flags &= ~NodeFlag::Clip;
                    else
                        flags |= NodeFlag::Clip;
                }
                if(_animations >= NodeAnimation::Removal)
                    nodesRemove.set(nodeHandleId(nodes()[i]));
            }

            ++advanceCallCount;
            return _animations;
        }

        Containers::StridedArrayView1D<const Vector2> expectedNodeOffsets;
        Containers::StridedArrayView1D<const NodeFlags> expectedNodeFlags;
        Containers::StridedArrayView1D<const bool> expectedNodesRemove;
        Int cleanCallCount = 0;
        Int advanceCallCount = 0;

        private:
            NodeAnimations _animations;
    };
    struct DataAnimator: AbstractDataAnimator {
        using AbstractDataAnimator::AbstractDataAnimator;
        using AbstractDataAnimator::create;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::DataAttachment;
        }
        void doClean(Containers::BitArrayView) override {
            ++cleanCallCount;
        }

        Int cleanCallCount = 0;
    };
    struct StyleAnimator: AbstractStyleAnimator {
        using AbstractStyleAnimator::AbstractStyleAnimator;
        using AbstractStyleAnimator::create;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::DataAttachment;
        }
        void doClean(Containers::BitArrayView) override {
            ++cleanCallCount;
        }

        Int cleanCallCount = 0;
    };
    Animator& animator = ui.setGenericAnimatorInstance(Containers::pointer<Animator>(ui.createAnimator()));
    /* GCC in Release mode complains that some of these may be used
       uninitialized, MSVC as well. They aren't. */
    NodeAnimator *nodeAnimator1{}, *nodeAnimator2{};
    DataAnimator* dataAnimator{};
    StyleAnimator* styleAnimator{};
    if(data.nodeAnimations1)
        nodeAnimator1 = &ui.setNodeAnimatorInstance(Containers::pointer<NodeAnimator>(ui.createAnimator(), *data.nodeAnimations1));
    if(data.nodeAnimations2)
        nodeAnimator2 = &ui.setNodeAnimatorInstance(Containers::pointer<NodeAnimator>(ui.createAnimator(), *data.nodeAnimations2));
    if(data.dataAnimations) {
        Containers::Pointer<DataAnimator> instance{InPlaceInit, ui.createAnimator()};
        layer.setAnimator(*instance);
        dataAnimator = &ui.setDataAnimatorInstance(Utility::move(instance));
    }
    if(data.styleAnimations) {
        Containers::Pointer<StyleAnimator> instance{InPlaceInit, ui.createAnimator()};
        layer.setAnimator(*instance);
        styleAnimator = &ui.setStyleAnimatorInstance(Utility::move(instance));
    }
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

    /* Calling clean() should be a no-op, not cleaning anything in the
       layer or animators */
    if(data.clean && data.noOp) {
        {
            CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
        CORRADE_COMPARE(layer.cleanCallCount, 0);
        CORRADE_COMPARE(animator.cleanCallCount, 0);
        if(data.nodeAnimations1)
            CORRADE_COMPARE(nodeAnimator1->cleanCallCount, 0);
        if(data.nodeAnimations2)
            CORRADE_COMPARE(nodeAnimator2->cleanCallCount, 0);
        if(data.dataAnimations)
            CORRADE_COMPARE(dataAnimator->cleanCallCount, 0);
        if(data.styleAnimations)
            CORRADE_COMPARE(styleAnimator->cleanCallCount, 0);
    }

    /* Calling advanceAnimations() should be a no-op, not calling anything in
       the layer or the animators */
    if(data.noOp) {
        {
            CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
            ui.advanceAnimations(0_nsec);
        }
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
        CORRADE_COMPARE(layer.cleanCallCount, 0);
        CORRADE_COMPARE(animator.cleanCallCount, 0);
        CORRADE_COMPARE(animator.advanceCallCount, 0);
        if(data.nodeAnimations1) {
            CORRADE_COMPARE(nodeAnimator1->cleanCallCount, 0);
            CORRADE_COMPARE(nodeAnimator1->advanceCallCount, 0);
        }
        if(data.nodeAnimations2) {
            CORRADE_COMPARE(nodeAnimator2->cleanCallCount, 0);
            CORRADE_COMPARE(nodeAnimator2->advanceCallCount, 0);
        }
        if(data.dataAnimations) {
            CORRADE_COMPARE(dataAnimator->cleanCallCount, 0);
            CORRADE_COMPARE(layer.advanceDataAnimationsCallCount, 0);
        }
        if(data.styleAnimations) {
            CORRADE_COMPARE(styleAnimator->cleanCallCount, 0);
            CORRADE_COMPARE(layer.advanceStyleAnimationsCallCount, 0);
        }
    }

    /* Creating an animation sets a state flag */
    if(data.runningAnimation) {
        animator.create(0_nsec, 10_nsec);
        CORRADE_COMPARE(ui.state(), data.expectedInitialState);
    }

    /* Creating & removing a node sets a state flag */
    ui.removeNode(ui.createNode({}, {}));
    CORRADE_COMPARE(ui.state(), data.expectedInitialState|UserInterfaceState::NeedsNodeClean);

    /* Calling clean() propagates that to the layer and node attachment
       animators and leaves just state flags that trigger an update() later,
       and potentially advanceAnimations() */
    if(data.clean) {
        {
            CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), data.expectedInitialState|UserInterfaceState::NeedsNodeUpdate);
        CORRADE_COMPARE(layer.cleanCallCount, 1);
        CORRADE_COMPARE(animator.cleanCallCount, 0);
        if(data.nodeAnimations1)
            CORRADE_COMPARE(nodeAnimator1->cleanCallCount, 1);
        if(data.nodeAnimations2)
            CORRADE_COMPARE(nodeAnimator2->cleanCallCount, 1);
        if(data.dataAnimations)
            CORRADE_COMPARE(dataAnimator->cleanCallCount, 1);
        if(data.styleAnimations)
            CORRADE_COMPARE(styleAnimator->cleanCallCount, 1);
    }

    /* Calling advanceAnimations() delegates to clean() if not already but
       otherwise is a no-op if no animations are running -- the animators
       don't need advancing */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
        ui.advanceAnimations(0_nsec);
    }
    CORRADE_COMPARE(ui.state(), data.expectedInitialState|UserInterfaceState::NeedsNodeUpdate);
    CORRADE_COMPARE(layer.cleanCallCount, 1);
    CORRADE_COMPARE(animator.cleanCallCount, 0);
    CORRADE_COMPARE(animator.advanceCallCount, data.runningAnimation ? 1 : 0);
    if(data.nodeAnimations1) {
        CORRADE_COMPARE(nodeAnimator1->cleanCallCount, 1);
        CORRADE_COMPARE(nodeAnimator1->advanceCallCount, 0);
    }
    if(data.nodeAnimations2) {
        CORRADE_COMPARE(nodeAnimator2->cleanCallCount, 1);
        CORRADE_COMPARE(nodeAnimator2->advanceCallCount, 0);
    }
    if(data.dataAnimations) {
        CORRADE_COMPARE(dataAnimator->cleanCallCount, 1);
        CORRADE_COMPARE(layer.advanceDataAnimationsCallCount, 0);
    }
    if(data.styleAnimations) {
        CORRADE_COMPARE(styleAnimator->cleanCallCount, 1);
        CORRADE_COMPARE(layer.advanceStyleAnimationsCallCount, 0);
    }

    /* Create two nodes and two data, trigger an update to clean the state
       flags */
    NodeHandle node1 = ui.createNode({1.0f, 2.0f}, {0.5f, 0.25f}, NodeFlag::Clip);
    NodeHandle node2 = ui.createNode({3.0f, 4.0f}, {0.75f, 1.0f}, NodeFlag::Disabled);
    DataHandle data1 = layer.create(node1);
    DataHandle data2 = layer.create(node2);
    CORRADE_COMPARE(ui.state(), data.expectedInitialState|UserInterfaceState::NeedsNodeUpdate);
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));

        Vector2 expectedNodeOffsets[]{
            {1.0f, 2.0f},
            {3.0f, 4.0f},
        };
        bool expectedNodesEnabled[]{
            true, false
        };
        Vector2 expectedClipRectOffsets[]{
            {1.0f, 2.0f},
            {0.0f, 0.0f}
        };
        bool expectedNodesValid[]{
            true, true
        };
        layer.expectedState = LayerState::NeedsNodeOrderUpdate|LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsNodeOffsetSizeUpdate|LayerState::NeedsDataUpdate;
        layer.expectedNodeOffsets = expectedNodeOffsets;
        layer.expectedNodesEnabled = expectedNodesEnabled;
        layer.expectedClipRectOffsets = expectedClipRectOffsets;
        layer.expectedNodesValid = expectedNodesValid;

        ui.update();
    }
    CORRADE_COMPARE(layer.cleanCallCount, 1);
    CORRADE_COMPARE(layer.updateCallCount, 1);
    CORRADE_COMPARE(animator.cleanCallCount, 0);
    CORRADE_COMPARE(animator.advanceCallCount, data.runningAnimation ? 1 : 0);
    if(data.nodeAnimations1)
        CORRADE_COMPARE(nodeAnimator1->cleanCallCount, 1);
    if(data.nodeAnimations2)
        CORRADE_COMPARE(nodeAnimator2->cleanCallCount, 1);
    if(data.dataAnimations)
        CORRADE_COMPARE(dataAnimator->cleanCallCount, 1);
    if(data.styleAnimations)
        CORRADE_COMPARE(styleAnimator->cleanCallCount, 1);

    /* Attach animations to the nodes and data. Should set a state flag to
       trigger animation advance. GCC in Release mode complains that some of
       these may be used uninitialized, MSVC as well. They aren't. */
    AnimationHandle nodeAnimation1{}, nodeAnimation2{}, dataAnimation1{}, dataAnimation2{}, styleAnimation1{}, styleAnimation2{};
    if(data.nodeAnimations1)
        nodeAnimation1 = nodeAnimator1->create(0_nsec, 10_nsec, node2);
    if(data.nodeAnimations2)
        nodeAnimation2 = nodeAnimator2->create(0_nsec, 10_nsec, node1);
    if(data.dataAnimations) {
        dataAnimation1 = dataAnimator->create(0_nsec, 10_nsec, data1);
        dataAnimation2 = dataAnimator->create(0_nsec, 10_nsec, data2);
    }
    if(data.styleAnimations) {
        styleAnimation2 = styleAnimator->create(0_nsec, 10_nsec, data1);
        styleAnimation1 = styleAnimator->create(0_nsec, 10_nsec, data2);
    }
    CORRADE_COMPARE(ui.state(), data.expectedInitialState|(data.nodeAnimations1 || data.nodeAnimations2 || data.dataAnimations || data.styleAnimations ?
        UserInterfaceState::NeedsAnimationAdvance : UserInterfaceStates{}));

    /* Advancing the animation then may set other flags */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));

        Vector2 expectedNodeOffsetsAnimator1[]{
            {1.0f, 2.0f},
            {3.0f, 4.0f},
        };
        NodeFlags expectedNodeFlagsAnimator1[]{
            NodeFlag::Clip,
            NodeFlag::Disabled
        };
        bool expectedNodesRemoveAnimator1[]{
            false, false
        };
        if(data.nodeAnimations1) {
            nodeAnimator1->expectedNodeOffsets = expectedNodeOffsetsAnimator1;
            nodeAnimator1->expectedNodeFlags = expectedNodeFlagsAnimator1;
            nodeAnimator1->expectedNodesRemove = expectedNodesRemoveAnimator1;
        }
        if(data.nodeAnimations2) {
            nodeAnimator2->expectedNodeOffsets = data.expectedNodeOffsetsAnimator2;
            nodeAnimator2->expectedNodeFlags = data.expectedNodeFlagsAnimator2;
            nodeAnimator2->expectedNodesRemove = data.expectedNodesRemoveAnimator2;
        }
        if(data.dataAnimations) {
            /* Nothing is verified for the data animator, except for the time
               value, which is always just 5 nsec */
        }
        if(data.styleAnimations) {
            /* Nothing is verified for the style animator, except for the time
               value, which is always just 5 nsec */
        }

        ui.advanceAnimations(5_nsec);
    }
    CORRADE_COMPARE(ui.state(), data.expectedAdvanceState|
        (data.nodeAnimations1 || data.nodeAnimations2 || data.dataAnimations || data.styleAnimations ?
            UserInterfaceState::NeedsAnimationAdvance : UserInterfaceStates{}));
    CORRADE_COMPARE(layer.cleanCallCount, 1);
    CORRADE_COMPARE(animator.cleanCallCount, 0);
    CORRADE_COMPARE(animator.advanceCallCount, data.runningAnimation ? 2 : 0);
    CORRADE_COMPARE(ui.isHandleValid(node1), data.expectedNode1Valid);
    CORRADE_COMPARE(ui.isHandleValid(node2), data.expectedNode2Valid);
    if(data.nodeAnimations1) {
        CORRADE_COMPARE(nodeAnimator1->cleanCallCount, 1);
        CORRADE_COMPARE(nodeAnimator1->advanceCallCount, 1);
    }
    if(data.nodeAnimations2) {
        CORRADE_COMPARE(nodeAnimator2->cleanCallCount, 1);
        CORRADE_COMPARE(nodeAnimator2->advanceCallCount, 1);
    }
    if(data.dataAnimations) {
        CORRADE_COMPARE(dataAnimator->cleanCallCount, 1);
        CORRADE_COMPARE(layer.advanceDataAnimationsCallCount, 1);
    }
    if(data.styleAnimations) {
        CORRADE_COMPARE(styleAnimator->cleanCallCount, 1);
        CORRADE_COMPARE(layer.advanceStyleAnimationsCallCount, 1);
    }

    /* If NeedsNodeClean was set, a clean() call then performs clean of
       animations attached to invalid nodes */
    if(data.clean && (data.expectedCleanAfterAnimation || data.noOp)) {
        {
            CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));
            ui.clean();
        }
        CORRADE_COMPARE(ui.state(), data.expectedInitialState|UserInterfaceState::NeedsNodeUpdate|(data.nodeAnimations1 || data.nodeAnimations2 || data.dataAnimations ?
            UserInterfaceState::NeedsAnimationAdvance : UserInterfaceStates{}));
        CORRADE_COMPARE(layer.cleanCallCount, 1 + data.expectedCleanAfterAnimation);
        CORRADE_COMPARE(animator.cleanCallCount, 0);
        CORRADE_COMPARE(animator.advanceCallCount, data.runningAnimation ? 2 : 0);
        CORRADE_COMPARE(ui.isHandleValid(node1), data.expectedNode1Valid);
        CORRADE_COMPARE(ui.isHandleValid(node2), data.expectedNode2Valid);
        if(data.nodeAnimations1) {
            CORRADE_COMPARE(nodeAnimator1->cleanCallCount, 1 + data.expectedCleanAfterAnimation);
            CORRADE_COMPARE(ui.isHandleValid(nodeAnimation1), data.expectedNode2Valid);
        }
        if(data.nodeAnimations2) {
            CORRADE_COMPARE(nodeAnimator2->cleanCallCount, 1 + data.expectedCleanAfterAnimation);
            CORRADE_COMPARE(ui.isHandleValid(nodeAnimation2), data.expectedNode1Valid);
        }
        if(data.dataAnimations) {
            CORRADE_COMPARE(dataAnimator->cleanCallCount, 1 + data.expectedCleanAfterAnimation);
            CORRADE_COMPARE(ui.isHandleValid(dataAnimation1), data.expectedNode1Valid);
            CORRADE_COMPARE(ui.isHandleValid(dataAnimation2), data.expectedNode2Valid);
        }
        if(data.styleAnimations) {
            CORRADE_COMPARE(styleAnimator->cleanCallCount, 1 + data.expectedCleanAfterAnimation);
            CORRADE_COMPARE(ui.isHandleValid(styleAnimation2), data.expectedNode1Valid);
            CORRADE_COMPARE(ui.isHandleValid(styleAnimation1), data.expectedNode2Valid);
        }
    }

    /* An update() then performs appropriate update in the layer */
    {
        CORRADE_ITERATION(Utility::format("{}:{}", __FILE__, __LINE__));

        bool expectedNodesValid[]{
            data.expectedNode1Valid, data.expectedNode2Valid
        };
        layer.expectedState = data.expectedLayerUpdateState;
        layer.expectedNodeOffsets = data.expectedNodeOffsetsLayer;
        layer.expectedNodesEnabled = data.expectedNodesEnabledLayer;
        layer.expectedClipRectOffsets = data.expectedClipRectOffsetsLayer;
        layer.expectedNodesValid = expectedNodesValid;

        ui.update();
    }
    CORRADE_COMPARE(ui.state(), data.expectedInitialState|(data.nodeAnimations1 || data.nodeAnimations2 || data.dataAnimations || data.styleAnimations ?
        UserInterfaceState::NeedsAnimationAdvance : UserInterfaceStates{}));
    CORRADE_COMPARE(layer.cleanCallCount, 1 + data.expectedCleanAfterAnimation);
    CORRADE_COMPARE(layer.updateCallCount, data.expectedLayerUpdateState ? 2 : 1);
    CORRADE_COMPARE(animator.cleanCallCount, 0);
    CORRADE_COMPARE(ui.isHandleValid(node1), data.expectedNode1Valid);
    CORRADE_COMPARE(ui.isHandleValid(node2), data.expectedNode2Valid);
    CORRADE_COMPARE(ui.isHandleValid(data1), data.expectedNode1Valid);
    CORRADE_COMPARE(ui.isHandleValid(data2), data.expectedNode2Valid);
    if(data.nodeAnimations1) {
        CORRADE_COMPARE(nodeAnimator1->cleanCallCount, 1 + data.expectedCleanAfterAnimation);
        CORRADE_COMPARE(ui.isHandleValid(nodeAnimation1), data.expectedNode2Valid);
    }
    if(data.nodeAnimations2) {
        CORRADE_COMPARE(nodeAnimator2->cleanCallCount, 1 + data.expectedCleanAfterAnimation);
        CORRADE_COMPARE(ui.isHandleValid(nodeAnimation2), data.expectedNode1Valid);
    }
    if(data.dataAnimations) {
        CORRADE_COMPARE(dataAnimator->cleanCallCount, 1 + data.expectedCleanAfterAnimation);
        CORRADE_COMPARE(ui.isHandleValid(dataAnimation1), data.expectedNode1Valid);
        CORRADE_COMPARE(ui.isHandleValid(dataAnimation2), data.expectedNode2Valid);
    }
    if(data.styleAnimations) {
        CORRADE_COMPARE(styleAnimator->cleanCallCount, 1 + data.expectedCleanAfterAnimation);
        CORRADE_COMPARE(ui.isHandleValid(styleAnimation2), data.expectedNode1Valid);
        CORRADE_COMPARE(ui.isHandleValid(styleAnimation1), data.expectedNode2Valid);
    }
}

void AbstractUserInterfaceTest::statePropagateFromLayers() {
    auto&& data = StatePropagateFromLayersData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

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
        using AbstractLayer::create;
        using AbstractLayer::remove;

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

    /* LayerState::Needs*DataUpdate on a removed layer isn't considered (well,
       because the instance is gone), and the layer without an instance is
       skipped. The "works correctly" aspect can't really be observed, we can
       only check that it doesn't crash. */
    ui.layer(layerRemoved).setNeedsUpdate(data.state);
    ui.removeLayer(layerRemoved);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate);

    ui.update();
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

    /* It also shouldn't stop at those, states after those get checked as well */
    ui.layer(layer1).setNeedsUpdate(data.state);
    CORRADE_COMPARE(ui.layer(layer1).state(), data.state);
    CORRADE_COMPARE(ui.layer(layer2).state(), LayerStates{});
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataUpdate);

    /* And updating should reset all of them again */
    ui.update();
    CORRADE_COMPARE(ui.layer(layer1).state(), LayerStates{});
    CORRADE_COMPARE(ui.layer(layer2).state(), LayerStates{});
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

    /* Creating a data results in NeedsDataUpdate on the layer */
    DataHandle data1 = ui.layer<Layer>(layer1).create();
    DataHandle data2 = ui.layer<Layer>(layer2).create();
    CORRADE_COMPARE(ui.layer(layer1).state(), LayerState::NeedsDataUpdate);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataUpdate);

    /* Attaching results in NeedsAttachmentUpdate in addition, plus extra
       states on the layer itself */
    ui.layer(layer1).attach(data1, node);
    CORRADE_COMPARE(ui.layer(layer1).state(), LayerState::NeedsAttachmentUpdate|LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsNodeOffsetSizeUpdate|LayerState::NeedsDataUpdate);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate);

    /* Hiding a node will set a UI-wide NeedsNodeUpdate flag */
    ui.addNodeFlags(node, NodeFlag::Hidden);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);

    /* Having the UI marked with NeedsNodeUpdate shouldn't prevent the
       NeedsDataClean from a later layer from being propagated to the UI-wide
       state */
    ui.layer<Layer>(layer2).remove(data2);
    CORRADE_COMPARE(ui.layer(layer2).state(), LayerState::NeedsDataUpdate|LayerState::NeedsDataClean);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate|UserInterfaceState::NeedsDataClean);
}

void AbstractUserInterfaceTest::statePropagateFromLayouters() {
    /* Tests more complex behavior of state propagation that isn't checked in
       the state() case above */

    /* Event/framebuffer scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};

    /*LayouterHandle layouterWithoutInstance =*/ ui.createLayer();
    LayouterHandle layouterRemoved = ui.createLayouter();
    LayouterHandle layouter1 = ui.createLayouter();
    LayouterHandle layouter2 = ui.createLayouter();

    struct Layouter: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::add;
        using AbstractLayouter::remove;

        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const  Containers::StridedArrayView1D<Vector2>&) override {}
    };
    ui.setLayouterInstance(Containers::pointer<Layouter>(layouterRemoved));
    ui.setLayouterInstance(Containers::pointer<Layouter>(layouter1));
    ui.setLayouterInstance(Containers::pointer<Layouter>(layouter2));
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

    /* Create a node for using later and make the state empty again */
    NodeHandle node = ui.createNode({}, {});
    ui.update();
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

    /* LayouterState::NeedsUpdate on a removed layouter isn't considered (well,
       because the instance is gone), and the layouter without an instance is
       skipped. The "works correctly" aspect can't really be observed, we can
       only check that it doesn't crash. */
    ui.layouter(layouterRemoved).setNeedsUpdate();
    ui.removeLayouter(layouterRemoved);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsLayoutAssignmentUpdate);

    ui.update();
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

    /* It also shouldn't stop at those, states after those get checked as well */
    ui.layouter(layouter2).setNeedsUpdate();
    CORRADE_COMPARE(ui.layouter(layouter1).state(), LayouterStates{});
    CORRADE_COMPARE(ui.layouter(layouter2).state(), LayouterState::NeedsUpdate);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsLayoutUpdate);

    /* And updating should reset all of them again. It should do that even
       though there are no layouts to actually update. */
    ui.update();
    CORRADE_COMPARE(ui.layouter(layouter1).state(), LayouterStates{});
    CORRADE_COMPARE(ui.layouter(layouter2).state(), LayouterStates{});
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

    /* Creating a layout results in NeedsAssignmentUpdate */
    LayoutHandle layout1 = ui.layouter<Layouter>(layouter1).add(node);
    /*LayoutHandle layout2 =*/ ui.layouter<Layouter>(layouter2).add(node);
    CORRADE_COMPARE(ui.layouter(layouter1).state(), LayouterState::NeedsAssignmentUpdate);
    CORRADE_COMPARE(ui.layouter(layouter2).state(), LayouterState::NeedsAssignmentUpdate);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsLayoutAssignmentUpdate);

    /* And updating should reset all of them again */
    ui.update();
    CORRADE_COMPARE(ui.layouter(layouter1).state(), LayouterStates{});
    CORRADE_COMPARE(ui.layouter(layouter2).state(), LayouterStates{});
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

    /* Removing a layout results in NeedsAssignmentUpdate again */
    ui.layouter<Layouter>(layouter1).remove(layout1);
    CORRADE_COMPARE(ui.layouter(layouter1).state(), LayouterState::NeedsAssignmentUpdate);
    CORRADE_COMPARE(ui.layouter(layouter2).state(), LayouterStates{});
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsLayoutAssignmentUpdate);
}

void AbstractUserInterfaceTest::statePropagateFromAnimators() {
    /* Compared to statePropagateFromLayers() and statePropagateFromAnimators()
       there (currently) isn't anything that would affect clean() or update(),
       it's just a notification to run advanceAnimations() */

    /* Event/framebuffer scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};

    struct GenericAnimator: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;
        using AbstractGenericAnimator::create;

        AnimatorFeatures doFeatures() const override { return {}; }
        void doAdvance(Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
    };
    /*AnimatorHandle animatorWithoutInstance =*/ ui.createAnimator();
    GenericAnimator& animatorRemoved = ui.setGenericAnimatorInstance(Containers::pointer<GenericAnimator>(ui.createAnimator()));
    /*GenericAnimator& animator1 =*/ ui.setGenericAnimatorInstance(Containers::pointer<GenericAnimator>(ui.createAnimator()));
    GenericAnimator& animator2 = ui.setGenericAnimatorInstance(Containers::pointer<GenericAnimator>(ui.createAnimator()));

    /* AnimatorState::NeedsAdvance on a removed animator isn't considered
       (well, because the instance is gone), and the animator without an
       instance is skipped. The "works correctly" aspect can't really be
       observed, we can only check that it doesn't crash. */
    animatorRemoved.create(10_nsec, 10_nsec);
    ui.removeAnimator(animatorRemoved.handle());
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

    /* Create an animation in the second animator. It shouldn't stop at the
       first animator when checking the state. */
    animator2.create(10_nsec, 10_nsec);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsAnimationAdvance);
}

void AbstractUserInterfaceTest::draw() {
    auto&& data = DrawData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* windowSize isn't used for anything here */
    AbstractUserInterface ui{{200.0f, 300.0f}, {20.0f, 30.0f}, {400, 500}};

    struct Renderer: AbstractRenderer {
        RendererFeatures doFeatures() const override { return {}; }
        void doSetupFramebuffers(const Vector2i&) override {}
        void doTransition(RendererTargetState, RendererTargetState, RendererDrawStates, RendererDrawStates) override {}
    };
    ui.setRendererInstance(Containers::pointer<Renderer>());

    struct Layer: AbstractLayer {
        explicit Layer(LayerHandle handle, LayerFeatures features): AbstractLayer{handle}, features{features} {}

        using AbstractLayer::create;

        LayerFeatures doFeatures() const override { return features; }

        void doSetSize(const Vector2& size, const Vector2i& framebufferSize) override {
            CORRADE_ITERATION(handle());
            ++setSizeCallCount;
            CORRADE_COMPARE(size, (Vector2{200.0f, 300.0f}));
            CORRADE_COMPARE(framebufferSize, (Vector2i{400, 500}));
        }

        void doComposite(AbstractRenderer&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, std::size_t, std::size_t) override {
            CORRADE_FAIL("This shouldn't be called");
        }

        void doUpdate(LayerStates states, const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes, const Containers::StridedArrayView1D<const Vector2>& compositeRectOffsets, const Containers::StridedArrayView1D<const Vector2>& compositeRectSizes) override {
            CORRADE_ITERATION(handle());
            CORRADE_COMPARE(states, LayerState::NeedsNodeOrderUpdate|LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsNodeOffsetSizeUpdate|LayerState::NeedsDataUpdate);
            /* doSetSize() should have been called exactly once at this point
               if this layer draws, and not at all if it doesn't */
            CORRADE_COMPARE(setSizeCallCount, features & LayerFeature::Draw ? 1 : 0);
            CORRADE_COMPARE_AS(dataIds,
                expectedDataIds,
                TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(clipRectIds,
                expectedClipRectIdsDataCounts.slice(&Containers::Pair<UnsignedInt, UnsignedInt>::first),
                TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(clipRectDataCounts,
                expectedClipRectIdsDataCounts.slice(&Containers::Pair<UnsignedInt, UnsignedInt>::second),
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
            CORRADE_COMPARE_AS(nodesEnabled,
                expectedNodesEnabled.sliceBit(0),
                TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(clipRectOffsets,
                expectedClipRectOffsetsSizes.slice(&Containers::Pair<Vector2, Vector2>::first),
                TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(clipRectSizes,
                expectedClipRectOffsetsSizes.slice(&Containers::Pair<Vector2, Vector2>::second),
                TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(compositeRectOffsets,
                Containers::ArrayView<Vector2>{},
                TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(compositeRectSizes,
                Containers::ArrayView<Vector2>{},
                TestSuite::Compare::Container);
            actualDataIds = dataIds;
            actualClipRectIds = clipRectIds;
            actualClipRectDataCounts = clipRectDataCounts;
            actualNodeOffsets = nodeOffsets;
            actualNodeSizes = nodeSizes;
            actualNodesEnabled = nodesEnabled;
            actualClipRectOffsets = clipRectOffsets;
            actualClipRectSizes = clipRectSizes;
            ++*updateCallCount;
        }

        void doDraw(const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, std::size_t offset, std::size_t count, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, std::size_t clipRectOffset, std::size_t clipRectCount, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes) override {
            CORRADE_ITERATION(handle());
            /* doSetSize() should have been called exactly once at this point */
            CORRADE_COMPARE(setSizeCallCount, 1);
            /* The passed views should be exactly the same */
            CORRADE_COMPARE(dataIds.data(), actualDataIds.data());
            CORRADE_COMPARE(dataIds.size(), actualDataIds.size());
            CORRADE_COMPARE(dataIds.stride(), actualDataIds.stride());
            CORRADE_COMPARE(clipRectIds.data(), actualClipRectIds.data());
            CORRADE_COMPARE(clipRectIds.size(), actualClipRectIds.size());
            CORRADE_COMPARE(clipRectIds.stride(), actualClipRectIds.stride());
            CORRADE_COMPARE(clipRectDataCounts.data(), actualClipRectDataCounts.data());
            CORRADE_COMPARE(clipRectDataCounts.size(), actualClipRectDataCounts.size());
            CORRADE_COMPARE(clipRectDataCounts.stride(), actualClipRectDataCounts.stride());
            CORRADE_COMPARE(nodeOffsets.data(), actualNodeOffsets.data());
            CORRADE_COMPARE(nodeOffsets.size(), actualNodeOffsets.size());
            CORRADE_COMPARE(nodeOffsets.stride(), actualNodeOffsets.stride());
            CORRADE_COMPARE(nodeSizes.data(), actualNodeSizes.data());
            CORRADE_COMPARE(nodeSizes.size(), actualNodeSizes.size());
            CORRADE_COMPARE(nodeSizes.stride(), actualNodeSizes.stride());
            CORRADE_COMPARE(nodesEnabled.data(), actualNodesEnabled.data());
            CORRADE_COMPARE(nodesEnabled.offset(), actualNodesEnabled.offset());
            CORRADE_COMPARE(nodesEnabled.size(), actualNodesEnabled.size());
            CORRADE_COMPARE(clipRectOffsets.data(), actualClipRectOffsets.data());
            CORRADE_COMPARE(clipRectOffsets.size(), actualClipRectOffsets.size());
            CORRADE_COMPARE(clipRectOffsets.stride(), actualClipRectOffsets.stride());
            CORRADE_COMPARE(clipRectSizes.data(), actualClipRectSizes.data());
            CORRADE_COMPARE(clipRectSizes.size(), actualClipRectSizes.size());
            CORRADE_COMPARE(clipRectSizes.stride(), actualClipRectSizes.stride());
            arrayAppend(*drawCalls, InPlaceInit, handle(),
                Containers::pair(offset, count),
                Containers::pair(clipRectOffset, clipRectCount));
        }

        LayerFeatures features;
        Containers::StridedArrayView1D<const UnsignedInt> expectedDataIds;
        Containers::StridedArrayView1D<const Containers::Pair<UnsignedInt, UnsignedInt>> expectedClipRectIdsDataCounts;
        Containers::StridedArrayView1D<const Containers::Pair<Vector2, Vector2>> expectedNodeOffsetsSizes;
        Containers::StridedArrayView1D<const bool> expectedNodesEnabled;
        Containers::StridedArrayView1D<const Containers::Pair<Vector2, Vector2>> expectedClipRectOffsetsSizes;
        Int* updateCallCount;
        Int setSizeCallCount = 0;
        Containers::Array<Containers::Triple<LayerHandle,
            Containers::Pair<std::size_t, std::size_t>,
            Containers::Pair<std::size_t, std::size_t>>>* drawCalls;

        Containers::StridedArrayView1D<const UnsignedInt> actualDataIds;
        Containers::StridedArrayView1D<const UnsignedInt> actualClipRectIds;
        Containers::StridedArrayView1D<const UnsignedInt> actualClipRectDataCounts;
        Containers::StridedArrayView1D<const Vector2> actualNodeOffsets;
        Containers::StridedArrayView1D<const Vector2> actualNodeSizes;
        Containers::BitArrayView actualNodesEnabled;
        Containers::StridedArrayView1D<const Vector2> actualClipRectOffsets;
        Containers::StridedArrayView1D<const Vector2> actualClipRectSizes;
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
       8 |          +-|       | top level  |
       9 +------------+-------|          +--------+
      10   +--------+ |layer1 +----------| layer2 |
           | culled | |  only   |        |  only  |
      11   +--------+ +---------+        +--------+ */
    NodeHandle topLevel = ui.createNode({1.0f, 3.0f}, {7.0f, 6.0f});
    NodeHandle left = ui.createNode(topLevel, {1.0f, 2.0f}, {1.0f, 2.0f}, NodeFlag::Clip|NodeFlag::NoEvents);
    NodeHandle right = ui.createNode(topLevel, {3.0f, 1.0f}, {3.0f, 4.0f}, NodeFlag::Clip|NodeFlag::Disabled);
    NodeHandle layer1Only = ui.createNode({5.0f, 9.0f}, {2.0f, 2.0f});
    NodeHandle anotherTopLevel = ui.createNode({6.0f, 5.0f}, {4.0f, 5.0f});
    NodeHandle layer2Only = ui.createNode({9.0f, 9.0f}, {2.0f, 2.0f});
    NodeHandle topLevelNotInOrder = ui.createNode({9.0f, 4.0f}, {2.0f, 3.0f});
    NodeHandle removed = ui.createNode(right, {}, {});
    NodeHandle topLevelHidden = ui.createNode({}, {}, NodeFlag::Hidden);
    NodeHandle culled = ui.createNode(left, {0.0f, 5.0f}, {2.0f, 1.0f});
    /* Overflows right, is clipped; is also clipping itself */
    NodeHandle nested = ui.createNode(right, {1.0f, 2.0f}, {3.0f, 3.0f}, NodeFlag::Clip);

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
        {},                           /* topLevelHidden */
        {},                           /* not in order */
        {},                           /* culled */
        {{5.0f, 6.0f}, {3.0f, 3.0f}}, /* nested */
    };
    bool expectedNodesEnabled[]{
        true,   /* topLevel */
        true,   /* left, is NoEvents but that doesn't affect drawing */
        false,  /* right, is Disabled */
        true,   /* layer1Only */
        true,   /* anotherTopLevel */
        true,   /* layer2Only */
        false,  /* removed */
        false,  /* topLevelHidden, is Hidden */
        false,  /* not in order */
        false,  /* culled */
        false,  /* nested, is a child of right, which is Disabled */
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

    /* The commented out variables are either removed, culled or hidden, thus
       not referenced from any doUpdate() or doDraw() below */
    DataHandle leftData2 = layer1Instance->create(left);
    DataHandle leftData1 = layer2Instance->create(left);
    DataHandle leftData3 = layer1Instance->create(left);
    DataHandle anotherTopLevelData1 = layer1Instance->create(anotherTopLevel);
    DataHandle anotherTopLevelData2 = layer2Instance->create(anotherTopLevel);
    DataHandle anotherTopLevelData3 = layer3Instance->create(anotherTopLevel);
    DataHandle anotherTopLevelData4 = layer2Instance->create(anotherTopLevel);
    DataHandle topLevelData = layer3Instance->create(topLevel);
    /*DataHandle culledData =*/ layer2Instance->create(culled);
    DataHandle nestedData = layer2Instance->create(nested);
    /*DataHandle topLevelNotInOrderData =*/ layer2Instance->create(topLevelNotInOrder);
    /*DataHandle removedData =*/ layer1Instance->create(removed);
    /*DataHandle topLevelHiddenData =*/ layer2Instance->create(topLevelHidden);
    DataHandle rightData1 = layer3Instance->create(right);
    DataHandle rightData2 = layer2Instance->create(right);
    DataHandle layer1OnlyData = layer1Instance->create(layer1Only);
    DataHandle layer2OnlyData = layer2Instance->create(layer2Only);

    /* These follow the node nesting order and then the order in which the
       data get attached below */
    UnsignedInt expectedLayer1DataIds[]{
        /* anotherTopLevel is reordered as first */
        dataHandleId(anotherTopLevelData1),
        /* Data belonging to topLevel are after it */
        dataHandleId(leftData2),
        dataHandleId(leftData3),
        /* removedData not here as the containing node is removed */
        dataHandleId(layer1OnlyData),
    };
    UnsignedInt expectedLayer2DataIds[]{
        /* anotherTopLevel */
        dataHandleId(anotherTopLevelData2),
        dataHandleId(anotherTopLevelData4),
        /* topLevel */
        dataHandleId(leftData1),
        dataHandleId(rightData2),
        dataHandleId(nestedData),
        /* Nothing for topLevelNotInOrderData and culledData as they're not
           visible */
        dataHandleId(layer2OnlyData),
    };
    UnsignedInt expectedLayer3DataIds[]{
        /* anotherTopLevel */
        dataHandleId(anotherTopLevelData3),
        /* topLevel */
        dataHandleId(topLevelData),
        dataHandleId(rightData1),
    };
    Containers::Pair<Vector2, Vector2> expectedClipRectOffsetsSizes[]{
        /* anotherTopLevel */
        {{}, {}},                               /* clip rect ID 0 */
        /* topLevel */
        {{}, {}},                               /* clip rect ID 1 */
            /* left */
            {{2.0f, 5.0f}, {1.0f, 2.0f}},       /* clip rect ID 2 */
            /* right */
            {{4.0f, 4.0f}, {3.0f, 4.0f}},       /* clip rect ID 3 */
                /* nested */
                {{5.0f, 6.0f}, {2.0f, 2.0f}},   /* clip rect ID 4 */
        /* layer1Only */
        {{}, {}},                               /* clip rect ID 5 */
        /* layer2Only */
        {{}, {}},                               /* clip rect ID 6 */
    };
    /* In each of these, the total count should match the size of the
       expectedLayer*DataIds arrays */
    Containers::Pair<UnsignedInt, UnsignedInt> expectedLayer1ClipRectIdsDataCounts[]{
        /* anotherTopLevel */
        {0, 1},
        /* left */
        {2, 2},
        /* layer1Only */
        {5, 1}
    };
    Containers::Pair<UnsignedInt, UnsignedInt> expectedLayer2ClipRectIdsDataCounts[]{
        /* anotherTopLevel */
        {0, 2},
        /* left */
        {2, 1},
        /* right */
        {3, 1},
        /* nested */
        {4, 1},
        /* layer2Only */
        {6, 1},
    };
    Containers::Pair<UnsignedInt, UnsignedInt> expectedLayer3ClipRectIdsDataCounts[]{
        /* anotherTopLevel */
        {0, 1},
        /* topLevel */
        {1, 1},
        /* right */
        {3, 1}
    };

    layer1Instance->expectedDataIds = expectedLayer1DataIds;
    layer2Instance->expectedDataIds = expectedLayer2DataIds;
    layer3Instance->expectedDataIds = expectedLayer3DataIds;
    layer1Instance->expectedNodeOffsetsSizes = expectedNodeOffsetsSizes;
    layer2Instance->expectedNodeOffsetsSizes = expectedNodeOffsetsSizes;
    layer3Instance->expectedNodeOffsetsSizes = expectedNodeOffsetsSizes;
    layer1Instance->expectedNodesEnabled = expectedNodesEnabled;
    layer2Instance->expectedNodesEnabled = expectedNodesEnabled;
    layer3Instance->expectedNodesEnabled = expectedNodesEnabled;
    layer1Instance->expectedClipRectIdsDataCounts = expectedLayer1ClipRectIdsDataCounts;
    layer2Instance->expectedClipRectIdsDataCounts = expectedLayer2ClipRectIdsDataCounts;
    layer3Instance->expectedClipRectIdsDataCounts = expectedLayer3ClipRectIdsDataCounts;
    layer1Instance->expectedClipRectOffsetsSizes = expectedClipRectOffsetsSizes;
    layer2Instance->expectedClipRectOffsetsSizes = expectedClipRectOffsetsSizes;
    layer3Instance->expectedClipRectOffsetsSizes = expectedClipRectOffsetsSizes;
    Int layer1UpdateCallCount = 0;
    Int layer2UpdateCallCount = 0;
    Int layer3UpdateCallCount = 0;
    layer1Instance->updateCallCount = &layer1UpdateCallCount;
    layer2Instance->updateCallCount = &layer2UpdateCallCount;
    layer3Instance->updateCallCount = &layer3UpdateCallCount;
    Containers::Array<Containers::Triple<LayerHandle,
            Containers::Pair<std::size_t, std::size_t>,
            Containers::Pair<std::size_t, std::size_t>>> drawCalls;
    layer1Instance->drawCalls = &drawCalls;
    layer2Instance->drawCalls = &drawCalls;
    layer3Instance->drawCalls = &drawCalls;
    ui.setLayerInstance(Utility::move(layer1Instance));
    ui.setLayerInstance(Utility::move(layer2Instance));
    ui.setLayerInstance(Utility::move(layer3Instance));

    ui.setNodeOrder(anotherTopLevel, topLevel);
    ui.clearNodeOrder(topLevelNotInOrder);
    ui.removeNode(removed);
    CORRADE_COMPARE(ui.layer(layer1).usedCount(), 5);
    CORRADE_COMPARE(ui.layer(layer2).usedCount(), 9);
    CORRADE_COMPARE(ui.layer(layer3).usedCount(), 3);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeClean);
    CORRADE_COMPARE(layer1UpdateCallCount, 0);
    CORRADE_COMPARE(layer2UpdateCallCount, 0);
    CORRADE_COMPARE(layer3UpdateCallCount, 0);

    if(data.clean) {
        ui.clean();
        CORRADE_COMPARE(ui.layer(layer1).usedCount(), 4);
        CORRADE_COMPARE(ui.layer(layer2).usedCount(), 9);
        CORRADE_COMPARE(ui.layer(layer3).usedCount(), 3);
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);
        CORRADE_COMPARE(layer1UpdateCallCount, 0);
        CORRADE_COMPARE(layer2UpdateCallCount, 0);
        CORRADE_COMPARE(layer3UpdateCallCount, 0);
    }

    /* update() should call clean() only if needed */
    if(data.update) {
        ui.update();
        CORRADE_COMPARE(ui.layer(layer1).usedCount(), 4);
        CORRADE_COMPARE(ui.layer(layer2).usedCount(), 9);
        CORRADE_COMPARE(ui.layer(layer3).usedCount(), 3);
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
        CORRADE_COMPARE(layer1UpdateCallCount, 1);
        CORRADE_COMPARE(layer2UpdateCallCount, 1);
        CORRADE_COMPARE(layer3UpdateCallCount, 1);
    }

    /* draw() should call update() and clean() only if needed */
    ui.draw();
    CORRADE_COMPARE(ui.layer(layer1).usedCount(), 4);
    CORRADE_COMPARE(ui.layer(layer2).usedCount(), 9);
    CORRADE_COMPARE(ui.layer(layer3).usedCount(), 3);
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(layer1UpdateCallCount, 1);
    CORRADE_COMPARE(layer2UpdateCallCount, 1);
    CORRADE_COMPARE(layer3UpdateCallCount, 1);
    CORRADE_COMPARE_AS(drawCalls, (Containers::arrayView<Containers::Triple<LayerHandle,
            Containers::Pair<std::size_t, std::size_t>,
            Containers::Pair<std::size_t, std::size_t>>>({
        /* anotherTopLevel rendered first */
            /* first data from expectedLayer1Data, first rect from
               expectedLayer1ClipRectIdsDataCounts */
            {layer1, {0, 1}, {0, 1}},
            /* first two data from expectedLayer2Data, first rect from
               expectedLayer2ClipRectIdsDataCounts */
            {layer2, {0, 2}, {0, 1}},
        /* then topLevel */
            /* next two data from expectedLayer1Data, second rect from
               expectedLayer1ClipRectIdsDataCounts */
            {layer1, {1, 2}, {1, 1}},
            /* and then next three data from expectedLayer2Data, second, third
               and fourth rect from expectedLayer2ClipRectIdsDataCounts */
            {layer2, {2, 3}, {1, 3}},
        /* then layer1Only, with only data from layer 1 and last rect from
           expectedLayer1ClipRectIdsDataCounts */
            {layer1, {3, 1}, {2, 1}},
        /* then layer2Only, with only data from layer 2 and last rect from
           expectedLayer2ClipRectIdsDataCounts */
            {layer2, {5, 1}, {4, 1}},
        /* layer 3 doesn't have LayerFeature::Draw, so draw() shouldn't be
           called with anything for it */
    })), TestSuite::Compare::Container);
}

void AbstractUserInterfaceTest::drawComposite() {
    /* windowSize isn't used for anything here */
    AbstractUserInterface ui{{200.0f, 300.0f}, {20.0f, 30.0f}, {400, 500}};

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    struct Renderer: AbstractRenderer {
        RendererFeatures doFeatures() const override {
            return RendererFeature::Composite;
        }
        void doSetupFramebuffers(const Vector2i& size) override {
            CORRADE_COMPARE(size, (Vector2i{400, 500}));
            ++setupFramebufferCallCount;
        }
        void doTransition(RendererTargetState, RendererTargetState, RendererDrawStates, RendererDrawStates) override {}

        Int setupFramebufferCallCount;
    };
    Renderer& renderer = ui.setRendererInstance(Containers::pointer<Renderer>());

    enum {
        Composite,
        Draw
    };

    struct Layer: AbstractLayer {
        explicit Layer(LayerHandle handle, LayerFeatures features): AbstractLayer{handle}, features{features} {}

        using AbstractLayer::create;

        LayerFeatures doFeatures() const override { return features; }

        void doSetSize(const Vector2& size, const Vector2i& framebufferSize) override {
            CORRADE_ITERATION(handle());
            ++setSizeCallCount;
            CORRADE_COMPARE(size, (Vector2{200.0f, 300.0f}));
            CORRADE_COMPARE(framebufferSize, (Vector2i{400, 500}));
        }

        void doUpdate(LayerStates, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::BitArrayView, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>& compositeRectOffsets, const Containers::StridedArrayView1D<const Vector2>& compositeRectSizes) override {
            CORRADE_ITERATION(handle());
            ++updateCallCount;
            CORRADE_COMPARE_AS(compositeRectOffsets,
                expectedCompositeOffsetsSizes
                    .slice(&Containers::Pair<Vector2, Vector2>::first),
                TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(compositeRectSizes,
                expectedCompositeOffsetsSizes
                    .slice(&Containers::Pair<Vector2, Vector2>::second),
                TestSuite::Compare::Container);
            actualCompositeRectOffsets = compositeRectOffsets;
            actualCompositeRectSizes = compositeRectSizes;
        }

        void doComposite(AbstractRenderer& renderer, const Containers::StridedArrayView1D<const Vector2>& compositeRectOffsets, const Containers::StridedArrayView1D<const Vector2>& compositeRectSizes, std::size_t offset, std::size_t count) override {
            CORRADE_ITERATION(handle());
            /* doSetSize() should have been called exactly once at this point,
               doUpdate() as well */
            CORRADE_COMPARE(setSizeCallCount, 1);
            CORRADE_COMPARE(updateCallCount, 1);

            CORRADE_VERIFY(features & LayerFeature::Composite);
            CORRADE_COMPARE(renderer.framebufferSize(), (Vector2i{400, 500}));

            CORRADE_COMPARE(compositeRectOffsets.data(), actualCompositeRectOffsets.data());
            CORRADE_COMPARE(compositeRectOffsets.size(), actualCompositeRectOffsets.size());
            CORRADE_COMPARE(compositeRectOffsets.stride(), actualCompositeRectOffsets.stride());
            CORRADE_COMPARE(compositeRectSizes.data(), actualCompositeRectSizes.data());
            CORRADE_COMPARE(compositeRectSizes.size(), actualCompositeRectSizes.size());
            CORRADE_COMPARE(compositeRectSizes.stride(), actualCompositeRectSizes.stride());

            arrayAppend(*compositeDrawCalls, InPlaceInit, handle(), Composite, Containers::pair(offset, count));
        }

        void doDraw(const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, std::size_t offset, std::size_t count, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, std::size_t, std::size_t, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::BitArrayView, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&) override {
            CORRADE_ITERATION(handle());
            /* doSetSize() should have been called exactly once at this point */
            CORRADE_COMPARE(setSizeCallCount, 1);
            CORRADE_COMPARE_AS(dataIds,
                expectedDataIds,
                TestSuite::Compare::Container);
            arrayAppend(*compositeDrawCalls, InPlaceInit, handle(), Draw,
                Containers::pair(offset, count));
        }

        LayerFeatures features;
        Containers::StridedArrayView1D<const Containers::Pair<Vector2, Vector2>> expectedCompositeOffsetsSizes;
        Containers::StridedArrayView1D<const UnsignedInt> expectedDataIds;
        Int setSizeCallCount = 0;
        Int updateCallCount = 0;
        Containers::Array<Containers::Triple<LayerHandle, Int,
            Containers::Pair<std::size_t, std::size_t>>>* compositeDrawCalls;

        Containers::StridedArrayView1D<const Vector2> actualCompositeRectOffsets;
        Containers::StridedArrayView1D<const Vector2> actualCompositeRectSizes;
    };

    /* Only drawing layers used here, non-drawing layers and their exclusion
       from the draw list is tested enough in draw() above */
    Layer& layer1Compositing = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), LayerFeature::Draw|LayerFeature::Composite));
    Layer& layer2 = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), LayerFeature::Draw|LayerFeature::Event));
    Layer& layer3Compositing = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), LayerFeature::Draw|LayerFeature::Composite|LayerFeature::Event));

    /* First top-level node, with one compositing layer and one
       non-compositing */
    NodeHandle topLevelNode1Composited = ui.createNode({20, 30}, {50, 60});
    /* Goes out of the parent node rectangle, should be included in the
       compositing rect */
    NodeHandle node1Composited = ui.createNode(topLevelNode1Composited, {30, -10}, {40, 30});
    DataHandle node1Data1Composited = layer3Compositing.create(node1Composited);
    DataHandle node1Data2 = layer2.create(node1Composited);

    /* Second top-level node, containing two composites and two draws from two
       layers, with two data from one layer and one from the other. This
       composition could theoretically be merged with the first top-level node
       as the two are non-overlapping. */
    /** @todo once the non-overlapping top-level node draw merging is
        implemented, add a second instance to this test case that verifies with
        an overlap */
    NodeHandle topLevelNode2Composited = ui.createNode({100, 150}, {20, 30});
    /* Goes out of the parent node rectangle, but the parent doesn't clip so it
       should be also included in the compositing rect */
    NodeHandle node2CompositedClipping = ui.createNode(topLevelNode2Composited, {5, 5}, {20, 30}, NodeFlag::Clip);
    /* Goes out of the parent node rectangle and the parent doesn't clip so it
       shouldn't be included in the compositing rect */
    NodeHandle node2CompositedClipped = ui.createNode(node2CompositedClipping, {10, 20}, {500, 700});
    DataHandle node2Data1Composited = layer1Compositing.create(node2CompositedClipped);
    DataHandle node2Data2Composited = layer3Compositing.create(node2CompositedClipped);
    DataHandle node2Data3Composited = layer1Compositing.create(node2CompositedClipped);
    DataHandle node2Data4Composited = layer1Compositing.create(node2CompositedClipping);

    /* Third top-level node, this draw should not be preceded by any composite
       call */
    NodeHandle topLevelNode3 = ui.createNode({100, 30}, {40, 60});
    DataHandle node3Data = layer2.create(topLevelNode3);

    /* Fourth top-level node, compositing layer 1 again. This *has to* happen
       independently of the previous composition  */
    NodeHandle topLevelNode4 = ui.createNode({20, 150}, {30, 20});
    DataHandle node4DataComposited = layer1Compositing.create(topLevelNode4);

    UnsignedInt expectedLayer1DataIds[]{
        dataHandleId(node2Data4Composited), /* First because it's assigned to
                                               parent node of the rest */
        dataHandleId(node2Data1Composited),
        dataHandleId(node2Data3Composited),
        dataHandleId(node4DataComposited),
    };
    UnsignedInt expectedLayer2DataIds[]{
        dataHandleId(node1Data2),
        dataHandleId(node3Data),
    };
    UnsignedInt expectedLayer3DataIds[]{
        dataHandleId(node1Data1Composited),
        dataHandleId(node2Data2Composited),
    };
    Containers::Pair<Vector2, Vector2> expectedLayer1CompositeOffsetsSizes[]{
        /* Second top-level node, clipped. First child is the clip rect
           itself. */
        {{105, 155}, {20, 30}},
        /* Second child twice because two data from this layer are assigned to
           it. The clip rect is {105, 155} to {125, 185} (while the parent is
           just to {120, 170}) and the node is {115, 175} to {515, 875}. */
        /** @todo the duplicates should be prunned when calculating the
            rects */
        {{115, 175}, {10, 10}},
        {{115, 175}, {10, 10}},
        /* Fourth top-level node -- same offset + size as the node itself */
        {{20, 150}, {30, 20}}
    };
    /* Layer 2 is not compositing */
    Containers::Pair<Vector2, Vector2> expectedLayer3CompositeOffsetsSizes[]{
        /* First top-level node. The clip rect is {50, 20} to {90, 50}, and the
           data is assigned directly to the node containing it. */
        {{50, 20}, {40, 30}},
        /* Second top-level node, same as in the case above, just data from a
           different layer */
        {{115, 175}, {10, 10}},
    };

    layer1Compositing.expectedDataIds = expectedLayer1DataIds;
    layer2.expectedDataIds = expectedLayer2DataIds;
    layer3Compositing.expectedDataIds = expectedLayer3DataIds;
    layer1Compositing.expectedCompositeOffsetsSizes = expectedLayer1CompositeOffsetsSizes;
    layer3Compositing.expectedCompositeOffsetsSizes = expectedLayer3CompositeOffsetsSizes;
    Containers::Array<Containers::Triple<LayerHandle, Int,
        Containers::Pair<std::size_t, std::size_t>>> compositeDrawCalls;
    layer1Compositing.compositeDrawCalls = &compositeDrawCalls;
    layer2.compositeDrawCalls = &compositeDrawCalls;
    layer3Compositing.compositeDrawCalls = &compositeDrawCalls;

    /* draw() should call composite() before draw() for draw calls from layers
       that advertise compositing */
    ui.draw();
    CORRADE_COMPARE(renderer.setupFramebufferCallCount, 1);
    CORRADE_COMPARE(layer1Compositing.setSizeCallCount, 1);
    CORRADE_COMPARE(layer2.setSizeCallCount, 1);
    CORRADE_COMPARE(layer3Compositing.setSizeCallCount, 1);
    CORRADE_COMPARE_AS(compositeDrawCalls, (Containers::arrayView<Containers::Triple<LayerHandle, Int,
            Containers::Pair<std::size_t, std::size_t>>>({
        /* First top-level node, non-composited data and then composited */
        {layer2.handle(), Draw, {0, 1}},
        /* The composite right now uses a range of rectangles that matches the
           range of data passed to draw. No deduplication or overlap handling
           anywhere so far. */
        {layer3Compositing.handle(), Composite, {0, 1}},
        {layer3Compositing.handle(), Draw, {0, 1}},
        /* Second top-level node, a composition with two data and then a
           composition with one data */
        /** @todo this composition could be merged with the above when
            non-overlapping top-level node draw merging is implemented */
        {layer1Compositing.handle(), Composite, {0, 3}},
        {layer1Compositing.handle(), Draw, {0, 3}},
        {layer3Compositing.handle(), Composite, {1, 1}},
        {layer3Compositing.handle(), Draw, {1, 1}},
        /* Third top-level node, just a draw */
        {layer2.handle(), Draw, {1, 1}},
        /* Fourth top-level node, composite and draw that has to happen in
           isolation because of the other top-level node in between */
        /** @todo or does it? if non-overlapping it also doesn't have to */
        {layer1Compositing.handle(), Composite, {3, 1}},
        {layer1Compositing.handle(), Draw, {3, 1}},
    })), TestSuite::Compare::Container);
}

void AbstractUserInterfaceTest::drawRendererTransitions() {
    AbstractUserInterface ui{{100, 100}};

    Containers::Array<Containers::Triple<Containers::Pair<LayerHandle, Int>, Containers::Pair<RendererTargetState, RendererTargetState>, Containers::Pair<RendererDrawStates, RendererDrawStates>>> called;

    struct Renderer: AbstractRenderer {
        explicit Renderer(Containers::Array<Containers::Triple<Containers::Pair<LayerHandle, Int>, Containers::Pair<RendererTargetState, RendererTargetState>, Containers::Pair<RendererDrawStates, RendererDrawStates>>>& called): _called(called) {}

        RendererFeatures doFeatures() const override {
            return RendererFeature::Composite;
        }
        void doSetupFramebuffers(const Vector2i&) override {}
        void doTransition(RendererTargetState targetStateFrom, RendererTargetState targetStateTo, RendererDrawStates drawStatesFrom, RendererDrawStates drawStatesTo) override {
            arrayAppend(_called, InPlaceInit,
                Containers::Pair<LayerHandle, Int>{},
                Containers::pair(targetStateFrom, targetStateTo),
                Containers::pair(drawStatesFrom, drawStatesTo));
        }

        Containers::Array<Containers::Triple<Containers::Pair<LayerHandle, Int>, Containers::Pair<RendererTargetState, RendererTargetState>, Containers::Pair<RendererDrawStates, RendererDrawStates>>>& _called;
    };
    ui.setRendererInstance(Containers::pointer<Renderer>(called));

    enum {
        Composite,
        Draw
    };

    struct Layer: AbstractLayer {
        explicit Layer(LayerHandle handle, LayerFeatures features, Containers::Array<Containers::Triple<Containers::Pair<LayerHandle, Int>, Containers::Pair<RendererTargetState, RendererTargetState>, Containers::Pair<RendererDrawStates, RendererDrawStates>>>& called): AbstractLayer{handle}, _features{features}, _called(called) {}

        using AbstractLayer::create;

        LayerFeatures doFeatures() const override { return _features; }

        void doComposite(AbstractRenderer&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, std::size_t, std::size_t) override {
            arrayAppend(_called, InPlaceInit,
                Containers::pair(handle(), Int(Composite)),
                Containers::Pair<RendererTargetState, RendererTargetState>{},
                Containers::Pair<RendererDrawStates, RendererDrawStates>{});
        }

        void doDraw(const Containers::StridedArrayView1D<const UnsignedInt>&, std::size_t, std::size_t, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, std::size_t, std::size_t, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, Containers::BitArrayView, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&) override {
            arrayAppend(_called, InPlaceInit,
                Containers::pair(handle(), Int(Draw)),
                Containers::Pair<RendererTargetState, RendererTargetState>{},
                Containers::Pair<RendererDrawStates, RendererDrawStates>{});
        }

        LayerFeatures _features;
        Containers::Array<Containers::Triple<Containers::Pair<LayerHandle, Int>, Containers::Pair<RendererTargetState, RendererTargetState>, Containers::Pair<RendererDrawStates, RendererDrawStates>>>& _called;
    };

    /* Only drawing layers used here, non-drawing layers and their exclusion
       from the draw list is tested enough in draw() above */
    Layer& layerWithScissorAndCompositing = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), LayerFeature::DrawUsesScissor|LayerFeature::Composite, called));
    Layer& layerWithBlendingScissor = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), LayerFeature::DrawUsesScissor|LayerFeature::DrawUsesBlending, called));
    Layer& layerWithNothing = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), LayerFeature::Draw, called));
    Layer& layerWithBlending = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), LayerFeature::DrawUsesBlending, called));

    NodeHandle topLevel = ui.createNode({}, {100, 50});
    NodeHandle topLevelChild = ui.createNode(topLevel, {}, {50, 50});
    NodeHandle topLevelHidden = ui.createNode(topLevel, {}, {50, 50}, NodeFlag::Hidden);
    /* Composited & drawn first, transitioning from Initial, enabling
       scissor for the draw but not compositing */
    layerWithScissorAndCompositing.create(topLevelChild);
    /* These two get drawn second, enabling blending in addition to scissor */
    layerWithBlendingScissor.create(topLevel);
    layerWithBlendingScissor.create(topLevelChild);
    /* This one doesn't get drawn */
    layerWithNothing.create(topLevelHidden);

    NodeHandle anotherTopLevel = ui.createNode({0, 50}, {100, 50});
    /* This one gets drawn third, with no transition */
    layerWithBlendingScissor.create(anotherTopLevel);
    NodeHandle anotherTopLevelChild = ui.createNode(anotherTopLevel, {}, {50, 50});
    /* Drawn fourth, transitioning to Scissor no longer enabled */
    layerWithBlending.create(anotherTopLevelChild);

    NodeHandle thirdTopLevel = ui.createNode({25, 25}, {50, 50});
    /* Composited & drawn fifth, transitioning to only Scissor enabled */
    layerWithScissorAndCompositing.create(thirdTopLevel);
    /* Drawn sixth, transitioning to nothing enabled */
    layerWithNothing.create(thirdTopLevel);
    /* Drawn seventh, transitioning to Blending enabled */
    layerWithBlending.create(thirdTopLevel);

    NodeHandle fifthTopLevel = ui.createNode({0, 0}, {100, 100});
    /* Drawn eighth, with no transition, and then finally to a Final state with
       nothing enabled */
    layerWithBlending.create(fifthTopLevel);

    /* Draw twice, second time the transition will be from Final to Initial */
    for(std::size_t i: {0, 1}) {
        CORRADE_ITERATION(i);

        called = {};
        ui.draw();
        CORRADE_COMPARE_AS(called, (Containers::arrayView<Containers::Triple<Containers::Pair<LayerHandle, Int>, Containers::Pair<RendererTargetState, RendererTargetState>, Containers::Pair<RendererDrawStates, RendererDrawStates>>>({
            /* This transition happens only the second time (see the
               exceptPrefix() at the end) */
            {{}, {RendererTargetState::Final, RendererTargetState::Initial},
                 {{}, {}}},

            {{}, {RendererTargetState::Initial, RendererTargetState::Composite},
                 {{}, {}}},
            {{layerWithScissorAndCompositing.handle(), Composite}, {}, {}},
                                                /* First draw composition */
            {{}, {RendererTargetState::Composite, RendererTargetState::Draw},
                 {{}, RendererDrawState::Scissor}},
            {{layerWithScissorAndCompositing.handle(), Draw}, {}, {}},
                                                            /* First draw */

            {{}, {RendererTargetState::Draw, RendererTargetState::Draw},
                 {RendererDrawState::Scissor, RendererDrawState::Blending|RendererDrawState::Scissor}},
            {{layerWithBlendingScissor.handle(), Draw}, {}, {}},
                                                            /* Second draw */
            {{layerWithBlendingScissor.handle(), Draw}, {}, {}},
                                                            /* Third draw */

            {{}, {RendererTargetState::Draw, RendererTargetState::Draw},
                 {RendererDrawState::Blending|RendererDrawState::Scissor,
                  RendererDrawState::Blending}},
            {{layerWithBlending.handle(), Draw}, {}, {}},   /* Fourth draw */

            {{}, {RendererTargetState::Draw, RendererTargetState::Composite},
                 {RendererDrawState::Blending, {}}},
            {{layerWithScissorAndCompositing.handle(), Composite}, {}, {}},
                                                /* Fifth draw composition */
            {{}, {RendererTargetState::Composite, RendererTargetState::Draw},
                 {{}, RendererDrawState::Scissor}},
            {{layerWithScissorAndCompositing.handle(), Draw}, {}, {}},
                                                            /* Fifth draw */

            {{}, {RendererTargetState::Draw, RendererTargetState::Draw},
                 {RendererDrawState::Scissor, {}}},
            {{layerWithNothing.handle(), Draw}, {}, {}},    /* Sixth draw */

            {{}, {RendererTargetState::Draw, RendererTargetState::Draw},
                 {{}, RendererDrawState::Blending}},
            {{layerWithBlending.handle(), Draw}, {}, {}},   /* Seventh draw */
            {{layerWithBlending.handle(), Draw}, {}, {}},   /* Eighth draw */

            {{}, {RendererTargetState::Draw, RendererTargetState::Final},
                 {RendererDrawState::Blending, {}}},
        })).exceptPrefix(i == 0 ? 1 : 0), TestSuite::Compare::Container);
    }
}

void AbstractUserInterfaceTest::drawEmpty() {
    auto&& data = DrawEmptyData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui{{100, 100}};

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    struct Renderer: AbstractRenderer {
        RendererFeatures doFeatures() const override { return {}; }
        void doSetupFramebuffers(const Vector2i& size) override {
            CORRADE_COMPARE(size, Vector2i{100});
            ++setupFramebufferCallCount;
        }
        void doTransition(RendererTargetState targetStateFrom, RendererTargetState targetStateTo, RendererDrawStates, RendererDrawStates) override {
            CORRADE_COMPARE(targetStateFrom, RendererTargetState::Initial);
            CORRADE_COMPARE(targetStateTo, RendererTargetState::Final);
            ++transitionCallCount;
        }

        Int setupFramebufferCallCount = 0;
        Int transitionCallCount = 0;
    };
    ui.setRendererInstance(Containers::pointer<Renderer>());

    struct Layer: AbstractLayer {
        explicit Layer(LayerHandle handle, bool node): AbstractLayer{handle}, _node{node} {}

        using AbstractLayer::create;

        LayerFeatures doFeatures() const override { return LayerFeature::Draw; }

        void doUpdate(LayerStates state, const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes, const Containers::StridedArrayView1D<const Vector2>& compositeRectOffsets, const Containers::StridedArrayView1D<const Vector2>& compositeRectSizes) override {
            CORRADE_COMPARE(state, LayerState::NeedsDataUpdate|(_node ? LayerState::NeedsNodeOffsetSizeUpdate|LayerState::NeedsNodeOrderUpdate|LayerState::NeedsNodeEnabledUpdate : LayerStates{}));
            CORRADE_COMPARE(dataIds.size(), 0);
            CORRADE_COMPARE(clipRectIds.size(), 0);
            CORRADE_COMPARE(clipRectDataCounts.size(), 0);
            CORRADE_COMPARE(nodeOffsets.size(), _node ? 1 : 0);
            CORRADE_COMPARE(nodeSizes.size(), _node ? 1 : 0);
            CORRADE_COMPARE(nodesEnabled.size(), _node ? 1 : 0);
            CORRADE_COMPARE(clipRectOffsets.size(), 0);
            CORRADE_COMPARE(clipRectSizes.size(), 0);
            CORRADE_COMPARE(compositeRectOffsets.size(), 0);
            CORRADE_COMPARE(compositeRectSizes.size(), 0);
            ++updateCallCount;
        }

        void doDraw(const Containers::StridedArrayView1D<const UnsignedInt>&, std::size_t, std::size_t, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, std::size_t, std::size_t, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, Containers::BitArrayView, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&) override {
            CORRADE_FAIL("This shouldn't be called");
        }

        Int updateCallCount = 0;

        private:
            bool _node;
    };

    Layer *layer1{}, *layer2{};
    if(data.layer1)
        layer1 = &ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), data.node));
    if(data.layer2)
        layer2 = &ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), data.node));

    if(data.node) {
        NodeHandle node = ui.createNode({}, {100, 100});
        if(data.layer1) {
            if(data.attach)
                layer1->create(node);
            /* Without an attachment no update is triggered by default, so
               force it */
            else {
                layer1->setNeedsUpdate(LayerState::NeedsDataUpdate);
            }
        }
        if(data.layer2) {
            if(data.attach)
                layer2->create(node);
            /* Without an attachment no update is triggered by default, so
               force it */
            else {
                layer2->setNeedsUpdate(LayerState::NeedsDataUpdate);
            }
        }
        if(data.hide)
            ui.addNodeFlags(node, NodeFlag::Hidden);
        else
            ui.clearNodeOrder(node);
    } else {
        /* Without a node no update is triggered by default, so force it */
        if(data.layer1)
            layer1->setNeedsUpdate(LayerState::NeedsDataUpdate);
        if(data.layer2)
            layer2->setNeedsUpdate(LayerState::NeedsDataUpdate);
    }

    CORRADE_COMPARE(ui.renderer<Renderer>().setupFramebufferCallCount, 1);
    CORRADE_COMPARE(ui.renderer<Renderer>().transitionCallCount, 0);
    if(data.layer1)
        CORRADE_COMPARE(layer1->updateCallCount, 0);
    if(data.layer2)
        CORRADE_COMPARE(layer2->updateCallCount, 0);

    if(data.clean) {
        ui.clean();
        if(data.layer1 || data.layer2)
            CORRADE_COMPARE_AS(ui.state(),
                UserInterfaceState::NeedsDataUpdate,
                TestSuite::Compare::GreaterOrEqual);
        else CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
        CORRADE_COMPARE(ui.renderer<Renderer>().setupFramebufferCallCount, 1);
        CORRADE_COMPARE(ui.renderer<Renderer>().transitionCallCount, 0);
    }
    if(data.update) {
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
        CORRADE_COMPARE(ui.renderer<Renderer>().setupFramebufferCallCount, 1);
        CORRADE_COMPARE(ui.renderer<Renderer>().transitionCallCount, 0);
    }
    ui.draw();
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.renderer<Renderer>().transitionCallCount, 1);
    if(data.layer1)
        CORRADE_COMPARE(layer1->updateCallCount, 1);
    if(data.layer2)
        CORRADE_COMPARE(layer2->updateCallCount, 1);
}

void AbstractUserInterfaceTest::drawNoRendererSet() {
    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui{{100, 100}};

    std::ostringstream out;
    Error redirectError{&out};
    ui.draw();
    CORRADE_COMPARE(out.str(), "Whee::AbstractUserInterface::draw(): no renderer instance set\n");
}

void AbstractUserInterfaceTest::eventEmpty() {
    auto&& data = CleanUpdateData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui{{100, 100}};
    CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
    CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
    CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
    CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
    CORRADE_COMPARE(ui.currentGlobalPointerPosition(), Containers::NullOpt);
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

    /* Just verify that this doesn't crash or assert, there's nothing visibly
       changing after these calls except for the global pointer position; the
       events stay unaccepted */
    if(data.clean) {
        ui.clean();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }
    if(data.update) {
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }
    PointerEvent pointerEvent{{}, Pointer::MouseRight};
    PointerMoveEvent pointerMoveEvent{{}, {}, {}};
    FocusEvent focusEvent{{}};
    KeyEvent keyEvent{{}, Key::C, {}};
    TextInputEvent textInputEvent{{}, "hola"};
    CORRADE_VERIFY(!ui.pointerPressEvent({15, 36}, pointerEvent));
    CORRADE_COMPARE(ui.currentGlobalPointerPosition(), (Vector2{15.0f, 36.0f}));
    CORRADE_VERIFY(!pointerEvent.isAccepted());
    CORRADE_VERIFY(!ui.pointerReleaseEvent({16, 35}, pointerEvent));
    CORRADE_COMPARE(ui.currentGlobalPointerPosition(), (Vector2{16.0f, 35.0f}));
    CORRADE_VERIFY(!pointerEvent.isAccepted());
    CORRADE_VERIFY(!ui.pointerMoveEvent({20, 22}, pointerMoveEvent));
    CORRADE_COMPARE(ui.currentGlobalPointerPosition(), (Vector2{20.0f, 22.0f}));
    CORRADE_VERIFY(!pointerMoveEvent.isAccepted());
    CORRADE_VERIFY(!ui.focusEvent(NodeHandle::Null, focusEvent));
    CORRADE_VERIFY(!focusEvent.isAccepted());
    CORRADE_VERIFY(!ui.keyPressEvent(keyEvent));
    CORRADE_VERIFY(!keyEvent.isAccepted());
    CORRADE_VERIFY(!ui.keyReleaseEvent(keyEvent));
    CORRADE_VERIFY(!keyEvent.isAccepted());
    CORRADE_VERIFY(!ui.textInputEvent(textInputEvent));
    CORRADE_VERIFY(!textInputEvent.isAccepted());
    CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
    CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
    CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
    CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
    CORRADE_COMPARE(ui.currentGlobalPointerPosition(), (Vector2{20.0f, 22.0f}));
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
}

void AbstractUserInterfaceTest::eventAlreadyAccepted() {
    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui{{100, 100}};
    PointerEvent pointerEvent{{}, Pointer::MouseRight};
    pointerEvent.setAccepted();
    PointerMoveEvent pointerMoveEvent{{}, {}, {}};
    pointerMoveEvent.setAccepted();
    FocusEvent focusEvent{{}};
    focusEvent.setAccepted();
    KeyEvent keyEvent{{}, Key::F, {}};
    keyEvent.setAccepted();
    TextInputEvent textInputEvent{{}, "nope"};
    textInputEvent.setAccepted();

    std::ostringstream out;
    Error redirectError{&out};
    ui.pointerPressEvent({}, pointerEvent);
    ui.pointerReleaseEvent({}, pointerEvent);
    ui.pointerMoveEvent({}, pointerMoveEvent);
    ui.focusEvent(NodeHandle::Null, focusEvent);
    ui.keyPressEvent(keyEvent);
    ui.keyReleaseEvent(keyEvent);
    ui.textInputEvent(textInputEvent);
    CORRADE_COMPARE_AS(out.str(),
        "Whee::AbstractUserInterface::pointerPressEvent(): event already accepted\n"
        "Whee::AbstractUserInterface::pointerReleaseEvent(): event already accepted\n"
        "Whee::AbstractUserInterface::pointerMoveEvent(): event already accepted\n"
        "Whee::AbstractUserInterface::focusEvent(): event already accepted\n"
        "Whee::AbstractUserInterface::keyPressEvent(): event already accepted\n"
        "Whee::AbstractUserInterface::keyReleaseEvent(): event already accepted\n"
        "Whee::AbstractUserInterface::textInputEvent(): event already accepted\n",
        TestSuite::Compare::String);
}

void AbstractUserInterfaceTest::eventNodePropagation() {
    auto&& data = EventNodePropagationData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* framebufferSize isn't used for anything here; events should get scaled
       to (0.1, 0.01) */
    AbstractUserInterface ui{{300.0f, 200.0f}, {3000.0f, 20000.0f}, {30, 20}};

    struct Layer: AbstractLayer {
        explicit Layer(LayerHandle handle, LayerFeatures features): AbstractLayer{handle}, features{features} {}

        using AbstractLayer::create;

        LayerFeatures doFeatures() const override { return features; }

        /* doClean() / doUpdate() tested thoroughly enough in draw() above */

        void doPointerPressEvent(UnsignedInt dataId, PointerEvent& event) override {
            /* The hover state is always false as there was no preceding move
               event that would mark the node as hovered */
            CORRADE_VERIFY(!event.isHovering());
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(*eventCalls, InPlaceInit, dataHandle(handle(), dataId, 1), event.position(), *accept);
            if(*accept)
                event.setAccepted();
        }
        void doPointerReleaseEvent(UnsignedInt, PointerEvent&) override {
            CORRADE_FAIL("This function shouldn't be called");
        }
        void doPointerTapOrClickEvent(UnsignedInt, PointerEvent&) override {
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

    struct Layouter: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::add;

        void doUpdate(Containers::BitArrayView layoutIdsToUpdate, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>& nodeOffsets, const  Containers::StridedArrayView1D<Vector2>& nodeSizes) override {
            const Containers::StridedArrayView1D<const NodeHandle> nodes = this->nodes();
            for(std::size_t i = 0; i != layoutIdsToUpdate.size(); ++i) {
                if(!layoutIdsToUpdate[i])
                    continue;
                nodeOffsets[nodeHandleId(nodes[i])].y() -= 1000.0f;
                nodeSizes[nodeHandleId(nodes[i])] *= 100.0f;
            }
        }
    };

    /* If the layouter is enabled, all nodes are shifted & scaled which makes
       them completely unreachable by events, and the layouter then undoes
       that */
    Vector2 baseNodeOffset{0.0f, data.layouter ? 1000.0f : 0.0f};
    Vector2 baseNodeScale{data.layouter ? 0.01f : 1.0f};
    NodeHandle bottom = ui.createNode(
        baseNodeOffset + Vector2{10.0f, 20.0f},
        baseNodeScale*Vector2{110.0f, 50.0f});
    NodeHandle top = ui.createNode(
        baseNodeOffset + Vector2{15.0f, 25.0f},
        baseNodeScale*Vector2{90.0f, 45.0f});
    NodeHandle topNested = ui.createNode(top,
        baseNodeOffset + Vector2{20.0f, 30.0f},
        baseNodeScale*Vector2{10.0f, 10.0f});
    NodeHandle removed = ui.createNode(topNested,
        baseNodeOffset,
        baseNodeScale*Vector2{10.0f, 10.0f});
    NodeHandle notInOrder = ui.createNode(
        baseNodeOffset,
        baseNodeScale*Vector2{200.0f, 200.0f});
    /* These three should all be invisible for events */
    NodeHandle hidden = ui.createNode(
        baseNodeOffset,
        baseNodeScale*Vector2{200.0f, 200.0f}, NodeFlag::Hidden);
    NodeHandle disabled = ui.createNode(
        baseNodeOffset,
        baseNodeScale*Vector2{200.0f, 200.0f}, NodeFlag::Disabled);
    NodeHandle noEvents = ui.createNode(
        baseNodeOffset,
        baseNodeScale*Vector2{200.0f, 200.0f}, NodeFlag::NoEvents);
    NodeHandle topNestedOutside = ui.createNode(topNested,
        baseNodeOffset + Vector2{7.5f, 7.5f},
        baseNodeScale*Vector2{10.0f, 10.0f});

    /* Update explicitly before adding the layouters / layers as
       NeedsLayoutUpdate and NeedsDataAttachmentUpdate is a subset of this, and
       having just those set may uncover accidental omissions in internal
       state updates compared to updating just once after creating both nodes
       and data */
    if(data.update) {
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    if(data.layouter) {
        Layouter& layouter = ui.setLayouterInstance(Containers::pointer<Layouter>(ui.createLayouter()));
        layouter.add(bottom);
        layouter.add(top);
        layouter.add(topNested);
        layouter.add(removed);
        layouter.add(hidden);
        layouter.add(disabled);
        layouter.add(noEvents);
        layouter.add(notInOrder);
        layouter.add(topNestedOutside);

        /* Again update explicitly before adding the layers to verify updating
           just the layouters doesn't break it for layers */
        if(data.update) {
            CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsLayoutAssignmentUpdate);
            ui.update();
            CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
        }
    }

    LayerHandle layer1 = ui.createLayer();
    Containers::Pointer<Layer> layer1Instance{InPlaceInit, layer1, LayerFeature::Event};

    LayerHandle layer2 = ui.createLayer();
    Containers::Pointer<Layer> layer2Instance{InPlaceInit, layer2, LayerFeature::Draw};

    LayerHandle layer3 = ui.createLayer();
    Containers::Pointer<Layer> layer3Instance{InPlaceInit, layer3, LayerFeature::Draw|LayerFeature::Event};

    /* The commented out variables are not participating in events in any
       way */
    DataHandle bottomData1 = layer1Instance->create(bottom);
    /*DataHandle bottomData2 =*/ layer2Instance->create(bottom);
    DataHandle topNestedData1 = layer3Instance->create(topNested);
    DataHandle topNestedData2 = layer1Instance->create(topNested);
    DataHandle topNestedData3 = layer3Instance->create(topNested);
    DataHandle topNestedOutsideData = layer3Instance->create(topNestedOutside);
    /*DataHandle notInOrderData =*/ layer1Instance->create(notInOrder);
    /*DataHandle hiddenData =*/ layer2Instance->create(hidden);
    /*DataHandle disabledData =*/ layer3Instance->create(disabled);
    /*DataHandle noEventsData =*/ layer1Instance->create(noEvents);
    /*DataHandle removedData =*/ layer3Instance->create(removed);
    DataHandle topData = layer3Instance->create(top);

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

    /* Update again to verify updating just data attachments doesn't break it
       for the subsequent operations */
    if(data.update) {
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate);
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    ui.clearNodeOrder(notInOrder);
    ui.removeNode(removed);
    CORRADE_COMPARE(ui.layer(layer1).usedCount(), 4);
    CORRADE_COMPARE(ui.layer(layer2).usedCount(), 2);
    CORRADE_COMPARE(ui.layer(layer3).usedCount(), 6);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeClean);

    if(data.clean) {
        ui.clean();
        CORRADE_COMPARE(ui.layer(layer1).usedCount(), 4);
        CORRADE_COMPARE(ui.layer(layer2).usedCount(), 2);
        CORRADE_COMPARE(ui.layer(layer3).usedCount(), 5);
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);
    }

    /* update() should call clean() only if needed */
    if(data.update) {
        ui.update();
        CORRADE_COMPARE(ui.layer(layer1).usedCount(), 4);
        CORRADE_COMPARE(ui.layer(layer2).usedCount(), 2);
        CORRADE_COMPARE(ui.layer(layer3).usedCount(), 5);
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    /* Completely outside, no hit */
    {
        layer1Accept = layer2Accept = layer3Accept = true;
        eventCalls = {};
        PointerEvent event{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(!ui.pointerPressEvent({3000.0f, 30000.0f}, event));
        CORRADE_COMPARE_AS(eventCalls, (Containers::arrayView<Containers::Triple<DataHandle, Vector2, bool>>({
        })), TestSuite::Compare::Container);

    /* On the notInOrder node that's not visible, no hit */
    } {
        layer1Accept = layer2Accept = layer3Accept = true;
        eventCalls = {};
        PointerEvent event{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(!ui.pointerPressEvent({1950.0f, 19500.0f}, event));
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
        CORRADE_COMPARE_AS(eventCalls, (Containers::arrayView<Containers::Triple<DataHandle, Vector2, bool>>({
        })), TestSuite::Compare::Container);

    /* On the top-level node with no other node covering it */
    } {
        layer1Accept = layer2Accept = layer3Accept = true;
        eventCalls = {};
        PointerEvent event{{}, Pointer::MouseLeft};
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
        PointerEvent event{{}, Pointer::MouseLeft};
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
        PointerEvent event{{}, Pointer::MouseLeft};
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
        PointerEvent event{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({1000.0f, 6000.0f}, event));
        CORRADE_COMPARE_AS(eventCalls, (Containers::arrayView<Containers::Triple<DataHandle, Vector2, bool>>({
            {topData, {85.0f, 35.0f}, false},
            {bottomData1, {90.0f, 40.0f}, true}
        })), TestSuite::Compare::Container);

    /* On a nested node. Multiple data attached, the one with highest ID gets
       picked first. Order in which they were attached doesn't matter. Goes
       through all attached nodes and returns true if any of them was
       accepted. */
    } {
        layer2Accept = layer3Accept = true;
        layer1Accept = false;
        eventCalls = {};
        PointerEvent event{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({400.0f, 6000.0f}, event));
        CORRADE_COMPARE_AS(eventCalls, (Containers::arrayView<Containers::Triple<DataHandle, Vector2, bool>>({
            /* There's removedData covering this same position but weren't
               considered as they belong to a removed node */
            {topNestedData3, {5.0f, 5.0f}, true},
            {topNestedData1, {5.0f, 5.0f}, true},
            {topNestedData2, {5.0f, 5.0f}, false},
        })), TestSuite::Compare::Container);

    /* It also continues going through the remaining after a not accepted
       one. */
    } {
        layer1Accept = layer2Accept = true;
        layer3Accept = false;
        eventCalls = {};
        PointerEvent event{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({400.0f, 6000.0f}, event));
        CORRADE_COMPARE_AS(eventCalls, (Containers::arrayView<Containers::Triple<DataHandle, Vector2, bool>>({
            {topNestedData3, {5.0f, 5.0f}, false},
            {topNestedData1, {5.0f, 5.0f}, false},
            {topNestedData2, {5.0f, 5.0f}, true},
        })), TestSuite::Compare::Container);

    /* If neither of the data accepted, falls through everything and then to
       parent nodes */
    } {
        layer1Accept = layer2Accept = layer3Accept = false;
        eventCalls = {};
        PointerEvent event{{}, Pointer::MouseLeft};
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
        PointerEvent event{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({430.0f, 6300.0f}, event));
        CORRADE_COMPARE_AS(eventCalls, (Containers::arrayView<Containers::Triple<DataHandle, Vector2, bool>>({
            {topNestedOutsideData, {0.5f, 0.5f}, true},
        })), TestSuite::Compare::Container);
    } {
        layer1Accept = layer2Accept = layer3Accept = true;
        eventCalls = {};
        PointerEvent event{{}, Pointer::MouseLeft};
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
        using AbstractLayer::create;

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
    DataHandle bottomData = ui.layer<Layer>(layer).create(bottom);
    DataHandle topData = ui.layer<Layer>(layer).create(top);

    /* Top left corner should go to the top node */
    {
        ui.layer<Layer>(layer).eventCalls = {};
        PointerEvent event{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({100.0f, 2000.0f}, event));
        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Pair<DataHandle, Vector2>>({
            {topData, {0.0f, 0.0f}},
        })), TestSuite::Compare::Container);

    /* Top edge should go to the top node */
    } {
        ui.layer<Layer>(layer).eventCalls = {};
        PointerEvent event{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({570.0f, 2000.0f}, event));
        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Pair<DataHandle, Vector2>>({
            {topData, {47.0f, 0.0f}},
        })), TestSuite::Compare::Container);

    /* Left edge should go to the top node */
    } {
        ui.layer<Layer>(layer).eventCalls = {};
        PointerEvent event{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({100.0f, 3400.0f}, event));
        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Pair<DataHandle, Vector2>>({
            {topData, {0.0f, 14.0f}},
        })), TestSuite::Compare::Container);

    /* Bottom right corner should go to the bottom node */
    } {
        ui.layer<Layer>(layer).eventCalls = {};
        PointerEvent event1{{}, Pointer::MouseLeft};
        PointerEvent event2{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({900.0f, 8000.0f}, event1));
        CORRADE_VERIFY(ui.pointerPressEvent({899.0f, 7990.0f}, event2));
        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Pair<DataHandle, Vector2>>({
            {bottomData, {90.0f, 80.0f}},
            {topData, {79.9f, 59.9f}},
        })), TestSuite::Compare::Container);

    /* Bottom edge should go to the top node */
    } {
        ui.layer<Layer>(layer).eventCalls = {};
        PointerEvent event1{{}, Pointer::MouseLeft};
        PointerEvent event2{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({900.0f, 3400.0f}, event1));
        CORRADE_VERIFY(ui.pointerPressEvent({899.0f, 3400.0f}, event2));
        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Pair<DataHandle, Vector2>>({
            {bottomData, {90.0f, 34.0f}},
            {topData, {79.9f, 14.0f}},
        })), TestSuite::Compare::Container);

    /* Right edge should go to the bottom node */
    } {
        ui.layer<Layer>(layer).eventCalls = {};
        PointerEvent event1{{}, Pointer::MouseLeft};
        PointerEvent event2{{}, Pointer::MouseLeft};
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
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doPointerPressEvent(UnsignedInt dataId, PointerEvent& event) override {
            CORRADE_COMPARE(event.time(), 12345_nsec);
            CORRADE_COMPARE(event.type(), Pointer::MouseLeft);
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, dataHandle(handle(), dataId, 1), event.position());
            event.setAccepted();
        }
        void doPointerReleaseEvent(UnsignedInt, PointerEvent&) override {
            CORRADE_FAIL("This function shouldn't be called.");
        }
        void doPointerTapOrClickEvent(UnsignedInt, PointerEvent&) override {
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

        Containers::Array<Containers::Pair<DataHandle, Vector2>> eventCalls;
    };

    NodeHandle node = ui.createNode({10.0f, 20.0f}, {20.0f, 20.0f});

    /* Update explicitly before adding the layer as NeedsDataAttachmentUpdate
       is a subset of this, and having just that one set may uncover accidental
       omissions in internal state updates compared to updating just once after
       creating both nodes and data */
    if(data.update) {
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<Layer>(layer));
    /*DataHandle data1 =*/ ui.layer<Layer>(layer).create();
    DataHandle data2 = ui.layer<Layer>(layer).create(node);
    DataHandle data3 = ui.layer<Layer>(layer).create(node);

    if(data.update) {
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate);
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    /* Outside, no hit. The pointer position gets remembered though. */
    {
        PointerEvent event{12345_nsec, Pointer::MouseLeft};
        CORRADE_VERIFY(!ui.pointerPressEvent({1000.0f, 10000.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentGlobalPointerPosition(), (Vector2{100.0f, 100.0f}));
        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Pair<DataHandle, Vector2>>({
        })), TestSuite::Compare::Container);

    /* Inside, hit */
    } {
        PointerEvent event{12345_nsec, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({200.0f, 2500.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentCapturedNode(), node);
        CORRADE_COMPARE(ui.currentGlobalPointerPosition(), (Vector2{20.0f, 25.0f}));
        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Pair<DataHandle, Vector2>>({
            {data3, {10.0f, 5.0f}},
            {data2, {10.0f, 5.0f}}
        })), TestSuite::Compare::Container);

    /* Pressing outside after a node was hit should reset the current pressed
       node */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent event1{12345_nsec, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({200.0f, 2500.0f}, event1));
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentCapturedNode(), node);
        CORRADE_COMPARE(ui.currentGlobalPointerPosition(), (Vector2{20.0f, 25.0f}));

        PointerEvent event2{12345_nsec, Pointer::MouseLeft};
        CORRADE_VERIFY(!ui.pointerPressEvent({1000.0f, 10000.0f}, event2));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentGlobalPointerPosition(), (Vector2{100.0f, 100.0f}));

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Pair<DataHandle, Vector2>>({
            {data3, {10.0f, 5.0f}},
            {data2, {10.0f, 5.0f}}
        })), TestSuite::Compare::Container);
    }

    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
}

void AbstractUserInterfaceTest::eventPointerPressNotAccepted() {
    /* Event scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};

    enum Event {
        Accepted = 1,
        Press = 2
    };
    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doPointerPressEvent(UnsignedInt dataId, PointerEvent& event) override {
            if(dataId == 0 || (dataId == 2 && accept2) || (dataId == 1 && accept1))
                event.setAccepted();
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Press|(event.isAccepted() ? Accepted : 0),
                dataHandle(handle(), dataId, 1), event.position());
        }

        Containers::Array<Containers::Triple<Int, DataHandle, Vector2>> eventCalls;
        bool accept1 = true,
            accept2 = true;
    };

    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer()));

    /* 10 --------     accepts maybe
       01     -------- accepts maybe
       00     -------- accepts always */
    NodeHandle node0 = ui.createNode({20.0f, 0.0f}, {20.0f, 20.0f});
    NodeHandle node1 = ui.createNode({10.0f, 0.0f}, {20.0f, 20.0f});
    DataHandle data00 = layer.create(node0);
    DataHandle data01 = layer.create(node0);
    DataHandle data10 = layer.create(node1);

    /* Second press on node 1 doesn't get accepted and falls back to node 0 */
    {
        layer.eventCalls = {};

        PointerEvent eventPress1{{}, Pointer::MouseLeft};
        layer.accept2 = true;
        CORRADE_VERIFY(ui.pointerPressEvent({15.0f, 10.0f}, eventPress1));
        CORRADE_COMPARE(ui.currentPressedNode(), node1);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentCapturedNode(), node1);

        PointerEvent eventPress2{{}, Pointer::Finger};
        layer.accept2 = false;
        CORRADE_VERIFY(ui.pointerPressEvent({25.0f, 15.0f}, eventPress2));
        CORRADE_COMPARE(ui.currentPressedNode(), node0);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentCapturedNode(), node0);

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Accepted, data10, {5.0f, 10.0f}},
            {Press, data10, {15.0f, 15.0f}}, /* not accepted */
            {Press|Accepted, data01, {5.0f, 15.0f}},
            {Press|Accepted, data00, {5.0f, 15.0f}},
        })), TestSuite::Compare::Container);

    /* Second press on node 1 doesn't get accepted and falls back outside of
       any other node, resetting the current pressed node */
    } {
        layer.eventCalls = {};

        PointerEvent eventPress1{{}, Pointer::MouseLeft};
        layer.accept2 = true;
        CORRADE_VERIFY(ui.pointerPressEvent({25.0f, 10.0f}, eventPress1));
        CORRADE_COMPARE(ui.currentPressedNode(), node1);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentCapturedNode(), node1);

        /* Yes, it's rather unlikely that another press happens without a
           release first, needing such specific handling, but let's have that
           covered because if some events get lost it'd be really hard to
           debug */
        PointerEvent eventPress2{{}, Pointer::Finger};
        layer.accept2 = false;
        CORRADE_VERIFY(!ui.pointerPressEvent({15.0f, 15.0f}, eventPress2));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Accepted, data10, {15.0f, 10.0f}},
            {Press, data10, {5.0f, 15.0f}}, /* not accepted */
            /* No other node to call press on */
        })), TestSuite::Compare::Container);
    }
}

void AbstractUserInterfaceTest::eventPointerRelease() {
    auto&& data = UpdateData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* framebufferSize isn't used for anything here; events should get scaled
       to (0.1, 0.01) */
    AbstractUserInterface ui{{300.0f, 200.0f}, {3000.0f, 20000.0f}, {30, 20}};

    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doPointerPressEvent(UnsignedInt, PointerEvent&) override {
            CORRADE_FAIL("This function shouldn't be called.");
        }
        void doPointerReleaseEvent(UnsignedInt dataId, PointerEvent& event) override {
            CORRADE_COMPARE(event.time(), 12345_nsec);
            CORRADE_COMPARE(event.type(), Pointer::MouseLeft);
            /* The hover state is always false as there was no preceding move
               event that would mark the node as hovered */
            CORRADE_VERIFY(!event.isHovering());
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, dataHandle(handle(), dataId, 1), event.position());
            event.setAccepted();
        }
        void doPointerTapOrClickEvent(UnsignedInt, PointerEvent&) override {
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

        Containers::Array<Containers::Pair<DataHandle, Vector2>> eventCalls;
    };

    NodeHandle node = ui.createNode({10.0f, 20.0f}, {20.0f, 20.0f});

    /* Update explicitly before adding the layer as NeedsDataAttachmentUpdate
       is a subset of this, and having just that one set may uncover accidental
       omissions in internal state updates compared to updating just once after
       creating both nodes and data */
    if(data.update) {
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<Layer>(layer));
    /*DataHandle data1 =*/ ui.layer<Layer>(layer).create();
    DataHandle data2 = ui.layer<Layer>(layer).create(node);
    DataHandle data3 = ui.layer<Layer>(layer).create(node);

    if(data.update) {
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate);
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    /* Outside, no hit. The pointer position gets remembered though. */
    {
        PointerEvent event{12345_nsec, Pointer::MouseLeft};
        CORRADE_VERIFY(!ui.pointerReleaseEvent({1000.0f, 10000.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentGlobalPointerPosition(), (Vector2{100.0f, 100.0f}));
        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Pair<DataHandle, Vector2>>({
        })), TestSuite::Compare::Container);

    /* Inside, hit, going through all data attachments. Highest ID gets picked
       first. */
    } {
        PointerEvent event{12345_nsec, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({200.0f, 2500.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentGlobalPointerPosition(), (Vector2{20.0f, 25.0f}));
        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Pair<DataHandle, Vector2>>({
            {data3, {10.0f, 5.0f}},
            {data2, {10.0f, 5.0f}}
        })), TestSuite::Compare::Container);
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
        Hovering = 1,
        Move = 2,
        Enter = 4,
        Leave = 6
    };
    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doPointerPressEvent(UnsignedInt, PointerEvent&) override {
            CORRADE_FAIL("This function shouldn't be called.");
        }
        void doPointerReleaseEvent(UnsignedInt, PointerEvent&) override {
            CORRADE_FAIL("This function shouldn't be called.");
        }
        void doPointerTapOrClickEvent(UnsignedInt, PointerEvent&) override {
            CORRADE_FAIL("This function shouldn't be called.");
        }
        void doPointerMoveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            CORRADE_COMPARE(event.time(), 12345_nsec);
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Move|(event.isHovering() ? Hovering : 0), dataHandle(handle(), dataId, 1), Vector4{event.position().x(), event.position().y(), event.relativePosition().x(), event.relativePosition().y()});
            event.setAccepted();
        }
        void doPointerEnterEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The time should be propagated to the synthesized event */
            CORRADE_COMPARE(event.time(), 12345_nsec);
            /* All enter events are hovering by definition */
            CORRADE_VERIFY(event.isHovering());
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Enter|(event.isHovering() ? Hovering : 0), dataHandle(handle(), dataId, 1), Vector4{event.position().x(), event.position().y(), event.relativePosition().x(), event.relativePosition().y()});
        }
        void doPointerLeaveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The time should be propagated to the synthesized event */
            CORRADE_COMPARE(event.time(), 12345_nsec);
            /* All leave events are not hovering by definition */
            CORRADE_VERIFY(!event.isHovering());
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Leave|(event.isHovering() ? Hovering : 0), dataHandle(handle(), dataId, 1), Vector4{event.position().x(), event.position().y(), event.relativePosition().x(), event.relativePosition().y()});
        }

        Containers::Array<Containers::Triple<Int, DataHandle, Vector4>> eventCalls;
    };

    /* Two nodes next to each other */
    NodeHandle left = ui.createNode({20.0f, 0.0f}, {20.0f, 20.0f});
    NodeHandle right = ui.createNode({40.0f, 0.0f}, {20.0f, 20.0f});

    /* Update explicitly before adding the layer as NeedsDataAttachmentUpdate
       is a subset of this, and having just that one set may uncover accidental
       omissions in internal state updates compared to updating just once after
       creating both nodes and data */
    if(data.update) {
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<Layer>(layer));
    DataHandle leftData1 = ui.layer<Layer>(layer).create(left);
    DataHandle leftData2 = ui.layer<Layer>(layer).create(left);
    DataHandle rightData = ui.layer<Layer>(layer).create(right);

    if(data.update) {
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate);
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    /* Outside, no hit. The pointer position gets remembered though. */
    {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerMoveEvent event{12345_nsec, {}, {}};
        CORRADE_VERIFY(!ui.pointerMoveEvent({100.0f, 1000.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentGlobalPointerPosition(), (Vector2{10.0f, 10.0f}));
        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector4>>({
        })), TestSuite::Compare::Container);

    /* Inside a node, going through all data attachments. Highest ID gets
       picked first. Relative to previous move event even though it didn't hit
       anything, the hovered node gets set to given node. */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerMoveEvent event1{12345_nsec, {}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({300.0f, 1100.0f}, event1));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), left);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentGlobalPointerPosition(), (Vector2{30.0f, 11.0f}));

        PointerMoveEvent event2{12345_nsec, {}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({350.0f, 1000.0f}, event2));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), left);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentGlobalPointerPosition(), (Vector2{35.0f, 10.0f}));

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector4>>({
            /* There's nothing to receive a Leave event. The events also
               don't have Hovering set yet, as current hovered node is null at
               that point. */
            {Move, leftData2, {10.0f, 11.0f, 20.0f, 1.0f}},
            {Move, leftData1, {10.0f, 11.0f, 20.0f, 1.0f}},
            /* It has to first execute the Move to discover a node that accepts
               the event, thus Enter can't be before the Move. It has the
               Hovering set, as that has to be there by definition. */
            {Enter|Hovering, leftData2, {10.0f, 11.0f, 0.0f, 0.0f}},
            {Enter|Hovering, leftData1, {10.0f, 11.0f, 0.0f, 0.0f}},
            /* Second move then has Hovering set, as the current hovered node
               matches the node on which the move is called */
            {Move|Hovering, leftData2, {15.0f, 10.0f, 5.0f, -1.0f}},
            {Move|Hovering, leftData1, {15.0f, 10.0f, 5.0f, -1.0f}},
            /* It stays on the same node, so no further Enter or Leave */
        })), TestSuite::Compare::Container);

    /* Inside and then to another node. Relative to previous move event even
       though it didn't hit anything, the hovered node changes based on what's
       under the pointer at the moment. */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerMoveEvent event1{12345_nsec, {}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({300.0f, 1100.0f}, event1));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), left);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentGlobalPointerPosition(), (Vector2{30.0f, 11.0f}));

        PointerMoveEvent event2{12345_nsec, {}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({550.0f, 1000.0f}, event2));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), right);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentGlobalPointerPosition(), (Vector2{55.0f, 10.0f}));

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector4>>({
            /* It stays on the same node, so no further Enter or Leave */
            {Move|Hovering, leftData2, {10.0f, 11.0f, -5.0f, 1.0f}},
            {Move|Hovering, leftData1, {10.0f, 11.0f, -5.0f, 1.0f}},
            /* Move onto the right node has no Hovering set initially as the
               current hovered node is still different at that point */
            {Move, rightData, {15.0f, 10.0f, 25.0f, -1.0f}},
            /* It has to first execute the Move to discover the next node that
               accepts the event, thus Leave can't be before the Move either
               because it could end up at the same node. Tested thoroughly in
               eventPointerMoveNotAccepted() below. */
            {Leave, leftData2, {35.0f, 10.0f, 0.0f, 0.0f}},
            {Leave, leftData1, {35.0f, 10.0f, 0.0f, 0.0f}},
            {Enter|Hovering, rightData, {15.0f, 10.0f, 0.0f, 0.0f}},
        })), TestSuite::Compare::Container);

    /* Out of the item, again relative to what happened last */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerMoveEvent event{12345_nsec, {}, {}};
        CORRADE_VERIFY(!ui.pointerMoveEvent({100.0f, 1100.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentGlobalPointerPosition(), (Vector2{10.0f, 11.0f}));
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
        PointerMoveEvent event2{12345_nsec, {}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({5500.0f, 100.0f}, event2));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), right);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentGlobalPointerPosition(), (Vector2{55.0f, 10.0f}));
        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector4>>({
            /* Is relative to the {-30, 11} that was above, without considering
               the 10x / 100x scale in any way */
            {Move, rightData, {15.0f, 10.0f, 45.0f, -1.0f}},
            {Enter|Hovering, rightData, {15.0f, 10.0f, 0.0f, 0.0f}},
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

        using AbstractLayer::create;

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
    /*DataHandle nodeData2 =*/ ui.layer<Layer>(layer).create(node);

    /* Press, move, release, move on the same node */
    {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent pressEvent{{}, Pointer::MouseRight};
        CORRADE_COMPARE(ui.pointerPressEvent({300.0f, 1000.0f}, pressEvent), data.accept);

        PointerMoveEvent moveEvent1{{}, {}, {}};
        CORRADE_COMPARE(ui.pointerMoveEvent({350.0f, 1500.0f}, moveEvent1), data.accept);

        PointerEvent releaseEvent{{}, Pointer::MouseMiddle};
        CORRADE_COMPARE(ui.pointerReleaseEvent({250.0f, 500.0f}, releaseEvent), data.accept);

        PointerMoveEvent moveEvent2{{}, {}, {}};
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

        PointerMoveEvent moveEvent{{}, {}, {}};
        CORRADE_COMPARE(ui.pointerMoveEvent({300.0f, 1000.0f}, moveEvent), data.accept);

        PointerEvent pressEvent{{}, Pointer::MouseMiddle};
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
        Hovering = 2,
        /* All below have to be multiples of 4 to not clash with the above */
        Move = 4,
        Enter = 8,
        Leave = 12
    };
    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doPointerMoveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            if(dataId == 0 || (dataId == 2 && accept2) || (dataId == 1 && accept1))
                event.setAccepted();
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit,
                (event.isAccepted() ? Accepted : 0)|
                (event.isHovering() ? Hovering : 0)|Move,
                dataHandle(handle(), dataId, 1), event.position());
        }
        void doPointerEnterEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit,
                (event.isHovering() ? Hovering : 0)|Enter,
                dataHandle(handle(), dataId, 1), event.position());
        }
        void doPointerLeaveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit,
                (event.isHovering() ? Hovering : 0)|Leave,
                dataHandle(handle(), dataId, 1), event.position());
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
    DataHandle data00 = ui.layer<Layer>(layer).create(node0);
    DataHandle data01 = ui.layer<Layer>(layer).create(node0);
    DataHandle data10 = ui.layer<Layer>(layer).create(node1);

    /* Move on node 1, but the second move doesn't get accepted and falls back
       to node 0, generating Leave and Enter events as appropriate */
    {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerMoveEvent eventMove1{{}, {}, {}};
        ui.layer<Layer>(layer).accept2 = true;
        CORRADE_VERIFY(ui.pointerMoveEvent({15.0f, 10.0f}, eventMove1));
        CORRADE_COMPARE(ui.currentHoveredNode(), node1);

        PointerMoveEvent eventMove2{{}, {}, {}};
        ui.layer<Layer>(layer).accept2 = false;
        CORRADE_VERIFY(ui.pointerMoveEvent({25.0f, 15.0f}, eventMove2));
        CORRADE_COMPARE(ui.currentHoveredNode(), node0);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Move|Accepted, data10, {5.0f, 10.0f}},
            {Enter|Hovering, data10, {5.0f, 10.0f}},
            /* The move is happening on the same node as was hovered before, so
               Hovering is set, however it's not accepted and so the next move
               (happening on a different node) won't have it set anymore */
            {Move|Hovering, data10, {15.0f, 15.0f}}, /* not accepted */
            {Move|Accepted, data01, {5.0f, 15.0f}},
            {Move|Accepted, data00, {5.0f, 15.0f}},
            {Leave, data10, {15.0f, 15.0f}},
            {Enter|Hovering, data01, {5.0f, 15.0f}},
            {Enter|Hovering, data00, {5.0f, 15.0f}}
        })), TestSuite::Compare::Container);

    /* Move from node 0 to node 1, but the second move doesn't get accepted and
       falls back to node 0, not generating any Enter/Leave event */
    } {
        /* Just to reset everything */
        /** @todo have a pointerCancelEvent() for this */
        PointerMoveEvent eventMoveReset{{}, {}, {}};
        ui.pointerMoveEvent({1000.0f, 1000.0f}, eventMoveReset);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        ui.layer<Layer>(layer).eventCalls = {};

        PointerMoveEvent eventMove1{{}, {}, {}};
        ui.layer<Layer>(layer).accept1 = true;
        CORRADE_VERIFY(ui.pointerMoveEvent({35.0f, 10.0f}, eventMove1));
        CORRADE_COMPARE(ui.currentHoveredNode(), node0);

        PointerMoveEvent eventMove2{{}, {}, {}};
        ui.layer<Layer>(layer).accept2 = false;
        CORRADE_VERIFY(ui.pointerMoveEvent({25.0f, 15.0f}, eventMove2));
        CORRADE_COMPARE(ui.currentHoveredNode(), node0);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Move|Accepted, data01, {15.0f, 10.0f}},
            {Move|Accepted, data00, {15.0f, 10.0f}},
            {Enter|Hovering, data01, {15.0f, 10.0f}},
            {Enter|Hovering, data00, {15.0f, 10.0f}},
            {Move, data10, {15.0f, 15.0f}}, /* not accepted */
            /* The move is happening on the same node as was hovered before, so
               Hovering is set */
            {Move|Accepted|Hovering, data01, {5.0f, 15.0f}},
            {Move|Accepted|Hovering, data00, {5.0f, 15.0f}},
            /* No Enter/Leave as we ended up staying on the same node */
        })), TestSuite::Compare::Container);

    /* Move on node 0, data01 doesn't accept while data00 does. Given it's all
       happening on the same node, no Enter/Leave should be generated. */
    } {
        /* Just to reset everything */
        /** @todo have a pointerCancelEvent() for this */
        PointerMoveEvent eventMoveReset{{}, {}, {}};
        ui.pointerMoveEvent({1000.0f, 1000.0f}, eventMoveReset);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        ui.layer<Layer>(layer).eventCalls = {};

        PointerMoveEvent eventMove1{{}, {}, {}};
        ui.layer<Layer>(layer).accept1 = true;
        CORRADE_VERIFY(ui.pointerMoveEvent({35.0f, 10.0f}, eventMove1));
        CORRADE_COMPARE(ui.currentHoveredNode(), node0);

        PointerMoveEvent eventMove2{{}, {}, {}};
        ui.layer<Layer>(layer).accept1 = false;
        CORRADE_VERIFY(ui.pointerMoveEvent({33.0f, 15.0f}, eventMove2));
        CORRADE_COMPARE(ui.currentHoveredNode(), node0);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Move|Accepted, data01, {15.0f, 10.0f}},
            {Move|Accepted, data00, {15.0f, 10.0f}},
            {Enter|Hovering, data01, {15.0f, 10.0f}},
            {Enter|Hovering, data00, {15.0f, 10.0f}},
            {Move|Hovering, data01, {13.0f, 15.0f}}, /* not accepted */
            {Move|Accepted|Hovering, data00, {13.0f, 15.0f}},
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
        using AbstractLayer::create;

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

    /* Nested node in order to verify that the hidden flag gets propagated
       through the hierarchy */
    NodeHandle node = ui.createNode({20.0f, 0.0f}, {20.0f, 20.0f});
    NodeHandle nested = ui.createNode(node, {}, {20.0f, 20.0f});

    /* Update explicitly before adding the layer as NeedsDataAttachmentUpdate
       is a subset of this, and having just that one set may uncover accidental
       omissions in internal state updates compared to updating just once after
       creating both nodes and data */
    if(data.update) {
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<Layer>(layer));
    DataHandle nestedData = ui.layer<Layer>(layer).create(nested);

    if(data.update) {
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate);
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    PointerMoveEvent eventMove1{{}, {}, {}};
    CORRADE_VERIFY(ui.pointerMoveEvent({300.0f, 1000.0f}, eventMove1));
    CORRADE_COMPARE(ui.currentHoveredNode(), nested);

    ui.setNodeOffset(node, {30.0f, 20.0f});
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsLayoutUpdate);

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    PointerMoveEvent eventMove2{{}, {}, {}};
    CORRADE_VERIFY(ui.pointerMoveEvent({350.0f, 2500.0f}, eventMove2));
    CORRADE_COMPARE(ui.currentHoveredNode(), nested);

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

void AbstractUserInterfaceTest::eventPointerMoveNodeBecomesHiddenDisabledNoEvents() {
    auto&& data = EventPointerNodeBecomesHiddenDisabledNoEventsData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Event scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};

    enum Event {
        Move = 0,
        Enter = 1,
        Leave = 2,
        VisibilityLost = 3,
        Update = 4
    };
    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

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
        void doVisibilityLostEvent(UnsignedInt dataId, VisibilityLostEvent& event) override {
            /* These can only be set if a focused node is no longer focusable */
            CORRADE_VERIFY(!event.isHovering());
            CORRADE_VERIFY(!event.isPressed());
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, VisibilityLost, dataHandle(handle(), dataId, 1), Vector2{});
        }
        void doUpdate(LayerStates, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, Containers::BitArrayView, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&) override {
            arrayAppend(eventCalls, InPlaceInit, Update, DataHandle::Null, Vector2{});
        }

        Containers::Array<Containers::Triple<Int, DataHandle, Vector2>> eventCalls;
    };

    /* Nested node in order to verify that the hidden/disabled/... flag gets
       propagated through the hierarchy */
    NodeHandle node = ui.createNode({20.0f, 0.0f}, {20.0f, 20.0f});
    NodeHandle nested = ui.createNode(node, {}, {20.0f, 20.0f});

    /* Update explicitly before adding the layer as NeedsDataAttachmentUpdate
       is a subset of this, and having just that one set may uncover accidental
       omissions in internal state updates compared to updating just once after
       creating both nodes and data */
    if(data.update) {
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<Layer>(layer));
    DataHandle nestedData = ui.layer<Layer>(layer).create(nested);

    if(data.update) {
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate);
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    PointerMoveEvent eventMove1{{}, {}, {}};
    CORRADE_VERIFY(ui.pointerMoveEvent({30.0f, 10.0f}, eventMove1));
    CORRADE_COMPARE(ui.currentHoveredNode(), nested);

    if(data.flags)
        ui.addNodeFlags(node, data.flags);
    else if(data.clearOrder)
        ui.clearNodeOrder(node);
    else CORRADE_INTERNAL_ASSERT_UNREACHABLE();
    /* The current hovered node stays after setting the flags, is only updated
       after update() -- there it also handles if any parent gets the flag as
       well */
    CORRADE_COMPARE(ui.currentHoveredNode(), nested);
    CORRADE_COMPARE(ui.state(), data.expectedState);

    if(data.update) {
        ui.update();

        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    PointerMoveEvent eventMove2{{}, {}, {}};
    /* There's no node to execute the move on */
    CORRADE_VERIFY(!ui.pointerMoveEvent({35.0f, 15.0f}, eventMove2));
    CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

    CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
        {Update, {}, {}},
        {Move, nestedData, {10.0f, 10.0f}},
        {Enter, nestedData, {10.0f, 10.0f}},
        /* The visibility lost event gets emitted before a doUpdate() so the
           changes can be directly reflected */
        {VisibilityLost, nestedData, {}},
        {Update, {}, {}},
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
        using AbstractLayer::create;

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
        void doVisibilityLostEvent(UnsignedInt, VisibilityLostEvent&) override {
            CORRADE_FAIL("This function shouldn't be called.");
        }

        Containers::Array<Containers::Triple<Int, DataHandle, Vector2>> eventCalls;
    };

    /* Nested node in order to verify that the removal gets propagated through
       the hierarchy */
    NodeHandle node = ui.createNode({20.0f, 0.0f}, {20.0f, 20.0f});
    NodeHandle nested = ui.createNode(node, {}, {20.0f, 20.0f});

    /* Update explicitly before adding the layer as NeedsDataAttachmentUpdate
       is a subset of this, and having just that one set may uncover accidental
       omissions in internal state updates compared to updating just once after
       creating both nodes and data */
    if(data.update) {
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<Layer>(layer));
    DataHandle nestedData = ui.layer<Layer>(layer).create(nested);

    if(data.update) {
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate);
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    PointerMoveEvent eventMove1{{}, {}, {}};
    CORRADE_VERIFY(ui.pointerMoveEvent({30.0f, 10.0f}, eventMove1));
    CORRADE_COMPARE(ui.currentHoveredNode(), nested);

    ui.removeNode(data.removeParent ? node : nested);
    /* The current hovered node stays after removal, is only updated after
       update() -- there it also handles if any parent is removed */
    CORRADE_COMPARE(ui.currentHoveredNode(), nested);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeClean);

    /* Add a new node in a different place, to verify the generation is
       correctly checked as well */
    if(!data.removeParent) {
        NodeHandle nestedReplacement = ui.createNode(node, {}, {40.0f, 20.0f});
        CORRADE_COMPARE(nodeHandleId(nestedReplacement), nodeHandleId(nested));
    }

    if(data.update) {
        ui.update();

        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    PointerMoveEvent eventMove2{{}, {}, {}};
    CORRADE_VERIFY(!ui.pointerMoveEvent({35.0f, 15.0f}, eventMove2));
    CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

    CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
        {Move, nestedData, {10.0f, 10.0f}},
        {Enter, nestedData, {10.0f, 10.0f}},
        /* There's no node to execute the Move on, nor a Leave */
    })), TestSuite::Compare::Container);

    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
}

void AbstractUserInterfaceTest::eventPointerMoveAllDataRemoved() {
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
        using AbstractLayer::create;
        using AbstractLayer::remove;

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
        void doVisibilityLostEvent(UnsignedInt, VisibilityLostEvent&) override {
            CORRADE_FAIL("This function shouldn't be called.");
        }

        Containers::Array<Containers::Triple<Int, DataHandle, Vector2>> eventCalls;
    };

    NodeHandle node = ui.createNode({20.0f, 0.0f}, {20.0f, 20.0f});

    /* Update explicitly before adding the layer as NeedsDataAttachmentUpdate
       is a subset of this, and having just that one set may uncover accidental
       omissions in internal state updates compared to updating just once after
       creating both nodes and data */
    if(data.update) {
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<Layer>(layer));
    DataHandle nodeData = ui.layer<Layer>(layer).create(node);

    if(data.update) {
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate);
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    PointerMoveEvent eventMove1{{}, {}, {}};
    CORRADE_VERIFY(ui.pointerMoveEvent({30.0f, 10.0f}, eventMove1));
    CORRADE_COMPARE(ui.currentHoveredNode(), node);

    ui.layer<Layer>(layer).remove(nodeData);
    /* The node stays hovered until an actual move event discovers there's no
       data anymore. This is consistent for example with a case where the data
       would change the area where they're active for events -- also something
       that the clean() / update() can't discover on its own, only actually
       firing the event can. */
    CORRADE_COMPARE(ui.currentHoveredNode(), node);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate|UserInterfaceState::NeedsDataClean);

    if(data.clean) {
        ui.clean();

        /* Same as above, the node stays hovered */
        CORRADE_COMPARE(ui.currentHoveredNode(), node);
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate);
    }

    if(data.update) {
        ui.update();

        /* Same as above, the node stays hovered */
        CORRADE_COMPARE(ui.currentHoveredNode(), node);
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    PointerMoveEvent eventMove2{{}, {}, {}};
    /* There's no data to execute the move on */
    CORRADE_VERIFY(!ui.pointerMoveEvent({35.0f, 15.0f}, eventMove2));
    CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

    CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
        {Move, nodeData, {10.0f, 10.0f}},
        {Enter, nodeData, {10.0f, 10.0f}},
        /* There's no data to execute the Move on, nor a Leave */
    })), TestSuite::Compare::Container);

    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
}

void AbstractUserInterfaceTest::eventCapture() {
    auto&& data = EventLayouterData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* framebufferSize isn't used for anything here; events should get scaled
       to (0.1, 0.01) */
    AbstractUserInterface ui{{300.0f, 200.0f}, {3000.0f, 20000.0f}, {30, 20}};

    enum Event {
        Captured = 1,
        Hovering = 2,
        /* All below have to be multiples of 4 to not clash with the above */
        Press = 4,
        Release = 8,
        Move = 12,
        Enter = 16,
        Leave = 20
    };
    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doPointerPressEvent(UnsignedInt dataId, PointerEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit,
                (event.isCaptured() ? Captured : 0)|
                (event.isHovering() ? Hovering : 0)|Press,
                dataHandle(handle(), dataId, 1), event.position());
            event.setAccepted();
        }
        void doPointerReleaseEvent(UnsignedInt dataId, PointerEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit,
                (event.isCaptured() ? Captured : 0)|
                (event.isHovering() ? Hovering : 0)|Release,
                dataHandle(handle(), dataId, 1), event.position());
            event.setAccepted();
        }
        /* tapOrClick event test in a dedicated eventTapOrClick() below as it
           would add too much noise here */
        void doPointerMoveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit,
                (event.isCaptured() ? Captured : 0)|
                (event.isHovering() ? Hovering : 0)|Move,
                dataHandle(handle(), dataId, 1), event.position());
            event.setAccepted();
        }
        void doPointerEnterEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit,
                (event.isCaptured() ? Captured : 0)|
                (event.isHovering() ? Hovering : 0)|Enter, dataHandle(handle(), dataId, 1), event.position());
        }
        void doPointerLeaveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit,
                (event.isCaptured() ? Captured : 0)|
                (event.isHovering() ? Hovering : 0)|Leave,
                dataHandle(handle(), dataId, 1), event.position());
        }

        Containers::Array<Containers::Triple<Int, DataHandle, Vector2>> eventCalls;
    };

    struct Layouter: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::add;

        void doUpdate(Containers::BitArrayView layoutIdsToUpdate, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>& nodeOffsets, const  Containers::StridedArrayView1D<Vector2>& nodeSizes) override {
            const Containers::StridedArrayView1D<const NodeHandle> nodes = this->nodes();
            for(std::size_t i = 0; i != layoutIdsToUpdate.size(); ++i) {
                if(!layoutIdsToUpdate[i])
                    continue;
                nodeOffsets[nodeHandleId(nodes[i])].y() -= 1000.0f;
                nodeSizes[nodeHandleId(nodes[i])] *= 100.0f;
            }
        }
    };

    /* Two nodes next to each other. If the layouter is enabled, the nodes are
       shifted & scaled which makes them completely unreachable by events, and
       the layouter then undoes that */
    Vector2 baseNodeOffset{0.0f, data.layouter ? 1000.0f : 0.0f};
    Vector2 baseNodeScale{data.layouter ? 0.01f : 1.0f};
    NodeHandle left = ui.createNode(
        baseNodeOffset + Vector2{20.0f, 0.0f},
        baseNodeScale*Vector2{20.0f, 20.0f});
    NodeHandle right = ui.createNode(
        baseNodeOffset + Vector2{40.0f, 0.0f},
        baseNodeScale*Vector2{20.0f, 20.0f});
    if(data.layouter) {
        Layouter& layouter = ui.setLayouterInstance(Containers::pointer<Layouter>(ui.createLayouter()));
        layouter.add(left);
        layouter.add(right);
    }

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<Layer>(layer));
    DataHandle leftData1 = ui.layer<Layer>(layer).create(left);
    DataHandle leftData2 = ui.layer<Layer>(layer).create(left);
    DataHandle rightData = ui.layer<Layer>(layer).create(right);

    /* Nothing captured initially */
    CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);

    /* Capture on the left node, release on it again. Going through all data
       attachments, highest ID gets picked first. */
    {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent eventPress{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({300.0f, 1000.0f}, eventPress));
        CORRADE_COMPARE(ui.currentCapturedNode(), left);
        /* No Enter/Leave events synthesized from Press at the moment, so no
           hovered node either */
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        PointerEvent eventRelease{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({320.0f, 1000.0f}, eventRelease));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        /* No Enter/Leave events synthesized from Release at the moment, so no
           hovered node either */
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        /* Hovering is not set as there's no currently hovered node */
        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, leftData2, {10.0f, 10.0f}},
            {Press|Captured, leftData1, {10.0f, 10.0f}},
            /* No Enter/Leave events synthesized from Press and Release at the
               moment */
            {Release|Captured, leftData2, {12.0f, 10.0f}},
            {Release|Captured, leftData1, {12.0f, 10.0f}},
        })), TestSuite::Compare::Container);

    /* Capture on the left node, release on the right one */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent eventPress{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({300.0f, 1000.0f}, eventPress));
        CORRADE_COMPARE(ui.currentCapturedNode(), left);
        /* No Enter/Leave events synthesized from Press at the moment, so no
           hovered node either */
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        PointerEvent eventRelease{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({500.0f, 1000.0f}, eventRelease));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        /* No Enter/Leave events synthesized from Release at the moment, so no
           hovered node either */
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        /* Hovering is not set on any as there's no currently hovered node */
        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, leftData2, {10.0f, 10.0f}},
            {Press|Captured, leftData1, {10.0f, 10.0f}},
            /* No Enter/Leave events synthesized from Press and Release at the
               moment */
            {Release|Captured, leftData2, {30.0f, 10.0f}}, /* actually on rightData */
            {Release|Captured, leftData1, {30.0f, 10.0f}}, /* actually on rightData */
        })), TestSuite::Compare::Container);

    /* Capture on the right node, release on the left one */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent eventPress{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({500.0f, 1000.0f}, eventPress));
        CORRADE_COMPARE(ui.currentCapturedNode(), right);
        /* No Enter/Leave events synthesized from Press at the moment, so no
           hovered node either */
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        PointerEvent eventRelease{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({300.0f, 1000.0f}, eventRelease));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        /* No Enter/Leave events synthesized from Release at the moment, so no
           hovered node either */
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        /* Hovering is not set on any as there's no currently hovered node */
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

        PointerMoveEvent eventMove1{{}, {}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({500.0f, 1500.0f}, eventMove1));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), right);

        PointerEvent eventPress{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({300.0f, 1000.0f}, eventPress));
        CORRADE_COMPARE(ui.currentCapturedNode(), left);
        /* No Enter/Leave events synthesized from Press at the moment, so the
           hovered node doesn't get updated until the next move */
        CORRADE_COMPARE(ui.currentHoveredNode(), right);

        PointerMoveEvent eventMove2{{}, {}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({350.0f, 1500.0f}, eventMove2));
        CORRADE_COMPARE(ui.currentCapturedNode(), left);
        CORRADE_COMPARE(ui.currentHoveredNode(), left);

        PointerEvent eventRelease{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({500.0f, 1000.0f}, eventRelease));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        /* No Enter/Leave events synthesized from Release at the moment, so the
           hovered node doesn't get updated until the next move */
        CORRADE_COMPARE(ui.currentHoveredNode(), left);

        PointerMoveEvent eventMove3{{}, {}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({550.0f, 1500.0f}, eventMove3));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), right);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            /* A move that happens before a press isn't captured. Hovering not
               set yet as current hovered node is null at the point move event
               is called. */
            {Move, rightData, {10.0f, 15.0f}},
            /* Neither is the Enter synthesized from it */
            {Enter|Hovering, rightData, {10.0f, 15.0f}},
            /* The right node is hovered at the moment, so Press doesn't get
               Hovering set */
            {Press|Captured, leftData2, {10.0f, 10.0f}},
            {Press|Captured, leftData1, {10.0f, 10.0f}},
            /* No Enter/Leave event synthesized from Press at the moment. If
               they would, neither would be captured. */
            /* A move that happens during a press is captured. Since no
               Enter/Leave events were synthesized from the Press, they get emitted here. The Leave event isn't captured, the Enter is, in
               order to allow a capture reset in its handler. The Move is the
               first happening on the left node, so it doesn't have Hovering
               set. */
            {Move|Captured, leftData2, {15.0f, 15.0f}},
            {Move|Captured, leftData1, {15.0f, 15.0f}},
            {Leave, rightData, {-5.0f, 15.0f}},
            {Enter|Captured|Hovering, leftData2, {15.0f, 15.0f}},
            {Enter|Captured|Hovering, leftData1, {15.0f, 15.0f}},
            /* These actually happen on rightData. Hovering isn't set as the
               event happens outside of the hovered node rectangle. */
            {Release|Captured, leftData2, {30.0f, 10.0f}},
            {Release|Captured, leftData1, {30.0f, 10.0f}},
            /* Again, no Leave event for leftData synthesized from Release at
               the moment. If it would, it *would* be captured. */
            /* A move that happens after a release isn't captured again,
               together with a matching Enter/Leave as we're on a different
               node again. The Enter/Leave is not captured either as it didn't
               happen during a capture, but after it was released. Hovering
               isn't set as the Move is the first happening on the right
               node. */
            {Move, rightData, {15.0f, 15.0f}},
            {Leave, leftData2, {35.0f, 15.0f}},
            {Leave, leftData1, {35.0f, 15.0f}},
            {Enter|Hovering, rightData, {15.0f, 15.0f}}
        })), TestSuite::Compare::Container);

    /* Captured moves synthesize a Leave when leaving and Enter when returning
       to the captured node area, but no corresponding Enter / Leave get
       synthesized for the other nodes that may be underneath */
    } {
        /* Just to reset everything */
        /** @todo have a pointerCancelEvent() for this */
        PointerEvent eventReleaseReset{{}, Pointer::MouseLeft};
        ui.pointerReleaseEvent({10000.0f, 10000.0f}, eventReleaseReset);
        PointerMoveEvent eventMoveReset{{}, {}, {}};
        ui.pointerMoveEvent({10000.0f, 10000.0f}, eventMoveReset);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent eventPress{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({500.0f, 1000.0f}, eventPress));
        CORRADE_COMPARE(ui.currentCapturedNode(), right);
        /* No Enter/Leave events synthesized from Press at the moment, so the
           hovered node doesn't get updated until the next move */
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        PointerMoveEvent eventMove1{{}, {}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({550.0f, 1500.0f}, eventMove1));
        CORRADE_COMPARE(ui.currentCapturedNode(), right);
        CORRADE_COMPARE(ui.currentHoveredNode(), right);

        PointerMoveEvent eventMove2{{}, {}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({350.0f, 1000.0f}, eventMove2));
        CORRADE_COMPARE(ui.currentCapturedNode(), right);
        /* We're on the `left` node, but since the pointer is captured on the
           `right` node, there's no hover */
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        PointerMoveEvent eventMove3{{}, {}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({450.0f, 500.0f}, eventMove3));
        CORRADE_COMPARE(ui.currentCapturedNode(), right);
        /* Moving back on the `right` node makes it hovered again */
        CORRADE_COMPARE(ui.currentHoveredNode(), right);

        PointerEvent eventRelease{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({500.0f, 1000.0f}, eventRelease));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        /* This isn't affected by the release */
        CORRADE_COMPARE(ui.currentHoveredNode(), right);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, rightData, {10.0f, 10.0f}},
            /* A captured move on the same node. The node isn't hovered at that
               point yet so it isn't marked as such. */
            {Move|Captured, rightData, {15.0f, 15.0f}},
            /* Which synthesizes an Enter event, as before this node wasn't
               hovered. It's captured in order to allow a capture reset in its
               handler. */
            {Enter|Captured|Hovering, rightData, {15.0f, 15.0f}},
            /* A captured move outside of the node */
            {Move|Captured, rightData, {-5.0f, 10.0f}},
            /* Which synthesizes a (captured) Leave event, but no matching
               Enter event for any other node underneath */
            {Leave|Captured, rightData, {-5.0f, 10.0f}},
            /* A capture move back again. The node again isn't hovered at that
               point yet so it isn't marked as such. */
            {Move|Captured, rightData, {5.0f, 5.0f}},
            /* Which synthesizes a (captured) Enter event again, but no
               matcing Leave for any other node underneath */
            {Enter|Captured|Hovering, rightData, {5.0f, 5.0f}},
            /* A release after a move back to the node area gets the Hovering
               flag too */
            {Release|Captured|Hovering, rightData, {10.0f, 10.0f}},
        })), TestSuite::Compare::Container);

    /* Capture on the right node, then capture again on the left one. In
       practice this can only happen if a release event is missed for some
       reason. */
    } {
        /* Just to reset everything */
        /** @todo have a pointerCancelEvent() for this */
        PointerEvent eventReleaseReset{{}, Pointer::MouseLeft};
        ui.pointerReleaseEvent({10000.0f, 10000.0f}, eventReleaseReset);
        PointerMoveEvent eventMoveReset{{}, {}, {}};
        ui.pointerMoveEvent({10000.0f, 10000.0f}, eventMoveReset);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent eventPress1{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({500.0f, 1000.0f}, eventPress1));
        CORRADE_COMPARE(ui.currentCapturedNode(), right);
        /* No Enter/Leave events synthesized from Press at the moment, so the
           hovered node doesn't get updated until the next move */
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        PointerEvent eventPress2{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({300.0f, 1000.0f}, eventPress2));
        CORRADE_COMPARE(ui.currentCapturedNode(), left);
        /* No Enter/Leave events synthesized from Press at the moment, so the
           hovered node doesn't get updated until the next move */
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, rightData, {10.0f, 10.0f}},
            /* No Enter/Leave events synthesized from Press and Release at the
               moment */
            {Press|Captured, leftData2, {10.0f, 10.0f}},
            {Press|Captured, leftData1, {10.0f, 10.0f}},
        })), TestSuite::Compare::Container);
    }
}

void AbstractUserInterfaceTest::eventCaptureEdges() {
    /* framebufferSize isn't used for anything here; events should get scaled
       to (0.1, 0.01) */
    AbstractUserInterface ui{{300.0f, 200.0f}, {3000.0f, 20000.0f}, {30, 20}};

    /* Like eventEdges(), but testing the special case with event capture where
       it's used to fire Enter and Leave events and setting isHovering() to a
       correct value */

    enum Event {
        Hovering = 1,
        /* All below have to be multiplies of 2 to not clash with the above */
        Press = 2,
        Release = 4,
        Move = 8,
        Enter = 10,
        Leave = 12
    };
    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doPointerPressEvent(UnsignedInt dataId, PointerEvent& event) override {
            CORRADE_COMPARE(dataId, 1);
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Press|(event.isHovering() ? Hovering : 0), event.position());
            event.setAccepted();
        }
        void doPointerReleaseEvent(UnsignedInt dataId, PointerEvent& event) override {
            CORRADE_COMPARE(dataId, 1);
            /* The release should always happen on a captured node as that's
               the codepath that's being tested */
            CORRADE_VERIFY(event.isCaptured());
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Release|(event.isHovering() ? Hovering : 0), event.position());
            event.setAccepted();
        }
        void doPointerMoveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            CORRADE_COMPARE(dataId, 1);
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Move|(event.isHovering() ? Hovering : 0), event.position());
            event.setAccepted();
        }
        void doPointerEnterEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            CORRADE_COMPARE(dataId, 1);
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Enter|(event.isHovering() ? Hovering : 0), event.position());
        }
        void doPointerLeaveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            CORRADE_COMPARE(dataId, 1);
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Leave|(event.isHovering() ? Hovering : 0), event.position());
        }

        Containers::Array<Containers::Pair<Int, Vector2>> eventCalls;
    };

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<Layer>(layer));

    NodeHandle node = ui.createNode({20.0f, 10.0f}, {20.0f, 20.0f});
    /*DataHandle nodeData1 =*/ ui.layer<Layer>(layer).create();
    /*DataHandle nodeData2 =*/ ui.layer<Layer>(layer).create(node);

    /* Set the node as initially hovered */
    PointerMoveEvent eventMove0{{}, {}, {}};
    CORRADE_VERIFY(ui.pointerMoveEvent({300.0f, 2500.0f}, eventMove0));
    CORRADE_COMPARE(ui.currentHoveredNode(), node);

    /* Top left corner */
    {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent eventPress1{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({300.0f, 2500.0f}, eventPress1));
        CORRADE_COMPARE(ui.currentCapturedNode(), node);
        CORRADE_COMPARE(ui.currentHoveredNode(), node);

        /* Move outside isn't hovering */
        PointerMoveEvent eventMove1{{}, {}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({199.0f, 990.0f}, eventMove1));
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        /* Move inside isn't hovering, but emits an Enter that is */
        PointerMoveEvent eventMove2{{}, {}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({200.0f, 1000.0f}, eventMove2));
        CORRADE_COMPARE(ui.currentHoveredNode(), node);

        /* Another move inside is now hovering */
        PointerMoveEvent eventMove3{{}, {}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({200.0f, 1000.0f}, eventMove3));

        /* The first release should be hovering, second not */
        PointerEvent eventRelease1{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({200.0f, 1000.0f}, eventRelease1));

        PointerEvent eventPress2{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({300.0f, 2500.0f}, eventPress2));

        PointerEvent eventRelease2{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({199.0f, 990.0f}, eventRelease2));

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Pair<Int, Vector2>>({
            {Press|Hovering, {10.0f, 15.0f}},
            {Move, {-0.1f, -0.1f}},
            {Leave, {-0.1f, -0.1f}},
            {Move, {0.0f, 0.0f}},
            {Enter|Hovering, {0.0f, 0.0f}},
            {Move|Hovering, {0.0f, 0.0f}},
            {Release|Hovering, {0.0f, 0.0f}},
            {Press|Hovering, {10.0f, 15.0f}},
            {Release, {-0.1f, -0.1f}},
        })), TestSuite::Compare::Container);

    /* Top edge */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent eventPress{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({300.0f, 2500.0f}, eventPress));
        CORRADE_COMPARE(ui.currentCapturedNode(), node);
        CORRADE_COMPARE(ui.currentHoveredNode(), node);

        /* Move outside isn't hovering */
        PointerMoveEvent eventMove1{{}, {}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({300.0f, 990.0f}, eventMove1));
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        /* Move inside isn't hovering, but emits an Enter that is */
        PointerMoveEvent eventMove2{{}, {}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({300.0f, 1000.0f}, eventMove2));
        CORRADE_COMPARE(ui.currentHoveredNode(), node);

        /* Another move inside is now hovering */
        PointerMoveEvent eventMove3{{}, {}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({300.0f, 1000.0f}, eventMove3));

        /* The first release should be hovering, second not */
        PointerEvent eventRelease1{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({300.0f, 1000.0f}, eventRelease1));

        PointerEvent eventPress2{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({300.0f, 2500.0f}, eventPress2));

        PointerEvent eventRelease2{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({300.0f, 990.0f}, eventRelease2));

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Pair<Int, Vector2>>({
            {Press|Hovering, {10.0f, 15.0f}},
            {Move, {10.0f, -0.1f}},
            {Leave, {10.0f, -0.1f}},
            {Move, {10.0f, 0.0f}},
            {Enter|Hovering, {10.0f, 0.0f}},
            {Move|Hovering, {10.0f, 0.0f}},
            {Release|Hovering, {10.0f, 0.0f}},
            {Press|Hovering, {10.0f, 15.0f}},
            {Release, {10.0f, -0.1f}},
        })), TestSuite::Compare::Container);

    /* Left edge */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent eventPress{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({300.0f, 2500.0f}, eventPress));
        CORRADE_COMPARE(ui.currentCapturedNode(), node);
        CORRADE_COMPARE(ui.currentHoveredNode(), node);

        /* Move outside isn't hovering */
        PointerMoveEvent eventMove1{{}, {}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({199.0f, 2500.0f}, eventMove1));
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        /* Move inside isn't hovering, but emits an Enter that is */
        PointerMoveEvent eventMove2{{}, {}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({200.0f, 2500.0f}, eventMove2));
        CORRADE_COMPARE(ui.currentHoveredNode(), node);

        /* Another move inside is now hovering */
        PointerMoveEvent eventMove3{{}, {}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({200.0f, 2500.0f}, eventMove3));

        /* The first release should be hovering, second not */
        PointerEvent eventRelease1{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({200.0f, 2500.0f}, eventRelease1));

        PointerEvent eventPress2{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({300.0f, 2500.0f}, eventPress2));

        PointerEvent eventRelease2{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({199.0f, 2500.0f}, eventRelease2));

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Pair<Int, Vector2>>({
            {Press|Hovering, {10.0f, 15.0f}},
            {Move, {-0.1f, 15.0f}},
            {Leave, {-0.1f, 15.0f}},
            {Move, {0.0f, 15.0f}},
            {Enter|Hovering, {0.0f, 15.0f}},
            {Move|Hovering, {0.0f, 15.0f}},
            {Release|Hovering, {0.0f, 15.0f}},
            {Press|Hovering, {10.0f, 15.0f}},
            {Release, {-0.1f, 15.0f}},
        })), TestSuite::Compare::Container);

    /* Bottom right corner */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent eventPress{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({300.0f, 2500.0f}, eventPress));
        CORRADE_COMPARE(ui.currentCapturedNode(), node);
        CORRADE_COMPARE(ui.currentHoveredNode(), node);

        /* Move outside isn't hovering */
        PointerMoveEvent eventMove1{{}, {}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({400.0f, 3000.0f}, eventMove1));
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        /* Move inside isn't hovering, but emits an Enter that is */
        PointerMoveEvent eventMove2{{}, {}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({399.0f, 2990.0f}, eventMove2));
        CORRADE_COMPARE(ui.currentHoveredNode(), node);

        /* Another move inside is now hovering */
        PointerMoveEvent eventMove3{{}, {}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({399.0f, 2990.0f}, eventMove3));

        /* The first release should be hovering, second not */
        PointerEvent eventRelease1{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({399.0f, 2990.0f}, eventRelease1));

        PointerEvent eventPress2{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({300.0f, 2500.0f}, eventPress2));

        PointerEvent eventRelease2{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({400.0f, 3000.0f}, eventRelease2));

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Pair<Int, Vector2>>({
            {Press|Hovering, {10.0f, 15.0f}},
            {Move, {20.0f, 20.0f}},
            {Leave, {20.0f, 20.0f}},
            {Move, {19.9f, 19.9f}},
            {Enter|Hovering, {19.9f, 19.9f}},
            {Move|Hovering, {19.9f, 19.9f}},
            {Release|Hovering, {19.9f, 19.9f}},
            {Press|Hovering, {10.0f, 15.0f}},
            {Release, {20.0f, 20.0f}},
        })), TestSuite::Compare::Container);

    /* Bottom edge */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent eventPress{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({300.0f, 2500.0f}, eventPress));
        CORRADE_COMPARE(ui.currentCapturedNode(), node);
        CORRADE_COMPARE(ui.currentHoveredNode(), node);

        /* Move outside isn't hovering */
        PointerMoveEvent eventMove1{{}, {}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({300.0f, 3000.0f}, eventMove1));
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        /* Move inside isn't hovering, but emits an Enter that is */
        PointerMoveEvent eventMove2{{}, {}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({300.0f, 2990.0f}, eventMove2));
        CORRADE_COMPARE(ui.currentHoveredNode(), node);

        /* Another move inside is now hovering */
        PointerMoveEvent eventMove3{{}, {}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({300.0f, 2990.0f}, eventMove3));

        /* The first release should be hovering, second not */
        PointerEvent eventRelease1{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({300.0f, 2990.0f}, eventRelease1));

        PointerEvent eventPress2{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({300.0f, 2500.0f}, eventPress2));

        PointerEvent eventRelease2{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({300.0f, 3000.0f}, eventRelease2));

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Pair<Int, Vector2>>({
            {Press|Hovering, {10.0f, 15.0f}},
            {Move, {10.0f, 20.0f}},
            {Leave, {10.0f, 20.0f}},
            {Move, {10.0f, 19.9f}},
            {Enter|Hovering, {10.0f, 19.9f}},
            {Move|Hovering, {10.0f, 19.9f}},
            {Release|Hovering, {10.0f, 19.9f}},
            {Press|Hovering, {10.0f, 15.0f}},
            {Release, {10.0f, 20.0f}},
        })), TestSuite::Compare::Container);

    /* Right edge */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent eventPress{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({300.0f, 2500.0f}, eventPress));
        CORRADE_COMPARE(ui.currentCapturedNode(), node);
        CORRADE_COMPARE(ui.currentHoveredNode(), node);

        PointerMoveEvent eventMove1{{}, {}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({400.0f, 2500.0f}, eventMove1));
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        PointerMoveEvent eventMove2{{}, {}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({399.0f, 2500.0f}, eventMove2));
        CORRADE_COMPARE(ui.currentHoveredNode(), node);

        /* Another move inside is now hovering */
        PointerMoveEvent eventMove3{{}, {}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({399.0f, 2500.0f}, eventMove3));

        /* The first release should be hovering, second not */
        PointerEvent eventRelease1{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({399.0f, 2500.0f}, eventRelease1));

        PointerEvent eventPress2{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({300.0f, 2500.0f}, eventPress2));

        PointerEvent eventRelease2{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({400.0f, 2500.0f}, eventRelease2));

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Pair<Int, Vector2>>({
            {Press|Hovering, {10.0f, 15.0f}},
            {Move, {20.0f, 15.0f}},
            {Leave, {20.0f, 15.0f}},
            {Move, {19.9f, 15.0f}},
            {Enter|Hovering, {19.9f, 15.0f}},
            {Move|Hovering, {19.9f, 15.0f}},
            {Release|Hovering, {19.9f, 15.0f}},
            {Press|Hovering, {10.0f, 15.0f}},
            {Release, {20.0f, 15.0f}},
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
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doPointerPressEvent(UnsignedInt dataId, PointerEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Press|(event.isCaptured() ? Captured : 0), dataHandle(handle(), dataId, 1), event.position());
            if((accept2 && dataId % 2 == 0) || (accept1 && dataId == 1))
                event.setAccepted();
        }
        void doPointerReleaseEvent(UnsignedInt dataId, PointerEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Release|(event.isCaptured() ? Captured : 0), dataHandle(handle(), dataId, 1), event.position());
            if((accept2 && dataId % 2 == 0) || (accept1 && dataId == 1))
                event.setAccepted();
        }
        /* tapOrClick event test in a dedicated eventTapOrClick() below as it
           would add too much noise here */
        void doPointerMoveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Move|(event.isCaptured() ? Captured : 0), dataHandle(handle(), dataId, 1), event.position());
            if((accept2 && dataId % 2 == 0) || (accept1 && dataId == 1))
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

        bool accept2 = true,
            accept1 = true;
        Containers::Array<Containers::Triple<Int, DataHandle, Vector2>> eventCalls;
    };

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<Layer>(layer));

    /* Two nodes next to each other */
    NodeHandle left = ui.createNode({20.0f, 0.0f}, {20.0f, 20.0f});
    NodeHandle right = ui.createNode({40.0f, 0.0f}, {20.0f, 20.0f});
    DataHandle leftData1 = ui.layer<Layer>(layer).create(left);
    /* Attachment order doesn't matter, it'll always pick rightData2 first
       because it has higher ID */
    DataHandle rightData1 = ui.layer<Layer>(layer).create(right);
    DataHandle rightData2 = ui.layer<Layer>(layer).create(right);

    /* If the press event isn't accepted, no capture should happen, so the
       release happens on the actual node that is under */
    {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent eventPress{{}, Pointer::MouseLeft};
        ui.layer<Layer>(layer).accept1 = false;
        ui.layer<Layer>(layer).accept2 = false;
        CORRADE_VERIFY(!ui.pointerPressEvent({30.0f, 10.0f}, eventPress));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        PointerEvent eventRelease{{}, Pointer::MouseLeft};
        ui.layer<Layer>(layer).accept1 = false;
        ui.layer<Layer>(layer).accept2 = true;
        CORRADE_VERIFY(ui.pointerReleaseEvent({50.0f, 10.0f}, eventRelease));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        /* No Enter/Leave events synthesized from Release at the moment, so the
           hovered node doesn't get updated until the next move */
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, leftData1, {10.0f, 10.0f}},
            /* The release event isn't happening on a captured node, so
               isCaptured() is false for it */
            {Release, rightData2, {10.0f, 10.0f}},
            {Release, rightData1, {10.0f, 10.0f}},
        })), TestSuite::Compare::Container);

    /* Same, but move instead of release */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent eventPress{{}, Pointer::MouseLeft};
        ui.layer<Layer>(layer).accept1 = false;
        ui.layer<Layer>(layer).accept2 = false;
        CORRADE_VERIFY(!ui.pointerPressEvent({30.0f, 10.0f}, eventPress));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        PointerMoveEvent eventMove{{}, {}, {}};
        ui.layer<Layer>(layer).accept1 = false;
        ui.layer<Layer>(layer).accept2 = true;
        CORRADE_VERIFY(ui.pointerMoveEvent({50.0f, 10.0f}, eventMove));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), right);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, leftData1, {10.0f, 10.0f}},
            /* The move event isn't happening on a captured node, so
               isCaptured() is false for it */
            {Move, rightData2, {10.0f, 10.0f}},
            {Move, rightData1, {10.0f, 10.0f}},
            {Enter, rightData2, {10.0f, 10.0f}},
            {Enter, rightData1, {10.0f, 10.0f}},
        })), TestSuite::Compare::Container);

    /* If the release event isn't accepted, the capture should still get
       reset nevertheless */
    } {
        /* Just to reset everything */
        /** @todo have a pointerCancelEvent() for this */
        PointerEvent eventReleaseReset{{}, Pointer::MouseLeft};
        ui.pointerReleaseEvent({1000.0f, 1000.0f}, eventReleaseReset);
        PointerMoveEvent eventMoveReset{{}, {}, {}};
        ui.pointerMoveEvent({1000.0f, 1000.0f}, eventMoveReset);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent eventPress{{}, Pointer::MouseLeft};
        ui.layer<Layer>(layer).accept1 = false;
        ui.layer<Layer>(layer).accept2 = true;
        CORRADE_VERIFY(ui.pointerPressEvent({30.0f, 10.0f}, eventPress));
        CORRADE_COMPARE(ui.currentCapturedNode(), left);
        /* No Enter/Leave events synthesized from Press at the moment, so the
           hovered node doesn't get updated until the next move */
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        PointerEvent eventRelease{{}, Pointer::MouseLeft};
        ui.layer<Layer>(layer).accept1 = false;
        ui.layer<Layer>(layer).accept2 = false;
        CORRADE_VERIFY(!ui.pointerReleaseEvent({50.0f, 10.0f}, eventRelease));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, leftData1, {10.0f, 10.0f}},
            {Release|Captured, leftData1, {30.0f, 10.0f}}, /* actually on rightData */
        })), TestSuite::Compare::Container);

    /* With move however, it should stay, even if it isn't accepted */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent eventPress{{}, Pointer::MouseLeft};
        ui.layer<Layer>(layer).accept1 = false;
        ui.layer<Layer>(layer).accept2 = true;
        CORRADE_VERIFY(ui.pointerPressEvent({30.0f, 10.0f}, eventPress));
        CORRADE_COMPARE(ui.currentCapturedNode(), left);
        /* No Enter/Leave events synthesized from Press at the moment, so the
           hovered node doesn't get updated until the next move */
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        PointerMoveEvent eventMove{{}, {}, {}};
        ui.layer<Layer>(layer).accept1 = false;
        ui.layer<Layer>(layer).accept2 = false;
        CORRADE_VERIFY(!ui.pointerMoveEvent({50.0f, 10.0f}, eventMove));
        CORRADE_COMPARE(ui.currentCapturedNode(), left);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, leftData1, {10.0f, 10.0f}},
            {Move|Captured, leftData1, {30.0f, 10.0f}}, /* actually on rightData */
            /* No node was hovered before, so no Leave is emitted */
        })), TestSuite::Compare::Container);

    /* Moving on the same node but not accepting the move causes Enter / Leave
       to be generated */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent eventPress{{}, Pointer::MouseLeft};
        ui.layer<Layer>(layer).accept1 = false;
        ui.layer<Layer>(layer).accept2 = true;
        CORRADE_VERIFY(ui.pointerPressEvent({30.0f, 10.0f}, eventPress));
        CORRADE_COMPARE(ui.currentCapturedNode(), left);
        /* No Enter/Leave events synthesized from Press at the moment, so the
           hovered node doesn't get updated until the next move */
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        PointerMoveEvent eventMove1{{}, {}, {}};
        ui.layer<Layer>(layer).accept1 = false;
        ui.layer<Layer>(layer).accept2 = false;
        CORRADE_VERIFY(!ui.pointerMoveEvent({35.0f, 15.0f}, eventMove1));
        CORRADE_COMPARE(ui.currentCapturedNode(), left);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        PointerMoveEvent eventMove2{{}, {}, {}};
        ui.layer<Layer>(layer).accept1 = false;
        ui.layer<Layer>(layer).accept2 = true;
        CORRADE_VERIFY(ui.pointerMoveEvent({30.0f, 10.0f}, eventMove2));
        CORRADE_COMPARE(ui.currentCapturedNode(), left);
        CORRADE_COMPARE(ui.currentHoveredNode(), left);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, leftData1, {10.0f, 10.0f}},
            {Move|Captured, leftData1, {15.0f, 15.0f}},
            /* No node was hovered before, so no Leave is emitted */
            {Move|Captured, leftData1, {10.0f, 10.0f}},
            {Enter|Captured, leftData1, {10.0f, 10.0f}},
        })), TestSuite::Compare::Container);

    /* Moving on the same node but capturing on a different data should *not*
       cause Enter / Leave to be generated */
    } {
        /* Just to reset everything */
        /** @todo have a pointerCancelEvent() for this */
        PointerEvent eventReleaseReset{{}, Pointer::MouseLeft};
        ui.pointerReleaseEvent({1000.0f, 1000.0f}, eventReleaseReset);
        PointerMoveEvent eventMoveReset{{}, {}, {}};
        ui.pointerMoveEvent({1000.0f, 1000.0f}, eventMoveReset);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        ui.layer<Layer>(layer).eventCalls = {};

        PointerMoveEvent eventMove1{{}, {}, {}};
        ui.layer<Layer>(layer).accept1 = true;
        ui.layer<Layer>(layer).accept2 = false;
        CORRADE_VERIFY(ui.pointerMoveEvent({50.0f, 10.0f}, eventMove1));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), right);

        PointerEvent eventPress{{}, Pointer::MouseLeft};
        ui.layer<Layer>(layer).accept1 = false;
        ui.layer<Layer>(layer).accept2 = true;
        CORRADE_VERIFY(ui.pointerPressEvent({50.0f, 10.0f}, eventPress));
        /* Capture is rightData1 */
        CORRADE_COMPARE(ui.currentCapturedNode(), right);
        /* Hover stays from the previous move, rightData2 */
        CORRADE_COMPARE(ui.currentHoveredNode(), right);

        PointerMoveEvent eventMove2{{}, {}, {}};
        ui.layer<Layer>(layer).accept1 = false;
        ui.layer<Layer>(layer).accept2 = true;
        CORRADE_VERIFY(ui.pointerMoveEvent({55.0f, 15.0f}, eventMove2));
        CORRADE_COMPARE(ui.currentCapturedNode(), right);
        /* The hovered node should now be rightData1 */
        CORRADE_COMPARE(ui.currentHoveredNode(), right);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Move, rightData2, {10.0f, 10.0f}}, /* not accepted */
            {Move, rightData1, {10.0f, 10.0f}},
            {Enter, rightData2, {10.0f, 10.0f}},
            {Enter, rightData1, {10.0f, 10.0f}},
            {Press|Captured, rightData2, {10.0f, 10.0f}},
            {Press|Captured, rightData1, {10.0f, 10.0f}},
            {Move|Captured, rightData2, {15.0f, 15.0f}},
            {Move|Captured, rightData1, {15.0f, 15.0f}},
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
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doPointerPressEvent(UnsignedInt dataId, PointerEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Press|(event.isCaptured() ? Captured : 0), dataHandle(handle(), dataId, 1), event.position());
            if(capture && capture->second() & dataId)
                event.setCaptured(capture->first());
            event.setAccepted();
        }
        void doPointerReleaseEvent(UnsignedInt dataId, PointerEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Release|(event.isCaptured() ? Captured : 0), dataHandle(handle(), dataId, 1), event.position());
            if(capture && capture->second() & dataId)
                event.setCaptured(capture->first());
            event.setAccepted();
        }
        /* tapOrClick event test in a dedicated eventTapOrClick() below as it
           would add too much noise here */
        void doPointerMoveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Move|(event.isCaptured() ? Captured : 0), dataHandle(handle(), dataId, 1), event.position());
            if(capture && capture->second() & dataId)
                event.setCaptured(capture->first());
            event.setAccepted();
        }
        void doPointerEnterEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Enter|(event.isCaptured() ? Captured : 0), dataHandle(handle(), dataId, 1), event.position());
            if(captureEnter && captureEnter->second() & dataId)
                event.setCaptured(captureEnter->first());
        }
        void doPointerLeaveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Leave|(event.isCaptured() ? Captured : 0), dataHandle(handle(), dataId, 1), event.position());
            if(captureLeave && captureLeave->second() & dataId)
                event.setCaptured(captureLeave->first());
        }

        /* Second member is a mask, not ID to be able to select more than one
           data for which the capture should be set. */
        Containers::Optional<Containers::Pair<bool, Int>> capture;
        Containers::Optional<Containers::Pair<bool, Int>> captureEnter;
        Containers::Optional<Containers::Pair<bool, Int>> captureLeave;
        Containers::Array<Containers::Triple<Int, DataHandle, Vector2>> eventCalls;
    };

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<Layer>(layer));

    /* Two nodes next to each other */
    NodeHandle left = ui.createNode({20.0f, 0.0f}, {20.0f, 20.0f});
    NodeHandle right = ui.createNode({40.0f, 0.0f}, {20.0f, 20.0f});
    /* Three data, using ID 1, 2 and 4 for easier masking */
    /*DataHandle data0 =*/ ui.layer<Layer>(layer).create();
    DataHandle leftData1 = ui.layer<Layer>(layer).create(left);
    DataHandle leftData2 = ui.layer<Layer>(layer).create(left);
    /*DataHandle data3 =*/ ui.layer<Layer>(layer).create();
    DataHandle rightData4 = ui.layer<Layer>(layer).create(right);

    /* If capture is disabled on press, the release happens on the actual node
       that is under. The capture status is preserved across calls, so if
       it gets disabled for one data, the second data get it disabled
       already. */
    {
        ui.layer<Layer>(layer).eventCalls = {};

        /* The leftData2 resets the capture already, so the second only passes
           it through */
        PointerEvent eventPress{{}, Pointer::MouseLeft};
        ui.layer<Layer>(layer).capture = Containers::pair(false, 2);
        ui.layer<Layer>(layer).captureEnter = {};
        ui.layer<Layer>(layer).captureLeave = {};
        CORRADE_VERIFY(ui.pointerPressEvent({30.0f, 10.0f}, eventPress));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        /* No Enter/Leave events synthesized from Press at the moment, so the
           hovered node doesn't get updated until the next move */
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        PointerEvent eventRelease{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({50.0f, 10.0f}, eventRelease));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        /* No Enter/Leave events synthesized from Release at the moment, so the
           hovered node doesn't get updated until the next move */
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, leftData2, {10.0f, 10.0f}},
            {Press, leftData1, {10.0f, 10.0f}},
            /* The release event isn't happening on a captured node, so
               isCaptured() is false for it */
            {Release, rightData4, {10.0f, 10.0f}},
        })), TestSuite::Compare::Container);

    /* Like above, but with leftData1 resetting the capture instead */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent eventPress{{}, Pointer::MouseLeft};
        ui.layer<Layer>(layer).capture = Containers::pair(false, 1);
        ui.layer<Layer>(layer).captureEnter = {};
        ui.layer<Layer>(layer).captureLeave = {};
        CORRADE_VERIFY(ui.pointerPressEvent({30.0f, 10.0f}, eventPress));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        PointerEvent eventRelease{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({50.0f, 10.0f}, eventRelease));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, leftData2, {10.0f, 10.0f}},
            {Press|Captured, leftData1, {10.0f, 10.0f}},
            {Release, rightData4, {10.0f, 10.0f}},
        })), TestSuite::Compare::Container);

    /* Same for move */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        /* The leftData2 resets the capture already, so the second only passes
           it through */
        PointerEvent eventPress{{}, Pointer::MouseLeft};
        ui.layer<Layer>(layer).capture = Containers::pair(false, 2);
        ui.layer<Layer>(layer).captureEnter = {};
        ui.layer<Layer>(layer).captureLeave = {};
        CORRADE_VERIFY(ui.pointerPressEvent({30.0f, 10.0f}, eventPress));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        /* No Enter/Leave events synthesized from Press at the moment, so the
           hovered node doesn't get updated until the next move */
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        PointerMoveEvent eventMove{{}, {}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({50.0f, 10.0f}, eventMove));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), right);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, leftData2, {10.0f, 10.0f}},
            {Press, leftData1, {10.0f, 10.0f}},
            /* The move event isn't happening on a captured node, so
               isCaptured() is false for it */
            {Move, rightData4, {10.0f, 10.0f}},
            {Enter, rightData4, {10.0f, 10.0f}}
        })), TestSuite::Compare::Container);

    /* Like above, but with leftData1 resetting the capture instead */
    } {
        /* Just to reset everything */
        /** @todo have a pointerCancelEvent() for this */
        PointerEvent eventReleaseReset{{}, Pointer::MouseLeft};
        ui.pointerReleaseEvent({1000.0f, 1000.0f}, eventReleaseReset);
        PointerMoveEvent eventMoveReset{{}, {}, {}};
        ui.pointerMoveEvent({1000.0f, 1000.0f}, eventMoveReset);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        ui.layer<Layer>(layer).eventCalls = {};

        PointerEvent eventPress{{}, Pointer::MouseLeft};
        ui.layer<Layer>(layer).capture = Containers::pair(false, 1);
        ui.layer<Layer>(layer).captureEnter = {};
        ui.layer<Layer>(layer).captureLeave = {};
        CORRADE_VERIFY(ui.pointerPressEvent({30.0f, 10.0f}, eventPress));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        PointerMoveEvent eventMove{{}, {}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({50.0f, 10.0f}, eventMove));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), right);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, leftData2, {10.0f, 10.0f}},
            {Press|Captured, leftData1, {10.0f, 10.0f}},
            {Move, rightData4, {10.0f, 10.0f}},
            {Enter, rightData4, {10.0f, 10.0f}}
        })), TestSuite::Compare::Container);

    /* If capture is disabled on release, it doesn't affect anything, except
       for the isCaptured() bit being false for subsequent data */
    } {
        /* Just to reset everything */
        /** @todo have a pointerCancelEvent() for this */
        PointerEvent eventReleaseReset{{}, Pointer::MouseLeft};
        ui.pointerReleaseEvent({1000.0f, 1000.0f}, eventReleaseReset);
        PointerMoveEvent eventMoveReset{{}, {}, {}};
        ui.pointerMoveEvent({1000.0f, 1000.0f}, eventMoveReset);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        ui.layer<Layer>(layer).eventCalls = {};

        /* Captures implicitly */
        PointerEvent eventPress{{}, Pointer::MouseLeft};
        ui.layer<Layer>(layer).capture = {};
        ui.layer<Layer>(layer).captureEnter = {};
        ui.layer<Layer>(layer).captureLeave = {};
        CORRADE_VERIFY(ui.pointerPressEvent({30.0f, 10.0f}, eventPress));
        CORRADE_COMPARE(ui.currentCapturedNode(), left);
        /* No Enter/Leave events synthesized from Press at the moment, so the
           hovered node doesn't get updated until the next move */
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        /* Releases on leftData2 already */
        PointerEvent eventRelease{{}, Pointer::MouseLeft};
        ui.layer<Layer>(layer).capture = Containers::pair(false, 7);
        ui.layer<Layer>(layer).captureEnter = {};
        ui.layer<Layer>(layer).captureLeave = {};
        CORRADE_VERIFY(ui.pointerReleaseEvent({50.0f, 10.0f}, eventRelease));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        /* No Enter/Leave events synthesized from Release at the moment, so the
           hovered node doesn't get updated until the next move */
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, leftData2, {10.0f, 10.0f}},
            {Press|Captured, leftData1, {10.0f, 10.0f}},
            {Release|Captured, leftData2, {30.0f, 10.0f}}, /* actually on rightData */
            /** @todo or have the isCaptured() bit reset back for all releases? */
            {Release, leftData1, {30.0f, 10.0f}}, /* actually on rightData */
        })), TestSuite::Compare::Container);

    /* For a move the capture can be disabled and re-enabled again. The next
       (move/release) event then happens either on the captured node or the
       actual node that's under. */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        /* Capture is implicit on a press, so this is the same as
           capture = {} */
        PointerEvent eventPress{{}, Pointer::MouseLeft};
        ui.layer<Layer>(layer).capture = Containers::pair(true, 7);
        ui.layer<Layer>(layer).captureEnter = {};
        ui.layer<Layer>(layer).captureLeave = {};
        CORRADE_VERIFY(ui.pointerPressEvent({30.0f, 10.0f}, eventPress));
        CORRADE_COMPARE(ui.currentCapturedNode(), left);
        /* No Enter/Leave events synthesized from Press at the moment, so the
           hovered node doesn't get updated until the next move */
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        /* leftData2 move will reset the capture, leftData1 move will pass it
           through unchanged */
        PointerMoveEvent eventMove1{{}, {}, {}};
        ui.layer<Layer>(layer).capture = Containers::pair(false, 2);
        ui.layer<Layer>(layer).captureEnter = {};
        ui.layer<Layer>(layer).captureLeave = {};
        CORRADE_VERIFY(ui.pointerMoveEvent({50.0f, 10.0f}, eventMove1));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        /* The event removed the capture, however it's not looking for a
           now-hovered node as that would mean doing the whole bubbling
           again */
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        /* This happens on (uncaptured) rightData4, which will capture it */
        PointerMoveEvent eventMove2{{}, {}, {}};
        ui.layer<Layer>(layer).capture = Containers::pair(true, 4);
        ui.layer<Layer>(layer).captureEnter = {};
        ui.layer<Layer>(layer).captureLeave = {};
        CORRADE_VERIFY(ui.pointerMoveEvent({55.0f, 15.0f}, eventMove2));
        CORRADE_COMPARE(ui.currentCapturedNode(), right);
        CORRADE_COMPARE(ui.currentHoveredNode(), right);

        /* rightData4 will reset the capture here */
        PointerMoveEvent eventMove3{{}, {}, {}};
        ui.layer<Layer>(layer).capture = Containers::pair(false, 4);
        ui.layer<Layer>(layer).captureEnter = {};
        ui.layer<Layer>(layer).captureLeave = {};
        CORRADE_VERIFY(ui.pointerMoveEvent({30.0f, 10.0f}, eventMove3));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        /* Again, as the event removed the capture there's no node to be
           hovered anymore */
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, leftData2, {10.0f, 10.0f}},
            {Press|Captured, leftData1, {10.0f, 10.0f}},
            {Move|Captured, leftData2, {30.0f, 10.0f}}, /* actually on rightData */
            {Move, leftData1, {30.0f, 10.0f}}, /* actually on rightData */
            /* No Enter/Leave as the hovered node stays null */
            {Move, rightData4, {15.0f, 15.0f}},
            /* Enter is captured as the move captured it */
            {Enter|Captured, rightData4, {15.0f, 15.0f}},
            {Move|Captured, rightData4, {-10.0f, 10.0f}}, /* actually on leftData */
            /* Leave not captured anymore as the move released it */
            {Leave, rightData4, {-10.0f, 10.0f}}
        })), TestSuite::Compare::Container);

    /* Capturing should also be possible on an uncaptured Enter event */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerMoveEvent eventMove1{{}, {}, {}};
        ui.layer<Layer>(layer).capture = {};
        ui.layer<Layer>(layer).captureEnter = {};
        ui.layer<Layer>(layer).captureLeave = {};
        CORRADE_VERIFY(ui.pointerMoveEvent({50.0f, 10.0f}, eventMove1));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), right);

        /* Capture happens on leftData2, leftData1 will pass it through
           unchanged */
        PointerMoveEvent eventMove2{{}, {}, {}};
        ui.layer<Layer>(layer).capture = {};
        ui.layer<Layer>(layer).captureEnter = Containers::pair(true, 2);
        ui.layer<Layer>(layer).captureLeave = {};
        CORRADE_VERIFY(ui.pointerMoveEvent({30.0f, 15.0f}, eventMove2));
        CORRADE_COMPARE(ui.currentCapturedNode(), left);
        CORRADE_COMPARE(ui.currentHoveredNode(), left);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Move, rightData4, {10.0f, 10.0f}},
            {Enter, rightData4, {10.0f, 10.0f}},
            {Move, leftData2, {10.0f, 15.0f}},
            {Move, leftData1, {10.0f, 15.0f}},
            {Leave, rightData4, {-10.0f, 15.0f}},
            {Enter, leftData2, {10.0f, 15.0f}},
            {Enter|Captured, leftData1, {10.0f, 15.0f}}
        })), TestSuite::Compare::Container);

    /* Releasing the capture should also be possible on a captured Leave
       event */
    } {
        /* Just to reset everything */
        /** @todo have a pointerCancelEvent() for this */
        PointerEvent eventReleaseReset{{}, Pointer::MouseLeft};
        ui.pointerReleaseEvent({1000.0f, 1000.0f}, eventReleaseReset);
        PointerMoveEvent eventMoveReset{{}, {}, {}};
        ui.pointerMoveEvent({1000.0f, 1000.0f}, eventMoveReset);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        ui.layer<Layer>(layer).eventCalls = {};

        /* Capture is unchanged with leftData2, leftData1 will enable it */
        PointerMoveEvent eventMove1{{}, {}, {}};
        ui.layer<Layer>(layer).capture = Containers::pair(true, 1);
        ui.layer<Layer>(layer).captureEnter = {};
        ui.layer<Layer>(layer).captureLeave = {};
        CORRADE_VERIFY(ui.pointerMoveEvent({30.0f, 10.0f}, eventMove1));
        CORRADE_COMPARE(ui.currentCapturedNode(), left);
        CORRADE_COMPARE(ui.currentHoveredNode(), left);

        /* Capture is reset with leftData2, leftData1 passes it through
           unchanged */
        PointerMoveEvent eventMove2{{}, {}, {}};
        ui.layer<Layer>(layer).capture = {};
        ui.layer<Layer>(layer).captureEnter = {};
        ui.layer<Layer>(layer).captureLeave = Containers::pair(false, 2);
        CORRADE_VERIFY(ui.pointerMoveEvent({50.0f, 15.0f}, eventMove2));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        /* The event removed the capture, however it's not looking for a
           now-hovered node as that would mean doing the whole bubbling
           again */
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Move, leftData2, {10.0f, 10.0f}},
            {Move, leftData1, {10.0f, 10.0f}},
            /* Yes, leftData2 will have isCaptured set on Enter even though it
               didn't enable it in Move */
            {Enter|Captured, leftData2, {10.0f, 10.0f}},
            {Enter|Captured, leftData1, {10.0f, 10.0f}},
            {Move|Captured, leftData2, {30.0f, 15.0f}}, /* actually on rightData */
            {Move|Captured, leftData1, {30.0f, 15.0f}}, /* actually on rightData */
            {Leave|Captured, leftData2, {30.0f, 15.0f}},
            {Leave, leftData1, {30.0f, 15.0f}},
        })), TestSuite::Compare::Container);

    /* Capturing on an uncaptured Leave event does nothing, except for the
       isCaptured() bit being true for subsequent data */
    } {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerMoveEvent eventMove1{{}, {}, {}};
        ui.layer<Layer>(layer).capture = {};
        ui.layer<Layer>(layer).captureEnter = {};
        ui.layer<Layer>(layer).captureLeave = {};
        CORRADE_VERIFY(ui.pointerMoveEvent({30.0f, 10.0f}, eventMove1));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), left);

        /* Set on both leftData2 and leftData1 but no effect on either */
        PointerMoveEvent eventMove2{{}, {}, {}};
        ui.layer<Layer>(layer).capture = {};
        ui.layer<Layer>(layer).captureEnter = {};
        ui.layer<Layer>(layer).captureLeave = Containers::pair(true, 3);
        CORRADE_VERIFY(ui.pointerMoveEvent({50.0f, 15.0f}, eventMove2));
        /* The capture isn't changed even though the Leave requested it */
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), right);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Move, leftData2, {10.0f, 10.0f}},
            {Move, leftData1, {10.0f, 10.0f}},
            {Enter, leftData2, {10.0f, 10.0f}},
            {Enter, leftData1, {10.0f, 10.0f}},
            {Move, rightData4, {10.0f, 15.0f}},
            {Leave, leftData2, {30.0f, 15.0f}},
            /** @todo or have the isCaptured() bit reset back for all leaves? */
            {Leave|Captured, leftData1, {30.0f, 15.0f}},
            {Enter, rightData4, {10.0f, 15.0f}}
        })), TestSuite::Compare::Container);

    /* Enabling capture on an uncaptured Move and then disabling it again on an
       Enter should keep it disabled */
    } {
        /* Just to reset everything */
        /** @todo have a pointerCancelEvent() for this */
        PointerMoveEvent eventMoveReset{{}, {}, {}};
        ui.pointerMoveEvent({1000.0f, 1000.0f}, eventMoveReset);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        ui.layer<Layer>(layer).eventCalls = {};

        PointerMoveEvent eventMove1{{}, {}, {}};
        ui.layer<Layer>(layer).capture = {};
        ui.layer<Layer>(layer).captureEnter = {};
        ui.layer<Layer>(layer).captureLeave = {};
        CORRADE_VERIFY(ui.pointerMoveEvent({50.0f, 10.0f}, eventMove1));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), right);

        /* Enabled on leftData2 move, leftData1 move and leftData2 enter passes
           it through; leftData1 enter disables it */
        PointerMoveEvent eventMove2{{}, {}, {}};
        ui.layer<Layer>(layer).capture = Containers::pair(true, 2);
        ui.layer<Layer>(layer).captureEnter = Containers::pair(false, 1);
        ui.layer<Layer>(layer).captureLeave = {};
        CORRADE_VERIFY(ui.pointerMoveEvent({30.0f, 15.0f}, eventMove2));
        /* No capture as Enter reset it again */
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), left);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Move, rightData4, {10.0f, 10.0f}},
            {Enter, rightData4, {10.0f, 10.0f}},
            {Move, leftData2, {10.0f, 15.0f}},
            {Move|Captured, leftData1, {10.0f, 15.0f}},
            /* Leave is only captured if it happens on a captured node, here it
               happens on some other */
            {Leave, rightData4, {-10.0f, 15.0f}},
            {Enter|Captured, leftData2, {10.0f, 15.0f}},
            {Enter|Captured, leftData1, {10.0f, 15.0f}} /* disables capture */
        })), TestSuite::Compare::Container);

    /* Disabling capture on a captured Move and then enabling it again in Leave
       should keep it enabled */
    } {
        /* Just to reset everything */
        /** @todo have a pointerCancelEvent() for this */
        PointerMoveEvent eventMoveReset{{}, {}, {}};
        ui.pointerMoveEvent({1000.0f, 1000.0f}, eventMoveReset);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        ui.layer<Layer>(layer).eventCalls = {};

        /* Capture enabled only on the (second) leftData1 move */
        PointerMoveEvent eventMove1{{}, {}, {}};
        ui.layer<Layer>(layer).capture = Containers::pair(true, 1);
        ui.layer<Layer>(layer).captureEnter = {};
        ui.layer<Layer>(layer).captureLeave = {};
        CORRADE_VERIFY(ui.pointerMoveEvent({30.0f, 10.0f}, eventMove1));
        CORRADE_COMPARE(ui.currentCapturedNode(), left);
        CORRADE_COMPARE(ui.currentHoveredNode(), left);

        /* Disabled on the leftData2 move already, leftData1 move and leftData2
           leave then passes it through. Enabled again on leftData1 leave. */
        PointerMoveEvent eventMove2{{}, {}, {}};
        ui.layer<Layer>(layer).capture = Containers::pair(false, 2);
        ui.layer<Layer>(layer).captureEnter = {};
        ui.layer<Layer>(layer).captureLeave = Containers::pair(true, 1);
        CORRADE_VERIFY(ui.pointerMoveEvent({50.0f, 15.0f}, eventMove2));
        CORRADE_COMPARE(ui.currentCapturedNode(), left);
        /* On the right node, but captured on the left, so no hover */
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Move, leftData2, {10.0f, 10.0f}},
            {Move, leftData1, {10.0f, 10.0f}},
            {Enter|Captured, leftData2, {10.0f, 10.0f}},
            {Enter|Captured, leftData1, {10.0f, 10.0f}},
            {Move|Captured, leftData2, {30.0f, 15.0f}},
            {Move, leftData1, {30.0f, 15.0f}},
            /* Leave is only captured if it happens on a captured node, here it
               happens on some other */
            {Leave, leftData2, {30.0f, 15.0f}},
            {Leave, leftData1, {30.0f, 15.0f}},
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
        using AbstractLayer::create;

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
            if(accept & dataId)
                event.setAccepted();
            if(capture && capture->second() & dataId)
                event.setCaptured(capture->first());
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

        /* It's a mask, not ID to be able to select more than one data for
           which the accept/capture should be set. */
        Int accept;
        Containers::Optional<Containers::Pair<bool, Int>> capture;
        Containers::Array<Containers::Triple<Int, DataHandle, Vector2>> eventCalls;
    };

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<Layer>(layer));

    /* Two nodes on top of each other, node 1 above */
    NodeHandle node1 = ui.createNode({20.0f, 0.0f}, {20.0f, 20.0f});
    NodeHandle node2 = ui.createNode({20.0f, 0.0f}, {20.0f, 20.0f});
    /* Three data, using ID 1, 2 and 4 for easier masking */
    /*DataHandle data0 =*/ ui.layer<Layer>(layer).create();
    DataHandle data1 = ui.layer<Layer>(layer).create(node1);
    DataHandle data2 = ui.layer<Layer>(layer).create(node2);
    /*DataHandle data3 =*/ ui.layer<Layer>(layer).create();
    DataHandle data4 = ui.layer<Layer>(layer).create(node2);

    /* Setting capture in events that don't get accepted should do nothing to
       subsequent events and nothing to the end result also */
    {
        ui.layer<Layer>(layer).eventCalls = {};

        PointerMoveEvent eventMove{{}, {}, {}};
        ui.layer<Layer>(layer).accept = 1;
        ui.layer<Layer>(layer).capture = Containers::pair(true, 2);
        CORRADE_VERIFY(ui.pointerMoveEvent({30.0f, 10.0f}, eventMove));
        /* Node 2 captures in a non-accepted event, which should be ignored */
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), node1);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Move, data4, {10.0f, 10.0f}}, /* does nothing */
            {Move, data2, {10.0f, 10.0f}}, /* capturing but not accepted */
            {Move, data1, {10.0f, 10.0f}}, /* shouldn't capture */
            {Enter, data1, {10.0f, 10.0f}}, /* neither this */
        })), TestSuite::Compare::Container);

    /* Like above, but reversed -- data4 captures but isn't accepted, data2
       does nothing */
    } {
        /* Just to reset everything */
        /** @todo have a pointerCancelEvent() for this */
        PointerEvent eventReleaseReset{{}, Pointer::MouseLeft};
        ui.pointerReleaseEvent({1000.0f, 1000.0f}, eventReleaseReset);
        PointerMoveEvent eventMoveReset{{}, {}, {}};
        ui.pointerMoveEvent({1000.0f, 1000.0f}, eventMoveReset);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        ui.layer<Layer>(layer).eventCalls = {};

        PointerMoveEvent eventMove{{}, {}, {}};
        ui.layer<Layer>(layer).accept = 1;
        ui.layer<Layer>(layer).capture = Containers::pair(true, 4);
        CORRADE_VERIFY(ui.pointerMoveEvent({30.0f, 10.0f}, eventMove));
        /* Node 2 captures in a non-accepted event, which should be ignored */
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), node1);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Move, data4, {10.0f, 10.0f}}, /* capturing but not accepted */
            {Move, data2, {10.0f, 10.0f}}, /* does nothing */
            {Move, data1, {10.0f, 10.0f}}, /* shouldn't capture */
            {Enter, data1, {10.0f, 10.0f}}, /* neither this */
        })), TestSuite::Compare::Container);

    /* Cancelling capture in a non-accepted captured move event (i.e., outside
       of bounds) should still work */
    } {
        /* Just to reset everything */
        /** @todo have a pointerCancelEvent() for this */
        PointerEvent eventReleaseReset{{}, Pointer::MouseLeft};
        ui.pointerReleaseEvent({1000.0f, 1000.0f}, eventReleaseReset);
        PointerMoveEvent eventMoveReset{{}, {}, {}};
        ui.pointerMoveEvent({1000.0f, 1000.0f}, eventMoveReset);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        ui.layer<Layer>(layer).eventCalls = {};

        /* The press event accepts and captures unconditionally */
        PointerEvent eventPress{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({30.0f, 10.0f}, eventPress));
        CORRADE_COMPARE(ui.currentCapturedNode(), node2);
        /* No Enter/Leave events synthesized from Press at the moment, so the
           hovered node doesn't get updated until the next move */
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        PointerMoveEvent eventMove{{}, {}, {}};
        ui.layer<Layer>(layer).accept = 0;
        ui.layer<Layer>(layer).capture = Containers::pair(false, 4);
        CORRADE_VERIFY(!ui.pointerMoveEvent({100.0f, 100.0f}, eventMove));
        /* The capture should be reset even though the move wasn't accepted */
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, data4, {10.0f, 10.0f}},
            {Press|Captured, data2, {10.0f, 10.0f}},
            {Move|Captured, data4, {80.0f, 100.0f}}, /* cancels the capture */
            {Move, data2, {80.0f, 100.0f}},
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
        using AbstractLayer::create;

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
            /* Release / move happens outside of the node */
            CORRADE_VERIFY(!event.isHovering());
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, dataHandle(handle(), dataId, 1), event.position());
            event.setAccepted();
        }
        /* tapOrClick event test in a dedicated eventTapOrClick() below as it
           would add too much noise here */
        void doPointerMoveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            CORRADE_VERIFY(event.isCaptured());
            /* Release / move happens outside of the node */
            CORRADE_VERIFY(!event.isHovering());
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, dataHandle(handle(), dataId, 1), event.position());
            event.setAccepted();
        }
        /* No enter/leave events, those are tested in
           eventPointerMoveNodePositionUpdated() already */
        void doVisibilityLostEvent(UnsignedInt, VisibilityLostEvent&) override {
            CORRADE_FAIL("This function shouldn't be called.");
        }

        Containers::Array<Containers::Pair<DataHandle, Vector2>> eventCalls;
    };

    /* A nested node to verify the event receives up-to-date position after its
       parent gets moved */
    NodeHandle node = ui.createNode({20.0f, 0.0f}, {20.0f, 20.0f});
    NodeHandle nested = ui.createNode(node, {}, {20.0f, 20.0f});

    /* Update explicitly before adding the layer as NeedsDataAttachmentUpdate
       is a subset of this, and having just that one set may uncover accidental
       omissions in internal state updates compared to updating just once after
       creating both nodes and data */
    if(data.update) {
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<Layer>(layer));
    DataHandle nestedData = ui.layer<Layer>(layer).create(nested);

    if(data.update) {
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate);
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    PointerEvent eventPress{{}, Pointer::MouseLeft};
    CORRADE_VERIFY(ui.pointerPressEvent({300.0f, 1000.0f}, eventPress));
    CORRADE_COMPARE(ui.currentCapturedNode(), nested);

    ui.setNodeOffset(node, {30.0f, 20.0f});
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsLayoutUpdate);

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    if(data.release) {
        PointerEvent eventRelease{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({320.0f, 1000.0f}, eventRelease));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
    } else if(data.move) {
        PointerMoveEvent eventMove{{}, {}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({320.0f, 1000.0f}, eventMove));
        CORRADE_COMPARE(ui.currentCapturedNode(), nested);
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

void AbstractUserInterfaceTest::eventCaptureNodeBecomesHiddenDisabledNoEvents() {
    auto&& data = EventCaptureNodeBecomesHiddenDisabledNoEventsData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Event scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};

    enum Event {
        Captured = 1,
        Press = 2,
        Release = 4,
        Move = 6,
        VisibilityLost = 8,
        Update = 10
    };
    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

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
        /* tapOrClick event test in a dedicated eventTapOrClick() below as it
           would add too much noise here */
        void doPointerMoveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Move|(event.isCaptured() ? Captured : 0), dataHandle(handle(), dataId, 1), event.position());
            event.setAccepted();
        }
        /* No enter/leave events, those are tested in
           eventPointerMoveNodeHidden() already */
        void doVisibilityLostEvent(UnsignedInt dataId, VisibilityLostEvent& event) override {
            /* These can only be set if a focused node is no longer focusable */
            CORRADE_VERIFY(!event.isHovering());
            CORRADE_VERIFY(!event.isPressed());
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, VisibilityLost, dataHandle(handle(), dataId, 1), Vector2{});
        }
        void doUpdate(LayerStates, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, Containers::BitArrayView, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&) override {
            arrayAppend(eventCalls, InPlaceInit, Update, DataHandle::Null, Vector2{});
        }

        Containers::Array<Containers::Triple<Int, DataHandle, Vector2>> eventCalls;
    };

    /* Two nodes next to each other, nested in order to verify that the
       hidden/disabled/... flag gets propagated through the hierarchy */
    NodeHandle left = ui.createNode({20.0f, 0.0f}, {20.0f, 20.0f});
    NodeHandle leftNested = ui.createNode(left, {}, {20.0f, 20.0f});
    NodeHandle right = ui.createNode({40.0f, 0.0f}, {20.0f, 20.0f});

    /* Update explicitly before adding the layer as NeedsDataAttachmentUpdate
       is a subset of this, and having just that one set may uncover accidental
       omissions in internal state updates compared to updating just once after
       creating both nodes and data */
    if(data.update) {
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<Layer>(layer));

    DataHandle leftData = ui.layer<Layer>(layer).create(leftNested);
    DataHandle rightData = ui.layer<Layer>(layer).create(right);

    if(data.update) {
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate);
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    PointerEvent eventPress{{}, Pointer::MouseLeft};
    CORRADE_VERIFY(ui.pointerPressEvent({30.0f, 10.0f}, eventPress));
    CORRADE_COMPARE(ui.currentPressedNode(), leftNested);
    CORRADE_COMPARE(ui.currentCapturedNode(), leftNested);

    if(data.move) {
        PointerMoveEvent eventMove{{}, {}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({30.0f, 10.0f}, eventMove));
        CORRADE_COMPARE(ui.currentHoveredNode(), leftNested);
    }

    if(data.flags)
        ui.addNodeFlags(left, data.flags);
    else if(data.clearOrder)
        ui.clearNodeOrder(left);
    else CORRADE_INTERNAL_ASSERT_UNREACHABLE();
    /* The current captured node stays after setting the flags, is only updated
       after update() -- there it also handles if any parent gets the flag as
       well */
    CORRADE_COMPARE(ui.currentPressedNode(), leftNested);
    CORRADE_COMPARE(ui.currentCapturedNode(), leftNested);
    CORRADE_COMPARE(ui.state(), data.expectedState);

    if(data.update) {
        ui.update();

        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    if(data.release) {
        PointerEvent eventRelease{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({50.0f, 10.0f}, eventRelease));
    } else if(data.move) {
        PointerMoveEvent eventMove{{}, {}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({50.0f, 10.0f}, eventMove));
    } else CORRADE_INTERNAL_ASSERT_UNREACHABLE();

    CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
    CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);

    if(data.move) {
        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Update, {}, {}},
            {Press|Captured, leftData, {10.0f, 10.0f}},
            {Move|Captured, leftData, {10.0f, 10.0f}},
            /* The node used to be pressed, captured and hovered, but only one
               visibility lost event is submitted for all. It's emitted before
               a doUpdate() so the changes can be directly reflected. */
            {VisibilityLost, leftData, {}},
            {Update, {}, {}},
            /* The move event isn't happening on a captured node, so
               isCaptured() is false for it */
            {Move, rightData, {10.0f, 10.0f}},
        })), TestSuite::Compare::Container);
    } else {
        CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Update, {}, {}},
            {Press|Captured, leftData, {10.0f, 10.0f}},
            /* The node used to be both pressed and captured, but only one
               visibility lost event is submitted for both. It's emitted before
               a doUpdate() so the changes can be directly reflected. */
            {VisibilityLost, leftData, {}},
            {Update, {}, {}},
            /* The release event isn't happening on a captured node, so
                isCaptured() is false for it */
            {Release, rightData, {10.0f, 10.0f}},
        })), TestSuite::Compare::Container);
    }

    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
}

void AbstractUserInterfaceTest::eventCaptureNodeRemoved() {
    auto&& data = EventCaptureNodeRemovedData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Event scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};

    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

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
        /* tapOrClick event test in a dedicated eventTapOrClick() below as it
           would add too much noise here */
        void doPointerMoveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, dataHandle(handle(), dataId, 1), event.position(), event.isCaptured());
            event.setAccepted();
        }
        /* No enter/leave events, those are tested in
           eventPointerMoveNodeRemoved() already */
        void doVisibilityLostEvent(UnsignedInt, VisibilityLostEvent&) override {
            CORRADE_FAIL("This function shouldn't be called.");
        }

        Containers::Array<Containers::Triple<DataHandle, Vector2, bool>> eventCalls;
    };

    /* Two nodes next to each other, nested in order to verify that the removal
       gets propagated through the hierarchy */
    NodeHandle left = ui.createNode({20.0f, 0.0f}, {20.0f, 20.0f});
    NodeHandle leftNested = ui.createNode(left, {}, {20.0f, 20.0f});
    NodeHandle right = ui.createNode({40.0f, 0.0f}, {20.0f, 20.0f});

    /* Update explicitly before adding the layer as NeedsDataAttachmentUpdate
       is a subset of this, and having just that one set may uncover accidental
       omissions in internal state updates compared to updating just once after
       creating both nodes and data */
    if(data.update) {
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<Layer>(layer));
    DataHandle leftData = ui.layer<Layer>(layer).create(leftNested);
    DataHandle rightData = ui.layer<Layer>(layer).create(right);

    if(data.update) {
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate);
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    PointerEvent eventPress{{}, Pointer::MouseLeft};
    CORRADE_VERIFY(ui.pointerPressEvent({30.0f, 10.0f}, eventPress));
    CORRADE_COMPARE(ui.currentCapturedNode(), leftNested);

    ui.removeNode(data.removeParent ? left : leftNested);
    /* The current hovered node stays after removal, is only updated after
       update() -- there it also handles if any parent is removed */
    CORRADE_COMPARE(ui.currentCapturedNode(), leftNested);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeClean);

    /* Add a visible node right in place of the removed one, to verify the
       generation is correctly checked as well */
    if(!data.removeParent) {
        NodeHandle leftNestedReplacement = ui.createNode(left, {}, {20.0f, 20.0f});
        CORRADE_COMPARE(nodeHandleId(leftNestedReplacement), nodeHandleId(leftNested));
    }

    if(data.update) {
        ui.update();

        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    if(data.release) {
        PointerEvent eventRelease{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({50.0f, 10.0f}, eventRelease));
    } else if(data.move) {
        PointerMoveEvent eventMove{{}, {}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({50.0f, 10.0f}, eventMove));
    } else CORRADE_INTERNAL_ASSERT_UNREACHABLE();

    CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);

    CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<DataHandle, Vector2, bool>>({
        {leftData, {10.0f, 10.0f}, true},
        /* The release / move event isn't happening on a captured node, so
           isCaptured() is false for it */
        {rightData, {10.0f, 10.0f}, false},
    })), TestSuite::Compare::Container);

    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
}

void AbstractUserInterfaceTest::eventCaptureAllDataRemoved() {
    auto&& data = EventCaptureCleanUpdateData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Event scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};

    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;
        using AbstractLayer::remove;

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doPointerPressEvent(UnsignedInt dataId, PointerEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, dataHandle(handle(), dataId, 1), event.position(), event.isCaptured());
            event.setAccepted();
        }
        /* tapOrClick event test in a dedicated eventTapOrClick() below as it
           would add too much noise here */
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
        void doVisibilityLostEvent(UnsignedInt, VisibilityLostEvent&) override {
            CORRADE_FAIL("This function shouldn't be called.");
        }

        Containers::Array<Containers::Triple<DataHandle, Vector2, bool>> eventCalls;
    };

    /* Two nodes next to each other */
    NodeHandle left = ui.createNode({20.0f, 0.0f}, {20.0f, 20.0f});
    NodeHandle right = ui.createNode({40.0f, 0.0f}, {20.0f, 20.0f});

    /* Update explicitly before adding the layer as NeedsDataAttachmentUpdate
       is a subset of this, and having just that one set may uncover accidental
       omissions in internal state updates compared to updating just once after
       creating both nodes and data */
    if(data.update) {
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<Layer>(layer));
    DataHandle leftData = ui.layer<Layer>(layer).create(left);
    /*DataHandle rightData =*/ ui.layer<Layer>(layer).create(right);

    if(data.update) {
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate);
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    PointerEvent eventPress{{}, Pointer::MouseLeft};
    CORRADE_VERIFY(ui.pointerPressEvent({30.0f, 10.0f}, eventPress));
    CORRADE_COMPARE(ui.currentCapturedNode(), left);

    ui.layer<Layer>(layer).remove(leftData);
    /* Similar to hover behavior, the node stays captured until an actual event
       discovers there's no data anymore. This is consistent for example with a
       case where the data would change the area where they're active for
       events -- also something that the clean() / update() can't discover on
       its own, only actually firing the event can. */
    CORRADE_COMPARE(ui.currentCapturedNode(), left);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate|UserInterfaceState::NeedsDataClean);

    if(data.clean) {
        ui.clean();

        /* Same as above, the node stays captured */
        CORRADE_COMPARE(ui.currentCapturedNode(), left);
    }

    if(data.update) {
        ui.update();

        /* Same as above, the node stays captured */
        CORRADE_COMPARE(ui.currentCapturedNode(), left);
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    if(data.release) {
        PointerEvent eventRelease{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(!ui.pointerReleaseEvent({50.0f, 10.0f}, eventRelease));

        /* In case of release, the captured node gets reset implicitly
           always */
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);

    } else if(data.move) {
        PointerMoveEvent eventMove{{}, {}, {}};
        CORRADE_VERIFY(!ui.pointerMoveEvent({50.0f, 10.0f}, eventMove));

        /* In case of move, the captured node doesn't get reset if it contains
           no data. Otherwise, if one would drag out of a button and the button
           would disappear, other elements would suddenly start reacting to the
           move which goes completely against the concept of a captured
           node. */
        CORRADE_COMPARE(ui.currentCapturedNode(), left);
    } else CORRADE_INTERNAL_ASSERT_UNREACHABLE();

    CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<DataHandle, Vector2, bool>>({
        {leftData, {10.0f, 10.0f}, true},
        /* There's no captured data to execute the event on. It doesn't
           propagate anywhere either, that would go against the capturing
           behavior. */
    })), TestSuite::Compare::Container);

    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
}

void AbstractUserInterfaceTest::eventTapOrClick() {
    auto&& data = EventLayouterData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* framebufferSize isn't used for anything here; events should get scaled
       to (0.1, 0.01) */
    AbstractUserInterface ui{{300.0f, 200.0f}, {3000.0f, 20000.0f}, {30, 20}};

    enum Event {
        Captured = 1,
        Press = 2,
        Release = 4,
        TapOrClick = 6,
        Move = 8
    };

    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doPointerPressEvent(UnsignedInt dataId, PointerEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Press|(event.isCaptured() ? Captured : 0), dataHandle(handle(), dataId, 1), event.position());
            if(acceptPress)
                event.setAccepted();
            if(capturePress)
                event.setCaptured(*capturePress);
        }
        void doPointerReleaseEvent(UnsignedInt dataId, PointerEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Release|(event.isCaptured() ? Captured : 0), dataHandle(handle(), dataId, 1), event.position());
            if(acceptRelease)
                event.setAccepted();
        }
        void doPointerTapOrClickEvent(UnsignedInt dataId, PointerEvent& event) override {
            /* The time should be propagated to the synthesized event */
            CORRADE_COMPARE(event.time(), 12345_nsec);
            CORRADE_COMPARE(event.type(), Pointer::MouseLeft);
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, TapOrClick|(event.isCaptured() ? Captured : 0), dataHandle(handle(), dataId, 1), event.position());
        }
        void doPointerMoveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Move|(event.isCaptured() ? Captured : 0), dataHandle(handle(), dataId, 1), event.position());
            if(acceptMove)
                event.setAccepted();
            if(captureMove)
                event.setCaptured(*captureMove);
        }

        bool acceptPress = true,
            acceptRelease = true,
            acceptMove = true;
        Containers::Optional<bool> capturePress, captureMove;
        Containers::Array<Containers::Triple<Int, DataHandle, Vector2>> eventCalls;
    };

    struct Layouter: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::add;

        void doUpdate(Containers::BitArrayView layoutIdsToUpdate, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>& nodeOffsets, const  Containers::StridedArrayView1D<Vector2>& nodeSizes) override {
            const Containers::StridedArrayView1D<const NodeHandle> nodes = this->nodes();
            for(std::size_t i = 0; i != layoutIdsToUpdate.size(); ++i) {
                if(!layoutIdsToUpdate[i])
                    continue;
                nodeOffsets[nodeHandleId(nodes[i])].y() -= 1000.0f;
                nodeSizes[nodeHandleId(nodes[i])] *= 100.0f;
            }
        }
    };

    /* Two nodes next to each other. If the layouter is enabled, the nodes are
       shifted & scaled which makes them completely unreachable by events, and
       the layouter then undoes that */
    Vector2 baseNodeOffset{0.0f, data.layouter ? 1000.0f : 0.0f};
    Vector2 baseNodeScale{data.layouter ? 0.01f : 1.0f};
    NodeHandle left = ui.createNode(
        baseNodeOffset + Vector2{20.0f, 0.0f},
        baseNodeScale*Vector2{20.0f, 20.0f});
    NodeHandle right = ui.createNode(
        baseNodeOffset + Vector2{40.0f, 0.0f},
        baseNodeScale*Vector2{20.0f, 20.0f});
    if(data.layouter) {
        Layouter& layouter = ui.setLayouterInstance(Containers::pointer<Layouter>(ui.createLayouter()));
        layouter.add(left);
        layouter.add(right);
    }

    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer()));
    DataHandle leftData1 = layer.create(left);
    DataHandle leftData2 = layer.create(left);
    DataHandle rightData = layer.create(right);

    /* Nothing pressed initially */
    CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);

    /* Press outside of everything doesn't set any pressed node */
    {
        layer.eventCalls = {};

        PointerEvent eventPress{54321_nsec, Pointer::MouseLeft};
        layer.acceptPress = true;
        layer.capturePress = {};
        CORRADE_VERIFY(!ui.pointerPressEvent({200.0f, 5000.0f}, eventPress));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
        })), TestSuite::Compare::Container);

    /* Press that's not accepted doesn't set any pressed node */
    } {
        layer.eventCalls = {};

        PointerEvent eventPress{54321_nsec, Pointer::MouseLeft};
        layer.acceptPress = false;
        layer.capturePress = {};
        CORRADE_VERIFY(!ui.pointerPressEvent({500.0f, 1000.0f}, eventPress));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, rightData, {10.0f, 10.0f}},
        })), TestSuite::Compare::Container);

    /* Release without any press before doesn't set any pressed node */
    } {
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);

        layer.eventCalls = {};

        PointerEvent eventRelease{12345_nsec, Pointer::MouseLeft};
        layer.acceptRelease = true;
        CORRADE_VERIFY(ui.pointerReleaseEvent({500.0f, 1000.0f}, eventRelease));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Release, rightData, {10.0f, 10.0f}},
        })), TestSuite::Compare::Container);

    /* Press and release on the same node */
    } {
        layer.eventCalls = {};

        PointerEvent eventPress{54321_nsec, Pointer::MouseLeft};
        layer.acceptPress = true;
        layer.capturePress = {};
        CORRADE_VERIFY(ui.pointerPressEvent({300.0f, 1000.0f}, eventPress));
        CORRADE_COMPARE(ui.currentCapturedNode(), left);
        CORRADE_COMPARE(ui.currentPressedNode(), left);

        PointerEvent eventRelease{12345_nsec, Pointer::MouseLeft};
        layer.acceptRelease = true;
        CORRADE_VERIFY(ui.pointerReleaseEvent({350.0f, 500.0f}, eventRelease));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, leftData2, {10.0f, 10.0f}},
            {Press|Captured, leftData1, {10.0f, 10.0f}},
            {Release|Captured, leftData2, {15.0f, 5.0f}},
            {Release|Captured, leftData1, {15.0f, 5.0f}},
            {TapOrClick|Captured, leftData2, {15.0f, 5.0f}},
            {TapOrClick|Captured, leftData1, {15.0f, 5.0f}},
        })), TestSuite::Compare::Container);

    /* Press that disables capture and release on the same node */
    } {
        layer.eventCalls = {};

        PointerEvent eventPress{54321_nsec, Pointer::MouseLeft};
        layer.acceptPress = true;
        layer.capturePress = false;
        CORRADE_VERIFY(ui.pointerPressEvent({300.0f, 1000.0f}, eventPress));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentPressedNode(), left);

        PointerEvent eventRelease{12345_nsec, Pointer::MouseLeft};
        layer.acceptRelease = true;
        CORRADE_VERIFY(ui.pointerReleaseEvent({350.0f, 500.0f}, eventRelease));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, leftData2, {10.0f, 10.0f}},
            {Press, leftData1, {10.0f, 10.0f}},
            {Release, leftData2, {15.0f, 5.0f}},
            {Release, leftData1, {15.0f, 5.0f}},
            {TapOrClick, leftData2, {15.0f, 5.0f}},
            {TapOrClick, leftData1, {15.0f, 5.0f}},
        })), TestSuite::Compare::Container);

    /* Press and release on different node */
    } {
        layer.eventCalls = {};

        PointerEvent eventPress{54321_nsec, Pointer::MouseLeft};
        layer.acceptPress = true;
        layer.capturePress = {};
        CORRADE_VERIFY(ui.pointerPressEvent({300.0f, 1000.0f}, eventPress));
        CORRADE_COMPARE(ui.currentCapturedNode(), left);
        CORRADE_COMPARE(ui.currentPressedNode(), left);

        PointerEvent eventRelease{12345_nsec, Pointer::MouseLeft};
        layer.acceptRelease = true;
        CORRADE_VERIFY(ui.pointerReleaseEvent({500.0f, 500.0f}, eventRelease));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, leftData2, {10.0f, 10.0f}},
            {Press|Captured, leftData1, {10.0f, 10.0f}},
            {Release|Captured, leftData2, {30.0f, 5.0f}}, /* actually rightData */
            {Release|Captured, leftData1, {30.0f, 5.0f}}, /* actually rightData */
            /* no TapOrClick */
        })), TestSuite::Compare::Container);

    /* Press that disables capture and release on different node */
    } {
        layer.eventCalls = {};

        PointerEvent eventPress{54321_nsec, Pointer::MouseLeft};
        layer.acceptPress = true;
        layer.capturePress = false;
        CORRADE_VERIFY(ui.pointerPressEvent({300.0f, 1000.0f}, eventPress));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentPressedNode(), left);

        PointerEvent eventRelease{12345_nsec, Pointer::MouseLeft};
        layer.acceptRelease = true;
        CORRADE_VERIFY(ui.pointerReleaseEvent({500.0f, 500.0f}, eventRelease));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, leftData2, {10.0f, 10.0f}},
            {Press, leftData1, {10.0f, 10.0f}},
            {Release, rightData, {10.0f, 5.0f}},
            /* no TapOrClick */
        })), TestSuite::Compare::Container);

    /* Press that disables capture and release outside of everything */
    } {
        layer.eventCalls = {};

        PointerEvent eventPress{54321_nsec, Pointer::MouseLeft};
        layer.acceptPress = true;
        layer.capturePress = false;
        CORRADE_VERIFY(ui.pointerPressEvent({300.0f, 1000.0f}, eventPress));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentPressedNode(), left);

        PointerEvent eventRelease{12345_nsec, Pointer::MouseLeft};
        layer.acceptRelease = true;
        CORRADE_VERIFY(!ui.pointerReleaseEvent({100.0f, 500.0f}, eventRelease));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, leftData2, {10.0f, 10.0f}},
            {Press, leftData1, {10.0f, 10.0f}},
            /* no node to call Release or TapOrClick on */
        })), TestSuite::Compare::Container);

    /* Press and unaccepted release on the same node */
    } {
        layer.eventCalls = {};

        PointerEvent eventPress{54321_nsec, Pointer::MouseLeft};
        layer.acceptPress = true;
        layer.capturePress = {};
        CORRADE_VERIFY(ui.pointerPressEvent({500.0f, 1000.0f}, eventPress));
        CORRADE_COMPARE(ui.currentCapturedNode(), right);
        CORRADE_COMPARE(ui.currentPressedNode(), right);

        PointerEvent eventRelease{12345_nsec, Pointer::MouseLeft};
        layer.acceptRelease = false;
        CORRADE_VERIFY(!ui.pointerReleaseEvent({550.0f, 500.0f}, eventRelease));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, rightData, {10.0f, 10.0f}},
            {Release|Captured, rightData, {15.0f, 5.0f}},
            /* Release not accepted, no TapOrClick */
        })), TestSuite::Compare::Container);

    /* Press, move and release on the same node */
    } {
        layer.eventCalls = {};

        PointerEvent eventPress{54321_nsec, Pointer::MouseLeft};
        layer.acceptPress = true;
        layer.capturePress = {};
        CORRADE_VERIFY(ui.pointerPressEvent({500.0f, 1000.0f}, eventPress));
        CORRADE_COMPARE(ui.currentCapturedNode(), right);
        CORRADE_COMPARE(ui.currentPressedNode(), right);

        PointerMoveEvent eventMove{54321_nsec, {}, {}};
        layer.acceptMove = true;
        layer.captureMove = {};
        CORRADE_VERIFY(ui.pointerMoveEvent({450.0f, 1000.0f}, eventMove));
        CORRADE_COMPARE(ui.currentCapturedNode(), right);
        CORRADE_COMPARE(ui.currentPressedNode(), right);

        PointerEvent eventRelease{12345_nsec, Pointer::MouseLeft};
        layer.acceptRelease = true;
        CORRADE_VERIFY(ui.pointerReleaseEvent({550.0f, 500.0f}, eventRelease));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, rightData, {10.0f, 10.0f}},
            {Move|Captured, rightData, {5.0f, 10.0f}},
            {Release|Captured, rightData, {15.0f, 5.0f}},
            {TapOrClick|Captured, rightData, {15.0f, 5.0f}}
        })), TestSuite::Compare::Container);

    /* Press, uncaptured move and release on the same node */
    } {
        layer.eventCalls = {};

        PointerEvent eventPress{54321_nsec, Pointer::MouseLeft};
        layer.acceptPress = true;
        layer.capturePress = {};
        CORRADE_VERIFY(ui.pointerPressEvent({500.0f, 1000.0f}, eventPress));
        CORRADE_COMPARE(ui.currentCapturedNode(), right);
        CORRADE_COMPARE(ui.currentPressedNode(), right);

        PointerMoveEvent eventMove{54321_nsec, {}, {}};
        layer.acceptMove = true;
        layer.captureMove = false;
        CORRADE_VERIFY(ui.pointerMoveEvent({450.0f, 1000.0f}, eventMove));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentPressedNode(), right);

        PointerEvent eventRelease{12345_nsec, Pointer::MouseLeft};
        layer.acceptRelease = true;
        CORRADE_VERIFY(ui.pointerReleaseEvent({550.0f, 500.0f}, eventRelease));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, rightData, {10.0f, 10.0f}},
            {Move|Captured, rightData, {5.0f, 10.0f}},
            {Release, rightData, {15.0f, 5.0f}},
            {TapOrClick, rightData, {15.0f, 5.0f}}
        })), TestSuite::Compare::Container);

    /* Press, move outside, and release on the same node */
    } {
        layer.eventCalls = {};

        PointerEvent eventPress{54321_nsec, Pointer::MouseLeft};
        layer.acceptPress = true;
        layer.capturePress = {};
        CORRADE_VERIFY(ui.pointerPressEvent({500.0f, 1000.0f}, eventPress));
        CORRADE_COMPARE(ui.currentCapturedNode(), right);
        CORRADE_COMPARE(ui.currentPressedNode(), right);

        PointerMoveEvent eventMove{54321_nsec, {}, {}};
        layer.acceptMove = true;
        layer.captureMove = {};
        CORRADE_VERIFY(ui.pointerMoveEvent({300.0f, 1000.0f}, eventMove));
        CORRADE_COMPARE(ui.currentCapturedNode(), right);
        /* As the event is captured, the pressed node stays */
        CORRADE_COMPARE(ui.currentPressedNode(), right);

        PointerEvent eventRelease{12345_nsec, Pointer::MouseLeft};
        layer.acceptRelease = true;
        CORRADE_VERIFY(ui.pointerReleaseEvent({550.0f, 500.0f}, eventRelease));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, rightData, {10.0f, 10.0f}},
            {Move|Captured, rightData, {-10.0f, 10.0f}}, /* actually leftDataN */
            {Release|Captured, rightData, {15.0f, 5.0f}},
            {TapOrClick|Captured, rightData, {15.0f, 5.0f}}
        })), TestSuite::Compare::Container);

    /* Press, uncaptured move outside, and release on the same node */
    } {
        layer.eventCalls = {};

        PointerEvent eventPress{54321_nsec, Pointer::MouseLeft};
        layer.acceptPress = true;
        layer.capturePress = {};
        CORRADE_VERIFY(ui.pointerPressEvent({500.0f, 1000.0f}, eventPress));
        CORRADE_COMPARE(ui.currentCapturedNode(), right);
        CORRADE_COMPARE(ui.currentPressedNode(), right);

        PointerMoveEvent eventMove{54321_nsec, {}, {}};
        layer.acceptMove = true;
        layer.captureMove = false;
        CORRADE_VERIFY(ui.pointerMoveEvent({300.0f, 1000.0f}, eventMove));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);

        PointerEvent eventRelease{12345_nsec, Pointer::MouseLeft};
        layer.acceptRelease = true;
        CORRADE_VERIFY(ui.pointerReleaseEvent({550.0f, 500.0f}, eventRelease));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, rightData, {10.0f, 10.0f}},
            {Move|Captured, rightData, {-10.0f, 10.0f}}, /* actually leftDataN */
            {Release, rightData, {15.0f, 5.0f}},
            /* no TapOrClick */
        })), TestSuite::Compare::Container);

    /* Press, unaccepted move and release on the same node */
    } {
        layer.eventCalls = {};

        PointerEvent eventPress{54321_nsec, Pointer::MouseLeft};
        layer.acceptPress = true;
        layer.capturePress = {};
        CORRADE_VERIFY(ui.pointerPressEvent({500.0f, 1000.0f}, eventPress));
        CORRADE_COMPARE(ui.currentCapturedNode(), right);
        CORRADE_COMPARE(ui.currentPressedNode(), right);

        PointerMoveEvent eventMove{54321_nsec, {}, {}};
        layer.acceptMove = false;
        layer.captureMove = {};
        CORRADE_VERIFY(!ui.pointerMoveEvent({450.0f, 1000.0f}, eventMove));
        CORRADE_COMPARE(ui.currentCapturedNode(), right);
        /* Not accepting the move means the event was not called on an active
           area, but since the event is captured the pressed node stays */
        CORRADE_COMPARE(ui.currentPressedNode(), right);

        PointerEvent eventRelease{12345_nsec, Pointer::MouseLeft};
        layer.acceptRelease = true;
        CORRADE_VERIFY(ui.pointerReleaseEvent({550.0f, 500.0f}, eventRelease));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, rightData, {10.0f, 10.0f}},
            {Move|Captured, rightData, {5.0f, 10.0f}}, /* not accepted */
            {Release|Captured, rightData, {15.0f, 5.0f}},
            {TapOrClick|Captured, rightData, {15.0f, 5.0f}}
        })), TestSuite::Compare::Container);

    /* Press, uncaptured unaccepted move and release on the same node */
    } {
        layer.eventCalls = {};

        PointerEvent eventPress{54321_nsec, Pointer::MouseLeft};
        layer.acceptPress = true;
        layer.capturePress = {};
        CORRADE_VERIFY(ui.pointerPressEvent({500.0f, 1000.0f}, eventPress));
        CORRADE_COMPARE(ui.currentCapturedNode(), right);
        CORRADE_COMPARE(ui.currentPressedNode(), right);

        PointerMoveEvent eventMove{54321_nsec, {}, {}};
        layer.acceptMove = false;
        layer.captureMove = false;
        CORRADE_VERIFY(!ui.pointerMoveEvent({450.0f, 1000.0f}, eventMove));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        /* Even though the move happened on the same node, as it wasn't
           accepted it was treated as being outside of the active area and
           the event not being captured reset the pressed node */
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);

        PointerEvent eventRelease{12345_nsec, Pointer::MouseLeft};
        layer.acceptRelease = true;
        CORRADE_VERIFY(ui.pointerReleaseEvent({550.0f, 500.0f}, eventRelease));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
            {Press|Captured, rightData, {10.0f, 10.0f}},
            {Move|Captured, rightData, {5.0f, 10.0f}},
            {Release, rightData, {15.0f, 5.0f}},
            /* no TapOrClick */
        })), TestSuite::Compare::Container);
    }
}

void AbstractUserInterfaceTest::eventTapOrClickNodeBecomesHiddenDisabledNoEvents() {
    auto&& data = EventPointerNodeBecomesHiddenDisabledNoEventsData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Event scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};

    enum Event {
        Press = 0,
        Release = 1,
        TapOrClick = 2,
        VisibilityLost = 3,
        Update = 4
    };
    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doPointerPressEvent(UnsignedInt dataId, PointerEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Press, dataHandle(handle(), dataId, 1), event.position());
            event.setAccepted();
        }
        void doPointerReleaseEvent(UnsignedInt dataId, PointerEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Release, dataHandle(handle(), dataId, 1), event.position());
            event.setAccepted();
        }
        void doPointerTapOrClickEvent(UnsignedInt dataId, PointerEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, TapOrClick, dataHandle(handle(), dataId, 1), event.position());
            event.setAccepted();
        }
        void doVisibilityLostEvent(UnsignedInt dataId, VisibilityLostEvent& event) override {
            /* These can only be set if a focused node is no longer focusable */
            CORRADE_VERIFY(!event.isHovering());
            CORRADE_VERIFY(!event.isPressed());
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, VisibilityLost, dataHandle(handle(), dataId, 1), Vector2{});
        }
        void doUpdate(LayerStates, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, Containers::BitArrayView, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&) override {
            arrayAppend(eventCalls, InPlaceInit, Update, DataHandle::Null, Vector2{});
        }

        Containers::Array<Containers::Triple<Int, DataHandle, Vector2>> eventCalls;
    };

    /* Nested node in order to verify that the hidden/disabled/... flag gets
       propagated through the hierarchy */
    NodeHandle node = ui.createNode({20.0f, 0.0f}, {20.0f, 20.0f});
    NodeHandle nested = ui.createNode(node, {}, {20.0f, 20.0f});

    /* Update explicitly before adding the layer as NeedsDataAttachmentUpdate
       is a subset of this, and having just that one set may uncover accidental
       omissions in internal state updates compared to updating just once after
       creating both nodes and data */
    if(data.update) {
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<Layer>(layer));
    DataHandle nestedData = ui.layer<Layer>(layer).create(nested);

    if(data.update) {
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate);
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    PointerEvent eventPress{{}, Pointer::MouseLeft};
    CORRADE_VERIFY(ui.pointerPressEvent({30.0f, 10.0f}, eventPress));
    CORRADE_COMPARE(ui.currentPressedNode(), nested);
    CORRADE_COMPARE(ui.currentCapturedNode(), nested);

    if(data.flags)
        ui.addNodeFlags(node, data.flags);
    else if(data.clearOrder)
        ui.clearNodeOrder(node);
    else CORRADE_INTERNAL_ASSERT_UNREACHABLE();
    /* The current pressed node stays after setting the flags, is only updated
       after update() -- there it also handles if any parent gets the flag as
       well */
    CORRADE_COMPARE(ui.currentPressedNode(), nested);
    CORRADE_COMPARE(ui.currentCapturedNode(), nested);
    CORRADE_COMPARE(ui.state(), data.expectedState);

    if(data.update) {
        ui.update();

        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    PointerEvent eventRelease{{}, Pointer::MouseLeft};
    /* There's no node to execute the release on */
    CORRADE_VERIFY(!ui.pointerReleaseEvent({50.0f, 10.0f}, eventRelease));
    CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
    CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);

    CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
        {Update, {}, {}},
        {Press, nestedData, {10.0f, 10.0f}},
        /* The node used to be both pressed and captured, but only one
           visibility lost event is submitted for both. It's emitted before a
           doUpdate() so the changes can be directly reflected. */
        {VisibilityLost, nestedData, {}},
        {Update, {}, {}},
        /* There's no node to execute the Release on, and thus neither a
           TapOrClick is emitted */
    })), TestSuite::Compare::Container);

    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
}

void AbstractUserInterfaceTest::eventTapOrClickNodeRemoved() {
    auto&& data = EventNodeRemovedData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Event scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};

    enum Event {
        Press = 0,
        Release = 1,
        TapOrClick = 2
    };
    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doPointerPressEvent(UnsignedInt dataId, PointerEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Press, dataHandle(handle(), dataId, 1), event.position());
            event.setAccepted();
        }
        void doPointerReleaseEvent(UnsignedInt dataId, PointerEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Release, dataHandle(handle(), dataId, 1), event.position());
            event.setAccepted();
        }
        void doPointerTapOrClickEvent(UnsignedInt dataId, PointerEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, TapOrClick, dataHandle(handle(), dataId, 1), event.position());
            event.setAccepted();
        }
        void doVisibilityLostEvent(UnsignedInt, VisibilityLostEvent&) override {
            CORRADE_FAIL("This function shouldn't be called.");
        }

        Containers::Array<Containers::Triple<Int, DataHandle, Vector2>> eventCalls;
    };

    /* Nested node in order to verify that the removal gets propagated through
       the hierarchy */
    NodeHandle node = ui.createNode({20.0f, 0.0f}, {20.0f, 20.0f});
    NodeHandle nested = ui.createNode(node, {}, {20.0f, 20.0f});

    /* Update explicitly before adding the layer as NeedsDataAttachmentUpdate
       is a subset of this, and having just that one set may uncover accidental
       omissions in internal state updates compared to updating just once after
       creating both nodes and data */
    if(data.update) {
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<Layer>(layer));
    DataHandle nestedData = ui.layer<Layer>(layer).create(nested);

    if(data.update) {
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate);
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    PointerEvent eventPress{{}, Pointer::MouseLeft};
    CORRADE_VERIFY(ui.pointerPressEvent({30.0f, 10.0f}, eventPress));
    CORRADE_COMPARE(ui.currentPressedNode(), nested);

    ui.removeNode(data.removeParent ? node : nested);
    /* The current pressed node stays after removal, is only updated after
       update() -- there it also handles if any parent is removed */
    CORRADE_COMPARE(ui.currentPressedNode(), nested);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeClean);

    /* Add a new node in a different place, to verify the generation is
       correctly checked as well */
    if(!data.removeParent) {
        NodeHandle nestedReplacement = ui.createNode(node, {}, {40.0f, 20.0f});
        CORRADE_COMPARE(nodeHandleId(nestedReplacement), nodeHandleId(nested));
    }

    if(data.update) {
        ui.update();

        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    PointerEvent eventRelease{{}, Pointer::MouseLeft};
    /* There's no node to execute the release on */
    CORRADE_VERIFY(!ui.pointerReleaseEvent({50.0f, 10.0f}, eventRelease));
    CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);

    CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
        {Press, nestedData, {10.0f, 10.0f}},
        /* There's no node to execute the Release on, and thus neither a
           TapOrClick is emitted */
    })), TestSuite::Compare::Container);

    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
}

void AbstractUserInterfaceTest::eventTapOrClickAllDataRemoved() {
    auto&& data = CleanUpdateData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Event scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};

    enum Event {
        Press = 0,
        Release = 1,
        TapOrClick = 2
    };
    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;
        using AbstractLayer::remove;

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doPointerPressEvent(UnsignedInt dataId, PointerEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Press, dataHandle(handle(), dataId, 1), event.position());
            event.setAccepted();
        }
        void doPointerReleaseEvent(UnsignedInt dataId, PointerEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Release, dataHandle(handle(), dataId, 1), event.position());
            event.setAccepted();
        }
        void doPointerTapOrClickEvent(UnsignedInt dataId, PointerEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, TapOrClick, dataHandle(handle(), dataId, 1), event.position());
            event.setAccepted();
        }
        void doVisibilityLostEvent(UnsignedInt, VisibilityLostEvent&) override {
            CORRADE_FAIL("This function shouldn't be called.");
        }

        Containers::Array<Containers::Triple<Int, DataHandle, Vector2>> eventCalls;
    };

    NodeHandle node = ui.createNode({20.0f, 0.0f}, {20.0f, 20.0f});

    /* Update explicitly before adding the layer as NeedsDataAttachmentUpdate
       is a subset of this, and having just that one set may uncover accidental
       omissions in internal state updates compared to updating just once after
       creating both nodes and data */
    if(data.update) {
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<Layer>(layer));
    DataHandle nodeData = ui.layer<Layer>(layer).create(node);

    if(data.update) {
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate);
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    PointerEvent eventPress{{}, Pointer::MouseLeft};
    CORRADE_VERIFY(ui.pointerPressEvent({30.0f, 10.0f}, eventPress));
    CORRADE_COMPARE(ui.currentPressedNode(), node);

    ui.layer<Layer>(layer).remove(nodeData);
    /* Similar to hover behavior, the node stays pressed until an actual event
       discovers there's no data anymore. This is consistent for example with a
       case where the data would change the area where they're active for
       events -- also something that the clean() / update() can't discover on
       its own, only actually firing the event can. */
    CORRADE_COMPARE(ui.currentPressedNode(), node);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate|UserInterfaceState::NeedsDataClean);

    if(data.clean) {
        ui.clean();

        /* Same as above, the node stays pressed */
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate);
    }

    if(data.update) {
        ui.update();

        /* Same as above, the node stays pressed */
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    PointerEvent eventRelease{{}, Pointer::MouseLeft};
    /* There's no data to execute the release on */
    CORRADE_VERIFY(!ui.pointerReleaseEvent({50.0f, 10.0f}, eventRelease));
    CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);

    CORRADE_COMPARE_AS(ui.layer<Layer>(layer).eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Vector2>>({
        {Press, nodeData, {10.0f, 10.0f}},
        /* There's no data to execute the Release on, and thus neither a
           TapOrClick is emitted */
    })), TestSuite::Compare::Container);

    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
}

void AbstractUserInterfaceTest::eventFocus() {
    /* Event scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};

    enum Event {
        Hovering = 1,
        Pressed = 2,
        Focused = 4,
        /* All below have to be multiplies of 8 to not clash with the above */
        Press = 8,
        Release = 16,
        Move = 24,
        Enter = 32,
        Leave = 40,
        Focus = 48,
        Blur = 56,
        KeyPress = 64,
        KeyRelease = 72
    };

    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doPointerPressEvent(UnsignedInt dataId, PointerEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit,
                (event.isHovering() ? Hovering : 0)|
                (event.isFocused() ? Focused : 0)|Press,
                dataHandle(handle(), dataId, 1));
            event.setAccepted();
        }
        void doPointerReleaseEvent(UnsignedInt dataId, PointerEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit,
                (event.isHovering() ? Hovering : 0)|
                (event.isFocused() ? Focused : 0)|Release,
                dataHandle(handle(), dataId, 1));
            event.setAccepted();
        }
        void doPointerMoveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit,
                (event.isHovering() ? Hovering : 0)|
                (event.isFocused() ? Focused : 0)|Move,
                dataHandle(handle(), dataId, 1));
            event.setAccepted();
        }
        void doPointerEnterEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit,
                (event.isHovering() ? Hovering : 0)|
                (event.isFocused() ? Focused : 0)|Enter,
                dataHandle(handle(), dataId, 1));
            event.setAccepted();
        }
        void doPointerLeaveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit,
                (event.isHovering() ? Hovering : 0)|
                (event.isFocused() ? Focused : 0)|Leave,
                dataHandle(handle(), dataId, 1));
            event.setAccepted();
        }
        void doFocusEvent(UnsignedInt dataId, FocusEvent& event) override {
            /* The time should be propagated even if the focus is synthesized
               from a press */
            CORRADE_COMPARE(event.time(), 12345_nsec);
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit,
                (event.isHovering() ? Hovering : 0)|
                (event.isPressed() ? Pressed : 0)|Focus,
                dataHandle(handle(), dataId, 1));
            event.setAccepted();
        }
        void doBlurEvent(UnsignedInt dataId, FocusEvent& event) override {
            /* The time should be propagated even if the blur is synthesized
               from a press */
            CORRADE_COMPARE(event.time(), 12345_nsec);
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit,
                (event.isHovering() ? Hovering : 0)|
                (event.isPressed() ? Pressed : 0)|Blur,
                dataHandle(handle(), dataId, 1));
        }
        void doKeyPressEvent(UnsignedInt dataId, KeyEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit,
                (event.isHovering() ? Hovering : 0)|
                (event.isFocused() ? Focused : 0)|KeyPress,
                dataHandle(handle(), dataId, 1));
            event.setAccepted();
        }
        void doKeyReleaseEvent(UnsignedInt dataId, KeyEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit,
                (event.isHovering() ? Hovering : 0)|
                (event.isFocused() ? Focused : 0)|KeyRelease,
                dataHandle(handle(), dataId, 1));
            event.setAccepted();
        }

        Containers::Array<Containers::Pair<Int, DataHandle>> eventCalls;
    };

    NodeHandle nodeFocusable1 = ui.createNode({30.0f, 20.0f}, {20.0f, 20.0f}, NodeFlag::Focusable);
    NodeHandle nodeFocusable2 = ui.createNode({50.0f, 20.0f}, {20.0f, 20.0f}, NodeFlag::Focusable);

    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer()));
    DataHandle dataFocusable11 = layer.create(nodeFocusable1);
    DataHandle dataFocusable12 = layer.create(nodeFocusable1);
    DataHandle dataFocusable2 = layer.create(nodeFocusable2);

    /* Calling with a null node does nothing */
    {
        FocusEvent focus{12345_nsec};
        CORRADE_VERIFY(!ui.focusEvent(NodeHandle::Null, focus));
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Pair<Int, DataHandle>>({
        })), TestSuite::Compare::Container);

    /* Calling with a focusable node makes it focused, calling it again focuses
       again to give the data a chance to perform focus animations again */
    } {
        layer.eventCalls = {};

        FocusEvent focus1{12345_nsec};
        CORRADE_VERIFY(ui.focusEvent(nodeFocusable2, focus1));
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeFocusable2);

        FocusEvent focus2{12345_nsec};
        CORRADE_VERIFY(ui.focusEvent(nodeFocusable2, focus2));
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeFocusable2);

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Pair<Int, DataHandle>>({
            {Focus, dataFocusable2},
            {Focus, dataFocusable2},
        })), TestSuite::Compare::Container);

    /* Calling with a different focusable node emits a focus event first (to
       know if it gets accepted), on all data, and then a blur event on the
       original node */
    } {
        layer.eventCalls = {};

        FocusEvent focus{12345_nsec};
        CORRADE_VERIFY(ui.focusEvent(nodeFocusable1, focus));
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeFocusable1);
        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Pair<Int, DataHandle>>({
            {Focus, dataFocusable12},
            {Focus, dataFocusable11},
            {Blur, dataFocusable2},
        })), TestSuite::Compare::Container);

    /* Calling with a null node emits just a blur event, returns false */
    } {
        layer.eventCalls = {};

        FocusEvent focus{12345_nsec};
        CORRADE_VERIFY(!ui.focusEvent(NodeHandle::Null, focus));
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Pair<Int, DataHandle>>({
            {Blur, dataFocusable12},
            {Blur, dataFocusable11}
        })), TestSuite::Compare::Container);

    /* If the node is hovered, it gets shown in the focus / blur events */
    } {
        layer.eventCalls = {};

        PointerMoveEvent move{{}, {}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({40.0f, 30.0f}, move));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), nodeFocusable1);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);

        FocusEvent focus1{12345_nsec};
        CORRADE_VERIFY(ui.focusEvent(nodeFocusable1, focus1));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), nodeFocusable1);
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeFocusable1);

        FocusEvent focus2{12345_nsec};
        CORRADE_VERIFY(ui.focusEvent(nodeFocusable2, focus2));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), nodeFocusable1);
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeFocusable2);

        FocusEvent focus3{12345_nsec};
        CORRADE_VERIFY(!ui.focusEvent(NodeHandle::Null, focus3));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), nodeFocusable1);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Pair<Int, DataHandle>>({
            {Move, dataFocusable12},
            {Move, dataFocusable11},
            {Enter|Hovering, dataFocusable12},
            {Enter|Hovering, dataFocusable11},
            {Focus|Hovering, dataFocusable12},
            {Focus|Hovering, dataFocusable11},
            {Focus, dataFocusable2},
            {Blur|Hovering, dataFocusable12},
            {Blur|Hovering, dataFocusable11},
            {Blur, dataFocusable2},
        })), TestSuite::Compare::Container);

    /* If the node is pressed & hovered, it gets shown in the focus / blur
       events */
    } {
        layer.eventCalls = {};

        /* A press event alone causes a focus event to happen, so we
           temporarily make the node non-focusable for it. Focus event from a
           press event is tested in eventFocusBlurByPointerPress() below. */
        ui.clearNodeFlags(nodeFocusable1, NodeFlag::Focusable);

        PointerEvent press{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({40.0f, 30.0f}, press));
        CORRADE_COMPARE(ui.currentPressedNode(), nodeFocusable1);
        CORRADE_COMPARE(ui.currentHoveredNode(), nodeFocusable1);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);

        ui.addNodeFlags(nodeFocusable1, NodeFlag::Focusable);

        FocusEvent focus1{12345_nsec};
        CORRADE_VERIFY(ui.focusEvent(nodeFocusable1, focus1));
        CORRADE_COMPARE(ui.currentPressedNode(), nodeFocusable1);
        CORRADE_COMPARE(ui.currentHoveredNode(), nodeFocusable1);
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeFocusable1);

        FocusEvent focus2{12345_nsec};
        CORRADE_VERIFY(ui.focusEvent(nodeFocusable2, focus2));
        CORRADE_COMPARE(ui.currentPressedNode(), nodeFocusable1);
        CORRADE_COMPARE(ui.currentHoveredNode(), nodeFocusable1);
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeFocusable2);

        FocusEvent focus3{12345_nsec};
        CORRADE_VERIFY(!ui.focusEvent(NodeHandle::Null, focus3));
        CORRADE_COMPARE(ui.currentPressedNode(), nodeFocusable1);
        CORRADE_COMPARE(ui.currentHoveredNode(), nodeFocusable1);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Pair<Int, DataHandle>>({
            {Press|Hovering, dataFocusable12},
            {Press|Hovering, dataFocusable11},
            {Focus|Hovering|Pressed, dataFocusable12},
            {Focus|Hovering|Pressed, dataFocusable11},
            {Focus, dataFocusable2},
            {Blur|Hovering|Pressed, dataFocusable12},
            {Blur|Hovering|Pressed, dataFocusable11},
            {Blur, dataFocusable2},
        })), TestSuite::Compare::Container);

    /* If the node is just pressed, it gets shown in the focus / blur events */
    } {
        /* Just to reset the hover */
        PointerMoveEvent move{{}, {}, {}};
        /* Returns true because it gets called on a node captured above */
        CORRADE_VERIFY(ui.pointerMoveEvent({1000.0f, 1000.0f}, move));
        CORRADE_COMPARE(ui.currentPressedNode(), nodeFocusable1);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);

        layer.eventCalls = {};

        FocusEvent focus1{12345_nsec};
        CORRADE_VERIFY(ui.focusEvent(nodeFocusable1, focus1));
        CORRADE_COMPARE(ui.currentPressedNode(), nodeFocusable1);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeFocusable1);

        FocusEvent focus2{12345_nsec};
        CORRADE_VERIFY(ui.focusEvent(nodeFocusable2, focus2));
        CORRADE_COMPARE(ui.currentPressedNode(), nodeFocusable1);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeFocusable2);

        FocusEvent focus3{12345_nsec};
        CORRADE_VERIFY(!ui.focusEvent(NodeHandle::Null, focus3));
        CORRADE_COMPARE(ui.currentPressedNode(), nodeFocusable1);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Pair<Int, DataHandle>>({
            {Focus|Pressed, dataFocusable12},
            {Focus|Pressed, dataFocusable11},
            {Focus, dataFocusable2},
            {Blur|Pressed, dataFocusable12},
            {Blur|Pressed, dataFocusable11},
            {Blur, dataFocusable2},
        })), TestSuite::Compare::Container);

    /* If called on a focused node, it gets shown in all pointer and key
       events. If some other node is focused, it doesn't get. */
    } {
        layer.eventCalls = {};

        /* Same node focused */
        FocusEvent focus1{12345_nsec};
        CORRADE_VERIFY(ui.focusEvent(nodeFocusable2, focus1));
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeFocusable2);

        PointerEvent press1{12345_nsec, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({55.0f, 35.0f}, press1));
        CORRADE_COMPARE(ui.currentPressedNode(), nodeFocusable2);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeFocusable2);

        PointerEvent release1{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({55.0f, 35.0f}, release1));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeFocusable2);

        PointerMoveEvent moveEnter1{{}, {}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({55.0f, 35.0f}, moveEnter1));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), nodeFocusable2);
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeFocusable2);

        PointerMoveEvent moveLeave1{{}, {}, {}};
        CORRADE_VERIFY(!ui.pointerMoveEvent({1000.0f, 1000.0f}, moveLeave1));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeFocusable2);

        KeyEvent keyPress1{{}, Key::C, {}};
        CORRADE_VERIFY(ui.keyPressEvent(keyPress1));

        KeyEvent keyRelease1{{}, Key::C, {}};
        CORRADE_VERIFY(ui.keyReleaseEvent(keyRelease1));

        /* Different node focused */
        FocusEvent focus2{12345_nsec};
        CORRADE_VERIFY(ui.focusEvent(nodeFocusable1, focus2));
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeFocusable1);

        PointerEvent press2{12345_nsec, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({55.0f, 35.0f}, press2));
        CORRADE_COMPARE(ui.currentPressedNode(), nodeFocusable2);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeFocusable2);

        /* Press changed the focused node, add it back */
        /** @todo some NodeFlag::NotChangingFocus for this */
        FocusEvent focus3{12345_nsec};
        CORRADE_VERIFY(ui.focusEvent(nodeFocusable1, focus3));
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeFocusable1);

        PointerEvent release2{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({55.0f, 35.0f}, release2));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeFocusable1);

        PointerMoveEvent moveEnter2{{}, {}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({55.0f, 35.0f}, moveEnter2));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), nodeFocusable2);
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeFocusable1);

        PointerMoveEvent moveLeave2{{}, {}, {}};
        CORRADE_VERIFY(!ui.pointerMoveEvent({1000.0f, 1000.0f}, moveLeave2));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeFocusable1);

        /* Key events always go to the focused node so they'll have Focused
           set, no reason to call them here as they'd be no different from
           above */

        /* No node focused */
        FocusEvent focus4{12345_nsec};
        CORRADE_VERIFY(!ui.focusEvent(NodeHandle::Null, focus4));
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);

        /* Press would change the focused node, temporarily remove the
           Focusable from the node to avoid that */
        /** @todo some NodeFlag::FocusableButNotByPointer for this */
        ui.clearNodeFlags(nodeFocusable2, NodeFlag::Focusable);

        PointerEvent press3{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({55.0f, 35.0f}, press3));
        CORRADE_COMPARE(ui.currentPressedNode(), nodeFocusable2);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);

        ui.addNodeFlags(nodeFocusable2, NodeFlag::Focusable);

        PointerEvent release3{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({55.0f, 35.0f}, release3));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);

        PointerMoveEvent moveEnter3{{}, {}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({55.0f, 35.0f}, moveEnter3));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), nodeFocusable2);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);

        /* With no focused node, the key events go to the hovered node, so they
           have to be before the leave event */
        KeyEvent keyPress3{{}, Key::C, {}};
        CORRADE_VERIFY(ui.keyPressEvent(keyPress3));

        KeyEvent keyRelease3{{}, Key::C, {}};
        CORRADE_VERIFY(ui.keyReleaseEvent(keyRelease3));

        PointerMoveEvent moveLeave3{{}, {}, {}};
        CORRADE_VERIFY(!ui.pointerMoveEvent({1000.0f, 1000.0f}, moveLeave3));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Pair<Int, DataHandle>>({
            {Focus, dataFocusable2},
            {Press|Focused, dataFocusable2},
            /** @todo some FocusableButNotByPointer for this */
            {Focus|Pressed, dataFocusable2}, /* Implied by the press */
            {Release|Focused, dataFocusable2},
            {Move|Focused, dataFocusable2},
            {Enter|Focused|Hovering, dataFocusable2},
            /* Move outside of any node not sent anywhere, just Leave */
            {Leave|Focused, dataFocusable2},
            {KeyPress|Focused, dataFocusable2},
            {KeyRelease|Focused, dataFocusable2},

            /* Focusing a different node, none of these are Focused anymore */
            {Focus, dataFocusable12},
            {Focus, dataFocusable11},
            {Blur, dataFocusable2},
            {Press, dataFocusable2},
            /* The press focuses a different node so there's another focus
               event putting it back */
            /** @todo some NodeFlag::NotChangingFocus for this */
            {Blur, dataFocusable12},
            {Blur, dataFocusable11},
            {Focus|Pressed, dataFocusable2},
            {Focus, dataFocusable12},
            {Focus, dataFocusable11},
            {Blur|Pressed, dataFocusable2},
            {Release, dataFocusable2},
            {Move, dataFocusable2},
            {Enter|Hovering, dataFocusable2},
            /* Move outside of any node not sent anywhere, just Leave */
            {Leave, dataFocusable2},

            /* Focusing nothing at all, none of these are Focused either */
            {Blur, dataFocusable12},
            {Blur, dataFocusable11},
            {Press, dataFocusable2},
            {Release, dataFocusable2},
            {Move, dataFocusable2},
            {Enter|Hovering, dataFocusable2},
            {KeyPress|Hovering, dataFocusable2},
            {KeyRelease|Hovering, dataFocusable2},
            /* Move outside of any node not sent anywhere, just Leave */
            {Leave, dataFocusable2},
        })), TestSuite::Compare::Container);
    }
}

void AbstractUserInterfaceTest::eventFocusNotAccepted() {
    /* Event scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};

    enum Event {
        Accepted = 1,
        Focus = 2,
        Blur = 4
    };

    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doFocusEvent(UnsignedInt dataId, FocusEvent& event) override {
            if(accept)
                event.setAccepted();
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit,
                (event.isAccepted() ? Accepted : 0)|Focus,
                dataHandle(handle(), dataId, 1));
        }
        void doBlurEvent(UnsignedInt dataId, FocusEvent&) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Blur,
                dataHandle(handle(), dataId, 1));
        }

        Containers::Array<Containers::Pair<Int, DataHandle>> eventCalls;
        bool accept = true;
    };

    NodeHandle nodeFocusable1 = ui.createNode({30.0f, 20.0f}, {20.0f, 20.0f}, NodeFlag::Focusable);
    NodeHandle nodeFocusable2 = ui.createNode({50.0f, 20.0f}, {20.0f, 20.0f}, NodeFlag::Focusable);

    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer()));
    DataHandle dataFocusable1 = layer.create(nodeFocusable1);
    DataHandle dataFocusable2 = layer.create(nodeFocusable2);

    /* Unlike with e.g. eventPointerMoveNotAccepted(), here's no fallback to
       nodes below going on, thus the test is a lot simpler */

    /* Not accepting an event returns false and doesn't set the focused node */
    {
        FocusEvent event{{}};
        layer.accept = false;
        CORRADE_VERIFY(!ui.focusEvent(nodeFocusable1, event));
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Pair<Int, DataHandle>>({
            {Focus, dataFocusable1},
        })), TestSuite::Compare::Container);

    /* With a node already focused, not accepting an event makes it stay
       focused */
    } {
        layer.eventCalls = {};

        FocusEvent event1{{}};
        layer.accept = true;
        CORRADE_VERIFY(ui.focusEvent(nodeFocusable1, event1));
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeFocusable1);

        FocusEvent event2{{}};
        layer.accept = false;
        CORRADE_VERIFY(!ui.focusEvent(nodeFocusable2, event2));
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeFocusable1);

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Pair<Int, DataHandle>>({
            {Focus|Accepted, dataFocusable1},
            {Focus, dataFocusable2}, /* not accepted, so not followed by Blur */
        })), TestSuite::Compare::Container);

    /* Calling a non-accepted focus event on the same node then calls Blur and
       makes it null */
    } {
        /* Just to reset everything */
        FocusEvent event0{{}};
        layer.accept = true;
        CORRADE_VERIFY(!ui.focusEvent(NodeHandle::Null, event0));
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);

        layer.eventCalls = {};

        FocusEvent event1{{}};
        layer.accept = true;
        CORRADE_VERIFY(ui.focusEvent(nodeFocusable2, event1));
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeFocusable2);

        FocusEvent event2{{}};
        layer.accept = false;
        CORRADE_VERIFY(!ui.focusEvent(nodeFocusable2, event2));
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Pair<Int, DataHandle>>({
            {Focus|Accepted, dataFocusable2},
            {Focus, dataFocusable2},
            {Blur, dataFocusable2}
        })), TestSuite::Compare::Container);
    }
}

void AbstractUserInterfaceTest::eventFocusNodeHiddenDisabledNoEvents() {
    auto&& data = EventFocusNodeHiddenDisabledNoEventsData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Event scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};

    enum Event {
        Focus = 0,
        Blur = 1
    };

    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doFocusEvent(UnsignedInt dataId, FocusEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Focus,
                dataHandle(handle(), dataId, 1));
            event.setAccepted();
        }
        void doBlurEvent(UnsignedInt dataId, FocusEvent&) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Blur,
                dataHandle(handle(), dataId, 1));
        }
        void doVisibilityLostEvent(UnsignedInt, VisibilityLostEvent&) override {
            CORRADE_FAIL("This function shouldn't be called.");
        }

        Containers::Array<Containers::Pair<Int, DataHandle>> eventCalls;
    };

    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer()));

    /* Nested node in order to verify that the hidden/disabled/... flag gets
       propagated through the hierarchy */
    NodeHandle node = ui.createNode({20.0f, 0.0f}, {20.0f, 20.0f});
    NodeHandle nested = ui.createNode(node, {}, {20.0f, 20.0f}, NodeFlag::Focusable);
    DataHandle nestedData = layer.create(nested);

    NodeHandle previouslyFocused = NodeHandle::Null;
    DataHandle previouslyFocusedData = DataHandle::Null;
    if(data.previousFocused) {
        previouslyFocused = ui.createNode({40.0f, 0.0f}, {20.0f, 20.0f}, NodeFlag::Focusable);
        previouslyFocusedData = layer.create(previouslyFocused);

        FocusEvent event{{}};
        CORRADE_VERIFY(ui.focusEvent(previouslyFocused, event));
        CORRADE_COMPARE(ui.currentFocusedNode(), previouslyFocused);
    }

    if(data.flags)
        ui.addNodeFlags(node, data.flags);
    else if(data.clearOrder)
        ui.clearNodeOrder(node);
    else CORRADE_INTERNAL_ASSERT_UNREACHABLE();

    /* Optionally call update() after setting the node flags, however
       focusEvent() should do that itself */
    if(data.update) {
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    /* The flag should propagate from the parent and prevent the node from
       getting focused. The previously focused node, if any, should stay. */
    {
        FocusEvent event{{}};
        CORRADE_VERIFY(!ui.focusEvent(nested, event));
        CORRADE_COMPARE(ui.currentFocusedNode(), previouslyFocused);
    }

    /* The only event that should get called is for the previous node focus, if
       any */
    if(data.previousFocused)
        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Pair<Int, DataHandle>>({
            {Focus, previouslyFocusedData},
        })), TestSuite::Compare::Container);
    else
        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Pair<Int, DataHandle>>({
        })), TestSuite::Compare::Container);

    /* Removing the flag again makes it work */
    layer.eventCalls = {};
    if(data.flags)
        ui.clearNodeFlags(node, data.flags);
    else if(data.clearOrder)
        ui.setNodeOrder(node, NodeHandle::Null);
    else CORRADE_INTERNAL_ASSERT_UNREACHABLE();

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    {
        FocusEvent event{{}};
        CORRADE_VERIFY(ui.focusEvent(nested, event));
        CORRADE_COMPARE(ui.currentFocusedNode(), nested);
    }

    if(data.previousFocused)
        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Pair<Int, DataHandle>>({
            {Focus, nestedData},
            {Blur, previouslyFocusedData}
        })), TestSuite::Compare::Container);
    else
        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Pair<Int, DataHandle>>({
            {Focus, nestedData},
        })), TestSuite::Compare::Container);

    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
}

void AbstractUserInterfaceTest::eventFocusNodeBecomesHiddenDisabledNoEventsNotFocusable() {
    auto&& data = EventFocusNodeBecomesHiddenDisabledNoEventsNotFocusableData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Event scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};

    enum Event {
        Hovering = 1,
        Pressed = 2,
        Press = 4,
        Move = 8,
        Focus = 12,
        VisibilityLost = 16
    };

    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doPointerPressEvent(UnsignedInt dataId, PointerEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Press|(event.isHovering() ? Hovering : 0),
                dataHandle(handle(), dataId, 1));
            event.setAccepted();
        }
        void doPointerMoveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Move|(event.isHovering() ? Hovering : 0),
                dataHandle(handle(), dataId, 1));
            event.setAccepted();
        }
        void doFocusEvent(UnsignedInt dataId, FocusEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit,
                (event.isHovering() ? Hovering : 0)|
                (event.isPressed() ? Pressed : 0)|Focus,
                dataHandle(handle(), dataId, 1));
            event.setAccepted();
        }
        void doBlurEvent(UnsignedInt, FocusEvent&) override {
            CORRADE_FAIL("This function shouldn't be called.");
        }
        void doVisibilityLostEvent(UnsignedInt dataId, VisibilityLostEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit,
                (event.isHovering() ? Hovering : 0)|
                (event.isPressed() ? Pressed : 0)|VisibilityLost,
                dataHandle(handle(), dataId, 1));
        }

        Containers::Array<Containers::Pair<Int, DataHandle>> eventCalls;
    };

    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer()));

    /* Nested node in order to verify that the hidden/disabled/... flag gets
       propagated through the hierarchy */
    NodeHandle node = ui.createNode({20.0f, 0.0f}, {20.0f, 20.0f});
    NodeHandle nested = ui.createNode(node, {}, {20.0f, 20.0f}, NodeFlag::Focusable);
    DataHandle nestedData = layer.create(nested);

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    if(data.pressed) {
        /* A press event alone causes a focus event to happen which is
           undesirable here, so we temporarily make the node non-focusable for
           it */
        ui.clearNodeFlags(nested, NodeFlag::Focusable);

        PointerEvent press{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({30.0f, 15.0f}, press));
        CORRADE_COMPARE(ui.currentPressedNode(), nested);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);

        ui.addNodeFlags(nested, NodeFlag::Focusable);

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Pair<Int, DataHandle>>({
            {Press, nestedData},
        })), TestSuite::Compare::Container);
        layer.eventCalls = {};
    }

    if(data.hovered) {
        PointerMoveEvent move{{}, {}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({30.0f, 15.0f}, move));
        CORRADE_COMPARE(ui.currentPressedNode(), data.pressed ? nested : NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), nested);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Pair<Int, DataHandle>>({
            {Move, nestedData},
        })), TestSuite::Compare::Container);
        layer.eventCalls = {};
    }

    FocusEvent focus1{{}};
    CORRADE_VERIFY(ui.focusEvent(nested, focus1));
    CORRADE_COMPARE(ui.currentFocusedNode(), nested);

    if(data.addFlags || data.clearFlags) {
        ui.addNodeFlags(node, data.addFlags);
        ui.clearNodeFlags(nested, data.clearFlags);
    } else if(data.clearOrder) {
        ui.clearNodeOrder(node);
    } else CORRADE_INTERNAL_ASSERT_UNREACHABLE();
    /* The current focused node stays after setting the flags, is only updated
       after update() -- there it also handles if any parent gets the flag as
       well */
    CORRADE_COMPARE(ui.currentFocusedNode(), nested);
    CORRADE_COMPARE(ui.state(), data.expectedState);

    if(data.update) {
        ui.update();

        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    FocusEvent focus2{{}};
    /* The node isn't focusable anymore. If Focusable was removed we cannot
       even call the focusEvent(), have to call update() to remove the stale
       focused node. */
    if(data.clearFlags != NodeFlag::Focusable)
        CORRADE_VERIFY(!ui.focusEvent(nested, focus2));
    else
        ui.update();
    CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

    CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Pair<Int, DataHandle>>({
        {Focus|(data.pressed ? Pressed : 0)|(data.hovered ? Hovering : 0), nestedData},
        /* VisibilityLost gets called instead of Blur. It should be pressed or
           hovering only if the node actually receives events. It should also
           get called just once, even if the node is simultaenously pressed,
           hovered and focused. */
        {VisibilityLost|(data.expectPressedHoveringBlur ? (data.pressed ? Pressed : 0)|(data.hovered ? Hovering : 0) : 0), nestedData}
    })), TestSuite::Compare::Container);
}

void AbstractUserInterfaceTest::eventFocusNodeRemoved() {
    auto&& data = EventNodeRemovedData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Event scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};

    enum Event {
        Focus = 0,
        Blur = 1
    };

    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doFocusEvent(UnsignedInt dataId, FocusEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Focus,
                dataHandle(handle(), dataId, 1));
            event.setAccepted();
        }
        void doBlurEvent(UnsignedInt dataId, FocusEvent&) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Blur,
                dataHandle(handle(), dataId, 1));
        }

        Containers::Array<Containers::Pair<Int, DataHandle>> eventCalls;
    };

    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer()));

    /* Nested node in order to verify that the removal gets propagated through
       the hierarchy */
    NodeHandle node = ui.createNode({20.0f, 0.0f}, {20.0f, 20.0f});
    NodeHandle nested = ui.createNode(node, {}, {20.0f, 20.0f}, NodeFlag::Focusable);
    DataHandle nestedData = layer.create(nested);

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    FocusEvent event1{{}};
    CORRADE_VERIFY(ui.focusEvent(nested, event1));
    CORRADE_COMPARE(ui.currentFocusedNode(), nested);

    ui.removeNode(data.removeParent ? node : nested);
    /* The current focused node stays after removal, is only updated after
       update() -- there it also handles if any parent is removed */
    CORRADE_COMPARE(ui.currentFocusedNode(), nested);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeClean);

    /* Add a new node in a different place, to verify the generation is
       correctly checked as well */
    if(!data.removeParent) {
        NodeHandle nestedReplacement = ui.createNode(node, {}, {40.0f, 20.0f});
        CORRADE_COMPARE(nodeHandleId(nestedReplacement), nodeHandleId(nested));
    }

    /* After an update the current focused node is no more */
    ui.update();
    CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

    /* Unlike e.g. eventPointerMoveNodeRemoved(), there's no reason to try to
       call focusEvent() again to check if the removed node is reused because
       the node handle has to be specified explicitly */

    CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Pair<Int, DataHandle>>({
        {Focus, nestedData},
        /* There's no node to execute the Blur on */
    })), TestSuite::Compare::Container);
}

void AbstractUserInterfaceTest::eventFocusAllDataRemoved() {
    auto&& data = CleanUpdateData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Event scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};

    enum Event {
        Focus = 0,
        Blur = 1
    };

    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;
        using AbstractLayer::remove;

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doFocusEvent(UnsignedInt dataId, FocusEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Focus,
                dataHandle(handle(), dataId, 1));
            event.setAccepted();
        }
        void doBlurEvent(UnsignedInt dataId, FocusEvent&) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Blur,
                dataHandle(handle(), dataId, 1));
        }

        Containers::Array<Containers::Pair<Int, DataHandle>> eventCalls;
    };

    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer()));

    /* Nested node in order to verify that the removal gets propagated through
       the hierarchy */
    NodeHandle node = ui.createNode({20.0f, 0.0f}, {20.0f, 20.0f}, NodeFlag::Focusable);
    DataHandle nodeData = layer.create(node);

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    FocusEvent event1{{}};
    CORRADE_VERIFY(ui.focusEvent(node, event1));
    CORRADE_COMPARE(ui.currentFocusedNode(), node);

    layer.remove(nodeData);
    /* The node stays focused until an actual move event discovers there's no
       data anymore. This is consistent for example with a case where the data
       would change the area where they're active for events -- also something
       that the clean() / update() can't discover on its own, only actually
       firing the event can. */
    CORRADE_COMPARE(ui.currentFocusedNode(), node);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate|UserInterfaceState::NeedsDataClean);

    if(data.clean) {
        ui.clean();

        /* Same as above, the node stays focused */
        CORRADE_COMPARE(ui.currentFocusedNode(), node);
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate);
    }

    if(data.update) {
        ui.update();

        /* Same as above, the node stays focused */
        CORRADE_COMPARE(ui.currentFocusedNode(), node);
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    FocusEvent event2{{}};
    /* There's no data to execute the move on */
    CORRADE_VERIFY(!ui.focusEvent(node, event2));
    CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);

    CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Pair<Int, DataHandle>>({
        {Focus, nodeData},
        /* There's no data to execute the Focus on, nor a Blur */
    })), TestSuite::Compare::Container);

    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
}

void AbstractUserInterfaceTest::eventFocusInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui{{100, 100}};

    /* Doesn't have NodeFlag::Focusable, but has some other flags */
    NodeHandle nodeNotFocusable = ui.createNode({}, {}, NodeFlag::Clip);

    FocusEvent event{{}};

    std::ostringstream out;
    Error redirectError{&out};
    ui.focusEvent(nodeNotFocusable, event);
    ui.focusEvent(nodeHandle(0x12345, 0xabc), event);
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractUserInterface::focusEvent(): node not focusable\n"
        "Whee::AbstractUserInterface::focusEvent(): invalid handle Whee::NodeHandle(0x12345, 0xabc)\n");
}

void AbstractUserInterfaceTest::eventFocusBlurByPointerPress() {
    /* Event scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};

    enum Event {
        Hovering = 1,
        Pressed = 2,
        Press = 4,
        Release = 8,
        Move = 12,
        Focus = 16,
        Blur = 20
    };

    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doPointerPressEvent(UnsignedInt dataId, PointerEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Press|(event.isHovering() ? Hovering : 0),
                dataHandle(handle(), dataId, 1));
            event.setAccepted();
        }
        void doPointerReleaseEvent(UnsignedInt dataId, PointerEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Release|(event.isHovering() ? Hovering : 0),
                dataHandle(handle(), dataId, 1));
            event.setAccepted();
        }
        void doPointerMoveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Move|(event.isHovering() ? Hovering : 0),
                dataHandle(handle(), dataId, 1));
            event.setAccepted();
        }
        void doFocusEvent(UnsignedInt dataId, FocusEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit,
                (event.isHovering() ? Hovering : 0)|
                (event.isPressed() ? Pressed : 0)|Focus,
                dataHandle(handle(), dataId, 1));
            event.setAccepted();
        }
        void doBlurEvent(UnsignedInt dataId, FocusEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit,
                (event.isHovering() ? Hovering : 0)|
                (event.isPressed() ? Pressed : 0)|Blur,
                dataHandle(handle(), dataId, 1));
        }

        Containers::Array<Containers::Pair<Int, DataHandle>> eventCalls;
    };

    NodeHandle node = ui.createNode({10.0f, 20.0f}, {20.0f, 20.0f});
    NodeHandle nodeFocusable1 = ui.createNode({30.0f, 20.0f}, {20.0f, 20.0f}, NodeFlag::Focusable);
    NodeHandle nodeFocusable2 = ui.createNode({50.0f, 20.0f}, {20.0f, 20.0f}, NodeFlag::Focusable);

    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer()));
    DataHandle data = layer.create(node);
    DataHandle dataFocusable11 = layer.create(nodeFocusable1);
    DataHandle dataFocusable12 = layer.create(nodeFocusable1);
    DataHandle dataFocusable2 = layer.create(nodeFocusable2);

    /* Pressing on a regular node or with anything except MouseLeft, Finger or
       Pen does not focus the node */
    {
        PointerEvent press1{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({15.0f, 30.0f}, press1));
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);

        PointerEvent press2{{}, Pointer::MouseRight};
        CORRADE_VERIFY(ui.pointerPressEvent({55.0f, 30.0f}, press2));
        CORRADE_COMPARE(ui.currentPressedNode(), nodeFocusable2);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);

        PointerEvent press3{{}, Pointer::MouseMiddle};
        CORRADE_VERIFY(ui.pointerPressEvent({40.0f, 30.0f}, press3));
        CORRADE_COMPARE(ui.currentPressedNode(), nodeFocusable1);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);

        PointerEvent press4{{}, Pointer::Eraser};
        CORRADE_VERIFY(ui.pointerPressEvent({55.0f, 35.0f}, press4));
        CORRADE_COMPARE(ui.currentPressedNode(), nodeFocusable2);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Pair<Int, DataHandle>>({
            {Press, data},
            {Press, dataFocusable2},
            {Press, dataFocusable12},
            {Press, dataFocusable11},
            {Press, dataFocusable2},
        })), TestSuite::Compare::Container);

    /* Pressing with non-primary pointers doesn't affect the currently focused
       node in any way, nor calls blur events for it */
    } {
        /* Just to reset everything */
        /** @todo have a pointerCancelEvent() for this */
        PointerEvent eventReleaseReset{{}, Pointer::MouseLeft};
        ui.pointerReleaseEvent({10000.0f, 10000.0f}, eventReleaseReset);

        layer.eventCalls = {};

        FocusEvent focus{{}};
        CORRADE_VERIFY(ui.focusEvent(nodeFocusable2, focus));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeFocusable2);

        PointerEvent press1{{}, Pointer::MouseRight};
        CORRADE_VERIFY(ui.pointerPressEvent({35.0f, 30.0f}, press1));
        CORRADE_COMPARE(ui.currentPressedNode(), nodeFocusable1);
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeFocusable2);

        PointerEvent press3{{}, Pointer::MouseMiddle};
        CORRADE_VERIFY(ui.pointerPressEvent({55.0f, 30.0f}, press3));
        CORRADE_COMPARE(ui.currentPressedNode(), nodeFocusable2);
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeFocusable2);

        PointerEvent press4{{}, Pointer::Eraser};
        CORRADE_VERIFY(!ui.pointerPressEvent({1000.0f, 1000.0f}, press4));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeFocusable2);

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Pair<Int, DataHandle>>({
            {Focus, dataFocusable2},
            {Press, dataFocusable12},
            {Press, dataFocusable11},
            {Press, dataFocusable2},
        })), TestSuite::Compare::Container);

    /* Pressing on a focusable node focuses it, pressing again focuses again
       to give the data a chance to perform focus animations again. Releasing
       doesn't remove the focus; pressing outside removes the focus again. */
    } {
        /* Just to reset everything */
        FocusEvent blur{{}};
        ui.focusEvent(NodeHandle::Null, blur);

        layer.eventCalls = {};

        PointerEvent press1{{}, Pointer::Pen};
        CORRADE_VERIFY(ui.pointerPressEvent({60.0f, 35.0f}, press1));
        CORRADE_COMPARE(ui.currentPressedNode(), nodeFocusable2);
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeFocusable2);

        PointerEvent press2{{}, Pointer::Finger};
        CORRADE_VERIFY(ui.pointerPressEvent({60.0f, 30.0f}, press2));
        CORRADE_COMPARE(ui.currentPressedNode(), nodeFocusable2);
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeFocusable2);

        PointerEvent release{{}, Pointer::Pen};
        CORRADE_VERIFY(ui.pointerReleaseEvent({60.0f, 35.0f}, release));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeFocusable2);

        PointerEvent press3{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(!ui.pointerPressEvent({1000.0f, 1000.0f}, press3));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Pair<Int, DataHandle>>({
            {Press, dataFocusable2},
            {Focus|Pressed, dataFocusable2},
            {Press, dataFocusable2},
            {Focus|Pressed, dataFocusable2},
            {Release, dataFocusable2},
            {Blur, dataFocusable2},
        })), TestSuite::Compare::Container);

    /* Pressing on another focusable node blurs it and focuses the other,
       pressing on a non-focusable node blurs the other but doesn't focus any
       node anymore */
    } {
        layer.eventCalls = {};

        PointerEvent press1{{}, Pointer::Finger};
        CORRADE_VERIFY(ui.pointerPressEvent({55.0f, 35.0f}, press1));
        CORRADE_COMPARE(ui.currentPressedNode(), nodeFocusable2);
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeFocusable2);

        PointerEvent press2{{}, Pointer::Pen};
        CORRADE_VERIFY(ui.pointerPressEvent({35.0f, 25.0f}, press2));
        CORRADE_COMPARE(ui.currentPressedNode(), nodeFocusable1);
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeFocusable1);

        PointerEvent press3{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({25.0f, 25.0f}, press3));
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Pair<Int, DataHandle>>({
            {Press, dataFocusable2},
            {Focus|Pressed, dataFocusable2},
            /* Also tests that the event is propagated to all data */
            {Press, dataFocusable12},
            {Press, dataFocusable11},
            {Blur, dataFocusable2},
            {Focus|Pressed, dataFocusable12},
            {Focus|Pressed, dataFocusable11},
            {Press, data},
            {Blur, dataFocusable12},
            {Blur, dataFocusable11},
        })), TestSuite::Compare::Container);

    /* If the node is hovered, it gets shown in the focus / blur events */
    } {
        /* Just to reset everything */
        /** @todo have a pointerCancelEvent() for this */
        PointerEvent eventReleaseReset{{}, Pointer::MouseLeft};
        ui.pointerReleaseEvent({10000.0f, 10000.0f}, eventReleaseReset);

        layer.eventCalls = {};

        PointerMoveEvent move{{}, {}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({35.0f, 25.0f}, move));
        CORRADE_COMPARE(ui.currentHoveredNode(), nodeFocusable1);
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);

        PointerEvent press1{{}, Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({45.0f, 25.0f}, press1));
        CORRADE_COMPARE(ui.currentHoveredNode(), nodeFocusable1);
        CORRADE_COMPARE(ui.currentPressedNode(), nodeFocusable1);
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeFocusable1);

        PointerEvent press2{{}, Pointer::Pen};
        CORRADE_VERIFY(ui.pointerPressEvent({55.0f, 35.0f}, press2));
        CORRADE_COMPARE(ui.currentHoveredNode(), nodeFocusable1);
        CORRADE_COMPARE(ui.currentPressedNode(), nodeFocusable2);
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeFocusable2);

        PointerEvent press3{{}, Pointer::Finger};
        CORRADE_VERIFY(ui.pointerPressEvent({25.0f, 25.0f}, press3));
        CORRADE_COMPARE(ui.currentHoveredNode(), nodeFocusable1);
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Pair<Int, DataHandle>>({
            {Move, dataFocusable12},
            {Move, dataFocusable11},
            {Press|Hovering, dataFocusable12},
            {Press|Hovering, dataFocusable11},
            {Focus|Pressed|Hovering, dataFocusable12},
            {Focus|Pressed|Hovering, dataFocusable11},
            {Press, dataFocusable2},
            /* Here the move event didn't happen on nodeFocusable2, so the
               original node is still hovered and the new one is only
               pressed */
            {Blur|Hovering, dataFocusable12},
            {Blur|Hovering, dataFocusable11},
            {Focus|Pressed, dataFocusable2},
            {Press, data},
            {Blur, dataFocusable2},
        })), TestSuite::Compare::Container);
    }
}

void AbstractUserInterfaceTest::eventFocusBlurByPointerPressNotAccepted() {
    /* Like eventFocusNotAccepted(). but handling the press event in addition
       and expecting different behavior (the previous focus not staying) */

    /* Event scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};

    enum Event {
        Accepted = 1,
        Press = 2,
        Focus = 4,
        Blur = 6
    };

    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doPointerPressEvent(UnsignedInt dataId, PointerEvent& event) override {
            if(acceptPress)
                event.setAccepted();
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit,
                (event.isAccepted() ? Accepted : 0)|Press,
                dataHandle(handle(), dataId, 1));
        }
        void doFocusEvent(UnsignedInt dataId, FocusEvent& event) override {
            if(acceptFocus)
                event.setAccepted();
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit,
                (event.isAccepted() ? Accepted : 0)|Focus,
                dataHandle(handle(), dataId, 1));
        }
        void doBlurEvent(UnsignedInt dataId, FocusEvent&) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Blur,
                dataHandle(handle(), dataId, 1));
        }

        Containers::Array<Containers::Pair<Int, DataHandle>> eventCalls;
        bool acceptPress = true,
            acceptFocus = true;
    };

    NodeHandle nodeFocusable1 = ui.createNode({30.0f, 20.0f}, {20.0f, 20.0f}, NodeFlag::Focusable);
    NodeHandle nodeFocusable2 = ui.createNode({50.0f, 20.0f}, {20.0f, 20.0f}, NodeFlag::Focusable);

    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer()));
    DataHandle dataFocusable1 = layer.create(nodeFocusable1);
    DataHandle dataFocusable2 = layer.create(nodeFocusable2);

    /* Unlike with e.g. eventPointerMoveNotAccepted(), here's no fallback to
       nodes below going on, thus the test is a lot simpler */

    /* Not accepting a focus event still returns true but doesn't set the
       focused node */
    {
        PointerEvent event{{}, Pointer::MouseLeft};
        layer.acceptPress = true;
        layer.acceptFocus = false;
        CORRADE_VERIFY(ui.pointerPressEvent({45.0f, 35.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), nodeFocusable1);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Pair<Int, DataHandle>>({
            {Press|Accepted, dataFocusable1},
            {Focus, dataFocusable1}
        })), TestSuite::Compare::Container);

    /* With a node already focused, pressing completely outside of anything
       makes it blurred */
    } {
        layer.eventCalls = {};

        PointerEvent event1{{}, Pointer::Pen};
        layer.acceptPress = true;
        layer.acceptFocus = true;
        CORRADE_VERIFY(ui.pointerPressEvent({40.0f, 30.0f}, event1));
        CORRADE_COMPARE(ui.currentPressedNode(), nodeFocusable1);
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeFocusable1);

        PointerEvent event2{{}, Pointer::Pen};
        layer.acceptPress = true;
        layer.acceptFocus = true;
        CORRADE_VERIFY(!ui.pointerPressEvent({1000.0f, 1000.0f}, event2));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Pair<Int, DataHandle>>({
            {Press|Accepted, dataFocusable1},
            {Focus|Accepted, dataFocusable1},
            {Blur, dataFocusable1},
            /* Press not directed anywhere */
        })), TestSuite::Compare::Container);

    /* With a node already focused, not accepting a press makes it blurred */
    } {
        layer.eventCalls = {};

        PointerEvent event1{{}, Pointer::Pen};
        layer.acceptPress = true;
        layer.acceptFocus = true;
        CORRADE_VERIFY(ui.pointerPressEvent({40.0f, 30.0f}, event1));
        CORRADE_COMPARE(ui.currentPressedNode(), nodeFocusable1);
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeFocusable1);

        PointerEvent event2{{}, Pointer::Finger};
        layer.acceptPress = false;
        layer.acceptFocus = true;
        CORRADE_VERIFY(!ui.pointerPressEvent({55.0f, 25.0f}, event2));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Pair<Int, DataHandle>>({
            {Press|Accepted, dataFocusable1},
            {Focus|Accepted, dataFocusable1},
            {Press, dataFocusable2},
            {Blur, dataFocusable1},
            /* Press not accepted, so Focus not even emitted */
        })), TestSuite::Compare::Container);

    /* With a node already focused, not accepting a focus event still returns
       true but also makes it blurred */
    } {
        layer.eventCalls = {};

        PointerEvent event1{{}, Pointer::MouseLeft};
        layer.acceptPress = true;
        layer.acceptFocus = true;
        CORRADE_VERIFY(ui.pointerPressEvent({40.0f, 30.0f}, event1));
        CORRADE_COMPARE(ui.currentPressedNode(), nodeFocusable1);
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeFocusable1);

        PointerEvent event2{{}, Pointer::Pen};
        layer.acceptPress = true;
        layer.acceptFocus = false;
        CORRADE_VERIFY(ui.pointerPressEvent({55.0f, 25.0f}, event2));
        CORRADE_COMPARE(ui.currentPressedNode(), nodeFocusable2);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Pair<Int, DataHandle>>({
            {Press|Accepted, dataFocusable1},
            {Focus|Accepted, dataFocusable1},
            {Press|Accepted, dataFocusable2},
            {Blur, dataFocusable1},
            {Focus, dataFocusable2},
        })), TestSuite::Compare::Container);

    /* Calling a non-accepted focus event on the same node then calls Blur and
       makes it null */
    } {
        layer.eventCalls = {};

        PointerEvent event1{{}, Pointer::MouseLeft};
        layer.acceptPress = true;
        layer.acceptFocus = true;
        CORRADE_VERIFY(ui.pointerPressEvent({60.0f, 25.0f}, event1));
        CORRADE_COMPARE(ui.currentPressedNode(), nodeFocusable2);
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeFocusable2);

        PointerEvent event2{{}, Pointer::MouseLeft};
        layer.acceptPress = true;
        layer.acceptFocus = false;
        CORRADE_VERIFY(ui.pointerPressEvent({55.0f, 35.0f}, event2));
        CORRADE_COMPARE(ui.currentPressedNode(), nodeFocusable2);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Pair<Int, DataHandle>>({
            {Press|Accepted, dataFocusable2},
            {Focus|Accepted, dataFocusable2},
            {Press|Accepted, dataFocusable2},
            {Focus, dataFocusable2},
            {Blur, dataFocusable2}
        })), TestSuite::Compare::Container);
    }
}

void AbstractUserInterfaceTest::eventFocusBlurByPointerPressNodeDisabledNoEvents() {
    auto&& data = EventFocusBlurByPointerPressNodeDisabledNoEventsData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Like eventFocusNodeHiddenDisabledNoEvents(), but handling the press
       event in addition and omitting the hidden bit (for which no presses
       happen) */

    /* Event scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};

    enum Event {
        Press = 0,
        Focus = 1,
        Blur = 2
    };

    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doPointerPressEvent(UnsignedInt dataId, PointerEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Press,
                dataHandle(handle(), dataId, 1));
            event.setAccepted();
        }
        void doFocusEvent(UnsignedInt dataId, FocusEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Focus,
                dataHandle(handle(), dataId, 1));
            event.setAccepted();
        }
        void doBlurEvent(UnsignedInt dataId, FocusEvent&) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Blur,
                dataHandle(handle(), dataId, 1));
        }

        Containers::Array<Containers::Pair<Int, DataHandle>> eventCalls;
    };

    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer()));

    /* Nested node in order to verify that the hidden/disabled/... flag gets
       propagated through the hierarchy */
    NodeHandle node = ui.createNode({20.0f, 0.0f}, {50.0f, 40.0f});
    NodeHandle nested = ui.createNode(node, {30.0f, 20.0f}, {20.0f, 20.0f}, NodeFlag::Focusable);
    DataHandle nestedData = layer.create(nested);

    NodeHandle previouslyFocused = NodeHandle::Null;
    DataHandle previouslyFocusedData = DataHandle::Null;
    if(data.previousFocused) {
        previouslyFocused = ui.createNode({20.0f, 0.0f}, {20.0f, 20.0f}, NodeFlag::Focusable);
        previouslyFocusedData = layer.create(previouslyFocused);

        PointerEvent press{{}, Pointer::Pen};
        CORRADE_VERIFY(ui.pointerPressEvent({30.0f, 15.0f}, press));
        CORRADE_COMPARE(ui.currentPressedNode(), previouslyFocused);
        CORRADE_COMPARE(ui.currentFocusedNode(), previouslyFocused);
    }

    ui.addNodeFlags(node, data.flag);

    /* The flag should propagate from the parent and prevent the node from
       getting pressed or focused. The previously focused node, if any, should
       get blurred, because that's the intended behavior when clicking outside
       of a focused node. As the node isn't accepting any events, the event
       handler returns false. */
    {
        PointerEvent press{{}, Pointer::Pen};
        CORRADE_VERIFY(!ui.pointerPressEvent({60.0f, 35.0f}, press));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
    }

    /* The only event that should get called is for the previous node focus, if
       any, and then a blur */
    if(data.previousFocused)
        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Pair<Int, DataHandle>>({
            {Press, previouslyFocusedData},
            {Focus, previouslyFocusedData},
            {Blur, previouslyFocusedData},
        })), TestSuite::Compare::Container);
    else
        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Pair<Int, DataHandle>>({
        })), TestSuite::Compare::Container);

    /* Removing the flag again makes it work */
    layer.eventCalls = {};
    ui.clearNodeFlags(node, data.flag);
    {
        PointerEvent press{{}, Pointer::Pen};
        CORRADE_VERIFY(ui.pointerPressEvent({60.0f, 35.0f}, press));
        CORRADE_COMPARE(ui.currentPressedNode(), nested);
        CORRADE_COMPARE(ui.currentFocusedNode(), nested);
    }
    CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Pair<Int, DataHandle>>({
        {Press, nestedData},
        {Focus, nestedData}
    })), TestSuite::Compare::Container);
}

void AbstractUserInterfaceTest::eventKeyPressRelease() {
    auto&& data = EventLayouterUpdateData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* framebufferSize isn't used for anything here; events should get scaled
       to (0.1, 0.01) */
    AbstractUserInterface ui{{300.0f, 200.0f}, {3000.0f, 20000.0f}, {30, 20}};

    enum Event {
        Captured = 1,
        Pressed = 2,
        Hovering = 4,
        /* All below have to be multiples of 8 to not clash with the above */
        Press = 8,
        Release = 16,
        PointerMove = 24,
        Focus = 32
    };

    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doKeyPressEvent(UnsignedInt dataId, KeyEvent& event) override {
            CORRADE_COMPARE(event.time(), 12345_nsec);
            CORRADE_COMPARE(event.key(), Key::C);
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit,
                (event.isCaptured() ? Captured : 0)|
                (event.isHovering() ? Hovering : 0)|Press,
                dataHandle(handle(), dataId, 1), event.position());
            if(acceptPress)
                event.setAccepted();
        }
        void doKeyReleaseEvent(UnsignedInt dataId, KeyEvent& event) override {
            CORRADE_COMPARE(event.time(), 12345_nsec);
            CORRADE_COMPARE(event.key(), Key::C);
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit,
                (event.isCaptured() ? Captured : 0)|
                (event.isHovering() ? Hovering : 0)|Release,
                dataHandle(handle(), dataId, 1), event.position());
            if(acceptRelease)
                event.setAccepted();
        }
        void doPointerMoveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit,
                (event.isCaptured() ? Captured : 0)|
                (event.isHovering() ? Hovering : 0)|PointerMove,
                dataHandle(handle(), dataId, 1), event.position());
            if(capturePointerMove)
                event.setCaptured(*capturePointerMove);
            event.setAccepted();
        }
        void doFocusEvent(UnsignedInt dataId, FocusEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit,
                (event.isPressed() ? Pressed : 0)|
                (event.isHovering() ? Hovering : 0)|Focus,
                dataHandle(handle(), dataId, 1), Containers::NullOpt);
            event.setAccepted();
        }

        bool acceptPress = true,
            acceptRelease = true;
        Containers::Optional<bool> capturePointerMove;
        Containers::Array<Containers::Triple<Int, DataHandle, Containers::Optional<Vector2>>> eventCalls;
    };

    struct Layouter: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::add;

        void doUpdate(Containers::BitArrayView layoutIdsToUpdate, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>& nodeOffsets, const  Containers::StridedArrayView1D<Vector2>& nodeSizes) override {
            const Containers::StridedArrayView1D<const NodeHandle> nodes = this->nodes();
            for(std::size_t i = 0; i != layoutIdsToUpdate.size(); ++i) {
                if(!layoutIdsToUpdate[i])
                    continue;
                nodeOffsets[nodeHandleId(nodes[i])].y() -= 1000.0f;
                nodeSizes[nodeHandleId(nodes[i])] *= 100.0f;
            }
        }
    };

    /* Two nodes next to each other. If the layouter is enabled, the nodes are
       shifted & scaled which makes them completely unreachable by events, and
       the layouter then undoes that */
    Vector2 baseNodeOffset{0.0f, data.layouter ? 1000.0f : 0.0f};
    Vector2 baseNodeScale{data.layouter ? 0.01f : 1.0f};
    NodeHandle left = ui.createNode(
        baseNodeOffset + Vector2{20.0f, 0.0f},
        baseNodeScale*Vector2{20.0f, 20.0f}, NodeFlag::Focusable);
    NodeHandle right = ui.createNode(
        baseNodeOffset + Vector2{40.0f, 0.0f},
        baseNodeScale*Vector2{20.0f, 20.0f}, NodeFlag::Focusable);

    /* Update explicitly before adding the layouter as
       NeedsLayoutAssignmentUpdate is a subset of this, and having just that
       one set may uncover accidental omissions in internal state updates
       compared to updating just once after creating both nodes and data */
    if(data.update) {
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    if(data.layouter) {
        Layouter& layouter = ui.setLayouterInstance(Containers::pointer<Layouter>(ui.createLayouter()));
        layouter.add(left);
        layouter.add(right);

        /* Update explicitly before adding the layer as
           NeedsDataAttachmentUpdate is a subset of this, and having just that
           one set may uncover accidental omissions in internal state updates
           compared to updating just once after creating both nodes and data */
        if(data.update) {
            CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsLayoutAssignmentUpdate);
            ui.update();
            CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
        }
    }

    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer()));
    DataHandle leftData1 = layer.create(left);
    DataHandle leftData2 = layer.create(left);
    DataHandle rightData = layer.create(right);

    if(data.update) {
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate);
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    /* Press / release with no preceding pointer event isn't propagated
       anywhere */
    {
        layer.eventCalls = {};

        KeyEvent eventPress{12345_nsec, Key::C, {}};
        layer.acceptPress = true;
        CORRADE_VERIFY(!ui.keyPressEvent(eventPress));

        KeyEvent eventRelease{12345_nsec, Key::C, {}};
        layer.acceptRelease = true;
        CORRADE_VERIFY(!ui.keyReleaseEvent(eventRelease));

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Containers::Optional<Vector2>>>({
        })), TestSuite::Compare::Container);

    /* A preceding pointer event that doesn't actually hover anything doesn't
       make them propagated anywhere either */
    } {
        layer.eventCalls = {};

        PointerMoveEvent eventMove{{}, {}, {}};
        CORRADE_VERIFY(!ui.pointerMoveEvent({200.0f, 5000.0f}, eventMove));
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentGlobalPointerPosition(), (Vector2{20.0f, 50.0f}));

        KeyEvent eventPress{12345_nsec, Key::C, {}};
        layer.acceptPress = true;
        CORRADE_VERIFY(!ui.keyPressEvent(eventPress));

        KeyEvent eventRelease{12345_nsec, Key::C, {}};
        layer.acceptRelease = true;
        CORRADE_VERIFY(!ui.keyReleaseEvent(eventRelease));

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Containers::Optional<Vector2>>>({
        })), TestSuite::Compare::Container);

    /* A pointer event that hovers a node finally makes them propagated
       there */
    } {
        layer.eventCalls = {};

        PointerMoveEvent eventMove{{}, {}, {}};
        layer.capturePointerMove = {};
        CORRADE_VERIFY(ui.pointerMoveEvent({500.0f, 1000.0f}, eventMove));
        CORRADE_COMPARE(ui.currentHoveredNode(), right);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentGlobalPointerPosition(), (Vector2{50.0f, 10.0f}));

        KeyEvent eventRelease{12345_nsec, Key::C, {}};
        layer.acceptRelease = true;
        CORRADE_VERIFY(ui.keyReleaseEvent(eventRelease));

        KeyEvent eventPress{12345_nsec, Key::C, {}};
        layer.acceptPress = true;
        CORRADE_VERIFY(ui.keyPressEvent(eventPress));

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Containers::Optional<Vector2>>>({
            {PointerMove, rightData, Vector2{10.0f, 10.0f}},
            {Release|Hovering, rightData, Vector2{10.0f, 10.0f}},
            {Press|Hovering, rightData, Vector2{10.0f, 10.0f}},
        })), TestSuite::Compare::Container);

    /* Not accepting the events gets correctly propagated */
    } {
        layer.eventCalls = {};

        PointerMoveEvent eventMove{{}, {}, {}};
        layer.capturePointerMove = {};
        CORRADE_VERIFY(ui.pointerMoveEvent({550.0f, 1000.0f}, eventMove));
        CORRADE_COMPARE(ui.currentHoveredNode(), right);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentGlobalPointerPosition(), (Vector2{55.0f, 10.0f}));

        KeyEvent eventPress{12345_nsec, Key::C, {}};
        layer.acceptPress = false;
        CORRADE_VERIFY(!ui.keyPressEvent(eventPress));

        KeyEvent eventRelease{12345_nsec, Key::C, {}};
        layer.acceptRelease = false;
        CORRADE_VERIFY(!ui.keyReleaseEvent(eventRelease));

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Containers::Optional<Vector2>>>({
            {PointerMove|Hovering, rightData, Vector2{15.0f, 10.0f}},
            {Press|Hovering, rightData, Vector2{15.0f, 10.0f}},
            {Release|Hovering, rightData, Vector2{15.0f, 10.0f}},
        })), TestSuite::Compare::Container);

    /* If a node is captured, it's picked instead of a node that's under
       the current pointer position. The capture doesn't get reset on key
       release. */
    } {
        layer.eventCalls = {};

        PointerMoveEvent eventMove1{{}, {}, {}};
        layer.capturePointerMove = true;
        CORRADE_VERIFY(ui.pointerMoveEvent({300.0f, 1000.0f}, eventMove1));
        CORRADE_COMPARE(ui.currentHoveredNode(), left);
        CORRADE_COMPARE(ui.currentCapturedNode(), left);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentGlobalPointerPosition(), (Vector2{30.0f, 10.0f}));

        PointerMoveEvent eventMove2{{}, {}, {}};
        layer.capturePointerMove = {};
        CORRADE_VERIFY(ui.pointerMoveEvent({500.0f, 1000.0f}, eventMove1));
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentCapturedNode(), left);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentGlobalPointerPosition(), (Vector2{50.0f, 10.0f}));

        KeyEvent eventPress{12345_nsec, Key::C, {}};
        layer.acceptPress = true;
        CORRADE_VERIFY(ui.keyPressEvent(eventPress));

        KeyEvent eventRelease{12345_nsec, Key::C, {}};
        layer.acceptRelease = true;
        CORRADE_VERIFY(ui.keyReleaseEvent(eventRelease));

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Containers::Optional<Vector2>>>({
            /* Verifying also that it gets propagated to all data */
            {PointerMove, leftData2, Vector2{10.0f, 10.0f}},
            {PointerMove|Captured, leftData1, Vector2{10.0f, 10.0f}},
            /* All below are actually the right node */
            {PointerMove|Captured, leftData2, Vector2{30.0f, 10.0f}},
            {PointerMove|Captured, leftData1, Vector2{30.0f, 10.0f}},
            {Press|Captured, leftData2, Vector2{30.0f, 10.0f}},
            {Press|Captured, leftData1, Vector2{30.0f, 10.0f}},
            {Release|Captured, leftData2, Vector2{30.0f, 10.0f}},
            {Release|Captured, leftData1, Vector2{30.0f, 10.0f}},
        })), TestSuite::Compare::Container);

    /* The capture also doesn't get reset if the key events aren't accepted.
       There's no way to reset the capture from the event either. */
    } {
        /* Just to reset everything */
        /** @todo have a pointerCancelEvent() for this */
        PointerMoveEvent eventMoveReset{{}, {}, {}};
        layer.capturePointerMove = false;
        CORRADE_VERIFY(ui.pointerMoveEvent({200.0f, 5000.0f}, eventMoveReset));
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);

        layer.eventCalls = {};

        PointerMoveEvent eventMove{{}, {}, {}};
        layer.capturePointerMove = true;
        CORRADE_VERIFY(ui.pointerMoveEvent({500.0f, 1000.0f}, eventMove));
        CORRADE_COMPARE(ui.currentCapturedNode(), right);
        CORRADE_COMPARE(ui.currentHoveredNode(), right);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentGlobalPointerPosition(), (Vector2{50.0f, 10.0f}));

        KeyEvent eventPress{12345_nsec, Key::C, {}};
        layer.acceptPress = false;
        CORRADE_VERIFY(!ui.keyPressEvent(eventPress));

        KeyEvent eventRelease{12345_nsec, Key::C, {}};
        layer.acceptRelease = false;
        CORRADE_VERIFY(!ui.keyReleaseEvent(eventRelease));

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Containers::Optional<Vector2>>>({
            {PointerMove, rightData, Vector2{10.0f, 10.0f}},
            {Press|Captured|Hovering, rightData, Vector2{10.0f, 10.0f}},
            {Release|Captured|Hovering, rightData, Vector2{10.0f, 10.0f}},
        })), TestSuite::Compare::Container);

    /* If a node is focused, it receives the events instead of a captured /
       hovered node */
    } {
        layer.eventCalls = {};

        FocusEvent eventFocus{{}};
        CORRADE_VERIFY(ui.focusEvent(left, eventFocus));
        CORRADE_COMPARE(ui.currentCapturedNode(), right);
        CORRADE_COMPARE(ui.currentHoveredNode(), right);
        CORRADE_COMPARE(ui.currentFocusedNode(), left);

        KeyEvent eventPress{12345_nsec, Key::C, {}};
        layer.acceptPress = true;
        CORRADE_VERIFY(ui.keyPressEvent(eventPress));

        KeyEvent eventRelease{12345_nsec, Key::C, {}};
        layer.acceptRelease = true;
        CORRADE_VERIFY(ui.keyReleaseEvent(eventRelease));

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Containers::Optional<Vector2>>>({
            {Focus, leftData2, Containers::NullOpt},
            {Focus, leftData1, Containers::NullOpt},
            {Press, leftData2, Containers::NullOpt},
            {Press, leftData1, Containers::NullOpt},
            {Release, leftData2, Containers::NullOpt},
            {Release, leftData1, Containers::NullOpt},
        })), TestSuite::Compare::Container);

    /* If the node is focused but also captured and hovered, the hovered bit is
       set but not captured, and the position is still omitted like in the
       above case */
    } {
        layer.eventCalls = {};

        FocusEvent eventFocus{{}};
        CORRADE_VERIFY(ui.focusEvent(right, eventFocus));
        CORRADE_COMPARE(ui.currentCapturedNode(), right);
        CORRADE_COMPARE(ui.currentHoveredNode(), right);
        CORRADE_COMPARE(ui.currentFocusedNode(), right);

        KeyEvent eventPress{12345_nsec, Key::C, {}};
        layer.acceptPress = true;
        CORRADE_VERIFY(ui.keyPressEvent(eventPress));

        KeyEvent eventRelease{12345_nsec, Key::C, {}};
        layer.acceptRelease = true;
        CORRADE_VERIFY(ui.keyReleaseEvent(eventRelease));

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Containers::Optional<Vector2>>>({
            {Focus|Hovering, rightData, Containers::NullOpt},
            {Press|Hovering, rightData, Containers::NullOpt},
            {Release|Hovering, rightData, Containers::NullOpt},
        })), TestSuite::Compare::Container);

    /* Removing the focus makes it behave the same as before, i.e. picking the
       node based on capture */
    } {
        layer.eventCalls = {};

        FocusEvent eventFocus{{}};
        CORRADE_VERIFY(!ui.focusEvent(NodeHandle::Null, eventFocus));
        CORRADE_COMPARE(ui.currentCapturedNode(), right);
        CORRADE_COMPARE(ui.currentHoveredNode(), right);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);

        KeyEvent eventPress{12345_nsec, Key::C, {}};
        layer.acceptPress = true;
        CORRADE_VERIFY(ui.keyPressEvent(eventPress));

        KeyEvent eventRelease{12345_nsec, Key::C, {}};
        layer.acceptRelease = true;
        CORRADE_VERIFY(ui.keyReleaseEvent(eventRelease));

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Containers::Optional<Vector2>>>({
            {Press|Captured|Hovering, rightData, Vector2{10.0f, 10.0f}},
            {Release|Captured|Hovering, rightData, Vector2{10.0f, 10.0f}},
        })), TestSuite::Compare::Container);
    }
}

void AbstractUserInterfaceTest::eventTextInput() {
    auto&& data = UpdateData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Event scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};

    enum Event {
        PointerMove = 0,
        Focus = 1,
        TextInput = 2
    };

    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doPointerMoveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, PointerMove,
                dataHandle(handle(), dataId, 1), "");
            event.setAccepted();
        }
        void doFocusEvent(UnsignedInt dataId, FocusEvent& event) override {
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, Focus,
                dataHandle(handle(), dataId, 1), "");
            event.setAccepted();
        }
        void doTextInputEvent(UnsignedInt dataId, TextInputEvent& event) override {
            CORRADE_COMPARE(event.time(), 12345_nsec);
            /* The data generation is faked here, but it matches as we don't
               reuse any data */
            arrayAppend(eventCalls, InPlaceInit, TextInput,
                dataHandle(handle(), dataId, 1), event.text());
            if(accept)
                event.setAccepted();
        }

        bool accept = true;
        Containers::Array<Containers::Triple<Int, DataHandle, Containers::StringView>> eventCalls;
    };

    /* Two nodes next to each other */
    NodeHandle left = ui.createNode({20.0f, 0.0f}, {20.0f, 20.0f}, NodeFlag::Focusable);
    NodeHandle right = ui.createNode({40.0f, 0.0f}, {20.0f, 20.0f}, NodeFlag::Focusable);

    /* Update explicitly before adding the layer as NeedsDataAttachmentUpdate
       is a subset of this, and having just that one set may uncover accidental
       omissions in internal state updates compared to updating just once after
       creating both nodes and data */
    if(data.update) {
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeUpdate);
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer()));
    DataHandle leftData1 = layer.create(left);
    DataHandle leftData2 = layer.create(left);
    DataHandle rightData = layer.create(right);

    if(data.update) {
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataAttachmentUpdate);
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    /* Text input event with no focused node isn't propagated anywhere */
    {
        layer.eventCalls = {};

        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);

        TextInputEvent event{12345_nsec, "hello"};
        layer.accept = true;
        CORRADE_VERIFY(!ui.textInputEvent(event));
        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Containers::StringView>>({
        })), TestSuite::Compare::Container);

    /* A hovered node doesn't propagate it anywhere either */
    } {
        layer.eventCalls = {};

        PointerMoveEvent eventMove{{}, {}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({30.0f, 10.0f}, eventMove));
        CORRADE_COMPARE(ui.currentHoveredNode(), left);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);

        TextInputEvent event{12345_nsec, "hello"};
        layer.accept = true;
        CORRADE_VERIFY(!ui.textInputEvent(event));
        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Containers::StringView>>({
            {PointerMove, leftData2, {}},
            {PointerMove, leftData1, {}},
        })), TestSuite::Compare::Container);

    /* With a focused node it gets propagated; focusing another directs it
       elsewhere, it gets called on all data */
    } {
        /* Just to reset the hover */
        /** @todo have a pointerCancelEvent() for this */
        PointerMoveEvent eventMoveReset{{}, {}, {}};
        ui.pointerMoveEvent({1000.0f, 1000.0f}, eventMoveReset);

        layer.eventCalls = {};

        FocusEvent eventFocus1{{}};
        CORRADE_VERIFY(ui.focusEvent(right, eventFocus1));

        TextInputEvent event1{12345_nsec, "hello"};
        layer.accept = true;
        CORRADE_VERIFY(ui.textInputEvent(event1));

        FocusEvent eventFocus2{{}};
        CORRADE_VERIFY(ui.focusEvent(left, eventFocus2));

        TextInputEvent event2{12345_nsec, "bye"};
        layer.accept = true;
        CORRADE_VERIFY(ui.textInputEvent(event2));

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Containers::StringView>>({
            {Focus, rightData, {}},
            {TextInput, rightData, "hello"},
            {Focus, leftData2, {}},
            {Focus, leftData1, {}},
            {TextInput, leftData2, "bye"},
            {TextInput, leftData1, "bye"},
        })), TestSuite::Compare::Container);

    /* Not accepting the event gets correctly propagated */
    } {
        layer.eventCalls = {};

        FocusEvent eventFocus{{}};
        CORRADE_VERIFY(ui.focusEvent(right, eventFocus));

        TextInputEvent event{12345_nsec, "nope"};
        layer.accept = false;
        CORRADE_VERIFY(!ui.textInputEvent(event));

        CORRADE_COMPARE_AS(layer.eventCalls, (Containers::arrayView<Containers::Triple<Int, DataHandle, Containers::StringView>>({
            {Focus, rightData, {}},
            {TextInput, rightData, "nope"},
        })), TestSuite::Compare::Container);
    }
}

}}}}

struct CustomEvent {};

struct Movable {
    Movable(Magnum::Nanoseconds time): time{time} {}
    Movable(const Movable&) = delete;
    Movable& operator=(const Movable&) = delete;
    /* With the defaulted moves MSVC 2015 segfaults (!!) when executing the
       test. LOLOL. */
    #ifndef CORRADE_MSVC2015_COMPATIBILITY
    Movable(Movable&&) = default;
    Movable& operator=(Movable&&) = default;
    #else
    Movable(Movable&& other): time{other.time} {}
    Movable& operator=(Movable&& other) {
        Corrade::Utility::swap(time, other.time);
        return *this;
    }
    #endif

    Magnum::Nanoseconds time;
};

struct Immovable {
    Immovable() = default;
    Immovable(const Immovable&) = delete;
    Immovable(Immovable&&) = delete;
    Immovable& operator=(const Immovable&) = delete;
    Immovable& operator=(Immovable&&) = delete;
};

namespace Magnum { namespace Whee { namespace Implementation {

template<> struct PointerEventConverter<CustomEvent> {
    static bool press(AbstractUserInterface& ui, CustomEvent&, Immovable&, Movable a) {
        PointerEvent e{a.time, Pointer::MouseLeft};
        return ui.pointerPressEvent(Vector2{}, e);
    }
    static bool release(AbstractUserInterface& ui, CustomEvent&, Immovable&, Movable a) {
        PointerEvent e{a.time, Pointer::MouseLeft};
        return ui.pointerReleaseEvent(Vector2{}, e);
    }
};
template<> struct PointerMoveEventConverter<CustomEvent> {
    static bool move(AbstractUserInterface& ui, CustomEvent&, Immovable&, Movable a) {
        PointerMoveEvent e{a.time, {}, {}};
        return ui.pointerMoveEvent(Vector2{}, e);
    }
};
template<> struct KeyEventConverter<CustomEvent> {
    static bool press(AbstractUserInterface& ui, CustomEvent&, Immovable&, Movable a) {
        KeyEvent e{a.time, Key::C, {}};
        return ui.keyPressEvent(e);
    }
    static bool release(AbstractUserInterface& ui, CustomEvent&, Immovable&, Movable a) {
        KeyEvent e{a.time, Key::E, {}};
        return ui.keyReleaseEvent(e);
    }
};
template<> struct TextInputEventConverter<CustomEvent> {
    static bool trigger(AbstractUserInterface& ui, CustomEvent&, Immovable&, Movable a) {
        TextInputEvent e{a.time, "e"};
        return ui.textInputEvent(e);
    }
};

}

namespace Test { namespace {

void AbstractUserInterfaceTest::eventConvertExternal() {
    /* Event scaling doesn't affect these tests */
    AbstractUserInterface ui{{100, 100}};

    enum Event {
        PointerPress = 0,
        PointerRelease = 1,
        PointerMove = 2,
        KeyPress = 3,
        KeyRelease = 4,
        TextInput = 5,
    };

    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override { return LayerFeature::Event; }

        void doPointerPressEvent(UnsignedInt, PointerEvent& event) override {
            CORRADE_COMPARE(event.time(), 13_nsec);
            arrayAppend(eventCalls, PointerPress);
            event.setAccepted();
        }
        void doPointerReleaseEvent(UnsignedInt, PointerEvent& event) override {
            CORRADE_COMPARE(event.time(), 14_nsec);
            arrayAppend(eventCalls, PointerRelease);
            event.setAccepted();
        }
        void doPointerMoveEvent(UnsignedInt, PointerMoveEvent& event) override {
            CORRADE_COMPARE(event.time(), 15_nsec);
            arrayAppend(eventCalls, PointerMove);
            event.setAccepted();
        }
        void doFocusEvent(UnsignedInt, FocusEvent& event) override {
            /* Just so we can accept text input events, not testing any
               EventConverter */
            event.setAccepted();
        }
        void doKeyPressEvent(UnsignedInt, KeyEvent& event) override {
            CORRADE_COMPARE(event.time(), 16_nsec);
            arrayAppend(eventCalls, KeyPress);
            event.setAccepted();
        }
        void doKeyReleaseEvent(UnsignedInt, KeyEvent& event) override {
            CORRADE_COMPARE(event.time(), 17_nsec);
            arrayAppend(eventCalls, KeyRelease);
            event.setAccepted();
        }
        void doTextInputEvent(UnsignedInt, TextInputEvent& event) override {
            CORRADE_COMPARE(event.time(), 18_nsec);
            arrayAppend(eventCalls, TextInput);
            event.setAccepted();
        }

        Containers::Array<Int> eventCalls;
    };
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer()));

    /* Node with single data */
    NodeHandle node = ui.createNode({0.0f, 0.0f}, {20.0f, 20.0f}, NodeFlag::Focusable);
    layer.create(node);

    /* Focus the node so it's able to take text input events */
    FocusEvent focusEvent{{}};
    CORRADE_VERIFY(ui.focusEvent(node, focusEvent));

    Immovable a;
    CustomEvent e;
    CORRADE_VERIFY(ui.pointerPressEvent(e, a, Movable{13_nsec}));
    CORRADE_VERIFY(ui.pointerReleaseEvent(e, a, Movable{14_nsec}));
    CORRADE_VERIFY(ui.pointerMoveEvent(e, a, Movable{15_nsec}));
    CORRADE_VERIFY(ui.keyPressEvent(e, a, Movable{16_nsec}));
    CORRADE_VERIFY(ui.keyReleaseEvent(e, a, Movable{17_nsec}));
    CORRADE_VERIFY(ui.textInputEvent(e, a, Movable{18_nsec}));
    CORRADE_COMPARE_AS(layer.eventCalls, Containers::arrayView<Int>({
        PointerPress,
        PointerRelease,
        PointerMove,
        KeyPress,
        KeyRelease,
        TextInput,
    }), TestSuite::Compare::Container);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Whee::Test::AbstractUserInterfaceTest)
