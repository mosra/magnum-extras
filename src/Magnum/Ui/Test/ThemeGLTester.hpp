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

#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/StridedArrayView.h>
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
#include "Magnum/Ui/DataLayer.h"
#include "Magnum/Ui/Event.h"
#include "Magnum/Ui/EventLayer.h"
#include "Magnum/Ui/GenericLayouter.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/LayoutLayer.h"
#include "Magnum/Ui/NodeFlags.h"
#include "Magnum/Ui/RendererGL.h"
#include "Magnum/Ui/SnapLayouter.h"
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
    Hovered = 1 << 0,
    Pressed = 1 << 1,
    /* Verifies hover alone, press alone and hover + press. Some widgets don't
       treat hover + press differently from hover alone, so it's separate. */
    HoveredPressed = Hovered|Pressed|(1 << 2),
    /* Verifies hover alone, press alone and focus. All focusable widgets
       currently ignore hover when focused. */
    Focused = Hovered|Pressed|(1 << 3),
    Disabled = 1 << 4,
    XfailLlvmpipe20 = 1 << 5
};
typedef Containers::EnumSet<Flag> Flags;

CORRADE_ENUMSET_OPERATORS(Flags)

struct ThemeGLTester: GL::OpenGLTester {
    explicit ThemeGLTester(const Containers::ArrayView<const Theme>& styles);

