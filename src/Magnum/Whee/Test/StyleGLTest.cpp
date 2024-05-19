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
#include <Corrade/Utility/Format.h>
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
#include "Magnum/Whee/BaseLayerGL.h"
#include "Magnum/Whee/TextLayerGL.h"
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
        Containers::Array<UserInterfaceGL> _styleUis;
        Containers::Array<UserInterfaceGL> _uis;
};

using namespace Math::Literals;

const struct {
    const char* name;
    const char* filePrefix;
    UnsignedInt index;
    Containers::Pointer<AbstractStyle> style;
} StyleData[]{
    {"m.css dark", "mcss-dark-", 0, Containers::pointer<McssDarkStyle>()},
    {"m.css dark SubdividedQuads", "mcss-dark-", 1, []{
        Containers::Pointer<McssDarkStyle> style{InPlaceInit};
        style->setBaseLayerFlags(BaseLayerSharedFlag::SubdividedQuads, {});
        return style;
    }()},
};

const struct {
    const char* name;
    const char* filename;
    Int styleCount;
    bool hoveredPressed, disabled;
    NodeHandle(*create)(UserInterface& ui, Int style, Int counter);
} RenderData[]{
    {"button text + icon, stateless", "button-text-icon.png",
        8, true, true,
        [](UserInterface& ui, Int style, Int counter) {
            /** @todo differently wide icons to test alignment */
            return button({ui, {96, 36}}, ButtonStyle(style), counter % 2 ? Icon::No : Icon::Yes, counter % 2 ? "Bye" : "Hello!").node();
        }},
    {"button text + icon", "button-text-icon.png",
        8, true, true,
        [](UserInterface& ui, Int style, Int counter) {
            return Button{{ui, {96, 36}}, ButtonStyle(style), counter % 2 ? Icon::No : Icon::Yes, counter % 2 ? "Bye" : "Hello!"}.release();
        }},
    {"button text + icon, setters", "button-text-icon.png",
        8, true, true,
        [](UserInterface& ui, Int style, Int counter) {
            Button button{{ui, {96, 36}}, ButtonStyle(style), Icon::No, "Hey"};
            button.setIcon(counter % 2 ? Icon::No : Icon::Yes);
            button.setText(counter % 2 ? "Bye" : "Hello!");
            return button.release();
        }},
    {"button text + icon, setters on empty", "button-text-icon.png",
        8, true, true,
        [](UserInterface& ui, Int style, Int counter) {
            Button button{{ui, {96, 36}}, ButtonStyle(style), Icon::None, ""};
            button.setIcon(counter % 2 ? Icon::No : Icon::Yes);
            button.setText(counter % 2 ? "Bye" : "Hello!");
            return button.release();
        }},
    {"button text + icon, setters on empty, different order", "button-text-icon.png",
        8, true, true,
        [](UserInterface& ui, Int style, Int counter) {
            Button button{{ui, {96, 36}}, ButtonStyle(style), Icon::None, ""};
            button.setText(counter % 2 ? "Bye" : "Hello!");
            button.setIcon(counter % 2 ? Icon::No : Icon::Yes);
            return button.release();
        }},
    {"button text + icon, setStyle()", "button-text-icon.png",
        8, true, true,
        [](UserInterface& ui, Int style, Int counter) {
            Button button{{ui, {96, 36}}, ButtonStyle(style == 0 ? 1 : 0), counter % 2 ? Icon::No : Icon::Yes, counter % 2 ? "Bye" : "Hello!"};
            button.setStyle(ButtonStyle(style));
            return button.release();
        }},
    {"button text + icon, setStyle() on empty, setters", "button-text-icon.png",
        8, true, true,
        [](UserInterface& ui, Int style, Int counter) {
            Button button{{ui, {96, 36}}, ButtonStyle(style == 0 ? 1 : 0), Icon::None, ""};
            button.setStyle(ButtonStyle(style));
            button.setIcon(counter % 2 ? Icon::No : Icon::Yes);
            button.setText(counter % 2 ? "Bye" : "Hello!");
            return button.release();
        }},
    {"button text, stateless", "button-text.png",
        8, true, true,
        [](UserInterface& ui, Int style, Int counter) {
            return button({ui, {64, 36}}, ButtonStyle(style), counter % 2 ? "Bye" : "Hello!").node();
        }},
    {"button text", "button-text.png",
        8, true, true,
        [](UserInterface& ui, Int style, Int counter) {
            return Button{{ui, {64, 36}}, ButtonStyle(style), counter % 2 ? "Bye" : "Hello!"}.release();
        }},
    {"button text, setters", "button-text.png",
        8, true, true,
        [](UserInterface& ui, Int style, Int counter) {
            Button button{{ui, {64, 36}}, ButtonStyle(style), "Hey"};
            button.setText(counter % 2 ? "Bye" : "Hello!");
            return button.release();
        }},
    {"button text, setters on empty", "button-text.png",
        8, true, true,
        [](UserInterface& ui, Int style, Int counter) {
            Button button{{ui, {64, 36}}, ButtonStyle(style), ""};
            button.setText(counter % 2 ? "Bye" : "Hello!");
            return button.release();
        }},
    {"button text, setStyle()", "button-text.png",
        8, true, true,
        [](UserInterface& ui, Int style, Int counter) {
            Button button{{ui, {64, 36}}, ButtonStyle(style == 0 ? 1 : 0), counter % 2 ? "Bye" : "Hello!"};
            button.setStyle(ButtonStyle(style));
            return button.release();
        }},
    {"button text, setStyle() on empty, setters", "button-text.png",
        8, true, true,
        [](UserInterface& ui, Int style, Int counter) {
            Button button{{ui, {64, 36}}, ButtonStyle(style == 0 ? 1 : 0), ""};
            button.setStyle(ButtonStyle(style));
            button.setText(counter % 2 ? "Bye" : "Hello!");
            return button.release();
        }},
    {"button icon, stateless", "button-icon.png",
        8, true, true,
        [](UserInterface& ui, Int style, Int counter) {
            /** @todo differently wide icons to test alignment */
            return button({ui, {48, 36}}, ButtonStyle(style), counter % 2 ? Icon::Yes : Icon::No).node();
        }},
    {"button icon", "button-icon.png",
        8, true, true,
        [](UserInterface& ui, Int style, Int counter) {
            return Button{{ui, {48, 36}}, ButtonStyle(style), counter % 2 ? Icon::Yes : Icon::No}.release();
        }},
    {"button icon, setters", "button-icon.png",
        8, true, true,
        [](UserInterface& ui, Int style, Int counter) {
            Button button{{ui, {48, 36}}, ButtonStyle(style), Icon::Yes};
            button.setIcon(counter % 2 ? Icon::Yes : Icon::No);
            return button.release();
        }},
    {"button icon, setters on empty", "button-icon.png",
        8, true, true,
        [](UserInterface& ui, Int style, Int counter) {
            Button button{{ui, {48, 36}}, ButtonStyle(style), Icon::None};
            button.setIcon(counter % 2 ? Icon::Yes : Icon::No);
            return button.release();
        }},
    {"button icon, setStyle()", "button-icon.png",
        8, true, true,
        [](UserInterface& ui, Int style, Int counter) {
            Button button{{ui, {48, 36}}, ButtonStyle(style == 0 ? 1 : 0), counter % 2 ? Icon::Yes : Icon::No};
            button.setStyle(ButtonStyle(style));
            return button.release();
        }},
    {"button icon, setStyle() on empty, setters", "button-icon.png",
        8, true, true,
        [](UserInterface& ui, Int style, Int counter) {
            Button button{{ui, {48, 36}}, ButtonStyle(style == 0 ? 1 : 0), Icon::None};
            button.setStyle(ButtonStyle(style));
            button.setIcon(counter % 2 ? Icon::Yes : Icon::No);
            return button.release();
        }},
};

