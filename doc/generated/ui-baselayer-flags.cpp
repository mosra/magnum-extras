/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023, 2024
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
#include <Corrade/Utility/Configuration.h>
#include <Corrade/Utility/Path.h>
#include <Magnum/Image.h>
#include <Magnum/ImageView.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureArray.h>
#include <Magnum/GL/TextureFormat.h>
#ifdef CORRADE_TARGET_APPLE
#include <Magnum/Platform/WindowlessCglApplication.h>
#elif defined(CORRADE_TARGET_UNIX)
#include <Magnum/Platform/WindowlessGlxApplication.h>
#elif defined(CORRADE_TARGET_WINDOWS)
#include <Magnum/Platform/WindowlessWglApplication.h>
#else
#error no windowless application available on this platform
#endif
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/AbstractImageConverter.h>
#include <Magnum/Trade/ImageData.h>

#include "Magnum/Ui/AbstractUserInterface.h"
#include "Magnum/Ui/BaseLayerGL.h"
#include "Magnum/Ui/RendererGL.h"

using namespace Magnum;
using namespace Magnum::Math::Literals;

namespace {

struct UiBaseLayerFlags: Platform::WindowlessApplication {
    explicit UiBaseLayerFlags(const Arguments& arguments): Platform::WindowlessApplication{arguments} {}

    int exec() override;
};

constexpr Vector2i ImageSize{512, 256};

int UiBaseLayerFlags::exec() {
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::One, GL::Renderer::BlendFunction::OneMinusSourceAlpha);

    /* The actual framebuffer size is 4x the UI size */
    Ui::AbstractUserInterface ui{{128.0f, 64.0f}, Vector2{ImageSize}, ImageSize};
    Ui::RendererGL& renderer = ui.setRendererInstance(Containers::pointer<Ui::RendererGL>(Ui::RendererGL::Flag::CompositingFramebuffer));

    PluginManager::Manager<Trade::AbstractImporter> importerManager;
    /* Use the StbImageImporter so we can keep files small but always import
       them as four-channel */
    if(PluginManager::PluginMetadata* metadata = importerManager.metadata("StbImageImporter")) {
        metadata->configuration().setValue("forceChannelCount", 4);
        importerManager.setPreferredPlugins("PngImporter", {"StbImageImporter"});
    } else Fatal{} << "StbImageImporter not found";

    PluginManager::Manager<Trade::AbstractImageConverter> converterManager;
    Containers::Pointer<Trade::AbstractImporter> importer = importerManager.loadAndInstantiate("AnyImageImporter");
    Containers::Pointer<Trade::AbstractImageConverter> converter = converterManager.loadAndInstantiate("AnyImageConverter");
    if(!importer || !converter)
        return 1;

    if(!importer->openFile(Utility::Path::join(Utility::Path::split(__FILE__).first(), "balloon.jpg")))
        return 2;
    Containers::Optional<Trade::ImageData2D> backgroundImage = importer->image2D(0);
    CORRADE_INTERNAL_ASSERT(backgroundImage && backgroundImage->size() == ImageSize);

    /* Exported by Inkscape from mask.svg as (non-premultiplied) RGBA8, just
       the selection and 384 DPI to match the expected size here. Then
       processed with PngImporter and
        magnum-imageconverter --in-place -i alphaMode=premultipliedLinear mask-premultiplied.png
       to have the alpha channel equal to the RGB channels because we're not
       generally sRGB-aware yet, and then ultimately with
        pngcrush -ow mask-premultiplied.png
       which turns the RGBA8 to RG8 to save space, and which StbImageImporter
       will then expand back to RGBA8 on import. */
    if(!importer->openFile(Utility::Path::join(Utility::Path::split(__FILE__).first(), "mask-premultiplied.png")))
        return 2;
    Containers::Optional<Trade::ImageData2D> mask = importer->image2D(0);
    CORRADE_INTERNAL_ASSERT(mask && mask->format() == PixelFormat::RGBA8Unorm && mask->size() == (Vector2i{112*4, 48*4}));

