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

struct UiSnapLayouter: Platform::WindowlessApplication {
    explicit UiSnapLayouter(const Arguments& arguments): Platform::WindowlessApplication{arguments} {}

    int exec() override;
};

constexpr Vector2i ImageSize{510, 260};

/** @todo ffs, duplicated six times, make a batch utility in Magnum */
Image2D unpremultiply(Image2D image) {
    for(Containers::StridedArrayView1D<Color4ub> row: image.pixels<Color4ub>())
        for(Color4ub& pixel: row)
            pixel = pixel.unpremultiplied();

    return image;
}

int UiSnapLayouter::exec() {
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::One, GL::Renderer::BlendFunction::OneMinusSourceAlpha);

    PluginManager::Manager<Trade::AbstractImageConverter> converterManager;
    Containers::Pointer<Trade::AbstractImageConverter> converter = converterManager.loadAndInstantiate("AnyImageConverter");
    if(!converter)
        return 1;

    Ui::AbstractUserInterface ui{ImageSize};
    Ui::RendererGL& renderer = ui.setRendererInstance(Containers::pointer<Ui::RendererGL>(Ui::RendererGL::Flag::CompositingFramebuffer));

    /* Inflate the node capacity so we have a consistent colormap for the node
       highlight in all cases below. Just allocate and then remove all nodes,
       assuming their handles are trivial like this. */
    /** @todo clean up once some reserveNodeCapacity() is a thing? */
    constexpr UnsignedInt nodeCapacity = 8;
    for(UnsignedInt i = 0; i != nodeCapacity; ++i)
        ui.createNode({}, {});
    for(UnsignedInt i = 0; i != nodeCapacity; ++i)
        ui.removeNode(Ui::nodeHandle(i, 1));
    CORRADE_INTERNAL_ASSERT(ui.nodeUsedCount() == 0);

    Ui::DebugLayer& debugLayer = ui.setLayerInstance(Containers::pointer<Ui::DebugLayerGL>(ui.createLayer(), Ui::DebugLayerSource::Nodes|Ui::DebugLayerSource::Layouters, Ui::DebugLayerFlags{}));
    debugLayer.setNodeHighlightColorMap(DebugTools::ColorMap::turbo(), 0.5f);

    Ui::SnapLayouter& layouter = ui.setLayouterInstance(Containers::pointer<Ui::SnapLayouter>(ui.createLayouter()));

/* [add] */
Ui::NodeHandle popupNode = ui.createNode({}, {400, 250});
Ui::NodeHandle acceptNode = ui.createNode(popupNode, {-10, -10}, {150, 50});
Ui::NodeHandle rejectNode = ui.createNode(popupNode, {}, {150, 50});

Ui::LayoutHandle popup = layouter.addExplicit(popupNode,
                                        Ui::Snaps{}, Ui::LayoutHandle::Null);
Ui::LayoutHandle accept = layouter.addExplicit(acceptNode,
                                        Ui::Snap::BottomRight, popup);
Ui::LayoutHandle reject = layouter.addExplicit(rejectNode, Ui::Snap::Left, accept);
/* [add] */
    static_cast<void>(reject);

    /* Make DebugLayer aware of all nodes and highlight nodes that have a
       layout assigned */
    ui.update();
    debugLayer.highlightNodes(layouter, [](const Ui::SnapLayouter&, Ui::LayouterDataHandle) {
        return true;
    });

    renderer.compositingFramebuffer().clearColor(0, 0x00000000_rgbaf);
    ui.draw();
    converter->convertToFile(unpremultiply(renderer.compositingFramebuffer().read(Range2Di::fromSize({50, 0}, {410, 260}), {PixelFormat::RGBA8Unorm})), "ui-snaplayouter-add.png");

/* [implicit-snap] */
Ui::NodeHandle contentsNode = ui.createNode(popupNode, {}, {});
Ui::NodeHandle titleNode = ui.createNode(contentsNode, {}, {300, 30});
Ui::NodeHandle detailsNode = ui.createNode(contentsNode, {}, {300, 40});

Ui::LayoutHandle contents = layouter.addExplicit(contentsNode, Ui::Snaps{}, popup);
layouter.setChildSnap(contents, Ui::Snap::Bottom|Ui::Snap::FillX);

/* Implicit child layouts */
Ui::LayoutHandle title = layouter.add(titleNode);
Ui::LayoutHandle details = layouter.add(detailsNode);
/* [implicit-snap] */
    static_cast<void>(contents);
    static_cast<void>(title);

    ui.update();
    debugLayer.highlightNodes(layouter, [](const Ui::SnapLayouter&, Ui::LayouterDataHandle) {
        return true;
    });

    renderer.compositingFramebuffer().clearColor(0, 0x00000000_rgbaf);
    ui.draw();
    converter->convertToFile(unpremultiply(renderer.compositingFramebuffer().read(Range2Di::fromSize({50, 0}, {410, 260}), {PixelFormat::RGBA8Unorm})), "ui-snaplayouter-implicit-snap.png");

