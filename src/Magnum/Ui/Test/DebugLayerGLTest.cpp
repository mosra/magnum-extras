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
#include <Corrade/Containers/StringIterable.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/PluginManager/PluginMetadata.h>
#include <Corrade/TestSuite/Compare/Numeric.h>
#include <Corrade/Utility/ConfigurationGroup.h>
#include <Corrade/Utility/Path.h>
#include <Magnum/Image.h>
#include <Magnum/ImageView.h>
#include <Magnum/DebugTools/CompareImage.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/OpenGLTester.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/Trade/AbstractImporter.h>

#include "Magnum/Ui/AbstractUserInterface.h"
#include "Magnum/Ui/BaseLayerGL.h" /* used in drawOrder() */
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
    bool highlightNode;
    Float nodeOffset;
    Containers::Optional<Color4> highlightColor;
} RenderData[]{
    /* Just to verify that no garbage is accidentally drawn by default */
    {"nothing", "empty.png",
        {}, {}, false, false, 0.0f, {}},
    {"node highlight", "node-highlight.png",
        DebugLayerSource::Nodes, DebugLayerFlag::NodeHighlight,
        false, true, 0.0f, {}},
    {"node highlight, partial update", "node-highlight.png",
        DebugLayerSource::Nodes, DebugLayerFlag::NodeHighlight,
        true, true, 0.0f, {}},
    {"node highlight, partial update, node offset change", "node-highlight.png",
        DebugLayerSource::Nodes, DebugLayerFlag::NodeHighlight,
        true, true, 35.0f, {}},
    {"node highlight, custom highlight color", "node-highlight-color.png",
        DebugLayerSource::Nodes, DebugLayerFlag::NodeHighlight,
        false, true, 0.0f, 0x3bd267ff_rgbaf*0.5f},
    {"node highlight, custom highlight color, partial update", "node-highlight-color.png",
        DebugLayerSource::Nodes, DebugLayerFlag::NodeHighlight,
        true, true, 0.0f, 0x3bd267ff_rgbaf*0.5f},
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
    DebugLayerGL layer{layerHandle(137, 0xfe), DebugLayerSource::NodeHierarchy, DebugLayerFlag::NodeHighlight};
    CORRADE_COMPARE(layer.handle(), layerHandle(137, 0xfe));
    CORRADE_COMPARE(layer.sources(), DebugLayerSource::NodeHierarchy);
    CORRADE_COMPARE(layer.flags(), DebugLayerFlag::NodeHighlight);
}

void DebugLayerGLTest::constructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<DebugLayerGL>{});
    CORRADE_VERIFY(!std::is_copy_assignable<DebugLayerGL>{});
}

