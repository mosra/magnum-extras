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
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/String.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/Utility/Path.h>
#include <Magnum/Image.h>
#include <Magnum/ImageView.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/Animation/Easing.h>
#ifdef MAGNUM_TARGET_EGL
#include <Magnum/Platform/WindowlessEglApplication.h>
#elif defined(CORRADE_TARGET_APPLE)
#include <Magnum/Platform/WindowlessCglApplication.h>
#elif defined(CORRADE_TARGET_UNIX)
#include <Magnum/Platform/WindowlessGlxApplication.h>
#elif defined(CORRADE_TARGET_WINDOWS)
#include <Magnum/Platform/WindowlessWglApplication.h>
#else
#error no windowless application available on this platform
#endif
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Time.h>
#include <Magnum/Text/GlyphCacheGL.h>
#include <Magnum/Trade/AbstractImageConverter.h>

#include "Magnum/Ui/Anchor.h"
#include "Magnum/Ui/Button.h"
#include "Magnum/Ui/BaseLayerGL.h"
#include "Magnum/Ui/TextLayer.h"
#include "Magnum/Ui/TextProperties.h"
#include "Magnum/Ui/EventLayer.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/LineLayerGL.h"
#include "Magnum/Ui/NodeAnimator.h"
#include "Magnum/Ui/NodeFlags.h"
#include "Magnum/Ui/RendererGL.h"
#include "Magnum/Ui/SnapLayouter.h"
#include "Magnum/Ui/Style.h"
#include "Magnum/Ui/TextLayerGL.h"
#include "Magnum/Ui/UserInterfaceGL.h"
#include "Magnum/Ui/DebugLayerGL.h"

#define DOXYGEN_ELLIPSIS(...) __VA_ARGS__

using namespace Magnum;
using namespace Containers::Literals;
using namespace Math::Literals;

namespace {

struct UiDebugLayer: Platform::WindowlessApplication {
    explicit UiDebugLayer(const Arguments& arguments): Platform::WindowlessApplication{arguments} {}

    int exec() override;
};

constexpr Vector2i ImageSize{256, 96};

/* [integration] */
class ColorLayer: public Ui::AbstractLayer {
    public:
        struct DebugIntegration;

        Color3 color(Ui::LayerDataHandle handle) const;

        DOXYGEN_ELLIPSIS(
            using Ui::AbstractLayer::AbstractLayer;
            using Ui::AbstractLayer::create;
            using Ui::AbstractLayer::remove;

            Ui::LayerFeatures doFeatures() const override { return {}; }
        )
};

struct ColorLayer::DebugIntegration {
    void print(Debug& debug, const ColorLayer& layer,
               Containers::StringView layerName, Ui::LayerDataHandle data) {
        /* Convert to an 8-bit color for brevity */
        Color3ub color8 = Math::pack<Color3ub>(layer.color(data));
        debug << "  Data" << Debug::packed << data
            << "from layer" << Debug::packed << layer.handle()
            << Debug::color(Debug::Color::Yellow) << layerName << Debug::resetColor
            << "with color";
        /* If colors aren't disabled, print also a color swatch besides the
           actual value. All other coloring will be automatically ignored if
           DisableColors is set. */
        if(!(debug.flags() & Debug::Flag::DisableColors))
            debug << Debug::color << color8;
        debug << color8 << Debug::newline;
    }
};
/* [integration] */

/* Used by abstractvisuallayer-style-names, needs to be defined here */
enum Style {
    Button = 13,
    ButtonPressed = 7,
    ButtonHovered = 11,
    ButtonPressedHovered = 3,
};
struct Transition {
    Style inactiveOut;
    Style inactiveOver;
    Style pressedOut;
    Style pressedOver;
};
Transition transition(Style style) {
    switch(style) {
        case Style::Button:
        case Style::ButtonHovered:
        case Style::ButtonPressed:
        case Style::ButtonPressedHovered:
            return {Style::Button,
                    Style::ButtonHovered,
                    Style::ButtonPressed,
                    Style::ButtonPressedHovered};
        default:
            return {style, style, style, style};
    }
}
template<Style Transition::*member> Style to(Style style) {
    return transition(style).*member;
}

Color3 ColorLayer::color(Ui::LayerDataHandle data) const {
    return layerDataHandleId(data) == 7 ? 0x3bd267_rgbf : 0x2f83cc_rgbf;
}

/** @todo ffs, duplicated three times, make a batch utility in Magnum */
Image2D unpremultiply(Image2D image) {
    for(Containers::StridedArrayView1D<Color4ub> row: image.pixels<Color4ub>())
        for(Color4ub& pixel: row)
            pixel = pixel.unpremultiplied();

    return image;
}

int UiDebugLayer::exec() {
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::One, GL::Renderer::BlendFunction::OneMinusSourceAlpha);

