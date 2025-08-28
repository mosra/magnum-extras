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
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Container.h>
#include <Corrade/TestSuite/Compare/Numeric.h>
#include <Corrade/TestSuite/Compare/String.h>
#include <Magnum/Math/Color.h>

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

        void construct();
        void constructInvalid();
        void constructCopy();
        void constructMove();

        void flags();
        void flagsInvalid();

        void nodeNameNoOp();
        void nodeName();
        void nodeNameInvalid();

        void layerNameNoOp();
        void layerName();
        void layerNameDebugIntegrationSetup();
        void layerNameDebugIntegrationTeardown();
        void layerNameDebugIntegration();
        void layerNameDebugIntegrationExplicit();
        void layerNameDebugIntegrationExplicitRvalue();
        void layerNameDebugIntegrationCopyConstructPlainStruct();
        void layerNameDebugIntegrationMoveConstructPlainStruct();
        void layerNameInvalid();

        void preUpdateNoUi();
        void preUpdateNoOp();
        void preUpdateTrackNodes();
        void preUpdateTrackLayers();

        void nodeHighlightSetters();
        /* No nodeHighlightNoUi() as pointer press to highlight a node can only
           happen if there's any data, which is only done if preUpdate() is
           called, which already checks that the layer is part of a UI */
        void nodeHighlightNoOp();
        void nodeHighlight();
        void nodeHighlightNoCallback();
        void nodeHighlightDebugIntegrationExplicit();
        void nodeHighlightDebugIntegrationExplicitRvalue();
        void nodeHighlightInvalid();
        void nodeHighlightDraw();
        void nodeHighlightNodeRemoved();
        void nodeHighlightToggle();

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
    DebugLayerFlags flags;
    bool expectNoState, expectNoNodes, expectNoLayers, expectNoData;
} PreUpdateNoOpData[]{
    {"",
        {}, {},
        true, true, true, true},
    {"nodes alone",
        DebugLayerSource::Nodes, {},
        false, false, true, true},
    {"layers alone",
        DebugLayerSource::Layers, {},
        false, true, false, true},
    {"node hierarchy",
        DebugLayerSource::NodeHierarchy, {},
        false, false, true, true},
    {"node data",
        DebugLayerSource::NodeData, {},
        false, false, false, true},
    {"node highlight",
        DebugLayerSource::Nodes, DebugLayerFlag::NodeHighlight,
        false, false, true, false},
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
    {"node highlight",
        DebugLayerSource::Nodes, DebugLayerFlag::NodeHighlight, true},
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
    DebugLayerFlags flags;
    Pointers acceptedPointers;
    PointerEventSource pointerSource;
    Pointer pointer;
    Modifiers modifiers;
    bool primary;
} NodeHighlightNoOpData[]{
    {"nothing enabled",
        {}, {}, {},
        PointerEventSource::Mouse, Pointer::MouseRight, Modifier::Ctrl, true},
    {"node highlight not enabled",
        DebugLayerSource::Nodes, {}, {},
        PointerEventSource::Mouse, Pointer::MouseRight, Modifier::Ctrl, true},
    {"different mouse pointer",
        DebugLayerSource::Nodes, DebugLayerFlag::NodeHighlight, {},
        PointerEventSource::Mouse, Pointer::MouseMiddle, Modifier::Ctrl, true},
    {"different pen pointer",
        DebugLayerSource::Nodes, DebugLayerFlag::NodeHighlight, {},
        PointerEventSource::Pen, Pointer::Pen, Modifier::Ctrl, true},
    {"too little modifiers",
        DebugLayerSource::Nodes, DebugLayerFlag::NodeHighlight, {},
        PointerEventSource::Mouse, Pointer::MouseRight, {}, true},
    {"too many modifiers",
        DebugLayerSource::Nodes, DebugLayerFlag::NodeHighlight, {},
        PointerEventSource::Mouse, Pointer::MouseRight, Modifier::Ctrl|Modifier::Shift, true},
    {"accepting also touches, but the touch is not primary",
        DebugLayerSource::Nodes, DebugLayerFlag::NodeHighlight,
        Pointer::Finger|Pointer::MouseRight,
        PointerEventSource::Touch, Pointer::Finger, Modifier::Ctrl, false},
};

