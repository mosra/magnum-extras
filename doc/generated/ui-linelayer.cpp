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

#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/StringView.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/Utility/Path.h>
#include <Magnum/Image.h>
#include <Magnum/ImageView.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/Renderer.h>
#ifdef CORRADE_TARGET_APPLE
#include <Magnum/Platform/WindowlessCglApplication.h>
#elif defined(CORRADE_TARGET_UNIX)
#include <Magnum/Platform/WindowlessGlxApplication.h>
#elif defined(CORRADE_TARGET_WINDOWS)
#include <Magnum/Platform/WindowlessWglApplication.h>
#else
#error no windowless application available on this platform
#endif
#include <Magnum/Primitives/Circle.h>
#include <Magnum/MeshTools/Transform.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/MeshData.h>
#include <Magnum/Trade/AbstractImageConverter.h>

#include "Magnum/Ui/AbstractUserInterface.h"
#include "Magnum/Ui/LineLayerGL.h"
#include "Magnum/Ui/RendererGL.h"

using namespace Magnum;
using namespace Magnum::Math::Literals;

namespace {

struct UiLineLayer: Platform::WindowlessApplication {
    explicit UiLineLayer(const Arguments& arguments): Platform::WindowlessApplication{arguments} {}

    int exec() override;
};

constexpr Vector2i ImageSize{512, 256};

/** @todo ffs, duplicated three times, make a batch utility in Magnum */
Image2D unpremultiply(Image2D image) {
    for(Containers::StridedArrayView1D<Color4ub> row: image.pixels<Color4ub>())
        for(Color4ub& pixel: row)
            pixel = pixel.unpremultiplied();

    return image;
}

int UiLineLayer::exec() {
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::One, GL::Renderer::BlendFunction::OneMinusSourceAlpha);

    /* The actual framebuffer size is 4x the UI size */
    Ui::AbstractUserInterface ui{{128.0f, 64.0f}, Vector2{ImageSize}, ImageSize};
    /* Using a compositing framebuffer because it's easier than setting up a
       custom framebuffer here */
    Ui::RendererGL& renderer = ui.setRendererInstance(Containers::pointer<Ui::RendererGL>(Ui::RendererGL::Flag::CompositingFramebuffer));

    PluginManager::Manager<Trade::AbstractImageConverter> converterManager;
    Containers::Pointer<Trade::AbstractImageConverter> converter = converterManager.loadAndInstantiate("AnyImageConverter");
    if(!converter)
        return 1;

    /* Images for the main style docs */
    Ui::LineLayerGL::Shared layerNoSmoothnessShared{Ui::LineLayerGL::Shared::Configuration{1}};
    layerNoSmoothnessShared.setStyle(
        Ui::LineLayerCommonStyleUniform{},
        {Ui::LineLayerStyleUniform{}
            .setColor(0x2f83cc_rgbf)
            .setWidth(2.0f)},
        {Ui::LineAlignment{}},
        {});
    Ui::LineLayer& layerNoSmoothness = ui.setLayerInstance(Containers::pointer<Ui::LineLayerGL>(ui.createLayer(), layerNoSmoothnessShared));

    {
        renderer.compositingFramebuffer().clearColor(0, 0x00000000_rgbaf);

        Ui::NodeHandle root = ui.createNode({}, {128, 64});
        layerNoSmoothness.create(0, {
            0, 1, 1, 2,
            3, 4, 4, 5, 5, 6, 6, 3,
            7, 7
        }, {
            {8.0f, -24.0f}, {8.0f, 24.0f}, {56.0f, 24.0f},
            {-56.0f, -24.0f}, {-8.0f, -24.0f}, {-8.0f, 24.0f}, {-56.0f, 24.0f},
            {56.0f, -24.0f}
        }, {}, root);
        ui.draw();
        ui.removeNode(root);
        converter->convertToFile(unpremultiply(renderer.compositingFramebuffer().read({{}, ImageSize}, {PixelFormat::RGBA8Unorm})), "ui-linelayer-create.png");
    }

    Ui::LineLayerGL::Shared layerShared{
        Ui::LineLayerGL::Shared::Configuration{7}
            .setCapStyle(Ui::LineCapStyle::Round)
    };
    layerShared.setStyle(
        Ui::LineLayerCommonStyleUniform{}
            .setSmoothness(1.0f),
        {Ui::LineLayerStyleUniform{} /* 0 */
            .setColor(0x2f83cc_rgbf),
         Ui::LineLayerStyleUniform{} /* 1 */
            .setColor(0xa5c9ea_rgbf)
            .setSmoothness(15.0f),
         Ui::LineLayerStyleUniform{} /* 2 */
            .setColor(0x2f83cc_rgbf)
            .setWidth(6.0f),
         Ui::LineLayerStyleUniform{} /* 3 */
            .setWidth(6.0f),
         Ui::LineLayerStyleUniform{} /* 4 */
            .setColor(0x2f83cc_rgbf)
            .setWidth(3.0f),
         Ui::LineLayerStyleUniform{} /* 5 */
            .setColor(0x292e32_rgbf)
            .setSmoothness(1.5f)
            .setWidth(12.0f),
         Ui::LineLayerStyleUniform{} /* 6 */
            .setColor(0xdcdcdc_rgbf)
            .setWidth(10.0f)},
        {Ui::LineAlignment{},
         Ui::LineAlignment{},
         Ui::LineAlignment::BottomRight,
         Ui::LineAlignment::BottomRight,
         Ui::LineAlignment{},
         Ui::LineAlignment{},
         Ui::LineAlignment{}},
        {{},
         {},
         Vector4{0.0f, 0.0f, 8.0f, 8.0f},
         Vector4{0.0f, 0.0f, 8.0f, 8.0f},
         {},
         {},
         {}});
    Ui::LineLayer& layer = ui.setLayerInstance(Containers::pointer<Ui::LineLayerGL>(ui.createLayer(), layerShared));

