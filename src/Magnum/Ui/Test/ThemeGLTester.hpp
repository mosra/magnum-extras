#ifndef Magnum_Ui_Test_ThemeGLTester_hpp
#define Magnum_Ui_Test_ThemeGLTester_hpp
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

#include "Magnum/Ui/Anchor.h"
#include "Magnum/Ui/BaseLayerGL.h"
#include "Magnum/Ui/BaseLayerAnimator.h"
#include "Magnum/Ui/Event.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/LayoutLayer.h"
#include "Magnum/Ui/NodeFlags.h"
#include "Magnum/Ui/RendererGL.h"
#include "Magnum/Ui/TextLayerGL.h"
#include "Magnum/Ui/TextLayerAnimator.h"
#include "Magnum/Ui/UserInterfaceGL.h"

#include "configure.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct Theme {
    const char* name;
    const char* filePrefix;
    bool hasAnimations;
    Containers::Pointer<AbstractTheme> theme;
};

enum class Flag {
    HoveredPressed = 1 << 0,
    Focused = 1 << 1,
    Disabled = 1 << 2,
    XfailLlvmpipe20 = 1 << 3
};
typedef Containers::EnumSet<Flag> Flags;

CORRADE_ENUMSET_OPERATORS(Flags)

struct ThemeGLTester: GL::OpenGLTester {
    explicit ThemeGLTester(const Containers::ArrayView<const Theme>& styles);

    void render(NodeHandle(*create)(UserInterface& ui, Int style, Int counter), const Theme& themeData, const char* filename, Flags flags, Int styleCount, Nanoseconds animationDelta, Float maxThreshold, Float meanThreshold);

    std::size_t themeCount() const { return _themes.size(); }

    private:
        PluginManager::Manager<Text::AbstractFont> _fontManager;
        PluginManager::Manager<Trade::AbstractImporter> _importerManager;
        Containers::ArrayView<const Theme> _themes;
        Containers::Array<UserInterfaceGL> _themeUis;
};

using namespace Math::Literals;

ThemeGLTester::ThemeGLTester(const Containers::ArrayView<const Theme>& themes): _themes{themes} {
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

    /* Create just one actually filled UI for each possible theme. Skip this on
       SwiftShader as it counts UBO size towards the uniform count limit, dying
       during shader compilation already if there's more than 256 vectors. */
    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
    if(!(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader))
    #endif
    {
        _themeUis = Containers::Array<UserInterfaceGL>{DirectInit, themes.size(), NoCreate};
        for(std::size_t i = 0; i != themes.size(); ++i)
            _themeUis[i].create({1024, 1024}, *themes[i].theme, &_importerManager, &_fontManager);
    }
}