const struct {
    TestSuite::TestCaseDescriptionSourceLocation name;
    DebugLayerSources sources;
    DebugLayerFlags flags;
    Containers::Optional<Containers::StringView> nodeName;
    bool reverseLayerOrder, someLayerNames, allLayerNames;
    Pointers acceptedPointers;
    Modifiers acceptedModifiers;
    PointerEventSource pointerSource;
    Pointer pointer;
    NodeFlags nodeFlags;
    bool nested, nestedTopLevel, children, hiddenChildren, disabledChildren, noEventsChildren;
    const char* expected;
} NodeHighlightData[]{
    {"",
        DebugLayerSource::Nodes, DebugLayerFlag::NodeHighlight,
        {}, false, false, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, true, false, false, false, false, false,
        "Node {0x3, 0x1}"},
    {"different used pointer",
        DebugLayerSource::Nodes, DebugLayerFlag::NodeHighlight,
        {}, false, false, false,
        {}, {}, PointerEventSource::Pen, Pointer::Eraser,
        {}, true, false, false, false, false, false,
        "Node {0x3, 0x1}"},
    {"different accepted and used pointer",
        DebugLayerSource::Nodes, DebugLayerFlag::NodeHighlight,
        {}, false, false, false,
        Pointer::Finger|Pointer::Pen, Modifier::Ctrl|Modifier::Shift|Modifier::Alt,
        PointerEventSource::Pen, Pointer::Pen,
        {}, true, false, false, false, false, false,
        "Node {0x3, 0x1}"},
    {"nested top-level node",
        DebugLayerSource::Nodes, DebugLayerFlag::NodeHighlight,
        "A very nice node"_s, false, false, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, true, true, false, false, false, false,
        "Top-level node {0x3, 0x1} A very nice node"},
    {"node name",
        DebugLayerSource::Nodes, DebugLayerFlag::NodeHighlight,
        "A very nice node"_s, false, false, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, true, false, false, false, false, false,
        "Node {0x3, 0x1} A very nice node"},
    /* Assuming node name will be always colored, testing the ColorOff /
       ColorAlways flags with it */
    {"node name, color off",
        DebugLayerSource::Nodes, DebugLayerFlag::NodeHighlight|DebugLayerFlag::ColorOff,
        "A very nice node"_s, false, false, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, true, false, false, false, false, false,
        "Node {0x3, 0x1} A very nice node"},
    /* ColorOff gets a precedence */
    {"node name, color always + color off",
        DebugLayerSource::Nodes, DebugLayerFlag::NodeHighlight|DebugLayerFlag::ColorAlways|DebugLayerFlag::ColorOff,
        "A very nice node"_s, false, false, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, true, false, false, false, false, false,
        "Node {0x3, 0x1} A very nice node"},
    {"empty node name",
        DebugLayerSource::Nodes, DebugLayerFlag::NodeHighlight,
        ""_s, false, false, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, true, false, false, false, false, false,
        "Node {0x3, 0x1}"},
    {"node flags",
        DebugLayerSource::Nodes, DebugLayerFlag::NodeHighlight,
        {}, false, false, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        NodeFlag::Clip|NodeFlag::FallthroughPointerEvents|NodeFlag::NoBlur,
        true, false, false, false, false, false,
        "Node {0x3, 0x1}\n"
        "  Flags: Clip|FallthroughPointerEvents|NoBlur"},
    {"hierarchy, root",
        DebugLayerSource::NodeHierarchy, DebugLayerFlag::NodeHighlight,
        {}, false, false, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, false, true, false, false, false, false,
        "Top-level node {0x3, 0x1}\n"
        "  Root node with 0 direct children"},
    {"hierarchy, nested",
        DebugLayerSource::NodeHierarchy, DebugLayerFlag::NodeHighlight,
        {}, false, false, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, true, false, false, false, false, false,
        "Node {0x3, 0x1}\n"
        "  Nested at level 3 with 0 direct children"},
    {"hierarchy, nested top-level",
        DebugLayerSource::NodeHierarchy, DebugLayerFlag::NodeHighlight,
        {}, false, false, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, true, true, false, false, false, false,
        "Top-level node {0x3, 0x1}\n"
        "  Nested at level 3 with 0 direct children"},
    {"hierarchy, children",
        DebugLayerSource::NodeHierarchy, DebugLayerFlag::NodeHighlight,
        {}, false, false, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, false, true, true, false, false, false,
        "Top-level node {0x3, 0x1}\n"
        "  Root node with 9 direct children"},
    {"hierarchy, nested, children, node flags",
        DebugLayerSource::NodeHierarchy, DebugLayerFlag::NodeHighlight,
        {}, false, false, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        NodeFlag::Clip|NodeFlag::FallthroughPointerEvents|NodeFlag::NoBlur,
        true, false, true, false, false, false,
        "Node {0x3, 0x1}\n"
        "  Flags: Clip|FallthroughPointerEvents|NoBlur\n"
        "  Nested at level 3 with 9 direct children"},
    {"hierarchy, hidden children",
        DebugLayerSource::NodeHierarchy, DebugLayerFlag::NodeHighlight,
        {}, false, false, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, false, true, true, true, false, false,
        "Top-level node {0x3, 0x1}\n"
        "  Root node with 9 direct children\n"
        "    of which 3 Hidden"},
    {"hierarchy, hidden and no events children",
        DebugLayerSource::NodeHierarchy, DebugLayerFlag::NodeHighlight,
        {}, false, false, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, false, true, true, true, false, true,
        "Top-level node {0x3, 0x1}\n"
        "  Root node with 9 direct children\n"
        "    of which 3 Hidden\n"
        "    of which 1 NoEvents"},
    {"hierarchy, nested node and disabled children",
        DebugLayerSource::NodeHierarchy, DebugLayerFlag::NodeHighlight,
        {}, false, false, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, true, false, true, false, true, false,
        "Node {0x3, 0x1}\n"
        "  Nested at level 3 with 9 direct children\n"
        "    of which 3 Disabled"},
    {"hierarchy, hidden, disabled and no events children",
        DebugLayerSource::NodeHierarchy, DebugLayerFlag::NodeHighlight,
        {}, false, false, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, false, true, true, true, true, true,
        "Top-level node {0x3, 0x1}\n"
        "  Root node with 9 direct children\n"
        "    of which 3 Hidden\n"
        "    of which 2 Disabled\n"
        "    of which 1 NoEvents"},
    {"data",
        DebugLayerSource::NodeData, DebugLayerFlag::NodeHighlight,
        {}, false, false, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, true, false, false, false, false, false,
        "Node {0x3, 0x1}\n"
        "  10 data from 4 layers"},
    {"data, some layer names",
        DebugLayerSource::NodeData, DebugLayerFlag::NodeHighlight,
        {}, false, true, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, true, false, false, false, false, false,
        "Node {0x3, 0x1}\n"
        "  1 data from layer {0x1, 0x1} Second\n"
        "  2 data from layer {0x4, 0x1} No.3\n"
        "  7 data from 2 other layers"},
    {"data details, some layer names",
        DebugLayerSource::NodeDataDetails, DebugLayerFlag::NodeHighlight,
        {}, false, true, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, true, false, false, false, false, false,
        "Node {0x3, 0x1}\n"
        "  1 data from layer {0x1, 0x1} Second\n"
        "  Layer No.3 (42069) data {0x0, 0x1} and a value of 1337\n"
        "  Layer No.3 (42069) data {0x1, 0x1} and a value of 1337\n"
        "  7 data from 2 other layers"},
    {"data details, all layer names",
        DebugLayerSource::NodeDataDetails, DebugLayerFlag::NodeHighlight,
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
        DebugLayerSource::NodeDataDetails, DebugLayerFlag::NodeHighlight,
        {}, true, true, true,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        {}, true, false, false, false, false, false,
        "Node {0x3, 0x1}\n"
        "  3 data from layer {0x6, 0x1} A layer\n"
        "  1 data from layer {0x5, 0x1} Second\n"
        "  Layer No.3 (42069) data {0x0, 0x1} and a value of 1337\n"
        "  Layer No.3 (42069) data {0x1, 0x1} and a value of 1337\n"
        "  4 data from layer {0x0, 0x1} The last ever"},
    {"node name, flags, nested top level, all hierarchy + data details, some layer names",
        DebugLayerSource::NodeHierarchy|DebugLayerSource::NodeDataDetails, DebugLayerFlag::NodeHighlight,
        "A very nice node"_s, false, true, false,
        {}, {}, PointerEventSource::Mouse, Pointer::MouseRight,
        NodeFlag::Clip|NodeFlag::Focusable, true, true, true, true, true, true,
        "Top-level node {0x3, 0x1} A very nice node\n"
        "  Flags: Clip|Focusable\n"
        "  Nested at level 3 with 9 direct children\n"
        "    of which 3 Hidden\n"
        "    of which 2 Disabled\n"
        "    of which 1 NoEvents\n"
        "  1 data from layer {0x1, 0x1} Second\n"
        "  Layer No.3 (42069) data {0x0, 0x1} and a value of 1337\n"
        "  Layer No.3 (42069) data {0x1, 0x1} and a value of 1337\n"
        "  7 data from 2 other layers"},
    /* The last case here is used in nodeHighlightNoCallback() to verify output
       w/o a callback and for visual color verification, it's expected to be
       the most complete, executing all coloring code paths */
};

const struct {
    const char* name;
    LayerFeatures features;
    bool event;
    bool partialUpdate;
    bool expected;
    Containers::Optional<Color4> highlightColor;
    Color4 expectedColor;
} NodeHighlightDrawData[]{
    {"no Draw feature",
        {}, false, true, false,
        {}, 0xff00ffff_rgbaf*0.5f},
    {"no Draw feature, highlight with an event",
        {}, true, true, false,
        {}, 0xff00ffff_rgbaf*0.5f},
    {"no Draw feature, highlight with an event, implicit update",
        {}, true, false, false,
        {}, 0xff00ffff_rgbaf*0.5f},
    {"",
        LayerFeature::Draw, false, true, true,
        {}, 0xff00ffff_rgbaf*0.5f},
    {"highlight with an event",
        LayerFeature::Draw, true, true, true,
        {}, 0xff00ffff_rgbaf*0.5f},
    {"highlight with an event, implicit update",
        LayerFeature::Draw, true, false, true,
        {}, 0xff00ffff_rgbaf*0.5f},
    {"custom highlight color",
        LayerFeature::Draw, false, true, true,
        0xff3366cc_rgbaf, 0xff3366cc_rgbaf},
    {"custom highlight color, highlight with an event",
        LayerFeature::Draw, true, true, true,
        0xff3366cc_rgbaf, 0xff3366cc_rgbaf},
    {"custom highlight color, highlight with an event, implicit update",
        LayerFeature::Draw, true, false, true,
        0xff3366cc_rgbaf, 0xff3366cc_rgbaf}
};

const struct {
    const char* name;
    bool removeParent;
    LayerFeatures features;
    bool expectDrawData;
} NodeHighlightNodeRemovedData[]{
    {"", false, {}, false},
    {"remove parent node", true, {}, false},
    {"layer with Draw", false, LayerFeature::Draw, true},
    {"layer with Draw, remove parent node", true, LayerFeature::Draw, true},
};

const struct {
    const char* name;
    LayerFeatures features;
    bool callback;
    bool expectDrawData;
} NodeHighlightToggleData[]{
    {"", {}, false, false},
    {"layer with Draw", LayerFeature::Draw, false, true},
    {"with callback", {}, true, false},
    {"with callback, layer with Draw", LayerFeature::Draw, true, true},
};