/* [implicit-snap-insert] */
Ui::NodeHandle subtitleNode = ui.createNode(contentsNode, {}, {300, 20});
Ui::LayoutHandle subtitle = layouter.add(subtitleNode, /*before*/ details);
/* [implicit-snap-insert] */
    static_cast<void>(subtitle);

    ui.update();
    debugLayer.highlightNodes(layouter, [](const Ui::SnapLayouter&, Ui::LayouterDataHandle) {
        return true;
    });

    renderer.compositingFramebuffer().clearColor(0, 0x00000000_rgbaf);
    ui.draw();
    converter->convertToFile(unpremultiply(renderer.compositingFramebuffer().read(Range2Di::fromSize({50, 0}, {410, 260}), {PixelFormat::RGBA8Unorm})), "ui-snaplayouter-implicit-snap-insert.png");

    ui.removeNode(popupNode);
    ui.update();

/* [nested-layouts] */
/* A root column layout */
Ui::NodeHandle rootNode = DOXYGEN_ELLIPSIS(ui.createNode({5, 5}, {}));
Ui::LayoutHandle root = layouter.add(rootNode);
layouter.setChildSnap(root, Ui::Snap::Bottom|Ui::Snap::FillX);

/* First row layout inside */
Ui::NodeHandle firstNode = ui.createNode(rootNode, {}, {});
Ui::LayoutHandle first = layouter.add(firstNode);
layouter.setChildSnap(first, Ui::Snap::Right);

/* Second row layout inside */
Ui::NodeHandle secondNode = ui.createNode(rootNode, {}, {});
Ui::LayoutHandle second = layouter.add(secondNode);
layouter.setChildSnap(second, Ui::Snap::Right);

/* Child nodes in both */
Ui::NodeHandle childNodes[]{
    ui.createNode(firstNode, {}, {200, 80}),
    ui.createNode(firstNode, {}, {200, 80}),
    ui.createNode(secondNode, {}, {140, 60}),
    ui.createNode(secondNode, {}, {140, 60}),
    ui.createNode(secondNode, {}, {140, 60}),
};
for(Ui::NodeHandle node: childNodes)
    layouter.add(node);
/* [nested-layouts] */

/* [nested-layouts-margins] */
Ui::LayoutLayer& layoutLayer = DOXYGEN_ELLIPSIS(ui.setLayerInstance(Containers::pointer<Ui::LayoutLayer>(ui.createLayer(), 1u)));
/* A single style with a 20-unit margin around the node */
layoutLayer.setStyle({}, {}, {}, {}, {Vector4{20.0f}});

/* Apply it to all children */
for(Ui::NodeHandle node: childNodes)
    layoutLayer.create(0, node);
/* [nested-layouts-margins] */
    ; /* GCC complains about suspicious indentation here otherwise, lol */

    ui.update();
    debugLayer.highlightNodes(layouter, [](const Ui::SnapLayouter&, Ui::LayouterDataHandle) {
        return true;
    });
    debugLayer.clearHighlightedNode(rootNode);

    renderer.compositingFramebuffer().clearColor(0, 0x00000000_rgbaf);
    ui.draw();
    converter->convertToFile(unpremultiply(renderer.compositingFramebuffer().read(Range2Di::fromSize({0, 30}, {510, 230}), {PixelFormat::RGBA8Unorm})), "ui-snaplayouter-nested-layouts.png");

/* [nested-layouts-margins-propagate] */
layouter.addFlags(first, Ui::SnapLayoutFlag::PropagateMargin);
layouter.addFlags(second, Ui::SnapLayoutFlag::PropagateMargin);
/* [nested-layouts-margins-propagate] */

    /* No new nodes to highlight in this case, so just redraw */

    renderer.compositingFramebuffer().clearColor(0, 0x00000000_rgbaf);
    ui.draw();
    converter->convertToFile(unpremultiply(renderer.compositingFramebuffer().read(Range2Di::fromSize({0, 50}, {510, 210}), {PixelFormat::RGBA8Unorm})), "ui-snaplayouter-nested-layouts-propagated.png");

    CORRADE_INTERNAL_ASSERT(ui.nodeCapacity() == nodeCapacity && ui.nodeUsedCount() == nodeCapacity);

    return 0;
}

}

MAGNUM_WINDOWLESSAPPLICATION_MAIN(UiSnapLayouter)
