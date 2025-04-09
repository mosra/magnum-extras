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
#include <Corrade/TestSuite/Compare/Numeric.h>
#include <Corrade/Utility/Algorithms.h>
#include <Magnum/Image.h>
#include <Magnum/ImageView.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/OpenGLTester.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/Text/AbstractFont.h>
#include <Magnum/Text/AbstractShaper.h>
#include <Magnum/Text/Alignment.h>
#include <Magnum/Text/GlyphCacheGL.h>

#include "Magnum/Ui/AbstractUserInterface.h"
#include "Magnum/Ui/TextLayerGL.h"
#include "Magnum/Ui/TextProperties.h"
#include "Magnum/Ui/RendererGL.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct TextLayerGLBenchmark: GL::OpenGLTester {
    explicit TextLayerGLBenchmark();

    void setup();
    void teardown();
    void fragment();

    private:
        GL::Texture2D _color{NoCreate};
        GL::Framebuffer _framebuffer{NoCreate};
};

using namespace Math::Literals;

const struct {
    const char* name;
    UnsignedInt dynamicStyleCount;
    bool drawCursor;
    const char* text;
} FragmentData[]{
    {"glyph quad", 0, false, "a"},
    {"glyph quad, dynamic styles", 1, false, "a"},
    {"cursor quad", 0, true, ""},
    {"cursor quad, dynamic styles", 1, true, ""},
};

TextLayerGLBenchmark::TextLayerGLBenchmark() {
    addInstancedBenchmarks({&TextLayerGLBenchmark::fragment}, 10,
        Containers::arraySize(FragmentData),
        &TextLayerGLBenchmark::setup,
        &TextLayerGLBenchmark::teardown,
        BenchmarkType::GpuTime);
}

constexpr Vector2i BenchmarkSize{2048, 2048};

void TextLayerGLBenchmark::setup() {
    _color = GL::Texture2D{};
    _color.setStorage(1, GL::TextureFormat::RGBA8, BenchmarkSize);
    _framebuffer = GL::Framebuffer{{{}, BenchmarkSize}};
    _framebuffer
        .attachTexture(GL::Framebuffer::ColorAttachment{0}, _color, 0)
        .clear(GL::FramebufferClear::Color)
        .bind();

    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::One, GL::Renderer::BlendFunction::OneMinusSourceAlpha);
    /* The RendererGL should enable these on its own if needed */
    GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::disable(GL::Renderer::Feature::Blending);
}

void TextLayerGLBenchmark::teardown() {
    _framebuffer = GL::Framebuffer{NoCreate};
    _color = GL::Texture2D{NoCreate};

    GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::disable(GL::Renderer::Feature::Blending);
}

void TextLayerGLBenchmark::fragment() {
    auto&& data = FragmentData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Renders a single quad over the whole size to benchmark mainly the
       fragment shader invocation. The quad is either a glyph or a cursor. */

    AbstractUserInterface ui{BenchmarkSize};
    ui.setRendererInstance(Containers::pointer<RendererGL>());

    struct Shaper: Text::AbstractShaper {
        using Text::AbstractShaper::AbstractShaper;

        UnsignedInt doShape(Containers::StringView string, UnsignedInt, UnsignedInt, Containers::ArrayView<const Text::FeatureRange>) override {
            return string.size();
        }
        void doGlyphIdsInto(const Containers::StridedArrayView1D<UnsignedInt>& ids) const override {
            ids[0] = 0;
        }
        void doGlyphOffsetsAdvancesInto(const Containers::StridedArrayView1D<Vector2>& offsets, const Containers::StridedArrayView1D<Vector2>& advances) const override {
            offsets[0] = {};
            advances[0] = {};
        }
        void doGlyphClustersInto(const Containers::StridedArrayView1D<UnsignedInt>&) const override {
            CORRADE_FAIL("This shouldn't be called.");
        }
    };

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return _opened; }
        Properties doOpenFile(Containers::StringView, Float size) override {
            _opened = true;
            return {size, 16.0f, -16.0f, 32.0f, 1};
        }
        void doClose() override { _opened = false; }

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return Containers::pointer<Shaper>(*this); }

        bool _opened = false;
    } font;
    font.openFile({}, 32.0f);

    /* If not drawing the cursor, add a single all-white glyph spanning the
       whole cache. Default padding is 1, reset it back to 0 to make this
       work. */
    Text::GlyphCacheGL cache{PixelFormat::R8Unorm, {32, 32}, {}};
    UnsignedInt fontId = cache.addFont(font.glyphCount(), &font);
    if(!data.drawCursor) {
        cache.addGlyph(fontId, 0, {-16, -16}, {{}, {32, 32}});
        Utility::copy(
            Containers::StridedArrayView2D<const char>{"\xff", {32, 32}, {0, 0}},
            cache.image().pixels<char>()[0]);
        cache.flushImage({{}, {32, 32}});
    }

    TextLayerGL::Shared shared{cache, TextLayer::Shared::Configuration{1}
        .setEditingStyleCount(1)
        .setDynamicStyleCount(data.dynamicStyleCount)
    };

    FontHandle fontHandle = shared.addFont(font, 2048.0f);

    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}
            .setColor(0xff3366_rgbf)},
        {fontHandle},
        {Text::Alignment::MiddleCenter},
        {}, {}, {},
        {data.drawCursor ? 0 : -1}, {-1}, {});
    shared.setEditingStyle(TextLayerCommonEditingStyleUniform{},
        {TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xff3366_rgbf)},
        {},
        {{BenchmarkSize.x()/2.0f, BenchmarkSize.y()/2.0f,
          BenchmarkSize.x()/2.0f, BenchmarkSize.y()/2.0f}});

    TextLayerGL& layer = ui.setLayerInstance(Containers::pointer<TextLayerGL>(ui.createLayer(), shared));

    NodeHandle node = ui.createNode({}, Vector2{BenchmarkSize});
    layer.create(0, data.text, {}, data.drawCursor ? TextDataFlag::Editable : TextDataFlags{}, node);

    ui.update();
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

    CORRADE_BENCHMARK(20)
        ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();

    /* Verify just a few pixels, the TextLayerGL test does the rest. However
       make sure that the whole area is filled, not just a part, to not have
       skewed benchmark results compared to other layers. */
    Image2D out = _framebuffer.read({{}, BenchmarkSize}, {PixelFormat::RGBA8Unorm});
    for(const Vector2i& coordinate: {Vector2i{0, 0},
                                     Vector2i{BenchmarkSize.x() - 1, 0},
                                     Vector2i{0, BenchmarkSize.y() - 1},
                                     BenchmarkSize - Vector2i{1},
                                     BenchmarkSize/2})
    {
        CORRADE_ITERATION(coordinate);
        CORRADE_COMPARE_WITH(
            Math::unpack<Color4>(
                out.pixels<Color4ub>()[std::size_t(coordinate.y())]
                                      [std::size_t(coordinate.x())]),
            0xff3366_rgbf,
            TestSuite::Compare::around(Color4{1.0f/255.0f, 1.0f/255.0f}));
    }
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::TextLayerGLBenchmark)
