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
#include <Corrade/Utility/Algorithms.h>
#include <Corrade/Utility/Path.h>
#include <Magnum/Image.h>
#include <Magnum/ImageView.h>
#include <Magnum/PixelFormat.h>
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
#include <Magnum/Text/AbstractFont.h>
#include <Magnum/Text/Alignment.h>
#include <Magnum/Text/Feature.h>
#include <Magnum/Text/DistanceFieldGlyphCacheGL.h>
#include <Magnum/TextureTools/Atlas.h>
#include <Magnum/Trade/AbstractImageConverter.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/ImageData.h>

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

constexpr Vector2i ImageSize{512, 256};

/** @todo ffs, duplicated three times, make a batch utility in Magnum */
Image2D unpremultiply(Image2D image) {
    for(Containers::StridedArrayView1D<Color4ub> row: image.pixels<Color4ub>())
        for(Color4ub& pixel: row)
            pixel = pixel.unpremultiplied();

    return image;
}

int UiTextLayer::exec() {
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

    PluginManager::Manager<Text::AbstractFont> fontManager;
    PluginManager::Manager<Trade::AbstractImporter> importerManager;

    /* Non-distance-field font & glyph cache */
    Containers::Pointer<Text::AbstractFont> font = fontManager.loadAndInstantiate("HarfBuzzFont");
    if(!font || !font->openFile(Utility::Path::join(Utility::Path::path(__FILE__), "../../src/Magnum/Ui/SourceSans3-Regular.otf"), 2*24.0f))
        return 1;
    Containers::Pointer<Text::AbstractFont> fontLarge = fontManager.loadAndInstantiate("HarfBuzzFont");
    if(!fontLarge || !fontLarge->openFile(Utility::Path::join(Utility::Path::path(__FILE__), "../../src/Magnum/Ui/SourceSans3-Regular.otf"), 2*40.0f))
        return 1;

    Text::GlyphCacheArrayGL glyphCache{PixelFormat::R8Unorm, {1024, 1024, 1}};
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
    Ui::TextLayerGL::Shared layerShared{glyphCache,
        Ui::TextLayerGL::Shared::Configuration{10}
            .setEditingStyleCount(6)
    };

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
        converter->convertToFile(unpremultiply(renderer.compositingFramebuffer().read({{0, 192}, {384, 256}}, {PixelFormat::RGBA8Unorm})), "ui-textlayer-style-color.png");
    } {
        renderer.compositingFramebuffer().clearColor(0, 0x00000000_rgbaf);

        Ui::NodeHandle root = ui.createNode({}, {48, 16});
        Ui::DataHandle hash = layer.create(2, "#", {}, root);
        Ui::DataHandle text = layer.create(3, "whee", {}, root);
        layer.setPadding(hash, {0.0f, 0.0f, layer.size(text).x(), 0.0f});
        layer.setPadding(text, {layer.size(hash).x(), 0.0f, 0.0f, 0.0f});

        ui.draw();
        ui.removeNode(root);
        converter->convertToFile(unpremultiply(renderer.compositingFramebuffer().read({{0, 192}, {192, 256}}, {PixelFormat::RGBA8Unorm})), "ui-textlayer-style-data-padding.png");
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
        converter->convertToFile(unpremultiply(renderer.compositingFramebuffer().read({{0, 128}, {256, 256}}, {PixelFormat::RGBA8Unorm})), "ui-textlayer-style-features.png");
    } {
        renderer.compositingFramebuffer().clearColor(0, 0x00000000_rgbaf);

        Ui::NodeHandle root = ui.createNode({}, {16, 16});
        layer.createGlyph(5, font->glyphForName("uni2615"), {}, root);

        ui.draw();
        ui.removeNode(root);
        converter->convertToFile(unpremultiply(renderer.compositingFramebuffer().read({{0, 192}, {64, 256}}, {PixelFormat::RGBA8Unorm})), "ui-textlayer-single-glyph.png");
    } {
        renderer.compositingFramebuffer().clearColor(0, 0x00000000_rgbaf);

        Ui::NodeHandle root = ui.createNode({}, {64, 16});
        Ui::DataHandle text = layer.create(7, "Hello world!", {}, Ui::TextDataFlag::Editable, root);
        layer.setCursor(text, 7, 4);

        ui.draw();
        ui.removeNode(root);
        converter->convertToFile(unpremultiply(renderer.compositingFramebuffer().read({{0, 192}, {256, 256}}, {PixelFormat::RGBA8Unorm})), "ui-textlayer-editing-color.png");
    } {
        renderer.compositingFramebuffer().clearColor(0, 0x00000000_rgbaf);

        Ui::NodeHandle root = ui.createNode({}, {64, 16});
        Ui::DataHandle text = layer.create(8, "Hello world!", {}, Ui::TextDataFlag::Editable, root);
        layer.setCursor(text, 7, 4);

        ui.draw();
        ui.removeNode(root);
        converter->convertToFile(unpremultiply(renderer.compositingFramebuffer().read({{0, 192}, {256, 256}}, {PixelFormat::RGBA8Unorm})), "ui-textlayer-editing-padding.png");
    } {
        renderer.compositingFramebuffer().clearColor(0, 0x00000000_rgbaf);

        Ui::NodeHandle root = ui.createNode({}, {64, 16});
        Ui::DataHandle text = layer.create(9, "Hello world!", {}, Ui::TextDataFlag::Editable, root);
        layer.setCursor(text, 7, 4);

        ui.draw();
        ui.removeNode(root);
        converter->convertToFile(unpremultiply(renderer.compositingFramebuffer().read({{0, 192}, {256, 256}}, {PixelFormat::RGBA8Unorm})), "ui-textlayer-editing-rounded.png");
    }

    /* Distance field glyph cache and font */
    Containers::Pointer<Text::AbstractFont> fontDistanceField =  fontManager.loadAndInstantiate("HarfBuzzFont");
    if(!fontDistanceField || !fontDistanceField->openFile(Utility::Path::join(Utility::Path::path(__FILE__), "../../src/Magnum/Ui/SourceSans3-Regular.otf"), 8*16.0f))
        return 1;
    Text::DistanceFieldGlyphCacheArrayGL glyphCacheDistanceField{{1024, 2048, 1}, {256, 512}, 20};

    /* Images for the distance field style docs */
    Ui::TextLayerGL::Shared layerSharedDistanceField{glyphCacheDistanceField,
        Ui::TextLayerGL::Shared::Configuration{6}
    };

    /* Extra "clock needle" glyph for the distance field cache. The SVG should
       be then verbatim copied into the [TextLayer-transformation] snippet in
       doc/snippets/Ui.cpp. */
    Ui::FontHandle needleFont;
    {
        Containers::Pointer<Trade::AbstractImporter> importer = importerManager.loadAndInstantiate("SvgImporter");
        if(!importer || !importer->openFile(Utility::Path::join(Utility::Path::path(__FILE__), "../artwork/ui-textlayer-needle.svg")))
            return 1;

        Containers::Optional<Trade::ImageData2D> image = importer->image2D(0);
        if(!image)
            return 1;

        Vector2i offset[1];
        Containers::Optional<Range2Di> out = glyphCacheDistanceField.atlas().add({image->size()}, offset);
        CORRADE_INTERNAL_ASSERT(out);
        Utility::copy(
            image->pixels<Color4ub>().slice(&Color4ub::r),
            glyphCacheDistanceField.image().pixels<UnsignedByte>()[0].sliceSize(
                {std::size_t(offset->y()), std::size_t(offset->x())},
                {std::size_t(image->size().y()), std::size_t(image->size().x())}));
        glyphCacheDistanceField.flushImage(*out);

        UnsignedInt fontId = glyphCacheDistanceField.addFont(1);
        glyphCacheDistanceField.addGlyph(fontId, 0, {-16, -16}, Range2Di::fromSize(*offset, image->size()));

        needleFont = layerSharedDistanceField.addInstancelessFont(fontId, 0.125f);
    }

    /* Glyphs added after the needle to fit in the space next to it */
    fontDistanceField->fillGlyphCache(glyphCacheDistanceField,
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "1234567890! .#:'?");

    Ui::FontHandle fontHandleDistanceField = layerSharedDistanceField.addFont(*fontDistanceField, 12.0f);
    Ui::FontHandle fontSmallHandleDistanceField = layerSharedDistanceField.addFont(*fontDistanceField, 8.0f);
    Ui::FontHandle fontLargeHandleDistanceField = layerSharedDistanceField.addFont(*fontDistanceField, 16.0f);

    layerSharedDistanceField.setStyle(
        Ui::TextLayerCommonStyleUniform{}
            .setSmoothness(1.0f),
        {Ui::TextLayerStyleUniform{}    /* 0 */
            .setColor(0x2f83cc_rgbf),
         Ui::TextLayerStyleUniform{}    /* 1 */
            .setColor(0xa5c9ea_rgbf),
         Ui::TextLayerStyleUniform{}    /* 2 */
            .setColor(0xdcdcdc_rgbf)
            .setSmoothness(4.0f),
         Ui::TextLayerStyleUniform{}    /* 3 */
            .setColor(0xdcdcdc_rgbf)
            .setOutlineColor(0x2f83cc_rgbf)
            .setOutlineWidth(1.25f)
            .setEdgeOffset(0.625f),
         Ui::TextLayerStyleUniform{}    /* 4 */
            .setColor(0x2f83cc_rgbf)
            .setEdgeOffset(0.75f),
         Ui::TextLayerStyleUniform{}    /* 5 */
            .setColor(0xdcdcdc_rgbf)
            .setOutlineColor(0x2f83cc_rgbf)
            .setOutlineWidth(0.5f)
            .setEdgeOffset(0.5f)},
        {fontSmallHandleDistanceField,
         fontLargeHandleDistanceField,
         fontHandleDistanceField,
         fontHandleDistanceField,
         fontHandleDistanceField,
         fontHandleDistanceField},
        {Text::Alignment::MiddleCenter,  /* 0 */
         Text::Alignment::MiddleCenter,  /* 1 */
         Text::Alignment::MiddleCenter,  /* 2 */
         Text::Alignment::MiddleCenter,  /* 3 */
         Text::Alignment::MiddleCenter,  /* 4 */
         Text::Alignment::LineCenter},   /* 5 */
        {}, {}, {}, {}, {},
        {{}, {}, {}, {}, {}, Vector4{2.0f, 0.0f, 2.0f, 0.0f} /* 5 */});
    Ui::TextLayer& layerDistanceField = ui.setLayerInstance(Containers::pointer<Ui::TextLayerGL>(ui.createLayer(), layerSharedDistanceField, Ui::TextLayerFlag::Transformable));

    {
        renderer.compositingFramebuffer().clearColor(0, 0x00000000_rgbaf);

        Ui::NodeHandle root = ui.createNode({}, {96, 20});
        Ui::NodeHandle small = ui.createNode(root, {}, {28, 20});
        Ui::NodeHandle big = ui.createNode(root, {28, 0}, {20, 20});
        Ui::NodeHandle smooth = ui.createNode(root, {48, 0}, {48, 20});
        layerDistanceField.create(0, "small", {}, small);
        layerDistanceField.create(1, "big", {}, big);
        layerDistanceField.create(2, "smooth", {}, smooth);
        ui.draw();
        ui.removeNode(root);
        converter->convertToFile(unpremultiply(renderer.compositingFramebuffer().read({{0, 176}, {384, 256}}, {PixelFormat::RGBA8Unorm})), "ui-textlayer-style-smoothness.png");
    } {
        renderer.compositingFramebuffer().clearColor(0, 0x00000000_rgbaf);

        Ui::NodeHandle root = ui.createNode({}, {96, 16});
        Ui::NodeHandle edgy = ui.createNode(root, {}, {48, 16});
        Ui::NodeHandle bulky = ui.createNode(root, {48, 0}, {48, 16});
        layerDistanceField.create(3, "edgy.", {}, edgy);
        layerDistanceField.create(4, "bulky!?", {}, bulky);
        ui.draw();
        ui.removeNode(root);
        converter->convertToFile(unpremultiply(renderer.compositingFramebuffer().read({{0, 192}, {384, 256}}, {PixelFormat::RGBA8Unorm})), "ui-textlayer-style-offset-outline.png");
    } {
        renderer.compositingFramebuffer().clearColor(0, 0x00000000_rgbaf);

        Ui::NodeHandle clock = ui.createNode({}, {64, 64});
        layerDistanceField.create(5, "12", Text::Alignment::TopCenter, clock);
        layerDistanceField.create(5, "3", Text::Alignment::MiddleRight, clock);
        layerDistanceField.create(5, "6", Text::Alignment::BottomCenter, clock);
        layerDistanceField.create(5, "9", Text::Alignment::MiddleLeft, clock);

        Ui::NodeHandle hours = ui.createNode(clock, {}, ui.nodeSize(clock));
        Ui::DataHandle hoursData = layerDistanceField.createGlyph(5, 0, needleFont, hours);
        layerDistanceField.rotate(hoursData, 360.0_degf*(10.0f + 11.0f/60.0f)/12.0f);
        layerDistanceField.scale(hoursData, 0.75f);

        Ui::NodeHandle minutes = ui.createNode(hours, {}, ui.nodeSize(hours));
        Ui::DataHandle minutesData = layerDistanceField.createGlyph(5, 0, needleFont, minutes);
        layerDistanceField.rotate(minutesData, 360.0_degf*11.0f/60.0f);

        ui.draw();
        ui.removeNode(clock);
        converter->convertToFile(unpremultiply(renderer.compositingFramebuffer().read({{0, 0}, {256, 256}}, {PixelFormat::RGBA8Unorm})), "ui-textlayer-transformation.png");
    }

    return 0;
}

}

MAGNUM_WINDOWLESSAPPLICATION_MAIN(UiTextLayer)
