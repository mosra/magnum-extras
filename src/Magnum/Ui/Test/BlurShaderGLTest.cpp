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
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/String.h>
#include <Corrade/Containers/StringIterable.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/PluginManager/PluginMetadata.h>
#include <Corrade/TestSuite/Compare/Numeric.h>
#include <Corrade/Utility/Algorithms.h>
#include <Corrade/Utility/Configuration.h>
#include <Corrade/Utility/Path.h>
#include <Corrade/Utility/Resource.h>
#include <Magnum/Image.h>
#include <Magnum/ImageView.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/Math/Color.h>
#include <Magnum/DebugTools/CompareImage.h>
#include <Magnum/GL/Extensions.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/OpenGLTester.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/GL/Version.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/ImageData.h>

#include "Magnum/Ui/Implementation/BlurShaderGL.h"
#include "Magnum/Ui/Test/BlurShaderGLTest.h"

#include "configure.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct BlurShaderGLTest: GL::OpenGLTester {
    explicit BlurShaderGLTest();

    void setup();
    void teardown();
    void render();
    void renderCustom16Cutoff8();

    private:
        GL::Mesh _square;

        PluginManager::Manager<Trade::AbstractImporter> _importerManager;
        GL::Texture2D _vertical{NoCreate}, _horizontal{NoCreate};
        GL::Framebuffer _verticalFramebuffer{NoCreate}, _horizontalFramebuffer{NoCreate};
};

using namespace Math::Literals;

const struct {
    const char* name;
    const char* filename;
    UnsignedInt radius;
    Float limit;
    bool flippedX, flippedY, transparent;
    Float maxThreshold, meanThreshold;
    /* If these are unset, the default gets used */
    struct {
        Float maxThreshold, meanThreshold;
    } llvmpipe21;
} RenderData[]{
    /* This should result in exactly the same image... */
    {"radius 0", "blur-input.png",
        0, 0.0f, false, false, false,
        0.0f, 0.0f, {}},
    /* ... and axis flipping should not add any weird errors to it either */
    {"radius 0, flipped on X", "blur-input.png",
        0, 0.0f, true, false, false,
        0.0f, 0.0f, {}},
    {"radius 0, flipped on Y", "blur-input.png",
        0, 0.0f, false, true, false,
        0.0f, 0.0f, {}},
    {"radius 0, flipped on XY", "blur-input.png",
        0, 0.0f, true, true, false,
        0.0f, 0.0f, {}},
    {"radius 0, transparent", "blur-input.png",
        0, 0.0f, true, true, true,
        0.0f, 0.0f, {}},
    /* This results in 4 discrete taps, so 2 interpolated taps with the first
       tap taking the center pixel twice. Shouldn't cause the image to get any
       brighter. */
    {"radius 3, limit 0", "blur-3.png",
        7, 0.0f, false, false, false,
        /* NVidia & llvmpipe have slight differences, older llvmpipe more */
        0.75f, 0.111f, {1.5f, 0.657f}},
    {"radius 3, limit 0, flipped on X", "blur-3.png",
        7, 0.0f, true, false, false,
        /* NVidia & llvmpipe have slight differences, older llvmpipe more */
        0.75f, 0.111f, {1.5f, 0.657f}},
    {"radius 3, limit 0, flipped on Y", "blur-3.png",
        7, 0.0f, false, true, false,
        /* NVidia & llvmpipe have slight differences, older llvmpipe more */
        0.75f, 0.111f, {1.5f, 0.657f}},
    {"radius 3, limit 0, flipped on XY", "blur-3.png",
        7, 0.0f, true, true, false,
        /* NVidia & llvmpipe have slight differences, older llvmpipe more */
        0.75f, 0.111f, {1.5f, 0.657f}},
    {"radius 3, limit 0, transparent", "blur-3.png",
        7, 0.0f, false, false, true,
        /* NVidia & llvmpipe have slight differences, older llvmpipe more */
        0.75f, 0.111f, {1.5f, 0.657f}},
    /* This results in 17 discrete taps, so 9 interpolated taps with the first
       tap being (non-interpolated) center pixel */
    {"radius 16, limit 0", "blur-16.png",
        16, 0.0f, false, false, false,
        /* NVidia & llvmpipe have slight differences, older llvmpipe more */
        0.75f, 0.091f, {1.25f, 0.557f}},
    /* Same sequence as for "radius 16" above, but with the ends clipped away
       (and then everything scaled accordingly, which is the main contribution
       factor to the difference). The result is almost the same, just with 8
       taps instead of 16 needed. */
    {"radius 16, limit 0.5/255", "blur-16.png",
        16, 0.5f/255.0f, false, false, false,
        /* NVidia & llvmpipe have slight differences */
        2.25f, 1.304f, {}},
    /* Max possible radius value, verify it still compiles & runs correctly */
    {"radius 31, limit 0", "blur-31.png",
        31, 0.0f, false, false, false,
        /* NVidia & llvmpipe have slight differences, older llvmpipe more */
        0.75f, 0.077f, {1.25f, 0.645f}},
};

