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

NOPERSPECTIVE in highp vec2 centerDistanceSigned;
flat in highp float halfSegmentLength;
NOPERSPECTIVE in highp float hasCap;
flat in mediump uint interpolatedStyle;
NOPERSPECTIVE in lowp vec4 interpolatedColor;

out lowp vec4 fragmentColor;

/* Coming from LineShader.in.frag. That file is added after this one in order
   to have the #define const from above affect it as well. */
mediump float lineBlendFactor(
    in highp vec2 centerDistanceSigned,
    in highp float halfSegmentLength,
    in highp float hasCap,
    in mediump const float width,
    in mediump const float smoothness);

void main() {
    mediump const float width = styles[interpolatedStyle].style_width;
    /* The common smoothness is in pixels, treated the same way as in BaseLayer
       and TextLayer. The per-style smoothness is in UI units for glows and
       such, and thus doesn't need to be scaled anymore. */
    mediump const float smoothness = max(commonStyle_smoothness*projection.z, styles[interpolatedStyle].style_smoothness);

    mediump const float factor = lineBlendFactor(
        centerDistanceSigned,
        halfSegmentLength,
        hasCap,
        width,
        smoothness);

    /* 0 is inside, 1 is outside */
    fragmentColor = interpolatedColor*(1.0 - factor);
}
