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
#include <Corrade/Containers/String.h>
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
#include <Magnum/Text/AbstractFont.h>
#include <Magnum/Text/Alignment.h>
#include <Magnum/Text/Feature.h>
#include <Magnum/Text/GlyphCacheGL.h>
#include <Magnum/Trade/AbstractImageConverter.h>

#include "Magnum/Ui/AbstractUserInterface.h"
#include "Magnum/Ui/RendererGL.h"
#include "Magnum/Ui/TextLayerGL.h"
#include "Magnum/Ui/TextProperties.h"

using namespace Magnum;
using namespace Magnum::Math::Literals;

namespace {

struct UiTextLayer: Platform::WindowlessApplication {
    explicit UiTextLayer(const Arguments& arguments): Platform::WindowlessApplication{arguments} {}

    int exec() override;
};

constexpr Vector2i ImageSize{512, 128};

/** @todo ffs, this is duplicated three times, turn it into some utility in
    Magnum */
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

int UiTextLayer::exec() {
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::One, GL::Renderer::BlendFunction::OneMinusSourceAlpha);

    /* The actual framebuffer size is 4x the UI size */
    Ui::AbstractUserInterface ui{{128.0f, 32.0f}, Vector2{ImageSize}, ImageSize};
    /* Using a compositing framebuffer because it's easier than setting up a
       custom framebuffer here */
    Ui::RendererGL& renderer = ui.setRendererInstance(Containers::pointer<Ui::RendererGL>(Ui::RendererGL::Flag::CompositingFramebuffer));

    PluginManager::Manager<Trade::AbstractImageConverter> converterManager;
    Containers::Pointer<Trade::AbstractImageConverter> converter = converterManager.loadAndInstantiate("AnyImageConverter");
    if(!converter)
        return 1;

    /* Font & glyph cache used by everything */
    PluginManager::Manager<Text::AbstractFont> fontManager;
    Containers::Pointer<Text::AbstractFont> font = fontManager.loadAndInstantiate("HarfBuzzFont");
    if(!font || !font->openFile(Utility::Path::join(Utility::Path::path(__FILE__), "../../src/Magnum/Ui/SourceSans3-Regular.otf"), 2*24.0f))
        return 1;
    Containers::Pointer<Text::AbstractFont> fontLarge = fontManager.loadAndInstantiate("HarfBuzzFont");
    if(!fontLarge || !fontLarge->openFile(Utility::Path::join(Utility::Path::path(__FILE__), "../../src/Magnum/Ui/SourceSans3-Regular.otf"), 2*40.0f))
        return 1;

    Text::GlyphCacheGL glyphCache{PixelFormat::R8Unorm, {1024, 1024}};
    font->fillGlyphCache(glyphCache,
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "1234567890! #:'");
    font->fillGlyphCache(glyphCache, {
        font->glyphForName("A.s"),
        font->glyphForName("B.s"),
        font->glyphForName("C.s"),
        font->glyphForName("D.s"),
        font->glyphForName("E.s"),
        font->glyphForName("F.s"),
        font->glyphForName("G.s"),
        font->glyphForName("H.s"),
        font->glyphForName("I.s"),
        font->glyphForName("J.s"),
        font->glyphForName("K.s"),
        font->glyphForName("L.s"),
        font->glyphForName("M.s"),
        font->glyphForName("N.s"),
        font->glyphForName("O.s"),
        font->glyphForName("P.s"),
        font->glyphForName("Q.s"),
        font->glyphForName("R.s"),
        font->glyphForName("S.s"),
        font->glyphForName("T.s"),
        font->glyphForName("U.s"),
        font->glyphForName("V.s"),
        font->glyphForName("W.s"),
        font->glyphForName("X.s"),
        font->glyphForName("Y.s"),
        font->glyphForName("Z.s"),
        font->glyphForName("four.t"),
        font->glyphForName("one.t"),
        font->glyphForName("eight.t"),
        font->glyphForName("a.a")
    });
    fontLarge->fillGlyphCache(glyphCache, {
        font->glyphForName("uni2615") /* coffee */
    });

    /* Images for the main style docs */
    Ui::TextLayerGL::Shared layerShared{
        Ui::TextLayerGL::Shared::Configuration{10}
            .setEditingStyleCount(6)
    };
    layerShared.setGlyphCache(glyphCache);

    Ui::FontHandle fontHandle = layerShared.addFont(*font, 12.0f);
    Ui::FontHandle fontLargeHandle = layerShared.addFont(*fontLarge, 20.0f);

