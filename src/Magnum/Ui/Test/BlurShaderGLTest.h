#ifndef Magnum_Ui_Test_BlurShaderGLTest_h
#define Magnum_Ui_Test_BlurShaderGLTest_h
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

/* Contains common code used by BlurShaderGLTest.cpp and
   BlurShaderGLBenchmark.cpp */

#include <Corrade/Containers/StringView.h>
#include <Corrade/Containers/StringIterable.h>
#include <Corrade/Utility/Resource.h>
#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Extensions.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Version.h>

namespace Magnum { namespace Ui { namespace Test { namespace {

using namespace Math::Literals;

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

            CORRADE_INTERNAL_ASSERT_OUTPUT(vert.compile() && frag.compile());

            attachShaders({vert, frag});
            CORRADE_INTERNAL_ASSERT_OUTPUT(link());

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


}}}}

#endif
