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

#include "Magnum/Ui/Anchor.h"
#include "Magnum/Ui/BaseLayerGL.h"
#include "Magnum/Ui/BaseLayerAnimator.h"
#include "Magnum/Ui/Button.h"
#include "Magnum/Ui/Event.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/Input.h"
#include "Magnum/Ui/Label.h"
#include "Magnum/Ui/NodeFlags.h"
#include "Magnum/Ui/RendererGL.h"
#include "Magnum/Ui/Style.h"
#include "Magnum/Ui/TextLayerGL.h"
#include "Magnum/Ui/TextLayerAnimator.h"
#include "Magnum/Ui/UserInterfaceGL.h"

#include "configure.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

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
    bool hasAnimations;
    Containers::Pointer<AbstractStyle> style;
} StyleData[]{
    {"m.css dark", "mcss-dark-", false, Containers::pointer<McssDarkStyle>()},
    {"m.css dark SubdividedQuads", "mcss-dark-", false, []{
        Containers::Pointer<McssDarkStyle> style{InPlaceInit};
        style->setBaseLayerFlags(BaseLayerSharedFlag::SubdividedQuads, {});
        return style;
    }()},
    {"m.css dark, EssentialAnimations", "mcss-dark-", true, Containers::pointer<McssDarkStyle>(McssDarkStyle::Feature::EssentialAnimations)},
    /* With full animations the longest duration is half a second */
    {"m.css dark, Animations", "mcss-dark-", true, Containers::pointer<McssDarkStyle>(McssDarkStyle::Feature::Animations)},
};

