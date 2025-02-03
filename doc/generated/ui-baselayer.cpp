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
#include <Corrade/Containers/StringIterable.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/PluginManager/PluginMetadata.h>
#include <Corrade/Utility/ConfigurationGroup.h>
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

struct UiBaseLayer: Platform::WindowlessApplication {
    explicit UiBaseLayer(const Arguments& arguments): Platform::WindowlessApplication{arguments} {}

    int exec() override;
};

constexpr Vector2i ImageSize{512, 256};

Image2D unpremultiply(Image2D image) {
    for(Containers::StridedArrayView1D<Color4ub> row: image.pixels<Color4ub>()) {
        for(Color4ub& pixel: row) {
            Color4 pixelf = Math::unpack<Color4>(pixel);
            if(pixelf.a())
                pixelf.rgb() /= pixelf.a();
            else
                pixelf.rgb() = 0x2f363f_rgbf;
            pixel = Math::pack<Color4ub>(pixelf);
        }
    }

    return image;
}

int UiBaseLayer::exec() {
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

    GL::Texture2DArray textureBalloon;
    textureBalloon
        .setMinificationFilter(GL::SamplerFilter::Linear)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setStorage(1, GL::textureFormat(backgroundImage->format()), {backgroundImage->size(), 1})
        .setSubImage(0, {}, ImageView2D{*backgroundImage});

    /* Images for the main style docs */
    Ui::BaseLayerGL::Shared layerNoSmoothnessShared{Ui::BaseLayerGL::Shared::Configuration{2}};
    layerNoSmoothnessShared.setStyle(
        Ui::BaseLayerCommonStyleUniform{},
        {Ui::BaseLayerStyleUniform{}
            .setColor(0x2f83cc_rgbf),
         Ui::BaseLayerStyleUniform{}
            .setColor(0xdcdcdc_rgbf, 0xa5c9ea_rgbf)},
        {});
    Ui::BaseLayerGL& layerNoSmoothness = ui.setLayerInstance(Containers::pointer<Ui::BaseLayerGL>(ui.createLayer(), layerNoSmoothnessShared));

    {
        renderer.compositingFramebuffer().clearColor(0, 0x00000000_rgbaf);

        Ui::NodeHandle root = ui.createNode({}, {128, 32});
        Ui::NodeHandle blue = ui.createNode(root, {4, 4}, {24, 24});
        Ui::NodeHandle gradient = ui.createNode(root, {36, 4}, {24, 24});
        Ui::NodeHandle colored = ui.createNode(root, {68, 4}, {24, 24});
        Ui::NodeHandle opacity = ui.createNode(root, {100, 4}, {24, 24});
        ui.setNodeOpacity(opacity, 0.25f);
        layerNoSmoothness.create(0, blue);
        layerNoSmoothness.create(1, gradient);
        layerNoSmoothness.setColor(layerNoSmoothness.create(1, colored), 0x3bd267_rgbf);
        layerNoSmoothness.create(1, opacity);
        ui.draw();
        ui.removeNode(root);
        /* GL coordinates are Y up, so take the upper half, not lower */
        converter->convertToFile(unpremultiply(renderer.compositingFramebuffer().read({{0, 128}, {512, 256}}, {PixelFormat::RGBA8Unorm})), "ui-baselayer-style-color.png");
    }

    Ui::BaseLayerGL::Shared layerShared{Ui::BaseLayerGL::Shared::Configuration{11}};
    layerShared.setStyle(
        Ui::BaseLayerCommonStyleUniform{}
            .setSmoothness(1.0f),
        {Ui::BaseLayerStyleUniform{} /* 0, close */
            .setCornerRadius(8.0f)
            .setColor(0xcd3431_rgbf),
         Ui::BaseLayerStyleUniform{} /* 1, heading */
            .setCornerRadius({8.0f, 1.0f, 8.0f, 1.0f})
            .setColor(0xdcdcdc_rgbf),
         Ui::BaseLayerStyleUniform{} /* 2, frame */
            .setColor(0x00000000_rgbaf)
            .setOutlineColor(0xdcdcdc_rgbf)
            .setOutlineWidth(1.0f),
         Ui::BaseLayerStyleUniform{} /* 3, different */
            .setColor(0xa5c9ea_rgbf)
            .setOutlineColor(0x405363_rgbf)
            .setOutlineWidth({1.0f, 1.0f, 16.0f, 1.0f})
            .setCornerRadius(12.0f)
            .setInnerOutlineCornerRadius(11.0f),
         Ui::BaseLayerStyleUniform{} /* 4, rounded */
            .setColor(0x2a703f_rgbf)
            .setOutlineColor(0x3bd267_rgbf)
            .setOutlineWidth(2.0f)
            .setCornerRadius(2.0f)
            .setInnerOutlineCornerRadius(10.0f),
         Ui::BaseLayerStyleUniform{} /* 5, progress */
            .setColor(0x3bd267_rgbf)
            .setOutlineColor(0x405363_rgbf)
            .setCornerRadius(6.0f)
            .setInnerOutlineCornerRadius(6.0f),
         Ui::BaseLayerStyleUniform{} /* 6, button outer */
            .setColor(0x00000000_rgbaf)
            .setOutlineColor(0xa5c9ea_rgbf)
            .setOutlineWidth(1.0f)
            .setCornerRadius(5.0f)
            .setInnerOutlineCornerRadius(4.0f),
         Ui::BaseLayerStyleUniform{} /* 7, button inner */
            .setColor(0xa5c9ea_rgbf)
            .setCornerRadius(2.0f),
         Ui::BaseLayerStyleUniform{} /* 8, progress under */
            .setColor(0x405363_rgbf*0.9f, 0x405363_rgbf*1.1f)
            .setCornerRadius(3.0f),
         Ui::BaseLayerStyleUniform{} /* 9, progress over */
            .setColor(0x3bd267_rgbf*1.1f, 0x3bd267_rgbf*0.9f)
            .setCornerRadius(6.0f),
         Ui::BaseLayerStyleUniform{}
            .setCornerRadius(12.0f)
            .setColor(0xffffffff_rgbaf*0.667f)
            .setOutlineColor(0x00000000_rgbaf)},
        {{}, /* 0 */
         {}, /* 1 */
         {}, /* 2 */
         {}, /* 3 */
         {}, /* 4 */
         {}, /* 5 */
         {}, /* 6 */
         Vector4{3.0f}, /* 7, button inner */
         Vector4{3.0f}, /* 8, progress under */
         {}, /* 9 */
         {}, /* 10 */
    });
    Ui::BaseLayerGL& layer = ui.setLayerInstance(Containers::pointer<Ui::BaseLayerGL>(ui.createLayer(), layerShared));

    {
        renderer.compositingFramebuffer().clearColor(0, 0x00000000_rgbaf);

        Ui::NodeHandle heading = ui.createNode({4, 4}, {120, 24});
        Ui::NodeHandle close = ui.createNode(heading, {100, 4}, {16, 16});
        layer.create(1, heading);
        layer.create(0, close);
        ui.draw();
        ui.removeNode(heading);
        /* GL coordinates are Y up, so take the upper half, not lower */
        converter->convertToFile(unpremultiply(renderer.compositingFramebuffer().read({{0, 128}, {512, 256}}, {PixelFormat::RGBA8Unorm})), "ui-baselayer-style-rounded-corners.png");
    } {
        renderer.compositingFramebuffer().clearColor(0, 0x00000000_rgbaf);

        Ui::NodeHandle root = ui.createNode({}, {128, 32});
        Ui::NodeHandle frame = ui.createNode(root, {4, 4}, {24, 24});
        Ui::NodeHandle different = ui.createNode(root, {40, 4}, {48, 24});
        Ui::NodeHandle rounded = ui.createNode(root, {100, 4}, {24, 24});
        layer.create(2, frame);
        layer.create(3, different);
        layer.create(4, rounded);
        ui.draw();
        ui.removeNode(root);
        /* GL coordinates are Y up, so take the upper half, not lower */
        converter->convertToFile(unpremultiply(renderer.compositingFramebuffer().read({{0, 128}, {512, 256}}, {PixelFormat::RGBA8Unorm})), "ui-baselayer-style-outline.png");
    } {
        renderer.compositingFramebuffer().clearColor(0, 0x00000000_rgbaf);

        Ui::NodeHandle progress = ui.createNode({2, 2}, {124, 12});
        Ui::DataHandle progressData = layer.create(5, progress);
        Float percentage = 72.0f;
        layer.setOutlineWidth(progressData, {0.0f, 0.0f, ui.nodeSize(progress).x()*(100.0f - percentage)/100.0f, 0.0f});
        ui.draw();
        ui.removeNode(progress);
        /* GL coordinates are Y up, so take the upper half, not lower */
        converter->convertToFile(unpremultiply(renderer.compositingFramebuffer().read({{0, 192}, {512, 256}}, {PixelFormat::RGBA8Unorm})), "ui-baselayer-style-outline-data-width.png");
    } {
        renderer.compositingFramebuffer().clearColor(0, 0x00000000_rgbaf);

        Ui::NodeHandle root = ui.createNode({}, {128, 32});
        Ui::NodeHandle button = ui.createNode(root, {4, 4}, {40, 24});
        layer.create(6, button);
        layer.create(7, button);
        Ui::NodeHandle progressUnder = ui.createNode(root, {52, 10}, {72, 12});
        Ui::NodeHandle progressOver = ui.createNode(progressUnder, {}, {72, 12});
        layer.create(8, progressUnder);
        Float percentage = 43.0f;
        Ui::DataHandle progressData = layer.create(9, progressOver);
        layer.setPadding(progressData, {0.0f, 0.0f, ui.nodeSize(progressUnder).x()*(100.0f - percentage)/100.0f, 0.0f});
        ui.draw();
        ui.removeNode(root);
        /* GL coordinates are Y up, so take the upper half, not lower */
        converter->convertToFile(unpremultiply(renderer.compositingFramebuffer().read({{0, 128}, {512, 256}}, {PixelFormat::RGBA8Unorm})), "ui-baselayer-style-padding.png");
    }

    Ui::BaseLayerGL::Shared layerTexturedShared{Ui::BaseLayerGL::Shared::Configuration{3}
        .addFlags(Ui::BaseLayerSharedFlag::Textured)
    };
    layerTexturedShared.setStyle(
        Ui::BaseLayerCommonStyleUniform{}
            .setSmoothness(1.0f),
        {Ui::BaseLayerStyleUniform{}, /* 0, image */
         Ui::BaseLayerStyleUniform{}  /* 1, outline */
            .setOutlineWidth(2.0f)
            .setOutlineColor(0xdcdcdcff_rgbaf*0.25f),
         Ui::BaseLayerStyleUniform{}  /* 2, avatar */
            .setCornerRadius(12.0f)},
        {});
    Ui::BaseLayerGL& layerTextured = ui.setLayerInstance(Containers::pointer<Ui::BaseLayerGL>(ui.createLayer(), layerTexturedShared));
    layerTextured.setTexture(textureBalloon);

    {
        renderer.compositingFramebuffer().clearColor(0, 0x00000000_rgbaf);

        Ui::NodeHandle root = ui.createNode({}, {128, 32});
        Ui::NodeHandle image = ui.createNode(root, {4, 4}, {40, 24});
        Ui::NodeHandle outlined = ui.createNode(root, {52, 4}, {40, 24});
        Ui::NodeHandle avatar = ui.createNode(root, {100, 4}, {24, 24});
        Ui::DataHandle imageData = layerTextured.create(0, image);
        layerTextured.setTextureCoordinates(imageData, {0.083333f, 0.0f, 0.0f}, {0.833333f, 1.0f});
        Ui::DataHandle outlinedData = layerTextured.create(1, outlined);
        layerTextured.setTextureCoordinates(outlinedData, {0.083333f, 0.0f, 0.0f}, {0.833333f, 1.0f});
        Ui::DataHandle avatarData = layerTextured.create(2, avatar);
        layerTextured.setTextureCoordinates(avatarData, {0.475f, 0.0f, 0.0f}, {0.1875f, 0.375f});
        ui.draw();
        ui.removeNode(root);
        /* GL coordinates are Y up, so take the upper half, not lower */
        converter->convertToFile(unpremultiply(renderer.compositingFramebuffer().read({{0, 128}, {512, 256}}, {PixelFormat::RGBA8Unorm})), "ui-baselayer-style-textured.png");
    }

    /* Images for the BaseLayerSharedFlag enum */
    {
        renderer.compositingTexture().setSubImage(0, {}, *backgroundImage);

        Ui::NodeHandle node = ui.createNode({8, 8}, {112, 48});
        layer.create(10, node);
        ui.draw();
        ui.removeNode(node);
        converter->convertToFile(renderer.compositingFramebuffer().read({{}, ImageSize}, {PixelFormat::RGBA8Unorm}), "ui-baselayer-flag-default.png");
    }

    Ui::BaseLayerGL::Shared layerSharedBackgroundBlur{Ui::BaseLayerGL::Shared::Configuration{1}
        .addFlags(Ui::BaseLayerSharedFlag::BackgroundBlur)
        .setBackgroundBlurRadius(31)};
    layerSharedBackgroundBlur.setStyle(
        Ui::BaseLayerCommonStyleUniform{}
            .setSmoothness(1.0f),
        {Ui::BaseLayerStyleUniform{}
            .setCornerRadius(12.0f)
            .setColor(0xffffffff_rgbaf*0.667f)},
        {});
    Ui::BaseLayerGL& layerBackgroundBlur = ui.setLayerInstance(Containers::pointer<Ui::BaseLayerGL>(ui.createLayer(), layerSharedBackgroundBlur));
    layerBackgroundBlur
        .setBackgroundBlurPassCount(8);

    {
        renderer.compositingTexture().setSubImage(0, {}, *backgroundImage);

        Ui::NodeHandle node = ui.createNode({8, 8}, {112, 48});
        layerBackgroundBlur.create(0, node);
        ui.draw();
        ui.removeNode(node);
        converter->convertToFile(renderer.compositingFramebuffer().read({{}, ImageSize}, {PixelFormat::RGBA8Unorm}), "ui-baselayer-flag-blur.png");
    }

    Ui::BaseLayerGL::Shared layerSharedBackgroundBlurAlpha{Ui::BaseLayerGL::Shared::Configuration{1}
        .addFlags(Ui::BaseLayerSharedFlag::BackgroundBlur)
        .setBackgroundBlurRadius(31)};
    layerSharedBackgroundBlurAlpha.setStyle(
        Ui::BaseLayerCommonStyleUniform{}
            .setSmoothness(1.0f)
            .setBackgroundBlurAlpha(0.75f),
        {Ui::BaseLayerStyleUniform{}
            .setCornerRadius(12.0f)
            .setColor(0xffffffff_rgbaf*0.667f)},
        {});
    Ui::BaseLayerGL& layerBackgroundBlurAlpha = ui.setLayerInstance(Containers::pointer<Ui::BaseLayerGL>(ui.createLayer(), layerSharedBackgroundBlurAlpha));
    layerBackgroundBlurAlpha
        .setBackgroundBlurPassCount(8);

    {
        renderer.compositingTexture().setSubImage(0, {}, *backgroundImage);

        Ui::NodeHandle node = ui.createNode({8, 8}, {112, 48});
        layerBackgroundBlurAlpha.create(0, node);
        ui.draw();
        ui.removeNode(node);
        converter->convertToFile(renderer.compositingFramebuffer().read({{}, ImageSize}, {PixelFormat::RGBA8Unorm}), "ui-baselayer-flag-blur-alpha.png");
    }

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

    {
        renderer.compositingTexture().setSubImage(0, {}, *backgroundImage);

        Ui::NodeHandle node = ui.createNode({8, 8}, {112, 48});
        layerBackgroundBlurTextured.create(0, node);
        ui.draw();
        ui.removeNode(node);
        converter->convertToFile(renderer.compositingFramebuffer().read({{}, ImageSize}, {PixelFormat::RGBA8Unorm}), "ui-baselayer-flag-blur-textured.png");
    }

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
        layerBackgroundBlurTextureMask.create(0, node);
        ui.draw();
        ui.removeNode(node);
        converter->convertToFile(renderer.compositingFramebuffer().read({{}, ImageSize}, {PixelFormat::RGBA8Unorm}), "ui-baselayer-flag-blur-textured-mask.png");
    }

    return 0;
}

}

MAGNUM_WINDOWLESSAPPLICATION_MAIN(UiBaseLayer)