BlurShaderGLTest::BlurShaderGLTest() {
    addInstancedTests({&BlurShaderGLTest::render},
        Containers::arraySize(RenderData),
        &BlurShaderGLTest::setup,
        &BlurShaderGLTest::teardown);

    addInstancedTests({&BlurShaderGLTest::renderCustom16Cutoff8},
        Containers::arraySize(RenderCustom16Cutoff8Data),
        &BlurShaderGLTest::setup,
        &BlurShaderGLTest::teardown);

    /* Prefer the StbImageImporter so we can keep files small but always import
       them as four-channel */
    if(PluginManager::PluginMetadata* metadata = _importerManager.metadata("StbImageImporter")) {
        metadata->configuration().setValue("forceChannelCount", 4);
        _importerManager.setPreferredPlugins("PngImporter", {"StbImageImporter"});
    }

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

/* Deliberately a non-square and "weird" size to catch accidents */
constexpr Vector2i RenderSize{160, 106};

void BlurShaderGLTest::setup() {
    _vertical = GL::Texture2D{};
    _vertical
        .setMinificationFilter(GL::SamplerFilter::Linear)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setWrapping(GL::SamplerWrapping::ClampToEdge)
        .setStorage(1, GL::TextureFormat::RGBA8, RenderSize);
    _verticalFramebuffer = GL::Framebuffer{{{}, RenderSize}};
    _verticalFramebuffer
        .attachTexture(GL::Framebuffer::ColorAttachment{0}, _vertical, 0);

    _horizontal = GL::Texture2D{};
    _horizontal
        .setMinificationFilter(GL::SamplerFilter::Linear)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setWrapping(GL::SamplerWrapping::ClampToEdge)
        .setStorage(1, GL::TextureFormat::RGBA8, RenderSize);
    _horizontalFramebuffer = GL::Framebuffer{{{}, RenderSize}};
    _horizontalFramebuffer
        .attachTexture(GL::Framebuffer::ColorAttachment{0}, _horizontal, 0);
}

void BlurShaderGLTest::teardown() {
    _vertical = GL::Texture2D{NoCreate};
    _verticalFramebuffer = GL::Framebuffer{NoCreate};
    _horizontal = GL::Texture2D{NoCreate};
    _horizontalFramebuffer = GL::Framebuffer{NoCreate};
}

void BlurShaderGLTest::render() {
    auto&& data = RenderData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    if(!(_importerManager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_importerManager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    Containers::Pointer<Trade::AbstractImporter> importer = _importerManager.loadAndInstantiate("AnyImageImporter");
    CORRADE_VERIFY(importer->openFile(Utility::Path::join(UI_TEST_DIR, "BaseLayerTestFiles/blur-input.png")));

    Containers::Optional<Trade::ImageData2D> image = importer->image2D(0);
    CORRADE_VERIFY(image);
    CORRADE_COMPARE(image->size(), RenderSize);

    if(data.flippedY)
        Utility::flipInPlace<0>(image->mutablePixels());
    if(data.flippedX)
        Utility::flipInPlace<1>(image->mutablePixels());

    /* Make the image transparent if desired -- the blur should ignore that and
       always produce a result with alpha set to 1.0 */
    if(data.transparent)
        for(Containers::StridedArrayView1D<Color4ub> row: image->mutablePixels<Color4ub>())
            for(Color4ub& pixel: row)
                pixel.a() = 0;

    GL::Texture2D input;
    input
        .setMinificationFilter(GL::SamplerFilter::Linear)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setWrapping(GL::SamplerWrapping::ClampToEdge)
        .setStorage(1, GL::textureFormat(image->format()), image->size())
        .setSubImage(0, {}, *image);

    BlurShaderGL shader{data.radius, data.limit};
    /* Internally this divides {2, -2}, resulting in an identity to match other
       vertex shaders in this test */
    shader.setProjection({2.0f, -2.0f});

    MAGNUM_VERIFY_NO_GL_ERROR();

    /* Vertical */
    _verticalFramebuffer.bind();
    shader
        .setDirection(Vector2::yAxis(1.0f/image->size().y()))
        .bindTexture(input)
        .draw(_square);
    MAGNUM_VERIFY_NO_GL_ERROR();

    /* Horizontal */
    _horizontalFramebuffer.bind();
    shader
        .setDirection(Vector2::xAxis(1.0f/image->size().x()))
        .bindTexture(_vertical)
        .draw(_square);
    MAGNUM_VERIFY_NO_GL_ERROR();

    Image2D actual = _horizontalFramebuffer.read({{}, RenderSize}, {PixelFormat::RGBA8Unorm});
    Containers::StridedArrayView2D<const Color4ub> pixels = actual.pixels<Color4ub>();
    if(data.flippedY)
        pixels = pixels.flipped<0>();
    if(data.flippedX)
        pixels = pixels.flipped<1>();

    const bool llvmpipe21Different = data.llvmpipe21.maxThreshold && GL::Context::current().rendererString().contains("llvmpipe") && (GL::Context::current().versionString().contains("Mesa 21") || GL::Context::current().versionString().contains("Mesa 20"));
    {
        CORRADE_EXPECT_FAIL_IF(llvmpipe21Different,
            "Mesa llvmpipe 21 and older has rounding errors resulting in significantly different blur output.");
        CORRADE_COMPARE_WITH(pixels,
            Utility::Path::join({UI_TEST_DIR, "BaseLayerTestFiles", data.filename}),
            (DebugTools::CompareImageToFile{_importerManager, data.maxThreshold, data.meanThreshold}));
    }
    if(llvmpipe21Different)
        CORRADE_COMPARE_WITH(pixels,
            Utility::Path::join({UI_TEST_DIR, "BaseLayerTestFiles", data.filename}),
            (DebugTools::CompareImageToFile{_importerManager, data.llvmpipe21.maxThreshold, data.llvmpipe21.meanThreshold}));

    /* Overal brightness of the blurred image shouldn't be same as of the
       input, i.e. a sum of the convolution weights being 1 */
    Float inputBrightness = 0.0f;
    Float outputBrightness = 0.0f;
    Containers::StridedArrayView2D<const Color4ub> inputPixels = image->pixels<Color4ub>();
    Containers::StridedArrayView2D<const Color4ub> outputPixels = actual.pixels<Color4ub>();
    CORRADE_VERIFY(inputPixels.isContiguous());
    CORRADE_VERIFY(outputPixels.isContiguous());
    Containers::ArrayView<const Color4ub> inputPixelsContiguous = inputPixels.asContiguous();
    Containers::ArrayView<const Color4ub> outputPixelsContiguous = outputPixels.asContiguous();
    CORRADE_COMPARE(outputPixelsContiguous.size(), inputPixelsContiguous.size());
    for(std::size_t i = 0; i != inputPixelsContiguous.size(); ++i) {
        inputBrightness += inputPixelsContiguous[i].value();
        outputBrightness += outputPixelsContiguous[i].value();
    }
    /* Verify the calculated expected brightness is sane for the input size */
    CORRADE_COMPARE_AS(inputBrightness, inputPixelsContiguous.size(),
        TestSuite::Compare::Less);
    /* Allow the blurred image brightness to differ by up to ~2.5% from the
       original, but not more. For older llvmpipe allow bigger difference. */
    CORRADE_COMPARE_WITH(outputBrightness, inputBrightness,
        TestSuite::Compare::around(inputPixelsContiguous.size()*(llvmpipe21Different ? 0.05f : 0.025f)));
}

void BlurShaderGLTest::renderCustom16Cutoff8() {
    auto&& data = RenderCustom16Cutoff8Data[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    if(!(_importerManager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_importerManager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    Containers::Pointer<Trade::AbstractImporter> importer = _importerManager.loadAndInstantiate("AnyImageImporter");
    CORRADE_VERIFY(importer->openFile(Utility::Path::join(UI_TEST_DIR, "BaseLayerTestFiles/blur-input.png")));

    Containers::Optional<Trade::ImageData2D> image = importer->image2D(0);
    CORRADE_VERIFY(image);
    CORRADE_COMPARE(image->size(), RenderSize);

    GL::Texture2D input;
    input
        .setMinificationFilter(GL::SamplerFilter::Linear)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setWrapping(GL::SamplerWrapping::ClampToEdge)
        .setStorage(1, GL::textureFormat(image->format()), image->size())
        .setSubImage(0, {}, *image);

    BlurShaderCustomRadius8 shader{data.vert, data.frag};

    MAGNUM_VERIFY_NO_GL_ERROR();

    /* Vertical */
    _verticalFramebuffer.bind();
    if(data.integerDirection)
        shader.setDirection(Vector2i::yAxis());
    else
        shader.setDirection(Vector2::yAxis(1.0f/image->size().y()));
    shader
        .bindTexture(input)
        .draw(_square);
    MAGNUM_VERIFY_NO_GL_ERROR();

    /* Horizontal */
    _horizontalFramebuffer.bind();
    if(data.integerDirection)
        shader.setDirection(Vector2i::xAxis());
    else
        shader.setDirection(Vector2::xAxis(1.0f/image->size().x()));
    shader
        .bindTexture(_vertical)
        .draw(_square);
    MAGNUM_VERIFY_NO_GL_ERROR();

    CORRADE_COMPARE_WITH(_horizontalFramebuffer.read({{}, RenderSize}, {PixelFormat::RGBA8Unorm}),
        Utility::Path::join(UI_TEST_DIR, "BaseLayerTestFiles/blur-16.png"),
        (DebugTools::CompareImageToFile{_importerManager, data.maxThreshold, data.meanThreshold}));
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::BlurShaderGLTest)
