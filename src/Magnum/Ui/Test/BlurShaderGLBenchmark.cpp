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

#include <Corrade/Containers/Iterable.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/TestSuite/Compare/Numeric.h>
#include <Magnum/Image.h>
#include <Magnum/ImageView.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/Math/Color.h>
#include <Magnum/DebugTools/CompareImage.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/OpenGLTester.h>
#include <Magnum/GL/TextureFormat.h>

#include "Magnum/Ui/Implementation/BlurShaderGL.h"
#include "Magnum/Ui/Test/BlurShaderGLTest.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct BlurShaderGLBenchmark: GL::OpenGLTester {
    explicit BlurShaderGLBenchmark();

    void setup();
    void teardown();
    void benchmark();
    void benchmarkCustom16Cutoff8();

    private:
        GL::Mesh _square;

        GL::Texture2D _vertical{NoCreate}, _horizontal{NoCreate};
        GL::Framebuffer _verticalFramebuffer{NoCreate}, _horizontalFramebuffer{NoCreate};
};

using namespace Math::Literals;

const struct {
    const char* name;
    UnsignedInt radius;
    Float limit, delta;
} BenchmarkData[]{
    {"radius 16, limit 0", 16, 0.0f, 0.0f},
    {"radius 16, limit 0.5/255", 16, 0.5f/255.0f, 0.1f},
    {"radius 8, limit 0.5/255", 8, 0.5f/255.0f, 0.1f},
    {"radius 4, limit 0", 4, 0.0f, 0.1f},
    {"radius 0, limit 0", 0, 0.0f, 0.0f},
};

BlurShaderGLBenchmark::BlurShaderGLBenchmark() {
    addInstancedBenchmarks({&BlurShaderGLBenchmark::benchmark},
        10, Containers::arraySize(BenchmarkData),
        &BlurShaderGLBenchmark::setup,
        &BlurShaderGLBenchmark::teardown,
        BenchmarkType::GpuTime);

    addInstancedBenchmarks({&BlurShaderGLBenchmark::benchmarkCustom16Cutoff8},
        10, Containers::arraySize(RenderCustom16Cutoff8Data),
        &BlurShaderGLBenchmark::setup,
        &BlurShaderGLBenchmark::teardown,
        BenchmarkType::GpuTime);

    /* The builtin shader asumes Y down, origin top left and takes an extra
       projection scale uniform which then flips it to Y up. The other variants
       in this test don't take a projection scale, so craft the data to have
       both behave the same way with a projection scale being identity. */
    _square
        .setPrimitive(GL::MeshPrimitive::TriangleStrip)
        .setCount(4)
        .addVertexBuffer(GL::Buffer{GL::Buffer::TargetHint::Array, {
            /* 2--3
               |\ |
               | \|
               0--1 */
            Vector2{0.0f, -2.0f},
            Vector2{2.0f, -2.0f},
            Vector2{0.0f,  0.0f},
            Vector2{2.0f,  0.0f},
        }}, 0, BlurShaderGL::Position{});
}

constexpr Vector2i BenchmarkSize{2048, 2048};

void BlurShaderGLBenchmark::setup() {
    _vertical = GL::Texture2D{};
    _vertical
        .setMinificationFilter(GL::SamplerFilter::Linear)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setWrapping(GL::SamplerWrapping::ClampToEdge)
        .setStorage(1, GL::TextureFormat::RGBA8, BenchmarkSize);
    _verticalFramebuffer = GL::Framebuffer{{{}, BenchmarkSize}};
    _verticalFramebuffer
        .attachTexture(GL::Framebuffer::ColorAttachment{0}, _vertical, 0);

    _horizontal = GL::Texture2D{};
    _horizontal
        .setMinificationFilter(GL::SamplerFilter::Linear)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setWrapping(GL::SamplerWrapping::ClampToEdge)
        .setStorage(1, GL::TextureFormat::RGBA8, BenchmarkSize);
    _horizontalFramebuffer = GL::Framebuffer{{{}, BenchmarkSize}};
    _horizontalFramebuffer
        .attachTexture(GL::Framebuffer::ColorAttachment{0}, _horizontal, 0);
}

void BlurShaderGLBenchmark::teardown() {
    _vertical = GL::Texture2D{NoCreate};
    _verticalFramebuffer = GL::Framebuffer{NoCreate};
    _horizontal = GL::Texture2D{NoCreate};
    _horizontalFramebuffer = GL::Framebuffer{NoCreate};
}

