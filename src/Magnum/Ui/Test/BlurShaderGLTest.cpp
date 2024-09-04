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

#include "configure.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct BlurShaderGLTest: GL::OpenGLTester {
    explicit BlurShaderGLTest();

    void renderSetup();
    void renderBenchmarkTeardown();
    void render();
    void renderCustom16Cutoff8();

    void benchmarkSetup();
    void benchmark();
    void benchmarkCustom16Cutoff8();

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
    bool flippedX, flippedY;
    Float maxThreshold, meanThreshold;
} RenderData[]{
    /* This should result in exactly the same image... */
    {"radius 0", "blur-input.png",
        0, 0.0f, false, false,
        0.0f, 0.0f},
    /* ... and axis flipping should not add any weird errors to it either */
    {"radius 0, flipped on X", "blur-input.png",
        0, 0.0f, true, false,
        0.0f, 0.0f},
    {"radius 0, flipped on Y", "blur-input.png",
        0, 0.0f, false, true,
        0.0f, 0.0f},
    {"radius 0, flipped on XY", "blur-input.png",
        0, 0.0f, true, true,
        0.0f, 0.0f},
    /* This results in 4 discrete taps, so 2 interpolated taps with the first
       tap taking the center pixel twice. Shouldn't cause the image to get any
       brighter. */
    {"radius 3, limit 0", "blur-3.png",
        7, 0.0f, false, false,
        /* NVidia has slight differences */
        0.75f, 0.044f},
    {"radius 3, limit 0, flipped on X", "blur-3.png",
        7, 0.0f, true, false,
        /* NVidia has slight differences */
        0.75f, 0.044f},
    {"radius 3, limit 0, flipped on Y", "blur-3.png",
        7, 0.0f, false, true,
        /* NVidia has slight differences */
        0.75f, 0.044f},
    {"radius 3, limit 0, flipped on XY", "blur-3.png",
        7, 0.0f, true, true,
        /* NVidia has slight differences */
        0.75f, 0.044f},
    /* This results in 17 discrete taps, so 9 interpolated taps with the first
       tap being (non-interpolated) center pixel */
    {"radius 16, limit 0", "blur-16.png",
        16, 0.0f, false, false,
        /* NVidia has slight differences */
        0.75f, 0.043f},
    /* Same sequence as for "radius 16" above, but with the ends clipped away
       (and then everything scaled accordingly, which is the main contribution
       factor to the difference). The result is almost the same, just with 8
       taps instead of 16 needed. */
    {"radius 16, limit 0.5/255", "blur-16.png",
        16, 0.5f/255.0f, false, false,
        /* NVidia has slight differences */
        2.25f, 1.304f},
    /* Max possible radius value, verify it still compiles & runs correctly */
    {"radius 31, limit 0", "blur-31.png",
        31, 0.0f, false, false,
        /* NVidia has slight differences */
        0.5f, 0.044f},
};

const struct {
    const char* name;
    const char* vert;
    const char* frag;
    bool integerDirection;
    Float maxThreshold, meanThreshold, benchmarkEpsilon;
} RenderCustom16Cutoff8Data[]{
    /* Version of BlurShaderGL before the linearly interpolated weights were
       added */
    {"discrete", "#line " CORRADE_LINE_STRING R"(
layout(location = 0) in highp vec2 position;

out mediump vec2 textureCoordinates;

void main() {
    /* To match what BlurShader.vert would do with identity projection */
    gl_Position = vec4(position + vec2(-1.0, 1.0), 0.0, 1.0);
    textureCoordinates = gl_Position.xy*0.5 + vec2(0.5);
})", "#line " CORRADE_LINE_STRING R"(
#define RADIUS 8
const highp float weights[9] = float[](0.140245, 0.131995, 0.109996, 0.0810496, 0.0526822, 0.0301041, 0.0150521, 0.00654438, 0.00245414);

#ifdef EXPLICIT_UNIFORM_LOCATION
layout(location = 1)
#endif
uniform highp vec2 direction;

#ifdef EXPLICIT_BINDING
layout(binding = 6)
#endif
uniform lowp sampler2D textureData;

in mediump vec2 textureCoordinates;

out lowp vec4 fragmentColor;

void main() {
    fragmentColor = texture(textureData, textureCoordinates)*weights[0];

    for(int i = 1; i < RADIUS; ++i) {
        fragmentColor += texture(textureData, textureCoordinates + float(i)*direction)*weights[i];
        fragmentColor += texture(textureData, textureCoordinates - float(i)*direction)*weights[i];
    }
})", false,
        /* Same as radius 8 in render() */
        2.25f, 1.304f, 0.1f},
    /* Variant of "discrete" that calculates the coefficients on the fly
       instead of pulling them from a table */
    {"discrete, dynamically calculated coefficients", "#line " CORRADE_LINE_STRING R"(
layout(location = 0) in highp vec2 position;

out mediump vec2 textureCoordinates;

void main() {
    /* To match what BlurShader.vert would do with identity projection */
    gl_Position = vec4(position + vec2(-1.0, 1.0), 0.0, 1.0);
    textureCoordinates = gl_Position.xy*0.5 + vec2(0.5);
})", "#line " CORRADE_LINE_STRING R"(
#define RADIUS 8
#define REAL_RADIUS 16

