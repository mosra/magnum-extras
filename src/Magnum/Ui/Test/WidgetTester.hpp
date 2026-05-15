#ifndef Magnum_Ui_Test_WidgetTester_hpp
#define Magnum_Ui_Test_WidgetTester_hpp
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

#include <Corrade/Containers/BitArrayView.h> /* for NodeOffsetSizeQueryLayer */
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/TestSuite/Tester.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/Text/AbstractFont.h>
#include <Magnum/Text/AbstractGlyphCache.h>
#include <Magnum/Text/AbstractShaper.h>
#include <Magnum/Text/Alignment.h>
#include <Magnum/Text/Script.h>

#include "Magnum/Ui/AbstractTheme.h"
#include "Magnum/Ui/AbstractTheme.hpp" /* styleTransition*() functions */
#include "Magnum/Ui/BaseLayer.h"
#include "Magnum/Ui/DataLayer.h"
#include "Magnum/Ui/EventLayer.h"
#include "Magnum/Ui/GenericLayouter.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/LayoutLayer.h"
#include "Magnum/Ui/SnapLayouter.h"
#include "Magnum/Ui/TextLayer.h"
#include "Magnum/Ui/UserInterface.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

/* Base WidgetTester class to be used by actual widget tests. Desired use case:

    - Derive the test from the `WidgetTester` class
    - Use the `ui` member and parent everything to `rootNode`
    - Add tests with the `setup()` and `teardown()` routines, which will ensure
      that everything (nodes, data, layouts, storages) is correctly cleaned up
      for the next test case
*/

struct TestUserInterface: UserInterface {
    explicit TestUserInterface(NoCreateT): UserInterface{NoCreate} {}
};

struct TestBaseLayerShared: BaseLayer::Shared {
    /* If a widget creation asserts due to a style being out of bounds, run the
       UiThemeTest to figure out what the BaseStyleCount should be updated
       to */
    explicit TestBaseLayerShared(): BaseLayer::Shared{Configuration{1, UnsignedInt(Implementation::BaseStyle::Count)}} {
        BaseLayerStyleUniform uniforms[1];
        UnsignedInt styleToUniform[]{0};
        setStyle(BaseLayerCommonStyleUniform{},
            uniforms,
            Containers::stridedArrayView(styleToUniform).broadcasted<0>(styleCount()),
            {});

        setStyleTransition<Implementation::BaseStyle,
            Implementation::styleTransitionToInactiveOut,
            Implementation::styleTransitionToInactiveOver,
            Implementation::styleTransitionToFocusedOut,
            Implementation::styleTransitionToFocusedOver,
            Implementation::styleTransitionToPressedOut,
            Implementation::styleTransitionToPressedOver,
            Implementation::styleTransitionToDisabled>();
    }

    void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
};

struct TestBaseLayer: BaseLayer {
    explicit TestBaseLayer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
};

struct TestTextLayerShared: TextLayer::Shared {
    /* If a widget creation asserts due to a style being out of bounds, run the
       UiThemeTest to figure out what the TextStyleCount should be updated
       to */
    explicit TestTextLayerShared(): TextLayer::Shared{glyphCache, Configuration{1, UnsignedInt(Implementation::TextStyle::Count)}} {
        font.openFile("", 16.0f);
        glyphCache.addFont(Implementation::IconCount + 1, &font);

        FontHandle fontHandle[]{addFont(font, 16.0f)};
        Text::Alignment alignment[]{Text::Alignment::MiddleCenter};
        TextLayerStyleUniform uniforms[1];
        UnsignedInt styleToUniform[]{0};
        setStyle(TextLayerCommonStyleUniform{}, uniforms,
            Containers::stridedArrayView(styleToUniform).broadcasted<0>(styleCount()),
            Containers::stridedArrayView(fontHandle).broadcasted<0>(styleCount()),
            Containers::stridedArrayView(alignment).broadcasted<0>(styleCount()),
            {}, {}, {}, {}, {}, {});

        setStyleTransition<Implementation::TextStyle,
            Implementation::styleTransitionToInactiveOut,
            Implementation::styleTransitionToInactiveOver,
            Implementation::styleTransitionToFocusedOut,
            Implementation::styleTransitionToFocusedOver,
            Implementation::styleTransitionToPressedOut,
            Implementation::styleTransitionToPressedOver,
            Implementation::styleTransitionToDisabled>();
    }

