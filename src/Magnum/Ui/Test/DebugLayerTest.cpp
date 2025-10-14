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

#include <Corrade/Containers/ArrayView.h>
#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/StridedBitArrayView.h>
#include <Corrade/Containers/StringIterable.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Container.h>
#include <Corrade/TestSuite/Compare/Numeric.h>
#include <Corrade/TestSuite/Compare/String.h>
#include <Magnum/Math/Color.h>

#include "Magnum/Ui/AbstractAnimator.h"
#include "Magnum/Ui/AbstractLayouter.h"
#include "Magnum/Ui/AbstractUserInterface.h"
#include "Magnum/Ui/DebugLayer.h"
#include "Magnum/Ui/Event.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/NodeFlags.h"
#include "Magnum/Ui/Implementation/debugLayerState.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct DebugLayerTest: TestSuite::Tester {
    public:
        explicit DebugLayerTest();

        void debugSource();
        void debugSources();
        void debugSourceSupersets();
        void debugFlag();
        void debugFlags();
        void debugFlagsSupersets();

        void construct();
        void constructInvalid();
        void constructCopy();
        void constructMove();

        void flags();
        void flagsInvalid();

        void nodeNameNoOp();
        void nodeName();
        void nodeNameInvalid();

        void layerLayouterAnimatorNameDebugIntegrationSetup();
        void layerLayouterAnimatorNameDebugIntegrationTeardown();

        void layerNameNoOp();
        void layerName();
        void layerNameDebugIntegration();
        void layerNameDebugIntegrationExplicit();
        void layerNameDebugIntegrationExplicitRvalue();
        void layerNameDebugIntegrationCopyConstructPlainStruct();
        void layerNameDebugIntegrationMoveConstructPlainStruct();
        void layerNameInvalid();

        void layouterNameNoOp();
        void layouterName();
        void layouterNameDebugIntegration();
        void layouterNameDebugIntegrationExplicit();
        void layouterNameDebugIntegrationExplicitRvalue();
        void layouterNameDebugIntegrationCopyConstructPlainStruct();
        void layouterNameDebugIntegrationMoveConstructPlainStruct();
        void layouterNameInvalid();

        void animatorNameNoOp();
        void animatorName();
        void animatorNameDebugIntegration();
        void animatorNameDebugIntegrationExplicit();
        void animatorNameDebugIntegrationExplicitRvalue();
        void animatorNameDebugIntegrationCopyConstructPlainStruct();
        void animatorNameDebugIntegrationMoveConstructPlainStruct();
        void animatorNameInvalid();

        void preUpdateNoUi();
        void preUpdateNoOp();
        void preUpdateTrackNodes();
        void preUpdateTrackLayers();
        void preUpdateTrackLayouters();
        void preUpdateTrackAnimators();

        void nodeInspectSetters();
        /* No nodeInspectNoUi() as pointer press to inspect a node can only
           happen if there's any data, which is only done if preUpdate() is
           called, which already checks that the layer is part of a UI */
        void nodeInspectNoOp();
        void nodeInspect();
        void nodeInspectNoCallback();
        void nodeInspectLayerDebugIntegrationExplicit();
        void nodeInspectLayerDebugIntegrationExplicitRvalue();
        void nodeInspectLayouterDebugIntegrationExplicit();
        void nodeInspectLayouterDebugIntegrationExplicitRvalue();
        void nodeInspectAnimatorDebugIntegrationExplicit();
        void nodeInspectAnimatorDebugIntegrationExplicitRvalue();
        void nodeInspectNodeRemoved();
        void nodeInspectInvalid();
        void nodeInspectToggle();
        void nodeInspectSkipNoData();

        void nodeHighlightSetters();
        /* Tests also currentHighlightedNodes() and clearHighlightedNodes() */
        void nodeHighlight();
        void nodeHighlightConditionResetCounters();
        void nodeHighlightConditionNodes();
        void nodeHighlightConditionData();
        void nodeHighlightConditionDataFunctions();
        void nodeHighlightNodeRemoved();
        void nodeHighlightInvalid();

        void updateEmpty();
        void updateDataOrder();
};

using namespace Containers::Literals;
using namespace Math::Literals;

const struct {
    const char* name;
    DebugLayerSources sources;
    bool used;
} LayerNameDebugIntegrationData[]{
    {"layers", DebugLayerSource::Layers, false},
    {"node data", DebugLayerSource::NodeData, false},
    {"node data details", DebugLayerSource::NodeDataDetails, true},
};

const struct {
    const char* name;
    DebugLayerSources sources;
    bool used;
} LayouterNameDebugIntegrationData[]{
    {"layouters", DebugLayerSource::Layouters, false},
    {"node layouts", DebugLayerSource::NodeLayouts, false},
    {"node layout details", DebugLayerSource::NodeLayoutDetails, true},
};

const struct {
    const char* name;
    DebugLayerSources sources;
    bool used;
} AnimatorNameDebugIntegrationData[]{
    {"animators", DebugLayerSource::Animators, false},
    {"node animations", DebugLayerSource::NodeAnimations, false},
    {"node animation details", DebugLayerSource::NodeAnimationDetails, true},
};

const struct {
    const char* name;
    DebugLayerSources sources;
    DebugLayerFlags flags;
    bool expectNoState, expectNoNodes, expectNoLayers, expectNoLayouters, expectNoAnimators, expectNoData;
} PreUpdateNoOpData[]{
    {"",
        {}, {},
        true, true, true, true, true, true},
    {"nodes alone",
        DebugLayerSource::Nodes, {},
        false, false, true, true, true, true},
    {"layers alone",
        DebugLayerSource::Layers, {},
        false, true, false, true, true, true},
    {"layouters alone",
        DebugLayerSource::Layouters, {},
        false, true, true, false, true, true},
    {"animators alone",
        DebugLayerSource::Animators, {},
        false, true, true, true, false, true},
    {"node hierarchy",
        DebugLayerSource::NodeHierarchy, {},
        false, false, true, true, true, true},
    {"node data",
        DebugLayerSource::NodeData, {},
        false, false, false, true, true, true},
    {"node layouts",
        DebugLayerSource::NodeLayouts, {},
        false, false, true, false, true, true},
    {"node animations",
        DebugLayerSource::NodeAnimations, {},
        false, false, true, true, false, true},
    {"node inspect",
        DebugLayerSource::Nodes, DebugLayerFlag::NodeInspect,
        false, false, true, true, true, false},
};

const struct {
    const char* name;
    DebugLayerSources sources;
    DebugLayerFlags flags;
    bool expectData;
} PreUpdateTrackNodesData[]{
    {"",
        DebugLayerSource::Nodes, {}, false},
    {"node data",
        DebugLayerSource::NodeData, {}, false},
    {"node inspect",
        DebugLayerSource::Nodes, DebugLayerFlag::NodeInspect, true},
};

const struct {
    const char* name;
    DebugLayerSources sources;
} PreUpdateTrackLayersData[]{
    {"",
        DebugLayerSource::Layers},
    {"node data",
        DebugLayerSource::NodeData},
};

const struct {
    const char* name;
    DebugLayerSources sources;
} PreUpdateTrackLayoutersData[]{
    {"",
        DebugLayerSource::Layouters},
    {"node layouts",
        DebugLayerSource::NodeLayouts},
};

const struct {
    const char* name;
    DebugLayerSources sources;
} PreUpdateTrackAnimatorsData[]{
    {"",
        DebugLayerSource::Animators},
    {"node animations",
        DebugLayerSource::NodeAnimations},
};

const struct {
    const char* name;
    LayerFeatures features;
    LayerStates expectedState;
} LayerDrawData[]{
    {"", {}, {}},
    {"layer with Draw", LayerFeature::Draw, LayerState::NeedsDataUpdate}
};

const struct {
    const char* name;
    DebugLayerSources sources;
    DebugLayerFlags flags;
    Pointers acceptedPointers;
    PointerEventSource pointerSource;
    Pointer pointer;
    Modifiers modifiers;
    bool primary;
} NodeInspectNoOpData[]{
    {"nothing enabled",
        {}, {}, {},
        PointerEventSource::Mouse, Pointer::MouseRight, Modifier::Ctrl, true},
    {"node inspect not enabled",
        DebugLayerSource::Nodes, {}, {},
        PointerEventSource::Mouse, Pointer::MouseRight, Modifier::Ctrl, true},
    {"different mouse pointer",
        DebugLayerSource::Nodes, DebugLayerFlag::NodeInspect, {},
        PointerEventSource::Mouse, Pointer::MouseMiddle, Modifier::Ctrl, true},
    {"different pen pointer",
        DebugLayerSource::Nodes, DebugLayerFlag::NodeInspect, {},
        PointerEventSource::Pen, Pointer::Pen, Modifier::Ctrl, true},
    {"too little modifiers",
        DebugLayerSource::Nodes, DebugLayerFlag::NodeInspect, {},
        PointerEventSource::Mouse, Pointer::MouseRight, {}, true},
    {"too many modifiers",
        DebugLayerSource::Nodes, DebugLayerFlag::NodeInspect, {},
        PointerEventSource::Mouse, Pointer::MouseRight, Modifier::Ctrl|Modifier::Shift, true},
    {"accepting also touches, but the touch is not primary",
        DebugLayerSource::Nodes, DebugLayerFlag::NodeInspect,
        Pointer::Finger|Pointer::MouseRight,
        PointerEventSource::Touch, Pointer::Finger, Modifier::Ctrl, false},
};

const struct {
    TestSuite::TestCaseDescriptionSourceLocation name;
    DebugLayerSources sources;
    DebugLayerFlags flags;
    Containers::Optional<Containers::StringView> nodeName;
    bool reverseLayerLayouterOrder, someLayerLayouterAnimatorNames, allLayerLayouterAnimatorNames;
    Pointers acceptedPointers;
    Modifiers acceptedModifiers;
    PointerEventSource pointerSource;
    Pointer pointer;
    NodeFlags nodeFlags;
    bool nested, nestedTopLevel, children, hiddenChildren, disabledChildren, noEventsChildren;
    const char* expected;
} NodeInspectData[]{
    {"",
        DebugLayerSource::Nodes, DebugLayerFlag::NodeInspect,
        {}, false, false, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, true, false, false, false, false, false,
        "Node {0x3, 0x1}"},
    {"different used pointer",
        DebugLayerSource::Nodes, DebugLayerFlag::NodeInspect,
        {}, false, false, false,
        {}, {}, PointerEventSource::Pen, Pointer::Eraser,
        {}, true, false, false, false, false, false,
        "Node {0x3, 0x1}"},
    {"different accepted and used pointer",
        DebugLayerSource::Nodes, DebugLayerFlag::NodeInspect,
        {}, false, false, false,
        Pointer::Finger|Pointer::Pen, Modifier::Ctrl|Modifier::Shift|Modifier::Alt,
        PointerEventSource::Pen, Pointer::Pen,
        {}, true, false, false, false, false, false,
        "Node {0x3, 0x1}"},
    {"nested top-level node",
        DebugLayerSource::Nodes, DebugLayerFlag::NodeInspect,
        "A very nice node"_s, false, false, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, true, true, false, false, false, false,
        "Top-level node {0x3, 0x1} A very nice node"},
    {"node name",
        DebugLayerSource::Nodes, DebugLayerFlag::NodeInspect,
        "A very nice node"_s, false, false, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, true, false, false, false, false, false,
        "Node {0x3, 0x1} A very nice node"},
    /* Assuming node name will be always colored, testing the ColorOff /
       ColorAlways flags with it */
    {"node name, color off",
        DebugLayerSource::Nodes, DebugLayerFlag::NodeInspect|DebugLayerFlag::ColorOff,
        "A very nice node"_s, false, false, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, true, false, false, false, false, false,
        "Node {0x3, 0x1} A very nice node"},
    /* ColorOff gets a precedence */
    {"node name, color always + color off",
        DebugLayerSource::Nodes, DebugLayerFlag::NodeInspect|DebugLayerFlag::ColorAlways|DebugLayerFlag::ColorOff,
        "A very nice node"_s, false, false, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, true, false, false, false, false, false,
        "Node {0x3, 0x1} A very nice node"},
    {"empty node name",
        DebugLayerSource::Nodes, DebugLayerFlag::NodeInspect,
        ""_s, false, false, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, true, false, false, false, false, false,
        "Node {0x3, 0x1}"},
    {"node flags",
        DebugLayerSource::Nodes, DebugLayerFlag::NodeInspect,
        {}, false, false, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        NodeFlag::Clip|NodeFlag::FallthroughPointerEvents|NodeFlag::NoBlur,
        true, false, false, false, false, false,
        "Node {0x3, 0x1}\n"
        "  Flags: Clip|FallthroughPointerEvents|NoBlur"},
    {"hierarchy, root",
        DebugLayerSource::NodeHierarchy, DebugLayerFlag::NodeInspect,
        {}, false, false, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, false, true, false, false, false, false,
        "Top-level node {0x3, 0x1}\n"
        "  Root node with 0 direct children"},
    {"hierarchy, nested",
        DebugLayerSource::NodeHierarchy, DebugLayerFlag::NodeInspect,
        {}, false, false, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, true, false, false, false, false, false,
        "Node {0x3, 0x1}\n"
        "  Nested at level 3 with 0 direct children"},
    {"hierarchy, nested top-level",
        DebugLayerSource::NodeHierarchy, DebugLayerFlag::NodeInspect,
        {}, false, false, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, true, true, false, false, false, false,
        "Top-level node {0x3, 0x1}\n"
        "  Nested at level 3 with 0 direct children"},
    {"hierarchy, children",
        DebugLayerSource::NodeHierarchy, DebugLayerFlag::NodeInspect,
        {}, false, false, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, false, true, true, false, false, false,
        "Top-level node {0x3, 0x1}\n"
        "  Root node with 9 direct children"},
    {"hierarchy, nested, children, node flags",
        DebugLayerSource::NodeHierarchy, DebugLayerFlag::NodeInspect,
        {}, false, false, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        NodeFlag::Clip|NodeFlag::FallthroughPointerEvents|NodeFlag::NoBlur,
        true, false, true, false, false, false,
        "Node {0x3, 0x1}\n"
        "  Flags: Clip|FallthroughPointerEvents|NoBlur\n"
        "  Nested at level 3 with 9 direct children"},
    {"hierarchy, hidden children",
        DebugLayerSource::NodeHierarchy, DebugLayerFlag::NodeInspect,
        {}, false, false, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, false, true, true, true, false, false,
        "Top-level node {0x3, 0x1}\n"
        "  Root node with 9 direct children\n"
        "    of which 3 Hidden"},
    {"hierarchy, hidden and no events children",
        DebugLayerSource::NodeHierarchy, DebugLayerFlag::NodeInspect,
        {}, false, false, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, false, true, true, true, false, true,
        "Top-level node {0x3, 0x1}\n"
        "  Root node with 9 direct children\n"
        "    of which 3 Hidden\n"
        "    of which 1 NoEvents"},
    {"hierarchy, nested node and disabled children",
        DebugLayerSource::NodeHierarchy, DebugLayerFlag::NodeInspect,
        {}, false, false, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, true, false, true, false, true, false,
        "Node {0x3, 0x1}\n"
        "  Nested at level 3 with 9 direct children\n"
        "    of which 3 Disabled"},
    {"hierarchy, hidden, disabled and no events children",
        DebugLayerSource::NodeHierarchy, DebugLayerFlag::NodeInspect,
        {}, false, false, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, false, true, true, true, true, true,
        "Top-level node {0x3, 0x1}\n"
        "  Root node with 9 direct children\n"
        "    of which 3 Hidden\n"
        "    of which 2 Disabled\n"
        "    of which 1 NoEvents"},
    {"offset and size",
        DebugLayerSource::NodeOffsetSize, DebugLayerFlag::NodeInspect,
        {}, false, false, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, true, false, false, false, false, false,
        "Node {0x3, 0x1}\n"
        "  Offset: {5, 10}, size: {20, 30}"},
    {"data",
        DebugLayerSource::NodeData, DebugLayerFlag::NodeInspect,
        {}, false, false, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, true, false, false, false, false, false,
        "Node {0x3, 0x1}\n"
        "  10 data from 4 layers"},
    {"data, some layer names",
        DebugLayerSource::NodeData, DebugLayerFlag::NodeInspect,
        {}, false, true, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, true, false, false, false, false, false,
        "Node {0x3, 0x1}\n"
        "  1 data from layer {0x1, 0x1} Second\n"
        "  2 data from layer {0x4, 0x1} No.3\n"
        "  7 data from 2 other layers"},
    {"data details, some layer names",
        DebugLayerSource::NodeDataDetails, DebugLayerFlag::NodeInspect,
        {}, false, true, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, true, false, false, false, false, false,
        "Node {0x3, 0x1}\n"
        "  1 data from layer {0x1, 0x1} Second\n"
        "  Layer No.3 (42069) data {0x0, 0x1} and a value of 1337\n"
        "  Layer No.3 (42069) data {0x1, 0x1} and a value of 1337\n"
        "  7 data from 2 other layers"},
    {"data details, all layer names",
        DebugLayerSource::NodeDataDetails, DebugLayerFlag::NodeInspect,
        {}, false, true, true,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, true, false, false, false, false, false,
        "Node {0x3, 0x1}\n"
        "  3 data from layer {0x0, 0x1} A layer\n"
        "  1 data from layer {0x1, 0x1} Second\n"
        "  Layer No.3 (42069) data {0x0, 0x1} and a value of 1337\n"
        "  Layer No.3 (42069) data {0x1, 0x1} and a value of 1337\n"
        "  4 data from layer {0x6, 0x1} The last ever"},
    {"data details, all layer names, reverse layer order",
        DebugLayerSource::NodeDataDetails, DebugLayerFlag::NodeInspect,
        {}, true, true, true,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, true, false, false, false, false, false,
        "Node {0x3, 0x1}\n"
        "  3 data from layer {0x6, 0x1} A layer\n"
        "  1 data from layer {0x5, 0x1} Second\n"
        "  Layer No.3 (42069) data {0x0, 0x1} and a value of 1337\n"
        "  Layer No.3 (42069) data {0x1, 0x1} and a value of 1337\n"
        "  4 data from layer {0x0, 0x1} The last ever"},
    {"layouts",
        DebugLayerSource::NodeLayouts, DebugLayerFlag::NodeInspect,
        {}, false, false, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, true, false, false, false, false, false,
        "Node {0x3, 0x1}\n"
        "  10 layouts from 4 layouters"},
    {"layouts, some layouter names",
        DebugLayerSource::NodeLayouts, DebugLayerFlag::NodeInspect,
        {}, false, true, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, true, false, false, false, false, false,
        "Node {0x3, 0x1}\n"
        "  1 layouts from layouter {0x1, 0x1} Supplementary\n"
        "  2 layouts from layouter {0x3, 0x1} Tertiary\n"
        "  7 layouts from 2 other layouters"},
    {"layout details, some layouter names",
        DebugLayerSource::NodeLayoutDetails, DebugLayerFlag::NodeInspect,
        {}, false, true, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, true, false, false, false, false, false,
        "Node {0x3, 0x1}\n"
        "  1 layouts from layouter {0x1, 0x1} Supplementary\n"
        "  Layouter Tertiary (96024) layout {0x0, 0x1} and a value of 7331\n"
        "  Layouter Tertiary (96024) layout {0x1, 0x1} and a value of 7331\n"
        "  7 layouts from 2 other layouters"},
    {"layout details, all layouter names",
        DebugLayerSource::NodeLayoutDetails, DebugLayerFlag::NodeInspect,
        {}, false, true, true,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, true, false, false, false, false, false,
        "Node {0x3, 0x1}\n"
        "  3 layouts from layouter {0x0, 0x1} Primary\n"
        "  1 layouts from layouter {0x1, 0x1} Supplementary\n"
        "  Layouter Tertiary (96024) layout {0x0, 0x1} and a value of 7331\n"
        "  Layouter Tertiary (96024) layout {0x1, 0x1} and a value of 7331\n"
        "  4 layouts from layouter {0x5, 0x1} Fallback"},
    {"layout details, all layouter names, reverse layouter order",
        DebugLayerSource::NodeLayoutDetails, DebugLayerFlag::NodeInspect,
        {}, true, true, true,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, true, false, false, false, false, false,
        "Node {0x3, 0x1}\n"
        "  3 layouts from layouter {0x5, 0x1} Primary\n"
        "  1 layouts from layouter {0x4, 0x1} Supplementary\n"
        "  Layouter Tertiary (96024) layout {0x0, 0x1} and a value of 7331\n"
        "  Layouter Tertiary (96024) layout {0x1, 0x1} and a value of 7331\n"
        "  4 layouts from layouter {0x0, 0x1} Fallback"},
    {"animations",
        DebugLayerSource::NodeAnimations, DebugLayerFlag::NodeInspect,
        {}, false, false, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, true, false, false, false, false, false,
        "Node {0x3, 0x1}\n"
        "  1 Reserved animations from 1 animators\n"
        "  2 Scheduled animations from 2 animators\n"
        "  3 Playing animations from 2 animators\n"
        "  1 Paused animations from 1 animators\n"
        "  3 Stopped animations from 2 animators"},
    {"animations, some animator names",
        DebugLayerSource::NodeAnimations, DebugLayerFlag::NodeInspect,
        {}, false, true, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, true, false, false, false, false, false,
        "Node {0x3, 0x1}\n"
        "  1 Scheduled animations from animator {0x1, 0x1} 2nd\n"
        "  2 Playing animations from animator {0x5, 0x1} No#3\n"
        "  1 Paused animations from animator {0x5, 0x1} No#3\n"
        "  1 Reserved animations from 1 other animators\n"
        "  1 Scheduled animations from 1 other animators\n"
        "  1 Playing animations from 1 other animators\n"
        "  3 Stopped animations from 2 other animators"},
    {"animation details, some animator names",
        DebugLayerSource::NodeAnimationDetails, DebugLayerFlag::NodeInspect,
        {}, false, true, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, true, false, false, false, false, false,
        "Node {0x3, 0x1}\n"
        "  1 Scheduled animations from animator {0x1, 0x1} 2nd\n"
        "  Animator No#3 (69420) Playing animation {0x0, 0x1} and a value of 1226\n"
        "  Animator No#3 (69420) Playing animation {0x1, 0x1} and a value of 1226\n"
        "  Animator No#3 (69420) Paused animation {0x2, 0x1} and a value of 1226\n"
        "  1 Reserved animations from 1 other animators\n"
        "  1 Scheduled animations from 1 other animators\n"
        "  1 Playing animations from 1 other animators\n"
        "  3 Stopped animations from 2 other animators"},
    {"animation details, all animator names",
        DebugLayerSource::NodeAnimationDetails, DebugLayerFlag::NodeInspect,
        {}, false, true, true,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, true, false, false, false, false, false,
        "Node {0x3, 0x1}\n"
        "  1 Reserved animations from animator {0x0, 0x1} An animator\n"
        "  1 Scheduled animations from animator {0x0, 0x1} An animator\n"
        "  2 Stopped animations from animator {0x0, 0x1} An animator\n"
        "  1 Scheduled animations from animator {0x1, 0x1} 2nd\n"
        "  Animator No#3 (69420) Playing animation {0x0, 0x1} and a value of 1226\n"
        "  Animator No#3 (69420) Playing animation {0x1, 0x1} and a value of 1226\n"
        "  Animator No#3 (69420) Paused animation {0x2, 0x1} and a value of 1226\n"
        "  1 Playing animations from animator {0x7, 0x1} Termanimator\n"
        "  1 Stopped animations from animator {0x7, 0x1} Termanimator"},
    {"node name, flags, nested top level, all hierarchy, offset and size + data, layout, animation details, some layer and animator names",
        DebugLayerSource::NodeHierarchy|DebugLayerSource::NodeOffsetSize|DebugLayerSource::NodeDataDetails|DebugLayerSource::NodeLayoutDetails|DebugLayerSource::NodeAnimationDetails, DebugLayerFlag::NodeInspect,
        "A very nice node"_s, false, true, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        NodeFlag::Clip|NodeFlag::Focusable, true, true, true, true, true, true,
        "Top-level node {0x3, 0x1} A very nice node\n"
        "  Offset: {5, 10}, size: {20, 30}\n"
        "  Flags: Clip|Focusable\n"
        "  Nested at level 3 with 9 direct children\n"
        "    of which 3 Hidden\n"
        "    of which 2 Disabled\n"
        "    of which 1 NoEvents\n"
        "  1 data from layer {0x1, 0x1} Second\n"
        "  Layer No.3 (42069) data {0x0, 0x1} and a value of 1337\n"
        "  Layer No.3 (42069) data {0x1, 0x1} and a value of 1337\n"
        "  7 data from 2 other layers\n"
        "  1 layouts from layouter {0x1, 0x1} Supplementary\n"
        "  Layouter Tertiary (96024) layout {0x0, 0x1} and a value of 7331\n"
        "  Layouter Tertiary (96024) layout {0x1, 0x1} and a value of 7331\n"
        "  7 layouts from 2 other layouters\n"
        "  1 Scheduled animations from animator {0x1, 0x1} 2nd\n"
        "  Animator No#3 (69420) Playing animation {0x0, 0x1} and a value of 1226\n"
        "  Animator No#3 (69420) Playing animation {0x1, 0x1} and a value of 1226\n"
        "  Animator No#3 (69420) Paused animation {0x2, 0x1} and a value of 1226\n"
        "  1 Reserved animations from 1 other animators\n"
        "  1 Scheduled animations from 1 other animators\n"
        "  1 Playing animations from 1 other animators\n"
        "  3 Stopped animations from 2 other animators"},
    /* The last case here is used in nodeInspectNoCallback() to verify output
       w/o a callback and for visual color verification, it's expected to be
       the most complete, executing all coloring code paths */
};

const struct {
    const char* name;
    bool removeParent;
} NodeInspectHighlightNodeRemovedData[]{
    {"", false},
    {"remove parent node", true},
};

const struct {
    const char* name;
    LayerFeatures features;
    bool callback;
    LayerStates expectedState;
} NodeInspectToggleData[]{
    {"",
        {}, false, {}},
    {"layer with Draw",
        LayerFeature::Draw, false, LayerState::NeedsDataUpdate},
    {"with callback",
        {}, true, {}},
    {"with callback, layer with Draw",
        LayerFeature::Draw, true, LayerState::NeedsDataUpdate},
};

const struct {
    const char* name;
    DebugLayerSources sources;
    DebugLayerFlags flags;
    bool belowData, event;
    bool expectAbove, expectBelow;
} NodeInspectSkipNoDataData[]{
    {"default",
        DebugLayerSource::NodeLayouts|DebugLayerSource::NodeAnimations,
        {}, true, true, true, false},
    {"skip no data to below",
        DebugLayerSource::NodeLayouts|DebugLayerSource::NodeAnimations,
        DebugLayerFlag::NodeInspectSkipNoData, true, true, false, true},
    {"skip no data to nowhere",
        DebugLayerSource::NodeLayouts|DebugLayerSource::NodeAnimations,
        DebugLayerFlag::NodeInspectSkipNoData, false, true, false, false},
    {"skip no data, programmatically",
        DebugLayerSource::NodeLayouts|DebugLayerSource::NodeAnimations,
        DebugLayerFlag::NodeInspectSkipNoData, true, false, true, false},
    {"skip no data, no layouts or animations",
        {},
        DebugLayerFlag::NodeInspectSkipNoData, true, true, false, true},
};

const struct {
    const char* name;
    DebugLayerSources sources;
    bool layer, layouter, animator;
    LayerFeatures features;
    LayerStates expectedState;
} NodeHighlightConditionDataData[]{
    {"layer",
        DebugLayerSource::Layers, true, false, false,
        {}, {}},
    {"layer, debug layer with Draw",
        DebugLayerSource::Layers, true, false, false,
        LayerFeature::Draw, LayerState::NeedsDataUpdate},
    {"layouter",
        DebugLayerSource::Layouters, false, true, false,
        {}, {}},
    {"layouter, debug layer with Draw",
        DebugLayerSource::Layouters, false, true, false,
        LayerFeature::Draw, LayerState::NeedsDataUpdate},
    {"animator",
        DebugLayerSource::Animators, false, false, true,
        {}, {}},
    {"animator, debug layer with Draw",
        DebugLayerSource::Animators, false, false, true,
        LayerFeature::Draw, LayerState::NeedsDataUpdate},
};

const struct {
    const char* name;
    DebugLayerSources sources;
    bool layer, layouter, animator;
} NodeHighlightConditionDataFunctionsData[]{
    {"layer",
        DebugLayerSource::Layers, true, false, false},
    {"layouter",
        DebugLayerSource::Layouters, false, true, false},
    {"animator",
        DebugLayerSource::Animators, false, false, true},
};

const struct {
    const char* name;
    LayerFeatures features;
    DebugLayerFlags flags;
    LayerStates states;
    bool emptyUpdate, expectDataUpdated;
} UpdateDataOrderData[]{
    {"node inspect, empty update",
        LayerFeature::Draw, DebugLayerFlag::NodeInspect,
        LayerState::NeedsDataUpdate, true, false},
    {"node inspect, node offset/size update only",
        LayerFeature::Draw, DebugLayerFlag::NodeInspect,
        LayerState::NeedsNodeOffsetSizeUpdate, false, true},
    {"node inspect, node order update only",
        LayerFeature::Draw, DebugLayerFlag::NodeInspect,
        LayerState::NeedsNodeOrderUpdate, false, true},
    /* These five shouldn't cause anything to be done in update(), resulting in
       the draw offset array to be empty */
    {"node inspect, no Draw feature",
        {}, DebugLayerFlag::NodeInspect,
        LayerState::NeedsDataUpdate, false, false},
    {"node inspect, node enabled update only",
        LayerFeature::Draw, DebugLayerFlag::NodeInspect,
        LayerState::NeedsNodeEnabledUpdate, false, false},
    {"node inspect, node opacity update only",
        LayerFeature::Draw, DebugLayerFlag::NodeInspect,
        LayerState::NeedsNodeOpacityUpdate, false, false},
    {"node inspect, shared data update only",
        LayerFeature::Draw, DebugLayerFlag::NodeInspect,
        LayerState::NeedsSharedDataUpdate, false, false},
    {"node inspect, common data update only",
        LayerFeature::Draw, DebugLayerFlag::NodeInspect,
        LayerState::NeedsCommonDataUpdate, false, false},
    /* This creates data on-demand for just the highlighted nodes, not
       implicitly for all */
    {"node highlight, empty update",
        LayerFeature::Draw, DebugLayerFlags{},
        LayerState::NeedsDataUpdate, true, false},
    {"node highlight, node offset/size update",
        LayerFeature::Draw, DebugLayerFlags{},
        LayerState::NeedsNodeOffsetSizeUpdate, false, true},
};

DebugLayerTest::DebugLayerTest() {
    addTests({&DebugLayerTest::debugSource,
              &DebugLayerTest::debugSources,
              &DebugLayerTest::debugSourceSupersets,
              &DebugLayerTest::debugFlag,
              &DebugLayerTest::debugFlags,
              &DebugLayerTest::debugFlagsSupersets,

              &DebugLayerTest::construct,
              &DebugLayerTest::constructInvalid,
              &DebugLayerTest::constructCopy,
              &DebugLayerTest::constructMove,

              &DebugLayerTest::flags,
              &DebugLayerTest::flagsInvalid,

              &DebugLayerTest::nodeNameNoOp,
              &DebugLayerTest::nodeName,
              &DebugLayerTest::nodeNameInvalid,

              &DebugLayerTest::layerNameNoOp,
              &DebugLayerTest::layerName});

    addInstancedTests({&DebugLayerTest::layerNameDebugIntegration,
                       &DebugLayerTest::layerNameDebugIntegrationExplicit,
                       &DebugLayerTest::layerNameDebugIntegrationExplicitRvalue},
        Containers::arraySize(LayerNameDebugIntegrationData),
        &DebugLayerTest::layerLayouterAnimatorNameDebugIntegrationSetup,
        &DebugLayerTest::layerLayouterAnimatorNameDebugIntegrationTeardown);

    addTests({&DebugLayerTest::layerNameDebugIntegrationCopyConstructPlainStruct,
              &DebugLayerTest::layerNameDebugIntegrationMoveConstructPlainStruct,
              &DebugLayerTest::layerNameInvalid,

              &DebugLayerTest::layouterNameNoOp,
              &DebugLayerTest::layouterName});

    addInstancedTests({&DebugLayerTest::layouterNameDebugIntegration,
                       &DebugLayerTest::layouterNameDebugIntegrationExplicit,
                       &DebugLayerTest::layouterNameDebugIntegrationExplicitRvalue},
        Containers::arraySize(LayouterNameDebugIntegrationData),
        &DebugLayerTest::layerLayouterAnimatorNameDebugIntegrationSetup,
        &DebugLayerTest::layerLayouterAnimatorNameDebugIntegrationTeardown);

    addTests({&DebugLayerTest::layouterNameDebugIntegrationCopyConstructPlainStruct,
              &DebugLayerTest::layouterNameDebugIntegrationMoveConstructPlainStruct,
              &DebugLayerTest::layouterNameInvalid,

              &DebugLayerTest::animatorNameNoOp,
              &DebugLayerTest::animatorName});

    addInstancedTests({&DebugLayerTest::animatorNameDebugIntegration,
                       &DebugLayerTest::animatorNameDebugIntegrationExplicit,
                       &DebugLayerTest::animatorNameDebugIntegrationExplicitRvalue},
        Containers::arraySize(AnimatorNameDebugIntegrationData),
        &DebugLayerTest::layerLayouterAnimatorNameDebugIntegrationSetup,
        &DebugLayerTest::layerLayouterAnimatorNameDebugIntegrationTeardown);

    addTests({&DebugLayerTest::animatorNameDebugIntegrationCopyConstructPlainStruct,
              &DebugLayerTest::animatorNameDebugIntegrationMoveConstructPlainStruct,
              &DebugLayerTest::animatorNameInvalid,

              &DebugLayerTest::preUpdateNoUi});

    addInstancedTests({&DebugLayerTest::preUpdateNoOp},
        Containers::arraySize(PreUpdateNoOpData));

    addInstancedTests({&DebugLayerTest::preUpdateTrackNodes},
        Containers::arraySize(PreUpdateTrackNodesData));

    addInstancedTests({&DebugLayerTest::preUpdateTrackLayers},
        Containers::arraySize(PreUpdateTrackLayersData));

    addInstancedTests({&DebugLayerTest::preUpdateTrackLayouters},
        Containers::arraySize(PreUpdateTrackLayoutersData));

    addInstancedTests({&DebugLayerTest::preUpdateTrackAnimators},
        Containers::arraySize(PreUpdateTrackAnimatorsData));

    addInstancedTests({&DebugLayerTest::nodeInspectSetters},
        Containers::arraySize(LayerDrawData));

    addInstancedTests({&DebugLayerTest::nodeInspectNoOp},
        Containers::arraySize(NodeInspectNoOpData));

    addInstancedTests({&DebugLayerTest::nodeInspect},
        Containers::arraySize(NodeInspectData));

    addTests({&DebugLayerTest::nodeInspectNoCallback,
              &DebugLayerTest::nodeInspectLayerDebugIntegrationExplicit,
              &DebugLayerTest::nodeInspectLayerDebugIntegrationExplicitRvalue,
              &DebugLayerTest::nodeInspectLayouterDebugIntegrationExplicit,
              &DebugLayerTest::nodeInspectLayouterDebugIntegrationExplicitRvalue,
              &DebugLayerTest::nodeInspectAnimatorDebugIntegrationExplicit,
              &DebugLayerTest::nodeInspectAnimatorDebugIntegrationExplicitRvalue});

    addInstancedTests({&DebugLayerTest::nodeInspectNodeRemoved},
        Containers::arraySize(NodeInspectHighlightNodeRemovedData));

    addTests({&DebugLayerTest::nodeInspectInvalid});

    addInstancedTests({&DebugLayerTest::nodeInspectToggle},
        Containers::arraySize(NodeInspectToggleData));

    addInstancedTests({&DebugLayerTest::nodeInspectSkipNoData},
        Containers::arraySize(NodeInspectSkipNoDataData));

    addInstancedTests({&DebugLayerTest::nodeHighlightSetters,
                       &DebugLayerTest::nodeHighlight},
        Containers::arraySize(LayerDrawData));

    addInstancedTests({&DebugLayerTest::nodeHighlightConditionNodes},
        Containers::arraySize(LayerDrawData),
        &DebugLayerTest::nodeHighlightConditionResetCounters,
        &DebugLayerTest::nodeHighlightConditionResetCounters);

    addInstancedTests({&DebugLayerTest::nodeHighlightConditionData},
        Containers::arraySize(NodeHighlightConditionDataData),
        &DebugLayerTest::nodeHighlightConditionResetCounters,
        &DebugLayerTest::nodeHighlightConditionResetCounters);

    addInstancedTests({&DebugLayerTest::nodeHighlightConditionDataFunctions},
        Containers::arraySize(NodeHighlightConditionDataFunctionsData),
        &DebugLayerTest::nodeHighlightConditionResetCounters,
        &DebugLayerTest::nodeHighlightConditionResetCounters);

    addInstancedTests({&DebugLayerTest::nodeHighlightNodeRemoved},
        Containers::arraySize(NodeInspectHighlightNodeRemovedData));

    addTests({&DebugLayerTest::nodeHighlightInvalid});

    addInstancedTests({&DebugLayerTest::updateEmpty},
        Containers::arraySize(LayerDrawData));

    addInstancedTests({&DebugLayerTest::updateDataOrder},
        Containers::arraySize(UpdateDataOrderData));
}

void DebugLayerTest::debugSource() {
    Containers::String out;
    Debug{&out} << DebugLayerSource::NodeHierarchy << DebugLayerSource(0xbeef);
    CORRADE_COMPARE(out, "Ui::DebugLayerSource::NodeHierarchy Ui::DebugLayerSource(0xbeef)\n");
}

void DebugLayerTest::debugSources() {
    Containers::String out;
    Debug{&out} << (DebugLayerSource::Nodes|DebugLayerSource::Layers|DebugLayerSource(0x8000)) << DebugLayerSources{};
    CORRADE_COMPARE(out, "Ui::DebugLayerSource::Nodes|Ui::DebugLayerSource::Layers|Ui::DebugLayerSource(0x8000) Ui::DebugLayerSources{}\n");
}

void DebugLayerTest::debugSourceSupersets() {
    /* NodeHierarchy is a superset of Nodes, so only one should be printed */
    {
        Containers::String out;
        Debug{&out} << (DebugLayerSource::Nodes|DebugLayerSource::NodeHierarchy);
        CORRADE_COMPARE(out, "Ui::DebugLayerSource::NodeHierarchy\n");

    /* NodeOffsetSize is a superset of Nodes, so only one should be printed */
    } {
        Containers::String out;
        Debug{&out} << (DebugLayerSource::Nodes|DebugLayerSource::NodeOffsetSize);
        CORRADE_COMPARE(out, "Ui::DebugLayerSource::NodeOffsetSize\n");

    /* NodeData is a superset of Nodes, so only one should be printed */
    } {
        Containers::String out;
        Debug{&out} << (DebugLayerSource::Nodes|DebugLayerSource::NodeData);
        CORRADE_COMPARE(out, "Ui::DebugLayerSource::NodeData\n");

    /* NodeLayouts is a superset of Nodes, so only one should be printed */
    } {
        Containers::String out;
        Debug{&out} << (DebugLayerSource::Nodes|DebugLayerSource::NodeLayouts);
        CORRADE_COMPARE(out, "Ui::DebugLayerSource::NodeLayouts\n");

    /* NodeAnimations is a superset of Nodes, so only one should be printed */
    } {
        Containers::String out;
        Debug{&out} << (DebugLayerSource::Nodes|DebugLayerSource::NodeAnimations);
        CORRADE_COMPARE(out, "Ui::DebugLayerSource::NodeAnimations\n");

    /* NodeData is a superset of Layers, so only one should be printed */
    } {
        Containers::String out;
        Debug{&out} << (DebugLayerSource::Layers|DebugLayerSource::NodeData);
        CORRADE_COMPARE(out, "Ui::DebugLayerSource::NodeData\n");

    /* NodeLayouts is a superset of Layouters, so only one should be printed */
    } {
        Containers::String out;
        Debug{&out} << (DebugLayerSource::Layouters|DebugLayerSource::NodeLayouts);
        CORRADE_COMPARE(out, "Ui::DebugLayerSource::NodeLayouts\n");

    /* NodeAnimations is a superset of Animators, so only one should be
       printed */
    } {
        Containers::String out;
        Debug{&out} << (DebugLayerSource::Animators|DebugLayerSource::NodeAnimations);
        CORRADE_COMPARE(out, "Ui::DebugLayerSource::NodeAnimations\n");

    /* NodeOffsetSize and NodeHierarchy are both a superset of Nodes, so both
       should be printed */
    } {
        Containers::String out;
        Debug{&out} << (DebugLayerSource::NodeOffsetSize|DebugLayerSource::NodeHierarchy);
        CORRADE_COMPARE(out, "Ui::DebugLayerSource::NodeOffsetSize|Ui::DebugLayerSource::NodeHierarchy\n");

    /* NodeHierarchy and NodeData are both a superset of Nodes, so both should
       be printed */
    } {
        Containers::String out;
        Debug{&out} << (DebugLayerSource::NodeHierarchy|DebugLayerSource::NodeData);
        CORRADE_COMPARE(out, "Ui::DebugLayerSource::NodeHierarchy|Ui::DebugLayerSource::NodeData\n");

    /* NodeHierarchy and NodeLayouts are both a superset of Nodes, so both
       should be printed */
    } {
        Containers::String out;
        Debug{&out} << (DebugLayerSource::NodeHierarchy|DebugLayerSource::NodeLayouts);
        CORRADE_COMPARE(out, "Ui::DebugLayerSource::NodeHierarchy|Ui::DebugLayerSource::NodeLayouts\n");

    /* NodeHierarchy and NodeAnimations are both a superset of Nodes, so both
       should be printed */
    } {
        Containers::String out;
        Debug{&out} << (DebugLayerSource::NodeHierarchy|DebugLayerSource::NodeAnimations);
        CORRADE_COMPARE(out, "Ui::DebugLayerSource::NodeHierarchy|Ui::DebugLayerSource::NodeAnimations\n");

    /* NodeOffsetSize and NodeAnimations are both a superset of Nodes, so both
       should be printed */
    } {
        Containers::String out;
        Debug{&out} << (DebugLayerSource::NodeOffsetSize|DebugLayerSource::NodeAnimations);
        CORRADE_COMPARE(out, "Ui::DebugLayerSource::NodeOffsetSize|Ui::DebugLayerSource::NodeAnimations\n");

    /* NodeData and NodeAnimations are both a superset of Nodes, so both should
       be printed */
    } {
        Containers::String out;
        Debug{&out} << (DebugLayerSource::NodeData|DebugLayerSource::NodeAnimations);
        CORRADE_COMPARE(out, "Ui::DebugLayerSource::NodeData|Ui::DebugLayerSource::NodeAnimations\n");

    /* NodeData and NodeLayouts are both a superset of Nodes, so both should
       be printed */
    } {
        Containers::String out;
        Debug{&out} << (DebugLayerSource::NodeData|DebugLayerSource::NodeAnimations);
        CORRADE_COMPARE(out, "Ui::DebugLayerSource::NodeData|Ui::DebugLayerSource::NodeAnimations\n");

    /* NodeOffsetSize, NodeData, NodeLayouts and NodeAnimations are all a
       superset of Nodes, so all should be printed. There are more combinations
       but all should be handled by the same logic. */
    } {
        Containers::String out;
        Debug{&out} << (DebugLayerSource::NodeOffsetSize|DebugLayerSource::NodeData|DebugLayerSource::NodeAnimations|DebugLayerSource::NodeLayouts);
        CORRADE_COMPARE(out, "Ui::DebugLayerSource::NodeOffsetSize|Ui::DebugLayerSource::NodeData|Ui::DebugLayerSource::NodeLayouts|Ui::DebugLayerSource::NodeAnimations\n");

    /* NodeDataDetails and NodeAnimationDetails are both a superset of Nodes,
       so both should be printed. There are more combinations but all should be
       handled by the same logic. */
    } {
        Containers::String out;
        Debug{&out} << (DebugLayerSource::NodeDataDetails|DebugLayerSource::NodeAnimationDetails);
        CORRADE_COMPARE(out, "Ui::DebugLayerSource::NodeDataDetails|Ui::DebugLayerSource::NodeAnimationDetails\n");

    /* NodeDataDetails, NodeAnimationDetails and NodeLayoutDetails are all a
       superset of Nodes, so all should be printed. There are more combinations
       but all should be handled by the same logic. */
    } {
        Containers::String out;
        Debug{&out} << (DebugLayerSource::NodeDataDetails|DebugLayerSource::NodeLayoutDetails|DebugLayerSource::NodeAnimationDetails);
        CORRADE_COMPARE(out, "Ui::DebugLayerSource::NodeDataDetails|Ui::DebugLayerSource::NodeLayoutDetails|Ui::DebugLayerSource::NodeAnimationDetails\n");

    /* NodeData, NodeLayouts, NodeAnimationDetails and NodeHierarchy are all a
       superset of Nodes, so all should be printed. There are more combinations
       but all should be handled by the same logic. */
    } {
        Containers::String out;
        Debug{&out} << (DebugLayerSource::NodeData|DebugLayerSource::NodeLayouts|DebugLayerSource::NodeAnimationDetails|DebugLayerSource::NodeHierarchy);
        CORRADE_COMPARE(out, "Ui::DebugLayerSource::NodeHierarchy|Ui::DebugLayerSource::NodeData|Ui::DebugLayerSource::NodeLayouts|Ui::DebugLayerSource::NodeAnimationDetails\n");

    /* NodeDataDetails is a superset of NodeData, so only one should be
       printed */
    } {
        Containers::String out;
        Debug{&out} << (DebugLayerSource::NodeData|DebugLayerSource::NodeDataDetails);
        CORRADE_COMPARE(out, "Ui::DebugLayerSource::NodeDataDetails\n");

    /* NodeLayoutDetails is a superset of NodeLayouts, so only one should be
       printed */
    } {
        Containers::String out;
        Debug{&out} << (DebugLayerSource::NodeLayouts|DebugLayerSource::NodeLayoutDetails);
        CORRADE_COMPARE(out, "Ui::DebugLayerSource::NodeLayoutDetails\n");

    /* NodeAnimationDetails is a superset of NodeAnimations, so only one should
       be printed */
    } {
        Containers::String out;
        Debug{&out} << (DebugLayerSource::NodeAnimations|DebugLayerSource::NodeAnimationDetails);
        CORRADE_COMPARE(out, "Ui::DebugLayerSource::NodeAnimationDetails\n");

    /* NodeHierarchy and NodeDataDetails are both a superset of Nodes, so both
       should be printed */
    } {
        Containers::String out;
        Debug{&out} << (DebugLayerSource::NodeHierarchy|DebugLayerSource::NodeDataDetails);
        CORRADE_COMPARE(out, "Ui::DebugLayerSource::NodeHierarchy|Ui::DebugLayerSource::NodeDataDetails\n");

    /* NodeHierarchy and NodeLayoutDetails are both a superset of Nodes, so
       both should be printed */
    } {
        Containers::String out;
        Debug{&out} << (DebugLayerSource::NodeHierarchy|DebugLayerSource::NodeLayoutDetails);
        CORRADE_COMPARE(out, "Ui::DebugLayerSource::NodeHierarchy|Ui::DebugLayerSource::NodeLayoutDetails\n");

    /* NodeHierarchy and NodeAnimationDetails are both a superset of Nodes, so
       both should be printed */
    } {
        Containers::String out;
        Debug{&out} << (DebugLayerSource::NodeHierarchy|DebugLayerSource::NodeAnimationDetails);
        CORRADE_COMPARE(out, "Ui::DebugLayerSource::NodeHierarchy|Ui::DebugLayerSource::NodeAnimationDetails\n");
    }
}

void DebugLayerTest::debugFlag() {
    Containers::String out;
    Debug{&out} << DebugLayerFlag::NodeInspect << DebugLayerFlag(0xef);
    CORRADE_COMPARE(out, "Ui::DebugLayerFlag::NodeInspect Ui::DebugLayerFlag(0xef)\n");
}

void DebugLayerTest::debugFlags() {
    Containers::String out;
    Debug{&out} << (DebugLayerFlag::NodeInspect|DebugLayerFlag::ColorAlways|DebugLayerFlag(0x80)) << DebugLayerFlags{};
    CORRADE_COMPARE(out, "Ui::DebugLayerFlag::NodeInspect|Ui::DebugLayerFlag::ColorAlways|Ui::DebugLayerFlag(0x80) Ui::DebugLayerFlags{}\n");
}

void DebugLayerTest::debugFlagsSupersets() {
    /* NodeInspectSkipNoData is a superset of NodeInspect, so only one should
       be printed */
    {
        Containers::String out;
        Debug{&out} << (DebugLayerFlag::NodeInspect|DebugLayerFlag::NodeInspectSkipNoData);
        CORRADE_COMPARE(out, "Ui::DebugLayerFlag::NodeInspectSkipNoData\n");
    }
}

void DebugLayerTest::construct() {
    DebugLayer layer{layerHandle(137, 0xfe), DebugLayerSource::NodeData|DebugLayerSource::NodeHierarchy, DebugLayerFlag::NodeInspect};
    CORRADE_COMPARE(layer.handle(), layerHandle(137, 0xfe));
    CORRADE_COMPARE(layer.sources(), DebugLayerSource::NodeData|DebugLayerSource::NodeHierarchy);
    CORRADE_COMPARE(layer.flags(), DebugLayerFlag::NodeInspect);

    /* Defaults for flag-related setters are tested in setters*() */
}

void DebugLayerTest::constructInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    Containers::String out;
    Error redirectError{&out};
    DebugLayer{layerHandle(0, 1), DebugLayerSource::Layers, DebugLayerFlag::NodeInspect};
    DebugLayer{layerHandle(0, 1), DebugLayerSource::Nodes|DebugLayerSource::Layers, DebugLayerFlag::NodeInspectSkipNoData};
    CORRADE_COMPARE_AS(out,
        "Ui::DebugLayer: Ui::DebugLayerSource::Nodes has to be enabled for Ui::DebugLayerFlag::NodeInspect\n"
        "Ui::DebugLayer: Ui::DebugLayerSource::NodeData has to be enabled for Ui::DebugLayerFlag::NodeInspectSkipNoData\n",
        TestSuite::Compare::String);
}

void DebugLayerTest::constructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<DebugLayer>{});
    CORRADE_VERIFY(!std::is_copy_assignable<DebugLayer>{});
}

void DebugLayerTest::constructMove() {
    DebugLayer a{layerHandle(137, 0xfe), DebugLayerSource::NodeData, DebugLayerFlag::NodeInspect};

    DebugLayer b{Utility::move(a)};
    CORRADE_COMPARE(b.handle(), layerHandle(137, 0xfe));
    CORRADE_COMPARE(b.sources(), DebugLayerSource::NodeData);
    CORRADE_COMPARE(b.flags(), DebugLayerFlag::NodeInspect);

    DebugLayer c{layerHandle(0, 2), DebugLayerSource::NodeHierarchy, {}};
    c = Utility::move(b);
    CORRADE_COMPARE(c.handle(), layerHandle(137, 0xfe));
    CORRADE_COMPARE(c.sources(), DebugLayerSource::NodeData);
    CORRADE_COMPARE(c.flags(), DebugLayerFlag::NodeInspect);

    CORRADE_VERIFY(std::is_nothrow_move_constructible<DebugLayer>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<DebugLayer>::value);
}

void DebugLayerTest::flags() {
    DebugLayer layer{layerHandle(0, 1), {}, {}};
    CORRADE_COMPARE(layer.flags(), DebugLayerFlags{});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Verify that the set / add / clear works and that it doesn't trigger any
       state update for these. For NodeInspect it does, which is tested in
       nodeInspectToggle(). */
    layer.setFlags(DebugLayerFlags{0x80}|DebugLayerFlag::ColorAlways);
    CORRADE_COMPARE(layer.flags(), DebugLayerFlags{0x80}|DebugLayerFlag::ColorAlways);
    CORRADE_COMPARE(layer.state(), LayerStates{});

    layer.addFlags(DebugLayerFlag::ColorOff);
    CORRADE_COMPARE(layer.flags(), DebugLayerFlags{0x80}|DebugLayerFlag::ColorAlways|DebugLayerFlag::ColorOff);
    CORRADE_COMPARE(layer.state(), LayerStates{});

    layer.clearFlags(DebugLayerFlag::ColorAlways|DebugLayerFlag::ColorOff);
    CORRADE_COMPARE(layer.flags(), DebugLayerFlags{0x80});
    CORRADE_COMPARE(layer.state(), LayerStates{});
}

void DebugLayerTest::flagsInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    DebugLayer layerNoNodes{layerHandle(0, 1), {}, {}};
    DebugLayer layerOnlyNodesLayers{layerHandle(0, 1), DebugLayerSource::Nodes|DebugLayerSource::Layers, {}};

    /* Clearing a NodeInspect / NodeInspectSkipNoData flag that wasn't there
       before is fine even if DebugLayerSource::Nodes / NodeData isn't
       present */
    layerNoNodes.setFlags({});
    layerOnlyNodesLayers.setFlags({});
    layerNoNodes.clearFlags(DebugLayerFlag::NodeInspect);
    layerOnlyNodesLayers.clearFlags(DebugLayerFlag::NodeInspectSkipNoData);

    Containers::String out;
    Error redirectError{&out};
    layerNoNodes.setFlags(DebugLayerFlag::NodeInspect);
    layerNoNodes.addFlags(DebugLayerFlag::NodeInspect);
    layerOnlyNodesLayers.setFlags(DebugLayerFlag::NodeInspectSkipNoData);
    layerOnlyNodesLayers.addFlags(DebugLayerFlag::NodeInspectSkipNoData);
    CORRADE_COMPARE_AS(out,
        "Ui::DebugLayer::setFlags(): Ui::DebugLayerSource::Nodes has to be enabled for Ui::DebugLayerFlag::NodeInspect\n"
        "Ui::DebugLayer::setFlags(): Ui::DebugLayerSource::Nodes has to be enabled for Ui::DebugLayerFlag::NodeInspect\n"
        "Ui::DebugLayer::setFlags(): Ui::DebugLayerSource::NodeData has to be enabled for Ui::DebugLayerFlag::NodeInspectSkipNoData\n"
        "Ui::DebugLayer::setFlags(): Ui::DebugLayerSource::NodeData has to be enabled for Ui::DebugLayerFlag::NodeInspectSkipNoData\n",
        TestSuite::Compare::String);
}

void DebugLayerTest::nodeNameNoOp() {
    /* If Nodes aren't enabled, the APIs don't assert but just don't do
       anything */

    AbstractUserInterface ui{{100, 100}};

    NodeHandle node1 = ui.createNode({}, {});
    NodeHandle node2 = ui.createNode({}, {});

    /* Picking a source that isn't just empty */
    DebugLayer& layer = ui.setLayerInstance(Containers::pointer<DebugLayer>(ui.createLayer(), DebugLayerSource::Layers, DebugLayerFlags{}));
    CORRADE_COMPARE(layer.nodeName(node1), "");
    CORRADE_COMPARE(layer.nodeName(node2), "");

    /* Setting a name doesn't remember anything */
    layer.setNodeName(node2, "A node");
    CORRADE_COMPARE(layer.nodeName(node2), "");
}

void DebugLayerTest::nodeName() {
    AbstractUserInterface ui{{100, 100}};

    NodeHandle node1 = ui.createNode({}, {});
    NodeHandle node2 = ui.createNode({}, {});

    struct Layer: DebugLayer {
        using DebugLayer::DebugLayer;

        const DebugLayer::State& stateData() {
            return *_state;
        }
    };
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), DebugLayerSource::NodeData, DebugLayerFlags{}));

    /* Initially the layer has no node entries even though there already are
       some created */
    CORRADE_VERIFY(layer.stateData().nodes.isEmpty());

    /* By default, any node has the name empty, no null-terminated or global
       flags guaranteed */
    CORRADE_COMPARE(layer.nodeName(node1), "");
    CORRADE_COMPARE(layer.nodeName(node2), "");

    /* Setting a name of a known layer resizes the entries array */
    layer.setNodeName(node2, "Node no.2");
    CORRADE_COMPARE(layer.stateData().nodes.size(), 2);
    CORRADE_COMPARE(layer.nodeName(node2), "Node no.2");

    /* A node outside of any existing bounds will have an empty name; a node
       with known ID but wrong generation also, no null-terminated or global
       flags guaranteed in this case either */
    CORRADE_COMPARE(layer.nodeName(nodeHandle(1048575, 1)), "");
    CORRADE_COMPARE(layer.nodeName(nodeHandle(nodeHandleId(node2), nodeHandleGeneration(node2) + 1)), "");

    /* Create more nodes, their names are empty again, and the size of the
       internal storage doesn't update implicitly to fit those */
    NodeHandle node3 = ui.createNode({}, {});
    NodeHandle node4 = ui.createNode({}, {});
    NodeHandle node5 = ui.createNode({}, {});
    CORRADE_COMPARE(layer.stateData().nodes.size(), 2);
    CORRADE_COMPARE(layer.nodeName(node3), "");
    CORRADE_COMPARE(layer.nodeName(node4), "");
    CORRADE_COMPARE(layer.nodeName(node5), "");

    /* It enlarges only once setting a name of one of these */
    layer.setNodeName(node4, "Fourth noad");
    CORRADE_COMPARE(layer.stateData().nodes.size(), 4);
    CORRADE_COMPARE(layer.nodeName(node4), "Fourth noad");

    /* Update doesn't clear the layer names */
    ui.update();
    CORRADE_COMPARE(layer.nodeName(node2), "Node no.2");
    CORRADE_COMPARE(layer.nodeName(node4), "Fourth noad");

    /* Setting a global string keeps a reference to it, local or
       non-null-terminated string is copied */
    Containers::StringView global = "Global"_s;
    layer.setNodeName(node1, global);
    CORRADE_COMPARE(layer.nodeName(node1), "Global");
    CORRADE_COMPARE(layer.nodeName(node1).data(), global.data());
    CORRADE_COMPARE(layer.nodeName(node1).flags(), Containers::StringViewFlag::Global|Containers::StringViewFlag::NullTerminated);

    Containers::StringView globalNonNullTerminated = "Global non null!"_s.exceptSuffix(1);
    layer.setNodeName(node3, globalNonNullTerminated);
    CORRADE_COMPARE(layer.nodeName(node3), "Global non null");
    CORRADE_VERIFY(layer.nodeName(node3).data() != globalNonNullTerminated.data());
    CORRADE_COMPARE(layer.nodeName(node3).flags(), Containers::StringViewFlag::NullTerminated);

    Containers::StringView local = "Local";
    layer.setNodeName(node5, local);
    CORRADE_COMPARE(layer.nodeName(node5), "Local");
    CORRADE_VERIFY(layer.nodeName(node5).data() != local.data());
    CORRADE_COMPARE(layer.nodeName(node5).flags(), Containers::StringViewFlag::NullTerminated);

    /* Removing a node makes the old name still available with the old
       handle */
    ui.removeNode(node4);
    CORRADE_COMPARE(layer.nodeName(node4), "Fourth noad");

    /* When creating a new node in the same slot, the new node doesn't have a
       name yet and the old still keeps it */
    NodeHandle node4Replacement = ui.createNode({}, {});
    CORRADE_COMPARE(nodeHandleId(node4Replacement), nodeHandleId(node4));
    CORRADE_COMPARE(layer.nodeName(node4), "Fourth noad");
    CORRADE_COMPARE(layer.nodeName(node4Replacement), "");

    /* Setting a name for the replacement node makes the old one unknown */
    layer.setNodeName(node4Replacement, "Replacement");
    CORRADE_COMPARE(layer.nodeName(node4), "");
    CORRADE_COMPARE(layer.nodeName(node4Replacement), "Replacement");

    /* Updating after removing a layer and creating a new one in the same slot
       forgets the name -- the handle gets updated internally, so it cannot
       keep the name */
    ui.removeNode(node4Replacement);
    NodeHandle node4Replacement2 = ui.createNode({}, {});
    CORRADE_COMPARE(nodeHandleId(node4Replacement2), nodeHandleId(node4Replacement));
    ui.update();
    CORRADE_COMPARE(layer.nodeName(node4Replacement), "");
    CORRADE_COMPARE(layer.nodeName(node4Replacement2), "");

    /* Updating after removing a node forgets the name as well */
    layer.setNodeName(node4Replacement2, "Replacement 2");
    CORRADE_COMPARE(layer.nodeName(node4Replacement2), "Replacement 2");
    ui.removeNode(node4Replacement2);
    ui.update();
    CORRADE_COMPARE(layer.nodeName(node4Replacement2), "");
}

void DebugLayerTest::nodeNameInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui{{100, 100}};
    AbstractUserInterface uiAnother{{100, 100}};

    DebugLayer& layer = ui.setLayerInstance(Containers::pointer<DebugLayer>(ui.createLayer(), DebugLayerSource::Nodes, DebugLayerFlags{}));
    DebugLayer layerNoUi{layerHandle(0, 1), {}, {}};

    Containers::String out;
    Error redirectError{&out};
    layerNoUi.nodeName({});
    layerNoUi.setNodeName({}, {});
    layer.nodeName(NodeHandle::Null);
    layer.setNodeName(NodeHandle::Null, {});
    CORRADE_COMPARE_AS(out,
        "Ui::DebugLayer::nodeName(): layer not part of a user interface\n"
        "Ui::DebugLayer::setNodeName(): layer not part of a user interface\n"
        "Ui::DebugLayer::nodeName(): handle is null\n"
        "Ui::DebugLayer::setNodeName(): handle is null\n",
        TestSuite::Compare::String);
}

int debugIntegrationConstructed = 0;
int debugIntegrationCopied = 0;
int debugIntegrationMoved = 0;
int debugIntegrationDestructed = 0;

void DebugLayerTest::layerLayouterAnimatorNameDebugIntegrationSetup() {
    debugIntegrationConstructed =
        debugIntegrationCopied =
            debugIntegrationMoved =
                debugIntegrationDestructed = 0;
}

void DebugLayerTest::layerLayouterAnimatorNameDebugIntegrationTeardown() {
    debugIntegrationConstructed =
        debugIntegrationCopied =
            debugIntegrationMoved =
                debugIntegrationDestructed = 0;
}

void DebugLayerTest::layerNameNoOp() {
    /* If Layers aren't enabled, the APIs don't assert but just don't do
       anything */

    AbstractUserInterface ui{{100, 100}};

    struct EmptyLayer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    };
    EmptyLayer& emptyLayer = ui.setLayerInstance(Containers::pointer<EmptyLayer>(ui.createLayer()));
    /* Picking a source that isn't Layers but also isn't just empty */
    DebugLayer& layer = ui.setLayerInstance(Containers::pointer<DebugLayer>(ui.createLayer(), DebugLayerSource::NodeHierarchy, DebugLayerFlags{}));
    CORRADE_COMPARE(layer.layerName(emptyLayer.handle()), "");
    /* Not even the debug layer itself is named */
    CORRADE_COMPARE(layer.layerName(layer.handle()), "");

    /* Setting a name doesn't remember anything */
    layer.setLayerName(emptyLayer, "Empty");
    CORRADE_COMPARE(layer.layerName(emptyLayer.handle()), "");
}

void DebugLayerTest::layerName() {
    AbstractUserInterface ui{{100, 100}};

    struct EmptyLayer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    };

    struct Layer: DebugLayer {
        using DebugLayer::DebugLayer;

        const DebugLayer::State& stateData() {
            return *_state;
        }
    };

    EmptyLayer& emptyLayer1 = ui.setLayerInstance(Containers::pointer<EmptyLayer>(ui.createLayer()));
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), DebugLayerSource::Layers, DebugLayerFlags{}));
    EmptyLayer& emptyLayer2 = ui.setLayerInstance(Containers::pointer<EmptyLayer>(ui.createLayer()));

    /* Initially the debug layer has only as many entries to store its own
       name, not for all */
    CORRADE_COMPARE(layer.stateData().layers.size(), 2);

    /* By default, any layer has the name empty, just the debug layer itself
       has it set, and there it's a global string. The empty names have no
       null-terminated or global flags guaranteed */
    CORRADE_COMPARE(layer.layerName(emptyLayer1.handle()), "");
    CORRADE_COMPARE(layer.layerName(layer.handle()), "Debug");
    CORRADE_COMPARE(layer.layerName(layer.handle()).flags(), Containers::StringViewFlag::Global|Containers::StringViewFlag::NullTerminated);
    CORRADE_COMPARE(layer.layerName(emptyLayer2.handle()), "");

    /* Setting a name of a known layer updates it */
    layer.setLayerName(emptyLayer1, "First empty");
    CORRADE_COMPARE(layer.layerName(emptyLayer1.handle()), "First empty");

    /* A layer outside of any existing bounds will have an empty name as well;
       a layer with known ID but wrong generation also, no null-terminated or
       global flags guaranteed in this case either */
    CORRADE_COMPARE(layer.layerName(layerHandle(255, 1)), "");
    CORRADE_COMPARE(layer.layerName(layerHandle(layerHandleId(emptyLayer1.handle()), layerHandleGeneration(emptyLayer1.handle()) + 1)), "");

    /* Create more layers, their names are empty again, and the size of the
       internal storage doesn't update implicitly to fit those */
    EmptyLayer& emptyLayer3 = ui.setLayerInstance(Containers::pointer<EmptyLayer>(ui.createLayer()));
    EmptyLayer& emptyLayer4 = ui.setLayerInstance(Containers::pointer<EmptyLayer>(ui.createLayer()));
    EmptyLayer& emptyLayer5 = ui.setLayerInstance(Containers::pointer<EmptyLayer>(ui.createLayer()));
    CORRADE_COMPARE(layer.stateData().layers.size(), 2);
    CORRADE_COMPARE(layer.layerName(emptyLayer3.handle()), "");
    CORRADE_COMPARE(layer.layerName(emptyLayer4.handle()), "");
    CORRADE_COMPARE(layer.layerName(emptyLayer5.handle()), "");

    /* It enlarges only once setting a name of one of these */
    layer.setLayerName(emptyLayer4, "Fourth");
    CORRADE_COMPARE(layer.stateData().layers.size(), 5);
    CORRADE_COMPARE(layer.layerName(emptyLayer4.handle()), "Fourth");

    /* Update doesn't clear the layer names */
    ui.update();
    CORRADE_COMPARE(layer.layerName(emptyLayer1.handle()), "First empty");
    CORRADE_COMPARE(layer.layerName(emptyLayer4.handle()), "Fourth");

    /* Setting a global string keeps a reference to it, local or
       non-null-terminated string is copied */
    Containers::StringView global = "Global"_s;
    layer.setLayerName(emptyLayer2, global);
    CORRADE_COMPARE(layer.layerName(emptyLayer2.handle()), "Global");
    CORRADE_COMPARE(layer.layerName(emptyLayer2.handle()).data(), global.data());
    CORRADE_COMPARE(layer.layerName(emptyLayer2.handle()).flags(), Containers::StringViewFlag::Global|Containers::StringViewFlag::NullTerminated);

    Containers::StringView globalNonNullTerminated = "Global non null!"_s.exceptSuffix(1);
    layer.setLayerName(emptyLayer3, globalNonNullTerminated);
    CORRADE_COMPARE(layer.layerName(emptyLayer3.handle()), "Global non null");
    CORRADE_VERIFY(layer.layerName(emptyLayer3.handle()).data() != globalNonNullTerminated.data());
    CORRADE_COMPARE(layer.layerName(emptyLayer3.handle()).flags(), Containers::StringViewFlag::NullTerminated);

    Containers::StringView local = "Local";
    layer.setLayerName(emptyLayer5, local);
    CORRADE_COMPARE(layer.layerName(emptyLayer5.handle()), "Local");
    CORRADE_VERIFY(layer.layerName(emptyLayer5.handle()).data() != local.data());
    CORRADE_COMPARE(layer.layerName(emptyLayer5.handle()).flags(), Containers::StringViewFlag::NullTerminated);

    /* Removing a layer makes the old name still available with the old
       handle */
    LayerHandle emptyLayer4Handle = emptyLayer4.handle();
    ui.removeLayer(emptyLayer4Handle);
    CORRADE_COMPARE(layer.layerName(emptyLayer4Handle), "Fourth");

    /* When creating a new layer in the same slot, the new layer doesn't have a
       name yet and the old still keeps it */
    EmptyLayer& emptyLayer4Replacement = ui.setLayerInstance(Containers::pointer<EmptyLayer>(ui.createLayer()));
    CORRADE_COMPARE(layerHandleId(emptyLayer4Replacement.handle()), layerHandleId(emptyLayer4Handle));
    CORRADE_COMPARE(layer.layerName(emptyLayer4Handle), "Fourth");
    CORRADE_COMPARE(layer.layerName(emptyLayer4Replacement.handle()), "");

    /* Setting a name for the replacement layer makes the old one unknown */
    layer.setLayerName(emptyLayer4Replacement, "Replacement");
    CORRADE_COMPARE(layer.layerName(emptyLayer4Handle), "");
    CORRADE_COMPARE(layer.layerName(emptyLayer4Replacement.handle()), "Replacement");

    /* Updating after removing a layer and creating a new one in the same slot
       forgets the name -- the handle gets updated internally, so it cannot
       keep the name */
    LayerHandle emptyLayer4ReplacementHandle = emptyLayer4Replacement.handle();
    ui.removeLayer(emptyLayer4ReplacementHandle);
    EmptyLayer& emptyLayer4Replacement2 = ui.setLayerInstance(Containers::pointer<EmptyLayer>(ui.createLayer()));
    CORRADE_COMPARE(layerHandleId(emptyLayer4Replacement2.handle()), layerHandleId(emptyLayer4ReplacementHandle));
    ui.update();
    CORRADE_COMPARE(layer.layerName(emptyLayer4ReplacementHandle), "");
    CORRADE_COMPARE(layer.layerName(emptyLayer4Replacement2.handle()), "");

    /* Updating after removing a layer forgets the name as well */
    layer.setLayerName(emptyLayer4Replacement2, "Replacement 2");
    LayerHandle emptyLayer4Replacement2Handle = emptyLayer4Replacement2.handle();
    CORRADE_COMPARE(layer.layerName(emptyLayer4Replacement2Handle), "Replacement 2");
    ui.removeLayer(emptyLayer4Replacement2Handle);
    ui.update();
    CORRADE_COMPARE(layer.layerName(emptyLayer4Replacement2Handle), "");

    /* It's possible to change the debug layer name */
    layer.setLayerName(layer, "This is a debug layer!");
    CORRADE_COMPARE(layer.layerName(layer.handle()), "This is a debug layer!");

    /* Even to an empty string, it doesn't go back to the default in that case */
    layer.setLayerName(layer, "");
    CORRADE_COMPARE(layer.layerName(layer.handle()), "");
}

void DebugLayerTest::layerNameDebugIntegration() {
    auto&& data = LayerNameDebugIntegrationData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui{{100, 100}};

    struct Layer: DebugLayer {
        using DebugLayer::DebugLayer;

        const DebugLayer::State& stateData() {
            return *_state;
        }
    };

    struct EmptyLayer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    };

    struct IntegratedLayer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        struct DebugIntegration {
            DebugIntegration() {
                ++debugIntegrationConstructed;
            }

            DebugIntegration(const DebugIntegration&) {
                ++debugIntegrationConstructed;
                ++debugIntegrationCopied;
            }

            DebugIntegration& operator=(const DebugIntegration&) {
                ++debugIntegrationCopied;
                return *this;
            }

            ~DebugIntegration() {
                ++debugIntegrationDestructed;
            }

            void print(Debug&, const IntegratedLayer&, const Containers::StringView&, LayerDataHandle) {
                CORRADE_FAIL("This shouldn't be called.");
            }
        };

        LayerFeatures doFeatures() const override { return {}; }
    };

    /* The debug layer itself has no integration as it's excluded from
       output */
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), data.sources, DebugLayerFlags{}));
    CORRADE_COMPARE(layer.stateData().layers.size(), 1);
    CORRADE_VERIFY(!layer.stateData().layers[0].integration);
    CORRADE_VERIFY(!layer.stateData().layers[0].deleter);
    CORRADE_VERIFY(!layer.stateData().layers[0].print);

    /* A layer w/o DebugIntegration doesn't have any integration */
    EmptyLayer& emptyLayer1 = ui.setLayerInstance(Containers::pointer<EmptyLayer>(ui.createLayer()));
    layer.setLayerName(emptyLayer1, "Empty layer 1");
    CORRADE_COMPARE(layer.stateData().layers.size(), 2);
    CORRADE_VERIFY(!layer.stateData().layers[1].integration);
    CORRADE_VERIFY(!layer.stateData().layers[1].deleter);
    CORRADE_VERIFY(!layer.stateData().layers[1].print);

    /* Setting a layer name with a concrete type should allocate the
       DebugIntegration instance */
    IntegratedLayer& integratedLayer1 = ui.setLayerInstance(Containers::pointer<IntegratedLayer>(ui.createLayer()));
    IntegratedLayer& integratedLayer2 = ui.setLayerInstance(Containers::pointer<IntegratedLayer>(ui.createLayer()));
    IntegratedLayer& integratedLayer3 = ui.setLayerInstance(Containers::pointer<IntegratedLayer>(ui.createLayer()));
    layer.setLayerName(integratedLayer1, "Integrated");
    layer.setLayerName(integratedLayer2, "Integrated 2");
    layer.setLayerName(integratedLayer3, "Integrated 3");
    CORRADE_COMPARE(layer.stateData().layers.size(), 5);
    CORRADE_COMPARE(layer.stateData().layers[2].name, "Integrated");
    CORRADE_COMPARE(layer.stateData().layers[3].name, "Integrated 2");
    CORRADE_COMPARE(layer.stateData().layers[4].name, "Integrated 3");
    CORRADE_COMPARE(layer.stateData().layers[2].integration, data.used);
    CORRADE_COMPARE(layer.stateData().layers[3].integration, data.used);
    CORRADE_COMPARE(layer.stateData().layers[4].integration, data.used);
    CORRADE_COMPARE(layer.stateData().layers[2].deleter, data.used);
    CORRADE_COMPARE(layer.stateData().layers[3].deleter, data.used);
    CORRADE_COMPARE(layer.stateData().layers[4].deleter, data.used);
    CORRADE_COMPARE(layer.stateData().layers[2].print, data.used);
    CORRADE_COMPARE(layer.stateData().layers[3].print, data.used);
    CORRADE_COMPARE(layer.stateData().layers[4].print, data.used);
    /* It delegates to setLayerName(const DebugIntegration&), so it makes a
       temporary instance that then gets copied. If not used, it gets only
       copied a bunch of times but not allocated. */
    CORRADE_COMPARE(debugIntegrationConstructed, data.used ? 6 : 3);
    CORRADE_COMPARE(debugIntegrationCopied, data.used ? 3 : 0);
    CORRADE_COMPARE(debugIntegrationDestructed, 3);

    /* Setting a layer name again deletes the old (if there is) and allocates a
       new one */
    layer.setLayerName(integratedLayer1, "Integrated 1");
    CORRADE_COMPARE(layer.stateData().layers.size(), 5);
    CORRADE_COMPARE(layer.stateData().layers[2].name, "Integrated 1");
    CORRADE_COMPARE(layer.stateData().layers[2].integration, data.used);
    CORRADE_COMPARE(layer.stateData().layers[2].deleter, data.used);
    CORRADE_COMPARE(layer.stateData().layers[2].print, data.used);
    CORRADE_COMPARE(debugIntegrationConstructed, data.used ? 8 : 4);
    CORRADE_COMPARE(debugIntegrationCopied, data.used ? 4 : 0);
    CORRADE_COMPARE(debugIntegrationDestructed, data.used ? 5 : 4);

    /* Adding a bunch more empty layers and setting name for the last will
       resize the internal storage, causing the integration allocation
       references to get moved, but not the instances themselves. They
       shouldn't get deleted. */
    /*EmptyLayer& emptyLayer2 =*/ ui.setLayerInstance(Containers::pointer<EmptyLayer>(ui.createLayer()));
    EmptyLayer& emptyLayer3 = ui.setLayerInstance(Containers::pointer<EmptyLayer>(ui.createLayer()));
    layer.setLayerName(emptyLayer3, "Empty 3");
    CORRADE_COMPARE(layer.stateData().layers.size(), 7);
    CORRADE_COMPARE(layer.stateData().layers[2].name, "Integrated 1");
    CORRADE_COMPARE(layer.stateData().layers[3].name, "Integrated 2");
    CORRADE_COMPARE(layer.stateData().layers[4].name, "Integrated 3");
    CORRADE_COMPARE(layer.stateData().layers[2].integration, data.used);
    CORRADE_COMPARE(layer.stateData().layers[3].integration, data.used);
    CORRADE_COMPARE(layer.stateData().layers[4].integration, data.used);
    CORRADE_COMPARE(layer.stateData().layers[2].deleter, data.used);
    CORRADE_COMPARE(layer.stateData().layers[3].deleter, data.used);
    CORRADE_COMPARE(layer.stateData().layers[4].deleter, data.used);
    CORRADE_COMPARE(layer.stateData().layers[2].print, data.used);
    CORRADE_COMPARE(layer.stateData().layers[3].print, data.used);
    CORRADE_COMPARE(layer.stateData().layers[4].print, data.used);
    CORRADE_COMPARE(debugIntegrationConstructed, data.used ? 8 : 4);
    CORRADE_COMPARE(debugIntegrationCopied, data.used ? 4 : 0);
    CORRADE_COMPARE(debugIntegrationDestructed, data.used ? 5 : 4);

    /* Setting a different name with only the base type deletes the
       integration, if there is */
    layer.setLayerName(static_cast<AbstractLayer&>(integratedLayer1), "No longer integrated 1");
    CORRADE_COMPARE(layer.stateData().layers.size(), 7);
    CORRADE_COMPARE(layer.stateData().layers[2].name, "No longer integrated 1");
    CORRADE_VERIFY(!layer.stateData().layers[2].integration);
    CORRADE_VERIFY(!layer.stateData().layers[2].deleter);
    CORRADE_VERIFY(!layer.stateData().layers[2].print);
    CORRADE_COMPARE(debugIntegrationConstructed, data.used ? 8 : 4);
    CORRADE_COMPARE(debugIntegrationCopied, data.used ? 4 : 0);
    CORRADE_COMPARE(debugIntegrationDestructed, data.used ? 6 : 4);

    /* Setting it back recreates it, if used */
    layer.setLayerName(integratedLayer1, "Integrated 1");
    CORRADE_COMPARE(layer.stateData().layers.size(), 7);
    CORRADE_COMPARE(layer.stateData().layers[2].name, "Integrated 1");
    CORRADE_COMPARE(layer.stateData().layers[2].integration, data.used);
    CORRADE_COMPARE(layer.stateData().layers[2].deleter, data.used);
    CORRADE_COMPARE(layer.stateData().layers[2].print, data.used);
    CORRADE_COMPARE(debugIntegrationConstructed, data.used ? 10 : 5);
    CORRADE_COMPARE(debugIntegrationCopied, data.used ? 5 : 0);
    CORRADE_COMPARE(debugIntegrationDestructed, data.used ? 7 : 5);

    /* Removing an integrated layer and replacing with non-integrated deletes
       the integration on next update(), if there is */
    LayerHandle integratedLayer2Handle = integratedLayer2.handle();
    ui.removeLayer(integratedLayer2Handle);
    EmptyLayer& integratedLayer2NonIntegratedReplacement = ui.setLayerInstance(Containers::pointer<EmptyLayer>(ui.createLayer()));
    CORRADE_COMPARE(layerHandleId(integratedLayer2NonIntegratedReplacement.handle()), layerHandleId(integratedLayer2Handle));
    CORRADE_COMPARE(layer.stateData().layers[3].name, "Integrated 2");
    CORRADE_COMPARE(layer.stateData().layers[3].integration, data.used);
    CORRADE_COMPARE(layer.stateData().layers[3].deleter, data.used);
    CORRADE_COMPARE(layer.stateData().layers[3].print, data.used);
    /* Not here yet ... */
    CORRADE_COMPARE(debugIntegrationConstructed, data.used ? 10 : 5);
    CORRADE_COMPARE(debugIntegrationCopied, data.used ? 5 : 0);
    CORRADE_COMPARE(debugIntegrationDestructed, data.used ? 7 : 5);

    ui.update();
    CORRADE_COMPARE(layer.stateData().layers.size(), 7);
    CORRADE_COMPARE(layer.stateData().layers[3].name, "");
    CORRADE_VERIFY(!layer.stateData().layers[3].integration);
    CORRADE_VERIFY(!layer.stateData().layers[3].deleter);
    CORRADE_VERIFY(!layer.stateData().layers[3].print);
    /* ... but here */
    CORRADE_COMPARE(debugIntegrationConstructed, data.used ? 10 : 5);
    CORRADE_COMPARE(debugIntegrationCopied, data.used ? 5 : 0);
    CORRADE_COMPARE(debugIntegrationDestructed, data.used ? 8 : 5);

    /* Removing an integrated layer w/o replacing deletes the integration on
       next update() as well, if there is */
    ui.removeLayer(integratedLayer3.handle());
    CORRADE_COMPARE(layer.stateData().layers[4].name, "Integrated 3");
    CORRADE_COMPARE(layer.stateData().layers[4].integration, data.used);
    CORRADE_COMPARE(layer.stateData().layers[4].deleter, data.used);
    CORRADE_COMPARE(layer.stateData().layers[4].print, data.used);
    /* Not here yet ... */
    CORRADE_COMPARE(debugIntegrationConstructed, data.used ? 10 : 5);
    CORRADE_COMPARE(debugIntegrationCopied, data.used ? 5 : 0);
    CORRADE_COMPARE(debugIntegrationDestructed, data.used ? 8 : 5);

    ui.update();
    CORRADE_COMPARE(layer.stateData().layers.size(), 7);
    CORRADE_COMPARE(layer.stateData().layers[4].name, "");
    CORRADE_VERIFY(!layer.stateData().layers[4].integration);
    CORRADE_VERIFY(!layer.stateData().layers[4].deleter);
    CORRADE_VERIFY(!layer.stateData().layers[4].print);
    /* ... but here */
    CORRADE_COMPARE(debugIntegrationConstructed, data.used ? 10 : 5);
    CORRADE_COMPARE(debugIntegrationCopied, data.used ? 5 : 0);
    CORRADE_COMPARE(debugIntegrationDestructed, data.used ? 9 : 5);

    /* Removing the whole debug layer deletes the remaining integration, if
       there is */
    ui.removeLayer(layer.handle());
    CORRADE_COMPARE(debugIntegrationConstructed, data.used ? 10 : 5);
    CORRADE_COMPARE(debugIntegrationCopied, data.used ? 5 : 0);
    CORRADE_COMPARE(debugIntegrationDestructed, data.used ? 10 : 5);
}

void DebugLayerTest::layerNameDebugIntegrationExplicit() {
    auto&& data = LayerNameDebugIntegrationData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* A subset of layerNameDebugIntegration() but with a DebugIntegration
       that only has a non-default constructor and gets copied */

    AbstractUserInterface ui{{100, 100}};

    struct Layer: DebugLayer {
        using DebugLayer::DebugLayer;

        const DebugLayer::State& stateData() {
            return *_state;
        }
    };
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), data.sources, DebugLayerFlags{}));

    struct EmptyLayer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    };

    struct IntegratedLayer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        struct DebugIntegration {
            DebugIntegration(int value, float): value{value} {
                ++debugIntegrationConstructed;
            }

            DebugIntegration(const DebugIntegration& other): value{other.value} {
                ++debugIntegrationConstructed;
                ++debugIntegrationCopied;
            }

            /* This one shouldn't get used */
            DebugIntegration& operator=(const DebugIntegration& other) = delete;

            ~DebugIntegration() {
                ++debugIntegrationDestructed;
            }

            /* Compared to layerNameDebugIntegration(), here the signature does
               match */
            void print(Debug&, const IntegratedLayer&, const Containers::StringView&, LayerDataHandle) {
                CORRADE_FAIL("This shouldn't be called.");
            }

            int value;
        };

        LayerFeatures doFeatures() const override { return {}; }
    };

    /* Setting a layer name with a concrete type won't allocate the
       DebugIntegration instance as it doesn't have a default constructor.
       Which isn't great, but if the DebugIntegration can be used in a default
       setup, it should have a default constructor, and if it doesn't, then
       allowing to treat the layer as generic is better than failing to set a
       name at all. */
    IntegratedLayer& integratedLayer1 = ui.setLayerInstance(Containers::pointer<IntegratedLayer>(ui.createLayer()));
    layer.setLayerName(integratedLayer1, "Integrated 1");
    CORRADE_COMPARE(layer.stateData().layers.size(), 2);
    CORRADE_COMPARE(layer.stateData().layers[1].name, "Integrated 1");
    CORRADE_VERIFY(!layer.stateData().layers[1].integration);
    CORRADE_VERIFY(!layer.stateData().layers[1].deleter);
    CORRADE_VERIFY(!layer.stateData().layers[1].print);
    CORRADE_COMPARE(debugIntegrationConstructed, 0);
    CORRADE_COMPARE(debugIntegrationCopied, 0);
    CORRADE_COMPARE(debugIntegrationDestructed, 0);

    IntegratedLayer& integratedLayer2 = ui.setLayerInstance(Containers::pointer<IntegratedLayer>(ui.createLayer()));
    {
        IntegratedLayer::DebugIntegration integration{1337, 4.5f};
        layer.setLayerName(integratedLayer2, "Integrated 2", integration);
        CORRADE_COMPARE(layer.stateData().layers.size(), 3);
        CORRADE_COMPARE(layer.stateData().layers[2].name, "Integrated 2");
        CORRADE_COMPARE(layer.stateData().layers[2].integration, data.used);
        if(data.used)
            CORRADE_COMPARE(static_cast<IntegratedLayer::DebugIntegration*>(layer.stateData().layers[2].integration)->value, 1337);
        CORRADE_COMPARE(layer.stateData().layers[2].deleter, data.used);
        CORRADE_COMPARE(layer.stateData().layers[2].print, data.used);
    }
    /* A local instance gets constructed, copied to the function, then
       internally moved to allocate the instance (which calls the copy
       constructor again) and then both temporaries get destructed. If not
       used, the final allocation isn't made. */
    CORRADE_COMPARE(debugIntegrationConstructed, data.used ? 3 : 2);
    CORRADE_COMPARE(debugIntegrationCopied, data.used ? 2 : 1);
    CORRADE_COMPARE(debugIntegrationDestructed, 2);

    /* Removing the whole debug layer deletes the integration in this case as
       well, if there is */
    ui.removeLayer(layer.handle());
    CORRADE_COMPARE(debugIntegrationConstructed, data.used ? 3 : 2);
    CORRADE_COMPARE(debugIntegrationCopied, data.used ? 2 : 1);
    CORRADE_COMPARE(debugIntegrationDestructed, data.used ? 3 : 2);
}

void DebugLayerTest::layerNameDebugIntegrationExplicitRvalue() {
    auto&& data = LayerNameDebugIntegrationData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* A subset of layerNameDebugIntegration() but with a DebugIntegration
       that only has a non-default constructor and gets moved */

    AbstractUserInterface ui{{100, 100}};

    struct Layer: DebugLayer {
        using DebugLayer::DebugLayer;

        const DebugLayer::State& stateData() {
            return *_state;
        }
    };
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), data.sources, DebugLayerFlags{}));

    struct EmptyLayer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    };

    struct IntegratedLayer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        struct DebugIntegration {
            DebugIntegration(int value, float): value{value} {
                ++debugIntegrationConstructed;
            }

            DebugIntegration(const DebugIntegration& other) = delete;
            DebugIntegration(DebugIntegration&& other): value{other.value} {
                ++debugIntegrationConstructed;
                ++debugIntegrationMoved;
            }

            DebugIntegration& operator=(const DebugIntegration& other) = delete;
            /* This one shouldn't get used */
            DebugIntegration& operator=(DebugIntegration&& other) = delete;

            ~DebugIntegration() {
                ++debugIntegrationDestructed;
            }

            void print(Debug&, const IntegratedLayer&, const Containers::StringView&, LayerDataHandle) {
                CORRADE_FAIL("This shouldn't be called.");
            }

            int value;
        };

        LayerFeatures doFeatures() const override { return {}; }
    };

    /* Setting a layer name with a concrete type won't allocate the
       DebugIntegration instance, same reasoning as in
       layerNameDebugIntegrationExplicit() */
    IntegratedLayer& integratedLayer1 = ui.setLayerInstance(Containers::pointer<IntegratedLayer>(ui.createLayer()));
    layer.setLayerName(integratedLayer1, "Integrated 1");
    CORRADE_COMPARE(layer.stateData().layers.size(), 2);
    CORRADE_COMPARE(layer.stateData().layers[1].name, "Integrated 1");
    CORRADE_VERIFY(!layer.stateData().layers[1].integration);
    CORRADE_VERIFY(!layer.stateData().layers[1].deleter);
    CORRADE_VERIFY(!layer.stateData().layers[1].print);
    CORRADE_COMPARE(debugIntegrationConstructed, 0);
    CORRADE_COMPARE(debugIntegrationMoved, 0);
    CORRADE_COMPARE(debugIntegrationDestructed, 0);

    IntegratedLayer& integratedLayer2 = ui.setLayerInstance(Containers::pointer<IntegratedLayer>(ui.createLayer()));
    layer.setLayerName(integratedLayer2, "Integrated 2", IntegratedLayer::DebugIntegration{1337, 4.5f});
    CORRADE_COMPARE(layer.stateData().layers.size(), 3);
    CORRADE_COMPARE(layer.stateData().layers[2].name, "Integrated 2");
    CORRADE_COMPARE(layer.stateData().layers[2].integration, data.used);
    if(data.used)
        CORRADE_COMPARE(static_cast<IntegratedLayer::DebugIntegration*>(layer.stateData().layers[2].integration)->value, 1337);
    CORRADE_COMPARE(layer.stateData().layers[2].deleter, data.used);
    CORRADE_COMPARE(layer.stateData().layers[2].print, data.used);
    /* A local instance gets moved to the function, then internally moved again
       to allocate the instance and then the temporary get destructed */
    CORRADE_COMPARE(debugIntegrationConstructed, data.used ? 2 : 1);
    CORRADE_COMPARE(debugIntegrationMoved, data.used ? 1 : 0);
    CORRADE_COMPARE(debugIntegrationDestructed, 1);

    /* Removing the whole debug layer deletes the integration in this case as
       well */
    ui.removeLayer(layer.handle());
    CORRADE_COMPARE(debugIntegrationConstructed, data.used ? 2 : 1);
    CORRADE_COMPARE(debugIntegrationMoved, data.used ? 1 : 0);
    CORRADE_COMPARE(debugIntegrationDestructed, data.used ? 2 : 1);
}

void DebugLayerTest::layerNameDebugIntegrationCopyConstructPlainStruct() {
    AbstractUserInterface ui{{100, 100}};

    struct Layer: DebugLayer {
        using DebugLayer::DebugLayer;

        const DebugLayer::State& stateData() {
            return *_state;
        }
    };
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), DebugLayerSource::NodeDataDetails, DebugLayerFlags{}));

    struct IntegratedLayer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        struct DebugIntegration {
            void print(Debug&, const IntegratedLayer&, Containers::StringView, LayerDataHandle) {}

            int a;
            char b;
        };

        LayerFeatures doFeatures() const override { return {}; }
    };
    IntegratedLayer& integratedLayer = ui.setLayerInstance(Containers::pointer<IntegratedLayer>(ui.createLayer()));

    /* This needs special handling on GCC 4.8, where
       new DebugIntegration{integration} (copy-construction) attempts to
       convert DebugIntegration to int to initialize the first member and
       fails miserably. Similar case is in Containers::Array etc. */
    IntegratedLayer::DebugIntegration integration;
    layer.setLayerName(integratedLayer, "Extremely Trivial", integration);
    CORRADE_COMPARE(layer.stateData().layers.size(), 2);
    CORRADE_VERIFY(layer.stateData().layers[1].integration);
}

void DebugLayerTest::layerNameDebugIntegrationMoveConstructPlainStruct() {
    AbstractUserInterface ui{{100, 100}};

    struct Layer: DebugLayer {
        using DebugLayer::DebugLayer;

        const DebugLayer::State& stateData() {
            return *_state;
        }
    };
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), DebugLayerSource::NodeDataDetails, DebugLayerFlags{}));

    struct IntegratedLayer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        struct DebugIntegration {
            void print(Debug&, const IntegratedLayer&, Containers::StringView, LayerDataHandle) {}

            int a;
            Containers::Pointer<char> b;
        };

        LayerFeatures doFeatures() const override { return {}; }
    };
    IntegratedLayer& integratedLayer1 = ui.setLayerInstance(Containers::pointer<IntegratedLayer>(ui.createLayer()));
    IntegratedLayer& integratedLayer2 = ui.setLayerInstance(Containers::pointer<IntegratedLayer>(ui.createLayer()));

    /* This needs special handling on GCC 4.8, where
       new DebugIntegration{Utility::move(integration)} attempts to convert
       DebugIntegration to int to initialize the first member and fails
       miserably. Similar case is in Containers::Array etc. */
    layer.setLayerName(integratedLayer1, "Extremely Trivial", IntegratedLayer::DebugIntegration{});
    /* This case internally does the above, so verify it works there as well */
    layer.setLayerName(integratedLayer2, "Extremely Trivial");
    CORRADE_COMPARE(layer.stateData().layers.size(), 3);
    CORRADE_VERIFY(layer.stateData().layers[1].integration);
    CORRADE_VERIFY(layer.stateData().layers[2].integration);
}

void DebugLayerTest::layerNameInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui{{100, 100}};
    AbstractUserInterface uiAnother{{100, 100}};

    /* Enabling NodeDataDetails so the integration is used in full, just in
       case */
    DebugLayer& layer = ui.setLayerInstance(Containers::pointer<DebugLayer>(ui.createLayer(), DebugLayerSource::NodeDataDetails, DebugLayerFlags{}));
    DebugLayer layerNoUi{layerHandle(0, 1), {}, {}};

    struct EmptyLayer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    };
    EmptyLayer& layerAnotherUi = uiAnother.setLayerInstance(Containers::pointer<EmptyLayer>(uiAnother.createLayer()));
    EmptyLayer layerArtificialHandle{layerHandle(0xab, 0x12)};

    struct IntegratedLayer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        struct DebugIntegration {
            void print(Debug&, const IntegratedLayer&, const Containers::StringView&, LayerDataHandle) {
                CORRADE_FAIL("This shouldn't be called.");
            }
        };

        LayerFeatures doFeatures() const override { return {}; }
    };
    IntegratedLayer& integratedLayer = ui.setLayerInstance(Containers::pointer<IntegratedLayer>(ui.createLayer()));
    IntegratedLayer& integratedLayerAnotherUi = uiAnother.setLayerInstance(Containers::pointer<IntegratedLayer>(uiAnother.createLayer()));
    IntegratedLayer::DebugIntegration integration;

    Containers::String out;
    Error redirectError{&out};
    layerNoUi.layerName(LayerHandle::Null);
    layerNoUi.setLayerName(layer, {});
    layerNoUi.setLayerName(integratedLayer, {});
    layerNoUi.setLayerName(integratedLayer, {}, integration);
    layerNoUi.setLayerName(integratedLayer, {}, IntegratedLayer::DebugIntegration{});
    layer.layerName(LayerHandle::Null);
    layer.setLayerName(layerAnotherUi, {});
    layer.setLayerName(layerArtificialHandle, {});
    layer.setLayerName(integratedLayerAnotherUi, {});
    layer.setLayerName(integratedLayerAnotherUi, {}, integration);
    layer.setLayerName(integratedLayerAnotherUi, {}, IntegratedLayer::DebugIntegration{});
    CORRADE_COMPARE_AS(out,
        "Ui::DebugLayer::layerName(): debug layer not part of a user interface\n"
        "Ui::DebugLayer::setLayerName(): debug layer not part of a user interface\n"
        "Ui::DebugLayer::setLayerName(): debug layer not part of a user interface\n"
        "Ui::DebugLayer::setLayerName(): debug layer not part of a user interface\n"
        "Ui::DebugLayer::setLayerName(): debug layer not part of a user interface\n"
        "Ui::DebugLayer::layerName(): handle is null\n"
        "Ui::DebugLayer::setLayerName(): layer not part of the same user interface\n"
        "Ui::DebugLayer::setLayerName(): layer not part of the same user interface\n"
        "Ui::DebugLayer::setLayerName(): layer not part of the same user interface\n"
        "Ui::DebugLayer::setLayerName(): layer not part of the same user interface\n"
        "Ui::DebugLayer::setLayerName(): layer not part of the same user interface\n",
        TestSuite::Compare::String);
}

void DebugLayerTest::layouterNameNoOp() {
    /* If Layouters aren't enabled, the APIs don't assert but just don't do
       anything */

    AbstractUserInterface ui{{100, 100}};

    struct EmptyLayouter: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;

        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&) override {}
    };
    EmptyLayouter& emptyLayouter = ui.setLayouterInstance(Containers::pointer<EmptyLayouter>(ui.createLayouter()));
    /* Picking a source that isn't Layouters but also isn't just empty */
    DebugLayer& layer = ui.setLayerInstance(Containers::pointer<DebugLayer>(ui.createLayer(), DebugLayerSource::NodeHierarchy, DebugLayerFlags{}));
    CORRADE_COMPARE(layer.layouterName(emptyLayouter.handle()), "");

    /* Setting a name doesn't remember anything */
    layer.setLayouterName(emptyLayouter, "Empty");
    CORRADE_COMPARE(layer.layouterName(emptyLayouter.handle()), "");
}

void DebugLayerTest::layouterName() {
    AbstractUserInterface ui{{100, 100}};

    struct EmptyLayouter: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;

        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&) override {}
    };

    struct Layer: DebugLayer {
        using DebugLayer::DebugLayer;

        const DebugLayer::State& stateData() {
            return *_state;
        }
    };

    EmptyLayouter& emptyLayouter1 = ui.setLayouterInstance(Containers::pointer<EmptyLayouter>(ui.createLayouter()));
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), DebugLayerSource::Layouters, DebugLayerFlags{}));
    EmptyLayouter& emptyLayouter2 = ui.setLayouterInstance(Containers::pointer<EmptyLayouter>(ui.createLayouter()));

    /* Initially the debug layer has no layouter entries */
    CORRADE_COMPARE(layer.stateData().layouters.size(), 0);

    /* By default, any animator has the name empty. The empty names have no
       null-terminated or global flags guaranteed. */
    CORRADE_COMPARE(layer.layouterName(emptyLayouter1.handle()), "");
    CORRADE_COMPARE(layer.layouterName(emptyLayouter2.handle()), "");

    /* Setting a name of the first animator enlarges the array to fit it.
       Compared to layers, which are resized to contain at least the debug
       layer itself, the array is empty initially so there's no pre-existing
       entry to update. */
    layer.setLayouterName(emptyLayouter1, "First empty");
    CORRADE_COMPARE(layer.stateData().layouters.size(), 1);
    CORRADE_COMPARE(layer.layouterName(emptyLayouter1.handle()), "First empty");

    /* A layouter outside of any existing bounds will have an empty name as
       well; a layer with known ID but wrong generation also, no
       null-terminated or global flags guaranteed in this case either */
    CORRADE_COMPARE(layer.layouterName(layouterHandle(255, 1)), "");
    CORRADE_COMPARE(layer.layouterName(layouterHandle(layouterHandleId(emptyLayouter1.handle()), layouterHandleGeneration(emptyLayouter1.handle()) + 1)), "");

    /* Create more layouters, their names are empty again, and the size of the
       internal storage doesn't update implicitly to fit those */
    EmptyLayouter& emptyLayouter3 = ui.setLayouterInstance(Containers::pointer<EmptyLayouter>(ui.createLayouter()));
    EmptyLayouter& emptyLayouter4 = ui.setLayouterInstance(Containers::pointer<EmptyLayouter>(ui.createLayouter()));
    EmptyLayouter& emptyLayouter5 = ui.setLayouterInstance(Containers::pointer<EmptyLayouter>(ui.createLayouter()));
    CORRADE_COMPARE(layer.stateData().layouters.size(), 1);
    CORRADE_COMPARE(layer.layouterName(emptyLayouter3.handle()), "");
    CORRADE_COMPARE(layer.layouterName(emptyLayouter4.handle()), "");
    CORRADE_COMPARE(layer.layouterName(emptyLayouter5.handle()), "");

    /* It enlarges only once setting a name of one of these */
    layer.setLayouterName(emptyLayouter4, "Fourth");
    CORRADE_COMPARE(layer.stateData().layouters.size(), 4);
    CORRADE_COMPARE(layer.layouterName(emptyLayouter4.handle()), "Fourth");

    /* Update doesn't clear the layouter names */
    ui.update();
    CORRADE_COMPARE(layer.layouterName(emptyLayouter1.handle()), "First empty");
    CORRADE_COMPARE(layer.layouterName(emptyLayouter4.handle()), "Fourth");

    /* Setting a global string keeps a reference to it, local or
       non-null-terminated string is copied */
    Containers::StringView global = "Global"_s;
    layer.setLayouterName(emptyLayouter2, global);
    CORRADE_COMPARE(layer.layouterName(emptyLayouter2.handle()), "Global");
    CORRADE_COMPARE(layer.layouterName(emptyLayouter2.handle()).data(), global.data());
    CORRADE_COMPARE(layer.layouterName(emptyLayouter2.handle()).flags(), Containers::StringViewFlag::Global|Containers::StringViewFlag::NullTerminated);

    Containers::StringView globalNonNullTerminated = "Global non null!"_s.exceptSuffix(1);
    layer.setLayouterName(emptyLayouter3, globalNonNullTerminated);
    CORRADE_COMPARE(layer.layouterName(emptyLayouter3.handle()), "Global non null");
    CORRADE_VERIFY(layer.layouterName(emptyLayouter3.handle()).data() != globalNonNullTerminated.data());
    CORRADE_COMPARE(layer.layouterName(emptyLayouter3.handle()).flags(), Containers::StringViewFlag::NullTerminated);

    Containers::StringView local = "Local";
    layer.setLayouterName(emptyLayouter5, local);
    CORRADE_COMPARE(layer.layouterName(emptyLayouter5.handle()), "Local");
    CORRADE_VERIFY(layer.layouterName(emptyLayouter5.handle()).data() != local.data());
    CORRADE_COMPARE(layer.layouterName(emptyLayouter5.handle()).flags(), Containers::StringViewFlag::NullTerminated);

    /* Removing a layouter makes the old name still available with the old
       handle */
    LayouterHandle emptyLayouter4Handle = emptyLayouter4.handle();
    ui.removeLayouter(emptyLayouter4Handle);
    CORRADE_COMPARE(layer.layouterName(emptyLayouter4Handle), "Fourth");

    /* When creating a new layouter in the same slot, the new layouter doesn't
       have a name yet and the old still keeps it */
    EmptyLayouter& emptyLayouter4Replacement = ui.setLayouterInstance(Containers::pointer<EmptyLayouter>(ui.createLayouter()));
    CORRADE_COMPARE(layouterHandleId(emptyLayouter4Replacement.handle()), layouterHandleId(emptyLayouter4Handle));
    CORRADE_COMPARE(layer.layouterName(emptyLayouter4Handle), "Fourth");
    CORRADE_COMPARE(layer.layouterName(emptyLayouter4Replacement.handle()), "");

    /* Setting a name for the replacement layouter makes the old one unknown */
    layer.setLayouterName(emptyLayouter4Replacement, "Replacement");
    CORRADE_COMPARE(layer.layouterName(emptyLayouter4Handle), "");
    CORRADE_COMPARE(layer.layouterName(emptyLayouter4Replacement.handle()), "Replacement");

    /* Updating after removing a layouter and creating a new one in the same
       slot forgets the name -- the handle gets updated internally, so it
       cannot keep the name */
    LayouterHandle emptyLayouter4ReplacementHandle = emptyLayouter4Replacement.handle();
    ui.removeLayouter(emptyLayouter4ReplacementHandle);
    EmptyLayouter& emptyLayouter4Replacement2 = ui.setLayouterInstance(Containers::pointer<EmptyLayouter>(ui.createLayouter()));
    CORRADE_COMPARE(layouterHandleId(emptyLayouter4Replacement2.handle()), layouterHandleId(emptyLayouter4ReplacementHandle));
    ui.update();
    CORRADE_COMPARE(layer.layouterName(emptyLayouter4ReplacementHandle), "");
    CORRADE_COMPARE(layer.layouterName(emptyLayouter4Replacement2.handle()), "");

    /* Updating after removing a layouter forgets the name as well */
    layer.setLayouterName(emptyLayouter4Replacement2, "Replacement 2");
    LayouterHandle emptyLayouter4Replacement2Handle = emptyLayouter4Replacement2.handle();
    CORRADE_COMPARE(layer.layouterName(emptyLayouter4Replacement2Handle), "Replacement 2");
    ui.removeLayouter(emptyLayouter4Replacement2Handle);
    ui.update();
    CORRADE_COMPARE(layer.layouterName(emptyLayouter4Replacement2Handle), "");
}

void DebugLayerTest::layouterNameDebugIntegration() {
    auto&& data = LayouterNameDebugIntegrationData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui{{100, 100}};

    struct Layer: DebugLayer {
        using DebugLayer::DebugLayer;

        const DebugLayer::State& stateData() {
            return *_state;
        }
    };

    struct EmptyLayouter: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;

        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&) override {}
    };

    struct IntegratedLayouter: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;

        struct DebugIntegration {
            DebugIntegration() {
                ++debugIntegrationConstructed;
            }

            DebugIntegration(const DebugIntegration&) {
                ++debugIntegrationConstructed;
                ++debugIntegrationCopied;
            }

            DebugIntegration& operator=(const DebugIntegration&) {
                ++debugIntegrationCopied;
                return *this;
            }

            ~DebugIntegration() {
                ++debugIntegrationDestructed;
            }

            void print(Debug&, const IntegratedLayouter&, const Containers::StringView&, LayouterDataHandle) {
                CORRADE_FAIL("This shouldn't be called.");
            }
        };

        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&) override {}
    };

    /* Initially there are no layouter entries */
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), data.sources, DebugLayerFlags{}));
    CORRADE_COMPARE(layer.stateData().layouters.size(), 0);

    /* A layouter w/o DebugIntegration doesn't have any integration */
    EmptyLayouter& emptyLayouter1 = ui.setLayouterInstance(Containers::pointer<EmptyLayouter>(ui.createLayouter()));
    layer.setLayouterName(emptyLayouter1, "Empty layouter 1");
    CORRADE_COMPARE(layer.stateData().layouters.size(), 1);
    CORRADE_VERIFY(!layer.stateData().layouters[0].integration);
    CORRADE_VERIFY(!layer.stateData().layouters[0].deleter);
    CORRADE_VERIFY(!layer.stateData().layouters[0].print);

    /* Setting a layouter name with a concrete type should allocate the
       DebugIntegration instance */
    IntegratedLayouter& integratedLayouter1 = ui.setLayouterInstance(Containers::pointer<IntegratedLayouter>(ui.createLayouter()));
    IntegratedLayouter& integratedLayouter2 = ui.setLayouterInstance(Containers::pointer<IntegratedLayouter>(ui.createLayouter()));
    IntegratedLayouter& integratedLayouter3 = ui.setLayouterInstance(Containers::pointer<IntegratedLayouter>(ui.createLayouter()));
    layer.setLayouterName(integratedLayouter1, "Integrated");
    layer.setLayouterName(integratedLayouter2, "Integrated 2");
    layer.setLayouterName(integratedLayouter3, "Integrated 3");
    CORRADE_COMPARE(layer.stateData().layouters.size(), 4);
    CORRADE_COMPARE(layer.stateData().layouters[1].name, "Integrated");
    CORRADE_COMPARE(layer.stateData().layouters[2].name, "Integrated 2");
    CORRADE_COMPARE(layer.stateData().layouters[3].name, "Integrated 3");
    CORRADE_COMPARE(layer.stateData().layouters[1].integration, data.used);
    CORRADE_COMPARE(layer.stateData().layouters[2].integration, data.used);
    CORRADE_COMPARE(layer.stateData().layouters[3].integration, data.used);
    CORRADE_COMPARE(layer.stateData().layouters[1].deleter, data.used);
    CORRADE_COMPARE(layer.stateData().layouters[2].deleter, data.used);
    CORRADE_COMPARE(layer.stateData().layouters[3].deleter, data.used);
    CORRADE_COMPARE(layer.stateData().layouters[1].print, data.used);
    CORRADE_COMPARE(layer.stateData().layouters[2].print, data.used);
    CORRADE_COMPARE(layer.stateData().layouters[3].print, data.used);
    /* It delegates to setLayouterName(const DebugIntegration&), so it makes a
       temporary instance that then gets copied. If not used, it gets only
       copied a bunch of times but not allocated. */
    CORRADE_COMPARE(debugIntegrationConstructed, data.used ? 6 : 3);
    CORRADE_COMPARE(debugIntegrationCopied, data.used ? 3 : 0);
    CORRADE_COMPARE(debugIntegrationDestructed, 3);

    /* Setting a layouter name again deletes the old (if there is) and
       allocates a new one */
    layer.setLayouterName(integratedLayouter1, "Integrated 1");
    CORRADE_COMPARE(layer.stateData().layouters.size(), 4);
    CORRADE_COMPARE(layer.stateData().layouters[1].name, "Integrated 1");
    CORRADE_COMPARE(layer.stateData().layouters[1].integration, data.used);
    CORRADE_COMPARE(layer.stateData().layouters[1].deleter, data.used);
    CORRADE_COMPARE(layer.stateData().layouters[1].print, data.used);
    CORRADE_COMPARE(debugIntegrationConstructed, data.used ? 8 : 4);
    CORRADE_COMPARE(debugIntegrationCopied, data.used ? 4 : 0);
    CORRADE_COMPARE(debugIntegrationDestructed, data.used ? 5 : 4);

    /* Adding a bunch more empty layouters and setting name for the last will
       resize the internal storage, causing the integration allocation
       references to get moved, but not the instances themselves. They
       shouldn't get deleted. */
    /*EmptyLayouter& emptyLayouter2 =*/ ui.setLayouterInstance(Containers::pointer<EmptyLayouter>(ui.createLayouter()));
    EmptyLayouter& emptyLayouter3 = ui.setLayouterInstance(Containers::pointer<EmptyLayouter>(ui.createLayouter()));
    layer.setLayouterName(emptyLayouter3, "Empty 3");
    CORRADE_COMPARE(layer.stateData().layouters.size(), 6);
    CORRADE_COMPARE(layer.stateData().layouters[1].name, "Integrated 1");
    CORRADE_COMPARE(layer.stateData().layouters[2].name, "Integrated 2");
    CORRADE_COMPARE(layer.stateData().layouters[3].name, "Integrated 3");
    CORRADE_COMPARE(layer.stateData().layouters[1].integration, data.used);
    CORRADE_COMPARE(layer.stateData().layouters[2].integration, data.used);
    CORRADE_COMPARE(layer.stateData().layouters[3].integration, data.used);
    CORRADE_COMPARE(layer.stateData().layouters[1].deleter, data.used);
    CORRADE_COMPARE(layer.stateData().layouters[2].deleter, data.used);
    CORRADE_COMPARE(layer.stateData().layouters[3].deleter, data.used);
    CORRADE_COMPARE(layer.stateData().layouters[1].print, data.used);
    CORRADE_COMPARE(layer.stateData().layouters[2].print, data.used);
    CORRADE_COMPARE(layer.stateData().layouters[3].print, data.used);
    CORRADE_COMPARE(debugIntegrationConstructed, data.used ? 8 : 4);
    CORRADE_COMPARE(debugIntegrationCopied, data.used ? 4 : 0);
    CORRADE_COMPARE(debugIntegrationDestructed, data.used ? 5 : 4);

    /* Setting a different name with only the base type deletes the
       integration, if there is */
    layer.setLayouterName(static_cast<AbstractLayouter&>(integratedLayouter1), "No longer integrated 1");
    CORRADE_COMPARE(layer.stateData().layouters.size(), 6);
    CORRADE_COMPARE(layer.stateData().layouters[1].name, "No longer integrated 1");
    CORRADE_VERIFY(!layer.stateData().layouters[1].integration);
    CORRADE_VERIFY(!layer.stateData().layouters[1].deleter);
    CORRADE_VERIFY(!layer.stateData().layouters[1].print);
    CORRADE_COMPARE(debugIntegrationConstructed, data.used ? 8 : 4);
    CORRADE_COMPARE(debugIntegrationCopied, data.used ? 4 : 0);
    CORRADE_COMPARE(debugIntegrationDestructed, data.used ? 6 : 4);

    /* Setting it back recreates it, if used */
    layer.setLayouterName(integratedLayouter1, "Integrated 1");
    CORRADE_COMPARE(layer.stateData().layouters.size(), 6);
    CORRADE_COMPARE(layer.stateData().layouters[1].name, "Integrated 1");
    CORRADE_COMPARE(layer.stateData().layouters[1].integration, data.used);
    CORRADE_COMPARE(layer.stateData().layouters[1].deleter, data.used);
    CORRADE_COMPARE(layer.stateData().layouters[1].print, data.used);
    CORRADE_COMPARE(debugIntegrationConstructed, data.used ? 10 : 5);
    CORRADE_COMPARE(debugIntegrationCopied, data.used ? 5 : 0);
    CORRADE_COMPARE(debugIntegrationDestructed, data.used ? 7 : 5);

    /* Removing an integrated layouter and replacing with non-integrated
       deletes the integration on next update(), if there is */
    LayouterHandle integratedLayouter2Handle = integratedLayouter2.handle();
    ui.removeLayouter(integratedLayouter2Handle);
    EmptyLayouter& integratedLayouter2NonIntegratedReplacement = ui.setLayouterInstance(Containers::pointer<EmptyLayouter>(ui.createLayouter()));
    CORRADE_COMPARE(layouterHandleId(integratedLayouter2NonIntegratedReplacement.handle()), layouterHandleId(integratedLayouter2Handle));
    CORRADE_COMPARE(layer.stateData().layouters[2].name, "Integrated 2");
    CORRADE_COMPARE(layer.stateData().layouters[2].integration, data.used);
    CORRADE_COMPARE(layer.stateData().layouters[2].deleter, data.used);
    CORRADE_COMPARE(layer.stateData().layouters[2].print, data.used);
    /* Not here yet ... */
    CORRADE_COMPARE(debugIntegrationConstructed, data.used ? 10 : 5);
    CORRADE_COMPARE(debugIntegrationCopied, data.used ? 5 : 0);
    CORRADE_COMPARE(debugIntegrationDestructed, data.used ? 7 : 5);

    ui.update();
    CORRADE_COMPARE(layer.stateData().layouters.size(), 6);
    CORRADE_COMPARE(layer.stateData().layouters[2].name, "");
    CORRADE_VERIFY(!layer.stateData().layouters[2].integration);
    CORRADE_VERIFY(!layer.stateData().layouters[2].deleter);
    CORRADE_VERIFY(!layer.stateData().layouters[2].print);
    /* ... but here */
    CORRADE_COMPARE(debugIntegrationConstructed, data.used ? 10 : 5);
    CORRADE_COMPARE(debugIntegrationCopied, data.used ? 5 : 0);
    CORRADE_COMPARE(debugIntegrationDestructed, data.used ? 8 : 5);

    /* Removing an integrated layouter w/o replacing deletes the integration on
       next update() as well, if there is */
    ui.removeLayouter(integratedLayouter3.handle());
    CORRADE_COMPARE(layer.stateData().layouters[3].name, "Integrated 3");
    CORRADE_COMPARE(layer.stateData().layouters[3].integration, data.used);
    CORRADE_COMPARE(layer.stateData().layouters[3].deleter, data.used);
    CORRADE_COMPARE(layer.stateData().layouters[3].print, data.used);
    /* Not here yet ... */
    CORRADE_COMPARE(debugIntegrationConstructed, data.used ? 10 : 5);
    CORRADE_COMPARE(debugIntegrationCopied, data.used ? 5 : 0);
    CORRADE_COMPARE(debugIntegrationDestructed, data.used ? 8 : 5);

    ui.update();
    CORRADE_COMPARE(layer.stateData().layouters.size(), 6);
    CORRADE_COMPARE(layer.stateData().layouters[3].name, "");
    CORRADE_VERIFY(!layer.stateData().layouters[3].integration);
    CORRADE_VERIFY(!layer.stateData().layouters[3].deleter);
    CORRADE_VERIFY(!layer.stateData().layouters[3].print);
    /* ... but here */
    CORRADE_COMPARE(debugIntegrationConstructed, data.used ? 10 : 5);
    CORRADE_COMPARE(debugIntegrationCopied, data.used ? 5 : 0);
    CORRADE_COMPARE(debugIntegrationDestructed, data.used ? 9 : 5);

    /* Removing the whole debug layer deletes the remaining integration, if
       there is */
    ui.removeLayer(layer.handle());
    CORRADE_COMPARE(debugIntegrationConstructed, data.used ? 10 : 5);
    CORRADE_COMPARE(debugIntegrationCopied, data.used ? 5 : 0);
    CORRADE_COMPARE(debugIntegrationDestructed, data.used ? 10 : 5);
}

void DebugLayerTest::layouterNameDebugIntegrationExplicit() {
    auto&& data = LayouterNameDebugIntegrationData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* A subset of layouterNameDebugIntegration() but with a DebugIntegration
       that only has a non-default constructor and gets copied */

    AbstractUserInterface ui{{100, 100}};

    struct Layer: DebugLayer {
        using DebugLayer::DebugLayer;

        const DebugLayer::State& stateData() {
            return *_state;
        }
    };
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), data.sources, DebugLayerFlags{}));

    struct EmptyLayouter: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;

        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&) override {}
    };

    struct IntegratedLayouter: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;

        struct DebugIntegration {
            DebugIntegration(int value, float): value{value} {
                ++debugIntegrationConstructed;
            }

            DebugIntegration(const DebugIntegration& other): value{other.value} {
                ++debugIntegrationConstructed;
                ++debugIntegrationCopied;
            }

            /* This one shouldn't get used */
            DebugIntegration& operator=(const DebugIntegration& other) = delete;

            ~DebugIntegration() {
                ++debugIntegrationDestructed;
            }

            /* Compared to layouterNameDebugIntegration(), here the signature does
               match */
            void print(Debug&, const IntegratedLayouter&, const Containers::StringView&, LayouterDataHandle) {
                CORRADE_FAIL("This shouldn't be called.");
            }

            int value;
        };

        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&) override {}
    };

    /* Setting a layouter name with a concrete type won't allocate the
       DebugIntegration instance, same reasoning as with layers in
       layerNameDebugIntegrationExplicit() */
    IntegratedLayouter& integratedLayouter1 = ui.setLayouterInstance(Containers::pointer<IntegratedLayouter>(ui.createLayouter()));
    layer.setLayouterName(integratedLayouter1, "Integrated 1");
    CORRADE_COMPARE(layer.stateData().layouters.size(), 1);
    CORRADE_COMPARE(layer.stateData().layouters[0].name, "Integrated 1");
    CORRADE_VERIFY(!layer.stateData().layouters[0].integration);
    CORRADE_VERIFY(!layer.stateData().layouters[0].deleter);
    CORRADE_VERIFY(!layer.stateData().layouters[0].print);
    CORRADE_COMPARE(debugIntegrationConstructed, 0);
    CORRADE_COMPARE(debugIntegrationCopied, 0);
    CORRADE_COMPARE(debugIntegrationDestructed, 0);

    IntegratedLayouter& integratedLayouter2 = ui.setLayouterInstance(Containers::pointer<IntegratedLayouter>(ui.createLayouter()));
    {
        IntegratedLayouter::DebugIntegration integration{1337, 4.5f};
        layer.setLayouterName(integratedLayouter2, "Integrated 2", integration);
        CORRADE_COMPARE(layer.stateData().layouters.size(), 2);
        CORRADE_COMPARE(layer.stateData().layouters[1].name, "Integrated 2");
        CORRADE_COMPARE(layer.stateData().layouters[1].integration, data.used);
        if(data.used)
            CORRADE_COMPARE(static_cast<IntegratedLayouter::DebugIntegration*>(layer.stateData().layouters[1].integration)->value, 1337);
        CORRADE_COMPARE(layer.stateData().layouters[1].deleter, data.used);
        CORRADE_COMPARE(layer.stateData().layouters[1].print, data.used);
    }
    /* A local instance gets constructed, copied to the function,
       then internally moved to allocate the instance (which calls the copy
       constructor again) and then both temporaries get destructed. If not
       used, the final allocation isn't made. */
    CORRADE_COMPARE(debugIntegrationConstructed, data.used ? 3 : 2);
    CORRADE_COMPARE(debugIntegrationCopied, data.used ? 2 : 1);
    CORRADE_COMPARE(debugIntegrationDestructed, 2);

    /* Removing the whole debug layer deletes the integration in this case as
       well, if there is */
    ui.removeLayer(layer.handle());
    CORRADE_COMPARE(debugIntegrationConstructed, data.used ? 3 : 2);
    CORRADE_COMPARE(debugIntegrationCopied, data.used ? 2 : 1);
    CORRADE_COMPARE(debugIntegrationDestructed, data.used ? 3 : 2);
}

void DebugLayerTest::layouterNameDebugIntegrationExplicitRvalue() {
    auto&& data = LayouterNameDebugIntegrationData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* A subset of layouterNameDebugIntegration() but with a DebugIntegration
       that only has a non-default constructor and gets moved */

    AbstractUserInterface ui{{100, 100}};

    struct Layer: DebugLayer {
        using DebugLayer::DebugLayer;

        const DebugLayer::State& stateData() {
            return *_state;
        }
    };

    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), data.sources, DebugLayerFlags{}));

    struct EmptyLayouter: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;

        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&) override {}
    };

    struct IntegratedLayouter: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;

        struct DebugIntegration {
            DebugIntegration(int value, float): value{value} {
                ++debugIntegrationConstructed;
            }

            DebugIntegration(const DebugIntegration& other) = delete;
            DebugIntegration(DebugIntegration&& other): value{other.value} {
                ++debugIntegrationConstructed;
                ++debugIntegrationMoved;
            }

            DebugIntegration& operator=(const DebugIntegration& other) = delete;
            /* This one shouldn't get used */
            DebugIntegration& operator=(DebugIntegration&& other) = delete;

            ~DebugIntegration() {
                ++debugIntegrationDestructed;
            }

            void print(Debug&, const IntegratedLayouter&, const Containers::StringView&, LayouterDataHandle) {
                CORRADE_FAIL("This shouldn't be called.");
            }

            int value;
        };

        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&) override {}
    };

    /* Setting a layouter name with a concrete type won't allocate the
       DebugIntegration instance, same reasoning as with layers in
       layerNameDebugIntegrationExplicit() */
    IntegratedLayouter& integratedLayouter1 = ui.setLayouterInstance(Containers::pointer<IntegratedLayouter>(ui.createLayouter()));
    layer.setLayouterName(integratedLayouter1, "Integrated 1");
    CORRADE_COMPARE(layer.stateData().layouters.size(), 1);
    CORRADE_COMPARE(layer.stateData().layouters[0].name, "Integrated 1");
    CORRADE_VERIFY(!layer.stateData().layouters[0].integration);
    CORRADE_VERIFY(!layer.stateData().layouters[0].deleter);
    CORRADE_VERIFY(!layer.stateData().layouters[0].print);
    CORRADE_COMPARE(debugIntegrationConstructed, 0);
    CORRADE_COMPARE(debugIntegrationMoved, 0);
    CORRADE_COMPARE(debugIntegrationDestructed, 0);

    IntegratedLayouter& integratedLayouter2 = ui.setLayouterInstance(Containers::pointer<IntegratedLayouter>(ui.createLayouter()));
    layer.setLayouterName(integratedLayouter2, "Integrated 2", IntegratedLayouter::DebugIntegration{1337, 4.5f});
    CORRADE_COMPARE(layer.stateData().layouters.size(), 2);
    CORRADE_COMPARE(layer.stateData().layouters[1].name, "Integrated 2");
    CORRADE_COMPARE(layer.stateData().layouters[1].integration, data.used);
    if(data.used)
        CORRADE_COMPARE(static_cast<IntegratedLayouter::DebugIntegration*>(layer.stateData().layouters[1].integration)->value, 1337);
    CORRADE_COMPARE(layer.stateData().layouters[1].deleter, data.used);
    CORRADE_COMPARE(layer.stateData().layouters[1].print, data.used);
    /* A local instance gets moved to the function, then internally moved again
       to allocate the instance and then the temporary get destructed */
    CORRADE_COMPARE(debugIntegrationConstructed, data.used ? 2 : 1);
    CORRADE_COMPARE(debugIntegrationMoved, data.used ? 1 : 0);
    CORRADE_COMPARE(debugIntegrationDestructed, 1);

    /* Removing the whole debug layer deletes the integration in this case as
       well */
    ui.removeLayer(layer.handle());
    CORRADE_COMPARE(debugIntegrationConstructed, data.used ? 2 : 1);
    CORRADE_COMPARE(debugIntegrationMoved, data.used ? 1 : 0);
    CORRADE_COMPARE(debugIntegrationDestructed, data.used ? 2 : 1);
}

void DebugLayerTest::layouterNameDebugIntegrationCopyConstructPlainStruct() {
    /* Like layerNameDebugIntegrationCopyConstructPlainStruct() but for
       layouters */

    AbstractUserInterface ui{{100, 100}};

    struct Layer: DebugLayer {
        using DebugLayer::DebugLayer;

        const DebugLayer::State& stateData() {
            return *_state;
        }
    };

    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), DebugLayerSource::NodeLayoutDetails, DebugLayerFlags{}));

    struct IntegratedLayouter: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;

        struct DebugIntegration {
            void print(Debug&, const IntegratedLayouter&, Containers::StringView, LayouterDataHandle) {}

            int a;
            char b;
        };

        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&) override {}
    };
    IntegratedLayouter& integratedLayouter = ui.setLayouterInstance(Containers::pointer<IntegratedLayouter>(ui.createLayouter()));

    /* This needs special handling on GCC 4.8, where
       new DebugIntegration{integration} (copy-construction) attempts to
       convert DebugIntegration to int to initialize the first member and
       fails miserably. Similar case is in Containers::Array etc. */
    IntegratedLayouter::DebugIntegration integration;
    layer.setLayouterName(integratedLayouter, "Extremely Trivial", integration);
    CORRADE_COMPARE(layer.stateData().layouters.size(), 1);
    CORRADE_VERIFY(layer.stateData().layouters[0].integration);
}

void DebugLayerTest::layouterNameDebugIntegrationMoveConstructPlainStruct() {
    /* Like layerNameDebugIntegrationMoveConstructPlainStruct() but for
       layouters */

    AbstractUserInterface ui{{100, 100}};

    struct Layer: DebugLayer {
        using DebugLayer::DebugLayer;

        const DebugLayer::State& stateData() {
            return *_state;
        }
    };

    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), DebugLayerSource::NodeLayoutDetails, DebugLayerFlags{}));

    struct IntegratedLayouter: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;

        struct DebugIntegration {
            void print(Debug&, const IntegratedLayouter&, Containers::StringView, LayouterDataHandle) {}

            int a;
            Containers::Pointer<char> b;
        };

        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&) override {}
    };
    IntegratedLayouter& integratedLayouter1 = ui.setLayouterInstance(Containers::pointer<IntegratedLayouter>(ui.createLayouter()));
    IntegratedLayouter& integratedLayouter2 = ui.setLayouterInstance(Containers::pointer<IntegratedLayouter>(ui.createLayouter()));

    /* This needs special handling on GCC 4.8, where
       new DebugIntegration{Utility::move(integration)} attempts to convert
       DebugIntegration to int to initialize the first member and fails
       miserably. Similar case is in Containers::Array etc. */
    layer.setLayouterName(integratedLayouter1, "Extremely Trivial", IntegratedLayouter::DebugIntegration{});
    /* This case internally does the above, so verify it works there as well */
    layer.setLayouterName(integratedLayouter2, "Extremely Trivial");
    CORRADE_COMPARE(layer.stateData().layouters.size(), 2);
    CORRADE_VERIFY(layer.stateData().layouters[0].integration);
    CORRADE_VERIFY(layer.stateData().layouters[1].integration);
}

void DebugLayerTest::layouterNameInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui{{100, 100}};
    AbstractUserInterface uiAnother{{100, 100}};

    /* Enabling NodeLayoutDetails so the integration is used in full, just in
       case */
    DebugLayer& layer = ui.setLayerInstance(Containers::pointer<DebugLayer>(ui.createLayer(), DebugLayerSource::NodeLayoutDetails, DebugLayerFlags{}));
    DebugLayer layerNoUi{layerHandle(0, 1), {}, {}};

    struct EmptyLayouter: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;

        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&) override {}
    };
    EmptyLayouter& layouter = ui.setLayouterInstance(Containers::pointer<EmptyLayouter>(ui.createLayouter()));
    EmptyLayouter& layouterAnotherUi = uiAnother.setLayouterInstance(Containers::pointer<EmptyLayouter>(uiAnother.createLayouter()));
    EmptyLayouter layouterArtificialHandle{layouterHandle(0xab, 0x12)};

    struct IntegratedLayouter: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;

        struct DebugIntegration {
            void print(Debug&, const IntegratedLayouter&, const Containers::StringView&, LayouterDataHandle) {
                CORRADE_FAIL("This shouldn't be called.");
            }
        };

        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&) override {}
    };
    IntegratedLayouter& integratedLayouter = ui.setLayouterInstance(Containers::pointer<IntegratedLayouter>(ui.createLayouter()));
    IntegratedLayouter& integratedLayouterAnotherUi = uiAnother.setLayouterInstance(Containers::pointer<IntegratedLayouter>(uiAnother.createLayouter()));
    IntegratedLayouter::DebugIntegration integration;

    Containers::String out;
    Error redirectError{&out};
    layerNoUi.layouterName(LayouterHandle::Null);
    layerNoUi.setLayouterName(layouter, {});
    layerNoUi.setLayouterName(integratedLayouter, {});
    layerNoUi.setLayouterName(integratedLayouter, {}, integration);
    layerNoUi.setLayouterName(integratedLayouter, {}, IntegratedLayouter::DebugIntegration{});
    layer.layouterName(LayouterHandle::Null);
    layer.setLayouterName(layouterAnotherUi, {});
    layer.setLayouterName(layouterArtificialHandle, {});
    layer.setLayouterName(integratedLayouterAnotherUi, {});
    layer.setLayouterName(integratedLayouterAnotherUi, {}, integration);
    layer.setLayouterName(integratedLayouterAnotherUi, {}, IntegratedLayouter::DebugIntegration{});
    CORRADE_COMPARE_AS(out,
        "Ui::DebugLayer::layouterName(): debug layer not part of a user interface\n"
        "Ui::DebugLayer::setLayouterName(): debug layer not part of a user interface\n"
        "Ui::DebugLayer::setLayouterName(): debug layer not part of a user interface\n"
        "Ui::DebugLayer::setLayouterName(): debug layer not part of a user interface\n"
        "Ui::DebugLayer::setLayouterName(): debug layer not part of a user interface\n"
        "Ui::DebugLayer::layouterName(): handle is null\n"
        "Ui::DebugLayer::setLayouterName(): layouter not part of the same user interface\n"
        "Ui::DebugLayer::setLayouterName(): layouter not part of the same user interface\n"
        "Ui::DebugLayer::setLayouterName(): layouter not part of the same user interface\n"
        "Ui::DebugLayer::setLayouterName(): layouter not part of the same user interface\n"
        "Ui::DebugLayer::setLayouterName(): layouter not part of the same user interface\n",
        TestSuite::Compare::String);
}

void DebugLayerTest::animatorNameNoOp() {
    /* Like layerNameNoOp(), but for animators. If Animators aren't enabled,
       the APIs don't assert but just don't do anything */

    AbstractUserInterface ui{{100, 100}};

    struct EmptyAnimator: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;

        AnimatorFeatures doFeatures() const override { return {}; }
        void doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
    };
    EmptyAnimator& emptyAnimator = ui.setGenericAnimatorInstance(Containers::pointer<EmptyAnimator>(ui.createAnimator()));
    /* Picking a source that isn't Animators but also isn't just empty */
    DebugLayer& layer = ui.setLayerInstance(Containers::pointer<DebugLayer>(ui.createLayer(), DebugLayerSource::NodeHierarchy, DebugLayerFlags{}));
    CORRADE_COMPARE(layer.animatorName(emptyAnimator.handle()), "");

    /* Setting a name doesn't remember anything */
    layer.setAnimatorName(emptyAnimator, "Empty");
    CORRADE_COMPARE(layer.animatorName(emptyAnimator.handle()), "");
}

void DebugLayerTest::animatorName() {
    /* Like layerName(), but for animators */

    AbstractUserInterface ui{{100, 100}};

    /* The animator type shouldn't matter, the layer should be able to store a
       name for it even if it doesn't have AnimatorFeature::NodeAttachment */
    struct EmptyGenericAnimator: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;

        AnimatorFeatures doFeatures() const override { return {}; }
        void doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
    };
    struct EmptyNodeAnimator: AbstractNodeAnimator {
        using AbstractNodeAnimator::AbstractNodeAnimator;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::NodeAttachment;
        }
        NodeAnimatorUpdates doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Float>&, const Containers::StridedArrayView1D<NodeFlags>&, Containers::MutableBitArrayView) override { return {}; }
    };

    struct Layer: DebugLayer {
        using DebugLayer::DebugLayer;

        const DebugLayer::State& stateData() {
            return *_state;
        }
    };
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), DebugLayerSource::Animators, DebugLayerFlags{}));

    EmptyGenericAnimator& emptyAnimator1 = ui.setGenericAnimatorInstance(Containers::pointer<EmptyGenericAnimator>(ui.createAnimator()));
    EmptyNodeAnimator& emptyAnimator2 = ui.setNodeAnimatorInstance(Containers::pointer<EmptyNodeAnimator>(ui.createAnimator()));

    /* Initially the debug layer has no animator entries */
    CORRADE_COMPARE(layer.stateData().animators.size(), 0);

    /* By default, any animator has the name empty. The empty names have no
       null-terminated or global flags guaranteed. */
    CORRADE_COMPARE(layer.animatorName(emptyAnimator1.handle()), "");
    CORRADE_COMPARE(layer.animatorName(emptyAnimator2.handle()), "");

    /* Setting a name of the first animator enlarges the array to fit it.
       Compared to layers, which are resized to contain at least the debug
       layer itself, the array is empty initially so there's no pre-existing
       entry to update. */
    layer.setAnimatorName(emptyAnimator1, "First empty");
    CORRADE_COMPARE(layer.stateData().animators.size(), 1);
    CORRADE_COMPARE(layer.animatorName(emptyAnimator1.handle()), "First empty");

    /* An animator outside of any existing bounds will have an empty name as
       well; an animator with known ID but wrong generation also, no
       null-terminated or global flags guaranteed in this case either */
    CORRADE_COMPARE(layer.animatorName(animatorHandle(255, 1)), "");
    CORRADE_COMPARE(layer.animatorName(animatorHandle(animatorHandleId(emptyAnimator1.handle()), animatorHandleGeneration(emptyAnimator1.handle()) + 1)), "");

    /* Create more animators, their names are empty again, and the size of the
       internal storage doesn't update implicitly to fit those */
    EmptyGenericAnimator& emptyAnimator3 = ui.setGenericAnimatorInstance(Containers::pointer<EmptyGenericAnimator>(ui.createAnimator()));
    EmptyNodeAnimator& emptyAnimator4 = ui.setNodeAnimatorInstance(Containers::pointer<EmptyNodeAnimator>(ui.createAnimator()));
    EmptyGenericAnimator& emptyAnimator5 = ui.setGenericAnimatorInstance(Containers::pointer<EmptyGenericAnimator>(ui.createAnimator()));
    CORRADE_COMPARE(layer.stateData().animators.size(), 1);
    CORRADE_COMPARE(layer.animatorName(emptyAnimator3.handle()), "");
    CORRADE_COMPARE(layer.animatorName(emptyAnimator4.handle()), "");
    CORRADE_COMPARE(layer.animatorName(emptyAnimator5.handle()), "");

    /* It enlarges only once setting a name of one of these */
    layer.setAnimatorName(emptyAnimator4, "Fourth");
    CORRADE_COMPARE(layer.stateData().animators.size(), 4);
    CORRADE_COMPARE(layer.animatorName(emptyAnimator4.handle()), "Fourth");

    /* Update doesn't clear the animator names */
    ui.update();
    CORRADE_COMPARE(layer.animatorName(emptyAnimator1.handle()), "First empty");
    CORRADE_COMPARE(layer.animatorName(emptyAnimator4.handle()), "Fourth");

    /* Setting a global string keeps a reference to it, local or
       non-null-terminated string is copied */
    Containers::StringView global = "Global"_s;
    layer.setAnimatorName(emptyAnimator2, global);
    CORRADE_COMPARE(layer.animatorName(emptyAnimator2.handle()), "Global");
    CORRADE_COMPARE(layer.animatorName(emptyAnimator2.handle()).data(), global.data());
    CORRADE_COMPARE(layer.animatorName(emptyAnimator2.handle()).flags(), Containers::StringViewFlag::Global|Containers::StringViewFlag::NullTerminated);

    Containers::StringView globalNonNullTerminated = "Global non null!"_s.exceptSuffix(1);
    layer.setAnimatorName(emptyAnimator3, globalNonNullTerminated);
    CORRADE_COMPARE(layer.animatorName(emptyAnimator3.handle()), "Global non null");
    CORRADE_VERIFY(layer.animatorName(emptyAnimator3.handle()).data() != globalNonNullTerminated.data());
    CORRADE_COMPARE(layer.animatorName(emptyAnimator3.handle()).flags(), Containers::StringViewFlag::NullTerminated);

    Containers::StringView local = "Local";
    layer.setAnimatorName(emptyAnimator5, local);
    CORRADE_COMPARE(layer.animatorName(emptyAnimator5.handle()), "Local");
    CORRADE_VERIFY(layer.animatorName(emptyAnimator5.handle()).data() != local.data());
    CORRADE_COMPARE(layer.animatorName(emptyAnimator5.handle()).flags(), Containers::StringViewFlag::NullTerminated);

    /* Removing an animator makes the old name still available with the old
       handle */
    AnimatorHandle emptyAnimator4Handle = emptyAnimator4.handle();
    ui.removeAnimator(emptyAnimator4Handle);
    CORRADE_COMPARE(layer.animatorName(emptyAnimator4Handle), "Fourth");

    /* When creating a new animator in the same slot, the new animator doesn't
       have a name yet and the old still keeps it */
    EmptyGenericAnimator& emptyAnimator4Replacement = ui.setGenericAnimatorInstance(Containers::pointer<EmptyGenericAnimator>(ui.createAnimator()));
    CORRADE_COMPARE(animatorHandleId(emptyAnimator4Replacement.handle()), animatorHandleId(emptyAnimator4Handle));
    CORRADE_COMPARE(layer.animatorName(emptyAnimator4Handle), "Fourth");
    CORRADE_COMPARE(layer.animatorName(emptyAnimator4Replacement.handle()), "");

    /* Setting a name for the replacement animator makes the old one unknown */
    layer.setAnimatorName(emptyAnimator4Replacement, "Replacement");
    CORRADE_COMPARE(layer.animatorName(emptyAnimator4Handle), "");
    CORRADE_COMPARE(layer.animatorName(emptyAnimator4Replacement.handle()), "Replacement");

    /* Updating after removing a layer and creating a new one in the same slot
       forgets the name -- the handle gets updated internally, so it cannot
       keep the name */
    AnimatorHandle emptyAnimator4ReplacementHandle = emptyAnimator4Replacement.handle();
    ui.removeAnimator(emptyAnimator4ReplacementHandle);
    EmptyNodeAnimator& emptyAnimator4Replacement2 = ui.setNodeAnimatorInstance(Containers::pointer<EmptyNodeAnimator>(ui.createAnimator()));
    CORRADE_COMPARE(animatorHandleId(emptyAnimator4Replacement2.handle()), animatorHandleId(emptyAnimator4ReplacementHandle));
    ui.update();
    CORRADE_COMPARE(layer.animatorName(emptyAnimator4ReplacementHandle), "");
    CORRADE_COMPARE(layer.animatorName(emptyAnimator4Replacement2.handle()), "");

    /* Updating after removing a layer forgets the name as well */
    layer.setAnimatorName(emptyAnimator4Replacement2, "Replacement 2");
    AnimatorHandle emptyAnimator4Replacement2Handle = emptyAnimator4Replacement2.handle();
    CORRADE_COMPARE(layer.animatorName(emptyAnimator4Replacement2Handle), "Replacement 2");
    ui.removeAnimator(emptyAnimator4Replacement2Handle);
    ui.update();
    CORRADE_COMPARE(layer.animatorName(emptyAnimator4Replacement2Handle), "");
}

void DebugLayerTest::animatorNameDebugIntegration() {
    auto&& data = AnimatorNameDebugIntegrationData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui{{100, 100}};

    struct Layer: DebugLayer {
        using DebugLayer::DebugLayer;

        const DebugLayer::State& stateData() {
            return *_state;
        }
    };

    struct EmptyAnimator: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;

        AnimatorFeatures doFeatures() const override { return {}; }
        void doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
    };

    /* It doesn't need to have AnimatorFeature::NodeAttachment to test the
       integration functionality */
    struct IntegratedAnimator: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;

        struct DebugIntegration {
            DebugIntegration() {
                ++debugIntegrationConstructed;
            }

            DebugIntegration(const DebugIntegration&) {
                ++debugIntegrationConstructed;
                ++debugIntegrationCopied;
            }

            DebugIntegration& operator=(const DebugIntegration&) {
                ++debugIntegrationCopied;
                return *this;
            }

            ~DebugIntegration() {
                ++debugIntegrationDestructed;
            }

            void print(Debug&, const IntegratedAnimator&, const Containers::StringView&, AnimatorDataHandle) {
                CORRADE_FAIL("This shouldn't be called.");
            }
        };

        AnimatorFeatures doFeatures() const override { return {}; }
        void doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
    };

    /* Initially there are no animator entries */
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), data.sources, DebugLayerFlags{}));
    CORRADE_COMPARE(layer.stateData().animators.size(), 0);

    /* An animator w/o DebugIntegration doesn't have any integration */
    EmptyAnimator& emptyAnimator1 = ui.setGenericAnimatorInstance(Containers::pointer<EmptyAnimator>(ui.createAnimator()));
    layer.setAnimatorName(emptyAnimator1, "Empty 1");
    CORRADE_COMPARE(layer.stateData().animators.size(), 1);
    CORRADE_VERIFY(!layer.stateData().animators[0].integration);
    CORRADE_VERIFY(!layer.stateData().animators[0].deleter);
    CORRADE_VERIFY(!layer.stateData().animators[0].print);

    /* Setting an animator name with a concrete type should allocate the
       DebugIntegration instance */
    IntegratedAnimator& integratedAnimator1 = ui.setGenericAnimatorInstance(Containers::pointer<IntegratedAnimator>(ui.createAnimator()));
    IntegratedAnimator& integratedAnimator2 = ui.setGenericAnimatorInstance(Containers::pointer<IntegratedAnimator>(ui.createAnimator()));
    IntegratedAnimator& integratedAnimator3 = ui.setGenericAnimatorInstance(Containers::pointer<IntegratedAnimator>(ui.createAnimator()));
    layer.setAnimatorName(integratedAnimator1, "Integrated");
    layer.setAnimatorName(integratedAnimator2, "Integrated 2");
    layer.setAnimatorName(integratedAnimator3, "Integrated 3");
    CORRADE_COMPARE(layer.stateData().animators.size(), 4);
    CORRADE_COMPARE(layer.stateData().animators[1].name, "Integrated");
    CORRADE_COMPARE(layer.stateData().animators[2].name, "Integrated 2");
    CORRADE_COMPARE(layer.stateData().animators[3].name, "Integrated 3");
    CORRADE_COMPARE(layer.stateData().animators[1].integration, data.used);
    CORRADE_COMPARE(layer.stateData().animators[2].integration, data.used);
    CORRADE_COMPARE(layer.stateData().animators[3].integration, data.used);
    CORRADE_COMPARE(layer.stateData().animators[1].deleter, data.used);
    CORRADE_COMPARE(layer.stateData().animators[2].deleter, data.used);
    CORRADE_COMPARE(layer.stateData().animators[3].deleter, data.used);
    CORRADE_COMPARE(layer.stateData().animators[1].print, data.used);
    CORRADE_COMPARE(layer.stateData().animators[2].print, data.used);
    CORRADE_COMPARE(layer.stateData().animators[3].print, data.used);
    /* It delegates to setAnimatorName(const DebugIntegration&), so it makes a
       temporary instance that then gets copied. If not used, it gets only
       copied a bunch of times but not allocated. */
    CORRADE_COMPARE(debugIntegrationConstructed, data.used ? 6 : 3);
    CORRADE_COMPARE(debugIntegrationCopied, data.used ? 3 : 0);
    CORRADE_COMPARE(debugIntegrationDestructed, 3);

    /* Setting an animator name again deletes the old (if there is) and
       allocates a new one */
    layer.setAnimatorName(integratedAnimator1, "Integrated 1");
    CORRADE_COMPARE(layer.stateData().animators.size(), 4);
    CORRADE_COMPARE(layer.stateData().animators[1].name, "Integrated 1");
    CORRADE_COMPARE(layer.stateData().animators[1].integration, data.used);
    CORRADE_COMPARE(layer.stateData().animators[1].deleter, data.used);
    CORRADE_COMPARE(layer.stateData().animators[1].print, data.used);
    CORRADE_COMPARE(debugIntegrationConstructed, data.used ? 8 : 4);
    CORRADE_COMPARE(debugIntegrationCopied, data.used ? 4 : 0);
    CORRADE_COMPARE(debugIntegrationDestructed, data.used ? 5 : 4);

    /* Adding a bunch more empty animators and setting name for the last will
       resize the internal storage, causing the integration allocation
       references to get moved, but not the instances themselves. They
       shouldn't get deleted. */
    /*EmptyAnimator& emptyAnimator2 =*/ ui.setGenericAnimatorInstance(Containers::pointer<EmptyAnimator>(ui.createAnimator()));
    EmptyAnimator& emptyAnimator3 = ui.setGenericAnimatorInstance(Containers::pointer<EmptyAnimator>(ui.createAnimator()));
    layer.setAnimatorName(emptyAnimator3, "Empty 3");
    CORRADE_COMPARE(layer.stateData().animators.size(), 6);
    CORRADE_COMPARE(layer.stateData().animators[1].name, "Integrated 1");
    CORRADE_COMPARE(layer.stateData().animators[2].name, "Integrated 2");
    CORRADE_COMPARE(layer.stateData().animators[3].name, "Integrated 3");
    CORRADE_COMPARE(layer.stateData().animators[1].integration, data.used);
    CORRADE_COMPARE(layer.stateData().animators[2].integration, data.used);
    CORRADE_COMPARE(layer.stateData().animators[3].integration, data.used);
    CORRADE_COMPARE(layer.stateData().animators[1].deleter, data.used);
    CORRADE_COMPARE(layer.stateData().animators[2].deleter, data.used);
    CORRADE_COMPARE(layer.stateData().animators[3].deleter, data.used);
    CORRADE_COMPARE(layer.stateData().animators[1].print, data.used);
    CORRADE_COMPARE(layer.stateData().animators[2].print, data.used);
    CORRADE_COMPARE(layer.stateData().animators[3].print, data.used);
    CORRADE_COMPARE(debugIntegrationConstructed, data.used ? 8 : 4);
    CORRADE_COMPARE(debugIntegrationCopied, data.used ? 4 : 0);
    CORRADE_COMPARE(debugIntegrationDestructed, data.used ? 5 : 4);

    /* Setting a different name with only the base type deletes the
       integration, if there is */
    layer.setAnimatorName(static_cast<AbstractAnimator&>(integratedAnimator1), "No longer integrated 1");
    CORRADE_COMPARE(layer.stateData().animators.size(), 6);
    CORRADE_COMPARE(layer.stateData().animators[1].name, "No longer integrated 1");
    CORRADE_VERIFY(!layer.stateData().animators[1].integration);
    CORRADE_VERIFY(!layer.stateData().animators[1].deleter);
    CORRADE_VERIFY(!layer.stateData().animators[1].print);
    CORRADE_COMPARE(debugIntegrationConstructed, data.used ? 8 : 4);
    CORRADE_COMPARE(debugIntegrationCopied, data.used ? 4 : 0);
    CORRADE_COMPARE(debugIntegrationDestructed, data.used ? 6 : 4);

    /* Setting it back recreates it, if used */
    layer.setAnimatorName(integratedAnimator1, "Integrated 1");
    CORRADE_COMPARE(layer.stateData().animators.size(), 6);
    CORRADE_COMPARE(layer.stateData().animators[1].name, "Integrated 1");
    CORRADE_COMPARE(layer.stateData().animators[1].integration, data.used);
    CORRADE_COMPARE(layer.stateData().animators[1].deleter, data.used);
    CORRADE_COMPARE(layer.stateData().animators[1].print, data.used);
    CORRADE_COMPARE(debugIntegrationConstructed, data.used ? 10 : 5);
    CORRADE_COMPARE(debugIntegrationCopied, data.used ? 5 : 0);
    CORRADE_COMPARE(debugIntegrationDestructed, data.used ? 7 : 5);

    /* Removing an integrated animator and replacing with non-integrated
       deletes the integration on next update(), if there is */
    AnimatorHandle integratedAnimator2Handle = integratedAnimator2.handle();
    ui.removeAnimator(integratedAnimator2Handle);
    EmptyAnimator& integratedAnimator2NonIntegratedReplacement = ui.setGenericAnimatorInstance(Containers::pointer<EmptyAnimator>(ui.createAnimator()));
    CORRADE_COMPARE(animatorHandleId(integratedAnimator2NonIntegratedReplacement.handle()), animatorHandleId(integratedAnimator2Handle));
    CORRADE_COMPARE(layer.stateData().animators[2].name, "Integrated 2");
    CORRADE_COMPARE(layer.stateData().animators[2].integration, data.used);
    CORRADE_COMPARE(layer.stateData().animators[2].deleter, data.used);
    CORRADE_COMPARE(layer.stateData().animators[2].print, data.used);
    /* Not here yet ... */
    CORRADE_COMPARE(debugIntegrationConstructed, data.used ? 10 : 5);
    CORRADE_COMPARE(debugIntegrationCopied, data.used ? 5 : 0);
    CORRADE_COMPARE(debugIntegrationDestructed, data.used ? 7 : 5);

    ui.update();
    CORRADE_COMPARE(layer.stateData().animators.size(), 6);
    CORRADE_COMPARE(layer.stateData().animators[2].name, "");
    CORRADE_VERIFY(!layer.stateData().animators[2].integration);
    CORRADE_VERIFY(!layer.stateData().animators[2].deleter);
    CORRADE_VERIFY(!layer.stateData().animators[2].print);
    /* ... but here */
    CORRADE_COMPARE(debugIntegrationConstructed, data.used ? 10 : 5);
    CORRADE_COMPARE(debugIntegrationCopied, data.used ? 5 : 0);
    CORRADE_COMPARE(debugIntegrationDestructed, data.used ? 8 : 5);

    /* Removing an integrated animator w/o replacing deletes the integration on
       next update() as well, if there is */
    ui.removeAnimator(integratedAnimator3.handle());
    CORRADE_COMPARE(layer.stateData().animators[3].name, "Integrated 3");
    CORRADE_COMPARE(layer.stateData().animators[3].integration, data.used);
    CORRADE_COMPARE(layer.stateData().animators[3].deleter, data.used);
    CORRADE_COMPARE(layer.stateData().animators[3].print, data.used);
    /* Not here yet ... */
    CORRADE_COMPARE(debugIntegrationConstructed, data.used ? 10 : 5);
    CORRADE_COMPARE(debugIntegrationCopied, data.used ? 5 : 0);
    CORRADE_COMPARE(debugIntegrationDestructed, data.used ? 8 : 5);

    ui.update();
    CORRADE_COMPARE(layer.stateData().animators.size(), 6);
    CORRADE_COMPARE(layer.stateData().animators[3].name, "");
    CORRADE_VERIFY(!layer.stateData().animators[3].integration);
    CORRADE_VERIFY(!layer.stateData().animators[3].deleter);
    CORRADE_VERIFY(!layer.stateData().animators[3].print);
    /* ... but here */
    CORRADE_COMPARE(debugIntegrationConstructed, data.used ? 10 : 5);
    CORRADE_COMPARE(debugIntegrationCopied, data.used ? 5 : 0);
    CORRADE_COMPARE(debugIntegrationDestructed, data.used ? 9 : 5);

    /* Removing the whole debug layer deletes the remaining integration, if
       there is */
    ui.removeLayer(layer.handle());
    CORRADE_COMPARE(debugIntegrationConstructed, data.used ? 10 : 5);
    CORRADE_COMPARE(debugIntegrationCopied, data.used ? 5 : 0);
    CORRADE_COMPARE(debugIntegrationDestructed, data.used ? 10 : 5);
}

void DebugLayerTest::animatorNameDebugIntegrationExplicit() {
    auto&& data = AnimatorNameDebugIntegrationData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Like layerNameDebugIntegrationExplicit() but for animators. A subset of
       animatorNameDebugIntegration() with a DebugIntegration that only has a
       non-default constructor and gets copied */

    AbstractUserInterface ui{{100, 100}};

    struct Layer: DebugLayer {
        using DebugLayer::DebugLayer;

        const DebugLayer::State& stateData() {
            return *_state;
        }
    };
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), data.sources, DebugLayerFlags{}));

    struct EmptyAnimator: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;

        AnimatorFeatures doFeatures() const override { return {}; }
        void doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
    };

    struct IntegratedAnimator: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;

        struct DebugIntegration {
            DebugIntegration(int value, float): value{value} {
                ++debugIntegrationConstructed;
            }

            DebugIntegration(const DebugIntegration& other): value{other.value} {
                ++debugIntegrationConstructed;
                ++debugIntegrationCopied;
            }

            /* This one shouldn't get used */
            DebugIntegration& operator=(const DebugIntegration& other) = delete;

            ~DebugIntegration() {
                ++debugIntegrationDestructed;
            }

            /* Compared to layerNameDebugIntegration(), here the signature does
               match */
            void print(Debug&, const IntegratedAnimator&, const Containers::StringView&, AnimatorDataHandle) {
                CORRADE_FAIL("This shouldn't be called.");
            }

            int value;
        };

        AnimatorFeatures doFeatures() const override { return {}; }
        void doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
    };

    /* Setting an animator name with a concrete type won't allocate the
       DebugIntegration instance, same reasoning as with layers in
       layerNameDebugIntegrationExplicit() */
    IntegratedAnimator& integratedAnimator1 = ui.setGenericAnimatorInstance(Containers::pointer<IntegratedAnimator>(ui.createAnimator()));
    layer.setAnimatorName(integratedAnimator1, "Integrated 1");
    CORRADE_COMPARE(layer.stateData().animators.size(), 1);
    CORRADE_COMPARE(layer.stateData().animators[0].name, "Integrated 1");
    CORRADE_VERIFY(!layer.stateData().animators[0].integration);
    CORRADE_VERIFY(!layer.stateData().animators[0].deleter);
    CORRADE_VERIFY(!layer.stateData().animators[0].print);
    CORRADE_COMPARE(debugIntegrationConstructed, 0);
    CORRADE_COMPARE(debugIntegrationCopied, 0);
    CORRADE_COMPARE(debugIntegrationDestructed, 0);

    IntegratedAnimator& integratedAnimator2 = ui.setGenericAnimatorInstance(Containers::pointer<IntegratedAnimator>(ui.createAnimator()));
    {
        IntegratedAnimator::DebugIntegration integration{1337, 4.5f};
        layer.setAnimatorName(integratedAnimator2, "Integrated 2", integration);
        CORRADE_COMPARE(layer.stateData().animators.size(), 2);
        CORRADE_COMPARE(layer.stateData().animators[1].name, "Integrated 2");
        CORRADE_COMPARE(layer.stateData().animators[1].integration, data.used);
        if(data.used)
            CORRADE_COMPARE(static_cast<IntegratedAnimator::DebugIntegration*>(layer.stateData().animators[1].integration)->value, 1337);
        CORRADE_COMPARE(layer.stateData().animators[1].deleter, data.used);
        CORRADE_COMPARE(layer.stateData().animators[1].print, data.used);
    }
    /* A local instance gets constructed, copied to the function, then
       internally moved to allocate the instance (which calls the copy
       constructor again) and then both temporaries get destructed. If not
       used, the final allocation isn't made. */
    CORRADE_COMPARE(debugIntegrationConstructed, data.used ? 3 : 2);
    CORRADE_COMPARE(debugIntegrationCopied, data.used ? 2 : 1);
    CORRADE_COMPARE(debugIntegrationDestructed, 2);

    /* Removing the whole debug layer deletes the integration in this case as
       well, if there is */
    ui.removeLayer(layer.handle());
    CORRADE_COMPARE(debugIntegrationConstructed, data.used ? 3 : 2);
    CORRADE_COMPARE(debugIntegrationCopied, data.used ? 2 : 1);
    CORRADE_COMPARE(debugIntegrationDestructed, data.used ? 3 : 2);
}

void DebugLayerTest::animatorNameDebugIntegrationExplicitRvalue() {
    auto&& data = AnimatorNameDebugIntegrationData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Like layerNameDebugIntegrationExplicitRvalue() but for animators. A
       subset of animatorNameDebugIntegration() but with a DebugIntegration
       that only has a non-default constructor and gets moved */

    AbstractUserInterface ui{{100, 100}};

    struct Layer: DebugLayer {
        using DebugLayer::DebugLayer;

        const DebugLayer::State& stateData() {
            return *_state;
        }
    };
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), data.sources, DebugLayerFlags{}));

    struct EmptyAnimator: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;

        AnimatorFeatures doFeatures() const override { return {}; }
        void doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
    };

    struct IntegratedAnimator: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;

        struct DebugIntegration {
            DebugIntegration(int value, float): value{value} {
                ++debugIntegrationConstructed;
            }

            DebugIntegration(const DebugIntegration& other) = delete;
            DebugIntegration(DebugIntegration&& other): value{other.value} {
                ++debugIntegrationConstructed;
                ++debugIntegrationMoved;
            }

            DebugIntegration& operator=(const DebugIntegration& other) = delete;
            /* This one shouldn't get used */
            DebugIntegration& operator=(DebugIntegration&& other) = delete;

            ~DebugIntegration() {
                ++debugIntegrationDestructed;
            }

            void print(Debug&, const IntegratedAnimator&, const Containers::StringView&, AnimatorDataHandle) {
                CORRADE_FAIL("This shouldn't be called.");
            }

            int value;
        };

        AnimatorFeatures doFeatures() const override { return {}; }
        void doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
    };

    /* Setting an animator name with a concrete type won't allocate the
       DebugIntegration instance, same reasoning as with layers in
       layerNameDebugIntegrationExplicit() */
    IntegratedAnimator& integratedAnimator1 = ui.setGenericAnimatorInstance(Containers::pointer<IntegratedAnimator>(ui.createAnimator()));
    layer.setAnimatorName(integratedAnimator1, "Integrated 1");
    CORRADE_COMPARE(layer.stateData().animators.size(), 1);
    CORRADE_COMPARE(layer.stateData().animators[0].name, "Integrated 1");
    CORRADE_VERIFY(!layer.stateData().animators[0].integration);
    CORRADE_VERIFY(!layer.stateData().animators[0].deleter);
    CORRADE_VERIFY(!layer.stateData().animators[0].print);
    CORRADE_COMPARE(debugIntegrationConstructed, 0);
    CORRADE_COMPARE(debugIntegrationMoved, 0);
    CORRADE_COMPARE(debugIntegrationDestructed, 0);

    IntegratedAnimator& integratedAnimator2 = ui.setGenericAnimatorInstance(Containers::pointer<IntegratedAnimator>(ui.createAnimator()));
    layer.setAnimatorName(integratedAnimator2, "Integrated 2", IntegratedAnimator::DebugIntegration{1337, 4.5f});
    CORRADE_COMPARE(layer.stateData().animators.size(), 2);
    CORRADE_COMPARE(layer.stateData().animators[1].name, "Integrated 2");
    CORRADE_COMPARE(layer.stateData().animators[1].integration, data.used);
    if(data.used)
        CORRADE_COMPARE(static_cast<IntegratedAnimator::DebugIntegration*>(layer.stateData().animators[1].integration)->value, 1337);
    CORRADE_COMPARE(layer.stateData().animators[1].deleter, data.used);
    CORRADE_COMPARE(layer.stateData().animators[1].print, data.used);
    /* A local instance gets moved to the function, then internally moved again
       to allocate the instance and then the temporary get destructed */
    CORRADE_COMPARE(debugIntegrationConstructed, data.used ? 2 : 1);
    CORRADE_COMPARE(debugIntegrationMoved, data.used ? 1 : 0);
    CORRADE_COMPARE(debugIntegrationDestructed, 1);

    /* Removing the whole debug layer deletes the integration in this case as
       well */
    ui.removeLayer(layer.handle());
    CORRADE_COMPARE(debugIntegrationConstructed, data.used ? 2 : 1);
    CORRADE_COMPARE(debugIntegrationMoved, data.used ? 1 : 0);
    CORRADE_COMPARE(debugIntegrationDestructed, data.used ? 2 : 1);
}

void DebugLayerTest::animatorNameDebugIntegrationCopyConstructPlainStruct() {
    /* Like layerNameDebugIntegrationCopyConstructPlainStruct() but for
       animators */

    AbstractUserInterface ui{{100, 100}};

    struct Layer: DebugLayer {
        using DebugLayer::DebugLayer;

        const DebugLayer::State& stateData() {
            return *_state;
        }
    };
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), DebugLayerSource::NodeAnimationDetails, DebugLayerFlags{}));

    struct IntegratedAnimator: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;

        struct DebugIntegration {
            void print(Debug&, const IntegratedAnimator&, Containers::StringView, AnimatorDataHandle) {}

            int a;
            char b;
        };

        AnimatorFeatures doFeatures() const override { return {}; }
        void doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
    };
    IntegratedAnimator& integratedAnimator = ui.setGenericAnimatorInstance(Containers::pointer<IntegratedAnimator>(ui.createAnimator()));

    /* This needs special handling on GCC 4.8, where
       new DebugIntegration{integration} (copy-construction) attempts to
       convert DebugIntegration to int to initialize the first member and
       fails miserably. Similar case is in Containers::Array etc. */
    IntegratedAnimator::DebugIntegration integration;
    layer.setAnimatorName(integratedAnimator, "Extremely Trivial", integration);
    CORRADE_COMPARE(layer.stateData().animators.size(), 1);
    CORRADE_VERIFY(layer.stateData().animators[0].integration);
}

void DebugLayerTest::animatorNameDebugIntegrationMoveConstructPlainStruct() {
    /* Like layerNameDebugIntegrationMoveConstructPlainStruct() but for
       animators */

    AbstractUserInterface ui{{100, 100}};

    struct Layer: DebugLayer {
        using DebugLayer::DebugLayer;

        const DebugLayer::State& stateData() {
            return *_state;
        }
    };
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), DebugLayerSource::NodeAnimationDetails, DebugLayerFlags{}));

    struct IntegratedAnimator: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;

        struct DebugIntegration {
            void print(Debug&, const IntegratedAnimator&, Containers::StringView, AnimatorDataHandle) {}

            int a;
            Containers::Pointer<char> b;
        };

        AnimatorFeatures doFeatures() const override { return {}; }
        void doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
    };
    IntegratedAnimator& integratedLayer1 = ui.setGenericAnimatorInstance(Containers::pointer<IntegratedAnimator>(ui.createAnimator()));
    IntegratedAnimator& integratedLayer2 = ui.setGenericAnimatorInstance(Containers::pointer<IntegratedAnimator>(ui.createAnimator()));

    /* This needs special handling on GCC 4.8, where
       new DebugIntegration{Utility::move(integration)} attempts to convert
       DebugIntegration to int to initialize the first member and fails
       miserably. Similar case is in Containers::Array etc. */
    layer.setAnimatorName(integratedLayer1, "Extremely Trivial", IntegratedAnimator::DebugIntegration{});
    /* This case internally does the above, so verify it works there as well */
    layer.setAnimatorName(integratedLayer2, "Extremely Trivial");
    CORRADE_COMPARE(layer.stateData().animators.size(), 2);
    CORRADE_VERIFY(layer.stateData().animators[0].integration);
    CORRADE_VERIFY(layer.stateData().animators[1].integration);
}

void DebugLayerTest::animatorNameInvalid() {
    /* Like layerNameInvalid(), but for animators */

    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui{{100, 100}};
    AbstractUserInterface uiAnother{{100, 100}};

    /* Enabling NodeAnimationDetails so the integration is used in full, just
       in case */
    DebugLayer& layer = ui.setLayerInstance(Containers::pointer<DebugLayer>(ui.createLayer(), DebugLayerSource::NodeAnimationDetails, DebugLayerFlags{}));
    DebugLayer layerNoUi{layerHandle(0, 1), {}, {}};

    struct EmptyAnimator: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;

        AnimatorFeatures doFeatures() const override { return {}; }
        void doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
    };
    EmptyAnimator& animator = ui.setGenericAnimatorInstance(Containers::pointer<EmptyAnimator>(ui.createAnimator()));
    EmptyAnimator& animatorAnotherUi = uiAnother.setGenericAnimatorInstance(Containers::pointer<EmptyAnimator>(uiAnother.createAnimator()));
    EmptyAnimator animatorArtificialHandle{animatorHandle(0xab, 0x12)};

    struct IntegratedAnimator: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;

        struct DebugIntegration {
            void print(Debug&, const IntegratedAnimator&, const Containers::StringView&, AnimatorDataHandle) {
                CORRADE_FAIL("This shouldn't be called.");
            }
        };

        AnimatorFeatures doFeatures() const override { return {}; }
        void doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
    };
    IntegratedAnimator& integratedAnimator = ui.setGenericAnimatorInstance(Containers::pointer<IntegratedAnimator>(ui.createAnimator()));
    IntegratedAnimator& integratedAnimatorAnotherUi = uiAnother.setGenericAnimatorInstance(Containers::pointer<IntegratedAnimator>(uiAnother.createAnimator()));
    IntegratedAnimator::DebugIntegration integration;

    Containers::String out;
    Error redirectError{&out};
    layerNoUi.animatorName(AnimatorHandle::Null);
    layerNoUi.setAnimatorName(animator, {});
    layerNoUi.setAnimatorName(integratedAnimator, {});
    layerNoUi.setAnimatorName(integratedAnimator, {}, integration);
    layerNoUi.setAnimatorName(integratedAnimator, {}, IntegratedAnimator::DebugIntegration{});
    layer.animatorName(AnimatorHandle::Null);
    layer.setAnimatorName(animatorAnotherUi, {});
    layer.setAnimatorName(animatorArtificialHandle, {});
    layer.setAnimatorName(integratedAnimatorAnotherUi, {});
    layer.setAnimatorName(integratedAnimatorAnotherUi, {}, integration);
    layer.setAnimatorName(integratedAnimatorAnotherUi, {}, IntegratedAnimator::DebugIntegration{});
    CORRADE_COMPARE_AS(out,
        "Ui::DebugLayer::animatorName(): debug layer not part of a user interface\n"
        "Ui::DebugLayer::setAnimatorName(): debug layer not part of a user interface\n"
        "Ui::DebugLayer::setAnimatorName(): debug layer not part of a user interface\n"
        "Ui::DebugLayer::setAnimatorName(): debug layer not part of a user interface\n"
        "Ui::DebugLayer::setAnimatorName(): debug layer not part of a user interface\n"
        "Ui::DebugLayer::animatorName(): handle is null\n"
        "Ui::DebugLayer::setAnimatorName(): animator not part of the same user interface\n"
        "Ui::DebugLayer::setAnimatorName(): animator not part of the same user interface\n"
        "Ui::DebugLayer::setAnimatorName(): animator not part of the same user interface\n"
        "Ui::DebugLayer::setAnimatorName(): animator not part of the same user interface\n"
        "Ui::DebugLayer::setAnimatorName(): animator not part of the same user interface\n",
        TestSuite::Compare::String);
}

void DebugLayerTest::preUpdateNoUi() {
    CORRADE_SKIP_IF_NO_ASSERT();

    DebugLayer layer{layerHandle(0, 1), {}, {}};

    Containers::String out;
    Error redirectError{&out};
    layer.preUpdate(LayerState::NeedsCommonDataUpdate);
    CORRADE_COMPARE(out,
        "Ui::DebugLayer::preUpdate(): layer not part of a user interface\n");
}

void DebugLayerTest::preUpdateNoOp() {
    auto&& data = PreUpdateNoOpData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* UI with some nodes and layers already present */
    AbstractUserInterface ui{{100, 100}};

    ui.createNode({}, {});
    ui.createNode({}, {});

    struct EmptyLayer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    };
    ui.setLayerInstance(Containers::pointer<EmptyLayer>(ui.createLayer()));

    struct EmptyLayouter: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;

        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&) override {}
    };
    ui.setLayouterInstance(Containers::pointer<EmptyLayouter>(ui.createLayouter()));

    struct EmptyGenericAnimator: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::NodeAttachment;
        }
        void doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
    };
    ui.setGenericAnimatorInstance(Containers::pointer<EmptyGenericAnimator>(ui.createAnimator()));

    /* Initially the layer will have nothing */
    struct Layer: DebugLayer {
        using DebugLayer::DebugLayer;

        const DebugLayer::State& stateData() {
            return *_state;
        }
    };
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), data.sources, data.flags));
    CORRADE_COMPARE(!layer.state(), data.expectNoState);
    CORRADE_VERIFY(!layer.usedCount());
    CORRADE_VERIFY(layer.stateData().nodes.isEmpty());
    /* Layers are pre-filled with the default name for the debug layer even
       before update() happens */
    CORRADE_COMPARE(layer.stateData().layers.isEmpty(), data.expectNoLayers);
    CORRADE_VERIFY(layer.stateData().layouters.isEmpty());
    CORRADE_VERIFY(layer.stateData().animators.isEmpty());

    /* The layer has the NeedsCommonDataUpdate set always, so UI update() will
       never fully clean that up */
    ui.update();
    CORRADE_COMPARE(!layer.state(), data.expectNoState);
    CORRADE_COMPARE(!layer.usedCount(), data.expectNoData);
    CORRADE_COMPARE(layer.stateData().nodes.isEmpty(), data.expectNoNodes);
    CORRADE_COMPARE(layer.stateData().layers.isEmpty(), data.expectNoLayers);
    CORRADE_COMPARE(layer.stateData().layouters.isEmpty(), data.expectNoLayouters);
    CORRADE_COMPARE(layer.stateData().animators.isEmpty(), data.expectNoAnimators);
}

void DebugLayerTest::preUpdateTrackNodes() {
    auto&& data = PreUpdateTrackNodesData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui{{100, 100}};

    NodeHandle node1 = ui.createNode({}, {});
    NodeHandle node2 = ui.createNode({}, {});

    /* Initially the layer will have nothing even though there are some nodes
       already, it'll however set a state to trigger population on next
       update */
    struct Layer: DebugLayer {
        using DebugLayer::DebugLayer;

        const DebugLayer::State& stateData() {
            return *_state;
        }
    };
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), data.sources, data.flags));
    CORRADE_VERIFY(layer.stateData().nodes.isEmpty());
    CORRADE_COMPARE(layer.usedCount(), 0);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);
    CORRADE_COMPARE_AS(ui.state(),
        UserInterfaceState::NeedsDataUpdate,
        TestSuite::Compare::GreaterOrEqual);

    /* Update will populate the nodes, the state will stay set even after */
    ui.update();
    CORRADE_COMPARE(layer.stateData().nodes.size(), 2);
    CORRADE_COMPARE(layer.stateData().nodes[0].handle, node1);
    CORRADE_COMPARE(layer.stateData().nodes[0].highlightData != LayerDataHandle::Null, data.expectData);
    CORRADE_COMPARE(layer.stateData().nodes[1].handle, node2);
    CORRADE_COMPARE(layer.stateData().nodes[1].highlightData != LayerDataHandle::Null, data.expectData);
    CORRADE_COMPARE(layer.usedCount(), data.expectData ? 2 : 0);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);
    CORRADE_COMPARE_AS(ui.state(),
        UserInterfaceState::NeedsDataUpdate,
        TestSuite::Compare::GreaterOrEqual);

    /* Adding more nodes resizes the internal storage after update */
    NodeHandle node3 = ui.createNode({}, {});
    NodeHandle node4 = ui.createNode({}, {});
    ui.update();
    CORRADE_COMPARE(layer.stateData().nodes.size(), 4);
    CORRADE_COMPARE(layer.usedCount(), data.expectData ? 4 : 0);
    CORRADE_COMPARE(layer.stateData().nodes[2].handle, node3);
    CORRADE_COMPARE(layer.stateData().nodes[2].highlightData != LayerDataHandle::Null, data.expectData);
    CORRADE_COMPARE(layer.stateData().nodes[3].handle, node4);
    CORRADE_COMPARE(layer.stateData().nodes[3].highlightData != LayerDataHandle::Null, data.expectData);

    /* Removing a node clears the handle and anything else, like a name that
       has been set. Replacing a node with another in the same spot does the
       same */
    layer.setNodeName(node2, "Hello!");
    layer.setNodeName(node3, "Hello?");
    CORRADE_COMPARE(layer.stateData().nodes[1].name, "Hello!");
    CORRADE_COMPARE(layer.stateData().nodes[2].name, "Hello?");
    ui.removeNode(node2);
    ui.removeNode(node3);
    NodeHandle node2Replacement = ui.createNode({}, {});
    ui.update();
    CORRADE_COMPARE(nodeHandleId(node2Replacement), nodeHandleId(node2));
    CORRADE_COMPARE(layer.stateData().nodes.size(), 4);
    CORRADE_COMPARE(layer.usedCount(), data.expectData ? 3 : 0);
    CORRADE_COMPARE(layer.stateData().nodes[1].handle, node2Replacement);
    CORRADE_COMPARE(layer.stateData().nodes[1].highlightData != LayerDataHandle::Null, data.expectData);
    CORRADE_COMPARE(layer.stateData().nodes[1].name, "");
    CORRADE_COMPARE(layer.stateData().nodes[2].handle, NodeHandle::Null);
    CORRADE_COMPARE(layer.stateData().nodes[2].highlightData, LayerDataHandle::Null);
    CORRADE_COMPARE(layer.stateData().nodes[2].name, "");
}

void DebugLayerTest::preUpdateTrackLayers() {
    auto&& data = PreUpdateTrackLayersData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui{{100, 100}};

    struct EmptyLayer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    };
    EmptyLayer& emptyLayer1 = ui.setLayerInstance(Containers::pointer<EmptyLayer>(ui.createLayer()));

    /* Initially the layer will have nothing even though there are some layers
       already, it'll however set a state to trigger population on next
       update */
    struct Layer: DebugLayer {
        using DebugLayer::DebugLayer;

        const DebugLayer::State& stateData() {
            return *_state;
        }
    };
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), data.sources, DebugLayerFlags{}));
    /* There's already an entry for name of the debug layer itself */
    CORRADE_COMPARE(layer.stateData().layers.size(), 2);
    CORRADE_COMPARE(layer.usedCount(), 0);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);
    CORRADE_COMPARE_AS(ui.state(),
        UserInterfaceState::NeedsDataUpdate,
        TestSuite::Compare::GreaterOrEqual);

    /* Update will populate the layers, the state will stay set even after. No
       data are created for the layers. */
    ui.update();
    CORRADE_COMPARE(layer.stateData().layers.size(), 2);
    CORRADE_COMPARE(layer.stateData().layers[0].handle, emptyLayer1.handle());
    CORRADE_COMPARE(layer.stateData().layers[1].handle, layer.handle());
    CORRADE_COMPARE(layer.usedCount(), 0);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);
    CORRADE_COMPARE_AS(ui.state(),
        UserInterfaceState::NeedsDataUpdate,
        TestSuite::Compare::GreaterOrEqual);

    /* Adding more layers resizes the internal storage after update */
    EmptyLayer& emptyLayer2 = ui.setLayerInstance(Containers::pointer<EmptyLayer>(ui.createLayer()));
    EmptyLayer& emptyLayer3 = ui.setLayerInstance(Containers::pointer<EmptyLayer>(ui.createLayer()));
    ui.update();
    CORRADE_COMPARE(layer.stateData().layers.size(), 4);
    CORRADE_COMPARE(layer.usedCount(), 0);
    CORRADE_COMPARE(layer.stateData().layers[2].handle, emptyLayer2.handle());
    CORRADE_COMPARE(layer.stateData().layers[3].handle, emptyLayer3.handle());

    /* Removing a layer clears the handle and anything else, like a name that
       has been set. Replacing a node with another in the same spot does the
       same. */
    layer.setLayerName(emptyLayer1, "Hello!");
    layer.setLayerName(emptyLayer2, "Hello?");
    CORRADE_COMPARE(layer.stateData().layers[0].name, "Hello!");
    CORRADE_COMPARE(layer.stateData().layers[2].name, "Hello?");
    LayerHandle emptyLayer2Handle = emptyLayer2.handle();
    ui.removeLayer(emptyLayer2Handle);
    ui.removeLayer(emptyLayer1.handle());
    EmptyLayer& emptyLayer2Replacement = ui.setLayerInstance(Containers::pointer<EmptyLayer>(ui.createLayer()));
    ui.update();
    CORRADE_COMPARE(layerHandleId(emptyLayer2Replacement.handle()), layerHandleId(emptyLayer2Handle));
    CORRADE_COMPARE(layer.stateData().layers.size(), 4);
    CORRADE_COMPARE(layer.stateData().layers[0].handle, LayerHandle::Null);
    CORRADE_COMPARE(layer.stateData().layers[0].name, "");
    CORRADE_COMPARE(layer.stateData().layers[2].handle, emptyLayer2Replacement.handle());
    CORRADE_COMPARE(layer.stateData().layers[2].name, "");
}

void DebugLayerTest::preUpdateTrackLayouters() {
    auto&& data = PreUpdateTrackLayoutersData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui{{100, 100}};

    struct EmptyLayouter: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;

        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&) override {}
    };
    EmptyLayouter& emptyLayouter1 = ui.setLayouterInstance(Containers::pointer<EmptyLayouter>(ui.createLayouter()));

    /* Initially the layer will have nothing even though there are some
       layouters already, it'll however set a state to trigger population on
       next update */
    struct Layer: DebugLayer {
        using DebugLayer::DebugLayer;

        const DebugLayer::State& stateData() {
            return *_state;
        }
    };
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), data.sources, DebugLayerFlags{}));
    CORRADE_COMPARE(layer.stateData().layouters.size(), 0);
    CORRADE_COMPARE(layer.usedCount(), 0);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);
    CORRADE_COMPARE_AS(ui.state(),
        UserInterfaceState::NeedsDataUpdate,
        TestSuite::Compare::GreaterOrEqual);

    /* Update will populate the layouters, the state will stay set even after.
       No data are created for the layouters. */
    ui.update();
    CORRADE_COMPARE(layer.stateData().layouters.size(), 1);
    CORRADE_COMPARE(layer.stateData().layouters[0].handle, emptyLayouter1.handle());
    CORRADE_COMPARE(layer.usedCount(), 0);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);
    CORRADE_COMPARE_AS(ui.state(),
        UserInterfaceState::NeedsDataUpdate,
        TestSuite::Compare::GreaterOrEqual);

    /* Adding more layouters resizes the internal storage after update */
    EmptyLayouter& emptyLayouter2 = ui.setLayouterInstance(Containers::pointer<EmptyLayouter>(ui.createLayouter()));
    EmptyLayouter& emptyLayouter3 = ui.setLayouterInstance(Containers::pointer<EmptyLayouter>(ui.createLayouter()));
    ui.update();
    CORRADE_COMPARE(layer.stateData().layouters.size(), 3);
    CORRADE_COMPARE(layer.usedCount(), 0);
    CORRADE_COMPARE(layer.stateData().layouters[1].handle, emptyLayouter2.handle());
    CORRADE_COMPARE(layer.stateData().layouters[2].handle, emptyLayouter3.handle());

    /* Removing a layer clears the handle and anything else, like a name that
       has been set. Replacing a node with another in the same spot does the
       same. */
    layer.setLayouterName(emptyLayouter1, "Hello!");
    layer.setLayouterName(emptyLayouter2, "Hello?");
    CORRADE_COMPARE(layer.stateData().layouters[0].name, "Hello!");
    CORRADE_COMPARE(layer.stateData().layouters[1].name, "Hello?");
    LayouterHandle emptyLayouter2Handle = emptyLayouter2.handle();
    ui.removeLayouter(emptyLayouter2Handle);
    ui.removeLayouter(emptyLayouter1.handle());
    EmptyLayouter& emptyLayouter2Replacement = ui.setLayouterInstance(Containers::pointer<EmptyLayouter>(ui.createLayouter()));
    ui.update();
    CORRADE_COMPARE(layouterHandleId(emptyLayouter2Replacement.handle()), layouterHandleId(emptyLayouter2Handle));
    CORRADE_COMPARE(layer.stateData().layouters.size(), 3);
    CORRADE_COMPARE(layer.stateData().layouters[0].handle, LayouterHandle::Null);
    CORRADE_COMPARE(layer.stateData().layouters[0].name, "");
    CORRADE_COMPARE(layer.stateData().layouters[1].handle, emptyLayouter2Replacement.handle());
    CORRADE_COMPARE(layer.stateData().layouters[1].name, "");
}

void DebugLayerTest::preUpdateTrackAnimators() {
    auto&& data = PreUpdateTrackAnimatorsData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Like preUpdateTrackLayers(), but for animators */

    AbstractUserInterface ui{{100, 100}};

    struct EmptyGenericAnimator: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;

        AnimatorFeatures doFeatures() const override { return {}; }
        void doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
    };
    struct EmptyNodeAnimator: AbstractNodeAnimator {
        using AbstractNodeAnimator::AbstractNodeAnimator;

        AnimatorFeatures doFeatures() const override { return AnimatorFeature::NodeAttachment; }
        NodeAnimatorUpdates doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Float>&, const Containers::StridedArrayView1D<NodeFlags>&, Containers::MutableBitArrayView) override { return {}; }
    };
    EmptyGenericAnimator& emptyAnimator1 = ui.setGenericAnimatorInstance(Containers::pointer<EmptyGenericAnimator>(ui.createAnimator()));

    /* Initially the layer will have nothing even though there are some
       animators already, it'll however set a state to trigger population on
       next update */
    struct Layer: DebugLayer {
        using DebugLayer::DebugLayer;

        const DebugLayer::State& stateData() {
            return *_state;
        }
    };
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), data.sources, DebugLayerFlags{}));
    CORRADE_COMPARE(layer.stateData().animators.size(), 0);
    CORRADE_COMPARE(layer.usedCount(), 0);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);
    CORRADE_COMPARE_AS(ui.state(),
        UserInterfaceState::NeedsDataUpdate,
        TestSuite::Compare::GreaterOrEqual);

    /* Update will populate the animators, the state will stay set even after.
       No data are created for the animators. */
    ui.update();
    CORRADE_COMPARE(layer.stateData().animators.size(), 1);
    CORRADE_COMPARE(layer.stateData().animators[0].handle, emptyAnimator1.handle());
    CORRADE_COMPARE(layer.usedCount(), 0);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);
    CORRADE_COMPARE_AS(ui.state(),
        UserInterfaceState::NeedsDataUpdate,
        TestSuite::Compare::GreaterOrEqual);

    /* Adding more animators resizes the internal storage after update */
    EmptyNodeAnimator& emptyAnimator2 = ui.setNodeAnimatorInstance(Containers::pointer<EmptyNodeAnimator>(ui.createAnimator()));
    EmptyGenericAnimator& emptyAnimator3 = ui.setGenericAnimatorInstance(Containers::pointer<EmptyGenericAnimator>(ui.createAnimator()));
    ui.update();
    CORRADE_COMPARE(layer.stateData().animators.size(), 3);
    CORRADE_COMPARE(layer.usedCount(), 0);
    CORRADE_COMPARE(layer.stateData().animators[1].handle, emptyAnimator2.handle());
    CORRADE_COMPARE(layer.stateData().animators[2].handle, emptyAnimator3.handle());

    /* Removing an animator clears the handle and anything else, like a name
       that has been set. Replacing a node or layer with another in the same
       spot does the same. */
    layer.setAnimatorName(emptyAnimator1, "Hello!");
    layer.setAnimatorName(emptyAnimator2, "Hello?");
    CORRADE_COMPARE(layer.stateData().animators[0].name, "Hello!");
    CORRADE_COMPARE(layer.stateData().animators[1].name, "Hello?");
    AnimatorHandle emptyAnimator2Handle = emptyAnimator2.handle();
    ui.removeAnimator(emptyAnimator2Handle);
    ui.removeAnimator(emptyAnimator1.handle());
    EmptyGenericAnimator& emptyAnimator2Replacement = ui.setGenericAnimatorInstance(Containers::pointer<EmptyGenericAnimator>(ui.createAnimator()));
    ui.update();
    CORRADE_COMPARE(animatorHandleId(emptyAnimator2Replacement.handle()), animatorHandleId(emptyAnimator2Handle));
    CORRADE_COMPARE(layer.stateData().animators.size(), 3);
    CORRADE_COMPARE(layer.stateData().animators[0].handle, AnimatorHandle::Null);
    CORRADE_COMPARE(layer.stateData().animators[0].name, "");
    CORRADE_COMPARE(layer.stateData().animators[1].handle, emptyAnimator2Replacement.handle());
    CORRADE_COMPARE(layer.stateData().animators[1].name, "");
}

void DebugLayerTest::nodeInspectSetters() {
    auto&& data = LayerDrawData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* These should work even with NodeInspect not set, so user code can set
       all those independently of deciding what to actually use */
    struct Layer: DebugLayer {
        explicit Layer(LayerHandle handle, LayerFeatures features): DebugLayer{handle, {}, {}}, _features{features} {}

        LayerFeatures doFeatures() const override {
            return DebugLayer::doFeatures()|_features;
        }

        private:
            LayerFeatures _features;
    } layer{layerHandle(0, 1), data.features};

    /* Required to be called before update() (because AbstractUserInterface
       guarantees the same on a higher level), not needed for anything here */
    if(data.features >= LayerFeature::Draw)
        layer.setSize({1, 1}, {1, 1});

    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Defaults */
    CORRADE_COMPARE(layer.nodeInspectColor(), 0xff00ffff_rgbaf*0.5f);
    CORRADE_COMPARE(layer.nodeInspectGesture(), Containers::pair(Pointer::MouseRight|Pointer::Eraser, ~~Modifier::Ctrl));
    CORRADE_VERIFY(!layer.hasNodeInspectCallback());

    /* Use of this one is further tested in update() and in DebugLayerGLTest.
       Changing the color causes NeedsDataUpdate to be set, but only if the
       layer draws anything. */
    layer.setNodeInspectColor(0x3399ff66_rgbaf);
    CORRADE_COMPARE(layer.nodeInspectColor(), 0x3399ff66_rgbaf);
    CORRADE_COMPARE(layer.state(), data.expectedState);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    layer.setNodeInspectGesture(Pointer::MouseMiddle|Pointer::Finger, Modifier::Alt|Modifier::Shift);
    CORRADE_COMPARE(layer.nodeInspectGesture(), Containers::pair(Pointer::MouseMiddle|Pointer::Finger, Modifier::Alt|Modifier::Shift));
    /* Setting the gesture doesn't need any update */
    CORRADE_COMPARE(layer.state(), LayerStates{});

    layer.setNodeInspectCallback([](Containers::StringView){});
    CORRADE_VERIFY(layer.hasNodeInspectCallback());
    /* Setting the callback doesn't need any update */
    CORRADE_COMPARE(layer.state(), LayerStates{});

    layer.setNodeInspectCallback(nullptr);
    CORRADE_VERIFY(!layer.hasNodeInspectCallback());
    /* Setting the callback doesn't need any update */
    CORRADE_COMPARE(layer.state(), LayerStates{});
}

void DebugLayerTest::nodeInspectNoOp() {
    auto&& data = NodeInspectNoOpData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui{{100, 100}};

    /* Node to catch the event on */
    NodeHandle node = ui.createNode({}, {100, 100});

    /* Layer to have the event fall to always */
    struct FallbackLayer: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Event;
        }

        void doPointerPressEvent(UnsignedInt, PointerEvent&) override {
            ++called;
        }

        Int called = 0;
    };
    FallbackLayer& fallbackLayer = ui.setLayerInstance(Containers::pointer<FallbackLayer>(ui.createLayer()));
    fallbackLayer.create(node);

    /* Debug layer on top */
    DebugLayer& layer = ui.setLayerInstance(Containers::pointer<DebugLayer>(ui.createLayer(), data.sources, data.flags));
    if(data.acceptedPointers)
        layer.setNodeInspectGesture(data.acceptedPointers, Modifier::Ctrl);
    Int callbackCalled = 0;
    layer.setNodeInspectCallback([&callbackCalled](Containers::StringView string) {
        ++callbackCalled;
        CORRADE_VERIFY(string);
    });

    /* The update should trigger the layer to create a data attached to the
       sole node */
    ui.update();
    CORRADE_COMPARE(ui.state(), data.sources >= DebugLayerSource::Nodes ? UserInterfaceState::NeedsDataUpdate : UserInterfaceStates{});
    CORRADE_COMPARE(layer.usedCount(), data.flags >= DebugLayerFlag::NodeInspect ? 1 : 0);

    /* The event should not be accepted, should produce no callback, but should
       fall through to the data under on the same node */
    PointerEvent event{{}, data.pointerSource, data.pointer, data.primary, 0, data.modifiers};
    CORRADE_VERIFY(!ui.pointerPressEvent({50, 50}, event));
    CORRADE_COMPARE(callbackCalled, 0);
    CORRADE_COMPARE(fallbackLayer.called, 1);

    /* If the feature is enabled and we provide a correct gesture, it should
       work. (All test case instances are expected to allow Ctrl+RMB.) */
    if(data.flags >= DebugLayerFlag::NodeInspect) {
        PointerEvent another{{}, PointerEventSource::Mouse, Pointer::MouseRight, true, 0, Modifier::Ctrl};
        CORRADE_VERIFY(ui.pointerPressEvent({50, 50}, another));
        CORRADE_COMPARE(callbackCalled, 1);
        CORRADE_COMPARE(fallbackLayer.called, 2);
    }
}

void DebugLayerTest::nodeInspect() {
    auto&& data = NodeInspectData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui{{100, 100}};

    /* Parents, used optionally to verify different output for root and nested
       nodes */
    NodeHandle parent1 = ui.createNode({20, 10}, {50, 50});
    NodeHandle parent2 = ui.createNode(parent1, {0, 5}, {40, 40});
    NodeHandle parent3 = ui.createNode(parent2, {15, 0}, {25, 35});

    /* The node is at an absolute offset {40, 20} in both cases */
    NodeHandle node = data.nested ?
        ui.createNode(parent3, {5, 10}, {20, 30}) :
        ui.createNode({40, 20}, {20, 30});
    /* If a node isn't nested, it's top-level implicitly, and the test instance
       should reflect that */
    CORRADE_INTERNAL_ASSERT(data.nested || data.nestedTopLevel);
    if(data.nested && data.nestedTopLevel)
        ui.setNodeOrder(node, NodeHandle::Null);
    if(data.nodeFlags)
        ui.setNodeFlags(node, data.nodeFlags);

    /* Node with no attachments just to verify switching to another one works
       as well */
    NodeHandle another = ui.createNode({70, 80}, {20, 20});

    /* Children, used optionally to verify different output with hierarchy
       enabled but no children */
    NodeHandle removedChild = NodeHandle::Null;
    if(data.children) {
        /*NodeHandle child1 =*/ ui.createNode(node, {}, {});
        NodeHandle child2 = ui.createNode(node, {}, {});
        NodeHandle child3 = ui.createNode(node, {}, {});
        NodeHandle child4 = ui.createNode(node, {}, {},
            data.hiddenChildren ? NodeFlag::Hidden : NodeFlags{});
        /* A child that gets removed and thus shouldn't be counted due to some
           stale state making it look like it's still parented to `node` */
        removedChild = ui.createNode(node, {}, {});
        /* Hidden takes a precedence over Disabled / NoEvents, so 3 are listed
           as Hidden. Disabled then takes a precedence over NoEvents so 2 are
           listed as Disabled, and just 1 as NoEvents. */
        /*NodeHandle child5 =*/ ui.createNode(node, {}, {},
            (data.hiddenChildren ? NodeFlag::Hidden : NodeFlags{})|
            (data.disabledChildren ? NodeFlag::Disabled : NodeFlags{}));
        /*NodeHandle child6 =*/ ui.createNode(node, {}, {},
            (data.hiddenChildren ? NodeFlag::Hidden : NodeFlags{})|
            (data.noEventsChildren ? NodeFlag::NoEvents : NodeFlags{}));
        NodeHandle child7 = ui.createNode(node, {}, {},
            data.disabledChildren ? NodeFlag::Disabled : NodeFlags{});
        /*NodeHandle child8 =*/ ui.createNode(node, {}, {},
            data.disabledChildren ? NodeFlag::Disabled : NodeFlags{});
        NodeHandle child9 = ui.createNode(node, {}, {},
            data.noEventsChildren ? NodeFlag::NoEvents : NodeFlags{});

        /* Secondary children shouldn't be shown */
        /*NodeHandle child21 =*/ ui.createNode(child2, {}, {});
        /*NodeHandle child22 =*/ ui.createNode(child2, {}, {});
        /*NodeHandle child31 =*/ ui.createNode(child3, {}, {});

        /* Children of the hidden, disabled, ... shouldn't be shown either */
        /*NodeHandle child41 =*/ ui.createNode(child4, {}, {});
        /*NodeHandle child71 =*/ ui.createNode(child7, {}, {});
        /*NodeHandle child91 =*/ ui.createNode(child9, {}, {});
    }

    /* The layers should always be printed in the draw order, regardless of the
       order they were created in */
    LayerHandle layers[7];
    if(!data.reverseLayerLayouterOrder) {
        layers[0] = ui.createLayer();
        layers[1] = ui.createLayer();
        layers[2] = ui.createLayer();
        layers[3] = ui.createLayer();
        layers[4] = ui.createLayer();
        layers[5] = ui.createLayer(); /* doesn't have any instance set */
        layers[6] = ui.createLayer();
    } else {
        layers[6] = ui.createLayer();
        layers[5] = ui.createLayer(layers[6]); /* doesn't have any instance set */
        layers[4] = ui.createLayer(layers[5]);
        layers[3] = ui.createLayer(layers[4]);
        layers[2] = ui.createLayer(layers[3]);
        layers[1] = ui.createLayer(layers[2]);
        layers[0] = ui.createLayer(layers[1]);
    }

    /* Add layers before ... */
    struct EmptyLayer: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override { return {}; }
    };
    EmptyLayer& emptyLayer1 = ui.setLayerInstance(Containers::pointer<EmptyLayer>(layers[0]));
    emptyLayer1.create(node);
    emptyLayer1.create(node);
    emptyLayer1.create(node);
    EmptyLayer& emptyLayer2 = ui.setLayerInstance(Containers::pointer<EmptyLayer>(layers[1]));
    emptyLayer2.create(node);

    /* ... a layer that gets subsequently removed and thus data from it
       shouldn't be counted, neither the name should be used ... */
    EmptyLayer& removedLayer = ui.setLayerInstance(Containers::pointer<EmptyLayer>(layers[2]));
    removedLayer.create(node);
    removedLayer.create(node);

    DebugLayer& layer = ui.setLayerInstance(Containers::pointer<DebugLayer>(layers[3], data.sources, data.flags));

    /* ... and also after, to make sure these are shown even if DebugLayer
       isn't last. The integrated layer has its own debug printer. */
    struct IntegratedLayer: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        struct DebugIntegration {
            /* This is deliberately *not* passing the name via const& to verify
               that the signature doesn't have to match exactly */
            void print(Debug& debug, const IntegratedLayer& layer, Containers::StringView layerName, LayerDataHandle data) {
                /* Printing the name colored to verify the color is disabled
                   correctly here as well */
                debug << "  Layer" << Debug::color(Debug::Color::Yellow) << layerName << Debug::resetColor << "(" << Debug::nospace << layer.value << Debug::nospace << ") data" << Debug::packed << data << "and a value of" << value << Debug::newline;
            }

            int value = 1337;
        };

        LayerFeatures doFeatures() const override { return {}; }

        int value = 42069;
    };
    IntegratedLayer& integratedLayer = ui.setLayerInstance(Containers::pointer<IntegratedLayer>(layers[4]));
    integratedLayer.create(node);
    integratedLayer.create(node);
    /* layers[5] has no instance */
    /* This layer is associated with an animator but data animator contents
       aren't reflected in the output so far */
    EmptyLayer& emptyLayer3 = ui.setLayerInstance(Containers::pointer<EmptyLayer>(layers[6]));
    emptyLayer3.create(node);
    DataHandle emptyLayer3Data1 = emptyLayer3.create(node);
    DataHandle emptyLayer3Data2 = emptyLayer3.create(node);
    emptyLayer3.create(node);

    /* The layouters should always be printed in the draw order, regardless of
       the order they were created in */
    LayouterHandle layouters[6];
    if(!data.reverseLayerLayouterOrder) {
        layouters[0] = ui.createLayouter();
        layouters[1] = ui.createLayouter();
        layouters[2] = ui.createLayouter();
        layouters[3] = ui.createLayouter();
        layouters[4] = ui.createLayouter(); /* doesn't have any instance set */
        layouters[5] = ui.createLayouter();
    } else {
        layouters[5] = ui.createLayouter();
        layouters[4] = ui.createLayouter(layouters[5]); /* doesn't have any instance set */
        layouters[3] = ui.createLayouter(layouters[4]);
        layouters[2] = ui.createLayouter(layouters[3]);
        layouters[1] = ui.createLayouter(layouters[2]);
        layouters[0] = ui.createLayouter(layouters[1]);
    }

    struct EmptyLayouter: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::add;

        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&) override {}
    };
    struct IntegratedLayouter: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::add;

        struct DebugIntegration {
            /* This is deliberately *not* passing the name via const& to verify
               that the signature doesn't have to match exactly */
            void print(Debug& debug, const IntegratedLayouter& layouter, Containers::StringView layouterName, LayouterDataHandle data) {
                /* Printing the name colored to verify the color is disabled
                   correctly here as well */
                debug << "  Layouter" << Debug::color(Debug::Color::Yellow) << layouterName << Debug::resetColor << "(" << Debug::nospace << layouter.value << Debug::nospace << ") layout" << Debug::packed << data << "and a value of" << value << Debug::newline;
            }

            int value = 7331;
        };

        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&) override {}

        int value = 96024;
    };

    EmptyLayouter& emptyLayouter1 = ui.setLayouterInstance(Containers::pointer<EmptyLayouter>(layouters[0]));
    emptyLayouter1.add(node);
    emptyLayouter1.add(node);
    emptyLayouter1.add(node);
    EmptyLayouter& emptyLayouter2 = ui.setLayouterInstance(Containers::pointer<EmptyLayouter>(layouters[1]));
    emptyLayouter2.add(node);
    /* Layouter that gets subsequently removed and thus data from it shouldn't
       be counted, neither the name should be used */
    EmptyLayouter& removedLayouter = ui.setLayouterInstance(Containers::pointer<EmptyLayouter>(layouters[2]));
    removedLayouter.add(node);
    removedLayouter.add(node);
    IntegratedLayouter& integratedLayouter = ui.setLayouterInstance(Containers::pointer<IntegratedLayouter>(layouters[3]));
    integratedLayouter.add(node);
    integratedLayouter.add(node);
    /* layouters[4] has no instance */
    EmptyLayouter& emptyLayouter3 = ui.setLayouterInstance(Containers::pointer<EmptyLayouter>(layouters[5]));
    emptyLayouter3.add(node);
    emptyLayouter3.add(node);
    emptyLayouter3.add(node);
    emptyLayouter3.add(node);

    struct EmptyNodeAnimator: AbstractNodeAnimator {
        using AbstractNodeAnimator::AbstractNodeAnimator;
        using AbstractNodeAnimator::create;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::NodeAttachment;
        }
        NodeAnimatorUpdates doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Float>&, const Containers::StridedArrayView1D<NodeFlags>&, Containers::MutableBitArrayView) override {
            return {};
        }
    };
    struct EmptyDataAnimator: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;
        using AbstractGenericAnimator::create;
        using AbstractGenericAnimator::setLayer;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::DataAttachment;
        }
        void doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
    };
    struct IntegratedAnimator: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;
        using AbstractGenericAnimator::create;

        struct DebugIntegration {
            /* This is deliberately *not* passing the name via const& to verify
               that the signature doesn't have to match exactly */
            void print(Debug& debug, const IntegratedAnimator& animator, Containers::StringView animatorName, AnimatorDataHandle animation) {
                /* Printing the name colored to verify the color is disabled
                   correctly here as well */
                debug << "  Animator" << Debug::color(Debug::Color::Yellow) << animatorName << Debug::resetColor << "(" << Debug::nospace << animator.value << Debug::nospace << ")" << Debug::color(Debug::Color::Cyan) << Debug::packed << animator.state(animation) << Debug::resetColor << "animation" << Debug::packed << animation << "and a value of" << value << Debug::newline;
            }

            int value = 1226;
        };

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::NodeAttachment;
        }
        void doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}

        int value = 69420;
    };

    /* Animators are always printed in the handle ID order */
    EmptyNodeAnimator& emptyAnimator1 = ui.setNodeAnimatorInstance(Containers::pointer<EmptyNodeAnimator>(ui.createAnimator()));
    emptyAnimator1.create(50_nsec, 10_nsec, node); /* scheduled */
    emptyAnimator1.create(-50_nsec, 10_nsec, node, AnimationFlag::KeepOncePlayed); /* stopped */
    emptyAnimator1.create(-50_nsec, 10_nsec, node, AnimationFlag::KeepOncePlayed); /* stopped */
    emptyAnimator1.create(Nanoseconds::max(), 10_nsec, node, AnimationFlag::KeepOncePlayed); /* reserved */
    EmptyNodeAnimator& emptyAnimator2 = ui.setNodeAnimatorInstance(Containers::pointer<EmptyNodeAnimator>(ui.createAnimator()));
    emptyAnimator2.create(50_nsec, 10_nsec, node); /* scheduled */
    /* Animator that gets subsequently removed and replaced with another, and
       thus data from it shouldn't be counted, neither the name should be
       used ... */
    EmptyNodeAnimator& removedAnimator1 = ui.setNodeAnimatorInstance(Containers::pointer<EmptyNodeAnimator>(ui.createAnimator()));
    removedAnimator1.create(50_nsec, 10_nsec, node);
    removedAnimator1.create(50_nsec, 10_nsec, node);
    /* Animator that gets subsequently removed but not replaced with another,
       so its slot should get skipped as invalid */
    EmptyNodeAnimator& removedAnimator2 = ui.setNodeAnimatorInstance(Containers::pointer<EmptyNodeAnimator>(ui.createAnimator()));
    removedAnimator2.create(50_nsec, 10_nsec, node);
    removedAnimator2.create(50_nsec, 10_nsec, node);
    /* Animator without an instance */
    /*AnimatorHandle instancelessAnimator =*/ ui.createAnimator();
    IntegratedAnimator& integratedAnimator = ui.setGenericAnimatorInstance(Containers::pointer<IntegratedAnimator>(ui.createAnimator()));
    integratedAnimator.create(-1_nsec, 10_nsec, node); /* playing */
    integratedAnimator.create(-1_nsec, 10_nsec, node); /* playing */
    AnimationHandle integratedAnimatorPaused = integratedAnimator.create(-1_nsec, 10_nsec, node); /* paused */
    integratedAnimator.pause(integratedAnimatorPaused, -25_nsec);
    /* Animator that has data attachments. The data are attached to the node
       but so far there's nothing that'd make them show. */
    Containers::Pointer<EmptyDataAnimator> dataAnimatorInstance{InPlaceInit, ui.createAnimator()};
    dataAnimatorInstance->setLayer(emptyLayer3);
    EmptyDataAnimator& dataAnimator = ui.setGenericAnimatorInstance(Utility::move(dataAnimatorInstance));
    dataAnimator.create(50_nsec, 10_nsec, emptyLayer3Data1);
    dataAnimator.create(50_nsec, 10_nsec, emptyLayer3Data2);
    EmptyNodeAnimator& emptyAnimator3 = ui.setNodeAnimatorInstance(Containers::pointer<EmptyNodeAnimator>(ui.createAnimator()));
    emptyAnimator3.create(-1_nsec, 10_nsec, node); /* playing */
    emptyAnimator3.create(-50_nsec, 10_nsec, node, AnimationFlag::KeepOncePlayed); /* stopped */

    Int called = 0;
    Containers::String out;
    const std::ostream* const defaultOutput = Debug::output();
    layer.setNodeInspectCallback([&out, defaultOutput, &called](Containers::StringView message) {
        /* There should be no output redirection anymore when calling this
           function */
        CORRADE_COMPARE(Debug::output(), defaultOutput);
        if(message) {
            CORRADE_COMPARE(message.flags(), Containers::StringViewFlag::NullTerminated);
            CORRADE_COMPARE(message[message.size()], '\0');
        }
        out = message;
        ++called;
    });

    if(data.someLayerLayouterAnimatorNames) {
        layer.setLayerName(emptyLayer2, "Second");
        layer.setLayerName(removedLayer, "Removed");
        layer.setLayerName(integratedLayer, "No.3");
        layer.setLayouterName(emptyLayouter2, "Supplementary");
        layer.setLayouterName(removedLayouter, "Removed");
        layer.setLayouterName(integratedLayouter, "Tertiary");
        layer.setAnimatorName(emptyAnimator2, "2nd");
        layer.setAnimatorName(removedAnimator1, "Removed");
        layer.setAnimatorName(dataAnimator, "Data");
        layer.setAnimatorName(integratedAnimator, "No#3");
    }
    if(data.allLayerLayouterAnimatorNames) {
        layer.setLayerName(emptyLayer1, "A layer");
        layer.setLayerName(emptyLayer3, "The last ever");
        layer.setLayouterName(emptyLayouter1, "Primary");
        layer.setLayouterName(emptyLayouter3, "Fallback");
        layer.setAnimatorName(emptyAnimator1, "An animator");
        layer.setAnimatorName(emptyAnimator3, "Termanimator");
    }
    if(data.acceptedPointers)
        layer.setNodeInspectGesture(data.acceptedPointers, data.acceptedModifiers);
    if(data.nodeName)
        layer.setNodeName(node, *data.nodeName);
    /* No node is inspected by default */
    CORRADE_COMPARE(layer.currentInspectedNode(), NodeHandle::Null);

    /* Update to trigger DebugLayer population */
    ui.update();

    /* Remove the layer, animator and child node after all DebugLayer setup;
       add layers, animators and nodes that aren't yet known by it and should
       thus be skipped */
    ui.removeLayer(removedLayer.handle());
    ui.removeLayouter(removedLayouter.handle());
    ui.removeAnimator(removedAnimator1.handle());
    if(removedChild != NodeHandle::Null)
        ui.removeNode(removedChild);
    /* This one is in place of removedChild */
    NodeHandle unknownNode1 = ui.createNode(node, {}, {});
    /* This one is new */
    NodeHandle unknownNode2 = ui.createNode(node, {}, {});
    /* These are is in place of removedLayer / removedLayouter /
       removedAnimator */
    EmptyLayer& unknownLayer1 = ui.setLayerInstance(Containers::pointer<EmptyLayer>(ui.createLayer()));
    EmptyLayouter& unknownLayouter1 = ui.setLayouterInstance(Containers::pointer<EmptyLayouter>(ui.createLayouter()));
    EmptyNodeAnimator& unknownAnimator1 = ui.setNodeAnimatorInstance(Containers::pointer<EmptyNodeAnimator>(ui.createAnimator()));
    unknownLayer1.create(node);
    unknownLayouter1.add(node);
    unknownAnimator1.create(50_nsec, 1_nsec, node);
    /* These are new */
    EmptyLayer& unknownLayer2 = ui.setLayerInstance(Containers::pointer<EmptyLayer>(ui.createLayer()));
    EmptyLayouter& unknownLayouter2 = ui.setLayouterInstance(Containers::pointer<EmptyLayouter>(ui.createLayouter()));
    EmptyNodeAnimator& unknownAnimator2 = ui.setNodeAnimatorInstance(Containers::pointer<EmptyNodeAnimator>(ui.createAnimator()));
    unknownLayer2.create(node);
    unknownLayouter2.add(node);
    unknownAnimator2.create(50_nsec, 1_nsec, node);
    /* Remove the other animator after adding others so there's a slot with an
       invalid handle */
    ui.removeAnimator(removedAnimator2.handle());

    /* Inspecting a Null node if nothing is inspected does nothing but returns
       true, as that's a valid scenario */
    CORRADE_VERIFY(layer.inspectNode(NodeHandle::Null));
    CORRADE_COMPARE(layer.currentInspectedNode(), NodeHandle::Null);
    CORRADE_COMPARE(called, 0);

    /* Inspecting a known node ID but with an invalid generation if nothing is
       inspected does nothing and returns false; same for ID clearly out of
       bounds */
    CORRADE_VERIFY(!layer.inspectNode(nodeHandle(nodeHandleId(node), nodeHandleGeneration(node) + 1)));
    CORRADE_VERIFY(!layer.inspectNode(nodeHandle(100000, 1)));
    CORRADE_COMPARE(layer.currentInspectedNode(), NodeHandle::Null);
    CORRADE_COMPARE(called, 0);

    /* Inspect the main node */
    CORRADE_VERIFY(layer.inspectNode(node));
    CORRADE_COMPARE(layer.currentInspectedNode(), node);
    CORRADE_COMPARE(called, 1);
    CORRADE_COMPARE_AS(out, data.expected, TestSuite::Compare::String);

    /* Inspecting it again does exactly the same (doesn't remove the
       highlight) */
    out = {};
    CORRADE_VERIFY(layer.inspectNode(node));
    CORRADE_COMPARE(layer.currentInspectedNode(), node);
    CORRADE_COMPARE(called, 2);
    CORRADE_COMPARE_AS(out, data.expected, TestSuite::Compare::String);

    /* Inspecting another node */
    Containers::String anotherExpected = "\n"_s.joinWithoutEmptyParts({
        "Top-level node {0x4, 0x1}",
        data.sources >= DebugLayerSource::NodeOffsetSize ?
            "  Offset: {70, 80}, size: {20, 20}" : "",
        data.sources >= DebugLayerSource::NodeHierarchy ?
            "  Root node with 0 direct children" : ""
    });
    out = {};
    CORRADE_VERIFY(layer.inspectNode(another));
    CORRADE_COMPARE(layer.currentInspectedNode(), another);
    CORRADE_COMPARE(called, 3);
    CORRADE_COMPARE_AS(out, anotherExpected, TestSuite::Compare::String);

    /* Inspecting Null removes the highlight and fires the callback with an
       empty string. Deliberately setting out to non-empty to verify that it
       gets emptied. */
    out = "this gonna be replaced";
    CORRADE_VERIFY(layer.inspectNode(NodeHandle::Null));
    CORRADE_COMPARE(layer.currentInspectedNode(), NodeHandle::Null);
    CORRADE_COMPARE(called, 4);
    CORRADE_COMPARE(out, "");

    /* Inspecting invalid node with another node inspected behaves almost the
       same, except that the function returns false. Again deliberately setting
       out to non-empty to verify that it gets emptied. */
    CORRADE_VERIFY(layer.inspectNode(another));
    CORRADE_COMPARE(layer.currentInspectedNode(), another);
    out = "this gonna be replaced";
    CORRADE_VERIFY(!layer.inspectNode(nodeHandle(100000, 1)));
    CORRADE_COMPARE(layer.currentInspectedNode(), NodeHandle::Null);
    CORRADE_COMPARE(called, 6); /* inspectNode() called twice in this case */
    CORRADE_COMPARE(out, "");

    /* The events implicitly call update(), meaning that the yet-unknown nodes,
       layers and animators will become known now. Remove them to have the same
       output as above. */
    ui.removeNode(unknownNode1);
    ui.removeNode(unknownNode2);
    ui.removeLayer(unknownLayer1.handle());
    ui.removeLayer(unknownLayer2.handle());
    ui.removeLayouter(unknownLayouter1.handle());
    ui.removeLayouter(unknownLayouter2.handle());
    ui.removeAnimator(unknownAnimator1.handle());
    ui.removeAnimator(unknownAnimator2.handle());

    /* Inspect the node by an event */
    out = {};
    PointerEvent press1{{}, data.pointerSource, data.pointer, true, 0, data.acceptedPointers ? data.acceptedModifiers : Modifier::Ctrl};
    CORRADE_VERIFY(ui.pointerPressEvent({45, 35}, press1));
    CORRADE_COMPARE(layer.currentInspectedNode(), node);
    CORRADE_COMPARE(called, 7);
    CORRADE_COMPARE_AS(out, data.expected, TestSuite::Compare::String);

    /* Inspect another node by an event */
    out = {};
    PointerEvent press2{{}, data.pointerSource, data.pointer, true, 0, data.acceptedPointers ? data.acceptedModifiers : Modifier::Ctrl};
    CORRADE_VERIFY(ui.pointerPressEvent({80, 90}, press2));
    CORRADE_COMPARE(layer.currentInspectedNode(), another);
    CORRADE_COMPARE(called, 8);
    CORRADE_COMPARE_AS(out, anotherExpected, TestSuite::Compare::String);

    /* Clicking completely outside of anything doesn't remove the highlight (as
       there's no way to do that, apart from temporarily making the node
       focusable and focused, which would interfere with styling) */
    out = "this is gonna stay";
    PointerEvent press3{{}, data.pointerSource, data.pointer, true, 0, data.acceptedPointers ? data.acceptedModifiers : Modifier::Ctrl};
    CORRADE_VERIFY(!ui.pointerPressEvent({100, 100}, press3));
    CORRADE_COMPARE(layer.currentInspectedNode(), another);
    CORRADE_COMPARE(called, 8);
    CORRADE_COMPARE(out, "this is gonna stay");

    /* Clicking on the node again removes the highlight, causing the callback
       to be called with an empty string. Deliberately setting out to non-empty
       to verify that it gets emptied. */
    out = "this gonna be replaced";
    PointerEvent press4{{}, data.pointerSource, data.pointer, true, 0, data.acceptedPointers ? data.acceptedModifiers : Modifier::Ctrl};
    CORRADE_VERIFY(ui.pointerPressEvent({80, 90}, press4));
    CORRADE_COMPARE(layer.currentInspectedNode(), NodeHandle::Null);
    CORRADE_COMPARE(called, 9);
    CORRADE_COMPARE(out, "");
}

void DebugLayerTest::nodeInspectNoCallback() {
    /* A trimmed down variant of nodeInspect() verifying behavior without a
       callback and for visual color verification */

    AbstractUserInterface ui{{100, 100}};

    NodeHandle parent1 = ui.createNode({20, 10}, {50, 50});
    NodeHandle parent2 = ui.createNode(parent1, {0, 5}, {40, 40});
    NodeHandle parent3 = ui.createNode(parent2, {15, 0}, {25, 35});
    NodeHandle node = ui.createNode(parent3, {5, 10}, {20, 30});
    NodeHandle another = ui.createNode({70, 80}, {20, 20});
    ui.setNodeOrder(node, NodeHandle::Null);
    ui.setNodeFlags(node, NodeFlag::Clip|NodeFlag::Focusable);

    /*NodeHandle child1 =*/ ui.createNode(node, {}, {});
    /*NodeHandle child2 =*/ ui.createNode(node, {}, {});
    /*NodeHandle child3 =*/ ui.createNode(node, {}, {});
    /*NodeHandle child4 =*/ ui.createNode(node, {}, {}, NodeFlag::Hidden);
    /*NodeHandle child5 =*/ ui.createNode(node, {}, {}, NodeFlag::Hidden);
    /*NodeHandle child6 =*/ ui.createNode(node, {}, {}, NodeFlag::Hidden);
    /*NodeHandle child7 =*/ ui.createNode(node, {}, {}, NodeFlag::Disabled);
    /*NodeHandle child8 =*/ ui.createNode(node, {}, {}, NodeFlag::Disabled);
    /*NodeHandle child9 =*/ ui.createNode(node, {}, {}, NodeFlag::NoEvents);

    struct EmptyLayer: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override { return {}; }
    };
    EmptyLayer& emptyLayer1 = ui.setLayerInstance(Containers::pointer<EmptyLayer>(ui.createLayer()));
    emptyLayer1.create(node);
    emptyLayer1.create(node);
    emptyLayer1.create(node);
    EmptyLayer& emptyLayer2 = ui.setLayerInstance(Containers::pointer<EmptyLayer>(ui.createLayer()));
    emptyLayer2.create(node);

    /* Just to match the layer handles to the nodeInspect() case */
    /*LayerHandle removedLayer =*/ ui.createLayer();

    DebugLayer& layer = ui.setLayerInstance(Containers::pointer<DebugLayer>(ui.createLayer(), DebugLayerSource::NodeOffsetSize|DebugLayerSource::NodeHierarchy|DebugLayerSource::NodeDataDetails|DebugLayerSource::NodeLayoutDetails|DebugLayerSource::NodeAnimationDetails, DebugLayerFlag::NodeInspect));

    struct IntegratedLayer: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        struct DebugIntegration {
            void print(Debug& debug, const IntegratedLayer& layer, const Containers::StringView& layerName, LayerDataHandle data) {
                /* Printing the name colored to verify the color is used when
                   printing directly to the output and not when not */
                debug << "  Layer" << Debug::color(Debug::Color::Yellow) << layerName << Debug::resetColor << "(" << Debug::nospace << layer.value << Debug::nospace << ") data" << Debug::packed << data << "and a value of" << value << Debug::newline;
            }

            int value = 1337;
        };

        LayerFeatures doFeatures() const override { return {}; }

        int value = 42069;
    };
    IntegratedLayer& integratedLayer = ui.setLayerInstance(Containers::pointer<IntegratedLayer>(ui.createLayer()));
    integratedLayer.create(node);
    integratedLayer.create(node);
    EmptyLayer& emptyLayer3 = ui.setLayerInstance(Containers::pointer<EmptyLayer>(ui.createLayer()));
    emptyLayer3.create(node);
    emptyLayer3.create(node);
    emptyLayer3.create(node);
    emptyLayer3.create(node);

    struct EmptyLayouter: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::add;

        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&) override {}
    };
    struct IntegratedLayouter: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::add;

        struct DebugIntegration {
            /* This is deliberately *not* passing the name via const& to verify
               that the signature doesn't have to match exactly */
            void print(Debug& debug, const IntegratedLayouter& layouter, Containers::StringView layouterName, LayouterDataHandle data) {
                /* Printing the name colored to verify the color is disabled
                   correctly here as well */
                debug << "  Layouter" << Debug::color(Debug::Color::Yellow) << layouterName << Debug::resetColor << "(" << Debug::nospace << layouter.value << Debug::nospace << ") layout" << Debug::packed << data << "and a value of" << value << Debug::newline;
            }

            int value = 7331;
        };

        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&) override {}

        int value = 96024;
    };

    EmptyLayouter& emptyLayouter1 = ui.setLayouterInstance(Containers::pointer<EmptyLayouter>(ui.createLayouter()));
    emptyLayouter1.add(node);
    emptyLayouter1.add(node);
    emptyLayouter1.add(node);
    EmptyLayouter& emptyLayouter2 = ui.setLayouterInstance(Containers::pointer<EmptyLayouter>(ui.createLayouter()));
    emptyLayouter2.add(node);
    /* Layouter that was removed / w/o an instance in nodeInspect() above */
    ui.createLayouter();
    IntegratedLayouter& integratedLayouter = ui.setLayouterInstance(Containers::pointer<IntegratedLayouter>(ui.createLayouter()));
    integratedLayouter.add(node);
    integratedLayouter.add(node);
    /* Layouter that didn't have instance in nodeInspect() above */
    ui.createLayouter();
    EmptyLayouter& emptyLayouter3 = ui.setLayouterInstance(Containers::pointer<EmptyLayouter>(ui.createLayouter()));
    emptyLayouter3.add(node);
    emptyLayouter3.add(node);
    emptyLayouter3.add(node);
    emptyLayouter3.add(node);

    struct EmptyNodeAnimator: AbstractNodeAnimator {
        using AbstractNodeAnimator::AbstractNodeAnimator;
        using AbstractNodeAnimator::create;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::NodeAttachment;
        }
        NodeAnimatorUpdates doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Float>&, const Containers::StridedArrayView1D<NodeFlags>&, Containers::MutableBitArrayView) override {
            return {};
        }
    };
    struct IntegratedAnimator: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;
        using AbstractGenericAnimator::create;

        struct DebugIntegration {
            /* This is deliberately *not* passing the name via const& to verify
               that the signature doesn't have to match exactly */
            void print(Debug& debug, const IntegratedAnimator& animator, Containers::StringView animatorName, AnimatorDataHandle animation) {
                /* Printing the name colored to verify the color is disabled
                   correctly here as well */
                debug << "  Animator" << Debug::color(Debug::Color::Yellow) << animatorName << Debug::resetColor << "(" << Debug::nospace << animator.value << Debug::nospace << ")" << Debug::color(Debug::Color::Cyan) << Debug::packed << animator.state(animation) << Debug::resetColor << "animation" << Debug::packed << animation << "and a value of" << value << Debug::newline;
            }

            int value = 1226;
        };

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::NodeAttachment;
        }
        void doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}

        int value = 69420;
    };
    EmptyNodeAnimator& emptyAnimator1 = ui.setNodeAnimatorInstance(Containers::pointer<EmptyNodeAnimator>(ui.createAnimator()));
    emptyAnimator1.create(50_nsec, 10_nsec, node); /* scheduled */
    emptyAnimator1.create(-50_nsec, 10_nsec, node, AnimationFlag::KeepOncePlayed); /* stopped */
    emptyAnimator1.create(-50_nsec, 10_nsec, node, AnimationFlag::KeepOncePlayed); /* stopped */
    emptyAnimator1.create(Nanoseconds::max(), 10_nsec, node, AnimationFlag::KeepOncePlayed); /* reserved */
    EmptyNodeAnimator& emptyAnimator2 = ui.setNodeAnimatorInstance(Containers::pointer<EmptyNodeAnimator>(ui.createAnimator()));
    emptyAnimator2.create(50_nsec, 10_nsec, node); /* scheduled */
    /* Animators that were removed / w/o an instance in nodeInspect() above */
    ui.createAnimator();
    ui.createAnimator();
    ui.createAnimator();
    IntegratedAnimator& integratedAnimator = ui.setGenericAnimatorInstance(Containers::pointer<IntegratedAnimator>(ui.createAnimator()));
    integratedAnimator.create(-1_nsec, 10_nsec, node); /* playing */
    integratedAnimator.create(-1_nsec, 10_nsec, node); /* playing */
    AnimationHandle integratedAnimatorPaused = integratedAnimator.create(-1_nsec, 10_nsec, node); /* paused */
    integratedAnimator.pause(integratedAnimatorPaused, -25_nsec);
    /* Data animator that was unused in nodeInspect() above */
    ui.createAnimator();
    EmptyNodeAnimator& emptyAnimator3 = ui.setNodeAnimatorInstance(Containers::pointer<EmptyNodeAnimator>(ui.createAnimator()));
    emptyAnimator3.create(-1_nsec, 10_nsec, node); /* playing */
    emptyAnimator3.create(-50_nsec, 10_nsec, node, AnimationFlag::KeepOncePlayed); /* stopped */

    layer.setNodeName(node, "A very nice node");
    layer.setLayerName(emptyLayer2, "Second");
    layer.setLayerName(integratedLayer, "No.3");
    layer.setLayouterName(emptyLayouter2, "Supplementary");
    layer.setLayouterName(integratedLayouter, "Tertiary");
    layer.setAnimatorName(emptyAnimator2, "2nd");
    layer.setAnimatorName(integratedAnimator, "No#3");

    /* Inspect the node and then another unnamed one for visual color
       verification. Using events as they delegate to inspectNode() and thus
       test the whole stack for color output. */
    {
        Debug{} << "======================== visual color verification start =======================";

        layer.addFlags(DebugLayerFlag::ColorAlways);

        PointerEvent press1{{}, PointerEventSource::Mouse, Pointer::MouseRight, true, 0, Modifier::Ctrl};
        PointerEvent press2{{}, PointerEventSource::Mouse, Pointer::MouseRight, true, 0, Modifier::Ctrl};
        CORRADE_VERIFY(ui.pointerPressEvent({45, 35}, press1));
        CORRADE_COMPARE(layer.currentInspectedNode(), node);
        CORRADE_VERIFY(ui.pointerPressEvent({80, 90}, press2));
        CORRADE_COMPARE(layer.currentInspectedNode(), another);

        layer.clearFlags(DebugLayerFlag::ColorAlways);

        Debug{} << "======================== visual color verification end =========================";
    }

    /* Do the same, but this time with output redirection to verify the
       contents. The internals automatically disable coloring if they detect
       the output isn't a TTY. */
    {
        Containers::String out;
        Debug redirectOutput{&out};
        PointerEvent press{{}, PointerEventSource::Mouse, Pointer::MouseRight, true, 0, Modifier::Ctrl};
        CORRADE_VERIFY(ui.pointerPressEvent({45, 35}, press));
        CORRADE_COMPARE(layer.currentInspectedNode(), node);
        /* The output always has a newline at the end which cannot be disabled
           so strip it here to have the comparison match the nodeInspect()
           case */
        CORRADE_COMPARE_AS(out,
            "\n",
            TestSuite::Compare::StringHasSuffix);
        CORRADE_COMPARE_AS(out.exceptSuffix("\n"),
            Containers::arrayView(NodeInspectData).back().expected,
            TestSuite::Compare::String);
    } {
        Containers::String out;
        Debug redirectOutput{&out};
        PointerEvent press{{}, PointerEventSource::Mouse, Pointer::MouseRight, true, 0, Modifier::Ctrl};
        CORRADE_VERIFY(ui.pointerPressEvent({80, 90}, press));
        CORRADE_COMPARE(layer.currentInspectedNode(), another);
        CORRADE_COMPARE_AS(out,
            "Top-level node {0x4, 0x1}\n"
            "  Offset: {70, 80}, size: {20, 20}\n"
            "  Root node with 0 direct children\n",
            TestSuite::Compare::String);

    /* Clicking the inspected node again removes the highlight, and nothing
       gets printed */
    } {
        Containers::String out;
        Debug redirectOutput{&out};
        PointerEvent press{{}, PointerEventSource::Mouse, Pointer::MouseRight, true, 0, Modifier::Ctrl};
        CORRADE_VERIFY(ui.pointerPressEvent({80, 90}, press));
        CORRADE_COMPARE(layer.currentInspectedNode(), NodeHandle::Null);
        CORRADE_COMPARE(out, "");
    }

    /* The same again, but with inspectNode() instead of events */
    {
        Containers::String out;
        Debug redirectOutput{&out};
        CORRADE_VERIFY(layer.inspectNode(node));
        CORRADE_COMPARE(layer.currentInspectedNode(), node);
        /* The output always has a newline at the end which cannot be disabled
           so strip it here to have the comparison match the nodeInspect()
           case */
        CORRADE_COMPARE_AS(out,
            "\n",
            TestSuite::Compare::StringHasSuffix);
        CORRADE_COMPARE_AS(out.exceptSuffix("\n"),
            Containers::arrayView(NodeInspectData).back().expected,
            TestSuite::Compare::String);
    } {
        Containers::String out;
        Debug redirectOutput{&out};
        CORRADE_VERIFY(layer.inspectNode(another));
        CORRADE_COMPARE(layer.currentInspectedNode(), another);
        CORRADE_COMPARE_AS(out,
            "Top-level node {0x4, 0x1}\n"
            "  Offset: {70, 80}, size: {20, 20}\n"
            "  Root node with 0 direct children\n",
            TestSuite::Compare::String);

    /* Passing Null removes the highlight, and nothing gets printed */
    } {
        Containers::String out;
        Debug redirectOutput{&out};
        CORRADE_VERIFY(layer.inspectNode(NodeHandle::Null));
        CORRADE_COMPARE(layer.currentInspectedNode(), NodeHandle::Null);
        CORRADE_COMPARE(out, "");
    }
}

void DebugLayerTest::nodeInspectLayerDebugIntegrationExplicit() {
    /* Implicit integration tested in nodeInspect() above, this verifies that
       the explicitly passed instance does the right thing as well */

    AbstractUserInterface ui{{100, 100}};

    NodeHandle node = ui.createNode({40, 20}, {20, 30});

    struct IntegratedLayer: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        struct DebugIntegration {
            explicit DebugIntegration(int value): value{value} {}

            /* Compared to noodeInspect(), here the signature does match */
            void print(Debug& debug, const IntegratedLayer& layer, const Containers::StringView& layerName, LayerDataHandle data) {
                /* Printing the name colored to verify the color is disabled
                   correctly here as well */
                debug << "  Layer" << Debug::color(Debug::Color::Yellow) << layerName << Debug::resetColor << "(" << Debug::nospace << layer.value << Debug::nospace << ") data" << Debug::packed << data << "and a value of" << value << Debug::newline;
            }

            int value;
        };

        LayerFeatures doFeatures() const override { return {}; }

        int value = 42069;
    };
    IntegratedLayer& integratedLayer = ui.setLayerInstance(Containers::pointer<IntegratedLayer>(ui.createLayer()));
    integratedLayer.create(node);
    integratedLayer.create(node);

    DebugLayer& layer = ui.setLayerInstance(Containers::pointer<DebugLayer>(ui.createLayer(), DebugLayerSource::NodeDataDetails, DebugLayerFlag::NodeInspect));

    IntegratedLayer::DebugIntegration integration{1337};
    layer.setLayerName(integratedLayer, "No.2", integration);

    ui.update();

    Containers::String out;
    {
        Debug redirectOutput{&out};
        CORRADE_VERIFY(layer.inspectNode(node));
    }
    CORRADE_COMPARE(layer.currentInspectedNode(), node);
    CORRADE_COMPARE_AS(out,
        "Top-level node {0x0, 0x1}\n"
        "  Layer No.2 (42069) data {0x0, 0x1} and a value of 1337\n"
        "  Layer No.2 (42069) data {0x1, 0x1} and a value of 1337\n",
        TestSuite::Compare::String);
}

void DebugLayerTest::nodeInspectLayerDebugIntegrationExplicitRvalue() {
    /* Like nodeInspectLayerDebugIntegrationExplicit(), but passing a
       move-only instance */

    AbstractUserInterface ui{{100, 100}};

    NodeHandle node = ui.createNode({40, 40}, {20, 20});

    /* Compared to nodeInspect() and nodeInspectLayerDebugIntegrationExplicit()
       here the whole DebugIntegration type is defined in a base class which
       should also be fine */
    struct IntegratedLayerBase: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        struct DebugIntegration {
            explicit DebugIntegration(int value): value{value} {}

            DebugIntegration(const DebugIntegration&) = delete;
            DebugIntegration(DebugIntegration&&) noexcept = default;
            DebugIntegration& operator=(const DebugIntegration&) = delete;
            /* Clang complains this one is unused. I want to avoid weirdness
               that might happen if one of these isn't defined, tho. */
            CORRADE_UNUSED DebugIntegration& operator=(DebugIntegration&&) noexcept = default;

            void print(Debug& debug, const IntegratedLayerBase& layer, Containers::StringView layerName, LayerDataHandle data) {
                /* Printing the name colored to verify the color is disabled
                   correctly here as well */
                debug << "  Layer" << Debug::color(Debug::Color::Yellow) << layerName << Debug::resetColor << "(" << Debug::nospace << layer.value << Debug::nospace << ") data" << Debug::packed << data << "and a value of" << value << Debug::newline;
            }

            int value;
        };

        int value = 42069;
    };
    struct IntegratedLayer: IntegratedLayerBase {
        using IntegratedLayerBase::IntegratedLayerBase;

        LayerFeatures doFeatures() const override { return {}; }
    };
    IntegratedLayer& integratedLayer = ui.setLayerInstance(Containers::pointer<IntegratedLayer>(ui.createLayer()));
    integratedLayer.create(node);
    integratedLayer.create(node);

    DebugLayer& layer = ui.setLayerInstance(Containers::pointer<DebugLayer>(ui.createLayer(), DebugLayerSource::NodeDataDetails, DebugLayerFlag::NodeInspect));

    layer.setLayerName(integratedLayer, "No.2", IntegratedLayer::DebugIntegration{1337});

    ui.update();

    Containers::String out;
    {
        Debug redirectOutput{&out};
        CORRADE_VERIFY(layer.inspectNode(node));
    }
    CORRADE_COMPARE(layer.currentInspectedNode(), node);
    CORRADE_COMPARE_AS(out,
        "Top-level node {0x0, 0x1}\n"
        "  Layer No.2 (42069) data {0x0, 0x1} and a value of 1337\n"
        "  Layer No.2 (42069) data {0x1, 0x1} and a value of 1337\n",
        TestSuite::Compare::String);
}

void DebugLayerTest::nodeInspectLayouterDebugIntegrationExplicit() {
    /* Implicit integration tested in nodeInspect() above, this verifies that
       the explicitly passed instance does the right thing as well */

    AbstractUserInterface ui{{100, 100}};

    NodeHandle node = ui.createNode({40, 20}, {20, 30});

    struct IntegratedLayouter: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::add;

        struct DebugIntegration {
            explicit DebugIntegration(int value): value{value} {}

            /* Compared to noodeInspect(), here the signature does match */
            void print(Debug& debug, const IntegratedLayouter& layouter, const Containers::StringView& layouterName, LayouterDataHandle data) {
                /* Printing the name colored to verify the color is disabled
                   correctly here as well */
                debug << "  Layouter" << Debug::color(Debug::Color::Yellow) << layouterName << Debug::resetColor << "(" << Debug::nospace << layouter.value << Debug::nospace << ") layout" << Debug::packed << data << "and a value of" << value << Debug::newline;
            }

            int value;
        };

        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&) override {}

        int value = 96024;
    };
    IntegratedLayouter& integratedLayouter = ui.setLayouterInstance(Containers::pointer<IntegratedLayouter>(ui.createLayouter()));
    integratedLayouter.add(node);
    integratedLayouter.add(node);

    DebugLayer& layer = ui.setLayerInstance(Containers::pointer<DebugLayer>(ui.createLayer(), DebugLayerSource::NodeLayoutDetails, DebugLayerFlag::NodeInspect));

    IntegratedLayouter::DebugIntegration integration{7331};
    layer.setLayouterName(integratedLayouter, "Secondary", integration);

    ui.update();

    Containers::String out;
    {
        Debug redirectOutput{&out};
        CORRADE_VERIFY(layer.inspectNode(node));
    }
    CORRADE_COMPARE(layer.currentInspectedNode(), node);
    CORRADE_COMPARE_AS(out,
        "Top-level node {0x0, 0x1}\n"
        "  Layouter Secondary (96024) layout {0x0, 0x1} and a value of 7331\n"
        "  Layouter Secondary (96024) layout {0x1, 0x1} and a value of 7331\n",
        TestSuite::Compare::String);
}

void DebugLayerTest::nodeInspectLayouterDebugIntegrationExplicitRvalue() {
    /* Like nodeInspectLayerDebugIntegrationExplicit(), but passing a
       move-only instance */

    AbstractUserInterface ui{{100, 100}};

    NodeHandle node = ui.createNode({40, 40}, {20, 20});

    /* Compared to nodeInspect() and
       nodeInspectLayouterDebugIntegrationExplicit() here the whole
       DebugIntegration type is defined in a base class which should also be
       fine */
    struct IntegratedLayouterBase: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::add;

        struct DebugIntegration {
            explicit DebugIntegration(int value): value{value} {}

            DebugIntegration(const DebugIntegration&) = delete;
            DebugIntegration(DebugIntegration&&) noexcept = default;
            DebugIntegration& operator=(const DebugIntegration&) = delete;
            /* Clang complains this one is unused. I want to avoid weirdness
               that might happen if one of these isn't defined, tho. */
            CORRADE_UNUSED DebugIntegration& operator=(DebugIntegration&&) noexcept = default;

            void print(Debug& debug, const IntegratedLayouterBase& layouter, Containers::StringView layouterName, LayouterDataHandle data) {
                /* Printing the name colored to verify the color is disabled
                   correctly here as well */
                debug << "  Layouter" << Debug::color(Debug::Color::Yellow) << layouterName << Debug::resetColor << "(" << Debug::nospace << layouter.value << Debug::nospace << ") layout" << Debug::packed << data << "and a value of" << value << Debug::newline;
            }

            int value;
        };

        int value = 96024;
    };
    struct IntegratedLayouter: IntegratedLayouterBase {
        using IntegratedLayouterBase::IntegratedLayouterBase;

        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&) override {}
    };
    IntegratedLayouter& integratedLayouter = ui.setLayouterInstance(Containers::pointer<IntegratedLayouter>(ui.createLayouter()));
    integratedLayouter.add(node);
    integratedLayouter.add(node);

    DebugLayer& layer = ui.setLayerInstance(Containers::pointer<DebugLayer>(ui.createLayer(), DebugLayerSource::NodeLayoutDetails, DebugLayerFlag::NodeInspect));

    layer.setLayouterName(integratedLayouter, "Secondary", IntegratedLayouter::DebugIntegration{7331});

    ui.update();

    Containers::String out;
    {
        Debug redirectOutput{&out};
        CORRADE_VERIFY(layer.inspectNode(node));
    }
    CORRADE_COMPARE(layer.currentInspectedNode(), node);
    CORRADE_COMPARE_AS(out,
        "Top-level node {0x0, 0x1}\n"
        "  Layouter Secondary (96024) layout {0x0, 0x1} and a value of 7331\n"
        "  Layouter Secondary (96024) layout {0x1, 0x1} and a value of 7331\n",
        TestSuite::Compare::String);
}

void DebugLayerTest::nodeInspectAnimatorDebugIntegrationExplicit() {
    /* Implicit integration tested in nodeInspect() above, this verifies that
       the explicitly passed instance does the right thing as well */

    AbstractUserInterface ui{{100, 100}};

    NodeHandle node = ui.createNode({40, 20}, {20, 30});

    DebugLayer& layer = ui.setLayerInstance(Containers::pointer<DebugLayer>(ui.createLayer(), DebugLayerSource::NodeAnimationDetails, DebugLayerFlag::NodeInspect));

    struct IntegratedAnimator: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;
        using AbstractGenericAnimator::create;

        struct DebugIntegration {
            explicit DebugIntegration(int value): value{value} {}

            /* This is deliberately *not* passing the name via const& to verify
               that the signature doesn't have to match exactly */
            void print(Debug& debug, const IntegratedAnimator& animator, Containers::StringView animatorName, AnimatorDataHandle animation) {
                /* Printing the name colored to verify the color is disabled
                   correctly here as well */
                debug << "  Animator" << Debug::color(Debug::Color::Yellow) << animatorName << Debug::resetColor << "(" << Debug::nospace << animator.value << Debug::nospace << ")" << Debug::color(Debug::Color::Cyan) << Debug::packed << animator.state(animation) << Debug::resetColor << "animation" << Debug::packed << animation << "and a value of" << value << Debug::newline;
            }

            int value;
        };

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::NodeAttachment;
        }
        void doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}

        int value = 69420;
    };
    IntegratedAnimator& integratedAnimator = ui.setGenericAnimatorInstance(Containers::pointer<IntegratedAnimator>(ui.createAnimator()));
    integratedAnimator.create(-1_nsec, 10_nsec, node); /* playing */
    AnimationHandle integratedAnimatorPaused = integratedAnimator.create(-1_nsec, 10_nsec, node); /* paused */
    integratedAnimator.pause(integratedAnimatorPaused, -25_nsec);

    IntegratedAnimator::DebugIntegration integration{1226};
    layer.setAnimatorName(integratedAnimator, "No#2", integration);

    ui.update();

    Containers::String out;
    {
        Debug redirectOutput{&out};
        CORRADE_VERIFY(layer.inspectNode(node));
    }
    CORRADE_COMPARE(layer.currentInspectedNode(), node);
    CORRADE_COMPARE_AS(out,
        "Top-level node {0x0, 0x1}\n"
        "  Animator No#2 (69420) Playing animation {0x0, 0x1} and a value of 1226\n"
        "  Animator No#2 (69420) Paused animation {0x1, 0x1} and a value of 1226\n",
        TestSuite::Compare::String);
}

void DebugLayerTest::nodeInspectAnimatorDebugIntegrationExplicitRvalue() {
    /* Implicit integration tested in nodeInspect() above, this verifies that
       the explicitly passed instance does the right thing as well */

    AbstractUserInterface ui{{100, 100}};

    NodeHandle node = ui.createNode({40, 20}, {20, 30});

    DebugLayer& layer = ui.setLayerInstance(Containers::pointer<DebugLayer>(ui.createLayer(), DebugLayerSource::NodeAnimationDetails, DebugLayerFlag::NodeInspect));

    /* Compared to nodeInspect() and
       nodeInspectAnimatorDebugIntegrationExplicit() here the whole
       DebugIntegration type is defined in a base class which should also be
       fine */
    struct IntegratedAnimatorBase: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;
        using AbstractGenericAnimator::create;

        struct DebugIntegration {
            explicit DebugIntegration(int value): value{value} {}

            DebugIntegration(const DebugIntegration&) = delete;
            DebugIntegration(DebugIntegration&&) noexcept = default;
            DebugIntegration& operator=(const DebugIntegration&) = delete;
            /* Clang complains this one is unused. I want to avoid weirdness
               that might happen if one of these isn't defined, tho. */
            CORRADE_UNUSED DebugIntegration& operator=(DebugIntegration&&) noexcept = default;

            /* This is deliberately *not* passing the name via const& to verify
               that the signature doesn't have to match exactly */
            void print(Debug& debug, const IntegratedAnimatorBase& animator, Containers::StringView animatorName, AnimatorDataHandle animation) {
                /* Printing the name colored to verify the color is disabled
                   correctly here as well */
                debug << "  Animator" << Debug::color(Debug::Color::Yellow) << animatorName << Debug::resetColor << "(" << Debug::nospace << animator.value << Debug::nospace << ")" << Debug::color(Debug::Color::Cyan) << Debug::packed << animator.state(animation) << Debug::resetColor << "animation" << Debug::packed << animation << "and a value of" << value << Debug::newline;
            }

            int value;
        };

        void doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}

        int value = 69420;
    };
    struct IntegratedAnimator: IntegratedAnimatorBase {
        using IntegratedAnimatorBase::IntegratedAnimatorBase;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::NodeAttachment;
        }
    };
    IntegratedAnimator& integratedAnimator = ui.setGenericAnimatorInstance(Containers::pointer<IntegratedAnimator>(ui.createAnimator()));
    integratedAnimator.create(-1_nsec, 10_nsec, node); /* playing */
    AnimationHandle integratedAnimatorPaused = integratedAnimator.create(-1_nsec, 10_nsec, node); /* paused */
    integratedAnimator.pause(integratedAnimatorPaused, -25_nsec);

    layer.setAnimatorName(integratedAnimator, "No#2", IntegratedAnimator::DebugIntegration{1226});

    ui.update();

    Containers::String out;
    {
        Debug redirectOutput{&out};
        CORRADE_VERIFY(layer.inspectNode(node));
    }
    CORRADE_COMPARE(layer.currentInspectedNode(), node);
    CORRADE_COMPARE_AS(out,
        "Top-level node {0x0, 0x1}\n"
        "  Animator No#2 (69420) Playing animation {0x0, 0x1} and a value of 1226\n"
        "  Animator No#2 (69420) Paused animation {0x1, 0x1} and a value of 1226\n",
        TestSuite::Compare::String);
}

void DebugLayerTest::nodeInspectNodeRemoved() {
    auto&& data = NodeInspectHighlightNodeRemovedData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui{{100, 100}};

    /* Node to catch the event on */
    NodeHandle parent = ui.createNode({}, {100, 100});
    NodeHandle parent2 = ui.createNode(parent, {}, {100, 100});
    NodeHandle node = ui.createNode(parent2, {}, {100, 100});

    DebugLayer& layer = ui.setLayerInstance(Containers::pointer<DebugLayer>(ui.createLayer(), DebugLayerSource::Nodes, DebugLayerFlag::NodeInspect));
    /* Just to silence the output */
    layer.setNodeInspectCallback([](Containers::StringView){});

    PointerEvent press{{}, PointerEventSource::Mouse, Pointer::MouseRight, true, 0, Modifier::Ctrl};
    CORRADE_VERIFY(ui.pointerPressEvent({50, 50}, press));
    CORRADE_COMPARE(layer.currentInspectedNode(), node);

    /* Right after removal it still reports the node as inspected */
    ui.removeNode(data.removeParent ? parent : node);
    CORRADE_COMPARE(layer.currentInspectedNode(), node);
    CORRADE_COMPARE_AS(ui.state(),
        UserInterfaceState::NeedsDataUpdate,
        TestSuite::Compare::GreaterOrEqual);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeClean);

    /* Only after a clean it gets cleaned */
    ui.clean();
    CORRADE_COMPARE(layer.currentInspectedNode(), NodeHandle::Null);
}

void DebugLayerTest::nodeInspectInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui{{100, 100}};
    NodeHandle node = ui.createNode({}, {});

    struct IntegratedLayer: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        struct DebugIntegration {
            void print(Debug& debug, const IntegratedLayer&, const Containers::StringView&, LayerDataHandle) {
                debug << "    Hello this is broken";
            }
        };

        LayerFeatures doFeatures() const override { return {}; }
    };
    IntegratedLayer& integratedLayer = ui.setLayerInstance(Containers::pointer<IntegratedLayer>(ui.createLayer()));
    integratedLayer.create(node);

    DebugLayer layerNoNodesNoInspect{layerHandle(0, 1), {}, {}};
    DebugLayer layerNoUi{layerHandle(0, 1), DebugLayerSource::Nodes, DebugLayerFlag::NodeInspect};

    DebugLayer& layer = ui.setLayerInstance(Containers::pointer<DebugLayer>(ui.createLayer(), DebugLayerSource::NodeDataDetails, DebugLayerFlag::NodeInspect));
    layer.setLayerName(integratedLayer, "BrokenPrint");
    /* To silence the output */
    layer.setNodeInspectCallback([](Containers::StringView) {});

    /* Make the layer aware of the node */
    ui.update();

    /* Calling functionality getters / setters is valid on a layer that doesn't
       have the feature enabled. The actual state queries and updates can't be
       called tho. */
    layerNoNodesNoInspect.hasNodeInspectCallback();
    layerNoNodesNoInspect.setNodeInspectCallback(nullptr);
    layerNoNodesNoInspect.nodeInspectGesture();
    layerNoNodesNoInspect.setNodeInspectGesture(Pointer::MouseRight, {});
    layerNoNodesNoInspect.nodeInspectColor();
    layerNoNodesNoInspect.setNodeInspectColor({});

    Containers::String out;
    Error redirectError{&out};
    layerNoNodesNoInspect.setNodeInspectGesture({}, Modifier::Ctrl);
    layerNoNodesNoInspect.currentInspectedNode();
    layerNoNodesNoInspect.inspectNode({});
    layerNoUi.inspectNode({});
    layer.inspectNode(node);
    CORRADE_COMPARE_AS(out,
        "Ui::DebugLayer::setNodeInspectGesture(): expected at least one pointer\n"
        "Ui::DebugLayer::currentInspectedNode(): Ui::DebugLayerFlag::NodeInspect not enabled\n"
        "Ui::DebugLayer::inspectNode(): Ui::DebugLayerFlag::NodeInspect not enabled\n"
        "Ui::DebugLayer::inspectNode(): layer not part of a user interface\n"
        /* Looks a bit weird but should hopefully contain enough info to
           discover where this happened */
        "Ui::DebugLayer: expected DebugIntegration::print() to end with a newline but got Hello this is broken\n",
        TestSuite::Compare::String);
}

void DebugLayerTest::nodeInspectToggle() {
    auto&& data = NodeInspectToggleData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui{{100, 100}};

    NodeHandle parent = ui.createNode({}, {100, 100});
    NodeHandle parent2 = ui.createNode(parent, {}, {100, 100});
    NodeHandle node = ui.createNode(parent2, {}, {100, 100});

    struct Layer: DebugLayer {
        explicit Layer(LayerHandle handle, DebugLayerSources sources, DebugLayerFlags flags, LayerFeatures features): DebugLayer{handle, sources, flags}, _features{features} {}
        const DebugLayer::State& stateData() {
            return *_state;
        }

        LayerFeatures doFeatures() const override {
            return DebugLayer::doFeatures()|_features;
        }

        private:
            LayerFeatures _features;
    };
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), DebugLayerSource::Nodes, DebugLayerFlags{0x80}, data.features));
    CORRADE_COMPARE(layer.flags(), DebugLayerFlags{0x80});

    Int called = 0;
    Containers::String out;
    if(data.callback)
        layer.setNodeInspectCallback([&called, &out](Containers::StringView message){
            out = message;
            ++called;
        });

    /* Make the DebugLayer aware of all nodes */
    ui.update();

    /* Adding the flag makes it possible to query the inspected node, but
       there's none */
    layer.addFlags(DebugLayerFlag::NodeInspect);
    CORRADE_COMPARE(layer.flags(), DebugLayerFlags{0x80}|DebugLayerFlag::NodeInspect);
    CORRADE_COMPARE(layer.currentInspectedNode(), NodeHandle::Null);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);
    if(data.callback)
        CORRADE_COMPARE(called, 0);
    CORRADE_COMPARE(out, "");

    /* Inspecting a null node if there's no node currently inspected does not
       set NeedsDataUpdate and doesn't call the callback either */
    CORRADE_VERIFY(layer.inspectNode(NodeHandle::Null));
    CORRADE_COMPARE(layer.currentInspectedNode(), NodeHandle::Null);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);
    if(data.callback)
        CORRADE_COMPARE(called, 0);
    CORRADE_COMPARE(out, "");

    {
        /* Don't care about the output if callback isn't set */
        Debug redirectOutput{nullptr};
        CORRADE_VERIFY(layer.inspectNode(node));
    }
    CORRADE_COMPARE(layer.currentInspectedNode(), node);
    if(data.callback) {
        CORRADE_COMPARE(called, 1);
        CORRADE_COMPARE(out, "Node {0x2, 0x1}");
    }
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate|data.expectedState);

    /* Update to clear the NeedsDataUpdate flag */
    ui.update();
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);

    /* Inspecting the same node only prints the callback again, but does not
       set NeedsDataUpdate */
    {
        /* Don't care about the output if callback isn't set */
        Debug redirectOutput{nullptr};
        CORRADE_VERIFY(layer.inspectNode(node));
    }
    CORRADE_COMPARE(layer.currentInspectedNode(), node);
    if(data.callback) {
        CORRADE_COMPARE(called, 2);
        CORRADE_COMPARE(out, "Node {0x2, 0x1}");
    }
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);

    /* Removing the flag calls the callback to remove the node. It isn't
       possible to query the current inspected node anymore, but the internal
       state has it unset. */
    layer.clearFlags(DebugLayerFlag::NodeInspect);
    CORRADE_COMPARE(layer.stateData().currentInspectedNode, NodeHandle::Null);
    if(data.callback) {
        CORRADE_COMPARE(called, 3);
        CORRADE_COMPARE(out, "");
    }
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate|data.expectedState);

    /* Update to clear the NeedsDataUpdate flag */
    ui.update();
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);

    /* Adding the flag back makes it possible to query it again, it's null. The
       callback doesn't get called this time as nothing changed, no state
       update is triggered either. */
    layer.setFlags(DebugLayerFlag::NodeInspect);
    CORRADE_COMPARE(layer.flags(), DebugLayerFlag::NodeInspect);
    CORRADE_COMPARE(layer.currentInspectedNode(), NodeHandle::Null);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);
    if(data.callback)
        CORRADE_COMPARE(called, 3);

    /* Removing the flag with nothing inspected also doesn't trigger
       anything */
    layer.setFlags({});
    CORRADE_COMPARE(layer.flags(), DebugLayerFlags{});
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);
    if(data.callback)
        CORRADE_COMPARE(called, 3);

    /* Add the flag, highlight & update to clear the flags */
    layer.setFlags(DebugLayerFlag::NodeInspect);
    {
        /* Don't care about the output if callback isn't set */
        Debug redirectOutput{nullptr};
        CORRADE_VERIFY(layer.inspectNode(node));
    }
    CORRADE_COMPARE(layer.currentInspectedNode(), node);
    if(data.callback) {
        CORRADE_COMPARE(called, 4);
        CORRADE_COMPARE(out, "Node {0x2, 0x1}");
    }
    ui.update();
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);

    /* Inspecting a null node with a node currently higlighted calls the
       callback with an empty string and sets NeedsDataUpdate if drawing to not
       render the highlight anymore */
    {
        /* Don't care about the output if callback isn't set */
        Debug redirectOutput{nullptr};
        CORRADE_VERIFY(layer.inspectNode(NodeHandle::Null));
    }
    CORRADE_COMPARE(layer.currentInspectedNode(), NodeHandle::Null);
    if(data.callback) {
        CORRADE_COMPARE(called, 5);
        CORRADE_COMPARE(out, "");
    }
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate|data.expectedState);
}

void DebugLayerTest::nodeInspectSkipNoData() {
    auto&& data = NodeInspectSkipNoDataData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui{{100, 100}};

    DebugLayer& layer = ui.setLayerInstance(Containers::pointer<DebugLayer>(ui.createLayer(), DebugLayerSource::NodeData|data.sources, DebugLayerFlag::NodeInspect|data.flags));

    struct EmptyLayer: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override { return {}; }
    };
    struct EmptyLayouter: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::add;

        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&) override {}
    };
    struct EmptyAnimator: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;
        using AbstractGenericAnimator::create;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::NodeAttachment;
        }
        void doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
    };
    EmptyLayer& emptyLayer = ui.setLayerInstance(Containers::pointer<EmptyLayer>(ui.createLayer()));
    EmptyLayouter& emptyLayouter = ui.setLayouterInstance(Containers::pointer<EmptyLayouter>(ui.createLayouter()));
    EmptyAnimator& emptyAnimator = ui.setGenericAnimatorInstance(Containers::pointer<EmptyAnimator>(ui.createAnimator()));

    /* A node below, optionally with a single data attached */
    NodeHandle below = ui.createNode({}, {100, 100});
    if(data.belowData)
        emptyLayer.create(below);

    /* A node above, with children, layout and animation, none of which should
       affect the condition to skip it */
    NodeHandle above = ui.createNode({}, {100, 100});
    /*NodeHandle aboveChild =*/ ui.createNode(above, {80, 80}, {10, 10});
    emptyLayouter.add(above);
    emptyAnimator.create({}, {}, above);

    /* Update to trigger DebugLayer population */
    ui.update();

    Containers::String out;
    {
        Debug redirectOutput{&out};
        if(data.event) {
            PointerEvent press{{}, PointerEventSource::Mouse, Pointer::MouseRight, true, 0, Modifier::Ctrl};
            CORRADE_COMPARE(ui.pointerPressEvent({45, 35}, press), data.expectAbove || data.expectBelow);
        } else {
            CORRADE_COMPARE(layer.inspectNode(above), data.expectAbove);
        }
    }

    if(data.expectAbove) {
        CORRADE_COMPARE(layer.currentInspectedNode(), above);
        CORRADE_COMPARE_AS(out,
            "Top-level node {0x1, 0x1}\n"
            "  1 layouts from 1 layouters\n"
            "  1 Stopped animations from 1 animators\n",
            TestSuite::Compare::String);
    } else if(data.expectBelow) {
        CORRADE_COMPARE(layer.currentInspectedNode(), below);
        CORRADE_COMPARE_AS(out,
            "Top-level node {0x0, 0x1}\n"
            "  1 data from 1 layers\n",
            TestSuite::Compare::String);
    } else {
        CORRADE_COMPARE(layer.currentInspectedNode(), NodeHandle::Null);
        CORRADE_COMPARE(out, "");
    }
}

void DebugLayerTest::nodeHighlightSetters() {
    auto&& data = LayerDrawData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* These should work even with DebugLayerSource::Nodes not set, so user
       code can set all those independently of deciding what to actually use */
    struct Layer: DebugLayer {
        explicit Layer(LayerHandle handle, LayerFeatures features): DebugLayer{handle, {}, {}}, _features{features} {}

        LayerFeatures doFeatures() const override {
            return DebugLayer::doFeatures()|_features;
        }

        private:
            LayerFeatures _features;
    } layer{layerHandle(0, 1), data.features};

    /* Required to be called before update() (because AbstractUserInterface
       guarantees the same on a higher level), not needed for anything here */
    if(data.features >= LayerFeature::Draw)
        layer.setSize({1, 1}, {1, 1});

    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Defaults */
    CORRADE_COMPARE_AS(layer.nodeHighlightColorMap(), Containers::arrayView({
        0x00ffff_rgb
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.nodeHighlightColorMapAlpha(), 0.25f);

    /* Changing the color map causes NeedsDataUpdate to be set, but only if the
       layer draws anything. The data are just referenced, not copied
       anywhere. */
    Color3ub colormap[]{
        0xff00ff_rgb,
        0x00ff00_rgb
    };
    layer.setNodeHighlightColorMap(colormap, 0.75f);
    CORRADE_COMPARE_AS(layer.nodeHighlightColorMap(), Containers::arrayView({
        0xff00ff_rgb,
        0x00ff00_rgb
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.nodeHighlightColorMap().data(), colormap);
    CORRADE_COMPARE(layer.nodeHighlightColorMapAlpha(), 0.75f);
    CORRADE_COMPARE(layer.state(), data.expectedState);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Default alpha */
    Color3ub colormap2[]{
        0xffff00_rgb
    };
    layer.setNodeHighlightColorMap(colormap2);
    CORRADE_COMPARE_AS(layer.nodeHighlightColorMap(), Containers::arrayView({
        0xffff00_rgb
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.nodeHighlightColorMap().data(), colormap2);
    CORRADE_COMPARE(layer.nodeHighlightColorMapAlpha(), 0.25f);
    CORRADE_COMPARE(layer.state(), data.expectedState);
}

void DebugLayerTest::nodeHighlight() {
    auto&& data = LayerDrawData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui{{100, 100}};

    struct Layer: DebugLayer {
        explicit Layer(LayerHandle handle, LayerFeatures features): DebugLayer{handle, DebugLayerSource::Nodes, {}}, _features{features} {}

        LayerFeatures doFeatures() const override {
            return DebugLayer::doFeatures()|_features;
        }

        private:
            LayerFeatures _features;
    };
    DebugLayer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), data.features));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);

    /* A bunch of nodes to highlight, some with a non-trivial generation */
    NodeHandle node0 = ui.createNode({}, {});
    NodeHandle node1 = ui.createNode({}, {});
    ui.removeNode(ui.createNode({}, {}));
    ui.removeNode(ui.createNode({}, {}));
    NodeHandle node2 = ui.createNode({}, {});

    /* By default the layer knows about no nodes and highlighting isn't
       possible */
    CORRADE_COMPARE_AS(layer.currentHighlightedNodes(), Containers::stridedArrayView<bool>({
    }).sliceBit(0), TestSuite::Compare::Container);
    CORRADE_VERIFY(!layer.highlightNode(node0));
    CORRADE_VERIFY(!layer.highlightNode(node2));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);
    CORRADE_COMPARE_AS(layer.currentHighlightedNodes(), Containers::stridedArrayView<bool>({
        /* empty */
    }).sliceBit(0), TestSuite::Compare::Container);

    /* Updating fills the mask for all nodes */
    ui.update();
    CORRADE_COMPARE_AS(layer.currentHighlightedNodes(), Containers::stridedArrayView<bool>({
        false,
        false,
        false,
    }).sliceBit(0), TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);

    /* Highlighting a known node works and sets NeedsDataUpdate if the layer
       draws anything */
    CORRADE_VERIFY(layer.highlightNode(node1));
    CORRADE_COMPARE_AS(layer.currentHighlightedNodes(), Containers::stridedArrayView<bool>({
        false,
        true,
        false,
    }).sliceBit(0), TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate|data.expectedState);

    /* Update to reset the state */
    ui.update();
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);

    /* Highlighting a node that's already highligted returns true but doesn't
       set NeedsDataUpdate */
    CORRADE_VERIFY(layer.highlightNode(node1));
    CORRADE_COMPARE_AS(layer.currentHighlightedNodes(), Containers::stridedArrayView<bool>({
        false,
        true,
        false,
    }).sliceBit(0), TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);

    /* Create more nodes, the layer isn't aware of them yet so cannot highlight
       them */
    NodeHandle node3 = ui.createNode({}, {});
    ui.removeNode(ui.createNode({}, {}));
    ui.removeNode(ui.createNode({}, {}));
    NodeHandle node4 = ui.createNode({}, {});
    NodeHandle node5 = ui.createNode({}, {});
    CORRADE_VERIFY(!layer.highlightNode(node5));
    CORRADE_COMPARE_AS(layer.currentHighlightedNodes(), Containers::stridedArrayView<bool>({
        false,
        true,
        false,
    }).sliceBit(0), TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);

    /* Setting a node name makes the layer aware of that particular node so
       it's then possible to highlight it. Not any other yet, tho. */
    layer.setNodeName(node4, "hello");
    CORRADE_VERIFY(!layer.highlightNode(node3));
    CORRADE_VERIFY(layer.highlightNode(node4));
    CORRADE_COMPARE_AS(layer.currentHighlightedNodes(), Containers::stridedArrayView<bool>({
        false,
        true,
        false,
        false,
        true,
    }).sliceBit(0), TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate|data.expectedState);

    /* Update to reset the state. This makes the layer aware of node5 as
       well. */
    ui.update();
    CORRADE_COMPARE_AS(layer.currentHighlightedNodes(), Containers::stridedArrayView<bool>({
        false,
        true,
        false,
        false,
        true,
        false, /* node5 */
    }).sliceBit(0), TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);

    /* Highlighting a node with a generation different from the one that is
       known to the layer doesn't highlight it, even though it's a node that's
       valid. Similarly, highlighting a node that has an ID larger than what's
       known by the layer doesn't work even though the handle is valid. Neither
       operation results in anything that'd warrant NeedsDataUpdate. */
    ui.removeNode(node2);
    NodeHandle node2Replacement = ui.createNode({}, {});
    NodeHandle node6 = ui.createNode({}, {});
    CORRADE_COMPARE(nodeHandleId(node2Replacement), nodeHandleId(node2));
    CORRADE_VERIFY(!layer.highlightNode(node2Replacement));
    CORRADE_VERIFY(!layer.highlightNode(node6));
    CORRADE_COMPARE_AS(layer.currentHighlightedNodes(), Containers::stridedArrayView<bool>({
        false,
        true,
        false,
        false,
        true,
        false,
    }).sliceBit(0), TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);

    /* Clearing highlighted nodes results in NeedsDataUpdate if the layer draws
       anything */
    layer.clearHighlightedNodes();
    CORRADE_COMPARE_AS(layer.currentHighlightedNodes(), Containers::stridedArrayView<bool>({
        false,
        false,
        false,
        false,
        false,
        false,
    }).sliceBit(0), TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate|data.expectedState);

    /* Update to reset the state. This makes the layer aware of node6 as
       well. */
    ui.update();
    CORRADE_COMPARE_AS(layer.currentHighlightedNodes(), Containers::stridedArrayView<bool>({
        false,
        false,
        false,
        false,
        false,
        false,
        false, /* node6 */
    }).sliceBit(0), TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);

    /* Clearing if there's nothing to clear sets it too, because that's a
       simpler operation than counting set bits */
    /** @todo update once BitArrayView implements any() */
    layer.clearHighlightedNodes();
    CORRADE_COMPARE_AS(layer.currentHighlightedNodes(), Containers::stridedArrayView<bool>({
        false,
        false,
        false,
        false,
        false,
        false,
        false,
    }).sliceBit(0), TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate|data.expectedState);
}

Int conditionCalled = 0;

void DebugLayerTest::nodeHighlightConditionResetCounters() {
    conditionCalled = 0;
}

void DebugLayerTest::nodeHighlightConditionNodes() {
    auto&& data = LayerDrawData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui{{100, 100}};

    struct Layer: DebugLayer {
        explicit Layer(LayerHandle handle, LayerFeatures features): DebugLayer{handle, DebugLayerSource::Nodes, {}}, _features{features} {}

        LayerFeatures doFeatures() const override {
            return DebugLayer::doFeatures()|_features;
        }

        private:
            LayerFeatures _features;
    };
    DebugLayer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), data.features));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);

    /* Before the layer knows about any node, the condition doesn't get called
       at all and the function returns false to indicate that */
    CORRADE_VERIFY(!layer.highlightNodes([](const AbstractUserInterface&, NodeHandle) {
        CORRADE_FAIL("This is expected to not be called.");
        return true;
    }));
    CORRADE_COMPARE_AS(layer.currentHighlightedNodes(), Containers::stridedArrayView<bool>({
        /* empty */
    }).sliceBit(0), TestSuite::Compare::Container);

    /* A bunch of nodes to highlight, some with a non-trivial generation,
       some removed */
    NodeHandle node0 = ui.createNode({}, {}, NodeFlag::Focusable);
    NodeHandle node1 = ui.createNode({}, {20, 10});
    ui.removeNode(ui.createNode({}, {}));
    ui.removeNode(ui.createNode({}, {}));
    NodeHandle node2Removed = ui.createNode({}, {});
    NodeHandle node3 = ui.createNode({}, {0, 10});
    ui.removeNode(ui.createNode({}, {}));
    NodeHandle node4 = ui.createNode({}, {30, 10}, NodeFlag::Focusable);
    NodeHandle node5 = ui.createNode({}, {});
    ui.removeNode(node2Removed);

    /* After an update, the condition gets called and the function returns
       true even if the condition failed for all nodes. Because nothing got
       highlighted, NeedsDataUpdate isn't set. */
    ui.update();
    CORRADE_VERIFY(layer.highlightNodes([](const AbstractUserInterface& ui, NodeHandle node) {
        ++conditionCalled;
        CORRADE_VERIFY(ui.isHandleValid(node));
        return false;
    }));
    CORRADE_COMPARE(conditionCalled, 5); /* Not called for the removed node */
    CORRADE_COMPARE_AS(layer.currentHighlightedNodes(), Containers::stridedArrayView<bool>({
        false,
        false,
        false, /* removed */
        false,
        false,
        false,
    }).sliceBit(0), TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);

    /* Highlighting a subset of nodes sets NeedsDataUpdate if the layer is
       drawing anything */
    CORRADE_VERIFY(layer.highlightNodes([](const AbstractUserInterface& ui, NodeHandle node) {
        ++conditionCalled;
        return ui.nodeSize(node).y() == 10.0f;
    }));
    CORRADE_COMPARE(conditionCalled, 5*2);
    CORRADE_COMPARE_AS(layer.currentHighlightedNodes(), Containers::stridedArrayView<bool>({
        false,
        true,
        false, /* removed */
        true,
        true,
        false,
    }).sliceBit(0), TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate|data.expectedState);

    /* Update to reset the state */
    ui.update();
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);

    /* Highlighting nodes that are already marked returns true but doesn't set
       NeedsDataUpdate */
    CORRADE_VERIFY(layer.highlightNodes([](const AbstractUserInterface& ui, NodeHandle node) {
        ++conditionCalled;
        return ui.nodeSize(node).x() > 0.0f;
    }));
    /* The condition isn't called for the already highlighted nodes because
       it'd not affect anything. Besides the removed node, there are just
       remaining two that aren't yet marked, for which it's called. */
    CORRADE_COMPARE(conditionCalled, 5*2 + 2);
    CORRADE_COMPARE_AS(layer.currentHighlightedNodes(), Containers::stridedArrayView<bool>({
        false, /* called */
        true,
        false, /* removed */
        true,
        true,
        false, /* called */
    }).sliceBit(0), TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);

    /* Highlighting a disjoint set marks the layer with NeedsDataUpdate if it
       draws anything */
    CORRADE_VERIFY(layer.highlightNodes([](const AbstractUserInterface& ui, NodeHandle node) {
        ++conditionCalled;
        return ui.nodeFlags(node) >= NodeFlag::Focusable;
    }));
    /* Again there are just 2 which are not yet highlighted, of which one now
       becomes higlighted */
    CORRADE_COMPARE(conditionCalled, 5*2 + 2*2);
    CORRADE_COMPARE_AS(layer.currentHighlightedNodes(), Containers::stridedArrayView<bool>({
        true, /* called, newly highlighted */
        true,
        false, /* removed */
        true,
        true,
        false, /* called */
    }).sliceBit(0), TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate|data.expectedState);

    /* Update to reset the state */
    ui.update();
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);

    /* Remove all nodes except one that's currently highlighted and update so
       there's exactly one highlighted node left for the next test */
    ui.removeNode(node0);
    ui.removeNode(node1);
    ui.removeNode(node4);
    ui.removeNode(node5);
    ui.update();
    CORRADE_COMPARE_AS(layer.currentHighlightedNodes(), Containers::stridedArrayView<bool>({
        false, /* removed */
        false, /* removed */
        false, /* removed previously */
        true,
        false, /* removed */
        false, /* removed */
    }).sliceBit(0), TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);

    /* Remove the remaining node and create a new node, with both yet unknown
       to the layer. Highlighting now has no known & valid nodes to go through
       and so the condition is never called, the function returns false and
       there's no need for NeedsDataUpdate to be set either. */
    ui.removeNode(node3);
    ui.createNode({}, {});
    CORRADE_VERIFY(!layer.highlightNodes([](const AbstractUserInterface&, NodeHandle) {
        CORRADE_FAIL("This is expected to not be called.");
        return true;
    }));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);
}

void DebugLayerTest::nodeHighlightConditionData() {
    auto&& data = NodeHighlightConditionDataData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui{{100, 100}};

    struct Layer: DebugLayer {
        explicit Layer(LayerHandle handle, DebugLayerSources sources, LayerFeatures features): DebugLayer{handle, DebugLayerSource::Nodes|sources, {}}, _features{features} {}

        LayerFeatures doFeatures() const override {
            return DebugLayer::doFeatures()|_features;
        }

        private:
            LayerFeatures _features;
    };
    DebugLayer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), data.sources, data.features));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);

    /* A bunch of nodes to highlight, some with a non-trivial generation,
       some to be removed */
    /*NodeHandle node0 =*/ ui.createNode({}, {});
    NodeHandle node1 = ui.createNode({}, {});
    ui.removeNode(ui.createNode({}, {}));
    ui.removeNode(ui.createNode({}, {}));
    NodeHandle node2EventuallyRemoved = ui.createNode({}, {});
    NodeHandle node3 = ui.createNode({}, {});
    ui.removeNode(ui.createNode({}, {}));
    NodeHandle node4 = ui.createNode({}, {});
    NodeHandle node5 = ui.createNode({}, {});

    /* Make the debug layer aware of the nodes. This alone isn't enough because
       the debug layer isn't aware of the layer / layouter / animator yet. */
    ui.update();

    /* Data from a layer / layouter / animator. Some not attached to any node,
       some removed, some attached to a node that's eventually removed. */
    struct CustomLayer: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::remove;

        DataHandle create(UnsignedInt id, NodeHandle node) {
            DataHandle handle = AbstractLayer::create(node);
            _ids[dataHandleId(handle)] = id;
            return handle;
        }

        UnsignedInt id(LayerDataHandle data) const {
            CORRADE_INTERNAL_ASSERT(isHandleValid(data));
            return _ids[layerDataHandleId(data)];
        }

        LayerFeatures doFeatures() const override { return {}; }

        private:
            UnsignedInt _ids[10]{};
    };
    struct CustomLayouter: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::remove;

        LayoutHandle add(UnsignedInt id, NodeHandle node) {
            LayoutHandle handle = AbstractLayouter::add(node);
            _ids[layoutHandleId(handle)] = id;
            return handle;
        }

        UnsignedInt id(LayouterDataHandle data) const {
            CORRADE_INTERNAL_ASSERT(isHandleValid(data));
            return _ids[layouterDataHandleId(data)];
        }

        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&) override {}

        private:
            UnsignedInt _ids[10]{};
    };
    struct CustomAnimator: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;
        using AbstractGenericAnimator::remove;

        AnimationHandle create(UnsignedInt id, NodeHandle node) {
            AnimationHandle handle = AbstractAnimator::create({}, {}, node);
            _ids[animationHandleId(handle)] = id;
            return handle;
        }

        UnsignedInt id(AnimatorDataHandle data) const {
            CORRADE_INTERNAL_ASSERT(isHandleValid(data));
            return _ids[animatorDataHandleId(data)];
        }

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::NodeAttachment;
        }
        void doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}

        private:
            UnsignedInt _ids[10]{};
    };

    CustomLayer* customLayer{};
    CustomLayouter* customLayouter{};
    CustomAnimator* customAnimator{};
    if(data.layer) {
        customLayer = &ui.setLayerInstance(Containers::pointer<CustomLayer>(ui.createLayer()));
        customLayer->create(12, node3);
        customLayer->create(3, node1);
        customLayer->create(7, node2EventuallyRemoved);
        customLayer->create(16, node4);
        DataHandle removed = customLayer->create(12, node5);
        customLayer->create(8, node2EventuallyRemoved);
        customLayer->create(6, node4);
        customLayer->create(12, NodeHandle::Null);
        customLayer->create(9, node1);
        customLayer->remove(removed);
    } else if(data.layouter) {
        customLayouter = &ui.setLayouterInstance(Containers::pointer<CustomLayouter>(ui.createLayouter()));
        customLayouter->add(12, node3);
        customLayouter->add(3, node1);
        customLayouter->add(7, node2EventuallyRemoved);
        customLayouter->add(16, node4);
        LayoutHandle removed = customLayouter->add(12, node5);
        customLayouter->add(8, node2EventuallyRemoved);
        customLayouter->add(6, node4);
        /* Layouters don't allow creating layouts that aren't assigned
           anywhere */
        customLayouter->add(9, node1);
        customLayouter->remove(removed);
    } else if(data.animator) {
        customAnimator = &ui.setGenericAnimatorInstance(Containers::pointer<CustomAnimator>(ui.createAnimator()));
        customAnimator->create(12, node3);
        customAnimator->create(3, node1);
        customAnimator->create(7, node2EventuallyRemoved);
        customAnimator->create(16, node4);
        AnimationHandle removed = customAnimator->create(12, node5);
        customAnimator->create(8, node2EventuallyRemoved);
        customAnimator->create(6, node4);
        customAnimator->create(12, NodeHandle::Null);
        customAnimator->create(9, node1);
        customAnimator->remove(removed);
    } else CORRADE_INTERNAL_ASSERT_UNREACHABLE();

    /* Before the debug layer knows about the layer / layouter / animator, the
       condition doesn't get called at all and the function returns false to
       indicate that. It doesn't matter that it knows about the nodes at that
       point already. */
    if(data.layer)
        CORRADE_VERIFY(!layer.highlightNodes(*customLayer, [](const CustomLayer&, LayerDataHandle) {
            CORRADE_FAIL("This is expected to not be called.");
            return true;
        }));
    else if(data.layouter)
        CORRADE_VERIFY(!layer.highlightNodes(*customLayouter, [](const CustomLayouter&, LayouterDataHandle) {
            CORRADE_FAIL("This is expected to not be called.");
            return true;
        }));
    else if(data.animator)
        CORRADE_VERIFY(!layer.highlightNodes(*customAnimator, [](const CustomAnimator&, AnimatorDataHandle) {
            CORRADE_FAIL("This is expected to not be called.");
            return true;
        }));
    else CORRADE_INTERNAL_ASSERT_UNREACHABLE();
    CORRADE_COMPARE_AS(layer.currentHighlightedNodes(), Containers::stridedArrayView<bool>({
        false,
        false,
        false,
        false,
        false,
        false,
    }).sliceBit(0), TestSuite::Compare::Container);

    /* After an update, the condition gets called and the function returns true
       even if the condition failed for all data. Because nothing got
       highlighted, NeedsDataUpdate isn't set. */
    ui.update();
    if(data.layer)
        CORRADE_VERIFY(layer.highlightNodes(*customLayer, [](const CustomLayer& layer, LayerDataHandle data) {
            ++conditionCalled;
            CORRADE_VERIFY(layer.isHandleValid(data));
            return false;
        }));
    else if(data.layouter)
        CORRADE_VERIFY(layer.highlightNodes(*customLayouter, [](const CustomLayouter& layouter, LayouterDataHandle layout) {
            ++conditionCalled;
            CORRADE_VERIFY(layouter.isHandleValid(layout));
            return false;
        }));
    else if(data.animator)
        CORRADE_VERIFY(layer.highlightNodes(*customAnimator, [](const CustomAnimator& animator, AnimatorDataHandle animation) {
            ++conditionCalled;
            CORRADE_VERIFY(animator.isHandleValid(animation));
            return false;
        }));
    else CORRADE_INTERNAL_ASSERT_UNREACHABLE();
    /* Not called for the no-attachment and removed data / layouts /
       animations. (Layouts aren't allowed to not be attached, so for them
       there's one less in total.) */
    CORRADE_COMPARE(conditionCalled, 7);
    CORRADE_COMPARE_AS(layer.currentHighlightedNodes(), Containers::stridedArrayView<bool>({
        false, /* no data attached */
        false,
        false,
        false,
        false,
        false, /* only removed data attached */
    }).sliceBit(0), TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);

    /* Highlighting a subset of nodes sets NeedsDataUpdate if the layer is
       drawing anything */
    if(data.layer)
        CORRADE_VERIFY(layer.highlightNodes(*customLayer, [](const CustomLayer& layer, LayerDataHandle data) {
            ++conditionCalled;
            return layer.id(data) % 3 == 0;
        }));
    else if(data.layouter)
        CORRADE_VERIFY(layer.highlightNodes(*customLayouter, [](const CustomLayouter& layouter, LayouterDataHandle layout) {
            ++conditionCalled;
            return layouter.id(layout) % 3 == 0;
        }));
    else if(data.animator)
        CORRADE_VERIFY(layer.highlightNodes(*customAnimator, [](const CustomAnimator& animator, AnimatorDataHandle animation) {
            ++conditionCalled;
            return animator.id(animation) % 3 == 0;
        }));
    else CORRADE_INTERNAL_ASSERT_UNREACHABLE();
    /* Additionally not called for the second data / layout / animation
       attached to node 1, because at that point it's already highlighted */
    CORRADE_COMPARE(conditionCalled, 7 + 6);
    CORRADE_COMPARE_AS(layer.currentHighlightedNodes(), Containers::stridedArrayView<bool>({
        false, /* no data attached */
        true,
        false,
        true,
        true,
        false, /* only removed data attached */
    }).sliceBit(0), TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate|data.expectedState);

    /* Update to reset the state */
    ui.update();
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);

    /* Highlighting nodes that are already marked returns true but doesn't set
       NeedsDataUpdate */
    if(data.layer)
        CORRADE_VERIFY(layer.highlightNodes(*customLayer, [](const CustomLayer& layer, LayerDataHandle data) {
            ++conditionCalled;
            return layer.id(data) % 6 == 0;
        }));
    else if(data.layouter)
        CORRADE_VERIFY(layer.highlightNodes(*customLayouter, [](const CustomLayouter& layouter, LayouterDataHandle layout) {
            ++conditionCalled;
            return layouter.id(layout) % 6 == 0;
        }));
    else if(data.animator)
        CORRADE_VERIFY(layer.highlightNodes(*customAnimator, [](const CustomAnimator& animator, AnimatorDataHandle animation) {
            ++conditionCalled;
            return animator.id(animation) % 6 == 0;
        }));
    else CORRADE_INTERNAL_ASSERT_UNREACHABLE();
    /* The condition isn't called for the already highlighted nodes because
       it'd not affect anything. Besides the removed data, there is just two
       remaining data attached to a node that isn't yet marked, for which it's
       called. */
    CORRADE_COMPARE(conditionCalled, 7 + 6 + 2);
    CORRADE_COMPARE_AS(layer.currentHighlightedNodes(), Containers::stridedArrayView<bool>({
        false, /* no data attached */
        true,
        false, /* called twice, neither returned true */
        true,
        true,
        false, /* only removed data attached */
    }).sliceBit(0), TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);

    /* Highlighting a disjoint set marks the layer with NeedsDataUpdate if it
       draws anything */
    if(data.layer)
        CORRADE_VERIFY(layer.highlightNodes(*customLayer, [](const CustomLayer& layer, LayerDataHandle data) {
            ++conditionCalled;
            return layer.id(data) % 4 == 0;
        }));
    else if(data.layouter)
        CORRADE_VERIFY(layer.highlightNodes(*customLayouter, [](const CustomLayouter& layouter, LayouterDataHandle layout) {
            ++conditionCalled;
            return layouter.id(layout) % 4 == 0;
        }));
    else if(data.animator)
        CORRADE_VERIFY(layer.highlightNodes(*customAnimator, [](const CustomAnimator& animator, AnimatorDataHandle animation) {
            ++conditionCalled;
            return animator.id(animation) % 4 == 0;
        }));
    else CORRADE_INTERNAL_ASSERT_UNREACHABLE();
    /* Again there's two data attached to a node that isn't yet highlighted, of
       which only the second return true */
    CORRADE_COMPARE(conditionCalled, 7 + 6 + 2 + 2);
    CORRADE_COMPARE_AS(layer.currentHighlightedNodes(), Containers::stridedArrayView<bool>({
        false, /* no data attached */
        true,
        true, /* called twice, first false, second true */
        true,
        true,
        false, /* only removed data attached */
    }).sliceBit(0), TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate|data.expectedState);

    /* Update to reset the state */
    ui.update();
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);

    /* Remove all nodes that have data attached except one that's currently
       highlighted and update so there's exactly one highlighted node with two
       data attached left for the next test */
    ui.removeNode(node1);
    ui.removeNode(node3);
    ui.removeNode(node4);
    ui.update();
    CORRADE_COMPARE_AS(layer.currentHighlightedNodes(), Containers::stridedArrayView<bool>({
        false, /* no data attached */
        false, /* removed */
        true,
        false, /* removed */
        false, /* removed */
        false, /* only removed data attached */
    }).sliceBit(0), TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);

    /* Remove the remaining node and create a new node with data attached, with
       both yet unknown to the debug layer. Highlighting now has:
       - one data not attached anywhere
       - two data attached to known but no longer valid node
       - one data attached to a node with known index but unknown generation
       - one data attached to a node with not yet known index
       and so the condition is never called, the function returns false and
       there's no need for NeedsDataUpdate to be set either. */
    ui.removeNode(node2EventuallyRemoved);
    ui.createNode({}, {});
    ui.createNode({}, {});
    ui.createNode({}, {});
    NodeHandle node2Replacement = ui.createNode({}, {});
    NodeHandle nodeUnknownId = ui.createNode({}, {});
    CORRADE_COMPARE(nodeHandleId(node2Replacement), nodeHandleId(node2EventuallyRemoved));
    CORRADE_COMPARE(nodeHandleId(nodeUnknownId), nodeHandleId(node5) + 1);
    if(data.layer) {
        customLayer->create(0, node2Replacement);
        customLayer->create(0, nodeUnknownId);
        CORRADE_VERIFY(!layer.highlightNodes(*customLayer, [](const CustomLayer&, LayerDataHandle) {
            CORRADE_FAIL("This is expected to not be called.");
            return true;
        }));
    } else if(data.layouter) {
        customLayouter->add(0, node2Replacement);
        customLayouter->add(0, nodeUnknownId);
        CORRADE_VERIFY(!layer.highlightNodes(*customLayouter, [](const CustomLayouter&, LayouterDataHandle) {
            CORRADE_FAIL("This is expected to not be called.");
            return true;
        }));
    } else if(data.animator) {
        customAnimator->create(0, node2Replacement);
        customAnimator->create(0, nodeUnknownId);
        CORRADE_VERIFY(!layer.highlightNodes(*customAnimator, [](const CustomAnimator&, AnimatorDataHandle) {
            CORRADE_FAIL("This is expected to not be called.");
            return true;
        }));
    } else CORRADE_INTERNAL_ASSERT_UNREACHABLE();
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);

    /* Removing the layer and adding a new layer the debug layer isn't aware
       of, along with data attached to still known nodes, and highlighting with
       that, also won't work until an update */
    if(data.layer) {
        LayerHandle customLayerHandle = customLayer->handle();
        ui.removeLayer(customLayerHandle);
        CustomLayer& customLayerReplacement = ui.setLayerInstance(Containers::pointer<CustomLayer>(ui.createLayer()));
        CORRADE_COMPARE(layerHandleId(customLayerReplacement.handle()), layerHandleId(customLayerHandle));
        customLayerReplacement.create(0, node5);
        CORRADE_VERIFY(!layer.highlightNodes(customLayerReplacement, [](const CustomLayer&, LayerDataHandle) {
            CORRADE_FAIL("This is expected to not be called.");
            return true;
        }));
    } else if(data.layouter) {
        LayouterHandle customLayouterHandle = customLayouter->handle();
        ui.removeLayouter(customLayouterHandle);
        CustomLayouter& customLayouterReplacement = ui.setLayouterInstance(Containers::pointer<CustomLayouter>(ui.createLayouter()));
        CORRADE_COMPARE(layouterHandleId(customLayouterReplacement.handle()), layouterHandleId(customLayouterHandle));
        customLayouterReplacement.add(0, node5);
        CORRADE_VERIFY(!layer.highlightNodes(customLayouterReplacement, [](const CustomLayouter&, LayouterDataHandle) {
            CORRADE_FAIL("This is expected to not be called.");
            return true;
        }));
    } else if(data.animator) {
        AnimatorHandle customAnimatorHandle = customAnimator->handle();
        ui.removeAnimator(customAnimatorHandle);
        CustomAnimator& customAnimatorReplacement = ui.setGenericAnimatorInstance(Containers::pointer<CustomAnimator>(ui.createAnimator()));
        CORRADE_COMPARE(animatorHandleId(customAnimatorReplacement.handle()), animatorHandleId(customAnimatorHandle));
        customAnimatorReplacement.create(0, node5);
        CORRADE_VERIFY(!layer.highlightNodes(customAnimatorReplacement, [](const CustomAnimator&, AnimatorDataHandle) {
            CORRADE_FAIL("This is expected to not be called.");
            return true;
        }));
    } else CORRADE_INTERNAL_ASSERT_UNREACHABLE();
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);
}

bool layerCondition(const AbstractLayer& layer, LayerDataHandle data) {
    ++conditionCalled;
    return layerDataHandleId(data) == nodeHandleId(layer.node(data));
}
bool layouterCondition(const AbstractLayouter& layouter, LayouterDataHandle layout) {
    ++conditionCalled;
    return layouterDataHandleId(layout) == nodeHandleId(layouter.node(layout));
}
bool animatorCondition(const AbstractAnimator& animator, AnimatorDataHandle animation) {
    ++conditionCalled;
    return animatorDataHandleId(animation) == nodeHandleId(animator.node(animation));
}

void DebugLayerTest::nodeHighlightConditionDataFunctions() {
    auto&& data = NodeHighlightConditionDataFunctionsData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui{{100, 100}};

    DebugLayer& layer = ui.setLayerInstance(Containers::pointer<DebugLayer>(ui.createLayer(), DebugLayerSource::Nodes|data.sources, DebugLayerFlags{}));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);

    /*NodeHandle node0 =*/ ui.createNode({}, {});
    NodeHandle node1 = ui.createNode({}, {});
    NodeHandle node2 = ui.createNode({}, {});
    NodeHandle node3 = ui.createNode({}, {});

    struct EmptyLayer: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override { return {}; }
    };
    struct EmptyLayouter: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::add;

        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&) override {}
    };
    struct EmptyAnimator: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;
        using AbstractGenericAnimator::create;

        AnimatorFeatures doFeatures() const override { return AnimatorFeature::NodeAttachment; }
        void doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
    };

    EmptyLayer* emptyLayer{};
    EmptyLayouter* emptyLayouter{};
    EmptyAnimator* emptyAnimator{};
    if(data.layer) {
        emptyLayer = &ui.setLayerInstance(Containers::pointer<EmptyLayer>(ui.createLayer()));
        emptyLayer->create(node3);
        emptyLayer->create(node1);
        emptyLayer->create(node2);
    } else if(data.layouter) {
        emptyLayouter = &ui.setLayouterInstance(Containers::pointer<EmptyLayouter>(ui.createLayouter()));
        emptyLayouter->add(node3);
        emptyLayouter->add(node1);
        emptyLayouter->add(node2);
    } else if(data.animator) {
        emptyAnimator = &ui.setGenericAnimatorInstance(Containers::pointer<EmptyAnimator>(ui.createAnimator()));
        emptyAnimator->create({}, {}, node3);
        emptyAnimator->create({}, {}, node1);
        emptyAnimator->create({}, {}, node2);
    } else CORRADE_INTERNAL_ASSERT_UNREACHABLE();

    /* Function taking a base class, selects node1 and node3 */
    ui.update();
    if(data.layer)
        CORRADE_VERIFY(layer.highlightNodes(*emptyLayer, [](const AbstractLayer& layer, LayerDataHandle data) {
            ++conditionCalled;
            return nodeHandleId(layer.node(data)) % 2 != 0;
        }));
    else if(data.layouter)
        CORRADE_VERIFY(layer.highlightNodes(*emptyLayouter, [](const AbstractLayouter& layouter, LayouterDataHandle layout) {
            ++conditionCalled;
            return nodeHandleId(layouter.node(layout)) % 2 != 0;
        }));
    else if(data.animator)
        CORRADE_VERIFY(layer.highlightNodes(*emptyAnimator, [](const AbstractAnimator& animator, AnimatorDataHandle animation) {
            ++conditionCalled;
            return nodeHandleId(animator.node(animation)) % 2 != 0;
        }));
    else CORRADE_INTERNAL_ASSERT_UNREACHABLE();
    CORRADE_COMPARE(conditionCalled, 3);
    CORRADE_COMPARE_AS(layer.currentHighlightedNodes(), Containers::stridedArrayView<bool>({
        false,
        true,
        false,
        true,
    }).sliceBit(0), TestSuite::Compare::Container);

    /* Raw function pointer, selects node2 and node3 */
    layer.clearHighlightedNodes();
    if(data.layer) {
        bool(*condition)(const EmptyLayer&, LayerDataHandle) = [](const EmptyLayer&, LayerDataHandle data) {
            ++conditionCalled;
            return layerDataHandleId(data) % 2 == 0;
        };
        CORRADE_VERIFY(layer.highlightNodes(*emptyLayer, condition));
    } else if(data.layouter) {
        bool(*condition)(const EmptyLayouter&, LayouterDataHandle) = [](const EmptyLayouter&, LayouterDataHandle layout) {
            ++conditionCalled;
            return layouterDataHandleId(layout) % 2 == 0;
        };
        CORRADE_VERIFY(layer.highlightNodes(*emptyLayouter, condition));
    } else if(data.animator) {
        bool(*condition)(const EmptyAnimator&, AnimatorDataHandle) = [](const EmptyAnimator&, AnimatorDataHandle animation) {
            ++conditionCalled;
            return animatorDataHandleId(animation) % 2 == 0;
        };
        CORRADE_VERIFY(layer.highlightNodes(*emptyAnimator, condition));
    } else CORRADE_INTERNAL_ASSERT_UNREACHABLE();
    CORRADE_COMPARE(conditionCalled, 3*2);
    CORRADE_COMPARE_AS(layer.currentHighlightedNodes(), Containers::stridedArrayView<bool>({
        false,
        false,
        true,
        true,
    }).sliceBit(0), TestSuite::Compare::Container);

    /* Plain function, taking a base class again, selects node1 and node2 */
    layer.clearHighlightedNodes();
    if(data.layer)
        CORRADE_VERIFY(layer.highlightNodes(*emptyLayer, layerCondition));
    else if(data.layouter)
        CORRADE_VERIFY(layer.highlightNodes(*emptyLayouter, layouterCondition));
    else if(data.animator)
        CORRADE_VERIFY(layer.highlightNodes(*emptyAnimator, animatorCondition));
    else CORRADE_INTERNAL_ASSERT_UNREACHABLE();
    CORRADE_COMPARE(conditionCalled, 3*3);
    CORRADE_COMPARE_AS(layer.currentHighlightedNodes(), Containers::stridedArrayView<bool>({
        false,
        true,
        true,
        false,
    }).sliceBit(0), TestSuite::Compare::Container);

    /* Functions taking derived classes should not compile */
    #if 0
    struct DerivedLayer: EmptyLayer {};
    struct DerivedLayouter: EmptyLayouter {};
    struct DerivedAnimator: EmptyAnimator {};
    layer.highlightNodes(*emptyLayer, [](const DerivedLayer&, LayerDataHandle) { return true; });
    layer.highlightNodes(*emptyLayouter, [](const DerivedLayouter&, LayouterDataHandle) { return true; });
    layer.highlightNodes(*emptyAnimator, [](const DerivedAnimator&, AnimatorDataHandle) { return true; });
    #endif
}

void DebugLayerTest::nodeHighlightNodeRemoved() {
    auto&& data = NodeInspectHighlightNodeRemovedData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui{{100, 100}};

    DebugLayer& layer = ui.setLayerInstance(Containers::pointer<DebugLayer>(ui.createLayer(), DebugLayerSource::Nodes, DebugLayerFlags{}));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);

    /* A bunch of nodes to highlight */
    NodeHandle parent = ui.createNode({}, {});
    NodeHandle parent2 = ui.createNode(parent, {}, {});
    NodeHandle node2 = ui.createNode(parent2, {}, {});
    /*NodeHandle node3 =*/ ui.createNode({}, {});
    NodeHandle node4 = ui.createNode({}, {});
    NodeHandle node5 = ui.createNode({}, {});

    /* Updating fills the mask for all nodes */
    ui.update();
    CORRADE_COMPARE_AS(layer.currentHighlightedNodes(), Containers::stridedArrayView<bool>({
        false,
        false,
        false,
        false,
        false,
        false,
    }).sliceBit(0), TestSuite::Compare::Container);

    /* Remove one node, remove & recycle another, then highlight the
       now-removed as well as another one that isn't removed. All of that
       should work. */
    ui.removeNode(node4);
    ui.removeNode(data.removeParent ? parent : node2);
    NodeHandle node4Replacement = ui.createNode({}, {});
    CORRADE_COMPARE(nodeHandleId(node4Replacement), nodeHandleId(node4));
    CORRADE_VERIFY(layer.highlightNode(node2));
    CORRADE_VERIFY(layer.highlightNode(node4));
    CORRADE_VERIFY(layer.highlightNode(node5));
    CORRADE_COMPARE_AS(layer.currentHighlightedNodes(), Containers::stridedArrayView<bool>({
        false,
        false,
        true,
        false,
        true,
        true,
    }).sliceBit(0), TestSuite::Compare::Container);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeClean);

    /* Updating clears the now-removed but keeps the remaining selected */
    /** @todo change to just clean() once that's done there */
    ui.update();
    CORRADE_COMPARE_AS(layer.currentHighlightedNodes(), Containers::stridedArrayView<bool>({
        false,
        false,
        false,
        false,
        false,
        true,
    }).sliceBit(0), TestSuite::Compare::Container);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataUpdate);

    /* Highlighting the original node doesn't work anymore, but the replacement
       does */
    CORRADE_VERIFY(!layer.highlightNode(node4));
    CORRADE_VERIFY(layer.highlightNode(node4Replacement));
    CORRADE_COMPARE_AS(layer.currentHighlightedNodes(), Containers::stridedArrayView<bool>({
        false,
        false,
        false,
        false,
        true,
        true,
    }).sliceBit(0), TestSuite::Compare::Container);
}

void DebugLayerTest::nodeHighlightInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui{{100, 100}};
    AbstractUserInterface uiAnother{{100, 100}};
    DebugLayer layerNoNodes{layerHandle(0, 1), {}, {}};
    DebugLayer& layerOnlyNodes = ui.setLayerInstance(Containers::pointer<DebugLayer>(ui.createLayer(), DebugLayerSource::Nodes, DebugLayerFlags{}));
    DebugLayer& layerOnlyLayers = ui.setLayerInstance(Containers::pointer<DebugLayer>(ui.createLayer(), DebugLayerSource::Layers, DebugLayerFlags{}));
    DebugLayer& layerOnlyLayouters = ui.setLayerInstance(Containers::pointer<DebugLayer>(ui.createLayer(), DebugLayerSource::Layouters, DebugLayerFlags{}));
    DebugLayer& layerOnlyAnimators = ui.setLayerInstance(Containers::pointer<DebugLayer>(ui.createLayer(), DebugLayerSource::Animators, DebugLayerFlags{}));
    DebugLayer layerNoUi{layerHandle(0, 1), DebugLayerSource::Nodes|DebugLayerSource::Layers|DebugLayerSource::Layouters|DebugLayerSource::Animators, {}};
    DebugLayer& layer = ui.setLayerInstance(Containers::pointer<DebugLayer>(ui.createLayer(), DebugLayerSource::Nodes|DebugLayerSource::Layers|DebugLayerSource::Layouters|DebugLayerSource::Animators, DebugLayerFlags{}));

    struct EmptyLayer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    };
    EmptyLayer& emptyLayer = ui.setLayerInstance(Containers::pointer<EmptyLayer>(ui.createLayer()));
    EmptyLayer& layerAnotherUi = uiAnother.setLayerInstance(Containers::pointer<EmptyLayer>(uiAnother.createLayer()));
    EmptyLayer layerArtificialHandle{layerHandle(0xab, 0x12)};

    struct EmptyLayouter: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;

        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>&, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&) override {}
    };
    EmptyLayouter& emptyLayouter = ui.setLayouterInstance(Containers::pointer<EmptyLayouter>(ui.createLayouter()));
    EmptyLayouter& layouterAnotherUi = uiAnother.setLayouterInstance(Containers::pointer<EmptyLayouter>(uiAnother.createLayouter()));
    EmptyLayouter layouterArtificialHandle{layouterHandle(0xab, 0x12)};

    struct EmptyAnimator: AbstractGenericAnimator {
        explicit EmptyAnimator(AnimatorHandle handle, AnimatorFeatures features): AbstractGenericAnimator{handle}, _features{features} {}

        AnimatorFeatures doFeatures() const override { return _features; }
        void doAdvance(Containers::BitArrayView, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}

        private:
            AnimatorFeatures _features;
    };
    EmptyAnimator& emptyAnimator = ui.setGenericAnimatorInstance(Containers::pointer<EmptyAnimator>(ui.createAnimator(), AnimatorFeature::NodeAttachment));
    EmptyAnimator& animatorAnotherUi = uiAnother.setGenericAnimatorInstance(Containers::pointer<EmptyAnimator>(uiAnother.createAnimator(), AnimatorFeature::NodeAttachment));
    EmptyAnimator animatorArtificialHandle{animatorHandle(0xab, 0x12), AnimatorFeature::NodeAttachment};
    EmptyAnimator& animatorNoNodeAttachments = ui.setGenericAnimatorInstance(Containers::pointer<EmptyAnimator>(ui.createAnimator(), AnimatorFeatures{}));

    /* Calling functionality getters / setters is valid on a layer that doesn't
       have the feature enabled or isn't part of the UI. The actual state
       queries and updates can't be called tho. */
    layerNoNodes.nodeHighlightColorMap();
    layerNoNodes.nodeHighlightColorMapAlpha();
    Color3ub colormap[1];
    layerNoNodes.setNodeHighlightColorMap(colormap);

    Containers::String out;
    Error redirectError{&out};
    layerNoNodes.setNodeHighlightColorMap({});

    layerNoNodes.currentHighlightedNodes();
    layerNoUi.currentHighlightedNodes();

    layerNoNodes.clearHighlightedNodes();
    layerNoUi.clearHighlightedNodes();

    layerNoNodes.highlightNode(nodeHandle(0, 1));
    layerNoUi.highlightNode(nodeHandle(0, 1));
    layer.highlightNode(NodeHandle::Null);

    layerNoNodes.highlightNodes([](const AbstractUserInterface&, NodeHandle) {
        return false;
    });
    layerNoNodes.highlightNodes(emptyLayer, [](const EmptyLayer&, LayerDataHandle) {
        return false;
    });
    layerNoNodes.highlightNodes(emptyLayouter, [](const EmptyLayouter&, LayouterDataHandle) {
        return false;
    });
    layerNoNodes.highlightNodes(emptyAnimator, [](const EmptyAnimator&, AnimatorDataHandle) {
        return false;
    });
    layerOnlyLayers.highlightNodes(emptyLayer, [](const EmptyLayer&, LayerDataHandle) {
        return false;
    });
    layerOnlyLayouters.highlightNodes(emptyLayouter, [](const EmptyLayouter&, LayouterDataHandle) {
        return false;
    });
    layerOnlyAnimators.highlightNodes(emptyAnimator, [](const EmptyAnimator&, AnimatorDataHandle) {
        return false;
    });
    layerOnlyNodes.highlightNodes(emptyLayer, [](const EmptyLayer&, LayerDataHandle) {
        return false;
    });
    layerOnlyNodes.highlightNodes(emptyLayouter, [](const EmptyLayouter&, LayouterDataHandle) {
        return false;
    });
    layerOnlyNodes.highlightNodes(emptyAnimator, [](const EmptyAnimator&, AnimatorDataHandle) {
        return false;
    });

    layerNoUi.highlightNodes([](const AbstractUserInterface&, NodeHandle) {
        return false;
    });
    layerNoUi.highlightNodes(emptyLayer, [](const EmptyLayer&, LayerDataHandle) {
        return false;
    });
    layerNoUi.highlightNodes(emptyLayouter, [](const EmptyLayouter&, LayouterDataHandle) {
        return false;
    });
    layerNoUi.highlightNodes(emptyAnimator, [](const EmptyAnimator&, AnimatorDataHandle) {
        return false;
    });

    layer.highlightNodes(layerAnotherUi, [](const EmptyLayer&, LayerDataHandle) {
        return false;
    });
    layer.highlightNodes(layerArtificialHandle, [](const EmptyLayer&, LayerDataHandle) {
        return false;
    });
    layer.highlightNodes(layouterAnotherUi, [](const EmptyLayouter&, LayouterDataHandle) {
        return false;
    });
    layer.highlightNodes(layouterArtificialHandle, [](const EmptyLayouter&, LayouterDataHandle) {
        return false;
    });
    layer.highlightNodes(animatorAnotherUi, [](const EmptyAnimator&, AnimatorDataHandle) {
        return false;
    });
    layer.highlightNodes(animatorArtificialHandle, [](const EmptyAnimator&, AnimatorDataHandle) {
        return false;
    });

    layer.highlightNodes(layer, [](const DebugLayer&, LayerDataHandle) {
        return false;
    });
    layer.highlightNodes(animatorNoNodeAttachments, [](const EmptyAnimator&, AnimatorDataHandle) {
        return false;
    });

    layer.highlightNodes(nullptr);
    layer.highlightNodes(emptyLayer, static_cast<bool(*)(const EmptyLayer&, LayerDataHandle)>(nullptr));
    layer.highlightNodes(emptyLayouter, static_cast<bool(*)(const EmptyLayouter&, LayouterDataHandle)>(nullptr));
    layer.highlightNodes(emptyAnimator, static_cast<bool(*)(const EmptyAnimator&, AnimatorDataHandle)>(nullptr));
    CORRADE_COMPARE_AS(out,
        "Ui::DebugLayer::setNodeHighlightColorMap(): expected colormap to have at least one element\n"

        "Ui::DebugLayer::currentHighlightedNodes(): Ui::DebugLayerSource::Nodes not enabled\n"
        "Ui::DebugLayer::currentHighlightedNodes(): layer not part of a user interface\n"

        "Ui::DebugLayer::clearHighlightedNodes(): Ui::DebugLayerSource::Nodes not enabled\n"
        "Ui::DebugLayer::clearHighlightedNodes(): layer not part of a user interface\n"

        "Ui::DebugLayer::highlightNode(): Ui::DebugLayerSource::Nodes not enabled\n"
        "Ui::DebugLayer::highlightNode(): layer not part of a user interface\n"
        "Ui::DebugLayer::highlightNode(): handle is null\n"

        "Ui::DebugLayer::highlightNodes(): Ui::DebugLayerSource::Nodes not enabled\n"
        "Ui::DebugLayer::highlightNodes(): Ui::DebugLayerSource::Nodes|Ui::DebugLayerSource::Layers not enabled\n"
        "Ui::DebugLayer::highlightNodes(): Ui::DebugLayerSource::Nodes|Ui::DebugLayerSource::Layouters not enabled\n"
        "Ui::DebugLayer::highlightNodes(): Ui::DebugLayerSource::Nodes|Ui::DebugLayerSource::Animators not enabled\n"
        "Ui::DebugLayer::highlightNodes(): Ui::DebugLayerSource::Nodes not enabled\n"
        "Ui::DebugLayer::highlightNodes(): Ui::DebugLayerSource::Nodes not enabled\n"
        "Ui::DebugLayer::highlightNodes(): Ui::DebugLayerSource::Nodes not enabled\n"
        "Ui::DebugLayer::highlightNodes(): Ui::DebugLayerSource::Layers not enabled\n"
        "Ui::DebugLayer::highlightNodes(): Ui::DebugLayerSource::Layouters not enabled\n"
        "Ui::DebugLayer::highlightNodes(): Ui::DebugLayerSource::Animators not enabled\n"

        "Ui::DebugLayer::highlightNodes(): debug layer not part of a user interface\n"
        "Ui::DebugLayer::highlightNodes(): debug layer not part of a user interface\n"
        "Ui::DebugLayer::highlightNodes(): debug layer not part of a user interface\n"
        "Ui::DebugLayer::highlightNodes(): debug layer not part of a user interface\n"

        "Ui::DebugLayer::highlightNodes(): layer not part of the same user interface\n"
        "Ui::DebugLayer::highlightNodes(): layer not part of the same user interface\n"
        "Ui::DebugLayer::highlightNodes(): layouter not part of the same user interface\n"
        "Ui::DebugLayer::highlightNodes(): layouter not part of the same user interface\n"
        "Ui::DebugLayer::highlightNodes(): animator not part of the same user interface\n"
        "Ui::DebugLayer::highlightNodes(): animator not part of the same user interface\n"

        "Ui::DebugLayer::highlightNodes(): can't highlight with a condition on the debug layer itself\n"
        "Ui::DebugLayer::highlightNodes(): only animators with Ui::AnimatorFeature::NodeAttachment can be used\n"

        "Ui::DebugLayer::highlightNodes(): condition is null\n"
        "Ui::DebugLayer::highlightNodes(): condition is null\n"
        "Ui::DebugLayer::highlightNodes(): condition is null\n"
        "Ui::DebugLayer::highlightNodes(): condition is null\n",
        TestSuite::Compare::String);
}

void DebugLayerTest::updateEmpty() {
    auto&& data = LayerDrawData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct Layer: DebugLayer {
        explicit Layer(LayerHandle handle, DebugLayerSources sources, DebugLayerFlags flags, LayerFeatures features): DebugLayer{handle, sources, flags}, _features{features} {}

        LayerFeatures doFeatures() const override {
            return DebugLayer::doFeatures()|_features;
        }

        private:
            LayerFeatures _features;
    } layer{layerHandle(0, 1), DebugLayerSource::Nodes, DebugLayerFlag::NodeInspect, data.features};

    /* Required to be called before update() (because AbstractUserInterface
       guarantees the same on a higher level), not needed for anything here */
    if(data.features >= LayerFeature::Draw)
        layer.setSize({1, 1}, {1, 1});

    /* Shouldn't crash or do anything weird */
    layer.update(LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsNodeOpacityUpdate|LayerState::NeedsNodeOrderUpdate|LayerState::NeedsNodeOffsetSizeUpdate|LayerState::NeedsAttachmentUpdate|LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate|LayerState::NeedsSharedDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_VERIFY(true);
}

void DebugLayerTest::updateDataOrder() {
    auto&& data = UpdateDataOrderData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui{{100, 100}};

    struct Layer: DebugLayer {
        explicit Layer(LayerHandle handle, DebugLayerSources sources, DebugLayerFlags flags, LayerFeatures features): DebugLayer{handle, sources, flags}, _features{features} {}

        DebugLayer::State& stateData() {
            return *_state;
        }

        LayerFeatures doFeatures() const override {
            return DebugLayer::doFeatures()|_features;
        }

        private:
            LayerFeatures _features;
    };
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), DebugLayerSource::Nodes, data.flags, data.features));
    /* Just to silence the output */
    layer.setNodeInspectCallback([](Containers::StringView){});
    /* Just to verify the color is actually used */
    layer.setNodeInspectColor(0xff3366cc_rgbaf);

    /* Colormap so every node below is interpolated _exactly_ on a dedicated
       entry */
    Color3ub colormap[]{
        0xff0000_rgb,
        0x00ff00_rgb, /* node1 */
        0x0000ff_rgb,
        0x00ffff_rgb, /* node3 */
        0xff00ff_rgb,
        0xffff00_rgb,
        0xffffff_rgb, /* node6 */
        0x000000_rgb
    };
    layer.setNodeHighlightColorMap(colormap, 0.5f);

    /* Create nodes in a way that there's a non-trivial mapping from node IDs
       to debug layer data IDs, as checked below */
    ui.createNode({}, {});
    NodeHandle node1 = ui.createNode({}, {});
    NodeHandle removedNode2 = ui.createNode({}, {});
    NodeHandle node3 = ui.createNode({}, {});
    NodeHandle removedNode4 = ui.createNode({}, {});
    ui.createNode({}, {});
    NodeHandle node6 = ui.createNode({}, {});
    NodeHandle node7 = ui.createNode({}, {});
    ui.removeNode(removedNode2);
    ui.removeNode(removedNode4);
    CORRADE_COMPARE(nodeHandleId(node1), 1);
    CORRADE_COMPARE(nodeHandleId(node3), 3);
    CORRADE_COMPARE(nodeHandleId(node6), 6);

    /* Update to make the debug layer aware of all nodes, highlight two nodes
       that are among dataIds */
    layer.preUpdate(LayerState::NeedsCommonDataUpdate);
    CORRADE_VERIFY(layer.highlightNode(node1));
    CORRADE_VERIFY(layer.highlightNode(node3));
    CORRADE_COMPARE_AS(layer.currentHighlightedNodes(), Containers::stridedArrayView({
        false,
        true,
        false,
        true,
        false,
        false,
        false,
        false,
    }).sliceBit(0), TestSuite::Compare::Container);

    /* For NodeInspect all known nodes get a matching data, for node highlight
       only those that are highlighted */
    layer.preUpdate(LayerState::NeedsCommonDataUpdate);
    if(data.flags >= DebugLayerFlag::NodeInspect) {
        CORRADE_COMPARE(layer.usedCount(), 6);
        CORRADE_COMPARE(layer.node(layerDataHandle(1, 1)), node1);
        CORRADE_COMPARE(layer.node(layerDataHandle(2, 1)), node3);
        CORRADE_COMPARE(layer.node(layerDataHandle(4, 1)), node6);
    } else {
        CORRADE_COMPARE(layer.usedCount(), 2);
        CORRADE_COMPARE(layer.node(layerDataHandle(0, 1)), node1);
        CORRADE_COMPARE(layer.node(layerDataHandle(1, 1)), node3);
    }

    /* Opacities and node enablement status are not used by the layer */
    Vector2 nodeOffsets[7];
    Vector2 nodeSizes[7];
    Float nodeOpacities[7];
    UnsignedByte nodesEnabledData[1];
    Containers::MutableBitArrayView nodesEnabled{nodesEnabledData, 0, 7};
    nodeOffsets[1] = {10.0f, 20.0f};
    nodeOffsets[3] = {20.0f, 10.0f};
    nodeOffsets[6] = {30.0f, 0.0f};
    nodeSizes[1] = {30.0f, 40.0f};
    nodeSizes[3] = {40.0f, 30.0f};
    nodeSizes[6] = {50.0f, 20.0f};

    /* An empty update should generate empty draw offsets and everything else */
    if(data.emptyUpdate) {
        layer.update(data.states, {}, {}, {}, nodeOffsets, nodeSizes, nodeOpacities, nodesEnabled, {}, {}, {}, {});
        CORRADE_COMPARE_AS(layer.stateData().highlightedNodeDrawOffsets,
            Containers::arrayView<UnsignedInt>({}),
            TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(layer.stateData().highlightedNodeIndices,
            Containers::arrayView<UnsignedInt>({}),
            TestSuite::Compare::Container);
        auto vertices = Containers::stridedArrayView(layer.stateData().highlightedNodeVertices);
        CORRADE_COMPARE_AS(vertices.slice(&decltype(vertices)::Type::position),
            Containers::stridedArrayView<Vector2>({}),
            TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(vertices.slice(&decltype(vertices)::Type::color),
            Containers::stridedArrayView<Color4>({}),
            TestSuite::Compare::Container);

        return;
    }

    /* In case of node inspect, data ID 5, attached to node 7, is not passed,
       thus its quad isn't present even if highlighted. In case of just node
       highlight, we have only exactly the data for nodes we want to highlight
       so far. */
    UnsignedInt dataIdsInspect[]{
        3,
        2, /* node 3 */
        4, /* node 6 */
        1, /* node 1 */
        0,
    };
    UnsignedInt dataIdsHighlight[]{
        1, /* node 3 */
        0, /* node 1 */
    };
    Containers::ArrayView<const UnsignedInt> dataIds =
        data.flags >= DebugLayerFlag::NodeInspect ?
            Containers::arrayView(dataIdsInspect) :
            Containers::arrayView(dataIdsHighlight);

    /* The initial highlight produces two quads */
    layer.update(data.states, dataIds, {}, {}, nodeOffsets, nodeSizes, nodeOpacities, nodesEnabled, {}, {}, {}, {});
    if(data.expectDataUpdated) {
        if(data.flags >= DebugLayerFlag::NodeInspect)
            CORRADE_COMPARE_AS(layer.stateData().highlightedNodeDrawOffsets, Containers::arrayView<UnsignedInt>({
                0,
                0, /* quad for node 3 */
                1,
                1, /* quad for node 1 */
                2,
                2, /* sentinel */
            }), TestSuite::Compare::Container);
        else
            CORRADE_COMPARE_AS(layer.stateData().highlightedNodeDrawOffsets, Containers::arrayView<UnsignedInt>({
                0, /* quad for node 3 */
                1, /* quad for node 1 */
                2, /* sentinel */
            }), TestSuite::Compare::Container);

        /* Indices are always the same, just different count of them */
        CORRADE_COMPARE_AS(layer.stateData().highlightedNodeIndices, Containers::arrayView<UnsignedInt>({
            0, 2, 1,
            2, 3, 1,

            4, 6, 5,
            6, 7, 5
        }), TestSuite::Compare::Container);

        auto vertices = Containers::stridedArrayView(layer.stateData().highlightedNodeVertices);
        CORRADE_COMPARE_AS(vertices.slice(&decltype(vertices)::Type::position), Containers::stridedArrayView<Vector2>({
            {20.0f, 10.0f}, /* node3 */
            {60.0f, 10.0f},
            {20.0f, 40.0f},
            {60.0f, 40.0f},

            {10.0f, 20.0f}, /* node1 */
            {40.0f, 20.0f},
            {10.0f, 60.0f},
            {40.0f, 60.0f},
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(vertices.slice(&decltype(vertices)::Type::color), Containers::stridedArrayView<Color4>({
            0x00ffffff_rgbaf*0.5f, /* node3 */
            0x00ffffff_rgbaf*0.5f,
            0x00ffffff_rgbaf*0.5f,
            0x00ffffff_rgbaf*0.5f,

            0x00ff00ff_rgbaf*0.5f, /* node1 */
            0x00ff00ff_rgbaf*0.5f,
            0x00ff00ff_rgbaf*0.5f,
            0x00ff00ff_rgbaf*0.5f,
        }), TestSuite::Compare::Container);
    } else CORRADE_COMPARE_AS(layer.stateData().highlightedNodeDrawOffsets,
        Containers::arrayView<UnsignedInt>({}),
        TestSuite::Compare::Container);

    /* Highlight a node that isn't among dataIds, results in no difference in
       actually drawn data. If we're not inspecting, it however results in one
       extra data being created for it in doPreUpdate(). */
    CORRADE_VERIFY(layer.highlightNode(node7));
    layer.preUpdate(LayerState::NeedsCommonDataUpdate);
    layer.update(data.states, dataIds, {}, {}, nodeOffsets, nodeSizes, nodeOpacities, nodesEnabled, {}, {}, {}, {});
    if(!(data.flags >= DebugLayerFlag::NodeInspect)) {
        CORRADE_COMPARE(layer.usedCount(), 3);
        CORRADE_COMPARE(layer.node(layerDataHandle(2, 1)), node7);
    }
    if(data.expectDataUpdated) {
        if(data.flags >= DebugLayerFlag::NodeInspect)
            CORRADE_COMPARE_AS(layer.stateData().highlightedNodeDrawOffsets, Containers::arrayView<UnsignedInt>({
                0,
                0, /* quad for node 3 */
                1,
                1, /* quad for node 1 */
                2,
                2, /* sentinel */
            }), TestSuite::Compare::Container);
        else
            CORRADE_COMPARE_AS(layer.stateData().highlightedNodeDrawOffsets, Containers::arrayView<UnsignedInt>({
                0, /* quad for node 3 */
                1, /* quad for node 1 */
                2, /* sentinel */
            }), TestSuite::Compare::Container);

        CORRADE_COMPARE_AS(layer.stateData().highlightedNodeIndices, Containers::arrayView<UnsignedInt>({
            0, 2, 1,
            2, 3, 1,

            4, 6, 5,
            6, 7, 5
        }), TestSuite::Compare::Container);

        auto vertices = Containers::stridedArrayView(layer.stateData().highlightedNodeVertices);
        CORRADE_COMPARE_AS(vertices.slice(&decltype(vertices)::Type::position), Containers::stridedArrayView<Vector2>({
            {20.0f, 10.0f}, /* node3 */
            {60.0f, 10.0f},
            {20.0f, 40.0f},
            {60.0f, 40.0f},

            {10.0f, 20.0f}, /* node1 */
            {40.0f, 20.0f},
            {10.0f, 60.0f},
            {40.0f, 60.0f},
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(vertices.slice(&decltype(vertices)::Type::color), Containers::stridedArrayView<Color4>({
            0x00ffffff_rgbaf*0.5f, /* node3 */
            0x00ffffff_rgbaf*0.5f,
            0x00ffffff_rgbaf*0.5f,
            0x00ffffff_rgbaf*0.5f,

            0x00ff00ff_rgbaf*0.5f, /* node1 */
            0x00ff00ff_rgbaf*0.5f,
            0x00ff00ff_rgbaf*0.5f,
            0x00ff00ff_rgbaf*0.5f,
        }), TestSuite::Compare::Container);
    } else CORRADE_COMPARE_AS(layer.stateData().highlightedNodeDrawOffsets,
        Containers::arrayView<UnsignedInt>({}),
        TestSuite::Compare::Container);

    /* If we're inspecting, test also inspecting vs highlight */
    if(data.flags >= DebugLayerFlag::NodeInspect) {
        /* Inspect one of the present nodes, should result just in color
           change, everything else the same */
        CORRADE_VERIFY(layer.inspectNode(node1));
        layer.update(data.states, dataIds, {}, {}, nodeOffsets, nodeSizes, nodeOpacities, nodesEnabled, {}, {}, {}, {});
        if(data.expectDataUpdated) {
            CORRADE_COMPARE_AS(layer.stateData().highlightedNodeDrawOffsets, Containers::arrayView<UnsignedInt>({
                0,
                0, /* quad for node 3 */
                1,
                1, /* quad for node 1 */
                2,
                2, /* sentinel */
            }), TestSuite::Compare::Container);

            CORRADE_COMPARE_AS(layer.stateData().highlightedNodeIndices, Containers::arrayView<UnsignedInt>({
                0, 2, 1,
                2, 3, 1,

                4, 6, 5,
                6, 7, 5
            }), TestSuite::Compare::Container);

            auto vertices = Containers::stridedArrayView(layer.stateData().highlightedNodeVertices);
            CORRADE_COMPARE_AS(vertices.slice(&decltype(vertices)::Type::position), Containers::stridedArrayView<Vector2>({
                {20.0f, 10.0f}, /* node3 */
                {60.0f, 10.0f},
                {20.0f, 40.0f},
                {60.0f, 40.0f},

                {10.0f, 20.0f}, /* node1 */
                {40.0f, 20.0f},
                {10.0f, 60.0f},
                {40.0f, 60.0f},
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(vertices.slice(&decltype(vertices)::Type::color), Containers::stridedArrayView<Color4>({
                0x00ffffff_rgbaf*0.5f, /* node3 */
                0x00ffffff_rgbaf*0.5f,
                0x00ffffff_rgbaf*0.5f,
                0x00ffffff_rgbaf*0.5f,

                0xff3366cc_rgbaf, /* node1, changed */
                0xff3366cc_rgbaf,
                0xff3366cc_rgbaf,
                0xff3366cc_rgbaf,
            }), TestSuite::Compare::Container);
        } else CORRADE_COMPARE_AS(layer.stateData().highlightedNodeDrawOffsets,
            Containers::arrayView<UnsignedInt>({}),
            TestSuite::Compare::Container);

        /* Inspect a node that isn't highlighted, the color of the previously
           inspected should change back and the index buffer grow by one
           quad */
        CORRADE_VERIFY(layer.inspectNode(node6));
        layer.update(data.states, dataIds, {}, {}, nodeOffsets, nodeSizes, nodeOpacities, nodesEnabled, {}, {}, {}, {});
        if(data.expectDataUpdated) {
            CORRADE_COMPARE_AS(layer.stateData().highlightedNodeDrawOffsets, Containers::arrayView<UnsignedInt>({
                0,
                0, /* quad for node 3 */
                1, /* quad for node 6 */
                2, /* quad for node 1 */
                3,
                3, /* sentinel */
            }), TestSuite::Compare::Container);

            CORRADE_COMPARE_AS(layer.stateData().highlightedNodeIndices, Containers::arrayView<UnsignedInt>({
                0, 2, 1,
                2, 3, 1,

                4, 6, 5,
                6, 7, 5,

                8, 10, 9,
                10, 11, 9
            }), TestSuite::Compare::Container);

            auto vertices = Containers::stridedArrayView(layer.stateData().highlightedNodeVertices);
            CORRADE_COMPARE_AS(vertices.slice(&decltype(vertices)::Type::position), Containers::stridedArrayView<Vector2>({
                {20.0f, 10.0f}, /* node3 */
                {60.0f, 10.0f},
                {20.0f, 40.0f},
                {60.0f, 40.0f},

                {30.0f,  0.0f}, /* node6 */
                {80.0f,  0.0f},
                {30.0f, 20.0f},
                {80.0f, 20.0f},

                {10.0f, 20.0f}, /* node1 */
                {40.0f, 20.0f},
                {10.0f, 60.0f},
                {40.0f, 60.0f},
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(vertices.slice(&decltype(vertices)::Type::color), Containers::stridedArrayView<Color4>({
                0x00ffffff_rgbaf*0.5f, /* node3 */
                0x00ffffff_rgbaf*0.5f,
                0x00ffffff_rgbaf*0.5f,
                0x00ffffff_rgbaf*0.5f,

                0xff3366cc_rgbaf, /* node6 */
                0xff3366cc_rgbaf,
                0xff3366cc_rgbaf,
                0xff3366cc_rgbaf,

                0x00ff00ff_rgbaf*0.5f, /* node1 */
                0x00ff00ff_rgbaf*0.5f,
                0x00ff00ff_rgbaf*0.5f,
                0x00ff00ff_rgbaf*0.5f,
            }), TestSuite::Compare::Container);
        } else CORRADE_COMPARE_AS(layer.stateData().highlightedNodeDrawOffsets,
            Containers::arrayView<UnsignedInt>({}),
            TestSuite::Compare::Container);
    }

    /* Remove all highlights, there should be just the inspected node alone
       if there is, the index buffer should stay at the original size */
    layer.clearHighlightedNodes();

    /* If we're inspecting, there's just the inspected quad left */
    if(data.flags >= DebugLayerFlag::NodeInspect) {
        layer.update(data.states, dataIds, {}, {}, nodeOffsets, nodeSizes, nodeOpacities, nodesEnabled, {}, {}, {}, {});
        if(data.expectDataUpdated) {
            CORRADE_COMPARE_AS(layer.stateData().highlightedNodeDrawOffsets, Containers::arrayView<UnsignedInt>({
                0,
                0,
                0, /* quad for node 6 */
                1,
                1,
                1, /* sentinel */
            }), TestSuite::Compare::Container);

            /* Unchanged */
            CORRADE_COMPARE_AS(layer.stateData().highlightedNodeIndices, Containers::arrayView<UnsignedInt>({
                0, 2, 1,
                2, 3, 1,

                4, 6, 5,
                6, 7, 5,

                8, 10, 9,
                10, 11, 9
            }), TestSuite::Compare::Container);

            auto vertices = Containers::stridedArrayView(layer.stateData().highlightedNodeVertices);
            CORRADE_COMPARE_AS(vertices.slice(&decltype(vertices)::Type::position), Containers::stridedArrayView<Vector2>({
                {30.0f,  0.0f}, /* node6 */
                {80.0f,  0.0f},
                {30.0f, 20.0f},
                {80.0f, 20.0f},
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(vertices.slice(&decltype(vertices)::Type::color), Containers::stridedArrayView<Color4>({
                0xff3366cc_rgbaf, /* node6 */
                0xff3366cc_rgbaf,
                0xff3366cc_rgbaf,
                0xff3366cc_rgbaf,
            }), TestSuite::Compare::Container);
        } else CORRADE_COMPARE_AS(layer.stateData().highlightedNodeDrawOffsets,
            Containers::arrayView<UnsignedInt>({}),
            TestSuite::Compare::Container);
    }

    /* If we're inspecting, inspect nothing, the draw offset and vertex array
       should be gone now, indices again untouched. If we're not inspecting,
       the final state is now also empty as we cleared all highlights above. */
    if(data.flags >= DebugLayerFlag::NodeInspect) {
        CORRADE_VERIFY(layer.inspectNode(NodeHandle::Null));
        CORRADE_COMPARE(layer.currentInspectedNode(), NodeHandle::Null);
    }

    layer.update(data.states, dataIds, {}, {}, nodeOffsets, nodeSizes, nodeOpacities, nodesEnabled, {}, {}, {}, {});
    CORRADE_COMPARE_AS(layer.stateData().highlightedNodeDrawOffsets,
        Containers::arrayView<UnsignedInt>({}),
        TestSuite::Compare::Container);
    if(data.expectDataUpdated) {
        CORRADE_COMPARE_AS(layer.stateData().highlightedNodeIndices, Containers::arrayView<UnsignedInt>({
            0, 2, 1,
            2, 3, 1,

            4, 6, 5,
            6, 7, 5,

            8, 10, 9,
            10, 11, 9
        }).exceptSuffix(
            /* If we're inspecting, at most three quads got drawn. If not, at
               most two. */
            data.flags >= DebugLayerFlag::NodeInspect ? 0 : 6
        ), TestSuite::Compare::Container);
    }
    auto vertices = Containers::stridedArrayView(layer.stateData().highlightedNodeVertices);
    CORRADE_COMPARE_AS(vertices.slice(&decltype(vertices)::Type::position),
        Containers::stridedArrayView<Vector2>({}),
        TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(vertices.slice(&decltype(vertices)::Type::color),
        Containers::stridedArrayView<Color4>({}),
        TestSuite::Compare::Container);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::DebugLayerTest)
