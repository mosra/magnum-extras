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

#include <sstream> /** @todo remove once Debug is stream-free */
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/StringIterable.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/PluginManager/PluginMetadata.h>
#include <Corrade/TestSuite/Compare/Numeric.h>
#include <Corrade/Utility/ConfigurationGroup.h>
#include <Corrade/Utility/DebugStl.h> /** @todo remove once Debug is stream-free */
#include <Corrade/Utility/Path.h>
#include <Magnum/Image.h>
#include <Magnum/ImageView.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/DebugTools/CompareImage.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/GL/OpenGLTester.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Trade/AbstractImporter.h>

#include "Magnum/Whee/AbstractUserInterface.h"
#include "Magnum/Whee/BaseLayerGL.h"
#include "Magnum/Whee/Event.h"
#include "Magnum/Whee/Handle.h"

#include "configure.h"

namespace Magnum { namespace Whee { namespace Test { namespace {

struct BaseLayerGLTest: GL::OpenGLTester {
    explicit BaseLayerGLTest();

    void sharedConstruct();
    /* NoCreate tested in BaseLayerGL_Test to verify it works without a GL
       context */
    void sharedConstructCopy();
    void sharedConstructMove();

    void sharedSetStyleInvalidSize();

    void construct();
    void constructCopy();
    void constructMove();

    void drawNoStyleSet();

    void renderSetup();
    void renderTeardown();
    void render();
    void renderCustomColor();
    void renderCustomOutlineWidth();
    void renderChangeStyle();

    void drawSetup();
    void drawTeardown();
    void drawOrder();

    void eventStyleTransition();

