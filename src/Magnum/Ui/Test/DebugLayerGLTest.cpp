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

#include <Corrade/Containers/Function.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/StridedBitArrayView.h>
#include <Corrade/Containers/StringIterable.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/PluginManager/PluginMetadata.h>
#include <Corrade/TestSuite/Compare/Container.h>
#include <Corrade/TestSuite/Compare/Numeric.h>
#include <Corrade/Utility/ConfigurationGroup.h>
#include <Corrade/Utility/Path.h>
#include <Magnum/Image.h>
#include <Magnum/ImageView.h>
#include <Magnum/DebugTools/CompareImage.h>
#include <Magnum/Math/Color.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/OpenGLTester.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/Trade/AbstractImporter.h>

#include "Magnum/Ui/AbstractUserInterface.h"
#include "Magnum/Ui/DebugLayerGL.h"
#include "Magnum/Ui/Event.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/NodeFlags.h"
#include "Magnum/Ui/RendererGL.h"

#include "configure.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct DebugLayerGLTest: GL::OpenGLTester {
    public:
        explicit DebugLayerGLTest();

        void construct();
        void constructCopy();
        void constructMove();

        void renderSetup();
        void renderTeardown();
        void render();

        void drawSetup();
        void drawTeardown();
        void drawOrder();

    private:
        PluginManager::Manager<Trade::AbstractImporter> _manager;
        GL::Texture2D _color{NoCreate};
        GL::Framebuffer _framebuffer{NoCreate};
};

using namespace Math::Literals;

const struct {
    const char* name;
    const char* filename;
    DebugLayerSources sources;
    DebugLayerFlags flags;
    bool partialUpdate;
    bool inspectNode, highlightNode;
    Float nodeOffset;
    Containers::Optional<Color4> inspectColor;
    Containers::Optional<Containers::Pair<Color3ub, Float>> highlightColor;
} RenderData[]{
    /* Just to verify that no garbage is accidentally drawn by default */
    {"nothing", "empty.png",
        {}, {}, false, false, false, 0.0f, {}, {}},
    {"node inspect enabled but nothing inspected", "empty.png",
        DebugLayerSource::Nodes, DebugLayerFlag::NodeInspect,
        false, false, false, 0.0f, {}, {}},
    {"node inspect", "node-inspect.png",
        DebugLayerSource::Nodes, DebugLayerFlag::NodeInspect,
        false, true, false, 0.0f, {}, {}},
    {"node inspect, partial update", "node-inspect.png",
        DebugLayerSource::Nodes, DebugLayerFlag::NodeInspect,
        true, true, false, 0.0f, {}, {}},
    {"node inspect, partial update, node offset change", "node-inspect.png",
        DebugLayerSource::Nodes, DebugLayerFlag::NodeInspect,
        true, true, false, 35.0f, {}, {}},
    {"node inspect, custom inspect color", "node-inspect-highlight-color.png",
        DebugLayerSource::Nodes, DebugLayerFlag::NodeInspect,
        false, true, false, 0.0f, 0x3bd267ff_rgbaf*0.5f, {}},
    {"node inspect, custom inspect color, partial update", "node-inspect-highlight-color.png",
        DebugLayerSource::Nodes, DebugLayerFlag::NodeInspect,
        true, true, false, 0.0f, 0x3bd267ff_rgbaf*0.5f, {}},
    {"node highlight enabled but nothing highlighted", "empty.png",
        DebugLayerSource::Nodes, {},
        false, false, false, 0.0f, {}, {}},
    {"node highlight", "node-highlight.png",
        DebugLayerSource::Nodes, {},
        false, false, true, 0.0f, {}, {}},
    {"node highlight, custom highlight color", "node-inspect-highlight-color.png",
        DebugLayerSource::Nodes, {},
        false, false, true, 0.0f, {}, {{0x3bd267_rgb, 0.5f}}},
    {"node highlight, custom highlight color, partial update", "node-inspect-highlight-color.png",
        DebugLayerSource::Nodes, {},
        true, false, true, 0.0f, {}, {{0x3bd267_rgb, 0.5f}}},
    /* The inspect color wins */
    {"node inspect and highlight", "node-inspect.png",
        DebugLayerSource::Nodes, DebugLayerFlag::NodeInspect,
        false, true, true, 0.0f, {}, {}},
};

