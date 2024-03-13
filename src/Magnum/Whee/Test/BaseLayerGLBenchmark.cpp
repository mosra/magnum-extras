/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023 Vladimír Vondruš <mosra@centrum.cz>

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
#include <Magnum/Image.h>
#include <Magnum/ImageView.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/OpenGLTester.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h>

#include "Magnum/Whee/AbstractUserInterface.h"
#include "Magnum/Whee/BaseLayerGL.h"
#include "Magnum/Whee/RendererGL.h"

namespace Magnum { namespace Whee { namespace Test { namespace {

struct BaseLayerGLBenchmark: GL::OpenGLTester {
    explicit BaseLayerGLBenchmark();

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
    BaseLayerGL::Shared::Flags flags;
} FragmentData[]{
    {"", {}},
    {"no rounded corners",
        BaseLayerGL::Shared::Flag::NoRoundedCorners},
    {"no outline",
        BaseLayerGL::Shared::Flag::NoOutline},
    {"no rounded corners or outline",
        BaseLayerGL::Shared::Flag::NoRoundedCorners|
        BaseLayerGL::Shared::Flag::NoOutline},
};

BaseLayerGLBenchmark::BaseLayerGLBenchmark() {
    addInstancedBenchmarks({&BaseLayerGLBenchmark::fragment}, 10,
        Containers::arraySize(FragmentData),
        &BaseLayerGLBenchmark::setup,
        &BaseLayerGLBenchmark::teardown,
        BenchmarkType::GpuTime);
}

constexpr Vector2i BenchmarkSize{2048, 2048};

void BaseLayerGLBenchmark::setup() {
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

void BaseLayerGLBenchmark::teardown() {
    _framebuffer = GL::Framebuffer{NoCreate};
    _color = GL::Texture2D{NoCreate};

    GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::disable(GL::Renderer::Feature::Blending);
}

void BaseLayerGLBenchmark::fragment() {
    auto&& data = FragmentData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Renders a single data over the whole size to benchmark mainly the
       fragment shader invocation */

    AbstractUserInterface ui{BenchmarkSize};
    ui.setRendererInstance(Containers::pointer<RendererGL>());

    BaseLayerGL::Shared shared{BaseLayer::Shared::Configuration{1}
        .setFlags(data.flags)
    };
    shared.setStyle(BaseLayerCommonStyleUniform{}, {
        BaseLayerStyleUniform{}
            .setColor(0xff3366_rgbf)
    }, {});

    BaseLayerGL& layer = ui.setLayerInstance(Containers::pointer<BaseLayerGL>(ui.createLayer(), shared));

    NodeHandle node = ui.createNode({}, Vector2{BenchmarkSize});
    layer.create(0, node);

    ui.update();
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

    CORRADE_BENCHMARK(20)
        ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();

    /* Verify just one pixel, the BaseLayerGLTest does the rest */
    Image2D out = _framebuffer.read({{}, BenchmarkSize}, {PixelFormat::RGBA8Unorm});
    CORRADE_COMPARE_WITH(
        Math::unpack<Color4>(
            out.pixels<Color4ub>()[std::size_t(BenchmarkSize.y()/2)]
                                  [std::size_t(BenchmarkSize.x()/2)]),
        0xff3366_rgbf,
        TestSuite::Compare::around(Color4{1.0f/255.0f, 1.0f/255.0f}));
}

}}}}

CORRADE_TEST_MAIN(Magnum::Whee::Test::BaseLayerGLBenchmark)