    void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } glyphCache{PixelFormat::R8Unorm, {32, 32}};

    struct TestFont: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return _opened; }
        Properties doOpenFile(Containers::StringView, Float) override {
            _opened = true;
            return {16.0f, 8.0f, -4.0f, 16.0f, 1};
        }
        void doClose() override { _opened = false; }

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override {
            struct Shaper: Text::AbstractShaper {
                using Text::AbstractShaper::AbstractShaper;

                bool doSetScript(Text::Script script) override {
                    _multiply = script == Text::Script::Braille ? 6 : 1;
                    return true;
                }
                UnsignedInt doShape(Containers::StringView text, UnsignedInt, UnsignedInt, Containers::ArrayView<const Text::FeatureRange>) override {
                    return text.size()*_multiply;
                }
                void doGlyphIdsInto(const Containers::StridedArrayView1D<UnsignedInt>& ids) const override {
                    for(UnsignedInt& id: ids)
                        id = 0;
                }
                void doGlyphOffsetsAdvancesInto(const Containers::StridedArrayView1D<Vector2>& offsets, const Containers::StridedArrayView1D<Vector2>& advances) const override {
                    /* Each next glyph has the advance and offset higher */
                    for(std::size_t i = 0; i != offsets.size(); ++i) {
                        offsets[i] = {};
                        advances[i] = {12.0f, 0.0f};
                    }
                }
                void doGlyphClustersInto(const Containers::StridedArrayView1D<UnsignedInt>& clusters) const override {
                    for(std::size_t i = 0; i != clusters.size(); ++i) {
                        clusters[i] = i;
                    }
                }

                private:
                    UnsignedInt _multiply = 1;
            };

            return Containers::pointer<Shaper>(*this);
        }

        bool _opened = false;
    } font;
};

struct TestTextLayer: TextLayer {
    explicit TestTextLayer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
};

struct NodeOffsetSizeQueryLayer;

struct WidgetTester: TestSuite::Tester {
    explicit WidgetTester();

    ~WidgetTester();

    void setup();
    void teardown();
    void setupNoCreate();
    void teardownNoCreate();

    /* These are used only by certain tests, and I do want the rest of the file
       to warn if anything here goes unused, so there's an anonymous namespace
       and a suppression */
    CORRADE_UNUSED static Containers::Pair<Vector2, Vector2> nodeOffsetSizeAfterLayout(UserInterface& ui, NodeOffsetSizeQueryLayer& layer, NodeHandle node);
    CORRADE_UNUSED static Vector2 nodeCenterAfterLayout(UserInterface& ui, NodeOffsetSizeQueryLayer& layer, NodeHandle node);
    CORRADE_UNUSED Containers::Pair<Vector2, Vector2> nodeOffsetSizeAfterLayout(NodeHandle node);
    CORRADE_UNUSED Vector2 nodeCenterAfterLayout(NodeHandle node);

    TestBaseLayerShared baseLayerShared;
    TestTextLayerShared textLayerShared;
    TestUserInterface ui{NoCreate};
    /* For testing that data binding APIs don't accept just the builtin data
       layer */
    DataLayer* customDataLayer;
    /* Deliberately an invalid anchor initially, to make sure nothing uses it
       before it's populated in setup(). Yeah, I know, this is abusing a
       construction path that shouldn't be used. There's (deliberately) no
       supported way to create an invalid anchor. */
    Anchor root = Widget{NoCreate};

    private:
        LayerHandle _nodeOffsetSizeQueryLayer = LayerHandle::Null;
};

WidgetTester::WidgetTester() {
    /* Adding the secondary data layer before everything else so it can
       properly affect other layers in doUpdate() callbacks */
    customDataLayer = &ui.setLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    ui.setDataLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()))
      .setBaseLayerInstance(Containers::pointer<TestBaseLayer>(ui.createLayer(), baseLayerShared))
      .setTextLayerInstance(Containers::pointer<TestTextLayer>(ui.createLayer(), textLayerShared))
      .setEventLayerInstance(Containers::pointer<EventLayer>(ui.createLayer()))
    /* If a widget creation asserts due to a style being out of bounds, run the
       UiThemeTest to figure out what the LayoutStyleCount should be updated
       to */
      .setLayoutLayerInstance(Containers::pointer<LayoutLayer>(ui.createLayer(), UnsignedInt(Implementation::LayoutStyle::Count)))
      .setSnapLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()))
      .setGenericLayouterInstance(Containers::pointer<GenericLayouter>(ui.createLayouter()))
      .setSize({1000, 1000});

    /* Base and text layer styles were set in the Test*Shared constructors
       already. The LayoutLayer style has to be set because otherwise
       ui.update(), which is called to remove unreferenced DataLayer storages,
       asserts. */
    ui.layoutLayer().setStyle({}, {}, {}, {}, {});
}

WidgetTester::~WidgetTester() {
    /* The expectation is that all test cases which create nodes use setup and
       teardown routines, so not even the rootAnchor should be present */
    teardownNoCreate();
}