    private:
        PluginManager::Manager<Trade::AbstractImporter> _manager;
        GL::Texture2D _color{NoCreate};
        GL::Framebuffer _framebuffer{NoCreate};
};

using namespace Math::Literals;

const struct {
    const char* name;
    const char* filename;
    BaseLayerStyleCommon styleCommon;
    BaseLayerStyleItem styleItem;
} RenderData[]{
    {"default", "default.png",
        BaseLayerStyleCommon{},
        BaseLayerStyleItem{}},
    {"gradient", "gradient.png",
        BaseLayerStyleCommon{},
        BaseLayerStyleItem{}
            .setColor(0xeeddaa_rgbf, 0x774422_rgbf)},
    {"rounded corners, all same, default smoothness", "rounded-corners-same-hard.png",
        BaseLayerStyleCommon{},
        BaseLayerStyleItem{}
            .setCornerRadius(24.0f)},
    {"rounded corners, all same", "rounded-corners-same.png",
        BaseLayerStyleCommon{}
            .setSmoothness(1.0f),
        BaseLayerStyleItem{}
            .setCornerRadius(24.0f)},
    {"rounded corners, different", "rounded-corners-different.png",
        BaseLayerStyleCommon{}
            .setSmoothness(1.0f),
        BaseLayerStyleItem{}
            /* Top left, bottom left, top right, bottom right; one radius is
               more than half of the height, one is zero */
            .setCornerRadius({4.0f, 44.0f, 24.0f, 0.0f})},
    {"outline, default color", "default.png",
        BaseLayerStyleCommon{},
        BaseLayerStyleItem{}
            .setOutlineWidth(8.0f)},
    {"outline, all sides same", "outline-same.png",
        BaseLayerStyleCommon{},
        BaseLayerStyleItem{}
            .setOutlineColor(0x7f7f7f_rgbf)
            .setOutlineWidth(8.0f)},
    {"outline, different", "outline-different.png",
        BaseLayerStyleCommon{},
        BaseLayerStyleItem{}
            .setOutlineColor(0x7f7f7f_rgbf)
            /* Left, top, right, bottom; one side is going over the center,
               one is zero */
            .setOutlineWidth({8.0f, 4.0f, 0.0f, 32.0f})},
    {"outline, rounded corners inside", "outline-rounded-corners-inside.png",
        BaseLayerStyleCommon{}
            .setSmoothness(1.0f),
        BaseLayerStyleItem{}
            .setOutlineColor(0x7f7f7f_rgbf)
            .setInnerOutlineCornerRadius(8.0f)
            .setOutlineWidth(8.0f)},
    {"outline, rounded corners, different", "outline-rounded-corners-both-different.png",
        BaseLayerStyleCommon{}
            .setSmoothness(1.0f),
        BaseLayerStyleItem{}
            .setOutlineColor(0x7f7f7f_rgbf)
            /* Top left, bottom left, top right, bottom right */
            .setCornerRadius({36.0f, 12.0f, 4.0f, 0.0f})
            /* Important is that the right side with zero outline width has at
               least one rounded corner, to verify it doesn't get clipped away
               due to the zero outline */
            .setInnerOutlineCornerRadius({18.0f, 6.0f, 0.0f, 18.0f})
            /* Left, top, right, bottom  */
            .setOutlineWidth({18.0f, 8.0f, 0.0f, 4.0f})},
    {"outline, rounded corners, different inner and outer smoothness", "outline-rounded-corners-different-smoothness.png",
        BaseLayerStyleCommon{}
            .setSmoothness(1.0f, 8.0f),
        BaseLayerStyleItem{}
            .setOutlineColor(0x7f7f7f_rgbf)
            .setCornerRadius(16.0f)
            .setInnerOutlineCornerRadius(8.0f)
            .setOutlineWidth(8.0f)},
    {"outline with gradient", "outline-gradient.png",
        BaseLayerStyleCommon{},
        BaseLayerStyleItem{}
            .setColor(0xffffff_rgbf, 0x333333_rgbf)
            /* It gets multiplied with the gradient */
            .setOutlineColor(0x3333ff_rgbf)
            .setOutlineWidth(8.0f)},
};

const struct {
    const char* name;
    bool setLater;
    bool partialUpdate;
} RenderCustomColorOutlineWidthData[]{
    {"", false, false},
    {"set later", true, false},
    {"set later, partial update", true, true},
};

const struct {
    const char* name;
    bool partialUpdate;
} RenderChangeStyleData[]{
    {"", false},
    {"partial update", true},
};

const struct {
    const char* name;
    bool dataInNodeOrder;
} DrawOrderData[]{
    {"data created in node order", true},
    {"data created randomly", false}
};

BaseLayerGLTest::BaseLayerGLTest() {
    addTests({&BaseLayerGLTest::sharedConstruct,
              &BaseLayerGLTest::sharedConstructCopy,
              &BaseLayerGLTest::sharedConstructMove,

              &BaseLayerGLTest::sharedSetStyleInvalidSize,

              &BaseLayerGLTest::construct,
              &BaseLayerGLTest::constructCopy,
              &BaseLayerGLTest::constructMove,

              &BaseLayerGLTest::drawNoStyleSet});

    addInstancedTests({&BaseLayerGLTest::render},
        Containers::arraySize(RenderData),
        &BaseLayerGLTest::renderSetup,
        &BaseLayerGLTest::renderTeardown);

    addInstancedTests({&BaseLayerGLTest::renderCustomColor,
                       &BaseLayerGLTest::renderCustomOutlineWidth},
        Containers::arraySize(RenderCustomColorOutlineWidthData),
        &BaseLayerGLTest::renderSetup,
        &BaseLayerGLTest::renderTeardown);

    addInstancedTests({&BaseLayerGLTest::renderChangeStyle},
        Containers::arraySize(RenderChangeStyleData),
        &BaseLayerGLTest::renderSetup,
        &BaseLayerGLTest::renderTeardown);

    addInstancedTests({&BaseLayerGLTest::drawOrder},
        Containers::arraySize(DrawOrderData),
        &BaseLayerGLTest::drawSetup,
        &BaseLayerGLTest::drawTeardown);

    addTests({&BaseLayerGLTest::eventStyleTransition},
        &BaseLayerGLTest::renderSetup,
        &BaseLayerGLTest::renderTeardown);

    /* Prefer the StbImageImporter so we can keep files small but always import
       them as four-channel */
    if(PluginManager::PluginMetadata* metadata = _manager.metadata("StbImageImporter")) {
        metadata->configuration().setValue("forceChannelCount", 4);
        _manager.setPreferredPlugins("PngImporter", {"StbImageImporter"});
    }
}

void BaseLayerGLTest::sharedConstruct() {
    BaseLayerGL::Shared shared{3};
    CORRADE_COMPARE(shared.styleCount(), 3);
}

void BaseLayerGLTest::sharedConstructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<BaseLayerGL::Shared>{});
    CORRADE_VERIFY(!std::is_copy_assignable<BaseLayerGL::Shared>{});
}

void BaseLayerGLTest::sharedConstructMove() {
    BaseLayerGL::Shared a{3};

    BaseLayerGL::Shared b{Utility::move(a)};
    CORRADE_COMPARE(b.styleCount(), 3);

    BaseLayerGL::Shared c{5};
    c = Utility::move(b);
    CORRADE_COMPARE(c.styleCount(), 3);

    CORRADE_VERIFY(std::is_nothrow_move_constructible<BaseLayerGL::Shared>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<BaseLayerGL::Shared>::value);
}

void BaseLayerGLTest::sharedSetStyleInvalidSize() {
    CORRADE_SKIP_IF_NO_ASSERT();

    BaseLayerGL::Shared shared{3};

    struct {
        BaseLayerStyleCommon common;
        BaseLayerStyleItem styles[2];
    } style;

    std::ostringstream out;
    Error redirectError{&out};
    shared.setStyle(style);
    CORRADE_COMPARE(out.str(), "Whee::BaseLayerGL::Shared::setStyle(): expected style data of 304 bytes, got 208\n");
}

void BaseLayerGLTest::construct() {
    BaseLayerGL::Shared shared{3};

    BaseLayerGL layer{layerHandle(137, 0xfe), shared};
    /* There isn't anything to query on the BaseLayerGL itself */
    CORRADE_COMPARE(layer.handle(), layerHandle(137, 0xfe));
}

void BaseLayerGLTest::constructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<BaseLayerGL>{});
    CORRADE_VERIFY(!std::is_copy_assignable<BaseLayerGL>{});
}