const struct {
    const char* name;
    LayerFeatures features;
} UpdateEmptyData[]{
    {"", {}},
    {"layer with Draw", LayerFeature::Draw}
};

const struct {
    const char* name;
    LayerStates states;
    /* Only items until the first ~UnsignedInt{} are used. ID 2 is the
       highligted node. */
    UnsignedInt dataIds[4];
    std::size_t expectedDrawOffset;
    bool expectVertexDataUpdated;
} UpdateDataOrderData[]{
    {"empty update",
        LayerState::NeedsDataUpdate,
        {~UnsignedInt{}},
        ~std::size_t{}, false},
    {"data drawn at offset 1",
        LayerState::NeedsDataUpdate,
        {3, 2, ~UnsignedInt{}},
        1, true},
    {"data drawn at offset 3",
        LayerState::NeedsDataUpdate,
        {3, 1, 0, 2},
        3, true},
    {"data drawn at offset 0",
        LayerState::NeedsDataUpdate,
        {2, ~UnsignedInt{}},
        0, true},
    {"data not drawn",
        LayerState::NeedsDataUpdate,
        {3, 1, ~UnsignedInt{}},
        ~std::size_t{}, false},
    {"node offset/size update only",
        LayerState::NeedsNodeOffsetSizeUpdate,
        {3, 2, 0, 1},
        1, true},
    {"node order update only",
        LayerState::NeedsNodeOrderUpdate,
        {3, 2, 0, 1},
        1, false},
    /* These four shouldn't cause anything to be done in update(), retaining the
       artificially set draw offset (i.e., keeping it exactly at what it was
       set to before) */
    {"node enabled update only",
        LayerState::NeedsNodeEnabledUpdate,
        {3, 2, 0, 1},
        666, false},
    {"node opacity update only",
        LayerState::NeedsNodeOpacityUpdate,
        {3, 2, 0, 1},
        666, false},
    {"shared data update only",
        LayerState::NeedsSharedDataUpdate,
        {3, 2, 0, 1},
        666, false},
    {"common data update only",
        LayerState::NeedsCommonDataUpdate,
        {3, 0, 2, 1},
        666, false},
};

DebugLayerTest::DebugLayerTest() {
    addTests({&DebugLayerTest::debugSource,
              &DebugLayerTest::debugSources,
              &DebugLayerTest::debugSourceSupersets,
              &DebugLayerTest::debugFlag,
              &DebugLayerTest::debugFlags,

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
        &DebugLayerTest::layerNameDebugIntegrationSetup,
        &DebugLayerTest::layerNameDebugIntegrationTeardown);

    addTests({&DebugLayerTest::layerNameDebugIntegrationCopyConstructPlainStruct,
              &DebugLayerTest::layerNameDebugIntegrationMoveConstructPlainStruct,
              &DebugLayerTest::layerNameInvalid,

              &DebugLayerTest::preUpdateNoUi});

    addInstancedTests({&DebugLayerTest::preUpdateNoOp},
        Containers::arraySize(PreUpdateNoOpData));

    addInstancedTests({&DebugLayerTest::preUpdateTrackNodes},
        Containers::arraySize(PreUpdateTrackNodesData));

    addInstancedTests({&DebugLayerTest::preUpdateTrackLayers},
        Containers::arraySize(PreUpdateTrackLayersData));

    addTests({&DebugLayerTest::nodeHighlightSetters});

    addInstancedTests({&DebugLayerTest::nodeHighlightNoOp},
        Containers::arraySize(NodeHighlightNoOpData));

    addInstancedTests({&DebugLayerTest::nodeHighlight},
        Containers::arraySize(NodeHighlightData));

    addTests({&DebugLayerTest::nodeHighlightNoCallback,
              &DebugLayerTest::nodeHighlightDebugIntegrationExplicit,
              &DebugLayerTest::nodeHighlightDebugIntegrationExplicitRvalue,
              &DebugLayerTest::nodeHighlightInvalid});

    addInstancedTests({&DebugLayerTest::nodeHighlightDraw},
        Containers::arraySize(NodeHighlightDrawData));

    addInstancedTests({&DebugLayerTest::nodeHighlightNodeRemoved},
        Containers::arraySize(NodeHighlightNodeRemovedData));

    addInstancedTests({&DebugLayerTest::nodeHighlightToggle},
        Containers::arraySize(NodeHighlightToggleData));

    addInstancedTests({&DebugLayerTest::updateEmpty},
        Containers::arraySize(UpdateEmptyData));

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

    /* NodeData is a superset of Nodes, so only one should be printed */
    } {
        Containers::String out;
        Debug{&out} << (DebugLayerSource::Nodes|DebugLayerSource::NodeData);
        CORRADE_COMPARE(out, "Ui::DebugLayerSource::NodeData\n");

    /* NodeData is a superset of Layers, so only one should be printed */
    } {
        Containers::String out;
        Debug{&out} << (DebugLayerSource::Layers|DebugLayerSource::NodeData);
        CORRADE_COMPARE(out, "Ui::DebugLayerSource::NodeData\n");

    /* NodeHierarchy and NodeData are both a superset of Nodes, so both should
       be printed */
    } {
        Containers::String out;
        Debug{&out} << (DebugLayerSource::NodeHierarchy|DebugLayerSource::NodeData);
        CORRADE_COMPARE(out, "Ui::DebugLayerSource::NodeHierarchy|Ui::DebugLayerSource::NodeData\n");

    /* NodeDataDetails is a superset of NodeData, so only one should be
       printed */
    } {
        Containers::String out;
        Debug{&out} << (DebugLayerSource::NodeData|DebugLayerSource::NodeDataDetails);
        CORRADE_COMPARE(out, "Ui::DebugLayerSource::NodeDataDetails\n");

    /* NodeHierarchy and NodeDataDetails are both a superset of Nodes, so both
       should be printed */
    } {
        Containers::String out;
        Debug{&out} << (DebugLayerSource::NodeHierarchy|DebugLayerSource::NodeDataDetails);
        CORRADE_COMPARE(out, "Ui::DebugLayerSource::NodeHierarchy|Ui::DebugLayerSource::NodeDataDetails\n");
    }
}

void DebugLayerTest::debugFlag() {
    Containers::String out;
    Debug{&out} << DebugLayerFlag::NodeHighlight << DebugLayerFlag(0xef);
    CORRADE_COMPARE(out, "Ui::DebugLayerFlag::NodeHighlight Ui::DebugLayerFlag(0xef)\n");
}

void DebugLayerTest::debugFlags() {
    Containers::String out;
    Debug{&out} << (DebugLayerFlag::NodeHighlight|DebugLayerFlag::ColorAlways|DebugLayerFlag(0x80)) << DebugLayerFlags{};
    CORRADE_COMPARE(out, "Ui::DebugLayerFlag::NodeHighlight|Ui::DebugLayerFlag::ColorAlways|Ui::DebugLayerFlag(0x80) Ui::DebugLayerFlags{}\n");
}

void DebugLayerTest::construct() {
    DebugLayer layer{layerHandle(137, 0xfe), DebugLayerSource::NodeData|DebugLayerSource::NodeHierarchy, DebugLayerFlag::NodeHighlight};
    CORRADE_COMPARE(layer.handle(), layerHandle(137, 0xfe));
    CORRADE_COMPARE(layer.sources(), DebugLayerSource::NodeData|DebugLayerSource::NodeHierarchy);
    CORRADE_COMPARE(layer.flags(), DebugLayerFlag::NodeHighlight);

    /* Defaults for flag-related setters are tested in setters*() */
}

void DebugLayerTest::constructInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    Containers::String out;
    Error redirectError{&out};
    DebugLayer{layerHandle(0, 1), DebugLayerSource::Layers, DebugLayerFlag::NodeHighlight};
    CORRADE_COMPARE_AS(out,
        "Ui::DebugLayer: Ui::DebugLayerSource::Nodes has to be enabled for Ui::DebugLayerFlag::NodeHighlight\n",
        TestSuite::Compare::String);
}