#ifdef EXPLICIT_UNIFORM_LOCATION
layout(location = 1)
#endif
uniform highp vec2 direction;

#ifdef EXPLICIT_BINDING
layout(binding = 6)
#endif
uniform lowp sampler2D textureData;

in mediump vec2 textureCoordinates;

out lowp vec4 fragmentColor;

/* Same equation as the sampled Gaussian in BlurShaderTest, normalization to
   have a sum of 1 is omitted as it's something like 1.000001 for radius 16 */
highp float weight(int i) {
    highp float s = sqrt(float(REAL_RADIUS*2 + 1)/2.0);
    /* 1.77 is sqrt(pi) */
    return 1.0/(s*1.7724538509055159)*exp(-float(i*i)/(s*s));
}

void main() {
    fragmentColor = texture(textureData, textureCoordinates)*weight(0);

    for(int i = 1; i < RADIUS; ++i) {
        highp float weighti = weight(i);
        fragmentColor += texture(textureData, textureCoordinates + float(i)*direction)*weighti;
        fragmentColor += texture(textureData, textureCoordinates - float(i)*direction)*weighti;
    }
})", false,
        /* The sampled Gaussian is slightly different from the binomial
           coefficients that are used in BlurShaderGL */
        3.75f, 2.389f, 0.2f},
    /* Variant of "discrete" that uses texel fetch instead of sampling (so, no
       implicit sample interpolation). In theory could achieve the same perf as
       the interpolated version of the code in BlurShaderGL on hardware that
       performs texture sampling in software. */
    {"discrete, texel fetch", "#line " CORRADE_LINE_STRING R"(
layout(location = 0) in highp vec2 position;

void main() {
    /* To match what BlurShader.vert would do with identity projection */
    gl_Position = vec4(position + vec2(-1.0, 1.0), 0.0, 1.0);
})", "#line " CORRADE_LINE_STRING R"(
#define RADIUS 8
const highp float weights[9] = float[](0.140245, 0.131995, 0.109996, 0.0810496, 0.0526822, 0.0301041, 0.0150521, 0.00654438, 0.00245414);

#ifdef EXPLICIT_UNIFORM_LOCATION
layout(location = 1)
#endif
uniform highp ivec2 direction;

#ifdef EXPLICIT_BINDING
layout(binding = 6)
#endif
uniform lowp sampler2D textureData;

out lowp vec4 fragmentColor;