void DebugLayerGLTest::constructMove() {
    DebugLayerGL a{layerHandle(137, 0xfe), DebugLayerSource::NodeHierarchy, DebugLayerFlag::NodeHighlight};

    DebugLayerGL b{Utility::move(a)};
    CORRADE_COMPARE(b.handle(), layerHandle(137, 0xfe));
    CORRADE_COMPARE(b.sources(), DebugLayerSource::NodeHierarchy);
    CORRADE_COMPARE(b.flags(), DebugLayerFlag::NodeHighlight);

    DebugLayerGL c{layerHandle(0, 2), DebugLayerSource::Nodes, DebugLayerFlags{}};
    c = Utility::move(b);
    CORRADE_COMPARE(c.handle(), layerHandle(137, 0xfe));
    CORRADE_COMPARE(c.sources(), DebugLayerSource::NodeHierarchy);
    CORRADE_COMPARE(c.flags(), DebugLayerFlag::NodeHighlight);

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
    layer.setNodeHighlightCallback([](Containers::StringView){});

    NodeHandle node = ui.createNode({8.0f + data.nodeOffset, 8.0f}, {112.0f, 48.0f});

    if(data.partialUpdate) {
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataUpdate);
        CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);
    }

    if(data.highlightColor) {
        layer.setNodeHighlightColor(*data.highlightColor);
        CORRADE_COMPARE_AS(ui.state(),
            UserInterfaceState::NeedsDataUpdate,
            TestSuite::Compare::GreaterOrEqual);
        CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate);
    }

    if(data.highlightNode) {
        /* Otherwise highlightNode() wouldn't know about the node yet */
        ui.update();
        CORRADE_VERIFY(layer.highlightNode(node));
        CORRADE_COMPARE(layer.currentHighlightedNode(), node);
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

    /* Since the DebugLayer currently draws just one rectangle at a time, it
       has to be combined with another layer to verify it's actually done in
       correct order respective to other draws. Thus picking the contents of
       BaseLayerGLTest::drawOrder(), highlighting each node and setting the
       color in a way that the blend between the two matches the original
       output from BaseLayerGLTest */

    AbstractUserInterface ui{DrawSize};
    ui.setRendererInstance(Containers::pointer<RendererGL>());
    BaseLayerGL::Shared layerShared{BaseLayer::Shared::Configuration{3}};
    layerShared.setStyle(BaseLayerCommonStyleUniform{}, {
        BaseLayerStyleUniform{}         /* 0, red */
            .setColor(0xff0000_rgbf),
        BaseLayerStyleUniform{}         /* 1, green */
            .setColor(0x00ff00_rgbf),
        BaseLayerStyleUniform{}         /* 2, blue */
            .setColor(0x0000ff_rgbf)
    }, {});

    BaseLayer& baseLayer = ui.setLayerInstance(Containers::pointer<BaseLayerGL>(ui.createLayer(), layerShared));

    DebugLayer& debugLayer = ui.setLayerInstance(Containers::pointer<DebugLayerGL>(ui.createLayer(), DebugLayerSource::Nodes, DebugLayerFlag::NodeHighlight));
    /* Just to silence the output */
    debugLayer.setNodeHighlightCallback([](Containers::StringView){});

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

    DataHandle topLevelBelowRedData = baseLayer.create(0, topLevelBelowRed);
    DataHandle topLevelOnTopGreenData = baseLayer.create(1, topLevelOnTopGreen);
    /*DataHandle topLevelHiddenBlueData =*/ baseLayer.create(2, topLevelHiddenBlue);
    DataHandle childBelowBlueData = baseLayer.create(2, childBelowBlue);
    DataHandle childAboveRedData = baseLayer.create(0, childAboveRed);

    /* So highlightNode() is aware of the added nodes */
    ui.update();

    struct {
        NodeHandle node;
        DataHandle baseLayerData;
        Color3 color;
        const char* expected;
    } nodes[]{
        {topLevelBelowRed, topLevelBelowRedData, 0xff0000_rgbf, "BaseLayerTestFiles/draw-order.png"},
        /* In this case the highlight is drawn over the two children (as it
           should) so it results in them having a different color as well */
        {topLevelOnTopGreen, topLevelOnTopGreenData, 0x00ff00_rgbf, "DebugLayerTestFiles/draw-order-green.png"},
        /* This isn't shown so no data need to get updated */
        {topLevelHiddenBlue, DataHandle::Null, {}, "BaseLayerTestFiles/draw-order.png"},
        /* In this case the highlight is drawn over the overlapping neighbor
           (as it should) so it results in it having a different color also */
        {childBelowBlue, childBelowBlueData, 0x0000ff_rgbf, "DebugLayerTestFiles/draw-order-blue.png"},
        {childAboveRed, childAboveRedData, 0xff0000_rgbf, "BaseLayerTestFiles/draw-order.png"},
    };
    for(const auto& node: nodes) {
        CORRADE_ITERATION(node.baseLayerData);

        /* Premultiplied alpha blending works in a way that results in
            baseColor*(1 - alpha) + highlightColor
           being written to the output. Thus we need to pick the base and
           highlight color in a way that is equal to the original if the colors
           get blended together, *and* is different from the original if the
           highlight isn't rendered at all or is rendered in a wrong place.

           By making the base color half the alpha it gets rendered wrong
           without highlight. The highlight also has half the alpha, which then
           makes the base color quarter the alpha, and so its RGB channels need
           to be 0.75, i.e. larger than the alpha. */
        debugLayer.setNodeHighlightColor({node.color*0.75f, 0.5f});
        /* Just a color multiplier that affects the color coming from style */
        if(node.baseLayerData != DataHandle::Null)
            baseLayer.setColor(node.baseLayerData, Color3{0.5f});
        CORRADE_VERIFY(debugLayer.highlightNode(node.node));

        _framebuffer.clear(GL::FramebufferClear::Color);
        ui.draw();

        /* Set the color multiplier back to not affect subsequent draws */if(node.baseLayerData != DataHandle::Null)
            baseLayer.setColor(node.baseLayerData, Color4{1.0f});

        MAGNUM_VERIFY_NO_GL_ERROR();

        if(!(_manager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
           !(_manager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
            CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

        #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
        /* While DebugLayer doesn't suffer from this, BaseLayer does. Same
           problem is with all builtin shaders, so this doesn't seem to be a
           bug in the base layer shader code. */
        if(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader)
            CORRADE_SKIP("UBOs with dynamically indexed arrays don't seem to work on SwiftShader, can't test.");
        #endif
        CORRADE_COMPARE_WITH(_framebuffer.read({{}, DrawSize}, {PixelFormat::RGBA8Unorm}),
            Utility::Path::join(UI_TEST_DIR, node.expected),
            DebugTools::CompareImageToFile{_manager});
    }
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::DebugLayerGLTest)