void BaseLayerGLTest::constructMove() {
    BaseLayerGL::Shared shared{3};
    BaseLayerGL::Shared shared2{5};

    BaseLayerGL a{layerHandle(137, 0xfe), shared};

    BaseLayerGL b{Utility::move(a)};
    /* There isn't anything to query on the BaseLayerGL itself */
    CORRADE_COMPARE(b.handle(), layerHandle(137, 0xfe));

    BaseLayerGL c{layerHandle(0, 2), shared2};
    c = Utility::move(b);
    CORRADE_COMPARE(c.handle(), layerHandle(137, 0xfe));

    CORRADE_VERIFY(std::is_nothrow_move_constructible<BaseLayerGL>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<BaseLayerGL>::value);
}

void BaseLayerGLTest::drawNoStyleSet() {
    CORRADE_SKIP_IF_NO_ASSERT();

    BaseLayerGL::Shared shared{3};
    BaseLayerGL layer{layerHandle(0, 1), shared};

    std::ostringstream out;
    Error redirectError{&out};
    layer.draw({}, 0, 0, {}, {}, 0, 0, {}, {}, {}, {});
    CORRADE_COMPARE(out.str(), "Whee::BaseLayerGL::draw(): no style data was set\n");
}

constexpr Vector2i RenderSize{128, 64};

void BaseLayerGLTest::renderSetup() {
    _color = GL::Texture2D{};
    _color.setStorage(1, GL::TextureFormat::RGBA8, RenderSize);
    _framebuffer = GL::Framebuffer{{{}, RenderSize}};
    _framebuffer
        .attachTexture(GL::Framebuffer::ColorAttachment{0}, _color, 0)
        .clear(GL::FramebufferClear::Color)
        .bind();

    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::One, GL::Renderer::BlendFunction::OneMinusSourceAlpha);
}

void BaseLayerGLTest::renderTeardown() {
    _framebuffer = GL::Framebuffer{NoCreate};
    _color = GL::Texture2D{NoCreate};

    GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::Blending);
}