const struct {
    const char* name;
    const char* filename;
    struct RenderDataProperties {
        Int styleCount;
        bool hoveredPressed, focused, disabled;
        Nanoseconds animationDelta;
        Float maxThreshold, meanThreshold;
        bool xfailLlvmpipe20;
    } properties;
    NodeHandle(*create)(UserInterface& ui, Int style, Int counter);
} RenderData[]{
    {"button text + icon, stateless", "button-text-icon.png",
        /* Button fade out animations are all 0.5 sec */
        {8, true, false, true, 0.5_sec, 2.0f, 0.0399f, true},
        [](UserInterface& ui, Int style, Int counter) {
            /** @todo differently wide icons to test alignment */
            return button({ui, {96, 36}}, counter % 2 ? Icon::No : Icon::Yes, counter % 2 ? "Bye" : "Hello!", ButtonStyle(style)).node();
        }},
    {"button text + icon", nullptr, {},
        [](UserInterface& ui, Int style, Int counter) {
            return Button{{ui, {96, 36}}, counter % 2 ? Icon::No : Icon::Yes, counter % 2 ? "Bye" : "Hello!", ButtonStyle(style)}.release();
        }},
    {"button text + icon, setters", nullptr, {},
        [](UserInterface& ui, Int style, Int counter) {
            Button button{{ui, {96, 36}}, Icon::No, "Hey", ButtonStyle(style)};
            button.setIcon(counter % 2 ? Icon::No : Icon::Yes);
            button.setText(counter % 2 ? "Bye" : "Hello!");
            return button.release();
        }},
    {"button text + icon, setters on empty", nullptr, {},
        [](UserInterface& ui, Int style, Int counter) {
            Button button{{ui, {96, 36}}, Icon::None, "", ButtonStyle(style)};
            button.setIcon(counter % 2 ? Icon::No : Icon::Yes);
            button.setText(counter % 2 ? "Bye" : "Hello!");
            return button.release();
        }},
    {"button text + icon, setters on empty, different order", nullptr, {},
        [](UserInterface& ui, Int style, Int counter) {
            Button button{{ui, {96, 36}}, Icon::None, "", ButtonStyle(style)};
            button.setText(counter % 2 ? "Bye" : "Hello!");
            button.setIcon(counter % 2 ? Icon::No : Icon::Yes);
            return button.release();
        }},
    {"button text + icon, setStyle()", nullptr, {},
        [](UserInterface& ui, Int style, Int counter) {
            Button button{{ui, {96, 36}}, counter % 2 ? Icon::No : Icon::Yes, counter % 2 ? "Bye" : "Hello!", ButtonStyle(style == 0 ? 1 : 0)};
            button.setStyle(ButtonStyle(style));
            return button.release();
        }},
    {"button text + icon, setStyle() on empty, setters", nullptr, {},
        [](UserInterface& ui, Int style, Int counter) {
            Button button{{ui, {96, 36}}, Icon::None, "", ButtonStyle(style == 0 ? 1 : 0)};
            button.setStyle(ButtonStyle(style));
            button.setIcon(counter % 2 ? Icon::No : Icon::Yes);
            button.setText(counter % 2 ? "Bye" : "Hello!");
            return button.release();
        }},

    {"button text, stateless", "button-text.png",
        /* Button fade out animations are all 0.5 sec */
        {8, true, false, true, 0.5_sec, 2.0f, 0.0386f, true},
        [](UserInterface& ui, Int style, Int counter) {
            return button({ui, {64, 36}}, counter % 2 ? "Bye" : "Hello!", ButtonStyle(style)).node();
        }},
    {"button text", nullptr, {},
        [](UserInterface& ui, Int style, Int counter) {
            return Button{{ui, {64, 36}}, counter % 2 ? "Bye" : "Hello!", ButtonStyle(style)}.release();
        }},
    {"button text, setters", nullptr, {},
        [](UserInterface& ui, Int style, Int counter) {
            Button button{{ui, {64, 36}}, "Hey", ButtonStyle(style)};
            button.setText(counter % 2 ? "Bye" : "Hello!");
            return button.release();
        }},
    {"button text, setters on empty", nullptr, {},
        [](UserInterface& ui, Int style, Int counter) {
            Button button{{ui, {64, 36}}, "", ButtonStyle(style)};
            button.setText(counter % 2 ? "Bye" : "Hello!");
            return button.release();
        }},
    {"button text, setStyle()", nullptr, {},
        [](UserInterface& ui, Int style, Int counter) {
            Button button{{ui, {64, 36}}, counter % 2 ? "Bye" : "Hello!", ButtonStyle(style == 0 ? 1 : 0)};
            button.setStyle(ButtonStyle(style));
            return button.release();
        }},
    {"button text, setStyle() on empty, setters", nullptr, {},
        [](UserInterface& ui, Int style, Int counter) {
            Button button{{ui, {64, 36}}, "", ButtonStyle(style == 0 ? 1 : 0)};
            button.setStyle(ButtonStyle(style));
            button.setText(counter % 2 ? "Bye" : "Hello!");
            return button.release();
        }},

    {"button icon, stateless", "button-icon.png",
        /* Button fade out animations are all 0.5 sec */
        {8, true, false, true, 0.5_sec, 1.25f, 0.0278f, true},
        [](UserInterface& ui, Int style, Int counter) {
            /** @todo differently wide icons to test alignment */
            return button({ui, {48, 36}}, counter % 2 ? Icon::Yes : Icon::No, ButtonStyle(style)).node();
        }},
    {"button icon", nullptr, {},
        [](UserInterface& ui, Int style, Int counter) {
            return Button{{ui, {48, 36}}, counter % 2 ? Icon::Yes : Icon::No, ButtonStyle(style)}.release();
        }},
    {"button icon, setters", nullptr, {},
        [](UserInterface& ui, Int style, Int counter) {
            Button button{{ui, {48, 36}}, Icon::Yes, ButtonStyle(style)};
            button.setIcon(counter % 2 ? Icon::Yes : Icon::No);
            return button.release();
        }},
    {"button icon, setters on empty", nullptr, {},
        [](UserInterface& ui, Int style, Int counter) {
            Button button{{ui, {48, 36}}, Icon::None, ButtonStyle(style)};
            button.setIcon(counter % 2 ? Icon::Yes : Icon::No);
            return button.release();
        }},
    {"button icon, setStyle()", nullptr, {},
        [](UserInterface& ui, Int style, Int counter) {
            Button button{{ui, {48, 36}}, counter % 2 ? Icon::Yes : Icon::No, ButtonStyle(style == 0 ? 1 : 0)};
            button.setStyle(ButtonStyle(style));
            return button.release();
        }},
    {"button icon, setStyle() on empty, setters", nullptr, {},
        [](UserInterface& ui, Int style, Int counter) {
            Button button{{ui, {48, 36}}, Icon::None, ButtonStyle(style == 0 ? 1 : 0)};
            button.setStyle(ButtonStyle(style));
            button.setIcon(counter % 2 ? Icon::Yes : Icon::No);
            return button.release();
        }},

    {"label text, stateless", "label-text.png",
        /* Label has no animations */
        {7, false, false, true, {}, 2.0f, 0.0248f, false},
        [](UserInterface& ui, Int style, Int counter) {
            return label({ui, {52, 36}}, counter % 3 ? "Bye" : "Hello!", LabelStyle(style)).node();
        }},
    {"label text", nullptr, {},
        [](UserInterface& ui, Int style, Int counter) {
            return Label{{ui, {52, 36}}, counter % 3 ? "Bye" : "Hello!", LabelStyle(style)}.release();
        }},
    {"label text, setters", nullptr, {},
        [](UserInterface& ui, Int style, Int counter) {
            Label label{{ui, {52, 36}}, "Hey", LabelStyle(style)};
            label.setText(counter % 3 ? "Bye" : "Hello!");
            return label.release();
        }},
    {"label text, setters from empty", nullptr, {},
        [](UserInterface& ui, Int style, Int counter) {
            Label label{{ui, {52, 36}}, "", LabelStyle(style)};
            label.setText(counter % 3 ? "Bye" : "Hello!");
            return label.release();
        }},
    {"label text, setStyle()", nullptr, {},
        [](UserInterface& ui, Int style, Int counter) {
            Label label{{ui, {52, 36}}, counter % 3 ? "Bye" : "Hello!", LabelStyle(style == 0 ? 1 : 0)};
            label.setStyle(LabelStyle(style));
            return label.release();
        }},
    {"label text, setStyle() on empty, setters", nullptr, {},
        [](UserInterface& ui, Int style, Int counter) {
            Label label{{ui, {52, 36}}, "", LabelStyle(style == 0 ? 1 : 0)};
            label.setStyle(LabelStyle(style));
            label.setText(counter % 3 ? "Bye" : "Hello!");
            return label.release();
        }},

    {"label icon, stateless", "label-icon.png",
        /* Label has no animations */
        {7, false, false, true, {}, 1.75f, 0.0099f, false},
        [](UserInterface& ui, Int style, Int counter) {
            /** @todo differently wide icons to test alignment */
            return label({ui, {48, 36}}, counter % 3 ? Icon::Yes : Icon::No, LabelStyle(style)).node();
        }},
    {"label icon", nullptr, {},
        [](UserInterface& ui, Int style, Int counter) {
            return Label{{ui, {48, 36}}, counter % 3 ? Icon::Yes : Icon::No, LabelStyle(style)}.release();
        }},
    {"label icon, setters", nullptr, {},
        [](UserInterface& ui, Int style, Int counter) {
            Label label{{ui, {48, 36}}, Icon::Yes, LabelStyle(style)};
            label.setIcon(counter % 3 ? Icon::Yes : Icon::No);
            return label.release();
        }},
    {"label icon, setters on empty", nullptr, {},
        [](UserInterface& ui, Int style, Int counter) {
            Label label{{ui, {48, 36}}, Icon::None, LabelStyle(style)};
            label.setIcon(counter % 3 ? Icon::Yes : Icon::No);
            return label.release();
        }},
    {"label icon, setStyle()", nullptr, {},
        [](UserInterface& ui, Int style, Int counter) {
            Label label{{ui, {48, 36}}, counter % 3 ? Icon::Yes : Icon::No, LabelStyle(style == 0 ? 1 : 0)};
            label.setStyle(LabelStyle(style));
            return label.release();
        }},
    {"label icon, setStyle() on empty, setters", nullptr, {},
        [](UserInterface& ui, Int style, Int counter) {
            Label label{{ui, {48, 36}}, Icon::None, LabelStyle(style == 0 ? 1 : 0)};
            label.setStyle(LabelStyle(style));
            label.setIcon(counter % 3 ? Icon::Yes : Icon::No);
            return label.release();
        }},

    {"input", "input.png",
        /* Input cursor blinking lasts 0.55 sec and is reversed every other
           iteration, so it'll be fully visible at twice as much */
        {5, true, true, true, 0.55_sec*2, 2.0f, 0.0229f, true},
        [](UserInterface& ui, Int style, Int counter) {
            Input input{{ui, {64, 36}}, counter % 2 ? "Edit..." : "Type?", InputStyle(style)};
            /** @todo use a cursor setting API once it exists */
            ui.textLayer().setCursor(input.textData(), counter % 2 ? 2 : 5, counter % 2 ? 5 : 2);
            return input.release();
        }},
    {"input, setters", nullptr, {},
        [](UserInterface& ui, Int style, Int counter) {
            Input input{{ui, {64, 36}}, "", InputStyle(style)};
            input.setText(counter % 2 ? "Edit..." : "Type?");
            /** @todo use a cursor setting API once it exists */
            ui.textLayer().setCursor(input.textData(), counter % 2 ? 2 : 5, counter % 2 ? 5 : 2);
            return input.release();
        }},
    {"input, setStyle()", nullptr, {},
        [](UserInterface& ui, Int style, Int counter) {
            Input input{{ui, {64, 36}}, counter % 2 ? "Edit..." : "Type?", InputStyle(style == 0 ? 1 : 0)};
            input.setStyle(InputStyle(style));
            /** @todo use a cursor setting API once it exists */
            ui.textLayer().setCursor(input.textData(), counter % 2 ? 2 : 5, counter % 2 ? 5 : 2);
            return input.release();
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
            _styleUis[i].create({1024, 1024}, *StyleData[i].style, &_importerManager, &_fontManager);
    }
}

void StyleGLTest::render() {
    UnsignedInt dataIndex = testCaseInstanceId() / Containers::arraySize(StyleData);
    auto&& data = RenderData[dataIndex];
    UnsignedInt styleDataIndex = testCaseInstanceId() % Containers::arraySize(StyleData);
    auto&& styleData = StyleData[styleDataIndex];
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

    /* The properties like expected output or which states are meant to be
       tested are usually the same for a set of variants. If the filename isn't
       set, go back in the data, find one where it is, and that's where the
       properties come from as well */
    UnsignedInt filenameIndex = dataIndex;
    while(!RenderData[filenameIndex].filename)
        --filenameIndex;
    auto&& properties = RenderData[filenameIndex].properties;

    /* As an UI instance has a global concept of a currently hovered / pressed
       / ... node, we have to have several instances in order to render
       multiple widgets in a hovered state at once. Yes, it's nasty, in a way.
       Initially the UI is set to a larger size, the actual size is set later
       once we know how much the widgets span.

       The focusable widgets currently ignore hover if pressed or focused.
       Which means it's the same count of extra styles (hovered, focused,
       pressed) like when handling just hover + press (hovered, pressed,
       hovered + pressed). */
    const std::size_t stateCount = 1 + (properties.hoveredPressed ? (properties.focused ? 3 : 3) : 0) + (properties.disabled ? 1 : 0);
    Containers::Array<UserInterfaceGL> uis{DirectInit, properties.styleCount*stateCount, NoCreate};
    for(UserInterfaceGL& ui: uis) {
        ui
            .setSize({1024, 1024})
            /* Not a compositing renderer with its own framebuffer as that would
            mean each instance would get its own, horrible inefficiency */
            /** @todo allow a setting non-owned renderer instance maybe? */
            .setRendererInstance(Containers::pointer<RendererGL>())
            .setBaseLayerInstance(Containers::pointer<BaseLayerGL>(ui.createLayer(), static_cast<BaseLayerGL::Shared&>(_styleUis[styleDataIndex].baseLayer().shared())))
            .setTextLayerInstance(Containers::pointer<TextLayerGL>(ui.createLayer(), static_cast<TextLayerGL::Shared&>(_styleUis[styleDataIndex].textLayer().shared())));
            /* Event layer not needed for anything yet */

        /* If dynamic styles are present (because the style requested them for
           animators), add also default style animators. Can't hook to just
           StyleData::hasAnimations, as presence of the animator might differ
           for each layer. */
        if(ui.baseLayer().shared().dynamicStyleCount())
            ui.setBaseLayerStyleAnimatorInstance(Containers::pointer<BaseLayerStyleAnimator>(ui.createAnimator()));
        if(ui.textLayer().shared().dynamicStyleCount())
            ui.setTextLayerStyleAnimatorInstance(Containers::pointer<TextLayerStyleAnimator>(ui.createAnimator()));
    }

    const Vector2 padding{8.0f};
    Nanoseconds now = 1773.0_sec;
    const Nanoseconds delta = StyleData[styleDataIndex].hasAnimations ? properties.animationDelta : 0_nsec;

    Int counter = 0;
    Vector2 size;
    for(Int style = 0; style != properties.styleCount; ++style) {
        {
            UserInterfaceGL& ui = uis[style*stateCount + 0];
            NodeHandle node = data.create(ui, style, counter++);
            size = ui.nodeSize(node);
            ui.setNodeOffset(node, padding + (padding + size)*Vector2{Vector2i{0, style}});
        }

        if(properties.focused) {
            {
                UserInterfaceGL& ui = uis[style*stateCount + 1];
                NodeHandle hover = data.create(ui, style, counter++);
                ui.setNodeOffset(hover, padding + (padding + size)*Vector2{Vector2i{1, style}});

                PointerMoveEvent move{now, PointerEventSource::Pen, {}, {}, true, 0, {}};
                CORRADE_VERIFY(ui.pointerMoveEvent(ui.nodeOffset(hover) + ui.nodeSize(hover)*0.5f, move));
            } {
                UserInterfaceGL& ui = uis[style*stateCount + 2];
                NodeHandle pressed = data.create(ui, style, counter++);
                ui.setNodeOffset(pressed, padding + (padding + size)*Vector2{Vector2i{2, style}});

                /* The node should become focused as well */
                PointerEvent press{now, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0, {}};
                CORRADE_VERIFY(ui.pointerPressEvent(ui.nodeOffset(pressed) + ui.nodeSize(pressed)*0.5f, press));
                CORRADE_COMPARE(ui.currentFocusedNode(), pressed);
            } {

                UserInterfaceGL& ui = uis[style*stateCount + 3];
                NodeHandle focused = data.create(ui, style, counter++);
                ui.setNodeOffset(focused, padding + (padding + size)*Vector2{Vector2i{3, style}});

                /* The node should become focused without a press */
                FocusEvent focus{now};
                CORRADE_VERIFY(ui.focusEvent(focused, focus));
                CORRADE_COMPARE(ui.currentFocusedNode(), focused);
            }

        } else if(properties.hoveredPressed) {
            {
                UserInterfaceGL& ui = uis[style*stateCount + 1];
                NodeHandle hover = data.create(ui, style, counter++);
                ui.setNodeOffset(hover, padding + (padding + size)*Vector2{Vector2i{1, style}});

                PointerMoveEvent move{now, PointerEventSource::Mouse, {}, {}, true, 0, {}};
                CORRADE_VERIFY(ui.pointerMoveEvent(ui.nodeOffset(hover) + ui.nodeSize(hover)*0.5f, move));
            } {
                UserInterfaceGL& ui = uis[style*stateCount + 2];
                NodeHandle pressedHover = data.create(ui, style, counter++);
                ui.setNodeOffset(pressedHover, padding + (padding + size)*Vector2{Vector2i{2, style}});

                PointerMoveEvent move{now, PointerEventSource::Mouse, {}, {}, true, 0, {}};
                CORRADE_VERIFY(ui.pointerMoveEvent(ui.nodeOffset(pressedHover) + ui.nodeSize(pressedHover)*0.5f, move));

                PointerEvent press{now, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0, {}};
                CORRADE_VERIFY(ui.pointerPressEvent(ui.nodeOffset(pressedHover) + ui.nodeSize(pressedHover)*0.5f, press));
            } {
                UserInterfaceGL& ui = uis[style*stateCount + 3];
                NodeHandle pressed = data.create(ui, style, counter++);
                ui.setNodeOffset(pressed, padding + (padding + size)*Vector2{Vector2i{3, style}});

                PointerEvent press{now, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0, {}};
                CORRADE_VERIFY(ui.pointerPressEvent(ui.nodeOffset(pressed) + ui.nodeSize(pressed)*0.5f, press));
            }
        }

        if(properties.disabled) {
            UserInterfaceGL& ui = uis[style*stateCount + (properties.hoveredPressed ? 4 : 1)];
            NodeHandle disabled = data.create(ui, style, counter++);
            ui.setNodeOffset(disabled, padding + (padding + size)*Vector2{Vector2i{properties.hoveredPressed ? 4 : 1, style}});

            ui.addNodeFlags(disabled, NodeFlag::Disabled);
        }
    }

    /* Calculate the actual UI size. To avoid strange issues with events not
       being handled etc., it should always be smaller than the original set
       above. */
    const Vector2i uiSize = Vector2i{padding} + Vector2i{size + padding}*Vector2i{1 + (properties.hoveredPressed ? 3 : 0) + (properties.disabled ? 1 : 0), properties.styleCount};
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

    /* Resize the UIs to what got actually used, advance animations past the
       style change and draw everything */
    for(UserInterfaceGL& ui: uis) {
        ui.setSize(uiSize);

        /* If time delta is set to nothing, we don't expect any animations
            and thus don't need to advance */
        if(delta != Nanoseconds{})
            ui.advanceAnimations(now + delta);

        ui.draw();
    }

    now += delta;

    MAGNUM_VERIFY_NO_GL_ERROR();

    {
        CORRADE_EXPECT_FAIL_IF(properties.xfailLlvmpipe20 && GL::Context::current().rendererString().contains("llvmpipe") && GL::Context::current().versionString().contains("Mesa 20"),
            "Mesa llvmpipe 20 renders the text in a completely different color for some reason.");
        CORRADE_COMPARE_WITH(framebuffer.read({{}, uiSize}, {PixelFormat::RGBA8Unorm}),
            Utility::Path::join({UI_TEST_DIR, "StyleTestFiles", Containers::StringView{styleData.filePrefix} + RenderData[filenameIndex].filename}),
            (DebugTools::CompareImageToFile{_importerManager, properties.maxThreshold, properties.meanThreshold}));
    }

    /* Verify that hovering the pressed and focused widgets doesn't have any
       difference in visuals */
    if(properties.focused) {
        /* Focused + pressed widget, should have no difference when hovered */
        for(Int style = 0; style != properties.styleCount; ++style) {
            UserInterfaceGL& ui = uis[style*stateCount + 2];
            /* We don't record the node handles, but each UI should have just
               one so this artificial one should be correct */
            NodeHandle node = nodeHandle(0, 1);
            CORRADE_VERIFY(ui.isHandleValid(node));

            PointerMoveEvent moveOver{now, PointerEventSource::Mouse, {}, {}, true, 0, {}};
            CORRADE_VERIFY(ui.pointerMoveEvent(ui.nodeOffset(node) + ui.nodeSize(node)*0.5f, moveOver));
        }

        /* Focused widget, should have no difference when hovered */
        for(Int style = 0; style != properties.styleCount; ++style) {
            UserInterfaceGL& ui = uis[style*stateCount + 3];
            NodeHandle node = nodeHandle(0, 1);
            CORRADE_VERIFY(ui.isHandleValid(node));

            PointerMoveEvent moveOver{now, PointerEventSource::Mouse, {}, {}, true, 0, {}};
            CORRADE_VERIFY(ui.pointerMoveEvent(ui.nodeOffset(node) + ui.nodeSize(node)*0.5f, moveOver));
        }

    /* Verify that roundtrip state changes result in the same visuals as
       originally. In order to handle animations correctly, the roundtrip is
       with animationAdvance() in the middle. */
    } else if(properties.hoveredPressed) {
        /* Pointer enter and leave on the inactive widget */
        for(Int style = 0; style != properties.styleCount; ++style) {
            UserInterfaceGL& ui = uis[style*stateCount];
            NodeHandle node = nodeHandle(0, 1);
            CORRADE_VERIFY(ui.isHandleValid(node));

            /* Move over, making the node hovered, i.e. looking the same as in
               the second column */
            PointerMoveEvent moveOver{now, PointerEventSource::Mouse, {}, {}, true, 0, {}};
            CORRADE_VERIFY(ui.pointerMoveEvent(ui.nodeOffset(node) + ui.nodeSize(node)*0.5f, moveOver));
        }

        /* Pointer leave and enter on the hovered widget */
        for(Int style = 0; style != properties.styleCount; ++style) {
            UserInterfaceGL& ui = uis[style*stateCount + 1];
            NodeHandle node = nodeHandle(0, 1);
            CORRADE_VERIFY(ui.isHandleValid(node));

            /* Move out, making the node inactive, i.e. looking the same as in
               the first column */
            PointerMoveEvent moveOut{now, PointerEventSource::Mouse, {}, {}, true, 0, {}};
            CORRADE_VERIFY(!ui.pointerMoveEvent(ui.nodeOffset(node) + ui.nodeSize(node)*1.5f, moveOut));
        }

        if(properties.focused) {
            /* Release on the focused + pressed widget */
            for(Int style = 0; style != properties.styleCount; ++style) {
                UserInterfaceGL& ui = uis[style*stateCount + 2];
                NodeHandle node = nodeHandle(0, 1);
                CORRADE_VERIFY(ui.isHandleValid(node));

                /* Release, making the node focused but not pressed, i.e.
                   looking the same as in the fourth column. */
                PointerEvent release{now, PointerEventSource::Pen, Pointer::Pen, true, 0, {}};
                CORRADE_VERIFY(ui.pointerReleaseEvent(ui.nodeOffset(node) + ui.nodeSize(node)*1.5f, release));
            }

            /* Press on the focused widget */
            for(Int style = 0; style != properties.styleCount; ++style) {
                UserInterfaceGL& ui = uis[style*stateCount + 2];
                NodeHandle node = nodeHandle(0, 1);
                CORRADE_VERIFY(ui.isHandleValid(node));

                /* Making the node focused and pressed, i.e.  looking the same
                   as in the third column. */
                PointerEvent press{now, PointerEventSource::Pen, Pointer::Pen, true, 0, {}};
                CORRADE_VERIFY(ui.pointerPressEvent(ui.nodeOffset(node) + ui.nodeSize(node)*0.5f, press));
            }

        } else {
            /* Pointer leave on the pressed + hovered widget */
            for(Int style = 0; style != properties.styleCount; ++style) {
                UserInterfaceGL& ui = uis[style*stateCount + 2];
                NodeHandle node = nodeHandle(0, 1);
                CORRADE_VERIFY(ui.isHandleValid(node));

                /* Making the node pressed but not hovered, i.e. looking the
                   same as in the fourth column. As the node is captured, the
                   event is accepted always. */
                PointerMoveEvent moveOut{now, PointerEventSource::Mouse, {}, {}, true, 0, {}};
                CORRADE_VERIFY(ui.pointerMoveEvent(ui.nodeOffset(node) + ui.nodeSize(node)*1.5f, moveOut));
            }

            /* Pointer enter on the pressed widget */
            for(Int style = 0; style != properties.styleCount; ++style) {
                UserInterfaceGL& ui = uis[style*stateCount + 3];
                NodeHandle node = nodeHandle(0, 1);
                CORRADE_VERIFY(ui.isHandleValid(node));

                /* Making the node pressed + hovered, i.e. looking the same as
                   in the third column */
                PointerMoveEvent moveOver{now, PointerEventSource::Mouse, {}, {}, true, 0, {}};
                CORRADE_VERIFY(ui.pointerMoveEvent(ui.nodeOffset(node) + ui.nodeSize(node)*0.5f, moveOver));
            }
        }

        /* Advance animations to perform the style changes. If time delta is
           set to nothing, we don't expect any animations and thus don't need
           to advance. */
        if(delta != Nanoseconds{}) for(UserInterfaceGL& ui: uis)
            ui.advanceAnimations(now + delta);
        now += delta;

        /* Pointer leave on the inactive widget */
        for(Int style = 0; style != properties.styleCount; ++style) {
            UserInterfaceGL& ui = uis[style*stateCount];
            NodeHandle node = nodeHandle(0, 1);
            CORRADE_VERIFY(ui.isHandleValid(node));

            PointerMoveEvent moveOut{now, PointerEventSource::Mouse, {}, {}, true, 0, {}};
            CORRADE_VERIFY(!ui.pointerMoveEvent(ui.nodeOffset(node) + ui.nodeSize(node)*1.5f, moveOut));
        }

        /* Pointer enter on the hovered widget */
        for(Int style = 0; style != properties.styleCount; ++style) {
            UserInterfaceGL& ui = uis[style*stateCount + 1];
            NodeHandle node = nodeHandle(0, 1);
            CORRADE_VERIFY(ui.isHandleValid(node));

            PointerMoveEvent moveOver{now, PointerEventSource::Mouse, {}, {}, true, 0, {}};
            CORRADE_VERIFY(ui.pointerMoveEvent(ui.nodeOffset(node) + ui.nodeSize(node)*0.5f, moveOver));
        }

        if(properties.focused) {
            /* Press again on the focused + pressed widget */
            for(Int style = 0; style != properties.styleCount; ++style) {
                UserInterfaceGL& ui = uis[style*stateCount + 2];
                NodeHandle node = nodeHandle(0, 1);
                CORRADE_VERIFY(ui.isHandleValid(node));

                PointerEvent press{now, PointerEventSource::Pen, Pointer::Pen, true, 0, {}};
                CORRADE_VERIFY(ui.pointerPressEvent(ui.nodeOffset(node) + ui.nodeSize(node)*0.5f, press));
            }

            /* Release again on the focused widget */
            for(Int style = 0; style != properties.styleCount; ++style) {
                UserInterfaceGL& ui = uis[style*stateCount + 2];
                NodeHandle node = nodeHandle(0, 1);
                CORRADE_VERIFY(ui.isHandleValid(node));

                PointerEvent release{now, PointerEventSource::Pen, Pointer::Pen, true, 0, {}};
                CORRADE_VERIFY(ui.pointerReleaseEvent(ui.nodeOffset(node) + ui.nodeSize(node)*1.5f, release));
            }

        } else {
            /* Pointer enter on the pressed + hovered widget */
            for(Int style = 0; style != properties.styleCount; ++style) {
                UserInterfaceGL& ui = uis[style*stateCount + 2];
                NodeHandle node = nodeHandle(0, 1);
                CORRADE_VERIFY(ui.isHandleValid(node));

                PointerMoveEvent moveOver{now, PointerEventSource::Mouse, {}, {}, true, 0, {}};
                CORRADE_VERIFY(ui.pointerMoveEvent(ui.nodeOffset(node) + ui.nodeSize(node)*0.5f, moveOver));
            }

            /* Pointer leave on the pressed widget */
            for(Int style = 0; style != properties.styleCount; ++style) {
                UserInterfaceGL& ui = uis[style*stateCount + 3];
                NodeHandle node = nodeHandle(0, 1);
                CORRADE_VERIFY(ui.isHandleValid(node));

                /* As the node is captured, the event is accepted always */
                PointerMoveEvent moveOut{now, PointerEventSource::Mouse, {}, {}, true, 0, {}};
                CORRADE_VERIFY(ui.pointerMoveEvent(ui.nodeOffset(node) + ui.nodeSize(node)*1.5f, moveOut));
            }
        }

        framebuffer.clearColor(0, 0x00000000_rgbaf);
        for(UserInterfaceGL& ui: uis) {
            /* If time delta is set to nothing, we don't expect any animations
               and thus don't need to advance */
            if(delta != Nanoseconds{})
                ui.advanceAnimations(now + delta);
            ui.draw();
        }

        MAGNUM_VERIFY_NO_GL_ERROR();

        {
            CORRADE_EXPECT_FAIL_IF(properties.xfailLlvmpipe20 && GL::Context::current().rendererString().contains("llvmpipe") && GL::Context::current().versionString().contains("Mesa 20"),
                "Mesa llvmpipe 20 renders the text in a completely different color for some reason.");
            CORRADE_COMPARE_WITH(framebuffer.read({{}, uiSize}, {PixelFormat::RGBA8Unorm}),
                Utility::Path::join({UI_TEST_DIR, "StyleTestFiles", Containers::StringView{styleData.filePrefix} + RenderData[filenameIndex].filename}),
                (DebugTools::CompareImageToFile{_importerManager, properties.maxThreshold, properties.meanThreshold}));
        }
    }
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::StyleGLTest)