const struct {
    const char* name;
    bool sequentialNodeOrder;
} DrawOrderData[]{
    {"sequential node order", true},
    {"nodes ordered randomly", false}
};

DebugLayerGLTest::DebugLayerGLTest() {
    addTests({&DebugLayerGLTest::construct,
              &DebugLayerGLTest::constructCopy,
              &DebugLayerGLTest::constructMove});

    addInstancedTests({&DebugLayerGLTest::render},
        Containers::arraySize(RenderData),
        &DebugLayerGLTest::renderSetup,
        &DebugLayerGLTest::renderTeardown);

    addInstancedTests({&DebugLayerGLTest::drawOrder},
        Containers::arraySize(DrawOrderData),
        &DebugLayerGLTest::drawSetup,
        &DebugLayerGLTest::drawTeardown);

    /* Prefer the StbImageImporter so we can keep files small but always import
       them as four-channel */
    if(PluginManager::PluginMetadata* metadata = _manager.metadata("StbImageImporter")) {
        metadata->configuration().setValue("forceChannelCount", 4);
        _manager.setPreferredPlugins("PngImporter", {"StbImageImporter"});
    }
}

void DebugLayerGLTest::construct() {
    DebugLayerGL layer{layerHandle(137, 0xfe), DebugLayerSource::NodeHierarchy, DebugLayerFlag::NodeInspect};
    CORRADE_COMPARE(layer.handle(), layerHandle(137, 0xfe));
    CORRADE_COMPARE(layer.sources(), DebugLayerSource::NodeHierarchy);
    CORRADE_COMPARE(layer.flags(), DebugLayerFlag::NodeInspect);
}

void DebugLayerGLTest::constructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<DebugLayerGL>{});
    CORRADE_VERIFY(!std::is_copy_assignable<DebugLayerGL>{});
}

void DebugLayerGLTest::constructMove() {
    DebugLayerGL a{layerHandle(137, 0xfe), DebugLayerSource::NodeHierarchy, DebugLayerFlag::NodeInspect};

    DebugLayerGL b{Utility::move(a)};
    CORRADE_COMPARE(b.handle(), layerHandle(137, 0xfe));
    CORRADE_COMPARE(b.sources(), DebugLayerSource::NodeHierarchy);
    CORRADE_COMPARE(b.flags(), DebugLayerFlag::NodeInspect);

    DebugLayerGL c{layerHandle(0, 2), DebugLayerSource::Nodes, DebugLayerFlags{}};
    c = Utility::move(b);
    CORRADE_COMPARE(c.handle(), layerHandle(137, 0xfe));
    CORRADE_COMPARE(c.sources(), DebugLayerSource::NodeHierarchy);
    CORRADE_COMPARE(c.flags(), DebugLayerFlag::NodeInspect);

    CORRADE_VERIFY(std::is_nothrow_move_constructible<DebugLayerGL>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<DebugLayerGL>::value);
}

constexpr Vector2i RenderSize{128, 64};

void DebugLayerGLTest::renderSetup() {
    _color = GL::Texture2D{};
    _color.setStorage(1, GL::TextureFormat::RGBA8, RenderSize);
    _framebuffer = GL::Framebuffer{{{}, RenderSize}};
    _framebuffer
        .attachTexture(GL::Framebuffer::ColorAttachment{0}, _color, 0)
        .clear(GL::FramebufferClear::Color)
        .bind();

    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::One, GL::Renderer::BlendFunction::OneMinusSourceAlpha);
    /* The RendererGL should enable these on its own if needed */
    GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::disable(GL::Renderer::Feature::Blending);
}