    GL::Texture2DArray texture;
    texture
        .setMinificationFilter(GL::SamplerFilter::Linear)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setStorage(1, GL::textureFormat(mask->format()), {mask->size(), 1})
        .setSubImage(0, {}, ImageView2D{*mask});

    Ui::BaseLayerGL::Shared layerShared{Ui::BaseLayerGL::Shared::Configuration{1}};
    layerShared.setStyle(
        Ui::BaseLayerCommonStyleUniform{}
            .setSmoothness(1.0f),
        {Ui::BaseLayerStyleUniform{}
            .setCornerRadius(12.0f)
            .setColor(0xffffffff_rgbaf*0.667f)
            .setOutlineColor(0x00000000_rgbaf)},
        {});
    Ui::BaseLayerGL& layer = ui.setLayerInstance(Containers::pointer<Ui::BaseLayerGL>(ui.createLayer(), layerShared));

    Ui::BaseLayerGL::Shared layerSharedBackgroundBlur{Ui::BaseLayerGL::Shared::Configuration{1}
        .addFlags(Ui::BaseLayerSharedFlag::BackgroundBlur)
        .setBackgroundBlurRadius(31)};
    layerSharedBackgroundBlur.setStyle(
        Ui::BaseLayerCommonStyleUniform{}
            .setSmoothness(1.0f),
        {Ui::BaseLayerStyleUniform{}
            .setCornerRadius(12.0f)
            .setColor(0xffffffff_rgbaf*0.667f)
            .setOutlineColor(0x00000000_rgbaf)},
        {});
    Ui::BaseLayerGL& layerBackgroundBlur = ui.setLayerInstance(Containers::pointer<Ui::BaseLayerGL>(ui.createLayer(), layerSharedBackgroundBlur));
    layerBackgroundBlur
        .setBackgroundBlurPassCount(8);

    Ui::BaseLayerGL::Shared layerSharedBackgroundBlurAlpha{Ui::BaseLayerGL::Shared::Configuration{1}
        .addFlags(Ui::BaseLayerSharedFlag::BackgroundBlur)
        .setBackgroundBlurRadius(31)};
    layerSharedBackgroundBlurAlpha.setStyle(
        Ui::BaseLayerCommonStyleUniform{}
            .setSmoothness(1.0f)
            .setBackgroundBlurAlpha(0.75f),
        {Ui::BaseLayerStyleUniform{}
            .setCornerRadius(12.0f)
            .setColor(0xffffffff_rgbaf*0.667f)
            .setOutlineColor(0x00000000_rgbaf)},
        {});
    Ui::BaseLayerGL& layerBackgroundBlurAlpha = ui.setLayerInstance(Containers::pointer<Ui::BaseLayerGL>(ui.createLayer(), layerSharedBackgroundBlurAlpha));
    layerBackgroundBlurAlpha
        .setBackgroundBlurPassCount(8);

    Ui::BaseLayerGL::Shared layerSharedBackgroundBlurTextured{Ui::BaseLayerGL::Shared::Configuration{1}
        .addFlags(Ui::BaseLayerSharedFlag::BackgroundBlur|
                  Ui::BaseLayerSharedFlag::Textured)
        .setBackgroundBlurRadius(31)};
    layerSharedBackgroundBlurTextured.setStyle(
        Ui::BaseLayerCommonStyleUniform{}
            .setSmoothness(1.0f),
        {Ui::BaseLayerStyleUniform{}
            .setCornerRadius(12.0f)
            .setInnerOutlineCornerRadius(4.0f)
            .setOutlineWidth(8.0f)
            .setColor(0xffffffff_rgbaf*0.667f)
            .setOutlineColor(0x2f83ccff_rgbaf*0.667f)},
        {});
    Ui::BaseLayerGL& layerBackgroundBlurTextured = ui.setLayerInstance(Containers::pointer<Ui::BaseLayerGL>(ui.createLayer(), layerSharedBackgroundBlurTextured));
    layerBackgroundBlurTextured
        .setBackgroundBlurPassCount(8)
        .setTexture(texture);