void DebugLayerTest::constructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<DebugLayer>{});
    CORRADE_VERIFY(!std::is_copy_assignable<DebugLayer>{});
}

void DebugLayerTest::constructMove() {
    DebugLayer a{layerHandle(137, 0xfe), DebugLayerSource::NodeData, DebugLayerFlag::NodeHighlight};

    DebugLayer b{Utility::move(a)};
    CORRADE_COMPARE(b.handle(), layerHandle(137, 0xfe));
    CORRADE_COMPARE(b.sources(), DebugLayerSource::NodeData);
    CORRADE_COMPARE(b.flags(), DebugLayerFlag::NodeHighlight);

    DebugLayer c{layerHandle(0, 2), DebugLayerSource::NodeHierarchy, {}};
    c = Utility::move(b);
    CORRADE_COMPARE(c.handle(), layerHandle(137, 0xfe));
    CORRADE_COMPARE(c.sources(), DebugLayerSource::NodeData);
    CORRADE_COMPARE(c.flags(), DebugLayerFlag::NodeHighlight);

    CORRADE_VERIFY(std::is_nothrow_move_constructible<DebugLayer>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<DebugLayer>::value);
}

void DebugLayerTest::flags() {
    DebugLayer layer{layerHandle(0, 1), {}, {}};
    CORRADE_COMPARE(layer.flags(), DebugLayerFlags{});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Verify that the set / add / clear works and that it doesn't trigger any
       state update for these. For NodeHighlight it does, which is tested in nodeHighlightToggle(). */
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

    DebugLayer layer{layerHandle(0, 1), {}, {}};

    /* Clearing a NodeHighlight flag that wasn't there before is fine even if
       DebugLayerSource::Nodes isn't present */
    layer.setFlags({});
    layer.clearFlags(DebugLayerFlag::NodeHighlight);

    Containers::String out;
    Error redirectError{&out};
    layer.setFlags(DebugLayerFlag::NodeHighlight);
    layer.addFlags(DebugLayerFlag::NodeHighlight);
    CORRADE_COMPARE_AS(out,
        "Ui::DebugLayer::setFlags(): Ui::DebugLayerSource::Nodes has to be enabled for Ui::DebugLayerFlag::NodeHighlight\n"
        "Ui::DebugLayer::setFlags(): Ui::DebugLayerSource::Nodes has to be enabled for Ui::DebugLayerFlag::NodeHighlight\n",
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

int debugIntegrationConstructed = 0;
int debugIntegrationCopied = 0;
int debugIntegrationMoved = 0;
int debugIntegrationDestructed = 0;

void DebugLayerTest::layerNameDebugIntegrationSetup() {
    debugIntegrationConstructed =
        debugIntegrationCopied =
            debugIntegrationMoved =
                debugIntegrationDestructed = 0;
}

void DebugLayerTest::layerNameDebugIntegrationTeardown() {
    debugIntegrationConstructed =
        debugIntegrationCopied =
            debugIntegrationMoved =
                debugIntegrationDestructed = 0;
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
        "Ui::DebugLayer::setLayerName(): layer not part of the same user interface\n",
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

    /* The layer has the NeedsCommonDataUpdate set always, so UI update() will
       never fully clean that up */
    ui.update();
    CORRADE_COMPARE(!layer.state(), data.expectNoState);
    CORRADE_COMPARE(!layer.usedCount(), data.expectNoData);
    CORRADE_COMPARE(layer.stateData().nodes.isEmpty(), data.expectNoNodes);
    CORRADE_COMPARE(layer.stateData().layers.isEmpty(), data.expectNoLayers);
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

void DebugLayerTest::nodeHighlightSetters() {
    /* These should work even with NodeHighlight not set, so user code can set
       all those independently of deciding what to actually use */
    DebugLayer layer{layerHandle(0, 1), {}, {}};

    /* Defaults */
    CORRADE_COMPARE(layer.nodeHighlightColor(), 0xff00ffff_rgbaf*0.5f);
    CORRADE_COMPARE(layer.nodeHighlightGesture(), Containers::pair(Pointer::MouseRight|Pointer::Eraser, ~~Modifier::Ctrl));
    CORRADE_VERIFY(!layer.hasNodeHighlightCallback());

    /* Use of this one is further tested in update() and in DebugLayerGLTest */
    layer.setNodeHighlightColor(0x3399ff66_rgbaf);
    CORRADE_COMPARE(layer.nodeHighlightColor(), 0x3399ff66_rgbaf);

    layer.setNodeHighlightGesture(Pointer::MouseMiddle|Pointer::Finger, Modifier::Alt|Modifier::Shift);
    CORRADE_COMPARE(layer.nodeHighlightGesture(), Containers::pair(Pointer::MouseMiddle|Pointer::Finger, Modifier::Alt|Modifier::Shift));

    layer.setNodeHighlightCallback([](Containers::StringView){});
    CORRADE_VERIFY(layer.hasNodeHighlightCallback());

    layer.setNodeHighlightCallback(nullptr);
    CORRADE_VERIFY(!layer.hasNodeHighlightCallback());
}

void DebugLayerTest::nodeHighlightNoOp() {
    auto&& data = NodeHighlightNoOpData[testCaseInstanceId()];
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
        layer.setNodeHighlightGesture(data.acceptedPointers, Modifier::Ctrl);
    Int callbackCalled = 0;
    layer.setNodeHighlightCallback([&callbackCalled](Containers::StringView string) {
        ++callbackCalled;
        CORRADE_VERIFY(string);
    });

    /* The update should trigger the layer to create a data attached to the
       sole node */
    ui.update();
    CORRADE_COMPARE(ui.state(), data.sources >= DebugLayerSource::Nodes ? UserInterfaceState::NeedsDataUpdate : UserInterfaceStates{});
    CORRADE_COMPARE(layer.usedCount(), data.flags >= DebugLayerFlag::NodeHighlight ? 1 : 0);

    /* The event should not be accepted, should produce no callback, but should
       fall through to the data under on the same node */
    PointerEvent event{{}, data.pointerSource, data.pointer, data.primary, 0, data.modifiers};
    CORRADE_VERIFY(!ui.pointerPressEvent({50, 50}, event));
    CORRADE_COMPARE(callbackCalled, 0);
    CORRADE_COMPARE(fallbackLayer.called, 1);

    /* If the feature is enabled and we provide a correct gesture, it should
       work. (All test case instances are expected to allow Ctrl+RMB.) */
    if(data.flags >= DebugLayerFlag::NodeHighlight) {
        PointerEvent another{{}, PointerEventSource::Mouse, Pointer::MouseRight, true, 0, Modifier::Ctrl};
        CORRADE_VERIFY(ui.pointerPressEvent({50, 50}, another));
        CORRADE_COMPARE(callbackCalled, 1);
        CORRADE_COMPARE(fallbackLayer.called, 2);
    }
}

void DebugLayerTest::nodeHighlight() {
    auto&& data = NodeHighlightData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui{{100, 100}};

    /* Parents, used optionally to verify different output for root and nested
       nodes */
    NodeHandle parent1 = ui.createNode({20, 10}, {50, 50});
    NodeHandle parent2 = ui.createNode(parent1, {0, 5}, {40, 40});
    NodeHandle parent3 = ui.createNode(parent2, {15, 0}, {25, 35});

    /* The node is at an absolute offset {40, 20} in both cases */
    NodeHandle node = data.nested ?
        ui.createNode(parent3, {5, 5}, {20, 30}) :
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
    if(!data.reverseLayerOrder) {
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
    EmptyLayer& emptyLayer3 = ui.setLayerInstance(Containers::pointer<EmptyLayer>(layers[6]));
    emptyLayer3.create(node);
    emptyLayer3.create(node);
    emptyLayer3.create(node);
    emptyLayer3.create(node);

    Int called = 0;
    Containers::String out;
    const std::ostream* const defaultOutput = Debug::output();
    layer.setNodeHighlightCallback([&out, defaultOutput, &called](Containers::StringView message) {
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

    if(data.someLayerNames) {
        layer.setLayerName(emptyLayer2, "Second");
        layer.setLayerName(removedLayer, "Removed");
        layer.setLayerName(integratedLayer, "No.3");
    }
    if(data.allLayerNames) {
        layer.setLayerName(emptyLayer1, "A layer");
        layer.setLayerName(emptyLayer3, "The last ever");
    }
    if(data.acceptedPointers)
        layer.setNodeHighlightGesture(data.acceptedPointers, data.acceptedModifiers);
    if(data.nodeName)
        layer.setNodeName(node, *data.nodeName);
    /* No node is highlighted by default */
    CORRADE_COMPARE(layer.currentHighlightedNode(), NodeHandle::Null);

    /* Update to trigger DebugLayer population */
    ui.update();

    /* Remove the layer and child node after all DebugLayer setup; add layers
       and nodes that aren't yet known by it and should thus be skipped */
    ui.removeLayer(removedLayer.handle());
    if(removedChild != NodeHandle::Null)
        ui.removeNode(removedChild);
    /* This one is in place of removedChild */
    NodeHandle unknownNode1 = ui.createNode(node, {}, {});
    /* This one is new */
    NodeHandle unknownNode2 = ui.createNode(node, {}, {});
    /* This one is in place of removedLayer */
    EmptyLayer& unknownLayer1 = ui.setLayerInstance(Containers::pointer<EmptyLayer>(ui.createLayer()));
    unknownLayer1.create(node);
    /* This one is new */
    EmptyLayer& unknownLayer2 = ui.setLayerInstance(Containers::pointer<EmptyLayer>(ui.createLayer()));
    unknownLayer2.create(node);

    /* Highlighting a Null node if nothing is highlighted does nothing but
       returns true, as that's a valid scenario */
    CORRADE_VERIFY(layer.highlightNode(NodeHandle::Null));
    CORRADE_COMPARE(layer.currentHighlightedNode(), NodeHandle::Null);
    CORRADE_COMPARE(called, 0);

    /* Highlighting a known node ID but with an invalid generation if nothing
       is highlighted does nothing and returns false; same for ID clearly out
       of bounds */
    CORRADE_VERIFY(!layer.highlightNode(nodeHandle(nodeHandleId(node), nodeHandleGeneration(node) + 1)));
    CORRADE_VERIFY(!layer.highlightNode(nodeHandle(100000, 1)));
    CORRADE_COMPARE(layer.currentHighlightedNode(), NodeHandle::Null);
    CORRADE_COMPARE(called, 0);

    /* Highlight the main node */
    CORRADE_VERIFY(layer.highlightNode(node));
    CORRADE_COMPARE(layer.currentHighlightedNode(), node);
    CORRADE_COMPARE(called, 1);
    CORRADE_COMPARE_AS(out, data.expected, TestSuite::Compare::String);

    /* Highlighting it again does exactly the same (doesn't remove the
       highlight) */
    out = {};
    CORRADE_VERIFY(layer.highlightNode(node));
    CORRADE_COMPARE(layer.currentHighlightedNode(), node);
    CORRADE_COMPARE(called, 2);
    CORRADE_COMPARE_AS(out, data.expected, TestSuite::Compare::String);

    /* Highlighting another node */
    const char* anotherExpected = data.sources >= DebugLayerSource::NodeHierarchy ?
        "Top-level node {0x4, 0x1}\n"
        "  Root node with 0 direct children" :
        "Top-level node {0x4, 0x1}";
    out = {};
    CORRADE_VERIFY(layer.highlightNode(another));
    CORRADE_COMPARE(layer.currentHighlightedNode(), another);
    CORRADE_COMPARE(called, 3);
    CORRADE_COMPARE_AS(out, anotherExpected, TestSuite::Compare::String);

    /* Highlighting Null removes the highlight and fires the callback with an
       empty string. Deliberately setting out to non-empty to verify that it
       gets emptied. */
    out = "this gonna be replaced";
    CORRADE_VERIFY(layer.highlightNode(NodeHandle::Null));
    CORRADE_COMPARE(layer.currentHighlightedNode(), NodeHandle::Null);
    CORRADE_COMPARE(called, 4);
    CORRADE_COMPARE(out, "");

    /* Highlightin invalid node with another node highlighted behaves almost
       the same, except that the function returns false. Again deliberately
       setting out to non-empty to verify that it gets emptied. */
    CORRADE_VERIFY(layer.highlightNode(another));
    CORRADE_COMPARE(layer.currentHighlightedNode(), another);
    out = "this gonna be replaced";
    CORRADE_VERIFY(!layer.highlightNode(nodeHandle(100000, 1)));
    CORRADE_COMPARE(layer.currentHighlightedNode(), NodeHandle::Null);
    CORRADE_COMPARE(called, 6); /* highlightNode() called twice in this case */
    CORRADE_COMPARE(out, "");

    /* The events implicitly call update(), meaning that the yet-unknown nodes
       and layers will become known now. Remove them to have the same output as
       above. */
    ui.removeNode(unknownNode1);
    ui.removeNode(unknownNode2);
    ui.removeLayer(unknownLayer1.handle());
    ui.removeLayer(unknownLayer2.handle());

    /* Highlight the node by an event */
    out = {};
    PointerEvent press1{{}, data.pointerSource, data.pointer, true, 0, data.acceptedPointers ? data.acceptedModifiers : Modifier::Ctrl};
    CORRADE_VERIFY(ui.pointerPressEvent({45, 35}, press1));
    CORRADE_COMPARE(layer.currentHighlightedNode(), node);
    CORRADE_COMPARE(called, 7);
    CORRADE_COMPARE_AS(out, data.expected, TestSuite::Compare::String);

    /* Highlight another node by an event */
    out = {};
    PointerEvent press2{{}, data.pointerSource, data.pointer, true, 0, data.acceptedPointers ? data.acceptedModifiers : Modifier::Ctrl};
    CORRADE_VERIFY(ui.pointerPressEvent({80, 90}, press2));
    CORRADE_COMPARE(layer.currentHighlightedNode(), another);
    CORRADE_COMPARE(called, 8);
    CORRADE_COMPARE_AS(out, anotherExpected, TestSuite::Compare::String);

    /* Clicking completely outside of anything doesn't remove the highlight (as
       there's no way to do that, apart from temporarily making the node
       focusable and focused, which would interfere with styling) */
    out = "this is gonna stay";
    PointerEvent press3{{}, data.pointerSource, data.pointer, true, 0, data.acceptedPointers ? data.acceptedModifiers : Modifier::Ctrl};
    CORRADE_VERIFY(!ui.pointerPressEvent({100, 100}, press3));
    CORRADE_COMPARE(layer.currentHighlightedNode(), another);
    CORRADE_COMPARE(called, 8);
    CORRADE_COMPARE(out, "this is gonna stay");

    /* Clicking on the node again removes the highlight, causing the callback
       to be called with an empty string. Deliberately setting out to non-empty
       to verify that it gets emptied. */
    out = "this gonna be replaced";
    PointerEvent press4{{}, data.pointerSource, data.pointer, true, 0, data.acceptedPointers ? data.acceptedModifiers : Modifier::Ctrl};
    CORRADE_VERIFY(ui.pointerPressEvent({80, 90}, press4));
    CORRADE_COMPARE(layer.currentHighlightedNode(), NodeHandle::Null);
    CORRADE_COMPARE(called, 9);
    CORRADE_COMPARE(out, "");
}

void DebugLayerTest::nodeHighlightNoCallback() {
    /* A trimmed down variant of nodeHighlight() verifying behavior without a
       callback and for visual color verification */

    AbstractUserInterface ui{{100, 100}};

    NodeHandle parent1 = ui.createNode({20, 10}, {50, 50});
    NodeHandle parent2 = ui.createNode(parent1, {0, 5}, {40, 40});
    NodeHandle parent3 = ui.createNode(parent2, {15, 0}, {25, 35});
    NodeHandle node = ui.createNode(parent3, {5, 5}, {20, 30});
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

    /* Just to match the layer handles to the nodeHighlight() case */
    /*LayerHandle removedLayer =*/ ui.createLayer();

    DebugLayer& layer = ui.setLayerInstance(Containers::pointer<DebugLayer>(ui.createLayer(), DebugLayerSource::NodeHierarchy|DebugLayerSource::NodeDataDetails, DebugLayerFlag::NodeHighlight));

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

    layer.setNodeName(node, "A very nice node");
    layer.setLayerName(emptyLayer2, "Second");
    layer.setLayerName(integratedLayer, "No.3");

    /* Highlight the node and then another unnamed one for visual color
       verification. Using events as they delegate to highlightNode() and thus
       test the whole stack for color output. */
    {
        Debug{} << "======================== visual color verification start =======================";

        layer.addFlags(DebugLayerFlag::ColorAlways);

        PointerEvent press1{{}, PointerEventSource::Mouse, Pointer::MouseRight, true, 0, Modifier::Ctrl};
        PointerEvent press2{{}, PointerEventSource::Mouse, Pointer::MouseRight, true, 0, Modifier::Ctrl};
        CORRADE_VERIFY(ui.pointerPressEvent({45, 35}, press1));
        CORRADE_COMPARE(layer.currentHighlightedNode(), node);
        CORRADE_VERIFY(ui.pointerPressEvent({80, 90}, press2));
        CORRADE_COMPARE(layer.currentHighlightedNode(), another);

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
        CORRADE_COMPARE(layer.currentHighlightedNode(), node);
        /* The output always has a newline at the end which cannot be disabled
           so strip it here to have the comparison match the nodeHighlight()
           case */
        CORRADE_COMPARE_AS(out,
            "\n",
            TestSuite::Compare::StringHasSuffix);
        CORRADE_COMPARE_AS(out.exceptSuffix("\n"),
            Containers::arrayView(NodeHighlightData).back().expected,
            TestSuite::Compare::String);
    } {
        Containers::String out;
        Debug redirectOutput{&out};
        PointerEvent press{{}, PointerEventSource::Mouse, Pointer::MouseRight, true, 0, Modifier::Ctrl};
        CORRADE_VERIFY(ui.pointerPressEvent({80, 90}, press));
        CORRADE_COMPARE(layer.currentHighlightedNode(), another);
        CORRADE_COMPARE_AS(out,
            "Top-level node {0x4, 0x1}\n"
            "  Root node with 0 direct children\n",
            TestSuite::Compare::String);

    /* Clicking the highlighted node again removes the highlight, and nothing
       gets printed */
    } {
        Containers::String out;
        Debug redirectOutput{&out};
        PointerEvent press{{}, PointerEventSource::Mouse, Pointer::MouseRight, true, 0, Modifier::Ctrl};
        CORRADE_VERIFY(ui.pointerPressEvent({80, 90}, press));
        CORRADE_COMPARE(layer.currentHighlightedNode(), NodeHandle::Null);
        CORRADE_COMPARE(out, "");
    }

    /* The same again, but with highlightNode() instead of events */
    {
        Containers::String out;
        Debug redirectOutput{&out};
        CORRADE_VERIFY(layer.highlightNode(node));
        CORRADE_COMPARE(layer.currentHighlightedNode(), node);
        /* The output always has a newline at the end which cannot be disabled
           so strip it here to have the comparison match the nodeHighlight()
           case */
        CORRADE_COMPARE_AS(out,
            "\n",
            TestSuite::Compare::StringHasSuffix);
        CORRADE_COMPARE_AS(out.exceptSuffix("\n"),
            Containers::arrayView(NodeHighlightData).back().expected,
            TestSuite::Compare::String);
    } {
        Containers::String out;
        Debug redirectOutput{&out};
        CORRADE_VERIFY(layer.highlightNode(another));
        CORRADE_COMPARE(layer.currentHighlightedNode(), another);
        CORRADE_COMPARE_AS(out,
            "Top-level node {0x4, 0x1}\n"
            "  Root node with 0 direct children\n",
            TestSuite::Compare::String);

    /* Passing Null removes the highlight, and nothing gets printed */
    } {
        Containers::String out;
        Debug redirectOutput{&out};
        CORRADE_VERIFY(layer.highlightNode(NodeHandle::Null));
        CORRADE_COMPARE(layer.currentHighlightedNode(), NodeHandle::Null);
        CORRADE_COMPARE(out, "");
    }
}

void DebugLayerTest::nodeHighlightDebugIntegrationExplicit() {
    /* Implicit integration tested in nodeHighlight() above, this verifies that
       the explicitly passed instance does the right thing as well */

    AbstractUserInterface ui{{100, 100}};

    NodeHandle node = ui.createNode({40, 20}, {20, 30});

    struct IntegratedLayer: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        struct DebugIntegration {
            explicit DebugIntegration(int value): value{value} {}

            /* Compared to noodeHighlight(), here the signature does match */
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

    DebugLayer& layer = ui.setLayerInstance(Containers::pointer<DebugLayer>(ui.createLayer(), DebugLayerSource::NodeDataDetails, DebugLayerFlag::NodeHighlight));

    IntegratedLayer::DebugIntegration integration{1337};
    layer.setLayerName(integratedLayer, "No.2", integration);

    ui.update();

    Containers::String out;
    {
        Debug redirectOutput{&out};
        CORRADE_VERIFY(layer.highlightNode(node));
    }
    CORRADE_COMPARE(layer.currentHighlightedNode(), node);
    CORRADE_COMPARE_AS(out,
        "Top-level node {0x0, 0x1}\n"
        "  Layer No.2 (42069) data {0x0, 0x1} and a value of 1337\n"
        "  Layer No.2 (42069) data {0x1, 0x1} and a value of 1337\n",
        TestSuite::Compare::String);
}

void DebugLayerTest::nodeHighlightDebugIntegrationExplicitRvalue() {
    /* Like nodeHighlightDebugIntegrationExplicit(), but passing a move-only
       instance */

    AbstractUserInterface ui{{100, 100}};

    NodeHandle node = ui.createNode({40, 40}, {20, 20});

    /* Compared to nodeHighlight() and nodeHighlightDebugIntegrationExplicit()
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

        LayerFeatures doFeatures() const override { return {}; }

        int value = 42069;
    };
    struct IntegratedLayer: IntegratedLayerBase {
        using IntegratedLayerBase::IntegratedLayerBase;

        LayerFeatures doFeatures() const override { return {}; }
    };
    IntegratedLayer& integratedLayer = ui.setLayerInstance(Containers::pointer<IntegratedLayer>(ui.createLayer()));
    integratedLayer.create(node);
    integratedLayer.create(node);

    DebugLayer& layer = ui.setLayerInstance(Containers::pointer<DebugLayer>(ui.createLayer(), DebugLayerSource::NodeDataDetails, DebugLayerFlag::NodeHighlight));

    layer.setLayerName(integratedLayer, "No.2", IntegratedLayer::DebugIntegration{1337});

    ui.update();

    Containers::String out;
    {
        Debug redirectOutput{&out};
        CORRADE_VERIFY(layer.highlightNode(node));
    }
    CORRADE_COMPARE(layer.currentHighlightedNode(), node);
    CORRADE_COMPARE_AS(out,
        "Top-level node {0x0, 0x1}\n"
        "  Layer No.2 (42069) data {0x0, 0x1} and a value of 1337\n"
        "  Layer No.2 (42069) data {0x1, 0x1} and a value of 1337\n",
        TestSuite::Compare::String);
}

void DebugLayerTest::nodeHighlightDraw() {
    auto&& data = NodeHighlightDrawData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui{{100, 100}};

    NodeHandle parent1 = ui.createNode({20, 10}, {50, 50});
    NodeHandle parent2 = ui.createNode(parent1, {0, 5}, {40, 40});
    NodeHandle parent3 = ui.createNode(parent2, {15, 0}, {25, 35});

    /* The node is at an absolute offset {40, 20} */
    NodeHandle node = ui.createNode(parent3, {5, 5}, {20, 30});

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
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), DebugLayerSource::Nodes, DebugLayerFlag::NodeHighlight, data.features));
    /* Just to silence the output */
    layer.setNodeHighlightCallback([](Containers::StringView){});

    if(data.partialUpdate) {
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);
    }

    /* Setting a highlight color marks the layer data as dirty */
    if(data.highlightColor) {
        layer.setNodeHighlightColor(*data.highlightColor);
        /* NeedsDataUpdate is set only if something is actually drawn */
        CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate|(data.expected ? LayerState::NeedsDataUpdate : LayerStates{}));

        if(data.partialUpdate) {
            ui.update();
            CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);
        }
    }

    /* Highlight the node */
    if(data.event) {
        PointerEvent event{{}, PointerEventSource::Pen, Pointer::Eraser, true, 0, Modifier::Ctrl};
        CORRADE_VERIFY(ui.pointerPressEvent({50, 30}, event));
    } else {
        CORRADE_VERIFY(layer.highlightNode(node));
    }
    CORRADE_COMPARE(layer.currentHighlightedNode(), node);
    /* NeedsDataUpdate is set only if something is actually drawn */
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate|(data.expected ? LayerState::NeedsDataUpdate : LayerStates{}));

    ui.update();
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);

    if(data.expected) {
        /* The node is drawn after all its parents */
        CORRADE_COMPARE(layer.stateData().highlightedNodeDrawOffset, 3);

        auto vertices = Containers::stridedArrayView(layer.stateData().highlightedNodeVertices);
        CORRADE_COMPARE_AS(vertices.slice(&decltype(vertices)::Type::position), Containers::arrayView<Vector2>({
            /* 2--3
               |\ | Made in a way that the triangle strip (012 123) has
               | \| counterclockwise winding.
               0--1 */
            {40.0f, 50.0f},
            {60.0f, 50.0f},
            {40.0f, 20.0f},
            {60.0f, 20.0f}
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(vertices.slice(&decltype(vertices)::Type::color), Containers::arrayView<Color4>({
            data.expectedColor,
            data.expectedColor,
            data.expectedColor,
            data.expectedColor
        }), TestSuite::Compare::Container);
    }

    /* Highlight a parent, just to verify the update goes as expected */
    if(data.event) {
        PointerEvent event{{}, PointerEventSource::Pen, Pointer::Eraser, true, 0, Modifier::Ctrl};
        CORRADE_VERIFY(ui.pointerPressEvent({30, 20}, event));
    } else {
        CORRADE_VERIFY(layer.highlightNode(parent2));
    }
    CORRADE_COMPARE(layer.currentHighlightedNode(), parent2);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate|(data.expected ? LayerState::NeedsDataUpdate : LayerStates{}));

    ui.update();
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);

    if(data.expected) {
        /* The node is drawn after all its parents */
        CORRADE_COMPARE(layer.stateData().highlightedNodeDrawOffset, 1);

        auto vertices = Containers::stridedArrayView(layer.stateData().highlightedNodeVertices);
        CORRADE_COMPARE_AS(vertices.slice(&decltype(vertices)::Type::position), Containers::arrayView<Vector2>({
            /* 2--3
               |\ | Made in a way that the triangle strip (012 123) has
               | \| counterclockwise winding.
               0--1 */
            {20.0f, 55.0f},
            {60.0f, 55.0f},
            {20.0f, 15.0f},
            {60.0f, 15.0f}
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(vertices.slice(&decltype(vertices)::Type::color), Containers::arrayView<Color4>({
            data.expectedColor,
            data.expectedColor,
            data.expectedColor,
            data.expectedColor
        }), TestSuite::Compare::Container);
    }

    /* Remove the highlight, the draw offset should be gone now */
    if(data.event) {
        PointerEvent event{{}, PointerEventSource::Pen, Pointer::Eraser, true, 0, Modifier::Ctrl};
        CORRADE_VERIFY(ui.pointerPressEvent({30, 20}, event));
    } else {
        CORRADE_VERIFY(layer.highlightNode(NodeHandle::Null));
    }
    CORRADE_COMPARE(layer.currentHighlightedNode(), NodeHandle::Null);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate|(data.expected ? LayerState::NeedsDataUpdate : LayerStates{}));

    ui.update();
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);

    if(data.expected) {
        CORRADE_COMPARE(layer.stateData().highlightedNodeDrawOffset, ~std::size_t{});

        /* The vertices stay set to whatever was there before, as they're not
           drawn anyway. Nothing to test for those. */
    }
}

void DebugLayerTest::nodeHighlightNodeRemoved() {
    auto&& data = NodeHighlightNodeRemovedData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui{{100, 100}};

    /* Node to catch the event on */
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
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), DebugLayerSource::Nodes, DebugLayerFlag::NodeHighlight, data.features));
    /* Just to silence the output */
    layer.setNodeHighlightCallback([](Containers::StringView){});

    PointerEvent press{{}, PointerEventSource::Mouse, Pointer::MouseRight, true, 0, Modifier::Ctrl};
    CORRADE_VERIFY(ui.pointerPressEvent({50, 50}, press));
    CORRADE_COMPARE(layer.currentHighlightedNode(), node);
    if(data.expectDrawData) {
        /* The draw offset gets calculated only after update() */
        ui.update();
        CORRADE_COMPARE(layer.stateData().highlightedNodeDrawOffset, 2);
    }

    /* Right after removal it still reports the node as highlighted */
    ui.removeNode(data.removeParent ? parent : node);
    CORRADE_COMPARE(layer.currentHighlightedNode(), node);
    CORRADE_COMPARE_AS(ui.state(),
        UserInterfaceState::NeedsDataUpdate,
        TestSuite::Compare::GreaterOrEqual);
    if(data.expectDrawData) {
        /* The draw offset gets updated in the doUpdate() call right before
           a draw due to NeedsDataUpdate being set, so it doesn't need to be
           cleared here as well */
        CORRADE_COMPARE(layer.stateData().highlightedNodeDrawOffset, 2);
    }
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeClean);

    /* Only after an update it gets cleaned */
    ui.update();
    CORRADE_COMPARE(layer.currentHighlightedNode(), NodeHandle::Null);
    if(data.expectDrawData)
        CORRADE_COMPARE(layer.stateData().highlightedNodeDrawOffset, ~std::size_t{});
}