    PluginManager::Manager<Trade::AbstractImageConverter> converterManager;
    Containers::Pointer<Trade::AbstractImageConverter> converter = converterManager.loadAndInstantiate("AnyImageConverter");
    if(!converter)
        return 1;

    Ui::UserInterfaceGL ui{NoCreate};
    /* Using a compositing framebuffer because it's easier than setting up a
       custom framebuffer here */
    ui
        /** @todo uh, can't setting renderer flags be doable in some more
            intuitive way? such as flags on the style? */
        .setRendererInstance(Containers::pointer<Ui::RendererGL>(Ui::RendererGL::Flag::CompositingFramebuffer))
        /* The actual framebuffer size is 2x the UI size */
        .setSize({128.0f, 48.0f}, Vector2{ImageSize}, ImageSize)
        .setStyle(Ui::McssDarkStyle{});

    Ui::DebugLayer& debugLayerHierarchy = ui.setLayerInstance(Containers::pointer<Ui::DebugLayerGL>(ui.createLayer(), Ui::DebugLayerSource::NodeDataDetails|Ui::DebugLayerSource::NodeHierarchy, Ui::DebugLayerFlag::NodeHighlight|Ui::DebugLayerFlag::ColorAlways));

    /* Button code, default visual state with no highlight. Adding some extra
       nodes and data to have the listed handles non-trivial. */
    ui.createNode({}, {});
    ui.createNode({}, {});
    ui.createNode({}, {});
    ui.createNode({}, {});
    Ui::NodeHandle root = Ui::snap(ui, Ui::Snap::Fill, {});
    ui.removeNode(ui.createNode({}, {}));
    Ui::NodeHandle hidden = Ui::snap(ui, Ui::Snap::Fill, {}, Ui::NodeFlag::Hidden);
    Ui::button(Ui::snap(ui, {}, hidden, {}), Ui::Icon::Yes, "Accept");
    Ui::button(Ui::snap(ui, {}, hidden, {}), Ui::Icon::Yes, "Accept");
    Ui::button(Ui::snap(ui, {}, hidden, {}), Ui::Icon::Yes, "Accept");
    /* Yeah this one deletes itself right away */
    Ui::Button{Ui::snap(ui, {}, hidden, {}), ""};
    ui.update();

/* [button] */
Ui::NodeHandle button = Ui::button(DOXYGEN_ELLIPSIS(Ui::snap(ui, {}, root, {112, 32})), Ui::Icon::Yes, "Accept");

ui.eventLayer().onTapOrClick(button, []{
    DOXYGEN_ELLIPSIS()
});
/* [button] */

    ui.renderer().compositingFramebuffer().clearColor(0, 0x00000000_rgbaf);
    ui.draw();
    converter->convertToFile(unpremultiply(ui.renderer().compositingFramebuffer().read({{}, ImageSize}, {PixelFormat::RGBA8Unorm})), "ui-debuglayer-node.png");

    /* Highlighted output and visual state */
    Containers::String out;
    {
        Debug redirectOutput{&out};
        CORRADE_INTERNAL_ASSERT(debugLayerHierarchy.highlightNode(button));
    }
    Debug{} << out;
    Utility::Path::write("ui-debuglayer-node-highlight.ansi", out);

    ui.renderer().compositingFramebuffer().clearColor(0, 0x00000000_rgbaf);
    ui.draw();
    converter->convertToFile(unpremultiply(ui.renderer().compositingFramebuffer().read({{}, ImageSize}, {PixelFormat::RGBA8Unorm})), "ui-debuglayer-node-highlight.png");

    /* Node and layer names. NodeDataDetails is enabled so casting to a base
       type to not have the integration picked yet */
    /** @todo once the integration does something even without NodeDataDetails
        being set (such as showing layer flags), this won't be enough and there
        needs to be multiple debug layers */
    debugLayerHierarchy.setLayerName(static_cast<Ui::AbstractLayer&>(ui.baseLayer()), "Base");
    debugLayerHierarchy.setLayerName(static_cast<Ui::AbstractLayer&>(ui.textLayer()), "Text");
    debugLayerHierarchy.setLayerName(static_cast<Ui::AbstractLayer&>(ui.eventLayer()), "Event");
    debugLayerHierarchy.setNodeName(button, "Accept button");