void WidgetTester::setup() {
    CORRADE_INTERNAL_ASSERT(!ui.isHandleValid(root));
    CORRADE_INTERNAL_ASSERT(ui.nodeUsedCount() == 0);
    CORRADE_INTERNAL_ASSERT(customDataLayer->storageUsedCount() == 0);
    CORRADE_INTERNAL_ASSERT(customDataLayer->usedCount() == 0);
    CORRADE_INTERNAL_ASSERT(ui.dataLayer().storageUsedCount() == 0);
    CORRADE_INTERNAL_ASSERT(ui.dataLayer().usedCount() == 0);
    CORRADE_INTERNAL_ASSERT(ui.baseLayer().usedCount() == 0);
    CORRADE_INTERNAL_ASSERT(ui.textLayer().usedCount() == 0);
    CORRADE_INTERNAL_ASSERT(ui.eventLayer().usedCount() == 0);
    CORRADE_INTERNAL_ASSERT(ui.layoutLayer().usedCount() == 0);
    CORRADE_INTERNAL_ASSERT(ui.snapLayouter().usedCount() == 0);
    CORRADE_INTERNAL_ASSERT(ui.genericLayouter().usedCount() == 0);
    root = Anchor{ui, {}, ui.size()};
}

void WidgetTester::teardown() {
    ui.removeNode(root);
    ui.clean();
    /* Unreferenced storages are removed in update(), not clean() */
    /** @todo remove this once unused storages are removed in clean() also */
    if(customDataLayer->storageUsedCount() ||
       ui.dataLayer().storageUsedCount())
        ui.update();
    CORRADE_INTERNAL_ASSERT(!ui.isHandleValid(root));
    CORRADE_INTERNAL_ASSERT(ui.nodeUsedCount() == 0);
    CORRADE_INTERNAL_ASSERT(customDataLayer->storageUsedCount() == 0);
    CORRADE_INTERNAL_ASSERT(customDataLayer->usedCount() == 0);
    CORRADE_INTERNAL_ASSERT(ui.dataLayer().storageUsedCount() == 0);
    CORRADE_INTERNAL_ASSERT(ui.dataLayer().usedCount() == 0);
    CORRADE_INTERNAL_ASSERT(ui.baseLayer().usedCount() == 0);
    CORRADE_INTERNAL_ASSERT(ui.textLayer().usedCount() == 0);
    CORRADE_INTERNAL_ASSERT(ui.eventLayer().usedCount() == 0);
    CORRADE_INTERNAL_ASSERT(ui.layoutLayer().usedCount() == 0);
    CORRADE_INTERNAL_ASSERT(ui.snapLayouter().usedCount() == 0);
    CORRADE_INTERNAL_ASSERT(ui.genericLayouter().usedCount() == 0);
    /* Reset back to invalid to avoid accidents. Again yeah, I know, this is
       abusing a construction path that shouldn't be used. */
    root = Widget{NoCreate};
}

void WidgetTester::setupNoCreate() {
    CORRADE_INTERNAL_ASSERT(!ui.isHandleValid(root));
    CORRADE_INTERNAL_ASSERT(ui.nodeUsedCount() == 0);
    CORRADE_INTERNAL_ASSERT(customDataLayer->storageUsedCount() == 0);
    CORRADE_INTERNAL_ASSERT(customDataLayer->usedCount() == 0);
    CORRADE_INTERNAL_ASSERT(ui.dataLayer().storageUsedCount() == 0);
    CORRADE_INTERNAL_ASSERT(ui.dataLayer().usedCount() == 0);
    CORRADE_INTERNAL_ASSERT(ui.baseLayer().usedCount() == 0);
    CORRADE_INTERNAL_ASSERT(ui.textLayer().usedCount() == 0);
    CORRADE_INTERNAL_ASSERT(ui.eventLayer().usedCount() == 0);
    CORRADE_INTERNAL_ASSERT(ui.layoutLayer().usedCount() == 0);
    CORRADE_INTERNAL_ASSERT(ui.snapLayouter().usedCount() == 0);
    CORRADE_INTERNAL_ASSERT(ui.genericLayouter().usedCount() == 0);
    /* Set to invalid to prevent the node from being used by accident. Again
       again yeah, I know. */
    root = Widget{NoCreate};
}