void BlurShaderGLBenchmark::benchmark() {
    auto&& data = BenchmarkData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    GL::Texture2D input;
    input
        .setMinificationFilter(GL::SamplerFilter::Linear)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setWrapping(GL::SamplerWrapping::ClampToEdge)
        .setStorage(1, GL::TextureFormat::RGBA8, BenchmarkSize)
        .setSubImage(0, {}, ImageView2D{PixelFormat::RGBA8Unorm, BenchmarkSize, Containers::Array<Color4ub>{DirectInit, std::size_t(BenchmarkSize.product()), 0x336699ff_rgba}});

    BlurShaderGL shader{data.radius, data.limit};
    /* Internally this divides {2, -2}, resulting in an identity to match other
       vertex shaders in this test */
    shader.setProjection({2.0f, -2.0f});

    MAGNUM_VERIFY_NO_GL_ERROR();

    GL::Texture2D* next = &input;

    CORRADE_BENCHMARK(10) {
        /* Vertical */
        _verticalFramebuffer.bind();
        shader
            .setDirection(Vector2::yAxis(1.0f/BenchmarkSize.y()))
            .bindTexture(*next)
            .draw(_square);

        /* Horizontal */
        _horizontalFramebuffer.bind();
        shader
            .setDirection(Vector2::xAxis(1.0f/BenchmarkSize.x()))
            .bindTexture(_vertical)
            .draw(_square);

        next = &_horizontal;
    }

    MAGNUM_VERIFY_NO_GL_ERROR();

    Image2D out = _horizontalFramebuffer.read({{}, BenchmarkSize}, {PixelFormat::RGBA8Unorm});
    CORRADE_COMPARE_WITH(
        Math::unpack<Color4>(
            out.pixels<Color4ub>()[std::size_t(BenchmarkSize.y()/2)]
                                  [std::size_t(BenchmarkSize.x()/2)]),
        0x336699ff_rgbaf,
        TestSuite::Compare::around(Color4{data.delta, data.delta}));
}

void BlurShaderGLBenchmark::benchmarkCustom16Cutoff8() {
    auto&& data = RenderCustom16Cutoff8Data[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    GL::Texture2D input;
    input
        .setMinificationFilter(GL::SamplerFilter::Linear)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setWrapping(GL::SamplerWrapping::ClampToEdge)
        .setStorage(1, GL::TextureFormat::RGBA8, BenchmarkSize)
        .setSubImage(0, {}, ImageView2D{PixelFormat::RGBA8Unorm, BenchmarkSize, Containers::Array<Color4ub>{DirectInit, std::size_t(BenchmarkSize.product()), 0xff336699_rgba}});

    BlurShaderCustomRadius8 shader{data.vert, data.frag};

    MAGNUM_VERIFY_NO_GL_ERROR();

    GL::Texture2D* next = &input;

    CORRADE_BENCHMARK(10) {
        /* Vertical */
        _verticalFramebuffer.bind();
        if(data.integerDirection)
            shader.setDirection(Vector2i::yAxis());
        else
            shader.setDirection(Vector2::yAxis(1.0f/BenchmarkSize.y()));
        shader
            .bindTexture(*next)
            .draw(_square);

        /* Horizontal */
        _horizontalFramebuffer.bind();
        if(data.integerDirection)
            shader.setDirection(Vector2i::xAxis());
        else
            shader.setDirection(Vector2::xAxis(1.0f/BenchmarkSize.x()));
        shader
            .bindTexture(_vertical)
            .draw(_square);

        next = &_horizontal;
    }

    MAGNUM_VERIFY_NO_GL_ERROR();

    Image2D out = _horizontalFramebuffer.read({{}, BenchmarkSize}, {PixelFormat::RGBA8Unorm});
    CORRADE_COMPARE_WITH(
        Math::unpack<Color4>(
            out.pixels<Color4ub>()[std::size_t(BenchmarkSize.y()/2)]
                                  [std::size_t(BenchmarkSize.x()/2)]),
        0xff336699_rgbaf,
        TestSuite::Compare::around(Color4{data.benchmarkEpsilon, data.benchmarkEpsilon}));
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::BlurShaderGLBenchmark)