    {
        renderer.compositingFramebuffer().clearColor(0, 0x00000000_rgbaf);

        Ui::NodeHandle root = ui.createNode({}, {128, 64});
        Ui::NodeHandle circle = ui.createNode(root, {4, 4}, {56, 56});
        Ui::NodeHandle azure = ui.createNode(root, {68, 4}, {56, 56});
        layer.createLoop(0,
            /* How lazy can I be, eh */
            MeshTools::transform2D(Primitives::circle2DWireframe(32), Matrix3::scaling(Vector2{16.0f}))
                .attribute<Vector2>(Trade::MeshAttribute::Position),
                {}, circle);
        layer.createLoop(1,
            MeshTools::transform2D(Primitives::circle2DWireframe(24), Matrix3::scaling(Vector2{16.0f}))
                .attribute<Vector2>(Trade::MeshAttribute::Position),
                {}, azure);
        ui.draw();
        ui.removeNode(root);
        converter->convertToFile(unpremultiply(renderer.compositingFramebuffer().read({{}, ImageSize}, {PixelFormat::RGBA8Unorm})), "ui-linelayer-style-smoothness.png");
    } {
        renderer.compositingFramebuffer().clearColor(0, 0x00000000_rgbaf);

        Ui::NodeHandle root = ui.createNode({}, {128, 32});
        Ui::NodeHandle blue = ui.createNode(root, {4, 4}, {24, 24});
        Ui::NodeHandle colored = ui.createNode(root, {36, 4}, {24, 24});
        Ui::NodeHandle opacity = ui.createNode(root, {68, 4}, {24, 24});
        ui.setNodeOpacity(opacity, 0.25f);
        Ui::NodeHandle gradient = ui.createNode(root, {100, 4}, {24, 24});
        layer.createStrip(2,
            MeshTools::transform2D(Primitives::circle2DWireframe(32), Matrix3::scaling(Vector2{12.0f}))
                .attribute<Vector2>(Trade::MeshAttribute::Position).slice(15, 28),
                {}, blue);
        Ui::DataHandle coloredData = layer.createStrip(3,
            MeshTools::transform2D(Primitives::circle2DWireframe(32), Matrix3::scaling(Vector2{12.0f}))
                .attribute<Vector2>(Trade::MeshAttribute::Position).slice(15, 28),
                {}, colored);
        layer.setColor(coloredData, 0x3bd267_rgbf);
        layer.createStrip(2,
            MeshTools::transform2D(Primitives::circle2DWireframe(32), Matrix3::scaling(Vector2{12.0f}))
                .attribute<Vector2>(Trade::MeshAttribute::Position).slice(15, 28),
                {}, opacity);
        Color3 colors[]{
            0xcd3431_rgbf,
            0xc7cf2f_rgbf,
            0x3bd267_rgbf,
            0x2f83cc_rgbf
        };
        layer.createStrip(3,
            MeshTools::transform2D(Primitives::circle2DWireframe(32), Matrix3::scaling(Vector2{12.0f}))
                .attribute<Vector2>(Trade::MeshAttribute::Position).slice(15, 28),
            Containers::arrayView<Color4>({
                colors[0],
                Math::lerp(colors[0], colors[1], 0.25f),
                Math::lerp(colors[0], colors[1], 0.50f),
                Math::lerp(colors[0], colors[1], 0.75f),
                colors[1],
                Math::lerp(colors[1], colors[2], 0.25f),
                Math::lerp(colors[1], colors[2], 0.50f),
                Math::lerp(colors[1], colors[2], 0.75f),
                colors[2],
                Math::lerp(colors[2], colors[3], 0.25f),
                Math::lerp(colors[2], colors[3], 0.50f),
                Math::lerp(colors[2], colors[3], 0.75f),
                colors[3],
            }), gradient);
        ui.draw();
        ui.removeNode(root);
        converter->convertToFile(unpremultiply(renderer.compositingFramebuffer().read({{0, 128}, ImageSize}, {PixelFormat::RGBA8Unorm})), "ui-linelayer-style-color.png");
    } {
        renderer.compositingFramebuffer().clearColor(0, 0x00000000_rgbaf);

        Ui::NodeHandle root = ui.createNode({}, {64, 64});
        Ui::NodeHandle circle = ui.createNode(root, {4, 4}, {56, 56});
        Ui::NodeHandle pointOuter = ui.createNode(circle, {}, {56, 56});
        Ui::NodeHandle point = ui.createNode(pointOuter, {}, {56, 56});
        layer.createLoop(4,
            /* How lazy can I be, eh */
            MeshTools::transform2D(Primitives::circle2DWireframe(64), Matrix3::scaling(Vector2{28.0f}))
                .attribute<Vector2>(Trade::MeshAttribute::Position),
                {}, circle);
        Vector2 data[]{
            -Matrix3::rotation(-37.0_degf).up()*28.0f,
        };
        layer.createLoop(5, data, {}, pointOuter);
        layer.createLoop(6, data, {}, point);
        ui.draw();
        ui.removeNode(root);
        converter->convertToFile(unpremultiply(renderer.compositingFramebuffer().read({{}, {256, 256}}, {PixelFormat::RGBA8Unorm})), "ui-linelayer-style-outline.png");
    }

    return 0;
}

}

MAGNUM_WINDOWLESSAPPLICATION_MAIN(UiLineLayer)