StyleGLTest::StyleGLTest() {
    addInstancedTests({&StyleGLTest::render},
        Containers::arraySize(RenderData)*Containers::arraySize(StyleData));

    /* Prefer the StbImageImporter so we can keep files small but always import
       them as four-channel */
    if(PluginManager::PluginMetadata* metadata = _importerManager.metadata("StbImageImporter")) {
        metadata->configuration().setValue("forceChannelCount", 4);
        _importerManager.setPreferredPlugins("PngImporter", {"StbImageImporter"});
    }
    /* Prefer the StbTrueTypeFont so we don't have differences in font
       rasterization when TrueTypeFont is available */
    if(_fontManager.load("StbTrueTypeFont") & PluginManager::LoadState::Loaded) {
        _fontManager.setPreferredPlugins("TrueTypeFont", {"StbTrueTypeFont"});
    }

    /* Create just one actually filled UI for each possible style. Skip this on
       SwiftShader as it counts UBO size towards the uniform count limit, dying
       during shader compilation already if there's more than 256 vectors. */
    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
    if(!(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader))
    #endif
    {
        _styleUis = Containers::Array<UserInterfaceGL>{DirectInit, Containers::arraySize(StyleData), NoCreate};
        for(std::size_t i = 0; i != _styleUis.size(); ++i)
            _styleUis[StyleData[i].index].create({1024, 1024}, *StyleData[i].style, &_importerManager, &_fontManager);
    }
}