void DebugLayerTest::nodeHighlightInvalid() {
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

    DebugLayer layerNoNodesNoHighlight{layerHandle(0, 1), {}, {}};
    DebugLayer layerNoUi{layerHandle(0, 1), DebugLayerSource::Nodes, DebugLayerFlag::NodeHighlight};

    DebugLayer& layer = ui.setLayerInstance(Containers::pointer<DebugLayer>(ui.createLayer(), DebugLayerSource::NodeDataDetails, DebugLayerFlag::NodeHighlight));
    layer.setLayerName(integratedLayer, "BrokenPrint");
    /* To silence the output */
    layer.setNodeHighlightCallback([](Containers::StringView) {});

    /* Make the layer aware of the node */
    ui.update();

    /* Calling functionality getters / setters is valid on a layer that doesn't
       have the feature enabled. The actual state queries and updates can't be
       called tho. */
    layerNoNodesNoHighlight.hasNodeHighlightCallback();
    layerNoNodesNoHighlight.setNodeHighlightCallback(nullptr);
    layerNoNodesNoHighlight.nodeHighlightGesture();
    layerNoNodesNoHighlight.setNodeHighlightGesture(Pointer::MouseRight, {});
    layerNoNodesNoHighlight.nodeHighlightColor();
    layerNoNodesNoHighlight.setNodeHighlightColor({});

    Containers::String out;
    Error redirectError{&out};
    layerNoNodesNoHighlight.setNodeHighlightGesture({}, Modifier::Ctrl);
    layerNoNodesNoHighlight.currentHighlightedNode();
    layerNoNodesNoHighlight.highlightNode({});
    layerNoUi.highlightNode({});
    layer.highlightNode(node);
    CORRADE_COMPARE_AS(out,
        "Ui::DebugLayer::setNodeHighlightGesture(): expected at least one pointer\n"
        "Ui::DebugLayer::currentHighlightedNode(): Ui::DebugLayerFlag::NodeHighlight not enabled\n"
        "Ui::DebugLayer::highlightNode(): Ui::DebugLayerFlag::NodeHighlight not enabled\n"
        "Ui::DebugLayer::highlightNode(): layer not part of a user interface\n"
        /* Looks a bit weird but should hopefully contain enough info to
           discover where this happened */
        "Ui::DebugLayer: expected DebugIntegration::print() to end with a newline but got Hello this is broken\n",
        TestSuite::Compare::String);
}