void ThemeGLTester::render(NodeHandle(*create)(UserInterface& ui, Int style, Int counter), const Theme& themeData, const char* const filename, const Flags flags, const Int styleCount, const Nanoseconds animationDelta, const Float maxThreshold, const Float meanThreshold) {
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
       once we know how much the widgets span.

       The focusable widgets currently ignore hover if pressed or focused.
       Which means it's the same count of extra styles (hovered, focused,
       pressed) like when handling just hover + press (hovered, pressed,
       hovered + pressed). */
    const std::size_t stateCount = 1 + (flags >= Flag::HoveredPressed ? (flags >= Flag::Focused ? 3 : 3) : 0) + (flags >= Flag::Disabled ? 1 : 0);
    Containers::Array<UserInterfaceGL> uis{DirectInit, styleCount*stateCount, NoCreate};
    for(UserInterfaceGL& ui: uis) {
        ui
            .setSize({1024, 1024})
            /* Not a compositing renderer with its own framebuffer as that
               would mean each instance would get its own, horrible
               inefficiency */
            /** @todo allow setting a non-owned renderer instance maybe? */
            .setRendererInstance(Containers::pointer<RendererGL>())
            .setBaseLayerInstance(Containers::pointer<BaseLayerGL>(ui.createLayer(), static_cast<BaseLayerGL::Shared&>(_themeUis[&themeData - _themes.data()].baseLayer().shared())))
            .setTextLayerInstance(Containers::pointer<TextLayerGL>(ui.createLayer(), static_cast<TextLayerGL::Shared&>(_themeUis[&themeData - _themes.data()].textLayer().shared())))
            /* Event layer not needed for anything yet */
            .setLayoutLayerInstance(Containers::pointer<LayoutLayer>(ui.createLayer(), _themeUis[&themeData - _themes.data()].layoutLayer().styleCount()));

        /* If dynamic styles are present (because the theme requested them for
           animators), add also default style animators. Can't hook to just
           Theme::hasAnimations, as presence of the animator might differ for
           each layer. */
        if(ui.baseLayer().shared().dynamicStyleCount())
            ui.setBaseLayerStyleAnimatorInstance(Containers::pointer<BaseLayerStyleAnimator>(ui.createAnimator()));
        if(ui.textLayer().shared().dynamicStyleCount())
            ui.setTextLayerStyleAnimatorInstance(Containers::pointer<TextLayerStyleAnimator>(ui.createAnimator()));
    }

    const Vector2 padding{8.0f};
    Nanoseconds now = 1773.0_sec;
    const Nanoseconds delta = themeData.hasAnimations ? animationDelta : 0_nsec;

    Int counter = 0;
    Vector2 size;
    for(Int style = 0; style != styleCount; ++style) {
        {
            UserInterfaceGL& ui = uis[style*stateCount + 0];
            NodeHandle node = create(ui, style, counter++);
            size = ui.nodeSize(node);
            ui.setNodeOffset(node, padding + (padding + size)*Vector2{Vector2i{0, style}});
        }

        if(flags >= Flag::Focused) {
            {
                UserInterfaceGL& ui = uis[style*stateCount + 1];
                NodeHandle hover = create(ui, style, counter++);
                ui.setNodeOffset(hover, padding + (padding + size)*Vector2{Vector2i{1, style}});

                PointerMoveEvent move{now, PointerEventSource::Pen, {}, {}, true, 0, {}};
                CORRADE_VERIFY(ui.pointerMoveEvent(ui.nodeOffset(hover) + ui.nodeSize(hover)*0.5f, move));
                CORRADE_COMPARE(ui.currentHoveredNode(), hover);
                CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
                CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
            } {
                UserInterfaceGL& ui = uis[style*stateCount + 2];
                NodeHandle pressed = create(ui, style, counter++);
                ui.setNodeOffset(pressed, padding + (padding + size)*Vector2{Vector2i{2, style}});

                /* The node should become focused as well */
                PointerEvent press{now, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0, {}};
                CORRADE_VERIFY(ui.pointerPressEvent(ui.nodeOffset(pressed) + ui.nodeSize(pressed)*0.5f, press));
                CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
                CORRADE_COMPARE(ui.currentPressedNode(), pressed);
                CORRADE_COMPARE(ui.currentFocusedNode(), pressed);
            } {
                UserInterfaceGL& ui = uis[style*stateCount + 3];
                NodeHandle focused = create(ui, style, counter++);
                ui.setNodeOffset(focused, padding + (padding + size)*Vector2{Vector2i{3, style}});

                /* The node should become focused without a press */
                FocusEvent focus{now};
                CORRADE_VERIFY(ui.focusEvent(focused, focus));
                CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
                CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
                CORRADE_COMPARE(ui.currentFocusedNode(), focused);
            }

        } else if(flags >= Flag::HoveredPressed) {
            {
                UserInterfaceGL& ui = uis[style*stateCount + 1];
                NodeHandle hover = create(ui, style, counter++);
                ui.setNodeOffset(hover, padding + (padding + size)*Vector2{Vector2i{1, style}});

                PointerMoveEvent move{now, PointerEventSource::Mouse, {}, {}, true, 0, {}};
                CORRADE_VERIFY(ui.pointerMoveEvent(ui.nodeOffset(hover) + ui.nodeSize(hover)*0.5f, move));
                CORRADE_COMPARE(ui.currentHoveredNode(), hover);
                CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
                CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
            } {
                UserInterfaceGL& ui = uis[style*stateCount + 2];
                NodeHandle pressedHover = create(ui, style, counter++);
                ui.setNodeOffset(pressedHover, padding + (padding + size)*Vector2{Vector2i{2, style}});

                PointerMoveEvent move{now, PointerEventSource::Mouse, {}, {}, true, 0, {}};
                PointerEvent press{now, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0, {}};
                CORRADE_VERIFY(ui.pointerMoveEvent(ui.nodeOffset(pressedHover) + ui.nodeSize(pressedHover)*0.5f, move));
                CORRADE_VERIFY(ui.pointerPressEvent(ui.nodeOffset(pressedHover) + ui.nodeSize(pressedHover)*0.5f, press));
                CORRADE_COMPARE(ui.currentHoveredNode(), pressedHover);
                CORRADE_COMPARE(ui.currentPressedNode(), pressedHover);
                CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
            } {
                UserInterfaceGL& ui = uis[style*stateCount + 3];
                NodeHandle pressed = create(ui, style, counter++);
                ui.setNodeOffset(pressed, padding + (padding + size)*Vector2{Vector2i{3, style}});

                PointerEvent press{now, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0, {}};
                CORRADE_VERIFY(ui.pointerPressEvent(ui.nodeOffset(pressed) + ui.nodeSize(pressed)*0.5f, press));
                CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
                CORRADE_COMPARE(ui.currentPressedNode(), pressed);
                CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
            }
        }

        if(flags >= Flag::Disabled) {
            UserInterfaceGL& ui = uis[style*stateCount + (flags >= Flag::HoveredPressed ? 4 : 1)];
            NodeHandle disabled = create(ui, style, counter++);
            ui.setNodeOffset(disabled, padding + (padding + size)*Vector2{Vector2i{flags >= Flag::HoveredPressed ? 4 : 1, style}});

            ui.addNodeFlags(disabled, NodeFlag::Disabled);
        }
    }

    /* Calculate the actual UI size. To avoid strange issues with events not
       being handled etc., it should always be smaller than the original set
       above. */
    const Vector2i uiSize = Vector2i{padding} + Vector2i{size + padding}*Vector2i{1 + (flags >= Flag::HoveredPressed ? 3 : 0) + (flags >= Flag::Disabled ? 1 : 0), styleCount};
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
        CORRADE_EXPECT_FAIL_IF(flags >= Flag::XfailLlvmpipe20 && GL::Context::current().rendererString().contains("llvmpipe") && GL::Context::current().versionString().contains("Mesa 20"),
            "Mesa llvmpipe 20 renders the text in a completely different color for some reason.");
        CORRADE_COMPARE_WITH(framebuffer.read({{}, uiSize}, {PixelFormat::RGBA8Unorm}),
            Utility::Path::join({UI_TEST_DIR, "ThemeTestFiles", Containers::StringView{themeData.filePrefix} + filename}),
            (DebugTools::CompareImageToFile{_importerManager, maxThreshold, meanThreshold}));
    }

    /* Verify that hovering the pressed and focused widgets doesn't have any
       difference in visuals */
    if(flags >= Flag::Focused) {
        /* Focused + pressed widget, should have no difference when hovered */
        for(Int style = 0; style != styleCount; ++style) {
            UserInterfaceGL& ui = uis[style*stateCount + 2];
            /* We don't record the node handles, but each UI should have just
               one so this artificial one should be correct */
            NodeHandle node = nodeHandle(0, 1);
            CORRADE_VERIFY(ui.isHandleValid(node));

            PointerMoveEvent moveOver{now, PointerEventSource::Mouse, {}, {}, true, 0, {}};
            CORRADE_VERIFY(ui.pointerMoveEvent(ui.nodeOffset(node) + ui.nodeSize(node)*0.5f, moveOver));
            CORRADE_COMPARE(ui.currentHoveredNode(), node);
            CORRADE_COMPARE(ui.currentPressedNode(), node);
            CORRADE_COMPARE(ui.currentFocusedNode(), node);
        }

        /* Focused widget, should have no difference when hovered */
        for(Int style = 0; style != styleCount; ++style) {
            UserInterfaceGL& ui = uis[style*stateCount + 3];
            NodeHandle node = nodeHandle(0, 1);
            CORRADE_VERIFY(ui.isHandleValid(node));

            PointerMoveEvent moveOver{now, PointerEventSource::Mouse, {}, {}, true, 0, {}};
            CORRADE_VERIFY(ui.pointerMoveEvent(ui.nodeOffset(node) + ui.nodeSize(node)*0.5f, moveOver));
            CORRADE_COMPARE(ui.currentHoveredNode(), node);
            CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
            CORRADE_COMPARE(ui.currentFocusedNode(), node);
        }

    /* Verify that roundtrip state changes result in the same visuals as
       originally. In order to handle animations correctly, the roundtrip is
       with animationAdvance() in the middle. */
    } else if(flags >= Flag::HoveredPressed) {
        /* Pointer enter (...and later leave) on the inactive widget */
        for(Int style = 0; style != styleCount; ++style) {
            UserInterfaceGL& ui = uis[style*stateCount];
            NodeHandle node = nodeHandle(0, 1);
            CORRADE_VERIFY(ui.isHandleValid(node));

            /* Move over, making the node hovered, i.e. looking the same as in
               the second column */
            PointerMoveEvent moveOver{now, PointerEventSource::Mouse, {}, {}, true, 0, {}};
            CORRADE_VERIFY(ui.pointerMoveEvent(ui.nodeOffset(node) + ui.nodeSize(node)*0.5f, moveOver));
            CORRADE_COMPARE(ui.currentHoveredNode(), node);
            CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
            CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        }

        /* Pointer leave (...and later enter) on the hovered widget */
        for(Int style = 0; style != styleCount; ++style) {
            UserInterfaceGL& ui = uis[style*stateCount + 1];
            NodeHandle node = nodeHandle(0, 1);
            CORRADE_VERIFY(ui.isHandleValid(node));

            /* Move out, making the node inactive, i.e. looking the same as in
               the first column */
            PointerMoveEvent moveOut{now, PointerEventSource::Mouse, {}, {}, true, 0, {}};
            CORRADE_VERIFY(!ui.pointerMoveEvent({}, moveOut));
            CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
            CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
            CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        }

        if(flags >= Flag::Focused) {
            /* Release (... and later press) on the focused + pressed widget */
            for(Int style = 0; style != styleCount; ++style) {
                UserInterfaceGL& ui = uis[style*stateCount + 2];
                NodeHandle node = nodeHandle(0, 1);
                CORRADE_VERIFY(ui.isHandleValid(node));

                /* Release, making the node focused but not pressed, i.e.
                   looking the same as in the fourth column. */
                PointerEvent release{now, PointerEventSource::Pen, Pointer::Pen, true, 0, {}};
                CORRADE_VERIFY(ui.pointerReleaseEvent({}, release));
                CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
                CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
                CORRADE_COMPARE(ui.currentFocusedNode(), node);
            }

            /* Press (... and later release) on the focused widget */
            for(Int style = 0; style != styleCount; ++style) {
                UserInterfaceGL& ui = uis[style*stateCount + 2];
                NodeHandle node = nodeHandle(0, 1);
                CORRADE_VERIFY(ui.isHandleValid(node));

                /* Making the node focused and pressed, i.e.  looking the same
                   as in the third column. */
                PointerEvent press{now, PointerEventSource::Pen, Pointer::Pen, true, 0, {}};
                CORRADE_VERIFY(ui.pointerPressEvent(ui.nodeOffset(node) + ui.nodeSize(node)*0.5f, press));
                CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
                CORRADE_COMPARE(ui.currentPressedNode(), node);
                CORRADE_COMPARE(ui.currentFocusedNode(), node);
            }

        } else {
            /* Pointer leave (... and later enter) on the pressed + hovered
               widget */
            for(Int style = 0; style != styleCount; ++style) {
                UserInterfaceGL& ui = uis[style*stateCount + 2];
                NodeHandle node = nodeHandle(0, 1);
                CORRADE_VERIFY(ui.isHandleValid(node));

                /* Making the node pressed but not hovered, i.e. looking the
                   same as in the fourth column. As the node is captured, the
                   event is accepted always. */
                PointerMoveEvent moveOut{now, PointerEventSource::Mouse, {}, {}, true, 0, {}};
                CORRADE_VERIFY(ui.pointerMoveEvent({}, moveOut));
                CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
                CORRADE_COMPARE(ui.currentPressedNode(), node);
                CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
            }

            /* Pointer enter (... and later leave) on the pressed widget */
            for(Int style = 0; style != styleCount; ++style) {
                UserInterfaceGL& ui = uis[style*stateCount + 3];
                NodeHandle node = nodeHandle(0, 1);
                CORRADE_VERIFY(ui.isHandleValid(node));

                /* Making the node pressed + hovered, i.e. looking the same as
                   in the third column */
                PointerMoveEvent moveOver{now, PointerEventSource::Mouse, {}, {}, true, 0, {}};
                CORRADE_VERIFY(ui.pointerMoveEvent(ui.nodeOffset(node) + ui.nodeSize(node)*0.5f, moveOver));
                CORRADE_COMPARE(ui.currentHoveredNode(), node);
                CORRADE_COMPARE(ui.currentPressedNode(), node);
                CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
            }
        }

        /* Advance animations to perform the style changes. If time delta is
           set to nothing, we don't expect any animations and thus don't need
           to advance. */
        if(delta != Nanoseconds{}) for(UserInterfaceGL& ui: uis)
            ui.advanceAnimations(now + delta);
        now += delta;

        /* Pointer leave (after previous enter) on the inactive widget */
        for(Int style = 0; style != styleCount; ++style) {
            UserInterfaceGL& ui = uis[style*stateCount];
            NodeHandle node = nodeHandle(0, 1);
            CORRADE_VERIFY(ui.isHandleValid(node));

            PointerMoveEvent moveOut{now, PointerEventSource::Mouse, {}, {}, true, 0, {}};
            CORRADE_VERIFY(!ui.pointerMoveEvent({}, moveOut));
            CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
            CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
            CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        }

        /* Pointer enter (after previous leave) on the hovered widget */
        for(Int style = 0; style != styleCount; ++style) {
            UserInterfaceGL& ui = uis[style*stateCount + 1];
            NodeHandle node = nodeHandle(0, 1);
            CORRADE_VERIFY(ui.isHandleValid(node));

            PointerMoveEvent moveOver{now, PointerEventSource::Mouse, {}, {}, true, 0, {}};
            CORRADE_VERIFY(ui.pointerMoveEvent(ui.nodeOffset(node) + ui.nodeSize(node)*0.5f, moveOver));
            CORRADE_COMPARE(ui.currentHoveredNode(), node);
            CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
            CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        }

        if(flags >= Flag::Focused) {
            /* Press (after previous release) on the focused + pressed
               widget */
            for(Int style = 0; style != styleCount; ++style) {
                UserInterfaceGL& ui = uis[style*stateCount + 2];
                NodeHandle node = nodeHandle(0, 1);
                CORRADE_VERIFY(ui.isHandleValid(node));

                PointerEvent press{now, PointerEventSource::Pen, Pointer::Pen, true, 0, {}};
                CORRADE_VERIFY(ui.pointerPressEvent(ui.nodeOffset(node) + ui.nodeSize(node)*0.5f, press));
                CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
                CORRADE_COMPARE(ui.currentPressedNode(), node);
                CORRADE_COMPARE(ui.currentFocusedNode(), node);
            }

            /* Release (after previous press) on the focused widget */
            for(Int style = 0; style != styleCount; ++style) {
                UserInterfaceGL& ui = uis[style*stateCount + 2];
                NodeHandle node = nodeHandle(0, 1);
                CORRADE_VERIFY(ui.isHandleValid(node));

                PointerEvent release{now, PointerEventSource::Pen, Pointer::Pen, true, 0, {}};
                CORRADE_VERIFY(ui.pointerReleaseEvent({}, release));
                CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
                CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
                CORRADE_COMPARE(ui.currentFocusedNode(), node);
            }

        } else {
            /* Pointer enter (after previous leave) on the pressed + hovered
               widget */
            for(Int style = 0; style != styleCount; ++style) {
                UserInterfaceGL& ui = uis[style*stateCount + 2];
                NodeHandle node = nodeHandle(0, 1);
                CORRADE_VERIFY(ui.isHandleValid(node));

                PointerMoveEvent moveOver{now, PointerEventSource::Mouse, {}, {}, true, 0, {}};
                CORRADE_VERIFY(ui.pointerMoveEvent(ui.nodeOffset(node) + ui.nodeSize(node)*0.5f, moveOver));
                CORRADE_COMPARE(ui.currentHoveredNode(), node);
                CORRADE_COMPARE(ui.currentPressedNode(), node);
                CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
            }

            /* Pointer leave (after previous enter) on the pressed widget */
            for(Int style = 0; style != styleCount; ++style) {
                UserInterfaceGL& ui = uis[style*stateCount + 3];
                NodeHandle node = nodeHandle(0, 1);
                CORRADE_VERIFY(ui.isHandleValid(node));

                /* As the node is captured, the event is accepted always */
                PointerMoveEvent moveOut{now, PointerEventSource::Mouse, {}, {}, true, 0, {}};
                CORRADE_VERIFY(ui.pointerMoveEvent({}, moveOut));
                CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
                CORRADE_COMPARE(ui.currentPressedNode(), node);
                CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
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
            CORRADE_EXPECT_FAIL_IF(flags >= Flag::XfailLlvmpipe20 && GL::Context::current().rendererString().contains("llvmpipe") && GL::Context::current().versionString().contains("Mesa 20"),
                "Mesa llvmpipe 20 renders the text in a completely different color for some reason.");
            CORRADE_COMPARE_WITH(framebuffer.read({{}, uiSize}, {PixelFormat::RGBA8Unorm}),
                Utility::Path::join({UI_TEST_DIR, "ThemeTestFiles", Containers::StringView{themeData.filePrefix} + filename}),
                (DebugTools::CompareImageToFile{_importerManager, maxThreshold, meanThreshold}));
        }
    }
}

}}}}

#endif
