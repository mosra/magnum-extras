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

#ifndef RUNTIME_CONST
#define const
#endif

struct StyleEntry {
    lowp vec4 color;
    highp vec4 widthSmoothnessMiterLimitReserved;
};

layout(std140
    #ifdef EXPLICIT_BINDING
    , binding = 0
    #endif
) uniform Style {
    mediump vec4 smoothnessReserved;
    StyleEntry styles[STYLE_COUNT];
};

#define style_width widthSmoothnessMiterLimitReserved.x
#define style_smoothness widthSmoothnessMiterLimitReserved.y
#define style_miterLimit widthSmoothnessMiterLimitReserved.z
#define commonStyle_smoothness smoothnessReserved.x

#ifdef EXPLICIT_UNIFORM_LOCATION
layout(location = 0)
#endif
uniform highp vec3 projection; /* xy = UI size to unit square scaling,
                                  z = pixel smoothness to UI size scaling */

layout(location = 0) in highp vec2 position;
layout(location = 1) in highp vec2 previousPosition;
layout(location = 2) in highp vec2 nextPosition;
layout(location = 3) in lowp vec4 color;
layout(location = 4) in mediump uint annotationStyle;

NOPERSPECTIVE out highp vec2 centerDistanceSigned;
flat out highp float halfSegmentLength;
NOPERSPECTIVE out highp float hasCap;
flat out mediump uint interpolatedStyle;
NOPERSPECTIVE out lowp vec4 interpolatedColor;

/* Coming from LineShader.in.vert. That file is added after this one in order
   to have the #define const from above affect it as well. */
highp vec2 expandLineVertex(
    in highp const vec2 transformedPosition,
    in highp const vec2 transformedPreviousPosition,
    in highp const vec2 transformedNextPosition,
    in lowp const uint annotation,
    in mediump const float width,
    in mediump const float smoothness,
    in highp const float miterLimit,
    in highp const vec2 viewportSize,
    out highp vec2 centerDistanceSigned,
    out highp float halfSegmentLength,
    out highp float hasCap);

void main() {
    mediump uint annotation = annotationStyle & 0x7u;
    mediump uint style = annotationStyle >> 3;
    mediump const float width = styles[style].style_width;
    /* The common smoothness is in pixels, treated the same way as in BaseLayer
       and TextLayer. The per-style smoothness is in UI units for glows and
       such, and thus doesn't need to be scaled anymore. */
    mediump const float smoothness = max(commonStyle_smoothness*projection.z, styles[style].style_smoothness);
    highp const float miterLimit = styles[style].style_miterLimit;
    interpolatedStyle = style;
    interpolatedColor = styles[style].color*color;

    /* The projection scales from UI size to the 2x2 unit square and Y-flips,
       the (-1, 1) then translates the origin from top left to center */
    highp const vec2 transformedPosition = projection.xy*position + vec2(-1.0, 1.0);
    highp const vec2 transformedPreviousPosition = projection.xy*previousPosition + vec2(-1.0, 1.0);
    highp const vec2 transformedNextPosition = projection.xy*nextPosition + vec2(-1.0, 1.0);

    highp const vec2 pointDirection = expandLineVertex(
        transformedPosition,
        transformedPreviousPosition,
        transformedNextPosition,
        annotation,
        width,
        smoothness,
        miterLimit,
        vec2(2.0, -2.0)/projection.xy,
        centerDistanceSigned,
        halfSegmentLength,
        hasCap);

    gl_Position = vec4(transformedPosition + pointDirection, 0.0, 1.0);
}