void BaseLayerGLTest::render() {
    auto&& data = RenderData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui{RenderSize};

    struct {
        BaseLayerStyleCommon common;
        /* To verify it's not always picking the first style */
        BaseLayerStyleItem itemAnother;
        BaseLayerStyleItem item;
    } style;
    style.common = data.styleCommon;
    style.item = data.styleItem;

    BaseLayerGL::Shared layerShared{2};
    layerShared.setStyle(style);

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<BaseLayerGL>(layer, layerShared));

    NodeHandle node = ui.createNode({8.0f, 8.0f}, {112.0f, 48.0f});
    ui.layer<BaseLayerGL>(layer).create(1, node);

    ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();

    if(!(_manager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_manager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
    /* Same problem is with all builtin shaders, so this doesn't seem to be a
       bug in the base layer shader code */
    if(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader)
        CORRADE_SKIP("UBOs with dynamically indexed arrays don't seem to work on SwiftShader, can't test.");
    #endif
    CORRADE_COMPARE_WITH(_framebuffer.read({{}, RenderSize}, {PixelFormat::RGBA8Unorm}),
        Utility::Path::join({WHEE_TEST_DIR, "BaseLayerTestFiles", data.filename}),
        DebugTools::CompareImageToFile{_manager});
}

void BaseLayerGLTest::renderCustomColor() {
    auto&& data = RenderCustomColorOutlineWidthData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Basically the same as the "gradient" case in render(), except that the
       color is additionally taken from the per-vertex data as well */

    AbstractUserInterface ui{RenderSize};

    struct {
        BaseLayerStyleCommon common;
        BaseLayerStyleItem item;
    } style;
    style.item.setColor(0xeeddaa_rgbf/0x336699_rgbf, 0x774422_rgbf/0x336699_rgbf);

    BaseLayerGL::Shared layerShared{1};
    layerShared.setStyle(style);

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<BaseLayerGL>(layer, layerShared));

    NodeHandle node = ui.createNode({8.0f, 8.0f}, {112.0f, 48.0f});
    DataHandle nodeData = data.setLater ?
        ui.layer<BaseLayerGL>(layer).create(0, node) :
        ui.layer<BaseLayerGL>(layer).create(0, 0x336699_rgbf, node);

    if(data.partialUpdate) {
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    if(data.setLater) {
        ui.layer<BaseLayerGL>(layer).setColor(nodeData, 0x336699_rgbf);
        CORRADE_COMPARE_AS(ui.state(),
            UserInterfaceState::NeedsDataUpdate,
            TestSuite::Compare::GreaterOrEqual);
    }

    ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();

    if(!(_manager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_manager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
    /* Same problem is with all builtin shaders, so this doesn't seem to be a
       bug in the base layer shader code */
    if(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader)
        CORRADE_SKIP("UBOs with dynamically indexed arrays don't seem to work on SwiftShader, can't test.");
    #endif
    CORRADE_COMPARE_WITH(_framebuffer.read({{}, RenderSize}, {PixelFormat::RGBA8Unorm}),
        Utility::Path::join(WHEE_TEST_DIR, "BaseLayerTestFiles/gradient.png"),
        DebugTools::CompareImageToFile{_manager});
}

void BaseLayerGLTest::renderCustomOutlineWidth() {
    auto&& data = RenderCustomColorOutlineWidthData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Basically the same as the "outline, all sides same" case in render(),
       except that the width is additionally taken from the per-vertex data as
       well */

    AbstractUserInterface ui{RenderSize};

    struct {
        BaseLayerStyleCommon common;
        BaseLayerStyleItem item;
    } style;
    style.item
        .setOutlineColor(0x7f7f7f_rgbf)
        .setOutlineWidth({16.0f, 2.0f, 4.0f, 0.0f});

    BaseLayerGL::Shared layerShared{1};
    layerShared.setStyle(style);

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<BaseLayerGL>(layer, layerShared));

    NodeHandle node = ui.createNode({8.0f, 8.0f}, {112.0f, 48.0f});
    DataHandle nodeData;
    if(data.setLater)
        nodeData = ui.layer<BaseLayerGL>(layer).create(0, node);
    else
        nodeData = ui.layer<BaseLayerGL>(layer).create(0, 0xffffff_rgbf, {-8.0f, 6.0f, 4.0f, 8.0f}, node);

    if(data.partialUpdate) {
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    if(data.setLater) {
        ui.layer<BaseLayerGL>(layer).setOutlineWidth(nodeData, {-8.0f, 6.0f, 4.0f, 8.0f});
        CORRADE_COMPARE_AS(ui.state(),
            UserInterfaceState::NeedsDataUpdate,
            TestSuite::Compare::GreaterOrEqual);
    }

    ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();

    if(!(_manager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_manager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
    /* Same problem is with all builtin shaders, so this doesn't seem to be a
       bug in the base layer shader code */
    if(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader)
        CORRADE_SKIP("UBOs with dynamically indexed arrays don't seem to work on SwiftShader, can't test.");
    #endif
    CORRADE_COMPARE_WITH(_framebuffer.read({{}, RenderSize}, {PixelFormat::RGBA8Unorm}),
        Utility::Path::join(WHEE_TEST_DIR, "BaseLayerTestFiles/outline-same.png"),
        DebugTools::CompareImageToFile{_manager});
}

void BaseLayerGLTest::renderChangeStyle() {
    auto&& data = RenderChangeStyleData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Basically the same as the "gradient" case in render(), except that the
       style ID is changed to it only later. */

    AbstractUserInterface ui{RenderSize};

    struct {
        BaseLayerStyleCommon common;
        BaseLayerStyleItem anotherItem;
        BaseLayerStyleItem item;
    } style;
    style.item.setColor(0xeeddaa_rgbf, 0x774422_rgbf);

    BaseLayerGL::Shared layerShared{2};
    layerShared.setStyle(style);

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<BaseLayerGL>(layer, layerShared));

    NodeHandle node = ui.createNode({8.0f, 8.0f}, {112.0f, 48.0f});
    DataHandle nodeData = ui.layer<BaseLayerGL>(layer).create(0, node);

    if(data.partialUpdate) {
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    ui.layer<BaseLayerGL>(layer).setStyle(nodeData, 1);
    CORRADE_COMPARE_AS(ui.state(),
        UserInterfaceState::NeedsDataUpdate,
        TestSuite::Compare::GreaterOrEqual);

    ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();

    if(!(_manager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_manager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
    /* Same problem is with all builtin shaders, so this doesn't seem to be a
       bug in the base layer shader code */
    if(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader)
        CORRADE_SKIP("UBOs with dynamically indexed arrays don't seem to work on SwiftShader, can't test.");
    #endif
    CORRADE_COMPARE_WITH(_framebuffer.read({{}, RenderSize}, {PixelFormat::RGBA8Unorm}),
        Utility::Path::join(WHEE_TEST_DIR, "BaseLayerTestFiles/gradient.png"),
        DebugTools::CompareImageToFile{_manager});
}

constexpr Vector2i DrawSize{64, 64};

void BaseLayerGLTest::drawSetup() {
    _color = GL::Texture2D{};
    _color.setStorage(1, GL::TextureFormat::RGBA8, DrawSize);
    _framebuffer = GL::Framebuffer{{{}, DrawSize}};
    _framebuffer
        .attachTexture(GL::Framebuffer::ColorAttachment{0}, _color, 0)
        .clear(GL::FramebufferClear::Color)
        .bind();

    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::One, GL::Renderer::BlendFunction::OneMinusSourceAlpha);
}

void BaseLayerGLTest::drawTeardown() {
    _framebuffer = GL::Framebuffer{NoCreate};
    _color = GL::Texture2D{NoCreate};

    GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::Blending);
}

void BaseLayerGLTest::drawOrder() {
    auto&& data = DrawOrderData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui{DrawSize};

    struct {
        BaseLayerStyleCommon common;
        BaseLayerStyleItem red;     /* 0 */
        BaseLayerStyleItem green;   /* 1 */
        BaseLayerStyleItem blue;    /* 2 */
    } style;
    style.red.setColor(0xff0000_rgbf);
    style.green.setColor(0x00ff00_rgbf);
    style.blue.setColor(0x0000ff_rgbf);

    BaseLayerGL::Shared layerShared{3};
    layerShared.setStyle(style);

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<BaseLayerGL>(layer, layerShared));

    NodeHandle topLevelOnTopGreen = ui.createNode({8.0f, 8.0f}, {32.0f, 32.0f});

    NodeHandle topLevelBelowRed = ui.createNode({24.0f, 24.0f}, {32.0f, 32.0f});
    ui.setNodeOrder(topLevelBelowRed, topLevelOnTopGreen);

    NodeHandle topLevelHiddenBlue = ui.createNode({24.0f, 8.0f}, {32.0f, 32.0f}, NodeFlag::Hidden);

    NodeHandle childBelowBlue = ui.createNode(topLevelOnTopGreen, {12.0f, 4.0f}, {16.0f, 16.0f});
    NodeHandle childAboveRed = ui.createNode(childBelowBlue, {-8.0f, 8.0f}, {16.0f, 16.0f});

    if(data.dataInNodeOrder) {
        ui.layer<BaseLayerGL>(layer).create(0, topLevelBelowRed);
        ui.layer<BaseLayerGL>(layer).create(1, topLevelOnTopGreen);
        ui.layer<BaseLayerGL>(layer).create(2, topLevelHiddenBlue);
        ui.layer<BaseLayerGL>(layer).create(2, childBelowBlue);
        ui.layer<BaseLayerGL>(layer).create(0, childAboveRed);
    } else {
        ui.layer<BaseLayerGL>(layer).create(1, topLevelOnTopGreen);
        ui.layer<BaseLayerGL>(layer).create(2, topLevelHiddenBlue);
        ui.layer<BaseLayerGL>(layer).create(0, topLevelBelowRed);
        ui.layer<BaseLayerGL>(layer).create(0, childAboveRed);
        ui.layer<BaseLayerGL>(layer).create(2, childBelowBlue);
    }

    ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();

    if(!(_manager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_manager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
    /* Same problem is with all builtin shaders, so this doesn't seem to be a
       bug in the base layer shader code */
    if(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader)
        CORRADE_SKIP("UBOs with dynamically indexed arrays don't seem to work on SwiftShader, can't test.");
    #endif
    CORRADE_COMPARE_WITH(_framebuffer.read({{}, DrawSize}, {PixelFormat::RGBA8Unorm}),
        Utility::Path::join(WHEE_TEST_DIR, "BaseLayerTestFiles/draw-order.png"),
        DebugTools::CompareImageToFile{_manager});
}

void BaseLayerGLTest::eventStyleTransition() {
    /* Switches between the "default" and "gradient" cases from render() after
       a press event. Everything else is tested in AbstractVisualLayerTest
       already. */

    AbstractUserInterface ui{RenderSize};

    struct {
        BaseLayerStyleCommon common;
        BaseLayerStyleItem defaults;
        BaseLayerStyleItem gradient;
    } style;
    style.gradient.setColor(0xeeddaa_rgbf, 0x774422_rgbf);

    BaseLayerGL::Shared layerShared{2};
    layerShared
        .setStyle(style)
        .setStyleTransition(
            [](UnsignedInt style) -> UnsignedInt {
                if(style == 0) return 1;
                CORRADE_INTERNAL_ASSERT_UNREACHABLE();
            },
            [](UnsignedInt) -> UnsignedInt {
                CORRADE_INTERNAL_ASSERT_UNREACHABLE();
            });

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<BaseLayerGL>(layer, layerShared));

    NodeHandle node = ui.createNode({8.0f, 8.0f}, {112.0f, 48.0f});
    ui.layer<BaseLayerGL>(layer).create(0, node);

    ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();
    Image2D before = _framebuffer.read({{}, RenderSize}, {PixelFormat::RGBA8Unorm});

    PointerEvent event{Pointer::MouseLeft};
    CORRADE_VERIFY(ui.pointerPressEvent({64.0f, 24.0f}, event));
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataUpdate);

    ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();
    Image2D after = _framebuffer.read({{}, RenderSize}, {PixelFormat::RGBA8Unorm});

    if(!(_manager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_manager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
    /* Same problem is with all builtin shaders, so this doesn't seem to be a
       bug in the base layer shader code */
    if(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader)
        CORRADE_SKIP("UBOs with dynamically indexed arrays don't seem to work on SwiftShader, can't test.");
    #endif
    CORRADE_COMPARE_WITH(before,
        Utility::Path::join(WHEE_TEST_DIR, "BaseLayerTestFiles/default.png"),
        DebugTools::CompareImageToFile{_manager});
    CORRADE_COMPARE_WITH(after,
        Utility::Path::join(WHEE_TEST_DIR, "BaseLayerTestFiles/gradient.png"),
        DebugTools::CompareImageToFile{_manager});
}

}}}}

CORRADE_TEST_MAIN(Magnum::Whee::Test::BaseLayerGLTest)