void WidgetTester::teardownNoCreate() {
    CORRADE_INTERNAL_ASSERT(!ui.isHandleValid(root));
    CORRADE_INTERNAL_ASSERT(ui.nodeUsedCount() == 0);
    CORRADE_INTERNAL_ASSERT(customDataLayer->storageUsedCount() == 0);
    CORRADE_INTERNAL_ASSERT(customDataLayer->usedCount() == 0);
    CORRADE_INTERNAL_ASSERT(ui.dataLayer().storageUsedCount() == 0);
    CORRADE_INTERNAL_ASSERT(ui.dataLayer().usedCount() == 0);
    CORRADE_INTERNAL_ASSERT(ui.baseLayer().usedCount() == 0);
    CORRADE_INTERNAL_ASSERT(ui.textLayer().usedCount() == 0);
    CORRADE_INTERNAL_ASSERT(ui.eventLayer().usedCount() == 0);
    CORRADE_INTERNAL_ASSERT(ui.layoutLayer().usedCount() == 0);
    CORRADE_INTERNAL_ASSERT(ui.snapLayouter().usedCount() == 0);
    CORRADE_INTERNAL_ASSERT(ui.genericLayouter().usedCount() == 0);
}

struct NodeOffsetSizeQueryLayer: AbstractLayer {
    using AbstractLayer::AbstractLayer;

    LayerFeatures doFeatures() const override { return {}; }
    void doUpdate(LayerStates, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::StridedArrayView1D<const Float>&, Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&) override {
        if(node != NodeHandle::Null) {
            const UnsignedInt nodeId = nodeHandleId(node);
            nodeEnabled = nodesEnabled[nodeId];
            nodeOffset = nodeOffsets[nodeId];
            nodeSize = nodeSizes[nodeId];
        }
    }

    NodeHandle node = NodeHandle::Null;
    bool nodeEnabled;
    Vector2 nodeOffset, nodeSize;
};

Containers::Pair<Vector2, Vector2> WidgetTester::nodeOffsetSizeAfterLayout(UserInterface& ui, NodeOffsetSizeQueryLayer& layer, const NodeHandle node) {
    /** @todo These are all assertions because a failure of some of these
        usually means a test programmer error, and not an error in the tested
        code, thus a backtrace is useful. However not always, so a
        CORRADE_FAIL() might be nicer if there was an option to produce a
        backtrace for such failures, such as introducing a -A / --abort-on-fail
        option (and renaming -X to --exit-on-fail) */
    CORRADE_ASSERT(ui.isHandleValid(node), "Node" << node << "isn't valid", {});

    layer.nodeEnabled = false;
    layer.node = node;
    layer.setNeedsUpdate(LayerState::NeedsDataUpdate);

    ui.update();
    return {layer.nodeOffset, layer.nodeSize};
}

Containers::Pair<Vector2, Vector2> WidgetTester::nodeOffsetSizeAfterLayout(const NodeHandle node) {
    /* Add the layer if not already */
    if(_nodeOffsetSizeQueryLayer == LayerHandle::Null) {
        _nodeOffsetSizeQueryLayer = ui.createLayer();
        ui.setLayerInstance(Containers::pointer<NodeOffsetSizeQueryLayer>(_nodeOffsetSizeQueryLayer));
    }

    return nodeOffsetSizeAfterLayout(ui, ui.layer<NodeOffsetSizeQueryLayer>(_nodeOffsetSizeQueryLayer), node);
}

Vector2 WidgetTester::nodeCenterAfterLayout(UserInterface& ui, NodeOffsetSizeQueryLayer& layer, const NodeHandle node) {
    Containers::Pair<Vector2, Vector2> offsetSize = nodeOffsetSizeAfterLayout(ui, layer, node);
    /* Firing this here and not in nodeOffsetSizeAfterLayout() as the
       assumption is that this function gets used for aiming events while the
       other for checking node properties, along with verifying that a
       particular node may have size set to zero, which then causes it to be
       culled and thus not marked as enabled. */
    CORRADE_ASSERT(layer.nodeEnabled,
        "Node" << node << "that's queried for center isn't marked as enabled, maybe it's culled or outside the UI size?", {});
    CORRADE_ASSERT(offsetSize.second().product(),
        "Node" << node << "that's queried for center has a size of" << offsetSize.second() << Debug::nospace << ", maybe layout properties are missing?", {});
    return offsetSize.first() + offsetSize.second()*0.5f;
}

Vector2 WidgetTester::nodeCenterAfterLayout(const NodeHandle node) {
    /* Add the layer if not already */
    if(_nodeOffsetSizeQueryLayer == LayerHandle::Null) {
        _nodeOffsetSizeQueryLayer = ui.createLayer();
        ui.setLayerInstance(Containers::pointer<NodeOffsetSizeQueryLayer>(_nodeOffsetSizeQueryLayer));
    }

    return nodeCenterAfterLayout(ui, ui.layer<NodeOffsetSizeQueryLayer>(_nodeOffsetSizeQueryLayer), node);
}

}}}}

#endif