    out = {};
    {
        Debug redirectOutput{&out};
        CORRADE_INTERNAL_ASSERT(debugLayerHierarchy.highlightNode(button));
    }
    Debug{} << out;
    Utility::Path::write("ui-debuglayer-node-highlight-names.ansi", out);

    /* Layer data attachment details. Deliberately set in order that doesn't
       match the draw order, to hint that it doesn't matter. */

/* [button-names] */
debugLayerHierarchy.setLayerName(ui.eventLayer(), "Event");
debugLayerHierarchy.setLayerName(ui.baseLayer(), "Base");
/* So it doesn't show the (arbitrary) padding from TextLayer */
debugLayerHierarchy.setLayerName(static_cast<Ui::AbstractVisualLayer&>(ui.textLayer()), "Text");
debugLayerHierarchy.setNodeName(button, "Accept button");
/* [button-names] */

    out = {};
    {
        Debug redirectOutput{&out};
        CORRADE_INTERNAL_ASSERT(debugLayerHierarchy.highlightNode(button));
    }
    Debug{} << out;
    Utility::Path::write("ui-debuglayer-node-highlight-details.ansi", out);

    /* Custom integration, with a debug layer that has NodeHierarchy disabled
       as that information is superfluous. Creating some more nodes and unused
       data to not have the listed handles too close to each other. */
    ui.removeLayer(debugLayerHierarchy.handle());
    Ui::DebugLayer& debugLayer = ui.setLayerInstance(Containers::pointer<Ui::DebugLayerGL>(ui.createLayer(), Ui::DebugLayerSource::NodeDataDetails|Ui::DebugLayerSource::NodeAnimationDetails, Ui::DebugLayerFlag::NodeHighlight|Ui::DebugLayerFlag::ColorAlways));
    debugLayer.setLayerName(ui.eventLayer(), "Event");

/* [integration-setLayerName] */
ColorLayer& colorLayer = ui.setLayerInstance(DOXYGEN_ELLIPSIS(Containers::pointer<ColorLayer>(ui.createLayer())));
DOXYGEN_ELLIPSIS()

debugLayer.setLayerName(colorLayer, "Shiny");
/* [integration-setLayerName] */

    ui.createNode({}, {});
    Ui::NodeHandle parent = ui.createNode(root, {}, {});
    Ui::NodeHandle colorNode = ui.createNode(parent, {}, {});
    colorLayer.create();
    colorLayer.create();
    colorLayer.create();
    colorLayer.create();
    colorLayer.remove(colorLayer.create());
    colorLayer.remove(colorLayer.create());
    colorLayer.create(colorNode);
    colorLayer.create();
    colorLayer.create();
    colorLayer.create(colorNode);

    ui.update();
    out = {};
    {
        Debug redirectOutput{&out};
        CORRADE_INTERNAL_ASSERT(debugLayer.highlightNode(colorNode));
    }
    Debug{} << out;
    Utility::Path::write("ui-debuglayer-integration.ansi", out);

    /* AbstractVisualLayer integration, default behavior. Using a BaseLayer to
       not have to create an ad-hoc subclass. */
    Ui::BaseLayerGL::Shared baseLayerShared{
        Ui::BaseLayerGL::Shared::Configuration{17}
    };
    baseLayerShared.setStyleTransition<Style,
        to<&Transition::inactiveOut>,
        to<&Transition::inactiveOver>,
        to<&Transition::inactiveOut>,
        to<&Transition::inactiveOver>,
        to<&Transition::pressedOut>,
        to<&Transition::pressedOver>,
        nullptr>();
    baseLayerShared.setStyle({}, {
        {}, {}, {}, {}, {}, {}, {}, {},
        {}, {}, {}, {}, {}, {}, {}, {},
        {}
    }, {});

    Ui::BaseLayer& visualLayer = ui.setLayerInstance(Containers::pointer<Ui::BaseLayerGL>(ui.createLayer(), baseLayerShared));
    debugLayer.setLayerName(static_cast<Ui::AbstractVisualLayer&>(visualLayer), "Styled");

    ui.createNode({}, {});
    ui.createNode({}, {});
    Ui::NodeHandle baseNode = ui.createNode(parent, {}, {});
    visualLayer.create(0);
    visualLayer.create(0);
    visualLayer.create(0);
    visualLayer.create(0);
    visualLayer.create(Style::ButtonHovered, baseNode);

