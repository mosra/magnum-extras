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

#include <Corrade/Containers/Function.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/StringView.h>
#include <Corrade/PluginManager/Manager.h>
#include <Magnum/Image.h>
#include <Magnum/ImageView.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/DebugTools/ColorMap.h>
#include <Magnum/Math/Color.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/Renderer.h>
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
#include <Magnum/Trade/AbstractImageConverter.h>

#include "Magnum/Ui/AbstractUserInterface.h"
#include "Magnum/Ui/DebugLayerGL.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/LayoutLayer.h"
#include "Magnum/Ui/RendererGL.h"
#include "Magnum/Ui/SnapLayouter.h"

#define DOXYGEN_ELLIPSIS(...) __VA_ARGS__

using namespace Magnum;
using namespace Math::Literals;

namespace {

struct UiLayoutLayer: Platform::WindowlessApplication {
    explicit UiLayoutLayer(const Arguments& arguments): Platform::WindowlessApplication{arguments} {}

    int exec() override;
};

constexpr Vector2i ImageSize{550, 90};

/** @todo ffs, duplicated six times, make a batch utility in Magnum */
Image2D unpremultiply(Image2D image) {
    for(Containers::StridedArrayView1D<Color4ub> row: image.pixels<Color4ub>())
        for(Color4ub& pixel: row)
            pixel = pixel.unpremultiplied();

    return image;
}

int UiLayoutLayer::exec() {
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::One, GL::Renderer::BlendFunction::OneMinusSourceAlpha);

    PluginManager::Manager<Trade::AbstractImageConverter> converterManager;
    Containers::Pointer<Trade::AbstractImageConverter> converter = converterManager.loadAndInstantiate("AnyImageConverter");
    if(!converter)
        return 1;

    Ui::AbstractUserInterface ui{ImageSize};
    Ui::RendererGL& renderer = ui.setRendererInstance(Containers::pointer<Ui::RendererGL>(Ui::RendererGL::Flag::CompositingFramebuffer));

    /* Unlike in ui-snaplayouter.cpp here we produce just a single image, so
       no need to prereserve any node capacity to have consistent coloring
       among several images */

    Ui::DebugLayer& debugLayer = ui.setLayerInstance(Containers::pointer<Ui::DebugLayerGL>(ui.createLayer(), Ui::DebugLayerSource::Nodes|Ui::DebugLayerSource::Layouters, Ui::DebugLayerFlags{}));
    debugLayer.setNodeHighlightColorMap(DebugTools::ColorMap::turbo(), 0.5f);

/* [setup] */
Ui::LayoutLayer& layer = ui.setLayerInstance(
    Containers::pointer<Ui::LayoutLayer>(ui.createLayer(), 3u));
/* [setup] */

/* [setup-style] */
enum class LayoutStyle {
    Menu,
    MenuIcon,
    MenuEntry,
    Count
};

struct Style {
    Vector2 minSize;
    Vector4 padding;
    Vector4 margin;
} styles[Int(LayoutStyle::Count)];
/* What isn't set stays at the default (zero) value */
styles[Int(LayoutStyle::Menu)].padding = {25.0f, 10.0f, 25.0f, 10.0f};
styles[Int(LayoutStyle::MenuEntry)].minSize = {100.0f, 40.0f};
styles[Int(LayoutStyle::MenuIcon)].minSize = {60.0f, 60.0f};
styles[Int(LayoutStyle::MenuEntry)].margin = Vector4{5.0f};
styles[Int(LayoutStyle::MenuIcon)].margin = Vector4{5.0f};

layer.setStyle(
    Containers::stridedArrayView(styles).slice(&Style::minSize),
    {}, /* max sizes not specified */
    {}, /* aspect ratios not specified */
    Containers::stridedArrayView(styles).slice(&Style::padding),
    Containers::stridedArrayView(styles).slice(&Style::margin));
/* [setup-style] */

/* [create] */
Ui::NodeHandle menuNode = ui.createNode({}, {});
layer.create(LayoutStyle::Menu, menuNode);

Ui::NodeHandle iconNode1 = ui.createNode(menuNode, {}, {});
layer.create(LayoutStyle::MenuIcon, iconNode1);

Ui::NodeHandle entryNode1 = ui.createNode(menuNode, {}, {});
layer.create(LayoutStyle::MenuEntry, entryNode1);

/* An entry that's deliberately wider */
Ui::NodeHandle entryNode2 = ui.createNode(menuNode, {}, {150, 0});
layer.create(LayoutStyle::MenuEntry, entryNode2);

Ui::NodeHandle iconNode2 = ui.createNode(menuNode, {}, {});
layer.create(LayoutStyle::MenuIcon, iconNode2);

Ui::NodeHandle entryNode3 = ui.createNode(menuNode, {}, {});
layer.create(LayoutStyle::MenuEntry, entryNode3);
/* [create] */

    /* Move the layout so it has some extra padding in the output */
    ui.setNodeOffset(menuNode, {5, 5});

/* [create-layout] */
Ui::SnapLayouter& layouter = DOXYGEN_ELLIPSIS(ui.setLayouterInstance(Containers::pointer<Ui::SnapLayouter>(ui.createLayouter())));

Ui::LayoutHandle menu = layouter.add(menuNode);
layouter.setChildSnap(menu, Ui::Snap::Right);

for(Ui::NodeHandle i: {iconNode1, entryNode1, entryNode2, iconNode2, entryNode3})
    layouter.add(i);
/* [create-layout] */
    ; /* GCC complains about suspicious indentation here otherwise, lol */

    /* Make DebugLayer aware of all nodes and highlight nodes that have a
       layout assigned */
    ui.update();
    debugLayer.highlightNodes(layouter, [](const Ui::SnapLayouter&, Ui::LayouterDataHandle) {
        return true;
    });

    renderer.compositingFramebuffer().clearColor(0, 0x00000000_rgbaf);
    ui.draw();
    converter->convertToFile(unpremultiply(renderer.compositingFramebuffer().read(Range2Di::fromSize({}, ImageSize), {PixelFormat::RGBA8Unorm})), "ui-layoutlayer.png");

    return 0;
}

}

MAGNUM_WINDOWLESSAPPLICATION_MAIN(UiLayoutLayer)