void DebugLayerGLTest::renderTeardown() {
    _framebuffer = GL::Framebuffer{NoCreate};
    _color = GL::Texture2D{NoCreate};

    GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::disable(GL::Renderer::Feature::Blending);
}

void DebugLayerGLTest::render() {
    auto&& data = RenderData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui{RenderSize};
    ui.setRendererInstance(Containers::pointer<RendererGL>());

    DebugLayer& layer = ui.setLayerInstance(Containers::pointer<DebugLayerGL>(ui.createLayer(), data.sources, data.flags));
    /* Just to silence the output */
    layer.setNodeInspectCallback([](Containers::StringView){});

    /* The node is third out of four, so it should use the color map color at
       index 2. The other nodes are present but not highlighted, should result
       in nothing else being drawn. */
    ui.createNode({}, {100, 100});
    ui.createNode({50, 0}, {50, 100});
    NodeHandle node = ui.createNode({8.0f + data.nodeOffset, 8.0f}, {112.0f, 48.0f});
    ui.createNode({0, 50}, {100, 50});

    if(data.partialUpdate) {
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataUpdate);
        CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);
    }

    if(data.inspectColor) {
        layer.setNodeInspectColor(*data.inspectColor);
        CORRADE_COMPARE_AS(ui.state(),
            UserInterfaceState::NeedsDataUpdate,
            TestSuite::Compare::GreaterOrEqual);
        CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate);
    }

    Color3ub colorMap[]{
        0xff0000_rgb,
        0x00ff00_rgb,
        data.highlightColor ? data.highlightColor->first() : 0xffffff_rgb,
        0x0000ff_rgb,
    };
    if(data.highlightColor && data.highlightNode) {
        layer.setNodeHighlightColorMap(colorMap, data.highlightColor->second());
        CORRADE_COMPARE_AS(ui.state(),
            UserInterfaceState::NeedsDataUpdate,
            TestSuite::Compare::GreaterOrEqual);
        CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate);
    }

    /* Otherwise inspectNode() / highlightNode() wouldn't know about the node
       yet */
    if(data.inspectNode || data.highlightNode)
        ui.update();

    if(data.inspectNode) {
        CORRADE_VERIFY(layer.inspectNode(node));
        CORRADE_COMPARE(layer.currentInspectedNode(), node);
    }

    if(data.highlightNode) {
        CORRADE_VERIFY(layer.highlightNode(node));
        CORRADE_COMPARE_AS(layer.currentHighlightedNodes(), Containers::stridedArrayView({
            false,
            false,
            true,
            false,
        }).sliceBit(0), TestSuite::Compare::Container);
    }

    /* Updating node offset/size later should still get correctly propagated */
    if(data.nodeOffset) {
        if(data.partialUpdate)
            ui.update();

        ui.setNodeOffset(node, {8.0f, 8.0f});
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsLayoutUpdate);
    }

    ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();

    if(!(_manager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_manager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    CORRADE_COMPARE_WITH(_framebuffer.read({{}, RenderSize}, {PixelFormat::RGBA8Unorm}),
        Utility::Path::join({UI_TEST_DIR, "DebugLayerTestFiles", data.filename}),
        /* SwiftShader has minor off-by-one differences */
        (DebugTools::CompareImageToFile{_manager, 0.75f, 0.5f}));
}

constexpr Vector2i DrawSize{64, 64};

void DebugLayerGLTest::drawSetup() {
    _color = GL::Texture2D{};
    _color.setStorage(1, GL::TextureFormat::RGBA8, DrawSize);
    _framebuffer = GL::Framebuffer{{{}, DrawSize}};
    _framebuffer
        .attachTexture(GL::Framebuffer::ColorAttachment{0}, _color, 0)
        .clear(GL::FramebufferClear::Color)
        .bind();

    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::One, GL::Renderer::BlendFunction::OneMinusSourceAlpha);
    /* The RendererGL should enable these on its own if needed */
    GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::disable(GL::Renderer::Feature::Blending);
}