    ui.update();
    out = {};
    {
        Debug redirectOutput{&out};
        CORRADE_INTERNAL_ASSERT(debugLayer.highlightNode(baseNode));
    }
    Debug{} << out;
    Utility::Path::write("ui-debuglayer-abstractvisuallayer.ansi", out);

    /* AbstractVisualLayer integration with supplied style names */

/* [abstractvisuallayer-style-names] */
debugLayer.setLayerName(visualLayer, "Styled", [](UnsignedInt style) {
    using namespace Containers::Literals;

    switch(Style(style)) {
        case Style::Button: return "Button"_s;
        case Style::ButtonHovered: return "ButtonHovered"_s;
        case Style::ButtonPressed: return "ButtonPressed"_s;
        case Style::ButtonPressedHovered: return "ButtonPressedHovered"_s;
        DOXYGEN_ELLIPSIS()
    }

    return ""_s;
});
/* [abstractvisuallayer-style-names] */

    ui.update();
    out = {};
    {
        Debug redirectOutput{&out};
        CORRADE_INTERNAL_ASSERT(debugLayer.highlightNode(baseNode));
    }
    Debug{} << out;
    Utility::Path::write("ui-debuglayer-abstractvisuallayer-style-names.ansi", out);

    /* BaseLayer integration */
    Ui::BaseLayer& baseLayer = ui.setLayerInstance(Containers::pointer<Ui::BaseLayerGL>(ui.createLayer(), baseLayerShared));
    debugLayer.setLayerName(baseLayer, "Base", [](UnsignedInt style) {
        return style == 9 ? "ColorSwatch"_s : ""_s;
    });
    Ui::NodeHandle baseNodeCustom = ui.createNode(parent, {}, {});
    Ui::DataHandle baseDataCustom = baseLayer.create(9, baseNodeCustom);
    baseLayer.setColor(baseDataCustom, 0x3bd267_rgbf);
    baseLayer.setPadding(baseDataCustom, {2.0f, 4.0f, 1.0f, 3.0f});

    ui.update();
    out = {};
    {
        Debug redirectOutput{&out};
        CORRADE_INTERNAL_ASSERT(debugLayer.highlightNode(baseNodeCustom));
    }
    Debug{} << out;
    Utility::Path::write("ui-debuglayer-baselayer.ansi", out);

    /* LineLayer integration */
    Ui::LineLayerGL::Shared lineLayerShared{
        Ui::LineLayerGL::Shared::Configuration{4}
    };
    lineLayerShared.setStyle({}, {
        {}, {}, {}, {},
    }, {
        {}, {}, {}, {}
    }, {});
    Ui::LineLayer& lineLayer = ui.setLayerInstance(Containers::pointer<Ui::LineLayerGL>(ui.createLayer(), lineLayerShared));
    debugLayer.setLayerName(lineLayer, "Line", [](UnsignedInt style) {
        return style == 2 ? "Graph"_s : ""_s;
    });
    Ui::NodeHandle lineNodeCustom = ui.createNode(parent, {}, {});
    Ui::DataHandle lineDataCustom = lineLayer.createLoop(2, {{}}, {}, lineNodeCustom);
    lineLayer.setAlignment(lineDataCustom, Ui::LineAlignment::BottomLeft);
    lineLayer.setPadding(lineDataCustom, {3.0f, 1.0f, 4.0f, 2.0f});

    ui.update();
    out = {};
    {
        Debug redirectOutput{&out};
        CORRADE_INTERNAL_ASSERT(debugLayer.highlightNode(lineNodeCustom));
    }
    Debug{} << out;
    Utility::Path::write("ui-debuglayer-linelayer.ansi", out);

    /* TextLayer integration */
    Ui::TextLayerGL::Shared textLayerShared{
        Text::GlyphCacheArrayGL{PixelFormat::RGBA8Unorm, {256, 256, 1}},
        Ui::TextLayerGL::Shared::Configuration{4}
    };
    Ui::FontHandle font = textLayerShared.addInstancelessFont(textLayerShared.glyphCache().addFont(1), 1.0f);
    textLayerShared.setStyle({}, {
        {}, {}, {}, {},
    }, {
        {}, {}, {}, font
    }, {
        {}, {}, {}, {}
    }, {}, {}, {}, {}, {}, {});
    Ui::TextLayer& textLayer = ui.setLayerInstance(Containers::pointer<Ui::TextLayerGL>(ui.createLayer(), textLayerShared));
    debugLayer.setLayerName(textLayer, "Text", [](UnsignedInt style) {
        return style == 3 ? "Label"_s : ""_s;
    });