void StyleGLTest::render() {
    auto&& data = RenderData[testCaseInstanceId() / Containers::arraySize(StyleData)];
    auto&& styleData = StyleData[testCaseInstanceId() % Containers::arraySize(StyleData)];
    setTestCaseDescription(Utility::format("{}, {}", styleData.name, data.name));

    if(!(_importerManager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_importerManager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");
    if(!(_fontManager.load("StbTrueTypeFont") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("StbTrueTypeFont plugin not found.");

    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
    /* Same problem is with all builtin shaders, so this doesn't seem to be a
       bug in the base layer shader code. Compared to other tests doing this as
       soon as possible because apparently the damn thing is counting UBO size
       towards the uniform count limit, FFS, so if there's more than 256
       vectors which equals to just about 42 styles, it blows up.

       Ideally, with the thing being shitty like this, we'd at least fill up
       the UIs in order to have the coverage recorded for that, but due to it
       dying during shader compilation already we cannot- */
    if(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader)
        CORRADE_SKIP("UBOs with dynamically indexed arrays don't seem to work on SwiftShader, can't test.");
    #endif

    /* As an UI instance has a global concept of a currently hovered / pressed
       / ... node, we have to have several instances in order to render
       multiple widgets in a hovered state at once. Yes, it's nasty, in a way.
       Initially the UI is set to a larger size, the actual size is set later
       once we know how much the widgets span. */
    const std::size_t stateCount = 1 + (data.hoveredPressed ? 3 : 0) + (data.disabled ? 1 : 0);
    Containers::Array<UserInterfaceGL> uis{DirectInit, data.styleCount*stateCount, NoCreate};
    for(UserInterfaceGL& ui: uis) ui
        .setSize({1024, 1024})
        /* Not a compositing renderer with its own framebuffer as that would
           mean each instance would get its own, horrible inefficiency */
        /** @todo allow a setting non-owned renderer instance maybe? */
        .setRendererInstance(Containers::pointer<RendererGL>())
        .setBaseLayerInstance(Containers::pointer<BaseLayerGL>(ui.createLayer(), static_cast<BaseLayerGL::Shared&>(_styleUis[styleData.index].baseLayer().shared())))
        .setTextLayerInstance(Containers::pointer<TextLayerGL>(ui.createLayer(), static_cast<TextLayerGL::Shared&>(_styleUis[styleData.index].textLayer().shared())));
        /* Event layer not needed for anything yet */

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

        if(data.hoveredPressed) {
            {
                UserInterfaceGL& ui = uis[style*stateCount + 1];
                NodeHandle hover = data.create(ui, style, counter++);
                ui.setNodeOffset(hover, padding + (padding + size)*Vector2{Vector2i{1, style}});

                PointerMoveEvent move{{}, {}, {}};
                CORRADE_VERIFY(ui.pointerMoveEvent(ui.nodeOffset(hover) + ui.nodeSize(hover)*0.5f, move));
            } {
                UserInterfaceGL& ui = uis[style*stateCount + 2];
                NodeHandle pressedHover = data.create(ui, style, counter++);
                ui.setNodeOffset(pressedHover, padding + (padding + size)*Vector2{Vector2i{2, style}});

                PointerMoveEvent move{{}, {}, {}};
                CORRADE_VERIFY(ui.pointerMoveEvent(ui.nodeOffset(pressedHover) + ui.nodeSize(pressedHover)*0.5f, move));

                PointerEvent press{{}, Pointer::MouseLeft};
                CORRADE_VERIFY(ui.pointerPressEvent(ui.nodeOffset(pressedHover) + ui.nodeSize(pressedHover)*0.5f, press));
            } {
                UserInterfaceGL& ui = uis[style*stateCount + 3];
                NodeHandle pressed = data.create(ui, style, counter++);
                ui.setNodeOffset(pressed, padding + (padding + size)*Vector2{Vector2i{3, style}});

                PointerEvent press{{}, Pointer::MouseLeft};
                CORRADE_VERIFY(ui.pointerPressEvent(ui.nodeOffset(pressed) + ui.nodeSize(pressed)*0.5f, press));
            }
        }

        if(data.disabled) {
            UserInterfaceGL& ui = uis[style*stateCount + (data.hoveredPressed ? 4 : 1)];
            NodeHandle disabled = data.create(ui, style, counter++);
            ui.setNodeOffset(disabled, padding + (padding + size)*Vector2{Vector2i{data.hoveredPressed ? 4 : 1, style}});

            ui.addNodeFlags(disabled, NodeFlag::Disabled);
        }
    }

    /* Calculate the actual UI size. To avoid strange issues with events not
       being handled etc., it should always be smaller than the original set
       above. */
    const Vector2i uiSize = Vector2i{padding} + Vector2i{size + padding}*Vector2i{1 + (data.hoveredPressed ? 3 : 0) + (data.disabled ? 1 : 0), data.styleCount};
    CORRADE_COMPARE_AS(Vector2{uiSize},
        uis[0].size(),
        TestSuite::Compare::LessOrEqual);

    /* Set up a framebuffer to render to based on the area used */
    GL::Texture2D color;
    GL::Framebuffer framebuffer{{{}, uiSize}};
    color.setStorage(1, GL::TextureFormat::RGBA8, uiSize);
    framebuffer
        .attachTexture(GL::Framebuffer::ColorAttachment{0}, color, 0)
        /* Transparent clear color to make it possible to see a difference
           between a semi-transparent and washed-out widget color */
        .clearColor(0, 0x00000000_rgbaf)
        .bind();
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::One, GL::Renderer::BlendFunction::OneMinusSourceAlpha);

    /* Resize the UIs to what got actually used and draw everything */
    for(UserInterfaceGL& ui: uis)
        ui
            .setSize(uiSize)
            .draw();

    MAGNUM_VERIFY_NO_GL_ERROR();

    CORRADE_COMPARE_WITH(framebuffer.read({{}, uiSize}, {PixelFormat::RGBA8Unorm}),
        Utility::Path::join({WHEE_TEST_DIR, "StyleTestFiles", Containers::StringView{styleData.filePrefix} + data.filename}),
        DebugTools::CompareImageToFile{_importerManager});

    /* Verify that roundtrip state changes result in the same visuals as
       originally */
    if(data.hoveredPressed) {
        /* Pointer enter and leave on the inactive widget */
        for(Int style = 0; style != data.styleCount; ++style) {
            UserInterfaceGL& ui = uis[style*stateCount];
            /* We don't record the node handles, but each UI should have just
               one so this artificial one should be correct */
            NodeHandle node = nodeHandle(0, 1);
            CORRADE_VERIFY(ui.isHandleValid(node));

            /* Move over, making the node hovered, i.e. looking the same as in
               the second column */
            PointerMoveEvent moveOver{{}, {}, {}};
            CORRADE_VERIFY(ui.pointerMoveEvent(ui.nodeOffset(node) + ui.nodeSize(node)*0.5f, moveOver));

            /* Move out again */
            PointerMoveEvent moveOut{{}, {}, {}};
            CORRADE_VERIFY(!ui.pointerMoveEvent(ui.nodeOffset(node) + ui.nodeSize(node)*1.5f, moveOut));
        }

        /* Pointer leave and enter on the hovered widget */
        for(Int style = 0; style != data.styleCount; ++style) {
            UserInterfaceGL& ui = uis[style*stateCount + 1];
            NodeHandle node = nodeHandle(0, 1);
            CORRADE_VERIFY(ui.isHandleValid(node));

            /* Move out, making the node inactive, i.e. looking the same as in
               the first column */
            PointerMoveEvent moveOut{{}, {}, {}};
            CORRADE_VERIFY(!ui.pointerMoveEvent(ui.nodeOffset(node) + ui.nodeSize(node)*1.5f, moveOut));

            /* Move over again */
            PointerMoveEvent moveOver{{}, {}, {}};
            CORRADE_VERIFY(ui.pointerMoveEvent(ui.nodeOffset(node) + ui.nodeSize(node)*0.5f, moveOver));
        }

        /* Pointer leave and enter on the pressed + hovered widget */
        for(Int style = 0; style != data.styleCount; ++style) {
            UserInterfaceGL& ui = uis[style*stateCount + 2];
            NodeHandle node = nodeHandle(0, 1);
            CORRADE_VERIFY(ui.isHandleValid(node));

            /* Move out, making the node pressed but not hovered, i.e. looking
               the same as in the fourth column. As the node is captured, the
               event is accepted always. */
            PointerMoveEvent moveOut{{}, {}, {}};
            CORRADE_VERIFY(ui.pointerMoveEvent(ui.nodeOffset(node) + ui.nodeSize(node)*1.5f, moveOut));

            /* Move over again */
            PointerMoveEvent moveOver{{}, {}, {}};
            CORRADE_VERIFY(ui.pointerMoveEvent(ui.nodeOffset(node) + ui.nodeSize(node)*0.5f, moveOver));
        }

        /* Pointer enter and leave on the pressed widget */
        for(Int style = 0; style != data.styleCount; ++style) {
            UserInterfaceGL& ui = uis[style*stateCount + 3];
            NodeHandle node = nodeHandle(0, 1);
            CORRADE_VERIFY(ui.isHandleValid(node));

            /* Move over, making the node pressed + hovered, i.e. looking the
               same as in the third column */
            PointerMoveEvent moveOver{{}, {}, {}};
            CORRADE_VERIFY(ui.pointerMoveEvent(ui.nodeOffset(node) + ui.nodeSize(node)*0.5f, moveOver));

            /* Move out again. As the node is captured, the event is accepted
               always. */
            PointerMoveEvent moveOut{{}, {}, {}};
            CORRADE_VERIFY(ui.pointerMoveEvent(ui.nodeOffset(node) + ui.nodeSize(node)*1.5f, moveOut));
        }

        framebuffer.clearColor(0, 0x00000000_rgbaf);
        for(UserInterfaceGL& ui: uis)
            ui.draw();

        MAGNUM_VERIFY_NO_GL_ERROR();

        CORRADE_COMPARE_WITH(framebuffer.read({{}, uiSize}, {PixelFormat::RGBA8Unorm}),
            Utility::Path::join({WHEE_TEST_DIR, "StyleTestFiles", Containers::StringView{styleData.filePrefix} + data.filename}),
            DebugTools::CompareImageToFile{_importerManager});
    }
}

}}}}

CORRADE_TEST_MAIN(Magnum::Whee::Test::StyleGLTest)