void DebugLayerGLTest::drawTeardown() {
    _framebuffer = GL::Framebuffer{NoCreate};
    _color = GL::Texture2D{NoCreate};

    GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::disable(GL::Renderer::Feature::Blending);
}

void DebugLayerGLTest::drawOrder() {
    auto&& data = DrawOrderData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui{DrawSize};
    ui.setRendererInstance(Containers::pointer<RendererGL>());

    DebugLayer& debugLayer = ui.setLayerInstance(Containers::pointer<DebugLayerGL>(ui.createLayer(), DebugLayerSource::Nodes, DebugLayerFlags{}));

    /* For drawing in order that doesn't match the node ID, create and then
       remove the nodes in random order to make the next ones created with
       non-sequential IDs */
    if(!data.sequentialNodeOrder) {
        NodeHandle node0 = ui.createNode({}, {});
        NodeHandle node1 = ui.createNode({}, {});
        NodeHandle node2 = ui.createNode({}, {});
        NodeHandle node3 = ui.createNode({}, {});
        NodeHandle node4 = ui.createNode({}, {});

        ui.removeNode(node3);
        ui.removeNode(node1);
        ui.removeNode(node2);
        ui.removeNode(node0);
        ui.removeNode(node4);
    }

    NodeHandle topLevelOnTopGreen = ui.createNode({8.0f, 8.0f}, {32.0f, 32.0f});

    NodeHandle topLevelBelowRed = ui.createNode({24.0f, 24.0f}, {32.0f, 32.0f});
    ui.setNodeOrder(topLevelBelowRed, topLevelOnTopGreen);

    NodeHandle topLevelHiddenBlue = ui.createNode({24.0f, 8.0f}, {32.0f, 32.0f}, NodeFlag::Hidden);

    NodeHandle childBelowBlue = ui.createNode(topLevelOnTopGreen, {12.0f, 4.0f}, {16.0f, 16.0f});
    NodeHandle childAboveRed = ui.createNode(childBelowBlue, {-8.0f, 8.0f}, {16.0f, 16.0f});

    /* Color maps. For sequential node order it's the colors from the above
       node handle names. */
    Color3ub colorMapSequentialNodeOrder[]{
        0x00ff00_rgb, /* 0, topLevelOnTopGreen */
        0xff0000_rgb, /* 1, topLevelBelowRed */
        0xff00ff_rgb, /* 2, topLevelHiddenBlue, unused */
        0x0000ff_rgb, /* 3, childBelowBlue */
        0xff0000_rgb, /* 4, childAboveRed */
    };
    /* In the other case they're shuffled, matching the numeric order in which
       the nodes were removed. */
    Color3ub colorMap[]{
        0x0000ff_rgb, /* 0, childBelowBlue */
        0xff0000_rgb, /* 1, topLevelBelowRed */
        0xff00ff_rgb, /* 2, topLevelHiddenBlue, unused */
        0x00ff00_rgb, /* 3, topLevelOnTopGreen */
        0xff0000_rgb, /* 4, childAboveRed */
    };
    debugLayer.setNodeHighlightColorMap(data.sequentialNodeOrder ?
        colorMapSequentialNodeOrder : colorMap, 1.0f);

    /* So highlightNode() is aware of the added nodes */
    ui.update();
    debugLayer.highlightNode(topLevelOnTopGreen);
    debugLayer.highlightNode(topLevelBelowRed);
    debugLayer.highlightNode(topLevelHiddenBlue);
    debugLayer.highlightNode(childBelowBlue);
    debugLayer.highlightNode(childAboveRed);

    ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();

    if(!(_manager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
        !(_manager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    CORRADE_COMPARE_WITH(_framebuffer.read({{}, DrawSize}, {PixelFormat::RGBA8Unorm}),
        Utility::Path::join(UI_TEST_DIR, "BaseLayerTestFiles/draw-order.png"),
        DebugTools::CompareImageToFile{_manager});
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::DebugLayerGLTest)