    layerShared.setStyle(
        Ui::TextLayerCommonStyleUniform{},
        {Ui::TextLayerStyleUniform{},   /* 0 */
         Ui::TextLayerStyleUniform{}    /* 1 */
            .setColor(0x2f83cc_rgbf),
         Ui::TextLayerStyleUniform{}    /* 2 */
            .setColor(0xa5c9ea_rgbf),
         Ui::TextLayerStyleUniform{}    /* 3 */
            .setColor(0x2f83cc_rgbf),
         Ui::TextLayerStyleUniform{}    /* 4 */
            .setColor(0xdcdcdc_rgbf),
         Ui::TextLayerStyleUniform{}    /* 5 */
            .setColor(0xc7cf2f_rgbf),
         Ui::TextLayerStyleUniform{}    /* 6, used as a selection style */
            .setColor(0x2f363f_rgbf),
         Ui::TextLayerStyleUniform{}    /* 7, editable, just color */
            .setColor(0xdcdcdc_rgbf),
         Ui::TextLayerStyleUniform{}    /* 8, editable, padding */
            .setColor(0xdcdcdc_rgbf),
         Ui::TextLayerStyleUniform{}    /* 9, editable, rounded */
            .setColor(0xdcdcdc_rgbf)},
        {fontHandle,
         fontHandle,
         fontHandle,
         fontHandle,
         fontHandle,
         fontLargeHandle,
         fontHandle,
         fontHandle,
         fontHandle,
         fontHandle},
        {Text::Alignment::MiddleCenter,
         Text::Alignment::MiddleCenter,
         Text::Alignment::MiddleCenter,
         Text::Alignment::MiddleCenter,
         Text::Alignment::MiddleCenter,
         Text::Alignment::MiddleCenter,
         Text::Alignment::MiddleCenter,
         Text::Alignment::MiddleCenter,
         Text::Alignment::MiddleCenter,
         Text::Alignment::MiddleCenter},
        {}, {}, {},
        {
            -1,
            -1,
            -1,
            -1,
            -1,
            -1,
            -1,
            0,
            2,
            4
        }, {
            -1,
            -1,
            -1,
            -1,
            -1,
            -1,
            -1,
            1,
            3,
            5
        }, {
            {},
            {},
            {0.0f, 0.0f, 2.0f, 0.0f},
            {2.0f, 0.0f, 0.0f, 0.0f},
            {},
            {},
            {},
            {},
            {},
            {}
        });
    layerShared.setEditingStyle(
        Ui::TextLayerCommonEditingStyleUniform{}
            .setSmoothness(1.0f),
        {Ui::TextLayerEditingStyleUniform{} /* style 7 cursor */
            .setBackgroundColor(0xa5c9ea_rgbf),
         Ui::TextLayerEditingStyleUniform{} /* style 7 selection */
            .setBackgroundColor(0x2f83cc_rgbf),
         Ui::TextLayerEditingStyleUniform{} /* style 8 cursor */
            .setBackgroundColor(0xa5c9ea_rgbf),
         Ui::TextLayerEditingStyleUniform{} /* style 8 selection */
            .setBackgroundColor(0x2f83cc_rgbf),
         Ui::TextLayerEditingStyleUniform{} /* style 9 cursor */
            .setBackgroundColor(0xa5c9ea_rgbf)
            .setCornerRadius(1.0f),
         Ui::TextLayerEditingStyleUniform{} /* style 9 selection */
            .setBackgroundColor(0x2f83cc_rgbf)
            .setCornerRadius(2.0f)},
        {-1, 6, -1, 6, -1, 6},
        {{1.0f, -1.0f, 1.0f, -1.0f}, /* style 7 cursor */
         {0.0f, -1.0f, 0.0f, -1.0f}, /* style 7 selection */
         {0.0f, -1.0f, 2.0f, -1.0f}, /* style 8 cursor */
         {1.0f, -2.0f, 2.0f, -2.0f}, /* style 8 selection */
         {0.0f, -1.0f, 2.0f, -1.0f}, /* style 9 cursor */
         {1.0f, -2.0f, 2.0f, -2.0f}} /* style 9 selection */
    );
    Ui::TextLayer& layer = ui.setLayerInstance(Containers::pointer<Ui::TextLayerGL>(ui.createLayer(), layerShared));

