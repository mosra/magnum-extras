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
#include <Magnum/Text/AbstractFont.h>
#include <Magnum/Trade/AbstractImporter.h>

#include "Magnum/Whee/Anchor.h"
#include "Magnum/Whee/Button.h"
#include "Magnum/Whee/Event.h"
#include "Magnum/Whee/Handle.h"
#include "Magnum/Whee/NodeFlags.h"
#include "Magnum/Whee/RendererGL.h"
#include "Magnum/Whee/Style.h"
#include "Magnum/Whee/UserInterfaceGL.h"

#include "configure.h"

namespace Magnum { namespace Whee { namespace Test { namespace {

struct StyleGLTest: GL::OpenGLTester {
    explicit StyleGLTest();

    void render();

    private:
        PluginManager::Manager<Text::AbstractFont> _fontManager;
        PluginManager::Manager<Trade::AbstractImporter> _importerManager;
};

const struct {
    const char* name;
    const char* filename;
    Containers::Pointer<AbstractStyle> style;
    Int styleCount;
    bool hoverActive, disabled;
    NodeHandle(*create)(UserInterface& ui, Int style, Int counter);
} RenderData[]{
    {"m.css dark, button text + icon, stateless", "mcss-dark-button-text-icon.png",
        Containers::pointer<McssDarkStyle>(), 8, true, true,
        [](UserInterface& ui, Int style, Int counter) {
            /** @todo differently wide icons to test alignment */
            return button({ui, {96, 36}}, ButtonStyle(style), counter % 2 ? Icon::No : Icon::Yes, counter % 2 ? "Bye" : "Hello!");
        }},
    {"m.css dark, button text + icon, stateful", "mcss-dark-button-text-icon.png",
        Containers::pointer<McssDarkStyle>(), 8, true, true,
        [](UserInterface& ui, Int style, Int counter) {
            return Button{{ui, {96, 36}}, ButtonStyle(style), counter % 2 ? Icon::No : Icon::Yes, counter % 2 ? "Bye" : "Hello!"}.release();
        }},
    {"m.css dark, button text + icon, stateful setters", "mcss-dark-button-text-icon.png",
        Containers::pointer<McssDarkStyle>(), 8, true, true,
        [](UserInterface& ui, Int style, Int counter) {
            Button button{{ui, {96, 36}}, ButtonStyle(style), Icon::No, "Hey"};
            button.setIcon(counter % 2 ? Icon::No : Icon::Yes);
            button.setText(counter % 2 ? "Bye" : "Hello!");
            return button.release();
        }},
    {"m.css dark, button text + icon, stateful setters from empty", "mcss-dark-button-text-icon.png",
        Containers::pointer<McssDarkStyle>(), 8, true, true,
        [](UserInterface& ui, Int style, Int counter) {
            Button button{{ui, {96, 36}}, ButtonStyle(style), Icon::None, ""};
            button.setIcon(counter % 2 ? Icon::No : Icon::Yes);
            button.setText(counter % 2 ? "Bye" : "Hello!");
            return button.release();
        }},
    {"m.css dark, button text + icon, stateful setters from empty, different order", "mcss-dark-button-text-icon.png",
        Containers::pointer<McssDarkStyle>(), 8, true, true,
        [](UserInterface& ui, Int style, Int counter) {
            Button button{{ui, {96, 36}}, ButtonStyle(style), Icon::None, ""};
            button.setText(counter % 2 ? "Bye" : "Hello!");
            button.setIcon(counter % 2 ? Icon::No : Icon::Yes);
            return button.release();
        }},
    {"m.css dark, button text, stateless", "mcss-dark-button-text.png",
        Containers::pointer<McssDarkStyle>(), 8, true, true,
        [](UserInterface& ui, Int style, Int counter) {
            return button({ui, {64, 36}}, ButtonStyle(style), counter % 2 ? "Bye" : "Hello!");
        }},
    {"m.css dark, button text, stateful", "mcss-dark-button-text.png",
        Containers::pointer<McssDarkStyle>(), 8, true, true,
        [](UserInterface& ui, Int style, Int counter) {
            return Button{{ui, {64, 36}}, ButtonStyle(style), counter % 2 ? "Bye" : "Hello!"}.release();
        }},
    {"m.css dark, button text, stateful setters", "mcss-dark-button-text.png",
        Containers::pointer<McssDarkStyle>(), 8, true, true,
        [](UserInterface& ui, Int style, Int counter) {
            Button button{{ui, {64, 36}}, ButtonStyle(style), "Hey"};
            button.setText(counter % 2 ? "Bye" : "Hello!");
            return button.release();
        }},
    {"m.css dark, button text, stateful setters from empty", "mcss-dark-button-text.png",
        Containers::pointer<McssDarkStyle>(), 8, true, true,
        [](UserInterface& ui, Int style, Int counter) {
            Button button{{ui, {64, 36}}, ButtonStyle(style), ""};
            button.setText(counter % 2 ? "Bye" : "Hello!");
            return button.release();
        }},
    {"m.css dark, button icon, stateless", "mcss-dark-button-icon.png",
        Containers::pointer<McssDarkStyle>(), 8, true, true,
        [](UserInterface& ui, Int style, Int counter) {
            /** @todo differently wide icons to test alignment */
            return button({ui, {48, 36}}, ButtonStyle(style), counter % 2 ? Icon::Yes : Icon::No);
        }},
    {"m.css dark, button icon, stateful", "mcss-dark-button-icon.png",
        Containers::pointer<McssDarkStyle>(), 8, true, true,
        [](UserInterface& ui, Int style, Int counter) {
            return Button{{ui, {48, 36}}, ButtonStyle(style), counter % 2 ? Icon::Yes : Icon::No}.release();
        }},
    {"m.css dark, button icon, stateful setters", "mcss-dark-button-icon.png",
        Containers::pointer<McssDarkStyle>(), 8, true, true,
        [](UserInterface& ui, Int style, Int counter) {
            Button button{{ui, {48, 36}}, ButtonStyle(style), Icon::Yes};
            button.setIcon(counter % 2 ? Icon::Yes : Icon::No);
            return button.release();
        }},
    {"m.css dark, button icon, stateful setters from empty", "mcss-dark-button-icon.png",
        Containers::pointer<McssDarkStyle>(), 8, true, true,
        [](UserInterface& ui, Int style, Int counter) {
            Button button{{ui, {48, 36}}, ButtonStyle(style), Icon::None};
            button.setIcon(counter % 2 ? Icon::Yes : Icon::No);
            return button.release();
        }},
};

StyleGLTest::StyleGLTest() {
    addInstancedTests({&StyleGLTest::render},
        Containers::arraySize(RenderData));

    /* Prefer the StbImageImporter so we can keep files small but always import
       them as four-channel */
    if(PluginManager::PluginMetadata* metadata = _importerManager.metadata("StbImageImporter")) {
        metadata->configuration().setValue("forceChannelCount", 4);
        _importerManager.setPreferredPlugins("PngImporter", {"StbImageImporter"});
    }
}

void StyleGLTest::render() {
    auto&& data = RenderData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    if(!(_importerManager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_importerManager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");
    if(!(_fontManager.load("StbTrueTypeFont") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("StbTrueTypeFont plugin not found.");

    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
    /* Same problem is with all builtin shaders, so this doesn't seem to be a
       bug in the base layer shader code. Compared to other cases doing this as
       early as possible as the test is quite heavy and waiting several minutes
       on the CI for all the SKIPs makes no sense. */
    if(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader)
        CORRADE_SKIP("UBOs with dynamically indexed arrays don't seem to work on SwiftShader, can't test.");
    #endif

    /* As an UI instance has a global concept of a currently hovered / pressed
       / ... node, we have to have several instances in order to render
       multiple widgets in a hovered state at once. Yes, it's nasty, in a
       way. Initially the UI is set to a trivial size, the actual size is set
       later once we know how much the widgets span. */
    const std::size_t stateCount = 1 + (data.hoverActive ? 3 : 0) + (data.disabled ? 1 : 0);
    Containers::Array<UserInterfaceGL> uis{DirectInit, data.styleCount*stateCount, Vector2i{1024, 1024}, *data.style, &_importerManager, &_fontManager};

    const Vector2 padding{8.0f};

    Int counter = 0;
    Vector2 size;
    for(Int style = 0; style != data.styleCount; ++style) {
        {
            UserInterfaceGL& ui = uis[style*stateCount + 0];
            NodeHandle node = data.create(ui, style, counter++);
            size = ui.nodeSize(node);
            ui.setNodeOffset(node, padding + (padding + size)*Vector2{Vector2i{0, style}});
        }

        if(data.hoverActive) {
            {
                UserInterfaceGL& ui = uis[style*stateCount + 1];
                NodeHandle hover = data.create(ui, style, counter++);
                ui.setNodeOffset(hover, padding + (padding + size)*Vector2{Vector2i{1, style}});

                PointerMoveEvent move{{}, {}};
                CORRADE_VERIFY(ui.pointerMoveEvent(ui.nodeOffset(hover) + ui.nodeSize(hover)*0.5f, move));
            } {
                UserInterfaceGL& ui = uis[style*stateCount + 2];
                NodeHandle activeHover = data.create(ui, style, counter++);
                ui.setNodeOffset(activeHover, padding + (padding + size)*Vector2{Vector2i{2, style}});

                PointerMoveEvent move{{}, {}};
                CORRADE_VERIFY(ui.pointerMoveEvent(ui.nodeOffset(activeHover) + ui.nodeSize(activeHover)*0.5f, move));

                PointerEvent press{Pointer::MouseLeft};
                CORRADE_VERIFY(ui.pointerPressEvent(ui.nodeOffset(activeHover) + ui.nodeSize(activeHover)*0.5f, press));
            } {
                UserInterfaceGL& ui = uis[style*stateCount + 3];
                NodeHandle active = data.create(ui, style, counter++);
                ui.setNodeOffset(active, padding + (padding + size)*Vector2{Vector2i{3, style}});

                PointerEvent press{Pointer::MouseLeft};
                CORRADE_VERIFY(ui.pointerPressEvent(ui.nodeOffset(active) + ui.nodeSize(active)*0.5f, press));
            }
        }

        if(data.disabled) {
            UserInterfaceGL& ui = uis[style*stateCount + (data.hoverActive ? 4 : 1)];
            NodeHandle disabled = data.create(ui, style, counter++);
            ui.setNodeOffset(disabled, padding + (padding + size)*Vector2{Vector2i{data.hoverActive ? 4 : 1, style}});

            ui.addNodeFlags(disabled, NodeFlag::Disabled);
        }
    }

    /* Calculate the actual UI size. To avoid strange issues with events not
       being handled etc., it should always be smaller than the original set
       above. */
    const Vector2i uiSize = Vector2i{padding} + Vector2i{size + padding}*Vector2i{1 + (data.hoverActive ? 3 : 0) + (data.disabled ? 1 : 0), data.styleCount};
    CORRADE_COMPARE_AS(Vector2{uiSize},
        uis[0].size(),
        TestSuite::Compare::LessOrEqual);

    /* Set up a framebuffer to render to based on the area used */
    GL::Texture2D color;
    GL::Framebuffer framebuffer{{{}, uiSize}};
    color.setStorage(1, GL::TextureFormat::RGBA8, uiSize);
    framebuffer
        .attachTexture(GL::Framebuffer::ColorAttachment{0}, color, 0)
        .clear(GL::FramebufferClear::Color)
        .bind();
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::One, GL::Renderer::BlendFunction::OneMinusSourceAlpha);

    /* Resize the UIs to what got actually used and draw everything */
    for(UserInterfaceGL& ui: uis)
        ui
            .setSize(uiSize)
            .draw();

    MAGNUM_VERIFY_NO_GL_ERROR();

    CORRADE_COMPARE_WITH(framebuffer.read({{}, uiSize}, {PixelFormat::RGBA8Unorm}),
        Utility::Path::join({WHEE_TEST_DIR, "StyleTestFiles", data.filename}),
        DebugTools::CompareImageToFile{_importerManager});

    /* Verify that a change back to inactive out state results in the same
       visuals as originally */
    if(data.hoverActive) {
        for(Int style = 0; style != data.styleCount; ++style) {
            UserInterfaceGL& ui = uis[style*stateCount];
            /* We don't record the node handles, but each UI should have just
               one so this artificial one should be correct */
            NodeHandle node = nodeHandle(0, 1);
            CORRADE_VERIFY(ui.isHandleValid(node));

            /* Move over, making the node hovered, i.e. looking the same as in
               the second column */
            PointerMoveEvent moveOver{{}, {}};
            CORRADE_VERIFY(ui.pointerMoveEvent(ui.nodeOffset(node) + ui.nodeSize(node)*0.5f, moveOver));

            /* Move out again */
            PointerMoveEvent moveOut{{}, {}};
            CORRADE_VERIFY(!ui.pointerMoveEvent(ui.nodeOffset(node) + ui.nodeSize(node)*1.5f, moveOver));
        }

        framebuffer.clear(GL::FramebufferClear::Color);
        for(UserInterfaceGL& ui: uis)
            ui.draw();

        MAGNUM_VERIFY_NO_GL_ERROR();

        CORRADE_COMPARE_WITH(framebuffer.read({{}, uiSize}, {PixelFormat::RGBA8Unorm}),
            Utility::Path::join({WHEE_TEST_DIR, "StyleTestFiles", data.filename}),
            DebugTools::CompareImageToFile{_importerManager});
    }
}

}}}}

CORRADE_TEST_MAIN(Magnum::Whee::Test::StyleGLTest)