    Ui::BaseLayerGL::Shared layerSharedBackgroundBlurTextureMask{Ui::BaseLayerGL::Shared::Configuration{1}
        .addFlags(Ui::BaseLayerSharedFlag::BackgroundBlur|
                  Ui::BaseLayerSharedFlag::TextureMask)
        .setBackgroundBlurRadius(31)};
    layerSharedBackgroundBlurTextureMask.setStyle(
        Ui::BaseLayerCommonStyleUniform{}
            .setSmoothness(1.0f),
        {Ui::BaseLayerStyleUniform{}
            .setCornerRadius(12.0f)
            .setInnerOutlineCornerRadius(4.0f)
            .setOutlineWidth(8.0f)
            .setColor(0xffffffff_rgbaf*0.667f)
            .setOutlineColor(0x2f83ccff_rgbaf*0.667f)},
        {});
    Ui::BaseLayerGL& layerBackgroundBlurTextureMask = ui.setLayerInstance(Containers::pointer<Ui::BaseLayerGL>(ui.createLayer(), layerSharedBackgroundBlurTextureMask));
    layerBackgroundBlurTextureMask
        .setBackgroundBlurPassCount(8)
        .setTexture(texture);

    {
        renderer.compositingTexture().setSubImage(0, {}, *backgroundImage);

        Ui::NodeHandle node = ui.createNode({8, 8}, {112, 48});
        layer.create(0, node);
        ui.draw();
        ui.removeNode(node);
        converter->convertToFile(renderer.compositingFramebuffer().read({{}, ImageSize}, {PixelFormat::RGBA8Unorm}), "ui-baselayer-default.png");
    } {
        renderer.compositingTexture().setSubImage(0, {}, *backgroundImage);

        Ui::NodeHandle node = ui.createNode({8, 8}, {112, 48});
        layerBackgroundBlur.create(0, node);
        ui.draw();
        ui.removeNode(node);
        converter->convertToFile(renderer.compositingFramebuffer().read({{}, ImageSize}, {PixelFormat::RGBA8Unorm}), "ui-baselayer-blur.png");
    }  {
        renderer.compositingTexture().setSubImage(0, {}, *backgroundImage);

        Ui::NodeHandle node = ui.createNode({8, 8}, {112, 48});
        layerBackgroundBlurAlpha.create(0, node);
        ui.draw();
        ui.removeNode(node);
        converter->convertToFile(renderer.compositingFramebuffer().read({{}, ImageSize}, {PixelFormat::RGBA8Unorm}), "ui-baselayer-blur-alpha.png");
    } {
        renderer.compositingTexture().setSubImage(0, {}, *backgroundImage);

        Ui::NodeHandle node = ui.createNode({8, 8}, {112, 48});
        layerBackgroundBlurTextured.create(0, node);
        ui.draw();
        ui.removeNode(node);
        converter->convertToFile(renderer.compositingFramebuffer().read({{}, ImageSize}, {PixelFormat::RGBA8Unorm}), "ui-baselayer-blur-textured.png");
    } {
        renderer.compositingTexture().setSubImage(0, {}, *backgroundImage);

        Ui::NodeHandle node = ui.createNode({8, 8}, {112, 48});
        layerBackgroundBlurTextureMask.create(0, node);
        ui.draw();
        ui.removeNode(node);
        converter->convertToFile(renderer.compositingFramebuffer().read({{}, ImageSize}, {PixelFormat::RGBA8Unorm}), "ui-baselayer-blur-textured-mask.png");
    }

    return 0;
}

}

MAGNUM_WINDOWLESSAPPLICATION_MAIN(UiBaseLayerFlags)