void main() {
    fragmentColor = texelFetch(textureData, ivec2(gl_FragCoord.xy), 0)*weights[0];
    for(int i = 1; i < RADIUS; ++i) {
        fragmentColor += texelFetch(textureData, clamp(ivec2(gl_FragCoord.xy) + i*direction, ivec2(0), textureSize(textureData, 0) - ivec2(1)), 0)*weights[i];
        fragmentColor += texelFetch(textureData, clamp(ivec2(gl_FragCoord.xy) - i*direction, ivec2(0), textureSize(textureData, 0) - ivec2(1)), 0)*weights[i];
    }
})", true,
        /* Same as radius 8 in render() */
        2.25f, 1.304f, 0.1f},
    /* Variant of "discrete" that doesn't have a loop. Unrolling the loop used
       to be considerably faster on certain GPUs in 2012, but not as much as
       also passing through the texture coordinates from the vertex shader (the
       next case below) */
    {"discrete, unrolled", nullptr, "#line " CORRADE_LINE_STRING R"(
#ifdef EXPLICIT_UNIFORM_LOCATION
layout(location = 1)
#endif
uniform highp vec2 direction;

#ifdef EXPLICIT_BINDING
layout(binding = 6)
#endif
uniform lowp sampler2D textureData;

in mediump vec2 textureCoordinates;

out lowp vec4 fragmentColor;

void main() {
    fragmentColor =      texture(textureData, textureCoordinates)*0.140245 +

        texture(textureData, textureCoordinates + 1.0*direction)*0.131995 +
        texture(textureData, textureCoordinates + 2.0*direction)*0.109996 +
        texture(textureData, textureCoordinates + 3.0*direction)*0.0810496 +
        texture(textureData, textureCoordinates + 4.0*direction)*0.0526822 +
        texture(textureData, textureCoordinates + 5.0*direction)*0.0301041 +
        texture(textureData, textureCoordinates + 6.0*direction)*0.0150521 +
        texture(textureData, textureCoordinates + 7.0*direction)*0.00654438 +
        texture(textureData, textureCoordinates + 8.0*direction)*0.00245414 +

        texture(textureData, textureCoordinates - 1.0*direction)*0.131995 +
        texture(textureData, textureCoordinates - 2.0*direction)*0.109996 +
        texture(textureData, textureCoordinates - 3.0*direction)*0.0810496 +
        texture(textureData, textureCoordinates - 4.0*direction)*0.0526822 +
        texture(textureData, textureCoordinates - 5.0*direction)*0.0301041 +
        texture(textureData, textureCoordinates - 6.0*direction)*0.0150521 +
        texture(textureData, textureCoordinates - 7.0*direction)*0.00654438 +
        texture(textureData, textureCoordinates - 8.0*direction)*0.00245414;
})", false,
        /* Same as radius 8 in render() */
        2.25f, 1.304f, 0.1f},
    /* Variant of the above unrolled case together with calculating the
       coordinates in the vertex shader in order to avoid "dependent texture
       reads" in the fragment shader used to be considerably faster in 2012 */
    {"discrete, coordinate passthrough, unrolled", "#line " CORRADE_LINE_STRING R"(
#ifdef EXPLICIT_UNIFORM_LOCATION
layout(location = 1)
#endif
uniform highp vec2 direction;

layout(location = 0) in highp vec2 position;

out mediump vec4 textureCoordinates[9];

void main() {
    /* To match what BlurShader.vert would do with identity projection */
    gl_Position = vec4(position + vec2(-1.0, 1.0), 0.0, 1.0);
    mediump vec2 baseTextureCoordinates = gl_Position.xy*0.5 + vec2(0.5);
    for(int i = 0; i != 9; ++i) {
        textureCoordinates[i].xy = baseTextureCoordinates + float(i)*direction;
        textureCoordinates[i].zw = baseTextureCoordinates - float(i)*direction;
    }
})", "#line " CORRADE_LINE_STRING R"(
#ifdef EXPLICIT_BINDING
layout(binding = 6)
#endif
uniform lowp sampler2D textureData;

in mediump vec4 textureCoordinates[9];

out lowp vec4 fragmentColor;

void main() {
    fragmentColor =
        texture(textureData, textureCoordinates[0].xy)*0.140245 +

        texture(textureData, textureCoordinates[1].xy)*0.131995 +
        texture(textureData, textureCoordinates[2].xy)*0.109996 +
        texture(textureData, textureCoordinates[3].xy)*0.0810496 +
        texture(textureData, textureCoordinates[4].xy)*0.0526822 +
        texture(textureData, textureCoordinates[5].xy)*0.0301041 +
        texture(textureData, textureCoordinates[6].xy)*0.0150521 +
        texture(textureData, textureCoordinates[7].xy)*0.00654438 +
        texture(textureData, textureCoordinates[8].xy)*0.00245414 +

        texture(textureData, textureCoordinates[1].zw)*0.131995 +
        texture(textureData, textureCoordinates[2].zw)*0.109996 +
        texture(textureData, textureCoordinates[3].zw)*0.0810496 +
        texture(textureData, textureCoordinates[4].zw)*0.0526822 +
        texture(textureData, textureCoordinates[5].zw)*0.0301041 +
        texture(textureData, textureCoordinates[6].zw)*0.0150521 +
        texture(textureData, textureCoordinates[7].zw)*0.00654438 +
        texture(textureData, textureCoordinates[8].zw)*0.00245414;
})", false,
        /* Same as radius 8 in render() */
        2.25f, 1.304f, 0.1f},
};

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