void DebugLayerTest::nodeHighlightToggle() {
    auto&& data = NodeHighlightToggleData[testCaseInstanceId()];
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
        layer.setNodeHighlightCallback([&called, &out](Containers::StringView message){
            out = message;
            ++called;
        });

    /* Make the DebugLayer aware of all nodes */
    ui.update();

    /* Adding the flag makes it possible to query the highlighted node, but
       there's none */
    layer.addFlags(DebugLayerFlag::NodeHighlight);
    CORRADE_COMPARE(layer.flags(), DebugLayerFlags{0x80}|DebugLayerFlag::NodeHighlight);
    CORRADE_COMPARE(layer.currentHighlightedNode(), NodeHandle::Null);
    if(data.callback)
        CORRADE_COMPARE(called, 0);
    CORRADE_COMPARE(out, "");

    {
        /* Don't care about the output if callback isn't set */
        Debug redirectOutput{nullptr};
        CORRADE_VERIFY(layer.highlightNode(node));
    }
    CORRADE_COMPARE(layer.currentHighlightedNode(), node);
    if(data.callback) {
        CORRADE_COMPARE(called, 1);
        CORRADE_COMPARE(out, "Node {0x2, 0x1}");
    }
    if(data.expectDrawData) {
        /* The draw offset gets calculated only after update() */
        CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate);
        ui.update();
        CORRADE_COMPARE(layer.stateData().highlightedNodeDrawOffset, 2);
    } else {
        CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);
    }

    /* Removing the flag calls the callback to remove the node. It isn't
       possible to query the current highlighted node anymore, but the internal
       state has it unset. */
    layer.clearFlags(DebugLayerFlag::NodeHighlight);
    CORRADE_COMPARE(layer.stateData().currentHighlightedNode, NodeHandle::Null);
    if(data.callback) {
        CORRADE_COMPARE(called, 2);
        CORRADE_COMPARE(out, "");
    }
    if(data.expectDrawData) {
        /* The state wouldn't need to include NeedsDataUpdate as the only thing
           that changes is the draw offset being cleared, affecting just the
           draw. We however need to trigger redraw somehow, so it's being
           set. */
        /** @todo clean up once NeedsDraw or some such is a thing */
        CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate);
        CORRADE_COMPARE(layer.stateData().highlightedNodeDrawOffset, ~std::size_t{});
    } else {
        CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);
    }

    /* Update to clear the NeedsDataUpdate flag */
    ui.update();
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);

    /* Adding the flag back makes it possible to query it again, it's null. The
       callback doesn't get called this time as nothing changed, no state
       update is triggered either. */
    layer.setFlags(DebugLayerFlag::NodeHighlight);
    CORRADE_COMPARE(layer.flags(), DebugLayerFlag::NodeHighlight);
    CORRADE_COMPARE(layer.currentHighlightedNode(), NodeHandle::Null);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);
    if(data.callback)
        CORRADE_COMPARE(called, 2);

    /* Removing the flag with nothing highlighted also doesn't trigger
       anything */
    layer.setFlags({});
    CORRADE_COMPARE(layer.flags(), DebugLayerFlags{});
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);
    if(data.callback)
        CORRADE_COMPARE(called, 2);
}