    void render(NodeHandle(*create)(UserInterface& ui, Int style, Flags flags, Int counter), const Theme& themeData, const char* filename, Flags flags, Int styleCount, Nanoseconds animationDelta, Float maxThreshold, Float meanThreshold);

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

void ThemeGLTester::render(NodeHandle(*create)(UserInterface& ui, Int style, Flags flags, Int counter), const Theme& themeData, const char* const filename, const Flags flags, const Int styleCount, const Nanoseconds animationDelta, const Float maxThreshold, const Float meanThreshold) {
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

    struct TestUserInterface: UserInterfaceGL {
        using UserInterfaceGL::UserInterfaceGL;

        /* Same as NodeOffsetSizeQueryLayer in WidgetTester.hpp but here local
           to the test case */
        struct NodeOffsetSizeQueryLayer: AbstractLayer {
            using AbstractLayer::AbstractLayer;

            LayerFeatures doFeatures() const override { return {}; }
            void doUpdate(LayerStates, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::StridedArrayView1D<const Float>&, Containers::BitArrayView, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&) override {
                if(node != NodeHandle::Null) {
                    const UnsignedInt nodeId = nodeHandleId(node);
                    nodeOffset = nodeOffsets[nodeId];
                    nodeSize = nodeSizes[nodeId];
                }
            }

            NodeHandle node = NodeHandle::Null;
            Vector2 nodeOffset, nodeSize;
        };

        Vector2 nodeCenterAfterLayout(const NodeHandle node) {
            if(!_nodeOffsetSizeQueryLayer) {
                _nodeOffsetSizeQueryLayer = &setLayerInstance(Containers::pointer<NodeOffsetSizeQueryLayer>(createLayer()));
            }

            _nodeOffsetSizeQueryLayer->node = node;
            _nodeOffsetSizeQueryLayer->setNeedsUpdate(LayerState::NeedsDataUpdate);
            update();
            return _nodeOffsetSizeQueryLayer->nodeOffset + _nodeOffsetSizeQueryLayer->nodeSize*0.5f;
        }

        /* Node of a particular widget to receive events. Since at most one
           node of the whole UI can be hovered/pressed/focused at a time, this
           is just one variable as well. */
        NodeHandle node = NodeHandle::Null;

        private:
            NodeOffsetSizeQueryLayer* _nodeOffsetSizeQueryLayer{};
    };

    /* As an UI instance has a global concept of a currently hovered / pressed
       / ... node, we have to have several instances in order to render
       multiple widgets in a hovered state at once. Yes, it's nasty, in a way.
       Initially the UI is set to a larger size, the actual size is set later
       once we know how much the widgets span. */
    Int stateCount = 1;
    if(flags >= Flag::Hovered)
        ++stateCount;
    if(flags >= Flag::Pressed)
        ++stateCount;
    /* The focusable widgets currently ignore hover if pressed or focused.
       Which means it's the same count of extra styles (hovered, focused,
       pressed) like when handling just hover + press (hovered, pressed,
       hovered + pressed). */
    if(flags >= Flag::Focused) {
        CORRADE_INTERNAL_ASSERT(!(flags >= Flag::HoveredPressed));
        ++stateCount;
    }
    /* Some widgets don't differentiate hover + press from hover alone, so they
       set just Hovered + Pressed */
    if(flags >= Flag::HoveredPressed)
        ++stateCount;
    if(flags >= Flag::Disabled)
        ++stateCount;
    Containers::Array<TestUserInterface> uis{DirectInit, styleCount*std::size_t(stateCount), NoCreate};
    for(TestUserInterface& ui: uis) {
        ui
            .setSize({1024, 1024})
            /* Not a compositing renderer with its own framebuffer as that
               would mean each instance would get its own, horrible
               inefficiency */
            /** @todo allow setting a non-owned renderer instance maybe? */
            .setRendererInstance(Containers::pointer<RendererGL>())
            .setDataLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()))
            .setBaseLayerInstance(Containers::pointer<BaseLayerGL>(ui.createLayer(), static_cast<BaseLayerGL::Shared&>(_themeUis[&themeData - _themes.data()].baseLayer().shared())))
            .setTextLayerInstance(Containers::pointer<TextLayerGL>(ui.createLayer(), static_cast<TextLayerGL::Shared&>(_themeUis[&themeData - _themes.data()].textLayer().shared())))
            .setEventLayerInstance(Containers::pointer<EventLayer>(ui.createLayer()))
            .setLayoutLayerInstance(Containers::pointer<LayoutLayer>(ui.createLayer(), _themeUis[&themeData - _themes.data()].layoutLayer().styleCount()))
            .setSnapLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()))
            .setGenericLayouterInstance(Containers::pointer<GenericLayouter>(ui.createLayouter()));

        themeData.theme->apply(ui, ThemeFeature::LayoutLayer, nullptr, nullptr);

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
            TestUserInterface& ui = uis[style*stateCount + 0];

            /* The data creation function returns the node to which events
               should be directed. For positioning use the top-level node. */
            ui.node = create(ui, style, Flags{}, counter++);
            NodeHandle topLevelNode = ui.node;
            while(ui.nodeParent(topLevelNode) != NodeHandle::Null)
                topLevelNode = ui.nodeParent(topLevelNode);

            size = ui.nodeSize(topLevelNode);
            ui.setNodeOffset(topLevelNode, padding + (padding + size)*Vector2{Vector2i{0, style}});
        }

        Int stateIndex = 0;

        if(flags >= Flag::Hovered) {
            TestUserInterface& ui = uis[style*stateCount + ++stateIndex];

            /* The data creation function returns the node to which events
               should be directed. For positioning use the top-level node. */
            ui.node = create(ui, style, Flag::Hovered, counter++);
            if(ui.node != NodeHandle::Null) {
                NodeHandle topLevelNode = ui.node;
                while(ui.nodeParent(topLevelNode) != NodeHandle::Null)
                    topLevelNode = ui.nodeParent(topLevelNode);
                ui.setNodeOffset(topLevelNode, padding + (padding + size)*Vector2{Vector2i{1, style}});

                PointerMoveEvent move{now, PointerEventSource::Pen, {}, {}, true, 0, {}};
                CORRADE_VERIFY(ui.pointerMoveEvent(ui.nodeCenterAfterLayout(ui.node), move));
                CORRADE_COMPARE(ui.currentHoveredNode(), ui.node);
                CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
                CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
            }
        }

        if(flags >= Flag::HoveredPressed)  {
            /* The focusable widgets currently ignore hover if pressed or
               focused, so it's either this branch or the one below */
            CORRADE_INTERNAL_ASSERT(!(flags >= Flag::Focused));

            TestUserInterface& ui = uis[style*stateCount + ++stateIndex];

            /* The data creation function returns the node to which events
               should be directed. For positioning use the top-level node. */
            ui.node = create(ui, style, Flag::HoveredPressed, counter++);
            if(ui.node != NodeHandle::Null) {
                NodeHandle topLevelNode = ui.node;
                while(ui.nodeParent(topLevelNode) != NodeHandle::Null)
                    topLevelNode = ui.nodeParent(topLevelNode);
                ui.setNodeOffset(topLevelNode, padding + (padding + size)*Vector2{Vector2i{stateIndex, style}});

                PointerMoveEvent move{now, PointerEventSource::Mouse, {}, {}, true, 0, {}};
                PointerEvent press{now, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0, {}};
                CORRADE_VERIFY(ui.pointerMoveEvent(ui.nodeCenterAfterLayout(ui.node), move));
                CORRADE_VERIFY(ui.pointerPressEvent(ui.nodeCenterAfterLayout(ui.node), press));
                CORRADE_COMPARE(ui.currentHoveredNode(), ui.node);
                CORRADE_COMPARE(ui.currentPressedNode(), ui.node);
                CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
            }
        }

        if(flags >= Flag::Focused) {
            /* The focusable widgets currently ignore hover if pressed or
               focused, so it's either this branch or the one above */
            CORRADE_INTERNAL_ASSERT(!(flags >= Flag::HoveredPressed));

            TestUserInterface& ui = uis[style*stateCount + ++stateIndex];

            /* The data creation function returns the node to which events
                should be directed. For positioning use the top-level
                node. */
            ui.node = create(ui, style, Flag::Focused, counter++);
            if(ui.node != NodeHandle::Null) {
                NodeHandle topLevelNode = ui.node;
                while(ui.nodeParent(topLevelNode) != NodeHandle::Null)
                    topLevelNode = ui.nodeParent(topLevelNode);
                ui.setNodeOffset(topLevelNode, padding + (padding + size)*Vector2{Vector2i{stateIndex, style}});

                /* The node should become focused without a press */
                FocusEvent focus{now};
                CORRADE_VERIFY(ui.focusEvent(ui.node, focus));
                CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
                CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
                CORRADE_COMPARE(ui.currentFocusedNode(), ui.node);
            }
        }

        if(flags >= Flag::Pressed) {
            TestUserInterface& ui = uis[style*stateCount + ++stateIndex];

            /* The data creation function returns the node to which events
               should be directed. For positioning use the top-level node. */
            ui.node = create(ui, style, Flag::Pressed, counter++);
            if(ui.node != NodeHandle::Null) {
                NodeHandle topLevelNode = ui.node;
                while(ui.nodeParent(topLevelNode) != NodeHandle::Null)
                    topLevelNode = ui.nodeParent(topLevelNode);
                ui.setNodeOffset(topLevelNode, padding + (padding + size)*Vector2{Vector2i{stateIndex, style}});

                PointerEvent press{now, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0, {}};
                CORRADE_VERIFY(ui.pointerPressEvent(ui.nodeCenterAfterLayout(ui.node), press));
                CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
                CORRADE_COMPARE(ui.currentPressedNode(), ui.node);
                /* If the node is focusable, it should become focused as well */
                CORRADE_COMPARE(ui.currentFocusedNode(),
                    flags >= Flag::Focused ? ui.node : NodeHandle::Null);
            }
        }

        if(flags >= Flag::Disabled) {
            TestUserInterface& ui = uis[style*stateCount + ++stateIndex];

            /* The data creation function returns the node to which events
               should be directed. For positioning use the top-level node. */
            ui.node = create(ui, style, Flag::Disabled, counter++);
            if(ui.node != NodeHandle::Null) {
                NodeHandle topLevelNode = ui.node;
                while(ui.nodeParent(topLevelNode) != NodeHandle::Null)
                    topLevelNode = ui.nodeParent(topLevelNode);
                ui.setNodeOffset(topLevelNode, padding + (padding + size)*Vector2{Vector2i{stateIndex, style}});

                /* Compared to other cases above, the whole widget is disabled, not
                just the individual node inside */
                ui.addNodeFlags(topLevelNode, NodeFlag::Disabled);
            }
        }

        CORRADE_INTERNAL_ASSERT(stateIndex + 1 == stateCount);
    }

    /* Calculate the actual UI size. To avoid strange issues with events not
       being handled etc., it should always be smaller than the original set
       above. */
    const Vector2i uiSize = Vector2i{padding} + Vector2i{size + padding}*Vector2i{stateCount, styleCount};
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

    /* If there's no hovered, focused or pressed state, we have no roundtrip
       tests left to do */
    if(!(flags & (Flag::Hovered|Flag::Pressed|Flag::Focused)))
        return;

    /* Verify that roundtrip state changes result in the same visuals as
       originally. In order to handle animations correctly, the roundtrip is
       with animationAdvance() in the middle. */
    for(Int style = 0; style != styleCount; ++style) {
        /* Pointer enter (...and later leave) on the inactive widget (which is
           always the first one) */
        if(flags >= Flag::Hovered) {
            TestUserInterface& ui = uis[style*stateCount + 0];

            /* Move over, making the node hovered, i.e. looking the same as in
               the second column. Do it only if the hovered widget (which is
               always at index 1) is actually there, otherwise we may not have
               a node to hover. */
            PointerMoveEvent moveOver{now, PointerEventSource::Mouse, {}, {}, true, 0, {}};
            if(uis[style*stateCount + 1].node != NodeHandle::Null) {
                CORRADE_VERIFY(ui.pointerMoveEvent(ui.nodeCenterAfterLayout(ui.node), moveOver));
                CORRADE_COMPARE(ui.currentHoveredNode(), ui.node);
                CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
                CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
            }
        }

        Int stateIndex = 0;

        /* Pointer leave (...and later enter) on the hovered widget */
        if(flags >= Flag::Hovered) {
            TestUserInterface& ui = uis[style*stateCount + ++stateIndex];

            /* Move out, making the node inactive, i.e. looking the same as in
               the first column */
            if(ui.node != NodeHandle::Null) {
                PointerMoveEvent moveOut{now, PointerEventSource::Mouse, {}, {}, true, 0, {}};
                CORRADE_VERIFY(!ui.pointerMoveEvent({}, moveOut));
                CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
                CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
                CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
            }
        }

        /* Pointer leave (... and later enter) on the pressed + hovered
           widget */
        if(flags >= Flag::HoveredPressed) {
            /* The focusable widgets currently ignore hover if pressed or
               focused, so it's either this branch or the one below */
            CORRADE_INTERNAL_ASSERT(!(flags >= Flag::Focused));

            TestUserInterface& ui = uis[style*stateCount + ++stateIndex];

            /* Making the node pressed but not hovered, i.e. looking the same
               as in the fourth column. As the node is captured, the event is
               accepted always */
            if(ui.node != NodeHandle::Null) {
                PointerMoveEvent moveOut{now, PointerEventSource::Mouse, {}, {}, true, 0, {}};
                CORRADE_VERIFY(ui.pointerMoveEvent({}, moveOut));
                CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
                CORRADE_COMPARE(ui.currentPressedNode(), ui.node);
                CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
            }
        }

        /* Press (... and later release) on the focused widget */
        if(flags >= Flag::Focused) {
            /* The focusable widgets currently ignore hover if pressed or
               focused, so it's either this branch or the one above */
            CORRADE_INTERNAL_ASSERT(!(flags >= Flag::HoveredPressed));

            TestUserInterface& ui = uis[style*stateCount + ++stateIndex];

            /* Making the node focused and pressed, i.e.  looking the same as
               in the third column */
            if(ui.node != NodeHandle::Null) {
                PointerEvent press{now, PointerEventSource::Pen, Pointer::Pen, true, 0, {}};
                CORRADE_VERIFY(ui.pointerPressEvent(ui.nodeCenterAfterLayout(ui.node), press));
                CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
                CORRADE_COMPARE(ui.currentPressedNode(), ui.node);
                CORRADE_COMPARE(ui.currentFocusedNode(), ui.node);
            }
        }

        /* Pointer enter (... and later leave) on the pressed widget */
        if(flags >= Flag::HoveredPressed) {
            /* The focusable widgets currently ignore hover if pressed or
               focused, so it's either this branch or the one below */
            CORRADE_INTERNAL_ASSERT(!(flags >= Flag::Focused));

            TestUserInterface& ui = uis[style*stateCount + ++stateIndex];

            /* Making the node pressed + hovered, i.e. looking the same as
                in the third column */
            if(ui.node != NodeHandle::Null) {
                PointerMoveEvent moveOver{now, PointerEventSource::Mouse, {}, {}, true, 0, {}};
                CORRADE_VERIFY(ui.pointerMoveEvent(ui.nodeCenterAfterLayout(ui.node), moveOver));
                CORRADE_COMPARE(ui.currentHoveredNode(), ui.node);
                CORRADE_COMPARE(ui.currentPressedNode(), ui.node);
                CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
            }
        }

        /* Release (... and later press) on the focused + pressed widget, or
           on the widget that doesn't react to press + hover */
        if(flags >= Flag::Focused || (!(flags >= Flag::HoveredPressed) && flags >= Flag::Pressed)) {
            /* The focusable widgets currently ignore hover if pressed or
               focused, so it's either this branch or the one above */
            CORRADE_INTERNAL_ASSERT(!(flags >= Flag::HoveredPressed));

            TestUserInterface& ui = uis[style*stateCount + ++stateIndex];

            /* Release, making the node focused but not pressed, i.e. looking
               the same as in the fourth column. */
            if(ui.node != NodeHandle::Null) {
                PointerEvent release{now, PointerEventSource::Pen, Pointer::Pen, true, 0, {}};
                CORRADE_VERIFY(ui.pointerReleaseEvent({}, release));
                CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
                CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
                CORRADE_COMPARE(ui.currentFocusedNode(),
                    flags >= Flag::Focused ? ui.node : NodeHandle::Null);
            }
        }

        /* We don't do any change to the disabled state, so it's one state
           less */
        CORRADE_INTERNAL_ASSERT(stateIndex + 1 == stateCount - 1);
    }

    /* Advance animations to perform the style changes. If time delta is set to
       nothing, we don't expect any animations and thus don't need to
       advance. */
    if(delta != Nanoseconds{}) for(UserInterfaceGL& ui: uis)
        ui.advanceAnimations(now + delta);
    now += delta;

    for(Int style = 0; style != styleCount; ++style) {
        /* Pointer leave (after previous enter) on the inactive widget (which is
           always the first one). Do it only if the hovered widget (which is
           always at index 1) is actually there, otherwise we may not have a
           node to hover. */
        if(flags >= Flag::Hovered) {
            TestUserInterface& ui = uis[style*stateCount + 0];
            if(uis[style*stateCount + 1].node != NodeHandle::Null) {
                PointerMoveEvent moveOut{now, PointerEventSource::Mouse, {}, {}, true, 0, {}};
                CORRADE_VERIFY(!ui.pointerMoveEvent({}, moveOut));
                CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
                CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
                CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
            }
        }

        Int stateIndex = 0;

        /* Pointer enter (after previous leave) on the hovered widget */
        if(flags >= Flag::Hovered) {
            TestUserInterface& ui = uis[style*stateCount + ++stateIndex];
            if(ui.node != NodeHandle::Null) {
                PointerMoveEvent moveOver{now, PointerEventSource::Mouse, {}, {}, true, 0, {}};
                CORRADE_VERIFY(ui.pointerMoveEvent(ui.nodeCenterAfterLayout(ui.node), moveOver));
                CORRADE_COMPARE(ui.currentHoveredNode(), ui.node);
                CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
                CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
            }
        }

        /* Pointer enter (after previous leave) on the pressed + hovered
           widget */
        if(flags >= Flag::HoveredPressed) {
            /* The focusable widgets currently ignore hover if pressed or
               focused, so it's either this branch or the one below */
            CORRADE_INTERNAL_ASSERT(!(flags >= Flag::Focused));

            TestUserInterface& ui = uis[style*stateCount + ++stateIndex];
            if(ui.node != NodeHandle::Null) {
                PointerMoveEvent moveOver{now, PointerEventSource::Mouse, {}, {}, true, 0, {}};
                CORRADE_VERIFY(ui.pointerMoveEvent(ui.nodeCenterAfterLayout(ui.node), moveOver));
                CORRADE_COMPARE(ui.currentHoveredNode(), ui.node);
                CORRADE_COMPARE(ui.currentPressedNode(), ui.node);
                CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
            }
        }

        /* Release (after previous press) on the focused widget */
        if(flags >= Flag::Focused) {
            /* The focusable widgets currently ignore hover if pressed or
               focused, so it's either this branch or the one above */
            CORRADE_INTERNAL_ASSERT(!(flags >= Flag::HoveredPressed));

            TestUserInterface& ui = uis[style*stateCount + ++stateIndex];
            if(ui.node != NodeHandle::Null) {
                PointerEvent release{now, PointerEventSource::Pen, Pointer::Pen, true, 0, {}};
                CORRADE_VERIFY(ui.pointerReleaseEvent({}, release));
                CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
                CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
                CORRADE_COMPARE(ui.currentFocusedNode(), ui.node);
            }
        }

        /* Pointer leave (after previous enter) on the pressed widget */
        if(flags >= Flag::HoveredPressed) {
            /* The focusable widgets currently ignore hover if pressed or
               focused, so it's either this branch or the one below */
            CORRADE_INTERNAL_ASSERT(!(flags >= Flag::Focused));

            TestUserInterface& ui = uis[style*stateCount + ++stateIndex];

            /* As the node is captured, the event is accepted always */
            if(ui.node != NodeHandle::Null) {
                PointerMoveEvent moveOut{now, PointerEventSource::Mouse, {}, {}, true, 0, {}};
                CORRADE_VERIFY(ui.pointerMoveEvent({}, moveOut));
                CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
                CORRADE_COMPARE(ui.currentPressedNode(), ui.node);
                CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
            }
        }

        /* Press (after previous release) on the focused + pressed widget, or
           on the widget that doesn't react to press + hover */
        if(flags >= Flag::Focused || (!(flags >= Flag::HoveredPressed) && flags >= Flag::Pressed)) {
            /* The focusable widgets currently ignore hover if pressed or
               focused, so it's either this branch or the one above */
            CORRADE_INTERNAL_ASSERT(!(flags >= Flag::HoveredPressed));

            TestUserInterface& ui = uis[style*stateCount + ++stateIndex];
            if(ui.node != NodeHandle::Null) {
                PointerEvent press{now, PointerEventSource::Pen, Pointer::Pen, true, 0, {}};
                CORRADE_VERIFY(ui.pointerPressEvent(ui.nodeCenterAfterLayout(ui.node), press));
                CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
                CORRADE_COMPARE(ui.currentPressedNode(), ui.node);
                CORRADE_COMPARE(ui.currentFocusedNode(),
                    flags >= Flag::Focused ? ui.node : NodeHandle::Null);
            }
        }

        /* We don't do any change to the disabled state, so it's one state
           less */
        CORRADE_INTERNAL_ASSERT(stateIndex + 1 == stateCount - 1);
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

    /* If there is a HoveredPressed state and there's no Focused state, we have
       no no-op tests left to do */
    if(flags >= Flag::HoveredPressed && !(flags >= Flag::Focused))
        return;

    /* Verify that hovering the pressed widgets that don't handle press + hover
       causes no difference in visuals */
    if(flags >= (Flag::Hovered|Flag::Pressed) && !(flags >= Flag::HoveredPressed)) {
        for(Int style = 0; style != styleCount; ++style) {
            /* 0 is inactive, 1 is hovered, 2 is pressed (or focused) in this
               case */
            TestUserInterface& ui = uis[style*stateCount + 2];
            if(ui.node != NodeHandle::Null) {
                PointerMoveEvent moveOver{now, PointerEventSource::Mouse, {}, {}, true, 0, {}};
                CORRADE_VERIFY(ui.pointerMoveEvent(ui.nodeCenterAfterLayout(ui.node), moveOver));
                CORRADE_COMPARE(ui.currentHoveredNode(), ui.node);
                CORRADE_COMPARE(ui.currentPressedNode(),
                    flags >= Flag::Focused ? NodeHandle::Null : ui.node);
                CORRADE_COMPARE(ui.currentFocusedNode(),
                    flags >= Flag::Focused ? ui.node : NodeHandle::Null);
            }
        }
    }

    /* Verify that hovering the focused widgets (which don't handle focus +
       hover) doesn't have any difference in visuals */
    if(flags >= Flag::Focused) {
        /* Focusable widgets currently ignore hover if pressed or focused */
        CORRADE_INTERNAL_ASSERT(!(flags >= Flag::HoveredPressed));

        for(Int style = 0; style != styleCount; ++style) {
            /* 0 is inactive, 1 is hovered, 2 is focused, 3 is pressed +
               focused in this case */
            TestUserInterface& ui = uis[style*stateCount + 3];
            if(ui.node != NodeHandle::Null) {
                PointerMoveEvent moveOver{now, PointerEventSource::Mouse, {}, {}, true, 0, {}};
                CORRADE_VERIFY(ui.pointerMoveEvent(ui.nodeCenterAfterLayout(ui.node), moveOver));
                CORRADE_COMPARE(ui.currentHoveredNode(), ui.node);
                CORRADE_COMPARE(ui.currentPressedNode(), ui.node);
                CORRADE_COMPARE(ui.currentFocusedNode(), ui.node);
            }
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

}}}}

#endif