    Ui::NodeHandle textNodeCustom = ui.createNode(parent, {}, {});
    Ui::DataHandle textDataCustom = textLayer.createGlyph(3, 0, {}, textNodeCustom);
    textLayer.setColor(textDataCustom, 0x2f83cc_rgbf);
    textLayer.setPadding(textDataCustom, 4.5f);

    ui.update();
    out = {};
    {
        Debug redirectOutput{&out};
        CORRADE_INTERNAL_ASSERT(debugLayer.highlightNode(textNodeCustom));
    }
    Debug{} << out;
    Utility::Path::write("ui-debuglayer-textlayer.ansi", out);

    /* EventLayer integration */
    ui.createNode({}, {});
    ui.createNode({}, {});
    Ui::NodeHandle eventNode = ui.createNode(parent, {}, {});
    ui.eventLayer().onEnter(eventNode, []{});
    /* This one should show that it's allocated */
    char large[128]{};
    ui.eventLayer().onTapOrClick(eventNode, [large]{ Debug{} << large[0]; });

    ui.update();
    out = {};
    {
        Debug redirectOutput{&out};
        CORRADE_INTERNAL_ASSERT(debugLayer.highlightNode(eventNode));
    }
    Debug{} << out;
    Utility::Path::write("ui-debuglayer-eventlayer.ansi", out);

    /* NodeAnimator integration. Creating some more animators and animations to
       have non-trivial handles. */
    ui.createAnimator();
    ui.createAnimator();
    ui.removeAnimator(ui.createAnimator());
    Ui::NodeAnimator& nodeAnimator = ui.setNodeAnimatorInstance(Containers::pointer<Ui::NodeAnimator>(ui.createAnimator()));
    debugLayer.setAnimatorName(nodeAnimator, "Node");
    Ui::NodeHandle nodeAnimatedNode = ui.createNode(parent, {}, {});
    nodeAnimator.create(Ui::NodeAnimation{}, nullptr, -10_nsec, 20_nsec, Ui::NodeHandle::Null);
    nodeAnimator.create(Ui::NodeAnimation{}, nullptr, -10_nsec, 20_nsec, Ui::NodeHandle::Null);
    nodeAnimator.create(Ui::NodeAnimation{}, nullptr, -10_nsec, 20_nsec, Ui::NodeHandle::Null);
    nodeAnimator.create(Ui::NodeAnimation{}, nullptr, -10_nsec, 20_nsec, Ui::NodeHandle::Null);
    nodeAnimator.remove(nodeAnimator.create(Ui::NodeAnimation{}, nullptr, -10_nsec, 20_nsec, Ui::NodeHandle::Null));
    nodeAnimator.remove(nodeAnimator.create(Ui::NodeAnimation{}, nullptr, -10_nsec, 20_nsec, Ui::NodeHandle::Null));
    Ui::AnimationHandle nodeAnimatedNodeAnimation = nodeAnimator.create(
        Ui::NodeAnimation{}
            .toOffsetX(500.0f)
            .fromOpacity(0.0f)
            .toOpacity(1.0f)
            .addFlagsBegin(Ui::NodeFlag::NoEvents|Ui::NodeFlag::Clip)
            .clearFlagsBegin(Ui::NodeFlag::Hidden)
            .clearFlagsEnd(Ui::NodeFlag::NoEvents|Ui::NodeFlag::Clip),
        Animation::Easing::linear, -10_nsec, 20_nsec, nodeAnimatedNode);

    ui.update();
    out = {};
    {
        Debug redirectOutput{&out};
        CORRADE_INTERNAL_ASSERT(debugLayer.highlightNode(nodeAnimatedNode));
    }
    Debug{} << out;
    Utility::Path::write("ui-debuglayer-nodeanimator.ansi", out);

    nodeAnimator.addFlags(nodeAnimatedNodeAnimation, Ui::AnimationFlag::Reverse);

    out = {};
    {
        Debug redirectOutput{&out};
        CORRADE_INTERNAL_ASSERT(debugLayer.highlightNode(nodeAnimatedNode));
    }
    Debug{} << out;
    Utility::Path::write("ui-debuglayer-nodeanimator-reverse.ansi", out);

    return 0;
}

}

MAGNUM_WINDOWLESSAPPLICATION_MAIN(UiDebugLayer)