    {
        renderer.compositingFramebuffer().clearColor(0, 0x00000000_rgbaf);

        Ui::NodeHandle root = ui.createNode({}, {96, 16});
        Ui::NodeHandle blue = ui.createNode(root, {}, {42, 16});
        Ui::NodeHandle colored = ui.createNode(root, {42, 0}, {27, 16});
        Ui::NodeHandle faded = ui.createNode(root, {69, 0}, {27, 16});
        ui.setNodeOpacity(faded, 0.25f);
        layer.create(1, "hello!", {}, blue);
        Ui::DataHandle coloredData = layer.create(0, "HEY", {}, colored);
        layer.setColor(coloredData, 0x3bd267_rgbf);
        layer.create(1, "shh", {}, faded);
        ui.draw();
        ui.removeNode(root);
        converter->convertToFile(unpremultiply(renderer.compositingFramebuffer().read({{0, 64}, {384, 128}}, {PixelFormat::RGBA8Unorm})), "ui-textlayer-style-color.png");
    } {
        renderer.compositingFramebuffer().clearColor(0, 0x00000000_rgbaf);

        Ui::NodeHandle root = ui.createNode({}, {48, 16});
        Ui::DataHandle hash = layer.create(2, "#", {}, root);
        Ui::DataHandle text = layer.create(3, "whee", {}, root);
        layer.setPadding(hash, {0.0f, 0.0f, layer.size(text).x(), 0.0f});
        layer.setPadding(text, {layer.size(hash).x(), 0.0f, 0.0f, 0.0f});

        ui.draw();
        ui.removeNode(root);
        converter->convertToFile(unpremultiply(renderer.compositingFramebuffer().read({{0, 64}, {192, 128}}, {PixelFormat::RGBA8Unorm})), "ui-textlayer-style-data-padding.png");
    } {
        renderer.compositingFramebuffer().clearColor(0, 0x00000000_rgbaf);

        Ui::NodeHandle root = ui.createNode({}, {128, 32});
        Ui::NodeHandle left = ui.createNode(root, {}, {128, 16});
        Ui::NodeHandle right = ui.createNode(root, {0, 16}, {128, 16});
        layer.create(4, "Status: 418 I'm a Teapot", {}, left);
        layer.create(4, "Status: 418 I'm a Teapot",
            Ui::TextProperties{}.setFeatures({
                Text::Feature::OldstyleFigures,
                {Text::Feature::CharacterVariants2, 2, 3},
                {Text::Feature::SmallCapitals, 8, ~UnsignedInt{}},
            }), right);

        ui.draw();
        ui.removeNode(root);
        converter->convertToFile(unpremultiply(renderer.compositingFramebuffer().read({{}, ImageSize}, {PixelFormat::RGBA8Unorm})), "ui-textlayer-style-features.png");
    } {
        renderer.compositingFramebuffer().clearColor(0, 0x00000000_rgbaf);

        Ui::NodeHandle root = ui.createNode({}, {16, 16});
        layer.createGlyph(5, font->glyphForName("uni2615"), {}, root);

        ui.draw();
        ui.removeNode(root);
        converter->convertToFile(unpremultiply(renderer.compositingFramebuffer().read({{0, 64}, {64, 128}}, {PixelFormat::RGBA8Unorm})), "ui-textlayer-single-glyph.png");
    } {
        renderer.compositingFramebuffer().clearColor(0, 0x00000000_rgbaf);

        Ui::NodeHandle root = ui.createNode({}, {64, 16});
        Ui::DataHandle text = layer.create(7, "Hello world!", {}, Ui::TextDataFlag::Editable, root);
        layer.setCursor(text, 7, 4);

        ui.draw();
        ui.removeNode(root);
        converter->convertToFile(unpremultiply(renderer.compositingFramebuffer().read({{0, 64}, {256, 128}}, {PixelFormat::RGBA8Unorm})), "ui-textlayer-editing-color.png");
    } {
        renderer.compositingFramebuffer().clearColor(0, 0x00000000_rgbaf);

        Ui::NodeHandle root = ui.createNode({}, {64, 16});
        Ui::DataHandle text = layer.create(8, "Hello world!", {}, Ui::TextDataFlag::Editable, root);
        layer.setCursor(text, 7, 4);

        ui.draw();
        ui.removeNode(root);
        converter->convertToFile(unpremultiply(renderer.compositingFramebuffer().read({{0, 64}, {256, 128}}, {PixelFormat::RGBA8Unorm})), "ui-textlayer-editing-padding.png");
    } {
        renderer.compositingFramebuffer().clearColor(0, 0x00000000_rgbaf);

        Ui::NodeHandle root = ui.createNode({}, {64, 16});
        Ui::DataHandle text = layer.create(9, "Hello world!", {}, Ui::TextDataFlag::Editable, root);
        layer.setCursor(text, 7, 4);

        ui.draw();
        ui.removeNode(root);
        converter->convertToFile(unpremultiply(renderer.compositingFramebuffer().read({{0, 64}, {256, 128}}, {PixelFormat::RGBA8Unorm})), "ui-textlayer-editing-rounded.png");
    }

    return 0;
}

}

MAGNUM_WINDOWLESSAPPLICATION_MAIN(UiTextLayer)