void DebugLayerTest::updateEmpty() {
    auto&& data = UpdateEmptyData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct Layer: DebugLayer {
        explicit Layer(LayerHandle handle, DebugLayerSources sources, DebugLayerFlags flags, LayerFeatures features): DebugLayer{handle, sources, flags}, _features{features} {}

        LayerFeatures doFeatures() const override {
            return DebugLayer::doFeatures()|_features;
        }

        private:
            LayerFeatures _features;
    } layer{layerHandle(0, 1), DebugLayerSource::Nodes, DebugLayerFlag::NodeHighlight, data.features};

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
        using DebugLayer::DebugLayer;

        DebugLayer::State& stateData() {
            return *_state;
        }

        LayerFeatures doFeatures() const override {
            return DebugLayer::doFeatures()|LayerFeature::Draw;
        }
    };
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), DebugLayerSource::Nodes, DebugLayerFlag::NodeHighlight));
    /* Just to silence the output */
    layer.setNodeHighlightCallback([](Containers::StringView){});

    /* Create nodes in a way that node with ID 3 is the one we'll highlight and
       it's associated with debug layer data ID 2, which the crafted data
       passed to update() depend on */
    ui.createNode({}, {});
    ui.createNode({}, {});
    NodeHandle removedNode = ui.createNode({}, {});
    NodeHandle node3 = ui.createNode({}, {});
    ui.removeNode(removedNode);
    ui.update();
    CORRADE_COMPARE(nodeHandleId(node3), 3);
    CORRADE_COMPARE(layer.usedCount(), 3);
    CORRADE_COMPARE(layer.node(layerDataHandle(2, 1)), node3);

    /* Highlight the node */
    CORRADE_VERIFY(layer.highlightNode(node3));

    /* Figure out the actual count of data IDs to send */
    std::size_t dataCount = 0;
    for(UnsignedInt i: data.dataIds) {
        if(i == ~UnsignedInt{})
            break;
        ++dataCount;
    }

    Vector2 nodeOffsets[4];
    Vector2 nodeSizes[4];
    Float nodeOpacities[4];
    UnsignedByte nodesEnabledData[1];
    Containers::MutableBitArrayView nodesEnabled{nodesEnabledData, 0, 4};
    nodeOffsets[3] = {20.0f, 10.0f};
    nodeSizes[3] = {40.0f, 30.0f};

    /* Set the draw offset to a silly value to detect if doUpdate() changed it
       at all. Initially it should be all 1s. */
    CORRADE_COMPARE(layer.stateData().highlightedNodeDrawOffset, ~std::size_t{});
    layer.stateData().highlightedNodeDrawOffset = 666;

    layer.update(data.states, Containers::arrayView(data.dataIds).prefix(dataCount), {}, {}, nodeOffsets, nodeSizes, nodeOpacities, nodesEnabled, {}, {}, {}, {});

    CORRADE_COMPARE(layer.stateData().highlightedNodeDrawOffset, data.expectedDrawOffset);
    if(data.expectVertexDataUpdated) {
        auto vertices = Containers::stridedArrayView(layer.stateData().highlightedNodeVertices);
        CORRADE_COMPARE_AS(vertices.slice(&decltype(vertices)::Type::position), Containers::stridedArrayView<Vector2>({
            {20.0f, 40.0f},
            {60.0f, 40.0f},
            {20.0f, 10.0f},
            {60.0f, 10.0f}
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(vertices.slice(&decltype(vertices)::Type::color), Containers::stridedArrayView<Color4>({
            0xff00ffff_rgbaf*0.5f,
            0xff00ffff_rgbaf*0.5f,
            0xff00ffff_rgbaf*0.5f,
            0xff00ffff_rgbaf*0.5f
        }), TestSuite::Compare::Container);
    }
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::DebugLayerTest)