BlurShaderGLTest::BlurShaderGLTest() {
    addInstancedTests({&BlurShaderGLTest::render},
        Containers::arraySize(RenderData),
        &BlurShaderGLTest::renderSetup,
        &BlurShaderGLTest::renderBenchmarkTeardown);

    addInstancedTests({&BlurShaderGLTest::renderCustom16Cutoff8},
        Containers::arraySize(RenderCustom16Cutoff8Data),
        &BlurShaderGLTest::renderSetup,
        &BlurShaderGLTest::renderBenchmarkTeardown);

    addInstancedBenchmarks({&BlurShaderGLTest::benchmark},
        10, Containers::arraySize(BenchmarkData),
        &BlurShaderGLTest::benchmarkSetup,
        &BlurShaderGLTest::renderBenchmarkTeardown,
        BenchmarkType::GpuTime);

    addInstancedBenchmarks({&BlurShaderGLTest::benchmarkCustom16Cutoff8},
        10, Containers::arraySize(RenderCustom16Cutoff8Data),
        &BlurShaderGLTest::benchmarkSetup,
        &BlurShaderGLTest::renderBenchmarkTeardown,
        BenchmarkType::GpuTime);

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

void BlurShaderGLTest::renderSetup() {
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

void BlurShaderGLTest::renderBenchmarkTeardown() {
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

    CORRADE_COMPARE_WITH(pixels,
        Utility::Path::join({UI_TEST_DIR, "BaseLayerTestFiles", data.filename}),
        (DebugTools::CompareImageToFile{_importerManager, data.maxThreshold, data.meanThreshold}));

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
       original, but not more */
    CORRADE_COMPARE_WITH(outputBrightness, inputBrightness,
        TestSuite::Compare::around(inputPixelsContiguous.size()*0.025f));
}

class BlurShaderCustomRadius8: public GL::AbstractShaderProgram {
    public:
        enum: Int {
            TextureBinding = 6
        };

        explicit BlurShaderCustomRadius8(const char* vertSource, const char* fragSource) {
            using namespace Containers::Literals;

            GL::Context& context = GL::Context::current();
            #ifndef MAGNUM_TARGET_GLES
            MAGNUM_ASSERT_GL_EXTENSION_SUPPORTED(GL::Extensions::ARB::explicit_attrib_location);
            #endif

            Utility::Resource rs{"MagnumUi"_s};

            const GL::Version version = context.supportedVersion({
                #ifndef MAGNUM_TARGET_GLES
                GL::Version::GL330
                #else
                GL::Version::GLES300
                    #ifndef MAGNUM_TARGET_WEBGL
                    , GL::Version::GLES310
                    #endif
                #endif
            });

            GL::Shader vert{version, GL::Shader::Type::Vertex};
            vert.addSource(rs.getString("compatibility.glsl"_s))
                .addSource(vertSource ? vertSource : rs.getString("BlurShader.vert"));

            GL::Shader frag{version, GL::Shader::Type::Fragment};
            frag.addSource(rs.getString("compatibility.glsl"_s))
                .addSource(fragSource);

            CORRADE_INTERNAL_ASSERT(vert.compile() && frag.compile());

            attachShaders({vert, frag});
            CORRADE_INTERNAL_ASSERT(link());

            #ifndef MAGNUM_TARGET_GLES
            if(!context.isExtensionSupported<GL::Extensions::ARB::explicit_uniform_location>())
            #elif !defined(MAGNUM_TARGET_GLES2) && !defined(MAGNUM_TARGET_WEBGL)
            if(version < GL::Version::GLES310)
            #endif
            {
                _directionUniform = uniformLocation("direction"_s);
            }

            #ifndef MAGNUM_TARGET_GLES
            if(!context.isExtensionSupported<GL::Extensions::ARB::shading_language_420pack>())
            #elif !defined(MAGNUM_TARGET_GLES2) && !defined(MAGNUM_TARGET_WEBGL)
            if(version < GL::Version::GLES310)
            #endif
            {
                setUniform(uniformLocation("textureData"_s), TextureBinding);
            }

            /* The builtin vertex shader has an extra projection uniform, the
               others not. Set it to an identity value to match the others. */
            if(!vertSource)
                setUniform(uniformLocation("projection"_s), Vector2{1.0f});
        }

        BlurShaderCustomRadius8& setDirection(const Vector2& direction) {
            setUniform(_directionUniform, direction);
            return *this;
        }

        BlurShaderCustomRadius8& setDirection(const Vector2i& direction) {
            setUniform(_directionUniform, direction);
            return *this;
        }

        BlurShaderCustomRadius8& bindTexture(GL::Texture2D& texture) {
            texture.bind(TextureBinding);
            return *this;
        }

    private:
        Int _directionUniform = 1;
};

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

constexpr Vector2i BenchmarkSize{2048, 2048};

void BlurShaderGLTest::benchmarkSetup() {
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

void BlurShaderGLTest::benchmark() {
    auto&& data = BenchmarkData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    GL::Texture2D input;
    input
        .setMinificationFilter(GL::SamplerFilter::Linear)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setWrapping(GL::SamplerWrapping::ClampToEdge)
        .setStorage(1, GL::TextureFormat::RGBA8, BenchmarkSize)
        .setSubImage(0, {}, ImageView2D{PixelFormat::RGBA8Unorm, BenchmarkSize, Containers::Array<Color4ub>{DirectInit, std::size_t(BenchmarkSize.product()), 0xff336699_rgba}});

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
        0xff336699_rgbaf,
        TestSuite::Compare::around(Color4{data.delta, data.delta}));
}

void BlurShaderGLTest::benchmarkCustom16Cutoff8() {
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

CORRADE_TEST_MAIN(Magnum::Ui::Test::BlurShaderGLTest)
